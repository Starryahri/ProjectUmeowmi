#include "TalkingObject.h"

#include "ActorSequenceComponent.h"
#include "ActorSequencePlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "DlgSystem/DlgContext.h"
#include "DlgSystem/DlgDialogue.h"
#include "DlgSystem/DlgManager.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ProjectUmeowmi/ProjectUmeowmiCharacter.h"
#include "ProjectUmeowmi/PUProjectUmeowmiGameInstance.h"
#include "ProjectUmeowmi/UI/PUDialogueBox.h"
#include "ProjectUmeowmi/UI/PUEmoteData.h"
#include "ProjectUmeowmi/UI/PUEmoteWidget.h"
#include "PUDishGiver.h"
//#include "DlgSystem/DlgDialogueParticipant.h"

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

    // Create and setup the interaction widget component (attached to root so widget and sphere can be positioned independently)
    InteractionWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionWidget"));
    InteractionWidget->SetupAttachment(RootComponent);
    InteractionWidget->SetWidgetSpace(InteractionWidgetSpace);
    InteractionWidget->SetVisibility(false);

    // Create and setup the emote widget component (also attached to root so it can be positioned independently)
    EmoteWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("EmoteWidget"));
    EmoteWidget->SetupAttachment(RootComponent);
    EmoteWidget->SetWidgetSpace(EmoteWidgetSpace);
    EmoteWidget->SetVisibility(false);
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

    // Configure emote widget (set space after SetWidgetClass - SetWidgetClass can reset space to World).
    // Do not set visibility false here - constructor already hides it. Otherwise we overwrite ShowEmoteByTag when called from Blueprint BeginPlay.
    if (EmoteWidget)
    {
        if (EmoteWidgetClass)
        {
            EmoteWidget->SetWidgetClass(EmoteWidgetClass);
        }
        EmoteWidget->SetWidgetSpace(EmoteWidgetSpace);
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

void ATalkingObject::TickFacePlayerLerp(float DeltaTime)
{
    // Player lerp to face NPC (runs first so both can lerp in same frame)
    if (bIsLerpingPlayerToFaceNPC)
    {
        APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
        ACharacter* PlayerCharacter = PC && PC->GetPawn() ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
        if (PlayerCharacter)
        {
            const float InterpSpeed = FMath::Max(1.0f, NPCFacingRotationSpeed) / 45.0f;
            const FRotator CurrentRot = PlayerCharacter->GetActorRotation();
            const FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetPlayerRotation, DeltaTime, InterpSpeed);
            PlayerCharacter->SetActorRotation(NewRot);

            const float YawTolerance = 1.0f;
            if (FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRot.Yaw, TargetPlayerRotation.Yaw)) < YawTolerance)
            {
                PlayerCharacter->SetActorRotation(TargetPlayerRotation);
                bIsLerpingPlayerToFaceNPC = false;
            }
        }
        else
        {
            bIsLerpingPlayerToFaceNPC = false;
        }
    }

    // Lerp back to original (player-driven, so it always runs)
    if (bIsLerpingBackToOriginal && GetRootComponent())
    {
        const float InterpSpeed = FMath::Max(1.0f, NPCFacingRotationSpeed) / 45.0f;
        const FRotator CurrentRot = GetRootComponent()->GetComponentRotation();
        const FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetNPCRotation, DeltaTime, InterpSpeed);
        GetRootComponent()->SetWorldRotation(NewRot);

        const float YawTolerance = 1.0f;
        if (FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRot.Yaw, TargetNPCRotation.Yaw)) < YawTolerance)
        {
            GetRootComponent()->SetWorldRotation(TargetNPCRotation);
            bIsLerpingBackToOriginal = false;
            if (bShowDebugFacePlayerLerp)
            {
                UE_LOG(LogTemp, Warning, TEXT("[FacePlayerLerp] %s LERP BACK DONE"), *GetName());
            }
            if (!bPlayerInRange)
            {
                if (AProjectUmeowmiCharacter* Character = Cast<AProjectUmeowmiCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn()))
                {
                    Character->UnregisterTalkingObject(this);
                }
            }
        }
        return;
    }

    if (!bIsLerpingToFacePlayer || !GetRootComponent()) return;

    const float InterpSpeed = FMath::Max(1.0f, NPCFacingRotationSpeed) / 45.0f;
    const FRotator CurrentRot = GetRootComponent()->GetComponentRotation();
    const FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetNPCRotation, DeltaTime, InterpSpeed);
    GetRootComponent()->SetWorldRotation(NewRot);

    if (bShowDebugFacePlayerLerp && (++FacePlayerLerpTickCount % 10 == 1))
    {
        const float DeltaYaw = FMath::FindDeltaAngleDegrees(CurrentRot.Yaw, TargetNPCRotation.Yaw);
        UE_LOG(LogTemp, Warning, TEXT("[FacePlayerLerp] %s TICK #%d CurrentYaw=%.1f TargetYaw=%.1f DeltaYaw=%.1f DeltaTime=%.3f"),
            *GetName(), FacePlayerLerpTickCount, CurrentRot.Yaw, TargetNPCRotation.Yaw, DeltaYaw, DeltaTime);
    }

    const float YawTolerance = 1.0f;
    if (FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentRot.Yaw, TargetNPCRotation.Yaw)) < YawTolerance)
    {
        GetRootComponent()->SetWorldRotation(TargetNPCRotation);
        bIsLerpingToFacePlayer = false;
        if (bShowDebugFacePlayerLerp)
        {
            UE_LOG(LogTemp, Warning, TEXT("[FacePlayerLerp] %s DONE"), *GetName());
        }
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

    // Handle generic unlock events using ParticipantName as the LockID in the GameInstance.
    // This allows doors (and other talking objects) to participate in the global lock system.
    if (EventName == TEXT("UnlockDoor") || EventName == TEXT("UnlockLevelTransition") || EventName == TEXT("UnlockTransition"))
    {
        if (ParticipantName == NAME_None)
        {
            UE_LOG(LogTemp, Warning, TEXT("ATalkingObject::OnDialogueEvent - Unlock event received but ParticipantName is NAME_None on %s"), *GetName());
            return false;
        }

        if (UPUProjectUmeowmiGameInstance* GI = Cast<UPUProjectUmeowmiGameInstance>(GetGameInstance()))
        {
            GI->UnlockLevelTransition(ParticipantName);
            UE_LOG(LogTemp, Log, TEXT("ATalkingObject::OnDialogueEvent - Unlocked object with ParticipantName as LockID: %s"), *ParticipantName.ToString());
            return true;
        }

        UE_LOG(LogTemp, Warning, TEXT("ATalkingObject::OnDialogueEvent - Unlock event but GameInstance was null for %s"), *GetName());
        return false;
    }

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
    if (ObjectType == ETalkingObjectType::Door)
    {
        const bool bDoorUnlocked = IsDoorUnlocked();

        // Unlocked door: behaves like before (plays DoorAction sequence)
        if (bDoorUnlocked)
        {
            return bPlayerInRange && !bIsInteracting;
        }

        // Locked door: allow interaction only if we have a LockedDoorDialogue to show
        const bool bHasLockedDialogue = (LockedDoorDialogue != nullptr);
        return bPlayerInRange && !bIsInteracting && bHasLockedDialogue;
    }
    return bPlayerInRange && !bIsInteracting && AvailableDialogues.Num() > 0;
}

