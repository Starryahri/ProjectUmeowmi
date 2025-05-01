#include "TalkingObject.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "DlgSystem/DlgManager.h"
#include "DlgSystem/DlgContext.h"
#include "DlgSystem/DlgDialogue.h"
#include "ProjectUmeowmi/ProjectUmeowmiCharacter.h"
#include "ProjectUmeowmi/UI/PUDialogueBox.h"
//#include "DlgSystem/DlgDialogueParticipant.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Engine/Engine.h"

ATalkingObject::ATalkingObject()
{
    // We can disable tick by default since we'll only need it for debug visualization
    PrimaryActorTick.bCanEverTick = false;

    // Create and setup the interaction sphere component
    InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    InteractionSphere->SetupAttachment(RootComponent);
    InteractionSphere->SetSphereRadius(InteractionRange);
    InteractionSphere->SetCollisionProfileName(TEXT("Trigger"));
    InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ATalkingObject::OnInteractionSphereBeginOverlap);
    InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ATalkingObject::OnInteractionSphereEndOverlap);

    // Create and setup the widget component
    InteractionWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionWidget"));
    InteractionWidget->SetupAttachment(RootComponent);
    InteractionWidget->SetWidgetSpace(EWidgetSpace::Screen);
    InteractionWidget->SetVisibility(false);
}

void ATalkingObject::BeginPlay()
{
    Super::BeginPlay();

    // Create the widget instance
    if (InteractionWidgetClass)
    {
        InteractionWidget->SetWidgetClass(InteractionWidgetClass);
    }
    
    // Enable tick if debug visualization is enabled
    if (bShowDebugRange)
    {
        PrimaryActorTick.bCanEverTick = true;
    }
}

void ATalkingObject::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Only draw debug visualization if enabled
    if (bShowDebugRange)
    {
        DrawDebugRange();
    }
}

bool ATalkingObject::CheckCondition(const UDlgContext* Context, FName ConditionName) const
{
    return false;
}

float ATalkingObject::GetFloatValue(FName ValueName) const
{
    return 0.0f;
}

int32 ATalkingObject::GetIntValue(FName ValueName) const
{
    return 0;
}

bool ATalkingObject::GetBoolValue(FName ValueName) const
{
    return false;
}

FName ATalkingObject::GetNameValue(FName ValueName) const
{
    return NAME_None;
}

bool ATalkingObject::OnDialogueEvent(UDlgContext* Context, FName EventName)
{
    return false;
}


// Interaction methods
bool ATalkingObject::CanInteract() const
{
    return bPlayerInRange && !bIsInteracting && AvailableDialogues.Num() > 0;
}

void ATalkingObject::StartInteraction()
{
    if (CanInteract())
    {
        // Show debug message
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Starting interaction with talking object!"));
        
        bIsInteracting = true;
        StartRandomDialogue();
    }
    else
    {
        // Show debug message when interaction is not possible
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Cannot start interaction!"));
    }
}

void ATalkingObject::EndInteraction()
{
    bIsInteracting = false;
    if (CurrentDialogueContext)
    {
        CurrentDialogueContext = nullptr;
    }
}

// Dialogue methods
void ATalkingObject::StartRandomDialogue()
{
    if (UDlgDialogue* Dialogue = GetRandomDialogue())
    {
        StartSpecificDialogue(Dialogue);
    }
}

void ATalkingObject::StartSpecificDialogue(UDlgDialogue* Dialogue)
{
    if (!Dialogue)
    {
        return;
    }

    // Get the player character
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (!PlayerCharacter)
    {
        return;
    }

    // Create participants array
    TArray<UObject*> Participants;
    Participants.Add(this);
    //Participants.Add(PlayerCharacter);

    // Start the dialogue
    CurrentDialogueContext = UDlgManager::StartDialogue(Dialogue, Participants);

    // We need to make the dialogue box from the AProjectUmeowmiCharacter visible
    // Grab the player character and the reference to the dialogue box
    AProjectUmeowmiCharacter* ProjectCharacter = Cast<AProjectUmeowmiCharacter>(PlayerCharacter);
    if (ProjectCharacter)
    {
        UPUDialogueBox* DialogueBox = ProjectCharacter->GetDialogueBox();
        if (DialogueBox)
        {
            DialogueBox->Open_Implementation(CurrentDialogueContext);
        }
    }
}

