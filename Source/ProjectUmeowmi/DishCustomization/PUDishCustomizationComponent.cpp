#include "PUDishCustomizationComponent.h"
#include "PUDishPreviewComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "../ProjectUmeowmiCharacter.h"
#include "../PUProjectUmeowmiGameInstance.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "InputMappingContext.h"
#include "Engine/GameViewportClient.h"
#include "../UI/PUDishCustomizationWidget.h"
#include "../UI/PUPlatingWidget.h"
#include "../UI/PUIngredientSlot.h"
#include "Engine/StaticMeshActor.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "PUIngredientMesh.h"
#include "Camera/CameraActor.h"
#include "Framework/Application/SlateApplication.h"
#include "TimerManager.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

// Debug output toggles (kept in code, but disabled by default to avoid log spam).
namespace
{
    // Enables logging of every ingredient tag when dish data is updated.
    constexpr bool bPU_LogDishDataIngredientTags = false;

    // Enables logging for ingredient drag (click detection, trace hits, position updates).
    constexpr bool bPU_LogIngredientDrag = true;

    // Enables logging for movement restoration when exiting customization (to debug "can look but not move").
    constexpr bool bPU_LogMovementRestore = true;

    void LogMovementState(APlayerController* PC, const TCHAR* Context)
    {
        if (!bPU_LogMovementRestore || !PC) return;
        bool bIgnoreMove = PC->IsMoveInputIgnored();
        bool bIgnoreLook = PC->IsLookInputIgnored();
        EMovementMode MoveMode = MOVE_None;
        if (APawn* Pawn = PC->GetPawn())
        {
            if (ACharacter* C = Cast<ACharacter>(Pawn))
            {
                if (UCharacterMovementComponent* M = C->GetCharacterMovement())
                    MoveMode = M->MovementMode;
            }
        }
        UE_LOG(LogTemp, Warning, TEXT("[MovementRestore] %s - IgnoreMove=%d IgnoreLook=%d MovementMode=%d"), Context, bIgnoreMove, bIgnoreLook, (int32)MoveMode);
    }
}

void UPUDishCustomizationComponent::SetHUDVisible(bool bShouldBeVisible)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    TArray<UUserWidget*> FoundWidgets;

    // If explicitly provided, just use that class.
    if (HUDWidgetClass)
    {
        UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, FoundWidgets, HUDWidgetClass, /*TopLevelOnly*/ false);
        for (UUserWidget* Widget : FoundWidgets)
        {
            if (IsValid(Widget))
            {
                Widget->SetVisibility(bShouldBeVisible ? ESlateVisibility::Visible : HUDHiddenVisibility);
            }
        }
        return;
    }

    // Fallback: search all user widgets and find something that looks like WBP_HUD.
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, FoundWidgets, UUserWidget::StaticClass(), /*TopLevelOnly*/ false);

    bool bFoundAny = false;
    for (UUserWidget* Widget : FoundWidgets)
    {
        if (!IsValid(Widget))
        {
            continue;
        }

        const FString WidgetName = Widget->GetName();
        const FString ClassName = Widget->GetClass() ? Widget->GetClass()->GetName() : FString();

        // Typical patterns are WBP_HUD_C, WBP_HUD_C_0, etc.
        if (WidgetName.Contains(TEXT("WBP_HUD"), ESearchCase::IgnoreCase) ||
            ClassName.Contains(TEXT("WBP_HUD"), ESearchCase::IgnoreCase))
        {
            Widget->SetVisibility(bShouldBeVisible ? ESlateVisibility::Visible : HUDHiddenVisibility);
            bFoundAny = true;
        }
    }

    if (!bFoundAny)
    {
        //UE_LOG(LogTemp,Verbose, TEXT("UPUDishCustomizationComponent::SetHUDVisible - No HUD widgets found (set HUDWidgetClass to be explicit)."));
    }
}

UPUDishCustomizationComponent::UPUDishCustomizationComponent()
{
    PrimaryComponentTick.bCanEverTick = true; // Enable tick for camera transitions
}

void UPUDishCustomizationComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UPUDishCustomizationComponent::BeginDestroy()
{
#if WITH_EDITOR
    // Unregister Slate delegate before destruction to prevent shutdown crash.
    // EndCustomization() may not be called when editor closes or level unloads.
    if (PreInputMouseDownHandle.IsValid() && FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().OnApplicationMousePreInputButtonDownListener().Remove(PreInputMouseDownHandle);
        PreInputMouseDownHandle.Reset();
    }
#endif
    Super::BeginDestroy();
}

void UPUDishCustomizationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsTransitioningCamera && (CurrentCharacter || CameraTransitionCharacter.Get()))
    {
        UpdateCameraTransition(DeltaTime);
    }

    // Update plating camera transition if active
    if (bPlatingCameraTransitioning)
    {
        UpdatePlatingCameraTransition(DeltaTime);
    }

    // Update mouse dragging if active
    if (bIsDragging)
    {
        UpdateMouseDrag();
    }
    else if (CurrentCharacter && CanSpawnIngredientsIn3D())
    {
        // Fallback: poll for mouse clicks in Tick (widget may block Enhanced Input)
        APlayerController* PC = Cast<APlayerController>(CurrentCharacter->GetController());
        if (PC)
        {
            bool bMouseDown = PC->IsInputKeyDown(EKeys::LeftMouseButton);
            if (bMouseDown && !bWasMouseDown)
            {
                HandleMouseClick(FInputActionValue());
            }
            else if (!bMouseDown && bWasMouseDown && !bIsDragging)
            {
                HandleMouseRelease(FInputActionValue());
            }
            bWasMouseDown = bMouseDown;
        }
    }
}

void UPUDishCustomizationComponent::StartCustomization(AProjectUmeowmiCharacter* Character)
{
    //UE_LOG(LogTemp,Display, TEXT("🚀 UPUDishCustomizationComponent::StartCustomization - STARTING CUSTOMIZATION"));
    
    if (!Character)
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - Failed to get Character"));
        return;
    }

    // Store the original dish container mesh before any changes
    StoreOriginalDishContainerMesh();

    //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Character valid: %s"), *Character->GetName());
    CameraTransitionCharacter = nullptr;
    CurrentCharacter = Character;
    bWasMouseDown = false;  // Reset for clean state when entering customization

    // Hide HUD while customizing (WBP_HUD).
    SetHUDVisible(false);

    // Get the player controller
    APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
    if (!PlayerController)
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - Failed to get Player Controller"));
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Player Controller valid: %s"), *PlayerController->GetName());

    // Set input mode first to ensure the input system is ready
    FInputModeGameAndUI InputMode;
    InputMode.SetWidgetToFocus(nullptr);
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PlayerController->SetInputMode(InputMode);

    // Enable mouse cursor and input
    PlayerController->SetIgnoreMoveInput(true);
    PlayerController->SetIgnoreLookInput(true);
    PlayerController->bShowMouseCursor = true;
    PlayerController->CurrentMouseCursor = EMouseCursor::Default;
    PlayerController->bEnableClickEvents = true;
    PlayerController->bEnableMouseOverEvents = true;

    // Center the mouse cursor
    int32 ViewportSizeX, ViewportSizeY;
    PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
    PlayerController->SetMouseLocation(ViewportSizeX / 2, ViewportSizeY / 2);

    //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Input mode set, viewport size: %dx%d"), ViewportSizeX, ViewportSizeY);

    // Handle mapping contexts
    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
    {
        //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Found Enhanced Input Subsystem"));
        
        // Store the original mapping context
        OriginalMappingContext = Character->GetDefaultMappingContext();
        //UE_LOG(LogTemp,Display, TEXT("📋 UPUDishCustomizationComponent::StartCustomization - Original mapping context: %s"), 
        //    OriginalMappingContext ? *OriginalMappingContext->GetName() : TEXT("None"));
        
        // Remove the original mapping context
        if (OriginalMappingContext)
        {
            Subsystem->RemoveMappingContext(OriginalMappingContext);
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Removed original mapping context"));
        }

        // Add the customization mapping context
        if (CustomizationMappingContext)
        {
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Adding customization mapping context: %s"), *CustomizationMappingContext->GetName());
            Subsystem->AddMappingContext(CustomizationMappingContext, 0);
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Added customization mapping context"));
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - No CustomizationMappingContext set"));
        }
    }
    else
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - Failed to get Enhanced Input Subsystem"));
    }

    // Setup input handling for exit action and controller mouse
    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerController->InputComponent))
    {
        //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Found Enhanced Input Component"));
        
        // First unbind any existing bindings to ensure clean state
        if (ControllerMouseAction)
        {
            EnhancedInputComponent->RemoveBindingByHandle(ControllerMouseBindingHandle);
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Removed existing controller mouse binding"));
        }
        
        if (ExitCustomizationAction)
        {
            EnhancedInputComponent->RemoveBindingByHandle(ExitActionBindingHandle);
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Removed existing exit action binding"));
        }

        // Now set up new bindings
        if (ExitCustomizationAction)
        {
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Binding exit action: %s"), *ExitCustomizationAction->GetName());
            ExitActionBindingHandle = EnhancedInputComponent->BindAction(ExitCustomizationAction, ETriggerEvent::Triggered, this, &UPUDishCustomizationComponent::HandleExitInput).GetHandle();
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Exit action bound with handle: %d"), ExitActionBindingHandle);
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - No ExitCustomizationAction set"));
        }

        if (ControllerMouseAction)
        {
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Binding controller mouse action: %s"), *ControllerMouseAction->GetName());
            // Bind to both Ongoing and Triggered events to ensure we catch all input
            ControllerMouseBindingHandle = EnhancedInputComponent->BindAction(ControllerMouseAction, ETriggerEvent::Ongoing, this, &UPUDishCustomizationComponent::HandleControllerMouse).GetHandle();
            EnhancedInputComponent->BindAction(ControllerMouseAction, ETriggerEvent::Triggered, this, &UPUDishCustomizationComponent::HandleControllerMouse);
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Controller mouse action bound with handle: %d"), ControllerMouseBindingHandle);
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - No ControllerMouseAction set"));
        }

        if (MouseClickAction)
        {
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Binding mouse click action: %s"), *MouseClickAction->GetName());
            MouseClickBindingHandle = EnhancedInputComponent->BindAction(MouseClickAction, ETriggerEvent::Started, this, &UPUDishCustomizationComponent::HandleMouseClick).GetHandle();
            EnhancedInputComponent->BindAction(MouseClickAction, ETriggerEvent::Completed, this, &UPUDishCustomizationComponent::HandleMouseRelease);
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Mouse click action bound with handle: %d"), MouseClickBindingHandle);
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - No MouseClickAction set"));
        }

#if WITH_EDITOR
        // Slate pre-input listener: fires BEFORE widgets consume the click (bypasses widget blocking)
        PreInputMouseDownHandle = FSlateApplication::Get().OnApplicationMousePreInputButtonDownListener().AddUObject(this, &UPUDishCustomizationComponent::OnPreInputMouseButtonDown);
#endif // WITH_EDITOR

        // Bind stage navigation actions
        if (NextStageAction)
        {
            NextStageBindingHandle = EnhancedInputComponent->BindAction(NextStageAction, ETriggerEvent::Triggered, this, &UPUDishCustomizationComponent::HandleNextStage).GetHandle();
        }

        if (PreviousStageAction)
        {
            UE_LOG(LogTemp, Warning, TEXT("🔙 Binding PreviousStageAction: %s"), *PreviousStageAction->GetName());
            PreviousStageBindingHandle = EnhancedInputComponent->BindAction(PreviousStageAction, ETriggerEvent::Triggered, this, &UPUDishCustomizationComponent::HandlePreviousStage).GetHandle();
            UE_LOG(LogTemp, Warning, TEXT("🔙 PreviousStageBindingHandle: %d"), PreviousStageBindingHandle);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("🔙 PreviousStageAction is NULL!"));
        }
    }
    else
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - Failed to get Enhanced Input Component"));
    }

    // Create and show the customization widget
    //UE_LOG(LogTemp,Display, TEXT("🎨 UPUDishCustomizationComponent::StartCustomization - About to create widget"));
    //UE_LOG(LogTemp,Display, TEXT("🎨 UPUDishCustomizationComponent::StartCustomization - CustomizationWidgetClass: %s"), 
    //    CustomizationWidgetClass ? *CustomizationWidgetClass->GetName() : TEXT("NULL"));
    
    if (CustomizationWidgetClass)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎨 UPUDishCustomizationComponent::StartCustomization - Creating widget with class: %s"), *CustomizationWidgetClass->GetName());
        
        UWorld* World = GetWorld();
        if (!World)
        {
            //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - No World available for widget creation"));
            return;
        }
        
        //UE_LOG(LogTemp,Display, TEXT("🎨 UPUDishCustomizationComponent::StartCustomization - World valid: %s"), *World->GetName());
        
        CustomizationWidget = CreateWidget<UUserWidget>(World, CustomizationWidgetClass);
        if (CustomizationWidget)
        {
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Widget created successfully: %s"), *CustomizationWidget->GetName());
            
            // Try to cast to our custom widget class and set up the connection
            if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
            {
                //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Widget is PUDishCustomizationWidget, connecting to component"));
                DishWidget->SetCustomizationComponent(this);
                
                //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Component connection completed"));
            }
            else
            {
                //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - Widget is not a PUDishCustomizationWidget, manual connection may be needed"));
                //UE_LOG(LogTemp,Display, TEXT("🎨 UPUDishCustomizationComponent::StartCustomization - Widget class: %s"), *CustomizationWidget->GetClass()->GetName());
            }
            
            // Pass the initial dish data to the widget during creation
            // The widget can access this data in its construction script or BeginPlay
            //UE_LOG(LogTemp,Display, TEXT("🎨 UPUDishCustomizationComponent::StartCustomization - About to add widget to viewport"));
            
            // Add to viewport with a lower Z-Order so it doesn't override the recipe book widget
            CustomizationWidget->AddToViewport(250);
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Widget added to viewport successfully with Z-Order -100"));
            
            // Re-hide HUD after AddToViewport - viewport updates can cause HUD to reappear (e.g. Slate invalidation, widget tree rebuild)
            SetHUDVisible(false);
            
            // Defer HUD hide to next frame - catches HUD created lazily or shown by Blueprint/animations after our frame
            if (UWorld* WorldForTimer = GetWorld())
            {
                TWeakObjectPtr<UPUDishCustomizationComponent> WeakThis(this);
                WorldForTimer->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakThis]()
                {
                    UPUDishCustomizationComponent* Comp = WeakThis.Get();
                    if (Comp && Comp->IsCustomizing())
                    {
                        Comp->SetHUDVisible(false);
                    }
                }));
            }
            
            // Check if widget is visible
            if (CustomizationWidget->IsVisible())
            {
                //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::StartCustomization - Widget is visible"));
            }
            else
            {
                //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - Widget is NOT visible"));
            }
            
            // Broadcast the initial dish data now that the widget is created and subscribed
            if (CurrentDishData.DishTag.IsValid())
            {
                //UE_LOG(LogTemp,Display, TEXT("📡 UPUDishCustomizationComponent::StartCustomization - Broadcasting initial dish data to newly created widget"));
                //UE_LOG(LogTemp,Display, TEXT("📡 UPUDishCustomizationComponent::StartCustomization - Dish data: %s"), *CurrentDishData.DisplayName.ToString());
                BroadcastInitialDishData(CurrentDishData);
            }
            else
            {
                //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartCustomization - No valid dish data to broadcast"));
            }
        }
        else
        {
            //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - Failed to create Customization UI Widget"));
            //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - Widget class: %s"), *CustomizationWidgetClass->GetName());
        }
    }
    else
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::StartCustomization - No CustomizationWidgetClass set"));
    }

    // Start camera transition to customization view
    //UE_LOG(LogTemp,Display, TEXT("🎬 UPUDishCustomizationComponent::StartCustomization - Starting camera transition"));
    StartCameraTransition(true);
    
    //UE_LOG(LogTemp,Display, TEXT("🎉 UPUDishCustomizationComponent::StartCustomization - CUSTOMIZATION STARTED SUCCESSFULLY"));
}

