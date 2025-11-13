#!/usr/bin/env python3
"""
Generate golden response CSV files for Spectral Freeze Pad.

Simulates: White noise spectrum + formant envelope shaping.
Based on formant frequencies in data/SHAPES_EXAMPLE.json.
"""

import numpy as np
import csv
from pathlib import Path

# Formant definitions (frequency_hz, bandwidth_hz)
VOWELS = {
    'AA': [(700, 90), (1220, 110), (2600, 170)],   # cat
    'AH': [(730, 90), (1090, 110), (2440, 170)],   # father
    'EE': [(270, 90), (2290, 110), (3010, 170)],   # beet
    'OH': [(570, 90), (840, 110), (2410, 170)],    # boat
    'OO': [(300, 90), (870, 110), (2240, 170)],    # boot
}

def generate_white_noise_spectrum(freq, seed=42):
    """
    Generate flat white noise magnitude spectrum with small random variation.

    Args:
        freq: Frequency array (Hz)
        seed: Random seed for reproducibility

    Returns:
        Magnitude spectrum (linear scale, mean=1.0, ±15% variation)
    """
    np.random.seed(seed)
    # Flat spectrum with random variation (simulates frozen white noise)
    noise_magnitude = 1.0 + np.random.randn(len(freq)) * 0.15
    noise_magnitude = np.abs(noise_magnitude)  # Ensure positive
    return noise_magnitude

def formant_envelope_peak(freq, f0, bw, intensity):
    """
    Single formant envelope peak (Gaussian-like spectral multiplier).

    Args:
        freq: Frequency array (Hz)
        f0: Formant center frequency (Hz)
        bw: Formant bandwidth (Hz)
        intensity: 0-1, controls peak sharpness (higher = narrower)

    Returns:
        Envelope curve (1.0 = no boost, 2.0 = +6 dB at peak)
    """
    # Scale bandwidth with intensity (high intensity = narrow peak)
    effective_bw = bw * (2.0 - intensity * 1.5)  # 2.0x at int=0, 0.5x at int=1

    # Gaussian peak
    envelope = np.exp(-((freq - f0) / effective_bw)**2)

    # Peak gain (1.0 - 2.0 range, higher intensity = more gain)
    peak_gain = 1.0 + (1.0 + intensity * 0.5)  # 2.0 at int=0.5, 2.5 at int=1

    return 1.0 + envelope * (peak_gain - 1.0)

def formant_envelope(freq, formants, intensity):
    """
    Complete formant envelope (sum of Gaussian peaks).

    Args:
        freq: Frequency array (Hz)
        formants: List of (f0, bw) tuples
        intensity: 0-1

    Returns:
        Envelope multiplier (1.0 = no change, >1.0 = boost)
    """
    envelope = np.ones_like(freq)

    for f0, bw in formants:
        peak = formant_envelope_peak(freq, f0, bw, intensity)
        envelope *= peak  # Multiplicative combination

    # Gentle highpass (attenuate below F1)
    f_min = formants[0][0]  # F1 frequency
    hp_factor = np.clip(freq / (f_min * 0.6), 0, 1)**1.2
    envelope *= (0.3 + 0.7 * hp_factor)  # Don't fully zero out low freqs

    # Gentle HF roll-off (natural spectral tilt)
    hf_factor = 1.0 / (1.0 + (freq / 12000)**1.5)
    envelope *= (0.5 + 0.5 * hf_factor)  # Gradual roll-off

    return envelope

def morph_envelopes(freq, vowel1_formants, vowel2_formants, morph, intensity):
    """
    Interpolate between two formant envelopes.

    Args:
        freq: Frequency array (Hz)
        vowel1_formants: Formants for vowel at morph=0
        vowel2_formants: Formants for vowel at morph=1
        morph: 0-1 blend factor
        intensity: 0-1

    Returns:
        Envelope multiplier
    """
    # Interpolate formant frequencies and bandwidths
    blended_formants = []
    for (f1, bw1), (f2, bw2) in zip(vowel1_formants, vowel2_formants):
        f_blend = f1 * (1 - morph) + f2 * morph
        bw_blend = bw1 * (1 - morph) + bw2 * morph
        blended_formants.append((f_blend, bw_blend))

    return formant_envelope(freq, blended_formants, intensity)

