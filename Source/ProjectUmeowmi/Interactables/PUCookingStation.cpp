#include "PUCookingStation.h"
#include "../ProjectUmeowmiCharacter.h"

APUCookingStation::APUCookingStation()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create and setup components
    StationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationMesh"));
    RootComponent = StationMesh;

    InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
    InteractionBox->SetupAttachment(RootComponent);
    InteractionBox->SetCollisionProfileName(TEXT("Trigger"));
    InteractionBox->SetBoxExtent(FVector(InteractionRange));

    DishCustomizationComponent = CreateDefaultSubobject<UPUDishCustomizationComponent>(TEXT("DishCustomizationComponent"));
    DishCustomizationComponent->SetupAttachment(RootComponent);

    // Set default values
    StationName = FText::FromString(TEXT("Cooking Station"));
    StationDescription = FText::FromString(TEXT("A station for customizing dishes"));
    InteractionRange = 200.0f;
    bIsInteractable = true;
}

void APUCookingStation::BeginPlay()
{
    Super::BeginPlay();

    // Bind to dish customization events
    if (DishCustomizationComponent)
    {
        DishCustomizationComponent->OnCustomizationEnded.AddDynamic(this, &APUCookingStation::OnCustomizationEnded);
    }

    // Bind to interaction box events
    if (InteractionBox)
    {
        InteractionBox->OnComponentBeginOverlap.AddDynamic(this, &APUCookingStation::OnInteractionBoxBeginOverlap);
        InteractionBox->OnComponentEndOverlap.AddDynamic(this, &APUCookingStation::OnInteractionBoxEndOverlap);
    }
}

void APUCookingStation::StartInteraction()
{
    if (CanInteract() && DishCustomizationComponent)
    {
        // Get the character that triggered the interaction
        AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
        if (Character)
        {
            // Start dish customization with the character reference
            DishCustomizationComponent->StartCustomization(Character);
            BroadcastInteractionStarted();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to get Character in StartInteraction"));
            BroadcastInteractionFailed();
        }
    }
    else
    {
        BroadcastInteractionFailed();
    }
}

void APUCookingStation::EndInteraction()
{
    if (DishCustomizationComponent)
    {
        DishCustomizationComponent->EndCustomization();
    }
    BroadcastInteractionEnded();
}

FText APUCookingStation::GetInteractionText() const
{
    return StationName;
}

FText APUCookingStation::GetInteractionDescription() const
{
    return StationDescription;
}

void APUCookingStation::OnCustomizationEnded()
{
    EndInteraction();
}

void APUCookingStation::OnInteractionBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(OtherActor))
    {
        Character->RegisterInteractable(this);
        BroadcastInteractionRangeEntered();
    }
}

void APUCookingStation::OnInteractionBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(OtherActor))
    {
        Character->UnregisterInteractable(this);
        BroadcastInteractionRangeExited();
    }
} 