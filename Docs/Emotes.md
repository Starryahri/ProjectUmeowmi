# Emotes — Developer Documentation

**Module:** `ProjectUmeowmi`  
**Primary source:** `Source/ProjectUmeowmi/UI/PUEmoteWidget.h`, `PUEmoteData.h`, `ProjectUmeowmiCharacter` (emote section)

Small **over-head icon** system: **`FPUEmoteData`** rows (tag → icon, sound, duration) and **`UPUEmoteWidget`** (single **`EmoteImage`**, optional **`FadeUp`** animation). The **player** uses a **`UWidgetComponent`** and **`ShowEmoteByTag`**; **`ATalkingObject`** can use a parallel pattern with **`TalkingObjectWidget`** — see **`Dialogue.md`** for NPC UI.

---

## Struct API: `FPUEmoteData`

**Parent:** `FTableRowBase`

| Property | Type | Description |
|----------|------|-------------|
| `EmoteTag` | `FGameplayTag` | Request id (e.g. under `Emote.*`). |
| `Icon` | `UTexture2D*` | Image above head. |
| `Sound` | `USoundBase*` | Optional one-shot when shown. |
| `Duration` | `float` | Visible time (seconds). |
| `bLoop` | `bool` | If true, no auto-dismiss — clear explicitly. |

---

## Class API: `UPUEmoteWidget`

**Parent:** `UUserWidget` (not `UPUCommonUserWidget`)

| Method | Description |
|--------|-------------|
| `SetEmoteIcon` / `ClearEmoteIcon` | Texture + visibility. |
| `PlayFadeIn` / `PlayFadeOut` | Uses widget animation **`FadeUp`** if bound. |
| `GetFadeUpDuration` | Timing helper. |

**BindWidget:** `EmoteImage`  
**BindWidgetAnimOptional:** `FadeUp`

---

## Player character integration

| Property / API | Description |
|----------------|-------------|
| `EmoteWidget` | `UWidgetComponent` hosting the widget class. |
| `bEnableEmotes`, `EmoteWidgetClass`, `EmoteWidgetSpace`, `EmoteDrawSize`, `EmoteScale` | Tuning. |
| `EmoteDataTable` | **`FPUEmoteData`** lookup for **`ShowEmoteByTag`**. |
| `ShowEmoteByTag` / `ClearEmote` / `IsEmoteActive` | Public API. |

---

## Cross-references

| Topic | Doc |
|-------|-----|
| Player hub (other systems) | `PlayerCharacter.md` |
| Widget map | `UI.md` |
| Dialogue / talking objects | `Dialogue.md` |

---

## Document change log

| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0.0 | 2026-03-25 | Documentation | Initial emotes documentation for ProjectUmeowmi. |
