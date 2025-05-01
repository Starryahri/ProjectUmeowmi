#pragma once

#include "CoreMinimal.h"
#include "PUCommonUserWidget.h"
#include "PUDialogueBox.generated.h"

class UDlgContext;
class UTextBlock;
class UImage;
class UVerticalBox;

/**
 * A dialogue box widget that can be used to display conversations and text in the game.
 */
UCLASS(Blueprintable, BlueprintType)
class PROJECTUMEOWMI_API UPUDialogueBox : public UPUCommonUserWidget
{
    GENERATED_BODY()

public:
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
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue")
    void Open(UDlgContext* ActiveContext);
    virtual void Open_Implementation(UDlgContext* ActiveContext);

    /** Event called when the dialogue box is closed */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue")
    void Close();
    virtual void Close_Implementation();

    /** Event called when the dialogue box content is updated */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue")
    void Update(UDlgContext* ActiveContext);
    virtual void Update_Implementation(UDlgContext* ActiveContext);
};