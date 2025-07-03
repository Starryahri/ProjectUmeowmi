// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "DlgSystem/DlgDialogueParticipant.h"
#include "Interfaces/PUInteractableInterface.h"
#include "DishCustomization/PUOrderBase.h"
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

	//Todo: Add input for cancel action
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Config", meta = (AllowPrivateAccess = "true"))
	// UInputAction* CancelAction;


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

	/** Current interactable object that can be interacted with */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue and Interaction|Interactable", meta = (AllowPrivateAccess = "true"))
	TScriptInterface<IPUInteractableInterface> CurrentInteractable;



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
	
	// Public wrappers for protected functions
	void HandleMove(const FInputActionValue& Value) { Move(Value); }
	void HandleLook(const FInputActionValue& Value) { Look(Value); }
	
	/** Called every frame to update camera position */
	virtual void Tick(float DeltaTime) override;
	
	// Camera getters
	FORCEINLINE float GetCameraOffset() const { return CameraOffset; }
	FORCEINLINE int32 GetCameraPositionIndex() const { return CameraPositionIndex; }
	FORCEINLINE UInputAction* GetRotateCameraAction() const { return RotateCameraAction; }
	FORCEINLINE void SetCameraOffset(float NewOffset) { CameraOffset = NewOffset; }
	FORCEINLINE void SetCameraPositionIndex(int32 NewIndex) { CameraPositionIndex = NewIndex; }
	
	// Input action getters
	FORCEINLINE UInputAction* GetZoomAction() const { return ZoomAction; }
	FORCEINLINE UInputAction* GetMoveAction() const { return MoveAction; }
	FORCEINLINE UInputAction* GetLookAction() const { return LookAction; }
	FORCEINLINE UInputAction* GetInteractAction() const { return InteractAction; }
	FORCEINLINE UInputAction* GetToggleGridMovementAction() const { return ToggleGridMovementAction; }
	FORCEINLINE UInputMappingContext* GetDefaultMappingContext() const { return DefaultMappingContext; }
	
	// Mouse visibility control
	void ShowMouseCursor();
	void HideMouseCursor();
	void SetMousePosition(int32 X, int32 Y);
	void CenterMouseCursor();
	
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

	// Interaction event handlers
	UFUNCTION()
	void OnInteractionStarted();

	UFUNCTION()
	void OnInteractionEnded();

	UFUNCTION()
	void OnInteractionFailed();

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

	/** Get the current talking object */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	ATalkingObject* GetCurrentTalkingObject() const { return CurrentTalkingObject; }

	void RegisterInteractable(TScriptInterface<IPUInteractableInterface> Interactable);
	void UnregisterInteractable(TScriptInterface<IPUInteractableInterface> Interactable);
	bool HasInteractableAvailable() const { return CurrentInteractable != nullptr; }

	// Order System Integration
	UFUNCTION(BlueprintCallable, Category = "Order System")
	void SetCurrentOrder(const FPUOrderBase& Order);

	UFUNCTION(BlueprintCallable, Category = "Order System")
	const FPUOrderBase& GetCurrentOrder() const { return CurrentOrder; }

	UFUNCTION(BlueprintCallable, Category = "Order System")
	bool HasCurrentOrder() const { return bHasCurrentOrder; }

	UFUNCTION(BlueprintCallable, Category = "Order System")
	void ClearCurrentOrder();

	UFUNCTION(BlueprintCallable, Category = "Order System")
	void SetOrderResult(bool bCompleted, float SatisfactionScore);

	UFUNCTION(BlueprintCallable, Category = "Order System")
	bool GetOrderCompleted() const { return bCurrentOrderCompleted; }

	UFUNCTION(BlueprintCallable, Category = "Order System")
	float GetOrderSatisfaction() const { return CurrentOrderSatisfaction; }

	// Order System Storage
	UPROPERTY(BlueprintReadWrite, Category = "Order System")
	FPUOrderBase CurrentOrder;

	UPROPERTY(BlueprintReadWrite, Category = "Order System")
	bool bHasCurrentOrder = false;

	UPROPERTY(BlueprintReadWrite, Category = "Order System")
	bool bCurrentOrderCompleted = false;

	UPROPERTY(BlueprintReadWrite, Category = "Order System")
	float CurrentOrderSatisfaction = 0.0f;


};