void UPUDishCustomizationComponent::EndCustomization()
{
    UWorld* World = GetWorld();
    APlayerController* WorldPC = World ? World->GetFirstPlayerController() : nullptr;

    if (bPU_LogMovementRestore)
        UE_LOG(LogTemp, Warning, TEXT("[MovementRestore] EndCustomization ENTRY - Component=%s CurrentCharacter=%s WorldPC=%s"), *GetName(), CurrentCharacter ? *CurrentCharacter->GetName() : TEXT("NULL"), WorldPC ? *WorldPC->GetName() : TEXT("NULL"));

    if (!CurrentCharacter)
    {
        if (bPU_LogMovementRestore)
            UE_LOG(LogTemp, Warning, TEXT("[MovementRestore] EndCustomization EARLY RETURN (no CurrentCharacter) - restoring via WorldPC"));
        // Still restore movement so player can move (e.g. component ref mismatch or already cleared)
        if (WorldPC)
        {
            LogMovementState(WorldPC, TEXT("EARLY before restore"));
            WorldPC->ResetIgnoreMoveInput();
            WorldPC->ResetIgnoreLookInput();
            UWidgetBlueprintLibrary::SetFocusToGameViewport();
            if (APawn* Pawn = WorldPC->GetPawn())
            {
                if (ACharacter* PlayerCharacter = Cast<ACharacter>(Pawn))
                {
                    if (UCharacterMovementComponent* MovementComp = PlayerCharacter->GetCharacterMovement())
                    {
                        MovementComp->SetMovementMode(MOVE_Walking);
                    }
                }
            }
            LogMovementState(WorldPC, TEXT("EARLY after restore"));
        }
        return;
    }

    // Restore HUD visibility when exiting customization
    SetHUDVisible(true);

    // End plating stage if we're in plating mode
    if (bPlatingMode)
    {
        EndPlatingStage();
    }

    bWasMouseDown = false;  // Reset for clean state when exiting

    // Get the player controller
    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (PlayerController)
    {
        // Re-enable physical mouse input
        PlayerController->bEnableMouseOverEvents = true;
        PlayerController->bEnableClickEvents = true;
        PlayerController->CurrentMouseCursor = EMouseCursor::Default;

#if WITH_EDITOR
        // Unregister Slate pre-input listener (must remove before component becomes invalid)
        if (PreInputMouseDownHandle.IsValid())
        {
            FSlateApplication::Get().OnApplicationMousePreInputButtonDownListener().Remove(PreInputMouseDownHandle);
            PreInputMouseDownHandle.Reset();
        }
#else
        PreInputMouseDownHandle.Reset();
#endif // WITH_EDITOR

        // Unbind the controller mouse action first (use Cast so we still restore move/look if this fails)
        if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
        {
            if (ControllerMouseAction)
            {
                EnhancedInputComponent->RemoveBindingByHandle(ControllerMouseBindingHandle);
                //UE_LOG(LogTemp,Log, TEXT("Unbound controller mouse action"));
            }
            if (MouseClickAction)
            {
                EnhancedInputComponent->RemoveBindingByHandle(MouseClickBindingHandle);
            }
            if (ExitCustomizationAction)
            {
                EnhancedInputComponent->RemoveBindingByHandle(ExitActionBindingHandle);
                //UE_LOG(LogTemp,Log, TEXT("Unbound exit action"));
            }

            if (NextStageAction)
            {
                EnhancedInputComponent->RemoveBindingByHandle(NextStageBindingHandle);
            }

            if (PreviousStageAction)
            {
                EnhancedInputComponent->RemoveBindingByHandle(PreviousStageBindingHandle);
            }
        }

        // Get the enhanced input subsystem
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            // Remove the customization mapping context
            if (CustomizationMappingContext)
            {
                Subsystem->RemoveMappingContext(CustomizationMappingContext);
                //UE_LOG(LogTemp,Log, TEXT("Removed customization mapping context"));
            }

            // Restore the original mapping context (contains Move, Look, etc.)
            UInputMappingContext* ContextToRestore = OriginalMappingContext;
            if (!ContextToRestore && CurrentCharacter)
            {
                ContextToRestore = CurrentCharacter->GetDefaultMappingContext();
                if (bPU_LogMovementRestore && ContextToRestore)
                    UE_LOG(LogTemp, Warning, TEXT("[MovementRestore] OriginalMappingContext was NULL - using Character->GetDefaultMappingContext()"));
            }
            if (ContextToRestore)
            {
                Subsystem->AddMappingContext(ContextToRestore, 0);
                //UE_LOG(LogTemp,Log, TEXT("Restored original mapping context"));
            }
            else if (bPU_LogMovementRestore)
            {
                UE_LOG(LogTemp, Warning, TEXT("[MovementRestore] WARNING: No mapping context to restore! Move/Look input may not work."));
            }
        }

        // Re-enable movement and look
        if (bPU_LogMovementRestore)
            LogMovementState(PlayerController, TEXT("MAIN before restore"));
        PlayerController->ResetIgnoreMoveInput();
        PlayerController->ResetIgnoreLookInput();
        PlayerController->bShowMouseCursor = true;

        // Restore character movement (dialogue box or other systems may have called DisableMovement)
        if (APawn* Pawn = PlayerController->GetPawn())
        {
            if (ACharacter* PlayerCharacter = Cast<ACharacter>(Pawn))
            {
                if (UCharacterMovementComponent* MovementComp = PlayerCharacter->GetCharacterMovement())
                {
                    MovementComp->SetMovementMode(MOVE_Walking);
                }
            }
        }
        if (bPU_LogMovementRestore)
        {
            LogMovementState(PlayerController, TEXT("MAIN after restore"));
            UE_LOG(LogTemp, Warning, TEXT("[MovementRestore] OriginalMappingContext=%s"), OriginalMappingContext ? *OriginalMappingContext->GetName() : TEXT("NULL"));
        }

        // Set input mode back to game and UI (mouse visible, no specific widget focus)
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(nullptr);
        InputMode.SetHideCursorDuringCapture(false);
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PlayerController->SetInputMode(InputMode);

        // Return focus to the game viewport so controller/gamepad works again (was stuck after closing customization UI)
        UWidgetBlueprintLibrary::SetFocusToGameViewport();
    }

    // Clean up the customization widget
    if (CustomizationWidget)
    {
        CustomizationWidget->RemoveFromParent();
        CustomizationWidget = nullptr;
        //UE_LOG(LogTemp,Log, TEXT("Customization UI Widget Removed"));
    }

    // Clean up the cooking stage widget
    if (CookingStageWidget)
    {
        CookingStageWidget->RemoveFromParent();
        CookingStageWidget = nullptr;
        //UE_LOG(LogTemp,Log, TEXT("Cooking Stage Widget Removed"));
    }

    // Switch back to character camera
    SwitchToCharacterCamera();
    
    // Start camera transition back to original view
    StartCameraTransition(false);

    // Store character for the outgoing camera transition, then clear CurrentCharacter so IsCustomizing()
    // returns false immediately. Otherwise other systems (e.g. GameInstance OnPopupWidgetClosed when a
    // popup closes) see IsCustomizing() true and re-apply ignore move/look, taking control away again.
    // UpdateCameraTransition uses CameraTransitionCharacter when CurrentCharacter is null.
    CameraTransitionCharacter = CurrentCharacter;
    CurrentCharacter = nullptr;

    // Always re-enable movement on the world's player controller so keyboard/joystick move works no matter what.
    if (WorldPC)
    {
        if (bPU_LogMovementRestore)
            LogMovementState(WorldPC, TEXT("WorldPC before restore"));
        WorldPC->ResetIgnoreMoveInput();
        WorldPC->ResetIgnoreLookInput();
        UWidgetBlueprintLibrary::SetFocusToGameViewport();

        // Restore character movement (belt-and-suspenders in case PlayerController path was skipped)
        if (APawn* Pawn = WorldPC->GetPawn())
        {
            if (ACharacter* PlayerCharacter = Cast<ACharacter>(Pawn))
            {
                if (UCharacterMovementComponent* MovementComp = PlayerCharacter->GetCharacterMovement())
                {
                    MovementComp->SetMovementMode(MOVE_Walking);
                }
            }
        }
        if (bPU_LogMovementRestore)
            LogMovementState(WorldPC, TEXT("WorldPC after restore"));
    }
}

void UPUDishCustomizationComponent::StartCameraTransition(bool bToCustomization)
{
    if (!CurrentCharacter)
    {
        return;
    }

    // Store current camera settings
    USpringArmComponent* CameraBoom = CurrentCharacter->GetCameraBoom();
    UCameraComponent* FollowCamera = CurrentCharacter->GetFollowCamera();
    if (!CameraBoom || !FollowCamera)
    {
        return;
    }

    bTransitioningToCustomization = bToCustomization;

    // Store original values if transitioning to customization
    if (bToCustomization)
    {
        OriginalCameraDistance = CameraBoom->TargetArmLength;
        OriginalCameraPitch = CameraBoom->GetRelativeRotation().Pitch;
        OriginalCameraYaw = CameraBoom->GetRelativeRotation().Yaw;
        OriginalOrthoWidth = FollowCamera->OrthoWidth;
        OriginalCameraOffset = CurrentCharacter->GetCameraOffset();
        OriginalCameraPositionIndex = CurrentCharacter->GetCameraPositionIndex();

        // Set target values for customization view
        TargetCameraDistance = CustomizationCameraDistance;
        TargetCameraPitch = CustomizationCameraPitch;
        TargetCameraYaw = OriginalCameraYaw; // Keep the same yaw
        TargetOrthoWidth = CustomizationOrthoWidth;
        TargetCameraOffset = OriginalCameraOffset; // Keep the same offset
        TargetCameraPositionIndex = OriginalCameraPositionIndex; // Keep the same position index
    }
    else
    {
        // Set target values back to original
        TargetCameraDistance = OriginalCameraDistance;
        TargetCameraPitch = OriginalCameraPitch;
        TargetCameraYaw = OriginalCameraYaw;
        TargetOrthoWidth = OriginalOrthoWidth;
        TargetCameraOffset = OriginalCameraOffset;
        TargetCameraPositionIndex = OriginalCameraPositionIndex;
    }

    bIsTransitioningCamera = true;
}

void UPUDishCustomizationComponent::SwitchToCookingCamera()
{
    if (!CurrentCharacter || !CurrentCharacter->GetWorld())
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwitchToCookingCamera - No character or world available"));
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToCookingCamera - Switching to cooking camera"));

    // Get the player controller
    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (!PlayerController)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwitchToCookingCamera - No player controller found"));
        return;
    }

    // Find the cooking station camera component
    if (!CookingStationCamera)
    {
        AActor* OwnerActor = GetOwner();
        if (OwnerActor)
        {
            CookingStationCamera = Cast<UCameraComponent>(OwnerActor->GetComponentByClass(UCameraComponent::StaticClass()));
            
            if (!CookingStationCamera)
            {
                // Try to find by name if the default class search didn't work
                TArray<UCameraComponent*> CameraComponents;
                OwnerActor->GetComponents<UCameraComponent>(CameraComponents);
                
                for (UCameraComponent* CameraComp : CameraComponents)
                {
                    if (CameraComp && CameraComp->GetName() == CookingStationCameraComponentName.ToString())
                    {
                        CookingStationCamera = CameraComp;
                        break;
                    }
                }
            }
            
            if (CookingStationCamera)
            {
                // CRITICAL: Set the CookingCamera as the active camera component on the actor FIRST
                AActor* StationActor = CookingStationCamera->GetOwner();
                if (StationActor)
                {
                    // Disable the plating camera first
                    TArray<UCameraComponent*> AllCameras;
                    StationActor->GetComponents<UCameraComponent>(AllCameras);
                    
                    for (UCameraComponent* Camera : AllCameras)
                    {
                        if (Camera && Camera->GetName() == TEXT("PlatingCamera"))
                        {
                            Camera->SetActive(false);
                            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToCookingCamera - Disabled PlatingCamera"));
                        }
                    }
                    
                    // Enable the cooking camera
                    CookingStationCamera->SetActive(true);
                    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToCookingCamera - Activated CookingCamera"));
                }

                // Configure the camera for orthographic projection
                CookingStationCamera->SetProjectionMode(ECameraProjectionMode::Orthographic);
                CookingStationCamera->OrthoWidth = CookingOrthoWidth;
                
                // Position the camera properly for smooth transition
                FVector CameraLocation = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 200.0f) + CookingCameraPositionOffset;
                FRotator CameraRotation = FRotator(CookingCameraPitch, CookingCameraYaw, 0.0f);
                
                CookingStationCamera->SetWorldLocation(CameraLocation);
                CookingStationCamera->SetWorldRotation(CameraRotation);
                
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToCookingCamera - Found and configured cooking camera component, width: %.2f, position: %s"), 
                //    CookingOrthoWidth, *CameraLocation.ToString());
            }
            else
            {
                //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::SwitchToCookingCamera - No camera component found on cooking station"));
                return;
            }
        }
        else
        {
            //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::SwitchToCookingCamera - No owner actor found"));
            return;
        }
    }

    // Switch to the cooking camera
    PlayerController->SetViewTargetWithBlend(CookingStationCamera->GetOwner(), 0.5f);
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToCookingCamera - Switched to cooking camera"));
}

