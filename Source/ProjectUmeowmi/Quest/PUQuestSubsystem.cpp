#include "ProjectUmeowmi/Quest/PUQuestSubsystem.h"
#include "ProjectUmeowmi/PUPlayerSaveGame.h"
#include "ProjectUmeowmi/PUProjectUmeowmiGameInstance.h"
#include "ProjectUmeowmi/Quest/PUQuestObjectiveContentRow.h"
#include "GameplayTagsManager.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"

namespace
{
const FPUQuestObjectiveContentRow* FindQuestObjectiveRow(const UDataTable* Table, const FGameplayTag& Tag)
{
	if (!Table || !Tag.IsValid())
	{
		return nullptr;
	}
	const FName RowName(*Tag.ToString());
	if (const FPUQuestObjectiveContentRow* Row = Table->FindRow<FPUQuestObjectiveContentRow>(RowName, TEXT("FindQuestObjectiveRow"), false))
	{
		return Row;
	}
	for (const FName& RowKey : Table->GetRowNames())
	{
		if (const FPUQuestObjectiveContentRow* Row = Table->FindRow<FPUQuestObjectiveContentRow>(RowKey, TEXT("FindQuestObjectiveRow"), false))
		{
			if (Row->ObjectiveTag == Tag)
			{
				return Row;
			}
		}
	}
	return nullptr;
}
}

void UPUQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	EnsureQuestRootTagDefault();
}

void UPUQuestSubsystem::EnsureQuestRootTagDefault()
{
	if (!QuestRootTag.IsValid())
	{
		QuestRootTag = UGameplayTagsManager::Get().RequestGameplayTag(FName(TEXT("Quest")), false);
	}
}

void UPUQuestSubsystem::ExportToSave(UPUPlayerSaveGame* Save) const
{
	if (!Save)
	{
		return;
	}
	Save->CompletedQuestObjectiveTags = CompletedQuestObjectiveTags;
	Save->CompletedQuestTags = CompletedQuestTags;
	Save->ActiveQuestTag = ActiveQuestTag;
	Save->ActiveObjectiveTag = ActiveObjectiveTag;
	Save->ObjectiveProgressCounters = ObjectiveProgressCounters;
}

void UPUQuestSubsystem::ImportFromSave(const UPUPlayerSaveGame* Save)
{
	if (!Save)
	{
		return;
	}
	CompletedQuestObjectiveTags = Save->CompletedQuestObjectiveTags;
	CompletedQuestTags = Save->CompletedQuestTags;
	ActiveQuestTag = Save->ActiveQuestTag;
	ActiveObjectiveTag = Save->ActiveObjectiveTag;
	ObjectiveProgressCounters = Save->ObjectiveProgressCounters;
}

bool UPUQuestSubsystem::MigrateQuestSaveIfNeeded(UPUPlayerSaveGame* Save)
{
	if (!Save || Save->SaveVersion >= 2)
	{
		return false;
	}
	Save->SaveVersion = 2;
	return true;
}

void UPUQuestSubsystem::ResetQuestStateForNewGame()
{
	CompletedQuestObjectiveTags.Empty();
	CompletedQuestTags.Empty();
	ActiveQuestTag = FGameplayTag();
	ActiveObjectiveTag = FGameplayTag();
	ObjectiveProgressCounters.Empty();
}

void UPUQuestSubsystem::BroadcastQuestObjectiveChanged()
{
	OnQuestObjectiveChanged.Broadcast(ActiveQuestTag, ActiveObjectiveTag);
}

void UPUQuestSubsystem::RequestSave(bool bSave)
{
	if (!bSave)
	{
		return;
	}
	if (UPUProjectUmeowmiGameInstance* GI = Cast<UPUProjectUmeowmiGameInstance>(GetGameInstance()))
	{
		GI->SaveGame();
	}
}

bool UPUQuestSubsystem::StartQuest(const FGameplayTag& QuestTag, const FGameplayTag& FirstObjectiveTag, bool bSave)
{
	if (!QuestTag.IsValid())
	{
		return false;
	}
	ActiveQuestTag = QuestTag;
	ActiveObjectiveTag = FirstObjectiveTag;
	BroadcastQuestObjectiveChanged();
	RequestSave(bSave);
	return true;
}

void UPUQuestSubsystem::SetActiveObjective(const FGameplayTag& ObjectiveTag, bool bSave)
{
	ActiveObjectiveTag = ObjectiveTag;
	BroadcastQuestObjectiveChanged();
	RequestSave(bSave);
}

bool UPUQuestSubsystem::CompleteObjective(const FGameplayTag& ObjectiveTag, bool bSave)
{
	if (!ObjectiveTag.IsValid())
	{
		return false;
	}
	CompletedQuestObjectiveTags.Add(ObjectiveTag);
	if (ActiveObjectiveTag == ObjectiveTag)
	{
		ActiveObjectiveTag = FGameplayTag();
	}
	BroadcastQuestObjectiveChanged();
	RequestSave(bSave);
	return true;
}

