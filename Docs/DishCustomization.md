# Dish Customization System — Developer Documentation

**Module:** `ProjectUmeowmi`  
**Primary source:** `Source/ProjectUmeowmi/DishCustomization/`, `Source/ProjectUmeowmi/UI/PUDishCustomizationWidget.h`

Gameplay-tag-driven cooking flow: **planning** (pick ingredients) → **cooking** (quantities, prep, time/temp) → **plating** (3D placement, liquids, scorecard snapshot) → **ending** UI. Dish state is `FPUDishBase` with per-instance `FIngredientInstance` rows. Orders (`FPUOrderBase` / `UPUOrderComponent`) define targets and satisfaction scoring — see also **`Orders.md`** for the dish-giver loop and player copy. Data tables supply `FPUDishBase`, `FPUIngredientBase`, and `FPUPreparationBase` rows.

---

## System overview

| Piece | Role |
|-------|------|
| `UPUDishCustomizationComponent` | Scene component on stations: starts/ends customization, cameras, input, 3D plating meshes, syncs `CurrentDishData` with UI. Set **`bUse2DCustomizationMode`** on the station instance to skip spring-arm framing, station camera blends, mesh spawn/drag, and plating bowl swap; exit broadcasts **`OnCustomizationEnded`** immediately (see class properties). |
| `UPUDishCustomizationWidget` | Multi-stage UI (Planning / Cooking / Plating / Ending); slots, pantry, navigation. |
| `FPUDishBase` | Authoring + runtime dish: tags, meshes, `IngredientInstances`, `PlatingEntries`, aspect aggregation. |
| `FIngredientInstance` | One stack in the dish: `InstanceID`, quantity, ingredient data, prep tags, time/temp sliders, plating transforms. |
| `FPUIngredientBase` | Data table row: visuals, flavor/texture aspects, preparations, liquids (Niagara). |
| `FPUPreparationBase` | Data table row: aspect modifiers, naming, tags. |
| `FPUOrderBase` | Order template: `BaseDish`, aspect targets, validation, satisfaction, completion dish. |
| `UPUOrderComponent` | Generates orders from dish/ingredient/prep tables; validates and scores. |
| `APUIngredientMesh` | World actor for draggable plated ingredients (incl. chopped procedural pieces). |
| `UPUDishPreviewComponent` | Character-mounted 3D preview from plating data. |
| `UPUDishBlueprintLibrary` / `UPUOrderBlueprintLibrary` / `UPUIngredientBlueprintLibrary` | Blueprint-safe mutations and queries. |
| `APUCookingStation` | `TalkingObject` + `DishCustomizationComponent`; can auto-start customization when player has an order. |

---

## Class API: `UPUDishCustomizationComponent`

**Class Name:** `UPUDishCustomizationComponent`

**Description**  
Owns the active customization session for a `AProjectUmeowmiCharacter`: spawns UI, switches cooking vs plating cameras, handles Enhanced Input (exit, stage next/prev, mouse), spawns and tracks `APUIngredientMesh` / Niagara liquids for plating, captures transforms back into `FPUDishBase`, and broadcasts delegates when dish data or planning state changes. Hides HUD widgets during customization when configured.

**Namespace**  
N/A. **Feature area:** Dish customization (`DishCustomization/`).

**Inheritance**  
**Parent class:** `USceneComponent`  
**Interfaces:** None.

**Constructors**  
`UPUDishCustomizationComponent()` — default subobject construction; typically owned by `APUCookingStation` or similar.

**Properties — events**

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `OnCustomizationEnded` | `FOnCustomizationEnded` | BlueprintAssignable | Fired when customization session ends. |
| `OnDishDataUpdated` | `FOnDishDataUpdated` | BlueprintAssignable | Fired with new `FPUDishBase` when dish changes. |
| `OnInitialDishDataReceived` | `FOnInitialDishDataReceived` | BlueprintAssignable | Fired when initial dish (e.g. from order) is applied. |
| `OnPlanningCompleted` | `FOnPlanningCompleted` | BlueprintAssignable | Fired with `FPUPlanningData` when planning finishes. |

