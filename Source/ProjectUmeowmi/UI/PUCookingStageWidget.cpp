#include "PUCookingStageWidget.h"
#include "PUIngredientQuantityControl.h"
#include "PUPreparationCheckbox.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/GameplayStatics.h"

UPUCookingStageWidget::UPUCookingStageWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
}

void UPUCookingStageWidget::NativeConstruct()
{
    Super::NativeConstruct();
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::NativeConstruct - Cooking stage widget constructed"));
}

void UPUCookingStageWidget::NativeDestruct()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::NativeDestruct - Cooking stage widget destructing"));
    
    Super::NativeDestruct();
}

void UPUCookingStageWidget::InitializeCookingStage(const FPUDishBase& DishData, const FVector& CookingStationLocation)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::InitializeCookingStage - Initializing cooking stage"));
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::InitializeCookingStage - Station location passed: %s"), 
        *CookingStationLocation.ToString());
    
    // Initialize the current dish data
    CurrentDishData = DishData;
    
    // Set carousel center based on cooking station location
    if (CookingStationLocation != FVector::ZeroVector)
    {
        CarouselCenter = CookingStationLocation + FVector(0.0f, 0.0f, 100.0f); // Offset above the station
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::InitializeCookingStage - Carousel center set to: %s"), 
            *CarouselCenter.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::InitializeCookingStage - Station location is zero, using default carousel center"));
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::InitializeCookingStage - Cooking stage initialized for dish: %s"), 
        *CurrentDishData.DisplayName.ToString());
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::InitializeCookingStage - Selected ingredients: %d"), 
        CurrentDishData.IngredientInstances.Num());
    
    // Spawn the cooking implement carousel
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::InitializeCookingStage - About to spawn carousel at center: %s"), 
        *CarouselCenter.ToString());
    SpawnCookingCarousel();
    
    // Create quantity controls for the selected ingredients
    //CreateQuantityControlsForSelectedIngredients();
    
    // Call Blueprint event
    OnCookingStageInitialized(CurrentDishData);
}

void UPUCookingStageWidget::CreateQuantityControlsForSelectedIngredients()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateQuantityControlsForSelectedIngredients - Creating quantity controls"));
    
    if (!QuantityControlClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::CreateQuantityControlsForSelectedIngredients - No quantity control class set"));
        return;
    }
    
    // Create quantity controls for each ingredient instance
    for (const FIngredientInstance& IngredientInstance : CurrentDishData.IngredientInstances)
    {
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateQuantityControlsForSelectedIngredients - Creating control for: %s"), 
            *IngredientInstance.IngredientData.DisplayName.ToString());
        
        // Create quantity control widget
        if (UPUIngredientQuantityControl* QuantityControl = CreateWidget<UPUIngredientQuantityControl>(this, QuantityControlClass))
        {
            QuantityControl->SetIngredientInstance(IngredientInstance);
            QuantityControl->SetPreparationCheckboxClass(PreparationCheckboxClass);
            
            // Bind events
            QuantityControl->OnQuantityControlChanged.AddDynamic(this, &UPUCookingStageWidget::OnQuantityControlChanged);
            QuantityControl->OnQuantityControlRemoved.AddDynamic(this, &UPUCookingStageWidget::OnQuantityControlRemoved);
            
            // Add to viewport (Blueprint will handle the actual placement)
            QuantityControl->AddToViewport();
            
            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateQuantityControlsForSelectedIngredients - Quantity control created for instance: %d"), 
                IngredientInstance.InstanceID);
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::CreateQuantityControlsForSelectedIngredients - Created %d quantity controls"), 
        CurrentDishData.IngredientInstances.Num());
}

void UPUCookingStageWidget::OnQuantityControlChanged(const FIngredientInstance& IngredientInstance)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnQuantityControlChanged - Quantity control changed for instance: %d"), 
        IngredientInstance.InstanceID);
    
    // Update the ingredient instance in the current dish data
    for (FIngredientInstance& Instance : CurrentDishData.IngredientInstances)
    {
        if (Instance.InstanceID == IngredientInstance.InstanceID)
        {
            Instance = IngredientInstance;
            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnQuantityControlChanged - Updated instance in dish data"));
            break;
        }
    }
}

