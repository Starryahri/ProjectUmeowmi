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

    // Create and setup the interaction widget
    InteractionWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionWidget"));
    InteractionWidget->SetupAttachment(RootComponent);
    InteractionWidget->SetWidgetSpace(EWidgetSpace::Screen);
    InteractionWidget->SetVisibility(false);

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
    UE_LOG(LogTemp, Log, TEXT("CookingStation::StartInteraction - Attempting to start interaction"));
    if (CanInteract() && DishCustomizationComponent)
    {
        // Get the character that triggered the interaction
        AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
        if (Character)
        {
            UE_LOG(LogTemp, Log, TEXT("CookingStation::StartInteraction - Got character reference"));
            
            // Check if we're already in customization mode
            if (DishCustomizationComponent->IsCustomizing())
            {
                UE_LOG(LogTemp, Log, TEXT("CookingStation::StartInteraction - Already in customization mode, ignoring interaction"));
                return;
            }

            // Hide the interaction widget
            if (InteractionWidget)
            {
                InteractionWidget->SetVisibility(false);
            }

            // Start dish customization with the character reference
            DishCustomizationComponent->StartCustomization(Character);
            BroadcastInteractionStarted();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("CookingStation::StartInteraction - Failed to get Character reference"));
            BroadcastInteractionFailed();
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CookingStation::StartInteraction - Cannot interact: CanInteract=%d, HasDishComponent=%d"), 
            CanInteract(), DishCustomizationComponent != nullptr);
        BroadcastInteractionFailed();
    }
}

void APUCookingStation::EndInteraction()
{
    if (DishCustomizationComponent)
    {
        DishCustomizationComponent->EndCustomization();
    }

    // Show the interaction widget again
    if (InteractionWidget)
    {
        InteractionWidget->SetVisibility(true);
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
        UE_LOG(LogTemp, Log, TEXT("CookingStation::OnInteractionBoxBeginOverlap - Player entered range"));
        Character->RegisterInteractable(this);
        BroadcastInteractionRangeEntered();

        // Show the interaction widget
        if (InteractionWidget)
        {
            InteractionWidget->SetVisibility(true);
        }
    }
}

void APUCookingStation::OnInteractionBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    if (AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(OtherActor))
    {
        UE_LOG(LogTemp, Log, TEXT("CookingStation::OnInteractionBoxEndOverlap - Player exited range"));
        Character->UnregisterInteractable(this);
        BroadcastInteractionRangeExited();

        // Hide the interaction widget
        if (InteractionWidget)
        {
            InteractionWidget->SetVisibility(false);
        }
    }
} 