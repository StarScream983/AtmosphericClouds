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

#pragma endregion

UCLASS(ClassGroup = (OrbisClouds), meta = (BlueprintSpawnableComponent, DisplayName = "Orbis Clouds Component"))
class ORBISCLOUDS_API UOrbisCloudsComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UOrbisCloudsComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbisClouds|Planet", meta = (ClampMin = "1.0"))
	float PlanetRadius = 500000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbisClouds|Planet", meta = (ClampMin = "1.0"))
	float CloudInnerRadius = 400000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbisClouds|Planet", meta = (ClampMin = "1.0"))
	float CloudOuterRadius = 500000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OrbisClouds|Planet", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CloudDensity = 1.f;

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
