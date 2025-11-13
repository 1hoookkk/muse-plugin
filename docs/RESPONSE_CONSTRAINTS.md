# Response Constraints - Acceptance Testing (Spectral Freeze Pad)

**Purpose**: Define measurable acceptance criteria for spectral freeze + envelope shaping behavior.

## 1. Test Methodology

### Freeze + Shape Response Testing

1. **Setup**: Load plugin, set parameters, enable FREEZE
2. **Input**: White noise (flat spectrum)
3. **Capture**: Freeze ON → spectrum captured
4. **Shape**: Apply envelope preset (pair, morph, intensity)
5. **Analysis**: FFT magnitude response of output (512 bins, 20 Hz - 20 kHz)
6. **Comparison**: Compare to golden CSV files in `data/golden-responses/`

### Tolerance Windows

- **20 Hz - 10 kHz**: ±1.0 dB per bin (accounts for STFT numerics)
- **10 kHz - 20 kHz**: ±1.5 dB per bin (relaxed for high-frequency noise)
- **Envelope peaks**: Must match formant frequencies within ±10% frequency, ±1.5 dB magnitude

## 2. Test Cases (Golden Responses)

10 test cases covering key parameter combinations:

### VOWEL Preset (0)

| File | Preset | Morph | Intensity | Expected Behavior |
|------|--------|-------|-----------|-------------------|
| `vowel_aa_int50_48k.csv` | 0 | 0.0 | 0.5 | White noise + AA envelope: peaks ~700Hz, ~1220Hz, ~2600Hz |
| `vowel_ah_int50_48k.csv` | 0 | 0.5 | 0.5 | White noise + AH envelope: peaks ~730Hz, ~1090Hz, ~2440Hz |
| `vowel_ee_int50_48k.csv` | 0 | 1.0 | 0.5 | White noise + EE envelope: peaks ~270Hz, ~2290Hz, ~3010Hz |
| `vowel_aa_int90_48k.csv` | 0 | 0.0 | 0.9 | AA envelope with narrow peaks (high Q) |

### BELL Preset (1)

| File | Preset | Morph | Intensity | Expected Behavior |
|------|--------|-------|-----------|-------------------|
| `bell_oh_int50_48k.csv` | 1 | 0.0 | 0.5 | White noise + OH envelope: peaks ~570Hz, ~840Hz, ~2410Hz |
| `bell_oo_int50_48k.csv` | 1 | 1.0 | 0.5 | White noise + OO envelope: peaks ~300Hz, ~870Hz, ~2240Hz |

### LOW Preset (2)

| File | Preset | Morph | Intensity | Expected Behavior |
|------|--------|-------|-----------|-------------------|
| `low_aa_int50_48k.csv` | 2 | 0.0 | 0.5 | White noise + AA envelope (same as VOWEL) |
| `low_oo_int50_48k.csv` | 2 | 1.0 | 0.5 | White noise + OO envelope (same as BELL) |

### SUB Preset (3)

| File | Preset | Morph | Intensity | Expected Behavior |
|------|--------|-------|-----------|-------------------|
| `sub_ah_int50_48k.csv` | 3 | 0.5 | 0.5 | White noise + AH envelope (static, morph ignored) |
| `sub_ah_int20_48k.csv` | 3 | 0.5 | 0.2 | AH envelope with wide peaks (low Q) |

### Test Parameters

- **Input**: White noise, RMS = 0.3 (-10 dBFS)
- **FREEZE**: true (spectrum captured)
- **Mix**: 1.0 (100% wet)
- **DANGER**: false (adaptive gain active)
- **Sample rate**: 48 kHz
- **Block size**: 512 samples

## 3. Expected Magnitude Response Characteristics

### Input Spectrum (Before Freeze)

White noise → flat spectrum (~0 dB ±3 dB variation across bins)

### Envelope Shaping (After Freeze)

**Flat noise floor** + **formant peaks**:
- Peaks at formant frequencies (F1, F2, F3)
- Peak heights: 3-10 dB above noise floor
- Peak widths controlled by `intensity`:
  - Low intensity (0.2): wide peaks (~200-400 Hz bandwidth)
  - Medium intensity (0.5): moderate peaks (~100-200 Hz)
  - High intensity (0.9): narrow peaks (~50-100 Hz)

### Peak Frequencies (Must Match Within ±10%)

| Vowel | F1 Target | F1 Tolerance | F2 Target | F2 Tolerance | F3 Target | F3 Tolerance |
|-------|-----------|--------------|-----------|--------------|-----------|--------------|
| AA | 700 Hz | 630-770 Hz | 1220 Hz | 1098-1342 Hz | 2600 Hz | 2340-2860 Hz |
| AH | 730 Hz | 657-803 Hz | 1090 Hz | 981-1199 Hz | 2440 Hz | 2196-2684 Hz |
| EE | 270 Hz | 243-297 Hz | 2290 Hz | 2061-2519 Hz | 3010 Hz | 2709-3311 Hz |
| OH | 570 Hz | 513-627 Hz | 840 Hz | 756-924 Hz | 2410 Hz | 2169-2651 Hz |
| OO | 300 Hz | 270-330 Hz | 870 Hz | 783-957 Hz | 2240 Hz | 2016-2464 Hz |

### Between-Peaks Response

