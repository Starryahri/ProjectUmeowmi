#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "../DishCustomization/PUIngredientBase.h"
#include "PUIngredientButton.generated.h"

class UButton;
class UTextBlock;
class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIngredientButtonClicked, const FPUIngredientBase&, IngredientData);

UCLASS(BlueprintType, Blueprintable)
class PROJECTUMEOWMI_API UPUIngredientButton : public UUserWidget
{
    GENERATED_BODY()

public:
    UPUIngredientButton(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // Set the ingredient data for this button
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button")
    void SetIngredientData(const FPUIngredientBase& InIngredientData);

    // Get the current ingredient data
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button")
    const FPUIngredientBase& GetIngredientData() const { return IngredientData; }

    // Get UI components (Blueprint accessible)
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Components")
    UButton* GetIngredientButton() const { return IngredientButton; }

    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Components")
    UTextBlock* GetIngredientNameText() const { return IngredientNameText; }

    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Components")
    UImage* GetIngredientIcon() const { return IngredientIcon; }

    // Event when button is clicked
    UPROPERTY(BlueprintAssignable, Category = "Ingredient Button|Events")
    FOnIngredientButtonClicked OnIngredientButtonClicked;

    // Events when button is hovered/unhovered
    UPROPERTY(BlueprintAssignable, Category = "Ingredient Button|Events")
    FOnIngredientButtonClicked OnIngredientButtonHovered;

    // Native drag events (only enabled in cooking stage)
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    // Enable/disable drag functionality
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Drag")
    void SetDragEnabled(bool bEnabled);

    // Check if drag is enabled
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Drag")
    bool IsDragEnabled() const { return bDragEnabled; }

    // Generate a unique instance ID for this ingredient
    UFUNCTION(BlueprintCallable, Category = "Ingredient Button|Drag")
    int32 GenerateUniqueInstanceID() const;

    UPROPERTY(BlueprintAssignable, Category = "Ingredient Button|Events")
    FOnIngredientButtonClicked OnIngredientButtonUnhovered;

protected:
    // Current ingredient data
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ingredient Data")
    FPUIngredientBase IngredientData;

    // UI Components (will be bound in Blueprint)
    UPROPERTY(meta = (BindWidget))
    UButton* IngredientButton;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* IngredientNameText;

    UPROPERTY(meta = (BindWidget))
    UImage* IngredientIcon;

    // Whether drag functionality is enabled
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ingredient Button|Drag")
    bool bDragEnabled = false;

    // Blueprint events that can be overridden
    UFUNCTION(BlueprintImplementableEvent, Category = "Ingredient Button")
    void OnIngredientDataSet(const FPUIngredientBase& InIngredientData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Ingredient Button")
    void OnButtonClicked();

    UFUNCTION(BlueprintImplementableEvent, Category = "Ingredient Button")
    void OnButtonHovered();

    UFUNCTION(BlueprintImplementableEvent, Category = "Ingredient Button")
    void OnButtonUnhovered();

private:
    // Button event handlers
    UFUNCTION()
    void OnIngredientButtonClickedInternal();

    UFUNCTION()
    void OnIngredientButtonHoveredInternal();

    UFUNCTION()
    void OnIngredientButtonUnhoveredInternal();
}; 