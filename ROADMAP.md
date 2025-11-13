# Muse Plugin - Development Roadmap

**Goal**: Build from specification to passing all acceptance tests.

## Milestone Overview

| Milestone | Focus | Deliverable | Tests |
|-----------|-------|-------------|-------|
| M1 | Scaffold | Compiles, params exist | Manual |
| M2 | DSP Core | Formant resonances work | Golden responses (3/10) |
| M3 | Parameters | All params functional | Golden responses (10/10) |
| M4 | Adaptive Gain | RMS normalization | RMS test |
| M5 | UI Layout | Pixel-perfect visuals | Screenshot compare |
| M6 | Polish | 10 FPS, AUTO mode, final validation | All tests pass |

---

## M1: Scaffold & Build System

**Goal**: Compile a minimal plugin with parameter definitions.

### Tasks

1. **CMake setup**
   - Add JUCE 8.0.10 (submodule or FetchContent)
   - Define VST3 target (AU optional)
   - Set C++17/20, enable warnings

2. **Parameter definitions**
   - Create APVTS with 6 parameters (see `PLUGIN_SPEC.md` section 3)
   - Correct IDs, ranges, defaults
   - No DSP logic yet (processBlock = passthrough)

3. **Editor stub**
   - 400×600px window, black background
   - No controls yet (just window size)

### Acceptance

- [ ] Builds on Windows/macOS without errors
- [ ] Loads in DAW (Reaper, Ableton, Logic, etc.)
- [ ] Parameters visible in DAW automation list
- [ ] Audio passes through unmodified

### Git Commit

```
M1: Scaffold - JUCE build system, parameters defined, passthrough audio
```

---

## M2: DSP Core - Basic Formant Filtering

**Goal**: Implement filter that produces formant resonances for one vowel.

### Tasks

1. **Formant filter implementation**
   - Choose topology (Z-plane, filter bank, state-variable, etc.)
   - Implement for ONE vowel (e.g., AA at morph=0.0, intensity=0.5, pair=0)
   - Hard-code formant frequencies for now (no morphing yet)

2. **Parameter wiring**
   - Read `mix` parameter, blend dry/wet
   - Ignore `morph`, `intensity`, `pair` for now (fixed)

3. **RT-safety**
   - No allocations in processBlock
   - NaN/Inf sanitization
   - Denormal suppression (`ScopedNoDenormals`)

### Acceptance

- [ ] `vowel_aa_int50_48k.csv` test passes (±0.5 dB)
- [ ] Audio output sounds resonant (vowel-like character)
- [ ] No clicks, pops, or instability

### Test Command

```bash
python tests/acceptance/test_responses.py --plugin build/Muse.vst3 --case vowel_aa_int50
```

### Git Commit

```
M2: DSP core - formant filter, AA vowel working, 1/10 tests pass
```

---

## M3: Parameter Control - Morph, Intensity, Pairs

**Goal**: All 6 parameters functional, all test cases pass.

### Tasks

1. **Morph implementation**
   - Load formant data for all 5 vowels (AA, AH, EE, OH, OO)
   - Interpolate formant frequencies based on `morph` parameter
   - Handle 3-stage (VOWEL) and 2-stage (BELL, LOW) morphing

2. **Intensity implementation**
   - Map `intensity` (0-1) to filter Q or pole radius
   - Low intensity = wide peaks, high intensity = narrow peaks
   - Clamp to stable range (avoid filter explosion)

3. **Pair selection**
   - Read `pair` parameter (0-3)
   - Select correct vowel set: VOWEL / BELL / LOW / SUB

4. **Parameter smoothing**
   - Smooth `morph`, `intensity`, `mix` over ~20ms
   - No smoothing for `pair` (discrete)

### Acceptance

- [ ] All 10 golden response tests pass (±0.5 dB)
- [ ] Morphing is smooth (no clicks during sweep)
- [ ] Intensity scales Q/bandwidth correctly
- [ ] SUB pair ignores morph (static AH)

### Test Command

```bash
python tests/acceptance/test_responses.py --plugin build/Muse.vst3
```

### Git Commit

```
M3: Parameters - morph, intensity, pair selection, 10/10 tests pass
```

---

## M4: Adaptive Gain & Danger Mode

**Goal**: Implement RMS normalization and danger mode.

### Tasks

1. **RMS measurement**
   - Compute input RMS per block
   - Compute wet output RMS per block

2. **Adaptive gain**
   - Calculate gain = input_RMS / output_RMS
   - Clamp gain to [0.1, 10.0]
   - Smooth gain over 100-200ms window
   - Apply gain to output when `danger=false`

3. **Danger mode**
   - When `danger=true`: skip adaptive gain, apply +3 dB boost

4. **UI feedback**
   - Write RMS level to atomic for LED status
   - Calculate LED state (FLOW / STRUGGLE / MELTDOWN)

### Acceptance

- [ ] Adaptive gain test passes (output RMS ≈ input RMS ±0.5 dB)
- [ ] Danger mode test passes (output ~3 dB louder)
- [ ] No audible pumping or gain artifacts

### Test Command

```bash
python tests/acceptance/test_rms.py --plugin build/Muse.vst3
```

### Git Commit

```
M4: Adaptive gain + danger mode, RMS normalization working
```

---

## M5: UI Layout & Graphics

**Goal**: Pixel-perfect UI matching specification.

### Tasks

1. **Load design tokens**
   - Parse `design/design-tokens.json`
   - Use colors, fonts, layout from tokens (no hardcoding)