void ATalkingObject::StartInteraction()
{
    if (!CanInteract())
    {
        UE_LOG(LogTemp, Warning, TEXT("TalkingObject::StartInteraction - Cannot start interaction! bPlayerInRange: %d, bIsInteracting: %d, AvailableDialogues.Num(): %d"),
            bPlayerInRange, bIsInteracting, AvailableDialogues.Num());
        return;
    }

    if (ObjectType == ETalkingObjectType::Door)
    {
        const bool bDoorUnlocked = IsDoorUnlocked();

        // If locked: play the locked-door dialogue (if configured) instead of opening
        if (!bDoorUnlocked)
        {
            if (LockedDoorDialogue)
            {
                UE_LOG(LogTemp, Log, TEXT("TalkingObject::StartInteraction - Door '%s' is locked. Starting LockedDoorDialogue using ParticipantName as LockID: %s"),
                    *GetName(), *ParticipantName.ToString());
                StartDialogueAndSetInteracting(LockedDoorDialogue);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("TalkingObject::StartInteraction - Door '%s' is locked but has no LockedDoorDialogue set"), *GetName());
            }
            return;
        }

        // Door type: find DoorAction component and toggle open/close
        TArray<UActorSequenceComponent*> SeqComps;
        GetComponents<UActorSequenceComponent>(SeqComps);
        UActorSequenceComponent* DoorActionComp = nullptr;
        for (UActorSequenceComponent* Comp : SeqComps)
        {
            if (Comp && Comp->GetFName() == FName(TEXT("DoorAction")))
            {
                DoorActionComp = Comp;
                break;
            }
        }
        if (DoorActionComp)
        {
            bIsInteracting = true;
            if (UActorSequencePlayer* Player = DoorActionComp->GetSequencePlayer())
            {
                if (bIsDoorOpen)
                {
                    Player->PlayReverse();
                    bIsDoorOpen = false;
                }
                else
                {
                    DoorActionComp->PlaySequence();
                    bIsDoorOpen = true;
                }
            }
            bIsInteracting = false; // Door interaction is instant (sequence plays), no dialogue to wait for
            UpdateInteractionWidget();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("TalkingObject::StartInteraction - Door '%s' has no ActorSequenceComponent named 'DoorAction'"), *GetName());
        }
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("TalkingObject::StartInteraction - Starting interaction"));
    StartDialogueAndSetInteracting(GetRandomDialogue());
}

