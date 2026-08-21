#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OrbisCloudsPlanetActor.generated.h"

class UOrbisCloudsComponent;

/** Planet actor with a default OrbisClouds scene component (cloud shell origin + radii). */
UCLASS(Blueprintable, meta = (DisplayName = "Orbis Clouds Planet"))
class ORBISCLOUDS_API AOrbisCloudsPlanetActor : public AActor
{
	GENERATED_BODY()

public:
	AOrbisCloudsPlanetActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OrbisClouds")
	TObjectPtr<UOrbisCloudsComponent> OrbisCloudsComponent;
};
