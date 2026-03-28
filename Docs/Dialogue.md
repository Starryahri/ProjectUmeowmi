# Dialogue System — Developer Documentation

**Module:** `ProjectUmeowmi`  
**Primary source:** `Source/ProjectUmeowmi/Dialogue/`, `Source/ProjectUmeowmi/UI/PUDialogueBox.h`, `Source/ProjectUmeowmi/PUProjectUmeowmiGameInstance.h` (dialogue content & settings)

Conversations use the **Dlg System** marketplace/plugin (`UDlgDialogue`, `UDlgContext`, `IDlgDialogueParticipant`). Project-specific code adds **TalkingObject** actors, **PUDialogueBox** / **PUDialogueOption** UI, **localized line resolution** via data table + gameplay tags, **quest integration** (conditions/events as tags), **orders** via **APUDishGiver**, and **game instance** hooks for open/close and typewriter settings. **Related:** **`Orders.md`** (order types, dish giver, player), **`GameInstance.md`** (dialogue line table and typewriter settings).

---

## System overview

| Piece | Role |
|-------|------|
| **Dlg System** | Dialogue assets, graph, speech nodes, custom events targeting participants. |
| `ATalkingObject` | Interactable actor implementing `IDlgDialogueParticipant`; sphere overlap, interaction widget, starts dialogue, quest bool/float hooks, custom events (`OnDialogueEvent`). |
| `AProjectUmeowmiCharacter` | Player participant; holds `UPUDialogueBox`, overlap selection, order storage, dish scoring / scorecard helpers. |
| `UPUDialogueBox` | Main dialogue UI: rich text, portraits, options, typewriter, vignette, skip mode. |
| `UPUDialogueOption` | One choice row; forwards selection to Dlg context. |
| `UPUDialogueNodeData` | Optional Dlg node data: giant portrait slot. |
| `FPUDialogueLineContentRow` | Data table row: tag id → `FText` line body. |
| `UPUProjectUmeowmiGameInstance` | `DialogueLineContentTable`, `DialogueLineRootTag`, `ResolveDialogueLineDisplayText`, dialogue open/close multicast, typewriter settings (persisted). |
| `APUDishGiver` | `ATalkingObject` + `UPUOrderComponent`; dialogue conditions and order generation events. |

---

## Third-party: Dlg System

- Participants are **UObject**s implementing **`IDlgDialogueParticipant`** (C++ interface).  
- `ATalkingObject` and `AProjectUmeowmiCharacter` supply name, display name, icon, and override **conditions / events** as documented below.  
- Author dialogue in the Dlg editor; reference participant names consistent with `ParticipantName` on actors and the player.

---

## Class API: `ATalkingObject`

**Class Name:** `ATalkingObject`

**Description**  
Base interactable for NPCs, props, doors, and systems. Provides interaction range, prompt widget, available dialogues, Dlg participant identity, optional facing, emotes, quest markers, and handling for **Dlg custom events** (`OnDialogueEvent_Implementation`). Overlapping targets can be cycled on the player character.

**Inheritance**  
**Parent class:** `AActor`  
**Interfaces:** `IDlgDialogueParticipant`

**Constructors**  
`ATalkingObject()` — standard actor placement in level or spawn.

**Properties — identity & dialogue**

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `ObjectType` | `ETalkingObjectType` | EditAnywhere | NPC, Prop, System, or Door (affects interaction and door-specific logic). |
| `InteractionRange` | `float` | EditAnywhere | Radius of interaction sphere. |
| `InteractionKey` | `FName` | EditAnywhere | Shown on prompt (e.g. “Interact”). |
| `InteractionIcon` | `UTexture2D*` | EditAnywhere | Optional prompt icon. |
| `AvailableDialogues` | `TArray<UDlgDialogue*>` | EditAnywhere | Pool for `StartRandomDialogue`. |
| `InteractionWidgetClass` | `TSubclassOf<UTalkingObjectWidget>` | EditAnywhere | World/screen prompt UI. |
| `InteractionWidgetSpace` | `EWidgetSpace` | EditAnywhere | Screen vs world. |
| `bScaleWidgetWithOrthoZoom` | `bool` | EditAnywhere | Scale prompt with ortho camera zoom. |
| `ReferenceOrthoWidth` | `float` | EditAnywhere | Reference for zoom scaling. |
| `ParticipantName` | `FName` | EditAnywhere | **Dlg participant id**; also used as **level transition LockID** for unlock events. |
| `DisplayName` | `FText` | EditAnywhere | Shown in UI. |
| `ParticipantIcon` | `UTexture2D*` | EditAnywhere | Default portrait for Dlg. |
| `AllowedParticipantNames` | `TArray<FName>` | EditAnywhere | Filters which participants are passed into dialogue when building the active list. |
| `CurrentDialogueContext` | `UDlgContext*` | Protected (`UPROPERTY` BlueprintReadWrite on class) | Active context while talking. **Other C++ types** (e.g. player character) use **`GetCurrentDialogueContext()`** — the member is not publicly accessible outside **`ATalkingObject`**. |

