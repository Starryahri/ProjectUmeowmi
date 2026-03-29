// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectUmeowmiCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Engine/DataTable.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Dialogue/TalkingObject.h"
#include "DlgSystem/DlgContext.h"
#include "DishCustomization/PUDishCustomizationComponent.h"
#include "UI/PUDialogueBox.h"
#include "UI/PUEmoteData.h"
#include "UI/PUEmoteWidget.h"
#include "UI/PUJournalWidget.h"
#include "UI/PUQuestObjectiveOffscreenIndicatorWidget.h"
#include "ProjectUmeowmi/UI/PUScorecardWidget.h"
#include "ProjectUmeowmi/UI/PUDishScoringWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Components/Widget.h"
#include "Components/PanelWidget.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/Texture.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/EngineTypes.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "RenderingThread.h"
#include "Components/SceneCaptureComponent.h"
#include "Components/PrimitiveComponent.h"
#include "InputCoreTypes.h"
#include "Engine/Engine.h"

#include "Interfaces/PUInteractableInterface.h"

namespace
{
	/** First UPUScorecardWidget under a dish scoring UserWidget (embedded in layout). */
	UPUScorecardWidget* FindFirstScorecardWidgetRecursive(UWidget* W)
	{
		if (!IsValid(W))
		{
			return nullptr;
		}
		if (UPUScorecardWidget* Found = Cast<UPUScorecardWidget>(W))
		{
			return Found;
		}
		if (UUserWidget* UW = Cast<UUserWidget>(W))
		{
			if (UPUScorecardWidget* Found = FindFirstScorecardWidgetRecursive(UW->GetRootWidget()))
			{
				return Found;
			}
		}
		if (UPanelWidget* Panel = Cast<UPanelWidget>(W))
		{
			const int32 N = Panel->GetChildrenCount();
			for (int32 i = 0; i < N; ++i)
			{
				if (UPUScorecardWidget* Found = FindFirstScorecardWidgetRecursive(Panel->GetChildAt(i)))
				{
					return Found;
				}
			}
		}
		return nullptr;
	}
}

// Output Log filter: search for [PUDialogueScoring] (dish scoring + dialogue viewport swap / refresh).
namespace PUDialogueScoringLog
{
	static constexpr const TCHAR* Tag = TEXT("[PUDialogueScoring]");
}

namespace
{
	/** Scene capture still draws sky/reflections unless these flags are cleared; keeps scorecard shots to dish geometry + lighting only. */
	void ApplyDishOnlyCaptureShowFlags(USceneCaptureComponent2D* Capture, bool bPostProcessingForTone)
	{
		if (!Capture)
		{
			return;
		}
		FEngineShowFlags& SF = Capture->ShowFlags;
		SF.SetAtmosphere(false);
		SF.SetFog(false);
		SF.SetVolumetricFog(false);
		SF.SetSkyLighting(false);
		SF.SetAmbientCubemap(false);
		SF.SetBloom(false);
		SF.SetReflectionEnvironment(false);
		SF.SetScreenSpaceReflections(false);
		SF.SetLumenReflections(false);
		SF.SetLumenGlobalIllumination(false);
		// When true: tonemapper + exposure (matches main view brightness better). When false: raw HDR can look darker.
		SF.SetPostProcessing(bPostProcessingForTone);
	}

	struct FDishCaptureSavedPostProcess
	{
		float PostProcessBlendWeight = 0.f;
		bool bOverride_AutoExposureBias = false;
		float AutoExposureBias = 0.f;
	};

	FDishCaptureSavedPostProcess SaveDishCapturePostProcess(USceneCaptureComponent2D* Capture)
	{
		FDishCaptureSavedPostProcess S;
		if (Capture)
		{
			S.PostProcessBlendWeight = Capture->PostProcessBlendWeight;
			S.bOverride_AutoExposureBias = Capture->PostProcessSettings.bOverride_AutoExposureBias != 0;
			S.AutoExposureBias = Capture->PostProcessSettings.AutoExposureBias;
		}
		return S;
	}

	void RestoreDishCapturePostProcess(USceneCaptureComponent2D* Capture, const FDishCaptureSavedPostProcess& S)
	{
		if (!Capture)
		{
			return;
		}
		Capture->PostProcessBlendWeight = S.PostProcessBlendWeight;
		Capture->PostProcessSettings.bOverride_AutoExposureBias = S.bOverride_AutoExposureBias;
		Capture->PostProcessSettings.AutoExposureBias = S.AutoExposureBias;
	}

	void ApplyDishCaptureCapturePostProcess(USceneCaptureComponent2D* Capture, bool bTone, float BlendWeightForCapture, float ExposureBias)
	{
		if (!Capture)
		{
			return;
		}
		if (bTone)
		{
			Capture->PostProcessBlendWeight = FMath::Clamp(BlendWeightForCapture, 0.f, 1.f);
			Capture->PostProcessSettings.bOverride_AutoExposureBias = true;
			Capture->PostProcessSettings.AutoExposureBias = ExposureBias;
		}
		else
		{
			Capture->PostProcessBlendWeight = 0.f;
		}
	}

	// Same multipliers as ConfigureDishCaptureCameraFromWorldBounds — single source for consistent scale.
	static constexpr float DishCaptureDistanceExtentMultiplier = 3.5f;
	static constexpr float DishCaptureMinCameraDistanceUU = 100.f;
	static constexpr float DishCaptureFallbackExtentUU = 40.f;

	static void GetDishCaptureBoundsCenterAndExtent(const FBox& InWorldBounds, const FVector& FallbackCenter, FVector& OutCenter, FVector& OutExtent)
	{
		if (InWorldBounds.IsValid != 0)
		{
			InWorldBounds.GetCenterAndExtents(OutCenter, OutExtent);
		}
		else
		{
			OutCenter = FallbackCenter;
			OutExtent = FVector(DishCaptureFallbackExtentUU, DishCaptureFallbackExtentUU, DishCaptureFallbackExtentUU);
		}
	}

	static void ComputeDishCaptureDistanceAndLift(
		float MaxExtent,
		float VerticalLiftExtentScale,
		float VerticalLiftMinUU,
		float& OutDistance,
		float& OutVerticalLift)
	{
		OutDistance = FMath::Max(MaxExtent * DishCaptureDistanceExtentMultiplier, DishCaptureMinCameraDistanceUU);
		OutVerticalLift = FMath::Max(MaxExtent * VerticalLiftExtentScale, VerticalLiftMinUU);
	}

	/** Stabilizes framing distance: camera uses max extent; tiny bounds jitter maps to the same snap bucket. */
	static FBox SnapDishCaptureWorldBounds(const FBox& In, float PaddingUU, float SnapUU)
	{
		if (In.IsValid == 0)
		{
			return In;
		}
		FVector Center, Extent;
		In.GetCenterAndExtents(Center, Extent);
		const float Px = Extent.X + PaddingUU;
		const float Py = Extent.Y + PaddingUU;
		const float Pz = Extent.Z + PaddingUU;
		const float Sx = SnapUU > KINDA_SMALL_NUMBER ? FMath::CeilToFloat(Px / SnapUU) * SnapUU : Px;
		const float Sy = SnapUU > KINDA_SMALL_NUMBER ? FMath::CeilToFloat(Py / SnapUU) * SnapUU : Py;
		const float Sz = SnapUU > KINDA_SMALL_NUMBER ? FMath::CeilToFloat(Pz / SnapUU) * SnapUU : Pz;
		const FVector ExtentOut(Sx, Sy, Sz);
		return FBox(Center - ExtentOut, Center + ExtentOut);
	}
}

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AProjectUmeowmiCharacter

AProjectUmeowmiCharacter::AProjectUmeowmiCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Create emote widget (above character head) - do NOT override Space; user sets Screen in Blueprint
	EmoteWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("EmoteWidget"));
	EmoteWidget->SetupAttachment(RootComponent);
	EmoteWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f)); // Above character head
	EmoteWidget->SetVisibility(false); // Hidden until ShowEmoteByTag

	// Create dish preview (above character head when carrying a dish)
	DishPreviewComponent = CreateDefaultSubobject<UPUDishPreviewComponent>(TEXT("DishPreview"));
	DishPreviewComponent->SetupAttachment(RootComponent);
	DishPreviewComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f)); // Above character head

	// Dish mesh created here (not in DishPreviewComponent) to avoid template/instance attachment mismatch in Blueprint subclasses (BP_Bao)
	DishPreviewMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DishPreviewMesh"));
	DishPreviewMeshComponent->SetupAttachment(DishPreviewComponent);
	DishPreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DishPreviewMeshComponent->SetCastShadow(true);
	DishPreviewMeshComponent->SetVisibility(false);
	DishPreviewComponent->SetDishMeshComponent(DishPreviewMeshComponent);

	DishCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("DishCapture"));
	DishCaptureComponent->SetupAttachment(RootComponent);
	DishCaptureComponent->bCaptureEveryFrame = false;
	DishCaptureComponent->bCaptureOnMovement = false;
	DishCaptureComponent->SetAutoActivate(false);
	DishCaptureComponent->ProjectionType = ECameraProjectionMode::Perspective;
	DishCaptureComponent->FOVAngle = 35.f;
	DishCaptureComponent->bAlwaysPersistRenderingState = true;
	// Default USceneCaptureComponent2D has bAutoCalculateOrthoPlanes=true. With ortho plating cameras, the engine
	// recomputes near/far from camera tilt + owner view-target (see FMinimalViewInfo::AutoCalculateOrthoPlanes);
	// that makes pitch/yaw read as sliding the frame (e.g. "moves down") instead of rotating the view.
	DishCaptureComponent->bAutoCalculateOrthoPlanes = false;
	// SceneColor (HDR): RGB scene + alpha useful for transparency; pair with RTF_RGBA16f + transparent clear.
	DishCaptureComponent->CaptureSource = SCS_SceneColorHDR;
	DishCaptureComponent->PostProcessBlendWeight = 0.f;
#if WITH_EDITORONLY_DATA
	// Editor-only: small sprite in viewport (member does not exist in non-editor builds).
	DishCaptureComponent->bVisualizeComponent = true;
#endif

	// Initialize target camera rotation
	TargetCameraRotation = FRotator(-15.0f, 45.0f, 0.0f);

	// Always show mouse cursor
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;
		PC->CurrentMouseCursor = EMouseCursor::Default;
	}

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AProjectUmeowmiCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Initialize the camera position based on the starting index
	InitializeCameraPosition();

	// Configure emote widget - set class; Draw Size comes from component in Blueprint
	// Must set bDrawAtDesiredSize=false or widget's desired size (e.g. 256) overrides component's Draw Size
	if (EmoteWidget && bEnableEmotes && EmoteWidgetClass)
	{
		EmoteWidget->SetWidgetClass(EmoteWidgetClass);
		EmoteWidget->SetDrawAtDesiredSize(false);
	}

	TryCreateQuestObjectiveOffscreenIndicator();

	//UE_LOG(LogTemp,Log, TEXT("Character BeginPlay - Camera initialized with position index: %d"), CameraPositionIndex);
}

void AProjectUmeowmiCharacter::TryCreateQuestObjectiveOffscreenIndicator()
{
	if (!bEnableQuestObjectiveOffscreenIndicator || QuestObjectiveOffscreenIndicator)
	{
		return;
	}
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		const TSubclassOf<UPUQuestObjectiveOffscreenIndicatorWidget> WidgetClass =
			QuestObjectiveOffscreenIndicatorClass ? QuestObjectiveOffscreenIndicatorClass.Get() : UPUQuestObjectiveOffscreenIndicatorWidget::StaticClass();
		QuestObjectiveOffscreenIndicator = CreateWidget<UPUQuestObjectiveOffscreenIndicatorWidget>(PC, WidgetClass);
		if (QuestObjectiveOffscreenIndicator)
		{
			QuestObjectiveOffscreenIndicator->AddToViewport(25);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AProjectUmeowmiCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	TryCreateQuestObjectiveOffscreenIndicator();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		//UE_LOG(LogTemp,Log, TEXT("Controller changed - PlayerController found"));
		
		// Always show mouse cursor
		PlayerController->bShowMouseCursor = true;
		PlayerController->CurrentMouseCursor = EMouseCursor::Default;
		
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			//UE_LOG(LogTemp,Log, TEXT("Enhanced Input Subsystem found"));
			
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
				//UE_LOG(LogTemp,Log, TEXT("Default mapping context added successfully"));
			}
			else
			{
				//UE_LOG(LogTemp,Warning, TEXT("Default mapping context is null!"));
			}
		}
		else
		{
			//UE_LOG(LogTemp,Warning, TEXT("Enhanced Input Subsystem not found!"));
		}
	}
	else
	{
		//UE_LOG(LogTemp,Warning, TEXT("Controller changed - No PlayerController found!"));
	}
}

