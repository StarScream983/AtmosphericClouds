#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Explorer.generated.h"

class UCapsuleComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class UFloatingPawnMovement;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class ORBISCLOUDS_API AExplorer : public APawn
{
	GENERATED_BODY()

public:
	AExplorer();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	FVector GetExplorerLocation() const;

protected:
	virtual void BeginPlay() override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Roll(const FInputActionValue& Value);
	void ChangeSpeed(const FInputActionValue& Value);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> RollAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ChangeSpeedAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "1.0", UIMin = "1.0", UIMax = "100000000.0", Units = "CentimetersPerSecond"))
	float FlightSpeed = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight", meta = (ClampMin = "1.01", UIMin = "1.01", ClampMax = "10.0", UIMax = "10.0"))
	float SpeedStepMultiplier = 2.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Explorer")
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Explorer")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Explorer")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Explorer")
	TObjectPtr<UFloatingPawnMovement> MovementComponent;
};
