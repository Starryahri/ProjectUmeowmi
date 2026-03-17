# PUAspectProfileWidget – Step-by-Step Setup

**PUAspectProfileWidget displays ONE aspect** (e.g. "Umami" or "Crispy"):
- Aspect name
- Top 3 contributing ingredients
- 5-star rating (★★★☆☆)

The **scorecard shows two of these** per profile (flavor profile = 2 widgets, texture profile = 2 widgets).

---

## Step 1: Create the Blueprint

1. In Content Browser: **Right-click → User Interface → Widget Blueprint**
2. Name it **WBP_AspectProfile**
3. Open it
4. In **Details** panel, set **Parent Class** to **PUAspectProfileWidget**

---

## Step 2: Build the Hierarchy

Use whatever root you prefer (SizeBox + Overlay, Vertical Box, etc.). Add these as children and name them exactly:

- **Text Block** → **AspectNameText**
- **Horizontal Box** → **IngredientsContainer**
- **Horizontal Box** → **StarRatingContainer**

### Example hierarchy (root is up to you)
```
Your Root (SizeBox, Overlay, Vertical Box, etc.)
├── AspectNameText (Text Block)       ← Aspect name, e.g. "Umami"
├── IngredientsContainer (Horizontal Box) ← Top 3 ingredients (C++ populates)
└── StarRatingContainer (Horizontal Box) ← ★★★☆☆ (C++ populates)
```

C++ binds by widget name, not by parent.

---

## Step 3: Verify

- **AspectNameText** – C++ sets the aspect name
- **IngredientsContainer** – C++ adds up to 3 ingredient names
- **StarRatingContainer** – C++ adds the star rating text

All three use `BindWidgetOptional`; missing widgets won't crash but that part won't display.

---

## Step 4: Use in Scorecard

In **WBP_Scorecard**, set **Aspect Profile Widget Class** in Class Defaults to **WBP_AspectProfile**. The scorecard spawns two of these for flavor and two for texture.
