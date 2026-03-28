# Scorecard & dish scoring — Developer Documentation

**Module:** `ProjectUmeowmi`  
**Primary source:** `Source/ProjectUmeowmi/UI/PUScorecardWidget.*`, `PUScorecardTypes.h`, `PUDishScoringWidget.*`, `PUAspectProfileWidget.*`, `PURadarChart.*`, `ProjectUmeowmiCharacter` (scorecard / dish capture / scoring mode), `PUDishBlueprintLibrary::GetScorecardData`

This doc covers the **post-order scorecard UI**, the **data structs** that feed it, and the **dish scoring mode** shell (root widget + viewport stack + dialogue swap). It does **not** repeat dialogue event tables — see **`Dialogue.md`**. For dish/order structs and blueprint helpers at large, see **`DishCustomization.md`**. Shared UI bases are in **`UI.md`**.

---

## Concepts

| Layer | Role |
|-------|------|
| **Scorecard** (`UPUScorecardWidget`) | Summary after delivery: seal tier, dish image, base ingredients, flavor/texture aspect rows. Data from **`FPUScorecardData`** (built by **`UPUDishBlueprintLibrary::GetScorecardData`** from the completed **`FPUOrderBase`**). |
| **Dish scoring mode** (`UPUDishScoringWidget`) | Optional full-screen (or layered) experience: animations, nested radar charts, etc. The character tracks an **active** scoring widget and may swap to a **scoring dialogue** box. |

These are **separate widgets**. Dialogue can show the scorecard via the **`ShowScorecard`** event, or enter dish scoring via **`BeginDishScoring`** / **`EndDishScoring`** — see **`Dialogue.md`**.

---

## Viewport Z-order (scoring stack)

Defined in `PUScorecardWidget.h` (**back → front**, higher Z draws on top and receives pointer hits first):

| Constant | Value | Role |
|----------|------|------|
| `PUScoringSceneViewportZOrder` | 50000 | **`UPUDishScoringWidget`** (dish scene / capture). |
| `PUScoringDialogueViewportZOrder` | 50001 | **`UPUDialogueBox`** — **all** dialogue boxes add to the viewport at this Z (same layer used during scoring and after). |
| `PUScorecardViewportZOrder` | 50002 | **`UPUScorecardWidget`** when added as a root viewport widget (scorecard above dialogue). |

Static accessors: **`GetDishScoringSceneViewportZOrder`**, **`GetScoringDialogueViewportZOrder`**, **`GetScorecardLayerViewportZOrder`**.

**Why the numbers matter:** If **`UPUDialogueBox`** used the default **`AddToViewport()`** Z (0), it would sit **below** the 50000–50002 stack. Any leftover dish/scorecard widget would **still receive mouse clicks** and block click-to-advance on dialogue. C++ now always adds **`UPUDialogueBox`** at **`PUScoringDialogueViewportZOrder`**.

The dish scoring root uses **`SelfHitTestInvisible`** so pointer input can pass through empty areas; interactive children remain hit-testable. See **`Dialogue.md`** for **`EndDishScoringMode`** cleanup (orphan sweep).

---

## Types: `PUScorecardTypes.h`

### `EPUScorecardSealTier`

| Value | Display |
|-------|---------|
| `Perfect` | A |
| `Great` | B |
| `Okay` | C |
| `NeedsImprovement` | F |

### `FPUBaseIngredientEntry`

| Property | Type | Description |
|----------|------|-------------|
| `DisplayName` | `FText` | Label for the row. |
| `PreviewTexture` | `UTexture2D*` | Optional icon. |
| `bObtained` | `bool` | Whether the player included this base ingredient (checkmark vs X on scorecard). |

### `FPUAspectRanking`

| Property | Type | Description |
|----------|------|-------------|
| `AspectName` | `FName` | Aspect id (e.g. flavor/texture axis). |
| `TopContributingIngredients` | `TArray<FPUBaseIngredientEntry>` | Up to three contributors with icons. |
| `TotalValue` | `float` | Aggregated value for the aspect. |
| `StarRating` | `int32` | 0–5 stars for this aspect. |

### `FPUAspectProfileData`

| Property | Type | Description |
|----------|------|-------------|
| `TopAspects` | `TArray<FPUAspectRanking>` | Top aspects for flavor or texture (scorecard creates one **`UPUAspectProfileWidget`** per ranking). |
| `StarRating` | `int32` | Overall profile stars (0–5). |

### `FPUScorecardData`

| Property | Type | Description |
|----------|------|-------------|
| `DisplayName` | `FText` | Title (typically completed dish display name). |
| `SealTier` | `EPUScorecardSealTier` | Grade seal. |
| `BaseIngredients` | `TArray<FPUBaseIngredientEntry>` | Recipe/base ingredient list with obtain status. |
| `FlavorProfile` | `FPUAspectProfileData` | Flavor aspects + stars. |
| `TextureProfile` | `FPUAspectProfileData` | Texture aspects + stars. |

