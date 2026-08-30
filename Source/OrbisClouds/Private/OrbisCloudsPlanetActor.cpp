#include "OrbisCloudsPlanetActor.h"
#include "OrbisCloudsComponent.h"

AOrbisCloudsPlanetActor::AOrbisCloudsPlanetActor()
{
	PrimaryActorTick.bCanEverTick = false;
	OrbisCloudsComponent = CreateDefaultSubobject<UOrbisCloudsComponent>(TEXT("OrbisCloudsComponent"));
	SetRootComponent(OrbisCloudsComponent);
}