void UPUCookingStageWidget::OnQuantityControlRemoved(int32 InstanceID, UPUIngredientQuantityControl* QuantityControlWidget)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnQuantityControlRemoved - Quantity control removed for instance: %d"), InstanceID);
    
    // Remove the ingredient instance from the current dish data
    CurrentDishData.IngredientInstances.RemoveAll([InstanceID](const FIngredientInstance& Instance) {
        return Instance.InstanceID == InstanceID;
    });
    
    // Remove the widget from viewport
    if (QuantityControlWidget)
    {
        QuantityControlWidget->RemoveFromParent();
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnQuantityControlRemoved - Widget removed from viewport"));
    }
}

void UPUCookingStageWidget::FinishCookingAndStartPlating()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::FinishCookingAndStartPlating - Finishing cooking and starting plating"));
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::FinishCookingAndStartPlating - Final dish has %d ingredients"), 
        CurrentDishData.IngredientInstances.Num());
    
    // Destroy carousel before finishing
    DestroyCookingCarousel();
    
    // Call Blueprint event
    OnCookingStageCompleted(CurrentDishData);
    
    // Broadcast the cooking completed event
    OnCookingCompleted.Broadcast(CurrentDishData);
    
    // Remove this widget from viewport
    RemoveFromParent();
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::FinishCookingAndStartPlating - Cooking stage completed"));
}

// Carousel Functions
void UPUCookingStageWidget::SpawnCookingCarousel()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SpawnCookingCarousel - Spawning cooking implement carousel"));
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SpawnCookingCarousel - Carousel center: %s, Radius: %.2f, Height: %.2f"), 
        *CarouselCenter.ToString(), CarouselRadius, CarouselHeight);
    
    // Clear any existing carousel
    DestroyCookingCarousel();
    
    if (CookingImplementMeshes.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::SpawnCookingCarousel - No cooking implement meshes defined"));
        return;
    }
    
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ PUCookingStageWidget::SpawnCookingCarousel - No world available"));
        return;
    }
    
    // Spawn cooking implement actors
    for (int32 i = 0; i < CookingImplementMeshes.Num(); ++i)
    {
        if (UStaticMesh* Mesh = CookingImplementMeshes[i])
        {
            // Calculate position for this implement
            FVector Position = CalculateImplementPosition(i, CookingImplementMeshes.Num());
            FRotator Rotation = CalculateImplementRotation(i, CookingImplementMeshes.Num());
            
            // Spawn the actor
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            
            AStaticMeshActor* ImplementActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Position, Rotation, SpawnParams);
            
            if (ImplementActor)
            {
                // Set the static mesh
                UStaticMeshComponent* MeshComponent = ImplementActor->GetStaticMeshComponent();
                if (MeshComponent)
                {
                    // Set mobility to movable so we can change the mesh at runtime
                    MeshComponent->SetMobility(EComponentMobility::Movable);
                    
                    MeshComponent->SetStaticMesh(Mesh);
                    
                    // Make the mesh 2 times bigger
                    MeshComponent->SetWorldScale3D(FVector(2.0f, 2.0f, 2.0f));
                    
                    // Set up collision for interaction
                    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                    MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
                    MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
                    MeshComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
                }
                
                // Add to our array
                SpawnedCookingImplements.Add(ImplementActor);
                
                UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SpawnCookingCarousel - Spawned implement %d at position %s"), 
                    i, *Position.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("❌ PUCookingStageWidget::SpawnCookingCarousel - Failed to spawn implement %d"), i);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::SpawnCookingCarousel - Invalid mesh at index %d"), i);
        }
    }
    
    bCarouselSpawned = true;
    SelectedImplementIndex = 0;
    
    // Set up hover detection for the carousel
    SetupHoverDetection();
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SpawnCookingCarousel - Carousel spawned with %d implements"), 
        SpawnedCookingImplements.Num());
}

void UPUCookingStageWidget::DestroyCookingCarousel()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::DestroyCookingCarousel - Destroying cooking implement carousel"));
    
    // Destroy all spawned actors
    for (AStaticMeshActor* Actor : SpawnedCookingImplements)
    {
        if (Actor && IsValid(Actor))
        {
            Actor->Destroy();
        }
    }
    
    SpawnedCookingImplements.Empty();
    bCarouselSpawned = false;
    SelectedImplementIndex = 0;
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::DestroyCookingCarousel - Carousel destroyed"));
}