void AProjectUmeowmiCharacter::PushJournalInputMappingLayer()
{
	if (!JournalMappingContext || bJournalInputLayerActive)
	{
		return;
	}
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(JournalMappingContext, JournalMappingContextPriority);
			bJournalInputLayerActive = true;
		}
	}
}

void AProjectUmeowmiCharacter::PopJournalInputMappingLayer()
{
	if (!JournalMappingContext || !bJournalInputLayerActive)
	{
		return;
	}
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(JournalMappingContext);
		}
	}
	bJournalInputLayerActive = false;
}

void AProjectUmeowmiCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::Look);

		// Rotating camera
		EnhancedInputComponent->BindAction(RotateCameraAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::GetCameraPositionIndex);

		// Zooming camera
		EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::ZoomCamera);

		// Toggle grid movement
		EnhancedInputComponent->BindAction(ToggleGridMovementAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::ToggleGridMovement);

		// Interact with talking objects
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::Interact);

		// Cycle between overlapping interact targets (Space bar)
		if (CycleInteractTargetAction)
		{
			EnhancedInputComponent->BindAction(CycleInteractTargetAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::CycleInteractTarget);
		}

		// Open/toggle journal (Start button, I key)
		if (OpenJournalAction)
		{
			EnhancedInputComponent->BindAction(OpenJournalAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::ToggleJournal);
		}

		// Journal Recipes tab: cycle dishes with bumpers (only when journal open on Recipes)
		if (JournalCycleDishPrevAction)
		{
			EnhancedInputComponent->BindAction(JournalCycleDishPrevAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::OnJournalCycleDishPrev);
		}
		if (JournalCycleDishNextAction)
		{
			EnhancedInputComponent->BindAction(JournalCycleDishNextAction, ETriggerEvent::Triggered, this, &AProjectUmeowmiCharacter::OnJournalCycleDishNext);
		}

		// Hold to skip dialogue (fast typewriter, no sound, auto-advance)
		if (SkipDialogueAction)
		{
			EnhancedInputComponent->BindAction(SkipDialogueAction, ETriggerEvent::Started, this, &AProjectUmeowmiCharacter::OnSkipDialogueStarted);
			EnhancedInputComponent->BindAction(SkipDialogueAction, ETriggerEvent::Completed, this, &AProjectUmeowmiCharacter::OnSkipDialogueCompleted);
		}

		// Jump
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AProjectUmeowmiCharacter::Move(const FInputActionValue& Value)
{
	// Block movement when journal is open
	UPUJournalWidget* Journal = JournalWidget;
	if (!Journal)
	{
		TArray<UUserWidget*> FoundWidgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), FoundWidgets, UPUJournalWidget::StaticClass(), false);
		for (UUserWidget* W : FoundWidgets)
		{
			if (UPUJournalWidget* J = Cast<UPUJournalWidget>(W))
			{
				Journal = J;
				break;
			}
		}
	}
	if (Journal && Journal->GetVisibility() == ESlateVisibility::Visible)
	{
		return;
	}

	// Get the input value
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// Use camera angle directly for movement direction, independent of character rotation
		const FRotator YawRotation(0, CameraOffset, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		if (bUseGridMovement)
		{
			// Only process new movement input if we're not already moving to a grid position
			if (!bIsMovingToGrid)
			{
				// For grid movement, we only move in cardinal directions
				// Determine which direction has the larger input
				if (FMath::Abs(MovementVector.X) > FMath::Abs(MovementVector.Y))
				{
					// Move horizontally
					FVector CurrentLocation = GetActorLocation();
					TargetGridPosition = CurrentLocation + RightDirection * GridSize * FMath::Sign(MovementVector.X);
					TargetGridPosition = SnapToGrid(TargetGridPosition);
					
					// Set target rotation based on movement direction
					TargetRotation = FRotator(0.0f, YawRotation.Yaw + (MovementVector.X > 0 ? 90.0f : -90.0f), 0.0f);
					
					bIsMovingToGrid = true;
				}
				else if (MovementVector.Y != 0)
				{
					// Move vertically
					FVector CurrentLocation = GetActorLocation();
					TargetGridPosition = CurrentLocation + ForwardDirection * GridSize * FMath::Sign(MovementVector.Y);
					TargetGridPosition = SnapToGrid(TargetGridPosition);
					
					// Set target rotation based on movement direction
					TargetRotation = FRotator(0.0f, YawRotation.Yaw + (MovementVector.Y > 0 ? 0.0f : 180.0f), 0.0f);
					
					bIsMovingToGrid = true;
				}
			}
		}
		else
		{
			// Normal movement
			AddMovementInput(ForwardDirection, MovementVector.Y);
			AddMovementInput(RightDirection, MovementVector.X);
		}
	}
}

void AProjectUmeowmiCharacter::InitializeCameraPosition()
{
	// Calculate the angle for the current camera position
	float AngleStep = 360.0f / NumberOfCameraPositions;
	float CurrentAngle = BaseCameraAngle + (CameraPositionIndex * AngleStep);
	
	// Update the target camera rotation based on calculated angle
	TargetCameraRotation = FRotator(-25.0f, CurrentAngle, 0.0f);
	CameraOffset = CurrentAngle;
	
	// Immediately set the camera rotation to avoid any interpolation delay
	if (CameraBoom)
	{
		CameraBoom->SetRelativeRotation(TargetCameraRotation);
	}
	
	UE_LOG(LogTemplateCharacter, Log, TEXT("Camera initialized - Position Index: %d, Angle: %f"), CameraPositionIndex, CurrentAngle);
}

void AProjectUmeowmiCharacter::InitializeCameraPositionFromBlueprint()
{
	InitializeCameraPosition();
}

void AProjectUmeowmiCharacter::GetCameraPositionIndex(const FInputActionValue& Value)
{
	// When journal is open, bumpers cycle dishes instead of rotating camera
	UPUJournalWidget* Journal = JournalWidget;
	if (!Journal)
	{
		TArray<UUserWidget*> FoundWidgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), FoundWidgets, UPUJournalWidget::StaticClass(), false);
		for (UUserWidget* W : FoundWidgets)
		{
			if (UPUJournalWidget* J = Cast<UPUJournalWidget>(W))
			{
				Journal = J;
				break;
			}
		}
	}
	if (Journal && Journal->GetVisibility() == ESlateVisibility::Visible)
	{
		return;
	}

	float InputValue = Value.Get<float>();
	UE_LOG(LogTemplateCharacter, Log, TEXT("Camera Position Index: %f"), InputValue);
	
	//If Value is negated, then we need to rotate the camera to the right	
	// Q or LB
	if (InputValue < 0)
	{
		CameraPositionIndex = (CameraPositionIndex + 1) % NumberOfCameraPositions;
		UE_LOG(LogTemplateCharacter, Log, TEXT("Camera Position Index: %d"), CameraPositionIndex);
	}
	
	//If Value is positive, then we need to rotate the camera to the left
	// E or RB
	else if (InputValue > 0)
	{
		CameraPositionIndex = (CameraPositionIndex - 1 + NumberOfCameraPositions) % NumberOfCameraPositions;
		UE_LOG(LogTemplateCharacter, Log, TEXT("Camera Position Index: %d"), CameraPositionIndex);
	}

	// Calculate the angle for the current camera position
	float AngleStep = 360.0f / NumberOfCameraPositions;
	float CurrentAngle = BaseCameraAngle + (CameraPositionIndex * AngleStep);
	
	// Update the target camera rotation based on calculated angle
	TargetCameraRotation = FRotator(-25.0f, CurrentAngle, 0.0f);
	CameraOffset = CurrentAngle;
	
	UE_LOG(LogTemplateCharacter, Log, TEXT("Camera Angle: %f"), CurrentAngle);
}

void AProjectUmeowmiCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		 //AddControllerYawInput(LookAxisVector.X);
		 //AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AProjectUmeowmiCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ATalkingObject* CurrentTalking = GetCurrentTalkingObject())
	{
		CurrentTalking->TickFacePlayerLerp(DeltaTime);
	}
	else
	{
		// Debug: uncomment to verify player Tick runs when no talking object
		// static int32 FrameCount = 0;
		// if (++FrameCount % 300 == 0) UE_LOG(LogTemp, Log, TEXT("[FacePlayerLerp] Player Tick, no CurrentTalkingObject"));
	}

	// Smoothly interpolate the camera rotation
	FRotator CurrentRotation = CameraBoom->GetRelativeRotation();
	FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetCameraRotation, DeltaTime, CameraTransitionSpeed);
	CameraBoom->SetRelativeRotation(NewRotation);

	// Handle grid movement
	if (bUseGridMovement && bIsMovingToGrid)
	{
		FVector CurrentLocation = GetActorLocation();
		FVector Direction = (TargetGridPosition - CurrentLocation).GetSafeNormal();
		float DistanceToTarget = FVector::Distance(CurrentLocation, TargetGridPosition);

		if (DistanceToTarget > 1.0f) // If we're not close enough to the target
		{
			// Move towards the target grid position
			AddMovementInput(Direction, 1.0f);
			
			// Smoothly rotate towards the target rotation
			FRotator CurrentActorRotation = GetActorRotation();
			FRotator NewActorRotation = FMath::RInterpTo(CurrentActorRotation, TargetRotation, DeltaTime, 15.0f);
			SetActorRotation(NewActorRotation);
		}
		else
		{
			// We've reached the target grid position
			SetActorLocation(TargetGridPosition);
			SetActorRotation(TargetRotation);
			bIsMovingToGrid = false;
		}
	}

	// Live dish capture preview: RT updates every frame; Up/Down adjust pitch (for tuning scorecard framing).
	// Do not refresh while PendingScorecardDishTexture is set — that uses the same RT as CaptureDishSnapshotFromPlatingStation
	// and would overwrite the station snapshot with head-preview framing before ShowScorecard consumes it.
	if (bDishCaptureLivePreview && bEnableDishCaptureForScorecard && DishPreviewComponent && DishPreviewComponent->HasPreview() && DishCaptureComponent
		&& !PendingScorecardDishTexture)
	{
		if (bDishCaptureLivePreviewPitchKeys)
		{
			if (APlayerController* PC = Cast<APlayerController>(GetController()))
			{
				const float PitchDelta = DishCaptureLivePreviewPitchDegreesPerSecond * DeltaTime;
				if (PC->IsInputKeyDown(EKeys::Up))
				{
					DishCaptureCameraPitchAfterAimDegrees = FMath::Clamp(DishCaptureCameraPitchAfterAimDegrees + PitchDelta, -89.f, 89.f);
				}
				if (PC->IsInputKeyDown(EKeys::Down))
				{
					DishCaptureCameraPitchAfterAimDegrees = FMath::Clamp(DishCaptureCameraPitchAfterAimDegrees - PitchDelta, -89.f, 89.f);
				}
			}
		}
		RefreshDishCapturePreviewFromDishPreview();
		if (bDishCaptureLivePreviewShowPitchOnScreen && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				91001,
				0.f,
				FColor::Green,
				FString::Printf(
					TEXT("PitchAfterAim %.1f | mesh P %.1f | cam tweak P %.1f"),
					DishCaptureCameraPitchAfterAimDegrees,
					DishCapturePreviewMeshRotationOffset.Pitch,
					DishCaptureCameraLocalRotation.Pitch));
		}
	}
}

void AProjectUmeowmiCharacter::ToggleGridMovement(const FInputActionValue& Value)
{
	bUseGridMovement = !bUseGridMovement;
	
	// Update movement speed based on grid mode
	if (bUseGridMovement)
	{
		GetCharacterMovement()->MaxWalkSpeed = GridMovementSpeed;
		// Initialize target position and rotation to current values when enabling grid movement
		TargetGridPosition = GetActorLocation();
		TargetRotation = GetActorRotation();
		bIsMovingToGrid = false;
		
		// Ensure character movement component settings are correct for grid movement
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = 500.0f; // Reset to default speed
		bIsMovingToGrid = false;
		
		// Reset character movement component settings
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}

	UE_LOG(LogTemplateCharacter, Log, TEXT("Grid Movement %s"), bUseGridMovement ? TEXT("Enabled") : TEXT("Disabled"));
}

FVector AProjectUmeowmiCharacter::SnapToGrid(const FVector& Location) const
{
	// Snap X and Y coordinates to the nearest grid point
	float SnappedX = FMath::RoundToFloat(Location.X / GridSize) * GridSize;
	float SnappedY = FMath::RoundToFloat(Location.Y / GridSize) * GridSize;
	
	// Keep Z coordinate unchanged
	return FVector(SnappedX, SnappedY, Location.Z);
}

