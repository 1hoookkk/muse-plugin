# Muse Plugin Specification

## 1. Plugin Metadata

- **Name**: Muse
- **Vendor**: [Your Company]
- **Version**: 1.0.0
- **Category**: Effect / Filter / Creative
- **Formats**: VST3, AU, Standalone (VST3 minimum required)
- **Unique ID**: `MuseVowel` (JUCE plugin code)

## 2. Audio Specifications

### Channel Configuration

- **Input**: Mono, Stereo (1-2 channels)
- **Output**: Matches input (1-2 channels)
- **Processing**: Stereo-linked (both channels use same filter state, independent processing)

### Sample Rates

- **Supported**: 44.1 kHz, 48 kHz, 88.2 kHz, 96 kHz, 176.4 kHz, 192 kHz
- **Default**: Host sample rate
- **Behavior**: Coefficients recalculated on SR change (no clicks/pops)

### Buffer Sizes

- **Supported**: 16 - 8192 samples
- **Default**: Host buffer size
- **Behavior**: Block-size agnostic (no assumptions about buffer length)

### Latency

- **Reported**: Implementation-dependent (STFT hop size)
- **Typical**: 512-1024 samples (10-21ms at 48kHz)
- **Acceptable range**: 256-2048 samples
- **Behavior**: Report actual latency via `getLatencySamples()` for DAW compensation

## 3. Parameters

All parameters use JUCE's `AudioProcessorValueTreeState` for automation and state management.

### Parameter Definitions

| ID | Name | Type | Range | Default | Smoothed | Units | Taper |
|----|------|------|-------|---------|----------|-------|-------|
| `morph` | Morph | Float | [0.0, 1.0] | 0.5 | Yes | Normalized | Linear |
| `intensity` | Intensity | Float | [0.0, 1.0] | 0.5 | Yes | Normalized | Linear |
| `mix` | Mix | Float | [0.0, 1.0] | 0.5 | Yes | Normalized | Linear |
| `pair` | Pair | Int | [0, 3] | 0 | **No** | Index | Step |
| `auto` | AUTO | Bool | [0, 1] | 0 | No | On/Off | Step |
| `danger` | DANGER | Bool | [0, 1] | 0 | No | On/Off | Step |

### Parameter Semantics

**`morph`** (0.0 - 1.0):
- Controls interpolation between spectral envelope shapes within the selected preset
- 0.0 = first vowel shape, 1.0 = last vowel shape
- VOWEL preset (3-stage): 0-0.5 = AA→AH, 0.5-1.0 = AH→EE
- BELL preset (2-stage): 0.0 = OH, 1.0 = OO
- LOW preset (2-stage): 0.0 = AA, 1.0 = OO
- SUB preset (static): morph ignored, always AH
- Envelope is applied to frozen spectrum as spectral multiplier

**`intensity`** (0.0 - 1.0):
- Controls envelope peak sharpness (spectral Q)
- 0.0 = wide, gentle peaks (subtle shaping)
- 1.0 = narrow, sharp peaks (aggressive shaping)
- Affects bandwidth of formant-like envelope peaks
- Must remain stable at all values

**`mix`** (0.0 - 1.0):
- Dry/wet blend
- 0.0 = 100% dry (bypassed)
- 1.0 = 100% wet (fully processed - frozen + shaped spectrum)
- Equal-power crossfade preferred (no volume dip at 0.5)

**`pair`** (0, 1, 2, 3):
- Selects spectral envelope preset:
  - 0 = VOWEL (AA → AH → EE) - bright, evolving
  - 1 = BELL (OH → OO) - warm, round
  - 2 = LOW (AA → OO) - sub-focused
  - 3 = SUB (AH static) - low-frequency emphasis
- Discrete (no smoothing between preset switches)
- Can cause brief transient if switched during freeze (acceptable)

**`auto`** (boolean) - **FREEZE control**:
- **UI label**: "FREEZE" (not "AUTO")
- When false: Live input passes through (with optional envelope shaping)
- When true: Spectrum is captured and held
  - Onset (off→on): Capture current spectrum, crossfade in over ~50ms
  - Hold: Output frozen + shaped spectrum
  - Release (on→off): Crossfade back to live input over ~100ms
- Clicking during audio captures that moment's spectrum

**`danger`** (boolean):
- When true: Bypasses adaptive makeup gain, applies +3 dB fixed boost
- When false: Adaptive makeup gain active (output RMS ≈ input RMS)
- Visual feedback: "DANGER ACTIVE" warning in UI

### Parameter Smoothing

**Smoothed parameters** (`morph`, `intensity`, `mix`):
- Smoothing time: ~20ms (calculate ramp length from sample rate)
- Update rate: Per-block (not per-sample) for efficiency
- Smoothing prevents zipper noise / audible steps

**Discrete parameters** (`pair`, `auto`, `danger`):
- No smoothing (instant change)
- `pair` switching may cause click (acceptable design choice)

## 4. DSP Requirements (Outcome-Based)

### Spectral Freeze + Envelope Shaping

**Target behavior** (verified by acceptance tests):

- **Freeze capture**: When FREEZE pressed, spectrum is captured and held smoothly
- **Envelope peaks**: Spectral envelope has clear formant-like peaks (vowel character)
- **Morph continuity**: Smooth envelope change as `morph` varies (no discontinuities)
- **Intensity effect**: Higher `intensity` → narrower, sharper envelope peaks
- **Preset differences**: Each preset produces distinct envelope patterns (see `RESPONSE_CONSTRAINTS.md`)
- **Clickless transitions**: Freeze on/off produces no audible pops or clicks

