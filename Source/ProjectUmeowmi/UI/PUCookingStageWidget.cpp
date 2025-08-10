#include "PUCookingStageWidget.h"
#include "PUIngredientQuantityControl.h"
#include "PUPreparationCheckbox.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Input/Events.h"
#include "Kismet/GameplayStatics.h"

UPUCookingStageWidget::UPUCookingStageWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Widgets tick by default, no need to enable explicitly
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

void UPUCookingStageWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Update carousel rotation if needed
    if (bCarouselSpawned)
    {
        if (bIsRotating)
        {
            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::NativeTick - Carousel is rotating, updating..."));
        }
        UpdateCarouselRotation(InDeltaTime);
        
        // Update hover detection for orthographic cameras
        UpdateHoverDetection();
        
        // Handle mouse clicks
        HandleMouseClick();
    }
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
    
    // Set up click detection for the carousel
    SetupClickDetection();
    
    // Set up initial front indicator
    UpdateFrontIndicator();
    
    // Debug collision detection
    DebugCollisionDetection();
    
    // Set up multi-hit collision detection for orthographic cameras
    SetupMultiHitCollisionDetection();
    
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
    if (TargetIndex < 0 || TargetIndex >= SpawnedCookingImplements.Num())
    {
        return;
    }

    // Calculate the target rotation angle
    // We want the selected item to be at the front (0 degrees)
    float AngleStep = 360.0f / SpawnedCookingImplements.Num();
    TargetRotationAngle = -(TargetIndex * AngleStep); // Negative because we want to rotate the carousel, not the item

    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::RotateCarouselToSelection - Rotating carousel to bring implement %d to front (angle: %.2f)"), 
        TargetIndex, TargetRotationAngle);

    // Start rotation
    bIsRotating = true;
    RotationProgress = 0.0f;
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::RotateCarouselToSelection - Rotation started: bIsRotating=%s, Progress=%.3f"), 
        bIsRotating ? TEXT("TRUE") : TEXT("FALSE"), RotationProgress);
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
                // Enable mouse over events with better collision for orthographic cameras
                MeshComponent->SetNotifyRigidBodyCollision(false);
                MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
                MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
                MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
                MeshComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
                MeshComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
                MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
                
                // Make sure the collision bounds are large enough for mouse detection
                MeshComponent->SetCollisionObjectType(ECC_WorldStatic);
                
                // For orthographic cameras, we need to ensure collision bounds are large enough
                // and that the collision doesn't block other objects behind
                MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);

                // Bind hover events
                MeshComponent->OnBeginCursorOver.AddDynamic(this, &UPUCookingStageWidget::OnImplementHoverBegin);
                MeshComponent->OnEndCursorOver.AddDynamic(this, &UPUCookingStageWidget::OnImplementHoverEnd);

                // Bind click events
                MeshComponent->OnClicked.AddDynamic(this, &UPUCookingStageWidget::OnImplementClicked);

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

void UPUCookingStageWidget::SetupClickDetection()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetupClickDetection - Setting up click detection for %d implements"), 
        SpawnedCookingImplements.Num());

    // Click detection is already set up in SetupHoverDetection
    // This function is here for future expansion if needed
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetupClickDetection - Click detection setup complete"));
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
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ PUCookingStageWidget::OnImplementHoverBegin - Could not find implement index for component: %s"), 
            TouchedComponent ? *TouchedComponent->GetName() : TEXT("NULL"));
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

void UPUCookingStageWidget::OnImplementClicked(UPrimitiveComponent* ClickedComponent, FKey ButtonPressed)
{
    int32 ImplementIndex = GetImplementIndexFromComponent(ClickedComponent);
    if (ImplementIndex >= 0)
    {
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::OnImplementClicked - Implement %d clicked with button: %s"), 
            ImplementIndex, *ButtonPressed.ToString());
        
        SelectCookingImplement(ImplementIndex);
    }
}



