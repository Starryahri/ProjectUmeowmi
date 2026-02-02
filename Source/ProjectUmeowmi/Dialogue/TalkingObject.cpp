#include "TalkingObject.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "DlgSystem/DlgManager.h"
#include "DlgSystem/DlgContext.h"
#include "DlgSystem/DlgDialogue.h"
#include "ProjectUmeowmi/ProjectUmeowmiCharacter.h"
#include "ProjectUmeowmi/UI/PUDialogueBox.h"
#include "PUDishGiver.h"
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

bool ATalkingObject::CheckCondition_Implementation(const UDlgContext* Context, FName ConditionName) const
{
    //UE_LOG(LogTemp,Display, TEXT("=== TalkingObject::CheckCondition CALLED ==="));
    //UE_LOG(LogTemp,Display, TEXT("Condition Name: %s"), *ConditionName.ToString());
    //UE_LOG(LogTemp,Display, TEXT("Context: %s"), Context ? TEXT("VALID") : TEXT("NULL"));
    //UE_LOG(LogTemp,Display, TEXT("This Object: %s"), *GetName());
    //UE_LOG(LogTemp,Display, TEXT("TalkingObject::CheckCondition - Returning FALSE (default behavior)"));
    return false;
}

float ATalkingObject::GetFloatValue_Implementation(FName ValueName) const
{
    return 0.0f;
}

int32 ATalkingObject::GetIntValue_Implementation(FName ValueName) const
{
    return 0;
}

bool ATalkingObject::GetBoolValue_Implementation(FName ValueName) const
{
    return false;
}

FName ATalkingObject::GetNameValue_Implementation(FName ValueName) const
{
    return NAME_None;
}

bool ATalkingObject::OnDialogueEvent_Implementation(UDlgContext* Context, FName EventName)
{
    //UE_LOG(LogTemp,Display, TEXT("=== ATalkingObject::OnDialogueEvent CALLED ==="));
    //UE_LOG(LogTemp,Display, TEXT("Event Name: %s"), *EventName.ToString());
    //UE_LOG(LogTemp,Display, TEXT("Context: %s"), Context ? TEXT("VALID") : TEXT("NULL"));
    //UE_LOG(LogTemp,Display, TEXT("This Object: %s"), *GetName());
    
    // Handle order generation event
    if (EventName == TEXT("GenerateOrder"))
    {
        //UE_LOG(LogTemp,Display, TEXT("ATalkingObject::OnDialogueEvent - Handling GenerateOrder event"));
        
        // Check if this is a dish giver
        if (APUDishGiver* DishGiver = Cast<APUDishGiver>(this))
        {
            //UE_LOG(LogTemp,Display, TEXT("ATalkingObject::OnDialogueEvent - Cast to APUDishGiver successful, calling GenerateAndGiveOrderToPlayer"));
            DishGiver->GenerateAndGiveOrderToPlayer();
            return true;
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("ATalkingObject::OnDialogueEvent - GenerateOrder event called on non-dish-giver object: %s"), *GetName());
            return false;
        }
    }
    
    //UE_LOG(LogTemp,Display, TEXT("ATalkingObject::OnDialogueEvent - Unknown event: %s"), *EventName.ToString());
    return false;
}


// Interaction methods
bool ATalkingObject::CanInteract() const
{
    //UE_LOG(LogTemp,Display, TEXT("TalkingObject::CanInteract - %s: bPlayerInRange=%d, bIsInteracting=%d, AvailableDialogues=%d"), 
    //    *GetName(), bPlayerInRange, bIsInteracting, AvailableDialogues.Num());
    return bPlayerInRange && !bIsInteracting && AvailableDialogues.Num() > 0;
}

void ATalkingObject::StartInteraction()
{
    if (CanInteract())
    {
        //UE_LOG(LogTemp,Log, TEXT("TalkingObject::StartInteraction - Starting interaction"));
        bIsInteracting = true;
        StartRandomDialogue();
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("TalkingObject::StartInteraction - Cannot start interaction! bPlayerInRange: %d, bIsInteracting: %d, AvailableDialogues.Num(): %d"), 
        //    bPlayerInRange, bIsInteracting, AvailableDialogues.Num());
    }
}

