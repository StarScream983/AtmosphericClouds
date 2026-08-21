#pragma once

#include "OrbisCloudsRenderTypes.h"
#include "SceneViewExtension.h"

class FOrbisCloudsViewExtension : public FWorldSceneViewExtension
{
public:
	FOrbisCloudsViewExtension(const FAutoRegister& AutoRegister, UWorld* InWorld);

	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;
	virtual void PrePostProcessPass_RenderThread(
		FRDGBuilder& GraphBuilder,
		const FSceneView& View,
		const FPostProcessingInputs& Inputs) override;

private:
	uint32 CachedFrameNumber = 0;
	bool bHasCachedPlanet = false;
	FOrbisCloudsPlanetRenderData CachedPlanet;
};
