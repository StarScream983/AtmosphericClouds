#include "OrbisCloudsSubsystem.h"

#include "OrbisCloudsComponent.h"
#include "OrbisCloudsImGui.h"
#include "OrbisCloudsViewExtension.h"
#include "EngineUtils.h"

void UOrbisCloudsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ViewExtension = FSceneViewExtensions::NewExtension<FOrbisCloudsViewExtension>(GetWorld());
}

void UOrbisCloudsSubsystem::Deinitialize()
{
	ViewExtension.Reset();
	Super::Deinitialize();
}

void UOrbisCloudsSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	OrbisCloudsImGui::Draw(GetRegisteredPlanet());
}

TStatId UOrbisCloudsSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UOrbisCloudsSubsystem, STATGROUP_Tickables);
}

void UOrbisCloudsSubsystem::RegisterPlanet(UOrbisCloudsComponent* Component)
{
	if (!IsValid(Component))
	{
		return;
	}

	RegisteredPlanet = Component;
}

void UOrbisCloudsSubsystem::UnregisterPlanet(UOrbisCloudsComponent* Component)
{
	if (RegisteredPlanet.Get() == Component)
	{
		RegisteredPlanet.Reset();
	}
}

bool UOrbisCloudsSubsystem::HasActivePlanet() const
{
	return RegisteredPlanet.IsValid();
}

UOrbisCloudsComponent* UOrbisCloudsSubsystem::GetRegisteredPlanet() const
{
	return RegisteredPlanet.Get();
}

bool UOrbisCloudsSubsystem::GetPlanetRenderData(FOrbisCloudsPlanetRenderData& OutPlanetData) const
{
	if (const UOrbisCloudsComponent* Component = RegisteredPlanet.Get())
	{
		OutPlanetData = Component->BuildPlanetRenderData();
		return true;
	}

	return false;
}

bool UOrbisCloudsSubsystem::FindPlanetRenderData(const UWorld* World, FOrbisCloudsPlanetRenderData& OutPlanetData)
{
	if (!World)
	{
		return false;
	}

	if (UOrbisCloudsSubsystem* Subsystem = World->GetSubsystem<UOrbisCloudsSubsystem>())
	{
		if (Subsystem->GetPlanetRenderData(OutPlanetData))
		{
			return true;
		}

		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (UOrbisCloudsComponent* Component = It->FindComponentByClass<UOrbisCloudsComponent>())
			{
				Subsystem->RegisterPlanet(Component);
				OutPlanetData = Component->BuildPlanetRenderData();
				return true;
			}
		}
	}

	return false;
}
