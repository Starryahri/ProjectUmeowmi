#include "TalkingObject.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraComponent.h"
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
    // DO NOT set collision profile during CDO construction - will be set in PostInitializeComponents
    InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ATalkingObject::OnInteractionSphereBeginOverlap);
    InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ATalkingObject::OnInteractionSphereEndOverlap);

    // Create and setup the widget component (attached to root so widget and sphere can be positioned independently)
    InteractionWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionWidget"));
    InteractionWidget->SetupAttachment(RootComponent);
    InteractionWidget->SetWidgetSpace(InteractionWidgetSpace);
    InteractionWidget->SetVisibility(false);
}

void ATalkingObject::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    // Sync sphere radius to InteractionRange (derived class constructors have run, Blueprint defaults applied for instances)
    SyncInteractionSphereToRange();

    // Set collision profile after GEngine is initialized (safe from CDO construction)
    // Double-check: ensure we're not in CDO construction AND GEngine is available AND component is not CDO
    if (InteractionSphere && !HasAnyFlags(RF_ClassDefaultObject) && !InteractionSphere->HasAnyFlags(RF_ClassDefaultObject) && GEngine)
    {
        InteractionSphere->SetCollisionProfileName(TEXT("Trigger"));
    }
}

#if WITH_EDITOR
void ATalkingObject::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropertyName = PropertyChangedEvent.GetPropertyName();
    if (PropertyName == GET_MEMBER_NAME_CHECKED(ATalkingObject, InteractionRange))
    {
        SyncInteractionSphereToRange();
    }
}
#endif

