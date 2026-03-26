# Quest System — Developer Documentation

**Module:** `ProjectUmeowmi`  
**Primary source:** `Source/ProjectUmeowmi/Quest/`

This document describes the gameplay-tag-driven quest and objective system: runtime state in `UPUQuestSubsystem`, authoring via a Data Table (`FPUQuestObjectiveContentRow`), persistence through `UPUProjectUmeowmiGameInstance` and `UPUPlayerSaveGame`, and optional HUD helpers.

---

## System overview

| Piece | Role |
|-------|------|
| `UPUQuestSubsystem` | Single source of truth for active quest/objective, completed sets, progress counters, and content lookup. |
| `FPUQuestObjectiveContentRow` | Data Table row: titles, descriptions, soft icon per objective. |
| `FPUQuestObjectiveDisplayInfo` | Blueprint-friendly struct returned after resolving a row and loading the icon. |
| `UPUProjectUmeowmiGameInstance` | Caches `QuestObjectiveContentTable`, forwards quest API to the subsystem, runs save/load and new-game quest setup. |
| `UPUPlayerSaveGame` | Stores quest fields when `SaveVersion >= 2`. |
| `UPUQuestObjectiveOffscreenIndicatorWidget` | Edge-of-screen marker for the active objective target. |

**Tag conventions:** Use gameplay tags under your configured root (default `Quest`, see `QuestRootTag`). Objective rows are found by Data Table row name `== ObjectiveTag.ToString()` or by matching `ObjectiveTag` on the row.

---

## Class API: `UPUQuestSubsystem`

**Class Name:** `UPUQuestSubsystem`

**Description**  
Holds quest and objective state for the game instance: active quest/objective, completed objectives and quests, optional per-objective progress counters, and display info resolved from a quest objective content Data Table. State is exported/imported through the save game by `UPUProjectUmeowmiGameInstance`. When `bSave` is true on mutating calls, the game instance’s `SaveGame()` is invoked (if the game instance is `UPUProjectUmeowmiGameInstance`).

**Namespace**  
N/A (C++). **Feature area:** Quest (`Source/ProjectUmeowmi/Quest/`).

**Inheritance**  
**Parent class:** `UGameInstanceSubsystem`  
**Interfaces implemented:** None.

**Constructors**  
Subsystems are created by the engine; there is no custom public constructor. Use `UGameInstance::GetSubsystem<UPUQuestSubsystem>()` (or `UPUProjectUmeowmiGameInstance::GetQuestSubsystem()`).

**Properties**

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `QuestRootTag` | `FGameplayTag` | EditAnywhere, BlueprintReadWrite, Category `Quest` | Root tag for quest content; dialogue and conditions typically use tags under this hierarchy. Default applied in `EnsureQuestRootTagDefault()` if invalid: tag `Quest`. |
| `OnQuestObjectiveChanged` | `FOnQuestObjectiveChanged` | BlueprintAssignable, Category `Quest\|Events` | Multicast delegate fired when active quest/objective changes or after relevant mutations (see `BroadcastQuestObjectiveChanged`). |
| `CompletedQuestObjectiveTags` | `TSet<FGameplayTag>` | BlueprintReadOnly, protected | Objectives that have been marked complete at least once. |
| `CompletedQuestTags` | `TSet<FGameplayTag>` | BlueprintReadOnly, protected | Quests marked fully complete. |
| `ActiveQuestTag` | `FGameplayTag` | BlueprintReadOnly, protected | Currently tracked quest; invalid if none. |
| `ActiveObjectiveTag` | `FGameplayTag` | BlueprintReadOnly, protected | Current objective the player should pursue; invalid if none. |
| `ObjectiveProgressCounters` | `TMap<FGameplayTag, int32>` | BlueprintReadOnly, protected | Optional counters per objective (e.g. “3 of 5”). |
| `CachedQuestObjectiveContentTable` | `TObjectPtr<UDataTable>` | `UPROPERTY`, protected | Fallback table if the game instance is not `UPUProjectUmeowmiGameInstance` or its table is unset; can be set via `SetCachedQuestObjectiveContentTable`. |

**Methods**

