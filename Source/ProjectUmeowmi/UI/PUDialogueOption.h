// Copyright 2025 Century Egg Studios, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "ProjectUmeowmi/UI/PUCommonUserWidget.h"
#include "PUDialogueOption.generated.h"

class UDlgContext;
class UTextBlock;
class UButton;
class UPUDialogueBox;

/**
 * Widget for displaying and handling dialogue options
 */
UCLASS()
class PROJECTUMEOWMI_API UPUDialogueOption : public UPUCommonUserWidget
{
	GENERATED_BODY()

public:
	/** The index of this option in the dialogue */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	int32 OptionIndex = 0;

	/** Widget to display the option text */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget), Category = "Dialogue")
	UTextBlock* OptionText;

	/** Button to handle option selection */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BindWidget), Category = "Dialogue")
	UButton* OptionButton;

	/** Current dialogue context */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	UDlgContext* CurrentContext;

	/** Parent dialogue box */
	UPROPERTY()
	UPUDialogueBox* ParentDialogueBox;

	/** Called when the widget is constructed */
	virtual void NativeConstruct() override;

	/** Event called when the dialogue box content is updated */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue")
	void Update(UDlgContext* ActiveContext);
	virtual void Update_Implementation(UDlgContext* ActiveContext);

	/** Select this dialogue option */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void SelectOption();

	/** Set the parent dialogue box */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void SetParentDialogueBox(UPUDialogueBox* DialogueBox);

protected:
	/** Handle button click event */
	UFUNCTION()
	void OnOptionClicked();
};
