#include "PUIngredientMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PUDishCustomizationComponent.h"
#include "Engine/Engine.h"

APUIngredientMesh::APUIngredientMesh()
{
    PrimaryActorTick.bCanEverTick = true;

    // Create and setup the mesh component
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    // DO NOT configure physics or collision during CDO construction
    // During CDO construction, GEngine is not initialized, which causes GetSimplePhysicalMaterial to fail
    // All physics and collision setup will be done in PostInitializeComponents() after GEngine is available
    // This includes SetCollisionProfileName, SetCollisionEnabled, and any physics-related calls
    
    //UE_LOG(LogTemp,Display, TEXT("🖱️ Ingredient mesh created with mouse events bound"));

    // Initialize state
    bIsHovered = false;
    bIsGrabbed = false;
    OriginalPosition = FVector::ZeroVector;
    OriginalRotation = FRotator::ZeroRotator;
}

void APUIngredientMesh::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    // Apply physics settings after components are initialized and GEngine is available
    // This ensures physics settings are applied even if they were skipped during CDO construction
    // Double-check: ensure we're not in CDO construction AND GEngine is available AND component is not CDO
    if (MeshComponent && !HasAnyFlags(RF_ClassDefaultObject) && !MeshComponent->HasAnyFlags(RF_ClassDefaultObject) && GEngine)
    {
        // Enable physics and collision for interactive ingredients
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        MeshComponent->SetCollisionProfileName(TEXT("PhysicsActor"));
        MeshComponent->SetGenerateOverlapEvents(true);
        MeshComponent->SetSimulatePhysics(true);
        MeshComponent->SetEnableGravity(true);
        MeshComponent->SetMobility(EComponentMobility::Movable);
        
        // Add physics damping to make movement less intense
        MeshComponent->SetLinearDamping(2.0f);
        MeshComponent->SetAngularDamping(5.0f);
        
        // Reduce mass to make ingredients less bouncy
        MeshComponent->SetMassScale(NAME_None, 0.5f);
        
        // Ensure visibility channel is blocked for mouse interaction
        MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        
        // Enable mouse interaction events
        MeshComponent->SetNotifyRigidBodyCollision(true);
    }
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
            
            // Re-apply visibility channel collision response for mouse interaction
            MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
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
    //UE_LOG(LogTemp,Display, TEXT("🖱️ Hover BEGIN called on ingredient: %s (Hovered: %s, Grabbed: %s)"), 
    //    *GetName(), bIsHovered ? TEXT("True") : TEXT("False"), bIsGrabbed ? TEXT("True") : TEXT("False"));
    
    if (!bIsHovered && !bIsGrabbed)
    {
        bIsHovered = true;
        //UE_LOG(LogTemp,Display, TEXT("🖱️ Hover BEGIN processed on ingredient: %s"), *GetName());
        
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
    //UE_LOG(LogTemp,Display, TEXT("🖱️ Hover END called on ingredient: %s (Hovered: %s, Grabbed: %s)"), 
    //    *GetName(), bIsHovered ? TEXT("True") : TEXT("False"), bIsGrabbed ? TEXT("True") : TEXT("False"));
    
    if (bIsHovered && !bIsGrabbed)
    {
        bIsHovered = false;
        //UE_LOG(LogTemp,Display, TEXT("🖱️ Hover END processed on ingredient: %s"), *GetName());
        
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
        FVector CurrentPos = GetActorLocation();
        bool bWasVisible = MeshComponent && MeshComponent->IsVisible();
        
        //UE_LOG(LogTemp,Display, TEXT("🖱️ [GRAB] OnMouseGrab - %s at (%.2f,%.2f,%.2f), Visible: %s"), 
        //    *GetName(), CurrentPos.X, CurrentPos.Y, CurrentPos.Z, bWasVisible ? TEXT("Yes") : TEXT("No"));
        
        bIsGrabbed = true;
        bIsHovered = false;
        
        // Store the current position as the original position for reference
        OriginalPosition = CurrentPos;
        
        // Disable physics while dragging to prevent interference
        if (MeshComponent)
        {
            MeshComponent->SetSimulatePhysics(false);
            MeshComponent->SetEnableGravity(false);
            
            // Ensure the mesh is still visible
            MeshComponent->SetVisibility(true);
            MeshComponent->SetHiddenInGame(false);
            
            //UE_LOG(LogTemp,Display, TEXT("🖱️ [GRAB] Disabled physics, ensured visibility for %s"), *GetName());
        }
        
        // Apply grabbed material
        if (GrabbedMaterial && MeshComponent)
        {
            MeshComponent->SetMaterial(0, GrabbedMaterial);
        }
        
        // Verify position after changes
        FVector PosAfterGrab = GetActorLocation();
        //UE_LOG(LogTemp,Display, TEXT("🖱️ [GRAB] After grab setup - %s at (%.2f,%.2f,%.2f)"), 
        //    *GetName(), PosAfterGrab.X, PosAfterGrab.Y, PosAfterGrab.Z);
        
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
    if (!bIsGrabbed)
    {
        return;
    }
    
    // Safety check: ensure we have a valid world
    UWorld* World = GetWorld();
    if (!World)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ APUIngredientMesh::UpdatePosition - No valid world"));
        return;
    }
    
    // Safety check: ensure the actor is still valid
    if (!IsValid(this))
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ APUIngredientMesh::UpdatePosition - Actor is no longer valid"));
        return;
    }
    
    // Smoothly move to the new position
    // Use the new position's Z directly (or add a small offset if needed)
    FVector CurrentLocation = GetActorLocation();
    FVector TargetLocation = NewPosition; // Use the provided position directly
    
    // Only update if we have a valid location
    if (TargetLocation.ContainsNaN())
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ [POS] UpdatePosition - %s: Target location contains NaN, ignoring"), *GetName());
        return;
    }
    
    // Check if target position is reasonable (not too far away)
    float DistanceFromOriginal = FVector::Dist(TargetLocation, OriginalPosition);
    if (DistanceFromOriginal > 10000.0f)
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ [POS] UpdatePosition - %s: Target position too far from original (%.2f units), clamping"), 
        //    *GetName(), DistanceFromOriginal);
        // Clamp to a reasonable distance
        FVector Direction = (TargetLocation - OriginalPosition).GetSafeNormal();
        TargetLocation = OriginalPosition + (Direction * 1000.0f);
    }
    
    FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, World->GetDeltaSeconds(), MovementSpeed);
    
    // Only set location if the actor is still valid and location is valid
    if (IsValid(this) && !NewLocation.ContainsNaN())
    {
        // Ensure the actor is still visible
        SetActorHiddenInGame(false);
        if (MeshComponent)
        {
            MeshComponent->SetVisibility(true);
            MeshComponent->SetHiddenInGame(false);
        }
        
        SetActorLocation(NewLocation, false, nullptr, ETeleportType::None);
        
        // Verify the location was set correctly
        FVector VerifyLocation = GetActorLocation();
        if (FVector::Dist(VerifyLocation, NewLocation) > 1.0f)
        {
            //UE_LOG(LogTemp,Warning, TEXT("⚠️ [POS] UpdatePosition - %s: Location mismatch! Set to (%.2f,%.2f,%.2f) but got (%.2f,%.2f,%.2f)"), 
            //    *GetName(), 
            //    NewLocation.X, NewLocation.Y, NewLocation.Z,
            //    VerifyLocation.X, VerifyLocation.Y, VerifyLocation.Z);
        }
        
        OnIngredientMoved.Broadcast(NewLocation);
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ [POS] UpdatePosition - %s: Cannot update position (Valid: %s, NaN: %s)"), 
        //    *GetName(), 
        //    IsValid(this) ? TEXT("Yes") : TEXT("No"),
        //    NewLocation.ContainsNaN() ? TEXT("Yes") : TEXT("No"));
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
    //UE_LOG(LogTemp,Display, TEXT("🖱️ Actor cursor over BEGIN: %s"), *GetName());
    OnMouseHoverBegin(nullptr);
}

