# Order System - Active Order Prevention

## Overview
This update prevents players from receiving multiple orders simultaneously. Players must complete their current order before receiving a new one from dish givers.

## Changes Made

### 1. Modified `APUDishGiver::StartInteraction()`
- **Before**: Always generated a new order when interacting with dish givers
- **After**: No longer generates orders automatically on interaction
  - Order generation is now controlled entirely by the dialogue system
  - Interaction only starts the dialogue system
  - Orders are generated only when dialogue events call `GenerateAndGiveOrderToPlayer()`

### 2. Enhanced Dialogue Conditions
Added new conditions that can be used in dialogue nodes:

- **`HasActiveOrder`**: Returns `true` if player has an active (incomplete) order **from this dish giver** (participant-scoped)
- **`OrderCompleted`**: Returns `true` if player has a completed order ready for submission **from this dish giver** (participant-scoped)
- **`NoActiveOrder`**: Returns `true` if player has no active order (global - prevents duplicate orders)

## Usage in Dialogue System

### Example Dialogue Flow
1. **Initial Interaction** (NoActiveOrder = true)
   - Show welcome dialogue
   - Use dialogue event to call `GenerateAndGiveOrderToPlayer()`
   - Show order details

2. **During Active Order** (HasActiveOrder = true)
   - Show reminder about current order
   - No new order generated

3. **Order Completed** (OrderCompleted = true)
   - Handle order completion
   - Show feedback and satisfaction score
   - Clear completed order
   - Ready for new order on next interaction

### Dialogue Node Setup
In your dialogue data, you can now use these conditions and events:

```
Node: "Welcome"
Condition: NoActiveOrder
Text: "Hello! I have a new order for you..."
Event: GenerateAndGiveOrderToPlayer

Node: "Current Order Reminder"
Condition: HasActiveOrder
Text: "How's that order coming along? I'm still waiting for it."

Node: "Order Completion"
Condition: OrderCompleted
Text: "Let me see what you've made..."
```

### Available Methods for Dialogue Events
- **`GenerateAndGiveOrderToPlayer()`**: Generates a new order and gives it to the player (only if they don't have an active order)
- **`RevealHint_Salt`**, **`RevealHint_Sweet`**, **`RevealHint_Crispy`**, etc.: Reveals that aspect as a hint on the player's current order (for radar chart display). Use format `RevealHint_` + aspect name. See `Docs/OrderHintRadarChartSetup.md`.

## Technical Details

### Key Methods Modified
- `APUDishGiver::StartInteraction()` - Main logic for order prevention
- `APUDishGiver::CheckCondition()` - Added order-related conditions
- `APUDishGiver::HandleOrderCompletion()` - Called when order is completed

### Order Flow
1. Player interacts with dish giver
2. System starts dialogue (no automatic order generation)
3. Dialogue system uses conditions to determine appropriate response
4. If dialogue decides to give order: Calls `GenerateAndGiveOrderToPlayer()` event
5. If player has completed order: Handle completion during dialogue

## Participant-Scoped Order Conditions
When multiple dish givers (e.g. Yeoh, Lola) share a dialogue or level, each participant's `HasActiveOrder` and `OrderCompleted` conditions are scoped to that participant. The order stores `OrderGiverParticipantName` when given to the player, so Yeoh only shows "reminder" dialogue when the player has Yeoh's order, and Lola only shows hers when the player has Lola's order. Orders loaded from save without this field (legacy) match any participant for backward compatibility.

## Benefits
- Prevents order confusion
- Ensures players complete orders before getting new ones
- Maintains game flow and progression
- Provides clear feedback on order status

## Testing
To test this system:
1. Get an order from a dish giver
2. Try to talk to the same or different dish giver
3. Verify no new order is generated
4. Complete the order
5. Talk to dish giver again
6. Verify order completion is handled and new order can be received 