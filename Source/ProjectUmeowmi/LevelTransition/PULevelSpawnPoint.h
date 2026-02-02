#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PULevelSpawnPoint.generated.h"

/**
 * Actor that marks a spawn location in a level.
 * Used by the level transition system to position players when entering a level.
 */
UCLASS()
class PROJECTUMEOWMI_API APULevelSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	APULevelSpawnPoint();
	
	// Get the spawn point tag/ID
	UFUNCTION(BlueprintCallable, Category = "Spawn Point")
	FName GetSpawnPointTag() const { return SpawnPointTag; }

	// Set the spawn point tag/ID
	UFUNCTION(BlueprintCallable, Category = "Spawn Point")
	void SetSpawnPointTag(FName NewTag) { SpawnPointTag = NewTag; }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void BeginPlay() override;

	// Unique tag/ID for this spawn point (used to match with level transitions)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Point")
	FName SpawnPointTag = NAME_None;

	// Display name for editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Point")
	FString SpawnPointName = TEXT("Spawn Point");

	// Visual representation in editor
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UStaticMeshComponent* VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UArrowComponent* DirectionArrow;
};

