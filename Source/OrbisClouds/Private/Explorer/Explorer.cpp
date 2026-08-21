#include "Explorer/Explorer.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Controller.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"

AExplorer::AExplorer()
{
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleComponent"));
	CapsuleComponent->InitCapsuleSize(5.f, 5.f);
	CapsuleComponent->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	CapsuleComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	CapsuleComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CapsuleComponent->CanCharacterStepUpOn = ECB_No;
	CapsuleComponent->SetShouldUpdatePhysicsVolume(true);
	CapsuleComponent->SetCanEverAffectNavigation(false);
	CapsuleComponent->bDynamicObstacle = false;
	SetRootComponent(CapsuleComponent);

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	MovementComponent->UpdatedComponent = RootComponent;
	MovementComponent->MaxSpeed = 3000.f;
	MovementComponent->Acceleration = 8000.f;
	MovementComponent->Deceleration = 8000.f;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritRoll = true;
	SpringArm->bEnableCameraLag = false;
	SpringArm->bEnableCameraRotationLag = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
}

void AExplorer::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent->MaxSpeed = FlightSpeed;
	MovementComponent->Acceleration = FlightSpeed * 4.f;
	MovementComponent->Deceleration = FlightSpeed * 4.f;

	const APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (PlayerController && DefaultMappingContext)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AExplorer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AExplorer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AExplorer::Move);
		}
		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AExplorer::Look);
		}
		if (RollAction)
		{
			EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Triggered, this, &AExplorer::Roll);
		}
		if (ChangeSpeedAction)
		{
			EnhancedInputComponent->BindAction(ChangeSpeedAction, ETriggerEvent::Triggered, this, &AExplorer::ChangeSpeed);
		}
	}
}

void AExplorer::Move(const FInputActionValue& Value)
{
	const FVector MovementVector = Value.Get<FVector>();
	const FQuat ActorQuat = GetActorQuat();

	AddMovementInput(ActorQuat.GetForwardVector(), MovementVector.X);
	AddMovementInput(ActorQuat.GetRightVector(), MovementVector.Y);
	AddMovementInput(ActorQuat.GetUpVector(), MovementVector.Z);
}

void AExplorer::Look(const FInputActionValue& Value)
{
	const FVector2D LookInput = Value.Get<FVector2D>();
	if (LookInput.IsNearlyZero())
	{
		return;
	}

	constexpr float Sensitivity = 0.015f;
	const FQuat CurrentRotation = GetActorQuat();
	const FQuat YawRotation(CurrentRotation.GetUpVector(), LookInput.X * Sensitivity);
	const FQuat PitchRotation(CurrentRotation.GetRightVector(), -LookInput.Y * Sensitivity);

	FQuat NewRotation = YawRotation * PitchRotation * CurrentRotation;
	NewRotation.Normalize();
	SetActorRotation(NewRotation);
}

void AExplorer::Roll(const FInputActionValue& Value)
{
	const float RollInput = Value.Get<float>();
	if (FMath::IsNearlyZero(RollInput))
	{
		return;
	}

	constexpr float RollSpeed = 1.5f;
	const FQuat CurrentRotation = GetActorQuat();
	const FQuat RollRotation(CurrentRotation.GetForwardVector(), RollInput * RollSpeed * GetWorld()->GetDeltaSeconds());

	FQuat NewRotation = RollRotation * CurrentRotation;
	NewRotation.Normalize();
	SetActorRotation(NewRotation);
}

void AExplorer::ChangeSpeed(const FInputActionValue& Value)
{
	const float WheelDirection = Value.Get<float>();
	if (FMath::IsNearlyZero(WheelDirection))
	{
		return;
	}

	FlightSpeed = FMath::Clamp(
		FlightSpeed * FMath::Pow(SpeedStepMultiplier, WheelDirection),
		1.f,
		100000000.f);

	MovementComponent->MaxSpeed = FlightSpeed;
	MovementComponent->Acceleration = FlightSpeed * 4.f;
	MovementComponent->Deceleration = FlightSpeed * 4.f;
}

FVector AExplorer::GetExplorerLocation() const
{
	return GetActorLocation();
}