void AProjectUmeowmiCharacter::ZoomCamera(const FInputActionValue& Value)
{
	if (bLockOrthoWidth)
	{
		if (bShowOrthoWidthDebug)
		{
			UE_LOG(LogTemplateCharacter, Display, TEXT("[OrthoWidth] Locked at %.1f (zoom input ignored)"), FollowCamera->OrthoWidth);
		}
		return;
	}

	// Get the zoom input value
	float ZoomValue = Value.Get<float>();
	
	// Determine if this is from a controller or mouse wheel
	// Controller input typically comes as a float between -1 and 1
	// Mouse wheel typically comes as a float with values like -1, 0, or 1
	
	// Calculate the zoom amount based on the input source
	float ZoomAmount = 0.0f;
	
	// If the absolute value is close to 1, it's likely from a controller
	if (FMath::IsNearlyEqual(FMath::Abs(ZoomValue), 1.0f, 0.1f))
	{
		// Controller input - use the controller zoom speed
		ZoomAmount = ZoomValue * ControllerZoomSpeed;
	}
	else
	{
		// Mouse wheel input - use the mouse wheel zoom speed
		ZoomAmount = ZoomValue * MouseWheelZoomSpeed;
	}
	
	// Get the current orthographic width
	float CurrentOrthoWidth = FollowCamera->OrthoWidth;
	
	// Calculate the new orthographic width
	// Note: For orthographic cameras, smaller width = more zoomed in
	float NewOrthoWidth = FMath::Clamp(CurrentOrthoWidth + ZoomAmount, MinOrthoWidth, MaxOrthoWidth);
	
	// Apply the new orthographic width
	FollowCamera->OrthoWidth = NewOrthoWidth;

	if (bShowOrthoWidthDebug)
	{
		UE_LOG(LogTemplateCharacter, Display, TEXT("[OrthoWidth] %.1f (range: %.1f - %.1f)"), NewOrthoWidth, MinOrthoWidth, MaxOrthoWidth);
	}
}

void AProjectUmeowmiCharacter::ToggleJournal(const FInputActionValue& Value)
{
	UPUJournalWidget* Journal = JournalWidget;
	if (!Journal)
	{
		// Fallback: search for journal widget in the world (e.g. if it's a child of HUD)
		TArray<UUserWidget*> FoundWidgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), FoundWidgets, UPUJournalWidget::StaticClass(), /*bTopLevelOnly=*/ false);
		for (UUserWidget* W : FoundWidgets)
		{
			if (UPUJournalWidget* J = Cast<UPUJournalWidget>(W))
			{
				Journal = J;
				break;
			}
		}
	}

	if (Journal)
	{
		const bool bIsVisible = Journal->GetVisibility() == ESlateVisibility::Visible;
		if (bIsVisible)
		{
			Journal->CloseJournal();
		}
		else
		{
			Journal->OpenJournal();
		}
	}
}

void AProjectUmeowmiCharacter::OnJournalCycleDishPrev(const FInputActionValue& Value)
{
	UPUJournalWidget* Journal = JournalWidget;
	if (!Journal)
	{
		TArray<UUserWidget*> FoundWidgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), FoundWidgets, UPUJournalWidget::StaticClass(), false);
		for (UUserWidget* W : FoundWidgets)
		{
			if (UPUJournalWidget* J = Cast<UPUJournalWidget>(W))
			{
				Journal = J;
				break;
			}
		}
	}
	if (Journal && Journal->GetVisibility() == ESlateVisibility::Visible)
	{
		Journal->CycleRecipesDish(-1);
	}
}

void AProjectUmeowmiCharacter::OnSkipDialogueStarted(const FInputActionValue& Value)
{
	if (DialogueBox && DialogueBox->IsDialogueInteractive())
	{
		DialogueBox->SetSkipMode(true);
	}
}

void AProjectUmeowmiCharacter::OnSkipDialogueCompleted(const FInputActionValue& Value)
{
	if (DialogueBox)
	{
		DialogueBox->SetSkipMode(false);
	}
}

void AProjectUmeowmiCharacter::OnJournalCycleDishNext(const FInputActionValue& Value)
{
	UPUJournalWidget* Journal = JournalWidget;
	if (!Journal)
	{
		TArray<UUserWidget*> FoundWidgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), FoundWidgets, UPUJournalWidget::StaticClass(), false);
		for (UUserWidget* W : FoundWidgets)
		{
			if (UPUJournalWidget* J = Cast<UPUJournalWidget>(W))
			{
				Journal = J;
				break;
			}
		}
	}
	if (Journal && Journal->GetVisibility() == ESlateVisibility::Visible)
	{
		Journal->CycleRecipesDish(1);
	}
}

void AProjectUmeowmiCharacter::Interact(const FInputActionValue& Value)
{
	ATalkingObject* CurrentTalking = GetCurrentTalkingObject();

	// When in dialogue, Interact advances the dialogue (skip typewriter or next line)
	if (CurrentTalking && DialogueBox && DialogueBox->IsDialogueInteractive())
	{
		DialogueBox->AdvanceDialogue();
		return;
	}
		
	if (CurrentTalking)
	{
		CurrentTalking->StartInteraction();
	}
	else if (CurrentInteractable)
	{
		CurrentInteractable->StartInteraction();
	}
}

ATalkingObject* AProjectUmeowmiCharacter::GetCurrentTalkingObject() const
{
	if (OverlappingTalkingObjects.IsValidIndex(SelectedTalkingObjectIndex))
	{
		ATalkingObject* Obj = OverlappingTalkingObjects[SelectedTalkingObjectIndex];
		return (Obj && IsValid(Obj)) ? Obj : nullptr;
	}
	return nullptr;
}

void AProjectUmeowmiCharacter::RegisterTalkingObject(ATalkingObject* TalkingObject)
{
	if (!TalkingObject || !IsValid(TalkingObject)) return;

	// Add to list if not already present (avoid duplicates from overlap order)
	int32 ExistingIndex = OverlappingTalkingObjects.Find(TalkingObject);
	if (ExistingIndex == INDEX_NONE)
	{
		OverlappingTalkingObjects.Add(TalkingObject);
		if (OverlappingTalkingObjects.Num() == 1)
		{
			SelectedTalkingObjectIndex = 0;
		}
		else
		{
			// Most recent overlap becomes the selected target
			SelectedTalkingObjectIndex = OverlappingTalkingObjects.Num() - 1;
		}
		// Refresh all overlapping widgets so opacity updates immediately (e.g. first one fades when second overlaps)
		for (ATalkingObject* Obj : OverlappingTalkingObjects)
		{
			if (Obj && IsValid(Obj))
			{
				Obj->RefreshInteractionWidget();
			}
		}
	}
}

void AProjectUmeowmiCharacter::UnregisterTalkingObject(ATalkingObject* TalkingObject)
{
	if (!TalkingObject) return;

	int32 RemovedIndex = OverlappingTalkingObjects.Find(TalkingObject);
	if (RemovedIndex != INDEX_NONE)
	{
		OverlappingTalkingObjects.RemoveAt(RemovedIndex);
		// Clamp selection index after removal
		if (OverlappingTalkingObjects.Num() == 0)
		{
			SelectedTalkingObjectIndex = 0;
		}
		else if (SelectedTalkingObjectIndex >= OverlappingTalkingObjects.Num())
		{
			SelectedTalkingObjectIndex = OverlappingTalkingObjects.Num() - 1;
		}
		else if (RemovedIndex < SelectedTalkingObjectIndex)
		{
			SelectedTalkingObjectIndex--;
		}
		// Refresh remaining widgets so the last one returns to full opacity
		for (ATalkingObject* Obj : OverlappingTalkingObjects)
		{
			if (Obj && IsValid(Obj))
			{
				Obj->RefreshInteractionWidget();
			}
		}
	}
}

void AProjectUmeowmiCharacter::CycleInteractTarget(const FInputActionValue& Value)
{
	// Don't cycle during active dialogue
	if (DialogueBox && DialogueBox->IsDialogueInteractive()) return;
	if (OverlappingTalkingObjects.Num() < 2) return;

	// Cycle forward (Space = next)
	SelectedTalkingObjectIndex = (SelectedTalkingObjectIndex + 1) % OverlappingTalkingObjects.Num();

		// Refresh all overlapping widgets so they can update selection state (e.g. highlight selected)
		for (ATalkingObject* Obj : OverlappingTalkingObjects)
		{
			if (Obj && IsValid(Obj))
			{
				Obj->RefreshInteractionWidget();
			}
		}
}

void AProjectUmeowmiCharacter::RegisterInteractable(TScriptInterface<IPUInteractableInterface> Interactable)
{
	if (Interactable)
	{
		CurrentInteractable = Interactable;
		
		// Bind to interaction events
		Interactable->OnInteractionStarted().AddUObject(this, &AProjectUmeowmiCharacter::OnInteractionStarted);
		Interactable->OnInteractionEnded().AddUObject(this, &AProjectUmeowmiCharacter::OnInteractionEnded);
		Interactable->OnInteractionFailed().AddUObject(this, &AProjectUmeowmiCharacter::OnInteractionFailed);
	}
}

void AProjectUmeowmiCharacter::UnregisterInteractable(TScriptInterface<IPUInteractableInterface> Interactable)
{
	if (CurrentInteractable == Interactable)
	{
		// Unbind from interaction events
		if (Interactable)
		{
			Interactable->OnInteractionStarted().RemoveAll(this);
			Interactable->OnInteractionEnded().RemoveAll(this);
			Interactable->OnInteractionFailed().RemoveAll(this);
		}
		
		CurrentInteractable = nullptr;
	}
}

void AProjectUmeowmiCharacter::OnInteractionStarted()
{
	// Handle interaction started
	//UE_LOG(LogTemp,Log, TEXT("Interaction started"));
}

void AProjectUmeowmiCharacter::OnInteractionEnded()
{
	// Handle interaction ended
	//UE_LOG(LogTemp,Log, TEXT("Interaction ended"));
}

void AProjectUmeowmiCharacter::OnInteractionFailed()
{
	// Handle interaction failed
	//UE_LOG(LogTemp,Log, TEXT("Interaction failed"));
}

//////////////////////////////////////////////////////////////////////////
// Emote

void AProjectUmeowmiCharacter::BeginFadeOutEmote()
{
	if (!EmoteWidget)
	{
		ClearEmote();
		return;
	}

	if (UPUEmoteWidget* EmoteUserWidget = Cast<UPUEmoteWidget>(EmoteWidget->GetWidget()))
	{
		EmoteUserWidget->PlayFadeOut();

		const float FadeDuration = EmoteUserWidget->GetFadeUpDuration();
		const float TimerDuration = (FadeDuration > 0.0f) ? (FadeDuration / 2.0f) : 0.25f;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(EmoteFadeOutTimerHandle);
			World->GetTimerManager().SetTimer(EmoteFadeOutTimerHandle, this, &AProjectUmeowmiCharacter::ClearEmote, TimerDuration, false);
		}
		else
		{
			ClearEmote();
		}
	}
	else
	{
		ClearEmote();
	}
}

void AProjectUmeowmiCharacter::ShowEmoteByTag(FGameplayTag EmoteTag)
{
	if (!bEnableEmotes || !EmoteWidget)
	{
		return;
	}

	if (!EmoteTag.IsValid())
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("ShowEmoteByTag - Invalid emote tag on %s"), *GetName());
		return;
	}

	if (!EmoteDataTable)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("ShowEmoteByTag - EmoteDataTable is not set on %s"), *GetName());
		return;
	}

	// DataTable rows use the tag's leaf name (part after last '.') lowercased, e.g. Emote.Happy -> "happy"
	FString TagStr = EmoteTag.ToString();
	int32 LastDot = INDEX_NONE;
	if (TagStr.FindLastChar(TEXT('.'), LastDot) && LastDot >= 0)
	{
		TagStr = TagStr.Mid(LastDot + 1);
	}
	TagStr = TagStr.ToLower();
	const FName RowName = FName(*TagStr);

	const FPUEmoteData* EmoteRow = EmoteDataTable->FindRow<FPUEmoteData>(RowName, TEXT("ShowEmoteByTag"));
	if (!EmoteRow)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("ShowEmoteByTag - No emote data row for tag %s on %s"), *EmoteTag.ToString(), *GetName());
		return;
	}

	if (!EmoteRow->Icon)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("ShowEmoteByTag - Emote row %s has no Icon on %s"), *RowName.ToString(), *GetName());
		return;
	}

	// Ensure widget is created - do NOT override Space or DrawSize (set on component in Blueprint)
	if (!EmoteWidget->GetWidget() && EmoteWidgetClass)
	{
		EmoteWidget->SetWidgetClass(EmoteWidgetClass);
		EmoteWidget->SetDrawAtDesiredSize(false);
	}

	UPUEmoteWidget* EmoteUserWidget = Cast<UPUEmoteWidget>(EmoteWidget->GetWidget());
	if (!EmoteUserWidget && EmoteWidgetClass)
	{
		EmoteWidget->SetWidgetClass(EmoteWidgetClass);
		EmoteWidget->SetDrawAtDesiredSize(false);
		EmoteUserWidget = Cast<UPUEmoteWidget>(EmoteWidget->GetWidget());
	}

	if (!EmoteUserWidget)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("ShowEmoteByTag - EmoteWidget is not of type UPUEmoteWidget on %s (set EmoteWidgetClass on this actor)"), *GetName());
		return;
	}

	EmoteUserWidget->SetEmoteIcon(EmoteRow->Icon);
	EmoteWidget->SetDrawAtDesiredSize(false); // Use component's Draw Size (128), not widget's desired size (256)
	EmoteWidget->SetVisibility(true);
	EmoteUserWidget->PlayFadeIn();
	ActiveEmoteTag = EmoteTag;

	if (EmoteRow->Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, EmoteRow->Sound, GetActorLocation());
	}

	if (!EmoteRow->bLoop)
	{
		const float Duration = EmoteRow->Duration > 0.0f ? EmoteRow->Duration : 2.0f;
		if (UWorld* WorldPtr = GetWorld())
		{
			WorldPtr->GetTimerManager().ClearTimer(EmoteHideTimerHandle);
			WorldPtr->GetTimerManager().ClearTimer(EmoteFadeOutTimerHandle);
			WorldPtr->GetTimerManager().SetTimer(EmoteHideTimerHandle, this, &AProjectUmeowmiCharacter::BeginFadeOutEmote, Duration, false);
		}
	}
}

