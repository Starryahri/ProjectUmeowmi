#include "PUDialogueBox.h"
#include "PUDialogueOption.h"
#include "DlgSystem/DlgContext.h"
#include "GameFramework/PlayerController.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/Button.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectUmeowmi/ProjectUmeowmiCharacter.h"
#include "ProjectUmeowmi/Dialogue/TalkingObject.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameViewportClient.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"

UPUDialogueBox::UPUDialogueBox(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UPUDialogueBox::Open(UDlgContext* ActiveContext)
{
    Open_Implementation(ActiveContext);
}

void UPUDialogueBox::Close()
{
    Close_Implementation();
}

void UPUDialogueBox::Update(UDlgContext* ActiveContext)
{
    Update_Implementation(ActiveContext);
}

void UPUDialogueBox::NativeConstruct()
{
    Super::NativeConstruct();
    SetVisibility(ESlateVisibility::Hidden);
    
    // Ensure we're focusable
    SetIsFocusable(true);
    
    // Add to viewport if not already there
    if (!IsInViewport())
    {
        AddToViewport();
    }

    // Initialize vignette intensity
    CurrentVignetteIntensity = 0.0f;
    TargetVignetteIntensity = 0.0f;
    bVignetteAnimating = false;
    CurrentFadeDuration = 1.0f; // Default - will be overridden when animation starts

    // Debug: Log material status on construct
    //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::NativeConstruct - VignetteMaterialDirect: %s"), 
    //    VignetteMaterialDirect ? *VignetteMaterialDirect->GetName() : TEXT("NULL"));
    //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::NativeConstruct - VignetteMaterial (soft): %s"), 
    //    VignetteMaterial.ToSoftObjectPath().IsNull() ? TEXT("NULL") : *VignetteMaterial.ToSoftObjectPath().ToString());
    //UE_LOG(LogTemp,Warning, TEXT("PUDialogueBox::NativeConstruct - Fade Durations: In=%.6f, Out=%.6f"), 
    //    VignetteFadeInDuration, VignetteFadeOutDuration);
}

void UPUDialogueBox::NativeDestruct()
{
    // Clear any active vignette animation timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(VignetteAnimationTimer);
    }

    // Stop animating
    bVignetteAnimating = false;

    // Clean up dynamic material reference
    VignetteDynamicMaterial = nullptr;

    Super::NativeDestruct();
}

void UPUDialogueBox::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Update vignette if animating
    if (bVignetteAnimating)
    {
        UpdateVignetteIntensity();
    }
}

void UPUDialogueBox::Open_Implementation(UDlgContext* ActiveContext)
{
    //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::Open_Implementation called"));
    // Debug logging for widget state
    //UE_LOG(LogTemp,Log, TEXT("DialogueBox pointer: %p"), this);
    //UE_LOG(LogTemp,Log, TEXT("Current visibility: %d"), (int32)GetVisibility());
    //UE_LOG(LogTemp,Log, TEXT("Is in viewport: %d"), IsInViewport());
    //UE_LOG(LogTemp,Log, TEXT("Parent widget: %p"), GetParent());

    // Initialize and animate vignette in
    //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::Open_Implementation - Starting vignette animation (Target Intensity: %.2f)"), VignetteIntensityTarget);
    InitializeVignetteMaterial();
    AnimateVignetteToTarget(VignetteIntensityTarget);

    // Make sure we're visible and can receive focus
    SetVisibility(ESlateVisibility::Visible);
    SetIsFocusable(true);

    // Ensure we're in the viewport
    if (!IsInViewport())
    {
        AddToViewport();
    }

    // Try to get the player controller
    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        // If we don't have a player controller yet, try to get it from the game instance
        if (UWorld* World = GetWorld())
        {
            PC = World->GetFirstPlayerController();
        }
    }

    if (PC)
    {
        //UE_LOG(LogTemp,Log, TEXT("PlayerController found: %p"), PC);
        
        // Get the local player
        ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
        if (!LocalPlayer)
        {
            //UE_LOG(LogTemp,Error, TEXT("PUDialogueBox::Open_Implementation - No local player found!"));
            return;
        }

        // Get the game viewport
        if (UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport())
        {
            // Disable player movement and input
            PC->SetIgnoreMoveInput(true);
            PC->SetIgnoreLookInput(true);

            // Disable player movement
            if (APawn* Pawn = PC->GetPawn())
            {
                if (ACharacter* PlayerCharacter = Cast<ACharacter>(Pawn))
                {
                    if (UCharacterMovementComponent* MovementComponent = PlayerCharacter->GetCharacterMovement())
                    {
                        MovementComponent->DisableMovement();
                        MovementComponent->StopMovementImmediately();
                    }
                }
            }
            
            // Set focus immediately
            if (IsValid(this) && IsValid(PC) && IsValid(LocalPlayer))
            {
                // Make sure we're still in the viewport
                if (!IsInViewport())
                {
                    AddToViewport();
                }

                // Get the widget's slate widget
                TSharedPtr<SWidget> SlateWidget = GetCachedWidget();
                if (SlateWidget.IsValid())
                {
                    // Set focus to this widget
                    FSlateApplication::Get().SetKeyboardFocus(SlateWidget);
                    
                    // If we have dialogue options, set focus to the first option
                    if (IsValid(DialogueOptions) && DialogueOptions->GetChildrenCount() > 0)
                    {
                        if (UPUDialogueOption* FirstOption = Cast<UPUDialogueOption>(DialogueOptions->GetChildAt(0)))
                        {
                            if (FirstOption->OptionButton)
                            {
                                TSharedPtr<SWidget> ButtonSlateWidget = FirstOption->OptionButton->GetCachedWidget();
                                if (ButtonSlateWidget.IsValid())
                                {
                                    FSlateApplication::Get().SetKeyboardFocus(ButtonSlateWidget);
                                }
                            }
                        }
                    }
                }
            }
        }
        else
        {
            //UE_LOG(LogTemp,Error, TEXT("PUDialogueBox::Open_Implementation - No game viewport found!"));
        }
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("No PlayerController found!"));
    }

    Update(ActiveContext);
}

