#include "TextureAuthoring/TextureAuthoringComponent.h"

#include "Async/Async.h"
#include "Async/ParallelFor.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/VolumeTexture.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#endif

UTextureAuthoringComponent::UTextureAuthoringComponent()
{
}

void UTextureAuthoringComponent::GenerateNoiseTextures()
{
	if (bIsGenerating)
	{
		return;
	}
	bIsGenerating = true;

	Async(EAsyncExecution::ThreadPool, [this]()
	{
		GenerateBaseShapeNoise();
		GenerateDetailNoise();
	},
	[this]()
	{
		// UObject/package creation is not thread-safe: SaveVolumeTextureAsset must run on the game thread.
		AsyncTask(ENamedThreads::GameThread, [this]()
		{
#if WITH_EDITOR
			SaveVolumeTextureAsset(BaseShapeTexels, BaseShapeTextureSize, TEXT("T_CloudBaseShape"));
			SaveVolumeTextureAsset(DetailTexels, DetailTextureSize, TEXT("T_CloudDetail"));
#endif
			bIsGenerating = false;
			UE_LOG(LogTemp, Log, TEXT("TextureAuthoring: noise generation and save complete."));
		});
	});
}

#if WITH_EDITOR
void UTextureAuthoringComponent::SaveVolumeTextureAsset(const TArray<uint8>& RgbaTexels, int32 Size, const FString& AssetName)
{
	if (RgbaTexels.Num() != Size * Size * Size * 4)
	{
		UE_LOG(LogTemp, Warning, TEXT("TextureAuthoring: %s texel buffer size mismatch, skipping save."), *AssetName);
		return;
	}

	// FTextureSource::Init expects TSF_BGRA8 byte order (B,G,R,A) — TSF_RGBA8 is deprecated in this engine
	// version and auto-converts to TSF_BGRA8 on load anyway. Our generation buffers are R,G,B,A; swap here
	// rather than changing the generation code, so channel names there still match the paper's own naming.
	TArray<uint8> BgraTexels;
	BgraTexels.SetNumUninitialized(RgbaTexels.Num());
	for (int32 i = 0; i < RgbaTexels.Num(); i += 4)
	{
		BgraTexels[i + 0] = RgbaTexels[i + 2]; // B <- R
		BgraTexels[i + 1] = RgbaTexels[i + 1]; // G <- G
		BgraTexels[i + 2] = RgbaTexels[i + 0]; // R <- B
		BgraTexels[i + 3] = RgbaTexels[i + 3]; // A <- A
	}

	const FString PackageName = OutputPackagePath + AssetName;
	UPackage* Package = CreatePackage(*PackageName);

	UVolumeTexture* NewTexture = NewObject<UVolumeTexture>(Package, FName(*AssetName), RF_Public | RF_Standalone);

	NewTexture->PreEditChange(nullptr);
	NewTexture->Source.Init(Size, Size, Size, 1, ETextureSourceFormat::TSF_BGRA8, BgraTexels.GetData());
	NewTexture->CompressionSettings = TC_Default;
	NewTexture->SRGB = false;
	NewTexture->MipGenSettings = TMGS_FromTextureGroup;
	NewTexture->PostEditChange();

	FAssetRegistryModule::AssetCreated(NewTexture);
	Package->MarkPackageDirty();

	const FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	const bool bSaved = UPackage::SavePackage(Package, NewTexture, *PackageFileName, SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("TextureAuthoring: %s -> %s (%s)"), *AssetName, *PackageFileName, bSaved ? TEXT("saved") : TEXT("FAILED"));
}
#endif

namespace
{
	// Small GLSL-semantics vec4 helpers used by PeriodicPerlin4D. Kept local to this file so the port
	// matches GLM's per-component operations exactly rather than relying on FVector4f operator behavior.

	FVector4f Floor4(const FVector4f& V)
	{
		return FVector4f(FMath::FloorToFloat(V.X), FMath::FloorToFloat(V.Y), FMath::FloorToFloat(V.Z), FMath::FloorToFloat(V.W));
	}

	FVector4f Fract4(const FVector4f& V)
	{
		return V - Floor4(V);
	}

