# Save / Load System — Developer Documentation

**Module:** `ProjectUmeowmi`  
**Primary source:** `Source/ProjectUmeowmi/PUPlayerSaveGame.*`, `Source/ProjectUmeowmi/PUProjectUmeowmiGameInstance.*` (Save/Load, persistence-related subsystems)

Persistent progress uses Unreal’s **`UGameplayStatics::SaveGameToSlot` / `LoadGameFromSlot`** with a single save class, **`UPUPlayerSaveGame`**. The authoritative runtime copy lives on **`UPUProjectUmeowmiGameInstance`**: it mirrors save fields into member variables, merges **quest** state through **`UPUQuestSubsystem::ExportToSave` / `ImportFromSave`**, and writes **`SaveVersion`** (currently **2**) for migrations.

---

## System overview

| Piece | Role |
|-------|------|
| `UPUPlayerSaveGame` | `USaveGame` subclass: serialized payload (ingredients, dishes, locks, dialogue settings, tutorial, quest fields, version). |
| `UPUProjectUmeowmiGameInstance` | Owns `PlayerSaveGame` pointer, copies to/from disk, drives **Init** load path, **CreateNewGame** reset, explicit **SaveGame** / **LoadGame**. |
| `UPUQuestSubsystem` | Quest state exported/imported inside `SaveGame` / `LoadGame`; **`MigrateQuestSaveIfNeeded`** bumps old saves to version 2. |

**Default slot:** `PlayerSave` (user index **0**). All Blueprint-callable APIs default to this slot name.

---

## Class API: `UPUPlayerSaveGame`

**Class Name:** `UPUPlayerSaveGame`

**Description**  
Holds only data that should persist between sessions. The game instance copies these fields immediately before `SaveGameToSlot` and applies them after `LoadGameFromSlot`.

**Inheritance**  
**Parent class:** `USaveGame`

**Constructors**  
`UPUPlayerSaveGame()` — sets **`SaveVersion = 2`**.

**Properties**

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `UnlockedIngredientTags` | `TSet<FGameplayTag>` | VisibleAnywhere | Pantry / unlock progression. |
| `UnlockedDishTags` | `TSet<FGameplayTag>` | VisibleAnywhere | Recipe journal unlocks. |
| `CompletedDialogueNames` | `TSet<FName>` | VisibleAnywhere | Dialogue assets marked complete (`MarkDialogueCompleted`). |
| `UnlockedLevelTransitionIDs` | `TSet<FName>` | VisibleAnywhere | Level transition **LockID** values cleared for travel. |
| `bUseDialogueTypewriterEffect` | `bool` | VisibleAnywhere | Global typewriter on/off. |
| `DialogueTypewriterCharacterDelay` | `float` | VisibleAnywhere | Seconds per character (normal typewriter). |
| `bDialogueTypewriterSkipOnInput` | `bool` | VisibleAnywhere | Click “next” completes line early. |
| `DialogueSkipModeCharacterDelay` | `float` | VisibleAnywhere | Seconds per character when skip mode is active (UI). |
| `bTutorialCompleted` | `bool` | VisibleAnywhere | Tutorial finished flag. |
| `TutorialStep` | `int32` | VisibleAnywhere | 0 = not started; 1–7 in progress / complete. |
| `CompletedQuestObjectiveTags` | `TSet<FGameplayTag>` | VisibleAnywhere | Quest subsystem. |
| `CompletedQuestTags` | `TSet<FGameplayTag>` | VisibleAnywhere | Quest subsystem. |
| `ActiveQuestTag` | `FGameplayTag` | VisibleAnywhere | Quest subsystem. |
| `ActiveObjectiveTag` | `FGameplayTag` | VisibleAnywhere | Quest subsystem. |
| `ObjectiveProgressCounters` | `TMap<FGameplayTag, int32>` | VisibleAnywhere | Quest subsystem. |
| `SaveVersion` | `int32` | VisibleAnywhere | **2** = current schema (includes quest block). |

**Notes**  
- **`DialogueTypewriterSound`** and other non-serialized settings on the game instance are **not** stored in `UPUPlayerSaveGame` (sound is configured on the instance, not in the save object).  
- **`CurrentDishTag`** (journal) is **not** persisted in the save class — runtime-only on the game instance.

---

## Integration: `UPUProjectUmeowmiGameInstance`

### `SaveGame(const FString& SlotName = "PlayerSave")`

1. Creates **`UPUPlayerSaveGame`** if `PlayerSaveGame` is null.  
2. Copies from instance state into the save object:  
   - `UnlockedIngredientTags`, `UnlockedDishTags`, `CompletedDialogueNames`, `UnlockedLevelTransitionIDs`  
   - Dialogue typewriter: `bUseDialogueTypewriterEffect`, `DialogueTypewriterCharacterDelay`, `bDialogueTypewriterSkipOnInput`, `DialogueSkipModeCharacterDelay`  
   - `bTutorialCompleted`, `TutorialStep`  