- **Valley depth**: 3-6 dB below noise floor (envelope dips between peaks)
- **Overall shape**: Smooth, no sharp discontinuities
- **Low-freq roll-off**: Gentle highpass below F1 (~6 dB/octave)
- **High-freq roll-off**: Gradual above F3 (~3-6 dB/octave)

## 4. Morph Continuity Test

**Setup**: Freeze white noise, sweep `morph` from 0.0 to 1.0 in 10 steps.

**Expected**:
- Envelope peak frequencies interpolate smoothly (no jumps >15% between steps)
- No discontinuities in magnitude (max delta <4 dB between steps)
- Peaks remain stable (no sudden drops or spikes)

**Verification**: Visual inspection of magnitude response animation.

## 5. Intensity Scaling Test

**Setup**: Freeze white noise, fix `morph=0.5`, sweep `intensity` from 0.0 to 1.0.

**Expected**:
- Peak frequencies stay constant (±5%)
- Peak heights increase slightly (~1-3 dB)
- Peak widths decrease (narrow with intensity)

**Measurement**:
- Measure 3dB bandwidth of F1 peak at intensity=0.2 and intensity=0.9
- Expect: BW(0.9) < 0.4 × BW(0.2)

## 6. Freeze Onset/Release Test

**Setup**: Start with white noise input, toggle FREEZE button.

**Pass criteria**:
- **Onset** (off→on): No audible click, smooth crossfade (~50ms)
- **Hold**: Spectrum remains stable (no drifting or modulation)
- **Release** (on→off): No audible click, smooth crossfade (~100ms)

**Test method**: Manual listening + waveform inspection for clicks.

## 7. Stability Test

**Inputs** (each tested with FREEZE on for 10 seconds):
- White noise (RMS = 0.5)
- Silence (all zeros)
- DC offset (+1.0 constant)
- Full-scale sine (1 kHz, amplitude=1.0)
- Impulse train (1 sample =1.0, 99 samples =0.0, repeat)

**Parameters**: All combinations of:
- preset: 0, 1, 2, 3
- morph: 0.0, 0.5, 1.0
- intensity: 0.0, 0.5, 0.9, 1.0

**Pass criteria**:
- Output remains bounded (no samples >10.0 in absolute value)
- No NaN or Inf in output
- No sustained oscillation after input stops

## 8. Adaptive Gain Test

**Setup**:
- Input: Pink noise, RMS = 0.2 (-14 dBFS)
- Parameters: preset=0, morph=0.5, intensity=0.5, mix=1.0, FREEZE=true, danger=false

**Measurement**:
- Freeze spectrum for 1 second
- Compute output RMS over 1 second
- Compare to input RMS

**Pass criteria**:
- Output RMS within ±0.5 dB of input RMS (factor of 0.944 - 1.059×)

**Danger mode test**:
- Set danger=true
- Output RMS should be ~3 dB louder than adaptive mode (factor ~1.41×)

## 9. Sample Rate Independence

**Test**: Run all golden response tests at 44.1k, 48k, and 96k.

**Expected**:
- Envelope peak frequencies scale proportionally (e.g., 700 Hz at 48k → 700 Hz at 96k)
- Magnitude response matches golden within tolerance (after frequency scaling)
- FFT size/hop may vary with SR (implementation choice)

## 10. CSV File Format

Golden response files are plain text CSV (UTF-8, LF line endings):

```csv
frequency_hz,magnitude_db
20.0,-8.3
41.0,-6.5
62.0,-4.2
...
19980.0,-15.7
20000.0,-16.1
```

**Columns**:
- `frequency_hz`: Center frequency of FFT bin (float)
- `magnitude_db`: Magnitude in dB (20 × log10(|H(f)|), float)

**Bins**: 512 rows (20 Hz - 20 kHz, logarithmic spacing preferred)

**Note**: Magnitude is **relative** (peak normalized to ~0 dB). Absolute level depends on input RMS.

## 11. Test Script Usage

```bash
# Run all tests
python tests/acceptance/test_responses.py --plugin build/Muse.vst3

# Run specific test
python tests/acceptance/test_responses.py --plugin build/Muse.vst3 --case vowel_aa_int50

# Visualize difference
python tests/acceptance/test_responses.py --plugin build/Muse.vst3 --case vowel_aa_int50 --plot

# Adjust tolerance
python tests/acceptance/test_responses.py --plugin build/Muse.vst3 --tolerance 1.5
```

## 12. Pass/Fail Criteria

**PASS**: All 10 test cases match golden responses within tolerance.

**FAIL**: Any test case exceeds tolerance in >10% of frequency bins.

**Conditional pass**: If failures are limited to >10 kHz region and <2.0 dB, acceptable (STFT numerical noise).

---

## 13. Key Differences from Vowel Filter Tests

| Aspect | Vowel Filter | Spectral Freeze Pad |
|--------|--------------|---------------------|
| **Input** | Impulse (Dirac delta) | White noise (frozen) |
| **Mechanism** | IIR resonant poles | Spectral envelope multiplication |
| **Phase** | Natural (from recursion) | Phase vocoder or zero-phase |
| **Peaks** | Filter resonances | Envelope-shaped noise |
| **Valleys** | Roll-off between peaks | Attenuated noise floor |
| **Latency** | 0 samples | FFT hop (512-1024) |

**Success = matching golden responses. Implementation details don't matter.**