void UPUDishCustomizationComponent::SetCookingCameraPositionOffset(const FVector& NewOffset)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SetCookingCameraPositionOffset - Setting camera offset to: %s"), 
    //    *NewOffset.ToString());
    
    CookingCameraPositionOffset = NewOffset;
    
    // If the cooking camera component is found, update its position
    if (CookingStationCamera)
    {
        FVector CameraLocation = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 200.0f) + NewOffset;
        FRotator CameraRotation = FRotator(CookingCameraPitch, CookingCameraYaw, 0.0f);
        
        CookingStationCamera->SetWorldLocation(CameraLocation);
        CookingStationCamera->SetWorldRotation(CameraRotation);
        
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SetCookingCameraPositionOffset - Updated existing camera position to: %s"), 
        //    *CameraLocation.ToString());
    }
}

void UPUDishCustomizationComponent::SwitchToCharacterCamera()
{
    if (!CurrentCharacter)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwitchToCharacterCamera - No character available"));
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToCharacterCamera - Switching to character camera"));

    // Get the player controller
    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (!PlayerController)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwitchToCharacterCamera - No player controller found"));
        return;
    }

    // Switch back to the character camera
    PlayerController->SetViewTargetWithBlend(CurrentCharacter, 0.5f);
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToCharacterCamera - Switched to character camera"));

    // Reset the cooking camera component reference
    CookingStationCamera = nullptr;
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToCharacterCamera - Reset cooking camera reference"));
}

void UPUDishCustomizationComponent::UpdateCameraTransition(float DeltaTime)
{
    AProjectUmeowmiCharacter* Char = CurrentCharacter ? CurrentCharacter : CameraTransitionCharacter.Get();
    if (!Char)
    {
        return;
    }

    USpringArmComponent* CameraBoom = Char->GetCameraBoom();
    UCameraComponent* FollowCamera = Char->GetFollowCamera();
    if (!CameraBoom || !FollowCamera)
    {
        return;
    }

    // Get current values
    float CurrentDistance = CameraBoom->TargetArmLength;
    float CurrentPitch = CameraBoom->GetRelativeRotation().Pitch;
    float CurrentYaw = CameraBoom->GetRelativeRotation().Yaw;
    float CurrentOrthoWidth = FollowCamera->OrthoWidth;
    float CurrentCameraOffset = Char->GetCameraOffset();
    int32 CurrentCameraPositionIndex = Char->GetCameraPositionIndex();

    // Interpolate values
    float NewDistance = FMath::FInterpTo(CurrentDistance, TargetCameraDistance, DeltaTime, CameraTransitionSpeed);
    float NewPitch = FMath::FInterpTo(CurrentPitch, TargetCameraPitch, DeltaTime, CameraTransitionSpeed);
    float NewYaw = FMath::FInterpTo(CurrentYaw, TargetCameraYaw, DeltaTime, CameraTransitionSpeed);
    float NewOrthoWidth = FMath::FInterpTo(CurrentOrthoWidth, TargetOrthoWidth, DeltaTime, CameraTransitionSpeed);
    float NewCameraOffset = FMath::FInterpTo(CurrentCameraOffset, TargetCameraOffset, DeltaTime, CameraTransitionSpeed);

    // Apply new values
    CameraBoom->TargetArmLength = NewDistance;
    CameraBoom->SetRelativeRotation(FRotator(NewPitch, NewYaw, 0.0f));
    FollowCamera->OrthoWidth = NewOrthoWidth;
    Char->SetCameraOffset(NewCameraOffset);
    Char->SetCameraPositionIndex(TargetCameraPositionIndex);

    // Check if we've reached the target
    if (FMath::IsNearlyEqual(NewDistance, TargetCameraDistance, 1.0f) &&
        FMath::IsNearlyEqual(NewPitch, TargetCameraPitch, 1.0f) &&
        FMath::IsNearlyEqual(NewYaw, TargetCameraYaw, 1.0f) &&
        FMath::IsNearlyEqual(NewOrthoWidth, TargetOrthoWidth, 1.0f) &&
        FMath::IsNearlyEqual(NewCameraOffset, TargetCameraOffset, 1.0f))
    {
        bIsTransitioningCamera = false;

        // Only run exit logic when we're transitioning OUT of customization (not when entering or going to cooking).
        // bTransitioningToCustomization: false = transitioning out, true = transitioning in or to cooking stage.
        // TargetOrthoWidth==OriginalOrthoWidth: ensures we're actually returning to original camera (not cooking target).
        const bool bIsExiting = !bTransitioningToCustomization && (TargetOrthoWidth == OriginalOrthoWidth);

        // If we're exiting customization (returning to original camera settings), re-enable collision detection
        if (bIsExiting && Char)
        {
            if (CameraBoom)
            {
                CameraBoom->bDoCollisionTest = true;
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::UpdateCameraTransition - Re-enabled spring arm collision detection"));
            }
        }

        // If we're exiting customization, clear the character reference and broadcast the end event
        if (bIsExiting)
        {
            // Transforms were already captured in EndPlatingStage; clear meshes now
            ClearAll3DIngredientMeshes();
            
            // Restore the original dish container mesh
            RestoreOriginalDishContainerMesh();
            
            // Ensure HUD is visible again when customization fully ends
            SetHUDVisible(true);

            CameraTransitionCharacter = nullptr;
            CurrentCharacter = nullptr;

            UWorld* World = GetWorld();
            if (UPUProjectUmeowmiGameInstance* GI = World ? World->GetGameInstance<UPUProjectUmeowmiGameInstance>() : nullptr)
            {
                GI->ClearCurrentDishTag();
            }
            OnCustomizationEnded.Broadcast();

            // Force restore move/look, input mode, and focus when transition fully ends (belt-and-suspenders so player can always move/interact)
            if (World)
            {
                if (APlayerController* PC = World->GetFirstPlayerController())
                {
                    if (bPU_LogMovementRestore)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("[MovementRestore] UpdateCameraTransition bIsExiting - restoring move/look"));
                        LogMovementState(PC, TEXT("CAMERA_TRANSITION before"));
                    }
                    PC->ResetIgnoreMoveInput();
                    PC->ResetIgnoreLookInput();
                    PC->bShowMouseCursor = true;
                    if (APawn* Pawn = PC->GetPawn())
                    {
                        if (ACharacter* PlayerCharacter = Cast<ACharacter>(Pawn))
                        {
                            if (UCharacterMovementComponent* MovementComp = PlayerCharacter->GetCharacterMovement())
                            {
                                MovementComp->SetMovementMode(MOVE_Walking);
                            }
                        }
                    }
                    if (bPU_LogMovementRestore)
                        LogMovementState(PC, TEXT("CAMERA_TRANSITION after"));
                    FInputModeGameAndUI InputMode;
                    InputMode.SetWidgetToFocus(nullptr);
                    InputMode.SetHideCursorDuringCapture(false);
                    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                    PC->SetInputMode(InputMode);
                    UWidgetBlueprintLibrary::SetFocusToGameViewport();
                }
            }
        }
    }
}

void UPUDishCustomizationComponent::HandleExitInput()
{
    //UE_LOG(LogTemp,Log, TEXT("Exit input received"));
    EndCustomization();
}

void UPUDishCustomizationComponent::HandleControllerMouse(const FInputActionValue& Value)
{
    //UE_LOG(LogTemp,Log, TEXT("HandleControllerMouse called"));
    
    if (!CurrentCharacter)
    {
        //UE_LOG(LogTemp,Warning, TEXT("HandleControllerMouse - No current character"));
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (!PlayerController)
    {
        //UE_LOG(LogTemp,Warning, TEXT("HandleControllerMouse - No player controller"));
        return;
    }

    // Get the input value (should be a Vector2D for the right stick)
    FVector2D StickInput = Value.Get<FVector2D>();
    //UE_LOG(LogTemp,Log, TEXT("HandleControllerMouse - Raw stick input: X=%.2f, Y=%.2f"), StickInput.X, StickInput.Y);
    
    // Apply deadzone
    if (FMath::Abs(StickInput.X) < ControllerMouseDeadzone)
    {
        StickInput.X = 0.0f;
    }
    if (FMath::Abs(StickInput.Y) < ControllerMouseDeadzone)
    {
        StickInput.Y = 0.0f;
    }
    //UE_LOG(LogTemp,Log, TEXT("HandleControllerMouse - After deadzone: X=%.2f, Y=%.2f"), StickInput.X, StickInput.Y);

    // Get viewport size
    int32 ViewportSizeX, ViewportSizeY;
    PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
    //UE_LOG(LogTemp,Log, TEXT("HandleControllerMouse - Viewport size: %dx%d"), ViewportSizeX, ViewportSizeY);

    // Get current mouse position
    float MouseX, MouseY;
    PlayerController->GetMousePosition(MouseX, MouseY);
    //UE_LOG(LogTemp,Log, TEXT("HandleControllerMouse - Current mouse position: X=%.2f, Y=%.2f"), MouseX, MouseY);

    // Calculate movement based on stick input and sensitivity
    float DeltaX = StickInput.X * ControllerMouseSensitivity;
    float DeltaY = StickInput.Y * ControllerMouseSensitivity;
    //UE_LOG(LogTemp,Log, TEXT("HandleControllerMouse - Calculated delta: X=%.2f, Y=%.2f"), DeltaX, DeltaY);

    // Calculate new position
    float NewMouseX = MouseX + DeltaX;
    float NewMouseY = MouseY + DeltaY;

    // Clamp to viewport bounds
    NewMouseX = FMath::Clamp(NewMouseX, 0.0f, static_cast<float>(ViewportSizeX));
    NewMouseY = FMath::Clamp(NewMouseY, 0.0f, static_cast<float>(ViewportSizeY));
    //UE_LOG(LogTemp,Log, TEXT("HandleControllerMouse - New mouse position: X=%.2f, Y=%.2f"), NewMouseX, NewMouseY);

    // Set new mouse position
    int32 NewX = static_cast<int32>(NewMouseX);
    int32 NewY = static_cast<int32>(NewMouseY);
    PlayerController->SetMouseLocation(NewX, NewY);
    
    // Verify the position was set
    float VerifyX, VerifyY;
    PlayerController->GetMousePosition(VerifyX, VerifyY);
    //UE_LOG(LogTemp,Log, TEXT("HandleControllerMouse - Verified mouse position: X=%.2f, Y=%.2f"), VerifyX, VerifyY);
}

void UPUDishCustomizationComponent::OnPreInputMouseButtonDown(const FPointerEvent& MouseEvent)
{
    // Fires BEFORE Slate widgets consume the click - bypasses widget blocking
    if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !CanSpawnIngredientsIn3D() || !CurrentCharacter)
    {
        return;
    }
    HandleMouseClick(FInputActionValue());
}