void AProjectUmeowmiCharacter::ClearEmote()
{
	if (EmoteWidget)
	{
		if (UPUEmoteWidget* EmoteUserWidget = Cast<UPUEmoteWidget>(EmoteWidget->GetWidget()))
		{
			EmoteUserWidget->ClearEmoteIcon();
		}
		EmoteWidget->SetVisibility(false);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EmoteHideTimerHandle);
		World->GetTimerManager().ClearTimer(EmoteFadeOutTimerHandle);
	}

	ActiveEmoteTag = FGameplayTag();
}

bool AProjectUmeowmiCharacter::IsEmoteActive() const
{
	return EmoteWidget && EmoteWidget->IsVisible();
}

// Order System Integration
void AProjectUmeowmiCharacter::SetCurrentOrder(const FPUOrderBase& Order)
{
	//UE_LOG(LogTemp,Display, TEXT("ProjectUmeowmiCharacter::SetCurrentOrder - Setting current order: %s"), *Order.OrderID.ToString());
	
	// Clean up any existing UObject references before setting new order
	if (bHasCurrentOrder)
	{
		//UE_LOG(LogTemp,Display, TEXT("SetCurrentOrder - Cleaning up existing UObject references"));
		CleanupOrderUObjectReferences(CurrentOrder);
	}
	
	CurrentOrder = Order;
	bHasCurrentOrder = true;
	bCurrentOrderCompleted = false;
	CurrentOrderSatisfaction = 0.0f;
	bStationDishCaptureValidForScorecard = false;
	
	//UE_LOG(LogTemp,Display, TEXT("ProjectUmeowmiCharacter::SetCurrentOrder - Order set successfully"));
}

void AProjectUmeowmiCharacter::RevealHintOnCurrentOrder(FName AspectName)
{
	UE_LOG(LogTemp, Display, TEXT("[Hint] RevealHintOnCurrentOrder(%s) - hasOrder=%d completed=%d"), *AspectName.ToString(), bHasCurrentOrder, bCurrentOrderCompleted);

	if (!bHasCurrentOrder || bCurrentOrderCompleted || AspectName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Hint] RevealHintOnCurrentOrder aborted - no active order or aspect invalid"));
		return;
	}

	for (const FOrderAspectRequirement& Req : CurrentOrder.TargetAspects)
	{
		if (Req.GetAspectName() == AspectName)
		{
			// Check if already discovered
			for (const FOrderAspectRequirement& Discovered : CurrentOrder.DiscoveredHints)
			{
				if (Discovered.GetAspectName() == AspectName)
				{
					UE_LOG(LogTemp, Display, TEXT("[Hint] Hint %s already revealed, skipping"), *AspectName.ToString());
					return;
				}
			}
			CurrentOrder.DiscoveredHints.Add(Req);
			UE_LOG(LogTemp, Display, TEXT("[Hint] SUCCESS: Revealed hint %s (target %.1f) - total discovered: %d"), *AspectName.ToString(), Req.TargetValue, CurrentOrder.DiscoveredHints.Num());
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Hint] RevealHintOnCurrentOrder - aspect %s not in order's TargetAspects"), *AspectName.ToString());
}

void AProjectUmeowmiCharacter::ClearCurrentOrder()
{
	//UE_LOG(LogTemp,Display, TEXT("=== CLEARING CURRENT ORDER ==="));
	//UE_LOG(LogTemp,Display, TEXT("Order ID: %s"), *CurrentOrder.OrderID.ToString());
	//UE_LOG(LogTemp,Display, TEXT("Order Description: %s"), *CurrentOrder.OrderDescription.ToString());
	//UE_LOG(LogTemp,Display, TEXT("Has Current Order: %s"), bHasCurrentOrder ? TEXT("TRUE") : TEXT("FALSE"));
	//UE_LOG(LogTemp,Display, TEXT("Is Completed: %s"), bCurrentOrderCompleted ? TEXT("TRUE") : TEXT("FALSE"));
	//UE_LOG(LogTemp,Display, TEXT("Satisfaction Score: %.2f"), CurrentOrderSatisfaction);
	
	// Properly clean up UObject references before clearing
	//UE_LOG(LogTemp,Display, TEXT("ClearCurrentOrder - Cleaning up UObject references"));
	CleanupOrderUObjectReferences(CurrentOrder);
	
	// Now safely clear the order data
	CurrentOrder = FPUOrderBase();
	bHasCurrentOrder = false;
	bCurrentOrderCompleted = false;
	CurrentOrderSatisfaction = 0.0f;

	// Clear the dish preview above the character's head
	if (DishPreviewComponent)
	{
		DishPreviewComponent->ClearPreview();
	}
	PendingScorecardDishTexture = nullptr;
	bStationDishCaptureValidForScorecard = false;
	
	//UE_LOG(LogTemp,Display, TEXT("=== ORDER CLEARED ==="));
	//UE_LOG(LogTemp,Display, TEXT("Has Current Order: %s"), bHasCurrentOrder ? TEXT("TRUE") : TEXT("FALSE"));
	//UE_LOG(LogTemp,Display, TEXT("Is Completed: %s"), bCurrentOrderCompleted ? TEXT("TRUE") : TEXT("FALSE"));
	//UE_LOG(LogTemp,Display, TEXT("Satisfaction Score: %.2f"), CurrentOrderSatisfaction);
	//UE_LOG(LogTemp,Display, TEXT("========================="));
}

void AProjectUmeowmiCharacter::SetOrderResult(bool bCompleted, float SatisfactionScore)
{
	//UE_LOG(LogTemp,Display, TEXT("ProjectUmeowmiCharacter::SetOrderResult - Order completed: YES, Satisfaction: %.2f"), 
	//	SatisfactionScore);
	
	// Orders are always completed when submitted - satisfaction score indicates quality
	bHasCurrentOrder = false;      // Clear active flag
	bCurrentOrderCompleted = true; // Set completed flag
	CurrentOrderSatisfaction = SatisfactionScore;
	
	// Display the order result immediately
	DisplayOrderResult();
}

void AProjectUmeowmiCharacter::DisplayOrderResult()
{
	//UE_LOG(LogTemp,Display, TEXT("ProjectUmeowmiCharacter::DisplayOrderResult - Displaying order result"));
	
	// Orders are always completed - satisfaction score indicates quality
	// Determine satisfaction level
	FString SatisfactionLevel;
	if (CurrentOrderSatisfaction >= 0.875f)
	{
		SatisfactionLevel = TEXT("Perfect!");
	}
	else if (CurrentOrderSatisfaction >= 0.625f)
	{
		SatisfactionLevel = TEXT("Great!");
	}
	else if (CurrentOrderSatisfaction >= 0.375f)
	{
		SatisfactionLevel = TEXT("Okay!");
	}
	else
	{
		SatisfactionLevel = TEXT("Needs Improvement.");
	}
	
	//UE_LOG(LogTemp,Display, TEXT("=== ORDER COMPLETED ==="));
	//UE_LOG(LogTemp,Display, TEXT("Order: %s"), *CurrentOrder.OrderDescription.ToString());
	//UE_LOG(LogTemp,Display, TEXT("Satisfaction: %s (%.1f%%)"), *SatisfactionLevel, CurrentOrderSatisfaction * 100.0f);
	//UE_LOG(LogTemp,Display, TEXT("====================="));
	
	// Call the order completed event
	OnOrderCompleted();
}

void AProjectUmeowmiCharacter::ClearCompletedOrder()
{
	//UE_LOG(LogTemp,Display, TEXT("=== CLEARING COMPLETED ORDER ==="));
	//UE_LOG(LogTemp,Display, TEXT("Order ID: %s"), *CurrentOrder.OrderID.ToString());
	//UE_LOG(LogTemp,Display, TEXT("Order Description: %s"), *CurrentOrder.OrderDescription.ToString());
	//UE_LOG(LogTemp,Display, TEXT("Has Current Order: %s"), bHasCurrentOrder ? TEXT("TRUE") : TEXT("FALSE"));
	//UE_LOG(LogTemp,Display, TEXT("Is Completed: %s"), bCurrentOrderCompleted ? TEXT("TRUE") : TEXT("FALSE"));
	//UE_LOG(LogTemp,Display, TEXT("Satisfaction Score: %.2f"), CurrentOrderSatisfaction);
	
	// Validate that we can safely clear the order
	if (!bCurrentOrderCompleted)
	{
		//UE_LOG(LogTemp,Warning, TEXT("ClearCompletedOrder - Cannot clear: order not completed"));
		return;
	}
	
	// Properly clean up UObject references before clearing
	//UE_LOG(LogTemp,Display, TEXT("ClearCompletedOrder - Cleaning up UObject references"));
	CleanupOrderUObjectReferences(CurrentOrder);
	
	// Now safely clear the order data
	CurrentOrder = FPUOrderBase();
	bHasCurrentOrder = false;
	bCurrentOrderCompleted = false;
	CurrentOrderSatisfaction = 0.0f;

	// Clear the dish preview above the character's head
	if (DishPreviewComponent)
	{
		DishPreviewComponent->ClearPreview();
	}
	PendingScorecardDishTexture = nullptr;
	bStationDishCaptureValidForScorecard = false;
	
	//UE_LOG(LogTemp,Display, TEXT("=== COMPLETED ORDER CLEARED ==="));
	//UE_LOG(LogTemp,Display, TEXT("Has Current Order: %s"), bHasCurrentOrder ? TEXT("TRUE") : TEXT("FALSE"));
	//UE_LOG(LogTemp,Display, TEXT("Is Completed: %s"), bCurrentOrderCompleted ? TEXT("TRUE") : TEXT("FALSE"));
	//UE_LOG(LogTemp,Display, TEXT("Satisfaction Score: %.2f"), CurrentOrderSatisfaction);
	//UE_LOG(LogTemp,Display, TEXT("================================="));
}

FText AProjectUmeowmiCharacter::GetOrderResultText() const
{
	if (!bCurrentOrderCompleted)
	{
		return FText::FromString(TEXT("No order completed yet."));
	}
	
	// Orders are always completed - satisfaction score indicates quality
	// Determine satisfaction level
	FString SatisfactionLevel;
	if (CurrentOrderSatisfaction >= 0.875f)
	{
		SatisfactionLevel = TEXT("Perfect!");
	}
	else if (CurrentOrderSatisfaction >= 0.625f)
	{
		SatisfactionLevel = TEXT("Great!");
	}
	else if (CurrentOrderSatisfaction >= 0.375f)
	{
		SatisfactionLevel = TEXT("Okay!");
	}
	else
	{
		SatisfactionLevel = TEXT("Needs Improvement.");
	}
	
	FString ResultText = FString::Printf(TEXT("Order Completed!\nSatisfaction: %s (%.1f%%)"), 
		*SatisfactionLevel, CurrentOrderSatisfaction * 100.0f);
	
	return FText::FromString(ResultText);
}

void AProjectUmeowmiCharacter::OnOrderCompleted()
{
}

