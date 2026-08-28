#include "OrbisCloudsViewExtension.h"

#include "OrbisCloudsCVars.h"
#include "OrbisCloudsShader.h"
#include "OrbisCloudsSubsystem.h"
#include "FXRenderingUtils.h"
#include "Math/DoubleFloat.h"
#include "PostProcess/PostProcessInputs.h"
#include "RenderGraphUtils.h"
#include "RHIStaticStates.h"
#include "SceneView.h"
#include "ScreenPass.h"
#include "SystemTextures.h"

static TAutoConsoleVariable<int32> CVarOrbisCloudsDebugSolid(
	TEXT("r.OrbisClouds.DebugSolid"),
	0,
	TEXT("1 = fullscreen magenta debug (verify pass runs). 0 = shell march."),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarOrbisCloudsDepthOcclusion(
	TEXT("r.OrbisClouds.DepthOcclusion"),
	1,
	TEXT("1 = clip shell by scene depth (terrain occludes). 0 = shell ignores depth."),
	ECVF_RenderThreadSafe);

TAutoConsoleVariable<int32> CVarOrbisCloudsViewMode(
	TEXT("r.OrbisClouds.ViewMode"),
	0,
	TEXT("0 = Cloud Coverage. 1 = Cloud Type. 2 = Clouds. 3 = DEBUG: near/far root used."),
	ECVF_RenderThreadSafe);

FOrbisCloudsViewExtension::FOrbisCloudsViewExtension(const FAutoRegister &AutoRegister, UWorld *InWorld)
	: FWorldSceneViewExtension(AutoRegister, InWorld)
{
}

void FOrbisCloudsViewExtension::BeginRenderViewFamily(FSceneViewFamily &InViewFamily)
{
	if (InViewFamily.FrameNumber != CachedFrameNumber)
	{
		bHasCachedPlanet = false;
		CachedFrameNumber = InViewFamily.FrameNumber;
	}

	if (const FSceneInterface *Scene = InViewFamily.Scene)
	{
		if (UWorld *ViewWorld = Scene->GetWorld())
		{
			bHasCachedPlanet = UOrbisCloudsSubsystem::FindPlanetRenderData(ViewWorld, CachedPlanet);
		}
	}
}

void FOrbisCloudsViewExtension::PrePostProcessPass_RenderThread(
	FRDGBuilder &GraphBuilder,
	const FSceneView &View,
	const FPostProcessingInputs &Inputs)
{
	const bool bDebugSolid = CVarOrbisCloudsDebugSolid.GetValueOnRenderThread() != 0;
	const bool bDepthOcclusion = CVarOrbisCloudsDepthOcclusion.GetValueOnRenderThread() != 0;

	FOrbisCloudsPlanetRenderData PlanetForPass = CachedPlanet;
	bool bShouldDraw = bHasCachedPlanet;

	if (!bShouldDraw && bDebugSolid)
	{
		PlanetForPass.PlanetCenter = FVector::ZeroVector;
		PlanetForPass.CloudInnerRadius = 520000000.f;
		PlanetForPass.CloudOuterRadius = 600000000.f;
		bShouldDraw = true;
	}

	if (!bShouldDraw || !View.Family)
	{
		return;
	}

	Inputs.Validate();
	if (!Inputs.SceneTextures)
	{
		return;
	}

	const FIntRect PrimaryViewRect = UE::FXRenderingUtils::GetRawViewRectUnsafe(View);
	FScreenPassTexture SceneColor((*Inputs.SceneTextures)->SceneColorTexture, PrimaryViewRect);
	if (!SceneColor.IsValid())
	{
		return;
	}

	FGlobalShaderMap *GlobalShaderMap = GetGlobalShaderMap(View.GetFeatureLevel());
	TShaderMapRef<FOrbisCloudsPS> PixelShader(GlobalShaderMap);
	if (!PixelShader.IsValid())
	{
		return;
	}

	FScreenPassRenderTarget Output(SceneColor, ERenderTargetLoadAction::ELoad);
	const FScreenPassTextureViewport OutputViewport(Output);

	const FVector ViewOrigin = View.ViewMatrices.GetViewOrigin();
	const FVector PlanetCenterRelative = PlanetForPass.PlanetCenter - ViewOrigin;

	// Double-float (High/Low) split of the same value, built directly from the double-precision FVector so
	// no precision is lost before it reaches the shader. Feeds ComputePreciseOuterSurfaceDirection in
	// OrbisClouds.ush — see the comment there for why the plain PlanetCenterRelative above isn't enough for
	// that specific computation.
	const FDFVector3 PlanetCenterRelativeDF(PlanetCenterRelative);

	FOrbisCloudsPS::FParameters *PassParameters = GraphBuilder.AllocParameters<FOrbisCloudsPS::FParameters>();
	PassParameters->PlanetCenterRelative = FVector3f(PlanetCenterRelative);
	PassParameters->PlanetCenterRelativeHi = PlanetCenterRelativeDF.High;
	PassParameters->PlanetCenterRelativeLo = PlanetCenterRelativeDF.Low;
	PassParameters->InnerRadius = PlanetForPass.CloudInnerRadius;
	PassParameters->OuterRadius = PlanetForPass.CloudOuterRadius;
	PassParameters->CloudDensity = PlanetForPass.CloudDensity;
	PassParameters->CloudCoverageNoiseScale = PlanetForPass.CloudCoverageNoiseScale;
	PassParameters->NoiseSeed = PlanetForPass.NoiseSeed;
	PassParameters->BaseNoiseType = PlanetForPass.BaseNoiseType;
	PassParameters->NoiseOutputMin = PlanetForPass.NoiseOutputMin;
	PassParameters->NoiseOutputMax = PlanetForPass.NoiseOutputMax;
	PassParameters->CloudsCoverageOctaves = PlanetForPass.CloudsCoverageOctaves;
	PassParameters->CloudsCoverageLacunarity = PlanetForPass.CloudsCoverageLacunarity;
	PassParameters->CloudsCoverageGain = PlanetForPass.CloudsCoverageGain;
	PassParameters->bCloudsCoverageUseWarp = PlanetForPass.bCloudsCoverageUseWarp ? 1u : 0u;
	PassParameters->CloudsCoverageWarpStrength = PlanetForPass.CloudsCoverageWarpStrength;
	PassParameters->CloudsCoverageWarpOctaves = PlanetForPass.CloudsCoverageWarpOctaves;
	PassParameters->CloudTypeNoiseScale = PlanetForPass.CloudTypeNoiseScale;
	PassParameters->CloudTypeNoiseSeed = PlanetForPass.CloudTypeNoiseSeed;
	PassParameters->CloudTypeNoiseType = PlanetForPass.CloudTypeNoiseType;
	PassParameters->CloudsTypeOctaves = PlanetForPass.CloudsTypeOctaves;
	PassParameters->CloudsTypeLacunarity = PlanetForPass.CloudsTypeLacunarity;
	PassParameters->CloudsTypeGain = PlanetForPass.CloudsTypeGain;
	PassParameters->CloudsViewMode = static_cast<uint32>(FMath::Clamp(
		CVarOrbisCloudsViewMode.GetValueOnRenderThread(),
		0,
		4));
	PassParameters->bDebugSolid = bDebugSolid ? 1u : 0u;
	PassParameters->bDepthOcclusion = bDepthOcclusion ? 1u : 0u;
	PassParameters->SceneDepthTexture = (*Inputs.SceneTextures)->SceneDepthTexture;
	PassParameters->SceneDepthSampler = TStaticSamplerState<SF_Point>::GetRHI();

	// SHADER_PARAMETER_RDG_TEXTURE always needs something bound, so fall back to a 1x1 black volume when
	// the component doesn't have a texture assigned — bHasBaseShapeNoiseTexture/bHasDetailNoiseTexture tell
	// the shader whether what's bound is real or just the dummy.
	PassParameters->bHasBaseShapeNoiseTexture = PlanetForPass.BaseShapeNoiseTextureRHI.IsValid() ? 1u : 0u;
	PassParameters->BaseShapeNoiseTexture = PlanetForPass.BaseShapeNoiseTextureRHI.IsValid()
												? RegisterExternalTexture(GraphBuilder, PlanetForPass.BaseShapeNoiseTextureRHI, TEXT("OrbisClouds.BaseShapeNoise"))
												: GSystemTextures.GetVolumetricBlackDummy(GraphBuilder);
	PassParameters->BaseShapeNoiseSampler = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();

	PassParameters->bHasDetailNoiseTexture = PlanetForPass.DetailNoiseTextureRHI.IsValid() ? 1u : 0u;
	PassParameters->DetailNoiseTexture = PlanetForPass.DetailNoiseTextureRHI.IsValid()
											 ? RegisterExternalTexture(GraphBuilder, PlanetForPass.DetailNoiseTextureRHI, TEXT("OrbisClouds.DetailNoise"))
											 : GSystemTextures.GetVolumetricBlackDummy(GraphBuilder);
	PassParameters->DetailNoiseSampler = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();

	PassParameters->View = View.ViewUniformBuffer;
	PassParameters->RenderTargets[0] = Output.GetRenderTargetBinding();

	TShaderMapRef<FScreenPassVS> VertexShader(GlobalShaderMap);
	FRHIBlendState *AlphaBlendState = FScreenPassPipelineState::FDefaultBlendState::GetRHI();
	FRHIDepthStencilState *DepthStencilState = FScreenPassPipelineState::FDefaultDepthStencilState::GetRHI();

	AddDrawScreenPass(
		GraphBuilder,
		RDG_EVENT_NAME("OrbisClouds"),
		View,
		OutputViewport,
		OutputViewport,
		VertexShader,
		PixelShader,
		AlphaBlendState,
		DepthStencilState,
		PassParameters);
}