void UPUDishCustomizationComponent::HandleMouseClick(const FInputActionValue& Value)
{
    if (bPU_LogIngredientDrag)
    {
        UE_LOG(LogTemp, Log, TEXT("[DRAG] HandleMouseClick called"));
    }

    if (!CurrentCharacter || bIsDragging)
    {
        if (bPU_LogIngredientDrag) UE_LOG(LogTemp, Log, TEXT("[DRAG] HandleMouseClick early out: Character=%s Dragging=%s"), CurrentCharacter ? TEXT("ok") : TEXT("null"), bIsDragging ? TEXT("yes") : TEXT("no"));
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (!PlayerController)
    {
        if (bPU_LogIngredientDrag) UE_LOG(LogTemp, Warning, TEXT("[DRAG] HandleMouseClick - No player controller"));
        return;
    }

    // Get mouse position
    float MouseX, MouseY;
    PlayerController->GetMousePosition(MouseX, MouseY);
    DragStartMousePosition = FVector(MouseX, MouseY, 0);

    // Use GetHitResultUnderCursor - same method the engine uses for mouse clicks.
    // Our custom trace was returning 0 hits (possibly due to ignore list or camera mismatch)
    // while NotifyActorOnClicked was firing, so we use the engine's hit detection for consistency.
    FHitResult HitResult;
    if (!PlayerController->GetHitResultUnderCursor(ECC_Visibility, true, HitResult))
    {
        if (bPU_LogIngredientDrag) UE_LOG(LogTemp, Warning, TEXT("[DRAG] HandleMouseClick - GetHitResultUnderCursor failed (screen %.0f,%.0f)"), MouseX, MouseY);
        return;
    }

    if (bPU_LogIngredientDrag)
    {
        UE_LOG(LogTemp, Log, TEXT("[DRAG] HandleMouseClick - Hit: %s (%s) at (%.0f,%.0f)"), HitResult.GetActor() ? *HitResult.GetActor()->GetName() : TEXT("null"), HitResult.GetComponent() ? *HitResult.GetComponent()->GetName() : TEXT("null"), MouseX, MouseY);
    }

    APUIngredientMesh* HitIngredient = Cast<APUIngredientMesh>(HitResult.GetActor());
    FHitResult IngredientHitResult = HitResult;

    if (HitIngredient)
    {
        if (bPU_LogIngredientDrag)
        {
            UE_LOG(LogTemp, Log, TEXT("[DRAG] HandleMouseClick - Started dragging ingredient: %s at (%.1f,%.1f,%.1f)"), *HitIngredient->GetName(), HitIngredient->GetActorLocation().X, HitIngredient->GetActorLocation().Y, HitIngredient->GetActorLocation().Z);
        }

        // Test mouse interaction for this ingredient
        HitIngredient->TestMouseInteraction();

        bIsDragging = true;
        CurrentlyDraggedIngredient = HitIngredient;
        DragStartPosition = HitIngredient->GetActorLocation();

        // Calculate offset between mouse and ingredient
        FVector MouseWorldPosition = IngredientHitResult.Location;
        DragOffset = DragStartPosition - MouseWorldPosition;

        // Call the ingredient's grab function
        HitIngredient->OnMouseGrab();
    }
    else
    {
        if (bPU_LogIngredientDrag && HitResult.GetActor())
        {
            UE_LOG(LogTemp, Warning, TEXT("[DRAG] HandleMouseClick - Hit %s but not an ingredient mesh"), *HitResult.GetActor()->GetName());
        }
    }
}

void UPUDishCustomizationComponent::HandleNextStage()
{
    if (CustomizationWidget)
    {
        if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
        {
            // Call the Blueprint event so animations can play first
            DishWidget->OnControllerNextStage();
        }
    }
}

void UPUDishCustomizationComponent::HandlePreviousStage()
{
    UE_LOG(LogTemp, Warning, TEXT("🔙 HandlePreviousStage called - CustomizationWidget: %s"), 
        CustomizationWidget ? *CustomizationWidget->GetName() : TEXT("NULL"));
    
    if (CustomizationWidget)
    {
        if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
        {
            UE_LOG(LogTemp, Warning, TEXT("🔙 Calling OnControllerPreviousStage on widget: %s"), *DishWidget->GetName());
            // Call the Blueprint event so animations can play first
            DishWidget->OnControllerPreviousStage();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("🔙 Failed to cast CustomizationWidget to UPUDishCustomizationWidget"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("🔙 CustomizationWidget is NULL!"));
    }
}

void UPUDishCustomizationComponent::HandleMouseRelease(const FInputActionValue& Value)
{
    //UE_LOG(LogTemp,Display, TEXT("🔍 [DRAG] Mouse release event received - Value: %s"), *Value.ToString());
    
    if (bIsDragging && CurrentlyDraggedIngredient)
    {
        FString IngredientName = IsValid(CurrentlyDraggedIngredient) ? CurrentlyDraggedIngredient->GetName() : TEXT("INVALID");
        FVector FinalPosition = IsValid(CurrentlyDraggedIngredient) ? CurrentlyDraggedIngredient->GetActorLocation() : FVector::ZeroVector;
        
        //UE_LOG(LogTemp,Display, TEXT("🖱️ [DRAG] Stopping drag for %s at final position (%.2f,%.2f,%.2f)"), 
        //    *IngredientName, FinalPosition.X, FinalPosition.Y, FinalPosition.Z);
        
        // Call the ingredient's release function
        if (IsValid(CurrentlyDraggedIngredient))
        {
            CurrentlyDraggedIngredient->OnMouseRelease();
            
            FVector PositionAfterRelease = CurrentlyDraggedIngredient->GetActorLocation();
            //UE_LOG(LogTemp,Display, TEXT("🖱️ [DRAG] After OnMouseRelease - %s at position (%.2f,%.2f,%.2f)"), 
            //    *CurrentlyDraggedIngredient->GetName(), 
            //    PositionAfterRelease.X, PositionAfterRelease.Y, PositionAfterRelease.Z);
        }
        
        bIsDragging = false;
        CurrentlyDraggedIngredient = nullptr;
    }
}

void UPUDishCustomizationComponent::StartDraggingIngredient(APUIngredientMesh* Ingredient)
{
    if (bPU_LogIngredientDrag)
    {
        UE_LOG(LogTemp, Log, TEXT("[DRAG] StartDraggingIngredient called for %s"), Ingredient ? *Ingredient->GetName() : TEXT("null"));
    }

    if (!Ingredient)
    {
        return;
    }
    
    if (!IsValid(Ingredient))
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ [DRAG] StartDraggingIngredient - Ingredient is not valid"));
        return;
    }
    
    if (!CurrentCharacter)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ [DRAG] StartDraggingIngredient - No character"));
        return;
    }
    
    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (!PlayerController)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ [DRAG] StartDraggingIngredient - No player controller"));
        return;
    }
    
    // Get current ingredient position before any changes
    FVector IngredientStartPos = Ingredient->GetActorLocation();
    //UE_LOG(LogTemp,Display, TEXT("🖱️ [DRAG] StartDraggingIngredient - %s at position (%.2f,%.2f,%.2f)"), 
    //    *Ingredient->GetName(), IngredientStartPos.X, IngredientStartPos.Y, IngredientStartPos.Z);
    
    // Get mouse position
    float MouseX, MouseY;
    PlayerController->GetMousePosition(MouseX, MouseY);
    
    // Convert screen position to world space and compute grab offset.
    // Use view-perpendicular plane through ingredient (works with any camera angle).
    FVector WorldLocation;
    FVector WorldDirection;
    if (PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection))
    {
        FVector PlanePoint(IngredientStartPos.X, IngredientStartPos.Y, IngredientStartPos.Z);
        float DirLenSq = WorldDirection.SizeSquared();
        if (DirLenSq > SMALL_NUMBER)
        {
            float T = FVector::DotProduct(PlanePoint - WorldLocation, WorldDirection) / DirLenSq;
            if (T > 0.0f && T < 10000.0f)
            {
                FVector MouseWorldPosition = WorldLocation + (WorldDirection * T);
                DragOffset = IngredientStartPos - MouseWorldPosition;
            }
            else
            {
                DragOffset = FVector::ZeroVector;
            }
        }
        else
        {
            DragOffset = FVector::ZeroVector;
        }
    }
    else
    {
        DragOffset = FVector::ZeroVector;
    }
    
    // Call the ingredient's grab function first to set up its state
    Ingredient->OnMouseGrab();
    
    // Verify ingredient is still valid after grab
    if (!IsValid(Ingredient))
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ [DRAG] StartDraggingIngredient - Ingredient became invalid after OnMouseGrab!"));
        return;
    }
    
    FVector IngredientAfterGrabPos = Ingredient->GetActorLocation();
    //UE_LOG(LogTemp,Display, TEXT("🖱️ [DRAG] After OnMouseGrab - %s at position (%.2f,%.2f,%.2f)"), 
    //    *Ingredient->GetName(), IngredientAfterGrabPos.X, IngredientAfterGrabPos.Y, IngredientAfterGrabPos.Z);
    
    bIsDragging = true;
    CurrentlyDraggedIngredient = Ingredient;
    DragStartPosition = IngredientAfterGrabPos;
    DragStartMousePosition = FVector(MouseX, MouseY, 0);
    
    //UE_LOG(LogTemp,Display, TEXT("✅ [DRAG] Started dragging ingredient: %s with offset: (%.2f,%.2f,%.2f)"), 
    //    *Ingredient->GetName(), DragOffset.X, DragOffset.Y, DragOffset.Z);
}

void UPUDishCustomizationComponent::UpdateMouseDrag()
{
    if (!bIsDragging || !CurrentCharacter)
    {
        return;
    }
    
    // Check if the ingredient is still valid (not destroyed)
    if (!IsValid(CurrentlyDraggedIngredient))
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ [DRAG] UpdateMouseDrag - Ingredient is no longer valid, stopping drag"));
        bIsDragging = false;
        CurrentlyDraggedIngredient = nullptr;
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (!PlayerController)
    {
        return;
    }

    // Check if mouse button is still held down
    bool bLeftMouseDown = PlayerController->IsInputKeyDown(EKeys::LeftMouseButton);
    if (!bLeftMouseDown)
    {
        // Mouse button released, stop dragging
        //UE_LOG(LogTemp,Display, TEXT("🖱️ [DRAG] UpdateMouseDrag - Mouse button released, stopping drag for %s"), 
        //    *CurrentlyDraggedIngredient->GetName());
        HandleMouseRelease(FInputActionValue());
        return;
    }

    // Use GetHitResultUnderCursor - same engine logic that works for HandleMouseClick.
    // When dragging, cursor is over the ingredient (or dish); use hit location X,Y + surface height.
    FHitResult HitResult;
    if (!PlayerController->GetHitResultUnderCursor(ECC_Visibility, true, HitResult))
    {
        return;
    }

    float SurfaceHeight = 0.0f;
    if (!GetPlateSurfaceHeight(SurfaceHeight))
    {
        SurfaceHeight = CurrentlyDraggedIngredient->GetActorLocation().Z;
    }

    // Project hit location onto dish surface (use X,Y from hit, Z from surface)
    FVector MouseWorldPosition(HitResult.Location.X, HitResult.Location.Y, SurfaceHeight);
    FVector NewPosition = MouseWorldPosition + DragOffset;

    AActor* OwnerActor = GetOwner();
    FVector RefPoint = OwnerActor ? OwnerActor->GetActorLocation() : FVector::ZeroVector;
    if (!NewPosition.ContainsNaN() && FVector::Dist(NewPosition, RefPoint) < 5000.0f)
    {
        CurrentlyDraggedIngredient->UpdatePosition(NewPosition);
    }
}

void UPUDishCustomizationComponent::UpdateCurrentDishData(const FPUDishBase& NewDishData)
{
    //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::UpdateCurrentDishData - Updating dish data with %d ingredients"), 
    //    NewDishData.IngredientInstances.Num());
    
    CurrentDishData = NewDishData;
    
    // Log the ingredients for debugging
    if (bPU_LogDishDataIngredientTags)
    {
        for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); i++)
        {
            const FIngredientInstance& Instance = CurrentDishData.IngredientInstances[i];
            //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::UpdateCurrentDishData - Ingredient %d: %s (Qty: %d)"),
            //    i, *Instance.IngredientData.IngredientTag.ToString(), Instance.Quantity);
        }
    }
}

void UPUDishCustomizationComponent::SyncDishDataFromUI(const FPUDishBase& DishDataFromUI)
{
    //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::SyncDishDataFromUI - Syncing dish data from UI with %d ingredients"), 
    //    DishDataFromUI.IngredientInstances.Num());
    
    // Update the current dish data with the data from the UI
    UpdateCurrentDishData(DishDataFromUI);
    
    // Broadcast the updated dish data to all subscribers
    OnDishDataUpdated.Broadcast(DishDataFromUI);
    
    //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::SyncDishDataFromUI - Dish data synced and broadcasted successfully"));
}

// This function is no longer needed - we'll use a different approach
void UPUDishCustomizationComponent::SetDishCustomizationComponentOnWidget(UUserWidget* Widget)
{
    // Removed to avoid crashes - using alternative data passing methods
    //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::SetDishCustomizationComponentOnWidget - Function disabled, using alternative approach"));
}

void UPUDishCustomizationComponent::SetWidgetComponentReference(UPUDishCustomizationWidget* Widget)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SetWidgetComponentReference - Setting widget component reference"));
    
    if (Widget)
    {
        // Set the component reference on the widget
        Widget->SetCustomizationComponent(this);
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SetWidgetComponentReference - Widget component reference set successfully"));
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SetWidgetComponentReference - Widget is null"));
    }
}

void UPUDishCustomizationComponent::SetActiveCustomizationWidget(UPUDishCustomizationWidget* ActiveWidget)
{
    if (ActiveWidget)
    {
        // Update the CustomizationWidget reference to track the currently active widget
        // This ensures EndCustomization() can properly clean up the active widget
        CustomizationWidget = ActiveWidget;
        //UE_LOG(LogTemp,Display, TEXT("🔄 UPUDishCustomizationComponent::SetActiveCustomizationWidget - Updated active widget to: %s"), 
        //    *ActiveWidget->GetName());
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SetActiveCustomizationWidget - Active widget is null"));
    }
}

void UPUDishCustomizationComponent::SetInitialDishData(const FPUDishBase& InitialDishData)
{
    //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::SetInitialDishData - Setting initial dish data: %s with %d ingredients"), 
    //    *InitialDishData.DisplayName.ToString(), InitialDishData.IngredientInstances.Num());

    EnsureDishIngredientsInPantry(InitialDishData);
    
    // Set the initial dish data
    UpdateCurrentDishData(InitialDishData);

    // Notify Game Instance so journal recipe section shows this dish when opened during customization
    if (UPUProjectUmeowmiGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance<UPUProjectUmeowmiGameInstance>() : nullptr)
    {
        GI->SetCurrentDishTag(InitialDishData.DishTag);
    }
    
    //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::SetInitialDishData - Initial dish data set successfully"));
}

// New event-driven data passing methods
void UPUDishCustomizationComponent::BroadcastDishDataUpdate(const FPUDishBase& NewDishData)
{
    //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::BroadcastDishDataUpdate - Broadcasting dish data update with %d ingredients"), 
    //    NewDishData.IngredientInstances.Num());
    
    // Update internal data
    UpdateCurrentDishData(NewDishData);
    
    // Broadcast the update to all subscribers
    OnDishDataUpdated.Broadcast(NewDishData);
}

void UPUDishCustomizationComponent::BroadcastInitialDishData(const FPUDishBase& InitialDishData)
{
    //UE_LOG(LogTemp,Display, TEXT("📡 UPUDishCustomizationComponent::BroadcastInitialDishData - Broadcasting initial dish data: %s"), 
    //    *InitialDishData.DisplayName.ToString());

    EnsureDishIngredientsInPantry(InitialDishData);
    
    CurrentDishData = InitialDishData;
    OnInitialDishDataReceived.Broadcast(InitialDishData);
}

void UPUDishCustomizationComponent::EnsureDishIngredientsInPantry(const FPUDishBase& Dish)
{
    UWorld* World = GetWorld();
    UPUProjectUmeowmiGameInstance* GI = World ? World->GetGameInstance<UPUProjectUmeowmiGameInstance>() : nullptr;
    if (!GI) return;

    TArray<FGameplayTag> TagsToUnlock;
    for (const FIngredientInstance& Instance : Dish.IngredientInstances)
    {
        FGameplayTag Tag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
        if (Tag.IsValid() && !GI->IsIngredientUnlocked(Tag))
        {
            TagsToUnlock.AddUnique(Tag);
        }
    }
    if (TagsToUnlock.Num() > 0)
    {
        GI->UnlockIngredients(TagsToUnlock, true); // silent: add to pantry without popup
    }
}

