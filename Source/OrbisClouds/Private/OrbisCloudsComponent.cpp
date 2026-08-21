#include "OrbisCloudsComponent.h"

#include "OrbisCloudsSubsystem.h"

#if WITH_EDITOR
#include <imgui.h>
#endif

namespace OrbisCloudsPlanetPresets
{
	const FVector SmallCenter(1000000., 1000000., 350000.);
	constexpr float SmallAtmosphereRadius = 500000.f;
	constexpr float SmallCloudInnerRadius = 480000.f;
	constexpr float SmallCloudOuterRadius = 500000.f;

	const FVector LargeCenter(700000000., 700000000., 350000.);
	constexpr float LargeAtmosphereRadius = 600000000.f;
	constexpr float LargeCloudInnerRadius = 580000000.f;
	constexpr float LargeCloudOuterRadius = 600000000.f;
}

UOrbisCloudsComponent::UOrbisCloudsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
#if WITH_EDITOR
	bTickInEditor = true;
#endif
	SetRelativeLocation(OrbisCloudsPlanetPresets::SmallCenter);
	ApplyPlanetScalePreset();
}

FOrbisCloudsPlanetRenderData UOrbisCloudsComponent::BuildPlanetRenderData() const
{
	FOrbisCloudsPlanetRenderData Data;
	Data.PlanetCenter = GetComponentLocation();
	Data.AtmosphereRadius = AtmosphereRadius;
	Data.CloudInnerRadius = CloudInnerRadius;
	Data.CloudOuterRadius = FMath::Max(CloudOuterRadius, CloudInnerRadius + 1.f);
	Data.CloudDensity = FMath::Clamp(CloudDensity, 0.f, 1.f);
	Data.NoiseCellsAcrossDiameter = FMath::Clamp(NoiseCellsAcrossDiameter, 0.1f, 256.f);
	Data.NoiseSeed = static_cast<uint32>(FMath::Max(NoiseSeed, 0));
	Data.BaseNoiseType = static_cast<uint32>(BaseNoise);
	const float ClampedNoiseMin = FMath::Clamp(NoiseOutputMin, -1.f, 1.f);
	const float ClampedNoiseMax = FMath::Clamp(NoiseOutputMax, -1.f, 1.f);
	Data.NoiseOutputMin = FMath::Min(ClampedNoiseMin, ClampedNoiseMax);
	Data.NoiseOutputMax = FMath::Max(ClampedNoiseMin, ClampedNoiseMax);
	return Data;
}

void UOrbisCloudsComponent::OnRegister()
{
	Super::OnRegister();
	if (PlanetScale != EOrbisCloudsPlanetScale::Custom)
	{
		ApplyPlanetScalePreset();
	}
	UpdateSubsystemRegistration();
}

void UOrbisCloudsComponent::OnUnregister()
{
	if (UWorld* World = GetWorld())
	{
		if (UOrbisCloudsSubsystem* Subsystem = World->GetSubsystem<UOrbisCloudsSubsystem>())
		{
			Subsystem->UnregisterPlanet(this);
		}
	}

	Super::OnUnregister();
}

void UOrbisCloudsComponent::BeginPlay()
{
	Super::BeginPlay();
	UpdateSubsystemRegistration();
}


void UOrbisCloudsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if WITH_EDITOR
	if (ImGui::Begin("OrbisClouds"))
	{
		if (ImGui::InputInt("Noise Seed", &NoiseSeed))
		{
			NoiseSeed = FMath::Max(NoiseSeed, 0);
			UpdateSubsystemRegistration();
		}

		int32 BaseNoiseIndex = static_cast<int32>(BaseNoise);
		const char* BaseNoiseNames[] = { "Perlin", "Simplex", "Value" };
		if (ImGui::Combo("Base Noise", &BaseNoiseIndex, BaseNoiseNames, UE_ARRAY_COUNT(BaseNoiseNames)))
		{
			BaseNoise = static_cast<EOrbisCloudsBaseNoise>(BaseNoiseIndex);
			UpdateSubsystemRegistration();
		}

		if (ImGui::DragFloat("Noise Cells Across Diameter", &NoiseCellsAcrossDiameter, 0.1f, 0.1f, 256.f, "%.2f"))
		{
			NoiseCellsAcrossDiameter = FMath::Clamp(NoiseCellsAcrossDiameter, 0.1f, 256.f);
			UpdateSubsystemRegistration();
		}

		ImGui::Separator();
		ImGui::TextUnformatted("Noise Output");

		if (ImGui::DragFloat("Noise Output Min", &NoiseOutputMin, 0.01f, -1.f, 1.f, "%.2f"))
		{
			NoiseOutputMin = FMath::Clamp(NoiseOutputMin, -1.f, NoiseOutputMax);
			UpdateSubsystemRegistration();
		}

		if (ImGui::DragFloat("Noise Output Max", &NoiseOutputMax, 0.01f, -1.f, 1.f, "%.2f"))
		{
			NoiseOutputMax = FMath::Clamp(NoiseOutputMax, NoiseOutputMin, 1.f);
			UpdateSubsystemRegistration();
		}
	}
	ImGui::End();
