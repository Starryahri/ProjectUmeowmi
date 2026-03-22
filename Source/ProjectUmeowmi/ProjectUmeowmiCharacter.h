// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture.h"
#include "Math/Box.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "GameplayTagContainer.h"
#include "Components/WidgetComponent.h"
#include "DlgSystem/DlgDialogueParticipant.h"
#include "Interfaces/PUInteractableInterface.h"
#include "DishCustomization/PUOrderBase.h"
#include "DishCustomization/PUDishPreviewComponent.h"
#include "ProjectUmeowmi/UI/PUScorecardWidget.h"
#include "ProjectUmeowmi/UI/PUDishScoringWidget.h"
#include "ProjectUmeowmi/UI/PUDialogueBox.h"
#include "ProjectUmeowmiCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UPUEmoteWidget;
struct FTimerHandle;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class ATalkingObject;
class UPUJournalWidget;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

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

	/** Cycle between overlapping interact targets (Space bar). Only active when 2+ talking objects overlap. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Config", meta = (AllowPrivateAccess = "true"))
	UInputAction* CycleInteractTargetAction;

	/** Open/Toggle Journal Input Action (Start button, I key) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Config", meta = (AllowPrivateAccess = "true"))
	UInputAction* OpenJournalAction;

	/** Cycle to previous dish in journal Recipes tab (LB / Left Bumper). Only active when journal is open on Recipes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Config", meta = (AllowPrivateAccess = "true"))
	UInputAction* JournalCycleDishPrevAction;

	/** Cycle to next dish in journal Recipes tab (RB / Right Bumper). Only active when journal is open on Recipes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Config", meta = (AllowPrivateAccess = "true"))
	UInputAction* JournalCycleDishNextAction;

	/** Hold to skip dialogue (fast typewriter, no sound, auto-advance). Only active when in dialogue. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Config", meta = (AllowPrivateAccess = "true"))
	UInputAction* SkipDialogueAction;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input Config", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config|Isometric", meta = (AllowPrivateAccess = "true"))
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

	/** When true, zoom/scroll does not change orthographic width. Use to lock the camera zoom. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config", meta = (AllowPrivateAccess = "true"))
	bool bLockOrthoWidth = false;

	/** When true, logs orthographic width to Output Log whenever you scroll/zoom. Use to find values for Min/MaxOrthoWidth. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Config", meta = (AllowPrivateAccess = "true"))
	bool bShowOrthoWidthDebug = false;

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
	/** List of talking objects currently in range (overlapping). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue and Interaction|Talking Object", meta = (AllowPrivateAccess = "true"))
	TArray<ATalkingObject*> OverlappingTalkingObjects;

	/** Index of the currently selected talking object in OverlappingTalkingObjects. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue and Interaction|Talking Object", meta = (AllowPrivateAccess = "true"))
	int32 SelectedTalkingObjectIndex = 0;

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

	/** Fallback class when restoring the dialogue box after scoring if no instance was present when entering scoring. Optional if DialogueBox is always set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue and Interaction|Dialogue Box", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UPUDialogueBox> DefaultDialogueBoxWidgetClass;

	/**
	 * Dialogue box layout used during dish scoring (portrait / scorecard layering). When entering dish scoring mode, the character
	 * swaps the active DialogueBox to this class if set; EndDishScoringMode restores the previous dialogue widget class.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue and Interaction|Dialogue Box", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UPUDialogueBox> ScoringDialogueBoxWidgetClass;

	/** Reference to the journal widget (assign in Blueprint if journal is in HUD). If unset, we search for it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue and Interaction|Journal", meta = (AllowPrivateAccess = "true"))
	UPUJournalWidget* JournalWidget;

	/** Current interactable object that can be interacted with */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dialogue and Interaction|Interactable", meta = (AllowPrivateAccess = "true"))
	TScriptInterface<IPUInteractableInterface> CurrentInteractable;


	////////////////////////////////////////////////////////////
	// Emote Configuration
	////////////////////////////////////////////////////////////
	/** Emote widget rendered above the player character. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Emote", meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* EmoteWidget;

	/** Master toggle for showing emotes above the player. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote", meta = (AllowPrivateAccess = "true"))
	bool bEnableEmotes = true;

	/** Widget class used to render emotes above the player. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote", meta = (AllowPrivateAccess = "true", EditCondition = "bEnableEmotes"))
	TSubclassOf<UPUEmoteWidget> EmoteWidgetClass;

	/** Space in which the emote widget is rendered (Screen or World). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote", meta = (AllowPrivateAccess = "true", EditCondition = "bEnableEmotes"))
	EWidgetSpace EmoteWidgetSpace = EWidgetSpace::Screen;

	/** Size of the emote widget in pixels (width x height). Affects both screen and world space. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote", meta = (AllowPrivateAccess = "true", EditCondition = "bEnableEmotes", ClampMin = "16", ClampMax = "512"))
	FVector2D EmoteDrawSize = FVector2D(128.0f, 128.0f);

	/** Scale multiplier for the emote widget (applied to the component). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote", meta = (AllowPrivateAccess = "true", EditCondition = "bEnableEmotes", ClampMin = "0.25", ClampMax = "4.0"))
	float EmoteScale = 1.0f;

	/** Data table mapping gameplay tags to emote data (icon, duration, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote", meta = (AllowPrivateAccess = "true", EditCondition = "bEnableEmotes"))
	UDataTable* EmoteDataTable = nullptr;


	////////////////////////////////////////////////////////////
	// Dish Preview (above head when carrying a dish)
	////////////////////////////////////////////////////////////
	/** 3D preview of plated dish shown above character when carrying a dish to give. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Preview", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPUDishPreviewComponent> DishPreviewComponent;

	/** Dish mesh for preview - created here (not in DishPreviewComponent) to avoid template/instance attachment mismatch in Blueprint subclasses. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Preview", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> DishPreviewMeshComponent;


    // IDlgDialogueParticipant Interface
	FName GetParticipantName_Implementation() const override { return ParticipantName; }
    FText GetParticipantDisplayName_Implementation(FName ActiveSpeaker) const override { return DisplayName; }
    UTexture2D* GetParticipantIcon_Implementation(FName ActiveSpeaker, FName ActiveSpeakerState) const override { return ParticipantIcon; }

public:
	AProjectUmeowmiCharacter();

	////////////////////////////////////////////////////////////
	// Dish capture (scorecard: station snapshot first, then head preview fallback)
	////////////////////////////////////////////////////////////
	/** When false, scorecard uses the static dish PreviewTexture from data (previous behavior). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order System|Dish Capture")
	bool bEnableDishCaptureForScorecard = true;

	/** Pixel size (square) of the baked dish snapshot for the scorecard. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order System|Dish Capture", meta = (ClampMin = "128", ClampMax = "2048"))
	int32 DishCaptureSize = 512;

	/**
	 * When true: each capture recomputes the Scene Capture transform.
	 * Plating station: same view direction as the plating camera (ray from dish bounds center through the plating camera) but distance from merged bounds (not the plating camera's world position), and fixed capture FOV 35 — so identical dish bounds give consistent on-screen scale.
	 * Fallback: frame from actor forward + merged bounds (head preview path).
	 * When false (default): the Dish Capture (Scene Capture 2D) component is not moved — use its transform and projection as you place it on the character (viewport / Blueprint).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order System|Dish Capture")
	bool bUseAutomaticDishCaptureFraming = false;

	/**
	 * Applied along the capture's view axes after framing (world offsets): X = camera right, Y = view forward, Z = camera up.
	 * When not using automatic framing, the component is reset to its authored pose first, then this is added.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order System|Dish Capture")
	FVector DishCaptureCameraLocalOffset = FVector::ZeroVector;

	/**
	 * Automatic bounds framing only (ConfigureDishCaptureCameraFromWorldBounds): camera position uses world Up * max(MaxExtent * ExtentScale, MinLiftUU).
	 * Larger values raise the eye above the dish center so look-at tilts down (more "into the bowl") without pitch-after-aim.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order System|Dish Capture", meta = (ClampMin = "0.0"))
	float DishCaptureCameraVerticalLiftExtentScale = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order System|Dish Capture", meta = (ClampMin = "0.0"))
	float DishCaptureCameraVerticalLiftMinUU = 40.f;

	/**
	 * Added to each axis of merged world bounds half-extents before snap (absorbs tiny CalcBounds / animation jitter).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order System|Dish Capture", meta = (ClampMin = "0.0"))
	float DishCaptureBoundsPaddingUU = 2.f;

	/**
	 * If > 0, each axis half-extent (after padding) is ceil-snapped to this grid in uu so camera distance tiers stay stable across captures.
	 * Set to 0 to disable snapping (only padding applies).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order System|Dish Capture", meta = (ClampMin = "0.0"))
	float DishCaptureBoundsExtentSnapUU = 1.f;

	/**
	 * When true, the dish scene capture runs the post-processing pipeline (tonemapper / exposure), closer to the main camera.
	 * When false, PostProcessing is off and SceneColorHDR can look darker than the game view.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order System|Dish Capture")
	bool bDishCapturePostProcessingTone = true;

	/** Blend weight for PostProcessSettings during capture (only if bDishCapturePostProcessingTone). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order System|Dish Capture", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bDishCapturePostProcessingTone"))
	float DishCapturePostProcessBlendWeightForCapture = 1.f;

	/** Extra exposure compensation (stops) for the capture only. Applied via PostProcessSettings override while capturing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order System|Dish Capture", meta = (EditCondition = "bDishCapturePostProcessingTone", ClampMin = "-4.0", ClampMax = "4.0"))
	float DishCaptureExposureBias = 0.5f;

	/**
	 * Extra pitch (degrees) applied in camera local space immediately after aim-at-bounds-center.
	 * Default 0: automatic framing already lifts the camera above the dish (ConfigureDishCaptureCameraFromWorldBounds) so look-at tilts down naturally.
	 * Use non-zero only if you need more tilt on top of that (e.g. plating-camera match with no lift).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order System|Dish Capture", meta = (ClampMin = "-89.0", ClampMax = "89.0"))
	float DishCaptureCameraPitchAfterAimDegrees = 0.f;

	/**
	 * Local-space Euler applied after aim-at-dish: world rotation = Quat(current) * Quat(this) (same as multiplying delta in component space after framing).
	 * Pitch/Yaw/Roll follow the default FRotator → FQuat convention.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order System|Dish Capture")
	FRotator DishCaptureCameraLocalRotation = FRotator::ZeroRotator;

	/**
	 * Rotates the head-preview dish root (DishPreview) only while the scorecard RT is captured, then restores.
	 * Combined with ComposeRotators(Saved, Offset) — first base pose, then offset (matches Blueprint Combine Rotators).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order System|Dish Capture")
	FRotator DishCapturePreviewMeshRotationOffset = FRotator::ZeroRotator;

	/** When true, refreshes the scorecard render target every frame from the head-preview dish. Hold Up/Down to adjust pitch in real time (see below). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order System|Dish Capture")
	bool bDishCaptureLivePreview = false;

	/** While live preview is on, hold keyboard Up/Down to change DishCaptureCameraPitchAfterAimDegrees (degrees per second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order System|Dish Capture", meta = (EditCondition = "bDishCaptureLivePreview"))
	bool bDishCaptureLivePreviewPitchKeys = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order System|Dish Capture", meta = (EditCondition = "bDishCaptureLivePreview && bDishCaptureLivePreviewPitchKeys"))
	float DishCaptureLivePreviewPitchDegreesPerSecond = 45.f;

	/** Draw Pitch/Yaw/Roll in the corner while live preview is active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Order System|Dish Capture", meta = (EditCondition = "bDishCaptureLivePreview"))
	bool bDishCaptureLivePreviewShowPitchOnScreen = true;

	/**
	 * Native scene capture (also listed in the Components panel as "DishCapture", Scene Capture 2D).
	 * Attached to the capsule root; used only when capturing the head dish for the scorecard.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Order System|Dish Capture", meta = (DisplayName = "Dish Capture (Scorecard)"))
	TObjectPtr<USceneCaptureComponent2D> DishCaptureComponent;

	/** Captures the live plated dish at the station (show-only) right after plating transforms are saved; used for the scorecard. */
	void CaptureDishSnapshotFromPlatingStation(class UPUDishCustomizationComponent* CustomizationComponent);

	/** Renders the head-preview dish into DishCaptureRenderTarget (same path as scorecard fallback). Use with bDishCaptureLivePreview or from Blueprint when tweaking offsets/rotation. */
	UFUNCTION(BlueprintCallable, Category = "Order System|Dish Capture")
	bool RefreshDishCapturePreviewFromDishPreview();

	void GetCameraPositionIndex(const FInputActionValue& Value);
	void ToggleGridMovement(const FInputActionValue& Value);
	void ZoomCamera(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);
	void ToggleJournal(const FInputActionValue& Value);
	void OnJournalCycleDishPrev(const FInputActionValue& Value);
	void OnJournalCycleDishNext(const FInputActionValue& Value);
	void OnSkipDialogueStarted(const FInputActionValue& Value);
	void OnSkipDialogueCompleted(const FInputActionValue& Value);
	
	/** Initialize camera position based on the starting index */
	void InitializeCameraPosition();
	
	/** Blueprint callable function to initialize camera position */
	UFUNCTION(BlueprintCallable, Category = "Camera Config|Isometric")
	void InitializeCameraPositionFromBlueprint();
	
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
	FORCEINLINE int32 GetNumberOfCameraPositions() const { return NumberOfCameraPositions; }
	
	// Input action getters
	FORCEINLINE UInputAction* GetZoomAction() const { return ZoomAction; }
	FORCEINLINE UInputAction* GetMoveAction() const { return MoveAction; }
	FORCEINLINE UInputAction* GetLookAction() const { return LookAction; }
	FORCEINLINE UInputAction* GetInteractAction() const { return InteractAction; }
	FORCEINLINE UInputAction* GetOpenJournalAction() const { return OpenJournalAction; }
	FORCEINLINE UInputAction* GetToggleGridMovementAction() const { return ToggleGridMovementAction; }
	FORCEINLINE UInputAction* GetJumpAction() const { return JumpAction; }
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
	virtual void BeginPlay() override;
	
	virtual void NotifyControllerChanged() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
	bool HasTalkingObjectAvailable() const { return GetCurrentTalkingObject() != nullptr; }

	/** Number of overlapping talking objects. Use to show "Press Space to switch" when >= 2. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Interaction")
	int32 GetOverlappingTalkingObjectCount() const { return OverlappingTalkingObjects.Num(); }

	/** Index of the selected talking object (0-based). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Interaction")
	int32 GetSelectedTalkingObjectIndex() const { return SelectedTalkingObjectIndex; }

	// Emote API
	/** Show an emote above the player, using EmoteDataTable to resolve the tag into an icon and optional extras. */
	UFUNCTION(BlueprintCallable, Category = "Emote")
	void ShowEmoteByTag(FGameplayTag EmoteTag);

	/** Clear any active emote immediately. */
	UFUNCTION(BlueprintCallable, Category = "Emote")
	void ClearEmote();

	/** Returns true if an emote is currently visible. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Emote")
	bool IsEmoteActive() const;

	/** Get the dish preview component (for showing plated dish above head). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dish Preview")
	UPUDishPreviewComponent* GetDishPreviewComponent() const { return DishPreviewComponent; }

	/** Get the dialogue box widget */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	FORCEINLINE UPUDialogueBox* GetDialogueBox() const { return DialogueBox; }

	/** Get the current talking object (selected from overlapping list). */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	ATalkingObject* GetCurrentTalkingObject() const;

	/** Cycle to the next/previous overlapping talking object. Call when CycleInteractTargetAction is pressed. */
	void CycleInteractTarget(const FInputActionValue& Value);

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

	/** Reveals a hint for the current order if the player has an active order and the aspect exists in TargetAspects. Call from dialogue (e.g. RevealHint_Salt). */
	UFUNCTION(BlueprintCallable, Category = "Order System")
	void RevealHintOnCurrentOrder(FName AspectName);

	UFUNCTION(BlueprintCallable, Category = "Order System")
	void SetOrderResult(bool bCompleted, float SatisfactionScore);

	UFUNCTION(BlueprintCallable, Category = "Order System")
	bool GetOrderCompleted() const { return bCurrentOrderCompleted; }

	UFUNCTION(BlueprintCallable, Category = "Order System")
	float GetOrderSatisfaction() const { return CurrentOrderSatisfaction; }

	UFUNCTION(BlueprintCallable, Category = "Order System")
	bool IsCurrentOrderCompleted() const { return bCurrentOrderCompleted; }

	// Order feedback and results
	UFUNCTION(BlueprintCallable, Category = "Order System")
	void DisplayOrderResult();

	UFUNCTION(BlueprintCallable, Category = "Order System")
	void ClearCompletedOrder();

	UFUNCTION(BlueprintCallable, Category = "Order System")
	FText GetOrderResultText() const;

	// Order completion events (for Blueprint use)
	UFUNCTION(BlueprintCallable, Category = "Order System")
	void OnOrderCompleted();

	UFUNCTION(BlueprintCallable, Category = "Order System")
	void OnOrderFailed();

	/**
	 * Populate the given scorecard with data from the current completed order (dish image, seal, aspects, etc.).
	 * Does not create the widget or add it to the viewport — create/add your widget first, then call this.
	 * When dish capture fallback runs, ShowFromOrder may execute on the next tick after capture completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Order System", meta = (DisplayName = "Show Scorecard"))
	class UPUScorecardWidget* ShowScorecard(class UPUScorecardWidget* ScorecardWidget);

	////////////////////////////////////////////////////////////
	// Dish scoring mode
	////////////////////////////////////////////////////////////
	/** Default dish scoring widget class (set on the character Blueprint). Used by BeginDishScoringModeFromClass / TryBeginDishScoringModeFromClass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Scoring")
	TSubclassOf<UPUDishScoringWidget> DishScoringWidgetClass;

	/**
	 * Creates the widget from DishScoringWidgetClass and enters dish scoring mode.
	 * Void return so this appears in Dlg "Unreal Function" events (the picker only lists functions with no parameters and no return value).
	 */
	UFUNCTION(BlueprintCallable, Category = "Dish Scoring")
	void BeginDishScoringModeFromClass();

	/**
	 * Same as BeginDishScoringModeFromClass but returns whether scoring mode is now active (false if class unset, no PC, or CreateWidget failed).
	 */
	UFUNCTION(BlueprintCallable, Category = "Dish Scoring")
	bool TryBeginDishScoringModeFromClass();

	/**
	 * Enter dish scoring mode: associates the widget with this character, adds it to the viewport if needed, and calls OnEnteredDishScoringMode on the widget.
	 * If another scoring widget is already active, it is closed first.
	 */
	UFUNCTION(BlueprintCallable, Category = "Dish Scoring")
	bool BeginDishScoringModeWithWidget(UPUDishScoringWidget* DishScoringWidget);

	/** Ends dish scoring: OnExitingDishScoringMode, remove widget, restore default dialogue layout. Dialogue event "EndDishScoring" calls this too. */
	UFUNCTION(BlueprintCallable, Category = "Dish Scoring")
	void EndDishScoringMode();

	UFUNCTION(BlueprintCallable, Category = "Dish Scoring")
	bool IsInDishScoringMode() const;

	// Order System Storage
	UPROPERTY(BlueprintReadWrite, Category = "Order System")
	FPUOrderBase CurrentOrder;

	UPROPERTY(BlueprintReadWrite, Category = "Order System")
	bool bHasCurrentOrder = false;

	UPROPERTY(BlueprintReadWrite, Category = "Order System")
	bool bCurrentOrderCompleted = false;

	UPROPERTY(BlueprintReadWrite, Category = "Order System")
	float CurrentOrderSatisfaction = 0.0f;

