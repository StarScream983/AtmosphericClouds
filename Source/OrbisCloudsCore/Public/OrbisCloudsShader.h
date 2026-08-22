#pragma once

#include "GlobalShader.h"
#include "ShaderParameterStruct.h"

class ORBISCLOUDSCORE_API FOrbisCloudsPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FOrbisCloudsPS);
	SHADER_USE_PARAMETER_STRUCT(FOrbisCloudsPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(FVector3f, PlanetCenterRelative)
		SHADER_PARAMETER(float, InnerRadius)
		SHADER_PARAMETER(float, OuterRadius)
		SHADER_PARAMETER(float, CloudDensity)
		SHADER_PARAMETER(float, CloudCoverageNoiseScale)
		SHADER_PARAMETER(uint32, NoiseSeed)
		SHADER_PARAMETER(uint32, BaseNoiseType)
		SHADER_PARAMETER(float, NoiseOutputMin)
		SHADER_PARAMETER(float, NoiseOutputMax)
		SHADER_PARAMETER(int32, CloudsCoverageOctaves)
		SHADER_PARAMETER(float, CloudsCoverageLacunarity)
		SHADER_PARAMETER(float, CloudsCoverageGain)
		SHADER_PARAMETER(float, CloudTypeNoiseScale)
		SHADER_PARAMETER(uint32, CloudTypeNoiseSeed)
		SHADER_PARAMETER(uint32, CloudTypeNoiseType)
		SHADER_PARAMETER(int32, CloudsTypeOctaves)
		SHADER_PARAMETER(float, CloudsTypeLacunarity)
		SHADER_PARAMETER(float, CloudsTypeGain)
		SHADER_PARAMETER(uint32, WeatherMapChannel)
		SHADER_PARAMETER(uint32, bDebugSolid)
		SHADER_PARAMETER(uint32, bDepthOcclusion)
		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, SceneDepthSampler)
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
		RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};
