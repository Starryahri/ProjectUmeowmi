# Game Instance — Developer Documentation

**Module:** `ProjectUmeowmi`  
**Primary source:** `Source/ProjectUmeowmi/PUProjectUmeowmiGameInstance.h` (implementation pairs with `.cpp`)

**`UPUProjectUmeowmiGameInstance`** is the **persistent hub** across level loads: transitions, save game, unlocks, journal data, quest **forwarding**, popups, dialogue line resolution, and global dialogue UI settings. This file summarizes **surface area** and points to deeper docs — it does not duplicate **`SaveLoad.md`**, **`Dialogue.md`**, **`QuestSystem.md`**, etc.

---

## Responsibilities (index)

| Category | Role | Detail doc |
|----------|------|----------------|
| **Level transition** | `TransitionToLevel`, spawn tagging, fade UI delegates, saved order carry | **`LevelTransitions.md`**, **`SaveLoad.md`** |
| **Save / load / new game** | `SaveGame`, `LoadGame`, `CreateNewGame`, `DeleteSaveGame`, `DoesSaveGameExist` | **`SaveLoad.md`** |
| **Ingredient inventory** | `UnlockIngredient(s)`, `IsIngredientUnlocked`, `GetUnlockedIngredients` | **`SaveLoad.md`** (persistence) |
| **Recipe journal** | `UnlockDish`, `GetOrderedUnlockedDishTags`, `CycleJournalDish`, `GetDishDataForTag`, `CurrentDishTag` | **`Journal.md`**, **`DishCustomization.md`** (tables) |
| **Quest** | Thin **Blueprint** API forwarding to **`UPUQuestSubsystem`**: start/complete objective & quest, progress, content table | **`QuestSystem.md`** |
| **Level locks** | `UnlockLevelTransition`, `IsLevelTransitionUnlocked` | **`LevelTransitions.md`** |
| **Popups** | `ShowPopup`, queue, ingredient unlock helpers | **`Popups.md`** |
| **Dialogue** | `ResolveDialogueLineDisplayText`, `DialogueLineContentTable`, `DialogueLineRootTag`; `NotifyDialogueOpened/Closed`, `IsDialogueOpen`; typewriter getters/setters | **`Dialogue.md`** |
| **Tutorial** | `IsTutorialModeEnabled`, `Get/SetTutorialStep`, `AdvanceTutorialStep`, `SetTutorialCompleted`, ingredient tags for steps | — |
| **Dialogue state (stub)** | `MarkDialogueCompleted`, `IsDialogueCompleted` | **`Dialogue.md`** (future) |

---

## Key API (selected)

### Level transition

| Method / member | Description |
|-----------------|-------------|
| `TransitionToLevel(TargetLevelName, SpawnPointTag, bUseFade)` | Loads target map and stores spawn resolution for **`OnLevelLoaded`**. |
| `OnLevelLoaded` | Positions player at spawn; Blueprint hook. |
| `GetSavedPlayerOrder` / `HasSavedPlayerOrder` / `ClearSavedPlayerOrder` | Order snapshot across transitions (see **`LevelTransitions.md`**). |
| `IsLevelTransitionInProgress` | True during fade/load. |
| `OnLevelTransitionUIHide` / `OnLevelTransitionUIShow` | Multicast — hide/show HUD during transition. |

### Recipe journal

| Method | Description |
|--------|-------------|
| `SetCurrentDishTag` / `GetCurrentDishTag` / `ClearCurrentDishTag` | Context while customizing; journal opens on this dish. |
| `GetDishDataForTag` | Fills **`FPUDishBase`** from configured **`DishDataTable`** (and related data). |
| `GetIngredientDataTable` | Authoring table reference. |

### Quest (forwarding)

`GetQuestSubsystem()` returns **`UPUQuestSubsystem`**. Game instance exposes **`StartQuest`**, **`SetActiveObjective`**, **`CompleteObjective`**, **`CompleteQuest`**, progress helpers, **`QuestObjectiveContentTable`**, and new-game **`InitialQuestTag`** / **`InitialObjectiveTag`** — see **`QuestSystem.md`**.

### Popups

`ShowPopup`, `ShowPopupWithCallback`, `ShowIngredientUnlockPopup`, `CloseCurrentPopup`, `OnPopupClosedEvent` — **`Popups.md`**.

### Dialogue content

| Member | Description |
|--------|-------------|
| `DialogueLineContentTable` | **`FPUDialogueLineContentRow`** rows; tag-based line body. |
| `ResolveDialogueLineDisplayText` | Table resolution for Dlg raw text. |

---

## Properties (authoring)

Set on the **Game Instance** Blueprint class: **`DishDataTable`**, **`IngredientDataTable`**, **`PopupWidgetClass`**, **`QuestObjectiveContentTable`**, dialogue tables, tutorial ingredient tags, save debug flags (`bClearSaveOnNewGame`, `bAlwaysStartNewGame`), etc.

---

## Cross-references

| Topic | Doc |
|-------|-----|
| Save slots and what persists | `SaveLoad.md` |
| Journal cycling and UI | `Journal.md` |
| Quest subsystem internals | `QuestSystem.md` |
| Transitions and locks | `LevelTransitions.md` |
| Popups | `Popups.md` |
| Dialogue lines and UI events | `Dialogue.md` |

---

## Document change log

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0.0 | 2026-03-25 | Documentation | Initial game instance hub documentation for ProjectUmeowmi. |
