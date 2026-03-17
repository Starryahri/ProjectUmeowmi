# Order Hint Radar Chart Setup

The radar chart can display dish giver preferences (hints) alongside the player's dish. Hints are discovered progressively through dialogue.

## Overview

- **Layer 0** (behind): Hint/guide shape — only discovered aspects are shown
- **Layer 1** (in front): Player's dish profile
- Use `Order.DiscoveredHints` — starts empty; grows as the player learns hints from dialogue

## 1. Revealing Hints via Dialogue

Add **Event** nodes to your dialogue (DlgSystem) with names like:

- `RevealHint_Salt` — reveals the Salt flavor hint
- `RevealHint_Sweet` — reveals the Sweet flavor hint
- `RevealHint_Crispy` — reveals the Crispy texture hint
- `RevealHint_Juicy` — reveals the Juicy texture hint
- etc.

**Format**: `RevealHint_` + aspect name (Umami, Salt, Sweet, Sour, Bitter, Spicy, Rich, Juicy, Tender, Chewy, Crispy, Crumbly)

The event only applies when:
- The dialogue is on a **Dish Giver**
- The player has an **active order from that dish giver**
- The aspect exists in the order's `TargetAspects`

## 2. Populating the Radar Chart from an Order

When showing the order UI (e.g. during cooking or in an order info panel):

```cpp
// Order from player
FPUOrderBase Order = Character->GetCurrentOrder();

// Current dish (from customization or empty)
FPUDishBase PlayerDish = GetCurrentPlayerDish(); // Your function

// Flavor radar — uses DiscoveredHints (partial hints). Pass Order.TargetAspects to show all hints.
FlavorRadarChart->SetValuesFromOrderFlavorProfile(Order, PlayerDish, Order.DiscoveredHints);

// Texture radar
TextureRadarChart->SetValuesFromOrderTextureProfile(Order, PlayerDish, Order.DiscoveredHints);
```

**Blueprint**: See [Blueprint Setup](#blueprint-setup) below for step-by-step instructions.

## 3. Toggling the Hint Layer

```cpp
RadarChart->SetShowHintLayer(true);   // Show guide shape (default)
RadarChart->SetShowHintLayer(false);  // Hide guide shape
```

## 4. Revealing Hints from Blueprint

You can also reveal hints directly:

```cpp
// On the player character
Character->RevealHintOnCurrentOrder(FName("Salt"));

// On the dish giver (checks order is from this giver)
DishGiver->RevealHintToPlayer(FName("Salt"));
```

## 5. Blueprint Setup (Step-by-Step)

The radar chart is updated in your **Dish Customization Widget Blueprint** via the `OnDishDataChanged` event. You need to switch to the order-based functions when the player has an active order.

### Where to Wire This

1. Open your **Dish Customization Widget Blueprint** (the one that contains the Flavor and/or Texture radar charts).
2. Find the **Event OnDishDataChanged** node (or create an override if you don't have one yet).
3. The event receives `DishData` (FPUDishBase) — that's the player's current dish.

### Blueprint Logic

```
Event OnDishDataChanged (DishData)
├── Get Player Controller
├── Get Controlled Pawn
├── Cast to ProjectUmeowmiCharacter → Character
├── Branch (Has Current Order?)
│   ├── True:
│   │   ├── Get Current Order (from Character) → Order
│   │   ├── FlavorRadarChart → Set Values From Order Flavor Profile
│   │   │       (Order, DishData, Order.DiscoveredHints)
│   │   ├── FlavorRadarChart → Set Show Hint Layer (true)
│   │   ├── [If you have a texture radar:]
│   │   │   TextureRadarChart → Set Values From Order Texture Profile
│   │   │       (Order, DishData, Order.DiscoveredHints)
│   │   └── TextureRadarChart → Set Show Hint Layer (true)
│   └── False:
│       └── [Your existing logic: Set Values From Dish Flavor Profile, etc.]
```

### Node-by-Node

1. **Event OnDishDataChanged** — Override this in your widget Blueprint.
2. **Get Player Controller** → **Get Controlled Pawn** → **Cast to ProjectUmeowmiCharacter**.
3. **Has Current Order** (on the Character) — returns true if the player has an active order.
4. **Get Current Order** (on the Character) — returns the `FPUOrderBase` struct.
5. On your **PURadarChart** (FlavorRadarChart):
   - **Set Values From Order Flavor Profile** — pins: `Order`, `PlayerDish` (use the `DishData` from the event), `DiscoveredHints` (use `Order.DiscoveredHints` — break the Order struct or use a "Get" to access it).
   - **Set Show Hint Layer** — `true` to show the hint shape.
6. Repeat for **TextureRadarChart** with **Set Values From Order Texture Profile** if you have one.

### Getting Order.DiscoveredHints in Blueprint

- Use **Break FPUOrderBase** on the Order to get all fields, including `DiscoveredHints`.
- Or connect the Order directly to **Set Values From Order Flavor Profile**; the third pin is `DiscoveredHints` — drag from the Order's `DiscoveredHints` output (from Break) or from a "Get" node.

### Refreshing After Hints Are Revealed

Hints are revealed during dialogue. The radar chart refreshes the next time `OnDishDataChanged` fires (e.g. when the player adds/removes an ingredient). If you want it to update immediately when returning from dialogue, call your refresh logic when the cooking UI is shown or when the player re-enters the cooking stage — for example, trigger `OnDishDataChanged` with the current dish data (from `Get Current Dish Data` on the widget).

## 6. Example Dialogue Flow

1. Player talks to dish giver
2. `GenerateOrder` event → order given
3. Player asks "What do you want?" → `RevealHint_Salt` event → Salt hint revealed
4. Player asks more → `RevealHint_Sweet` event → Sweet hint revealed
5. Order UI radar shows partial shape (Salt + Sweet) behind player's dish
