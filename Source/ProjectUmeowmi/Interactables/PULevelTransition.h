#pragma once

#include "CoreMinimal.h"
#include "../Dialogue/TalkingObject.h"
#include "PULevelTransition.generated.h"

UCLASS()
class PROJECTUMEOWMI_API APULevelTransition : public ATalkingObject
{
	GENERATED_BODY()

public:
	APULevelTransition();

	virtual void BeginPlay() override;

	// Override interaction to trigger level transition
	virtual void StartInteraction() override;
	
	// Override CanInteract so we can use the talking-object style widget without requiring dialogues
	virtual bool CanInteract() const override;

	// Override dialogue event to support level transitions from dialogue
	virtual bool OnDialogueEvent_Implementation(UDlgContext* Context, FName EventName) override;

protected:
	// The target level to transition to (e.g., "L_Chapter0_2_LolaRoom")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Transition")
	FString TargetLevelName;

	// The spawn point tag/ID in the target level
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Transition")
	FName TargetSpawnPointTag;

	// Whether to use fade transition
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Transition")
	bool bUseFadeTransition = true;

	// Whether to auto-trigger on overlap (if false, requires interaction)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Transition")
	bool bAutoTrigger = false;

	// Optional: additional overlap handler for auto-trigger behavior
	UFUNCTION()
	void OnTransitionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	// Helper to perform the actual transition
	void PerformTransition();
};