**Properties — UI & widget classes**

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `CustomizationWidgetClass` | `TSubclassOf<UUserWidget>` | EditDefaultsOnly, BlueprintReadOnly | Base widget to spawn for customization (often overridden per stage). |
| `HUDWidgetClass` | `TSubclassOf<UUserWidget>` | EditDefaultsOnly, BlueprintReadOnly | Optional explicit HUD class to hide during customization. |
| `HUDHiddenVisibility` | `ESlateVisibility` | EditDefaultsOnly, BlueprintReadOnly | Visibility applied to HUD while customizing. |
| `OriginalWidgetClass` | `TSubclassOf<UUserWidget>` | `UPROPERTY` | Stored when swapping to plating widget. |
| `CookingStageWidgetClass` | `TSubclassOf<UPUDishCustomizationWidget>` | EditAnywhere, BlueprintReadWrite | Widget for cooking stage. |
| `PlatingWidgetClass` | `TSubclassOf<UUserWidget>` | EditAnywhere, BlueprintReadWrite | Widget for plating stage. |

**Properties — input**

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `ExitCustomizationAction` | `UInputAction*` | EditDefaultsOnly, BlueprintReadOnly | Exit customization. |
| `ControllerMouseAction` | `UInputAction*` | EditDefaultsOnly, BlueprintReadOnly | Controller cursor movement. |
| `MouseClickAction` | `UInputAction*` | EditDefaultsOnly, BlueprintReadOnly | Click / grab for plating. |
| `NextStageAction` | `UInputAction*` | EditDefaultsOnly, BlueprintReadOnly | Advance stage. |
| `PreviousStageAction` | `UInputAction*` | EditDefaultsOnly, BlueprintReadOnly | Previous stage. |
| `CustomizationMappingContext` | `UInputMappingContext*` | EditDefaultsOnly, BlueprintReadOnly | IMC layered while customizing. |
| `ControllerMouseSensitivity` | `float` | EditDefaultsOnly, BlueprintReadOnly | Controller mouse speed. |
| `ControllerMouseDeadzone` | `float` | EditDefaultsOnly, BlueprintReadOnly | Stick deadzone for virtual mouse. |
| **`bUse2DCustomizationMode`** | **`bool`** | **EditAnywhere, BlueprintReadWrite** | **When true:** skips cooking/plating station cameras and blends, 3D ingredient spawning/drag, plating bowl mesh swap; **`EndCustomization`** clears meshes/restores bowl, clears GI dish tag, restores input/movement, and schedules **`OnCustomizationEnded`** for the next tick (no spring-arm customization enter/exit). |

**Properties — cameras (representative; all EditDefaultsOnly, BlueprintReadOnly)**  
Used only when **`bUse2DCustomizationMode`** is false (legacy station cameras).

| Property Name | Type | Description |
|---------------|------|-------------|
| `CookingCameraPitch` / `Yaw` / `OrthoWidth` | `float` | Cooking station ortho framing (`SwitchToCookingCamera`). |
| `CookingCameraPositionOffset` | `FVector` | Fine position tweak (left/right, forward/back, up/down). |
| `CookingStationCameraComponentName` | `FName` | Name of `UCameraComponent` on station (default `CookingCamera`). |
| `PlatingCameraDistance` / `Pitch` / `Yaw` / `OrthoWidth` | `float` | Plating station framing. |
| `PlatingCameraPositionOffset` | `FVector` | Plating camera offset. |
| `PlatingStationCameraComponentName` | `FName` | Default `PlatingCamera`. |

