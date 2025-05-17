#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "PUDishBase.h"
#include "../ProjectUmeowmiCharacter.h"
#include "PUDishCustomizationComponent.generated.h"

// Forward declarations
class UUserWidget;
class UInputAction;
class UEnhancedInputComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCustomizationEnded);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTUMEOWMI_API UPUDishCustomizationComponent : public USceneComponent
{
    GENERATED_BODY()

public:    
    UPUDishCustomizationComponent();

    virtual void BeginPlay() override;

    // Activation/Deactivation
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    void StartCustomization(AProjectUmeowmiCharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    void EndCustomization();

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Dish Customization")
    FOnCustomizationEnded OnCustomizationEnded;

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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization|Data")
    FPUDishBase CurrentDishData;

protected:
    // Internal state management
    UPROPERTY()
    UUserWidget* CustomizationWidget;

    UPROPERTY()
    AProjectUmeowmiCharacter* CurrentCharacter;

    // Input handling
    void HandleExitInput();
}; 