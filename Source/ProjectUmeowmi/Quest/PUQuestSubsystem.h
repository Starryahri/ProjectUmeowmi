#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "ProjectUmeowmi/Quest/PUQuestObjectiveContentRow.h"
#include "PUQuestSubsystem.generated.h"

class UPUPlayerSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnQuestObjectiveChanged, FGameplayTag, QuestTag, FGameplayTag, ObjectiveTag);

/**
 * Quest / objective state and events. Persisted via UPUProjectUmeowmiGameInstance SaveGame/LoadGame.
 */
UCLASS()
class PROJECTUMEOWMI_API UPUQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Root tag for quest content (dialogue events / conditions use tags under this hierarchy). Default: Quest. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest", meta = (Categories = "Quest"))
	FGameplayTag QuestRootTag;

	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool StartQuest(const FGameplayTag& QuestTag, const FGameplayTag& FirstObjectiveTag, bool bSave = true);

	UFUNCTION(BlueprintCallable, Category = "Quest")
	void SetActiveObjective(const FGameplayTag& ObjectiveTag, bool bSave = true);

	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool CompleteObjective(const FGameplayTag& ObjectiveTag, bool bSave = true);

	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool CompleteQuest(const FGameplayTag& QuestTag, bool bSave = true);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	bool IsObjectiveCompleted(const FGameplayTag& ObjectiveTag) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	bool IsQuestCompleted(const FGameplayTag& QuestTag) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	bool IsObjectiveActive(const FGameplayTag& ObjectiveTag) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	bool IsQuestActive(const FGameplayTag& QuestTag) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	FGameplayTag GetActiveQuestTag() const { return ActiveQuestTag; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	FGameplayTag GetActiveObjectiveTag() const { return ActiveObjectiveTag; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	TSet<FGameplayTag> GetCompletedObjectiveTags() const { return CompletedQuestObjectiveTags; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	TSet<FGameplayTag> GetCompletedQuestTags() const { return CompletedQuestTags; }

	UFUNCTION(BlueprintCallable, Category = "Quest")
	void AddObjectiveProgress(const FGameplayTag& ObjectiveTag, int32 Delta, bool bSave = true);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	int32 GetObjectiveProgress(const FGameplayTag& ObjectiveTag) const;

	UFUNCTION(BlueprintCallable, Category = "Quest")
	void ClearActiveQuestState(bool bSave = true);

	// --- Authoring (Data Table) ---
	/**
	 * Fill OutInfo from the quest objective content table (set on Game Instance).
	 * Row lookup: first by row name == ObjectiveTag.ToString(), then by row's ObjectiveTag field.
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest|Content")
	bool GetObjectiveDisplayInfo(const FGameplayTag& ObjectiveTag, FPUQuestObjectiveDisplayInfo& OutInfo) const;

	UFUNCTION(BlueprintCallable, Category = "Quest|Content")
	bool GetActiveObjectiveDisplayInfo(FPUQuestObjectiveDisplayInfo& OutInfo) const;

	/** First row whose QuestTag matches (useful for journal header when you only have a quest tag). */
	UFUNCTION(BlueprintCallable, Category = "Quest|Content")
	bool GetQuestDisplayInfo(const FGameplayTag& QuestTag, FText& OutQuestTitle, FText& OutFirstObjectiveTitle) const;

	/** Copy quest fields into save object before writing disk (called by Game Instance). */
	void ExportToSave(UPUPlayerSaveGame* Save) const;

	/** Restore quest fields from loaded save (called by Game Instance). */
	void ImportFromSave(const UPUPlayerSaveGame* Save);

	/** If save predates quest data, bump version; caller should SaveGame when this returns true. */
	bool MigrateQuestSaveIfNeeded(UPUPlayerSaveGame* Save);

	/** Clears quest state when starting a new game (called by Game Instance). */
	void ResetQuestStateForNewGame();

	void EnsureQuestRootTagDefault();

	void BroadcastQuestObjectiveChanged();

	UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
	FOnQuestObjectiveChanged OnQuestObjectiveChanged;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	TSet<FGameplayTag> CompletedQuestObjectiveTags;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	TSet<FGameplayTag> CompletedQuestTags;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	FGameplayTag ActiveQuestTag;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	FGameplayTag ActiveObjectiveTag;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	TMap<FGameplayTag, int32> ObjectiveProgressCounters;

	void RequestSave(bool bSave);
};
