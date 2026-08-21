// Copyright Epic Games, Inc. All Rights Reserved.

#include "OrbisCloudsCore.h"

#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"

#define LOCTEXT_NAMESPACE "FOrbisCloudsCoreModule"

namespace
{
	const TCHAR* PluginName = TEXT("AtmosphericClouds");
	const TCHAR* ShaderVirtualPath = TEXT("/Plugin/OrbisClouds");
}

void FOrbisCloudsCoreModule::StartupModule()
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(PluginName);
	checkf(Plugin.IsValid(), TEXT("Failed to find plugin '%s' for shader directory mapping."), PluginName);

	const FString PluginShaderDir = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"), TEXT("Private"));
	AddShaderSourceDirectoryMapping(ShaderVirtualPath, PluginShaderDir);
}

void FOrbisCloudsCoreModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FOrbisCloudsCoreModule, OrbisCloudsCore)
