# UI Guide - Layout, Aesthetic, and Behavior

**Aesthetic**: 1990s hardware sampler (E-mu, Akai) with "haunted" character.

## 1. Window Specifications

- **Size**: 400×600 pixels (fixed, not resizable)
- **Background**: Chassis color (see design tokens)
- **Style**: Skeuomorphic hardware (beveled knobs, inset LCD, powder-coat texture)

## 2. Color Palette & Fonts

**DO NOT hardcode colors or fonts.** Load from `design/design-tokens.json`.

See that file for:
- Chassis colors
- LCD background
- Mint OLED color
- Knob gradients
- Status LED colors
- Font families, sizes, weights

## 3. Component Layout (Exact Positions)

All positions in pixels (x, y, width, height):

```
LCD Panel:         24,  60, 352, 150
  └─ HalftoneMouth: 32,  68, 336, 134  (inside LCD, 8px padding)

Title "MUSE":       0,  24, 400,  20  (centered)
Status LED:        24,  20,  12,  12
LED Label:         44,  14, 120,  24  (text: "FLOW" / "STRUGGLE" / "MELTDOWN")

MORPH Knob:        90, 250,  72,  72
INTENSITY Knob:   238, 250,  72,  72
MIX Knob:         164, 400,  72,  72

FREEZE Button:    164, 220,  72,  22
DANGER Button:    260, 220,  90,  22
Pair Badge:       164, 245,  72,  14  (shows "VOWEL" / "BELL" / "LOW" / "SUB")

Serial Number:      0, 560, 400,  12  (centered, text: "EMU-Z-1993-MUSE")
```

## 4. LCD Display Panel

### Visual Style

