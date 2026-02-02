# Preparation Checkbox System Setup

This document explains how to set up and use the preparation checkbox system for ingredient quantity controls.

## Overview

The preparation system allows players to apply various preparations (like "chopped", "cooked", "seasoned") to ingredients. These preparations are displayed as checkboxes in the ingredient quantity control widgets.

## Components

### 1. PUPreparationCheckbox Widget
- **File**: `PUPreparationCheckbox.h/cpp`
- **Purpose**: Individual checkbox widget for a single preparation option
- **UI Components**:
  - `PreparationCheckBox` (UCheckBox) - The actual checkbox
  - `PreparationNameText` (UTextBlock) - Display name of the preparation
  - `PreparationIcon` (UImage) - Optional icon for the preparation

### 2. PUIngredientQuantityControl Widget
- **File**: `PUIngredientQuantityControl.h/cpp`
- **Purpose**: Main quantity control widget that now includes preparation checkboxes
- **New Components**:
  - `PreparationsScrollBox` (UScrollBox) - Container for preparation checkboxes
  - `PreparationCheckboxClass` (TSubclassOf) - Reference to the checkbox widget class

## Setup Instructions

### 1. Create the Preparation Checkbox Blueprint Widget

1. In the Unreal Editor, create a new Widget Blueprint
2. Set the parent class to `PUPreparationCheckbox`
3. Add the following UI components:
   - **CheckBox** named `PreparationCheckBox`
   - **TextBlock** named `PreparationNameText`
   - **Image** named `PreparationIcon` (optional)
4. Bind these components in the Blueprint's "Bind Widget" section

### 2. Update the Quantity Control Blueprint Widget

1. Open your existing quantity control widget Blueprint
2. Add a **ScrollBox** named `PreparationsScrollBox` where you want the preparation checkboxes to appear
3. Bind the `PreparationsScrollBox` component
4. Set the `PreparationCheckboxClass` property to reference your preparation checkbox Blueprint

### 3. Update the Dish Customization Widget

1. Open your dish customization widget Blueprint
2. Set the `PreparationCheckboxClass` property to reference your preparation checkbox Blueprint
3. In the Blueprint event `OnQuantityControlCreated`, call `SetPreparationCheckboxClass` on the quantity control widget

### 4. Set Up Preparation Data Table

1. Create a Data Table with the row structure `FPUPreparationBase`
2. Add preparation entries with:
   - **PreparationTag**: Gameplay tag (e.g., "Prep.Chopped", "Prep.Cooked")
   - **DisplayName**: Human-readable name (e.g., "Chopped", "Cooked")
   - **Description**: Optional description
   - **IconTexture**: Optional icon texture
   - **PrepTexture**: Optional preparation texture
   - **NamePrefix/NameSuffix**: How the preparation affects ingredient names
   - **PropertyModifiers**: How the preparation affects ingredient properties

### 5. Link Preparation Data to Ingredients

1. In your ingredient data table, set the `PreparationDataTable` field to reference your preparation data table
2. This tells the system which preparations are available for each ingredient

## How It Works

1. **Ingredient Added**: When an ingredient is added to a dish, the quantity control widget is created
2. **Preparation Checkboxes Created**: The widget queries the ingredient's preparation data table and creates checkboxes for each available preparation
3. **Checkbox Interaction**: When a player clicks a checkbox:
   - The preparation is added/removed from the ingredient instance
   - The ingredient's properties are modified according to the preparation
   - The ingredient's display name is updated
4. **Data Synchronization**: Changes are automatically synced back to the dish data

## Example Preparation Data

```cpp
// Example preparation entry in data table
PreparationTag: "Prep.Chopped"
DisplayName: "Chopped"
Description: "Cut into small pieces"
NamePrefix: "Chopped"
PropertyModifiers: 
  - PropertyType: "Thickness", ModificationType: "Additive", Value: -0.3
  - PropertyType: "Cohesion", ModificationType: "Additive", Value: -0.2
```

## Troubleshooting

### Checkboxes Not Appearing
- Verify `PreparationCheckboxClass` is set in both the quantity control and dish customization widgets
- Check that the ingredient has a valid `PreparationDataTable` reference
- Ensure the preparation data table has valid entries

### Checkbox Events Not Working
- Verify the checkbox Blueprint has the correct component bindings
- Check that the `OnPreparationCheckboxChanged` event is properly bound
- Ensure the preparation tags are valid GameplayTags

### Preparation Effects Not Applied
- Verify the preparation data has valid `PropertyModifiers`
- Check that the ingredient has the properties being modified
- Ensure the preparation data table is properly referenced

## Blueprint Events

### PUPreparationCheckbox Events
- `OnPreparationDataSet` - Called when preparation data is set
- `OnCheckedStateChanged` - Called when checkbox state changes

### PUIngredientQuantityControl Events
- `OnPreparationChanged` - Called when a preparation is added/removed
- `OnQuantityControlChanged` - Called when any change occurs (including preparations)

## C++ Integration

The system is designed to work seamlessly with both Blueprint and C++:
- All functions are marked as `BlueprintCallable`
- Events are marked as `BlueprintAssignable`
- Blueprint events can be overridden for custom behavior 