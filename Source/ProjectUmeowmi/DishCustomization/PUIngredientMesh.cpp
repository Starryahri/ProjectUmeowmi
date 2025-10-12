#include "PUIngredientMesh.h"
#include "Components/StaticMeshComponent.h"

APUIngredientMesh::APUIngredientMesh()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create and setup the mesh component
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    // Enable physics and collision for interactive ingredients
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MeshComponent->SetCollisionProfileName(TEXT("PhysicsActor"));
    MeshComponent->SetGenerateOverlapEvents(true);
    MeshComponent->SetSimulatePhysics(true);
    MeshComponent->SetEnableGravity(true);
    MeshComponent->SetMobility(EComponentMobility::Movable);
    
    // Add physics damping to make movement less intense
    MeshComponent->SetLinearDamping(2.0f);      // Reduces linear velocity over time
    MeshComponent->SetAngularDamping(5.0f);    // Reduces rotation over time
    
    // Reduce mass to make ingredients less bouncy
    MeshComponent->SetMassScale(NAME_None, 0.5f);  // Reduce mass scale
    
    // Ensure visibility channel is blocked for mouse interaction
    MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    
    // Enable mouse interaction events
    MeshComponent->SetNotifyRigidBodyCollision(true);
    
    UE_LOG(LogTemp, Display, TEXT("🖱️ Ingredient mesh created with mouse events bound"));

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
            
            // Ensure physics is enabled after setting the mesh
            MeshComponent->SetSimulatePhysics(true);
            MeshComponent->SetEnableGravity(true);
            MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            
            // Re-apply physics damping and mass settings
            MeshComponent->SetLinearDamping(2.0f);
            MeshComponent->SetAngularDamping(5.0f);
            MeshComponent->SetMassScale(NAME_None, 0.5f);
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

void APUIngredientMesh::OnMouseHoverBegin(UPrimitiveComponent* TouchedComponent)
{
    UE_LOG(LogTemp, Display, TEXT("🖱️ Hover BEGIN called on ingredient: %s (Hovered: %s, Grabbed: %s)"), 
        *GetName(), bIsHovered ? TEXT("True") : TEXT("False"), bIsGrabbed ? TEXT("True") : TEXT("False"));
    
    if (!bIsHovered && !bIsGrabbed)
    {
        bIsHovered = true;
        UE_LOG(LogTemp, Display, TEXT("🖱️ Hover BEGIN processed on ingredient: %s"), *GetName());
        
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

void APUIngredientMesh::OnMouseHoverEnd(UPrimitiveComponent* TouchedComponent)
{
    UE_LOG(LogTemp, Display, TEXT("🖱️ Hover END called on ingredient: %s (Hovered: %s, Grabbed: %s)"), 
        *GetName(), bIsHovered ? TEXT("True") : TEXT("False"), bIsGrabbed ? TEXT("True") : TEXT("False"));
    
    if (bIsHovered && !bIsGrabbed)
    {
        bIsHovered = false;
        UE_LOG(LogTemp, Display, TEXT("🖱️ Hover END processed on ingredient: %s"), *GetName());
        
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
        // Return to original height smoothly
        FVector NewLocation = GetActorLocation();
        NewLocation.Z = FMath::FInterpTo(NewLocation.Z, OriginalPosition.Z, GetWorld()->GetDeltaSeconds(), 5.0f);
        SetActorLocation(NewLocation);
    }
}

void APUIngredientMesh::OnMouseGrab()
{
    if (!bIsGrabbed)
    {
        bIsGrabbed = true;
        bIsHovered = false;
        
        // Disable physics while dragging to prevent interference
        MeshComponent->SetSimulatePhysics(false);
        MeshComponent->SetEnableGravity(false);
        
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
        
        // Re-enable physics after dragging with gentle release
        MeshComponent->SetSimulatePhysics(true);
        MeshComponent->SetEnableGravity(true);
        
        // Apply gentle physics settings for smoother release
        MeshComponent->SetLinearDamping(3.0f);      // Higher damping for release
        MeshComponent->SetAngularDamping(8.0f);    // Higher angular damping for release
        
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

void APUIngredientMesh::NotifyActorBeginCursorOver()
{
    UE_LOG(LogTemp, Display, TEXT("🖱️ Actor cursor over BEGIN: %s"), *GetName());
    OnMouseHoverBegin(nullptr);
}

void APUIngredientMesh::NotifyActorEndCursorOver()
{
    UE_LOG(LogTemp, Display, TEXT("🖱️ Actor cursor over END: %s"), *GetName());
    OnMouseHoverEnd(nullptr);
}

void APUIngredientMesh::TestMouseInteraction()
{
    UE_LOG(LogTemp, Display, TEXT("🧪 Testing mouse interaction for ingredient: %s"), *GetName());
    
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (PlayerController)
    {
        // Get mouse position
        float MouseX, MouseY;
        PlayerController->GetMousePosition(MouseX, MouseY);
        
        UE_LOG(LogTemp, Display, TEXT("🧪 Mouse position: (%.0f, %.0f)"), MouseX, MouseY);
        
        // Raycast from mouse position
        FHitResult HitResult;
        if (PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
        {
            UE_LOG(LogTemp, Display, TEXT("🧪 Hit something: %s"), HitResult.GetActor() ? *HitResult.GetActor()->GetName() : TEXT("NULL"));
            
            if (HitResult.GetActor() == this)
            {
                UE_LOG(LogTemp, Display, TEXT("🧪 SUCCESS: Mouse is hitting this ingredient!"));
            }
            else
            {
                UE_LOG(LogTemp, Display, TEXT("🧪 Mouse is hitting something else"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Display, TEXT("🧪 No hit under cursor"));
        }
    }
} 