# Rich Text Dialogue Setup

The dialogue box now uses **Common Rich Text Block** (from CommonUI) to support colored, italic, and underlined text. Use markup tags in your dialogue text.

## Required: Update Your PUDialogueBox Blueprint

**You must update your Blueprint** or dialogue will not display:

1. Open your **PUDialogueBox** Blueprint (the one that extends the C++ class).
2. In the hierarchy, find the **DialogueText** widget (currently a Text Block).
3. **Delete** the DialogueText Text Block.
4. From the Palette, add a **Common Rich Text Block** (under Common UI) to the same parent.
5. Name it exactly **DialogueText** (required for binding).
6. Copy over any styling (font, size, color) from the old Text Block onto the Common Rich Text Block's **Default Text Style** or create a Data Table (below).

## Optional: Create a Style Data Table (for markup tags)

To use markup like `<Red>spicy</>` or `<Italic>creamy</>`:

1. **Create a Data Table**: Right-click in Content Browser → Miscellaneous → Data Table.
2. Choose **Rich Text Style Row** as the row structure.
3. Add rows for each style you want:
   - **Default** – base style for normal text (required)
   - **Red**, **Italic**, **Underline**, **Emphasis**, etc.
4. For each row, set the **Text Style** (font, color, italic, underline brush, etc.).
5. Select your **DialogueText** Common Rich Text Block in the PUDialogueBox Blueprint.
6. In Details, set **Text Styles Set** to your new Data Table.

## Using Markup in Dialogue

In your DlgSystem dialogue nodes, type:

```
Hello! I'd like something <Red>spicy</> and <Italic>creamy</> today.
```

Format: `<StyleName>your text</>` where StyleName matches a row in your Data Table.

## Typewriter Effect

The typewriter advances by visible characters only, so markup tags are never shown as raw text. Styled text appears correctly as it types out.
