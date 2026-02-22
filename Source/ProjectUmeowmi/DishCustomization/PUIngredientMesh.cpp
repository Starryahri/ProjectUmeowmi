#include "PUIngredientMesh.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "KismetProceduralMeshLibrary.h"
#include "PUDishCustomizationComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "PhysicsEngine/BodySetup.h"
#include "Materials/Material.h"
#include "MaterialDomain.h"

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

    // Do NOT enable physics here - MeshComponent has no mesh yet, which can cause
    // "Collision Enabled is incompatible" when collision defaults to NoCollision.
    // Physics is enabled in InitializeWithIngredient (non-chopped) or never (chopped).
    if (MeshComponent && !HasAnyFlags(RF_ClassDefaultObject) && !MeshComponent->HasAnyFlags(RF_ClassDefaultObject) && GEngine)
    {
        MeshComponent->SetMobility(EComponentMobility::Movable);
        MeshComponent->SetCollisionProfileName(TEXT("PhysicsActor"));
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);  // No mesh yet; InitializeWithIngredient will enable
        MeshComponent->SetSimulatePhysics(false);
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
            
            // Enable physics and collision (PostInitializeComponents leaves them off until mesh is set)
            MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            MeshComponent->SetSimulatePhysics(true);
            MeshComponent->SetEnableGravity(true);
            MeshComponent->SetGenerateOverlapEvents(true);
            
            // Re-apply physics damping and mass settings
            MeshComponent->SetLinearDamping(2.0f);
            MeshComponent->SetAngularDamping(5.0f);
            MeshComponent->SetMassScale(NAME_None, 0.5f);
            
            // Re-apply visibility channel collision response for mouse interaction
            MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
            MeshComponent->SetNotifyRigidBodyCollision(true);
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

