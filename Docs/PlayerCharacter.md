# Player Character — Developer Documentation

**Module:** `ProjectUmeowmi`  
**Primary source:** `Source/ProjectUmeowmi/ProjectUmeowmiCharacter.h` / `.cpp`

**`AProjectUmeowmiCharacter`** is the **player pawn**: **Enhanced Input**, isometric **camera**, **TalkingObject** overlap and selection, **Dlg** participant, **order** state, **dish preview** and **scene capture** for the scorecard, **journal** access, **dish scoring** UI mode, **emotes**, optional **quest off-screen indicator**, and optional **`IPUInteractableInterface`** registration. This doc is a **hub** — feature details stay in the linked guides.

---

## Major subsystems on the character

| Area | What lives here | Read more |
|------|-----------------|-----------|
| **Input** | Move, look, zoom, rotate camera, interact, journal open, journal dish cycle, skip dialogue, grid toggle, jump | — |
| **Camera** | Spring arm, orthographic zoom, optional grid movement | — |
| **Dialogue** | `DialogueBox`, `DefaultDialogueBoxWidgetClass`, `ScoringDialogueBoxWidgetClass`; `GetDialogueBox`, `GetCurrentTalkingObject`, talking object list / cycle; **`ATalkingObject::GetCurrentDialogueContext`** (read active `UDlgContext` from the selected NPC); `RefreshDialogueBoxFromContext`, `SyncDialogueBoxToScoringLayer` (scoring layout + context rebind) | `Dialogue.md` |
| **Orders** | `CurrentOrder`, `bHasCurrentOrder`, `bCurrentOrderCompleted`, `SetCurrentOrder`, `ClearCurrentOrder`, `RevealHintOnCurrentOrder`, `SetOrderResult`, `ShowScorecard` | `Orders.md`, `Scorecard.md` |
| **Dish capture (scorecard)** | `DishCaptureComponent`, `CaptureDishSnapshotFromPlatingStation`, `RefreshDishCapturePreviewFromDishPreview`, many tuning properties | `Scorecard.md`, `DishCustomization.md` |
| **Dish scoring mode** | `DishScoringWidgetClass`, `BeginDishScoringModeWithWidget`, `EndDishScoringMode`, `IsInDishScoringMode`; scorecard stack: `RefreshDialogueBoxFromContext`, `SyncDialogueBoxToScoringLayer`, `RemoveOrphanScoringStackViewportWidgets`, tracked `ElevatedDishScoringScorecard` / `ActiveDialogueShowScorecardWidget` | `Scorecard.md`, `Dialogue.md` |
| **Journal** | `JournalWidget` ref, `ToggleJournal` (input), bumper cycle when Recipes active | `Journal.md` |
| **Emotes** | `EmoteWidget` component, `EmoteDataTable`, `ShowEmoteByTag`, `ClearEmote` | `Emotes.md` |
| **Quest HUD** | Optional `QuestObjectiveOffscreenIndicator` | `QuestSystem.md` |
| **Dish preview** | `DishPreviewComponent` — plated dish above head | `DishCustomization.md` |
| **Interactable interface** | `RegisterInteractable` / `UnregisterInteractable`, `CurrentInteractable` | `Interactables.md` |

---

## Order API (player copy)

The **authoritative** generation path often starts on **`APUDishGiver`** / **`UPUOrderComponent`**, but the **player** holds the active **`FPUOrderBase`** for gameplay.

| Method | Description |
|--------|-------------|
| `SetCurrentOrder` / `GetCurrentOrder` / `HasCurrentOrder` / `ClearCurrentOrder` | Owns the working order struct. |
| `RevealHintOnCurrentOrder(AspectName)` | Adds hint to order (dialogue events). |
| `SetOrderResult` / `GetOrderCompleted` / `GetOrderSatisfaction` | Completion and satisfaction. |
| `ClearCompletedOrder` / `DisplayOrderResult` / `GetOrderResultText` | Feedback and cleanup. |
| `ShowScorecard(UPUScorecardWidget*)` | Populates scorecard from current completed order + capture pipeline. |

Full struct fields: **`Orders.md`**.

---

## IDlgDialogueParticipant

Implements **`ParticipantName`**, **`DisplayName`**, **`ParticipantIcon`** for the **player** as a Dlg participant.

---

## Cross-references

| Topic | Doc |
|-------|-----|
| Game instance (not on character) | `GameInstance.md` |
| Dish customization component (on stations / actors) | `DishCustomization.md` |
| Talking objects and stations | `Interactables.md`, `Dialogue.md` |

---

## Document change log

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0.0 | 2026-03-25 | Documentation | Initial player character hub documentation for ProjectUmeowmi. |
| 1.1.0 | 2026-03-26 | Documentation | Listed **`RefreshDialogueBoxFromContext`** and **`SyncDialogueBoxToScoringLayer`** under dialogue / dish scoring — see **`Dialogue.md`**. |
| 1.2.0 | 2026-03-26 | Documentation | Dialogue row: **`ATalkingObject::GetCurrentDialogueContext`** for reading the active context from the overlap target — see **`Dialogue.md`**. |
| 1.3.0 | 2026-03-28 | Documentation | Dish scoring row: **`RemoveOrphanScoringStackViewportWidgets`** and tracked scorecard pointers — see **`Scorecard.md`** / **`Dialogue.md`** (viewport Z and **`EndDishScoringMode`**). |
