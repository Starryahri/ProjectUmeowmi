#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PUDialogueBox.generated.h"

class UTextBlock;
class UImage;
class UVerticalBox;
class UButton;
class UDlgContext;
class UPUDialogueOption;

/**
 * A dialogue box widget that can be used to display conversations and text in the game.
 */
UCLASS()
class PROJECTUMEOWMI_API UPUDialogueBox : public UUserWidget
{
    GENERATED_BODY()

public:
    UPUDialogueBox(const FObjectInitializer& ObjectInitializer);

    /** The current active dialogue context */
    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    UDlgContext* CurrentContext;

    /** Widget to display the speaker's name */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* ParticipantNameText;

    /** Widget to display the dialogue text */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* DialogueText;

    /** Widget to display the participant's image */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UImage* ParticipantImage;

    /** Widget to contain the dialogue options */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UVerticalBox* DialogueOptions;

    /** Called when the widget is constructed */
    virtual void NativeConstruct() override;

    /** Event called when the dialogue box is opened */
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void Open(UDlgContext* ActiveContext);

    /** Event called when the dialogue box is closed */
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void Close();

    /** Event called when the dialogue box content is updated */
    UFUNCTION(BlueprintCallable, Category = "Dialogue")
    void Update(UDlgContext* ActiveContext);

    // Implementation functions
    virtual void Open_Implementation(UDlgContext* ActiveContext);
    virtual void Close_Implementation();
    virtual void Update_Implementation(UDlgContext* ActiveContext);

private:
    // No timer handles needed
};