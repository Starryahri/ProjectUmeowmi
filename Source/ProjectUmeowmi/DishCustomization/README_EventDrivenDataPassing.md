# Event-Driven Data Passing for Dish Customization

This document explains how to use the event-driven data passing system for dish customization.

## Overview

The event-driven approach uses Unreal's delegate system to broadcast dish data changes from the cooking station to the dish customization component and UI widgets. This prevents crashes and provides decoupled communication.

## How It Works

1. **Cooking Station** broadcasts dish data using delegate events
2. **Dish Customization Component** receives the data and can broadcast updates
3. **UI Widgets** can subscribe to data changes and update accordingly

## Usage

### From Cooking Station to Dish Customization Component

```cpp
// In your cooking station's StartInteraction()
if (CurrentOrder.BaseDish.DishTag.IsValid())
{
    // Broadcast initial dish data
    DishCustomizationComponent->BroadcastInitialDishData(CurrentOrder.BaseDish);
}
```

### Broadcasting Data Updates

```cpp
// When dish data changes
CustomizationComponent->BroadcastDishDataUpdate(UpdatedDishData);
```

### Subscribing to Events (in Blueprint or C++)

```cpp
// Subscribe to initial data
CustomizationComponent->OnInitialDishDataReceived.AddDynamic(this, &MyClass::OnInitialDataReceived);

// Subscribe to data updates
CustomizationComponent->OnDishDataUpdated.AddDynamic(this, &MyClass::OnDataUpdated);
```

## Available Events

- `OnInitialDishDataReceived` - Fired when initial dish data is set
- `OnDishDataUpdated` - Fired when dish data is updated
- `OnCustomizationEnded` - Fired when customization mode ends

## Benefits

- ✅ **No Crashes**: Prevents null pointer issues
- ✅ **Decoupled**: Components don't need direct references
- ✅ **Flexible**: Easy to add/remove listeners
- ✅ **Thread-Safe**: Uses Unreal's delegate system
- ✅ **Simple**: Easy to understand and implement

## Example Implementation

See `PUCookingStation.cpp` for a complete example of how the cooking station uses this system. 