	float GlslMod(float X, float Y)
	{
		return X - Y * FMath::FloorToFloat(X / Y);
	}

	FVector4f Mod4(const FVector4f& V, const FVector4f& Y)
	{
		return FVector4f(GlslMod(V.X, Y.X), GlslMod(V.Y, Y.Y), GlslMod(V.Z, Y.Z), GlslMod(V.W, Y.W));
	}

	FVector4f Abs4(const FVector4f& V)
	{
		return FVector4f(FMath::Abs(V.X), FMath::Abs(V.Y), FMath::Abs(V.Z), FMath::Abs(V.W));
	}

	// GLSL step(edge, x): 0 if x < edge, else 1.
	FVector4f Step4(const FVector4f& Edge, const FVector4f& X)
	{
		return FVector4f(
			X.X < Edge.X ? 0.f : 1.f,
			X.Y < Edge.Y ? 0.f : 1.f,
			X.Z < Edge.Z ? 0.f : 1.f,
			X.W < Edge.W ? 0.f : 1.f);
	}

	FVector4f Mix4(const FVector4f& A, const FVector4f& B, float T)
	{
		return A + (B - A) * T;
	}

	float Dot4(const FVector4f& A, const FVector4f& B)
	{
		return A.X * B.X + A.Y * B.Y + A.Z * B.Z + A.W * B.W;
	}

	float Mod289(float X)
	{
		return X - FMath::FloorToFloat(X * (1.0f / 289.0f)) * 289.0f;
	}

	FVector4f Permute4(const FVector4f& X)
	{
		const FVector4f Y = (X * 34.0f + FVector4f(1.f, 1.f, 1.f, 1.f)) * X;
		return FVector4f(Mod289(Y.X), Mod289(Y.Y), Mod289(Y.Z), Mod289(Y.W));
	}

	FVector4f TaylorInvSqrt4(const FVector4f& R)
	{
		return FVector4f(1.79284291400159f, 1.79284291400159f, 1.79284291400159f, 1.79284291400159f) - R * 0.85373472095314f;
	}

	FVector4f Fade4(const FVector4f& T)
	{
		return (T * T * T) * (T * (T * 6.0f - FVector4f(15.f, 15.f, 15.f, 15.f)) + FVector4f(10.f, 10.f, 10.f, 10.f));
	}
}

float UTextureAuthoringComponent::Hash(float N)
{
	return FMath::Frac(FMath::Sin(N + 1.951f) * 43758.5453f);
}

// hash based 3d value noise
float UTextureAuthoringComponent::ValueNoise(const FVector3f& X)
{
	const FVector3f P(FMath::FloorToFloat(X.X), FMath::FloorToFloat(X.Y), FMath::FloorToFloat(X.Z));
	FVector3f F = X - P;

	F = FVector3f(F.X * F.X * (3.0f - 2.0f * F.X), F.Y * F.Y * (3.0f - 2.0f * F.Y), F.Z * F.Z * (3.0f - 2.0f * F.Z));
	const float N = P.X + P.Y * 57.0f + 113.0f * P.Z;
	return FMath::Lerp(
		FMath::Lerp(
			FMath::Lerp(Hash(N + 0.0f), Hash(N + 1.0f), F.X),
			FMath::Lerp(Hash(N + 57.0f), Hash(N + 58.0f), F.X),
			F.Y),
		FMath::Lerp(
			FMath::Lerp(Hash(N + 113.0f), Hash(N + 114.0f), F.X),
			FMath::Lerp(Hash(N + 170.0f), Hash(N + 171.0f), F.X),
			F.Y),
		F.Z);
}

float UTextureAuthoringComponent::Cells(const FVector3f& Position, float CellCount)
{
	const FVector3f PCell = Position * CellCount;
	float D = 1.0e10f;
	for (int32 Xo = -1; Xo <= 1; Xo++)
	{
		for (int32 Yo = -1; Yo <= 1; Yo++)
		{
			for (int32 Zo = -1; Zo <= 1; Zo++)
			{
				FVector3f Tp = FVector3f(
					FMath::FloorToFloat(PCell.X) + Xo,
					FMath::FloorToFloat(PCell.Y) + Yo,
					FMath::FloorToFloat(PCell.Z) + Zo);

				const FVector3f Wrapped(GlslMod(Tp.X, CellCount), GlslMod(Tp.Y, CellCount), GlslMod(Tp.Z, CellCount));
				Tp = PCell - Tp - FVector3f(ValueNoise(Wrapped));

				D = FMath::Min(D, Tp.Dot(Tp));
			}
		}
	}
	D = FMath::Min(D, 1.0f);
	D = FMath::Max(D, 0.0f);
	return D;
}