**Properties — components**

| Property Name | Type | Description |
|---------------|------|-------------|
| `InteractionWidget` | `UWidgetComponent*` | Prompt widget. |
| `EmoteWidget` | `UWidgetComponent*` | Overhead emotes. |
| `InteractionSphere` | `USphereComponent*` | Overlap for range. |
| `QuestMarkerWidget` | `UWidgetComponent*` | Optional objective marker. |

**Properties — door (when `ObjectType == Door`)**

| Property Name | Type | Description |
|---------------|------|-------------|
| `bIsDoorOpen` | `bool` | Toggled on interact when unlocked. |
| `LockedDoorDialogue` | `UDlgDialogue*` | Dialogue when locked. |

**Properties — NPC facing**

| Property Name | Type | Description |
|---------------|------|-------------|
| `bRotateNPCToFacePlayer` / axis flags | `bool` | Lerp NPC to face player when dialogue starts. |
| `NPCFacingYawOffset` | `float` | Mesh forward correction. |
| `NPCFacingRotationSpeed` | `float` | Degrees per second. |
| `bRotatePlayerToFaceNPC` / axis flags | `bool` | Rotate player toward NPC. |
| `PlayerFacingYawOffset` | `float` | Player mesh correction. |

**Properties — dialogue-driven UI classes**

| Property Name | Type | Description |
|---------------|------|-------------|
| `ScorecardWidgetClass` | `TSubclassOf<UPUScorecardWidget>` | Used by `ShowScorecard` event. |
| `DishScoringWidgetClass` | `TSubclassOf<UPUDishScoringWidget>` | Override for `BeginDishScoring` (else player default). |

**Properties — emote & quest (abbrev.)**  
`bEnableEmotes`, `EmoteWidgetClass`, `EmoteDataTable`, draw size/scale/space; `bEnableQuestMarker`, `QuestObjectiveTags` (and legacy `QuestObjectiveTag` merged at load), `QuestMarkerWidgetClass`, etc.

**Methods (selected)**

| Method | Description |
|--------|-------------|
| `StartInteraction` / `EndInteraction` | Begin/end interact state; may start dialogue. |
| `StartRandomDialogue` / `StartSpecificDialogue` / `StartDialogueAndSetInteracting` | Entry points for Dlg. |
| `GetCurrentDialogueContext` | Public **`UDlgContext*`** accessor (BlueprintCallable / BlueprintPure); use instead of reading **`CurrentDialogueContext`** from outside the class. |
| `CanInteract` | Range, door lock, dialogue availability. |
| `SetInteractionWidgetClass` / `SetInteractionKey` / `SetInteractionIcon` / `RefreshInteractionWidget` | Runtime prompt updates. |
| `HideInteractionWidgetForTransition` | Hides prompt during level transition. |
| `ShowEmoteByTag` / `ClearEmote` / `IsEmoteActive` | Emote API. |
| `RefreshQuestMarkerFromGameInstance` / `MigrateDeprecatedQuestObjectiveTagIfNeeded` | Quest marker sync. |
| `TickFacePlayerLerp` | Call from player tick while NPC is rotating. |

**`IDlgDialogueParticipant` — project behavior**

| Method | Behavior |
|--------|----------|
| `CheckCondition_Implementation` | If `ConditionName` resolves to a **valid gameplay tag** under `UPUQuestSubsystem::QuestRootTag`, returns **`IsObjectiveCompleted(Tag)`**. |
| `GetBoolValue_Implementation` | Same tag pattern → **`IsObjectiveActive(Tag)`**. |
| `GetFloatValue` / `GetIntValue` / `GetNameValue` | Default: neutral / zero / `NAME_None`. |
| `OnDialogueEvent_Implementation` | See **Dialogue events** below. |

**Events**

| Event | Parameters | Description |
|-------|------------|-------------|
| `OnPlayerEnteredInteractionSphere` | `ATalkingObject*` | Player entered range. |
| `OnPlayerExitedInteractionSphere` | `ATalkingObject*` | Player left range. |

