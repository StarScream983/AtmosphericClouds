#include "OrbisCloudsComponent.h"

#include "OrbisCloudsSubsystem.h"

#if WITH_EDITOR
#include <imgui.h>
#endif

namespace OrbisCloudsPlanetPresets
{
	const FVector SmallCenter(1000000., 1000000., 350000.);
	constexpr float SmallAtmosphereRadius = 500000.f;
	constexpr float SmallCloudInnerRadius = 400000.f;
	constexpr float SmallCloudOuterRadius = 500000.f;

	const FVector LargeCenter(700000000., 700000000., 350000.);
	constexpr float LargeAtmosphereRadius = 600000000.f;
	constexpr float LargeCloudInnerRadius = 520000000.f;
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
		ImGui::TextUnformatted("hello");
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
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UOrbisCloudsComponent, CloudDensity))
	{
		UpdateSubsystemRegistration();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

#pragma endregion