| Method | Description | Parameters | Return |
|--------|-------------|------------|--------|
| `Initialize` | Subsystem init; calls `EnsureQuestRootTagDefault`. | `Collection` (`FSubsystemCollectionBase&`) | `void` |
| `StartQuest` | Sets active quest and first objective; broadcasts and optionally saves. | `QuestTag`, `FirstObjectiveTag` (`FGameplayTag`), `bSave` (`bool`, default true) | `bool` — false if `QuestTag` invalid |
| `SetActiveObjective` | Sets the active objective tag; broadcasts and optionally saves. | `ObjectiveTag` (`FGameplayTag`), `bSave` (`bool`) | `void` |
| `CompleteObjective` | Adds tag to completed objectives; clears active objective if it matches; broadcasts and optionally saves. | `ObjectiveTag` (`FGameplayTag`), `bSave` (`bool`) | `bool` — false if tag invalid |
| `CompleteQuest` | Adds quest to completed quests; clears active quest/objective if they match; broadcasts and optionally saves. | `QuestTag` (`FGameplayTag`), `bSave` (`bool`) | `bool` — false if tag invalid |
| `IsObjectiveCompleted` | Whether the objective tag is in the completed set. | `ObjectiveTag` (`FGameplayTag`) | `bool` |
| `IsQuestCompleted` | Whether the quest tag is in the completed quest set. | `QuestTag` (`FGameplayTag`) | `bool` |
| `IsObjectiveActive` | Whether the tag matches `ActiveObjectiveTag`. | `ObjectiveTag` (`FGameplayTag`) | `bool` |
| `IsQuestActive` | Whether the tag matches `ActiveQuestTag`. | `QuestTag` (`FGameplayTag`) | `bool` |
| `GetActiveQuestTag` | Current active quest tag. | — | `FGameplayTag` |
| `GetActiveObjectiveTag` | Current active objective tag. | — | `FGameplayTag` |
| `GetCompletedObjectiveTags` | Copy of completed objective tags. | — | `TSet<FGameplayTag>` |
| `GetCompletedQuestTags` | Copy of completed quest tags. | — | `TSet<FGameplayTag>` |
| `AddObjectiveProgress` | Adds `Delta` to the counter for `ObjectiveTag`; no-op if tag invalid or delta 0; optionally saves. | `ObjectiveTag` (`FGameplayTag`), `Delta` (`int32`), `bSave` (`bool`) | `void` |
| `GetObjectiveProgress` | Counter value for the objective (0 if invalid/missing). | `ObjectiveTag` (`FGameplayTag`) | `int32` |
| `ClearActiveQuestState` | Clears active quest and objective tags; broadcasts and optionally saves. | `bSave` (`bool`) | `void` |
| `GetObjectiveDisplayInfo` | Fills `OutInfo` from the resolved content table; loads icon synchronously when possible. | `ObjectiveTag`, `OutInfo` (`FPUQuestObjectiveDisplayInfo&`) | `bool` — whether a row was found |
| `GetActiveObjectiveDisplayInfo` | Same as `GetObjectiveDisplayInfo` for the current active objective. | `OutInfo` (`FPUQuestObjectiveDisplayInfo&`) | `bool` |
| `GetQuestDisplayInfo` | Scans table for first row whose `QuestTag` matches; fills quest title and that row’s objective title. | `QuestTag`, `OutQuestTitle`, `OutFirstObjectiveTitle` (`FText&`) | `bool` |
| `ExportToSave` | Copies subsystem state into `UPUPlayerSaveGame` (used by game instance before writing disk). | `Save` (`UPUPlayerSaveGame*`) | `void` |
| `ImportFromSave` | Restores state from save object. | `Save` (`const UPUPlayerSaveGame*`) | `void` |
| `MigrateQuestSaveIfNeeded` | If `SaveVersion < 2`, sets version to 2; caller should persist if true. | `Save` (`UPUPlayerSaveGame*`) | `bool` |
| `ResetQuestStateForNewGame` | Clears completed sets, active tags, and progress map. | — | `void` |
| `EnsureQuestRootTagDefault` | If `QuestRootTag` invalid, requests default `Quest` tag from gameplay tag manager. | — | `void` |
| `BroadcastQuestObjectiveChanged` | Broadcasts `OnQuestObjectiveChanged` with current active quest and objective tags. | — | `void` |
| `SetCachedQuestObjectiveContentTable` | Sets fallback content table for Blueprint-only or non-`UPUProjectUmeowmiGameInstance` setups. | `Table` (`UDataTable*`) | `void` |

**Events**

