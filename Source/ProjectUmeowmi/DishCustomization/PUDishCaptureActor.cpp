#include "PUDishCaptureActor.h"
#include "PUIngredientMesh.h"
#include "Camera/CameraTypes.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "Materials/Material.h"
#include "Kismet/GameplayStatics.h"

APUDishCaptureActor::APUDishCaptureActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	CaptureSceneOffset = FVector(10000.0f, 10000.0f, 10000.0f);  // Far from game world

	IngredientsRoot = CreateDefaultSubobject<USceneComponent>(TEXT("IngredientsRoot"));
	IngredientsRoot->SetupAttachment(RootComponent);

	DishMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DishMesh"));
	DishMeshComponent->SetupAttachment(RootComponent);
	DishMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DishMeshComponent->SetCastShadow(true);

	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(RootComponent);
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;
	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
}

void APUDishCaptureActor::BeginPlay()
{
	Super::BeginPlay();
}

void APUDishCaptureActor::ClearCaptureScene()
{
	DishMeshComponent->SetStaticMesh(nullptr);

	TArray<USceneComponent*> ChildComponents;
	IngredientsRoot->GetChildrenComponents(false, ChildComponents);
	for (USceneComponent* Child : ChildComponents)
	{
		if (UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Child))
		{
			MeshComp->DestroyComponent();
		}
	}
}

bool APUDishCaptureActor::SetupCaptureScene(const FPUDishBase& DishData, const FVector& SceneOriginWorld, const TArray<APUIngredientMesh*>& IngredientMeshes, float IngredientMeshScale, const FVector& PlateBoundsExtent)
{
	ClearCaptureScene();

	// Dish mesh
	UStaticMesh* DishMesh = nullptr;
	if (DishData.DishMesh.IsValid())
	{
		DishMesh = DishData.DishMesh.LoadSynchronous();
	}
	if (!DishMesh && !DishData.DishMesh.ToSoftObjectPath().IsNull())
	{
		DishMesh = LoadObject<UStaticMesh>(nullptr, *DishData.DishMesh.ToString());
	}
	if (DishMesh)
	{
		DishMeshComponent->SetStaticMesh(DishMesh);
		DishMeshComponent->SetWorldLocation(GetActorLocation());
		DishMeshComponent->SetWorldScale3D(FVector::OneVector);

		if (PlateBoundsExtent.SizeSquared() > KINDA_SMALL_NUMBER)
		{
			FBox MeshBounds = DishMesh->GetBoundingBox();
			FVector MeshExtent = MeshBounds.GetExtent();
			if (MeshExtent.SizeSquared() > KINDA_SMALL_NUMBER)
			{
				float GameRadius = FMath::Max(PlateBoundsExtent.X, PlateBoundsExtent.Y);
				float MeshRadius = FMath::Max(MeshExtent.X, MeshExtent.Y);
				if (MeshRadius > KINDA_SMALL_NUMBER)
				{
					float DishScale = GameRadius / MeshRadius;
					DishMeshComponent->SetWorldScale3D(FVector(DishScale, DishScale, DishScale));
				}
			}
		}
	}

	// Use actual transforms from spawned ingredient meshes
	for (APUIngredientMesh* MeshActor : IngredientMeshes)
	{
		if (!IsValid(MeshActor))
		{
			continue;
		}

		UStaticMesh* IngredientMesh = MeshActor->GetIngredientData().IngredientMesh.LoadSynchronous();
		if (!IngredientMesh)
		{
			IngredientMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube"));
		}
		if (!IngredientMesh)
		{
			continue;
		}

		FTransform WorldTransform = MeshActor->GetCaptureTransform();
		FVector LocalPos = WorldTransform.GetLocation() - SceneOriginWorld;
		FRotator LocalRot = WorldTransform.Rotator();
		FVector LocalScale = WorldTransform.GetScale3D();
		if (LocalScale.SizeSquared() < KINDA_SMALL_NUMBER)
		{
			LocalScale = FVector(IngredientMeshScale, IngredientMeshScale, IngredientMeshScale);
		}

		UStaticMeshComponent* IngredientComp = NewObject<UStaticMeshComponent>(this);
		IngredientComp->SetupAttachment(IngredientsRoot);
		IngredientComp->RegisterComponent();
		IngredientComp->SetStaticMesh(IngredientMesh);
		IngredientComp->SetRelativeLocation(LocalPos);
		IngredientComp->SetRelativeRotation(LocalRot);
		IngredientComp->SetRelativeScale3D(LocalScale);
		IngredientComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		IngredientComp->SetCastShadow(true);

		if (MeshActor->GetIngredientData().MaterialInstance.IsValid() || !MeshActor->GetIngredientData().MaterialInstance.ToSoftObjectPath().IsNull())
		{
			if (UMaterialInterface* Mat = MeshActor->GetIngredientData().MaterialInstance.LoadSynchronous())
			{
				IngredientComp->SetMaterial(0, Mat);
			}
		}
	}

	return true;
}

