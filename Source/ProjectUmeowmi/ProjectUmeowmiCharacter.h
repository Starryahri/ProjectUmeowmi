// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "DlgSystem/DlgDialogueParticipant.h"
#include "ProjectUmeowmiCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class ATalkingObject;
class UPUDialogueBox;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class AProjectUmeowmiCharacter : public ACharacter, public IDlgDialogueParticipant
{
	GENERATED_BODY()

	////////////////////////////////////////////////////////////
	// Input Configuration
	////////////////////////////////////////////////////////////
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Config", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	// Todo: Marked for deletion
	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Config", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Config", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Config", meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Rotate Camera Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Config", meta = (AllowPrivateAccess = "true"))
	UInputAction* RotateCameraAction;

	/** Zoom Camera Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Config", meta = (AllowPrivateAccess = "true"))
	UInputAction* ZoomAction;

	/** Interact Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Config", meta = (AllowPrivateAccess = "true"))
	UInputAction* InteractAction;


	////////////////////////////////////////////////////////////
	// Camera Configuration
	////////////////////////////////////////////////////////////
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Config|Isometric", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Config|Isometric", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Camera Offset */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Config|Isometric", meta = (AllowPrivateAccess = "true"))
	float CameraOffset = 45.f;

	/** Number of camera positions around the character */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Config|Isometric", meta = (AllowPrivateAccess = "true"))
	int32 NumberOfCameraPositions = 4;

	/** Base angle for the first camera position */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Config|Isometric", meta = (AllowPrivateAccess = "true"))
	float BaseCameraAngle = 45.0f;

	/** Current camera position index */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Config|Isometric", meta = (AllowPrivateAccess = "true"))
	int32 CameraPositionIndex = 0;

	/** Camera transition speed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Config|Isometric", meta = (AllowPrivateAccess = "true"))
	float CameraTransitionSpeed = 5.0f;

	/** Target camera rotation */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera Config|Isometric", meta = (AllowPrivateAccess = "true"))
	FRotator TargetCameraRotation;

	/** Minimum camera arm length */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Config", meta = (AllowPrivateAccess = "true"))
	float MinCameraArmLength = 200.0f;

	/** Maximum camera arm length */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Config", meta = (AllowPrivateAccess = "true"))
	float MaxCameraArmLength = 800.0f;

	/** Minimum orthographic width (zoomed in) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Config", meta = (AllowPrivateAccess = "true"))
	float MinOrthoWidth = 500.0f;

	/** Maximum orthographic width (zoomed out) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Config", meta = (AllowPrivateAccess = "true"))
	float MaxOrthoWidth = 2000.0f;

	/** Zoom speed for mouse wheel */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Config", meta = (AllowPrivateAccess = "true"))
	float MouseWheelZoomSpeed = 100.0f;

	/** Zoom speed for controller stick */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Config", meta = (AllowPrivateAccess = "true"))
	float ControllerZoomSpeed = 200.0f;


	////////////////////////////////////////////////////////////
	// Movement Configuration
	////////////////////////////////////////////////////////////
	/** Grid movement settings */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement Config|Grid", meta = (AllowPrivateAccess = "true"))
	bool bUseGridMovement = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement Config|Grid", meta = (AllowPrivateAccess = "true"))
	float GridSize = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement Config|Grid", meta = (AllowPrivateAccess = "true"))
	float GridMovementSpeed = 500.0f;

	/** Target grid position for movement */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement Config|Grid", meta = (AllowPrivateAccess = "true"))
	FVector TargetGridPosition;

	/** Target rotation for grid movement */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement Config|Grid", meta = (AllowPrivateAccess = "true"))
	FRotator TargetRotation;

	/** Whether we're currently moving to a grid position */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement Config|Grid", meta = (AllowPrivateAccess = "true"))
	bool bIsMovingToGrid;

	/** Toggle Grid Movement Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement Config|Grid", meta = (AllowPrivateAccess = "true"))
	UInputAction* ToggleGridMovementAction;


	////////////////////////////////////////////////////////////
	// Dialogue and Interaction Configuration
	////////////////////////////////////////////////////////////
	/** Current talking object that can be interacted with */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue and Interaction|Talking Object", meta = (AllowPrivateAccess = "true"))
	ATalkingObject* CurrentTalkingObject;

	/** Name of the dialogue participant */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue and Interaction|Talking Object", meta = (AllowPrivateAccess = "true"))
	FName ParticipantName;

	/** Display name of the dialogue participant */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue and Interaction|Talking Object", meta = (AllowPrivateAccess = "true"))	
	FText DisplayName;

	/** Icon of the dialogue participant */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue and Interaction|Talking Object", meta = (AllowPrivateAccess = "true"))
	UTexture2D* ParticipantIcon;

	/** Reference to the dialogue box widget */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue and Interaction|Dialogue Box", meta = (AllowPrivateAccess = "true"))
	UPUDialogueBox* DialogueBox;

    // IDlgDialogueParticipant Interface
	FName GetParticipantName_Implementation() const override { return ParticipantName; }
    FText GetParticipantDisplayName_Implementation(FName ActiveSpeaker) const override { return DisplayName; }
    UTexture2D* GetParticipantIcon_Implementation(FName ActiveSpeaker, FName ActiveSpeakerState) const override { return ParticipantIcon; }

public:
	AProjectUmeowmiCharacter();
	void GetCameraPositionIndex(const FInputActionValue& Value);
	void ToggleGridMovement(const FInputActionValue& Value);
	void ZoomCamera(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);
	
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
	
	/** Register a talking object for interaction */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void RegisterTalkingObject(ATalkingObject* TalkingObject);
	
	/** Unregister a talking object from interaction */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void UnregisterTalkingObject(ATalkingObject* TalkingObject);
	
	/** Check if there's a talking object available for interaction */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool HasTalkingObjectAvailable() const { return CurrentTalkingObject != nullptr; }

	/** Get the dialogue box widget */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	FORCEINLINE UPUDialogueBox* GetDialogueBox() const { return DialogueBox; }
};
