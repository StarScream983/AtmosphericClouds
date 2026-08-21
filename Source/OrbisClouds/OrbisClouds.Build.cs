// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OrbisClouds : ModuleRules
{
	public OrbisClouds(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bTreatAsEngineModule = true;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"OrbisCloudsCore",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"RenderCore",
				"Renderer",
				"RHI",
				"Projects",
				"ImGui",
			}
		);
	}
}