void UPUDishCustomizationComponent::StartPlanningMode()
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::StartPlanningMode - Starting planning mode"));
    
    bInPlanningMode = true;
    
    // Initialize planning data with current dish
    CurrentPlanningData.TargetDish = CurrentDishData;
    CurrentPlanningData.SelectedIngredients.Empty();
    CurrentPlanningData.bPlanningCompleted = false;
    
    // Populate SelectedIngredients from existing dish ingredients
    // Extract unique ingredients from IngredientInstances (planning mode uses ingredients without quantities)
    TMap<FGameplayTag, FPUIngredientBase> UniqueIngredients;
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        // Use convenient field if available, fallback to data field
        FGameplayTag InstanceTag = Instance.IngredientTag.IsValid() ? Instance.IngredientTag : Instance.IngredientData.IngredientTag;
        
        // Only add if we haven't seen this ingredient tag before
        if (InstanceTag.IsValid() && !UniqueIngredients.Contains(InstanceTag))
        {
            // Use the ingredient data from the instance (which already has preparations applied)
            UniqueIngredients.Add(InstanceTag, Instance.IngredientData);
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::StartPlanningMode - Added existing ingredient to SelectedIngredients: %s"), 
            //    *InstanceTag.ToString());
        }
    }
    
    // Add all unique ingredients to SelectedIngredients
    UniqueIngredients.GenerateValueArray(CurrentPlanningData.SelectedIngredients);
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::StartPlanningMode - Populated %d existing ingredients into SelectedIngredients"), 
    //    CurrentPlanningData.SelectedIngredients.Num());
    
    // Notify the widget to start planning mode
    if (CustomizationWidget)
    {
        if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
        {
            DishWidget->StartPlanningMode();
        }
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::StartPlanningMode - Planning mode started for dish: %s"), 
    //    *CurrentPlanningData.TargetDish.DisplayName.ToString());
}

void UPUDishCustomizationComponent::TransitionToCookingStage(const FPUDishBase& DishData)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::TransitionToCookingStage - Transitioning to cooking stage"));
    
    // Store the dish data
    CurrentDishData = DishData;
    
    // Switch to cooking stage camera
    SwitchToCookingCamera();
    
    // Save reference to dish widget BEFORE removing it (needed for ingredient slots)
    UPUDishCustomizationWidget* SavedDishWidget = nullptr;
    if (CustomizationWidget)
    {
        SavedDishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget);
    }
    
    // Remove the current customization widget
    if (CustomizationWidget)
    {
        CustomizationWidget->RemoveFromParent();
        CustomizationWidget = nullptr;
    }
    
    // Create and show the cooking stage widget (now uses PUDishCustomizationWidget or subclass)
    if (CurrentCharacter && CurrentCharacter->GetWorld())
    {
        // Create cooking stage widget (should be a subclass of PUDishCustomizationWidget)
        if (UPUDishCustomizationWidget* CookingWidget = CreateWidget<UPUDishCustomizationWidget>(CurrentCharacter->GetWorld(), CookingStageWidgetClass))
        {
            // Store reference to cooking stage widget
            CookingStageWidget = CookingWidget;
            
            // Add to viewport first
            CookingWidget->AddToViewport(250); // Same Z-order as customization widget
            
            // Get the cooking station location (this component's owner location)
            FVector CookingStationLocation = GetOwner()->GetActorLocation();
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::TransitionToCookingStage - Cooking station location: %s"), 
            //    *CookingStationLocation.ToString());
            
            // Set the dish customization component reference
            CookingWidget->SetCustomizationComponent(this);
            
            // Broadcast initial dish data to the new widget
            BroadcastInitialDishData(DishData);
            
            // Set as active widget so stage navigation (X/B) and input routing work correctly
            SetActiveCustomizationWidget(CookingWidget);
            
            // Note: The new cooking stage widget should handle initialization in its Blueprint
            // or override OnInitialDishDataReceived to set up ingredient slots, etc.
            // The old InitializeCookingStage method is no longer used - initialization happens
            // through the standard dish data flow
            
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::TransitionToCookingStage - Cooking stage widget created and added to viewport"));
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::TransitionToCookingStage - Failed to create cooking stage widget"));
        }
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::TransitionToCookingStage - Planning completed with %d selected ingredients"), 
    //    DishData.IngredientInstances.Num());
}

void UPUDishCustomizationComponent::SetDataTables(UDataTable* DishTable, UDataTable* IngredientTable, UDataTable* PreparationTable)
{
    //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::SetDataTables - Setting data table references"));
    
    IngredientDataTable = IngredientTable;
    PreparationDataTable = PreparationTable;
}

TArray<FPUIngredientBase> UPUDishCustomizationComponent::GetIngredientData() const
{
    TArray<FPUIngredientBase> IngredientData;
    
    if (IngredientDataTable)
    {
        // Get the game instance to check unlocked ingredients
        UWorld* World = GetWorld();
        UPUProjectUmeowmiGameInstance* GameInstance = World ? Cast<UPUProjectUmeowmiGameInstance>(World->GetGameInstance()) : nullptr;
        
        //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::GetIngredientData - Getting ingredient data from table: %s"), *IngredientDataTable->GetName());
        
        TArray<FName> RowNames = IngredientDataTable->GetRowNames();
        for (const FName& RowName : RowNames)
        {
            if (FPUIngredientBase* Ingredient = IngredientDataTable->FindRow<FPUIngredientBase>(RowName, TEXT("GetIngredientData")))
            {
                // Only include ingredients that are unlocked (if game instance is available)
                // If no game instance, include all ingredients (for backwards compatibility or editor use)
                if (GameInstance)
                {
                    if (GameInstance->IsIngredientUnlocked(Ingredient->IngredientTag))
                    {
                        IngredientData.Add(*Ingredient);
                    }
                }
                else
                {
                    // No game instance available, include all ingredients
                    // This can happen in editor or before game instance is initialized
                    IngredientData.Add(*Ingredient);
                }
            }
        }
        
        //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::GetIngredientData - Retrieved %d unlocked ingredients (out of %d total)"), 
        //    IngredientData.Num(), RowNames.Num());
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishCustomizationComponent::GetIngredientData - No ingredient data table available"));
    }
    
    return IngredientData;
}

TArray<FPUPreparationBase> UPUDishCustomizationComponent::GetPreparationData() const
{
    TArray<FPUPreparationBase> PreparationData;
    
    if (PreparationDataTable)
    {
        //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::GetPreparationData - Getting preparation data from table: %s"), *PreparationDataTable->GetName());
        
        TArray<FName> RowNames = PreparationDataTable->GetRowNames();
        for (const FName& RowName : RowNames)
        {
            if (FPUPreparationBase* Preparation = PreparationDataTable->FindRow<FPUPreparationBase>(RowName, TEXT("GetPreparationData")))
            {
                PreparationData.Add(*Preparation);
            }
        }
        
        //UE_LOG(LogTemp,Display, TEXT("UPUDishCustomizationComponent::GetPreparationData - Retrieved %d preparations"), PreparationData.Num());
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("UPUDishCustomizationComponent::GetPreparationData - No preparation data table available"));
    }
    
    return PreparationData;
}

// Plating-specific functions
void UPUDishCustomizationComponent::SpawnIngredientIn3D(const FGameplayTag& IngredientTag, const FVector& WorldPosition)
{
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - START - Ingredient %s at position (%.2f,%.2f,%.2f)"), 
    //    *IngredientTag.ToString(), WorldPosition.X, WorldPosition.Y, WorldPosition.Z);

    if (!CanSpawnIngredientsIn3D())
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SpawnIngredientIn3D - Cannot spawn (not in cooking or plating stage)"));
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - In plating mode, checking %d ingredient instances"), 
    //    CurrentDishData.IngredientInstances.Num());

    // Find the ingredient in the current dish data
    for (int32 i = 0; i < CurrentDishData.IngredientInstances.Num(); ++i)
    {
        const FIngredientInstance& Instance = CurrentDishData.IngredientInstances[i];
        //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - Checking instance %d: %s vs %s"), 
        //    i, *Instance.IngredientData.IngredientTag.ToString(), *IngredientTag.ToString());
        
        if (Instance.IngredientData.IngredientTag == IngredientTag)
        {
            // Check if we can place this ingredient (quantity limits)
            if (!CanPlaceIngredient(Instance.InstanceID))
            {
                //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SpawnIngredientIn3D - Cannot place ingredient %s - quantity limit reached"), 
                //    *IngredientTag.ToString());
                //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - END - Failed (quantity limit)"));
                return;
            }
            
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - Found matching ingredient! Setting plating position"));
            
            // Set the plating position for this ingredient
            CurrentDishData.SetIngredientPlating(Instance.InstanceID, WorldPosition, FRotator::ZeroRotator, FVector::OneVector);
            
            // Track the placement
            PlaceIngredient(Instance.InstanceID);
            
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::SpawnIngredientIn3D - Set plating for instance %d"), Instance.InstanceID);
            
            // Spawn visual 3D mesh
            SpawnVisualIngredientMesh(Instance, WorldPosition);
            
            // Broadcast the updated dish data
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - Broadcasting OnDishDataUpdated"));
            OnDishDataUpdated.Broadcast(CurrentDishData);
            
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - END - Success"));
            return;
        }
    }

    //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SpawnIngredientIn3D - Ingredient %s not found in current dish"), *IngredientTag.ToString());
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3D - END - Failed"));
}

void UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID(int32 InstanceID, const FVector& WorldPosition)
{
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - START - InstanceID %d at position (%.2f,%.2f,%.2f)"), 
    //    InstanceID, WorldPosition.X, WorldPosition.Y, WorldPosition.Z);

    if (!CanSpawnIngredientsIn3D())
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - Cannot spawn (not in cooking or plating stage)"));
        return;
    }

    // Find the specific ingredient instance by InstanceID
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.InstanceID == InstanceID)
        {
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - Found instance %d: %s"), 
            //    InstanceID, *Instance.IngredientData.DisplayName.ToString());
            
            // Check if we can place this ingredient (quantity limits)
            if (!CanPlaceIngredient(InstanceID))
            {
                //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - Cannot place ingredient %s (InstanceID: %d) - quantity limit reached"), 
                //    *Instance.IngredientData.DisplayName.ToString(), InstanceID);
                //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - END - Failed (quantity limit)"));
                return;
            }
            
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - Can place ingredient! Setting plating position"));
            
            // Set the plating position for this ingredient
            CurrentDishData.SetIngredientPlating(InstanceID, WorldPosition, FRotator::ZeroRotator, FVector::OneVector);
            
            // Track the placement
            PlaceIngredient(InstanceID);
            
            // Update the ingredient slot's quantity display (NOT buttons - we use slots in plating mode)
            UpdateIngredientSlotQuantity(InstanceID);
            
            //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - Set plating for instance %d"), InstanceID);
            
            // Spawn visual 3D mesh
            SpawnVisualIngredientMesh(Instance, WorldPosition);
            
            // Broadcast the updated dish data
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - Broadcasting OnDishDataUpdated"));
            OnDishDataUpdated.Broadcast(CurrentDishData);
            
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - END - Success"));
            return;
        }
    }

    //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - InstanceID %d not found in current dish"), InstanceID);
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SpawnIngredientIn3DByInstanceID - END - Failed"));
}

void UPUDishCustomizationComponent::SetPlatingMode(bool bInPlatingMode)
{
    bPlatingMode = bInPlatingMode;
    
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SetPlatingMode - Plating mode set to: %s"), 
    //    bPlatingMode ? TEXT("TRUE") : TEXT("FALSE"));
}

bool UPUDishCustomizationComponent::IsPlatingMode() const
{
    return bPlatingMode;
}

bool UPUDishCustomizationComponent::CanSpawnIngredientsIn3D() const
{
    // Allow spawning in both plating and cooking stages
    if (bPlatingMode)
    {
        return true;
    }
    // Check if we're in cooking stage (active widget has Cooking stage type)
    if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
    {
        return DishWidget->GetStageType() == EDishCustomizationStageType::Cooking;
    }
    return false;
}

FVector UPUDishCustomizationComponent::GetSpawnPositionAboveStation() const
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return FVector::ZeroVector;
    }

    // Use DishContainer mesh component so we spawn directly above the bowl, not the station center (which may be over platform/collision)
    TArray<UStaticMeshComponent*> MeshComponents;
    OwnerActor->GetComponents<UStaticMeshComponent>(MeshComponents);
    for (UStaticMeshComponent* MeshComp : MeshComponents)
    {
        if (MeshComp && MeshComp->GetName().Contains(TEXT("DishContainer"), ESearchCase::IgnoreCase))
        {
            FBoxSphereBounds DishBounds = MeshComp->CalcBounds(MeshComp->GetComponentTransform());
            // Spawn above center of dish container; use dish bounds for X,Y,Z so we're not over platform/rim
            float SurfaceHeight = DishBounds.Origin.Z + DishBounds.BoxExtent.Z;  // Top of bowl
            float SpawnHeight = SurfaceHeight + IngredientSpawnHeightOffset;
            return FVector(DishBounds.Origin.X, DishBounds.Origin.Y, SpawnHeight);
        }
    }

    // Fallback: use station bounds
    FBox StationBoundsBox = OwnerActor->GetComponentsBoundingBox();
    FVector Center = StationBoundsBox.GetCenter();
    FVector Extent = StationBoundsBox.GetExtent();
    float SurfaceHeight = Center.Z + Extent.Z;
    return FVector(Center.X, Center.Y, SurfaceHeight + IngredientSpawnHeightOffset);
}

bool UPUDishCustomizationComponent::GetPlateSurfaceHeight(float& OutSurfaceHeight) const
{
    FVector DummyCenter;
    return GetPlateSurfaceInfo(OutSurfaceHeight, DummyCenter);
}

