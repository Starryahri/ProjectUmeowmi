#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "PUDishBase.h"
#include "PUIngredientBase.h"
#include "PUIngredientMesh.generated.h"

class UProceduralMeshComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIngredientMoved, const FVector&, NewPosition);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIngredientRotated, const FRotator&, NewRotation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnIngredientGrabbed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnIngredientReleased);

UCLASS()
class PROJECTUMEOWMI_API APUIngredientMesh : public AActor
{
    GENERATED_BODY()

public:
    APUIngredientMesh();

    // Called after components are initialized
    virtual void PostInitializeComponents() override;

    virtual void Tick(float DeltaTime) override;

    // Setup the mesh with ingredient data
    UFUNCTION(BlueprintCallable, Category = "Ingredient")
    void InitializeWithIngredient(const FPUIngredientBase& IngredientData);

    // Setup with ingredient data and preparations (handles chopped mesh slicing)
    UFUNCTION(BlueprintCallable, Category = "Ingredient")
    void InitializeWithIngredientInstance(const struct FIngredientInstance& IngredientInstance);

    // Whether this ingredient uses sliced procedural meshes (chopped/minced)
    UFUNCTION(BlueprintCallable, Category = "Ingredient")
    bool IsChopped() const { return bIsChopped; }

    // Apply scale to the ingredient; for chopped/minced, sets scale on each procedural mesh piece (actor scale doesn't propagate correctly)
    UFUNCTION(BlueprintCallable, Category = "Ingredient")
    void SetIngredientScale(const FVector& Scale);

    // Mouse interaction functions
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Interaction")
    void OnMouseHoverBegin(UPrimitiveComponent* TouchedComponent);

    UFUNCTION(BlueprintCallable, Category = "Ingredient|Interaction")
    void OnMouseHoverEnd(UPrimitiveComponent* TouchedComponent);

    // Override actor mouse interaction
    virtual void NotifyActorBeginCursorOver() override;
    virtual void NotifyActorEndCursorOver() override;
    virtual void NotifyActorOnClicked(FKey ButtonPressed) override;

    // Debug function to test mouse interaction
    UFUNCTION(BlueprintCallable, Category = "Ingredient|Debug")
    void TestMouseInteraction();

    UFUNCTION(BlueprintCallable, Category = "Ingredient|Interaction")
    void OnMouseGrab();

    UFUNCTION(BlueprintCallable, Category = "Ingredient|Interaction")
    void OnMouseRelease();

    UFUNCTION(BlueprintCallable, Category = "Ingredient|Interaction")
    void UpdatePosition(const FVector& NewPosition);

    UFUNCTION(BlueprintCallable, Category = "Ingredient|Interaction")
    void UpdateRotation(const FRotator& NewRotation);

    /** InstanceID from dish data when used for plating - used to capture final transform before cleanup. */
    UFUNCTION(BlueprintCallable, Category = "Ingredient")
    void SetPlatingInstanceID(int32 InInstanceID) { PlatingInstanceID = InInstanceID; }
    int32 GetPlatingInstanceID() const { return PlatingInstanceID; }

    /** Returns world transforms of each chopped/minced procedural mesh piece. Empty if not chopped. */
    UFUNCTION(BlueprintCallable, Category = "Ingredient")
    TArray<FTransform> GetChoppedPieceWorldTransforms() const;

    /** Applies world transforms to each chopped/minced piece. Transforms are offset by Offset (e.g. when copying to preview at new location). */
    UFUNCTION(BlueprintCallable, Category = "Ingredient")
    void ApplyChoppedPieceTransforms(const TArray<FTransform>& Transforms, FVector Offset);

    /**
     * Visible primitives for scorecard / scene-capture (static mesh, or chopped procedural pieces only).
     * Prefer this over GetComponents<UPrimitiveComponent> — procedural slice state can leave components in a bad order for generic walks.
     */
    void GatherSnapshotPrimitiveComponents(TArray<class UPrimitiveComponent*>& OutPrimitives) const;

protected:
    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    // Procedural mesh pieces for chopped ingredients (when bIsChopped is true)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TArray<UProceduralMeshComponent*> ChoppedMeshPieces;

    // Whether this ingredient uses sliced procedural meshes (chopped preparation)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    bool bIsChopped = false;

    // Interaction properties
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    float HoverHeight = 2.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    float MovementSpeed = 5.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    float RotationSpeed = 45.0f;

    // Visual feedback
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
    UMaterialInterface* DefaultMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
    UMaterialInterface* HoverMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
    UMaterialInterface* GrabbedMaterial;

    /** When > 0, destroy this ingredient if it falls below this Z level (e.g. hits the ground). 0 = disabled. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (ClampMin = "0.0"))
    float GroundDestroyZThreshold = 50.0f;

    // State tracking
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    bool bIsHovered;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    bool bIsGrabbed;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    FVector OriginalPosition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    FRotator OriginalRotation;

    // Ingredient data
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Data")
    FPUIngredientBase IngredientData;

    /** InstanceID when used for plating - links mesh to dish data for transform capture. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Data")
    int32 PlatingInstanceID = -1;

public:
    // Event dispatchers
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnIngredientMoved OnIngredientMoved;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnIngredientRotated OnIngredientRotated;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnIngredientGrabbed OnIngredientGrabbed;

    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnIngredientReleased OnIngredientReleased;
}; 