void ATalkingObject::EndInteraction()
{
    UE_LOG(LogTemp, Log, TEXT("TalkingObject::EndInteraction - Ending interaction for %s"), *GetName());
    bIsInteracting = false;
    bIsLerpingToFacePlayer = false;
    bIsLerpingPlayerToFaceNPC = false;

    // For NPCs: lerp back to original rotation (only if we rotated them during dialogue)
    if (ObjectType == ETalkingObjectType::NPC && bRotateNPCToFacePlayer && GetRootComponent())
    {
        TargetNPCRotation = OriginalNPCRotation;
        bIsLerpingBackToOriginal = true;
        if (bShowDebugFacePlayerLerp)
        {
            UE_LOG(LogTemp, Warning, TEXT("[FacePlayerLerp] %s LERP BACK START TargetYaw=%.1f"), *GetName(), OriginalNPCRotation.Yaw);
        }
    }

    // Properly clear the dialogue context to prevent dangling references
    if (CurrentDialogueContext)
    {
        UE_LOG(LogTemp, Log, TEXT("TalkingObject::EndInteraction - Clearing dialogue context"));
        CurrentDialogueContext = nullptr;
    }

    // Only unregister if the player has left the interaction sphere.
    // If we're lerping back, keep registered so player can drive the lerp; unregister when lerp completes.
    if (!bPlayerInRange && !bIsLerpingBackToOriginal)
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
        // Hide interaction prompt while actively in dialogue
        UpdateInteractionWidget();
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

    // For NPCs: rotate both to face each other
    if (ObjectType == ETalkingObjectType::NPC)
    {
        const FVector NPCLoc = GetActorLocation();
        const FVector PlayerLoc = PlayerCharacter->GetActorLocation();

        // NPC faces player (lerped via timer)
        if (bRotateNPCToFacePlayer && GetRootComponent())
        {
            FVector DirToPlayer = PlayerLoc - NPCLoc;
            DirToPlayer.Z = 0.0f;
            if (DirToPlayer.Normalize())
            {
                const FRotator CurrentRot = GetRootComponent()->GetComponentRotation();
                const float TargetYaw = bRotateNPCAroundYaw ? (DirToPlayer.Rotation().Yaw + NPCFacingYawOffset) : CurrentRot.Yaw;
                const float TargetPitch = bRotateNPCAroundPitch ? 0.0f : CurrentRot.Pitch;
                const float TargetRoll = bRotateNPCAroundRoll ? 0.0f : CurrentRot.Roll;

                OriginalNPCRotation = CurrentRot;
                TargetNPCRotation = FRotator(TargetPitch, TargetYaw, TargetRoll);
                bIsLerpingToFacePlayer = true;
                if (bShowDebugFacePlayerLerp)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[FacePlayerLerp] %s START CurrentYaw=%.1f TargetYaw=%.1f"),
                        *GetName(), CurrentRot.Yaw, TargetNPCRotation.Yaw);
                }
            }
        }

        // Player faces NPC: lerp the whole character (driven by TickFacePlayerLerp)
        if (bRotatePlayerToFaceNPC)
        {
            FVector DirToNPC = NPCLoc - PlayerLoc;
            DirToNPC.Z = 0.0f;
            if (DirToNPC.Normalize())
            {
                const FRotator CurrentRot = PlayerCharacter->GetActorRotation();
                const float TargetYaw = bRotatePlayerAroundYaw ? (DirToNPC.Rotation().Yaw + PlayerFacingYawOffset) : CurrentRot.Yaw;
                const float TargetPitch = bRotatePlayerAroundPitch ? 0.0f : CurrentRot.Pitch;
                const float TargetRoll = bRotatePlayerAroundRoll ? 0.0f : CurrentRot.Roll;

                TargetPlayerRotation = FRotator(TargetPitch, TargetYaw, TargetRoll);
                bIsLerpingPlayerToFaceNPC = true;
            }
        }
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

        // If we're not currently interacting and not lerping back, unregister from the character
        // (Keep registered during lerp-back so player can drive it)
        if (!bIsInteracting && !bIsLerpingBackToOriginal)
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
void ATalkingObject::HideInteractionWidgetForTransition()
{
    if (InteractionWidget)
    {
        InteractionWidget->SetVisibility(false);
    }
}

