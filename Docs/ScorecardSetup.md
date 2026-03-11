# Scorecard Widget Setup

The scorecard displays when you complete and deliver an order to a dish giver. It shows:
- **Dish capture** – Image of the dish (or preview texture)
- **Seal of approval** – Animated in, 3 tiers (Perfect/Great/Good) based on satisfaction score
- **Base ingredients** – Original ingredients from the dish data table
- **Flavor profile** – Top 2 flavor aspects, top 3 contributing ingredients each, 5-star integer rating
- **Texture profile** – Same structure for top 2 texture aspects

## Blueprint Setup

### 1. Create WBP_Scorecard

1. Create a Widget Blueprint that inherits from **PUScorecardWidget**.
2. Add these child widgets from the **Palette** (drag and drop, name them exactly):
   - **DishImage** – Image (for the dish capture)
   - **SealImage** – Image (for the seal; texture changes by tier)
   - **BaseIngredientsContainer** – Vertical Box (for base ingredients list)
   - **FlavorProfileContainer** – Vertical Box (flavor profile is spawned into this)
   - **TextureProfileContainer** – Vertical Box (texture profile is spawned into this)

3. Optionally set **Aspect Profile Widget Class** in Class Defaults if you created a custom WBP_AspectProfile. Leave empty to use the default.

4. Assign the 3 seal textures in Class Defaults:
   - **Seal Texture Perfect** (≥ 0.9 satisfaction)
   - **Seal Texture Great** (≥ 0.7)
   - **Seal Texture Good** (≥ 0.5)

### 2. Showing the Scorecard from Dialogue

**In the DlgSystem dialogue tree:**
1. Add an **Event** node with the name **ShowScorecard** (same pattern as GenerateOrder).
2. Place it on the node where the player delivers the order (e.g. after the dish giver accepts it).

**On the Dish Giver (or any TalkingObject):**
1. Select the dish giver in the level (or open its Blueprint).
2. In **Details**, find **Scorecard Widget Class** under "Talking Object | Dialogue".
3. Set it to **WBP_Scorecard**.

When the dialogue hits the ShowScorecard event, the scorecard will appear. The order must already be completed (player submitted at cooking station).

**From Blueprint (non-dialogue):** Get the player character and call **Show Scorecard** with WBP_Scorecard.

### 3. Seal Animation

`PlaySealAnimation` is called automatically from `ShowFromOrder`. Override it in Blueprint to add your animation (e.g. scale in, fade in). The seal image is already set based on the tier.

## C++ Usage

```cpp
// Get scorecard data from a completed order
FPUScorecardData Data = UPUDishBlueprintLibrary::GetScorecardData(Order);

// Or show directly on a scorecard widget
ScorecardWidget->ShowFromOrder(Character->GetCurrentOrder(), nullptr);
```

## Data Flow

- **Seal tier**: `FinalSatisfactionScore` → Perfect (≥0.9), Great (≥0.7), Good (≥0.5)
- **Base ingredients**: From `Order.BaseDish.IngredientInstances`
- **Flavor/Texture profiles**: From `Order.CompletedDish` – top 2 aspects by total value, top 3 ingredients per aspect by contribution
- **Star rating**: Integer 0–5, from aspect totals (0–5 scale)
