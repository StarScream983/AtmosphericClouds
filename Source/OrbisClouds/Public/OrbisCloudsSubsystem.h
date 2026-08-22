#pragma once

#include "CoreMinimal.h"
#include "OrbisCloudsRenderTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "OrbisCloudsSubsystem.generated.h"

class UOrbisCloudsComponent;
class FOrbisCloudsViewExtension;

UCLASS()
class ORBISCLOUDS_API UOrbisCloudsSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableInEditor() const override { return true; }

	void RegisterPlanet(UOrbisCloudsComponent* Component);
	void UnregisterPlanet(UOrbisCloudsComponent* Component);

	bool HasActivePlanet() const;
	UOrbisCloudsComponent* GetRegisteredPlanet() const;
	bool GetPlanetRenderData(FOrbisCloudsPlanetRenderData& OutPlanetData) const;
	static bool FindPlanetRenderData(const UWorld* World, FOrbisCloudsPlanetRenderData& OutPlanetData);

private:
	TWeakObjectPtr<UOrbisCloudsComponent> RegisteredPlanet;
	TSharedPtr<FOrbisCloudsViewExtension, ESPMode::ThreadSafe> ViewExtension;
};