bool UPUQuestSubsystem::CompleteQuest(const FGameplayTag& QuestTag, bool bSave)
{
	if (!QuestTag.IsValid())
	{
		return false;
	}
	CompletedQuestTags.Add(QuestTag);
	if (ActiveQuestTag == QuestTag)
	{
		ActiveQuestTag = FGameplayTag();
		ActiveObjectiveTag = FGameplayTag();
	}
	BroadcastQuestObjectiveChanged();
	RequestSave(bSave);
	return true;
}

bool UPUQuestSubsystem::IsObjectiveCompleted(const FGameplayTag& ObjectiveTag) const
{
	return ObjectiveTag.IsValid() && CompletedQuestObjectiveTags.Contains(ObjectiveTag);
}

bool UPUQuestSubsystem::IsQuestCompleted(const FGameplayTag& QuestTag) const
{
	return QuestTag.IsValid() && CompletedQuestTags.Contains(QuestTag);
}

bool UPUQuestSubsystem::IsObjectiveActive(const FGameplayTag& ObjectiveTag) const
{
	return ObjectiveTag.IsValid() && ActiveObjectiveTag == ObjectiveTag;
}

bool UPUQuestSubsystem::IsQuestActive(const FGameplayTag& QuestTag) const
{
	return QuestTag.IsValid() && ActiveQuestTag == QuestTag;
}

void UPUQuestSubsystem::AddObjectiveProgress(const FGameplayTag& ObjectiveTag, int32 Delta, bool bSave)
{
	if (!ObjectiveTag.IsValid() || Delta == 0)
	{
		return;
	}
	const int32 NewVal = ObjectiveProgressCounters.FindRef(ObjectiveTag) + Delta;
	ObjectiveProgressCounters.Add(ObjectiveTag, NewVal);
	RequestSave(bSave);
}

int32 UPUQuestSubsystem::GetObjectiveProgress(const FGameplayTag& ObjectiveTag) const
{
	if (!ObjectiveTag.IsValid())
	{
		return 0;
	}
	return ObjectiveProgressCounters.FindRef(ObjectiveTag);
}

void UPUQuestSubsystem::ClearActiveQuestState(bool bSave)
{
	ActiveQuestTag = FGameplayTag();
	ActiveObjectiveTag = FGameplayTag();
	BroadcastQuestObjectiveChanged();
	RequestSave(bSave);
}

bool UPUQuestSubsystem::GetObjectiveDisplayInfo(const FGameplayTag& ObjectiveTag, FPUQuestObjectiveDisplayInfo& OutInfo) const
{
	OutInfo = FPUQuestObjectiveDisplayInfo();
	const UDataTable* Table = nullptr;
	if (const UPUProjectUmeowmiGameInstance* GI = Cast<UPUProjectUmeowmiGameInstance>(GetGameInstance()))
	{
		Table = GI->QuestObjectiveContentTable;
	}
	if (!Table)
	{
		return false;
	}
	const FPUQuestObjectiveContentRow* Row = FindQuestObjectiveRow(Table, ObjectiveTag);
	if (!Row)
	{
		return false;
	}
	OutInfo.bFound = true;
	OutInfo.ObjectiveTag = Row->ObjectiveTag.IsValid() ? Row->ObjectiveTag : ObjectiveTag;
	OutInfo.QuestTag = Row->QuestTag;
	OutInfo.QuestTitle = Row->QuestTitle;
	OutInfo.ObjectiveTitle = Row->ObjectiveTitle;
	OutInfo.ObjectiveDescription = Row->ObjectiveDescription;
	OutInfo.Icon = Row->Icon.IsValid() ? Row->Icon.LoadSynchronous() : nullptr;
	return true;
}

bool UPUQuestSubsystem::GetActiveObjectiveDisplayInfo(FPUQuestObjectiveDisplayInfo& OutInfo) const
{
	return GetObjectiveDisplayInfo(GetActiveObjectiveTag(), OutInfo);
}

bool UPUQuestSubsystem::GetQuestDisplayInfo(const FGameplayTag& QuestTag, FText& OutQuestTitle, FText& OutFirstObjectiveTitle) const
{
	OutQuestTitle = FText::GetEmpty();
	OutFirstObjectiveTitle = FText::GetEmpty();
	const UDataTable* Table = nullptr;
	if (const UPUProjectUmeowmiGameInstance* GI = Cast<UPUProjectUmeowmiGameInstance>(GetGameInstance()))
	{
		Table = GI->QuestObjectiveContentTable;
	}
	if (!Table || !QuestTag.IsValid())
	{
		return false;
	}
	for (const FName& RowKey : Table->GetRowNames())
	{
		if (const FPUQuestObjectiveContentRow* Row = Table->FindRow<FPUQuestObjectiveContentRow>(RowKey, TEXT("GetQuestDisplayInfo"), false))
		{
			if (Row->QuestTag == QuestTag)
			{
				OutQuestTitle = Row->QuestTitle;
				OutFirstObjectiveTitle = Row->ObjectiveTitle;
				return true;
			}
		}
	}
	return false;
}
