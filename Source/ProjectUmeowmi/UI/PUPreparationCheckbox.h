#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "PUPreparationCheckbox.generated.h"

class UCheckBox;
class UTextBlock;
class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPreparationCheckboxChanged, const FGameplayTag&, PreparationTag, bool, bIsChecked);

UCLASS(BlueprintType, Blueprintable)
class PROJECTUMEOWMI_API UPUPreparationCheckbox : public UUserWidget
{
    GENERATED_BODY()

public:
    UPUPreparationCheckbox(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // Set the preparation data for this checkbox
    UFUNCTION(BlueprintCallable, Category = "Preparation Checkbox")
    void SetPreparationData(const FGameplayTag& InPreparationTag, const FText& InDisplayName, const FText& InDescription, UTexture2D* InPreviewTexture);

    // Get the preparation tag
    UFUNCTION(BlueprintCallable, Category = "Preparation Checkbox")
    const FGameplayTag& GetPreparationTag() const { return PreparationTag; }

    // Set the checked state
    UFUNCTION(BlueprintCallable, Category = "Preparation Checkbox")
    void SetChecked(bool bIsChecked);

    // Get the current checked state
    UFUNCTION(BlueprintCallable, Category = "Preparation Checkbox")
    bool IsChecked() const;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Preparation Checkbox|Events")
    FOnPreparationCheckboxChanged OnPreparationCheckboxChanged;

protected:
    // Preparation data
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preparation Data")
    FGameplayTag PreparationTag;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preparation Data")
    FText DisplayName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preparation Data")
    FText Description;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preparation Data")
    UTexture2D* PreviewTexture;

    // UI Components (will be bound in Blueprint)
    UPROPERTY(meta = (BindWidget))
    UCheckBox* PreparationCheckBox;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* PreparationNameText;

    UPROPERTY(meta = (BindWidget))
    UImage* PreparationIcon;

    // Get UI components (Blueprint accessible)
    UFUNCTION(BlueprintCallable, Category = "Preparation Checkbox|Components")
    UCheckBox* GetPreparationCheckBox() const { return PreparationCheckBox; }

    UFUNCTION(BlueprintCallable, Category = "Preparation Checkbox|Components")
    UTextBlock* GetPreparationNameText() const { return PreparationNameText; }

    UFUNCTION(BlueprintCallable, Category = "Preparation Checkbox|Components")
    UImage* GetPreparationIcon() const { return PreparationIcon; }

    // Blueprint events that can be overridden
    UFUNCTION(BlueprintImplementableEvent, Category = "Preparation Checkbox")
    void OnPreparationDataSet(const FGameplayTag& InPreparationTag, const FText& InDisplayName, const FText& InDescription);

    UFUNCTION(BlueprintImplementableEvent, Category = "Preparation Checkbox")
    void OnCheckedStateChanged(bool bIsChecked);

private:
    // Checkbox event handler
    UFUNCTION()
    void OnCheckBoxChanged(bool bIsChecked);

    // Helper functions
    void UpdateUI();
}; 