# Popups — Developer Documentation

**Module:** `ProjectUmeowmi`  
**Primary source:** `Source/ProjectUmeowmi/UI/PUPopupData.h`, `Source/ProjectUmeowmi/UI/PUPopupWidget.*`, `Source/ProjectUmeowmi/PUProjectUmeowmiGameInstance.*` (popup manager)

**Popups** are data-driven **`FPopupData`** structs rendered by **`UPUPopupWidget`** (subclass of **`UPUCommonUserWidget`** — see **`UI.md`** for base UI patterns). **`UPUProjectUmeowmiGameInstance`** creates the widget, manages a **queue**, restores **input** after close, and exposes **ingredient unlock** helpers.

---

## Data structures

### `EPopupType`

| Value | Use |
|-------|-----|
| `Notification` | Short notices (e.g. ingredient unlock). |
| `Tutorial` | Tutorial copy. |
| `Confirmation` | Yes/No style. |
| `Info` / `Warning` / `Error` | Styling hooks in `UPUPopupWidget::UpdatePopupStyle`. |

### `FPopupButtonData`

| Property | Type | Description |
|----------|------|-------------|
| `ButtonID` | `FName` | Passed to callbacks when pressed (e.g. `YES`, `NO`, `OK`). |
| `ButtonLabel` | `FText` | Visible label. |
| `bIsPrimary` | `bool` | Primary styling. |

### `FPopupData`

| Property | Type | Description |
|----------|------|-------------|
| `PopupType` | `EPopupType` | Category for styling/behavior. |
| `Title` | `FText` | Header (optional). |
| `Message` | `FText` | Body. |
| `Icon` | `UTexture2D*` | Optional icon. |
| `Buttons` | `TArray<FPopupButtonData>` | Empty → default OK in widget logic. |
| `bModal` | `bool` | If true, move/look input ignored while open. |
| `bAutoDismiss` | `bool` | Auto-close after timer. |
| `AutoDismissTime` | `float` | Seconds (if auto-dismiss). |
| `bShowCloseButton` | `bool` | Corner close control. |
| `HorizontalAlignment` | `float` | 0–1 viewport horizontal anchor. |
| `VerticalAlignment` | `float` | 0–1 viewport vertical anchor. |
| `PositionOffset` | `FVector2D` | Pixel nudge from anchor. |
| `SizeOverride` | `FVector2D` | If both &gt; 0, overrides desired size. |
| `AdditionalData` | `TArray<FGameplayTag>` | Extra tags (e.g. ingredient unlocks). |

---

## Class API: `UPUPopupWidget`

**Class Name:** `UPUPopupWidget`

**Description**  
Renders **`FPopupData`**: title, message, icon, dynamically spawned buttons (`UButton` or custom **`ButtonWidgetClass`**), optional auto-dismiss, viewport alignment, style by **`EPopupType`**. Calls **`UPUProjectUmeowmiGameInstance::NotifyPopupClosed(ButtonID)`** when the user closes the popup or presses a button. Supports **F / A** (gamepad) in **`NativeOnPreviewKeyDown`** for dismiss/confirm.

**Inheritance**  
**Parent class:** `UPUCommonUserWidget`

**Bind widgets (expected in Blueprint)**  
`TitleText`, `MessageText`, `IconImage`, `ButtonsContainer`, `CloseButton`, `PopupBorder`.

**Key methods**

| Method | Description |
|--------|-------------|
| `SetPopupData` | Applies data, layout, style, buttons, timers. |
| `Close(ButtonID)` | Removes from viewport and notifies game instance. |
| `GetPopupData` | Current `FPopupData`. |
| `GetPreferredFocusTarget` | First button, close, or self — for controller focus. |
| `HandleButtonClickWithID` / `HandleButtonClickWithIDDirect` | For spawned buttons. |

**Properties (selected)**