void ATalkingObject::SetInteractionWidgetClass(TSubclassOf<UTalkingObjectWidget> NewWidgetClass)
{
    InteractionWidgetClass = NewWidgetClass;
    if (InteractionWidget && InteractionWidgetClass)
    {
        InteractionWidget->SetWidgetClass(InteractionWidgetClass);
        UpdateInteractionWidget();
    }
}

void ATalkingObject::SetInteractionKey(FName NewKey)
{
    InteractionKey = NewKey;
    UpdateInteractionWidget();
}

void ATalkingObject::SetInteractionIcon(UTexture2D* NewIcon)
{
    InteractionIcon = NewIcon;
    UpdateInteractionWidget();
}

void ATalkingObject::SyncInteractionSphereToRange()
{
    if (InteractionSphere)
    {
        InteractionSphere->SetSphereRadius(InteractionRange);
    }
}

void ATalkingObject::RefreshInteractionWidget()
{
	UpdateInteractionWidget();
}

void ATalkingObject::UpdateInteractionWidget()
{
    if (!InteractionWidget)
    {
        return;
    }

    // Hide interaction UI during level transitions
    if (UWorld* World = GetWorld())
    {
        if (UPUProjectUmeowmiGameInstance* GI = Cast<UPUProjectUmeowmiGameInstance>(World->GetGameInstance()))
        {
            if (GI->IsLevelTransitionInProgress())
            {
                InteractionWidget->SetVisibility(false);
                return;
            }
        }
    }

    const bool bCanInteractNow = CanInteract();
    InteractionWidget->SetVisibility(bCanInteractNow);

    if (bCanInteractNow)
    {
        if (UTalkingObjectWidget* Widget = Cast<UTalkingObjectWidget>(InteractionWidget->GetWidget()))
        {
            FString DisplayKey = InteractionKey.ToString();
            bool bIsSelected = true;
            int32 Total = 1;

            AProjectUmeowmiCharacter* PlayerChar = nullptr;
            if (UWorld* World = GetWorld())
            {
                if (APlayerController* PC = World->GetFirstPlayerController())
                {
                    PlayerChar = Cast<AProjectUmeowmiCharacter>(PC->GetPawn());
                }
            }
            if (PlayerChar)
            {
                Total = PlayerChar->GetOverlappingTalkingObjectCount();
                if (Total >= 2)
                {
                    bIsSelected = (PlayerChar->GetCurrentTalkingObject() == this);
                }
            }

            Widget->SetInteractionKey(DisplayKey);
            Widget->SetInteractionIcon(InteractionIcon);
            Widget->SetSelectionState(bIsSelected, Total);
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

bool ATalkingObject::IsDoorUnlocked() const
{
    // Only meaningful for Door type; other types are treated as unlocked here.
    if (ObjectType != ETalkingObjectType::Door)
    {
        return true;
    }

    // If no ParticipantName is set, treat the door as always unlocked.
    if (ParticipantName == NAME_None)
    {
        return true;
    }

    const UWorld* World = GetWorld();
    if (!World)
    {
        // Fail-open to avoid soft-locking the player due to missing world context.
        return true;
    }

    if (const UPUProjectUmeowmiGameInstance* GI = Cast<UPUProjectUmeowmiGameInstance>(World->GetGameInstance()))
    {
        return GI->IsLevelTransitionUnlocked(ParticipantName);
    }

    // If we can't reach the GameInstance, default to unlocked to avoid unintended locks.
    return true;
}

void ATalkingObject::BeginFadeOutEmote()
{
    if (!EmoteWidget)
    {
        ClearEmote();
        return;
    }

    if (UPUEmoteWidget* EmoteUserWidget = Cast<UPUEmoteWidget>(EmoteWidget->GetWidget()))
    {
        EmoteUserWidget->PlayFadeOut();

        const float FadeDuration = EmoteUserWidget->GetFadeUpDuration();
        const float TimerDuration = (FadeDuration > 0.0f) ? (FadeDuration / 2.0f) : 0.25f;
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(EmoteFadeOutTimerHandle);
            World->GetTimerManager().SetTimer(EmoteFadeOutTimerHandle, this, &ATalkingObject::ClearEmote, TimerDuration, false);
        }
        else
        {
            ClearEmote();
        }
    }
    else
    {
        ClearEmote();
    }
}

void ATalkingObject::ShowEmoteByTag(FGameplayTag EmoteTag)
{
    if (bShowDebugEmotes)
    {
        UE_LOG(LogTemp, Display, TEXT("[Emote] %s ShowEmoteByTag called with tag: %s"), *GetName(), *EmoteTag.ToString());
    }

    if (!bEnableEmotes || !EmoteWidget)
    {
        if (bShowDebugEmotes)
        {
            UE_LOG(LogTemp, Display, TEXT("[Emote] %s - Skipped: bEnableEmotes=%d EmoteWidget=%s"), *GetName(), bEnableEmotes ? 1 : 0, EmoteWidget ? TEXT("valid") : TEXT("null"));
        }
        return;
    }

    if (!EmoteTag.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("ATalkingObject::ShowEmoteByTag - Invalid emote tag on %s"), *GetName());
        return;
    }

    if (!EmoteDataTable)
    {
        UE_LOG(LogTemp, Warning, TEXT("ATalkingObject::ShowEmoteByTag - EmoteDataTable is not set on %s"), *GetName());
        return;
    }

    // DataTable rows use the tag's leaf name (part after last '.') lowercased, e.g. Emote.Happy -> "happy"
    FString TagStr = EmoteTag.ToString();
    int32 LastDot = INDEX_NONE;
    if (TagStr.FindLastChar(TEXT('.'), LastDot) && LastDot >= 0)
    {
        TagStr = TagStr.Mid(LastDot + 1);
    }
    TagStr = TagStr.ToLower();
    const FName RowName = FName(*TagStr);

    if (bShowDebugEmotes)
    {
        UE_LOG(LogTemp, Display, TEXT("[Emote] %s - Looking up row name: %s in EmoteDataTable (from tag %s)"), *GetName(), *RowName.ToString(), *EmoteTag.ToString());
    }

    const FPUEmoteData* EmoteRow = EmoteDataTable->FindRow<FPUEmoteData>(RowName, TEXT("ShowEmoteByTag"));
    if (!EmoteRow)
    {
        UE_LOG(LogTemp, Warning, TEXT("ATalkingObject::ShowEmoteByTag - No emote data row for tag %s on %s"), *EmoteTag.ToString(), *GetName());
        return;
    }

    if (!EmoteRow->Icon)
    {
        UE_LOG(LogTemp, Warning, TEXT("ATalkingObject::ShowEmoteByTag - Emote row %s has no Icon on %s"), *RowName.ToString(), *GetName());
        return;
    }

    // Ensure widget is created (e.g. when ShowEmoteByTag is called from Blueprint BeginPlay before our init)
    if (!EmoteWidget->GetWidget() && EmoteWidgetClass)
    {
        EmoteWidget->SetWidgetClass(EmoteWidgetClass);
        EmoteWidget->SetWidgetSpace(EmoteWidgetSpace);
    }

    UPUEmoteWidget* EmoteUserWidget = Cast<UPUEmoteWidget>(EmoteWidget->GetWidget());
    if (!EmoteUserWidget && EmoteWidgetClass)
    {
        if (bShowDebugEmotes)
        {
            UE_LOG(LogTemp, Display, TEXT("[Emote] %s - Widget was wrong type, re-setting EmoteWidgetClass and retrying"), *GetName());
        }
        EmoteWidget->SetWidgetClass(EmoteWidgetClass);
        EmoteWidget->SetWidgetSpace(EmoteWidgetSpace);
        EmoteUserWidget = Cast<UPUEmoteWidget>(EmoteWidget->GetWidget());
    }

    if (!EmoteUserWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("ATalkingObject::ShowEmoteByTag - EmoteWidget is not of type UPUEmoteWidget on %s (set EmoteWidgetClass on this actor)"), *GetName());
        return;
    }

    EmoteUserWidget->SetEmoteIcon(EmoteRow->Icon);
    EmoteWidget->SetWidgetSpace(EmoteWidgetSpace);
    EmoteWidget->SetVisibility(true);
    EmoteUserWidget->PlayFadeIn();
    ActiveEmoteTag = EmoteTag;

    if (bShowDebugEmotes)
    {
        UE_LOG(LogTemp, Display, TEXT("[Emote] %s - Showing emote %s (Icon=%s Duration=%.2f bLoop=%d)"), *GetName(), *EmoteTag.ToString(), EmoteRow->Icon ? *EmoteRow->Icon->GetName() : TEXT("null"), EmoteRow->Duration, EmoteRow->bLoop ? 1 : 0);
    }

    if (EmoteRow->Sound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, EmoteRow->Sound, GetActorLocation());
    }

    if (!EmoteRow->bLoop)
    {
        const float Duration = EmoteRow->Duration > 0.0f ? EmoteRow->Duration : 2.0f;
        if (UWorld* WorldPtr = GetWorld())
        {
            WorldPtr->GetTimerManager().ClearTimer(EmoteHideTimerHandle);
            WorldPtr->GetTimerManager().ClearTimer(EmoteFadeOutTimerHandle);
            WorldPtr->GetTimerManager().SetTimer(EmoteHideTimerHandle, this, &ATalkingObject::BeginFadeOutEmote, Duration, false);
            if (bShowDebugEmotes)
            {
                UE_LOG(LogTemp, Display, TEXT("[Emote] %s - Auto-hide timer set for %.2fs"), *GetName(), Duration);
            }
        }
    }
}