void UPUCookingStageWidget::SelectCookingImplement(int32 ImplementIndex)
{
    if (!bCarouselSpawned || SpawnedCookingImplements.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::SelectCookingImplement - Carousel not spawned or no implements available"));
        return;
    }
    
    if (ImplementIndex < 0 || ImplementIndex >= SpawnedCookingImplements.Num())
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::SelectCookingImplement - Invalid implement index: %d"), ImplementIndex);
        return;
    }
    
    if (SelectedImplementIndex == ImplementIndex)
    {
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SelectCookingImplement - Implement %d already selected"), ImplementIndex);
        return;
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SelectCookingImplement - Selecting implement %d (was %d)"), 
        ImplementIndex, SelectedImplementIndex);
    
    SelectedImplementIndex = ImplementIndex;
    
    // Rotate carousel to bring selected item to front
    RotateCarouselToSelection(ImplementIndex);
    
    // Broadcast event
    OnCookingImplementSelected.Broadcast(ImplementIndex);
}

void UPUCookingStageWidget::PositionCookingImplements()
{
    if (!bCarouselSpawned || SpawnedCookingImplements.Num() == 0)
    {
        return;
    }
    
    for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
    {
        if (AStaticMeshActor* Actor = SpawnedCookingImplements[i])
        {
            FVector Position = CalculateImplementPosition(i, SpawnedCookingImplements.Num());
            FRotator Rotation = CalculateImplementRotation(i, SpawnedCookingImplements.Num());
            
            Actor->SetActorLocation(Position);
            Actor->SetActorRotation(Rotation);
        }
    }
}

void UPUCookingStageWidget::RotateCarouselToSelection(int32 TargetIndex)
{
    if (!bCarouselSpawned || SpawnedCookingImplements.Num() == 0)
    {
        return;
    }
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::RotateCarouselToSelection - Rotating carousel to selection %d"), TargetIndex);
    
    // For now, just reposition all implements
    // Later we can add smooth animation
    PositionCookingImplements();
}

FVector UPUCookingStageWidget::CalculateImplementPosition(int32 Index, int32 TotalCount) const
{
    if (TotalCount <= 0)
    {
        return CarouselCenter;
    }
    
    // Calculate angle for this implement
    float AngleInRadians = FMath::DegreesToRadians((360.0f / TotalCount) * Index);
    
    // Calculate position on circle
    float X = CarouselCenter.X + (CarouselRadius * FMath::Cos(AngleInRadians));
    float Y = CarouselCenter.Y + (CarouselRadius * FMath::Sin(AngleInRadians));
    float Z = CarouselCenter.Z + CarouselHeight;
    
    FVector Position = FVector(X, Y, Z);
    
    UE_LOG(LogTemp, Display, TEXT("🍳 CalculateImplementPosition - Index: %d, Total: %d, Angle: %.2f°, Center: %s, Radius: %.2f, Position: %s"), 
        Index, TotalCount, (360.0f / TotalCount) * Index, *CarouselCenter.ToString(), CarouselRadius, *Position.ToString());
    
    return Position;
}

FRotator UPUCookingStageWidget::CalculateImplementRotation(int32 Index, int32 TotalCount) const
{
    if (TotalCount <= 0)
    {
        return FRotator::ZeroRotator;
    }
    
    // Calculate angle for this implement
    float AngleInDegrees = (360.0f / TotalCount) * Index;
    
    // Make implements face toward the center of the carousel
    // Add 180 degrees so they face inward
    float Yaw = AngleInDegrees + 180.0f;
    
    return FRotator(0.0f, Yaw, 0.0f);
}

void UPUCookingStageWidget::SetCarouselCenter(const FVector& NewCenter)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetCarouselCenter - Setting carousel center to: %s"), 
        *NewCenter.ToString());
    
    CarouselCenter = NewCenter;
    
    // If carousel is already spawned, reposition it
    if (bCarouselSpawned)
    {
        PositionCookingImplements();
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetCarouselCenter - Repositioned existing carousel"));
    }
}

void UPUCookingStageWidget::SetCookingStationLocation(const FVector& StationLocation)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetCookingStationLocation - Setting station location: %s"), 
        *StationLocation.ToString());
    
    // Set carousel center based on cooking station location
    CarouselCenter = StationLocation + FVector(0.0f, 0.0f, 100.0f); // Offset above the station
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetCookingStationLocation - Carousel center set to: %s"), 
        *CarouselCenter.ToString());
    
    // If carousel is already spawned, reposition it
    if (bCarouselSpawned)
    {
        PositionCookingImplements();
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetCookingStationLocation - Repositioned existing carousel"));
    }
}

