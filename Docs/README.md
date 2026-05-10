# ProjectUmeowmi — Documentation index

Unreal Engine 5 game project. Gameplay centers on **dialogue** (Dlg), **dish customization** and **orders**, **quests**, **journal/recipes**, and **level transitions** with save-backed progression.

Class/API-style guides live in **`Docs/`** (this folder). Folder-level `README` files under `Source/` cover some **feature-specific setup**; see below.

---

## Developer documentation

| Document | Topic |
|----------|--------|
| [`Architecture.md`](Architecture.md) | High-level module map and data flow between major systems |
| [`GameInstance.md`](GameInstance.md) | `UPUProjectUmeowmiGameInstance` — hub API and cross-references |
| [`PlayerCharacter.md`](PlayerCharacter.md) | `AProjectUmeowmiCharacter` — responsibilities and links to feature docs |
| [`Orders.md`](Orders.md) | `FPUOrderBase`, `UPUOrderComponent`, dish giver, validation/satisfaction |
| [`Dialogue.md`](Dialogue.md) | Dlg, `ATalkingObject`, dialogue UI, line table, talking-object events |
| [`DishCustomization.md`](DishCustomization.md) | Customization component, `FPUDishBase`, blueprint libraries, plating |
| [`DishCustomizationRoadmap.md`](DishCustomizationRoadmap.md) | Phased plan: 2D shell, Congee pipeline, spine audit, slimming legacy 3D |
| [`Scorecard.md`](Scorecard.md) | Scorecard widget, `FPUScorecardData`, dish scoring mode, radar chart |
| [`Journal.md`](Journal.md) | Journal tabs, recipes section, game instance journal API |
| [`QuestSystem.md`](QuestSystem.md) | Quest subsystem, objectives, save fields |
| [`SaveLoad.md`](SaveLoad.md) | `UPUPlayerSaveGame`, create/load/new game |
| [`LevelTransitions.md`](LevelTransitions.md) | `APULevelTransition`, spawn points, locks |
| [`Interactables.md`](Interactables.md) | `IPUInteractableInterface`, TalkingObject vs interface path |
| [`UI.md`](UI.md) | Shared UI bases (`UPUCommonUserWidget`), widget map |
| [`Popups.md`](Popups.md) | `FPopupData`, game instance popup manager |
| [`Emotes.md`](Emotes.md) | `UPUEmoteWidget`, `FPUEmoteData` |

---

## Designer setup & content (this folder)

| Document | Topic |
|----------|--------|
| [`OrderHintRadarChartSetup.md`](OrderHintRadarChartSetup.md) | Order hints and radar chart |
| [`RichTextDialogueSetup.md`](RichTextDialogueSetup.md) | Rich text dialogue |
| [`ScorecardSetup.md`](ScorecardSetup.md) | Scorecard UI setup |
| [`AspectProfileWidgetSetup.md`](AspectProfileWidgetSetup.md) | Aspect profile widget |

---

## Other references

| Location | Purpose |
|----------|--------|
| [`Ingredient_Flavor_Profiles.md`](Ingredient_Flavor_Profiles.md) | Ingredient/aspect design notes |
| [`../Source/ProjectUmeowmi/Dialogue/OrderSystem_README.md`](../Source/ProjectUmeowmi/Dialogue/OrderSystem_README.md) | Order/dialogue behavior notes (active order prevention, conditions) |
| [`COOK_ERROR_FIX.md`](COOK_ERROR_FIX.md) | Cook / build troubleshooting |
| [`DEBUG_PHYSICS_ERROR.md`](DEBUG_PHYSICS_ERROR.md) | Physics debug notes |

Repository entry point: **[`../README.md`](../README.md)** (project root).

---

## Engine

Developed with **Unreal Engine 5** (project targets a specific minor version; see `.uproject` and engine association in your environment).

---

## Document change log

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0.0 | 2026-03-25 | Documentation | Documentation index; developer guides consolidated under `Docs/`. |
