#pragma once

#include "CoreMinimal.h"
#include "OrbisCloudsRenderTypes.h"
#include "Components/SceneComponent.h"
#include "OrbisCloudsComponent.generated.h"

#pragma region PlanetScalePresets

UENUM(BlueprintType)
enum class EOrbisCloudsPlanetScale : uint8
{
	Custom UMETA(DisplayName = "Custom"),
	Small UMETA(DisplayName = "Small (debug)"),
	Large UMETA(DisplayName = "Large (planet)"),
};

UENUM(BlueprintType)
enum class EOrbisCloudsBaseNoise : uint8
{
	Perlin UMETA(DisplayName = "Perlin"),
	Simplex UMETA(DisplayName = "Simplex"),
	Value UMETA(DisplayName = "Value"),
};

UENUM(BlueprintType)
enum class EOrbisCloudsWeatherMapChannel : uint8
{
	CloudCoverage UMETA(DisplayName = "Cloud Coverage"),
	CloudType UMETA(DisplayName = "Cloud Type"),
};

#pragma endregion

UCLASS(ClassGroup = (OrbisClouds), meta = (BlueprintSpawnableComponent, DisplayName = "Orbis Clouds Component"))
class ORBISCLOUDS_API UOrbisCloudsComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UOrbisCloudsComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbisClouds|Planet", meta = (ClampMin = "1.0"))
	float AtmosphereRadius = 500000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbisClouds|Planet", meta = (ClampMin = "1.0"))
	float CloudInnerRadius = 480000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbisClouds|Planet", meta = (ClampMin = "1.0"))
	float CloudOuterRadius = 500000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbisClouds|Planet", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CloudDensity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbisClouds|Cloud Coverage", meta = (ClampMin = "0.1", ClampMax = "2048.0"))
	float CloudCoverageNoiseScale = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbisClouds|Cloud Coverage", meta = (ClampMin = "0"))
	int32 NoiseSeed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbisClouds|Cloud Coverage")
	EOrbisCloudsBaseNoise BaseNoise = EOrbisCloudsBaseNoise::Simplex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbisClouds|Cloud Coverage", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float NoiseOutputMin = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbisClouds|Cloud Coverage", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float NoiseOutputMax = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbisClouds|Cloud Type", meta = (ClampMin = "0.1", ClampMax = "4096.0"))
	float CloudTypeNoiseScale = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbisClouds|Cloud Type", meta = (ClampMin = "0"))
	int32 CloudTypeNoiseSeed = 7331;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbisClouds|Cloud Type")
	EOrbisCloudsBaseNoise CloudTypeNoise = EOrbisCloudsBaseNoise::Simplex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbisClouds|Debug")
	EOrbisCloudsWeatherMapChannel DisplayedWeatherMapChannel = EOrbisCloudsWeatherMapChannel::CloudCoverage;

	FOrbisCloudsPlanetRenderData BuildPlanetRenderData() const;

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void UpdateSubsystemRegistration();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbisClouds|Planet")
	EOrbisCloudsPlanetScale PlanetScale = EOrbisCloudsPlanetScale::Small;

	UFUNCTION(CallInEditor, Category = "OrbisClouds|Planet")
	void ApplySmallPlanetPreset();

	UFUNCTION(CallInEditor, Category = "OrbisClouds|Planet")
	void ApplyLargePlanetPreset();

protected:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void ApplyPlanetScalePreset();
};
