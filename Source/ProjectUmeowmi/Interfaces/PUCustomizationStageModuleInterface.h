#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "../DishCustomization/PUDishBase.h"
#include "PUCustomizationStageModuleInterface.generated.h"

class UPUDishCustomizationWidget;
class UPUDishCustomizationComponent;
class UPUIngredientSlot;

/** Optional contract for widgets mounted into `StageModuleSlot` (`FPUDishCustomizationStageDescriptor::StageWidgetClass`). */
UINTERFACE(BlueprintType, MinimalAPI)
class UPUCustomizationStageModuleInterface : public UInterface
{
    GENERATED_BODY()
};

class PROJECTUMEOWMI_API IPUCustomizationStageModuleInterface
{
    GENERATED_BODY()

public:
    /** Called immediately after the widget is parented under `StageModuleSlot`. Cache OwnerShell / CustomizationComponent from here. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dish Customization|Stage Module")
    void InitializeStageModule(
        UPUDishCustomizationWidget* OwnerShell,
        UPUDishCustomizationComponent* CustomizationComponent,
        const FPUDishCustomizationStageDescriptor& StageDescriptor);

    /** Called before the widget is removed (pipeline advance, clear slot, teardown). Drop input subscriptions and timers here. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dish Customization|Stage Module")
    void ShutdownStageModule();

    /**
     * Keyboard/gamepad focus moved along the ingredient rail (or left the rail).
     * StripSlot is nullptr when no rail-focused slot applies (focus left rail, or sync found no rail strip).
     * Blueprint: never read Strip Slot without Is Valid; when invalid, hide/clear vignette previews.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dish Customization|Stage Module")
    void OnIngredientStripSlotFocusChanged(UPUIngredientSlot* StripSlot);

    /**
     * Toggle stage-specific minigame mode (e.g. chopping) for the ingredient strip slot that invoked this.
     * Implement on chop/marinate/etc. modules; return true when handled so strip consumes input.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dish Customization|Stage Module")
    bool ToggleStageMinigameFromIngredientStripSlot(UPUIngredientSlot* StripSlot);

    /**
     * Ingredient rail strip slot filled from pantry/prepped picker (see `CompletePendingStripFillAndClosePantry`).
     * Marination stage uses this to mirror the ingredient into bowl visuals.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dish Customization|Stage Module")
    void OnIngredientAddedToStripSlot(UPUIngredientSlot* StripSlot, const FIngredientInstance& IngredientInstance);
};