| Event Name | Description | Parameters |
|------------|-------------|------------|
| `OnQuestObjectiveChanged` (`FOnQuestObjectiveChanged`) | Fired when quest-related state updates and `BroadcastQuestObjectiveChanged` runs. | `QuestTag` (`FGameplayTag`), `ObjectiveTag` (`FGameplayTag`) |

**Usage example (C++)**

```cpp
#include "ProjectUmeowmi/Quest/PUQuestSubsystem.h"

void AMyActor::Foo(UGameInstance* GI)
{
    if (UPUQuestSubsystem* Quest = GI ? GI->GetSubsystem<UPUQuestSubsystem>() : nullptr)
    {
        Quest->StartQuest(MyQuestTag, MyFirstObjectiveTag, true);
        Quest->AddObjectiveProgress(MyFirstObjectiveTag, 1, true);
    }
}
```

**Dependencies**  
`UGameInstanceSubsystem`, `GameplayTagContainer`, `PUQuestObjectiveContentRow.h`, `Engine/DataTable`, `UPUProjectUmeowmiGameInstance` (for `SaveGame()` and default `QuestObjectiveContentTable` resolution), `UPUPlayerSaveGame`, `UTexture2D` (icon resolution).

**Notes**  
- `ResolveQuestObjectiveContentTable()` prefers `UPUProjectUmeowmiGameInstance::QuestObjectiveContentTable`, else `CachedQuestObjectiveContentTable`.  
- Row lookup: `FindRow` by row name `FName(*ObjectiveTag.ToString())`, then scan rows for `ObjectiveTag == Tag`.  
- `AddObjectiveProgress` does not broadcast `OnQuestObjectiveChanged` (only `RequestSave` when saving).  
- Save migration: quest data is tied to `SaveVersion` 2 on `UPUPlayerSaveGame`.

---

## Struct API: `FPUQuestObjectiveContentRow`

**Description**  
Authoring row for a Data Table: bind display strings and icon to gameplay tags. Prefer row names equal to the objective tag string (e.g. `Quest.Chapter1.TalkToNpc`).

**Parent:** `FTableRowBase`

**Properties**

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `ObjectiveTag` | `FGameplayTag` | EditAnywhere, BlueprintReadWrite | Must match the tag used in logic/dialogue for this objective. |
| `QuestTag` | `FGameplayTag` | EditAnywhere, BlueprintReadWrite | Parent quest tag for grouping or journal. |
| `QuestTitle` | `FText` | EditAnywhere, BlueprintReadWrite | Display name for the quest (may repeat on multiple rows of the same quest). |
| `ObjectiveTitle` | `FText` | EditAnywhere, BlueprintReadWrite | Short line for HUD tracker or prompts. |
| `ObjectiveDescription` | `FText` | EditAnywhere, BlueprintReadWrite | Longer copy for journal or detail UI. |
| `Icon` | `TSoftObjectPtr<UTexture2D>` | EditAnywhere, BlueprintReadWrite | Soft reference to objective icon; resolved at display time. |

**Change Log**  
See table at end of document.

---

## Struct API: `FPUQuestObjectiveDisplayInfo`

**Description**  
Runtime bundle returned to Blueprint/C++ after resolving a content row; icon is loaded into `TObjectPtr<UTexture2D>` when possible.

**Properties**

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `bFound` | `bool` | BlueprintReadOnly | Whether resolution succeeded. |
| `ObjectiveTag` | `FGameplayTag` | BlueprintReadOnly | Objective tag from row or the requested tag. |
| `QuestTag` | `FGameplayTag` | BlueprintReadOnly | Parent quest tag from row. |
| `QuestTitle` | `FText` | BlueprintReadOnly | From row. |
| `ObjectiveTitle` | `FText` | BlueprintReadOnly | From row. |
| `ObjectiveDescription` | `FText` | BlueprintReadOnly | From row. |
| `Icon` | `TObjectPtr<UTexture2D>` | BlueprintReadOnly | Loaded texture, or null if missing/failed. |

---

## Class API: `UPUQuestObjectiveOffscreenIndicatorWidget`

**Class Name:** `UPUQuestObjectiveOffscreenIndicatorWidget`

**Description**  
Full-screen overlay that draws the active quest objective marker at the viewport edge when the target is off-screen; hides the world-space quest marker on the matching `TalkingObject` while the edge marker is shown.

**Inheritance**  
**Parent class:** `UUserWidget`  
**Interfaces:** None.

