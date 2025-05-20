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
class UInputMappingContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCustomizationEnded);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTUMEOWMI_API UPUDishCustomizationComponent : public USceneComponent
{
    GENERATED_BODY()

public:    
    UPUDishCustomizationComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Activation/Deactivation
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    void StartCustomization(AProjectUmeowmiCharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    void EndCustomization();

    // Check if currently customizing
    UFUNCTION(BlueprintCallable, Category = "Dish Customization")
    bool IsCustomizing() const { return CurrentCharacter != nullptr; }

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Dish Customization")
    FOnCustomizationEnded OnCustomizationEnded;

    // UI Management
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization")
    TSubclassOf<UUserWidget> CustomizationWidgetClass;

    // Input Actions
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization")
    class UInputAction* ExitCustomizationAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization")
    class UInputAction* ControllerMouseAction;

    // Input Mapping Context for customization mode
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization")
    class UInputMappingContext* CustomizationMappingContext;

    // Controller Mouse Settings
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Controller Mouse")
    float ControllerMouseSensitivity = 50.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Controller Mouse")
    float ControllerMouseDeadzone = 0.2f;

    // Camera Management
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Camera")
    float CustomizationCameraDistance = 200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Camera")
    float CustomizationCameraPitch = -25.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Camera")
    float CameraTransitionSpeed = 2.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Customization|Camera")
    float CustomizationOrthoWidth = 500.0f;

    // Current dish being customized
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Customization|Data")
    FPUDishBase CurrentDishData;

protected:
    // Internal state management
    UPROPERTY()
    UUserWidget* CustomizationWidget;

    UPROPERTY()
    AProjectUmeowmiCharacter* CurrentCharacter;

    // Input context management
    UPROPERTY()
    UInputMappingContext* OriginalMappingContext;

    // Input binding handles
    uint32 ExitActionBindingHandle;
    uint32 ControllerMouseBindingHandle;

    // Camera transition state
    bool bIsTransitioningCamera = false;
    float OriginalCameraDistance = 0.0f;
    float OriginalCameraPitch = 0.0f;
    float OriginalCameraYaw = 0.0f;
    float OriginalOrthoWidth = 0.0f;
    float OriginalCameraOffset = 0.0f;
    int32 OriginalCameraPositionIndex = 0;
    float TargetCameraDistance = 0.0f;
    float TargetCameraPitch = 0.0f;
    float TargetCameraYaw = 0.0f;
    float TargetOrthoWidth = 0.0f;
    float TargetCameraOffset = 0.0f;
    int32 TargetCameraPositionIndex = 0;

    // Input handling
    void HandleExitInput();
    void HandleControllerMouse(const FInputActionValue& Value);

    // Camera handling
    void StartCameraTransition(bool bToCustomization);
    void UpdateCameraTransition(float DeltaTime);
}; 