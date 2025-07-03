#pragma once

#include "CoreMinimal.h"
#include "PUInteractableBase.h"
#include "../DishCustomization/PUDishCustomizationComponent.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "PUCookingStation.generated.h"

UCLASS()
class PROJECTUMEOWMI_API APUCookingStation : public APUInteractableBase
{
    GENERATED_BODY()

public:
    APUCookingStation();

    // APUInteractableBase overrides
    virtual void StartInteraction() override;
    virtual void EndInteraction() override;
    virtual FText GetInteractionText() const override;
    virtual FText GetInteractionDescription() const override;

protected:
    virtual void BeginPlay() override;

    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* StationMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* InteractionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UPUDishCustomizationComponent* DishCustomizationComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UWidgetComponent* InteractionWidget;

    // Interaction properties
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FText StationName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FText StationDescription;

    // Helper functions
    void OnCustomizationEnded();

    // Order validation
    UFUNCTION(BlueprintCallable, Category = "Cooking Station|Orders")
    bool ValidateDishAgainstOrder(const FPUDishBase& Dish, const FPUOrderBase& Order, float& OutSatisfactionScore) const;

    UFUNCTION(BlueprintCallable, Category = "Cooking Station|Orders")
    float CalculateSatisfactionScore(const FPUDishBase& Dish, const FPUOrderBase& Order) const;

    // Interaction range events
    UFUNCTION()
    void OnInteractionBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnInteractionBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
}; 