- **Background**: Light cool grey (#F1F4F5 from design tokens)
- **Border**: Beveled 3D inset effect
  - Outer highlight (top-left): light, catching virtual light
  - Inner shadow (bottom-right): dark, recessed look
  - Deep inner shadow: black alpha=0.7, 1px inset for "glass" depth

### HalftoneMouth Visualization

**CRITICAL: 10 FPS Update Cadence (NOT 60 FPS)**

This is **not a bug or oversight**. The 10 FPS stutter is the design.

#### Update Timing

```cpp
// Editor has 60 FPS timer for smooth knob interaction
void timerCallback() {
    repaint();  // Knobs at 60 FPS

    // Mouth updates at 10 FPS (every 6 frames)
    if (++frameCounter_ % 6 == 0) {
        updateMouth();  // Read audio level, morph, intensity
        mouth_.triggerUpdate();  // Repaints mouth at 10 FPS
    }
}
```

**Why 10 FPS**:
- Vintage hardware aesthetic (VFD displays, early OLED)
- "Haunted" character (intentional jank)
- Reduces CPU usage for complex dot rendering

**If you smooth this to 60 FPS, you have failed the aesthetic requirement.**

#### Visual Characteristics

**Shape**: Filled almond lip (no teeth, no inner hole)
- Superellipse or ellipse with horizontal/vertical tapers
- Width ≈ 48% of display width
- Height ≈ 33% of display height (varies with intensity)
- Morphs shape with `morph` parameter (wider/narrower, taller/shorter)

**Rendering**: Dense halftone dot matrix
- Dot color: Dark steel grey (#3B4A52 from design tokens)
- Background: Light LCD grey (matches panel)
- Density: 100-140 columns × 50-60 rows (adjust for performance)
- Dot size: Variable (larger near edges, smaller in center for depth)
- Style: Crisp, print-like (no glow, no blur)

**Animation** (at 10 FPS):
- Brightness: Scales with audio RMS (dim when quiet, bright when loud)
- Breathing: Subtle scale oscillation (~2% amplitude, slow)
- Transient pulses: Brief scale increase on sharp transients
- Optional: Rare micro-expressions (blink, sigh) every 8-10 seconds
- Optional: Rare glitch frames (dropout) every 30 seconds

**Performance**: Render in <5ms per frame (50% of 10 FPS budget).

#### Implementation Freedom

**You MAY use**:
- CPU-based dot rendering (Graphics::fillEllipse per dot)
- Pre-rendered image cache (update only when parameters change)
- GPU shader (if you can keep 10 FPS aesthetic)

**You MUST match**:
- Visual appearance (screenshot comparison)
- 10 FPS update rate
- Performance budget (<5ms render time)

## 5. Knobs (3× 72×72px)

### Visual Style

**Layers** (bottom to top):
1. Drop shadow: Black alpha=0.5, 1px offset
2. Outer bezel: Radial gradient (light to dark), inset shadows
3. Center circle: Flat chassis color (80% of outer diameter)
4. Mint indicator line: 2px × 12px, starts 6px from center

**Beveled effect**:
- Top-left: lighter (catching light)
- Bottom-right: darker (recessed)
- Inset shadows create 3D depth

**Wear simulation** (optional but recommended):
- Deterministic per-knob scratches (3-6 thin lines)
- Center darkening (simulates finger wear)
- Mechanical wobble (±0.4px random offset, deterministic seed per knob)

### Interaction

- **Drag**: Vertical drag only (mouse down + move up/down)
- **Sensitivity**: 150px vertical travel = full range (0.0 - 1.0)
- **Velocity-based**: Slower drag = finer control
- **Double-click**: Reset to 0.5 (center)
- **Scroll wheel**: Enabled
- **Value tooltip**: Show on drag (optional)

### Labels & Values

**Label** (above knob):
- Font: Sans-serif semibold, 14px, wide letter-spacing
- Color: Mint with OLED glow effect
- Text: "MORPH" / "INTENSITY" / "MIX"

**Value** (below knob):
- Font: Monospace, 12px
- Color: Mint with OLED glow
- Format: "0.0" - "1.0" (1 decimal place)

## 6. OLED Glow Effect

**DO NOT use naive Gaussian blur** (too expensive).

**Use multi-layer offset shadows**:

```cpp
// Pseudo-code for glow text
void drawOLEDText(Graphics& g, String text, Rectangle area) {
    Colour mint = designTokens["oled_mint"];

    // Outer glow (8px radius, very subtle)
    g.setColour(mint.withAlpha(0.15f));
    g.drawText(text, area.translated(-2, -2));
    g.drawText(text, area.translated(2, 2));
    g.drawText(text, area.translated(-2, 2));
    g.drawText(text, area.translated(2, -2));

    // Middle glow (5px)
    g.setColour(mint.withAlpha(0.25f));
    g.drawText(text, area.translated(-1, -1));
    g.drawText(text, area.translated(1, 1));
    g.drawText(text, area.translated(-1, 1));
    g.drawText(text, area.translated(1, -1));

    // Inner glow (2px)
    g.setColour(mint.withAlpha(0.35f));
    g.drawText(text, area.translated(0, -1));
    g.drawText(text, area.translated(0, 1));
    g.drawText(text, area.translated(-1, 0));
    g.drawText(text, area.translated(1, 0));

    // Core text (bright)
    g.setColour(mint);
    g.drawText(text, area);
}
```

**Cost**: ~9× text draws per label (acceptable for 3 knobs + 2 labels).

## 7. Status LED (Top-Left)

### Visual

- **Circle**: 12×12px at (24, 20)
- **Glow halo**: 4px expansion, alpha=0.25
- **Colors** (from design tokens):
  - FLOW: Green (#42D697)
  - STRUGGLE: Amber (#E8BF3D)
  - MELTDOWN: Red (#DB3F3F)

### Behavior

- Audio thread writes RMS level to atomic: `std::atomic<float> rmsLevel_`
- UI thread reads level in timer (10 FPS sufficient)
- Thresholds (example, adjust to match reference screenshots):
  - RMS < 0.5 (-6 dBFS) → FLOW
  - 0.5 ≤ RMS < 0.85 (-1.5 dBFS) → STRUGGLE
  - RMS ≥ 0.85 → MELTDOWN

### Label

- Font: Sans-serif bold, 11px
- Color: Mint alpha=0.85
- Text: "FLOW" / "STRUGGLE" / "MELTDOWN" (matches LED state)

## 8. Buttons (FREEZE, DANGER)

### Style

- **Size**: FREEZE=72×22px, DANGER=90×22px
- **Colors**: Load from design tokens
  - Off: Dark background, mint text alpha=0.5
  - On: Colored background (green for FREEZE, red for DANGER), white text
- **Toggle**: Single click toggles state
- **Visual feedback**: Background color changes instantly

### FREEZE Button

- **Label**: "FREEZE" (maps to `auto` parameter ID internally)
- **When OFF**: Live input passes through with envelope shaping
- **When ON**: Spectrum is captured and held
  - Onset (off→on): Captures current spectrum, smooth crossfade (~50ms)
  - Hold: Output sustains frozen + shaped spectrum
  - Release (on→off): Crossfade back to live input (~100ms)
- **Behavior**: Click during audio to freeze that moment's spectrum
- **Pair badge**: Shows currently selected pair ("VOWEL", "BELL", "LOW", "SUB")

### DANGER Button

- When ON: "DANGER ACTIVE" warning text appears near status LED (red, 10px font)

## 9. Powder-Coat Texture

**Purpose**: Simulate matte powder-coat chassis finish.

**Implementation** (recommended):

```cpp
// Pre-render once in constructor (95% faster than per-frame)
Image cachedTexture = Image(ARGB, 400, 600, true);
Graphics g(cachedTexture);

Random random(42);  // Fixed seed for consistency
for (int i = 0; i < 1200; ++i) {
    float x = random.nextFloat() * 400;
    float y = random.nextFloat() * 600;
    float alpha = random.nextFloat() * 0.04f;  // Very subtle
    g.setColour(Colours::white.withAlpha(alpha));
    g.fillRect(x, y, 1.0f, 1.0f);
}

// In paint():
g.drawImageAt(cachedTexture, 0, 0);
```

**Optional**: Add burn marks, scratches, wear zones (see `PluginEditor.cpp` in reference repo for algorithm).

## 10. Chassis Corruption (Optional)

**Aesthetic enhancement**: Simulates aged hardware.

- **Burn marks**: 3-5 irregular dark spots (overlapping circles)
- **Scratches**: 8-12 thin random lines
- **Knob wear**: Subtle darkening around knob centers

**Implementation**: Pre-render in constructor (deterministic seed 1993 for consistency).

## 11. Serial Number Badge

- **Position**: Bottom center (0, 560, 400, 12)
- **Font**: Monospace, 8px
- **Color**: Mint alpha=0.15 (very faded)
- **Text**: "EMU-Z-1993-MUSE"

## 12. Interaction Guidelines

### Knob Dragging

- **Feel**: Velocity-based (slower drag = finer control)
- **Sensitivity**: Adjustable (150px full range is good starting point)
- **Visual feedback**: Value updates in real-time, knob indicator rotates smoothly

### Button Clicking

- **Response**: Instant toggle (no animation delay)
- **Visual feedback**: Background color change
- **Parameter update**: Linked to APVTS, automatable

### Keyboard Focus

- **Disabled**: Knobs should NOT accept keyboard focus (prevents spacebar conflicts in DAWs)
- **Interaction**: Mouse-only

## 13. Performance Targets

- **Full repaint**: <16ms (60 FPS capable for knobs)
- **Mouth update**: <5ms (10 FPS, 50% budget)
- **Idle CPU**: <0.1% when no audio playing

## 14. Accessibility Notes

- **High contrast**: Mint on dark chassis (WCAG AAA)
- **Large hit targets**: Knobs are 72×72px (easy to grab)
- **Clear labels**: All controls labeled in readable font
- **No critical color-only info**: LED state also has text label

## 15. Screenshot Comparison

**Final validation**: Capture screenshot of running plugin, compare to mockup.

**Acceptable differences**:
- Font rendering (platform-specific antialiasing)
- Minor color variation (<5% RGB delta)

**Unacceptable differences**:
- Wrong layout (component positions off by >2px)
- Wrong update rate (mouth at 60 FPS instead of 10 FPS)
- Missing effects (no OLED glow, no powder-coat texture)

---

**The 10 FPS mouth is SACRED. Do not smooth it. This is the design.**
