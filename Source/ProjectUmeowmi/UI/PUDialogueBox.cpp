#include "PUDialogueBox.h"
#include "PUDialogueOption.h"
#include "DlgSystem/DlgContext.h"
#include "GameFramework/PlayerController.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"

void UPUDialogueBox::NativeConstruct()
{
    Super::NativeConstruct();
    SetVisibility(ESlateVisibility::Hidden);
}

void UPUDialogueBox::Open_Implementation(UDlgContext* ActiveContext)
{
    UE_LOG(LogTemp, Log, TEXT("PUDialogueBox::Open_Implementation called"));
    
    // Debug logging for widget state
    UE_LOG(LogTemp, Log, TEXT("DialogueBox pointer: %p"), this);
    UE_LOG(LogTemp, Log, TEXT("Current visibility: %d"), (int32)GetVisibility());
    UE_LOG(LogTemp, Log, TEXT("Is in viewport: %d"), IsInViewport());
    UE_LOG(LogTemp, Log, TEXT("Parent widget: %p"), GetParent());

    SetVisibility(ESlateVisibility::Visible);

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
        UE_LOG(LogTemp, Log, TEXT("Current mouse cursor visibility: %d"), PC->bShowMouseCursor);
        
        PC->bShowMouseCursor = true;
        PC->SetInputMode(FInputModeUIOnly());
        
        UE_LOG(LogTemp, Log, TEXT("New mouse cursor visibility: %d"), PC->bShowMouseCursor);
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
        UE_LOG(LogTemp, Log, TEXT("Current mouse cursor state: %d"), PC->bShowMouseCursor);
        
        PC->bShowMouseCursor = false;
        UE_LOG(LogTemp, Log, TEXT("Mouse cursor hidden. New state: %d"), PC->bShowMouseCursor);
        
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);
        UE_LOG(LogTemp, Log, TEXT("Input mode set to game only"));
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
        // First check if the widget has children and if it does loop through them
        if (IsValid(DialogueOptions))
        {
            for (int32 i = 0; i < DialogueOptions->GetChildrenCount(); i++)
            {
                UPUDialogueOption* Option = Cast<UPUDialogueOption>(DialogueOptions->GetChildAt(i));
                if (IsValid(Option))
                {
                    // Set this dialogue box as the parent
                    Option->SetParentDialogueBox(this);
                    Option->Update(ActiveContext);
                }
            }
        }

        // Check if dialogue has ended
        if (ActiveContext->HasDialogueEnded())
        {
            UE_LOG(LogTemp, Log, TEXT("PUDialogueBox::Update - Dialogue has ended, closing dialogue box"));
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