void ATalkingObject::EndInteraction()
{
    //UE_LOG(LogTemp,Log, TEXT("TalkingObject::EndInteraction - Ending interaction for %s"), *GetName());
    bIsInteracting = false;
    
    // Properly clear the dialogue context to prevent dangling references
    if (CurrentDialogueContext)
    {
        //UE_LOG(LogTemp,Log, TEXT("TalkingObject::EndInteraction - Clearing dialogue context"));
        CurrentDialogueContext = nullptr;
    }

    // Get the player character and clear the talking object reference
    if (AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn()))
    {
        //UE_LOG(LogTemp,Log, TEXT("TalkingObject::EndInteraction - Unregistering talking object from character"));
        Character->UnregisterTalkingObject(this);
    }
    else
    {
        //UE_LOG(LogTemp,Warning, TEXT("TalkingObject::EndInteraction - Failed to get character reference"));
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
        //UE_LOG(LogTemp,Warning, TEXT("TalkingObject::StartSpecificDialogue - Invalid dialogue provided"));
        return;
    }

    // Clear any existing dialogue context first to prevent dangling references
    if (CurrentDialogueContext)
    {
        //UE_LOG(LogTemp,Log, TEXT("TalkingObject::StartSpecificDialogue - Clearing existing dialogue context"));
        CurrentDialogueContext = nullptr;
    }

    // Get the player controller first
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    if (!PlayerController)
    {
        //UE_LOG(LogTemp,Error, TEXT("TalkingObject::StartSpecificDialogue - Failed to get player controller!"));
        return;
    }

    // Get the local player
    ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
    if (!LocalPlayer)
    {
        //UE_LOG(LogTemp,Error, TEXT("TalkingObject::StartSpecificDialogue - Failed to get local player!"));
        return;
    }

    // Get the player character
    ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerController->GetPawn());
    if (!PlayerCharacter)
    {
        //UE_LOG(LogTemp,Error, TEXT("TalkingObject::StartSpecificDialogue - Failed to get player character!"));
        return;
    }

    // Create participants array with proper validation
    TArray<UObject*> Participants;
    
    // Add the talking object itself (validate first)
    if (IsValid(this))
    {
        Participants.Add(this);
        //UE_LOG(LogTemp,Display, TEXT("TalkingObject::StartSpecificDialogue - Added talking object as participant: %s"), *GetName());
    }
    else
    {
        //UE_LOG(LogTemp,Error, TEXT("TalkingObject::StartSpecificDialogue - Talking object is not valid!"));
        return;
    }

    // For NPCs, add the player character (Bao)
    if (ObjectType == ETalkingObjectType::NPC)
    {
        // Add the player character (validate first)
        if (IsValid(PlayerCharacter))
        {
            Participants.Add(PlayerCharacter);
            //UE_LOG(LogTemp,Display, TEXT("TalkingObject::StartSpecificDialogue - Added player character as participant"));
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("TalkingObject::StartSpecificDialogue - Player character is not valid, skipping"));
        }

        // Get all other NPCs with dialogue participant interface
        TArray<UObject*> AllParticipants = UDlgManager::GetObjectsWithDialogueParticipantInterface(this);
        //UE_LOG(LogTemp,Display, TEXT("TalkingObject::StartSpecificDialogue - Found %d total participants in level"), AllParticipants.Num());

        // Add other NPCs that are in our allowed list (with validation)
        for (UObject* Participant : AllParticipants)
        {
            // Validate participant before using
            if (!IsValid(Participant))
            {
                //UE_LOG(LogTemp,Warning, TEXT("TalkingObject::StartSpecificDialogue - Skipping invalid participant"));
                continue;
            }

            // Additional safety check: ensure participant is still in the world
            if (AActor* ActorParticipant = Cast<AActor>(Participant))
            {
                if (!IsValid(ActorParticipant) || !ActorParticipant->IsValidLowLevel())
                {
                    //UE_LOG(LogTemp,Warning, TEXT("TalkingObject::StartSpecificDialogue - Skipping invalid actor participant: %s"), *ActorParticipant->GetName());
                    continue;
                }
            }

            if (Participant != this && Participant != PlayerCharacter) // Skip self and player since we already added them
            {
                // Get the participant name with safety check
                FName FoundParticipantName = NAME_None;
                if (Participant->GetClass()->ImplementsInterface(UDlgDialogueParticipant::StaticClass()))
                {
                    FoundParticipantName = IDlgDialogueParticipant::Execute_GetParticipantName(Participant);
                }
                else
                {
                    //UE_LOG(LogTemp,Warning, TEXT("TalkingObject::StartSpecificDialogue - Participant doesn't implement dialogue interface: %s"), *Participant->GetName());
                    continue;
                }
                
                // Check if this participant is in our allowed list
                if (AllowedParticipantNames.Num() == 0 || AllowedParticipantNames.Contains(FoundParticipantName))
                {
                    Participants.Add(Participant);
                    //UE_LOG(LogTemp,Display, TEXT("TalkingObject::StartSpecificDialogue - Added allowed participant: %s (Name: %s)"), 
                    //    *Participant->GetName(), 
                    //    *FoundParticipantName.ToString());
                }
                else
                {
                    //UE_LOG(LogTemp,Display, TEXT("TalkingObject::StartSpecificDialogue - Skipping participant: %s (Name: %s) - Not in allowed list"), 
                    //    *Participant->GetName(), 
                    //    *FoundParticipantName.ToString());
                }
            }
        }
    }

    // Validate we have at least the talking object as a participant
    if (Participants.Num() == 0)
    {
        //UE_LOG(LogTemp,Error, TEXT("TalkingObject::StartSpecificDialogue - No valid participants found!"));
        return;
    }

    // Final validation: ensure all participants are still valid before starting dialogue
    for (int32 i = Participants.Num() - 1; i >= 0; --i)
    {
        if (!IsValid(Participants[i]))
        {
            //UE_LOG(LogTemp,Warning, TEXT("TalkingObject::StartSpecificDialogue - Removing invalid participant at index %d"), i);
            Participants.RemoveAt(i);
        }
    }

    // Check again after removing invalid participants
    if (Participants.Num() == 0)
    {
        //UE_LOG(LogTemp,Error, TEXT("TalkingObject::StartSpecificDialogue - No valid participants remaining after final validation!"));
        return;
    }

    // Start the dialogue with validated participants
    CurrentDialogueContext = UDlgManager::StartDialogue(Dialogue, Participants);

    // Log the participants in the dialogue context
    if (CurrentDialogueContext)
    {
        const TMap<FName, UObject*>& ParticipantsMap = CurrentDialogueContext->GetParticipantsMap();
        //UE_LOG(LogTemp,Display, TEXT("TalkingObject::StartSpecificDialogue - Dialogue context created with %d participants:"), ParticipantsMap.Num());
        
        for (const auto& Pair : ParticipantsMap)
        {
            FName ParticipantNameKey = Pair.Key;
            UObject* Participant = Pair.Value;
            
            // Validate participant before logging
            if (IsValid(Participant))
            {
                //UE_LOG(LogTemp,Display, TEXT("  - Participant: %s (Name: %s)"), 
                //    *Participant->GetName(), 
                //    *ParticipantNameKey.ToString());
            }
            else
            {
                //UE_LOG(LogTemp,Warning, TEXT("  - Invalid participant (Name: %s)"), *ParticipantNameKey.ToString());
            }
        }

        // We need to make the dialogue box from the AProjectUmeowmiCharacter visible
        AProjectUmeowmiCharacter* ProjectCharacter = Cast<AProjectUmeowmiCharacter>(PlayerCharacter);
        if (ProjectCharacter)
        {
            UPUDialogueBox* DialogueBox = ProjectCharacter->GetDialogueBox();
            if (DialogueBox)
            {
                DialogueBox->Open(CurrentDialogueContext);
            }
            else
            {
                //UE_LOG(LogTemp,Error, TEXT("TalkingObject::StartSpecificDialogue - Failed to get dialogue box from player character!"));
            }
        }
        else
        {
            //UE_LOG(LogTemp,Error, TEXT("TalkingObject::StartSpecificDialogue - Failed to cast player character to ProjectUmeowmiCharacter!"));
        }
    }
    else
    {
        //UE_LOG(LogTemp,Error, TEXT("TalkingObject::StartSpecificDialogue - Failed to create dialogue context!"));
    }
}

