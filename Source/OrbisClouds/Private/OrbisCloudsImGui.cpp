#include "OrbisCloudsImGui.h"

#if WITH_EDITOR
#include "OrbisCloudsComponent.h"
#include "OrbisCloudsCVars.h"

#include <imgui.h>
#endif

void OrbisCloudsImGui::Register()
{
}

void OrbisCloudsImGui::Unregister()
{
}

void OrbisCloudsImGui::Draw(UOrbisCloudsComponent* Component)
{
#if WITH_EDITOR
	if (!ImGui::Begin("OrbisClouds"))
	{
		ImGui::End();
		return;
	}

	ImGui::TextUnformatted("IMGUI DEBUG CLASS");

	int32 WeatherMapChannelIndex = FMath::Clamp(
		CVarOrbisCloudsWeatherMapChannel.GetValueOnGameThread(),
		0,
		1);
	const char* WeatherMapChannelNames[] = { "Cloud Coverage", "Cloud Type" };
	if (ImGui::Combo(
		"Displayed Weather Map",
		&WeatherMapChannelIndex,
		WeatherMapChannelNames,
		UE_ARRAY_COUNT(WeatherMapChannelNames)))
	{
		CVarOrbisCloudsWeatherMapChannel->Set(WeatherMapChannelIndex);
	}

	if (!Component)
	{
		ImGui::TextUnformatted("No OrbisClouds component.");
		ImGui::End();
		return;
	}

	if (ImGui::Button("Small Radius"))
	{
		Component->ApplySmallPlanetPreset();
	}
	ImGui::SameLine();
	if (ImGui::Button("Large Radius"))
	{
		Component->ApplyLargePlanetPreset();
	}

	ImGui::TextUnformatted("Cloud Coverage");

	if (ImGui::InputInt("Seed##CloudCoverage", &Component->NoiseSeed))
	{
		Component->NoiseSeed = FMath::Max(Component->NoiseSeed, 0);
		Component->NotifyChanged();
	}

	int32 BaseNoiseIndex = static_cast<int32>(Component->BaseNoise);
	const char* BaseNoiseNames[] = { "Perlin", "Simplex", "Value" };
	if (ImGui::Combo("Base Noise##CloudCoverage", &BaseNoiseIndex, BaseNoiseNames, UE_ARRAY_COUNT(BaseNoiseNames)))
	{
		Component->BaseNoise = static_cast<EOrbisCloudsBaseNoise>(BaseNoiseIndex);
		Component->NotifyChanged();
	}

	if (ImGui::DragFloat("Noise Scale##CloudCoverage", &Component->CloudCoverageNoiseScale, 0.1f, 0.1f, 2048.f, "%.2f"))
	{
		Component->CloudCoverageNoiseScale = FMath::Clamp(Component->CloudCoverageNoiseScale, 0.1f, 2048.f);
		Component->NotifyChanged();
	}

	ImGui::TextUnformatted("Cloud Type");

	if (ImGui::InputInt("Seed##CloudType", &Component->CloudTypeNoiseSeed))
	{
		Component->CloudTypeNoiseSeed = FMath::Max(Component->CloudTypeNoiseSeed, 0);
		Component->NotifyChanged();
	}

	int32 CloudTypeNoiseIndex = static_cast<int32>(Component->CloudTypeNoise);
	if (ImGui::Combo("Base Noise##CloudType", &CloudTypeNoiseIndex, BaseNoiseNames, UE_ARRAY_COUNT(BaseNoiseNames)))
	{
		Component->CloudTypeNoise = static_cast<EOrbisCloudsBaseNoise>(CloudTypeNoiseIndex);
		Component->NotifyChanged();
	}

	if (ImGui::DragFloat("Noise Scale##CloudType", &Component->CloudTypeNoiseScale, 0.1f, 0.1f, 4096.f, "%.2f"))
	{
		Component->CloudTypeNoiseScale = FMath::Clamp(Component->CloudTypeNoiseScale, 0.1f, 4096.f);
		Component->NotifyChanged();
	}

	ImGui::Separator();
	ImGui::TextUnformatted("Noise Output");

	if (ImGui::DragFloat("Noise Output Min", &Component->NoiseOutputMin, 0.01f, -1.f, 1.f, "%.2f"))
	{
		Component->NoiseOutputMin = FMath::Clamp(Component->NoiseOutputMin, -1.f, Component->NoiseOutputMax);
		Component->NotifyChanged();
	}

	if (ImGui::DragFloat("Noise Output Max", &Component->NoiseOutputMax, 0.01f, -1.f, 1.f, "%.2f"))
	{
		Component->NoiseOutputMax = FMath::Clamp(Component->NoiseOutputMax, Component->NoiseOutputMin, 1.f);
		Component->NotifyChanged();
	}

	ImGui::End();
#endif
}
