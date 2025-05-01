// Copyright 2025 Century Egg Studios, All rights reserved

#pragma once

#include "CoreMinimal.h"
#include "ProjectUmeowmi/UI/PUCommonUserWidget.h"
#include "PUDialogueOption.generated.h"

class UDlgContext;
class UTextBlock;
/**
 * 
 */
UCLASS()
class PROJECTUMEOWMI_API UPUDialogueOption : public UPUCommonUserWidget
{
	GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Dialogue")
    int32 OptionIndex = 0;

    UPROPERTY(EditAnywhere, meta = (BindWidget))
    UTextBlock* OptionText;

public:
    /** Event called when the dialogue box content is updated */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue")
    void Update(UDlgContext* ActiveContext);
    virtual void Update_Implementation(UDlgContext* ActiveContext);
};