void ATalkingObject::BeginPlay()
{
    Super::BeginPlay();

    // Sync sphere radius and widget position to InteractionRange (handles Blueprint overrides and derived class values)
    SyncInteractionSphereToRange();

    // Apply widget space (Screen or World) - World space allows scaling with orthographic zoom
    if (InteractionWidget)
    {
        InteractionWidget->SetWidgetSpace(InteractionWidgetSpace);
    }

    // Create the widget instance
    if (InteractionWidgetClass)
    {
        InteractionWidget->SetWidgetClass(InteractionWidgetClass);
    }

    // Cache base DrawSize for ortho scaling (used when bScaleWidgetWithOrthoZoom is true)
    if (InteractionWidget)
    {
        CachedBaseDrawSize = InteractionWidget->GetDrawSize();
        if (CachedBaseDrawSize.X <= 0 || CachedBaseDrawSize.Y <= 0)
        {
            CachedBaseDrawSize = FVector2D(500.0f, 500.0f);
        }
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

    if (bShowDebugRange)
    {
        DrawDebugRange();
    }
    if (bScaleWidgetWithOrthoZoom && InteractionWidget && InteractionWidget->IsVisible())
    {
        UpdateOrthoWidgetScale();
    }
}

bool ATalkingObject::CheckCondition_Implementation(const UDlgContext* Context, FName ConditionName) const
{
    UE_LOG(LogTemp, Display, TEXT("=== TalkingObject::CheckCondition CALLED ==="));
    UE_LOG(LogTemp, Display, TEXT("Condition Name: %s"), *ConditionName.ToString());
    UE_LOG(LogTemp, Display, TEXT("Context: %s"), Context ? TEXT("VALID") : TEXT("NULL"));
    UE_LOG(LogTemp, Display, TEXT("This Object: %s"), *GetName());
    UE_LOG(LogTemp, Display, TEXT("TalkingObject::CheckCondition - Returning FALSE (default behavior)"));
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
    UE_LOG(LogTemp, Display, TEXT("=== ATalkingObject::OnDialogueEvent CALLED ==="));
    UE_LOG(LogTemp, Display, TEXT("Event Name: %s"), *EventName.ToString());
    UE_LOG(LogTemp, Display, TEXT("Context: %s"), Context ? TEXT("VALID") : TEXT("NULL"));
    UE_LOG(LogTemp, Display, TEXT("This Object: %s"), *GetName());
    
    // Handle order generation event
    if (EventName == TEXT("GenerateOrder"))
    {
        UE_LOG(LogTemp, Display, TEXT("ATalkingObject::OnDialogueEvent - Handling GenerateOrder event"));
        
        // Check if this is a dish giver
        if (APUDishGiver* DishGiver = Cast<APUDishGiver>(this))
        {
            UE_LOG(LogTemp, Display, TEXT("ATalkingObject::OnDialogueEvent - Cast to APUDishGiver successful, calling GenerateAndGiveOrderToPlayer"));
            DishGiver->GenerateAndGiveOrderToPlayer();
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ATalkingObject::OnDialogueEvent - GenerateOrder event called on non-dish-giver object: %s"), *GetName());
            return false;
        }
    }
    
    UE_LOG(LogTemp, Display, TEXT("ATalkingObject::OnDialogueEvent - Unknown event: %s"), *EventName.ToString());
    return false;
}


// Interaction methods
bool ATalkingObject::CanInteract() const
{
    UE_LOG(LogTemp, Display, TEXT("TalkingObject::CanInteract - %s: bPlayerInRange=%d, bIsInteracting=%d, AvailableDialogues=%d"),
        *GetName(), bPlayerInRange, bIsInteracting, AvailableDialogues.Num());
    return bPlayerInRange && !bIsInteracting && AvailableDialogues.Num() > 0;
}

void ATalkingObject::StartInteraction()
{
    if (CanInteract())
    {
        UE_LOG(LogTemp, Log, TEXT("TalkingObject::StartInteraction - Starting interaction"));
        StartDialogueAndSetInteracting(GetRandomDialogue());
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
    
    // Properly clear the dialogue context to prevent dangling references
    if (CurrentDialogueContext)
    {
        UE_LOG(LogTemp, Log, TEXT("TalkingObject::EndInteraction - Clearing dialogue context"));
        CurrentDialogueContext = nullptr;
    }

    // Only unregister if the player has left the interaction sphere.
    // If they're still in range, keep them registered so they can interact again without having to leave and re-enter.
    if (!bPlayerInRange)
    {
        if (AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn()))
        {
            Character->UnregisterTalkingObject(this);
        }
    }
    else
    {
        // Player still in range - update widget so interact prompt shows again
        UpdateInteractionWidget();
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

void ATalkingObject::StartDialogueAndSetInteracting(UDlgDialogue* Dialogue)
{
    if (Dialogue)
    {
        bIsInteracting = true;
        StartSpecificDialogue(Dialogue);
    }
}

void ATalkingObject::StartSpecificDialogue(UDlgDialogue* Dialogue)
{
    if (!Dialogue)
    {
        UE_LOG(LogTemp, Warning, TEXT("TalkingObject::StartSpecificDialogue - Invalid dialogue provided"));
        return;
    }

    // Clear any existing dialogue context first to prevent dangling references
    if (CurrentDialogueContext)
    {
        UE_LOG(LogTemp, Log, TEXT("TalkingObject::StartSpecificDialogue - Clearing existing dialogue context"));
        CurrentDialogueContext = nullptr;
    }

    // Get the player controller first
    APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Error, TEXT("TalkingObject::StartSpecificDialogue - Failed to get player controller!"));
        return;
    }

    // Get the local player
    ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
    if (!LocalPlayer)
    {
        UE_LOG(LogTemp, Error, TEXT("TalkingObject::StartSpecificDialogue - Failed to get local player!"));
        return;
    }

    // Get the player character
    ACharacter* PlayerCharacter = Cast<ACharacter>(PlayerController->GetPawn());
    if (!PlayerCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("TalkingObject::StartSpecificDialogue - Failed to get player character!"));
        return;
    }

    // Build participants using the same filtered list as debug (AllowedParticipantNames, ObjectType)
    TArray<UObject*> Participants = BuildActiveParticipantsList();

    UE_LOG(LogTemp, Display, TEXT("TalkingObject::StartSpecificDialogue - Found %d active participants for this interactable"), Participants.Num());

    // Validate we have at least one participant
    if (Participants.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("TalkingObject::StartSpecificDialogue - No valid participants found!"));
        return;
    }

    // Final validation: ensure all participants are still valid before starting dialogue
    for (int32 i = Participants.Num() - 1; i >= 0; --i)
    {
        if (!IsValid(Participants[i]))
        {
            UE_LOG(LogTemp, Warning, TEXT("TalkingObject::StartSpecificDialogue - Removing invalid participant at index %d"), i);
            Participants.RemoveAt(i);
        }
    }

    // Check again after removing invalid participants
    if (Participants.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("TalkingObject::StartSpecificDialogue - No valid participants remaining after final validation!"));
        return;
    }

    // Start the dialogue with validated participants
    CurrentDialogueContext = UDlgManager::StartDialogue(Dialogue, Participants);

    // Log the participants in the dialogue context when debug is enabled
    if (CurrentDialogueContext && bShowDebugParticipants)
    {
        const TMap<FName, UObject*>& ParticipantsMap = CurrentDialogueContext->GetParticipantsMap();
        UE_LOG(LogTemp, Warning, TEXT("[%s] Dialogue started - Using %d participants:"), *GetName(), ParticipantsMap.Num());
        int32 idx = 0;
        for (const auto& Pair : ParticipantsMap)
        {
            FName ParticipantNameKey = Pair.Key;
            UObject* Participant = Pair.Value;
            FString TypeStr;
            if (APawn* Pawn = Cast<APawn>(Participant))
            {
                TypeStr = FString::Printf(TEXT("Pawn (Character=%s)"), Pawn->IsA<ACharacter>() ? TEXT("Yes") : TEXT("No"));
            }
            else if (APlayerController* PC = Cast<APlayerController>(Participant))
            {
                TypeStr = TEXT("PlayerController");
            }
            else if (AActor* Actor = Cast<AActor>(Participant))
            {
                TypeStr = FString::Printf(TEXT("Actor (%s)"), *Actor->GetClass()->GetName());
            }
            else
            {
                TypeStr = Participant ? FString::Printf(TEXT("UObject (%s)"), *Participant->GetClass()->GetName()) : TEXT("NULL");
            }
            FText PartDisplayName = IsValid(Participant) ? IDlgDialogueParticipant::Execute_GetParticipantDisplayName(Participant, NAME_None) : FText::GetEmpty();
            UE_LOG(LogTemp, Warning, TEXT("  [%d] Name=%s DisplayName=\"%s\" Type=%s Path=%s"),
                idx++, *ParticipantNameKey.ToString(), *PartDisplayName.ToString(), *TypeStr,
                IsValid(Participant) ? *Participant->GetPathName() : TEXT("(invalid)"));
        }
    }

    if (CurrentDialogueContext)
    {
        // Make the dialogue box from the AProjectUmeowmiCharacter visible
        AProjectUmeowmiCharacter* ProjectCharacter = Cast<AProjectUmeowmiCharacter>(PlayerCharacter);
        if (ProjectCharacter)
        {
            UPUDialogueBox* DialogueBox = ProjectCharacter->GetDialogueBox();
            if (DialogueBox)
            {
                DialogueBox->Open(CurrentDialogueContext);
            }
        }
    }
}

