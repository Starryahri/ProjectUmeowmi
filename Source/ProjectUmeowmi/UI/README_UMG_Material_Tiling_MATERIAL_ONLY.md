# UMG Material Tiling - MATERIAL ONLY Solution (No Code!)

## The Problem
Your 512x512 tileable texture with a panner is being stretched. You want to fix it **purely in the Material Editor** with no code.

## The Solution: Use ScreenPosition + ViewSize

Since you said ScreenPosition was "closer", we can make it work by converting it properly!

### Material Setup (Material Editor Only)

1. **Add `ScreenPosition` node**
   - This gives you normalized screen coordinates (0-1 range)

2. **Add `ViewSize` node** (or `ScreenSize`)
   - Right-click → Search "ViewSize" or "ScreenSize"
   - This gives you the viewport/screen resolution in pixels (e.g., 1920x1080)
   - If ViewSize doesn't exist, try `ScreenSize` or `ViewportSize`

3. **Convert to Pixel Coordinates**
   - Add a **Multiply** node
   - Connect `ScreenPosition` → **A**
   - Connect `ViewSize` → **B**
   - This converts normalized coords to pixel coordinates

4. **Calculate Tiling**
   - Add a **Divide** node
   - Connect the **Multiply** output → **A**
   - Add a **Constant2Vector** with value (512, 512) → **B**
   - This gives you: (PixelCoords / TextureSize) = tiling factor

5. **Add Your Panner**
   - Connect the **Divide** output → **Coordinate** input of your Panner
   - Keep your existing panner speed settings

6. **Connect to Texture Sample**
   - Connect **Panner** output → **UVs** input of texture sample

### Node Flow:
```
ScreenPosition
    ↓
Multiply ← ViewSize (or ScreenSize)
    ↓
Divide ← Constant2Vector(512, 512)
    ↓
Panner
    ↓
Texture Sample → Base Color
```

### Alternative: If ViewSize Doesn't Work

If `ViewSize` or `ScreenSize` nodes don't exist or don't work:

**Option 1: Use PixelSize**
1. Add `PixelSize` node (gives size of one pixel in UV space)
2. Add `Divide`: `Constant(1.0) ÷ PixelSize` = pixels per unit
3. Multiply `ScreenPosition × (1.0 ÷ PixelSize)` to get pixel coords
4. Then divide by texture size (512)

**Option 2: Hardcode Screen Resolution**
1. Use `ScreenPosition`
2. Multiply by a **Constant2Vector** with your target screen resolution (e.g., 1920, 1080)
3. Divide by texture size (512, 512)
4. This works but assumes a fixed screen size

**Option 3: Use Absolute World Position (if available)**
- Some versions have `AbsoluteWorldPosition` that can be converted
- Multiply by a scale factor to get pixel coordinates

### Why This Works

- `ScreenPosition` gives you where on the screen (0-1)
- `ViewSize` gives you screen resolution in pixels
- Multiplying converts to pixel coordinates
- Dividing by texture size gives proper tiling
- Result: Texture tiles at its native 512x512 pixel size!

### Important Notes

- **Material Domain**: Must be **User Interface**
- **Blend Mode**: Translucent or Opaque
- **Shading Model**: Unlit

### Troubleshooting

**Still stretched?**
- Try `ScreenSize` instead of `ViewSize`
- Try `ViewportSize` instead of `ViewSize`
- Check that you're dividing: `(ScreenPosition × ViewSize) ÷ TextureSize`
- Make sure the order is: ScreenPosition → Multiply → Divide → Panner

**Can't find ViewSize/ScreenSize?**
- Use Option 1 with PixelSize
- Or Option 2 with hardcoded resolution
- Search in Material Editor for "View", "Screen", "Size" nodes

