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
    
    CurrentContext = ActiveContext;
    SetVisibility(ESlateVisibility::Visible);

    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->bShowMouseCursor = true;
        PC->SetInputMode(FInputModeUIOnly());
    }

    Update(ActiveContext);
}

void UPUDialogueBox::Close_Implementation()
{
    UE_LOG(LogTemp, Log, TEXT("PUDialogueBox::Close_Implementation called"));
    
    SetVisibility(ESlateVisibility::Hidden);

    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
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
                    Option->Update(ActiveContext);
                }
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("PUDialogueBox::Update_Implementation called with invalid context"));
        Close();
    }
}