void AProjectUmeowmiCharacter::EnsureDishCaptureRenderTarget()
{
	const int32 Size = FMath::Clamp(DishCaptureSize, 128, 2048);
	const bool bNeedsRecreate = !DishCaptureRenderTarget
		|| DishCaptureRenderTarget->SizeX != Size
		|| DishCaptureRenderTarget->RenderTargetFormat != RTF_RGBA16f;
	if (bNeedsRecreate)
	{
		// Float RGBA required for SceneColorHDR; alpha channel used for empty / compositing.
		DishCaptureRenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(
			this, Size, Size, RTF_RGBA16f, FLinearColor::Transparent, false);
	}
}

void AProjectUmeowmiCharacter::ConfigureDishCaptureCamera()
{
	if (!DishPreviewComponent || !DishCaptureComponent)
	{
		return;
	}

	FBox DishPreviewWorldBounds = DishPreviewComponent->ComputePreviewWorldBounds();
	DishPreviewWorldBounds = SnapDishCaptureWorldBounds(DishPreviewWorldBounds, DishCaptureBoundsPaddingUU, DishCaptureBoundsExtentSnapUU);
	ConfigureDishCaptureCameraFromWorldBounds(DishPreviewWorldBounds, DishPreviewComponent->GetComponentLocation());
}

void AProjectUmeowmiCharacter::ApplyDishCaptureCameraTweaks(const FVector& DishFocusWorld)
{
	if (!DishCaptureComponent)
	{
		return;
	}

	// Manual framing: reset to authored transform each capture, then apply nudges (otherwise AddLocal accumulates).
	if (!bUseAutomaticDishCaptureFraming)
	{
		if (!bDishCaptureRelativeBaseCaptured)
		{
			DishCaptureRelativeBaseAtStart = DishCaptureComponent->GetRelativeTransform();
			bDishCaptureRelativeBaseCaptured = true;
		}
		DishCaptureComponent->SetRelativeTransform(DishCaptureRelativeBaseAtStart);
	}

	// Aim at dish center from current capture position (automatic: after plating/bounds configure; manual: after authored reset).
	// Must run in automatic mode too — otherwise plating-camera rotation + ortho keeps tweaks from reading as tilt relative to the food.
	const FVector CamLoc = DishCaptureComponent->GetComponentLocation();
	const FVector ToFocus = DishFocusWorld - CamLoc;
	if (ToFocus.SizeSquared() > FMath::Square(1.f))
	{
		// Same as Kismet FindLookAtRotation = FRotationMatrix::MakeFromX(Target - Start).
		DishCaptureComponent->SetWorldRotation(UKismetMathLibrary::FindLookAtRotation(CamLoc, DishFocusWorld));
	}
	// Aim-at-center + camera near the dish plane => horizontal view => rim edge-on. Tilt view down in local space to see into the bowl.
	if (!FMath::IsNearlyZero(DishCaptureCameraPitchAfterAimDegrees, 0.01f))
	{
		const FQuat BaseQ = FQuat(DishCaptureComponent->GetComponentRotation());
		const FQuat PitchQ = FQuat(FRotator(DishCaptureCameraPitchAfterAimDegrees, 0.f, 0.f));
		DishCaptureComponent->SetWorldRotation((BaseQ * PitchQ).Rotator());
	}

	if (!DishCaptureCameraLocalOffset.IsNearlyZero(0.01f))
	{
		// Match camera-relative axes: X = right, Y = forward, Z = up (in view space).
		const FVector Right = DishCaptureComponent->GetRightVector();
		const FVector Up = DishCaptureComponent->GetUpVector();
		const FVector Forward = DishCaptureComponent->GetForwardVector();
		DishCaptureComponent->AddWorldOffset(
			Right * DishCaptureCameraLocalOffset.X + Forward * DishCaptureCameraLocalOffset.Y + Up * DishCaptureCameraLocalOffset.Z);
	}
	// Local Euler after aim: world rotation = Quat(base) * Quat(delta) (delta in component local space after framing).
	if (!DishCaptureCameraLocalRotation.Equals(FRotator::ZeroRotator, 0.01f))
	{
		const FQuat BaseQ = FQuat(DishCaptureComponent->GetComponentRotation());
		const FQuat DeltaQ = FQuat(DishCaptureCameraLocalRotation);
		DishCaptureComponent->SetWorldRotation((BaseQ * DeltaQ).Rotator());
	}
}

void AProjectUmeowmiCharacter::ConfigureDishCaptureCameraFromWorldBounds(const FBox& InWorldBounds, const FVector& FallbackCenter)
{
	if (!DishCaptureComponent)
	{
		return;
	}

	FVector Center;
	FVector Extent;
	GetDishCaptureBoundsCenterAndExtent(InWorldBounds, FallbackCenter, Center, Extent);

	const float MaxExtent = FMath::Max3(Extent.X, Extent.Y, Extent.Z);
	float Distance = 0.f;
	float VerticalLift = 0.f;
	ComputeDishCaptureDistanceAndLift(MaxExtent, DishCaptureCameraVerticalLiftExtentScale, DishCaptureCameraVerticalLiftMinUU, Distance, VerticalLift);

	const FVector Forward = GetActorForwardVector();
	const FVector Up = FVector::UpVector;
	const FVector CamLoc = Center - Forward * Distance + Up * VerticalLift;

	DishCaptureComponent->ProjectionType = ECameraProjectionMode::Perspective;
	DishCaptureComponent->FOVAngle = 35.f;
	DishCaptureComponent->SetWorldLocation(CamLoc);
	DishCaptureComponent->SetWorldRotation(UKismetMathLibrary::FindLookAtRotation(CamLoc, Center));
}

void AProjectUmeowmiCharacter::ConfigureDishCaptureCameraToMatchPlatingCamera(UCameraComponent* PlatingCamera, const FBox& InWorldBoundsFallback, const FVector& FallbackCenter)
{
	if (!DishCaptureComponent)
	{
		return;
	}

	FVector Center;
	FVector Extent;
	GetDishCaptureBoundsCenterAndExtent(InWorldBoundsFallback, FallbackCenter, Center, Extent);
	const float MaxExtent = FMath::Max3(Extent.X, Extent.Y, Extent.Z);
	float Distance = 0.f;
	float VerticalLift = 0.f;
	ComputeDishCaptureDistanceAndLift(MaxExtent, DishCaptureCameraVerticalLiftExtentScale, DishCaptureCameraVerticalLiftMinUU, Distance, VerticalLift);
	const FVector Up = FVector::UpVector;

	if (PlatingCamera)
	{
		// Do not snap to the plating camera's world position — that distance changes with station layout, zoom, and timing.
		// Stay on the same view ray (center → plating camera) but use bounds-derived distance so identical dish bounds → identical scale.
		const FVector PlatingLoc = PlatingCamera->GetComponentLocation();
		FVector RadialFromCenterToCamera = PlatingLoc - Center;
		if (!RadialFromCenterToCamera.Normalize())
		{
			RadialFromCenterToCamera = -PlatingCamera->GetForwardVector();
		}
		const FVector CamLoc = Center + RadialFromCenterToCamera * Distance + Up * VerticalLift;

		DishCaptureComponent->ProjectionType = ECameraProjectionMode::Perspective;
		// Fixed FOV (same as bounds fallback). Copying plating FOV would change apparent dish size when the plating camera zooms.
		DishCaptureComponent->FOVAngle = 35.f;
		DishCaptureComponent->SetWorldLocation(CamLoc);
		DishCaptureComponent->SetWorldRotation(UKismetMathLibrary::FindLookAtRotation(CamLoc, Center));
		return;
	}

	ConfigureDishCaptureCameraFromWorldBounds(InWorldBoundsFallback, FallbackCenter);
}

void AProjectUmeowmiCharacter::CaptureDishSnapshotFromPlatingStation(UPUDishCustomizationComponent* CustomizationComponent)
{
	if (!bEnableDishCaptureForScorecard || !CustomizationComponent || !DishCaptureComponent || !GetWorld())
	{
		return;
	}

	TArray<UPrimitiveComponent*> Prims;
	CustomizationComponent->GatherDishSnapshotPrimitives(Prims);
	if (Prims.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Scorecard] CaptureDishSnapshotFromPlatingStation: no snapshot primitives — scorecard will fall back to head-preview capture at ShowScorecard if needed."));
		return;
	}

	FBox MergedWorldBounds(ForceInit);
	for (UPrimitiveComponent* P : Prims)
	{
		if (P && IsValid(P))
		{
			MergedWorldBounds += P->CalcBounds(P->GetComponentTransform()).GetBox();
		}
	}

	if (MergedWorldBounds.IsValid != 0)
	{
		MergedWorldBounds = SnapDishCaptureWorldBounds(MergedWorldBounds, DishCaptureBoundsPaddingUU, DishCaptureBoundsExtentSnapUU);
	}

	FVector FallbackCenter;
	if (MergedWorldBounds.IsValid != 0)
	{
		FallbackCenter = MergedWorldBounds.GetCenter();
	}
	else
	{
		FallbackCenter = CustomizationComponent->GetComponentLocation();
	}

	EnsureDishCaptureRenderTarget();
	const FEngineShowFlags SavedShowFlags = DishCaptureComponent->ShowFlags;
	const ESceneCaptureSource SavedCaptureSource = DishCaptureComponent->CaptureSource;
	const FDishCaptureSavedPostProcess SavedPP = SaveDishCapturePostProcess(DishCaptureComponent);
	DishCaptureComponent->CaptureSource = SCS_SceneColorHDR;
	ApplyDishCaptureCapturePostProcess(DishCaptureComponent, bDishCapturePostProcessingTone, DishCapturePostProcessBlendWeightForCapture, DishCaptureExposureBias);
	DishCaptureComponent->TextureTarget = DishCaptureRenderTarget;
	DishCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	DishCaptureComponent->ShowOnlyComponents.Empty();
	for (UPrimitiveComponent* P : Prims)
	{
		if (P && IsValid(P))
		{
			DishCaptureComponent->ShowOnlyComponents.Add(P);
		}
	}
	DishCaptureComponent->ShowOnlyActors.Empty();
	DishCaptureComponent->HiddenComponents.Empty();
	DishCaptureComponent->HiddenActors.Empty();

	ApplyDishOnlyCaptureShowFlags(DishCaptureComponent, bDishCapturePostProcessingTone);

	UKismetRenderingLibrary::ClearRenderTarget2D(this, DishCaptureRenderTarget, FLinearColor::Transparent);

	if (bUseAutomaticDishCaptureFraming)
	{
		UCameraComponent* PlatingCam = CustomizationComponent->GetPlatingStationCamera();
		ConfigureDishCaptureCameraToMatchPlatingCamera(PlatingCam, MergedWorldBounds, FallbackCenter);
	}
	const FVector DishFocus = (MergedWorldBounds.IsValid != 0) ? MergedWorldBounds.GetCenter() : FallbackCenter;
	ApplyDishCaptureCameraTweaks(DishFocus);

	DishCaptureComponent->CaptureScene();
	DishCaptureComponent->CaptureSource = SavedCaptureSource;
	RestoreDishCapturePostProcess(DishCaptureComponent, SavedPP);
	DishCaptureComponent->ShowFlags = SavedShowFlags;
	DishCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	DishCaptureComponent->ShowOnlyComponents.Empty();
	FlushRenderingCommands();

	// Pass live scene capture RT to scorecard material (DishRender); no CPU bake.
	PendingScorecardDishTexture = DishCaptureRenderTarget;
	LastDishCaptureTexture = DishCaptureRenderTarget;
	bStationDishCaptureValidForScorecard = true;
}

void AProjectUmeowmiCharacter::PopulateScorecardWidget(UPUScorecardWidget* ScorecardWidget, UTexture* OptionalDishTexture)
{
	if (!ScorecardWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Scorecard] PopulateScorecardWidget aborted: ScorecardWidget is null"));
		return;
	}

	ScorecardWidget->ShowFromOrder(CurrentOrder, OptionalDishTexture);
}