2. **Chassis & textures**
   - Fill window with chassis color
   - Pre-render powder-coat texture (1200 dots, deterministic seed 42)
   - Optional: burn marks, scratches (deterministic seed 1993)

3. **LCD panel**
   - Draw beveled panel at (24, 60, 352, 150)
   - Light grey background (#F1F4F5)
   - Inset shadows for depth

4. **Knobs (3×)**
   - Beveled circular knobs at specified positions
   - Radial gradient, inset shadows
   - Mint indicator line
   - Labels above (with OLED glow effect)
   - Values below (1 decimal, monospace)

5. **Buttons**
   - AUTO button (72×22px)
   - DANGER button (90×22px)
   - Toggle states, color changes

6. **Status LED**
   - 12×12px circle at (24, 20)
   - Color based on RMS level
   - Label text ("FLOW" / "STRUGGLE" / "MELTDOWN")

7. **Serial number**
   - "EMU-Z-1993-MUSE" at bottom, 8px mono, mint alpha=0.15

8. **OLED glow**
   - Multi-layer offset shadow technique (NOT Gaussian blur)
   - 8×9 text draws per label (acceptable cost)

### Acceptance

- [ ] Screenshot matches mockup (±2px positions, ±5% colors)
- [ ] All controls at correct positions (use component overlay to verify)
- [ ] Knobs respond to drag, buttons toggle correctly
- [ ] OLED glow visible on text
- [ ] Powder-coat texture visible (subtle grain)

### Git Commit

```
M5: UI layout - pixel-perfect graphics, all controls positioned
```

---

## M6: Polish - 10 FPS Mouth, AUTO Mode, Final Validation

**Goal**: Complete all features, pass all tests.

### Tasks

1. **HalftoneMouth component**
   - Dense dot matrix (100-140 cols × 50-60 rows)
   - Almond lip shape (superellipse or ellipse with tapers)
   - Dark dots (#3B4A52) on light LCD background
   - Edge-weighted dot sizes (larger at edges)

2. **10 FPS update**
   - Editor timer at 60 FPS for knobs
   - Mouth updates every 6th frame (10 FPS)
   - Read audio level, morph, intensity from processor
   - Brightness scales with RMS
   - Shape morphs with parameters

3. **Animation**
   - Breathing (subtle scale ~2%)
   - Transient pulses (scale increase on peaks)
   - Optional: micro-expressions (blink, sigh)
   - Optional: glitch frames (rare dropout)

4. **AUTO mode**
   - Spectral analysis (simple band energy or FFT)
   - Select pair based on content:
     - <80 Hz → SUB
     - 80-150 Hz → LOW
     - 150-300 Hz → BELL
     - >300 Hz → VOWEL
   - Hysteresis (3-6 frames before switching)
   - Update pair badge in UI

5. **Performance optimization**
   - Pre-render powder-coat once (constructor)
   - Mouth render <5ms per frame
   - CPU <5% at 48kHz, 512 samples

6. **State save/load**
   - Test preset save/load
   - Verify deterministic (same params → same sound)

### Acceptance

- [ ] All 10 golden response tests pass
- [ ] RMS normalization test passes
- [ ] AUTO mode test passes (correct pair for known signals)
- [ ] Mouth updates at 10 FPS (visual confirmation - stutter is visible)
- [ ] Screenshot matches mockup
- [ ] No allocations in processBlock (manual code review)
- [ ] Builds cleanly on Windows + macOS
- [ ] Passes pluginval --strictness-level 10 (if available)

### Test Commands

```bash
# All tests
python tests/acceptance/test_responses.py --plugin build/Muse.vst3
python tests/acceptance/test_rms.py --plugin build/Muse.vst3
python tests/acceptance/test_auto.py --plugin build/Muse.vst3

# Pluginval (if available)
pluginval --strictness-level 10 --validate build/Muse.vst3
```

### Git Commit

```
M6: Polish - 10 FPS mouth, AUTO mode, all tests pass
```

---

## Post-M6: Optional Enhancements

**Not required for v1.0, but nice to have:**

- [ ] ARM64 macOS build (Apple Silicon)
- [ ] CLAP format
- [ ] Preset browser UI (basic save/load works via DAW)
- [ ] Additional micro-expressions / glitch effects
- [ ] Advanced chassis corruption (more wear patterns)
- [ ] Accessibility features (screen reader support)

---

## Development Tips

### Milestone Gating

**Do NOT skip milestones.** Each builds on the previous:
- M1 establishes build system (required for M2)
- M2 establishes DSP core (required for M3)
- M3 establishes parameter control (required for M4)
- M4 establishes gain structure (required for M5 LED status)
- M5 establishes UI layout (required for M6 mouth component)

### Commit Hygiene

- One commit per milestone (keep history clean)
- Clear commit messages
- Tag milestones: `git tag M1`, `git tag M2`, etc.

### Testing Cadence

- Run tests after EVERY milestone (don't accumulate failures)
- If a test fails: fix it before moving on
- Use `--plot` flag to visualize differences

### Performance Profiling

- Profile DSP early (M2-M3) to stay under CPU budget
- Profile UI rendering (M5-M6) to hit frame time targets
- Use Xcode Instruments (macOS) or Very Sleepy (Windows)

### Getting Unstuck

- Re-read specs if behavior is unclear
- Check golden responses with `--plot` to see what's wrong
- Ask for clarification (open GitHub issue)

---

**Success = M6 complete, all tests pass. Ship it.**