**Constructors**  
`UPUQuestObjectiveOffscreenIndicatorWidget(const FObjectInitializer& ObjectInitializer)` — standard widget construction; add to viewport like other `UUserWidget` instances.

**Properties**

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `ScreenEdgeMargin` | `float` | EditAnywhere, BlueprintReadOnly, Category `Quest\|Markers` | Inset from viewport edges when clamping (pixels). Default `48.f`. |
| `MarkerDrawSize` | `FVector2D` | EditAnywhere, BlueprintReadOnly, Category `Quest\|Markers` | Edge marker size in layout pixels. Default `(64, 64)`. |
| `RootCanvas` | `UCanvasPanel*` | private `UPROPERTY` | Internal root; built in `EnsureWidgetTreeBuilt`. |
| `MarkerImage` | `UImage*` | private `UPROPERTY` | Internal marker image widget. |

**Methods (key)**  
| Method | Description |
|--------|-------------|
| `NativeOnInitialized` | Widget setup. |
| `NativeTick` | Updates indicator from geometry. |
| `EnsureWidgetTreeBuilt` / `UpdateIndicator` | Private helpers for layout and tracking. |

**Dependencies**  
`TalkingObject` / world quest marker behavior (see in-game wiring), active objective from `UPUQuestSubsystem`.

---

## Integration: `UPUProjectUmeowmiGameInstance` (quest-related)

These members exist on the project game instance; they forward to `UPUQuestSubsystem` or supply configuration.

**Quest configuration properties**

| Property Name | Type | Description |
|---------------|------|-------------|
| `QuestObjectiveContentTable` | `TObjectPtr<UDataTable>` | Authoring table for `FPUQuestObjectiveContentRow`. Set in Blueprint or defaults. |
| `bAutoStartInitialQuestOnNewGame` | `bool` | If true with valid `InitialQuestTag` and `InitialObjectiveTag`, `CreateNewGame` starts that quest (save not requested for that `StartQuest` call). |
| `InitialQuestTag` | `FGameplayTag` | Quest to start on new game. |
| `InitialObjectiveTag` | `FGameplayTag` | First objective on new game. |

**Quest API (forwarded)**  
`GetQuestSubsystem`, `GetQuestRootTag` / `SetQuestRootTag`, `StartQuest`, `SetActiveObjective`, `CompleteObjective`, `CompleteQuest`, `IsObjectiveCompleted`, `IsQuestCompleted`, `IsObjectiveActive`, `IsQuestActive`, `GetActiveQuestTag`, `GetActiveObjectiveTag`, `GetCompletedObjectiveTags`, `GetCompletedQuestTags`, `AddObjectiveProgress`, `GetObjectiveProgress`, `ClearActiveQuestState`, `GetObjectiveDisplayInfo`, `GetActiveObjectiveDisplayInfo`, `GetQuestDisplayInfo`.

**Lifecycle hooks**  
- **Init:** Assigns `SetCachedQuestObjectiveContentTable(QuestObjectiveContentTable)`, then after load/new game calls `EnsureQuestRootTagDefault` and `BroadcastQuestObjectiveChanged`.  
- **SaveGame:** `QuestSubsystem->ExportToSave(PlayerSaveGame)`; sets `SaveVersion = 2`.  
- **LoadGame:** `ImportFromSave`; `MigrateQuestSaveIfNeeded` may trigger an immediate `SaveGame()`.  
- **CreateNewGame:** `ResetQuestStateForNewGame`, then optional `StartQuest` from initial tags, then `ExportToSave` on the new save object.

---

## Persistence: `UPUPlayerSaveGame` (quest fields)

| Property Name | Type | Description |
|---------------|------|-------------|
| `CompletedQuestObjectiveTags` | `TSet<FGameplayTag>` | Completed objectives. |
| `CompletedQuestTags` | `TSet<FGameplayTag>` | Completed quests. |
| `ActiveQuestTag` | `FGameplayTag` | Active quest. |
| `ActiveObjectiveTag` | `FGameplayTag` | Active objective. |
| `ObjectiveProgressCounters` | `TMap<FGameplayTag, int32>` | Progress counters. |
| `SaveVersion` | `int32` | Must be `>= 2` for quest payload; migration handled in subsystem + game instance. |

---

## Document change log

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0.0 | 2025-03-25 | Documentation | Initial quest system API documentation for ProjectUmeowmi. |