// Collision events
void ATalkingObject::OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
    if (OtherActor == PlayerCharacter)
    {
        bPlayerInRange = true;
        UpdateInteractionWidget();
        OnPlayerEnteredInteractionSphere.Broadcast(this);

        if (bShowDebugParticipants)
        {
            TArray<UObject*> ActiveParticipants = BuildActiveParticipantsList();
            UE_LOG(LogTemp, Warning, TEXT("[%s] Player entered - Active participants for this interactable (%d):"), *GetName(), ActiveParticipants.Num());
            for (int32 i = 0; i < ActiveParticipants.Num(); ++i)
            {
                UObject* P = ActiveParticipants[i];
                if (!IsValid(P)) continue;
                FName PartName = IDlgDialogueParticipant::Execute_GetParticipantName(P);
                FText PartDisplayName = IDlgDialogueParticipant::Execute_GetParticipantDisplayName(P, NAME_None);
                FString TypeStr;
                if (APawn* Pawn = Cast<APawn>(P))
                {
                    TypeStr = FString::Printf(TEXT("Pawn (Character=%s)"), Pawn->IsA<ACharacter>() ? TEXT("Yes") : TEXT("No"));
                }
                else if (APlayerController* PC = Cast<APlayerController>(P))
                {
                    TypeStr = TEXT("PlayerController");
                }
                else if (AActor* Actor = Cast<AActor>(P))
                {
                    TypeStr = FString::Printf(TEXT("Actor (%s)"), *Actor->GetClass()->GetName());
                }
                else
                {
                    TypeStr = FString::Printf(TEXT("UObject (%s)"), *P->GetClass()->GetName());
                }
                UE_LOG(LogTemp, Warning, TEXT("  [%d] Name=%s DisplayName=\"%s\" Type=%s Path=%s"),
                    i, *PartName.ToString(), *PartDisplayName.ToString(), *TypeStr, *P->GetPathName());
            }
        }
        
        // Register this talking object with the player character
        if (AProjectUmeowmiCharacter* ProjectCharacter = Cast<AProjectUmeowmiCharacter>(PlayerCharacter))
        {
            ProjectCharacter->RegisterTalkingObject(this);
        }
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
void ATalkingObject::SyncInteractionSphereToRange()
{
    if (InteractionSphere)
    {
        InteractionSphere->SetSphereRadius(InteractionRange);
    }
}

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

    // Enable tick when widget is visible and we need ortho scaling
    PrimaryActorTick.bCanEverTick = bShowDebugRange || (bCanInteractNow && bScaleWidgetWithOrthoZoom);
}

