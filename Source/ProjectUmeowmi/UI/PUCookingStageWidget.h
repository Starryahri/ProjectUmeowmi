#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "../DishCustomization/PUDishBase.h"
#include "PUIngredientQuantityControl.h"
#include "PUPreparationCheckbox.h"
#include "PUCookingStageWidget.generated.h"

class UPUIngredientQuantityControl;
class AStaticMeshActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCookingCompleted, const FPUDishBase&, FinalDishData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCookingImplementSelected, int32, ImplementIndex);

UCLASS(BlueprintType, Blueprintable)
class PROJECTUMEOWMI_API UPUCookingStageWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPUCookingStageWidget(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // Initialize the cooking stage with dish data
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void InitializeCookingStage(const FPUDishBase& DishData, const FVector& CookingStationLocation = FVector::ZeroVector);

    // Get the current dish data
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    const FPUDishBase& GetCurrentDishData() const { return CurrentDishData; }

    // Get the planning data
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    const FPUPlanningData& GetPlanningData() const { return PlanningData; }

    // Finish cooking and transition to plating
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void FinishCookingAndStartPlating();

    // Carousel Functions
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Carousel")
    void SpawnCookingCarousel();

    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Carousel")
    void DestroyCookingCarousel();

    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Carousel")
    void SelectCookingImplement(int32 ImplementIndex);

    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Carousel")
    int32 GetSelectedImplementIndex() const { return SelectedImplementIndex; }

    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Carousel")
    void SetCarouselCenter(const FVector& NewCenter);

    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Carousel")
    void SetCookingStationLocation(const FVector& StationLocation);

    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget|Carousel")
    void FindAndSetNearestCookingStation();

    // Blueprint events
    UFUNCTION(BlueprintImplementableEvent, Category = "Cooking Stage Widget")
    void OnCookingStageInitialized(const FPUDishBase& DishData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Cooking Stage Widget")
    void OnCookingStageCompleted(const FPUDishBase& FinalDishData);

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Cooking Stage Widget|Events")
    FOnCookingCompleted OnCookingCompleted;

    UPROPERTY(BlueprintAssignable, Category = "Cooking Stage Widget|Events")
    FOnCookingImplementSelected OnCookingImplementSelected;

protected:
    // Current dish data (being built during cooking)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cooking Data")
    FPUDishBase CurrentDishData;

    // Planning data from the planning stage
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Planning Data")
    FPUPlanningData PlanningData;

    // Widget class references
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes")
    TSubclassOf<UPUIngredientQuantityControl> QuantityControlClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Classes")
    TSubclassOf<UPUPreparationCheckbox> PreparationCheckboxClass;

    // Carousel Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Carousel")
    TArray<class UStaticMesh*> CookingImplementMeshes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Carousel")
    TArray<FString> CookingImplementNames;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Carousel")
    FVector CarouselCenter = FVector(0.0f, 0.0f, 100.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Carousel")
    float CarouselRadius = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Carousel")
    float CarouselHeight = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Carousel")
    float RotationSpeed = 2.0f;

    // Hover Detection Properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Hover")
    TSubclassOf<UUserWidget> HoverTextWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Hover")
    float HoverTextOffset = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Hover")
    FLinearColor HoverGlowColor = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Hover")
    float HoverGlowIntensity = 2.0f;

    // Front indicator properties
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Carousel")
    float FrontItemScale = 1.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking Stage Widget|Carousel")
    float FrontItemHeight = 50.0f;

private:
    // Create quantity controls for selected ingredients
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void CreateQuantityControlsForSelectedIngredients();

    // Handle quantity control changes
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void OnQuantityControlChanged(const FIngredientInstance& IngredientInstance);

    // Handle quantity control removal
    UFUNCTION(BlueprintCallable, Category = "Cooking Stage Widget")
    void OnQuantityControlRemoved(int32 InstanceID, class UPUIngredientQuantityControl* QuantityControlWidget);

    // Carousel private functions
    void PositionCookingImplements();
    void RotateCarouselToSelection(int32 TargetIndex);
    FVector CalculateImplementPosition(int32 Index, int32 TotalCount) const;
    FRotator CalculateImplementRotation(int32 Index, int32 TotalCount) const;

    // Hover detection functions
    void SetupHoverDetection();
    UFUNCTION()
    void OnImplementHoverBegin(UPrimitiveComponent* TouchedComponent);
    UFUNCTION()
    void OnImplementHoverEnd(UPrimitiveComponent* TouchedComponent);
    void ShowHoverText(int32 ImplementIndex, bool bShow);
    void ApplyHoverVisualEffect(int32 ImplementIndex, bool bApply);
    int32 GetImplementIndexFromComponent(UPrimitiveComponent* Component) const;

    // Click detection functions
    void SetupClickDetection();
    UFUNCTION()
    void OnImplementClicked(UPrimitiveComponent* ClickedComponent, FKey ButtonPressed);
    void UpdateCarouselRotation(float DeltaTime);
    void ApplySelectionVisualEffect(int32 ImplementIndex, bool bApply);
    void UpdateFrontIndicator();
    int32 GetFrontImplementIndex() const;
    void DebugCollisionDetection();
    void SetupMultiHitCollisionDetection();
    void OnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent);
    void UpdateHoverDetection();
    bool IsPointInCarouselBounds(const FVector2D& ScreenPoint) const;
    void HandleMouseClick();
    bool IsMouseOverImplement(int32 ImplementIndex) const;

    // Carousel state
    UPROPERTY()
    TArray<AStaticMeshActor*> SpawnedCookingImplements;

    UPROPERTY()
    TArray<UWidgetComponent*> HoverTextComponents;

    UPROPERTY()
    int32 SelectedImplementIndex = 0;

    UPROPERTY()
    int32 HoveredImplementIndex = -1;

    UPROPERTY()
    bool bCarouselSpawned = false;

    // Carousel rotation state
    UPROPERTY()
    bool bIsRotating = false;

    UPROPERTY()
    float CurrentRotationAngle = 0.0f;

    UPROPERTY()
    float TargetRotationAngle = 0.0f;

    UPROPERTY()
    float RotationProgress = 0.0f;
}; 