void ATalkingObject::ClearEmote()
{
    if (bShowDebugEmotes && (EmoteWidget && EmoteWidget->IsVisible()))
    {
        UE_LOG(LogTemp, Display, TEXT("[Emote] %s ClearEmote - hiding emote (was %s)"), *GetName(), *ActiveEmoteTag.ToString());
    }

    if (EmoteWidget)
    {
        if (UPUEmoteWidget* EmoteUserWidget = Cast<UPUEmoteWidget>(EmoteWidget->GetWidget()))
        {
            EmoteUserWidget->ClearEmoteIcon();
        }
        EmoteWidget->SetVisibility(false);
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(EmoteHideTimerHandle);
        World->GetTimerManager().ClearTimer(EmoteFadeOutTimerHandle);
    }

    ActiveEmoteTag = FGameplayTag();
}

bool ATalkingObject::IsEmoteActive() const
{
    return EmoteWidget && EmoteWidget->IsVisible();
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
    bIsLerpingToFacePlayer = false;
    bIsLerpingBackToOriginal = false;

    // Clear dialogue context to prevent dangling references
    if (CurrentDialogueContext)
    {
        UE_LOG(LogTemp, Log, TEXT("TalkingObject::EndPlay - Clearing dialogue context"));
        CurrentDialogueContext = nullptr;
    }
    
    // Clear used dialogues set
    UsedDialogues.Empty();

    // Clear any pending emote timers
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(EmoteHideTimerHandle);
        World->GetTimerManager().ClearTimer(EmoteFadeOutTimerHandle);
    }
    
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