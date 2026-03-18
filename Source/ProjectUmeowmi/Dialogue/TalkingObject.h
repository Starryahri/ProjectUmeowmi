#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/WidgetComponent.h"
#include "GameplayTagContainer.h"
#include "DlgSystem/DlgDialogueParticipant.h"
#include "TalkingObjectWidget.h"
#include "ProjectUmeowmi/UI/PUScorecardWidget.h"
#include "TalkingObject.generated.h"

// Forward declarations
class UWidgetComponent;
class USphereComponent;
class UDlgDialogue;
class UDlgContext;
class UDataTable;
class UPUEmoteWidget;
struct FTimerHandle;

// Delegate for when a player enters the interaction sphere
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerEnteredInteractionSphere, class ATalkingObject*, TalkingObject);
// Delegate for when a player exits the interaction sphere
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerExitedInteractionSphere, class ATalkingObject*, TalkingObject);

UENUM(BlueprintType)
enum class ETalkingObjectType : uint8
{
    NPC UMETA(DisplayName = "NPC"),
    Prop UMETA(DisplayName = "Prop"),
    System UMETA(DisplayName = "System"),
    Door UMETA(DisplayName = "Door")
};

/**
 * Base class for objects that can participate in dialogues
 */
UCLASS()
class PROJECTUMEOWMI_API ATalkingObject : public AActor, public IDlgDialogueParticipant
{
    GENERATED_BODY()

public:
    ATalkingObject();

    virtual void BeginPlay() override;
    virtual void PostInitializeComponents() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
    
    // We'll keep Tick for now but mark it as virtual so we can override it in derived classes if needed
    virtual void Tick(float DeltaTime) override;

    // IDlgDialogueParticipant Interface
    FName GetParticipantName_Implementation() const override { return ParticipantName; }
    FText GetParticipantDisplayName_Implementation(FName ActiveSpeaker) const override { return DisplayName; }
    UTexture2D* GetParticipantIcon_Implementation(FName ActiveSpeaker, FName ActiveSpeakerState) const override { return ParticipantIcon; }
    virtual bool CheckCondition_Implementation(const UDlgContext* Context, FName ConditionName) const;
    virtual float GetFloatValue_Implementation(FName ValueName) const;
    virtual int32 GetIntValue_Implementation(FName ValueName) const;
    virtual bool GetBoolValue_Implementation(FName ValueName) const;
    virtual FName GetNameValue_Implementation(FName ValueName) const;
    virtual bool OnDialogueEvent_Implementation(UDlgContext* Context, FName EventName);

    // Interaction methods
    virtual bool CanInteract() const;
    virtual void StartInteraction();
    virtual void EndInteraction();

    // Dialogue methods
    void StartRandomDialogue();
    void StartSpecificDialogue(UDlgDialogue* Dialogue);

    /** Start a specific dialogue and set interacting state. Use when triggering dialogue outside of StartRandomDialogue (e.g. locked level transition). */
    void StartDialogueAndSetInteracting(UDlgDialogue* Dialogue);

    // Collision events
    UFUNCTION()
    void OnInteractionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
    
    UFUNCTION()
    void OnInteractionSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    // Debug methods
    UFUNCTION(BlueprintCallable, Category = "Talking Object|Debug")
    void ToggleDebugVisualization();

    // Delegate events
    UPROPERTY(BlueprintAssignable, Category = "Talking Object|Events")
    FOnPlayerEnteredInteractionSphere OnPlayerEnteredInteractionSphere;

    UPROPERTY(BlueprintAssignable, Category = "Talking Object|Events")
    FOnPlayerExitedInteractionSphere OnPlayerExitedInteractionSphere;

    // Getters for talking object information
    UFUNCTION(BlueprintCallable, Category = "Talking Object|Info")
    FName GetInteractionKeyName() const { return InteractionKey; }

    UFUNCTION(BlueprintCallable, Category = "Talking Object|Info")
    FText GetTalkingObjectDisplayName() const { return DisplayName; }