def generate_freeze_pad_response(freq, vowel1_formants, vowel2_formants, morph, intensity, seed=42):
    """
    Generate spectral freeze pad magnitude response.

    Simulates: Frozen white noise spectrum × formant envelope.

    Args:
        freq: Frequency array (Hz)
        vowel1_formants: Formants for vowel at morph=0
        vowel2_formants: Formants for vowel at morph=1
        morph: 0-1 blend factor
        intensity: 0-1
        seed: Random seed for noise generation

    Returns:
        Magnitude response (linear scale)
    """
    # Generate white noise spectrum (frozen)
    noise = generate_white_noise_spectrum(freq, seed)

    # Generate formant envelope
    envelope = morph_envelopes(freq, vowel1_formants, vowel2_formants, morph, intensity)

    # Apply envelope to noise (spectral multiplication)
    shaped_magnitude = noise * envelope

    return shaped_magnitude

def generate_csv(filename, freq, magnitude_db):
    """Write CSV file with frequency and magnitude columns."""
    with open(filename, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['frequency_hz', 'magnitude_db'])
        for f_hz, mag_db in zip(freq, magnitude_db):
            writer.writerow([f'{f_hz:.1f}', f'{mag_db:.2f}'])
    print(f"✓ {filename.name}")

def main():
    # Frequency array (512 bins, 20 Hz - 20 kHz, log spacing)
    freq = np.logspace(np.log10(20), np.log10(20000), 512)

    output_dir = Path(__file__).parent.parent / 'data' / 'golden-responses'
    output_dir.mkdir(parents=True, exist_ok=True)

    # Test cases: (filename, pair, vowel1, vowel2, morph, intensity, seed)
    test_cases = [
        # VOWEL preset (3-stage morph: AA → AH → EE)
        ('vowel_aa_int50_48k.csv', 'VOWEL', 'AA', 'AH', 0.0, 0.5, 100),
        ('vowel_ah_int50_48k.csv', 'VOWEL', 'AA', 'EE', 0.5, 0.5, 101),  # Pure AH at middle
        ('vowel_ee_int50_48k.csv', 'VOWEL', 'AH', 'EE', 1.0, 0.5, 102),
        ('vowel_aa_int90_48k.csv', 'VOWEL', 'AA', 'AH', 0.0, 0.9, 103),  # High intensity

        # BELL preset (2-stage morph: OH → OO)
        ('bell_oh_int50_48k.csv', 'BELL', 'OH', 'OO', 0.0, 0.5, 104),
        ('bell_oo_int50_48k.csv', 'BELL', 'OH', 'OO', 1.0, 0.5, 105),

        # LOW preset (2-stage morph: AA → OO)
        ('low_aa_int50_48k.csv', 'LOW', 'AA', 'OO', 0.0, 0.5, 106),
        ('low_oo_int50_48k.csv', 'LOW', 'AA', 'OO', 1.0, 0.5, 107),

        # SUB preset (static AH)
        ('sub_ah_int50_48k.csv', 'SUB', 'AH', 'AH', 0.5, 0.5, 108),  # Morph ignored
        ('sub_ah_int20_48k.csv', 'SUB', 'AH', 'AH', 0.5, 0.2, 109),  # Low intensity
    ]

    print("Generating spectral freeze pad golden responses...\n")

    for filename, pair, v1, v2, morph, intensity, seed in test_cases:
        # Special handling for 3-stage VOWEL morph
        if pair == 'VOWEL' and morph == 0.5:
            # morph=0.5 should be pure AH (middle of AA→AH→EE)
            magnitude = generate_freeze_pad_response(
                freq, VOWELS['AH'], VOWELS['AH'], 0.0, intensity, seed
            )
        else:
            magnitude = generate_freeze_pad_response(
                freq, VOWELS[v1], VOWELS[v2], morph, intensity, seed
            )

        # Convert to dB (normalize to peak = 0 dB)
        magnitude_db = 20 * np.log10(magnitude + 1e-12)
        magnitude_db -= np.max(magnitude_db)  # Peak normalization

        # Write CSV
        output_path = output_dir / filename
        generate_csv(output_path, freq, magnitude_db)

    print(f"\n✅ Generated {len(test_cases)} spectral freeze pad golden responses")
    print(f"📁 Location: {output_dir}")
    print("\nCharacteristics:")
    print("  - Frozen white noise spectrum (flat ±3 dB)")
    print("  - Formant envelope peaks at vowel frequencies")
    print("  - Peak heights: 3-10 dB above noise floor")
    print("  - Peak widths controlled by intensity parameter")
    print("\nValidation:")
    print("  - Check formant peaks visible at expected frequencies")
    print("  - Verify noise floor remains ~flat between peaks")
    print("  - Optionally: capture from real plugin for tighter accuracy")

if __name__ == '__main__':
    main()