**Dependencies**  
Dlg System, `UPUProjectUmeowmiGameInstance`, `UPUQuestSubsystem`, `APUDishGiver`, `AProjectUmeowmiCharacter`, UI widgets.

**Notes**  
- Quest **events**: if `EventName` resolves to a gameplay tag under `QuestRootTag`, **`CompleteObjective(EventTag)`** runs.  
- **String-based events** (not registered as tags) handle unlocks, orders, hints, scorecard, dish scoring — see below.

---

## Dialogue events handled on `ATalkingObject` (`OnDialogueEvent`)

| Event name | Behavior |
|------------|----------|
| *(Gameplay tag under `QuestRootTag`)* | `UPUQuestSubsystem::CompleteObjective` |
| `UnlockDoor`, `UnlockLevelTransition`, `UnlockTransition` | `UPUProjectUmeowmiGameInstance::UnlockLevelTransition(ParticipantName)` — **`ParticipantName` must be the LockID** |
| `GenerateOrder` | `APUDishGiver::GenerateAndGiveOrderToPlayer` (only if actor is `APUDishGiver`) |
| `RevealHint_<Aspect>` | e.g. `RevealHint_Salt` → aspect `Salt`; `APUDishGiver::RevealHintToPlayer` |
| `ShowScorecard` | Ensures the dialogue box is on the **scoring viewport stack** (`SyncDialogueBoxToScoringLayer` with the active `UDlgContext`), then creates `ScorecardWidgetClass`, adds to viewport, `AProjectUmeowmiCharacter::ShowScorecard` — see **Scoring dialogue layout** below. |
| `EndDishScoring` | `AProjectUmeowmiCharacter::EndDishScoringMode` (restores non-scoring dialogue box and **re-applies** the current node to the restored widget when dialogue is still active). |
| `BeginDishScoring` | Creates scoring widget from `DishScoringWidgetClass` (talking object or player) and `BeginDishScoringModeWithWidget`, or `TryBeginDishScoringModeFromClass` on player; then **`RefreshDialogueBoxFromContext`** with the event context so the **new** scoring-layout dialogue box shows the current line/options. **You can fire this on any node** — it does not need to be the first node of the dialogue. |

Unknown events log and return false.

### Scoring dialogue layout (`ShowScorecard` / `BeginDishScoring`)

Entering **dish scoring mode** swaps the player’s **`DialogueBox`** to **`ScoringDialogueBoxWidgetClass`** (higher viewport Z, same stack as the scorecard — see **`Scorecard.md`**). That **replaces the widget instance** that received **`Open`** when the conversation started. Without re-binding, **`UDlgContext`** would still drive the **old** (removed) widget, leaving the visible box empty and blocking progression.

**C++ behavior (summary):**

- **`AProjectUmeowmiCharacter::ApplyScoringDialogueViewportLayer()`** — if **`ScoringDialogueBoxWidgetClass`** is set, **`SwapToScoringDialogueBox()`** (new scoring-layout widget). If it is **not** set, **reparents** the current **`DialogueBox`** to **`PUScoringDialogueViewportZOrder`** so it sits in the same stack as scorecard/dish scoring (logged as a warning — prefer assigning **`ScoringDialogueBoxWidgetClass`** on the character for a proper layout).
- **`AProjectUmeowmiCharacter::RefreshDialogueBoxFromContext(UDlgContext*)`** — defers **`OpenFromContextResync`** to the next tick so Dlg node data is consistent; skips queue / **`Close()`** if dialogue has already ended (avoids blank box). Then **`SetDialogueInputFocus()`** so Interact / advance still targets the visible widget after a swap.
- **`AProjectUmeowmiCharacter::SyncDialogueBoxToScoringLayer(UDlgContext*)`** — **`ApplyScoringDialogueViewportLayer`**, then **`RefreshDialogueBoxFromContext`**.
- **`UPUDialogueBox::AdvanceDialogue()`** — if Dlg reports options but **`DialogueOptions`** has **no** pre-placed **`UPUDialogueOption`** children (alternate Blueprint layout), advances via **`ChooseOption(0)`** + **`Update`** so dialogue does not softlock.
- **`BeginDishScoring`** (dialogue event): after entering scoring mode, **`RefreshDialogueBoxFromContext(Context)`** uses the event’s context (reliable when multiple **`ATalkingObject`**s overlap).
- **`ShowScorecard`** (dialogue event): **`SyncDialogueBoxToScoringLayer(Context)`** before creating the scorecard so dialogue sits on the **scoring stack** (see **`Scorecard.md`** for Z-order). The event also creates a scorecard widget and calls **`ShowScorecard`** on the player; that instance is tracked separately from the **elevated** embedded scorecard used during dish scoring.
- **Viewport Z and click-to-advance:** **`UPUDialogueBox`** adds to the viewport at **`PUScoringDialogueViewportZOrder` (50001)**, not default Z 0. The dish layer is **50000** and scorecard overlays **50002**; dialogue at **0** would draw **under** those layers and **lose mouse hits** to any leftover scoring UI.
- **`EndDishScoringMode`**: tears down tracked scorecard layers, **`RemoveOrphanScoringStackViewportWidgets()`**, then **`RestoreNonScoringDialogueBox`** (swap path) **or** relayers the dialogue box to **`PUScoringDialogueViewportZOrder`** (relayer-only path) — **even when** **`ActiveDishScoringWidget`** is already **null**. **`RefreshDialogueBoxFromContext`** runs when a talking object and context exist, same rule. See **`Scorecard.md`** for the full teardown list.

