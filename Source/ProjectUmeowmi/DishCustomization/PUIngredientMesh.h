#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PUIngredientBase.h"
#include "PUIngredientMesh.generated.h"

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

    // Setup the mesh with ingredient data
    UFUNCTION(BlueprintCallable, Category = "Ingredient")
    void InitializeWithIngredient(const FPUIngredientBase& IngredientData);

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

protected:
    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;

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