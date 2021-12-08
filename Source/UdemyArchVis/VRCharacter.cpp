// Fill out your copyright notice in the Description page of Project Settings.


#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Math/Color.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"
#include "Components/PostProcessComponent.h"
#include "MotionControllerComponent.h"
#include "Kismet/GameplayStatics.h"
// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetRootComponent());

	LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
	LeftController->SetupAttachment(GetRootComponent());
	LeftController->SetTrackingSource(EControllerHand::Left);

	LeftControllerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftControllerMesh"));
	LeftControllerMesh->SetupAttachment(LeftController);

	RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
	RightController->SetupAttachment(GetRootComponent());
	RightController->SetTrackingSource(EControllerHand::Right);
	
	RightControllerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightControllerMesh"));
	RightControllerMesh->SetupAttachment(RightController);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());

	PostProcessingComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessingComponent"));
	PostProcessingComponent->SetupAttachment(GetRootComponent());
}

void AVRCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) {
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("Forward"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Right"), this, &AVRCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Rotate"), this, &AVRCharacter::Rotate);
	PlayerInputComponent->BindAction(TEXT("Teleport"), EInputEvent::IE_Released, this, &AVRCharacter::Teleport);
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	DestinationMarker->SetVisibility(false);

	if (BlinkerMaterialBase != nullptr) {
		BlinkerMaterial = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, this);
		PostProcessingComponent->AddOrUpdateBlendable(BlinkerMaterial);
	}

}

void AVRCharacter::MoveForward(float throttle)
{
	
	AddMovementInput(Camera->GetForwardVector() * throttle);
}

void AVRCharacter::MoveRight(float throttle)
{
	AddMovementInput(Camera->GetRightVector() * throttle);
}

void AVRCharacter::Rotate(float throttle)
{
	AddControllerYawInput(1.0 * throttle);
}

void AVRCharacter::Teleport() {
	if (!DestinationMarker->IsVisible()) {
		return;
	}
	auto controller = this->GetController<APlayerController>();
	auto cameraManager = controller->PlayerCameraManager;
	cameraManager->StartCameraFade(100.0, 0.0, MaxTimeToFade, FLinearColor::Black);

	FTimerHandle TimerHandle;
    this->GetWorldTimerManager().SetTimer(TimerHandle, this, &AVRCharacter::OnTeleport, MaxFadeDuration, false);
}

void AVRCharacter::OnTeleport() {
	auto capsuleHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	auto endLoc = DestinationMarker->GetComponentLocation() + capsuleHeight;
	SetActorLocation(endLoc);

	auto controller = this->GetController<APlayerController>();
	auto cameraManager = controller->PlayerCameraManager;
	cameraManager->StopCameraFade();
}

void AVRCharacter::UpdateDestinationMarker() {
	auto Root = GetRootComponent();
	auto World = Root->GetWorld();

	FPredictProjectilePathResult ProjectionResult;
	auto ControllerLoc = RightController->GetComponentLocation();
	auto LookDirection = RightController->GetForwardVector();
	ProjectParabolicArc(World, ControllerLoc, LookDirection, ProjectionResult);
	
	if (ProjectionResult.HitResult.bBlockingHit && isOnNavMesh(ProjectionResult.HitResult.Location)) {
		DestinationMarker->SetVisibility(true);
		DestinationMarker->SetWorldLocation(ProjectionResult.HitResult.Location);
	}
	else {
		DestinationMarker->SetVisibility(false);
	}
}

void AVRCharacter::ProjectParabolicArc(UObject* World, FVector &StartLocation, FVector &Look, FPredictProjectilePathResult& Result) {
	FVector LaunchVelocity = Look * TeleportProjectileSpeed;
	auto PathPredictParams = FPredictProjectilePathParams(10.0, StartLocation, LaunchVelocity, 1000, ECC_Visibility, this);
	PathPredictParams.bTraceComplex = true;
	PathPredictParams.DrawDebugType = EDrawDebugTrace::ForOneFrame;
	UGameplayStatics::PredictProjectilePath(World, PathPredictParams, Result);
}

bool AVRCharacter::isOnNavMesh(FVector& hitLocation) {
	auto navSystem = UNavigationSystemV1::GetNavigationSystem(GetWorld());
	FNavLocation navLocation;
	bool isInMesh = navSystem->ProjectPointToNavigation(hitLocation, navLocation, TeleportBounds);

	return isInMesh;
}


// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateDestinationMarker();

	auto Velocity = GetVelocity();
	auto maxSingleDirectionVelocity = Velocity.Size();
	//UpdateBlinderRadius(maxSingleDirectionVelocity);
	//UpdateBlinderCenter(Velocity);
}

void AVRCharacter::UpdateBlinderCenter(FVector &velocity) {
	if (!BlinkerMaterial) {
		return;
	}

	FVector2D Center;

	FVector MovementDirection = velocity.GetSafeNormal();
	if (MovementDirection.IsNearlyZero()) {
		Center = FVector2D(0.5, 0.5);
	}

	auto PlayerController = this->GetController<APlayerController>();
	if (!PlayerController) {
		Center = FVector2D(0.5, 0.5);
	}

	auto MovementSign = FVector::DotProduct(Camera->GetForwardVector(), MovementDirection);
	FVector WorldStationaryLocation = Camera->GetComponentLocation() + (MovementSign * MovementDirection) * 500;
	FVector2D ScreenStationaryLocation;
	PlayerController->ProjectWorldLocationToScreen(WorldStationaryLocation, ScreenStationaryLocation);

	int32 SizeX, SizeY;
	PlayerController->GetViewportSize(SizeX, SizeY);

	ScreenStationaryLocation.X /= SizeX;
	ScreenStationaryLocation.Y /= SizeY;

	BlinkerMaterial->SetVectorParameterValue(TEXT("BlinkerCenter"), FLinearColor(ScreenStationaryLocation.X, ScreenStationaryLocation.Y, 0));
}

void AVRCharacter::UpdateBlinderRadius(float maxCharVelocity) {
	if (!RadiusVsVelocityCurve || !BlinkerMaterial) {
		return;
	};

	auto curveValueAtVelocity = RadiusVsVelocityCurve->GetFloatValue(maxCharVelocity);
	BlinkerMaterial->SetScalarParameterValue(TEXT("BlinkerRadius"), curveValueAtVelocity);
}
