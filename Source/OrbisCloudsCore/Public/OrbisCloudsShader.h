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
