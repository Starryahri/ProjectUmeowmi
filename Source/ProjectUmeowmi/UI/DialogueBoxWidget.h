#pragma once

#include "CoreMinimal.h"
#include "PUCommonUserWidget.h"
#include "Components/TextBlock.h"
#include "DialogueBoxWidget.generated.h"

/**
 * Widget class for displaying dialogue boxes using Common UI
 */
UCLASS()
class PROJECTUMEOWMI_API UDialogueBoxWidget : public UPUCommonUserWidget
{
    GENERATED_BODY()

public:
    /** Updates the dialogue box with new text */
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void Update(const FText& DialogueText);

protected:
    virtual void NativeConstruct() override;

    /** The text block displaying the dialogue text */
    UPROPERTY(meta = (BindWidget))
    UTextBlock* DialogueTextBlock;
}; 