void UPUCookingStageWidget::UpdateCarouselRotation(float DeltaTime)
{
    if (!bIsRotating)
    {
        return;
    }

    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateCarouselRotation - Rotating: Progress=%.3f, CurrentAngle=%.2f, TargetAngle=%.2f"), 
        RotationProgress, CurrentRotationAngle, TargetRotationAngle);

    // Update rotation progress
    float ProgressIncrement = DeltaTime * RotationSpeed;
    RotationProgress += ProgressIncrement;
    
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateCarouselRotation - Progress increment: %.4f, Total progress: %.3f"), 
        ProgressIncrement, RotationProgress);
    
    if (RotationProgress >= 1.0f)
    {
        // Rotation complete
        RotationProgress = 1.0f;
        bIsRotating = false;
        CurrentRotationAngle = TargetRotationAngle;
        
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateCarouselRotation - Carousel rotation complete"));
    }
    else
    {
        // Interpolate rotation
        float Alpha = FMath::SmoothStep(0.0f, 1.0f, RotationProgress);
        CurrentRotationAngle = FMath::Lerp(CurrentRotationAngle, TargetRotationAngle, Alpha);
    }

    // Apply rotation to all implements
    for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
    {
        AStaticMeshActor* ImplementActor = SpawnedCookingImplements[i];
        if (ImplementActor)
        {
            // Calculate the base position for this implement
            FVector BasePosition = CalculateImplementPosition(i, SpawnedCookingImplements.Num());
            
            // Apply carousel rotation
            FVector RotatedPosition = CarouselCenter + FQuat(FVector::UpVector, FMath::DegreesToRadians(CurrentRotationAngle))
                .RotateVector(BasePosition - CarouselCenter);
            
            // Set the new position
            ImplementActor->SetActorLocation(RotatedPosition);
            
            // Update rotation to face outward
            FRotator BaseRotation = CalculateImplementRotation(i, SpawnedCookingImplements.Num());
            FRotator RotatedRotation = BaseRotation + FRotator(0.0f, CurrentRotationAngle, 0.0f);
            ImplementActor->SetActorRotation(RotatedRotation);
            
            UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateCarouselRotation - Implement %d: BasePos=%s, RotatedPos=%s, Angle=%.2f"), 
                i, *BasePosition.ToString(), *RotatedPosition.ToString(), CurrentRotationAngle);
        }
    }

    // Update front indicator after rotation
    UpdateFrontIndicator();
}

void UPUCookingStageWidget::ApplySelectionVisualEffect(int32 ImplementIndex, bool bApply)
{
    if (ImplementIndex < 0 || ImplementIndex >= SpawnedCookingImplements.Num())
    {
        return;
    }

    AStaticMeshActor* ImplementActor = SpawnedCookingImplements[ImplementIndex];
    if (ImplementActor)
    {
        UStaticMeshComponent* MeshComponent = ImplementActor->GetStaticMeshComponent();
        if (MeshComponent)
        {
            if (bApply)
            {
                // Apply selection effect - gentle up/down bobbing
                // We'll implement this with a simple scale effect for now
                FVector CurrentScale = MeshComponent->GetRelativeScale3D();
                MeshComponent->SetRelativeScale3D(CurrentScale * 1.2f); // Make selected item 20% bigger
                
                UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::ApplySelectionVisualEffect - Applied selection effect to implement %d"), ImplementIndex);
            }
            else
            {
                // Remove selection effect
                FVector CurrentScale = MeshComponent->GetRelativeScale3D();
                MeshComponent->SetRelativeScale3D(CurrentScale / 1.2f); // Restore original scale
                
                UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::ApplySelectionVisualEffect - Removed selection effect from implement %d"), ImplementIndex);
            }
        }
    }
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

void UPUCookingStageWidget::UpdateFrontIndicator()
{
    int32 FrontIndex = GetFrontImplementIndex();
    
    // Reset all implements to normal scale and height
    for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
    {
        AStaticMeshActor* ImplementActor = SpawnedCookingImplements[i];
        if (ImplementActor)
        {
            UStaticMeshComponent* MeshComponent = ImplementActor->GetStaticMeshComponent();
            if (MeshComponent)
            {
                // Reset to base scale (2.0f as set in SpawnCookingCarousel)
                MeshComponent->SetRelativeScale3D(FVector(2.0f, 2.0f, 2.0f));
                
                // Reset to base height
                FVector CurrentLocation = ImplementActor->GetActorLocation();
                CurrentLocation.Z = CarouselCenter.Z + CarouselHeight;
                ImplementActor->SetActorLocation(CurrentLocation);
            }
        }
    }
    
    // Apply front indicator to the front item
    if (FrontIndex >= 0 && FrontIndex < SpawnedCookingImplements.Num())
    {
        AStaticMeshActor* FrontActor = SpawnedCookingImplements[FrontIndex];
        if (FrontActor)
        {
            UStaticMeshComponent* MeshComponent = FrontActor->GetStaticMeshComponent();
            if (MeshComponent)
            {
                // Scale up the front item
                MeshComponent->SetRelativeScale3D(FVector(2.0f * FrontItemScale, 2.0f * FrontItemScale, 2.0f * FrontItemScale));
                
                // Raise the front item
                FVector CurrentLocation = FrontActor->GetActorLocation();
                CurrentLocation.Z += FrontItemHeight;
                FrontActor->SetActorLocation(CurrentLocation);
                
                UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateFrontIndicator - Front item is implement %d (scale: %.2f, height: %.2f)"), 
                    FrontIndex, FrontItemScale, FrontItemHeight);
            }
        }
    }
}