---

## Building data: `GetScorecardData`

**`UPUDishBlueprintLibrary::GetScorecardData(const FPUOrderBase& Order)`** (`PUDishBlueprintLibrary.cpp`) fills **`FPUScorecardData`** from the order’s completed dish and base recipe:

- **`DisplayName`** — from the completed dish display name helper.
- **`SealTier`** — from **`Order.GetFinalSatisfactionScore()`** with thresholds: ≥0.875 Perfect, ≥0.625 Great, ≥0.375 Okay, else Needs Improvement.
- **`BaseIngredients`** — unique base ingredients from **`Order.BaseDish`** when it has instances; else from completed dish. **`bObtained`** checks the completed dish when using the recipe list.
- **`FlavorProfile` / `TextureProfile`** — built via internal **`BuildAspectProfileFromOrder`** (target aspects, star logic).

For API placement and related dish helpers, see **`DishCustomization.md`** (`UPUDishBlueprintLibrary`).

---

## Class API: `UPUScorecardWidget`

**Parent:** `UPUCommonUserWidget` — see **`UI.md`**

**Description**  
Renders **`FPUScorecardData`**: dish name, optional **`DishImage`** (material parameter **`DishRender`** on **`DishImageMaterial`**), seal image by tier, base-ingredient rows, and dynamically created **`UPUAspectProfileWidget`** instances in flavor/texture containers.

**Methods**

| Method | Description |
|--------|-------------|
| `AddToViewportScoringStack` | **`AddToViewport`** at **`PUScorecardViewportZOrder`** (no-op if widget is embedded under a parent — use visibility in Blueprint). |
| `SetScorecardData` | Assigns data and refreshes display. |
| `SetDishImage` | Sets override texture for the dish image. |
| `ShowFromOrder` | **`GetScorecardData(Order)`**, then **`SetDishImage`** if provided, **`UpdateDisplay`**. |
| `PlaySealAnimation` | Seal presentation hook. |
| `Close` | Broadcasts **`OnScorecardClosed`**. |

**Events:** **`OnScorecardClosed`**

**BindWidgetOptional (selected):** `DishNameText`, `DishImage`, `SealImage`, `BaseIngredientsContainer`, `FlavorProfileContainer`, `TextureProfileContainer`

**Defaults:** **`AspectProfileWidgetClass`**, seal textures per tier, optional checkmark/X textures, **`DishImageMaterial`** (UI material with **`DishRender`**).

---

## Character: `ShowScorecard` and dish capture

**`AProjectUmeowmiCharacter::ShowScorecard(UPUScorecardWidget*)`**

- Requires **`bCurrentOrderCompleted`** and a non-null widget; otherwise aborts.
- If the widget **has a parent** (embedded), only population runs — no **`AddToViewport`**.
- Else **`AddToViewport(PUScorecardViewportZOrder)`**.
- Dish texture priority (when dish capture is enabled): pending capture texture → station snapshot render target → async fallback capture from head **`DishPreview`** — then **`PopulateScorecardWidget`**, which calls **`ScorecardWidget->ShowFromOrder(CurrentOrder, OptionalDishTexture)`**.

Plating-station capture and scorecard snapshot behavior are tied to customization/plating flow — see **`DishCustomization.md`** (e.g. **`CaptureScorecardSnapshotFromPlatingStation`**).

---

## Class API: `UPUDishScoringWidget`

**Parent:** `UPUCommonUserWidget`

**Description**  
Root widget for **dish scoring mode**. The character calls **`BeginDishScoringModeWithWidget`**, which sets the owner, adds the widget at **`PUScoringSceneViewportZOrder`**, and calls **`OnEnteredDishScoringMode`**. **`RequestEndDishScoringMode`** ends mode on the owner (**`EndDishScoringMode`**).

**Methods**

| Method | Description |
|--------|-------------|
| `GetDishScoringOwnerCharacter` | Owning **`AProjectUmeowmiCharacter`**. |
| `RequestEndDishScoringMode` | Calls **`EndDishScoringMode`** on owner. |
| `OnEnteredDishScoringMode` / `OnExitingDishScoringMode` | BlueprintNativeEvents. |

**Character integration:** **`BeginDishScoringModeWithWidget`** may call **`SwapToScoringDialogueBox`** (scoring **`UPUDialogueBox`** at **`PUScoringDialogueViewportZOrder`**). **`EndDishScoringMode`**:

