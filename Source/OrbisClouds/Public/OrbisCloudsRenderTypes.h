#pragma once

#include "CoreMinimal.h"

struct FOrbisCloudsPlanetRenderData
{
	FVector PlanetCenter = FVector::ZeroVector;
	float AtmosphereRadius = 0.f;
	float CloudInnerRadius = 0.f;
	float CloudOuterRadius = 0.f;
	float CloudDensity = 0.f;
	float CloudCoverageNoiseScale = 4.f;
	uint32 NoiseSeed = 1337u;
	uint32 BaseNoiseType = 1u;
	float NoiseOutputMin = -1.f;
	float NoiseOutputMax = 1.f;
	float CloudTypeNoiseScale = 2.f;
	uint32 CloudTypeNoiseSeed = 7331u;
	uint32 CloudTypeNoiseType = 1u;
	uint32 WeatherMapChannel = 0u;
};
