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
}

void UPUDialogueBox::Open_Implementation(UDlgContext* ActiveContext)
{
    UE_LOG(LogTemp, Log, TEXT("PUDialogueBox::Open_Implementation called"));
    // Debug logging for widget state
    UE_LOG(LogTemp, Log, TEXT("DialogueBox pointer: %p"), this);
    UE_LOG(LogTemp, Log, TEXT("Current visibility: %d"), (int32)GetVisibility());
    UE_LOG(LogTemp, Log, TEXT("Is in viewport: %d"), IsInViewport());
    UE_LOG(LogTemp, Log, TEXT("Parent widget: %p"), GetParent());

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
        UE_LOG(LogTemp, Log, TEXT("PlayerController found: %p"), PC);
        
        // Get the local player
        ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
        if (!LocalPlayer)
        {
            UE_LOG(LogTemp, Error, TEXT("PUDialogueBox::Open_Implementation - No local player found!"));
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
            
            // Wait a frame before setting focus to ensure everything is properly initialized
            FTimerHandle TimerHandle;
            GetWorld()->GetTimerManager().SetTimerForNextTick([this, PC, LocalPlayer]()
            {
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
            });
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("PUDialogueBox::Open_Implementation - No game viewport found!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No PlayerController found!"));
    }

    Update(ActiveContext);
}

void UPUDialogueBox::Close_Implementation()
{
    UE_LOG(LogTemp, Log, TEXT("PUDialogueBox::Close_Implementation called"));
    UE_LOG(LogTemp, Log, TEXT("Current visibility state: %d"), (int32)GetVisibility());
    
    SetVisibility(ESlateVisibility::Hidden);
    UE_LOG(LogTemp, Log, TEXT("Visibility set to hidden. New visibility state: %d"), (int32)GetVisibility());

    // Try to get the player controller from the widget owner first
    APlayerController* PC = GetOwningPlayer();
    
    // If we don't have a player controller, try to get it from the world
    if (!PC)
    {
        UE_LOG(LogTemp, Log, TEXT("No player controller from widget owner, trying to get from world"));
        if (UWorld* World = GetWorld())
        {
            PC = World->GetFirstPlayerController();
        }
    }

    if (PC)
    {
        UE_LOG(LogTemp, Log, TEXT("Found player controller: %p"), PC);
        
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
        UE_LOG(LogTemp, Warning, TEXT("Could not find player controller!"));
    }
}

void UPUDialogueBox::Update_Implementation(UDlgContext* ActiveContext)
{
    UE_LOG(LogTemp, Log, TEXT("PUDialogueBox::Update_Implementation called"));
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
            UE_LOG(LogTemp, Log, TEXT("PUDialogueBox::Update - Dialogue has ended, closing dialogue box"));
            
            // Get the player character and find the current talking object
            if (ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0))
            {
                if (AProjectUmeowmiCharacter* ProjectCharacter = Cast<AProjectUmeowmiCharacter>(PlayerCharacter))
                {
                    if (ATalkingObject* TalkingObject = ProjectCharacter->GetCurrentTalkingObject())
                    {
                        UE_LOG(LogTemp, Log, TEXT("PUDialogueBox::Update - Ending interaction with talking object"));
                        TalkingObject->EndInteraction();
                    }
                }
            }

            if (GetVisibility() != ESlateVisibility::Hidden)
            {
                Close();
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("PUDialogueBox::Update_Implementation called with invalid context"));
        if (GetVisibility() != ESlateVisibility::Hidden)
        {
            Close();
        }
    }
}

