#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GameplayTagContainer.h"
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
    /** Generate a new order (dish from AvailableDishTags pool or fallback). */
    UFUNCTION(BlueprintCallable, Category = "Order System")
    void GenerateNewOrder();

    /** Generate a new order for a specific dish tag. Use invalid tag to use pool/random instead. */
    UFUNCTION(BlueprintCallable, Category = "Order System", meta = (DisplayName = "Generate New Order With Dish"))
    void GenerateNewOrderWithDish(FGameplayTag DishTag);

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
    /** Pool of dish tags to choose from when generating orders (random pick if no override tag). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Order System|Generation", meta = (Categories = "Dish"))
    TArray<FGameplayTag> AvailableDishTags;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Order System|Generation")
    int32 DefaultMinIngredients = 3;

    /** One or more aspect requirements (flavor and/or texture). e.g. Salt, Sweet, Umami (flavor), Crispy (texture). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Order System|Generation")
    TArray<FOrderAspectRequirement> DefaultTargetAspects;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Order System|Generation")
    FText DefaultOrderDescription = FText::FromString(TEXT("Make me something with {0} ingredients. I want it {1}."));

    // Data tables for generating orders
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Order System|Generation")
    UDataTable* DishDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Order System|Generation")
    UDataTable* IngredientDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Order System|Generation")
    UDataTable* PreparationDataTable;

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Order System")
    FPUOrderBase CurrentOrder;

    UPROPERTY(BlueprintReadOnly, Category = "Order System")
    bool bHasActiveOrder = false;

    /** OptionalDishTag: if valid, use it; else pick from AvailableDishTags; else fallback to GetRandomDishTag / Congee. */
    void GenerateSimpleOrder(FGameplayTag OptionalDishTag);
}; 