# Level Transition System — Developer Documentation

**Module:** `ProjectUmeowmi`  
**Primary source:** `Source/ProjectUmeowmi/Interactables/PULevelTransition.*`, `Source/ProjectUmeowmi/LevelTransition/PULevelSpawnPoint.*`, `Source/ProjectUmeowmi/PUProjectUmeowmiGameInstance.*` (level transition section)

Players move between maps via **`UPUProjectUmeowmiGameInstance::TransitionToLevel`**, triggered from **`APULevelTransition`** interactables (or Blueprint/C++). Destination placement uses **`APULevelSpawnPoint`** actors tagged to match the transition’s **`TargetSpawnPointTag`**. Optional **locks** (`LockID`) gate transitions until **`UnlockLevelTransition`** runs (dialogue events, Blueprint, or C++); unlocked IDs persist in **save data**.

---

## System overview

| Piece | Role |
|-------|------|
| `APULevelTransition` | `ATalkingObject` subclass: prompt + interact (or auto-trigger overlap) calls `TransitionToLevel` with configured level name and spawn tag; locked state plays `LockedDialogue` instead. |
| `APULevelSpawnPoint` | Placed in destination maps; `SpawnPointTag` matches `TargetSpawnPointTag`; optional `CameraPositionIndex` for isometric camera. |
| `UPUProjectUmeowmiGameInstance` | Fade sequence, `OpenLevel`, `OnLevelLoaded`, spawn positioning, order save/restore, UI hide/show delegates, lock set persisted via `SaveGame`. |
| Save game | `UPUPlayerSaveGame::UnlockedLevelTransitionIDs` mirrors `UnlockedLevelTransitionIDs` on the game instance. |

---

## Class API: `APULevelTransition`

**Class Name:** `APULevelTransition`

**Description**  
Interactable exit volume/door: inherits **`ATalkingObject`** with `ObjectType = System`. Does **not** require Dlg assets in `AvailableDialogues` for the prompt—**`CanInteract`** returns true when unlocked, or when locked **and** `LockedDialogue` is set (so the player can open the “locked” conversation). **`StartInteraction`** either calls **`PerformTransition`** (if unlocked) or **`StartDialogueAndSetInteracting(LockedDialogue)`** (if locked). Optional **`bAutoTrigger`** runs **`PerformTransition`** on sphere overlap when unlocked.

**Inheritance**  
**Parent class:** `ATalkingObject`  
**Interfaces:** (via parent) `IDlgDialogueParticipant`

**Constructors**  
`APULevelTransition()` — sets interaction range 250, default fade on, auto-trigger off.

**Properties**

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `TargetLevelName` | `FString` | EditAnywhere, BlueprintReadWrite | Destination map: short name (e.g. `L_Chapter0_2_LolaRoom`) or `/Game/...` path (game instance strips to filename). |
| `LockID` | `FName` | EditAnywhere, BlueprintReadWrite | If not `None`, transition is locked until that ID is in `UnlockedLevelTransitionIDs` on the game instance. |
| `LockedDialogue` | `UDlgDialogue*` | EditAnywhere, BlueprintReadWrite | Dialogue when locked; if null while locked, interaction logs a warning and does nothing visible. |
| `TargetSpawnPointTag` | `FName` | EditAnywhere, BlueprintReadWrite | Must match `APULevelSpawnPoint::SpawnPointTag` in the target level. |
| `bUseFadeTransition` | `bool` | EditAnywhere, BlueprintReadWrite | Passed to `TransitionToLevel` (default true). |
| `bAutoTrigger` | `bool` | EditAnywhere, BlueprintReadWrite | If true and unlocked, overlap immediately transitions (no interact press). |

**Methods**

| Method | Description | Return |
|--------|-------------|--------|
| `IsUnlocked` | `LockID == None` or game instance `IsLevelTransitionUnlocked(LockID)`. | `bool` |
| `CanInteract` | In range and (unlocked **or** locked with `LockedDialogue`). | `bool` |
| `StartInteraction` | Unlocked → `PerformTransition`; locked → start `LockedDialogue`. | `void` |
| `OnDialogueEvent_Implementation` | `UnlockLevelTransition` / `UnlockTransition` unlocks **this actor’s** `LockID`; `TransitionLevel` / `LevelTransition` calls `PerformTransition` if unlocked. Other events → `Super`. | `bool` |

**Events**  
Uses parent `ATalkingObject` overlap delegates.

**Usage**  
Place actor, set `TargetLevelName`, `TargetSpawnPointTag`, and optional `LockID` + `LockedDialogue`. Unlock via **`UPUProjectUmeowmiGameInstance::UnlockLevelTransition(LockID)`** or dialogue on **this** actor (see `OnDialogueEvent_Implementation`).

**Dependencies**  
`UPUProjectUmeowmiGameInstance`, Dlg (for locked flow and events).

**Notes**  
- Base **`ATalkingObject`** still fires **`UnlockDoor` / `UnlockLevelTransition` / `UnlockTransition`** using **`ParticipantName`** as LockID; **`APULevelTransition`** additionally handles unlock/transition events tied to **`LockID`** on the same class. Prefer consistent use of **`LockID`** on the transition actor vs **`ParticipantName`** for generic talking objects (see Dialogue doc).

---

## Class API: `APULevelSpawnPoint`

**Class Name:** `APULevelSpawnPoint`

**Description**  
Marker actor for player spawn position and rotation after a transition. **`GetAllActorsOfClass`** finds spawn points; the tag must match **`PendingSpawnPointTag`** from the last transition. If no match, the **first** spawn point in the level is used (warning logged). If none exist, default **PlayerStart** behavior applies (warning logged).

