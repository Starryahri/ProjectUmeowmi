#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DlgSystem/DlgDialogueParticipant.h"
#include "TalkingObjectWidget.h"
#include "TalkingObject.generated.h"

class UWidgetComponent;
class UDlgDialogue;
class UDlgContext;

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
    virtual void Tick(float DeltaTime) override;

    // IDlgDialogueParticipant Interface
    virtual FName GetParticipantName() const;
    virtual FText GetParticipantDisplayName(FName ActiveSpeaker) const;
    virtual UTexture2D* GetParticipantIcon(FName ActiveSpeaker, FName ActiveSpeakerState) const;
    virtual bool CheckCondition(const UDlgContext* Context, FName ConditionName) const;
    virtual float GetFloatValue(FName ValueName) const;
    virtual int32 GetIntValue(FName ValueName) const;
    virtual bool GetBoolValue(FName ValueName) const;
    virtual FName GetNameValue(FName ValueName) const;
    virtual bool OnDialogueEvent(UDlgContext* Context, FName EventName);
    virtual bool ModifyFloatValue(FName ValueName, bool bDelta, float Value);
    virtual bool ModifyIntValue(FName ValueName, bool bDelta, int32 Value);
    virtual bool ModifyBoolValue(FName ValueName, bool bNewValue);
    virtual bool ModifyNameValue(FName ValueName, FName NameValue);

    // Interaction methods
    bool CanInteract() const;
    void StartInteraction();
    void EndInteraction();

    // Dialogue methods
    void StartRandomDialogue();
    void StartSpecificDialogue(UDlgDialogue* Dialogue);

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

    // Debug visualization
    UPROPERTY(EditAnywhere, Category = "Talking Object|Debug")
    bool bShowDebugRange = false;

private:
    // Internal state
    bool bIsInteracting = false;
    UDlgContext* CurrentDialogueContext = nullptr;
    TSet<UDlgDialogue*> UsedDialogues;

    // Helper methods
    void UpdateInteractionWidget();
    bool IsPlayerInRange() const;
    UDlgDialogue* GetRandomDialogue() const;
    void ResetUsedDialogues();
    void DrawDebugRange() const;
}; 