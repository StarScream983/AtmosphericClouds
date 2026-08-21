// Copyright Epic Games, Inc. All Rights Reserved.

#include "OrbisClouds.h"

//#include "OrbisCloudsImGui.h"

#define LOCTEXT_NAMESPACE "FOrbisCloudsModule"

void FOrbisCloudsModule::StartupModule()
{
	//OrbisCloudsImGui::Register();
}

void FOrbisCloudsModule::ShutdownModule()
{
	//OrbisCloudsImGui::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FOrbisCloudsModule, OrbisClouds)
