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

    // Create and setup the root component
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

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
    UE_LOG(LogTemp, Log, TEXT("TalkingObject::CanInteract - bPlayerInRange: %d, bIsInteracting: %d, AvailableDialogues.Num(): %d"), 
        bPlayerInRange, bIsInteracting, AvailableDialogues.Num());
    return bPlayerInRange && !bIsInteracting && AvailableDialogues.Num() > 0;
}

void ATalkingObject::StartInteraction()
{
    if (CanInteract())
    {
        UE_LOG(LogTemp, Log, TEXT("TalkingObject::StartInteraction - Starting interaction"));
        bIsInteracting = true;
        StartRandomDialogue();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("TalkingObject::StartInteraction - Cannot start interaction! bPlayerInRange: %d, bIsInteracting: %d, AvailableDialogues.Num(): %d"), 
            bPlayerInRange, bIsInteracting, AvailableDialogues.Num());
    }
}

void ATalkingObject::EndInteraction()
{
    UE_LOG(LogTemp, Log, TEXT("TalkingObject::EndInteraction - Ending interaction for %s"), *GetName());
    bIsInteracting = false;
    if (CurrentDialogueContext)
    {
        CurrentDialogueContext = nullptr;
    }

    // Get the player character and clear the talking object reference
    if (AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn()))
    {
        UE_LOG(LogTemp, Log, TEXT("TalkingObject::EndInteraction - Unregistering talking object from character"));
        Character->UnregisterTalkingObject(this);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("TalkingObject::EndInteraction - Failed to get character reference"));
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
    
    // Add the talking object itself
    Participants.Add(this);
    UE_LOG(LogTemp, Display, TEXT("TalkingObject::StartSpecificDialogue - Added talking object as participant: %s"), *GetName());

    // TODO: AHRIA - 2025-05-01 - This is a temporary fix, but finding all participants in the level is not the best way to do this.
    // TODO: AHRIA - We need to find a way to only get the participants that are in the allowed list.

    // For NPCs, search for all actors with the participant interface
    if (ObjectType == ETalkingObjectType::NPC)
    {
        // Always add the player character first
        Participants.Add(PlayerCharacter);
        UE_LOG(LogTemp, Display, TEXT("TalkingObject::StartSpecificDialogue - Added player character as participant"));

        // Get all objects with dialogue participant interface
        TArray<UObject*> AllParticipants = UDlgManager::GetObjectsWithDialogueParticipantInterface(this);
        UE_LOG(LogTemp, Display, TEXT("TalkingObject::StartSpecificDialogue - Found %d total participants in level"), AllParticipants.Num());

        // Add all found participants that are in our allowed list
        for (UObject* Participant : AllParticipants)
        {
            if (Participant != this && Participant != PlayerCharacter) // Skip self and player since we already added them
            {
                // Get the participant name
                FName FoundParticipantName = IDlgDialogueParticipant::Execute_GetParticipantName(Participant);
                
                // Check if this participant is in our allowed list
                if (AllowedParticipantNames.Num() == 0 || AllowedParticipantNames.Contains(FoundParticipantName))
                {
                    Participants.Add(Participant);
                    UE_LOG(LogTemp, Display, TEXT("TalkingObject::StartSpecificDialogue - Added allowed participant: %s (Name: %s)"), 
                        *Participant->GetName(), 
                        *FoundParticipantName.ToString());
                }
                else
                {
                    UE_LOG(LogTemp, Display, TEXT("TalkingObject::StartSpecificDialogue - Skipping participant: %s (Name: %s) - Not in allowed list"), 
                        *Participant->GetName(), 
                        *FoundParticipantName.ToString());
                }
            }
        }
    }
    // For System type, we'll leave it empty for now as requested
    // For Prop type, we'll just use the talking object itself

    // Start the dialogue
    CurrentDialogueContext = UDlgManager::StartDialogue(Dialogue, Participants);

    // Log the participants in the dialogue context
    if (CurrentDialogueContext)
    {
        const TMap<FName, UObject*>& ParticipantsMap = CurrentDialogueContext->GetParticipantsMap();
        UE_LOG(LogTemp, Display, TEXT("TalkingObject::StartSpecificDialogue - Dialogue context created with %d participants:"), ParticipantsMap.Num());
        
        for (const auto& Pair : ParticipantsMap)
        {
            FName ParticipantNameKey = Pair.Key;
            UObject* Participant = Pair.Value;
            UE_LOG(LogTemp, Display, TEXT("  - Participant: %s (Name: %s)"), 
                *Participant->GetName(), 
                *ParticipantNameKey.ToString());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("TalkingObject::StartSpecificDialogue - Failed to create dialogue context!"));
    }

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
        else
        {
            UE_LOG(LogTemp, Error, TEXT("TalkingObject::StartSpecificDialogue - Failed to get dialogue box from player character!"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("TalkingObject::StartSpecificDialogue - Failed to cast player character to ProjectUmeowmiCharacter!"));
    }
}

// Collision events
void ATalkingObject::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (OtherActor == PlayerCharacter)
    {
        UE_LOG(LogTemp, Log, TEXT("TalkingObject::OnInteractionSphereBeginOverlap - Player entered range of %s"), *GetName());
        bPlayerInRange = true;
        UpdateInteractionWidget();
        OnPlayerEnteredInteractionSphere.Broadcast(this);
    }
}

void ATalkingObject::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (OtherActor == PlayerCharacter)
    {
        UE_LOG(LogTemp, Log, TEXT("TalkingObject::OnInteractionSphereEndOverlap - Player exited range of %s"), *GetName());
        bPlayerInRange = false;
        UpdateInteractionWidget();
        OnPlayerExitedInteractionSphere.Broadcast(this);

        // If we're not currently interacting, unregister from the character
        if (!bIsInteracting)
        {
            if (AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(PlayerCharacter))
            {
                UE_LOG(LogTemp, Log, TEXT("TalkingObject::OnInteractionSphereEndOverlap - Unregistering talking object from character (no interaction occurred)"));
                Character->UnregisterTalkingObject(this);
            }
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