float UTextureAuthoringComponent::WorleyNoise(const FVector3f& Position, float CellCount)
{
	return Cells(Position, CellCount);
}

float UTextureAuthoringComponent::PeriodicPerlin4D(const FVector4f& Position, const FVector4f& Rep)
{
	const FVector4f Pi0 = Mod4(Floor4(Position), Rep);
	const FVector4f Pi1 = Mod4(Pi0 + FVector4f(1.f, 1.f, 1.f, 1.f), Rep);
	const FVector4f Pf0 = Fract4(Position);
	const FVector4f Pf1 = Pf0 - FVector4f(1.f, 1.f, 1.f, 1.f);

	const FVector4f Ix(Pi0.X, Pi1.X, Pi0.X, Pi1.X);
	const FVector4f Iy(Pi0.Y, Pi0.Y, Pi1.Y, Pi1.Y);
	const FVector4f Iz0(Pi0.Z, Pi0.Z, Pi0.Z, Pi0.Z);
	const FVector4f Iz1(Pi1.Z, Pi1.Z, Pi1.Z, Pi1.Z);
	const FVector4f Iw0(Pi0.W, Pi0.W, Pi0.W, Pi0.W);
	const FVector4f Iw1(Pi1.W, Pi1.W, Pi1.W, Pi1.W);

	const FVector4f Ixy = Permute4(Permute4(Ix) + Iy);
	const FVector4f Ixy0 = Permute4(Ixy + Iz0);
	const FVector4f Ixy1 = Permute4(Ixy + Iz1);
	const FVector4f Ixy00 = Permute4(Ixy0 + Iw0);
	const FVector4f Ixy01 = Permute4(Ixy0 + Iw1);
	const FVector4f Ixy10 = Permute4(Ixy1 + Iw0);
	const FVector4f Ixy11 = Permute4(Ixy1 + Iw1);

	auto Grad = [](const FVector4f& Ixy_, FVector4f& OutGx, FVector4f& OutGy, FVector4f& OutGz, FVector4f& OutGw)
	{
		FVector4f Gx = Ixy_ / 7.0f;
		FVector4f Gy = Floor4(Gx) / 7.0f;
		FVector4f Gz = Floor4(Gy) / 6.0f;
		Gx = Fract4(Gx) - FVector4f(0.5f, 0.5f, 0.5f, 0.5f);
		Gy = Fract4(Gy) - FVector4f(0.5f, 0.5f, 0.5f, 0.5f);
		Gz = Fract4(Gz) - FVector4f(0.5f, 0.5f, 0.5f, 0.5f);
		const FVector4f Gw = FVector4f(0.75f, 0.75f, 0.75f, 0.75f) - Abs4(Gx) - Abs4(Gy) - Abs4(Gz);
		const FVector4f Sw = Step4(Gw, FVector4f(0.f, 0.f, 0.f, 0.f));
		Gx = Gx - Sw * (Step4(FVector4f(0.f, 0.f, 0.f, 0.f), Gx) - FVector4f(0.5f, 0.5f, 0.5f, 0.5f));
		Gy = Gy - Sw * (Step4(FVector4f(0.f, 0.f, 0.f, 0.f), Gy) - FVector4f(0.5f, 0.5f, 0.5f, 0.5f));
		OutGx = Gx;
		OutGy = Gy;
		OutGz = Gz;
		OutGw = Gw;
	};

	FVector4f Gx00, Gy00, Gz00, Gw00;
	Grad(Ixy00, Gx00, Gy00, Gz00, Gw00);
	FVector4f Gx01, Gy01, Gz01, Gw01;
	Grad(Ixy01, Gx01, Gy01, Gz01, Gw01);
	FVector4f Gx10, Gy10, Gz10, Gw10;
	Grad(Ixy10, Gx10, Gy10, Gz10, Gw10);
	FVector4f Gx11, Gy11, Gz11, Gw11;
	Grad(Ixy11, Gx11, Gy11, Gz11, Gw11);

	FVector4f G0000(Gx00.X, Gy00.X, Gz00.X, Gw00.X);
	FVector4f G1000(Gx00.Y, Gy00.Y, Gz00.Y, Gw00.Y);
	FVector4f G0100(Gx00.Z, Gy00.Z, Gz00.Z, Gw00.Z);
	FVector4f G1100(Gx00.W, Gy00.W, Gz00.W, Gw00.W);
	FVector4f G0010(Gx10.X, Gy10.X, Gz10.X, Gw10.X);
	FVector4f G1010(Gx10.Y, Gy10.Y, Gz10.Y, Gw10.Y);
	FVector4f G0110(Gx10.Z, Gy10.Z, Gz10.Z, Gw10.Z);
	FVector4f G1110(Gx10.W, Gy10.W, Gz10.W, Gw10.W);
	FVector4f G0001(Gx01.X, Gy01.X, Gz01.X, Gw01.X);
	FVector4f G1001(Gx01.Y, Gy01.Y, Gz01.Y, Gw01.Y);
	FVector4f G0101(Gx01.Z, Gy01.Z, Gz01.Z, Gw01.Z);
	FVector4f G1101(Gx01.W, Gy01.W, Gz01.W, Gw01.W);
	FVector4f G0011(Gx11.X, Gy11.X, Gz11.X, Gw11.X);
	FVector4f G1011(Gx11.Y, Gy11.Y, Gz11.Y, Gw11.Y);
	FVector4f G0111(Gx11.Z, Gy11.Z, Gz11.Z, Gw11.Z);
	FVector4f G1111(Gx11.W, Gy11.W, Gz11.W, Gw11.W);

	const FVector4f Norm00 = TaylorInvSqrt4(FVector4f(Dot4(G0000, G0000), Dot4(G0100, G0100), Dot4(G1000, G1000), Dot4(G1100, G1100)));
	G0000 *= Norm00.X;
	G0100 *= Norm00.Y;
	G1000 *= Norm00.Z;
	G1100 *= Norm00.W;

	const FVector4f Norm01 = TaylorInvSqrt4(FVector4f(Dot4(G0001, G0001), Dot4(G0101, G0101), Dot4(G1001, G1001), Dot4(G1101, G1101)));
	G0001 *= Norm01.X;
	G0101 *= Norm01.Y;
	G1001 *= Norm01.Z;
	G1101 *= Norm01.W;

	const FVector4f Norm10 = TaylorInvSqrt4(FVector4f(Dot4(G0010, G0010), Dot4(G0110, G0110), Dot4(G1010, G1010), Dot4(G1110, G1110)));
	G0010 *= Norm10.X;
	G0110 *= Norm10.Y;
	G1010 *= Norm10.Z;
	G1110 *= Norm10.W;

	const FVector4f Norm11 = TaylorInvSqrt4(FVector4f(Dot4(G0011, G0011), Dot4(G0111, G0111), Dot4(G1011, G1011), Dot4(G1111, G1111)));
	G0011 *= Norm11.X;
	G0111 *= Norm11.Y;
	G1011 *= Norm11.Z;
	G1111 *= Norm11.W;

	const float N0000 = Dot4(G0000, Pf0);
	const float N1000 = Dot4(G1000, FVector4f(Pf1.X, Pf0.Y, Pf0.Z, Pf0.W));
	const float N0100 = Dot4(G0100, FVector4f(Pf0.X, Pf1.Y, Pf0.Z, Pf0.W));
	const float N1100 = Dot4(G1100, FVector4f(Pf1.X, Pf1.Y, Pf0.Z, Pf0.W));
	const float N0010 = Dot4(G0010, FVector4f(Pf0.X, Pf0.Y, Pf1.Z, Pf0.W));
	const float N1010 = Dot4(G1010, FVector4f(Pf1.X, Pf0.Y, Pf1.Z, Pf0.W));
	const float N0110 = Dot4(G0110, FVector4f(Pf0.X, Pf1.Y, Pf1.Z, Pf0.W));
	const float N1110 = Dot4(G1110, FVector4f(Pf1.X, Pf1.Y, Pf1.Z, Pf0.W));
	const float N0001 = Dot4(G0001, FVector4f(Pf0.X, Pf0.Y, Pf0.Z, Pf1.W));
	const float N1001 = Dot4(G1001, FVector4f(Pf1.X, Pf0.Y, Pf0.Z, Pf1.W));
	const float N0101 = Dot4(G0101, FVector4f(Pf0.X, Pf1.Y, Pf0.Z, Pf1.W));
	const float N1101 = Dot4(G1101, FVector4f(Pf1.X, Pf1.Y, Pf0.Z, Pf1.W));
	const float N0011 = Dot4(G0011, FVector4f(Pf0.X, Pf0.Y, Pf1.Z, Pf1.W));
	const float N1011 = Dot4(G1011, FVector4f(Pf1.X, Pf0.Y, Pf1.Z, Pf1.W));
	const float N0111 = Dot4(G0111, FVector4f(Pf0.X, Pf1.Y, Pf1.Z, Pf1.W));
	const float N1111 = Dot4(G1111, Pf1);

	const FVector4f FadeXyzw = Fade4(Pf0);
	const FVector4f N0w = Mix4(FVector4f(N0000, N1000, N0100, N1100), FVector4f(N0001, N1001, N0101, N1101), FadeXyzw.W);
	const FVector4f N1w = Mix4(FVector4f(N0010, N1010, N0110, N1110), FVector4f(N0011, N1011, N0111, N1111), FadeXyzw.W);
	const FVector4f Nzw = Mix4(N0w, N1w, FadeXyzw.Z);
	const FVector2f Nyzw = FVector2f(FMath::Lerp(Nzw.X, Nzw.Z, FadeXyzw.Y), FMath::Lerp(Nzw.Y, Nzw.W, FadeXyzw.Y));
	const float Nxyzw = FMath::Lerp(Nyzw.X, Nyzw.Y, FadeXyzw.X);
	return 2.2f * Nxyzw;
}

