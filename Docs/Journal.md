# Journal — Developer Documentation

**Module:** `ProjectUmeowmi`  
**Primary source:** `Source/ProjectUmeowmi/UI/PUJournalWidget.*`, `PUJournalTabListWidget.*`, `PUJournalTypes.h`, `PUJournalSectionWidget.h`, section widgets (`PURecipesSectionWidget`, …)

The **journal** is the in-game recipe book: **vertical tabs** (Common UI) drive a **`UCommonActivatableWidgetSwitcher`** of section pages. **Recipes** is the primary implemented section; **Ingredients, People, Town** are thin subclasses ready for layout. **Unlocked dishes** and **current dish** come from **`UPUProjectUmeowmiGameInstance`** (see **`SaveLoad.md`**). This doc maps **widgets and flow**; it does not duplicate **`FPUDishBase`** or full customization APIs — see **`DishCustomization.md`**.

---

## Architecture

```
AProjectUmeowmiCharacter  →  UPUJournalWidget (Open/Close, CycleRecipesDish)
        ↑                           ↓
   OpenJournalAction            UPUJournalTabListWidget  ↔  ContentSwitcher
   JournalCycleDish*                   ↓
                               Section widgets (UPUJournalSectionWidget subclasses)
```

---

## Types: `EJournalSectionType` & `JournalTabNames`

**File:** `PUJournalTypes.h`

**`EJournalSectionType`:** `Recipes`, `Ingredients`, `People`, `Town`, `Settings`.

**`JournalTabNames` namespace** — `FName` ids for **`UCommonTabListWidgetBase`**: `Recipes`, `Ingredients`, `People`, `Town`, `Settings` (string names match enum intent).

---

## Class API: `UPUJournalWidget`

**Parent:** `UPUCommonUserWidget` (see **`UI.md`**)

**Description**  
Root journal UI: binds **`TabButtonsContainer`**, **`TabList`**, **`ContentSwitcher`**. **`NativeConstruct`** calls **`RegisterJournalTabs`**, which wires **`UPUJournalTabListWidget`** to the switcher, creates section widgets from configured classes, and registers tabs in order (**Recipes** first = index 0 for **`GetRecipesSection`**).

**Key methods**

| Method | Description |
|--------|-------------|
| `OpenJournal` | Shows widget; restores **`LastSelectedTabID`** if **`bRestoreLastTabOnOpen`**, else selects **Recipes**. |
| `CloseJournal` | Saves active tab to **`LastSelectedTabID`**, collapses visibility. |
| `SwitchToSection` | Maps **`EJournalSectionType`** → tab id, **`SelectTabByID`**. |
| `GetActiveSection` | From tab list active id. |
| `CycleRecipesDish(Direction)` | Only if active section is **Recipes**; calls **`UPUProjectUmeowmiGameInstance::CycleJournalDish`**, then **`UPURecipesSectionWidget::DisplayDishByTag`**. |
| `GetTabList` / `GetContentSwitcher` | Accessors. |
| `GetTabSlotPadding` | Padding used when building tabs (from **`TabSlotPadding`**). |

**Properties (selected)**

| Property | Type | Description |
|----------|------|-------------|
| `TabButtonsContainer` | `UVerticalBox*` | BindWidget — vertical strip for tab buttons. |
| `TabList` | `UPUJournalTabListWidget*` | BindWidget — tab controller. |
| `ContentSwitcher` | `UCommonActivatableWidgetSwitcher*` | BindWidget — page content. |
| `RecipesSectionClass` … `SettingsSectionClass` | `TSubclassOf<UUserWidget>` | Section widget classes; null class **skips** that tab. |
| `TabButtonClass` | `TSubclassOf<UCommonButtonBase>` | Tab chrome; **required** for **`RegisterJournalTabs`**. |
| `TabButtonLabelWidgetName` | `FName` | Optional text block name for tab labels. |
| `TabSlotPadding` | `FMargin` | Gap between tab buttons. |
| `bRestoreLastTabOnOpen` | `bool` | Remember last tab between opens. |
| `LastSelectedTabID` | `FName` | Transient persistence. |
| `SectionWidgets` | `TArray<UUserWidget*>` | Created sections (Recipes at index 0). |