#endif
}

void UOrbisCloudsComponent::UpdateSubsystemRegistration()
{
	if (UWorld* World = GetWorld())
	{
		if (UOrbisCloudsSubsystem* Subsystem = World->GetSubsystem<UOrbisCloudsSubsystem>())
		{
			Subsystem->RegisterPlanet(this);
		}
	}
}

#pragma region PlanetScalePresets

#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

void UOrbisCloudsComponent::ApplySmallPlanetPreset()
{
	PlanetScale = EOrbisCloudsPlanetScale::Small;
	ApplyPlanetScalePreset();
}

void UOrbisCloudsComponent::ApplyLargePlanetPreset()
{
	PlanetScale = EOrbisCloudsPlanetScale::Large;
	ApplyPlanetScalePreset();
}

void UOrbisCloudsComponent::ApplyPlanetScalePreset()
{
	switch (PlanetScale)
	{
	case EOrbisCloudsPlanetScale::Small:
		SetRelativeLocation(OrbisCloudsPlanetPresets::SmallCenter);
		AtmosphereRadius = OrbisCloudsPlanetPresets::SmallAtmosphereRadius;
		CloudInnerRadius = OrbisCloudsPlanetPresets::SmallCloudInnerRadius;
		CloudOuterRadius = OrbisCloudsPlanetPresets::SmallCloudOuterRadius;
		break;
	case EOrbisCloudsPlanetScale::Large:
		SetRelativeLocation(OrbisCloudsPlanetPresets::LargeCenter);
		AtmosphereRadius = OrbisCloudsPlanetPresets::LargeAtmosphereRadius;
		CloudInnerRadius = OrbisCloudsPlanetPresets::LargeCloudInnerRadius;
		CloudOuterRadius = OrbisCloudsPlanetPresets::LargeCloudOuterRadius;
		break;
	case EOrbisCloudsPlanetScale::Custom:
		break;
	}

#if WITH_EDITOR
	if (GIsEditor)
	{
		Modify();
	}
#endif

	UpdateSubsystemRegistration();
}

#if WITH_EDITOR
void UOrbisCloudsComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(UOrbisCloudsComponent, PlanetScale))
	{
		ApplyPlanetScalePreset();
	}
	else if (PlanetScale != EOrbisCloudsPlanetScale::Custom
		&& (PropertyName == GET_MEMBER_NAME_CHECKED(UOrbisCloudsComponent, AtmosphereRadius)
			|| PropertyName == GET_MEMBER_NAME_CHECKED(UOrbisCloudsComponent, CloudInnerRadius)
			|| PropertyName == GET_MEMBER_NAME_CHECKED(UOrbisCloudsComponent, CloudOuterRadius)
			|| PropertyName == USceneComponent::GetRelativeLocationPropertyName()))
	{
		PlanetScale = EOrbisCloudsPlanetScale::Custom;
		UpdateSubsystemRegistration();
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UOrbisCloudsComponent, CloudDensity)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UOrbisCloudsComponent, NoiseCellsAcrossDiameter)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UOrbisCloudsComponent, NoiseSeed)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UOrbisCloudsComponent, BaseNoise)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UOrbisCloudsComponent, NoiseOutputMin)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UOrbisCloudsComponent, NoiseOutputMax))
	{
		UpdateSubsystemRegistration();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

#pragma endregion
