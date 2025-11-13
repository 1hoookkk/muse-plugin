# Claude Operating Instructions

**Your mission**: Build a JUCE audio plugin that passes all tests in this repo.

## Core Principles

1. **Outcome-driven**: Match test outputs and visual spec. Internal architecture is your choice.
2. **Test-gated**: Every milestone must pass `tests/acceptance/test_responses.py` before moving on.
3. **RT-safe**: Audio thread NEVER allocates, locks, or touches UI. Read `docs/PLUGIN_SPEC.md` RT rules.
4. **Aesthetic precision**: UI matches `docs/UI_GUIDE.md` exactly. The 10 FPS stutter is SACRED.
5. **Minimal dependencies**: JUCE 8 only. No external DSP libs, no large assets.

## What is Prescriptive vs Flexible

### MUST (Contractual - No Flexibility)

These define the user-visible contract and safety guarantees:

- ✅ **Parameter IDs, ranges, defaults** (`docs/PLUGIN_SPEC.md` section 3)
- ✅ **UI layout** (400×600px, exact component bounds, `docs/UI_GUIDE.md`)
- ✅ **10 FPS mouth update** (not 60 FPS - this is the aesthetic)
- ✅ **RT-safety rules** (no allocs/locks on audio thread)
- ✅ **Acceptance tests** (golden responses within tolerance)
- ✅ **Visual aesthetic** (colors, fonts, "haunted hardware" look)

### SHOULD (Outcome-Based - Test What, Not How)

Implementation must achieve these outcomes (tests verify):

- ✅ **Adaptive gain**: Output RMS tracks input RMS within ±0.5 dB (test measures this)
- ✅ **AUTO mode**: Detects low/mid/high content and selects pair (test feeds known signals)
- ✅ **Status LED**: Matches visual states in reference screenshots (test compares pixels)
- ✅ **Smooth parameter changes**: No audible clicks/zippers (test automates and listens)

### MAY (Implementation Freedom)

You choose the internals, as long as tests pass:

- ✅ **Filter topology**: Z-plane cascade, biquad, state-variable, etc. (just match golden responses)
- ✅ **Rendering technique**: CPU dots, cached image, GPU shader (just hit 10 FPS + visuals)
- ✅ **Coefficient update strategy**: Per-block, per-buffer (just avoid zippers)
- ✅ **Pole interpolation**: Linear, spline, lookup table (just match magnitude curves)

**Bottom line**: If golden responses match ±0.5 dB and UI looks right, internals don't matter.

## Workflow

### Start Here
```bash
# 1. Read specifications (in this order)
cat docs/PLUGIN_SPEC.md      # Parameters, formats, RT constraints
cat docs/DSP_OVERVIEW.md      # Z-plane theory (equations, not code)
cat docs/RESPONSE_CONSTRAINTS.md  # Acceptance windows
cat docs/UI_GUIDE.md          # Layout, aesthetic, 10 FPS rule
cat ROADMAP.md                # Milestone order

# 2. Verify skeleton builds
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# 3. Run tests (will fail - that's expected)
python tests/acceptance/test_responses.py --plugin build/Muse_artefacts/Release/VST3/Muse.vst3
```

### Milestone Loop (M1 → M6)

For each milestone in `ROADMAP.md`:

1. **Implement** the milestone's scope (src/*.cpp, src/dsp/*.cpp)
2. **Build** without warnings: `cmake --build build --config Release`
3. **Test** acceptance: `python tests/acceptance/test_responses.py`
4. **Commit** when tests pass: `git add . && git commit -m "M2: Z-plane scaffold + morph"`
5. **Next** milestone

**Do NOT skip ahead**. Milestones are ordered for dependency reasons.

## Contractual Requirements (MUST)

### Parameters (see `docs/PLUGIN_SPEC.md`)

```cpp
// Exact IDs, ranges, defaults (do NOT change these)
"morph"     : [0.0, 1.0], default 0.5  // smoothed
"intensity" : [0.0, 1.0], default 0.5  // smoothed
"mix"       : [0.0, 1.0], default 0.5  // smoothed
"pair"      : [0, 3],     default 0    // discrete (no smoothing)
"auto"      : bool,       default false
"danger"    : bool,       default false
```

### RT-Safety (see `docs/PLUGIN_SPEC.md` section 5)

