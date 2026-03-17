#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "PUDishBase.h"
#include "PUDishPreviewComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDishPreview, Log, All);

class UStaticMesh;
class UStaticMeshComponent;
class APUIngredientMesh;

/**
 * Displays a 3D clone of a plated dish above the character (or wherever attached).
 * Uses plating transforms from dish data - no physics, just visual placement.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTUMEOWMI_API UPUDishPreviewComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UPUDishPreviewComponent();

    /** Initialize with an externally-created DishMeshComponent (avoids template/instance mismatch in Blueprint subclasses). Call from owning Actor's constructor. */
    void SetDishMeshComponent(UStaticMeshComponent* InDishMesh);

    /** Build the preview from dish data. Clears any existing preview first. */
    UFUNCTION(BlueprintCallable, Category = "Dish Preview")
    void BuildFromDishData(const FPUDishBase& DishData);

    /** Clear the preview (remove dish and ingredients). */
    UFUNCTION(BlueprintCallable, Category = "Dish Preview")
    void ClearPreview();

    /** Whether a preview is currently displayed. */
    UFUNCTION(BlueprintCallable, Category = "Dish Preview")
    bool HasPreview() const { return bHasPreview; }

    /** Scale factor for the preview (e.g. 0.2 for above-head display). Tweak in Blueprint if too small/large. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Preview", meta = (ClampMin = "0.01", ClampMax = "2.0"))
    float PreviewScale = 0.2f;

    /** Height offset above character root for dish and ingredients (Z in character local space). Tweak in Blueprint if too high/low. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Preview", meta = (ClampMin = "0.0", ClampMax = "300.0"))
    float OffsetAboveHeadZ = 90.0f;

    /** Small Z offset added to ingredients so they sit on the dish surface (preserves relative Z from plating, moves all up slightly). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Preview")
    float IngredientZOffset = 5.0f;

    /** Fallback dish mesh when dish data has none (e.g. data table row missing DishMesh). Set in character Blueprint. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Preview")
    TSoftObjectPtr<UStaticMesh> DefaultDishMesh;

    /** When true, shows on-screen debug messages for dish preview (BuildFromDishData, ClearPreview, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Preview")
    bool bEnableDishPreviewDebug = false;

    /** Blueprint class for ingredient meshes (uses APUIngredientMesh if not set). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dish Preview")
    TSubclassOf<APUIngredientMesh> IngredientMeshClass;

protected:
    /** Dish mesh - created by owning Actor to avoid template/instance attachment mismatch in Blueprint subclasses. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Dish Preview", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UStaticMeshComponent> DishMeshComponent;

    UPROPERTY()
    TArray<APUIngredientMesh*> PreviewIngredientMeshes;

    UPROPERTY()
    bool bHasPreview = false;
};
