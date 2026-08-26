#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TextureAuthoringComponent.generated.h"

UCLASS(ClassGroup = (OrbisClouds), meta = (BlueprintSpawnableComponent, DisplayName = "Texture Authoring Component"))
class ORBISCLOUDS_API UTextureAuthoringComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTextureAuthoringComponent();

	// Kicks off base-shape + detail generation on a background thread pool task so the calling
	// (game/editor) thread doesn't block for the duration of the bake. Sets bIsGenerating for the duration.
	UFUNCTION(CallInEditor, Category = "TextureAuthoring")
	void GenerateNoiseTextures();

	// Generates the 128^3, 4-channel cloud base-shape volume (R = Perlin-Worley, G/B/A = Worley FBM at increasing
	// frequency) into BaseShapeTexels, one slice per ParallelFor iteration. Port of Sebastien Hilaire's
	// TileableVolumeNoise main.cpp (https://github.com/sebh/TileableVolumeNoise).
	void GenerateBaseShapeNoise();

	// Generates the 32^3, 3-channel cloud erosion/detail volume (R/G/B = Worley FBM at increasing frequency,
	// A unused/constant) into DetailTexels. Same source as GenerateBaseShapeNoise.
	void GenerateDetailNoise();

	UPROPERTY()
	TArray<uint8> BaseShapeTexels;

	UPROPERTY()
	TArray<uint8> DetailTexels;

	UPROPERTY(EditAnywhere, Category = "TextureAuthoring")
	int32 BaseShapeTextureSize = 128;

	UPROPERTY(EditAnywhere, Category = "TextureAuthoring")
	int32 DetailTextureSize = 32;

	UPROPERTY(VisibleAnywhere, Category = "TextureAuthoring")
	bool bIsGenerating = false;

	UPROPERTY(EditAnywhere, Category = "TextureAuthoring")
	FString OutputPackagePath = TEXT("/Game/OrbisClouds/Generated/");

#if WITH_EDITOR
	// Builds a UVolumeTexture asset from an R,G,B,A-ordered texel buffer and saves it as a .uasset under
	// OutputPackagePath. Must be called on the game thread (UObject/package creation is not thread-safe).
	void SaveVolumeTextureAsset(const TArray<uint8>& RgbaTexels, int32 Size, const FString& AssetName);
#endif

private:
	// Tileable noise primitives ported from Sebastien Hilaire's TileableVolumeNoise
	// (https://github.com/sebh/TileableVolumeNoise). GLM types swapped for FVector3f/FVector4f; algorithm unchanged.

	/// Tileable Worley noise value in [0, 1].
	/// @param Position 3D coordinate in [0, 1], the range of the repeatable pattern.
	/// @param CellCount the number of cells for the repetitive pattern.
	static float WorleyNoise(const FVector3f& Position, float CellCount);

	/// Tileable Perlin noise value in [0, 1].
	/// @param Position 3D coordinate in [0, 1], the range of the repeatable pattern.
	static float PerlinNoise(const FVector3f& Position, float Frequency, int32 OctaveCount);

	// Worley noise function based on https://www.shadertoy.com/view/Xl2XRR by Marc-Andre Loyer
	static float Hash(float N);
	static float ValueNoise(const FVector3f& X);
	static float Cells(const FVector3f& Position, float CellCount);

	// Classic Perlin noise, periodic version. Direct port of GLM's glm::perlin(vec4, vec4) as used by
	// TileableVolumeNoise (based on Stefan Gustavson / Ashima Arts' webgl-noise).
	static float PeriodicPerlin4D(const FVector4f& Position, const FVector4f& Rep);
};
