#pragma once

#include "CoreMinimal.h"

struct FOrbisCloudsPlanetRenderData
{
	FVector PlanetCenter = FVector::ZeroVector;
	float AtmosphereRadius = 0.f;
	float CloudInnerRadius = 0.f;
	float CloudOuterRadius = 0.f;
	float CloudDensity = 0.f;
	float NoiseCellsAcrossDiameter = 4.f;
	uint32 NoiseSeed = 1337u;
	uint32 BaseNoiseType = 1u;
	float NoiseOutputMin = -1.f;
	float NoiseOutputMax = 1.f;
};