    UFUNCTION(BlueprintCallable, Category = "Talking Object|Info")
    FName GetTalkingObjectName() const { return ParticipantName; }
    
    UFUNCTION(BlueprintCallable, Category = "Talking Object|Info")
    ETalkingObjectType GetTalkingObjectType() const { return ObjectType; }

    /** Set the interaction widget class at runtime. Refreshes the widget immediately if the player is in range. */
    UFUNCTION(BlueprintCallable, Category = "Talking Object|Config")
    void SetInteractionWidgetClass(TSubclassOf<UTalkingObjectWidget> NewWidgetClass);

    /** Set the interaction key text at runtime (e.g. "Interact", "Talk", "E"). Refreshes the widget immediately if the player is in range. */
    UFUNCTION(BlueprintCallable, Category = "Talking Object|Config")
    void SetInteractionKey(FName NewKey);

    /** Set the interaction icon at runtime. Pass nullptr to hide the icon. Refreshes the widget immediately if the player is in range. */
    UFUNCTION(BlueprintCallable, Category = "Talking Object|Config")
    void SetInteractionIcon(UTexture2D* NewIcon);

    /** Call from player Tick to drive NPC face-player lerp. Only does work when NPC is lerping. */
    void TickFacePlayerLerp(float DeltaTime);

    /** Called when a level transition starts. Hides the interaction widget so it doesn't show during fade/load. */
    UFUNCTION(BlueprintCallable, Category = "Talking Object|Level Transition")
    void HideInteractionWidgetForTransition();

    // Emote API
    /** Show an emote above this talking object, using EmoteDataTable to resolve the tag into an icon and optional extras. */
    UFUNCTION(BlueprintCallable, Category = "Talking Object|Emote")
    void ShowEmoteByTag(FGameplayTag EmoteTag);

    /** Clear any active emote immediately. */
    UFUNCTION(BlueprintCallable, Category = "Talking Object|Emote")
    void ClearEmote();

    /** Returns true if an emote is currently visible. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Talking Object|Emote")
    bool IsEmoteActive() const;

    /** Refresh the interaction widget (visibility, key, icon). Call when selection changes among overlapping targets. */
    UFUNCTION(BlueprintCallable, Category = "Talking Object|Config")
    void RefreshInteractionWidget();

protected:
    /** Called when emote duration expires; plays fade-out then clears after animation. */
    void BeginFadeOutEmote();
    // Configurable properties
    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    ETalkingObjectType ObjectType;

    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    float InteractionRange = 200.0f;

    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    FName InteractionKey = FName(TEXT("Interact"));

    /** Optional icon displayed in the interaction widget. Can be set at runtime via SetInteractionIcon. */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    UTexture2D* InteractionIcon = nullptr;

    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    TArray<UDlgDialogue*> AvailableDialogues;

    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    TSubclassOf<UTalkingObjectWidget> InteractionWidgetClass;

    /** Space in which the interaction widget is rendered. World space scales with orthographic zoom when bScaleWidgetWithOrthoZoom is true. */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    EWidgetSpace InteractionWidgetSpace = EWidgetSpace::Screen;

    /** When true and using an orthographic camera, scale the interaction widget with zoom (OrthoWidth). */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    bool bScaleWidgetWithOrthoZoom = false;

    /** Reference orthographic width at which the widget displays at its base size. Used when bScaleWidgetWithOrthoZoom is true. */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Config", meta = (EditCondition = "bScaleWidgetWithOrthoZoom"))
    float ReferenceOrthoWidth = 1000.0f;

    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    FName ParticipantName;

    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    FText DisplayName;

    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    UTexture2D* ParticipantIcon;

    // Names of participants to include in the dialogue
    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    TArray<FName> AllowedParticipantNames;

    // Visual components
    UPROPERTY(VisibleAnywhere, Category = "Talking Object|Components")
    UWidgetComponent* InteractionWidget;

    /** Optional emote widget rendered above this talking object (e.g. cat-face icons). */
    UPROPERTY(VisibleAnywhere, Category = "Talking Object|Components")
    UWidgetComponent* EmoteWidget;