**Notes**  
- **`ContentSwitcher`** has transition animation **disabled** for instant tab switches.  
- Default **`LastSelectedTabID`** in ctor: **`JournalTabNames::Recipes`**.

---

## Class API: `UPUJournalTabListWidget`

**Parent:** `UCommonTabListWidgetBase`

**Description**  
Adds tab buttons into the parent’s **`TabButtonsContainer`** via **`HandleTabCreation` / `HandleTabRemoval`**. Call **`SetTabButtonsContainer`** and **`SetOwningJournal`** from the journal so padding is read from the live **`UPUJournalWidget`**.

---

## Class API: `UPUJournalSectionWidget`

**Parent:** `UCommonActivatableWidget`

**Description**  
Abstract base for each switcher page. **`SectionType`** is set per Blueprint subclass. **`NativeOnActivated` / `NativeOnDeactivated`** forward to **`OnSectionActivated`** / **`OnSectionDeactivated`** BlueprintNativeEvents for lazy refresh.

---

## Section widgets

| Class | Purpose | Notes |
|-------|---------|--------|
| **`UPURecipesSectionWidget`** | Two-page recipe layout; **`DisplayDishByTag`**, **`PopulateIngredientsList`**, **`SetRecipeIllustration`** (loads soft **`JournalTexture`**). | Default C++ **`DisplayDishByTag_Implementation`**; override in BP for layout. Optional **`IngredientsContainer`**, **`RecipeIllustrationImage`**. |
| **`UPUIngredientsSectionWidget`** | Ingredient collection UI. | Stub — extend in Blueprint. |
| **`UPUPeopleSectionWidget`** | NPC / preferences. | Stub. |
| **`UPUTownSectionWidget`** | Town / locations. | Stub. |
| **`UPUSettingsSectionWidget`** | Options. | Stub. |

**Helper:** **`UPURecipeIngredientEntryWidget`** — per-line ingredient row for **`PopulateIngredientsList`**.

---

## Game instance integration

**Class:** `UPUProjectUmeowmiGameInstance`

| API | Role |
|-----|------|
| `UnlockedDishTags` / `UnlockDish` / `GetOrderedUnlockedDishTags` | Save-backed unlocks (**`SaveLoad.md`**). |
| `CurrentDishTag` / `SetCurrentDishTag` / `ClearCurrentDishTag` | Dish to show first when opening journal during customization. |
| `CycleJournalDish(Direction)` | Returns next/previous **`FGameplayTag`** in unlocked list (sorted). |
| `GetDishDataForTag` | Resolves **`FPUDishBase`** from **`DishDataTable`** (+ ingredient table as implemented). |

**`UPUJournalWidget::CycleRecipesDish`** depends on **`CycleJournalDish`** and **`DisplayDishByTag`**.

---

## Player character integration

| Item | Description |
|------|-------------|
| `JournalWidget` | Optional direct reference to **`UPUJournalWidget`**; if null, **`ToggleJournal`** searches widgets of class **`UPUJournalWidget`**. |
| `OpenJournalAction` | Toggles open/close (**`OpenJournal`** / **`CloseJournal`**). |
| `JournalCycleDishPrevAction` / `JournalCycleDishNextAction` | When journal **visible**, call **`CycleRecipesDish(-1)`** / **`CycleRecipesDish(1)`** (bumpers). |

Some input paths (e.g. customization) only forward bumper cycling when the journal is open and visible — see **`ProjectUmeowmiCharacter.cpp`** for exact conditions.

---

## Cross-references

| Topic | Doc |
|-------|-----|
| Shared UI bases | `UI.md` |
| Dish data, meshes, journal textures | `DishCustomization.md` |
| Persisted unlocks | `SaveLoad.md` |
| `GetDishDataForTag` / recipe tables | `PUProjectUmeowmiGameInstance` header + `DishCustomization.md` |

---

## Document change log

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0.0 | 2025-03-25 | Documentation | Initial journal system documentation for ProjectUmeowmi. |
