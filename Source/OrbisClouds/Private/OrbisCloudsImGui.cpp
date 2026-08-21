#include "OrbisCloudsImGui.h"

#if WITH_EDITOR
#include "ImGuiModule.h"

#include <imgui.h>

static FImGuiDelegateHandle GOrbisCloudsImGuiHandle;

static void DrawOrbisCloudsImGui()
{
	if (ImGui::Begin("OrbisClouds"))
	{
		ImGui::TextUnformatted("OrbisClouds ImGui is active.");
	}

	ImGui::End();
}
#endif

void OrbisCloudsImGui::Register()
{
#if WITH_EDITOR
	if (!FImGuiModule::IsAvailable())
	{
		return;
	}

	if (!GOrbisCloudsImGuiHandle.IsValid())
	{
		GOrbisCloudsImGuiHandle = FImGuiModule::Get().AddMultiContextImGuiDelegate(
			FImGuiDelegate::CreateStatic(&DrawOrbisCloudsImGui));
	}
#endif
}

void OrbisCloudsImGui::Unregister()
{
#if WITH_EDITOR
	if (!FImGuiModule::IsAvailable() || !GOrbisCloudsImGuiHandle.IsValid())
	{
		return;
	}

	FImGuiModule::Get().RemoveImGuiDelegate(GOrbisCloudsImGuiHandle);
	GOrbisCloudsImGuiHandle.Reset();
#endif
}
