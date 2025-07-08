# UI Setup Guide for Dish Customization

This guide shows you how to set up your UI widgets to receive dish data via the event-driven system.

## Step 1: Create a Blueprint Widget

1. **Create a new Widget Blueprint**
   - Right-click in Content Browser → User Interface → Widget Blueprint
   - Name it something like `WBP_DishCustomization`

2. **Set the Parent Class**
   - Open the Widget Blueprint
   - In the Class Settings, set "Parent Class" to `PUDishCustomizationWidget`

## Step 2: Set Up the Widget Blueprint

### **Option A: Use the Built-in Events (Recommended)**

The `PUDishCustomizationWidget` class provides these Blueprint events:

- **OnDishDataReceived** - Called when initial dish data is received
- **OnDishDataChanged** - Called when dish data is updated
- **OnCustomizationModeEnded** - Called when customization ends

### **Example Blueprint Setup:**

1. **In the Event Graph:**
   ```
   Event OnDishDataReceived (FPUDishBase DishData)
   ├── Print String: "Received dish data: " + DishData.DisplayName
   ├── Update Dish Name Text Block with DishData.DisplayName
   └── Call "Update Ingredient List" function
   ```

2. **Create a function "Update Ingredient List":**
   ```
   Function Update Ingredient List (FPUDishBase DishData)
   ├── Clear Ingredient List (your list widget)
   ├── For Each IngredientInstance in DishData.IngredientInstances
   │   ├── Create Ingredient Item Widget
   │   ├── Set Ingredient Name
   │   ├── Set Ingredient Quantity
   │   └── Add to Ingredient List
   ```

### **Option B: Manual Event Binding**

If you're using a different widget class, you can manually bind to events:

1. **In the Widget's Event Construct:**
   ```
   Event Construct
   ├── Get Owning Player
   ├── Get Player Controller
   ├── Get Pawn
   ├── Get Component by Class (PUDishCustomizationComponent)
   ├── Bind Event to OnInitialDishDataReceived
   ├── Bind Event to OnDishDataUpdated
   └── Bind Event to OnCustomizationEnded
   ```

## Step 3: Set the Widget Class in the Component

1. **In your Cooking Station Blueprint:**
   - Select the Dish Customization Component
   - Set "Customization Widget Class" to your `WBP_DishCustomization`

## Step 4: Test the Connection

1. **Add some debug prints to verify data flow:**
   ```
   Event OnDishDataReceived
   ├── Print String: "Widget received dish data!"
   ├── Print String: "Dish name: " + DishData.DisplayName
   └── Print String: "Ingredients: " + DishData.IngredientInstances.Num
   ```

2. **Check the Output Log** to see if the events are firing correctly.

## Step 5: Update Your UI Elements

### **Common UI Elements to Update:**

- **Dish Name Text Block** - `DishData.DisplayName`
- **Ingredient List** - `DishData.IngredientInstances`
- **Dish Preview Image** - `DishData.PreviewTexture`
- **Custom Name Input** - `DishData.CustomName`

### **Example: Update Dish Name**
```
Event OnDishDataReceived (FPUDishBase DishData)
├── Set Text (DishNameTextBlock, DishData.DisplayName)
```

### **Example: Update Ingredient List**
```
Event OnDishDataReceived (FPUDishBase DishData)
├── Clear List (IngredientListView)
├── For Each IngredientInstance in DishData.IngredientInstances
│   ├── Create Widget (IngredientItemWidget)
│   ├── Set Ingredient Name (IngredientInstance.IngredientTag)
│   ├── Set Quantity (IngredientInstance.Quantity)
│   └── Add to List (IngredientListView)
```

## Step 6: Send Data Back to Component

When the user makes changes in the UI, send the data back:

```
Event On Ingredient Added Button Clicked
├── Get Current Dish Data
├── Add Ingredient to Dish Data
└── Call UpdateDishData (Modified Dish Data)
```

## Troubleshooting

### **Widget not receiving data?**
- Check that the widget class is set to `PUDishCustomizationWidget`
- Verify the component reference is set correctly
- Check the Output Log for error messages

### **Events not firing?**
- Make sure you're using the correct event names
- Check that the widget is properly subscribed to the component
- Verify the component is broadcasting events

### **Data not updating?**
- Ensure you're calling `UpdateDishData()` when making changes
- Check that the component reference is valid
- Verify the data structure is correct

## Example Complete Setup

Here's a complete example of what your widget might look like:

```
Event OnDishDataReceived (FPUDishBase DishData)
├── Set Text (DishNameText, DishData.DisplayName)
├── Set Image (DishPreviewImage, DishData.PreviewTexture)
├── Set Text (CustomNameInput, DishData.CustomName)
└── Call UpdateIngredientList (DishData)

Event OnDishDataChanged (FPUDishBase DishData)
├── Call UpdateIngredientList (DishData)

Event OnCustomizationModeEnded
├── Hide Widget
└── Reset UI State

Function UpdateIngredientList (FPUDishBase DishData)
├── Clear List (IngredientListView)
├── For Each IngredientInstance in DishData.IngredientInstances
│   ├── Create Widget (IngredientItemWidget)
│   ├── Set Ingredient Name
│   ├── Set Quantity
│   └── Add to List
``` 