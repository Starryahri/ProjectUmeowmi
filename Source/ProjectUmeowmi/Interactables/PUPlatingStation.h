#pragma once

#include "CoreMinimal.h"
#include "../Dialogue/TalkingObject.h"
#include "../DishCustomization/PUDishCustomizationComponent.h"
#include "../DishCustomization/PUDishBase.h"
#include "../DishCustomization/PUOrderBase.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "PUPlatingStation.generated.h"

UCLASS()
class PROJECTUMEOWMI_API APUPlatingStation : public ATalkingObject
{
    GENERATED_BODY()

public:
    APUPlatingStation();

    // ATalkingObject overrides
    virtual void StartInteraction() override;
    virtual void EndInteraction() override;

protected:
    virtual void BeginPlay() override;

    // Plating Station specific components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* StationMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* InteractionBox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UPUDishCustomizationComponent* PlatingComponent;

    // Plating widget class
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Plating|UI")
    TSubclassOf<class UPUPlatingWidget> PlatingWidgetClass;

    // 3D Scene Components for plating
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* DishMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* IngredientSpawnPoint;

    // Plating Station specific properties
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FText StationName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FText StationDescription;

    // Data tables for plating (same as cooking station)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Tables")
    UDataTable* DishDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Tables")
    UDataTable* IngredientDataTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data Tables")
    UDataTable* PreparationDataTable;

    // Helper functions
    UFUNCTION()
    void OnPlatingEnded();

    // Plating Station specific dialogue methods
    UFUNCTION(BlueprintCallable, Category = "Plating Station|Dialogue")
    void StartNoOrderDialogue();

    // Override dialogue participant methods for plating station specific logic
    virtual bool CheckCondition_Implementation(const UDlgContext* Context, FName ConditionName) const override;
    virtual bool OnDialogueEvent_Implementation(UDlgContext* Context, FName EventName) override;
}; 