// Collision events
void ATalkingObject::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    // Check if the overlapping actor is the player
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (OtherActor == PlayerCharacter)
    {
        bPlayerInRange = true;
        
        // Update the widget visibility
        UpdateInteractionWidget();
        
        // Broadcast the delegate
        OnPlayerEnteredInteractionSphere.Broadcast(this);
        
        // display this object's participant name
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Participant name:"));
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, *ParticipantName.ToString());


        // Display debug message on screen
        if (GEngine)
        {
            FString Message = FString::Printf(TEXT("Player entered interaction range of %s"), *DisplayName.ToString());
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, Message);
        }
    }
}

void ATalkingObject::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    // Check if the actor that stopped overlapping is the player
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (OtherActor == PlayerCharacter)
    {
        bPlayerInRange = false;
        
        // Update the widget visibility
        UpdateInteractionWidget();
        
        // Broadcast the delegate
        OnPlayerExitedInteractionSphere.Broadcast(this);
        
        // Display debug message on screen
        if (GEngine)
        {
            FString Message = FString::Printf(TEXT("Player exited interaction range of %s"), *DisplayName.ToString());
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, Message);
        }
    }
}

// Debug methods
void ATalkingObject::ToggleDebugVisualization()
{
    bShowDebugRange = !bShowDebugRange;
    
    // Enable or disable tick based on debug visualization
    PrimaryActorTick.bCanEverTick = bShowDebugRange;
    
    // Draw the debug range once when toggled
    if (bShowDebugRange)
    {
        DrawDebugRange();
    }
}

// Helper methods
void ATalkingObject::UpdateInteractionWidget()
{
    if (!InteractionWidget)
    {
        return;
    }

    const bool bCanInteractNow = CanInteract();
    InteractionWidget->SetVisibility(bCanInteractNow);

    if (bCanInteractNow)
    {
        if (UTalkingObjectWidget* Widget = Cast<UTalkingObjectWidget>(InteractionWidget->GetWidget()))
        {
            Widget->SetInteractionKey(InteractionKey.ToString());
        }
    }
}

bool ATalkingObject::IsPlayerInRange() const
{
    return bPlayerInRange;
}

UDlgDialogue* ATalkingObject::GetRandomDialogue() const
{
    if (AvailableDialogues.Num() == 0)
    {
        return nullptr;
    }

    // If all dialogues have been used, reset the tracking
    if (UsedDialogues.Num() >= AvailableDialogues.Num())
    {
        const_cast<ATalkingObject*>(this)->ResetUsedDialogues();
    }

    // Find a dialogue that hasn't been used yet
    TArray<UDlgDialogue*> AvailableUnusedDialogues;
    for (UDlgDialogue* Dialogue : AvailableDialogues)
    {
        if (!UsedDialogues.Contains(Dialogue))
        {
            AvailableUnusedDialogues.Add(Dialogue);
        }
    }

    // Select a random dialogue from the unused ones
    if (AvailableUnusedDialogues.Num() > 0)
    {
        const int32 RandomIndex = FMath::RandRange(0, AvailableUnusedDialogues.Num() - 1);
        UDlgDialogue* SelectedDialogue = AvailableUnusedDialogues[RandomIndex];
        const_cast<ATalkingObject*>(this)->UsedDialogues.Add(SelectedDialogue);
        return SelectedDialogue;
    }

    return nullptr;
}

void ATalkingObject::ResetUsedDialogues()
{
    UsedDialogues.Empty();
}

void ATalkingObject::DrawDebugRange() const
{
    if (!bShowDebugRange)
    {
        return;
    }

    const FVector Location = GetActorLocation();
    const FColor DebugColor = FColor::Green;
    const float LifeTime = -1.0f;
    const uint8 DepthPriority = 0;
    const float Thickness = 2.0f;

    // Draw the interaction range sphere
    DrawDebugSphere(
        GetWorld(),
        Location,
        InteractionRange,
        32, // Number of segments
        DebugColor,
        false,
        LifeTime,
        DepthPriority,
        Thickness
    );
} 