**Properties — data & state**

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `CurrentDishData` | `FPUDishBase` | EditAnywhere, BlueprintReadWrite | Authoritative dish during session. |
| `CurrentPlanningData` | `FPUPlanningData` | VisibleAnywhere, BlueprintReadOnly | Planning selections and target dish. |
| `bInPlanningMode` | `bool` | VisibleAnywhere, BlueprintReadOnly | True during planning UI phase. |
| `IngredientDataTable` | `UDataTable*` | EditAnywhere, BlueprintReadWrite | Ingredient rows (`FPUIngredientBase`). |
| `PreparationDataTable` | `UDataTable*` | EditAnywhere, BlueprintReadWrite | Preparation rows (`FPUPreparationBase`). |
| `PlatingDishMesh` | `TSoftObjectPtr<UStaticMesh>` | EditAnywhere, BlueprintReadWrite | Mesh swapped for plating container. |
| `IngredientMeshScale` | `FVector` | EditAnywhere, BlueprintReadWrite | Scale for spawned ingredient actors. |
| `IngredientSpawnHeightOffset` | `float` | EditAnywhere, BlueprintReadWrite | Z offset above plate when spawning meshes. |
| `IngredientMeshClass` | `TSubclassOf<APUIngredientMesh>` | EditAnywhere, BlueprintReadWrite | Class for 3D ingredients (materials set on class defaults). |
| `OriginalDishContainerMesh` | `UStaticMesh*` | `UPROPERTY` | Cached mesh before swap for restoration. |
| `OriginalDishContainerChildren` | `TArray<UStaticMesh*>` | `UPROPERTY` | Child meshes stored with container. |

**Properties — internal (selected)**  
| Property Name | Type | Description |
|---------------|------|-------------|
| `CustomizationWidget` | `UUserWidget*` | Active root customization widget instance. |
| `CookingStageWidget` | `UPUDishCustomizationWidget*` | Cooking widget instance. |
| `CurrentCharacter` | `AProjectUmeowmiCharacter*` | Player in customization (`IsCustomizing` when non-null). |
| `CookingStationCamera` / `PlatingStationCamera` | `UCameraComponent*` | Resolved camera components. |

**Methods (selected)**

| Method | Description | Parameters | Return |
|--------|-------------|------------|--------|
| `StartCustomization` | Begin session for character: HUD, widget, cameras, input. | `Character` (`AProjectUmeowmiCharacter*`) | `void` |
| `EndCustomization` | Tear down UI, cameras, input, spawned meshes. | — | `void` |
| `IsCustomizing` | Whether a character is in customization. | — | `bool` |
| `GetCurrentCharacter` | Current player or null. | — | `AProjectUmeowmiCharacter*` |
| `UpdateCurrentDishData` | Sets `CurrentDishData` and updates listeners. | `NewDishData` | `void` |
| `GetCurrentDishData` | Const reference to dish. | — | `const FPUDishBase&` |
| `SyncDishDataFromUI` | Push UI state into component dish. | `DishDataFromUI` | `void` |
| `SetWidgetComponentReference` / `SetDishCustomizationComponentOnWidget` | Bind widget ↔ component. | Widget | `void` |
| `SetActiveCustomizationWidget` | Track widget for stage navigation. | `ActiveWidget` | `void` |
| `SetInitialDishData` | Seed dish from order or recipe. | `InitialDishData` | `void` |
| `SetDataTables` | Assign dish / ingredient / preparation tables. | Three `UDataTable*` | `void` |
| `GetIngredientData` / `GetPreparationData` | Table rows as arrays for UI. | — | `TArray<...>` |
| `SpawnIngredientIn3D` / `SpawnIngredientIn3DByInstanceID` | Spawn world mesh at position. | Tag or `InstanceID`, `WorldPosition` | `void` |
| `GetSpawnPositionAboveStation` | Spawn point above pan/plate. | — | `FVector` |
| `SetPlatingMode` / `IsPlatingMode` / `CanSpawnIngredientsIn3D` | Plating stage gating. | — | `void` / `bool` |
| `TransitionToPlatingStage` / `EndPlatingStage` | Enter/leave plating; snapshot scorecard as needed. | `DishData` where relevant | `void` |
| `CapturePlatingTransformsFromMeshes` | Write mesh transforms into `PlatingEntries`. | — | `void` |
| `CaptureScorecardSnapshotFromPlatingStation` | RT capture for scorecard while meshes exist. | — | `void` |
| `GatherDishSnapshotPrimitives` | Primitives for scene capture. | `OutPrimitives` | `void` |
| `GetPlatingStationCamera` | Plating camera for framing. | — | `UCameraComponent*` |
| `ClearAll3DIngredientMeshes` | Destroy spawned actors / Niagara. | — | `void` |
| `StartDraggingIngredient` | Begin drag on `APUIngredientMesh`. | `Ingredient` | `void` |
| `SwitchToCookingCamera` / `SwitchToPlatingCamera` | Camera mode. | — | `void` |
| `SetCookingCameraPositionOffset` | Adjust cooking offset at runtime. | `NewOffset` | `void` |
| `ResetPlatingPlacements` | Reset plating placement state. | — | `void` |
| `SwapDishContainerMesh` / `RestoreOriginalDishContainerMesh` | Mesh swap for plating bowl/plate. | `NewDishMesh` | `void` |
| `StartPlanningMode` | Enter planning. | — | `void` |
| `TransitionToCookingStage` | Leave planning with dish data. | `DishData` | `void` |
| `IsInPlanningMode` | Planning flag. | — | `bool` |
| `BroadcastDishDataUpdate` / `BroadcastInitialDishData` | Manual delegate broadcast. | `FPUDishBase` | `void` |
| `CanPlaceIngredientByTag` / `GetRemainingQuantityByTag` / `GetPlacedQuantityByTag` | Plating limits by tag. | Tags / IDs | `bool` / `int32` |