void UPUDialogueBox::Close_Implementation()
{
    //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::Close_Implementation called"));
    //UE_LOG(LogTemp,Log, TEXT("Current visibility state: %d"), (int32)GetVisibility());
    
    // Clear the context reference to prevent dangling references
    CurrentContext = nullptr;
    
    SetVisibility(ESlateVisibility::Hidden);
    //UE_LOG(LogTemp,Log, TEXT("Visibility set to hidden. New visibility state: %d"), (int32)GetVisibility());

    // Animate vignette out
    //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::Close_Implementation - Starting vignette fade out"));
    AnimateVignetteToTarget(0.0f);

    // Try to get the player controller from the widget owner first
    APlayerController* PC = GetOwningPlayer();
    
    // If we don't have a player controller, try to get it from the world
    if (!PC)
    {
        //UE_LOG(LogTemp,Log, TEXT("No player controller from widget owner, trying to get from world"));
        if (UWorld* World = GetWorld())
        {
            PC = World->GetFirstPlayerController();
        }
    }

    if (PC)
    {
        //UE_LOG(LogTemp,Log, TEXT("Found player controller: %p"), PC);
        
        // Re-enable player movement and input
        PC->SetIgnoreMoveInput(false);
        PC->SetIgnoreLookInput(false);
        
        // Re-enable player movement
        if (APawn* Pawn = PC->GetPawn())
        {
            if (ACharacter* PlayerCharacter = Cast<ACharacter>(Pawn))
            {
                if (UCharacterMovementComponent* MovementComponent = PlayerCharacter->GetCharacterMovement())
                {
                    MovementComponent->SetMovementMode(MOVE_Walking);
                }
            }
        }
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("Could not find player controller!"));
    }
}

void UPUDialogueBox::SetVignetteMaterial(UMaterialInterface* NewVignetteMaterial)
{
    if (NewVignetteMaterial)
    {
        // Set both the direct reference and the soft pointer
        VignetteMaterialDirect = NewVignetteMaterial;
        VignetteMaterial = NewVignetteMaterial;
        //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::SetVignetteMaterial - Vignette material set to: %s (Type: %s)"), 
        //    *NewVignetteMaterial->GetName(), *NewVignetteMaterial->GetClass()->GetName());
        
        // If we already have a dynamic material, we need to recreate it with the new base material
        if (VignetteDynamicMaterial)
        {
            // Remove old material from camera
            if (UCameraComponent* PlayerCamera = GetPlayerCamera())
            {
                PlayerCamera->PostProcessSettings.WeightedBlendables.Array.RemoveAll(
                    [this](const FWeightedBlendable& Blendable)
                    {
                        return Blendable.Object == VignetteDynamicMaterial;
                    }
                );
            }
            
            // Clear old dynamic material
            VignetteDynamicMaterial = nullptr;
        }
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("PUDialogueBox::SetVignetteMaterial - Attempted to set null vignette material"));
    }
}