**Authoring:** Keep **`ScoringDialogueBoxWidgetClass`** (and scorecard/dish scoring classes) configured on the player Blueprint so **`ShowScorecard`** alone can still move dialogue into the correct layer.

---

## Class API: `APUDishGiver`

**Parent:** `ATalkingObject`

**Description**  
NPC that owns **`UPUOrderComponent`**, exposes order generation to dialogue/Blueprint, implements **Dlg** conditions for orders (`HasActiveOrder`, `OrderCompleted`, `NoActiveOrder` — see implementation), and helpers to generate orders, reveal hints, and analyze completion for dialogue variables.

**Key methods**

| Method | Description |
|--------|-------------|
| `GenerateOrderForDialogue` | Dialogue hook to build order text/data. |
| `GetOrderDialogueText` | FText for UI/lines. |
| `GenerateAndGiveOrderToPlayer` / `GenerateAndGiveOrderToPlayerWithDish` | Push order to player. |
| `RevealHintToPlayer` | Reveal radar hint by aspect `FName`. |
| `GetCurrentOrder` / `HasActiveOrder` / `ValidateDish` / `GetSatisfactionScore` | Delegate to `OrderComponent`. |
| `HandleOrderCompletion` / `ClearCompletedOrderFromPlayer` / `MarkOrderForClearing` / `ExecuteDelayedOrderClearing` | Completion flow. |
| `CheckCondition_Implementation` / `GetBoolValue_Implementation` / `GetParticipantDisplayName_Implementation` | Extended for order state (see source). |

**Properties:** `OrderComponent`; dialogue-exposed order/completion fields (`OrderDescription`, `MinIngredientCount`, satisfaction text, etc.) for Dlg variable binding in Blueprint.

---

## Class API: `UPUDialogueBox`

**Parent:** `UUserWidget`

**Description**  
Dlg-driven dialogue panel: binds **Common Rich Text** for styled body text, name, standard and optional **giant** portrait, vertical list for **`UPUDialogueOption`**, **typewriter** with optional per-char sound (from game instance settings), **vignette** post-process via dynamic material, **skip mode** (fast text, no sound, auto-advance when one option), click-to-advance, and **keyboard** (E/Space advance, F skip) when focused.

**Properties — bind widgets**

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `CurrentContext` | `UDlgContext*` | BlueprintReadOnly | Active Dlg context. |
| `ParticipantNameText` | `UTextBlock*` | BindWidget | Speaker name. |
| `DialogueText` | `UCommonRichTextBlock*` | BindWidget | Body (rich text styles from data table). |
| `ParticipantImage` | `UImage*` | BindWidget | Portrait. |
| `GiantParticipantImage` | `UImage*` | BindWidgetOptional | Large portrait when node has `UPUDialogueNodeData::bUseGiantPortraitSlot`. |
| `DialogueOptions` | `UVerticalBox*` | BindWidget | Option container. |
| `SkipButton` | `UButton*` | BindWidgetOptional | Toggles skip mode. |

**Properties — vignette (protected)**