- **No allocations** in `processBlock()` (heap, vector resize, string ops)
- **No locks** (mutex, spinlock, even "fast" locks)
- **No UI calls** from audio thread
- **NaN/Inf sanitization**: Check input and internal state, replace with 0.0f
- **Denormal suppression**: Use `ScopedNoDenormals` or equivalent
- **Bounded time**: No unbounded loops or recursion

### UI Layout (see `docs/UI_GUIDE.md`)

- **Window**: 400×600px, fixed (not resizable)
- **Colors**: Load from `design/design-tokens.json` (do NOT hardcode hex values)
- **Component bounds**: Exact pixel positions in UI guide section 3
- **10 FPS mouth**: HalftoneMouth updates at 10 Hz (NOT 60 Hz)
- **Fonts**: Use design tokens for sizes/weights
- **Aesthetic**: "Haunted hardware" look (powder-coat, wear, OLED glow)

### Acceptance Tests (see `docs/RESPONSE_CONSTRAINTS.md`)

- **Golden responses**: Match `data/golden-responses/*.csv` within ±0.5 dB per bin
- **Tolerance relaxed**: ±1.0 dB for bins >10kHz (numerical noise acceptable)
- **Coverage**: 10 test cases (VOWEL/BELL pairs, various morph/intensity values)
- **Test script**: `python tests/acceptance/test_responses.py --plugin <path>`

## Outcome Requirements (SHOULD)

These specify *what* must happen, not *how* you implement it:

### DSP Behavior

- **Formant resonances**: Magnitude response shows clear formant peaks matching test CSVs
- **Morph transitions**: Smooth frequency response change as MORPH varies (no clicks)
- **Intensity effect**: Higher intensity → higher Q / narrower peaks (visible in magnitude)
- **Stability**: Filter never explodes (output stays bounded for any input)
- **Adaptive gain**: When DANGER=false, output RMS ≈ input RMS ±0.5 dB (test measures this)
- **Danger mode**: When DANGER=true, output ~3 dB louder than adaptive mode

### AUTO Mode

- **Content detection**: Test feeds known signals (sine sweeps, noise bands)
- **Expected behavior**:
  - Sub-bass (<80 Hz energy) → SUB pair selected
  - Low (80-150 Hz) → LOW pair
  - Mid (150-300 Hz) → BELL pair
  - High (>300 Hz) → VOWEL pair
- **Switching**: Pair badge updates to show detected pair name

### Visual Behavior

- **Status LED**: Color matches audio state (test compares screenshots)
  - Low level → green (FLOW)
  - Medium level → amber (STRUGGLE)
  - High level → red (MELTDOWN)
- **Mouth animation**: Reacts to audio (brighter = louder) and morph (shape changes)
- **10 FPS cadence**: Mouth updates every ~100ms (visible stutter, not smooth)

## Common Pitfalls (Avoid These)

### ❌ DSP Mistakes

1. **Updating coefficients per-sample** → Kills CPU, causes zipper noise
   - ✅ Update once per block, use previous coefficients for entire block

2. **Not sanitizing filter state** → NaN propagation, silent output
   - ✅ Check `std::isnan(state[i])` after recursion, reset to 0.0f if true

3. **Poles outside unit circle** → Filter explodes
   - ✅ Clamp `|p| ≤ 0.97` after interpolation

4. **Using std::pow() in processBlock** → Not RT-safe on some platforms
   - ✅ Pre-compute or use lookup tables

### ❌ UI Mistakes

1. **Smooth 60 FPS mouth updates** → WRONG, spec requires 10 FPS stutter
   - ✅ Use separate Timer at 10 FPS for HalftoneMouth

2. **Per-frame texture generation** → Kills performance
   - ✅ Pre-render powder-coat and corruption layers in constructor

3. **Hardcoded colors** → Fragile, wrong aesthetic
   - ✅ Load from `design/design-tokens.json`

4. **Naive OLED glow (Gaussian blur)** → Too expensive
   - ✅ Use offset shadow technique (see `docs/UI_GUIDE.md` section 5.2)

### ❌ Testing Mistakes

1. **Ignoring test failures** → Plugin won't match spec
   - ✅ If test fails, check magnitude plot: `--plot` flag shows difference

2. **Tweaking tolerance to pass** → Cheating
   - ✅ Fix implementation to match golden responses

