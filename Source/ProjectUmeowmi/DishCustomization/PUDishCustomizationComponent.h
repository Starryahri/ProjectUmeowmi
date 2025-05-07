#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "PUDishBase.h"
#include "PUDishCustomizationComponent.generated.h"

class UUserWidget;
class UInputAction;
class UEnhancedInputComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTUMEOWMI_API UPUDishCustomizationComponent : public USceneComponent
{
    GENERATED_BODY()

public:    
    UPUDishCustomizationComponent();

    virtual void BeginPlay() override;

    // Activation/Deactivation
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    void StartCustomization();

    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    void EndCustomization();

    // UI Management
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization")
    TSubclassOf<UUserWidget> CustomizationWidgetClass;

    // Input Action for exiting customization
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization")
    class UInputAction* ExitCustomizationAction;

    // Camera Management
    UFUNCTION(BlueprintImplementableEvent, Category = "Dish Customization")
    void SetupCustomizationCamera();

    // Current dish being customized
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization")
    FPUDishBase CurrentDishData;

protected:
    // Internal state management
    UPROPERTY()
    UUserWidget* CustomizationWidget;

    // Input handling
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent);
    void HandleExitInput();
}; 