void UPUDialogueBox::DebugVignetteMaterial() const
{
    //UE_LOG(LogTemp,Warning, TEXT("=== VIGNETTE MATERIAL DEBUG ==="));
    //UE_LOG(LogTemp,Warning, TEXT("VignetteMaterialDirect: %s"), 
    //    VignetteMaterialDirect ? *VignetteMaterialDirect->GetName() : TEXT("NULL"));
    //UE_LOG(LogTemp,Warning, TEXT("VignetteMaterial (soft): %s"), 
    //    VignetteMaterial.ToSoftObjectPath().IsNull() ? TEXT("NULL") : *VignetteMaterial.ToSoftObjectPath().ToString());
    //UE_LOG(LogTemp,Warning, TEXT("VignetteDynamicMaterial: %s"), 
    //    VignetteDynamicMaterial ? *VignetteDynamicMaterial->GetName() : TEXT("NULL"));
    //UE_LOG(LogTemp,Warning, TEXT("Current Intensity: %.2f, Target: %.2f"), CurrentVignetteIntensity, TargetVignetteIntensity);
    //UE_LOG(LogTemp,Warning, TEXT("================================="));
}

void UPUDialogueBox::Update_Implementation(UDlgContext* ActiveContext)
{
    //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::Update_Implementation called"));
    
    // Store the context for reference
    CurrentContext = ActiveContext;
    
    if (IsValid(ActiveContext))
    {
        FText ParticipantDisplayName = ActiveContext->GetActiveNodeParticipantDisplayName();
        FText NodeText = ActiveContext->GetActiveNodeText();

        if (IsValid(ParticipantNameText))
        {
            ParticipantNameText->SetText(ParticipantDisplayName);
        }
        if (IsValid(DialogueText))
        {
            DialogueText->SetText(NodeText);
        }
        if (IsValid(ParticipantImage))
        {
            ParticipantImage->SetBrushFromTexture(ActiveContext->GetActiveNodeParticipantIcon());
        }

        // Update and recurse through the rest of the options if there are any in the options widget
        if (IsValid(DialogueOptions))
        {
            for (int32 i = 0; i < DialogueOptions->GetChildrenCount(); i++)
            {
                UPUDialogueOption* Option = Cast<UPUDialogueOption>(DialogueOptions->GetChildAt(i));
                if (IsValid(Option))
                {
                    Option->SetParentDialogueBox(this);
                    Option->Update(ActiveContext);
                }
            }
        }

        // Check if dialogue has ended
        if (ActiveContext->HasDialogueEnded())
        {
            //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::Update - Dialogue has ended, closing dialogue box"));
            
            // Get the player character and find the current talking object
            if (ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0))
            {
                if (AProjectUmeowmiCharacter* ProjectCharacter = Cast<AProjectUmeowmiCharacter>(PlayerCharacter))
                {
                    if (ATalkingObject* TalkingObject = ProjectCharacter->GetCurrentTalkingObject())
                    {
                        //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::Update - Ending interaction with talking object"));
                        TalkingObject->EndInteraction();
                    }
                }
            }

            // Clear the context reference before closing
            CurrentContext = nullptr;
            
            if (GetVisibility() != ESlateVisibility::Hidden)
            {
                Close();
            }
        }
    }
    else
    {
        //UE_LOG(LogTemp,Error, TEXT("PUDialogueBox::Update_Implementation called with invalid context"));
        
        // Clear the context reference
        CurrentContext = nullptr;
        
        if (GetVisibility() != ESlateVisibility::Hidden)
        {
            Close();
        }
    }
}

UCameraComponent* UPUDialogueBox::GetPlayerCamera() const
{
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (APawn* Pawn = PC->GetPawn())
            {
                if (AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(Pawn))
                {
                    return Character->GetFollowCamera();
                }
            }
        }
    }
    return nullptr;
}

