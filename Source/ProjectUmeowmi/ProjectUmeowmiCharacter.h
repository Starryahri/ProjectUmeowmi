// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "ProjectUmeowmiCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class AProjectUmeowmiCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Rotate Camera Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* RotateCameraAction;

	/** Zoom Camera Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ZoomAction;

	/** Camera Offset */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Isometric Camera", meta = (AllowPrivateAccess = "true"))
	float CameraOffset = 45.f;

	/** Number of camera positions around the character */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Isometric Camera", meta = (AllowPrivateAccess = "true"))
	int32 NumberOfCameraPositions = 4;

	/** Base angle for the first camera position */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Isometric Camera", meta = (AllowPrivateAccess = "true"))
	float BaseCameraAngle = 45.0f;

	/** Current camera position index */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Isometric Camera", meta = (AllowPrivateAccess = "true"))
	int32 CameraPositionIndex = 0;

	/** Camera transition speed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Isometric Camera", meta = (AllowPrivateAccess = "true"))
	float CameraTransitionSpeed = 5.0f;

	/** Target camera rotation */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Isometric Camera", meta = (AllowPrivateAccess = "true"))
	FRotator TargetCameraRotation;

	/** Grid movement settings */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bUseGridMovement = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float GridSize = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float GridMovementSpeed = 500.0f;

	/** Target grid position for movement */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	FVector TargetGridPosition;

	/** Target rotation for grid movement */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	FRotator TargetRotation;

	/** Whether we're currently moving to a grid position */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsMovingToGrid;

	/** Toggle Grid Movement Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ToggleGridMovementAction;

	/** Minimum camera arm length */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Isometric Camera", meta = (AllowPrivateAccess = "true"))
	float MinCameraArmLength = 200.0f;

	/** Maximum camera arm length */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Isometric Camera", meta = (AllowPrivateAccess = "true"))
	float MaxCameraArmLength = 800.0f;

	/** Minimum orthographic width (zoomed in) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Isometric Camera", meta = (AllowPrivateAccess = "true"))
	float MinOrthoWidth = 500.0f;

	/** Maximum orthographic width (zoomed out) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Isometric Camera", meta = (AllowPrivateAccess = "true"))
	float MaxOrthoWidth = 2000.0f;

	/** Zoom speed for mouse wheel */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Isometric Camera", meta = (AllowPrivateAccess = "true"))
	float MouseWheelZoomSpeed = 100.0f;

	/** Zoom speed for controller stick */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Isometric Camera", meta = (AllowPrivateAccess = "true"))
	float ControllerZoomSpeed = 200.0f;

public:
	AProjectUmeowmiCharacter();
	void GetCameraPositionIndex(const FInputActionValue& Value);
	void ToggleGridMovement(const FInputActionValue& Value);
	void ZoomCamera(const FInputActionValue& Value);
	
	/** Called every frame to update camera position */
	virtual void Tick(float DeltaTime) override;
	
protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Helper function to snap position to grid */
	FVector SnapToGrid(const FVector& Location) const;
			
protected:
	virtual void NotifyControllerChanged() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

