#include "PULevelSpawnPoint.h"
#include "../PUProjectUmeowmiGameInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

APULevelSpawnPoint::APULevelSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create root scene component
	USceneComponent* RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootSceneComponent;

	// Create visual mesh (cylinder for visibility in editor)
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetHiddenInGame(true); // Only visible in editor

	// Try to set a default mesh (cylinder)
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CylinderMesh.Object);
		VisualMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.2f));
		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	}

	// Create direction arrow
	DirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("DirectionArrow"));
	DirectionArrow->SetupAttachment(RootComponent);
	DirectionArrow->SetArrowColor(FLinearColor::Green);
	DirectionArrow->SetArrowSize(2.0f);
	DirectionArrow->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));

	// Set default spawn point tag
	SpawnPointTag = TEXT("Default");
	SpawnPointName = TEXT("Default Spawn Point");
}

void APULevelSpawnPoint::BeginPlay()
{
	Super::BeginPlay();

	// Notify GameInstance that the level is ready so it can position the player
	if (UWorld* World = GetWorld())
	{
		if (UPUProjectUmeowmiGameInstance* GameInstance = World->GetGameInstance<UPUProjectUmeowmiGameInstance>())
		{
			GameInstance->OnLevelLoaded();
		}
	}
}

#if WITH_EDITOR
void APULevelSpawnPoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Update actor label when spawn point tag changes
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(APULevelSpawnPoint, SpawnPointTag))
	{
		FString NewLabel = FString::Printf(TEXT("SpawnPoint_%s"), *SpawnPointTag.ToString());
		SetActorLabel(NewLabel);
	}
}
#endif