// Collision events
void ATalkingObject::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (OtherActor == PlayerCharacter)
    {
        //UE_LOG(LogTemp,Display, TEXT("TalkingObject::OnInteractionSphereBeginOverlap - Player entered range of %s (Class: %s)"), 
        //    *GetName(), *GetClass()->GetName());
        bPlayerInRange = true;
        UpdateInteractionWidget();
        OnPlayerEnteredInteractionSphere.Broadcast(this);
        
        // Register this talking object with the player character
        if (AProjectUmeowmiCharacter* ProjectCharacter = Cast<AProjectUmeowmiCharacter>(PlayerCharacter))
        {
            //UE_LOG(LogTemp,Display, TEXT("TalkingObject::OnInteractionSphereBeginOverlap - Registering talking object with character"));
            ProjectCharacter->RegisterTalkingObject(this);
        }
        else
        {
            //UE_LOG(LogTemp,Warning, TEXT("TalkingObject::OnInteractionSphereBeginOverlap - Failed to cast player character to ProjectUmeowmiCharacter"));
        }
    }
}

void ATalkingObject::OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (OtherActor == PlayerCharacter)
    {
        //UE_LOG(LogTemp,Log, TEXT("TalkingObject::OnInteractionSphereEndOverlap - Player exited range of %s"), *GetName());
        bPlayerInRange = false;
        UpdateInteractionWidget();
        OnPlayerExitedInteractionSphere.Broadcast(this);

        // If we're not currently interacting, unregister from the character
        if (!bIsInteracting)
        {
            if (AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(PlayerCharacter))
            {
                //UE_LOG(LogTemp,Log, TEXT("TalkingObject::OnInteractionSphereEndOverlap - Unregistering talking object from character (no interaction occurred)"));
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

void ATalkingObject::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    //UE_LOG(LogTemp,Log, TEXT("TalkingObject::EndPlay - Cleaning up talking object: %s"), *GetName());
    
    // Clear dialogue context to prevent dangling references
    if (CurrentDialogueContext)
    {
        //UE_LOG(LogTemp,Log, TEXT("TalkingObject::EndPlay - Clearing dialogue context"));
        CurrentDialogueContext = nullptr;
    }
    
    // Clear used dialogues set
    UsedDialogues.Empty();
    
    // Unregister from player character if still registered
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(PC->GetPawn()))
            {
                //UE_LOG(LogTemp,Log, TEXT("TalkingObject::EndPlay - Unregistering from player character"));
                Character->UnregisterTalkingObject(this);
            }
        }
    }
    
    Super::EndPlay(EndPlayReason);
} 