**Events**  
See table above (`OnCustomizationEnded`, `OnDishDataUpdated`, `OnInitialDishDataReceived`, `OnPlanningCompleted`).

**Usage example (conceptual)**  
Cooking station calls `DishCustomizationComponent->StartCustomization(Player)` after resolving order; UI calls `SyncDishDataFromUI` or `UpdateCurrentDishData` as the player edits; `EndCustomization` returns control to gameplay.

**Dependencies**  
`AProjectUmeowmiCharacter`, `UPUDishCustomizationWidget`, `APUIngredientMesh`, `UDataTable`, Enhanced Input, optional Niagara for liquids.

**Notes**  
- `EnsureDishIngredientsInPantry` (private) unlocks pantry entries for ingredients present on the incoming dish.  
- Slate pre-input mouse path supports clicks when widgets consume events.  
- Weak pointers track spawned meshes because actors may self-destroy (e.g. `GroundDestroyZThreshold`).

---

## Struct API: `FPUDishBase`

**Parent:** `FTableRowBase` (data table row for recipes).

**Description**  
Canonical dish: identity tags, display strings, soft textures/meshes, optional per-dish ingredient table reference, `IngredientInstances`, and `PlatingEntries` for saved 3D layout.

**Properties (selected)**

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `DishTag` | `FGameplayTag` | EditAnywhere, BlueprintReadWrite | Primary dish tag (`Dish` category). |
| `DishName` | `FName` | EditAnywhere, BlueprintReadWrite | Row / legacy name. |
| `DisplayName` | `FText` | EditAnywhere, BlueprintReadWrite | UI name. |
| `Description` | `FText` | EditAnywhere, BlueprintReadWrite | Journal / flavor text. |
| `PreviewTexture` / `JournalTexture` | `TSoftObjectPtr<UTexture2D>` | EditAnywhere, BlueprintReadWrite | UI art (load via blueprint library helpers). |
| `DishMesh` | `TSoftObjectPtr<UStaticMesh>` | EditAnywhere, BlueprintReadWrite | Container mesh for plating. |
| `IngredientDataTable` | `TSoftObjectPtr<UDataTable>` | EditAnywhere, BlueprintReadWrite | Optional per-dish ingredient source. |
| `IngredientInstances` | `TArray<FIngredientInstance>` | EditAnywhere, BlueprintReadWrite | All stacks in the dish. |
| `PlatingEntries` | `TArray<FPUPlatingEntry>` | EditAnywhere, BlueprintReadWrite | Per-mesh or liquid entry on plate. |
| `PlatingDishCenter` | `FVector` | EditAnywhere, BlueprintReadWrite | World origin used when replaying layout. |
| `DishTags` | `FGameplayTagContainer` | EditAnywhere, BlueprintReadWrite | Extra tags. |
| `CustomName` | `FText` | EditAnywhere, BlueprintReadWrite | Player-facing rename. |

**Methods (selected)**  
`GetTotalFlavorAspect` / `GetTotalTextureAspect`, `HasIngredient`, `GetCurrentDisplayName`, `GetIngredient` / `GetIngredientForInstance` / `GetIngredientForInstanceID`, `GetAllIngredients`, `GetAllIngredientInstances`, `GenerateNewInstanceID` / `GenerateUniqueInstanceID`, plating getters/setters, aspect aggregation across instances.