bool APUDishCaptureActor::CaptureDishToTexture(
	const FPUDishBase& DishData,
	const FVector& SceneOriginWorld,
	const TArray<APUIngredientMesh*>& IngredientMeshes,
	float IngredientMeshScale,
	const FVector& PlateBoundsExtent,
	UTexture2D*& OutTexture)
{
	OutTexture = nullptr;
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	SetActorLocation(CaptureSceneOffset);

	if (!SetupCaptureScene(DishData, SceneOriginWorld, IngredientMeshes, IngredientMeshScale, PlateBoundsExtent))
	{
		return false;
	}

	// Create or reuse render target
	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	if (!RenderTarget)
	{
		return false;
	}
	RenderTarget->RenderTargetFormat = RTF_RGBA8;
	RenderTarget->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);  // Transparent background
	RenderTarget->InitAutoFormat(CaptureResolution, CaptureResolution);
	RenderTarget->UpdateResource();

	// Only render our dish and ingredients (transparent everywhere else)
	SceneCapture->ClearShowOnlyComponents();
	SceneCapture->ShowOnlyActorComponents(this, true);  // Sets PRM_UseShowOnlyList, includes dish + ingredients

	// Zoom in: closer camera, narrower FOV
	FVector CameraOffset(70.0f, 0.0f, 35.0f);
	SceneCapture->TextureTarget = RenderTarget;
	SceneCapture->SetWorldLocation(GetActorLocation() + CameraOffset);
	SceneCapture->SetWorldRotation(FRotationMatrix::MakeFromX(-CameraOffset.GetSafeNormal()).Rotator());
	SceneCapture->FOVAngle = 42.0f;
	SceneCapture->OrthoWidth = 120.0f;
	SceneCapture->ProjectionType = ECameraProjectionMode::Perspective;
	SceneCapture->bConsiderUnrenderedOpaquePixelAsFullyTranslucent = true;  // Unrendered pixels stay transparent

	// Capture
	SceneCapture->CaptureScene();

	// Copy render target to UTexture2D for UI use
	OutTexture = UTexture2D::CreateTransient(CaptureResolution, CaptureResolution, PF_B8G8R8A8);
	if (OutTexture)
	{
		OutTexture->AddToRoot();
		FTextureRenderTargetResource* RTResource = RenderTarget->GameThread_GetRenderTargetResource();
		if (RTResource)
		{
			FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
			ReadFlags.SetLinearToGamma(false);
			TArray<FColor> OutPixels;
			if (RTResource->ReadPixels(OutPixels, ReadFlags) && OutTexture->GetPlatformData())
			{
				void* MipData = OutTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(MipData, OutPixels.GetData(), OutPixels.Num() * sizeof(FColor));
				OutTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
				OutTexture->UpdateResource();
			}
		}
	}

	// Cleanup
	ClearCaptureScene();
	RenderTarget->ConditionalBeginDestroy();

	return OutTexture != nullptr;
}