float UTextureAuthoringComponent::PerlinNoise(const FVector3f& Position, float Frequency, int32 OctaveCount)
{
	const float OctaveFrequencyFactor = 2.0f; // noise frequency factor between octaves, forced to 2

	float Sum = 0.0f;
	float WeightSum = 0.0f;
	float Weight = 0.5f;
	for (int32 Oct = 0; Oct < OctaveCount; Oct++)
	{
		const FVector4f P = FVector4f(Position.X, Position.Y, Position.Z, 0.0f) * Frequency;
		const float Val = PeriodicPerlin4D(P, FVector4f(Frequency, Frequency, Frequency, Frequency));

		Sum += Val * Weight;
		WeightSum += Weight;

		Weight *= Weight;
		Frequency *= OctaveFrequencyFactor;
	}

	float Noise = (Sum / WeightSum) * 0.5f + 0.5f;
	Noise = FMath::Min(Noise, 1.0f);
	Noise = FMath::Max(Noise, 0.0f);
	return Noise;
}

void UTextureAuthoringComponent::GenerateBaseShapeNoise()
{
	const int32 Size = BaseShapeTextureSize;
	BaseShapeTexels.SetNumUninitialized(Size * Size * Size * 4);

	// special weight for Perlin-Worley, matches TileableVolumeNoise main.cpp
	const float FrequencyMul[6] = { 2.0f, 8.0f, 14.0f, 20.0f, 26.0f, 32.0f };

	ParallelFor(Size, [&](int32 S)
	{
		const FVector3f NormFact(1.0f / float(Size));
		for (int32 T = 0; T < Size; T++)
		{
			for (int32 R = 0; R < Size; R++)
			{
				const FVector3f Coord = FVector3f(S, T, R) * NormFact;

				// Perlin FBM noise
				const int32 OctaveCount = 3;
				const float Frequency = 8.0f;
				const float PerlinNoiseValue = PerlinNoise(Coord, Frequency, OctaveCount);

				float PerlinWorleyNoise = 0.0f;
				{
					const float CellCount = 4;
					const float WorleyNoise0 = 1.0f - WorleyNoise(Coord, CellCount * FrequencyMul[0]);
					const float WorleyNoise1 = 1.0f - WorleyNoise(Coord, CellCount * FrequencyMul[1]);
					const float WorleyNoise2 = 1.0f - WorleyNoise(Coord, CellCount * FrequencyMul[2]);

					const float WorleyFBM = WorleyNoise0 * 0.625f + WorleyNoise1 * 0.25f + WorleyNoise2 * 0.125f;

					// mapping Perlin noise in between Worley as minimum and 1.0 as maximum,
					// as described in GPU Pro 7 p.101.
					PerlinWorleyNoise = FMath::Lerp(WorleyFBM, 1.0f, PerlinNoiseValue);
				}

				const float CellCount = 4;
				const float WorleyNoise0 = 1.0f - WorleyNoise(Coord, CellCount * 1);
				const float WorleyNoise1 = 1.0f - WorleyNoise(Coord, CellCount * 2);
				const float WorleyNoise2 = 1.0f - WorleyNoise(Coord, CellCount * 4);
				const float WorleyNoise3 = 1.0f - WorleyNoise(Coord, CellCount * 8);
				const float WorleyNoise4 = 1.0f - WorleyNoise(Coord, CellCount * 16);

				const float WorleyFBM0 = WorleyNoise1 * 0.625f + WorleyNoise2 * 0.25f + WorleyNoise3 * 0.125f;
				const float WorleyFBM1 = WorleyNoise2 * 0.625f + WorleyNoise3 * 0.25f + WorleyNoise4 * 0.125f;
				const float WorleyFBM2 = WorleyNoise3 * 0.75f + WorleyNoise4 * 0.25f;

				int32 Addr = (R * Size * Size + T * Size + S) * 4;
				BaseShapeTexels[Addr] = static_cast<uint8>(255.0f * PerlinWorleyNoise);
				BaseShapeTexels[Addr + 1] = static_cast<uint8>(255.0f * WorleyFBM0);
				BaseShapeTexels[Addr + 2] = static_cast<uint8>(255.0f * WorleyFBM1);
				BaseShapeTexels[Addr + 3] = static_cast<uint8>(255.0f * WorleyFBM2);
			}
		}
	});
}