---

## Struct API: `FIngredientInstance`

**Description**  
One logical ingredient stack in a dish with stable `InstanceID`, quantity, embedded `FPUIngredientBase` (with active preparations), time/temp normalized values, and placement/plating transforms.

**Properties**

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `InstanceID` | `int32` | EditAnywhere, BlueprintReadWrite | Stable id for UI and 3D mapping. |
| `Quantity` | `int32` | EditAnywhere, BlueprintReadWrite | Stack count. |
| `IngredientData` | `FPUIngredientBase` | EditAnywhere, BlueprintReadWrite | Snapshot with preparations. |
| `IngredientTag` | `FGameplayTag` | EditAnywhere, BlueprintReadWrite | Redundant tag for templates. |
| `Preparations` | `FGameplayTagContainer` | EditAnywhere, BlueprintReadWrite | Redundant prep tags for templates. |
| `PlacementPosition` / `PlacementRotation` | `FVector` / `FRotator` | EditAnywhere, BlueprintReadWrite | Optional layout metadata. |
| `PlatingPosition` / `PlatingRotation` / `PlatingScale` | `FVector` / `FRotator` / `FVector` | EditAnywhere, BlueprintReadWrite | Saved plating pose. |
| `bIsPlated` | `bool` | EditAnywhere, BlueprintReadWrite | Whether placed on plate. |
| `TimeValue` / `TemperatureValue` | `float` | EditAnywhere, BlueprintReadWrite | 0–1 sliders mapped to discrete cook states. |

---

## Struct API: `FPUPlatingEntry`

**Description**  
One visual element on the plate: mesh or liquid (`bIsLiquid`), optional `ChoppedPieceTransforms` for procedural slices.

| Property Name | Type | Description |
|---------------|------|-------------|
| `InstanceID` | `int32` | Links to `FIngredientInstance`. |
| `Position` / `Rotation` / `Scale` | `FVector` / `FRotator` / `FVector` | World-space pose. |
| `bIsLiquid` | `bool` | Niagara vs static mesh. |
| `ChoppedPieceTransforms` | `TArray<FTransform>` | Per-piece poses for chopped/minced. |

---

## Struct API: `FPUPlanningData`

| Property Name | Type | Description |
|---------------|------|-------------|
| `SelectedIngredients` | `TArray<FPUIngredientBase>` | Chosen ingredients before quantities. |
| `TargetDish` | `FPUDishBase` | Dish being planned toward. |
| `bPlanningCompleted` | `bool` | Planning finished flag. |

---

## Struct API: `FPUIngredientBase`

**Parent:** `FTableRowBase`.

**Description**  
Ingredient definition: tags, textures, meshes, `FFlavorAspects` / `FTextureAspects` (0–5), min/max/current quantity, default placements, `ActivePreparations`, optional liquid Niagara, time/temp modifier tables, special effects by quantity.

**Enums:** `ETimeState`, `ETemperatureState`, `EPUFlavorAspect`, `EPUTextureAspect`, `EPAspectCategory` — used with `FTimeTempModifier` and aspect APIs.

**Key property groups:** Basic id, visual (incl. `bIsLiquid`, `LiquidParticleSystem`, `CapMaterialInstance`, `MeshScale`, `AverageTintColor`), aspects, quantity, placement, preparations, data table refs, `TimeTemperatureModifiers` + `bUseCustomTimeTempModifiers`.

**Methods (selected):** `Get/SetFlavorAspect`, `Get/SetTextureAspect`, `ApplyPreparation` / `RemovePreparation` / `HasPreparation`, `CalculateTimeTempModifiedAspects`, `GetModifiedFlavorAspects` / `GetModifiedTextureAspects`, `MapTimeValueToState` / `MapTemperatureValueToState`.

---

## Struct API: `FPUPreparationBase`

**Parent:** `FTableRowBase`.

**Description**  
Named preparation with icons, optional name prefix/suffix/override, `FAspectModifier` list (additive/multiplicative), required/incompatible tags, and special effect tags.

**Related:** `FAspectModifier` (`EAspectType`, `EModificationType`, enum aspect pickers, `ModificationValue`).

---

## Struct API: `FPUOrderBase` & `FOrderAspectRequirement`

