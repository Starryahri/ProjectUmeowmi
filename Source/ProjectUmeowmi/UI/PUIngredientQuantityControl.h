#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../DishCustomization/PUDishBase.h"
#include "PUIngredientQuantityControl.generated.h"

class UButton;
class UTextBlock;
class UImage;
class UScrollBox;
class UCheckBox;
class USpinBox;
class UPUPreparationCheckbox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuantityControlChanged, const FIngredientInstance&, IngredientInstance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuantityControlRemoved, int32, InstanceID, class UPUIngredientQuantityControl*, QuantityControlWidget);

UCLASS(BlueprintType, Blueprintable)
class PROJECTUMEOWMI_API UPUIngredientQuantityControl : public UUserWidget
{
    GENERATED_BODY()

public:
    UPUIngredientQuantityControl(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // Set the ingredient instance data for this control
    UFUNCTION(BlueprintCallable, Category = "Quantity Control")
    void SetIngredientInstance(const FIngredientInstance& InIngredientInstance);

    // Set the preparation checkbox class
    UFUNCTION(BlueprintCallable, Category = "Quantity Control")
    void SetPreparationCheckboxClass(TSubclassOf<UPUPreparationCheckbox> InPreparationCheckboxClass);

    // Get the current ingredient instance data
    UFUNCTION(BlueprintCallable, Category = "Quantity Control")
    const FIngredientInstance& GetIngredientInstance() const { return IngredientInstance; }

    // Get the instance ID
    UFUNCTION(BlueprintCallable, Category = "Quantity Control")
    int32 GetInstanceID() const { return IngredientInstance.InstanceID; }

    // Update quantity
    UFUNCTION(BlueprintCallable, Category = "Quantity Control")
    void SetQuantity(int32 NewQuantity);

    // Add/remove preparation
    UFUNCTION(BlueprintCallable, Category = "Quantity Control")
    void AddPreparation(const FGameplayTag& PreparationTag);

    UFUNCTION(BlueprintCallable, Category = "Quantity Control")
    void RemovePreparation(const FGameplayTag& PreparationTag);

    UFUNCTION(BlueprintCallable, Category = "Quantity Control")
    bool HasPreparation(const FGameplayTag& PreparationTag) const;

    // Remove this ingredient instance completely
    UFUNCTION(BlueprintCallable, Category = "Quantity Control")
    void RemoveIngredientInstance();

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Quantity Control|Events")
    FOnQuantityControlChanged OnQuantityControlChanged;

    UPROPERTY(BlueprintAssignable, Category = "Quantity Control|Events")
    FOnQuantityControlRemoved OnQuantityControlRemoved;

protected:
    // Current ingredient instance data
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ingredient Instance")
    FIngredientInstance IngredientInstance;

    // UI Components (will be bound in Blueprint)
    UPROPERTY(meta = (BindWidget))
    UTextBlock* IngredientNameText;

    UPROPERTY(meta = (BindWidget))
    UImage* IngredientIcon;

    UPROPERTY(meta = (BindWidget))
    UButton* DecreaseQuantityButton;



    UPROPERTY(meta = (BindWidget))
    UButton* IncreaseQuantityButton;

    UPROPERTY(meta = (BindWidget))
    UButton* RemoveButton;

    UPROPERTY(meta = (BindWidget))
    UScrollBox* PreparationsScrollBox;

    // Preparation checkbox class reference
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes")
    TSubclassOf<UPUPreparationCheckbox> PreparationCheckboxClass;

    // Get UI components (Blueprint accessible)
    UFUNCTION(BlueprintCallable, Category = "Quantity Control|Components")
    UTextBlock* GetIngredientNameText() const { return IngredientNameText; }

    UFUNCTION(BlueprintCallable, Category = "Quantity Control|Components")
    UImage* GetIngredientIcon() const { return IngredientIcon; }

    UFUNCTION(BlueprintCallable, Category = "Quantity Control|Components")
    UButton* GetDecreaseQuantityButton() const { return DecreaseQuantityButton; }



    UFUNCTION(BlueprintCallable, Category = "Quantity Control|Components")
    UButton* GetIncreaseQuantityButton() const { return IncreaseQuantityButton; }

    UFUNCTION(BlueprintCallable, Category = "Quantity Control|Components")
    UButton* GetRemoveButton() const { return RemoveButton; }

    UFUNCTION(BlueprintCallable, Category = "Quantity Control|Components")
    UScrollBox* GetPreparationsScrollBox() const { return PreparationsScrollBox; }

    // Blueprint events that can be overridden
    UFUNCTION(BlueprintImplementableEvent, Category = "Quantity Control")
    void OnIngredientInstanceSet(const FIngredientInstance& InIngredientInstance);

    UFUNCTION(BlueprintImplementableEvent, Category = "Quantity Control")
    void OnQuantityChanged(int32 NewQuantity);

    UFUNCTION(BlueprintImplementableEvent, Category = "Quantity Control")
    void OnPreparationChanged(const FGameplayTag& PreparationTag, bool bIsAdded);

    UFUNCTION(BlueprintImplementableEvent, Category = "Quantity Control")
    void OnIngredientRemoved();

private:
    // Button event handlers
    UFUNCTION()
    void OnDecreaseQuantityClicked();

    UFUNCTION()
    void OnIncreaseQuantityClicked();

    UFUNCTION()
    void OnRemoveButtonClicked();

    // Preparation checkbox event handler
    UFUNCTION()
    void OnPreparationCheckboxChanged(const FGameplayTag& PreparationTag, bool bIsChecked);

    // Helper functions
    void UpdateQuantityControls();
    void UpdatePreparationCheckboxes();
    void UpdateIngredientDisplay();
    void BroadcastChange();
    
    // Preparation management
    void ClearPreparationCheckboxes();
    void CreatePreparationCheckbox(const FPUPreparationBase& PreparationData, bool bIsCurrentlyApplied);
}; 