bool AProjectUmeowmiCharacter::RefreshDishCapturePreviewFromDishPreview()
{
	if (!bEnableDishCaptureForScorecard || !DishPreviewComponent || !DishCaptureComponent || !GetWorld())
	{
		return false;
	}
	if (!DishPreviewComponent->HasPreview())
	{
		return false;
	}

	const FRotator SavedDishPreviewRot = DishPreviewComponent->GetRelativeRotation();
	if (!DishCapturePreviewMeshRotationOffset.Equals(FRotator::ZeroRotator, KINDA_SMALL_NUMBER))
	{
		// ComposeRotators(A,B) = first A then B (implemented as B*A). Wrong order was Saved*Offset; use engine combine.
		DishPreviewComponent->SetRelativeRotation(
			UKismetMathLibrary::ComposeRotators(SavedDishPreviewRot, DishCapturePreviewMeshRotationOffset));
	}

	TArray<UPrimitiveComponent*> PreviewPrims;
	DishPreviewComponent->GatherSnapshotPrimitives(PreviewPrims);
	if (PreviewPrims.Num() == 0)
	{
		DishPreviewComponent->SetRelativeRotation(SavedDishPreviewRot);
		return false;
	}

	EnsureDishCaptureRenderTarget();
	const FEngineShowFlags SavedShowFlags = DishCaptureComponent->ShowFlags;
	const ESceneCaptureSource SavedCaptureSource = DishCaptureComponent->CaptureSource;
	const FDishCaptureSavedPostProcess SavedPP = SaveDishCapturePostProcess(DishCaptureComponent);
	DishCaptureComponent->CaptureSource = SCS_SceneColorHDR;
	ApplyDishCaptureCapturePostProcess(DishCaptureComponent, bDishCapturePostProcessingTone, DishCapturePostProcessBlendWeightForCapture, DishCaptureExposureBias);
	DishCaptureComponent->TextureTarget = DishCaptureRenderTarget;
	DishCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	DishCaptureComponent->ShowOnlyComponents.Empty();
	for (UPrimitiveComponent* P : PreviewPrims)
	{
		if (P && IsValid(P))
		{
			DishCaptureComponent->ShowOnlyComponents.Add(P);
		}
	}
	DishCaptureComponent->ShowOnlyActors.Empty();
	DishCaptureComponent->HiddenComponents.Empty();
	DishCaptureComponent->HiddenActors.Empty();
	ApplyDishOnlyCaptureShowFlags(DishCaptureComponent, bDishCapturePostProcessingTone);
	UKismetRenderingLibrary::ClearRenderTarget2D(this, DishCaptureRenderTarget, FLinearColor::Transparent);
	FBox PreviewBounds = DishPreviewComponent->ComputePreviewWorldBounds();
	PreviewBounds = SnapDishCaptureWorldBounds(PreviewBounds, DishCaptureBoundsPaddingUU, DishCaptureBoundsExtentSnapUU);
	if (bUseAutomaticDishCaptureFraming)
	{
		ConfigureDishCaptureCameraFromWorldBounds(PreviewBounds, DishPreviewComponent->GetComponentLocation());
	}
	FVector DishFocus = DishPreviewComponent->GetComponentLocation();
	if (PreviewBounds.IsValid != 0)
	{
		DishFocus = PreviewBounds.GetCenter();
	}
	ApplyDishCaptureCameraTweaks(DishFocus);
	DishCaptureComponent->CaptureScene();
	DishCaptureComponent->CaptureSource = SavedCaptureSource;
	RestoreDishCapturePostProcess(DishCaptureComponent, SavedPP);
	DishCaptureComponent->ShowFlags = SavedShowFlags;
	DishCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	DishCaptureComponent->ShowOnlyComponents.Empty();
	FlushRenderingCommands();
	DishPreviewComponent->SetRelativeRotation(SavedDishPreviewRot);
	LastDishCaptureTexture = DishCaptureRenderTarget;
	return true;
}

UPUScorecardWidget* AProjectUmeowmiCharacter::ShowScorecard(UPUScorecardWidget* ScorecardWidget)
{
	if (!bCurrentOrderCompleted || !ScorecardWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Scorecard] ShowScorecard aborted: order not completed or widget null"));
		return nullptr;
	}

	// Replace previous dialogue ShowScorecard overlay (same Z as elevated embedded scorecard).
	if (ActiveDialogueShowScorecardWidget && ActiveDialogueShowScorecardWidget != ScorecardWidget)
	{
		if (ActiveDialogueShowScorecardWidget->IsInViewport())
		{
			ActiveDialogueShowScorecardWidget->RemoveFromParent();
		}
		ActiveDialogueShowScorecardWidget = nullptr;
	}

	// Embedded scorecards cannot AddToViewport (already parented); root-level gets scoring-stack Z.
	if (ScorecardWidget->GetParent())
	{
		// Populate only.
	}
	else
	{
		ScorecardWidget->AddToViewport(PUScorecardViewportZOrder);
	}

	if (bEnableDishCaptureForScorecard && PendingScorecardDishTexture)
	{
		UTexture* DishTex = PendingScorecardDishTexture;
		PendingScorecardDishTexture = nullptr;
		LastDishCaptureTexture = DishTex;
		PopulateScorecardWidget(ScorecardWidget, DishTex);
		ActiveDialogueShowScorecardWidget = ScorecardWidget;
		return ScorecardWidget;
	}

	// Pending is cleared after the first ShowScorecard; do not re-capture from head preview (different camera + scale).
	if (bEnableDishCaptureForScorecard && bStationDishCaptureValidForScorecard && DishCaptureRenderTarget)
	{
		LastDishCaptureTexture = DishCaptureRenderTarget;
		PopulateScorecardWidget(ScorecardWidget, DishCaptureRenderTarget);
		ActiveDialogueShowScorecardWidget = ScorecardWidget;
		return ScorecardWidget;
	}

	if (bEnableDishCaptureForScorecard && DishPreviewComponent && DishPreviewComponent->HasPreview() && DishCaptureComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Scorecard] Texture source: FALLBACK — capturing DishPreview (above-head) now; station snapshot was missing or invalid. For plating-station shot, ensure EndPlatingStage ran (exit customization while in plating) and GatherDishSnapshotPrimitives returned prims."));
		if (!RefreshDishCapturePreviewFromDishPreview())
		{
			PopulateScorecardWidget(ScorecardWidget, nullptr);
			ActiveDialogueShowScorecardWidget = ScorecardWidget;
			return ScorecardWidget;
		}

		TWeakObjectPtr<AProjectUmeowmiCharacter> WeakThis(this);
		TWeakObjectPtr<UPUScorecardWidget> WeakWidget(ScorecardWidget);
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakThis, WeakWidget]()
		{
			if (!WeakThis.IsValid() || !WeakWidget.IsValid())
			{
				return;
			}
			AProjectUmeowmiCharacter* Self = WeakThis.Get();
			UPUScorecardWidget* Widget = WeakWidget.Get();
			if (Self->DishCaptureRenderTarget)
			{
				Self->LastDishCaptureTexture = Self->DishCaptureRenderTarget;
				Self->PopulateScorecardWidget(Widget, Self->DishCaptureRenderTarget);
			}
			else
			{
				Self->PopulateScorecardWidget(Widget, nullptr);
			}
		}));

		ActiveDialogueShowScorecardWidget = ScorecardWidget;
		return ScorecardWidget;
	}

	PopulateScorecardWidget(ScorecardWidget, nullptr);
	ActiveDialogueShowScorecardWidget = ScorecardWidget;
	return ScorecardWidget;
}

void AProjectUmeowmiCharacter::TearDownElevatedDishScoringScorecard()
{
	if (!ElevatedDishScoringScorecard)
	{
		return;
	}
	if (ElevatedDishScoringScorecard->IsInViewport())
	{
		ElevatedDishScoringScorecard->RemoveFromParent();
	}
	ElevatedDishScoringScorecard = nullptr;
}

void AProjectUmeowmiCharacter::TearDownDialogueShowScorecardWidget()
{
	if (!ActiveDialogueShowScorecardWidget)
	{
		return;
	}
	if (ActiveDialogueShowScorecardWidget->IsInViewport())
	{
		ActiveDialogueShowScorecardWidget->RemoveFromParent();
	}
	ActiveDialogueShowScorecardWidget = nullptr;
}

void AProjectUmeowmiCharacter::RemoveOrphanScoringStackViewportWidgets()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<UUserWidget*> Found;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Found, UPUDishScoringWidget::StaticClass(), false);
	for (UUserWidget* W : Found)
	{
		UPUDishScoringWidget* D = Cast<UPUDishScoringWidget>(W);
		if (!D || !D->IsInViewport())
		{
			continue;
		}
		UE_LOG(LogTemp, Display, TEXT("%s RemoveOrphanScoringStackViewportWidgets: removing stray UPUDishScoringWidget %s"), PUDialogueScoringLog::Tag, *D->GetClass()->GetName());
		D->RemoveFromParent();
		if (ActiveDishScoringWidget == D)
		{
			ActiveDishScoringWidget = nullptr;
		}
	}

	Found.Reset();
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Found, UPUScorecardWidget::StaticClass(), false);
	for (UUserWidget* W : Found)
	{
		UPUScorecardWidget* S = Cast<UPUScorecardWidget>(W);
		if (!S || !S->IsInViewport())
		{
			continue;
		}
		UE_LOG(LogTemp, Display, TEXT("%s RemoveOrphanScoringStackViewportWidgets: removing stray UPUScorecardWidget %s"), PUDialogueScoringLog::Tag, *S->GetClass()->GetName());
		S->RemoveFromParent();
		if (ElevatedDishScoringScorecard == S)
		{
			ElevatedDishScoringScorecard = nullptr;
		}
		if (ActiveDialogueShowScorecardWidget == S)
		{
			ActiveDialogueShowScorecardWidget = nullptr;
		}
	}
}

void AProjectUmeowmiCharacter::ElevateEmbeddedDishScoringScorecardAboveDialogue()
{
	// After the first elevation the scorecard is no longer under the dish widget tree; keep the existing viewport slot.
	if (ElevatedDishScoringScorecard && ElevatedDishScoringScorecard->IsInViewport())
	{
		return;
	}

	if (!ActiveDishScoringWidget)
	{
		return;
	}

	UPUScorecardWidget* Scorecard = FindFirstScorecardWidgetRecursive(ActiveDishScoringWidget->GetRootWidget());
	if (!Scorecard)
	{
		return;
	}

	// Detach from dish layout (Z 50000) and add above scoring dialogue (50001) so both stay visible.
	if (Scorecard->GetParent() || Scorecard->IsInViewport())
	{
		Scorecard->RemoveFromParent();
	}
	Scorecard->AddToViewport(PUScorecardViewportZOrder);
	Scorecard->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ElevatedDishScoringScorecard = Scorecard;

	UE_LOG(LogTemp, Display, TEXT("%s ElevateEmbeddedDishScoringScorecardAboveDialogue: %s -> viewport Z=%d (above dialogue Z=%d)"),
		PUDialogueScoringLog::Tag,
		*Scorecard->GetClass()->GetName(),
		PUScorecardViewportZOrder,
		PUScoringDialogueViewportZOrder);
}

void AProjectUmeowmiCharacter::RemoveActiveDishScoringWidgetFromViewport()
{
	if (!ActiveDishScoringWidget)
	{
		return;
	}
	TearDownElevatedDishScoringScorecard();
	UE_LOG(LogTemp, Display, TEXT("%s RemoveActiveDishScoringWidgetFromViewport: %s"), PUDialogueScoringLog::Tag, *ActiveDishScoringWidget->GetClass()->GetName());
	UPUDishScoringWidget* Widget = ActiveDishScoringWidget;
	ActiveDishScoringWidget = nullptr;
	Widget->OnExitingDishScoringMode();
	Widget->SetDishScoringOwnerCharacter(nullptr);
	if (Widget->IsInViewport())
	{
		Widget->RemoveFromParent();
	}
}

void AProjectUmeowmiCharacter::ApplyScoringDialogueViewportLayer()
{
	UE_LOG(LogTemp, Display, TEXT("%s [1/ApplyScoringDialogueViewportLayer] ENTER char=%s bUsingScoringDialogueBox=%d bRelayeredOnly=%d ScoringDlgClass=%s DefaultDlgClass=%s CurrentDialogueBox=%s"),
		PUDialogueScoringLog::Tag,
		*GetName(),
		bUsingScoringDialogueBox,
		bDialogueBoxRelayeredToScoringStackOnly,
		ScoringDialogueBoxWidgetClass ? *ScoringDialogueBoxWidgetClass->GetName() : TEXT("null"),
		DefaultDialogueBoxWidgetClass ? *DefaultDialogueBoxWidgetClass->GetName() : TEXT("null"),
		DialogueBox ? *DialogueBox->GetClass()->GetName() : TEXT("null"));
	if (bUsingScoringDialogueBox || bDialogueBoxRelayeredToScoringStackOnly)
	{
		UE_LOG(LogTemp, Display, TEXT("%s [1/ApplyScoringDialogueViewportLayer] SKIP (already on scoring stack)"),
			PUDialogueScoringLog::Tag);
		return;
	}
	if (ScoringDialogueBoxWidgetClass)
	{
		UE_LOG(LogTemp, Display, TEXT("%s [1/ApplyScoringDialogueViewportLayer] CALL SwapToScoringDialogueBox class=%s"),
			PUDialogueScoringLog::Tag, *ScoringDialogueBoxWidgetClass->GetName());
		SwapToScoringDialogueBox();
		return;
	}
	if (DialogueBox)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Dialogue] ScoringDialogueBoxWidgetClass is not set on %s — reparenting current DialogueBox to scoring viewport Z (%d). Set ScoringDialogueBoxWidgetClass for a dedicated scoring layout."),
			*GetName(), PUScoringDialogueViewportZOrder);
		UE_LOG(LogTemp, Display, TEXT("%s ApplyScoringDialogueViewportLayer: relayering existing DialogueBox=%s to Z=%d (no ScoringDialogueBoxWidgetClass)"),
			PUDialogueScoringLog::Tag, *DialogueBox->GetClass()->GetName(), PUScoringDialogueViewportZOrder);
		if (DialogueBox->IsInViewport())
		{
			DialogueBox->RemoveFromParent();
		}
		DialogueBox->AddToViewport(PUScoringDialogueViewportZOrder);
		DialogueBox->SetVisibility(ESlateVisibility::Visible);
		bDialogueBoxRelayeredToScoringStackOnly = true;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s ApplyScoringDialogueViewportLayer: no DialogueBox and no ScoringDialogueBoxWidgetClass — nothing to do"), PUDialogueScoringLog::Tag);
	}
}