**FOrderAspectRequirement**  
Flavor or texture target via enums, `TargetValue`, and `AspectName` for Blueprint breaks. `GetAspectName()` returns `FName` for scoring.

**FPUOrderBase**  
| Property Name | Type | Description |
|---------------|------|-------------|
| `OrderID` | `FName` | Identifier. |
| `OrderGiverParticipantName` | `FName` | Scopes dialogue conditions per NPC. |
| `OrderDescription` | `FText` | Player-facing text. |
| `MinIngredientCount` | `int32` | Minimum ingredients. |
| `TargetAspects` | `TArray<FOrderAspectRequirement>` | All must be in ratio band for satisfaction. |
| `DiscoveredHints` | `TArray<FOrderAspectRequirement>` | Partial radar hints. |
| `OrderDialogueText` | `FText` | Dialogue hook. |
| `BaseDish` | `FPUDishBase` | Template dish for the order. |
| `CompletedDish` | `FPUDishBase` | Filled when player finishes. |
| `FinalSatisfactionScore` | `float` | Set on completion (>0 implies completed). |

**Methods:** `ValidateDish`, `GetSatisfactionScore`, `GetOrderDisplayText`, `IsCompleted`, getters for completed dish/score.

---

## Class API: `UPUOrderComponent`

**Parent:** `USceneComponent` (e.g. on dish giver).

**Description**  
Builds `FPUOrderBase` from data tables and defaults; exposes validation and satisfaction; broadcasts `OnOrderGenerated` and `OnOrderCompleted`.

**Properties**

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `OnOrderGenerated` | `FOnOrderGenerated` | BlueprintAssignable | New order ready. |
| `OnOrderCompleted` | `FOnOrderCompleted` | BlueprintAssignable | Order completed with final data. |
| `AvailableDishTags` | `TArray<FGameplayTag>` | EditDefaultsOnly, BlueprintReadOnly | Pool for random orders. |
| `DefaultMinIngredients` | `int32` | EditDefaultsOnly, BlueprintReadOnly | Default min ingredient count. |
| `DefaultTargetAspects` | `TArray<FOrderAspectRequirement>` | EditDefaultsOnly, BlueprintReadOnly | Default aspect lines. |
| `DefaultOrderDescription` | `FText` | EditDefaultsOnly, BlueprintReadOnly | Template string for generated copy. |
| `DishDataTable` / `IngredientDataTable` / `PreparationDataTable` | `UDataTable*` | EditDefaultsOnly, BlueprintReadOnly | Generation sources. |
| `CurrentOrder` | `FPUOrderBase` | BlueprintReadOnly, protected | Active order payload. |
| `bHasActiveOrder` | `bool` | BlueprintReadOnly, protected | Whether an order is active. |

**Methods:** `GenerateNewOrder`, `GenerateNewOrderWithDish`, `ClearCurrentOrder`, `GetCurrentOrder`, `HasActiveOrder`, `ValidateDish`, `GetSatisfactionScore`.

---

## Class API: `UPUDishPreviewComponent`

**Parent:** `USceneComponent`.

**Description**  
Builds a non-physics replica of a plated dish above the character for feedback; uses `PlatingEntries` and optional `DefaultDishMesh`.

**Properties**

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `PreviewScale` | `float` | EditAnywhere, BlueprintReadWrite | Overall scale (e.g. 0.2 for above-head). |
| `OffsetAboveHeadZ` | `float` | EditAnywhere, BlueprintReadWrite | Height in character space. |
| `IngredientZOffset` | `float` | EditAnywhere, BlueprintReadWrite | Fine-tune ingredient Z vs plate. |
| `DefaultDishMesh` | `TSoftObjectPtr<UStaticMesh>` | EditAnywhere, BlueprintReadWrite | Fallback plate mesh. |
| `bEnableDishPreviewDebug` | `bool` | EditAnywhere, BlueprintReadWrite | Verbose logging. |
| `IngredientMeshClass` | `TSubclassOf<APUIngredientMesh>` | EditAnywhere, BlueprintReadWrite | Ingredient actor class. |
| `DishMeshComponent` | `UStaticMeshComponent*` | VisibleAnywhere, BlueprintReadOnly | Plate mesh component. |

