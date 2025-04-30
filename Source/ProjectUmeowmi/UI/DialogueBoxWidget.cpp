#include "DialogueBoxWidget.h"
#include "Components/TextBlock.h"

void UDialogueBoxWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UDialogueBoxWidget::Update(const FText& DialogueText)
{
    // Set the dialogue text
    if (DialogueTextBlock)
    {
        DialogueTextBlock->SetText(DialogueText);
    }
} 