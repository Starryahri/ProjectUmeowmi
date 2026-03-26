# Orders — Developer Documentation

**Module:** `ProjectUmeowmi`  
**Primary source:** `DishCustomization/PUOrderBase.h`, `PUOrderComponent.h`, `PUOrderBlueprintLibrary.h`, `Dialogue/PUDishGiver.*`, `ProjectUmeowmiCharacter` (order copy)

Orders describe **what the player should cook**: base recipe (**`FPUDishBase` BaseDish**), **aspect targets**, **completed dish**, and **satisfaction**. **Generation** usually happens on **`APUDishGiver`** via **`UPUOrderComponent`**; the **player character** holds the active **`FPUOrderBase`**. Dialogue **conditions** and **events** tie the loop together — see **`Dialogue.md`**. Behavioral notes (single active order, dialogue-only generation) are in **[`../Source/ProjectUmeowmi/Dialogue/OrderSystem_README.md`](../Source/ProjectUmeowmi/Dialogue/OrderSystem_README.md)**.

---

## Types: `FOrderAspectRequirement` & `EOrderAspectType`

**File:** `PUOrderBase.h`

| Field | Type | Description |
|-------|------|-------------|
| `AspectType` | `EOrderAspectType` | `Flavor` or `Texture`. |
| `FlavorAspect` / `TextureAspect` | `EPUFlavorAspect` / `EPUTextureAspect` | Enum selectors (defined with ingredients — see **`DishCustomization.md`**). |
| `TargetValue` | `float` | Target aspect total; scoring uses player vs target ratio (see struct comments). |
| `AspectName` | `FName` | Synced name for APIs / hints. |

---

## Struct API: `FPUOrderBase`

**Parent:** `FTableRowBase` (can be used as data table rows)

| Field | Type | Description |
|-------|------|-------------|
| `OrderID` | `FName` | Identifier. |
| `OrderGiverParticipantName` | `FName` | Scopes dialogue conditions (**HasActiveOrder**, etc.) per NPC. |
| `OrderDescription` | `FText` | Player-facing summary. |
| `MinIngredientCount` | `int32` | Minimum ingredients. |
| `TargetAspects` | `TArray<FOrderAspectRequirement>` | Required aspects (all must be met for validation). |
| `DiscoveredHints` | `TArray<FOrderAspectRequirement>` | Subset revealed in dialogue — used for partial radar display (**`PURadarChart`** — **`Scorecard.md`**). |
| `OrderDialogueText` | `FText` | Dialogue-specific copy. |
| `BaseDish` | `FPUDishBase` | Recipe / template for the order. |
| `CompletedDish` | `FPUDishBase` | What the player submitted. |
| `FinalSatisfactionScore` | `float` | Set when order completes; drives scorecard seal (**`Scorecard.md`**). |

**Methods:** `ValidateDish`, `GetSatisfactionScore`, `GetOrderDisplayText`, `GetCompletedDish`, `GetFinalSatisfactionScore`, `IsCompleted`, logging helpers.

---

## Class API: `UPUOrderComponent`

**Parent:** `USceneComponent` — lives on **`APUDishGiver`** (and similar).

| Method | Description |
|--------|-------------|
| `GenerateNewOrder` | Random/ pool dish tag + `GenerateSimpleOrder`. |
| `GenerateNewOrderWithDish` | Specific **`FGameplayTag`**. |
| `ClearCurrentOrder` | Resets component order. |
| `GetCurrentOrder` / `HasActiveOrder` | Access state. |
| `ValidateDish` / `GetSatisfactionScore` | Delegate to struct logic. |

**Delegates:** `OnOrderGenerated`, `OnOrderCompleted`

**Config:** `AvailableDishTags`, `DefaultMinIngredients`, `DefaultTargetAspects`, `DishDataTable`, `IngredientDataTable`, `PreparationDataTable`

---

## Class API: `APUDishGiver`

**Parent:** `ATalkingObject`

Owns **`UPUOrderComponent`**. Dialogue-driven methods include **`GenerateAndGiveOrderToPlayer`**, **`GenerateAndGiveOrderToPlayerWithDish`**, **`RevealHintToPlayer`**, **`HandleOrderCompletion`**, **`ClearCompletedOrderFromPlayer`**, order **conditions** in **`CheckCondition_Implementation`**. See **`Dialogue.md`** for event names.

---

## Blueprint library: `UPUOrderBlueprintLibrary`

| Function | Description |
|----------|-------------|
| `ValidateDish` / `GetSatisfactionScore` | Static order vs dish. |
| `GetOrderDisplayText` | UI string. |
| `CreateSimpleOrder` | Build a minimal **`FPUOrderBase`**. |
| `LogOrderDetails` / `LogValidationResults` | Debug. |

---

## Player character

**`AProjectUmeowmiCharacter`** stores **`CurrentOrder`**, completion flags, and routes **`ShowScorecard`**. See **`PlayerCharacter.md`**.

---

## Cross-references

| Topic | Doc |
|-------|-----|
| Dish and ingredient structs | `DishCustomization.md` |
| Dialogue events (`GenerateAndGiveOrderToPlayer`, `RevealHint_*`) | `Dialogue.md` |
| Scorecard data from order | `Scorecard.md` |
| Game instance saved order across levels | `GameInstance.md`, `LevelTransitions.md`, `SaveLoad.md` |

---

## Document change log

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0.0 | 2026-03-25 | Documentation | Initial orders system documentation for ProjectUmeowmi. |