**Methods:** `SetDishMeshComponent`, `BuildFromDishData`, `ClearPreview`, `HasPreview`, `ComputePreviewWorldBounds`, `GatherSnapshotPrimitives`.

---

## Class API: `APUIngredientMesh`

**Parent:** `AActor`.

**Description**  
Interactive plated ingredient: static mesh or procedural chopped pieces, hover/grab materials, drag callbacks, `PlatingInstanceID` for capture, optional ground destroy Z.

**Events:** `OnIngredientMoved`, `OnIngredientRotated`, `OnIngredientGrabbed`, `OnIngredientReleased`.

**Key methods:** `InitializeWithIngredient`, `InitializeWithIngredientInstance`, `IsChopped`, `SetIngredientScale`, `SetPlatingInstanceID`, `GetChoppedPieceWorldTransforms`, `ApplyChoppedPieceTransforms`, `GatherSnapshotPrimitiveComponents`.

---

## Class API: `APUDish`

**Parent:** `AActor`.

**Description**  
Lightweight actor wrapper around `FPUDishBase` with Blueprint events on add/remove ingredient. Prefer `FPUDishBase` for persistence and UI; use this for placed actors if needed.

**Properties:** `DishData` (`FPUDishBase`, EditAnywhere, BlueprintReadOnly on actor).

---

## Blueprint function libraries (summary)

**`UPUDishBlueprintLibrary`** — Mutates `FPUDishBase`: add/remove instances, quantities, preparations by index or ID, aspect totals, data table fetch, random dish tag, display name, plating helpers, `GetLoadedJournalTexture` / `GetLoadedPreviewTexture`, `IsDishSuspicious`, `GetEndingStageText`, `GetScorecardData` (`FPUScorecardData` from order — UI and structs: **`Scorecard.md`**).

**`UPUOrderBlueprintLibrary`** — `ValidateDish`, `GetSatisfactionScore`, logging helpers, `GetOrderDisplayText`, `CreateSimpleOrder`.

**`UPUIngredientBlueprintLibrary`** — Preparation apply/remove, aspect get/set, totals, effects at quantity.

---

## Class API: `UPUDishCustomizationWidget` (summary)

**Parent:** `UUserWidget`.

**Description**  
Stage-based UI (`EDishCustomizationStageType`: Planning, Cooking, Plating, Ending). Subscribes to `UPUDishCustomizationComponent` delegates, owns `CurrentDishData`, drives slot creation (`CreateSlots`, `CreateSlotsFromDishData`), pantry vs prep vs plating containers, planning toggles, navigation (`GoToStage`, `GoToNextStage`, `GoToPreviousStage`), and ending copy via `GetEndingStageTextForCurrentDish`.

**Key methods:** `SetCustomizationComponent`, `OnInitialDishDataReceived`, `OnDishDataUpdated`, `UpdateDishData`, `EndCustomizationFromUI`, `CreateIngredientButtons` / `CreateIngredientSlots` / `CreatePlatingIngredientSlots`, planning `ToggleIngredientSelection` / `FinishPlanningAndStartCooking`, quantity controls, popup/dialogue focus restore hooks.

**Note:** Full bind-widget layout and stage-specific logic live in Blueprint subclasses; see header for additional categories (order radar, tutorial, etc.).

---

## Integration: `APUCookingStation`

- **Components:** `StationMesh`, `InteractionBox`, `DishCustomizationComponent`.  
- **Orders:** `bStartCustomizationImmediatelyWhenHasOrder` — if true and player has an active order, interaction can skip dialogue and open customization.  
- **Data tables:** `DishDataTable`, `IngredientDataTable`, `PreparationDataTable` — passed into customization / order generation.  
- **Helpers:** `StartCustomizationFromCurrentOrder`, `OnCustomizationEnded`, `EndDialogueOnly`.

---

## Document change log

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0.0 | 2025-03-25 | Documentation | Initial dish customization system API documentation for ProjectUmeowmi. |
| 1.1.0 | 2025-03-25 | Documentation | Cross-reference to **`Scorecard.md`** from `GetScorecardData` in blueprint library summary. |
| 1.2.0 | 2026-03-25 | Documentation | Overview cross-reference to **`Orders.md`**. |