void AProjectUmeowmiCharacter::SwapToScoringDialogueBox()
{
	if (!ScoringDialogueBoxWidgetClass || bUsingScoringDialogueBox)
	{
		UE_LOG(LogTemp, Display, TEXT("%s [2/SwapToScoringDialogueBox] NO-OP class=%s bUsingScoringDialogueBox=%d"),
			PUDialogueScoringLog::Tag,
			ScoringDialogueBoxWidgetClass ? *ScoringDialogueBoxWidgetClass->GetName() : TEXT("null"),
			bUsingScoringDialogueBox);
		return;
	}
	bDialogueBoxRelayeredToScoringStackOnly = false;
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s [2/SwapToScoringDialogueBox] ABORT no PlayerController"), PUDialogueScoringLog::Tag);
		return;
	}

	UPUDialogueBox* const OldBox = DialogueBox;
	UPUDialogueBox* NewBox = CreateWidget<UPUDialogueBox>(PC, ScoringDialogueBoxWidgetClass);
	if (!NewBox)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s [2/SwapToScoringDialogueBox] ABORT CreateWidget failed for %s"),
			PUDialogueScoringLog::Tag, *ScoringDialogueBoxWidgetClass->GetName());
		return;
	}
	UE_LOG(LogTemp, Display, TEXT("%s [2/SwapToScoringDialogueBox] Created NewBox=%p class=%s OldBox=%p"),
		PUDialogueScoringLog::Tag, NewBox, *NewBox->GetClass()->GetName(), OldBox);

	if (DialogueBox)
	{
		CachedNonScoringDialogueBoxClass = DialogueBox->GetClass();
		DialogueBox->RemoveFromParent();
	}
	else if (DefaultDialogueBoxWidgetClass)
	{
		CachedNonScoringDialogueBoxClass = DefaultDialogueBoxWidgetClass;
	}
	else
	{
		CachedNonScoringDialogueBoxClass = nullptr;
	}

	DialogueBox = NewBox;
	bUsingScoringDialogueBox = true;

	// NativeConstruct may have AddToViewport() at default Z while Hidden — remove so we have a single slot at the scoring Z.
	if (NewBox->IsInViewport())
	{
		NewBox->RemoveFromParent();
	}
	NewBox->AddToViewport(PUScoringDialogueViewportZOrder);
	// After AddToViewport, Slate sync can reapply the Blueprint root visibility (often Hidden for a duplicate layout).
	NewBox->SetVisibility(ESlateVisibility::Visible);
	UE_LOG(LogTemp, Display, TEXT("%s [2/SwapToScoringDialogueBox] DONE DialogueBox ptr=%p new=%s Z=%d InViewport=%d Vis=%d cachedRestoreClass=%s"),
		PUDialogueScoringLog::Tag,
		DialogueBox,
		*NewBox->GetClass()->GetName(),
		PUScoringDialogueViewportZOrder,
		NewBox->IsInViewport() ? 1 : 0,
		(int32)NewBox->GetVisibility(),
		CachedNonScoringDialogueBoxClass ? *CachedNonScoringDialogueBoxClass->GetName() : TEXT("null"));
}

void AProjectUmeowmiCharacter::RestoreNonScoringDialogueBox()
{
	if (!bUsingScoringDialogueBox)
	{
		return;
	}
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Dialogue] RestoreNonScoringDialogueBox: no PlayerController"));
		bUsingScoringDialogueBox = false;
		CachedNonScoringDialogueBoxClass = nullptr;
		return;
	}

	TSubclassOf<UPUDialogueBox> RestoreClass = CachedNonScoringDialogueBoxClass ? CachedNonScoringDialogueBoxClass : DefaultDialogueBoxWidgetClass;
	if (!RestoreClass)
	{
		if (DialogueBox)
		{
			DialogueBox->RemoveFromParent();
			DialogueBox = nullptr;
		}
		bUsingScoringDialogueBox = false;
		CachedNonScoringDialogueBoxClass = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("[Dialogue] RestoreNonScoringDialogueBox: no class to restore (set DefaultDialogueBoxWidgetClass or assign DialogueBox before scoring)"));
		return;
	}

	UPUDialogueBox* NewBox = CreateWidget<UPUDialogueBox>(PC, RestoreClass);
	if (!NewBox)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Dialogue] RestoreNonScoringDialogueBox: CreateWidget failed for %s"), *RestoreClass->GetName());
		return;
	}

	if (DialogueBox)
	{
		DialogueBox->RemoveFromParent();
	}
	DialogueBox = NewBox;
	bUsingScoringDialogueBox = false;
	CachedNonScoringDialogueBoxClass = nullptr;
	UE_LOG(LogTemp, Display, TEXT("%s RestoreNonScoringDialogueBox: restored DialogueBox=%s"), PUDialogueScoringLog::Tag, *NewBox->GetClass()->GetName());
}

void AProjectUmeowmiCharacter::BeginDishScoringModeFromClass()
{
	(void)TryBeginDishScoringModeFromClass();
}

bool AProjectUmeowmiCharacter::TryBeginDishScoringModeFromClass()
{
	if (!DishScoringWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DishScoring] TryBeginDishScoringModeFromClass: DishScoringWidgetClass not set on %s"), *GetName());
		return false;
	}
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DishScoring] TryBeginDishScoringModeFromClass: no PlayerController"));
		return false;
	}
	UPUDishScoringWidget* Widget = CreateWidget<UPUDishScoringWidget>(PC, DishScoringWidgetClass);
	if (!Widget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DishScoring] TryBeginDishScoringModeFromClass: CreateWidget failed for class %s"),
			*DishScoringWidgetClass->GetName());
		return false;
	}
	return BeginDishScoringModeWithWidget(Widget);
}

bool AProjectUmeowmiCharacter::BeginDishScoringModeWithWidget(UPUDishScoringWidget* DishScoringWidget)
{
	UE_LOG(LogTemp, Display, TEXT("%s [3/BeginDishScoringModeWithWidget] ENTER widget=%p class=%s"),
		PUDialogueScoringLog::Tag, DishScoringWidget, DishScoringWidget ? *DishScoringWidget->GetClass()->GetName() : TEXT("null"));
	if (!DishScoringWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s [3/BeginDishScoringModeWithWidget] ABORT null widget"), PUDialogueScoringLog::Tag);
		return false;
	}

	if (ActiveDishScoringWidget != nullptr && ActiveDishScoringWidget != DishScoringWidget)
	{
		UE_LOG(LogTemp, Display, TEXT("%s [3/BeginDishScoringModeWithWidget] replacing previous scoring widget"), PUDialogueScoringLog::Tag);
		RemoveActiveDishScoringWidgetFromViewport();
	}
	else if (ActiveDishScoringWidget == DishScoringWidget)
	{
		UE_LOG(LogTemp, Display, TEXT("%s [3/BeginDishScoringModeWithWidget] same instance re-add viewport Z=%d"), PUDialogueScoringLog::Tag, PUScoringSceneViewportZOrder);
		DishScoringWidget->AddToViewport(PUScoringSceneViewportZOrder);
		// Front layer: pass pointer through empty areas to dialogue/scorecard below (interactive children still hit-test).
		DishScoringWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ElevateEmbeddedDishScoringScorecardAboveDialogue();
		return true;
	}

	if (!bUsingScoringDialogueBox && !bDialogueBoxRelayeredToScoringStackOnly)
	{
		UE_LOG(LogTemp, Display, TEXT("%s [3/BeginDishScoringModeWithWidget] CALL ApplyScoringDialogueViewportLayer"), PUDialogueScoringLog::Tag);
		ApplyScoringDialogueViewportLayer();
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("%s [3/BeginDishScoringModeWithWidget] SKIP ApplyScoringDialogueViewportLayer (already layered) bUsingScoringBox=%d bRelayeredOnly=%d"),
			PUDialogueScoringLog::Tag, bUsingScoringDialogueBox, bDialogueBoxRelayeredToScoringStackOnly);
	}

	ActiveDishScoringWidget = DishScoringWidget;
	DishScoringWidget->SetDishScoringOwnerCharacter(this);
	DishScoringWidget->AddToViewport(PUScoringSceneViewportZOrder);
	DishScoringWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	DishScoringWidget->OnEnteredDishScoringMode();
	// After BP builds children: embedded scorecard must sit above scoring dialogue (50001) or dialogue covers it.
	ElevateEmbeddedDishScoringScorecardAboveDialogue();
	UE_LOG(LogTemp, Display, TEXT("%s [3/BeginDishScoringModeWithWidget] DONE scoringWgt=%p class=%s InViewport=%d bUsingScoringBox=%d bRelayeredOnly=%d DialogueBox=%p %s Z_scoringDlg=%d Z_scene=%d"),
		PUDialogueScoringLog::Tag,
		DishScoringWidget,
		*DishScoringWidget->GetClass()->GetName(),
		DishScoringWidget->IsInViewport() ? 1 : 0,
		bUsingScoringDialogueBox,
		bDialogueBoxRelayeredToScoringStackOnly,
		DialogueBox,
		DialogueBox ? *DialogueBox->GetClass()->GetName() : TEXT("null"),
		PUScoringDialogueViewportZOrder,
		PUScoringSceneViewportZOrder);
	return true;
}

void AProjectUmeowmiCharacter::EndDishScoringMode()
{
	// Always remove elevated scorecard from viewport (Z 50002). If ActiveDishScoringWidget was already cleared,
	// RemoveActiveDishScoringWidgetFromViewport never ran and the scorecard could still steal mouse hits above dialogue.
	TearDownElevatedDishScoringScorecard();
	// ShowScorecard dialogue event creates a separate widget at the same Z — it is not ElevatedDishScoringScorecard.
	TearDownDialogueShowScorecardWidget();

	const bool bHadActiveScoringWidget = (ActiveDishScoringWidget != nullptr);
	UE_LOG(LogTemp, Display, TEXT("%s EndDishScoringMode: hadActiveScoringWidget=%d bUsingScoringDialogueBox=%d bRelayeredOnly=%d"),
		PUDialogueScoringLog::Tag,
		bHadActiveScoringWidget ? 1 : 0,
		bUsingScoringDialogueBox ? 1 : 0,
		bDialogueBoxRelayeredToScoringStackOnly ? 1 : 0);

	if (bHadActiveScoringWidget)
	{
		RemoveActiveDishScoringWidgetFromViewport();
	}
	RemoveOrphanScoringStackViewportWidgets();

	// Restore default dialogue layout whenever we were on the scoring stack — independent of ActiveDishScoringWidget.
	// Otherwise: ShowScorecard/SyncDialogueBoxToScoringLayer can swap to WBP_ScorecardDialogueBox without BeginDishScoring
	// (ActiveDishScoringWidget still null). EndDishScoring would skip restore + refresh and leave the scoring dialogue
	// (or wrong Z) so click-to-advance on the "original" widget never comes back.
	if (bUsingScoringDialogueBox)
	{
		UE_LOG(LogTemp, Display, TEXT("%s EndDishScoringMode: restoring non-scoring dialogue box (swap path)"), PUDialogueScoringLog::Tag);
		RestoreNonScoringDialogueBox();
	}
	else if (bDialogueBoxRelayeredToScoringStackOnly && DialogueBox)
	{
		UE_LOG(LogTemp, Display, TEXT("%s EndDishScoringMode: restoring dialogue viewport Z from relayer-only path"), PUDialogueScoringLog::Tag);
		DialogueBox->RemoveFromParent();
			DialogueBox->AddToViewport(PUScoringDialogueViewportZOrder);
			DialogueBox->SetVisibility(ESlateVisibility::Visible);
		bDialogueBoxRelayeredToScoringStackOnly = false;
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("%s EndDishScoringMode: no dialogue layout restore (bUsingScoringDialogueBox=%d bRelayeredOnly=%d)"),
			PUDialogueScoringLog::Tag, bUsingScoringDialogueBox, bDialogueBoxRelayeredToScoringStackOnly);
	}

	if (ATalkingObject* TO = GetCurrentTalkingObject())
	{
		if (UDlgContext* DialogueCtx = TO->GetCurrentDialogueContext())
		{
			UE_LOG(LogTemp, Display, TEXT("%s EndDishScoringMode: RefreshDialogueBoxFromContext talkingObject=%s"), PUDialogueScoringLog::Tag, *TO->GetName());
			RefreshDialogueBoxFromContext(DialogueCtx);
		}
		else
		{
			UE_LOG(LogTemp, Display, TEXT("%s EndDishScoringMode: no GetCurrentDialogueContext on %s — skip refresh"), PUDialogueScoringLog::Tag, *TO->GetName());
		}
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("%s EndDishScoringMode: GetCurrentTalkingObject() null — skip refresh"), PUDialogueScoringLog::Tag);
	}
}

