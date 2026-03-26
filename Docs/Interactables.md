# Interactables — Developer Documentation

**Module:** `ProjectUmeowmi`  
**Primary source:** `Source/ProjectUmeowmi/Interactables/`, `Source/ProjectUmeowmi/Interfaces/PUInteractableInterface.h`, `Source/ProjectUmeowmi/Dialogue/TalkingObject.*`

This doc ties together **world interaction** types: the generic **`IPUInteractableInterface`** / **`APUInteractableBase`** path, and the **primary gameplay path** built on **`ATalkingObject`** (dialogue prompt, overlap, Dlg). **Cooking / plating stations** and **level transitions** are **not** re-documented in full here—those details live in **`DishCustomization.md`**, **`Dialogue.md`**, and **`LevelTransitions.md`** to avoid redundancy.

---

## Two interaction patterns

| Pattern | Core types | How the player engages |
|--------|------------|-------------------------|
| **TalkingObject (primary)** | `ATalkingObject` → `APUCookingStation`, `APUPlatingStation`, `APULevelTransition`, `APUDishGiver`, … | **Sphere overlap** → prompt widget → **Interact** input → `StartInteraction` / dialogue / `UPUDishCustomizationComponent` / `TransitionToLevel`. |
| **Interface (optional / future)** | `IPUInteractableInterface`, `APUInteractableBase` | **Text + range + delegates**; `AProjectUmeowmiCharacter`: **`RegisterInteractable` / `UnregisterInteractable`** (see below). |

**`ATalkingObject` does not implement `IPUInteractableInterface`.** The two systems are separate. Most shipped flows use **TalkingObject** + overlap lists on the character.

---

## Interface: `IPUInteractableInterface`

**Header:** `Source/ProjectUmeowmi/Interfaces/PUInteractableInterface.h`

**Description**  
Blueprint/C++ interface for actors that expose a simple **CanInteract / Start / End** contract, **FText** prompts, **interaction range**, and **multicast delegates** for UI or gameplay hooks.

**Methods (pure virtual)**

| Method | Purpose |
|--------|---------|
| `CanInteract()` | Whether interaction is allowed right now. |
| `StartInteraction()` / `EndInteraction()` | Begin / end interaction. |
| `GetInteractionText()` / `GetInteractionDescription()` | Prompt strings. |
| `GetInteractionRange()` | Range for UI or queries. |
| `IsInteractable()` | Master enable flag. |
| `OnInteractionStarted()` … `OnInteractionFailed()` | Delegate getters (multicast). |

**Delegates:** `FOnInteractionStarted`, `FOnInteractionEnded`, `FOnInteractionRangeEntered`, `FOnInteractionRangeExited`, `FOnInteractionStateChanged`, `FOnInteractionFailed`.

---

## Class API: `APUInteractableBase`

**Class Name:** `APUInteractableBase`

**Description**  
Minimal **`AActor`** implementation of **`IPUInteractableInterface`**: editable **Text / Description / Range / bIsInteractable**, broadcast helpers for all delegates. **`StartInteraction`** broadcasts **Started** or **Failed**; **`EndInteraction`** broadcasts **Ended**. Range enter/exit are **not** wired automatically—you must call the broadcast helpers from overlap code if you use this base.

**Inheritance**  
**Parent:** `AActor`  
**Interfaces:** `IPUInteractableInterface`

**Properties**

| Property Name | Type | Access | Description |
|---------------|------|--------|-------------|
| `InteractionText` | `FText` | EditAnywhere, BlueprintReadWrite | Short prompt. |
| `InteractionDescription` | `FText` | EditAnywhere, BlueprintReadWrite | Longer hint. |
| `InteractionRange` | `float` | EditAnywhere, BlueprintReadWrite | Nominal range (default 200). |
| `bIsInteractable` | `bool` | EditAnywhere, BlueprintReadWrite | Gates **`CanInteract`** / **`IsInteractable`**. |

---

## Actor map (project interactables)

| Actor | Base | Role | Doc |
|-------|------|------|-----|
| `APUCookingStation` | `ATalkingObject` | Dish customization + dialogue; optional skip dialogue when player has order. | **`DishCustomization.md`**, **`Dialogue.md`** |
| `APUPlatingStation` | `ATalkingObject` | Plating stage via `UPUDishCustomizationComponent` + `PlatingWidgetClass`. | **`DishCustomization.md`**, **`Dialogue.md`** |
| `APULevelTransition` | `ATalkingObject` | Map change + locks; optional auto-trigger. | **`LevelTransitions.md`**, **`Dialogue.md`** (events) |
| `APUDishGiver` | `ATalkingObject` | Orders + dialogue; `UPUOrderComponent`. | **`DishCustomization.md`** (orders), **`Dialogue.md`** |
| `ATalkingObject` | `AActor` | Generic NPC/prop/door/system interactable; Dlg participant, emotes, quest markers. | **`Dialogue.md`**, **`QuestSystem.md`** |

**`APUInteractableBase`** — generic template; **no** subclasses in `Interactables/` beyond the stations above (stations use **TalkingObject**, not this base).

---

## Player character: `RegisterInteractable` / `CurrentInteractable`

**`AProjectUmeowmiCharacter`** exposes:

- **`TScriptInterface<IPUInteractableInterface> CurrentInteractable`**
- **`RegisterInteractable` / `UnregisterInteractable`** — binds to **`OnInteractionStarted`**, **`OnInteractionEnded`**, **`OnInteractionFailed`** (handlers are mostly stubs / logging today).

**Note:** As of this documentation pass, **no other project source** calls **`RegisterInteractable`**—the **TalkingObject** pipeline uses **overlap + `Interact`** and does not go through this interface. The interface is available for **future** interactables or Blueprint wiring.

---

## When to read which doc

| Topic | File |
|-------|------|
| Dialogue, Dlg events, talking objects | `Dialogue.md` |
| Customization component, orders at stations | `DishCustomization.md` |
| Changing levels, locks, spawn points | `LevelTransitions.md` |
| Quest markers on NPCs | `QuestSystem.md` |

---

## Document change log

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0.0 | 2025-03-25 | Documentation | Initial interactables overview for ProjectUmeowmi. |