**Golden responses** (`data/golden-responses/*.csv`):
- 10 test cases: frozen white noise + various envelope presets/morph/intensity values
- Plugin magnitude response must match within ±1.0 dB per frequency bin
- Tolerance relaxed to ±1.5 dB for bins >10 kHz (STFT numerical noise acceptable)

### Stability

**Requirement**: STFT processing remains stable for ALL parameter combinations and input signals.

- No output explosion (unbounded growth)
- No NaN or Inf propagation
- Graceful handling of extreme inputs (silence, DC, full-scale sine, noise)
- Rapid freeze on/off toggling does not cause instability

**Implementation** (flexible):
- FFT bin sanitization (check for NaN/Inf after FFT)
- State reset on detected issues
- Crossfade to silence if corruption detected (acceptable transient)

### Adaptive Makeup Gain

**Requirement** (when `danger` = false):
- Output RMS level approximately matches input RMS level
- Tolerance: ±0.5 dB over 100ms window
- Purpose: Prevent perceived volume jump when enabling effect

**Implementation** (flexible):
- Per-block RMS measurement and gain adjustment
- Windowed moving average (100-200ms)
- Clamp gain to reasonable range (e.g., [0.1, 10.0] to prevent extreme corrections)

**When `danger` = true**:
- Bypass adaptive gain entirely
- Apply fixed +3 dB boost (linear gain ≈ 1.41×)

## 5. Real-Time Safety Rules (Mandatory)

These rules are **contractual** and will be verified manually:

### Audio Thread Constraints

The `processBlock()` method and all functions it calls **MUST NOT**:

- ❌ Allocate heap memory (`new`, `malloc`, `std::vector::push_back`, `std::string` ops)
- ❌ Use locks (`std::mutex`, `std::atomic` with sequentially-consistent ordering, spinlocks)
- ❌ Touch UI components or call repaint()
- ❌ Perform file I/O or network I/O
- ❌ Call non-RT-safe library functions (`std::pow` on some platforms, `printf`, logging with allocation)
- ❌ Use unbounded loops or recursion

### Audio Thread Practices

The `processBlock()` method **MUST**:

- ✅ Have bounded worst-case execution time
- ✅ Sanitize input (check for NaN/Inf, replace with 0.0f)
- ✅ Sanitize filter state after recursion (check for NaN/Inf)
- ✅ Enable denormal suppression (`ScopedNoDenormals` or FTZ/DAZ flags)
- ✅ Read parameters via atomics or lock-free structures

### Communication: Audio Thread → UI Thread

Use atomics or JUCE's `AbstractFifo` / `AudioProcessorValueTreeState`:

```cpp
// Audio thread writes
std::atomic<float> audioLevel_;
audioLevel_.store(rmsLevel, std::memory_order_relaxed);

// UI thread reads (in timer callback)
float level = audioLevel_.load(std::memory_order_relaxed);
```

## 6. State Management

### Preset Save/Load

- **Method**: JUCE's `getStateInformation()` / `setStateInformation()`
- **Format**: XML via `AudioProcessorValueTreeState::copyState()`
- **Stability**: Same parameter values → identical sound (deterministic)
- **Compatibility**: Forward-compatible (new versions load old presets)

### Parameter Recall

- All 6 parameters saved in state
- On load: parameters update, DSP recalculates coefficients
- No clicks/pops on preset load (use parameter smoothing)

## 7. Performance Targets

### CPU Usage

- **Target**: <5% single core at 48 kHz, 512 samples/block (modern CPU ~3 GHz)
- **Stretch**: <2% with optimizations
- **Test machine**: Windows 10, Intel i7 or equivalent

### Memory

- **Heap**: < 1 MB for plugin instance (including UI)
- **Stack**: < 100 KB per `processBlock()` call

### UI Rendering

- **Mouth visualization**: < 5ms per frame at 10 FPS (50% of frame budget)
- **Full repaint**: < 16ms (60 FPS capable for knob interaction)

## 8. Build Requirements

### Framework

- **JUCE**: 8.0.10 (latest stable)
- **Modules**: `audio_processors`, `audio_utils`, `dsp`, `graphics`, `gui_basics`

### Language

- **C++17** minimum (prefer C++20 if JUCE supports)
- Compile with warnings enabled (`-Wall -Wextra` on GCC/Clang, `/W4` on MSVC)

### Platforms

- **Windows**: MSVC 2019+ (x64)
- **macOS**: Xcode 13+ (x64, ARM64)

### Validation

- Pass `pluginval --strictness-level 10` (if available)
- No crashes, no validation errors, no buffer overruns

## 9. Testing Requirements

See `RESPONSE_CONSTRAINTS.md` for detailed acceptance criteria.

### Automated Tests

- **Frequency response**: Match golden CSVs within tolerance
- **RMS normalization**: Adaptive gain test (sine sweep, measure output RMS)
- **AUTO mode**: Feed known signals, verify pair selection
- **State save/load**: Save, modify, load, verify parameters restored

### Manual Tests

- **RT-safety**: Visual inspection of `processBlock()` code (no allocs/locks)
- **UI layout**: Screenshot compare against mockups (pixel-perfect)
- **10 FPS aesthetic**: Visual confirmation of mouth stutter (not smooth 60 FPS)

## 10. Out of Scope

These features are **explicitly excluded** from v1.0:

- Preset browser UI (basic save/load via DAW only)
- MIDI learn or modulation
- Oversampling (keep 1× for vintage character)
- Sidechain input
- Built-in effects (reverb, delay, etc.)
- Resizable UI
- Additional vowel pairs beyond the 4 specified

---

**Next**: See `DSP_OVERVIEW.md` for formant theory and reference implementations.