int32 UPUCookingStageWidget::GetFrontImplementIndex() const
{
    if (SpawnedCookingImplements.Num() == 0)
    {
        return -1;
    }
    
    // The front item is the one at 0 degrees (facing the camera)
    // We need to calculate which implement is currently at the front based on the carousel rotation
    
    // Calculate the angle of each implement relative to the front
    float AngleStep = 360.0f / SpawnedCookingImplements.Num();
    
    for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
    {
        // Calculate the current angle of this implement
        float ImplementAngle = (i * AngleStep) + CurrentRotationAngle;
        
        // Normalize to 0-360 range
        ImplementAngle = FMath::Fmod(ImplementAngle + 360.0f, 360.0f);
        
        // Check if this implement is at the front (within a small tolerance)
        if (FMath::Abs(ImplementAngle) < 10.0f || FMath::Abs(ImplementAngle - 360.0f) < 10.0f)
        {
            return i;
        }
    }
    
    // If no exact match, return the closest one
    float ClosestAngle = 360.0f;
    int32 ClosestIndex = 0;
    
    for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
    {
        float ImplementAngle = (i * AngleStep) + CurrentRotationAngle;
        ImplementAngle = FMath::Fmod(ImplementAngle + 360.0f, 360.0f);
        
        float DistanceToFront = FMath::Min(FMath::Abs(ImplementAngle), FMath::Abs(ImplementAngle - 360.0f));
        if (DistanceToFront < ClosestAngle)
        {
            ClosestAngle = DistanceToFront;
            ClosestIndex = i;
        }
    }
    
    return ClosestIndex;
}

void UPUCookingStageWidget::DebugCollisionDetection()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::DebugCollisionDetection - Checking collision setup for %d implements"), 
        SpawnedCookingImplements.Num());

    for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
    {
        AStaticMeshActor* ImplementActor = SpawnedCookingImplements[i];
        if (ImplementActor)
        {
            UStaticMeshComponent* MeshComponent = ImplementActor->GetStaticMeshComponent();
            if (MeshComponent)
            {
                FVector Location = ImplementActor->GetActorLocation();
                FVector Bounds = MeshComponent->Bounds.BoxExtent;
                
                UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::DebugCollisionDetection - Implement %d: Location=%s, Bounds=%s, CollisionEnabled=%s"), 
                    i, *Location.ToString(), *Bounds.ToString(), 
                    MeshComponent->GetCollisionEnabled() == ECollisionEnabled::QueryOnly ? TEXT("QueryOnly") : TEXT("Other"));
            }
        }
    }
} 

void UPUCookingStageWidget::SetupMultiHitCollisionDetection()
{
    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetupMultiHitCollisionDetection - Setting up multi-hit collision detection"));
    
    // For orthographic cameras, we need to disable the automatic collision detection
    // and implement our own raycast system that can detect all objects
    for (AStaticMeshActor* ImplementActor : SpawnedCookingImplements)
    {
        if (ImplementActor)
        {
            UStaticMeshComponent* MeshComponent = ImplementActor->GetStaticMeshComponent();
            if (MeshComponent)
            {
                // Disable automatic cursor over events for orthographic cameras
                MeshComponent->OnBeginCursorOver.RemoveAll(this);
                MeshComponent->OnEndCursorOver.RemoveAll(this);
                
                UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::SetupMultiHitCollisionDetection - Disabled automatic collision detection for orthographic camera"));
            }
        }
    }
}

void UPUCookingStageWidget::OnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // For orthographic cameras, we need to implement custom hover detection
    UpdateHoverDetection();
}