void UPUCookingStageWidget::FindAndSetNearestCookingStation()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::FindAndSetNearestCookingStation - No world available"));
        return;
    }
    
    // Get player character location
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::FindAndSetNearestCookingStation - No player controller found"));
        return;
    }
    
    APawn* PlayerPawn = PC->GetPawn();
    if (!PlayerPawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::FindAndSetNearestCookingStation - No player pawn found"));
        return;
    }
    
    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    
    // Find all actors with "CookingStation" tag
    TArray<AActor*> CookingStations;
    UGameplayStatics::GetAllActorsWithTag(World, FName("CookingStation"), CookingStations);
    
    if (CookingStations.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::FindAndSetNearestCookingStation - No cooking stations found with 'CookingStation' tag"));
        return;
    }
    
    // Find the nearest cooking station
    AActor* NearestStation = nullptr;
    float NearestDistance = MAX_FLT;
    
    for (AActor* Station : CookingStations)
    {
        if (Station)
        {
            float Distance = FVector::Dist(PlayerLocation, Station->GetActorLocation());
            if (Distance < NearestDistance)
            {
                NearestDistance = Distance;
                NearestStation = Station;
            }
        }
    }
    
    if (NearestStation)
    {
        FVector StationLocation = NearestStation->GetActorLocation();
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::FindAndSetNearestCookingStation - Found nearest cooking station at: %s"), 
            *StationLocation.ToString());
        
        // Set carousel center based on cooking station location
        CarouselCenter = StationLocation + FVector(0.0f, 0.0f, 100.0f); // Offset above the station
        
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::FindAndSetNearestCookingStation - Carousel center set to: %s"), 
            *CarouselCenter.ToString());
        
        // If carousel is already spawned, reposition it
        if (bCarouselSpawned)
        {
            PositionCookingImplements();
            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::FindAndSetNearestCookingStation - Repositioned existing carousel"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::FindAndSetNearestCookingStation - No valid cooking station found"));
    }
}

// Hover Detection Functions
void UPUCookingStageWidget::SetupHoverDetection()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetupHoverDetection - Setting up hover detection for %d implements"), 
        SpawnedCookingImplements.Num());

    // Clear existing hover text components
    if (HoverTextComponents.Num() > 0)
    {
        for (UWidgetComponent* TextComponent : HoverTextComponents)
        {
            if (TextComponent)
            {
                TextComponent->DestroyComponent();
            }
        }
    }
    HoverTextComponents.Empty();

    // Set up hover detection for each implement
    for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
    {
        AStaticMeshActor* ImplementActor = SpawnedCookingImplements[i];
        if (ImplementActor)
        {
            UStaticMeshComponent* MeshComponent = ImplementActor->GetStaticMeshComponent();
            if (MeshComponent)
            {
                // Enable mouse over events
                MeshComponent->SetNotifyRigidBodyCollision(false);
                MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
                MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
                MeshComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);

                // Bind hover events
                MeshComponent->OnBeginCursorOver.AddDynamic(this, &UPUCookingStageWidget::OnImplementHoverBegin);
                MeshComponent->OnEndCursorOver.AddDynamic(this, &UPUCookingStageWidget::OnImplementHoverEnd);

                // Create hover text component
                if (HoverTextWidgetClass)
                {
                    UWidgetComponent* TextComponent = NewObject<UWidgetComponent>(ImplementActor);
                    if (TextComponent)
                    {
                        TextComponent->SetupAttachment(MeshComponent);
                        TextComponent->SetWidgetClass(HoverTextWidgetClass);
                        
                        // For orthographic cameras, use larger draw size
                        TextComponent->SetDrawSize(FVector2D(300.0f, 100.0f));
                        TextComponent->SetRelativeLocation(FVector(0.0f, 0.0f, HoverTextOffset));
                        TextComponent->SetVisibility(false);
                        
                        // Important for orthographic cameras - make it always face the camera
                        TextComponent->SetWidgetSpace(EWidgetSpace::World);
                        TextComponent->SetPivot(FVector2D(0.5f, 0.5f));
                        
                        TextComponent->RegisterComponent();

                        HoverTextComponents.Add(TextComponent);
                        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetupHoverDetection - Created hover text component for implement %d (orthographic optimized)"), i);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::SetupHoverDetection - Failed to create text component for implement %d"), i);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::SetupHoverDetection - No HoverTextWidgetClass set for implement %d"), i);
                }
            }
        }
    }

    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetupHoverDetection - Hover detection setup complete"));
}