bool UPUDishCustomizationComponent::GetPlateSurfaceInfo(float& OutSurfaceHeight, FVector& OutSurfaceCenter) const
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return false;
    }

    TArray<UStaticMeshComponent*> MeshComponents;
    OwnerActor->GetComponents<UStaticMeshComponent>(MeshComponents);
    for (UStaticMeshComponent* MeshComp : MeshComponents)
    {
        if (MeshComp && MeshComp->GetName().Contains(TEXT("DishContainer"), ESearchCase::IgnoreCase))
        {
            FBoxSphereBounds DishBounds = MeshComp->CalcBounds(MeshComp->GetComponentTransform());
            OutSurfaceHeight = DishBounds.Origin.Z + DishBounds.BoxExtent.Z;  // Top of bowl/plate
            OutSurfaceCenter = FVector(DishBounds.Origin.X, DishBounds.Origin.Y, OutSurfaceHeight);
            return true;
        }
    }

    return false;
}

void UPUDishCustomizationComponent::TransitionToPlatingStage(const FPUDishBase& DishData)
{
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Transitioning to plating stage"));
    
    // Set plating mode
    SetPlatingMode(true);
    
    // Reset placement tracking for new plating session
    ResetPlatingPlacements();
    
    // Update the current dish data
    CurrentDishData = DishData;

    // Preload cap and material instances for procedural mesh ingredients (same pattern as prepped UI)
    // Ensures they're in memory before spawning - fixes materials not loading on game restart
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        const FPUIngredientBase& Ing = Instance.IngredientData;
        if (Ing.CapMaterialInstance.IsValid() || !Ing.CapMaterialInstance.ToSoftObjectPath().IsNull())
        {
            Ing.CapMaterialInstance.LoadSynchronous();
        }
        if (Ing.MaterialInstance.IsValid() || !Ing.MaterialInstance.ToSoftObjectPath().IsNull())
        {
            Ing.MaterialInstance.LoadSynchronous();
        }
    }
    
    // Switch to plating widget class if available
    if (PlatingWidgetClass)
    {
        // Store the original widget class before switching
        OriginalWidgetClass = CustomizationWidgetClass;
        CustomizationWidgetClass = PlatingWidgetClass;
        //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Stored original widget class and switched to plating widget class"));
        
        // Remove the current widget if it exists
        if (CustomizationWidget)
        {
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Removing current widget"));
            CustomizationWidget->RemoveFromParent();
            CustomizationWidget = nullptr;
        }
        
        // Create the new plating widget
        UWorld* World = GetWorld();
        if (World)
        {
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Creating new plating widget"));
            CustomizationWidget = CreateWidget<UUserWidget>(World, CustomizationWidgetClass);
            
            if (CustomizationWidget)
            {
                //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::TransitionToPlatingStage - Plating widget created successfully"));
                
                // Try to cast to dish customization widget and set up the connection
                if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
                {
                    //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::TransitionToPlatingStage - Widget is UPUDishCustomizationWidget, connecting to component"));
                    DishWidget->SetCustomizationComponent(this);
                }
                else
                {
                    //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::TransitionToPlatingStage - Widget is not a UPUDishCustomizationWidget"));
                }
                
                // Add the widget to viewport
                CustomizationWidget->AddToViewport();
                //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::TransitionToPlatingStage - Plating widget added to viewport"));
                
                // Create plating ingredient buttons
                if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
                {
                    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Creating plating ingredient slots"));
                    DishWidget->CreatePlatingIngredientSlots();
                }
            }
            else
            {
                //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::TransitionToPlatingStage - Failed to create plating widget"));
            }
        }
        else
        {
            //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::TransitionToPlatingStage - No world available"));
        }
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::TransitionToPlatingStage - No plating widget class set"));
    }
    
    // Broadcast the dish data to the new widget
    BroadcastInitialDishData(DishData);
    
    // Switch to plating camera
    SwitchToPlatingCamera();
    
    // Swap to dish mesh from data table (DishData.DishMesh); fallback to PlatingDishMesh if not set
    TSoftObjectPtr<UStaticMesh> MeshToUse = DishData.DishMesh;
    if (!MeshToUse.IsValid() && MeshToUse.ToSoftObjectPath().IsNull())
    {
        MeshToUse = PlatingDishMesh;  // Fallback to component's default plating mesh
    }

    UStaticMesh* LoadedMesh = nullptr;
    if (MeshToUse.IsValid())
    {
        LoadedMesh = MeshToUse.LoadSynchronous();
    }
    if (!LoadedMesh && !MeshToUse.ToSoftObjectPath().IsNull())
    {
        LoadedMesh = LoadObject<UStaticMesh>(nullptr, *MeshToUse.ToString());
    }
    if (LoadedMesh)
    {
        SwapDishContainerMesh(LoadedMesh);
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::TransitionToPlatingStage - Plating stage transition complete"));
}

void UPUDishCustomizationComponent::EndPlatingStage()
{
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::EndPlatingStage - Ending plating stage"));

    // Capture transforms from live ingredient meshes BEFORE any cleanup (widget removal, mesh destruction)
    CapturePlatingTransformsFromMeshes();

    // Set plating mode to false
    SetPlatingMode(false);

    // Remove the plating widget from viewport
    if (CustomizationWidget)
    {
        //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::EndPlatingStage - Removing plating widget from viewport"));
        CustomizationWidget->RemoveFromParent();
        CustomizationWidget = nullptr;
    }

    // Restore the original widget class
    if (OriginalWidgetClass)
    {
        CustomizationWidgetClass = OriginalWidgetClass;
        //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::EndPlatingStage - Restored original widget class"));
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::EndPlatingStage - No original widget class stored"));
    }

    // Clear the original widget class reference
    OriginalWidgetClass = nullptr;

    // Switch back to cooking camera for the next cooking session
    SwitchToCookingCamera();
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::EndPlatingStage - Switched back to cooking camera"));

    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::EndPlatingStage - Plating stage ended"));
}

void UPUDishCustomizationComponent::SpawnVisualIngredientMesh(const FIngredientInstance& IngredientInstance, const FVector& WorldPosition)
{
    // Get the owner actor (should be the plating station or dish)
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    // Get the world
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FPUIngredientBase& IngredientData = IngredientInstance.IngredientData;

    // Liquid path: spawn Niagara fill system instead of mesh
    if (IngredientData.bIsLiquid && IngredientData.LiquidParticleSystem.IsValid())
    {
        UNiagaraSystem* NiagaraSystem = IngredientData.LiquidParticleSystem.LoadSynchronous();
        if (NiagaraSystem)
        {
            UNiagaraComponent* NiagaraComp = NewObject<UNiagaraComponent>(OwnerActor, UNiagaraComponent::StaticClass(), NAME_None, RF_Transient);
            if (NiagaraComp)
            {
                NiagaraComp->SetAsset(NiagaraSystem);
                NiagaraComp->SetAutoActivate(true);
                NiagaraComp->RegisterComponent();
                NiagaraComp->AttachToComponent(OwnerActor->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
                NiagaraComp->SetWorldLocation(WorldPosition);
                NiagaraComp->Activate(true);

                SpawnedLiquidComponents.Add(TPair<int32, UNiagaraComponent*>(IngredientInstance.InstanceID, NiagaraComp));
            }
        }
        return;
    }

    // Solid path: spawn mesh actor
    UStaticMesh* IngredientMesh = IngredientData.IngredientMesh.LoadSynchronous();
    if (!IngredientMesh)
    {
        IngredientMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube"));
        if (!IngredientMesh)
        {
            return;
        }
    }

    // Use the WorldPosition that was already converted from screen coordinates
    // Add an offset above the surface to ensure ingredients are visible and clickable
    // Scale the offset so larger ingredients spawn higher; smaller ones lower
    // Ingredient data table MeshScale overrides everything when set
    FVector EffectiveScale;
    if (IngredientData.MeshScale.SizeSquared() > KINDA_SMALL_NUMBER)
    {
        EffectiveScale = IngredientData.MeshScale;
    }
    else
    {
        FVector InstanceScale = (IngredientInstance.PlatingScale.SizeSquared() > KINDA_SMALL_NUMBER)
            ? IngredientInstance.PlatingScale : FVector::OneVector;
        EffectiveScale = IngredientMeshScale * InstanceScale;
    }
    float ScaleFactor = FMath::Max(EffectiveScale.GetMax(), 0.01f);  // Avoid zero
    FVector SpawnPosition = WorldPosition + FVector(0, 0, 20 * ScaleFactor);

    // Spawn the interactive ingredient mesh actor
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = OwnerActor;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    UClass* MeshClass = IngredientMeshClass ? IngredientMeshClass.Get() : APUIngredientMesh::StaticClass();
    APUIngredientMesh* SpawnedIngredient = World->SpawnActor<APUIngredientMesh>(MeshClass, SpawnPosition, FRotator::ZeroRotator, SpawnParams);
    
    if (SpawnedIngredient)
    {
        // Initialize with full instance data (handles chopped preparation via procedural mesh slicing)
        SpawnedIngredient->InitializeWithIngredientInstance(IngredientInstance);
        
        // Set mesh for non-chopped case (InitializeWithIngredientInstance handles chopped)
        if (!SpawnedIngredient->IsChopped())
        {
            UStaticMeshComponent* MeshComp = SpawnedIngredient->FindComponentByClass<UStaticMeshComponent>();
            if (MeshComp && IngredientMesh)
            {
                MeshComp->SetMobility(EComponentMobility::Movable);
                MeshComp->SetStaticMesh(IngredientMesh);
                // Re-apply collision/physics after SetStaticMesh (mesh asset can override to incompatible state)
                MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                MeshComp->SetCollisionProfileName(TEXT("PhysicsActor"));
                MeshComp->SetSimulatePhysics(true);
                MeshComp->SetGenerateOverlapEvents(true);
                MeshComp->SetNotifyRigidBodyCollision(true);
            }
        }
        
        // Scale the ingredient; for chopped/minced, must set scale on proc mesh pieces directly (actor scale doesn't propagate)
        SpawnedIngredient->SetIngredientScale(EffectiveScale);

        // Store InstanceID for transform capture before cleanup
        SpawnedIngredient->SetPlatingInstanceID(IngredientInstance.InstanceID);
        
        // Track the spawned mesh for cleanup
        SpawnedIngredientMeshes.Add(SpawnedIngredient);
    }
}

void UPUDishCustomizationComponent::StartCookingStageCameraTransition()
{
    if (!CurrentCharacter)
    {
        return;
    }

    USpringArmComponent* CameraBoom = CurrentCharacter->GetCameraBoom();
    UCameraComponent* FollowCamera = CurrentCharacter->GetFollowCamera();
    if (!CameraBoom || !FollowCamera)
    {
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::StartCookingStageCameraTransition - Starting cooking stage camera transition"));

    // Disable collision detection on the spring arm to prevent jittering
    CameraBoom->bDoCollisionTest = false;
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::StartCookingStageCameraTransition - Disabled spring arm collision detection"));

    // Set target values for cooking stage view
    TargetCameraDistance = CookingCameraDistance;
    TargetCameraPitch = CookingCameraPitch;
    TargetCameraYaw = CookingCameraYaw;
    TargetOrthoWidth = CookingOrthoWidth;
    TargetCameraOffset = OriginalCameraOffset; // Keep the same offset
    TargetCameraPositionIndex = OriginalCameraPositionIndex; // Keep the same position index

    bIsTransitioningCamera = true;
}


void UPUDishCustomizationComponent::SwitchToPlatingCamera()
{
    if (!CurrentCharacter || !GetWorld())
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwitchToPlatingCamera - No character or world available"));
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToPlatingCamera - Switching to plating camera"));

    APlayerController* PlayerController = Cast<APlayerController>(CurrentCharacter->GetController());
    if (!PlayerController)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwitchToPlatingCamera - No player controller found"));
        return;
    }

    // Find the plating station camera component
    if (!PlatingStationCamera)
    {
        AActor* OwnerActor = GetOwner();
        if (OwnerActor)
        {
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToPlatingCamera - Owner actor: %s"), *OwnerActor->GetName());
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToPlatingCamera - Looking for camera component named: %s"), *PlatingStationCameraComponentName.ToString());
            
            // Search for camera component with specific name (since there are multiple cameras)
            TArray<UCameraComponent*> CameraComponents;
            OwnerActor->GetComponents<UCameraComponent>(CameraComponents);
            
            //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToPlatingCamera - Found %d camera components"), CameraComponents.Num());
            
            for (UCameraComponent* CameraComp : CameraComponents)
            {
                //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToPlatingCamera - Camera component: %s"), *CameraComp->GetName());
                if (CameraComp && CameraComp->GetName() == PlatingStationCameraComponentName.ToString())
                {
                    PlatingStationCamera = CameraComp;
                    //UE_LOG(LogTemp,Display, TEXT("✅ UPUDishCustomizationComponent::SwitchToPlatingCamera - Found matching camera component: %s"), *CameraComp->GetName());
                    break;
                }
            }
            
            if (!PlatingStationCamera)
            {
                //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::SwitchToPlatingCamera - No camera component found with name: %s"), *PlatingStationCameraComponentName.ToString());
            }
        }
        else
        {
            //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::SwitchToPlatingCamera - No owner actor found"));
            return;
        }
    }

    if (PlatingStationCamera)
    {
        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SwitchToPlatingCamera - Configuring plating camera: %s"), *PlatingStationCamera->GetName());
        
        // Get transition start position: CookingCamera when viewing station, else current view (e.g. when skipping Cooking)
        FVector StartLocation = PlatingStationCamera->GetComponentLocation();
        FRotator StartRotation = PlatingStationCamera->GetComponentRotation();
        float StartOrthoWidth = PlatingOrthoWidth;
        AActor* StationActor = PlatingStationCamera->GetOwner();
        const bool bViewingStation = StationActor && PlayerController && PlayerController->GetViewTarget() == StationActor;
        if (bViewingStation && StationActor)
        {
            TArray<UCameraComponent*> AllCameras;
            StationActor->GetComponents<UCameraComponent>(AllCameras);
            for (UCameraComponent* Camera : AllCameras)
            {
                if (Camera && Camera->GetName() == TEXT("CookingCamera"))
                {
                    StartLocation = Camera->GetComponentLocation();
                    StartRotation = Camera->GetComponentRotation();
                    StartOrthoWidth = Camera->OrthoWidth;
                    break;
                }
            }
        }
        else if (PlayerController && PlayerController->PlayerCameraManager)
        {
            // Skipping Cooking or not viewing station - use current view so transition starts from where the player is looking
            StartLocation = PlayerController->PlayerCameraManager->GetCameraLocation();
            StartRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
        }

        // Set PlatingCamera to START position first so transition animates from cooking view to plating view
        PlatingStationCamera->SetProjectionMode(ECameraProjectionMode::Orthographic);
        PlatingStationCamera->OrthoWidth = StartOrthoWidth;
        PlatingStationCamera->SetWorldLocation(StartLocation);
        PlatingStationCamera->SetWorldRotation(StartRotation);

        // Disable cooking camera and enable plating camera
        if (StationActor)
        {
            TArray<UCameraComponent*> AllCameras;
            StationActor->GetComponents<UCameraComponent>(AllCameras);
            for (UCameraComponent* Camera : AllCameras)
            {
                if (Camera && Camera->GetName() == TEXT("CookingCamera"))
                {
                    Camera->SetActive(false);
                    break;
                }
            }
            PlatingStationCamera->SetActive(true);
        }

        // Ensure we're viewing the station (needed when skipping Cooking stage - ViewTarget might still be character)
        if (StationActor)
        {
            PlayerController->SetViewTargetWithBlend(StationActor, 0.0f);  // Instant - our transition handles the animation
        }

        // Set target position so StartPlatingCameraTransition can read it, then restore start for first frame
        FVector TargetLocation = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 200.0f) + PlatingCameraPositionOffset;
        FRotator TargetRotation = FRotator(PlatingCameraPitch, PlatingCameraYaw, 0.0f);
        PlatingStationCamera->SetWorldLocation(TargetLocation);
        PlatingStationCamera->SetWorldRotation(TargetRotation);

        // Start smooth transition (reads target from camera; pass explicit start when we used fallback e.g. skipping Cooking)
        StartPlatingCameraTransition(&StartLocation, &StartRotation, StartOrthoWidth);

        // Restore start position for first frame - UpdatePlatingCameraTransition will animate from here
        PlatingStationCamera->SetWorldLocation(StartLocation);
        PlatingStationCamera->SetWorldRotation(StartRotation);
        PlatingStationCamera->OrthoWidth = StartOrthoWidth;
    }
    else
    {
        //UE_LOG(LogTemp,Error, TEXT("❌ UPUDishCustomizationComponent::SwitchToPlatingCamera - No camera component found on plating station"));
    }
}