| Property Name | Type | Description |
|---------------|------|-------------|
| `VignetteMaterial` | `TSoftObjectPtr<UMaterialInterface>` | Soft-loaded vignette. |
| `VignetteMaterialDirect` | `TObjectPtr<UMaterialInterface>` | Direct reference (preferred). |
| `VignetteIntensityParameterName` | `FName` | Material parameter (default `Intensity`). |
| `VignetteIntensityTarget` | `float` | Strength when open (0–1). |
| `VignetteFadeInDuration` / `VignetteFadeOutDuration` | `float` | Fade timings. |
| `SkipModeCharacterDelay` | `float` | Typewriter speed when skip mode on. |

**Methods**

| Method | Description | Return |
|--------|-------------|--------|
| `Open` / `Open_Implementation` | Show UI for context. | `void` |
| `Close` / `Close_Implementation` | Hide and cleanup. | `void` |
| `Update` / `Update_Implementation` | Refresh line/options from context. | `void` |
| `SetVignetteMaterial` | Runtime vignette swap. | `void` |
| `IsTypewriterActive` | Typewriter running. | `bool` |
| `CompleteTypewriter` | Reveal full line immediately. | `void` |
| `AdvanceDialogue` | Skip typewriter or advance line; if there are Dlg options but no option widgets, uses **`ChooseOption(0)`**. | `void` |
| `IsDialogueInteractive` | **`true`** when visibility is **`Visible`**, **`SelfHitTestInvisible`**, or **`HitTestInvisible`** (not **`Hidden`** / **`Collapsed`**) — used for click / key advance and character **`Interact`** gating. | `bool` |
| `SetDialogueInputFocus` | Restore Slate keyboard focus after swapping dialogue layout mid-conversation. | `void` |
| `GetFocusTarget` | Widget to focus after popups. | `UWidget*` |
| `IsSkipMode` / `SetSkipMode` | Skip mode. | `bool` / `void` |
| `DebugVignetteMaterial` | Debug vignette setup. | `void` |

**Dependencies**  
Dlg System, Common UI rich text, optional game instance for line resolution and settings.

---

## Class API: `UPUDialogueOption`

**Parent:** `UPUCommonUserWidget`

**Description**  
Single dialogue choice: `OptionIndex`, `OptionText`, `OptionButton`, `SelectOption` → Dlg selection; `Update` syncs from `UDlgContext`.

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `OptionIndex` | `int32` | EditAnywhere, BlueprintReadOnly | Index in Dlg option list. |
| `OptionText` | `UTextBlock*` | BindWidget | Label. |
| `OptionButton` | `UButton*` | BindWidget | Click target. |
| `CurrentContext` | `UDlgContext*` | BlueprintReadOnly | Dlg context. |
| `ParentDialogueBox` | `UPUDialogueBox*` | `UPROPERTY` | Owner box. |

---

## Class API: `UPUDialogueNodeData`

**Parent:** `UDlgNodeData`

**Description**  
Add to Dlg speech nodes in editor. **`bUseGiantPortraitSlot`**: when true, `UPUDialogueBox` shows the giant image slot; speaker state still drives which texture is used.

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `bUseGiantPortraitSlot` | `bool` | EditAnywhere, BlueprintReadWrite | Use giant vs standard portrait image. |

---

## Struct API: `FPUDialogueLineContentRow`

**Parent:** `FTableRowBase`

**Description**  
Localized (or centralized) **line body** text. Prefer **row name == gameplay tag string** (e.g. `D.Chapter.Scene.Line01`). `LineTag` is optional fallback if row names differ.

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `LineTag` | `FGameplayTag` | EditAnywhere, BlueprintReadWrite | Optional tag matching row (`D` category). |
| `Line` | `FText` | EditAnywhere, BlueprintReadWrite | Displayed line text. |

**Resolution:** `UPUProjectUmeowmiGameInstance::ResolveDialogueLineDisplayText` — if Dlg raw text is a registered **dialogue line tag** and a table row exists, returns **`Line`**; otherwise returns the raw string (legacy inline text).

---

## Class API: `UTalkingObjectWidget`

**Parent:** `UUserWidget`

**Description**  
Prompt above interactables: **`SetInteractionKey`**, **`SetInteractionIcon`**, **`SetSelectionState`** (multi-target dimming). Bind widgets: `InteractionKeyText`, `InteractionIcon`.

---

## Integration: `AProjectUmeowmiCharacter` (dialogue-related)

