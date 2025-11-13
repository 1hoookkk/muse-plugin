# Golden Response Files

## Purpose

These CSV files contain reference magnitude responses for acceptance testing.

**Source**: Generated from synthetic formant targets (NOT EMU-authentic data).

## Format

Plain text CSV (UTF-8, LF line endings):

```csv
frequency_hz,magnitude_db
20.0,-15.3
39.1,-14.8
...
20000.0,-22.1
```

### Columns

- `frequency_hz`: FFT bin center frequency (float, Hz)
- `magnitude_db`: Magnitude in dB (20 × log10(|H(f)|), float)

### Bins

- **Count**: 512 bins
- **Range**: 20 Hz - 20 kHz
- **Spacing**: Logarithmic preferred (perceptually uniform)

## Test Conditions

All files generated at:
- **Sample rate**: 48 kHz
- **Block size**: 512 samples
- **Input**: White noise impulse (1 sample at +1.0, rest silence)
- **Mix**: 1.0 (100% wet)
- **AUTO**: false
- **DANGER**: false

## Naming Convention

`{pair}_{vowel}_{intensity}_48k.csv`

- **pair**: `vowel`, `bell`, `low`, `sub`
- **vowel**: `aa`, `ah`, `ee`, `oh`, `oo`
- **intensity**: `intXX` where XX = intensity × 100 (e.g., `int50` = 0.5)
- **48k**: Sample rate

### Examples

- `vowel_aa_int50_48k.csv`: VOWEL pair, morph=0.0 (AA), intensity=0.5
- `bell_oo_int90_48k.csv`: BELL pair, morph=1.0 (OO), intensity=0.9
- `sub_ah_int20_48k.csv`: SUB pair, morph=any (static AH), intensity=0.2

## Test Cases (10 Required)

1. `vowel_aa_int50_48k.csv` - VOWEL, morph=0.0, int=0.5
2. `vowel_ah_int50_48k.csv` - VOWEL, morph=0.5, int=0.5
3. `vowel_ee_int50_48k.csv` - VOWEL, morph=1.0, int=0.5
4. `vowel_aa_int90_48k.csv` - VOWEL, morph=0.0, int=0.9
5. `bell_oh_int50_48k.csv` - BELL, morph=0.0, int=0.5
6. `bell_oo_int50_48k.csv` - BELL, morph=1.0, int=0.5
7. `low_aa_int50_48k.csv` - LOW, morph=0.0, int=0.5
8. `low_oo_int50_48k.csv` - LOW, morph=1.0, int=0.5
9. `sub_ah_int50_48k.csv` - SUB, morph=0.5, int=0.5
10. `sub_ah_int20_48k.csv` - SUB, morph=0.5, int=0.2

## Generating Golden Responses

### Option 1: From Reference Plugin

If you have a working reference implementation:

```python
import numpy as np
import csv

def generate_golden_response(plugin_path, pair, morph, intensity, output_csv):
    # Load plugin (using python-vst3 or similar)
    plugin = load_vst3(plugin_path)
    plugin.set_parameter("pair", pair)
    plugin.set_parameter("morph", morph)
    plugin.set_parameter("intensity", intensity)
    plugin.set_parameter("mix", 1.0)

    # Generate impulse
    impulse = np.zeros(8192)
    impulse[0] = 1.0

    # Process
    output = plugin.process(impulse, sample_rate=48000)

    # FFT (magnitude only)
    fft = np.fft.rfft(output)
    magnitude = np.abs(fft)
    magnitude_db = 20 * np.log10(magnitude + 1e-12)

    # Frequency bins (logarithmic)
    freqs = np.logspace(np.log10(20), np.log10(20000), 512)
    # Interpolate FFT to log freq scale
    fft_freqs = np.fft.rfftfreq(len(output), 1/48000)
    mag_interp = np.interp(freqs, fft_freqs, magnitude_db)

    # Write CSV
    with open(output_csv, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['frequency_hz', 'magnitude_db'])
        for freq, mag in zip(freqs, mag_interp):
            writer.writerow([f'{freq:.1f}', f'{mag:.2f}'])
```

### Option 2: From Formant Theory

Generate synthetic responses from formant definitions in `SHAPES_EXAMPLE.json`:

```python
def formant_response(freq, f0, bw):
    """Single formant (resonant peak) transfer function magnitude."""
    # Simplified: |H(f)| ≈ 1 / sqrt((f - f0)^2 + (bw/2)^2)
    return 1.0 / np.sqrt((freq - f0)**2 + (bw/2)**2)

def vowel_response(freq, formants):
    """Sum of formant responses."""
    response = np.zeros_like(freq)
    for formant in formants:
        f0 = formant['frequency_hz']
        bw = formant['bandwidth_hz']
        response += formant_response(freq, f0, bw)
    return response

# Example for AA vowel
freqs = np.logspace(np.log10(20), np.log10(20000), 512)
aa_formants = [
    {'frequency_hz': 700, 'bandwidth_hz': 90},
    {'frequency_hz': 1220, 'bandwidth_hz': 110},
    {'frequency_hz': 2600, 'bandwidth_hz': 170}
]
response = vowel_response(freqs, aa_formants)
magnitude_db = 20 * np.log10(response)

# Normalize to 0 dB at peak
magnitude_db -= np.max(magnitude_db)
```

## Tolerance

- **20 Hz - 10 kHz**: ±0.5 dB per bin
- **10 kHz - 20 kHz**: ±1.0 dB per bin (relaxed)

## Usage

Test script compares plugin output to these files:

```bash
python tests/acceptance/test_responses.py --plugin build/Muse.vst3
```

Pass criteria: All 10 files match within tolerance.
