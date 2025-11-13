# How to Capture Golden Responses from Your Plugin

## What You Get

**Synthetic responses** (already generated): Pure formant theory, clean, idealized.
**Real plugin responses** (you can capture): Your exact implementation including all quirks.

## Why Capture Real Responses?

Your plugin has implementation details that synthetic responses don't capture:
- Tanh saturation (adds harmonic coloration)
- Exact Z-plane pole solver numerics
- Adaptive gain smoothing (~29ms time constant)
- Any numerical differences from pure theory

**Result**: Claude's implementation will be tested against YOUR sound, not idealized theory.

## How to Capture

### Option 1: Using Python (Easiest)

```bash
# Install pedalboard
pip install pedalboard numpy

# Capture from your VST3
cd /c/Muse/muse-plugin-spec
python scripts/capture_real_plugin.py ../MuseAudio/build/Muse_artefacts/Release/VST3/Muse.vst3
```

This will **overwrite** the synthetic CSVs with real captures from your plugin.

### Option 2: Using DAW + Analysis Tool

1. **Load plugin** in Reaper/Ableton/Logic
2. **Set parameters**:
   - Pair = 0 (VOWEL)
   - Morph = 0.0 (AA)
   - Intensity = 0.5
   - Mix = 1.0 (100% wet)
   - AUTO = off
   - DANGER = off
3. **Send impulse** (single sample at +1.0, rest silence)
4. **Record output**
5. **FFT analysis** (512 bins, 20 Hz - 20 kHz log scale)
6. **Export magnitude** as CSV (dB, normalized to 0 dB peak)
7. **Repeat** for all 10 test cases

### Option 3: Build Test Harness

Add to your CMakeLists.txt:

```cmake
add_executable(CaptureGoldenResponses
    tools/CaptureGoldenResponses.cpp
    source/PluginProcessor.cpp
    source/dsp/MuseZPlaneEngine.cpp)

target_link_libraries(CaptureGoldenResponses PRIVATE juce::juce_audio_processors)
```

Then call `PluginProcessor::processBlock()` offline with impulse input.

## Validation

After capturing, check that formant peaks are visible:

```bash
# Check AA formant at ~700 Hz
grep "^69[0-9]\|^70[0-9]\|^71[0-9]" vowel_aa_int50_48k.csv

# Should show peak near 699-700 Hz with ~0 dB
```

## When to Use Which

| Use Case | Synthetic | Real Plugin |
|----------|-----------|-------------|
| **Early development** | ✅ Good enough | ⚠️ Overkill |
| **Final validation** | ❌ May diverge | ✅ Exact match |
| **Cross-team testing** | ✅ Reproducible | ❌ Requires build |
| **CI/CD** | ✅ Check in repo | ⚠️ Brittle |

## Recommendation

1. **Start with synthetic** (already done) ✅
2. **Capture real** before shipping (optional but recommended)
3. **Document in RESPONSE_CONSTRAINTS.md** which you used

---

**Current status**: Synthetic responses generated. Ready to use for experiment.
**Optional next step**: Capture real responses for tighter validation.
