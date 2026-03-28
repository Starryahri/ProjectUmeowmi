# UI — Developer Documentation

**Module:** `ProjectUmeowmi`  
**Primary source:** `Source/ProjectUmeowmi/UI/`

Project UI builds on **Common UI** (`UCommonUserWidget`, `UCommonButtonBase`). Shared bases keep styling and behavior consistent across screens. Feature-specific widgets (dialogue, cooking, journal, etc.) are documented in the **system** guides; this file describes **shared types** and a **map** of major widgets.

**Related:** Modal notifications and the game-instance popup pipeline are documented in **`Popups.md`** (not duplicated here).

---

## Base classes

| Class | Parent | Role |
|-------|--------|------|
| **`UPUCommonUserWidget`** | `UCommonUserWidget` | Abstract base for most project screens: consistent styling, `NativeConstruct` / `NativeDestruct`. |
| **`UPUCommonButton`** | `UCommonButtonBase` | Project-wide button; set style on a child Blueprint to affect all instances. |

New screens should inherit **`UPUCommonUserWidget`** (or a project intermediate Blueprint) so Common UI theming and activation policy stay aligned.

---

## Feature areas (widget map)

Representative headers under `Source/ProjectUmeowmi/UI/`:

| Area | Widgets / types |
|------|-------------------|
| **Dialogue** | `UPUDialogueBox` (adds to viewport at **`PUScoringDialogueViewportZOrder`** — see **`Scorecard.md`**), `UPUDialogueOption` |
| **Dish customization** | `UPUDishCustomizationWidget`, `PUPlatingWidget`, `PUIngredientSlot`, `PUIngredientButton`, `PURadialMenu`, `PUPreparationCheckbox`, `PUIngredientQuantityControl`, … |
| **Journal** | `UPUJournalWidget`, `PUJournalTabListWidget`, `PURecipesSectionWidget`, `PUIngredientsSectionWidget`, `PUJournalSectionWidget`, `PUTownSectionWidget`, `PUPeopleSectionWidget`, `PUSettingsSectionWidget`, `PUJournalTypes` — see **`Journal.md`** |
| **Orders / scoring** | `UPUScorecardWidget`, `PUScorecardTypes`, `UPUDishScoringWidget`, `PURadarChart`, `PUAspectProfileWidget` — see **`Scorecard.md`** |
| **Quest** | `UPUQuestObjectiveOffscreenIndicatorWidget` |
| **Emotes** | `UPUEmoteWidget`, `PUEmoteData` — see **`Emotes.md`** |
| **Popups** | `UPUPopupWidget`, `PUPopupData` — see **`Popups.md`** |
| **Misc** | `SDirectionLineWidget` (Slate), `PUIngredientDragDropOperation`, `PURecipeIngredientEntryWidget` |

---

## Where to read more

| Topic | Doc |
|-------|-----|
| Dialogue UI | `Dialogue.md` |
| Customization / plating UI | `DishCustomization.md` |
| Quest HUD markers | `QuestSystem.md` |
| Popups (data, game instance, input) | `Popups.md` |
| Save-backed settings shown in UI | `SaveLoad.md` |
| Interactables vs TalkingObject, `IPUInteractableInterface` | `Interactables.md` |
| Journal (tabs, recipes section, game instance) | `Journal.md` |
| Scorecard, dish scoring mode, radar chart | `Scorecard.md` |
| Architecture overview | `Architecture.md` |
| Game instance hub | `GameInstance.md` |
| Player character hub | `PlayerCharacter.md` |
| Orders (structs, component, dish giver) | `Orders.md` |
| Emotes (widget + data table) | `Emotes.md` |
| Full doc index | `README.md` |

---

## Document change log

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0.0 | 2025-03-25 | Documentation | UI architecture documentation (split from former `UIPopups.md`; popups in `Popups.md`). |
| 1.1.0 | 2025-03-25 | Documentation | Linked **`Journal.md`** from the feature map and “Where to read more.” |
| 1.2.0 | 2025-03-25 | Documentation | Linked **`Scorecard.md`** for orders/scoring UI. |
| 1.3.0 | 2026-03-25 | Documentation | Linked hub docs (`Architecture`, `GameInstance`, `PlayerCharacter`, `Orders`, `Emotes`, `README`). |
| 1.4.0 | 2026-03-25 | Documentation | All developer docs moved under **`Docs/`**; index is **`Docs/README.md`**. |
| 1.5.0 | 2026-03-28 | Documentation | Dialogue row: viewport Z for **`UPUDialogueBox`** per **`Scorecard.md`**. |