void UPUDialogueBox::InitializeVignetteMaterial()
{
    //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::InitializeVignetteMaterial - Called"));
    
    // Check if material is set (try both soft pointer and direct pointer)
    UMaterialInterface* BaseMaterial = nullptr;
    
    // First try direct reference (more reliable for Blueprint)
    if (VignetteMaterialDirect)
    {
        BaseMaterial = VignetteMaterialDirect;
        //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::InitializeVignetteMaterial - Using direct material reference: %s (Type: %s)"), 
        //    *BaseMaterial->GetName(), *BaseMaterial->GetClass()->GetName());
    }
    // Then try to load from soft pointer
    else if (VignetteMaterial.IsValid())
    {
        BaseMaterial = VignetteMaterial.LoadSynchronous();
        //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::InitializeVignetteMaterial - VignetteMaterial soft pointer is valid, loading: %s"), *VignetteMaterial.ToString());
    }
    else
    {
        // Try to load even if IsValid() returns false (sometimes the path exists but IsValid() fails)
        if (!VignetteMaterial.ToSoftObjectPath().IsNull())
        {
            //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::InitializeVignetteMaterial - Soft pointer path exists, attempting to load: %s"), *VignetteMaterial.ToSoftObjectPath().ToString());
            BaseMaterial = VignetteMaterial.LoadSynchronous();
        }
    }
    
    if (!BaseMaterial)
    {
        //UE_LOG(LogTemp,Error, TEXT("PUDialogueBox::InitializeVignetteMaterial - VignetteMaterial is not set or could not be loaded!"));
        //UE_LOG(LogTemp,Error, TEXT("  - Direct reference (VignetteMaterialDirect): %s"), 
        //    VignetteMaterialDirect ? *VignetteMaterialDirect->GetName() : TEXT("NULL"));
        //UE_LOG(LogTemp,Error, TEXT("  - Soft pointer path (VignetteMaterial): %s"), 
        //    VignetteMaterial.ToSoftObjectPath().IsNull() ? TEXT("NULL") : *VignetteMaterial.ToSoftObjectPath().ToString());
        //UE_LOG(LogTemp,Error, TEXT("  - INSTRUCTIONS:"));
        //UE_LOG(LogTemp,Error, TEXT("    1. Open your PUDialogueBox Blueprint"));
        //UE_LOG(LogTemp,Error, TEXT("    2. In Details panel, find 'Vignette|Settings' category"));
        //UE_LOG(LogTemp,Error, TEXT("    3. Set 'Vignette Material Direct' to your Material Instance"));
        //UE_LOG(LogTemp,Error, TEXT("    4. Recompile and test again"));
        return;
    }
    
    if (VignetteDynamicMaterial)
    {
        //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::InitializeVignetteMaterial - Dynamic material already exists, skipping initialization"));
        return;
    }
    
    //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::InitializeVignetteMaterial - Base material loaded successfully: %s (Type: %s)"), 
    //    *BaseMaterial->GetName(), *BaseMaterial->GetClass()->GetName());

    // Get the player's camera
    UCameraComponent* PlayerCamera = GetPlayerCamera();
    if (!PlayerCamera)
    {
        //UE_LOG(LogTemp,Warning, TEXT("PUDialogueBox::InitializeVignetteMaterial - Could not find player camera"));
        return;
    }

    // Create dynamic material instance
    //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::InitializeVignetteMaterial - Creating dynamic material instance from: %s"), *BaseMaterial->GetName());
    VignetteDynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
    if (!VignetteDynamicMaterial)
    {
        //UE_LOG(LogTemp,Warning, TEXT("PUDialogueBox::InitializeVignetteMaterial - Failed to create dynamic material instance"));
        return;
    }

    //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::InitializeVignetteMaterial - Dynamic material instance created successfully"));
    
    // Set initial intensity to 0
    CurrentVignetteIntensity = 0.0f;
    //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::InitializeVignetteMaterial - Setting initial intensity parameter '%s' to 0.0"), *VignetteIntensityParameterName.ToString());
    VignetteDynamicMaterial->SetScalarParameterValue(VignetteIntensityParameterName, 0.0f);

    // Check if the blendable is already in the array to avoid duplicates
    bool bAlreadyAdded = false;
    for (const FWeightedBlendable& ExistingBlendable : PlayerCamera->PostProcessSettings.WeightedBlendables.Array)
    {
        if (ExistingBlendable.Object == VignetteDynamicMaterial)
        {
            bAlreadyAdded = true;
            break;
        }
    }

    // Add the material to the camera's post process blendables if not already added
    // Note: This uses WeightedBlendables.Array, which is the runtime equivalent of the 
    // "Post Process Materials" array visible in the editor under Camera Settings -> Render Features
    if (!bAlreadyAdded)
    {
        FWeightedBlendable Blendable;
        Blendable.Object = VignetteDynamicMaterial;
        Blendable.Weight = 1.0f;
        
        PlayerCamera->PostProcessSettings.WeightedBlendables.Array.Add(Blendable);
        //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::InitializeVignetteMaterial - ✓ Vignette material initialized and added to camera post process blendables (Camera: %s, Blendables count: %d)"), 
        //    *PlayerCamera->GetName(), PlayerCamera->PostProcessSettings.WeightedBlendables.Array.Num());
    }
    else
    {
        //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::InitializeVignetteMaterial - Vignette material already added to camera post process blendables"));
    }
}