3. **Not testing at multiple sample rates** → SR-dependent bugs
   - ✅ Script tests 44.1k, 48k, 96k automatically

## Reference Approach (Non-Binding)

**You are FREE to use any architecture that passes tests.** This section shows *one* viable approach.

### Option A: Z-Plane Cascade (All-Pole Resonator)

A common implementation uses 6 biquad sections (12 poles total) positioned in the Z-plane:

```cpp
// Convert formant (freq, BW) to pole (r, θ)
float freqToTheta(float freq, float sampleRate) {
    return juce::MathConstants<float>::twoPi * freq / sampleRate;
}
float bwToRadius(float bandwidth, float sampleRate) {
    return 1.0f - (juce::MathConstants<float>::pi * bandwidth / sampleRate);
}

// Each biquad implements conjugate pole pair
// H(z) = 1 / (1 - 2r·cos(θ)·z⁻¹ + r²·z⁻²)
void BiquadSection::setCoefficients(float r, float theta) {
    a1_ = -2.0f * r * std::cos(theta);
    a2_ = r * r;
}

float BiquadSection::processSample(float x) {
    float y = x - a1_ * z1_ - a2_ * z2_;
    z2_ = z1_;
    z1_ = y;
    // Sanitize if unstable
    if (std::isnan(y) || std::isinf(y)) { z1_ = z2_ = y = 0.0f; }
    return y;
}
```

**Vowel morphing**: Interpolate pole positions (r, θ) between target vowel shapes.

### Option B: Formant Filter Bank

Alternatively, use parallel formant filters (F1/F2/F3) with bandwidth control:

- Each formant = bandpass or resonant filter
- MORPH adjusts formant frequencies
- INTENSITY adjusts Q / bandwidth
- Sum outputs and normalize

### Option C: State-Variable or Other Topology

Any filter topology that produces formant-like peaks is acceptable, including:
- State-variable filters with frequency/Q modulation
- Parametric EQ bank (3-6 bands)
- Physical model (digital waveguide, modal synthesis)

**The ONLY requirement**: Match golden magnitude responses within ±0.5 dB.

## When You Get Stuck

### If tests fail:

```bash
# 1. Visualize the difference
python tests/acceptance/test_responses.py --plugin <path> --plot --case vowel_aa_int50

# 2. Check pole positions (add debug print)
# Expected: 6 pole pairs forming formant peaks at F1, F2, F3

# 3. Verify coefficient math
# Use MATLAB/Python to compute expected biquad coefficients offline
```

### If build fails:

```bash
# 1. Check JUCE version
cmake --version  # Need 3.24+
# JUCE 8.0.10 should be auto-fetched by CMakeLists.txt

# 2. Clean rebuild
rm -rf build && cmake -S . -B build && cmake --build build
```

### If UI doesn't match spec:

```bash
# 1. Check design tokens loaded
cat design/design-tokens.json

# 2. Verify component bounds
# Use JUCE's component overlay (Cmd+F12 in plugin window)

# 3. Capture screenshot, compare to docs/UI_GUIDE.md mockup
```

## Acceptance Checklist

Before marking a milestone complete:

- [ ] Code compiles without warnings (`-Wall -Wextra` on Clang/GCC)
- [ ] `test_responses.py` passes for relevant test cases
- [ ] No allocations in `processBlock()` (manually verified)
- [ ] UI matches spec (screenshot compare)
- [ ] Git commit with clear message

## Communication

**Ask questions if specs are ambiguous**, but prefer:
- ✅ "Spec says X, but Y is unclear - which interpretation?"
- ❌ "I think it should work like Z" (implement spec, not intuition)

**When reporting progress**:
- ✅ "M2 complete: 10/10 test cases pass, poles stable, UI scaffold done"
- ❌ "Made some progress on DSP"

## Final Reminder

**The 10 FPS mouth stutter is NOT a bug. It is the DESIGN.**

If you find yourself thinking "this should be 60 FPS" → you're wrong.
If you find yourself thinking "let me add a feature not in spec" → don't.
If you find yourself thinking "I'll skip this test for now" → stop and fix it.

**Ship what the spec describes. Nothing more, nothing less.**

---

**When you're ready**: Start with `ROADMAP.md` → Milestone 1.
