#pragma once

#include "CoreMinimal.h"

struct FOrbisCloudsPlanetRenderData
{
	FVector PlanetCenter = FVector::ZeroVector;
	float AtmosphereRadius = 0.f;
	float CloudInnerRadius = 0.f;
	float CloudOuterRadius = 0.f;
	float CloudDensity = 0.f;
};