private:
	// Helper function to clean up UObject references in orders
	void CleanupOrderUObjectReferences(FPUOrderBase& Order);

	void EnsureDishCaptureRenderTarget();
	void ConfigureDishCaptureCamera();
	void ConfigureDishCaptureCameraFromWorldBounds(const FBox& InWorldBounds, const FVector& FallbackCenter);
	void ConfigureDishCaptureCameraToMatchPlatingCamera(class UCameraComponent* PlatingCamera, const FBox& InWorldBoundsFallback, const FVector& FallbackCenter);
	/** Applies DishCaptureCameraLocalOffset / DishCaptureCameraLocalRotation after base placement.
	 *  Aims the capture at DishFocusWorld from its current location (after Configure / manual reset), same convention as ConfigureDishCaptureCameraFromWorldBounds,
	 *  so pitch/yaw/roll tweaks tilt relative to the dish in both manual and automatic framing. */
	void ApplyDishCaptureCameraTweaks(const FVector& DishFocusWorld);
	void PopulateScorecardWidget(class UPUScorecardWidget* ScorecardWidget, class UTexture* OptionalDishTexture);

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> DishCaptureRenderTarget;

	/** Last dish texture passed to the scorecard (scene capture RT or preview texture). Keeps GC refs. */
	UPROPERTY()
	TObjectPtr<UTexture> LastDishCaptureTexture;

	/** Pending dish for scorecard: usually DishCaptureRenderTarget (live RT for material "DishRender"). Cleared when scorecard opens. */
	UPROPERTY()
	TObjectPtr<UTexture> PendingScorecardDishTexture;

	/** True after CaptureDishSnapshotFromPlatingStation succeeds for this order. ShowScorecard reuses DishCaptureRenderTarget instead of RefreshDishCapturePreviewFromDishPreview (different framing). Cleared on new order / clear order. */
	bool bStationDishCaptureValidForScorecard = false;

	/** First time we use manual Dish Capture framing, we store relative transform so offsets don't accumulate each capture. */
	UPROPERTY(Transient)
	bool bDishCaptureRelativeBaseCaptured = false;

	/** Relative transform of DishCapture when manual base was snapshotted (authored placement). */
	UPROPERTY(Transient)
	FTransform DishCaptureRelativeBaseAtStart;

	/** Active dish scoring root widget while in scoring mode (Transient — not saved). */
	UPROPERTY(Transient)
	TObjectPtr<UPUDishScoringWidget> ActiveDishScoringWidget;

	/** Dialogue box class to restore after dish scoring (captured when swapping to scoring layout). */
	UPROPERTY(Transient)
	TSubclassOf<UPUDialogueBox> CachedNonScoringDialogueBoxClass;

	/** True while DialogueBox is the scoring-layout instance. */
	bool bUsingScoringDialogueBox = false;

	/** Removes the active dish scoring widget only (does not restore dialogue). Used when replacing one scoring widget with another. */
	void RemoveActiveDishScoringWidgetFromViewport();

	void SwapToScoringDialogueBox();
	void RestoreNonScoringDialogueBox();

	/** Called when emote duration expires; plays fade-out then clears after animation. */
	void BeginFadeOutEmote();

	FTimerHandle EmoteHideTimerHandle;
	FTimerHandle EmoteFadeOutTimerHandle;
	FGameplayTag ActiveEmoteTag;

};
