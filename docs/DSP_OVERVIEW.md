# DSP Overview - Spectral Freeze/Morph Pad (Outcome-Based)

**Goal**: Capture and hold a spectrum, shape it with formant-like envelopes, resynthesize smoothly.

## 1. Conceptual Model

**Spectral Freeze Pad** = Real-time STFT processor that can "freeze" incoming audio spectrum and reshape it with vowel-like envelopes.

### Core Operations

1. **Freeze**: Capture magnitude spectrum when FREEZE button pressed
2. **Hold**: Sustain frozen spectrum (with optional decay)
3. **Shape**: Multiply frozen magnitude by formant envelope (vowel-like peaks)
4. **Resynthesize**: IFFT back to time domain
5. **Mix**: Blend with dry signal

### Vowel-Like Envelopes

Same formant frequency concept as classic vowel filters, but applied as *spectral multipliers* instead of resonant poles:

- **VOWEL preset**: AA → AH → EE (3-stage morph)
- **BELL preset**: OH → OO (2-stage morph)
- **LOW preset**: AA → OO (2-stage morph)
- **SUB preset**: AH (static, low-frequency emphasis)

Each envelope is a smooth curve with 3-5 peaks at formant frequencies (F1, F2, F3...).

## 2. Required Behavior (Test-Verified)

### Freeze Capture

**FREEZE button behavior**:
- **Off → On**: Capture current spectrum, hold it
- **On**: Output frozen + shaped spectrum (ignores new input)
- **On → Off**: Crossfade back to live input over ~100ms

**Capture method** (implementation flexible):
- Average magnitude over N STFT frames (smoothing)
- Single-frame snapshot (simpler, may be noisier)
- RMS-weighted average (emphasizes loud frames)

### Envelope Shaping

**Envelope generation** (reuses formant theory from vowel filter spec):
- Generate smooth spectral envelope with peaks at formant frequencies
- `morph` (0-1) interpolates peak positions between vowels
- `intensity` (0-1) controls peak sharpness (Q)

**Application**:
```
M_out(f) = M_frozen(f) × E_envelope(f)
```

Where:
- `M_frozen` = captured magnitude spectrum
- `E_envelope` = formant envelope (0-2× range, typically peaks at ~2×)

### Phase Handling

**Two acceptable approaches**:

**Option A: Phase vocoder** (time-stretch quality)
- Track phase increments across frames
- Apply smoothed phase advance during freeze
- Result: Frozen spectrum "drones" with natural phase evolution

**Option B: Zero-phase reconstruction** (pad/synth quality)
- Discard input phase, use zero/minimum phase
- Result: "Purer" pad sound, less natural

**Either is acceptable** if:
- ✅ No clicks on freeze onset/release
- ✅ Output sounds continuous (not glitchy)
- ✅ Passes golden response tests (magnitude-only)

### Parameter Effects

**`pair` (0-3)** - Envelope preset:
- 0 = VOWEL: AA → AH → EE (bright, evolving)
- 1 = BELL: OH → OO (warm, round)
- 2 = LOW: AA → OO (wide → narrow, sub-focused)
- 3 = SUB: AH (static, emphasizes <500 Hz)

**`morph` (0.0 → 1.0)**:
- Interpolates formant peak positions smoothly
- VOWEL: 0-0.5 = AA→AH, 0.5-1.0 = AH→EE
- BELL/LOW: 0.0 = first vowel, 1.0 = second vowel
- SUB: morph ignored (static envelope)

**`intensity` (0.0 → 1.0)**:
- Low: Wide, gentle envelope peaks (subtle shaping)
- High: Narrow, sharp envelope peaks (aggressive shaping)
- Maps to Q-like behavior (but in spectral domain, not filter poles)

**`mix` (0.0 → 1.0)**:
- 0.0 = 100% dry (bypass)
- 1.0 = 100% wet (pure frozen/shaped spectrum)
- Equal-power crossfade preferred

**`FREEZE` (boolean)** - Mapped to `auto` parameter ID:
- false = Live input (pass through with optional shaping)
- true = Frozen spectrum (capture and hold)

**`danger` (boolean)**:
- false = Adaptive RMS normalization (output ≈ input level)
- true = +3 dB fixed boost

### Stability Constraints

**MUST remain stable for**:
- All parameter combinations
- All sample rates (44.1k - 192k)
- Extreme inputs (silence, DC, full-scale sine, noise)
- Rapid freeze on/off toggling

**If issues occur**: Crossfade to silence over 50ms (acceptable transient).

## 3. Implementation Approaches (Choose One)

### Option A: STFT with Overlap-Add

**Structure**:
- STFT: Hann window, 50-75% overlap
- FFT size: 1024-4096 samples (trade latency vs. frequency resolution)
- Freeze buffer: Store magnitude spectrum (half FFT size + 1 bins)
- Envelope: Generate for each STFT frame, multiply frozen magnitude
- IFFT: Reconstruct with shaped magnitude + phase
- Overlap-add: Blend windowed frames