- Tears down **elevated** embedded scorecard (**`ElevatedDishScoringScorecard`**) and the **`ShowScorecard`** dialogue-event scorecard (**`ActiveDialogueShowScorecardWidget`**) if still in the viewport.
- Removes the active **`UPUDishScoringWidget`** when **`ActiveDishScoringWidget`** is set.
- **`RemoveOrphanScoringStackViewportWidgets()`** — walks **`GetAllWidgetsOfClass`** for **`UPUDishScoringWidget`** and **`UPUScorecardWidget`** still **`IsInViewport()`** and **`RemoveFromParent`** (handles stray instances when the tracked pointer was already cleared).
- **`RestoreNonScoringDialogueBox`** and/or relayer **`AddToViewport(PUScoringDialogueViewportZOrder)`** (not Z 0) when **`bUsingScoringDialogueBox`** / **`bDialogueBoxRelayeredToScoringStackOnly`** — **independent of** whether **`ActiveDishScoringWidget`** is **null** (e.g. **`ShowScorecard`** / **`SyncDialogueBoxToScoringLayer`** without **`BeginDishScoring`**).
- **`RefreshDialogueBoxFromContext`** when **`GetCurrentTalkingObject()->GetCurrentDialogueContext()`** is valid (same independence from the dish widget pointer).

Then re-syncs the active dialogue node to the **restored** default dialogue widget when the player is still in range.

Swapping the dialogue box **replaces the widget instance** that **`Open`** was called on at dialogue start. The project therefore uses:

- **`AProjectUmeowmiCharacter::RefreshDialogueBoxFromContext(UDlgContext*)`** — **`Update`** on the current **`DialogueBox`** so text/options match **`UDlgContext`** (required after swap/restore mid-conversation).
- **`AProjectUmeowmiCharacter::SyncDialogueBoxToScoringLayer(UDlgContext*)`** — swaps to **`ScoringDialogueBoxWidgetClass`** when needed, then refresh (used by the **`ShowScorecard`** dialogue event so the scorecard and dialogue share the scoring stack).

Dialogue events **`BeginDishScoring`** and **`ShowScorecard`** call these from **`ATalkingObject`**; **`BeginDishScoring`** does **not** need to be on the first node — see **`Dialogue.md`**.

Default widget class: **`DishScoringWidgetClass`** on the character; dialogue can override via **`TalkingObject`** — **`Dialogue.md`**.

---

## Class API: `UPUAspectProfileWidget`

**Parent:** `UPUCommonUserWidget`

**Description**  
One **aspect row**: name, top contributing ingredients (optional panel), 0–5 stars (text or filled/unfilled textures), optional **`AspectBorder`** tint from **`AspectColorDataTable`** (row name = aspect name).

**Methods:** **`SetAspectData`**, **`GetAspectData`**

---

## Class API: `UPURadarChart`

**Parent:** **`URadarChart`** (Marketplace plugin — see Engine plugin `RadarChart`)

**Description**  
Project extension for ingredient/dish/order-driven **radar** values: segment names, icons, animated values, **dual-layer** hint vs player shapes (**`SetValuesFromOrderFlavorProfile`** / **`SetValuesFromOrderTextureProfile`** with **`DiscoveredHints`**), and **fluctuation** animations with **`OnFluctuationAnimationComplete`**.

Used from **dish scoring** Blueprint layouts (not wired in core C++ scorecard widget). Helpers: **`FindRadarChartInWidget`**, **`AreAllRadarChartsAnimationComplete`**.

---

## Cross-references

| Topic | Doc |
|-------|-----|
| `ShowScorecard`, `BeginDishScoring`, widget classes on `ATalkingObject` | `Dialogue.md` |
| `GetScorecardData`, orders, dishes, plating capture | `DishCustomization.md` |
| `UPUCommonUserWidget`, widget map | `UI.md` |

---

## Document change log

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0.0 | 2025-03-25 | Documentation | Initial scorecard and dish scoring documentation for ProjectUmeowmi. |
| 1.1.0 | 2026-03-26 | Documentation | Document **`RefreshDialogueBoxFromContext`**, **`SyncDialogueBoxToScoringLayer`**, post-**`EndDishScoringMode`** restore sync, and dialogue event behavior — see **`Dialogue.md`**. |
| 1.2.0 | 2026-03-26 | Documentation | **`EndDishScoringMode`** restore path: **`GetCurrentTalkingObject()->GetCurrentDialogueContext()`** (see **`Dialogue.md`**). |
| 1.3.0 | 2026-03-28 | Documentation | **Corrected** viewport Z table (50000 dish → 50001 dialogue → 50002 scorecard). Documented **`PUScoringDialogueViewportZOrder`** for all **`UPUDialogueBox`**, **`RemoveOrphanScoringStackViewportWidgets`**, tracked scorecard pointers, and **`EndDishScoringMode`** restore/refresh **without** requiring **`ActiveDishScoringWidget`**. |