void UPUDialogueBox::AnimateVignetteToTarget(float TargetIntensity)
{
    //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::AnimateVignetteToTarget - Called with target intensity: %.2f"), TargetIntensity);
    
    // Make sure we have the material initialized
    if (!VignetteDynamicMaterial)
    {
        //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::AnimateVignetteToTarget - Material not initialized, initializing now..."));
        InitializeVignetteMaterial();
        if (!VignetteDynamicMaterial)
        {
            //UE_LOG(LogTemp,Error, TEXT("PUDialogueBox::AnimateVignetteToTarget - Failed to initialize vignette material!"));
            return;
        }
    }

    // Set the target intensity
    TargetVignetteIntensity = FMath::Clamp(TargetIntensity, 0.0f, 1.0f);
    
    // Store starting intensity for linear interpolation
    StartVignetteIntensity = CurrentVignetteIntensity;
    VignetteAnimationTime = 0.0f;
    
    // Determine which fade duration to use based on direction and STORE IT
    bool bIsFadingIn = (TargetIntensity > CurrentVignetteIntensity);
    CurrentFadeDuration = bIsFadingIn ? VignetteFadeInDuration : VignetteFadeOutDuration;
    
    //UE_LOG(LogTemp,Warning, TEXT("=== VIGNETTE FADE DEBUG ==="));
    //UE_LOG(LogTemp,Warning, TEXT("Fade Direction: %s"), bIsFadingIn ? TEXT("IN") : TEXT("OUT"));
    //UE_LOG(LogTemp,Warning, TEXT("VignetteFadeInDuration (from Blueprint): %.6f"), VignetteFadeInDuration);
    //UE_LOG(LogTemp,Warning, TEXT("VignetteFadeOutDuration (from Blueprint): %.6f"), VignetteFadeOutDuration);
    //UE_LOG(LogTemp,Warning, TEXT("CurrentFadeDuration (being used): %.6f"), CurrentFadeDuration);
    //UE_LOG(LogTemp,Warning, TEXT("Starting fade: %.2f -> %.2f"), StartVignetteIntensity, TargetVignetteIntensity);
    //UE_LOG(LogTemp,Warning, TEXT("==========================="));

    // Clear any existing timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(VignetteAnimationTimer);

        // If we're already at the target, set it immediately
        if (FMath::IsNearlyEqual(CurrentVignetteIntensity, TargetVignetteIntensity, 0.01f))
        {
            CurrentVignetteIntensity = TargetVignetteIntensity;
            VignetteDynamicMaterial->SetScalarParameterValue(VignetteIntensityParameterName, CurrentVignetteIntensity);
            //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::AnimateVignetteToTarget - Already at target, skipping animation"));
            return;
        }

        // If fade duration is 0 or very small, set immediately
        if (CurrentFadeDuration <= 0.001f)
        {
            CurrentVignetteIntensity = TargetVignetteIntensity;
            VignetteDynamicMaterial->SetScalarParameterValue(VignetteIntensityParameterName, CurrentVignetteIntensity);
            //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::AnimateVignetteToTarget - Fade duration is 0, setting immediately"));
            return;
        }

        // Start animating - use both tick and timer as backup
        bVignetteAnimating = true;
        
        // Also set up a timer as backup (in case tick isn't enabled)
        World->GetTimerManager().SetTimer(
            VignetteAnimationTimer,
            this,
            &UPUDialogueBox::UpdateVignetteIntensity,
            0.016f, // Update at ~60fps
            true
        );
        
        //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::AnimateVignetteToTarget - Vignette animation started (using Tick and Timer)"));
    }
}