void APUIngredientMesh::InitializeWithIngredientInstance(const FIngredientInstance& IngredientInstance)
{
    IngredientData = IngredientInstance.IngredientData;

    // Check if ingredient has Chop or Mince preparation (both get sliced mesh)
    static const FGameplayTag PrepChopTag = FGameplayTag::RequestGameplayTag(FName("Prep.Chop"));
    static const FGameplayTag PrepMinceTag = FGameplayTag::RequestGameplayTag(FName("Prep.Mince"));

    const bool bHasChopPrep = IngredientInstance.IngredientData.ActivePreparations.HasTag(PrepChopTag) ||
                              IngredientInstance.IngredientData.ActivePreparations.HasTag(PrepMinceTag) ||
                              IngredientInstance.Preparations.HasTag(PrepChopTag) ||
                              IngredientInstance.Preparations.HasTag(PrepMinceTag);
    const bool bIsMinced = IngredientInstance.IngredientData.ActivePreparations.HasTag(PrepMinceTag) ||
                          IngredientInstance.Preparations.HasTag(PrepMinceTag);

    if (bHasChopPrep && IngredientData.IngredientMesh.IsValid())
    {
        UStaticMesh* SourceMesh = IngredientData.IngredientMesh.LoadSynchronous();
        if (SourceMesh && GetWorld())
        {
            bIsChopped = true;
            MeshComponent->SetVisibility(false);
            MeshComponent->SetSimulatePhysics(false);  // Must disable before NoCollision to avoid invalid state
            MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

            // Get material for exterior (resolve with fallbacks)
            UMaterialInterface* IngredientMaterial = nullptr;
            if (IngredientData.MaterialInstance.IsValid())
            {
                IngredientMaterial = IngredientData.MaterialInstance.LoadSynchronous();
            }
            if (!IngredientMaterial && DefaultMaterial)
            {
                IngredientMaterial = DefaultMaterial;
            }
            if (!IngredientMaterial && SourceMesh)
            {
                IngredientMaterial = SourceMesh->GetMaterial(0);
            }
            if (!IngredientMaterial)
            {
                IngredientMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
            }

            // Get material for cut surfaces (caps); separate so it doesn't interfere with MaterialInstance
            UMaterialInterface* CapMaterial = nullptr;
            if (IngredientData.CapMaterialInstance.IsValid())
            {
                CapMaterial = IngredientData.CapMaterialInstance.LoadSynchronous();
            }
            if (!CapMaterial)
            {
                CapMaterial = IngredientMaterial;  // Fallback to exterior material
            }

            // Create temporary StaticMeshComponent for copying
            AStaticMeshActor* TempActor = GetWorld()->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator);
            if (TempActor)
            {
                UStaticMeshComponent* TempComp = TempActor->GetStaticMeshComponent();
                TempComp->SetMobility(EComponentMobility::Movable);  // Required before SetStaticMesh
                TempComp->SetStaticMesh(SourceMesh);
                if (IngredientMaterial)
                {
                    TempComp->SetMaterial(0, IngredientMaterial);
                }

                // Create first procedural mesh and copy from static mesh
                UProceduralMeshComponent* FirstProcMesh = NewObject<UProceduralMeshComponent>(this);
                FirstProcMesh->bUseComplexAsSimpleCollision = false;  // Must set BEFORE RegisterComponent
                FirstProcMesh->SetupAttachment(RootComponent);
                FirstProcMesh->RegisterComponent();

                // Note: For cooked builds, enable "Allow CPU Access" on the source Static Mesh asset
                // (Details panel when mesh is selected) to avoid GetSectionFromStaticMesh warnings.
                UKismetProceduralMeshLibrary::CopyProceduralMeshFromStaticMeshComponent(TempComp, 0, FirstProcMesh, false);
                TempActor->Destroy();

                // Compute bounds from sections (Bounds may be stale before first render)
                FBox MeshBounds(ForceInit);
                for (int32 SecIdx = 0; SecIdx < FirstProcMesh->GetNumSections(); ++SecIdx)
                {
                    if (const FProcMeshSection* Sec = FirstProcMesh->GetProcMeshSection(SecIdx))
                    {
                        MeshBounds += Sec->SectionLocalBox;
                    }
                }
                if (!MeshBounds.IsValid)
                {
                    bIsChopped = false;
                }
                else
                {

                // Add simple box collision (slice requires source collision; ComplexAsSimple is incompatible with physics)
                FVector Min = MeshBounds.Min, Max = MeshBounds.Max;
                TArray<FVector> BoxVerts = {
                    FVector(Min.X, Min.Y, Min.Z), FVector(Max.X, Min.Y, Min.Z), FVector(Max.X, Max.Y, Min.Z), FVector(Min.X, Max.Y, Min.Z),
                    FVector(Min.X, Min.Y, Max.Z), FVector(Max.X, Min.Y, Max.Z), FVector(Max.X, Max.Y, Max.Z), FVector(Min.X, Max.Y, Max.Z)
                };
                FirstProcMesh->AddCollisionConvexMesh(BoxVerts);

                // Slice into 4 pieces (2 perpendicular plane cuts through mesh center)
                // Use bounds extents to pick axes: slice perpendicular to the two largest dimensions
                // so the plane always intersects the mesh (avoids BoxCompare==1 where slice is skipped)
                FVector LocalCenter = MeshBounds.GetCenter();
                FVector Extent = MeshBounds.GetExtent();
                FVector WorldCenter = FirstProcMesh->GetComponentToWorld().TransformPosition(LocalCenter);
                FTransform CompToWorld = FirstProcMesh->GetComponentToWorld();
                // Pick axes by extent: largest = axis 0, second = axis 1, smallest = axis 2 (for mince)
                int32 Axis0 = 0, Axis1 = 1, Axis2 = 2;
                if (Extent.Y >= Extent.X && Extent.Y >= Extent.Z) { Axis0 = 1; Axis1 = (Extent.X >= Extent.Z) ? 0 : 2; }
                else if (Extent.Z >= Extent.X && Extent.Z >= Extent.Y) { Axis0 = 2; Axis1 = (Extent.X >= Extent.Y) ? 0 : 1; }
                else { Axis0 = 0; Axis1 = (Extent.Y >= Extent.Z) ? 1 : 2; }
                for (int32 i = 0; i < 3; ++i) { if (i != Axis0 && i != Axis1) { Axis2 = i; break; } }
                FVector LocalNorm0(0,0,0); LocalNorm0[Axis0] = 1.f;
                FVector LocalNorm1(0,0,0); LocalNorm1[Axis1] = 1.f;
                FVector LocalNorm2(0,0,0); LocalNorm2[Axis2] = 1.f;
                FVector WorldNormalX = CompToWorld.TransformVectorNoScale(LocalNorm0);
                FVector WorldNormalY = CompToWorld.TransformVectorNoScale(LocalNorm1);
                FVector WorldNormalZ = CompToWorld.TransformVectorNoScale(LocalNorm2);

                // First slice - through center, perpendicular to largest extent axis
                UProceduralMeshComponent* OtherHalf1 = nullptr;
                UKismetProceduralMeshLibrary::SliceProceduralMesh(
                    FirstProcMesh,
                    WorldCenter,
                    WorldNormalX,
                    true,
                    OtherHalf1,
                    EProcMeshSliceCapOption::CreateNewSectionForCap,
                    CapMaterial
                );

                if (OtherHalf1)
                {
                    OtherHalf1->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
                }

                // Second slice on first half - through center, perpendicular to Y
                UProceduralMeshComponent* OtherHalf2 = nullptr;
                UKismetProceduralMeshLibrary::SliceProceduralMesh(
                    FirstProcMesh,
                    WorldCenter,
                    WorldNormalY,
                    true,
                    OtherHalf2,
                    EProcMeshSliceCapOption::CreateNewSectionForCap,
                    CapMaterial
                );

                if (OtherHalf2)
                {
                    OtherHalf2->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
                }

                // Second slice on first other half (same plane through world center)
                UProceduralMeshComponent* OtherHalf3 = nullptr;
                if (OtherHalf1)
                {
                    UKismetProceduralMeshLibrary::SliceProceduralMesh(
                        OtherHalf1,
                        WorldCenter,
                        WorldNormalY,
                        true,
                        OtherHalf3,
                        EProcMeshSliceCapOption::CreateNewSectionForCap,
                        CapMaterial
                    );
                    if (OtherHalf3)
                    {
                        OtherHalf3->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
                    }
                }

                // Mince: third slice (perpendicular to Z) on all 4 pieces -> 8 pieces total
                TArray<UProceduralMeshComponent*> AllPieces;
                AllPieces.Add(FirstProcMesh);
                if (OtherHalf1) AllPieces.Add(OtherHalf1);
                if (OtherHalf2) AllPieces.Add(OtherHalf2);
                if (OtherHalf3) AllPieces.Add(OtherHalf3);

                if (bIsMinced)
                {
                    TArray<UProceduralMeshComponent*> NewPieces;
                    for (UProceduralMeshComponent* Piece : AllPieces)
                    {
                        if (Piece)
                        {
                            UProceduralMeshComponent* OtherHalf = nullptr;
                            UKismetProceduralMeshLibrary::SliceProceduralMesh(
                                Piece,
                                WorldCenter,
                                WorldNormalZ,
                                true,
                                OtherHalf,
                                EProcMeshSliceCapOption::CreateNewSectionForCap,
                                CapMaterial
                            );
                            if (OtherHalf)
                            {
                                OtherHalf->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
                                NewPieces.Add(OtherHalf);
                            }
                        }
                    }
                    for (UProceduralMeshComponent* P : NewPieces)
                    {
                        AllPieces.Add(P);
                    }
                }

                // Collect all pieces
                ChoppedMeshPieces = MoveTemp(AllPieces);

                // Apply physics, collision, and materials to all pieces
                for (UProceduralMeshComponent* ProcMesh : ChoppedMeshPieces)
                {
                    if (ProcMesh)
                    {
                        ProcMesh->bUseComplexAsSimpleCollision = false;
                        if (UBodySetup* BodySetup = ProcMesh->GetBodySetup())
                        {
                            BodySetup->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseDefault;
                            BodySetup->InvalidatePhysicsData();
                        }
                        ProcMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                        ProcMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
                        ProcMesh->SetSimulatePhysics(true);
                        ProcMesh->SetEnableGravity(true);
                        ProcMesh->SetLinearDamping(2.0f);
                        ProcMesh->SetAngularDamping(5.0f);
                        ProcMesh->SetMassScale(NAME_None, 0.15f);
                        ProcMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
                        ProcMesh->RecreatePhysicsState();
                        // Force exterior material onto non-cap sections; caps already have CapMaterial from SliceProceduralMesh
                        for (int32 SecIdx = 0; SecIdx < ProcMesh->GetNumSections(); ++SecIdx)
                        {
                            UMaterialInterface* CurrentMat = ProcMesh->GetMaterial(SecIdx);
                            if (CurrentMat != CapMaterial)
                            {
                                ProcMesh->SetMaterial(SecIdx, IngredientMaterial);
                            }
                        }
                        ProcMesh->SetVisibility(true);
                        ProcMesh->MarkRenderStateDirty();
                    }
                }
                }  // else MeshBounds.IsValid
            }
            else
            {
                bIsChopped = false;
            }
        }
        else
        {
            bIsChopped = false;
        }
    }

    if (!bIsChopped)
    {
        InitializeWithIngredient(IngredientInstance.IngredientData);
        return;
    }

    OriginalPosition = GetActorLocation();
    OriginalRotation = GetActorRotation();
}