**Inheritance**  
**Parent class:** `AActor`

**Properties**

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `SpawnPointTag` | `FName` | EditAnywhere, BlueprintReadWrite | Id matched by `TransitionToLevel`’s spawn tag parameter. |
| `SpawnPointName` | `FString` | EditAnywhere, BlueprintReadWrite | Editor label only. |
| `CameraPositionIndex` | `int32` | EditAnywhere, BlueprintReadWrite | Isometric camera slot (0-based, clamped to character’s `NumberOfCameraPositions`). |
| `VisualMesh` | `UStaticMeshComponent*` | VisibleAnywhere, BlueprintReadOnly | Editor visualization. |
| `DirectionArrow` | `UArrowComponent*` | VisibleAnywhere, BlueprintReadOnly | Facing hint. |

**Methods:** `GetSpawnPointTag` / `SetSpawnPointTag`, `GetCameraPositionIndex` / `SetCameraPositionIndex`.

**Dependencies**  
`AProjectUmeowmiCharacter` for `SetCameraPositionIndex` + `InitializeCameraPositionFromBlueprint` after teleport.

---

## Integration: `UPUProjectUmeowmiGameInstance` (level transitions)

**Flow**

1. **`TransitionToLevel(TargetLevelName, SpawnPointTag, bUseFade)`**  
   - Ignores if **`bTransitionInProgress`**.  
   - **`SavePlayerState()`** — currently persists **active player order** (`FPUOrderBase`) if the character has a current order.  
   - Sets **`PendingSpawnPointTag`**, **`bTransitionInProgress = true`**.  
   - **`OnTransitionStarted(TargetLevelName)`** (BlueprintImplementableEvent).  
   - **`OnLevelTransitionUIHide`** — hide HUD-relevant UI during fade/load.  
   - Hides interaction widgets on **all** **`ATalkingObject`** actors (`HideInteractionWidgetForTransition`).  
   - Normalizes **`TargetLevelName`** to a short level name for **`OpenLevel`**.  
   - If **`bUseFade`**: black **fade out** (~1s), then **`LoadLevelAfterFade`** → **`UGameplayStatics::OpenLevel`**.  
   - If **no fade**: **`OpenLevel`** immediately (async).

2. **`HandlePostLoadMap`** (delegate)  
   - After load, short delay (0.1s), then **`OnLevelLoaded()`** if transition was in progress.

3. **`OnLevelLoaded()`**  
   - **`PositionPlayerAtSpawnPoint(PendingSpawnPointTag)`** — find **`APULevelSpawnPoint`**, teleport pawn, apply camera index.  
   - **`RestorePlayerState()`** — reapplies **saved order** to character if **`bHasSavedOrder`**.  
   - **Fade in** (~1.5s black → clear).  
   - Clears **`bTransitionInProgress`** and pending tag.  
   - **`OnTransitionCompleted()`**.  
   - After fade-in duration, **`OnLevelTransitionUIShow`** (timer) restores HUD visibility.

**Public API**

| Method / Property | Description |
|-------------------|-------------|
| `TransitionToLevel` | Entry point; see flow above. |
| `OnLevelLoaded` | Blueprint-callable hook; normally invoked from post-load pipeline. |
| `IsLevelTransitionInProgress` | True during fade/load until `OnLevelLoaded` completes. |
| `OnTransitionStarted` / `OnTransitionCompleted` | Blueprint events on game instance subclass. |
| `OnLevelTransitionUIHide` / `OnLevelTransitionUIShow` | Multicast delegates for HUD/overlays. |
| `GetSavedPlayerOrder` / `HasSavedPlayerOrder` / `ClearSavedPlayerOrder` | Order snapshot across maps. |
| `UnlockLevelTransition(LockID)` | Adds ID to **`UnlockedLevelTransitionIDs`**, calls **`SaveGame()`**. Returns false if `LockID` is `None`. |
| `IsLevelTransitionUnlocked(LockID)` | `true` if `LockID` is `None`, or set contains ID. |
| `GetUnlockedLevelTransitions` | Copy of unlocked IDs. |

**Protected / internal fields** (from header): `PendingSpawnPointTag`, `bTransitionInProgress`, `PendingLevelPath`, `SavedPlayerOrder`, `bHasSavedOrder`, `LevelTransitionUIShowTimerHandle`.

**Dependencies**  
`UGameplayStatics::OpenLevel`, `APlayerController::ClientSetCameraFade`, `APULevelSpawnPoint`, `AProjectUmeowmiCharacter`.

**Notes**  
- Packaged builds: prefer **short level name** for `TargetLevelName` (see comments in `TransitionToLevel`).  
- **Save data:** unlocks persist via normal save pipeline (`Import`/`Export` on `UPUPlayerSaveGame` for `UnlockedLevelTransitionIDs`).

---

## Persistence: `UPUPlayerSaveGame`

| Property Name | Type | Description |
|---------------|------|-------------|
| `UnlockedLevelTransitionIDs` | `TSet<FName>` | Lock IDs cleared for transitions (loaded into game instance on `LoadGame`). |

---

## Dialogue integration (recap)

- **`ATalkingObject::OnDialogueEvent`**: `UnlockDoor`, `UnlockLevelTransition`, `UnlockTransition` → **`UnlockLevelTransition(ParticipantName)`** — use when LockID is the participant name.  
- **`APULevelTransition::OnDialogueEvent`**: same unlock event names but uses **`LockID`** on the transition actor; **`TransitionLevel`** / **`LevelTransition`** performs the configured transition when unlocked.

---

## Document change log

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0.0 | 2025-03-25 | Documentation | Initial level transition system API documentation for ProjectUmeowmi. |