bool AProjectUmeowmiCharacter::IsInDishScoringMode() const
{
	return ActiveDishScoringWidget != nullptr;
}

void AProjectUmeowmiCharacter::RefreshDialogueBoxFromContext(UDlgContext* Context)
{
	if (!Context || !DialogueBox)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s [4/RefreshDialogueBoxFromContext] ABORT Context=%s DialogueBox=%s"),
			PUDialogueScoringLog::Tag,
			Context ? TEXT("OK") : TEXT("null"),
			DialogueBox ? *DialogueBox->GetClass()->GetName() : TEXT("null"));
		return;
	}
	const int32 OptionsNum = Context->GetOptionsNum();
	const bool bEnded = Context->HasDialogueEnded();
	const int32 ActiveIdx = Context->GetActiveNodeIndex();
	const FString CtxStr = Context->GetContextString();
	UE_LOG(LogTemp, Display, TEXT("%s [4/RefreshDialogueBoxFromContext] QUEUE next tick box=%p %s ctx=%p HasEnded=%d ActiveNodeIndex=%d OptionsNum=%d Time=%.4f"),
		PUDialogueScoringLog::Tag,
		DialogueBox,
		*DialogueBox->GetClass()->GetName(),
		Context,
		bEnded ? 1 : 0,
		ActiveIdx,
		OptionsNum,
		GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0);
	UE_LOG(LogTemp, Display, TEXT("%s [4/RefreshDialogueBoxFromContext] DlgContextString: %s"), PUDialogueScoringLog::Tag, *CtxStr);

	// Dialogue already ended — do not queue OpenFromContextResync (would re-show a blank box with stale portrait).
	if (bEnded)
	{
		UE_LOG(LogTemp, Display, TEXT("%s [4/RefreshDialogueBoxFromContext] SKIP queue (HasDialogueEnded at schedule time)"), PUDialogueScoringLog::Tag);
		if (DialogueBox->IsInViewport())
		{
			DialogueBox->Close();
		}
		return;
	}

	// Dlg runs FireNodeEnterEvents (OnDialogueEvent / BeginDishScoring) BEFORE ReevaluateChildren on the same node.
	// Same-frame Update/OpenFromContextResync sees stale GetOptionsNum() and edge data — breaks options, typewriter, and can trip Close() paths.
	if (UWorld* World = GetWorld())
	{
		TWeakObjectPtr<UPUDialogueBox> WeakBox(DialogueBox);
		TWeakObjectPtr<UDlgContext> WeakCtx(Context);
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakBox, WeakCtx]()
		{
			if (!WeakBox.IsValid() || !WeakCtx.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("%s [4/RefreshDialogueBoxFromContext] DEFERRED ABORT boxValid=%d ctxValid=%d (destroyed before tick?)"),
					PUDialogueScoringLog::Tag, WeakBox.IsValid(), WeakCtx.IsValid());
				return;
			}
			const int32 OptAfter = WeakCtx->GetOptionsNum();
			const bool bEndedAfter = WeakCtx->HasDialogueEnded();
			UE_LOG(LogTemp, Display, TEXT("%s [4/RefreshDialogueBoxFromContext] DEFERRED RUN box=%p %s ctx=%p HasEnded=%d ActiveNodeIndex=%d OptionsNum=%d Time=%.4f"),
				PUDialogueScoringLog::Tag,
				WeakBox.Get(),
				WeakBox.IsValid() ? *WeakBox->GetClass()->GetName() : TEXT("?"),
				WeakCtx.Get(),
				bEndedAfter ? 1 : 0,
				WeakCtx->GetActiveNodeIndex(),
				OptAfter,
				WeakBox->GetWorld() ? WeakBox->GetWorld()->GetTimeSeconds() : -1.0);
			// Between schedule and this tick, ChooseOption can advance to End — Update already Close()d; do not reopen.
			if (bEndedAfter)
			{
				UE_LOG(LogTemp, Display, TEXT("%s [4/RefreshDialogueBoxFromContext] DEFERRED SKIP (HasDialogueEnded) — not OpenFromContextResync; Close if still in viewport"),
					PUDialogueScoringLog::Tag);
				if (WeakBox->IsInViewport())
				{
					WeakBox->Close();
				}
				return;
			}
			WeakBox->OpenFromContextResync(WeakCtx.Get());
			UE_LOG(LogTemp, Display, TEXT("%s [4/RefreshDialogueBoxFromContext] after OpenFromContextResync Vis=%d InViewport=%d"),
				PUDialogueScoringLog::Tag,
				(int32)WeakBox->GetVisibility(),
				WeakBox->IsInViewport() ? 1 : 0);
			WeakBox->SetDialogueInputFocus();
			UE_LOG(LogTemp, Display, TEXT("%s [4/RefreshDialogueBoxFromContext] after SetDialogueInputFocus"), PUDialogueScoringLog::Tag);
		}));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s [4/RefreshDialogueBoxFromContext] ABORT no World (cannot schedule tick)"), PUDialogueScoringLog::Tag);
	}
}

void AProjectUmeowmiCharacter::SyncDialogueBoxToScoringLayer(UDlgContext* Context)
{
	if (!Context)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s [SyncScoringLayer] ABORT Context null"), PUDialogueScoringLog::Tag);
		return;
	}
	UE_LOG(LogTemp, Display, TEXT("%s [SyncScoringLayer] ENTER ctx=%p HasEnded=%d ActiveIdx=%d OptionsNum=%d"),
		PUDialogueScoringLog::Tag,
		Context,
		Context->HasDialogueEnded() ? 1 : 0,
		Context->GetActiveNodeIndex(),
		Context->GetOptionsNum());
	UE_LOG(LogTemp, Display, TEXT("%s [SyncScoringLayer] DlgContextString: %s"), PUDialogueScoringLog::Tag, *Context->GetContextString());
	ApplyScoringDialogueViewportLayer();
	RefreshDialogueBoxFromContext(Context);
	UE_LOG(LogTemp, Display, TEXT("%s [SyncScoringLayer] EXIT (refresh queued or aborted)"), PUDialogueScoringLog::Tag);
}

void AProjectUmeowmiCharacter::OnOrderFailed()
{
	//UE_LOG(LogTemp,Display, TEXT("ProjectUmeowmiCharacter::OnOrderFailed - Order completed with low satisfaction"));
	
	// This function can be overridden in Blueprints to add visual/audio feedback
	// Note: Orders are now always completed - this event is for low satisfaction scenarios
	//UE_LOG(LogTemp,Display, TEXT("⚠️ ORDER COMPLETED WITH LOW SATISFACTION ⚠️"));
	//UE_LOG(LogTemp,Display, TEXT("Satisfaction: %.1f%% - Try again for better results!"), CurrentOrderSatisfaction * 100.0f);
}

void AProjectUmeowmiCharacter::CleanupOrderUObjectReferences(FPUOrderBase& Order)
{
	// Clear UObject references in the completed dish
	if (Order.CompletedDish.PreviewTexture)
	{
		Order.CompletedDish.PreviewTexture = nullptr;
	}
	if (Order.CompletedDish.IngredientDataTable.IsValid())
	{
		Order.CompletedDish.IngredientDataTable = nullptr;
	}
	
	// Clear UObject references in all ingredient instances
	for (FIngredientInstance& Instance : Order.CompletedDish.IngredientInstances)
	{
		if (Instance.IngredientData.PreviewTexture)
		{
			Instance.IngredientData.PreviewTexture = nullptr;
		}
		if (Instance.IngredientData.MaterialInstance.IsValid())
		{
			Instance.IngredientData.MaterialInstance = nullptr;
		}
		if (Instance.IngredientData.IngredientMesh.IsValid())
		{
			Instance.IngredientData.IngredientMesh = nullptr;
		}
		if (Instance.IngredientData.PreparationDataTable.IsValid())
		{
			Instance.IngredientData.PreparationDataTable = nullptr;
		}
	}
	
	// Clear UObject references in the base dish
	if (Order.BaseDish.PreviewTexture)
	{
		Order.BaseDish.PreviewTexture = nullptr;
	}
	if (Order.BaseDish.IngredientDataTable.IsValid())
	{
		Order.BaseDish.IngredientDataTable = nullptr;
	}
	
	// Clear UObject references in base dish ingredient instances
	for (FIngredientInstance& Instance : Order.BaseDish.IngredientInstances)
	{
		if (Instance.IngredientData.PreviewTexture)
		{
			Instance.IngredientData.PreviewTexture = nullptr;
		}
		if (Instance.IngredientData.MaterialInstance.IsValid())
		{
			Instance.IngredientData.MaterialInstance = nullptr;
		}
		if (Instance.IngredientData.IngredientMesh.IsValid())
		{
			Instance.IngredientData.IngredientMesh = nullptr;
		}
		if (Instance.IngredientData.PreparationDataTable.IsValid())
		{
			Instance.IngredientData.PreparationDataTable = nullptr;
		}
	}
}

void AProjectUmeowmiCharacter::ShowMouseCursor()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;
		PC->CurrentMouseCursor = EMouseCursor::Default;
	}
}

void AProjectUmeowmiCharacter::HideMouseCursor()
{
	// Do nothing - we want the cursor to always be visible
}

void AProjectUmeowmiCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//UE_LOG(LogTemp,Log, TEXT("ProjectUmeowmiCharacter::EndPlay - Cleaning up character: %s"), *GetName());

	PopJournalInputMappingLayer();
	
	// Clear order UObject references to prevent garbage collection issues
	if (bHasCurrentOrder || bCurrentOrderCompleted)
	{
		//UE_LOG(LogTemp,Log, TEXT("ProjectUmeowmiCharacter::EndPlay - Cleaning up order UObject references"));
		CleanupOrderUObjectReferences(CurrentOrder);
		
		// Clear the order data
		CurrentOrder = FPUOrderBase();
		bHasCurrentOrder = false;
		bCurrentOrderCompleted = false;
		CurrentOrderSatisfaction = 0.0f;
		PendingScorecardDishTexture = nullptr;
		bStationDishCaptureValidForScorecard = false;
	}
	
	// Clear overlapping talking objects to prevent dangling references
	OverlappingTalkingObjects.Empty();
	SelectedTalkingObjectIndex = 0;
	
	// Clear interactable reference
	if (CurrentInteractable)
	{
		//UE_LOG(LogTemp,Log, TEXT("ProjectUmeowmiCharacter::EndPlay - Clearing interactable reference"));
		CurrentInteractable = nullptr;
	}
	
	// Clear dialogue box reference
	if (DialogueBox)
	{
		//UE_LOG(LogTemp,Log, TEXT("ProjectUmeowmiCharacter::EndPlay - Clearing dialogue box reference"));
		DialogueBox = nullptr;
	}

	// Clear journal widget reference
	if (JournalWidget)
	{
		JournalWidget = nullptr;
	}

	// Clear any pending emote timers
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EmoteHideTimerHandle);
		World->GetTimerManager().ClearTimer(EmoteFadeOutTimerHandle);
	}

	if (QuestObjectiveOffscreenIndicator)
	{
		QuestObjectiveOffscreenIndicator->RemoveFromParent();
		QuestObjectiveOffscreenIndicator = nullptr;
	}
	
	Super::EndPlay(EndPlayReason);
}

void AProjectUmeowmiCharacter::SetMousePosition(int32 X, int32 Y)
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetMouseLocation(X, Y);
	}
}

void AProjectUmeowmiCharacter::CenterMouseCursor()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		int32 ViewportSizeX, ViewportSizeY;
		PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
		PC->SetMouseLocation(ViewportSizeX / 2, ViewportSizeY / 2);
	}
}