void APUIngredientMesh::NotifyActorEndCursorOver()
{
    //UE_LOG(LogTemp,Display, TEXT("🖱️ Actor cursor over END: %s"), *GetName());
    OnMouseHoverEnd(nullptr);
}

void APUIngredientMesh::NotifyActorOnClicked(FKey ButtonPressed)
{
    FVector CurrentPos = GetActorLocation();
    //UE_LOG(LogTemp,Display, TEXT("🖱️ [CLICK] Actor clicked: %s (Button: %s) at position (%.2f,%.2f,%.2f)"), 
    //    *GetName(), *ButtonPressed.ToString(), CurrentPos.X, CurrentPos.Y, CurrentPos.Z);
    
    // Only handle left mouse button clicks
    if (ButtonPressed == EKeys::LeftMouseButton)
    {
        // Find the dish customization component to handle the drag
        TArray<AActor*> FoundActors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), FoundActors);
        
        for (AActor* Actor : FoundActors)
        {
            if (UPUDishCustomizationComponent* DishComponent = Actor->FindComponentByClass<UPUDishCustomizationComponent>())
            {
                if (DishComponent->IsPlatingMode())
                {
                    //UE_LOG(LogTemp,Display, TEXT("🖱️ [CLICK] Notifying dish customization component of ingredient click for %s"), *GetName());
                    
                    // Verify ingredient is still valid before proceeding
                    if (!IsValid(this))
                    {
                        //UE_LOG(LogTemp,Error, TEXT("❌ [CLICK] Ingredient %s is no longer valid!"), *GetName());
                        return;
                    }
                    
                    // Notify the component to start dragging this ingredient
                    // Don't call OnMouseGrab here - let StartDraggingIngredient handle it
                    DishComponent->StartDraggingIngredient(this);
                    
                    // Verify position after starting drag
                    if (IsValid(this))
                    {
                        FVector PosAfterStartDrag = GetActorLocation();
                        //UE_LOG(LogTemp,Display, TEXT("🖱️ [CLICK] After StartDraggingIngredient - %s at position (%.2f,%.2f,%.2f)"), 
                        //    *GetName(), PosAfterStartDrag.X, PosAfterStartDrag.Y, PosAfterStartDrag.Z);
                    }
                    return;
                }
            }
        }
        
        //UE_LOG(LogTemp,Warning, TEXT("⚠️ [CLICK] Could not find dish customization component in plating mode"));
    }
}

void APUIngredientMesh::TestMouseInteraction()
{
    //UE_LOG(LogTemp,Display, TEXT("🧪 Testing mouse interaction for ingredient: %s"), *GetName());
    
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    if (PlayerController)
    {
        // Get mouse position
        float MouseX, MouseY;
        PlayerController->GetMousePosition(MouseX, MouseY);
        
        //UE_LOG(LogTemp,Display, TEXT("🧪 Mouse position: (%.0f, %.0f)"), MouseX, MouseY);
        
        // Raycast from mouse position
        FHitResult HitResult;
        if (PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
        {
            //UE_LOG(LogTemp,Display, TEXT("🧪 Hit something: %s"), HitResult.GetActor() ? *HitResult.GetActor()->GetName() : TEXT("NULL"));
            
            if (HitResult.GetActor() == this)
            {
                //UE_LOG(LogTemp,Display, TEXT("🧪 SUCCESS: Mouse is hitting this ingredient!"));
            }
            else
            {
                //UE_LOG(LogTemp,Display, TEXT("🧪 Mouse is hitting something else"));
            }
        }
        else
        {
            //UE_LOG(LogTemp,Display, TEXT("🧪 No hit under cursor"));
        }
    }
} 