    // Collision component
    UPROPERTY(VisibleAnywhere, Category = "Talking Object|Components")
    USphereComponent* InteractionSphere;

    // Debug visualization
    UPROPERTY(EditAnywhere, Category = "Talking Object|Debug")
    bool bShowDebugRange = false;

    /** When true, logs the active participants for this interactable (filtered by AllowedParticipantNames) when entering the sphere and when dialogue starts. */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Debug")
    bool bShowDebugParticipants = false;

    /** When true, logs NPC face-player lerp start, tick progress, and completion. Set true on NPC to debug. */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Debug")
    bool bShowDebugFacePlayerLerp = true;

    /** When true, logs emote show/clear and lookup steps. Set true on TalkingObject to debug emotes. */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Debug")
    bool bShowDebugEmotes = false;

    /** For Door type: whether the door is currently open. Toggled on each interaction. */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Config", meta = (EditCondition = "ObjectType == ETalkingObjectType::Door"))
    bool bIsDoorOpen = false;

    /** For Door type: optional dialogue shown when the door is locked. Uses ParticipantName as the lock ID. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talking Object|Config", meta = (EditCondition = "ObjectType == ETalkingObjectType::Door"))
    UDlgDialogue* LockedDoorDialogue = nullptr;

    /** For NPC type: Master toggle to rotate NPC to face the player when dialogue starts. */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Config", meta = (EditCondition = "ObjectType == ETalkingObjectType::NPC"))
    bool bRotateNPCToFacePlayer = true;

    /** For NPC type: Rotate around Yaw (horizontal) to face the player. Disable to preserve current yaw. */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Config", meta = (EditCondition = "ObjectType == ETalkingObjectType::NPC && bRotateNPCToFacePlayer"))
    bool bRotateNPCAroundYaw = true;

    /** For NPC type: Apply pitch rotation (0 = upright). Disable to preserve current pitch (e.g. lying down). */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Config", meta = (EditCondition = "ObjectType == ETalkingObjectType::NPC && bRotateNPCToFacePlayer"))
    bool bRotateNPCAroundPitch = false;

    /** For NPC type: Apply roll rotation (0 = upright). Disable to preserve current roll (e.g. lying down). */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Config", meta = (EditCondition = "ObjectType == ETalkingObjectType::NPC && bRotateNPCToFacePlayer"))
    bool bRotateNPCAroundRoll = false;

    /** For NPC type: Yaw offset (degrees) when facing the player. Use if mesh forward differs from Unreal's +X (e.g. -90 if mesh faces +Y). */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Config", meta = (EditCondition = "ObjectType == ETalkingObjectType::NPC"))
    float NPCFacingYawOffset = -90.0f;

    /** For NPC type: Master toggle to rotate player to face the NPC when dialogue starts. */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Config", meta = (EditCondition = "ObjectType == ETalkingObjectType::NPC"))
    bool bRotatePlayerToFaceNPC = true;

    /** For NPC type: Rotate player around Yaw to face the NPC. Disable to preserve current yaw. */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Config", meta = (EditCondition = "ObjectType == ETalkingObjectType::NPC && bRotatePlayerToFaceNPC"))
    bool bRotatePlayerAroundYaw = true;

    /** For NPC type: Apply pitch to player. Disable to preserve current pitch. */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Config", meta = (EditCondition = "ObjectType == ETalkingObjectType::NPC && bRotatePlayerToFaceNPC"))
    bool bRotatePlayerAroundPitch = false;

    /** For NPC type: Apply roll to player. Disable to preserve current roll. */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Config", meta = (EditCondition = "ObjectType == ETalkingObjectType::NPC && bRotatePlayerToFaceNPC"))
    bool bRotatePlayerAroundRoll = false;

    /** For NPC type: Yaw offset (degrees) for player when facing the NPC. Tweak if player mesh forward differs. */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Config", meta = (EditCondition = "ObjectType == ETalkingObjectType::NPC"))
    float PlayerFacingYawOffset = -90.0f;