void UPUDishCustomizationComponent::StartPlatingCameraTransition(const FVector* ExplicitStartLocation, const FRotator* ExplicitStartRotation, float ExplicitStartOrthoWidth)
{
    if (!PlatingStationCamera || !CurrentCharacter)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::StartPlatingCameraTransition - No camera or character available"));
        return;
    }

    //UE_LOG(LogTemp,Display, TEXT("🎬 UPUDishCustomizationComponent::StartPlatingCameraTransition - Starting smooth camera transition"));

    // Use explicit start when provided (e.g. when skipping Cooking), else derive from CookingCamera
    FVector CurrentLocation = PlatingStationCamera->GetComponentLocation();
    FRotator CurrentRotation = PlatingStationCamera->GetComponentRotation();
    float CurrentOrthoWidth = PlatingStationCamera->OrthoWidth;

    if (ExplicitStartLocation && ExplicitStartRotation && ExplicitStartOrthoWidth >= 0.0f)
    {
        CurrentLocation = *ExplicitStartLocation;
        CurrentRotation = *ExplicitStartRotation;
        CurrentOrthoWidth = ExplicitStartOrthoWidth;
    }
    else
    {
        AActor* StationActor = PlatingStationCamera->GetOwner();
        if (StationActor)
        {
            TArray<UCameraComponent*> AllCameras;
            StationActor->GetComponents<UCameraComponent>(AllCameras);
            for (UCameraComponent* Camera : AllCameras)
            {
                if (Camera && Camera->GetName() == TEXT("CookingCamera"))
                {
                    CurrentLocation = Camera->GetComponentLocation();
                    CurrentRotation = Camera->GetComponentRotation();
                    CurrentOrthoWidth = Camera->OrthoWidth;
                    break;
                }
            }
        }
    }

    // Set up transition state
    bPlatingCameraTransitioning = true;
    PlatingCameraTransitionTime = 0.0f;
    PlatingCameraStartLocation = CurrentLocation;
    PlatingCameraStartRotation = CurrentRotation;

    // Target position and properties (already set in SwitchToPlatingCamera)
    PlatingCameraTargetLocation = PlatingStationCamera->GetComponentLocation();
    PlatingCameraTargetRotation = PlatingStationCamera->GetComponentRotation();

    // Store start and target ortho widths for smooth transition
    PlatingCameraStartOrthoWidth = CurrentOrthoWidth;
    PlatingCameraTargetOrthoWidth = PlatingOrthoWidth;

    //UE_LOG(LogTemp,Display, TEXT("🎬 UPUDishCustomizationComponent::StartPlatingCameraTransition - Start: %s (Ortho: %.2f), Target: %s (Ortho: %.2f)"), 
    //    *PlatingCameraStartLocation.ToString(), PlatingCameraStartOrthoWidth,
    //    *PlatingCameraTargetLocation.ToString(), PlatingCameraTargetOrthoWidth);
}

void UPUDishCustomizationComponent::UpdatePlatingCameraTransition(float DeltaTime)
{
    if (!bPlatingCameraTransitioning || !PlatingStationCamera)
    {
        return;
    }

    PlatingCameraTransitionTime += DeltaTime;
    float Alpha = FMath::Clamp(PlatingCameraTransitionTime / PlatingCameraTransitionDuration, 0.0f, 1.0f);

    // Use smooth interpolation
    float SmoothAlpha = FMath::SmoothStep(0.0f, 1.0f, Alpha);

    // Interpolate position and rotation
    FVector NewLocation = FMath::Lerp(PlatingCameraStartLocation, PlatingCameraTargetLocation, SmoothAlpha);
    FRotator NewRotation = FMath::Lerp(PlatingCameraStartRotation, PlatingCameraTargetRotation, SmoothAlpha);

    // Interpolate ortho width smoothly
    float NewOrthoWidth = FMath::Lerp(PlatingCameraStartOrthoWidth, PlatingCameraTargetOrthoWidth, SmoothAlpha);

    // Update camera position, rotation, and ortho width
    PlatingStationCamera->SetWorldLocation(NewLocation);
    PlatingStationCamera->SetWorldRotation(NewRotation);
    PlatingStationCamera->OrthoWidth = NewOrthoWidth;

    // Check if transition is complete
    if (Alpha >= 1.0f)
    {
        bPlatingCameraTransitioning = false;
        //UE_LOG(LogTemp,Display, TEXT("🎬 UPUDishCustomizationComponent::UpdatePlatingCameraTransition - Transition complete"));
    }
}

void UPUDishCustomizationComponent::SetPlatingCameraPositionOffset(const FVector& NewOffset)
{
    //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SetPlatingCameraPositionOffset - Setting camera offset to: %s"),
    //    *NewOffset.ToString());

    PlatingCameraPositionOffset = NewOffset;

    // If the plating camera component is found, update its position
    if (PlatingStationCamera)
    {
        FVector CameraLocation = GetOwner()->GetActorLocation() + FVector(0.0f, 0.0f, 200.0f) + PlatingCameraPositionOffset;
        FRotator CameraRotation = FRotator(PlatingCameraPitch, PlatingCameraYaw, 0.0f);

        PlatingStationCamera->SetWorldLocation(CameraLocation);
        PlatingStationCamera->SetWorldRotation(CameraRotation);

        //UE_LOG(LogTemp,Display, TEXT("🎯 UPUDishCustomizationComponent::SetPlatingCameraPositionOffset - Updated existing camera position to: %s"),
        //    *CameraLocation.ToString());
    }
}

// Plating placement limits implementation
bool UPUDishCustomizationComponent::CanPlaceIngredient(int32 InstanceID) const
{
    // Find the ingredient instance
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.InstanceID == InstanceID)
        {
            int32 PlacedQuantity = GetPlacedQuantity(InstanceID);
            bool bCanPlace = PlacedQuantity < Instance.Quantity;
            
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::CanPlaceIngredient - Instance %d: Placed %d/%d, Can place: %s"), 
            //    InstanceID, PlacedQuantity, Instance.Quantity, bCanPlace ? TEXT("Yes") : TEXT("No"));
            
            return bCanPlace;
        }
    }
    
    //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::CanPlaceIngredient - Instance %d not found"), InstanceID);
    return false;
}

int32 UPUDishCustomizationComponent::GetRemainingQuantity(int32 InstanceID) const
{
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.InstanceID == InstanceID)
        {
            int32 PlacedQuantity = GetPlacedQuantity(InstanceID);
            int32 Remaining = Instance.Quantity - PlacedQuantity;
            
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::GetRemainingQuantity - Instance %d: %d remaining"), 
            //    InstanceID, Remaining);
            
            return Remaining;
        }
    }
    
    //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::GetRemainingQuantity - Instance %d not found"), InstanceID);
    return 0;
}

int32 UPUDishCustomizationComponent::GetPlacedQuantity(int32 InstanceID) const
{
    if (const int32* PlacedQuantity = PlacedIngredientQuantities.Find(InstanceID))
    {
        return *PlacedQuantity;
    }
    return 0;
}

void UPUDishCustomizationComponent::PlaceIngredient(int32 InstanceID)
{
    int32 CurrentPlaced = GetPlacedQuantity(InstanceID);
    PlacedIngredientQuantities.Add(InstanceID, CurrentPlaced + 1);
    
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::PlaceIngredient - Instance %d: Placed %d"), 
    //    InstanceID, CurrentPlaced + 1);
}

void UPUDishCustomizationComponent::RemoveIngredient(int32 InstanceID)
{
    if (int32* PlacedQuantity = PlacedIngredientQuantities.Find(InstanceID))
    {
        if (*PlacedQuantity > 0)
        {
            (*PlacedQuantity)--;
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::RemoveIngredient - Instance %d: Now placed %d"), 
            //    InstanceID, *PlacedQuantity);
        }
    }
}

void UPUDishCustomizationComponent::ResetPlatingPlacements()
{
    PlacedIngredientQuantities.Empty();
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::ResetPlatingPlacements - Reset all placement tracking"));
}

// Blueprint-callable plating limits
bool UPUDishCustomizationComponent::CanPlaceIngredientByTag(const FGameplayTag& IngredientTag) const
{
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.IngredientData.IngredientTag == IngredientTag)
        {
            return CanPlaceIngredient(Instance.InstanceID);
        }
    }
    
    //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::CanPlaceIngredientByTag - Ingredient %s not found"), *IngredientTag.ToString());
    return false;
}

int32 UPUDishCustomizationComponent::GetRemainingQuantityByTag(const FGameplayTag& IngredientTag) const
{
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.IngredientData.IngredientTag == IngredientTag)
        {
            return GetRemainingQuantity(Instance.InstanceID);
        }
    }
    
    //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::GetRemainingQuantityByTag - Ingredient %s not found"), *IngredientTag.ToString());
    return 0;
}

int32 UPUDishCustomizationComponent::GetPlacedQuantityByTag(const FGameplayTag& IngredientTag) const
{
    for (const FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.IngredientData.IngredientTag == IngredientTag)
        {
            return GetPlacedQuantity(Instance.InstanceID);
        }
    }
    
    //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::GetPlacedQuantityByTag - Ingredient %s not found"), *IngredientTag.ToString());
    return 0;
}

void UPUDishCustomizationComponent::UpdateIngredientSlotQuantity(int32 InstanceID)
{
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::UpdateIngredientSlotQuantity - Updating slot quantity for InstanceID: %d"), InstanceID);
    
    // Find the ingredient slot and update its quantity
    if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
    {
        // Get the created ingredient slots from the widget
        const TArray<class UPUIngredientSlot*>& IngredientSlots = DishWidget->GetCreatedIngredientSlots();
        
        // Find the slot with matching InstanceID
        for (UPUIngredientSlot* IngredientSlot : IngredientSlots)
        {
            if (IngredientSlot && IngredientSlot->GetIngredientInstance().InstanceID == InstanceID)
            {
                // Decrease the slot's remaining quantity
                IngredientSlot->DecreaseQuantity();
                //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::UpdateIngredientSlotQuantity - Decreased quantity for slot (InstanceID: %d)"), InstanceID);
                return;
            }
        }
        
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::UpdateIngredientSlotQuantity - No slot found for InstanceID: %d"), InstanceID);
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::UpdateIngredientSlotQuantity - No customization widget found"));
    }
}

void UPUDishCustomizationComponent::ResetPlating()
{
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::ResetPlating - Resetting all plating"));
    
    // Clear all placed ingredient quantities
    PlacedIngredientQuantities.Empty();
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::ResetPlating - Cleared placed ingredient quantities"));
    
    // Reset all ingredient slot quantities (for plating stage)
    if (UPUDishCustomizationWidget* DishWidget = Cast<UPUDishCustomizationWidget>(CustomizationWidget))
    {
        // Get the created ingredient slots from the widget
        const TArray<class UPUIngredientSlot*>& IngredientSlots = DishWidget->GetCreatedIngredientSlots();
        
        //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::ResetPlating - Found %d ingredient slots to reset"), 
        //    IngredientSlots.Num());
        
        // Reset each slot's quantity from the dish data
        // In plating mode, slots are created with ActiveIngredientArea location (not Plating)
        // So we reset all slots that have ingredients, regardless of location
        for (UPUIngredientSlot* IngredientSlot : IngredientSlots)
        {
            if (IngredientSlot && IngredientSlot->GetIngredientInstance().InstanceID != 0)
            {
                IngredientSlot->ResetQuantityFromDishData();
                //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::ResetPlating - Reset slot for InstanceID: %d (Location: %d)"), 
                //    IngredientSlot->GetIngredientInstance().InstanceID, (int32)IngredientSlot->GetLocation());
            }
        }
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::ResetPlating - No customization widget found"));
    }
    
    // Clear all 3D ingredient meshes
    ClearAll3DIngredientMeshes();
    
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::ResetPlating - Plating reset complete"));
}

