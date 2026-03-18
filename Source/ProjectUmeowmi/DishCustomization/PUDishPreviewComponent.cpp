#include "PUDishPreviewComponent.h"
#include "PUIngredientMesh.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

DEFINE_LOG_CATEGORY(LogDishPreview);

UPUDishPreviewComponent::UPUDishPreviewComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    DishMeshComponent = nullptr;  // Set via SetDishMeshComponent from owning Actor to avoid template/instance mismatch
}

void UPUDishPreviewComponent::SetDishMeshComponent(UStaticMeshComponent* InDishMesh)
{
    DishMeshComponent = InDishMesh;
}

void UPUDishPreviewComponent::BuildFromDishData(const FPUDishBase& DishData)
{
    UE_LOG(LogDishPreview, Log, TEXT("BuildFromDishData - START (dish: %s, %d ingredients)"),
        *DishData.DishName.ToString(), DishData.IngredientInstances.Num());
    if (bEnableDishPreviewDebug && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("[DishPreview] BuildFromDishData START - %s (%d ingredients)"), *DishData.DishName.ToString(), DishData.IngredientInstances.Num()));
    }

    ClearPreview();

    UWorld* World = GetWorld();
    AActor* Owner = GetOwner();
    if (!World || !Owner)
    {
        UE_LOG(LogDishPreview, Warning, TEXT("BuildFromDishData - No World or Owner"));
        if (bEnableDishPreviewDebug && GEngine) { GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("[DishPreview] ERROR: No World or Owner")); }
        return;
    }

    // Parent: character's root - ensures preview follows player
    USceneComponent* ParentComponent = Owner->GetRootComponent();
    if (!ParentComponent)
    {
        UE_LOG(LogDishPreview, Warning, TEXT("BuildFromDishData - Owner has no RootComponent"));
        return;
    }

    // Base position: character's location + offset above head (same for dish and all ingredients)
    const FVector OffsetAboveHead = FVector(0.0f, 0.0f, OffsetAboveHeadZ);
    FVector BaseWorldPos = Owner->GetActorLocation() + Owner->GetActorQuat().RotateVector(OffsetAboveHead);
    FRotator BaseWorldRot = Owner->GetActorRotation();
    UE_LOG(LogDishPreview, Log, TEXT("BuildFromDishData - Character at (%.1f, %.1f, %.1f), preview base (%.1f, %.1f, %.1f)"),
        Owner->GetActorLocation().X, Owner->GetActorLocation().Y, Owner->GetActorLocation().Z,
        BaseWorldPos.X, BaseWorldPos.Y, BaseWorldPos.Z);

    // Origin for ingredient placement: use captured dish center when available, else centroid of plated items
    FVector Origin = FVector::ZeroVector;
    const bool bHasDishCenter = DishData.PlatingDishCenter.SizeSquared() > KINDA_SMALL_NUMBER;
    if (bHasDishCenter)
    {
        Origin = DishData.PlatingDishCenter;
        UE_LOG(LogDishPreview, Log, TEXT("BuildFromDishData - using captured dish center as origin (%.1f, %.1f, %.1f)"),
            Origin.X, Origin.Y, Origin.Z);
    }
    else
    {
        int32 PlatedCount = 0;
        if (DishData.PlatingEntries.Num() > 0)
        {
            for (const FPUPlatingEntry& Entry : DishData.PlatingEntries)
            {
                Origin += Entry.Position;
                PlatedCount++;
            }
        }
        else
        {
            for (const FIngredientInstance& Instance : DishData.IngredientInstances)
            {
                if (Instance.bIsPlated)
                {
                    Origin += Instance.PlatingPosition;
                    PlatedCount++;
                }
            }
        }
        if (PlatedCount > 0)
        {
            Origin /= PlatedCount;
        }
        UE_LOG(LogDishPreview, Log, TEXT("BuildFromDishData - %d plating entries, centroid origin (%.1f, %.1f, %.1f)"),
            PlatedCount, Origin.X, Origin.Y, Origin.Z);
    }

    // Dish mesh: try DishData.DishMesh first, then DefaultDishMesh fallback
    UStaticMesh* DishMesh = nullptr;
    if (DishData.DishMesh.IsValid())
    {
        DishMesh = DishData.DishMesh.LoadSynchronous();
    }
    if (!DishMesh && !DishData.DishMesh.ToSoftObjectPath().IsNull())
    {
        DishMesh = LoadObject<UStaticMesh>(nullptr, *DishData.DishMesh.ToString());
    }
    if (!DishMesh && DefaultDishMesh.IsValid())
    {
        DishMesh = DefaultDishMesh.LoadSynchronous();
        UE_LOG(LogDishPreview, Log, TEXT("BuildFromDishData - Using DefaultDishMesh fallback: %s"), DishMesh ? *DishMesh->GetName() : TEXT("null"));
    }
    if (!DishMesh && !DefaultDishMesh.ToSoftObjectPath().IsNull())
    {
        DishMesh = LoadObject<UStaticMesh>(nullptr, *DefaultDishMesh.ToString());
    }
    if (DishMesh && DishMeshComponent)
    {
        DishMeshComponent->SetStaticMesh(DishMesh);
        DishMeshComponent->SetVisibility(true);
        DishMeshComponent->SetWorldRotation(FRotator::ZeroRotator);  // No rotation - fixed orientation
        // Use scale 1.0 so ingredient positions (exact copy) match the dish size
        DishMeshComponent->SetWorldScale3D(FVector(1.0f));

        // Offset dish so its SURFACE (top of bounds) aligns with BaseWorldPos
        FBoxSphereBounds MeshBounds = DishMesh->GetBounds();
        float SurfaceOffsetZ = MeshBounds.Origin.Z + MeshBounds.BoxExtent.Z;  // Top of mesh in local space (scale 1.0)
        DishMeshComponent->SetWorldLocation(BaseWorldPos - FVector(0.0f, 0.0f, SurfaceOffsetZ));

        DishMeshComponent->AttachToComponent(ParentComponent, FAttachmentTransformRules::KeepWorldTransform);
        UE_LOG(LogDishPreview, Log, TEXT("BuildFromDishData - Dish mesh set: %s"), *DishMesh->GetName());
        if (bEnableDishPreviewDebug && GEngine) { GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("[DishPreview] Dish mesh: %s"), *DishMesh->GetName())); }
    }
    else
    {
        UE_LOG(LogDishPreview, Warning, TEXT("BuildFromDishData - No dish mesh (set DishMesh in data table or DefaultDishMesh on character)"));
        if (bEnableDishPreviewDebug && GEngine) { GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("[DishPreview] No dish mesh - set DefaultDishMesh on character Blueprint")); }
    }

    // Spawn from PlatingEntries (one per mesh - captures ALL plated meshes) or fallback to bIsPlated instances
    UClass* MeshClass = IngredientMeshClass ? IngredientMeshClass.Get() : APUIngredientMesh::StaticClass();
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = Owner;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    int32 SpawnedCount = 0;
    int32 NonPlatedIndex = 0;

    // Dish position is the origin for ingredients - use dish (or component) as parent
    USceneComponent* IngredientParent = (DishMeshComponent && DishMeshComponent->GetStaticMesh()) ? DishMeshComponent : ParentComponent;

    if (DishData.PlatingEntries.Num() > 0)
    {
        // Use PlatingEntries - one spawn per mesh/liquid (handles multiple of same ingredient)
        // Dish position (BaseWorldPos) is the new zero - ingredients are offset from there
        for (const FPUPlatingEntry& Entry : DishData.PlatingEntries)
        {
            FIngredientInstance Instance;
            if (!DishData.GetIngredientInstanceByID(Entry.InstanceID, Instance))
            {
                UE_LOG(LogDishPreview, Warning, TEXT("BuildFromDishData - PlatingEntry InstanceID %d not found in dish"), Entry.InstanceID);
                continue;
            }

            // Exact copy: use captured positions and rotations. No scaling - preserve layout exactly as plated.
            FVector OffsetFromOrigin = (Entry.Position - Origin);
            OffsetFromOrigin.Z += IngredientZOffset;  // Move up slightly so ingredients sit on dish surface
            FVector WorldPos = BaseWorldPos + OffsetFromOrigin;
            FRotator WorldRot = Entry.Rotation;

            // Liquid path: spawn Niagara fill instead of mesh (use Entry.bIsLiquid or ingredient data)
            if ((Entry.bIsLiquid || Instance.IngredientData.bIsLiquid) && Instance.IngredientData.LiquidParticleSystem.IsValid())
            {
                UNiagaraSystem* NiagaraSystem = Instance.IngredientData.LiquidParticleSystem.LoadSynchronous();
                if (NiagaraSystem && Owner)
                {
                    UNiagaraComponent* NiagaraComp = NewObject<UNiagaraComponent>(Owner, UNiagaraComponent::StaticClass(), NAME_None, RF_Transient);
                    if (NiagaraComp)
                    {
                        NiagaraComp->SetAsset(NiagaraSystem);
                        NiagaraComp->SetAutoActivate(true);
                        NiagaraComp->RegisterComponent();
                        NiagaraComp->AttachToComponent(IngredientParent, FAttachmentTransformRules::KeepWorldTransform);
                        NiagaraComp->SetWorldLocation(WorldPos);
                        NiagaraComp->SetWorldRotation(WorldRot);
                        NiagaraComp->SetWorldScale3D(FVector(1.0f));
                        NiagaraComp->Activate(true);
                        PreviewLiquidComponents.Add(NiagaraComp);
                        SpawnedCount++;
                    }
                }
                continue;
            }

            // Solid path: spawn mesh (use captured rotation and scale - exact copy)
            FVector EffectiveScale;
            if (Instance.IngredientData.MeshScale.SizeSquared() > KINDA_SMALL_NUMBER)
            {
                EffectiveScale = Instance.IngredientData.MeshScale;
            }
            else
            {
                EffectiveScale = (Entry.Scale.SizeSquared() > KINDA_SMALL_NUMBER) ? Entry.Scale : FVector::OneVector;
            }

            APUIngredientMesh* Spawned = World->SpawnActor<APUIngredientMesh>(MeshClass, WorldPos, WorldRot, SpawnParams);
            if (!Spawned)
            {
                UE_LOG(LogDishPreview, Warning, TEXT("BuildFromDishData - Failed to spawn ingredient InstanceID %d"), Entry.InstanceID);
                continue;
            }

            Spawned->InitializeWithIngredientInstance(Instance);

            if (UStaticMeshComponent* MeshComp = Spawned->FindComponentByClass<UStaticMeshComponent>())
            {
                MeshComp->SetSimulatePhysics(false);
                MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            }
            for (UActorComponent* Comp : Spawned->GetComponents())
            {
                if (UProceduralMeshComponent* ProcMesh = Cast<UProceduralMeshComponent>(Comp))
                {
                    ProcMesh->SetSimulatePhysics(false);
                    ProcMesh->SetEnableGravity(false);
                    ProcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                    ProcMesh->SetMobility(EComponentMobility::Movable);
                    ProcMesh->DestroyPhysicsState();  // Remove physics body so component follows parent
                    ProcMesh->AttachToComponent(Spawned->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);  // Re-attach: disabling physics does NOT auto-reattach
                }
            }

            Spawned->SetIngredientScale(EffectiveScale);
            // Restore per-piece layout for chopped/minced ingredients (captured from plating)
            if (Entry.ChoppedPieceTransforms.Num() > 0)
            {
                Spawned->ApplyChoppedPieceTransforms(Entry.ChoppedPieceTransforms, WorldPos - Entry.Position);
            }
            Spawned->AttachToComponent(IngredientParent, FAttachmentTransformRules::KeepWorldTransform);
            PreviewIngredientMeshes.Add(Spawned);
            SpawnedCount++;
        }
    }
    else
    {
        // Fallback: use IngredientInstances with bIsPlated
        for (const FIngredientInstance& Instance : DishData.IngredientInstances)
        {
            FVector WorldPos;
            FRotator LocalRot;
            FVector InstanceScale;

            if (Instance.bIsPlated)
            {
                FVector OffsetFromOrigin = (Instance.PlatingPosition - Origin);
                OffsetFromOrigin.Z += IngredientZOffset;  // Move up slightly
                WorldPos = BaseWorldPos + OffsetFromOrigin;
                LocalRot = Instance.PlatingRotation;
                if (Instance.IngredientData.MeshScale.SizeSquared() > KINDA_SMALL_NUMBER)
                {
                    InstanceScale = Instance.IngredientData.MeshScale;
                }
                else
                {
                    InstanceScale = (Instance.PlatingScale.SizeSquared() > KINDA_SMALL_NUMBER)
                        ? Instance.PlatingScale : FVector::OneVector;
                }
            }
            else
            {
                const float FanSpacing = 25.0f;
                FVector OffsetFromOrigin = FVector(NonPlatedIndex * FanSpacing, 0.0f, 0.0f) * PreviewScale;
                NonPlatedIndex++;
                OffsetFromOrigin.Z += IngredientZOffset;
                WorldPos = BaseWorldPos + OffsetFromOrigin;
                LocalRot = FRotator::ZeroRotator;
                InstanceScale = (Instance.IngredientData.MeshScale.SizeSquared() > KINDA_SMALL_NUMBER)
                    ? Instance.IngredientData.MeshScale : FVector::OneVector;
            }

            FVector EffectiveScale = InstanceScale;
            FRotator WorldRot = Instance.bIsPlated ? Instance.PlatingRotation : FRotator::ZeroRotator;

            // Liquid path (fallback when PlatingEntries empty): spawn Niagara instead of mesh
            if (Instance.IngredientData.bIsLiquid && Instance.IngredientData.LiquidParticleSystem.IsValid())
            {
                UNiagaraSystem* NiagaraSystem = Instance.IngredientData.LiquidParticleSystem.LoadSynchronous();
                if (NiagaraSystem && Owner)
                {
                    UNiagaraComponent* NiagaraComp = NewObject<UNiagaraComponent>(Owner, UNiagaraComponent::StaticClass(), NAME_None, RF_Transient);
                    if (NiagaraComp)
                    {
                        NiagaraComp->SetAsset(NiagaraSystem);
                        NiagaraComp->SetAutoActivate(true);
                        NiagaraComp->RegisterComponent();
                        NiagaraComp->AttachToComponent(IngredientParent, FAttachmentTransformRules::KeepWorldTransform);
                        NiagaraComp->SetWorldLocation(WorldPos);
                        NiagaraComp->SetWorldRotation(WorldRot);
                        NiagaraComp->SetWorldScale3D(FVector(1.0f));
                        NiagaraComp->Activate(true);
                        PreviewLiquidComponents.Add(NiagaraComp);
                        SpawnedCount++;
                    }
                }
            }
            else
            {
                APUIngredientMesh* Spawned = World->SpawnActor<APUIngredientMesh>(MeshClass, WorldPos, WorldRot, SpawnParams);
                if (!Spawned)
                {
                    UE_LOG(LogDishPreview, Warning, TEXT("BuildFromDishData - Failed to spawn ingredient InstanceID %d"), Instance.InstanceID);
                    continue;
                }

                Spawned->InitializeWithIngredientInstance(Instance);

                if (UStaticMeshComponent* MeshComp = Spawned->FindComponentByClass<UStaticMeshComponent>())
                {
                    MeshComp->SetSimulatePhysics(false);
                    MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                }
                for (UActorComponent* Comp : Spawned->GetComponents())
                {
                    if (UProceduralMeshComponent* ProcMesh = Cast<UProceduralMeshComponent>(Comp))
                    {
                        ProcMesh->SetSimulatePhysics(false);
                        ProcMesh->SetEnableGravity(false);
                        ProcMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                        ProcMesh->SetMobility(EComponentMobility::Movable);
                        ProcMesh->DestroyPhysicsState();  // Remove physics body so component follows parent
                        ProcMesh->AttachToComponent(Spawned->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);  // Re-attach: disabling physics does NOT auto-reattach
                    }
                }

                Spawned->SetIngredientScale(EffectiveScale);
                Spawned->AttachToComponent(IngredientParent, FAttachmentTransformRules::KeepWorldTransform);
                PreviewIngredientMeshes.Add(Spawned);
                SpawnedCount++;
            }
        }
    }

    UE_LOG(LogDishPreview, Log, TEXT("BuildFromDishData - DONE: dish=%s, %d ingredients spawned"),
        DishMesh ? TEXT("yes") : TEXT("no"), SpawnedCount);
    if (bEnableDishPreviewDebug && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("[DishPreview] DONE: dish=%s, %d ingredients"), DishMesh ? TEXT("yes") : TEXT("no"), SpawnedCount));
    }

    bHasPreview = true;
}

void UPUDishPreviewComponent::ClearPreview()
{
    UE_LOG(LogDishPreview, Log, TEXT("ClearPreview - clearing %d ingredients, %d liquids"), PreviewIngredientMeshes.Num(), PreviewLiquidComponents.Num());
    if (bEnableDishPreviewDebug && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor(128, 128, 128), FString::Printf(TEXT("[DishPreview] ClearPreview - %d ingredients, %d liquids"), PreviewIngredientMeshes.Num(), PreviewLiquidComponents.Num()));
    }

    for (APUIngredientMesh* Mesh : PreviewIngredientMeshes)
    {
        if (Mesh && IsValid(Mesh))
        {
            Mesh->Destroy();
        }
    }
    PreviewIngredientMeshes.Empty();

    for (UNiagaraComponent* NiagaraComp : PreviewLiquidComponents)
    {
        if (NiagaraComp && IsValid(NiagaraComp))
        {
            NiagaraComp->Deactivate();
            NiagaraComp->DestroyComponent();
        }
    }
    PreviewLiquidComponents.Empty();

    if (DishMeshComponent)
    {
        DishMeshComponent->SetStaticMesh(nullptr);
        DishMeshComponent->SetVisibility(false);
    }

    bHasPreview = false;
}