void ATalkingObject::UpdateOrthoWidgetScale()
{
    if (!InteractionWidget || !GetWorld())
    {
        return;
    }

    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC || !PC->GetPawn())
    {
        return;
    }

    UCameraComponent* Camera = PC->GetPawn()->FindComponentByClass<UCameraComponent>();
    if (!Camera || Camera->ProjectionMode != ECameraProjectionMode::Orthographic)
    {
        return;
    }

    const float CurrentOrthoWidth = Camera->OrthoWidth;
    if (CurrentOrthoWidth <= 0.0f)
    {
        return;
    }

    // Scale DrawSize so widget appears larger when zoomed in (small OrthoWidth) and smaller when zoomed out
    const float ScaleFactor = ReferenceOrthoWidth / CurrentOrthoWidth;
    const float ClampedScale = FMath::Clamp(ScaleFactor, 0.1f, 10.0f);
    const FVector2D NewDrawSize(
        FMath::RoundToFloat(CachedBaseDrawSize.X * ClampedScale),
        FMath::RoundToFloat(CachedBaseDrawSize.Y * ClampedScale)
    );

    // Clamp to valid render target dimensions
    const int32 MinSize = 16;
    const int32 MaxSize = 4096;
    const FVector2D ClampedDrawSize(
        FMath::Clamp(static_cast<int32>(NewDrawSize.X), MinSize, MaxSize),
        FMath::Clamp(static_cast<int32>(NewDrawSize.Y), MinSize, MaxSize)
    );

    InteractionWidget->SetDrawSize(ClampedDrawSize);
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
    if (!bShowDebugRange || !InteractionSphere)
    {
        return;
    }

    const FVector Location = InteractionSphere->GetComponentLocation();
    const float Radius = InteractionSphere->GetScaledSphereRadius();
    const FColor DebugColor = FColor::Green;
    const float LifeTime = -1.0f;
    const uint8 DepthPriority = 0;
    const float Thickness = 2.0f;

    // Draw the interaction range sphere (uses actual sphere position and radius)
    DrawDebugSphere(
        GetWorld(),
        Location,
        Radius,
        32, // Number of segments
        DebugColor,
        false,
        LifeTime,
        DepthPriority,
        Thickness
    );
}

TArray<UObject*> ATalkingObject::BuildActiveParticipantsList() const
{
    TArray<UObject*> Participants;

    // Always include the talking object itself (the interactable you're talking to)
    if (IsValid(const_cast<ATalkingObject*>(this)))
    {
        Participants.Add(const_cast<ATalkingObject*>(this));
    }

    // Add any additional participants explicitly listed in AllowedParticipantNames

    // For NPCs and Props, add participants from the level when in AllowedParticipantNames
    if (ObjectType == ETalkingObjectType::NPC || ObjectType == ETalkingObjectType::Prop)
    {
        TArray<UObject*> AllParticipants = UDlgManager::GetObjectsWithDialogueParticipantInterface(const_cast<ATalkingObject*>(this));

        for (UObject* Participant : AllParticipants)
        {
            if (!IsValid(Participant)) continue;

            if (AActor* ActorParticipant = Cast<AActor>(Participant))
            {
                if (!IsValid(ActorParticipant) || !ActorParticipant->IsValidLowLevel()) continue;
            }

            if (Participant == this)
            {
                continue; // Skip self (already added)
            }

            if (!Participant->GetClass()->ImplementsInterface(UDlgDialogueParticipant::StaticClass())) continue;

            FName FoundParticipantName = IDlgDialogueParticipant::Execute_GetParticipantName(Participant);
            if (FoundParticipantName.IsNone()) continue;

            // Only add participants explicitly listed in AllowedParticipantNames (empty = add none from level)
            if (AllowedParticipantNames.Num() > 0 && AllowedParticipantNames.Contains(FoundParticipantName))
            {
                bool bAlreadyAdded = false;
                for (UObject* Existing : Participants)
                {
                    if (IsValid(Existing) && Existing->GetClass()->ImplementsInterface(UDlgDialogueParticipant::StaticClass()))
                    {
                        if (IDlgDialogueParticipant::Execute_GetParticipantName(Existing) == FoundParticipantName)
                        {
                            bAlreadyAdded = true;
                            break;
                        }
                    }
                }
                if (!bAlreadyAdded)
                {
                    Participants.Add(Participant);
                }
            }
        }
    }

    return Participants;
}

void ATalkingObject::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(LogTemp, Log, TEXT("TalkingObject::EndPlay - Cleaning up talking object: %s"), *GetName());
    
    // Clear dialogue context to prevent dangling references
    if (CurrentDialogueContext)
    {
        UE_LOG(LogTemp, Log, TEXT("TalkingObject::EndPlay - Clearing dialogue context"));
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
                UE_LOG(LogTemp, Log, TEXT("TalkingObject::EndPlay - Unregistering from player character"));
                Character->UnregisterTalkingObject(this);
            }
        }
    }
    
    Super::EndPlay(EndPlayReason);
} 