void UTextureAuthoringComponent::GenerateDetailNoise()
{
	const int32 Size = DetailTextureSize;
	DetailTexels.SetNumUninitialized(Size * Size * Size * 4);

	ParallelFor(Size, [&](int32 S)
	{
		const FVector3f NormFact(1.0f / float(Size));
		for (int32 T = 0; T < Size; T++)
		{
			for (int32 R = 0; R < Size; R++)
			{
				const FVector3f Coord = FVector3f(S, T, R) * NormFact;

				// 3 octaves, higher base cellCount than the base-shape texture since this is the detail layer.
				const float CellCount = 2;
				const float WorleyNoise0 = 1.0f - WorleyNoise(Coord, CellCount * 1);
				const float WorleyNoise1 = 1.0f - WorleyNoise(Coord, CellCount * 2);
				const float WorleyNoise2 = 1.0f - WorleyNoise(Coord, CellCount * 4);
				const float WorleyNoise3 = 1.0f - WorleyNoise(Coord, CellCount * 8);

				const float WorleyFBM0 = WorleyNoise0 * 0.625f + WorleyNoise1 * 0.25f + WorleyNoise2 * 0.125f;
				const float WorleyFBM1 = WorleyNoise1 * 0.625f + WorleyNoise2 * 0.25f + WorleyNoise3 * 0.125f;
				const float WorleyFBM2 = WorleyNoise2 * 0.75f + WorleyNoise3 * 0.25f;

				int32 Addr = (R * Size * Size + T * Size + S) * 4;
				DetailTexels[Addr] = static_cast<uint8>(255.0f * WorleyFBM0);
				DetailTexels[Addr + 1] = static_cast<uint8>(255.0f * WorleyFBM1);
				DetailTexels[Addr + 2] = static_cast<uint8>(255.0f * WorleyFBM2);
				DetailTexels[Addr + 3] = 255; // unused, constant
			}
		}
	});
}
