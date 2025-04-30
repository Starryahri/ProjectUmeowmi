#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DlgSystem/DlgDialogueParticipant.h"
#include "TalkingObjectWidget.h"
#include "TalkingObject.generated.h"

// Forward declarations
class UWidgetComponent;
class USphereComponent;
class UDlgDialogue;
class UDlgContext;

// Delegate for when a player enters the interaction sphere
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerEnteredInteractionSphere, class ATalkingObject*, TalkingObject);
// Delegate for when a player exits the interaction sphere
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerExitedInteractionSphere, class ATalkingObject*, TalkingObject);

UENUM(BlueprintType)
enum class ETalkingObjectType : uint8
{
    NPC UMETA(DisplayName = "NPC"),
    Prop UMETA(DisplayName = "Prop"),
    System UMETA(DisplayName = "System")
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
    
    // We'll keep Tick for now but mark it as virtual so we can override it in derived classes if needed
    virtual void Tick(float DeltaTime) override;

    // IDlgDialogueParticipant Interface
    FName GetParticipantName_Implementation() const override { return ParticipantName; }
    FText GetParticipantDisplayName_Implementation(FName ActiveSpeaker) const override { return DisplayName; }
    UTexture2D* GetParticipantIcon_Implementation(FName ActiveSpeaker, FName ActiveSpeakerState) const override { return ParticipantIcon; }
    virtual bool CheckCondition(const UDlgContext* Context, FName ConditionName) const;
    virtual float GetFloatValue(FName ValueName) const;
    virtual int32 GetIntValue(FName ValueName) const;
    virtual bool GetBoolValue(FName ValueName) const;
    virtual FName GetNameValue(FName ValueName) const;
    virtual bool OnDialogueEvent(UDlgContext* Context, FName EventName);

    // Interaction methods
    bool CanInteract() const;
    void StartInteraction();
    void EndInteraction();

    // Dialogue methods
    void StartRandomDialogue();
    void StartSpecificDialogue(UDlgDialogue* Dialogue);

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

protected:
    // Configurable properties
    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    ETalkingObjectType ObjectType;

    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    float InteractionRange = 200.0f;

    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    FName InteractionKey = FName(TEXT("Interact"));

    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    TArray<UDlgDialogue*> AvailableDialogues;

    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    TSubclassOf<UTalkingObjectWidget> InteractionWidgetClass;

    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    FName ParticipantName;

    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    FText DisplayName;

    UPROPERTY(EditAnywhere, Category = "Talking Object|Config")
    UTexture2D* ParticipantIcon;

    // Visual components
    UPROPERTY(VisibleAnywhere, Category = "Talking Object|Components")
    UWidgetComponent* InteractionWidget;

    // Collision component
    UPROPERTY(VisibleAnywhere, Category = "Talking Object|Components")
    USphereComponent* InteractionSphere;

    // Debug visualization
    UPROPERTY(EditAnywhere, Category = "Talking Object|Debug")
    bool bShowDebugRange = false;

private:
    // Internal state
    bool bIsInteracting = false;
    UDlgContext* CurrentDialogueContext = nullptr;
    TSet<UDlgDialogue*> UsedDialogues;
    bool bPlayerInRange = false;

    // Helper methods
    void UpdateInteractionWidget();
    bool IsPlayerInRange() const;
    UDlgDialogue* GetRandomDialogue() const;
    void ResetUsedDialogues();
    void DrawDebugRange() const;
}; 