3. **`UPUQuestSubsystem::ExportToSave(PlayerSaveGame)`** — quest sets and maps.  
4. Sets **`PlayerSaveGame->SaveVersion = 2`**.  
5. **`UGameplayStatics::SaveGameToSlot(PlayerSaveGame, SlotName, 0)`**.

Returns **true** if the platform save succeeds.

### `LoadGame(const FString& SlotName = "PlayerSave")`

1. Returns **false** if no file in slot.  
2. Loads and casts to **`UPUPlayerSaveGame`**.  
3. Copies all mirrored fields from save object into the game instance (same list as above).  
4. **`UPUQuestSubsystem::ImportFromSave`**.  
5. **`MigrateQuestSaveIfNeeded`**: if **`SaveVersion < 2`**, sets version to 2 and triggers **`SaveGame()`** to persist the upgraded file.  
6. **Dish migration:** if **`UnlockedDishTags`** is empty but **`StartingDishTags`** is non-empty, fills from starting dishes (log).  

Returns **true** on success.

### `CreateNewGame(bool bClearSaveFile = true)`

- Respects **`bClearSaveOnNewGame`** when **`bClearSaveFile`** is true: may **`DeleteSaveGame`**.  
- Clears inventory, dishes, dialogue names, level locks, tutorial, current dish tag.  
- **`UPUQuestSubsystem::ResetQuestStateForNewGame`**.  
- Reapplies **`StartingIngredientTags`** / **`StartingDishTags`**.  
- Optionally **`StartQuest(InitialQuestTag, InitialObjectiveTag, false)`** when **`bAutoStartInitialQuestOnNewGame`** and tags valid.  
- Builds a fresh **`UPUPlayerSaveGame`** in memory with current state, **`ExportToSave`**, **`SaveVersion = 2`** — **does not** call **`SaveGame()`** to disk at the end; persistence happens on the next explicit save or auto-save path.

### Other APIs

| API | Description |
|-----|-------------|
| `DoesSaveGameExist` | Wraps `UGameplayStatics::DoesSaveGameExist(SlotName, 0)`. |
| `DeleteSaveGame` | Deletes slot file; sets **`PlayerSaveGame = nullptr`**. |

### Startup (`Init`)

- If **`bAlwaysStartNewGame`**: **`CreateNewGame()`**, then quest init broadcast; **returns** (ignores existing file).  
- Else **`LoadGame()`**; on failure **`CreateNewGame()`**.  
- Then quest root default + **`BroadcastQuestObjectiveChanged`** on subsystem.

### Debug properties

| Property | Description |
|----------|-------------|
| `bClearSaveOnNewGame` | When true (default), **`CreateNewGame(true)`** tends to delete the save file per internal logic. |
| `bAlwaysStartNewGame` | Forces new game path on **every** launch (ignores load). |

---

## Automatic save triggers (non-exhaustive)

These call **`SaveGame()`** after mutating persisted state:

| Trigger | Notes |
|---------|--------|
| `UnlockIngredient` | After adding a new tag. |
| `UnlockIngredients` | If any **new** unlocks. |
| `UnlockDish` | After adding a dish tag. |
| `MarkDialogueCompleted` | After adding a dialogue name. |
| `AdvanceTutorialStep` | When not yet at final step (final step calls **`SetTutorialCompleted`**). |
| `SetTutorialCompleted` | Sets completed + step 7. |
| `SetDialogueTypewriterEnabled` / `SetDialogueTypewriterSpeed` / `SetDialogueTypewriterSkipOnInput` / `SetDialogueSkipModeSpeed` | Persists dialogue UX settings. |
| `UnlockLevelTransition` | After adding a LockID. |
| Quest ops with **`bSave = true`** | Subsystem **`RequestSave`** → **`SaveGame()`** on the project game instance. |
| **`LoadGame`** after quest migration | If **`MigrateQuestSaveIfNeeded`** returns true. |

**Not auto-saved:** e.g. **`SetDialogueTypewriterSound`** (no save call in current implementation — sound is instance/editor config, not in `UPUPlayerSaveGame`).

---

## Versioning and migration

| Version | Meaning |
|---------|---------|
| **&lt; 2** | Older file without quest fields; **`MigrateQuestSaveIfNeeded`** sets **`SaveVersion = 2`** and may re-save. |
| **2** | Current: full quest payload on `UPUPlayerSaveGame`. |

Extend by bumping **`SaveVersion`**, adding fields to **`UPUPlayerSaveGame`**, and implementing migration in load paths (similar to quest migration).

---

## Cross-references

- **Quest:** `QuestSystem.md` — `ExportToSave` / `ImportFromSave` / `MigrateQuestSaveIfNeeded`.  
- **Level transitions:** `LevelTransitions.md` — `UnlockedLevelTransitionIDs`.  
- **Orders across maps:** in-memory **`SavedPlayerOrder`** on game instance during **`TransitionToLevel`** (not the same as slot save; see LevelTransitions doc).

---

## Document change log

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0.0 | 2025-03-25 | Documentation | Initial save/load system API documentation for ProjectUmeowmi. |
