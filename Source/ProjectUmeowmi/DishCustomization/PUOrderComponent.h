#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "PUOrderBase.h"
#include "PUOrderComponent.generated.h"

// Forward declarations
class AProjectUmeowmiCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOrderGenerated, const FPUOrderBase&, NewOrder);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOrderCompleted, const FPUOrderBase&, CompletedOrder);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTUMEOWMI_API UPUOrderComponent : public USceneComponent
{
    GENERATED_BODY()

public:    
    UPUOrderComponent();

    virtual void BeginPlay() override;

    // Order Management
    UFUNCTION(BlueprintCallable, Category = "Order System")
    void GenerateNewOrder();

    UFUNCTION(BlueprintCallable, Category = "Order System")
    void ClearCurrentOrder();

    // Order Access
    UFUNCTION(BlueprintCallable, Category = "Order System")
    const FPUOrderBase& GetCurrentOrder() const { return CurrentOrder; }

    UFUNCTION(BlueprintCallable, Category = "Order System")
    bool HasActiveOrder() const { return bHasActiveOrder; }

    // Order Validation
    UFUNCTION(BlueprintCallable, Category = "Order System")
    bool ValidateDish(const FPUDishBase& Dish) const;

    UFUNCTION(BlueprintCallable, Category = "Order System")
    float GetSatisfactionScore(const FPUDishBase& Dish) const;

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Order System")
    FOnOrderGenerated OnOrderGenerated;

    UPROPERTY(BlueprintAssignable, Category = "Order System")
    FOnOrderCompleted OnOrderCompleted;

    // Order Generation Settings
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Order System|Generation")
    int32 DefaultMinIngredients = 3;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Order System|Generation")
    FName DefaultTargetFlavor = FName(TEXT("Saltiness"));

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Order System|Generation")
    float DefaultMinFlavorValue = 5.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Order System|Generation")
    FText DefaultOrderDescription = FText::FromString(TEXT("Make me congee with {0} ingredients. Make it {1}."));

protected:
    // Current order data
    UPROPERTY(BlueprintReadOnly, Category = "Order System")
    FPUOrderBase CurrentOrder;

    UPROPERTY(BlueprintReadOnly, Category = "Order System")
    bool bHasActiveOrder = false;

    // Order generation
    void GenerateSimpleOrder();
}; 