void UPUDishCustomizationComponent::CapturePlatingTransformsFromMeshes()
{
    UE_LOG(LogDishPreview, Log, TEXT("CapturePlatingTransformsFromMeshes - %d meshes, %d liquids to capture"), SpawnedIngredientMeshes.Num(), SpawnedLiquidComponents.Num());

    // Capture dish surface center for consistent ingredient placement when copying to preview
    float SurfaceHeight;
    if (GetPlateSurfaceInfo(SurfaceHeight, CurrentDishData.PlatingDishCenter))
    {
        UE_LOG(LogDishPreview, Log, TEXT("CapturePlatingTransformsFromMeshes - dish center (%.1f, %.1f, %.1f)"), CurrentDishData.PlatingDishCenter.X, CurrentDishData.PlatingDishCenter.Y, CurrentDishData.PlatingDishCenter.Z);
    }
    else
    {
        CurrentDishData.PlatingDishCenter = FVector::ZeroVector;
    }

    CurrentDishData.PlatingEntries.Empty();
    int32 Captured = 0;

    for (APUIngredientMesh* IngredientMesh : SpawnedIngredientMeshes)
    {
        if (!IngredientMesh || !IsValid(IngredientMesh))
        {
            continue;
        }

        const int32 InstanceID = IngredientMesh->GetPlatingInstanceID();
        if (InstanceID < 0)
        {
            UE_LOG(LogDishPreview, Warning, TEXT("CapturePlatingTransformsFromMeshes - mesh has invalid InstanceID %d, skipping"), InstanceID);
            continue;
        }

        const FVector WorldPos = IngredientMesh->GetActorLocation();
        const FRotator WorldRot = IngredientMesh->GetActorRotation();
        FVector WorldScale = IngredientMesh->GetActorScale3D();
        if (WorldScale.SizeSquared() < KINDA_SMALL_NUMBER)
        {
            WorldScale = FVector::OneVector;
        }

        CurrentDishData.SetIngredientPlating(InstanceID, WorldPos, WorldRot, WorldScale);
        FPUPlatingEntry Entry;
        Entry.InstanceID = InstanceID;
        Entry.Position = WorldPos;
        Entry.Rotation = WorldRot;
        Entry.Scale = WorldScale;
        Entry.bIsLiquid = false;
        // For chopped/minced ingredients, capture each piece's world transform so preview shows broken-apart layout
        if (IngredientMesh->IsChopped())
        {
            Entry.ChoppedPieceTransforms = IngredientMesh->GetChoppedPieceWorldTransforms();
        }
        CurrentDishData.PlatingEntries.Add(Entry);
        Captured++;
        UE_LOG(LogDishPreview, Log, TEXT("CapturePlatingTransformsFromMeshes - InstanceID %d at (%.1f, %.1f, %.1f)"), InstanceID, WorldPos.X, WorldPos.Y, WorldPos.Z);
    }

    for (const TPair<int32, UNiagaraComponent*>& Pair : SpawnedLiquidComponents)
    {
        const int32 InstanceID = Pair.Key;
        UNiagaraComponent* NiagaraComp = Pair.Value;
        if (!NiagaraComp || !IsValid(NiagaraComp))
        {
            continue;
        }

        const FVector WorldPos = NiagaraComp->GetComponentLocation();
        CurrentDishData.SetIngredientPlating(InstanceID, WorldPos, FRotator::ZeroRotator, FVector::OneVector);
        FPUPlatingEntry Entry;
        Entry.InstanceID = InstanceID;
        Entry.Position = WorldPos;
        Entry.Rotation = FRotator::ZeroRotator;
        Entry.Scale = FVector::OneVector;
        Entry.bIsLiquid = true;
        CurrentDishData.PlatingEntries.Add(Entry);
        Captured++;
        UE_LOG(LogDishPreview, Log, TEXT("CapturePlatingTransformsFromMeshes - liquid InstanceID %d at (%.1f, %.1f, %.1f)"), InstanceID, WorldPos.X, WorldPos.Y, WorldPos.Z);
    }

    UE_LOG(LogDishPreview, Log, TEXT("CapturePlatingTransformsFromMeshes - captured %d transforms"), Captured);
}

void UPUDishCustomizationComponent::ClearAll3DIngredientMeshes()
{
    // Stop any active dragging before clearing
    if (bIsDragging && CurrentlyDraggedIngredient)
    {
        bIsDragging = false;
        if (IsValid(CurrentlyDraggedIngredient))
        {
            CurrentlyDraggedIngredient->OnMouseRelease();
        }
        CurrentlyDraggedIngredient = nullptr;
    }
    
    // Destroy all tracked ingredient meshes
    for (APUIngredientMesh* IngredientMesh : SpawnedIngredientMeshes)
    {
        if (IngredientMesh != nullptr && IsValid(IngredientMesh))
        {
            IngredientMesh->Destroy();
        }
    }
    SpawnedIngredientMeshes.Empty();
    
    // Destroy all tracked liquid Niagara components
    for (const TPair<int32, UNiagaraComponent*>& Pair : SpawnedLiquidComponents)
    {
        if (UNiagaraComponent* NiagaraComp = Pair.Value)
        {
            if (IsValid(NiagaraComp))
            {
                NiagaraComp->Deactivate();
                NiagaraComp->DestroyComponent();
            }
        }
    }
    SpawnedLiquidComponents.Empty();
}

void UPUDishCustomizationComponent::SwapDishContainerMesh(UStaticMesh* NewDishMesh)
{
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SwapDishContainerMesh - Swapping dish container mesh"));
    
    if (!NewDishMesh)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwapDishContainerMesh - NewDishMesh is null"));
        return;
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SwapDishContainerMesh - NewDishMesh is valid: %s"), 
    //    *NewDishMesh->GetName());
    
    // Use the owner (the cooking station this component belongs to) - NOT a world search.
    // With multiple cooking stations, a world search would always pick the first instance.
    AActor* DishStation = GetOwner();
    if (!DishStation)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwapDishContainerMesh - DishStation not found"));
        return;
    }
    
    // Find the DishContainer child component (nested in StationMesh)
    UStaticMeshComponent* DishContainer = nullptr;
    
    // First, find the StationMesh component
    UStaticMeshComponent* StationMesh = nullptr;
    TArray<UStaticMeshComponent*> AllMeshComponents;
    DishStation->GetComponents<UStaticMeshComponent>(AllMeshComponents);
    
    for (UStaticMeshComponent* MeshComp : AllMeshComponents)
    {
        if (MeshComp->GetName().Contains(TEXT("StationMesh")))
        {
            StationMesh = MeshComp;
            break;
        }
    }
    
    if (!StationMesh)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwapDishContainerMesh - StationMesh component not found"));
        return;
    }
    
    // Now find DishContainer as a child of StationMesh using a simpler approach
    TArray<UStaticMeshComponent*> AllMeshComponents2;
    DishStation->GetComponents<UStaticMeshComponent>(AllMeshComponents2);
    
    for (UStaticMeshComponent* MeshComp : AllMeshComponents2)
    {
        if (MeshComp->GetName().Contains(TEXT("DishContainer")))
        {
            DishContainer = MeshComp;
            break;
        }
    }
    
    if (!DishContainer)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwapDishContainerMesh - DishContainer component not found"));
        return;
    }
    
    // Clear any child static mesh components under DishContainer
    // Get all components from the DishContainer's owner actor and clear only child meshes
    TArray<UStaticMeshComponent*> AllChildMeshes;
    DishContainer->GetOwner()->GetComponents<UStaticMeshComponent>(AllChildMeshes);
    for (UStaticMeshComponent* ChildMesh : AllChildMeshes)
    {
        // Only clear meshes that are children of DishContainer, not StationMesh or other main components
        if (ChildMesh != DishContainer && ChildMesh != StationMesh && 
            ChildMesh->GetAttachParent() == DishContainer)
        {
            ChildMesh->SetStaticMesh(nullptr);
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SwapDishContainerMesh - Cleared child mesh: %s"), 
            //    *ChildMesh->GetName());
        }
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SwapDishContainerMesh - Cleared child meshes only"));
    
    // Add the new dish mesh under DishContainer
    if (NewDishMesh)
    {
        // Set the mesh on the main DishContainer component
        DishContainer->SetStaticMesh(NewDishMesh);
        
        //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::SwapDishContainerMesh - Set mesh on DishContainer: %s"), 
        //    *NewDishMesh->GetName());
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::SwapDishContainerMesh - NewDishMesh is null!"));
    }
}

void UPUDishCustomizationComponent::RestoreOriginalDishContainerMesh()
{
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::RestoreOriginalDishContainerMesh - Restoring original dish container mesh"));
    
    // Use the owner (the cooking station this component belongs to) - NOT a world search.
    // With multiple cooking stations, a world search would always pick the first instance.
    AActor* DishStation = GetOwner();
    if (!DishStation)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::RestoreOriginalDishContainerMesh - BP_CookingStation not found"));
        return;
    }
    
    // Find the DishContainer child component
    UStaticMeshComponent* DishContainer = nullptr;
    TArray<UStaticMeshComponent*> AllMeshComponents2;
    DishStation->GetComponents<UStaticMeshComponent>(AllMeshComponents2);
    
    for (UStaticMeshComponent* MeshComp : AllMeshComponents2)
    {
        if (MeshComp->GetName().Contains(TEXT("DishContainer")))
        {
            DishContainer = MeshComp;
            break;
        }
    }
    
    if (!DishContainer)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::RestoreOriginalDishContainerMesh - DishContainer component not found"));
        return;
    }
    
    // Clear any existing children first
    TArray<UStaticMeshComponent*> AllChildMeshes;
    DishContainer->GetOwner()->GetComponents<UStaticMeshComponent>(AllChildMeshes);
    for (UStaticMeshComponent* ChildMesh : AllChildMeshes)
    {
        // Only clear meshes that are children of DishContainer, not StationMesh or other main components
        if (ChildMesh != DishContainer && ChildMesh->GetAttachParent() == DishContainer)
        {
            ChildMesh->SetStaticMesh(nullptr);
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::RestoreOriginalDishContainerMesh - Cleared child mesh: %s"), 
            //    *ChildMesh->GetName());
        }
    }

    // Restore the original mesh
    if (OriginalDishContainerMesh)
    {
        DishContainer->SetStaticMesh(OriginalDishContainerMesh);
        //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::RestoreOriginalDishContainerMesh - Restored original mesh: %s"), 
        //    *OriginalDishContainerMesh->GetName());
    }
    else
    {
        // Clear the mesh if no original was stored
        DishContainer->SetStaticMesh(nullptr);
        //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::RestoreOriginalDishContainerMesh - Cleared mesh (no original stored)"));
    }

    // Restore the original children meshes
    for (UStaticMesh* ChildMesh : OriginalDishContainerChildren)
    {
        if (ChildMesh)
        {
            // Create a new static mesh component for the child
            UStaticMeshComponent* NewChildComponent = NewObject<UStaticMeshComponent>(DishContainer->GetOwner());
            NewChildComponent->SetStaticMesh(ChildMesh);
            NewChildComponent->SetupAttachment(DishContainer);
            NewChildComponent->RegisterComponent();
            
            //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::RestoreOriginalDishContainerMesh - Restored child mesh: %s"), 
            //    *ChildMesh->GetName());
        }
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::RestoreOriginalDishContainerMesh - Restored %d child meshes"), 
    //    OriginalDishContainerChildren.Num());
}

void UPUDishCustomizationComponent::StoreOriginalDishContainerMesh()
{
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::StoreOriginalDishContainerMesh - Storing original dish container mesh"));
    
    // Use the owner (the cooking station this component belongs to) - NOT a world search.
    // With multiple cooking stations, a world search would always pick the first instance.
    AActor* DishStation = GetOwner();
    if (!DishStation)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::StoreOriginalDishContainerMesh - BP_CookingStation not found"));
        return;
    }
    
    // Find the DishContainer child component
    UStaticMeshComponent* DishContainer = nullptr;
    TArray<UStaticMeshComponent*> AllMeshComponents2;
    DishStation->GetComponents<UStaticMeshComponent>(AllMeshComponents2);
    
    for (UStaticMeshComponent* MeshComp : AllMeshComponents2)
    {
        if (MeshComp->GetName().Contains(TEXT("DishContainer")))
        {
            DishContainer = MeshComp;
            break;
        }
    }
    
    if (!DishContainer)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ UPUDishCustomizationComponent::StoreOriginalDishContainerMesh - DishContainer component not found"));
        return;
    }
    
    // Store the original mesh
    OriginalDishContainerMesh = DishContainer->GetStaticMesh();
    if (OriginalDishContainerMesh)
    {
        //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::StoreOriginalDishContainerMesh - Stored original mesh: %s"), 
        //    *OriginalDishContainerMesh->GetName());
    }
    else
    {
        //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::StoreOriginalDishContainerMesh - No original mesh found (DishContainer was empty)"));
    }

    // Store the original children meshes
    OriginalDishContainerChildren.Empty();
    TArray<UStaticMeshComponent*> AllChildMeshes;
    DishContainer->GetOwner()->GetComponents<UStaticMeshComponent>(AllChildMeshes);
    for (UStaticMeshComponent* ChildMesh : AllChildMeshes)
    {
        // Only store meshes that are children of DishContainer, not StationMesh or other main components
        if (ChildMesh != DishContainer && ChildMesh->GetAttachParent() == DishContainer)
        {
            UStaticMesh* ChildStaticMesh = ChildMesh->GetStaticMesh();
            if (ChildStaticMesh)
            {
                OriginalDishContainerChildren.Add(ChildStaticMesh);
                //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::StoreOriginalDishContainerMesh - Stored child mesh: %s"), 
                //    *ChildStaticMesh->GetName());
            }
        }
    }
    
    //UE_LOG(LogTemp,Display, TEXT("🍽️ UPUDishCustomizationComponent::StoreOriginalDishContainerMesh - Stored %d child meshes"), 
    //    OriginalDishContainerChildren.Num());
}