**Latency**: FFT_size × (1 - overlap) samples (acceptable)

**Pros**: Standard approach, well-documented
**Cons**: Moderate CPU, requires careful windowing

### Option B: Filterbank (Parallel Bands)

**Structure**:
- 32-128 parallel bandpass filters (log-spaced)
- Per-band envelope followers (freeze = hold peak)
- Envelope shaping: Multiply band gains
- Sum bands for output

**Latency**: Minimal (filter group delay only)

**Pros**: Lower latency, easier phase handling
**Cons**: More complex, harder to match golden responses exactly

### Option C: Hybrid (Coarse Filterbank + STFT)

**Structure**:
- Coarse filterbank (8-16 bands) for freeze capture
- Fine STFT for envelope shaping
- Combine for output

**Pros**: Balance of latency and resolution
**Cons**: More code complexity

### Recommendation

**Start with Option A** (STFT) - most predictable for golden response matching.

## 4. Envelope Generation

**Formant envelope formula** (same as original vowel filter spec):

For each frequency bin `f`:
```
E(f) = Σ peak_gain_i × exp(-((f - f_i) / bw_i)^2)
```

Where:
- `f_i` = formant center frequency (interpolated by morph)
- `bw_i` = formant bandwidth (scaled by intensity)
- `peak_gain_i` = 1.0 - 2.0 (typical)

**Intensity mapping**:
- Low intensity: `bw = formant_bw × 2.0` (wide peaks)
- High intensity: `bw = formant_bw × 0.5` (narrow peaks)

**Normalize**: Scale envelope so max value = 2.0 (allows up to +6 dB boost at peaks).

## 5. Adaptive Gain

**Requirement** (when `danger` = false):
- Measure input RMS (pre-freeze)
- Measure output RMS (post-synthesis, pre-mix)
- Apply gain so output RMS ≈ input RMS

**Implementation** (flexible):
- Per-block RMS with exponential smoothing
- Clamp gain to [0.1, 10.0]
- Smooth gain changes over ~50ms

**When `danger` = true**: Skip adaptive gain, apply fixed +3 dB.

## 6. Freeze Onset/Release Smoothing

**Critical**: Avoid clicks when toggling FREEZE.

**Onset** (off → on):
- Crossfade from live input to frozen spectrum over ~50ms
- Use equal-power fade curve

**Release** (on → off):
- Crossfade from frozen spectrum to live input over ~100ms
- Longer fade-out prevents abrupt cut

**Implementation**: Ramp `freeze_blend` parameter (0-1) smoothly.

## 7. CPU Budget

**Target**: <10% single core at 48 kHz, 512 samples/block

**Typical costs**:
- 2048-point FFT: ~0.5-1.0% per frame
- Envelope generation: <0.1%
- Overlap-add: ~0.2%

**Optimization strategies**:
- Reuse FFT plans (allocate in prepare, not process)
- SIMD for envelope multiplication
- Skip processing when freeze=off and mix=0 (bypass)

## 8. NaN/Inf Sanitization

**Required**: Same as vowel filter spec.

```cpp
// Input sanitization
if (std::isnan(sample) || std::isinf(sample))
    sample = 0.0f;

// FFT output sanitization
for (auto& bin : fft_output) {
    if (std::isnan(bin) || std::isinf(bin))
        bin = 0.0f;
}
```

## 9. Denormal Suppression

**Required**: Enable FTZ/DAZ in processBlock.

```cpp
ScopedNoDenormals noDenormals;
```

## 10. Acceptance Criteria Summary

✅ **Golden responses** match CSVs within ±1.0 dB (frozen spectrum with envelope shaping)
✅ **Clickless freeze** (onset/release smooth, no audible pops)
✅ **Stable** for all parameters and inputs (no NaN, Inf, or explosions)
✅ **Adaptive gain** works (output RMS ≈ input RMS ±0.5 dB when danger=false)
✅ **CPU budget** met (<10% at 48k/512)
✅ **RT-safe** (no allocs, locks, or UI calls in processBlock)
✅ **Latency** reported correctly (FFT size × hop if using STFT)

---

**Implementation is your choice. Tests define success.**

## 11. Differences from Vowel Filter

| Aspect | Vowel Filter | Spectral Freeze Pad |
|--------|--------------|---------------------|
| **Core DSP** | IIR resonant poles | STFT freeze + envelope |
| **Freeze param** | N/A | FREEZE button captures |
| **Latency** | 0 samples | FFT size × hop (acceptable) |
| **Phase** | Natural (from poles) | Phase vocoder or zero-phase |
| **CPU** | ~2-5% | ~5-10% (STFT overhead) |
| **Golden tests** | Formant peaks | Frozen + shaped spectrum |

**UI/aesthetic/parameters**: Identical (seamless pivot).