| Topic | Description |
|-------|-------------|
| `IDlgDialogueParticipant` | Player `ParticipantName` / `DisplayName` / `ParticipantIcon` (see character defaults). |
| `DialogueBox` | Reference to active `UPUDialogueBox` (often set in HUD). |
| `DefaultDialogueBoxWidgetClass` | Fallback class when restoring after scoring. |
| `ScoringDialogueBoxWidgetClass` | Alternate layout during dish scoring; swap/restore with `SwapToScoringDialogueBox` / `RestoreNonScoringDialogueBox`. |
| `ApplyScoringDialogueViewportLayer` / `RefreshDialogueBoxFromContext` / `SyncDialogueBoxToScoringLayer` | Scoring stack: swap widget class **or** reparent current box to scoring Z; rebind + refocus — see **Scoring dialogue layout** above. |
| `EndDishScoringMode` / `RemoveOrphanScoringStackViewportWidgets` | End scoring: tear down scorecard layers, sweep stray **`UPUDishScoringWidget`** / **`UPUScorecardWidget`**, restore dialogue class / Z, **`RefreshDialogueBoxFromContext`** — see **`Scorecard.md`**. |
| `SkipDialogueAction` | Hold to enable skip mode on dialogue box. |
| `GetDialogueBox` / `GetCurrentTalkingObject` | Accessors. |
| Overlap list | Tracks `ATalkingObject`s; **CycleInteractTarget** switches selection. |

Order/scorecard/dish scoring APIs tie into dialogue events listed above.

---

## Integration: `UPUProjectUmeowmiGameInstance` (dialogue)

| Property / API | Description |
|----------------|-------------|
| `DialogueLineContentTable` | `FPUDialogueLineContentRow` data table. |
| `DialogueLineRootTag` | Only tag ids under this root resolve from table (default `D` hierarchy if unset). |
| `ResolveDialogueLineDisplayText` | Resolves tag-based Dlg lines to `FText`. |
| `NotifyDialogueOpened` / `NotifyDialogueClosed` | Internal: sets `bDialogueOpen`, fires **`OnDialogueOpenedEvent`** / **`OnDialogueClosedEvent`**. |
| `IsDialogueOpen` | Query. |
| **Dialogue settings** (persisted on save) | `Get/SetDialogueTypewriterEnabled`, character delay, skip-on-input, typewriter sound, pitch variation, skip-mode delay — used by dialogue UI/audio. |

---

## Usage notes for designers

1. **Participants:** Match Dlg participant names to **`ParticipantName`** on `ATalkingObject` and the player character.  
2. **Quests:** Use **gameplay tags** under **`QuestRootTag`** for **conditions** (completed / active) and **custom events** (complete objective).  
3. **Locks:** Use **`UnlockLevelTransition`** (or alias names) with **`ParticipantName == LockID`** on doors/transitions.  
4. **Orders:** Use **`APUDishGiver`**, **`GenerateOrder`** event, and hint events **`RevealHint_<AspectName>`**.  
5. **Lines:** Prefer tag strings in Dlg speech text + **`DialogueLineContentTable`** for copy; fallback inline text still works.  
6. **Portraits:** Use **`UPUDialogueNodeData`** on speech nodes for giant layout.

---

## Document change log

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0.0 | 2025-03-25 | Documentation | Initial dialogue system API documentation for ProjectUmeowmi. |
| 1.1.0 | 2026-03-25 | Documentation | Intro cross-references to **`Orders.md`** and **`GameInstance.md`**. |
| 1.2.0 | 2026-03-26 | Documentation | **`ShowScorecard`** / **`BeginDishScoring`**: document scoring dialogue layout swap, **`RefreshDialogueBoxFromContext`**, **`SyncDialogueBoxToScoringLayer`**, and that **`BeginDishScoring`** may run on any node. |
| 1.3.0 | 2026-03-26 | Documentation | **`ATalkingObject::GetCurrentDialogueContext()`** public accessor; **`CurrentDialogueContext`** documented as protected for external C++ access; **`EndDishScoringMode`** doc uses getter. |
| 1.4.0 | 2026-03-26 | Documentation | **`ApplyScoringDialogueViewportLayer`**, relayer when **`ScoringDialogueBoxWidgetClass`** unset, **`SetDialogueInputFocus`**, **`AdvanceDialogue`** fallback without option slots. |
| 1.5.0 | 2026-03-28 | Documentation | Scoring stack: **`PUScoringDialogueViewportZOrder`** for **`UPUDialogueBox`** **`AddToViewport`**, **`EndDishScoringMode`** restore/refresh independent of **`ActiveDishScoringWidget`**, orphan viewport sweep, tracked **`ShowScorecard`** scorecard, **`IsDialogueInteractive`**, **`RefreshDialogueBoxFromContext`** ended-dialogue guard — cross-ref **`Scorecard.md`**. |
