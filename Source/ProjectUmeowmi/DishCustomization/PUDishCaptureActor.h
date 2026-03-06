#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PUDishBase.h"
#include "PUDishCaptureActor.generated.h"

class APUIngredientMesh;
class USceneCaptureComponent2D;
class UStaticMeshComponent;

/**
 * Actor that captures a plated dish to a texture via render-to-texture.
 * Spawns the dish mesh and ingredients in an isolated scene, then uses
 * SceneCaptureComponent2D to render to a UTextureRenderTarget2D.
 */
UCLASS()
class PROJECTUMEOWMI_API APUDishCaptureActor : public AActor
{
	GENERATED_BODY()

public:
	APUDishCaptureActor();

	/**
	 * Capture the given dish to a texture.
	 * @param DishData - The completed dish (used for dish mesh)
	 * @param SceneOriginWorld - World position of the dish center
	 * @param IngredientMeshes - Spawned ingredient meshes; their actual transforms are used (not FPUDishBase plating data)
	 * @param IngredientMeshScale - Fallback scale when mesh has no scale
	 * @param PlateBoundsExtent - Half-extents of the plate (for dish scale matching; pass zero to skip)
	 * @param OutTexture - The captured texture (caller must manage lifetime)
	 * @return true if capture succeeded
	 */
	UFUNCTION(BlueprintCallable, Category = "Dish Capture")
	bool CaptureDishToTexture(
		const FPUDishBase& DishData,
		const FVector& SceneOriginWorld,
		const TArray<APUIngredientMesh*>& IngredientMeshes,
		float IngredientMeshScale,
		const FVector& PlateBoundsExtent,
		UTexture2D*& OutTexture);

	/** Render target size for the capture (square). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dish Capture")
	int32 CaptureResolution = 512;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneCaptureComponent2D* SceneCapture;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* DishMeshComponent;

	/** Root for spawned ingredient meshes. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* IngredientsRoot;

	/** Offset from world origin - we spawn far away to avoid z-fighting with game world. */
	FVector CaptureSceneOffset;

	void ClearCaptureScene();
	bool SetupCaptureScene(const FPUDishBase& DishData, const FVector& SceneOriginWorld, const TArray<APUIngredientMesh*>& IngredientMeshes, float IngredientMeshScale, const FVector& PlateBoundsExtent);
};