void APUIngredientMesh::SetIngredientScale(const FVector& Scale)
{
    if (bIsChopped)
    {
        // Chopped/minced: actor scale doesn't propagate to proc mesh pieces; set scale on each piece directly
        for (UProceduralMeshComponent* ProcMesh : ChoppedMeshPieces)
        {
            if (ProcMesh)
            {
                ProcMesh->SetRelativeScale3D(Scale);
                ProcMesh->RecreatePhysicsState();  // Collision must update for new scale
            }
        }
    }
    else
    {
        SetActorScale3D(Scale);
    }
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
            if (bIsChopped)
            {
                for (UProceduralMeshComponent* ProcMesh : ChoppedMeshPieces)
                {
                    if (ProcMesh)
                    {
                        for (int32 i = 0; i < ProcMesh->GetNumMaterials(); ++i)
                        {
                            ProcMesh->SetMaterial(i, HoverMaterial);
                        }
                    }
                }
            }
            else if (MeshComponent)
            {
                MeshComponent->SetMaterial(0, HoverMaterial);
            }
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
        UMaterialInterface* RestoreMaterial = nullptr;
        if (IngredientData.MaterialInstance.IsValid())
        {
            RestoreMaterial = IngredientData.MaterialInstance.LoadSynchronous();
        }
        if (!RestoreMaterial && DefaultMaterial)
        {
            RestoreMaterial = DefaultMaterial;
        }
        if (RestoreMaterial)
        {
            if (bIsChopped)
            {
                for (UProceduralMeshComponent* ProcMesh : ChoppedMeshPieces)
                {
                    if (ProcMesh)
                    {
                        for (int32 i = 0; i < ProcMesh->GetNumMaterials(); ++i)
                        {
                            ProcMesh->SetMaterial(i, RestoreMaterial);
                        }
                    }
                }
            }
            else if (MeshComponent)
            {
                MeshComponent->SetMaterial(0, RestoreMaterial);
            }
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
        if (bIsChopped)
        {
            for (UProceduralMeshComponent* ProcMesh : ChoppedMeshPieces)
            {
                if (ProcMesh)
                {
                    ProcMesh->SetSimulatePhysics(false);
                    ProcMesh->SetEnableGravity(false);
                    ProcMesh->SetVisibility(true);
                    ProcMesh->SetHiddenInGame(false);
                }
            }
        }
        else if (MeshComponent)
        {
            MeshComponent->SetSimulatePhysics(false);
            MeshComponent->SetEnableGravity(false);
            MeshComponent->SetVisibility(true);
            MeshComponent->SetHiddenInGame(false);
        }
        
        // Apply grabbed material
        if (GrabbedMaterial)
        {
            if (bIsChopped)
            {
                for (UProceduralMeshComponent* ProcMesh : ChoppedMeshPieces)
                {
                    if (ProcMesh)
                    {
                        for (int32 i = 0; i < ProcMesh->GetNumMaterials(); ++i)
                        {
                            ProcMesh->SetMaterial(i, GrabbedMaterial);
                        }
                    }
                }
            }
            else if (MeshComponent)
            {
                MeshComponent->SetMaterial(0, GrabbedMaterial);
            }
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
        if (bIsChopped)
        {
            for (UProceduralMeshComponent* ProcMesh : ChoppedMeshPieces)
            {
                if (ProcMesh)
                {
                    ProcMesh->SetSimulatePhysics(true);
                    ProcMesh->SetEnableGravity(true);
                    ProcMesh->SetLinearDamping(3.0f);
                    ProcMesh->SetAngularDamping(8.0f);
                }
            }
        }
        else if (MeshComponent)
        {
            MeshComponent->SetSimulatePhysics(true);
            MeshComponent->SetEnableGravity(true);
            MeshComponent->SetLinearDamping(3.0f);
            MeshComponent->SetAngularDamping(8.0f);
        }
        
        // Restore original material
        UMaterialInterface* RestoreMaterial = nullptr;
        if (IngredientData.MaterialInstance.IsValid())
        {
            RestoreMaterial = IngredientData.MaterialInstance.LoadSynchronous();
        }
        if (!RestoreMaterial && DefaultMaterial)
        {
            RestoreMaterial = DefaultMaterial;
        }
        if (RestoreMaterial)
        {
            if (bIsChopped)
            {
                for (UProceduralMeshComponent* ProcMesh : ChoppedMeshPieces)
                {
                    if (ProcMesh)
                    {
                        for (int32 i = 0; i < ProcMesh->GetNumMaterials(); ++i)
                        {
                            ProcMesh->SetMaterial(i, RestoreMaterial);
                        }
                    }
                }
            }
            else if (MeshComponent)
            {
                MeshComponent->SetMaterial(0, RestoreMaterial);
            }
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
        if (bIsChopped)
        {
            for (UProceduralMeshComponent* ProcMesh : ChoppedMeshPieces)
            {
                if (ProcMesh)
                {
                    ProcMesh->SetVisibility(true);
                    ProcMesh->SetHiddenInGame(false);
                }
            }
        }
        else if (MeshComponent)
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