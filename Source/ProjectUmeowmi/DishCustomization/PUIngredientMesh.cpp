#include "PUIngredientMesh.h"
#include "Components/StaticMeshComponent.h"

APUIngredientMesh::APUIngredientMesh()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create and setup the mesh component
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    // Enable mouse interaction
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    MeshComponent->SetCollisionProfileName(TEXT("UI"));
    MeshComponent->SetGenerateOverlapEvents(true);

    // Initialize state
    bIsHovered = false;
    bIsGrabbed = false;
    OriginalPosition = FVector::ZeroVector;
    OriginalRotation = FRotator::ZeroRotator;
}

void APUIngredientMesh::InitializeWithIngredient(const FPUIngredientBase& InIngredientData)
{
    IngredientData = InIngredientData;

    // Set the mesh
    if (IngredientData.IngredientMesh.IsValid())
    {
        if (UStaticMesh* LoadedMesh = IngredientData.IngredientMesh.LoadSynchronous())
        {
            MeshComponent->SetStaticMesh(LoadedMesh);
        }
    }

    // Set the material
    if (IngredientData.MaterialInstance.IsValid())
    {
        if (UMaterialInterface* LoadedMaterial = IngredientData.MaterialInstance.LoadSynchronous())
        {
            MeshComponent->SetMaterial(0, LoadedMaterial);
        }
    }
    else if (DefaultMaterial)
    {
        MeshComponent->SetMaterial(0, DefaultMaterial);
    }

    // Store initial position and rotation
    OriginalPosition = GetActorLocation();
    OriginalRotation = GetActorRotation();
}

void APUIngredientMesh::OnMouseHoverBegin()
{
    if (!bIsHovered && !bIsGrabbed)
    {
        bIsHovered = true;
        // Apply hover material
        if (HoverMaterial)
        {
            MeshComponent->SetMaterial(0, HoverMaterial);
        }
        // Lift the ingredient slightly
        FVector NewLocation = GetActorLocation();
        NewLocation.Z += HoverHeight;
        SetActorLocation(NewLocation);
    }
}

void APUIngredientMesh::OnMouseHoverEnd()
{
    if (bIsHovered && !bIsGrabbed)
    {
        bIsHovered = false;
        // Restore original material
        if (IngredientData.MaterialInstance.IsValid())
        {
            if (UMaterialInterface* LoadedMaterial = IngredientData.MaterialInstance.LoadSynchronous())
            {
                MeshComponent->SetMaterial(0, LoadedMaterial);
            }
        }
        else if (DefaultMaterial)
        {
            MeshComponent->SetMaterial(0, DefaultMaterial);
        }
        // Return to original height
        FVector NewLocation = GetActorLocation();
        NewLocation.Z = OriginalPosition.Z;
        SetActorLocation(NewLocation);
    }
}

void APUIngredientMesh::OnMouseGrab()
{
    if (!bIsGrabbed)
    {
        bIsGrabbed = true;
        bIsHovered = false;
        // Apply grabbed material
        if (GrabbedMaterial)
        {
            MeshComponent->SetMaterial(0, GrabbedMaterial);
        }
        // Broadcast the grabbed event
        OnIngredientGrabbed.Broadcast();
    }
}

void APUIngredientMesh::OnMouseRelease()
{
    if (bIsGrabbed)
    {
        bIsGrabbed = false;
        // Restore original material
        if (IngredientData.MaterialInstance.IsValid())
        {
            if (UMaterialInterface* LoadedMaterial = IngredientData.MaterialInstance.LoadSynchronous())
            {
                MeshComponent->SetMaterial(0, LoadedMaterial);
            }
        }
        else if (DefaultMaterial)
        {
            MeshComponent->SetMaterial(0, DefaultMaterial);
        }
        // Update original position and rotation
        OriginalPosition = GetActorLocation();
        OriginalRotation = GetActorRotation();
        // Broadcast the released event
        OnIngredientReleased.Broadcast();
    }
}

void APUIngredientMesh::UpdatePosition(const FVector& NewPosition)
{
    if (bIsGrabbed)
    {
        // Smoothly move to the new position
        FVector CurrentLocation = GetActorLocation();
        FVector TargetLocation = FVector(NewPosition.X, NewPosition.Y, OriginalPosition.Z + HoverHeight);
        FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, GetWorld()->GetDeltaSeconds(), MovementSpeed);
        SetActorLocation(NewLocation);
        OnIngredientMoved.Broadcast(NewLocation);
    }
}

void APUIngredientMesh::UpdateRotation(const FRotator& NewRotation)
{
    if (bIsGrabbed)
    {
        // Smoothly rotate to the new rotation
        FRotator CurrentRotation = GetActorRotation();
        FRotator TargetRotation = NewRotation;
        FRotator NewRot = FMath::RInterpTo(CurrentRotation, TargetRotation, GetWorld()->GetDeltaSeconds(), RotationSpeed);
        SetActorRotation(NewRot);
        OnIngredientRotated.Broadcast(NewRot);
    }
} 