void UPUDialogueBox::UpdateVignetteIntensity()
{
    // Log first call to verify timer is working
    static bool bFirstCall = true;
    if (bFirstCall)
    {
        //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::UpdateVignetteIntensity - ✓ FIRST CALL - Timer is working!"));
        bFirstCall = false;
    }
    
    if (!VignetteDynamicMaterial)
    {
        //UE_LOG(LogTemp,Warning, TEXT("PUDialogueBox::UpdateVignetteIntensity - VignetteDynamicMaterial is null, clearing timer"));
        // Clear timer if material is invalid
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(VignetteAnimationTimer);
        }
        bFirstCall = true; // Reset for next animation
        return;
    }

    // Get delta time
    float DeltaTime = 0.016f; // Default frame time
    UWorld* World = GetWorld();
    if (!World)
    {
        //UE_LOG(LogTemp,Warning, TEXT("PUDialogueBox::UpdateVignetteIntensity - No world, clearing timer"));
        World->GetTimerManager().ClearTimer(VignetteAnimationTimer);
        bFirstCall = true;
        return;
    }
    
    DeltaTime = World->GetDeltaSeconds();
    
    // Safety check - if delta time is 0 or invalid, skip this frame
    if (DeltaTime <= 0.0f || DeltaTime > 1.0f)
    {
        //UE_LOG(LogTemp,VeryVerbose, TEXT("PUDialogueBox::UpdateVignetteIntensity - Invalid delta time: %.4f, skipping"), DeltaTime);
        return;
    }

    // Accumulate animation time
    VignetteAnimationTime += DeltaTime;

    // Use the stored fade duration (set when animation started)
    // Calculate normalized progress (0.0 to 1.0) based on fade duration
    if (CurrentFadeDuration <= 0.001f)
    {
        // Instant - set to target immediately
        CurrentVignetteIntensity = TargetVignetteIntensity;
        VignetteDynamicMaterial->SetScalarParameterValue(VignetteIntensityParameterName, CurrentVignetteIntensity);
        bVignetteAnimating = false;
        if (World)
        {
            World->GetTimerManager().ClearTimer(VignetteAnimationTimer);
        }
        //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::UpdateVignetteIntensity - Instant fade (duration was 0)"));
        return;
    }
    
    float NormalizedProgress = FMath::Clamp(VignetteAnimationTime / CurrentFadeDuration, 0.0f, 1.0f);

    // Use linear interpolation for smooth, predictable fade
    float PreviousIntensity = CurrentVignetteIntensity;
    CurrentVignetteIntensity = FMath::Lerp(StartVignetteIntensity, TargetVignetteIntensity, NormalizedProgress);

    // Update the material parameter
    VignetteDynamicMaterial->SetScalarParameterValue(VignetteIntensityParameterName, CurrentVignetteIntensity);
    
    // Log every 10th update to avoid spam (roughly once per ~0.16 seconds at 60fps)
    static int32 UpdateCounter = 0;
    if (++UpdateCounter % 10 == 0 || FMath::Abs(PreviousIntensity - CurrentVignetteIntensity) > 0.1f)
    {
        //UE_LOG(LogTemp,VeryVerbose, TEXT("PUDialogueBox::UpdateVignetteIntensity - Time: %.2f/%.2f (%.1f%%), Intensity: %.3f -> %.3f"), 
        //    VignetteAnimationTime, CurrentFadeDuration, NormalizedProgress * 100.0f, PreviousIntensity, CurrentVignetteIntensity);
    }

    // Check if we've reached the target (either by time or by value)
    if (NormalizedProgress >= 1.0f || FMath::IsNearlyEqual(CurrentVignetteIntensity, TargetVignetteIntensity, 0.001f))
    {
        // Ensure we're exactly at the target
        CurrentVignetteIntensity = TargetVignetteIntensity;
        VignetteDynamicMaterial->SetScalarParameterValue(VignetteIntensityParameterName, CurrentVignetteIntensity);

        // Stop animating
        bVignetteAnimating = false;
        
        // Clear timer
        if (World)
        {
            World->GetTimerManager().ClearTimer(VignetteAnimationTimer);
        }

        //UE_LOG(LogTemp,Log, TEXT("PUDialogueBox::UpdateVignetteIntensity - ✓ Vignette animation complete at intensity: %.2f (took %.2f seconds)"), 
        //    CurrentVignetteIntensity, VignetteAnimationTime);
    }
}