void UPUCookingStageWidget::UpdateHoverDetection()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return;
    }
    
    // Get mouse position
    FVector2D MousePosition;
    PC->GetMousePosition(MousePosition.X, MousePosition.Y);
    
    // Check if mouse is in carousel bounds
    if (!IsPointInCarouselBounds(MousePosition))
    {
        if (HoveredImplementIndex >= 0)
        {
            ShowHoverText(HoveredImplementIndex, false);
            ApplyHoverVisualEffect(HoveredImplementIndex, false);
            HoveredImplementIndex = -1;
        }
        return;
    }
    
    // For orthographic cameras, we need to check distance to each implement
    // and find the closest one that's actually visible
    float ClosestDistance = 10000.0f;
    int32 ClosestIndex = -1;
    
    for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
    {
        AStaticMeshActor* ImplementActor = SpawnedCookingImplements[i];
        if (ImplementActor)
        {
            // Convert world position to screen position
            FVector WorldLocation = ImplementActor->GetActorLocation();
            FVector2D ScreenLocation;
            if (PC->ProjectWorldLocationToScreen(WorldLocation, ScreenLocation))
            {
                // Calculate distance from mouse to this implement on screen
                float Distance = FVector2D::Distance(MousePosition, ScreenLocation);
                
                // Debug: Log distances for troubleshooting
                static float DebugTimer = 0.0f;
                DebugTimer += 0.016f; // Assuming 60fps
                if (DebugTimer > 1.0f) // Log once per second
                {
                    UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateHoverDetection - Implement %d: Mouse=(%.1f,%.1f), Screen=(%.1f,%.1f), Distance=%.1f"), 
                        i, MousePosition.X, MousePosition.Y, ScreenLocation.X, ScreenLocation.Y, Distance);
                    DebugTimer = 0.0f;
                }
                
                // Check if this implement is closer and within reasonable distance
                if (Distance < ClosestDistance && Distance < 120.0f) // 120 pixel radius for hover
                {
                    ClosestDistance = Distance;
                    ClosestIndex = i;
                }
            }
        }
    }
    
    // Update hover state
    if (ClosestIndex >= 0 && ClosestIndex != HoveredImplementIndex)
    {
        // Clear previous hover
        if (HoveredImplementIndex >= 0)
        {
            ShowHoverText(HoveredImplementIndex, false);
            ApplyHoverVisualEffect(HoveredImplementIndex, false);
        }
        
        // Set new hover
        HoveredImplementIndex = ClosestIndex;
        ShowHoverText(HoveredImplementIndex, true);
        ApplyHoverVisualEffect(HoveredImplementIndex, true);
        
        UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::UpdateHoverDetection - Hovering over implement %d (distance: %.2f)"), 
            ClosestIndex, ClosestDistance);
    }
}

bool UPUCookingStageWidget::IsPointInCarouselBounds(const FVector2D& ScreenPoint) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }
    
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return false;
    }
    
    // Convert carousel center to screen position
    FVector2D CarouselScreenLocation;
    if (PC->ProjectWorldLocationToScreen(CarouselCenter, CarouselScreenLocation))
    {
        // Check if mouse is within carousel radius (converted to screen space)
        float Distance = FVector2D::Distance(ScreenPoint, CarouselScreenLocation);
        return Distance < 300.0f; // Approximate carousel radius in screen space
    }
    
    return false;
}

void UPUCookingStageWidget::HandleMouseClick()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return;
    }
    
    // Check if left mouse button is pressed
    if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
    {
        // Find which implement is currently being hovered
        for (int32 i = 0; i < SpawnedCookingImplements.Num(); ++i)
        {
            if (IsMouseOverImplement(i))
            {
                UE_LOG(LogTemp, Display, TEXT("🍳 PUCookingStageWidget::HandleMouseClick - Clicked on implement %d"), i);
                SelectCookingImplement(i);
                break;
            }
        }
    }
}

bool UPUCookingStageWidget::IsMouseOverImplement(int32 ImplementIndex) const
{
    if (ImplementIndex < 0 || ImplementIndex >= SpawnedCookingImplements.Num())
    {
        return false;
    }
    
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }
    
    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return false;
    }
    
    // Get mouse position
    FVector2D MousePosition;
    PC->GetMousePosition(MousePosition.X, MousePosition.Y);
    
    // Get implement position
    AStaticMeshActor* ImplementActor = SpawnedCookingImplements[ImplementIndex];
    if (!ImplementActor)
    {
        return false;
    }
    
    // Convert world position to screen position
    FVector WorldLocation = ImplementActor->GetActorLocation();
    FVector2D ScreenLocation;
    if (PC->ProjectWorldLocationToScreen(WorldLocation, ScreenLocation))
    {
        // Calculate distance from mouse to this implement on screen
        float Distance = FVector2D::Distance(MousePosition, ScreenLocation);
        
        // Check if mouse is within click radius (smaller than hover radius for precision)
        return Distance < 80.0f; // 80 pixel radius for clicks
    }
    
    return false;
} 