    /** For NPC type: Rotation speed (degrees/sec) when lerping to face the player. Higher = faster. */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Config", meta = (EditCondition = "ObjectType == ETalkingObjectType::NPC"))
    float NPCFacingRotationSpeed = 360.0f;

    /** Widget class for ShowScorecard dialogue event. Assign WBP_Scorecard. Used when dialogue fires "ShowScorecard" event. */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Dialogue")
    TSubclassOf<UPUScorecardWidget> ScorecardWidgetClass;

    // Emote configuration

    /** Master toggle for showing emotes above this talking object. */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Emote")
    bool bEnableEmotes = true;

    /** Widget class used to render emotes above this talking object. */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Emote", meta = (EditCondition = "bEnableEmotes"))
    TSubclassOf<UPUEmoteWidget> EmoteWidgetClass;

    /** Space in which the emote widget is rendered (Screen or World). Default: World (overhead bubble). */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Emote", meta = (EditCondition = "bEnableEmotes"))
    EWidgetSpace EmoteWidgetSpace = EWidgetSpace::World;

    /** Size of the emote widget in pixels (width x height). Affects both screen and world space. */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Emote", meta = (EditCondition = "bEnableEmotes", ClampMin = "16", ClampMax = "512"))
    FVector2D EmoteDrawSize = FVector2D(128.0f, 128.0f);

    /** Scale multiplier for the emote widget (applied to the component). */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Emote", meta = (EditCondition = "bEnableEmotes", ClampMin = "0.25", ClampMax = "4.0"))
    float EmoteScale = 1.0f;

    /** Data table mapping gameplay tags to emote data (icon, duration, etc.). */
    UPROPERTY(EditAnywhere, Category = "Talking Object|Emote", meta = (EditCondition = "bEnableEmotes"))
    UDataTable* EmoteDataTable = nullptr;

    // Dialogue context
    UPROPERTY(BlueprintReadWrite, Category = Dialogue)
    UDlgContext* CurrentDialogueContext = nullptr;
    
protected:
    // Protected helper for derived classes to check if player is in range
    bool IsPlayerInRange() const;

private:
    // Internal state
    bool bIsInteracting = false;

    // Add UPROPERTY() to prevent garbage collection issues
    UPROPERTY()
    TSet<UDlgDialogue*> UsedDialogues;
    
    bool bPlayerInRange = false;

    /** Cached base DrawSize for ortho scaling. Stored when widget is first shown. */
    FVector2D CachedBaseDrawSize = FVector2D(500.0f, 500.0f);

    bool bIsLerpingToFacePlayer = false;
    bool bIsLerpingBackToOriginal = false;
    FRotator TargetNPCRotation;
    FRotator OriginalNPCRotation;
    int32 FacePlayerLerpTickCount = 0;

    bool bIsLerpingPlayerToFaceNPC = false;
    FRotator TargetPlayerRotation;

    // Emote state

    /** Currently active emote tag (if any). */
    UPROPERTY()
    FGameplayTag ActiveEmoteTag;

    /** Timer used to auto-hide emotes after their duration (triggers fade-out + clear). */
    FTimerHandle EmoteHideTimerHandle;

    /** Timer used to clear emote after fade-out finishes. */
    FTimerHandle EmoteFadeOutTimerHandle;

    // Helper methods (internal)
    void UpdateInteractionWidget();
    /** Syncs the interaction sphere radius and widget attachment to match InteractionRange. Call when InteractionRange may have changed. */
    void SyncInteractionSphereToRange();
    /** Updates the interaction widget's scale/DrawSize based on orthographic camera zoom when bScaleWidgetWithOrthoZoom is true. */
    void UpdateOrthoWidgetScale();
    UDlgDialogue* GetRandomDialogue() const;
    void ResetUsedDialogues();
    void DrawDebugRange() const;
    /** Builds the filtered participant list for this interactable (same logic used when starting dialogue). */
    TArray<UObject*> BuildActiveParticipantsList() const;

    /** For Door type: checks the global lock state using ParticipantName as the lock ID. */
    bool IsDoorUnlocked() const;
}; 