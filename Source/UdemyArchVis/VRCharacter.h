// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "VRCharacter.generated.h"

UCLASS()
class UDEMYARCHVIS_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

	public:
		// Sets default values for this character's properties
		AVRCharacter();


	protected:
		// Called when the game starts or when spawned
		virtual void BeginPlay() override;
		UPROPERTY(VisibleAnywhere)
		class UCameraComponent* Camera;

		UPROPERTY()
		class UMotionControllerComponent* LeftController;

		UPROPERTY()
		class UMotionControllerComponent* RightController;

		UPROPERTY(VisibleAnywhere)
		class UStaticMeshComponent* LeftControllerMesh;
		UPROPERTY(VisibleAnywhere)
		class UStaticMeshComponent* RightControllerMesh;

		UPROPERTY(VisibleAnywhere)
		class UStaticMeshComponent* DestinationMarker;

		virtual void MoveForward(float throttle);
		virtual void MoveRight(float throttle);
		virtual void Rotate(float throttle);
		virtual void UpdateDestinationMarker();
		virtual void Teleport();

		UFUNCTION()
		virtual void OnTeleport();

		UPROPERTY()
		class UMaterialInstanceDynamic* BlinkerMaterial;

	public:	
		// Called every frame
		virtual void Tick(float DeltaTime) override;

		// Called to bind functionality to input
		virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	private:
		UPROPERTY(EditAnywhere)
		float TeleportProjectileSpeed = 1000.0;
		UPROPERTY(EditAnywhere)
		float MaxTeleportDistance = 1000;
		UPROPERTY(EditAnywhere)
		float MaxTimeToFade = 200;
		UPROPERTY(EditAnywhere)
		float MaxFadeDuration = 0.2;
		UPROPERTY(EditAnywhere)
		FVector TeleportBounds = FVector(100, 100, 100);
		virtual bool isOnNavMesh(FVector &hitLocation);
		UPROPERTY(EditAnywhere)
		class UMaterialInterface* BlinkerMaterialBase;

		UPROPERTY()
		class UPostProcessComponent* PostProcessingComponent;

		UPROPERTY(EditAnywhere)
		class UCurveFloat* RadiusVsVelocityCurve;

		UFUNCTION()
		virtual void UpdateBlinderRadius(float maxCharVelocity);
		UFUNCTION()
		virtual void UpdateBlinderCenter(FVector &velocity);

		virtual void ProjectParabolicArc(UObject* World, FVector &StartLocation, FVector& Look, FPredictProjectilePathResult& Result);
};
