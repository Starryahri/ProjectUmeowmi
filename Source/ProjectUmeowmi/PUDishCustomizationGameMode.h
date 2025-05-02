#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DishCustomization/PUDishCustomizationData.h"
#include "PUDishCustomizationGameMode.generated.h"

UCLASS()
class PROJECTUMEOWMI_API APUDishCustomizationGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    APUDishCustomizationGameMode();

    virtual void BeginPlay() override;

    // Function to transition to this game mode
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    static void TransitionToDishCustomizationMode();

    // Function to exit dish customization mode
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    void ExitDishCustomizationMode();

protected:
    // Camera setup for dish customization view
    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization")
    void SetupCustomizationCamera();

    // Current dish being customized
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization")
    FDishCustomizationData CurrentDishData;
}; 