void UPUCookingStageWidget::OnImplementHoverBegin(UPrimitiveComponent* TouchedComponent)
{
    int32 ImplementIndex = GetImplementIndexFromComponent(TouchedComponent);
    if (ImplementIndex >= 0)
    {
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnImplementHoverBegin - Implement %d hover begin"), ImplementIndex);
        
        HoveredImplementIndex = ImplementIndex;
        ShowHoverText(ImplementIndex, true);
        ApplyHoverVisualEffect(ImplementIndex, true);
    }
}

void UPUCookingStageWidget::OnImplementHoverEnd(UPrimitiveComponent* TouchedComponent)
{
    int32 ImplementIndex = GetImplementIndexFromComponent(TouchedComponent);
    if (ImplementIndex >= 0)
    {
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnImplementHoverEnd - Implement %d hover end"), ImplementIndex);
        
        if (HoveredImplementIndex == ImplementIndex)
        {
            HoveredImplementIndex = -1;
        }
        ShowHoverText(ImplementIndex, false);
        ApplyHoverVisualEffect(ImplementIndex, false);
    }
}

int32 UPUCookingStageWidget::GetImplementIndexFromComponent(UPrimitiveComponent* Component) const
{
    if (!Component)
    {
        return -1;
    }

    // Find which implement this component belongs to
    for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
    {
        AStaticMeshActor* ImplementActor = SpawnedCookingImplements[i];
        if (ImplementActor && ImplementActor->GetStaticMeshComponent() == Component)
        {
            return i;
        }
    }

    return -1;
}

void UPUCookingStageWidget::ShowHoverText(int32 ImplementIndex, bool bShow)
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::ShowHoverText - Attempting to %s hover text for implement %d (Total components: %d)"), 
        bShow ? TEXT("show") : TEXT("hide"), ImplementIndex, HoverTextComponents.Num());
    
    if (ImplementIndex >= 0 && ImplementIndex < HoverTextComponents.Num())
    {
        UWidgetComponent* TextComponent = HoverTextComponents[ImplementIndex];
        if (TextComponent)
        {
            TextComponent->SetVisibility(bShow);
            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::ShowHoverText - %s hover text for implement %d"), 
                bShow ? TEXT("Showing") : TEXT("Hiding"), ImplementIndex);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::ShowHoverText - Text component is null for implement %d"), ImplementIndex);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::ShowHoverText - Invalid implement index %d (valid range: 0-%d)"), 
            ImplementIndex, HoverTextComponents.Num() - 1);
    }
}

void UPUCookingStageWidget::ApplyHoverVisualEffect(int32 ImplementIndex, bool bApply)
{
    if (ImplementIndex >= 0 && ImplementIndex < SpawnedCookingImplements.Num())
    {
        AStaticMeshActor* ImplementActor = SpawnedCookingImplements[ImplementIndex];
        if (ImplementActor)
        {
            UStaticMeshComponent* MeshComponent = ImplementActor->GetStaticMeshComponent();
            if (MeshComponent)
            {
                if (bApply)
                {
                    // For orthographic cameras, use a more visible approach
                    // Scale up the mesh slightly to create a "glow" effect
                    FVector CurrentScale = MeshComponent->GetRelativeScale3D();
                    MeshComponent->SetRelativeScale3D(CurrentScale * 1.1f);
                    
                    // Also try custom depth (may work better with orthographic)
                    MeshComponent->SetRenderCustomDepth(true);
                    MeshComponent->SetCustomDepthStencilValue(1);
                    
                    // Set material parameter for glow
                    MeshComponent->SetScalarParameterValueOnMaterials(TEXT("GlowIntensity"), HoverGlowIntensity);
                    
                    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::ApplyHoverVisualEffect - Applied hover effect to implement %d (scaled up)"), ImplementIndex);
                }
                else
                {
                    // Restore original scale
                    FVector CurrentScale = MeshComponent->GetRelativeScale3D();
                    MeshComponent->SetRelativeScale3D(CurrentScale / 1.1f);
                    
                    // Remove custom depth
                    MeshComponent->SetRenderCustomDepth(false);
                    MeshComponent->SetScalarParameterValueOnMaterials(TEXT("GlowIntensity"), 0.0f);
                    
                    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::ApplyHoverVisualEffect - Removed hover effect from implement %d (scaled down)"), ImplementIndex);
                }
            }
        }
    }
} 