| Property | Type | Description |
|----------|------|-------------|
| `ButtonWidgetClass` | `TSubclassOf<UUserWidget>` | Optional custom row button; else `UButton`. |
| `ButtonLabelWidgetName` | `FName` | Text block name inside custom button for labels. |
| `CurrentPopupData` | `FPopupData` | Active configuration. |

**Viewport:** Added at **z-order 1000** from the game instance.

---

## Integration: `UPUProjectUmeowmiGameInstance` (popup manager)

**Configuration**

| Property | Description |
|----------|-------------|
| `PopupWidgetClass` | `TSoftClassPtr<UPUPopupWidget>` — **required**. Loaded synchronously when showing. Set in **Game Instance Blueprint** or **`DefaultEngine.ini`**: `[/Script/ProjectUmeowmi.PUProjectUmeowmiGameInstance] PopupWidgetClass=/Game/.../WBP_Popup.WBP_Popup_C` |

**API**

| Method | Description |
|--------|-------------|
| `ShowPopup(PopupData)` | Shows popup; no C++ callback (use **`OnPopupClosedEvent`** in Blueprint). |
| `ShowPopupWithCallback(PopupData, FOnPopupClosed)` | Stores **single** delegate; executed in **`OnPopupWidgetClosed`** with **`ButtonID`**. |
| `ShowIngredientUnlockPopup` | Builds notification **`FPopupData`** for one tag; optional display name. |
| `ShowIngredientUnlockPopupMultiple` | Multiple tags (lists up to 5 names + count). |
| `CloseCurrentPopup` | Calls **`Close(NAME_None)`** on current widget. |
| `NotifyPopupClosed(ButtonID)` | Called by **`UPUPopupWidget`** — routes to **`OnPopupWidgetClosed`**. |
| `IsPopupShowing` | `CurrentPopupWidget != nullptr`. |

**Events**

| Delegate | When | Payload |
|----------|------|---------|
| `OnPopupClosedEvent` | After popup closes | `FName ButtonID` — pressed button, or `NAME_None` if closed otherwise |

**Queue behavior**  
If **`ShowPopup`** is called while **`CurrentPopupWidget`** is non-null, **`FPopupData`** is **appended to `PopupQueue`**. **Callbacks for queued items are not reliably stored** (warning logged). When the active popup closes, **`ProcessPopupQueue`** shows the next entry with **`ShowPopup`** (no callback). Prefer serializing popup requests from gameplay or extending the queue if you need per-item callbacks.

**Input when opening**  
- **`FInputModeUIOnly`** with focus on **`GetPreferredFocusTarget`**.  
- **`bModal`**: **`SetIgnoreMoveInput(true)`**, **`SetIgnoreLookInput(true)`**.  
- Mouse cursor shown.

**Input when closing (`OnPopupWidgetClosed`)**  
- **`ResetIgnoreInputFlags`** on the player controller.  
- If **dish customization** is active or **dialogue** is visible, move/look stay ignored and focus can return to the dialogue box.  
- Otherwise **`FInputModeGameAndUI`** with **DoNotLock** mouse, optional focus restore.  
- Executes **`CurrentPopupCallback`** if bound (**`ShowPopupWithCallback`**).  
- **`OnPopupClosedEvent.Broadcast(ButtonID)`**.  
- Clears **`CurrentPopupWidget`**, then **`ProcessPopupQueue`**.

---

## Usage examples

**Blueprint — bind to close event**  
Subscribe to **`OnPopupClosedEvent`** on the Game Instance (e.g. widget construct). **`ButtonID`** matches **`FPopupButtonData::ButtonID`**.

**C++ — callback**  
```cpp
GI->ShowPopupWithCallback(PopupData, FOnPopupClosed::CreateLambda([](FName ButtonID)
{
    // Handle ButtonID
}));
```

---

## Document change log

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0.0 | 2025-03-25 | Documentation | Popups documentation (split from former `UIPopups.md`; shared UI bases in `UI.md`). |
