#!/usr/bin/env python3
"""
Capture golden responses from actual Muse plugin (VST3/AU).

Requires: pedalboard (pip install pedalboard)
"""

import numpy as np
import csv
from pathlib import Path

try:
    from pedalboard import load_plugin
except ImportError:
    print("ERROR: pedalboard not installed")
    print("Install: pip install pedalboard")
    exit(1)

def capture_response(plugin, pair, morph, intensity, sample_rate=48000):
    """
    Capture magnitude response from plugin.

    Args:
        plugin: Loaded pedalboard plugin instance
        pair: 0-3 (VOWEL, BELL, LOW, SUB)
        morph: 0.0-1.0
        intensity: 0.0-1.0
        sample_rate: Hz

    Returns:
        (frequencies, magnitude_db) arrays
    """
    # Set parameters
    plugin.pair = float(pair)
    plugin.morph = morph
    plugin.intensity = intensity
    plugin.mix = 1.0  # 100% wet
    if hasattr(plugin, 'auto'):
        plugin.auto = 0.0  # Manual mode
    if hasattr(plugin, 'danger'):
        plugin.danger = 0.0  # Normal mode

    # Generate impulse (1 sample at 1.0, rest silence)
    impulse_length = 8192
    impulse = np.zeros(impulse_length, dtype=np.float32)
    impulse[0] = 1.0

    # Process (stereo)
    impulse_stereo = np.stack([impulse, impulse])
    output = plugin.process(impulse_stereo, sample_rate=sample_rate)

    # FFT magnitude (use left channel)
    fft = np.fft.rfft(output[0])
    magnitude = np.abs(fft)

    # Frequency bins
    fft_freqs = np.fft.rfftfreq(impulse_length, 1.0 / sample_rate)

    # Resample to 512 log-spaced bins (20 Hz - 20 kHz)
    target_freqs = np.logspace(np.log10(20), np.log10(20000), 512)
    magnitude_interp = np.interp(target_freqs, fft_freqs, magnitude)

    # Convert to dB (normalize to peak = 0 dB)
    magnitude_db = 20 * np.log10(magnitude_interp + 1e-12)
    magnitude_db -= np.max(magnitude_db)

    return target_freqs, magnitude_db

def save_csv(filename, freq, magnitude_db):
    """Write CSV file."""
    with open(filename, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['frequency_hz', 'magnitude_db'])
        for f_hz, mag_db in zip(freq, magnitude_db):
            writer.writerow([f'{f_hz:.1f}', f'{mag_db:.2f}'])
    print(f"✓ {filename.name}")

def main():
    import sys

    if len(sys.argv) < 2:
        print("Usage: python capture_real_plugin.py <path-to-Muse.vst3>")
        print("")
        print("Example:")
        print("  python capture_real_plugin.py build/Muse.vst3")
        print("  python capture_real_plugin.py build/Muse.component  # macOS AU")
        exit(1)

    plugin_path = sys.argv[1]
    print(f"Loading plugin: {plugin_path}")

    try:
        plugin = load_plugin(plugin_path)
        print(f"✓ Loaded: {plugin.name}")
    except Exception as e:
        print(f"ERROR: Failed to load plugin: {e}")
        exit(1)

    output_dir = Path(__file__).parent.parent / 'data' / 'golden-responses'
    output_dir.mkdir(parents=True, exist_ok=True)

    # Test cases: (filename, pair, morph, intensity)
    test_cases = [
        ('vowel_aa_int50_48k.csv', 0, 0.0, 0.5),   # VOWEL, AA
        ('vowel_ah_int50_48k.csv', 0, 0.5, 0.5),   # VOWEL, AH (middle)
        ('vowel_ee_int50_48k.csv', 0, 1.0, 0.5),   # VOWEL, EE
        ('vowel_aa_int90_48k.csv', 0, 0.0, 0.9),   # VOWEL, AA high intensity

        ('bell_oh_int50_48k.csv', 1, 0.0, 0.5),    # BELL, OH
        ('bell_oo_int50_48k.csv', 1, 1.0, 0.5),    # BELL, OO

        ('low_aa_int50_48k.csv', 2, 0.0, 0.5),     # LOW, AA
        ('low_oo_int50_48k.csv', 2, 1.0, 0.5),     # LOW, OO

        ('sub_ah_int50_48k.csv', 3, 0.5, 0.5),     # SUB, AH (static)
        ('sub_ah_int20_48k.csv', 3, 0.5, 0.2),     # SUB, AH low intensity
    ]

    print(f"\nCapturing {len(test_cases)} responses...")
    for filename, pair, morph, intensity in test_cases:
        freq, mag_db = capture_response(plugin, pair, morph, intensity)
        save_csv(output_dir / filename, freq, mag_db)

    print(f"\n✅ Captured {len(test_cases)} golden responses")
    print(f"📁 Saved to: {output_dir}")
    print("\nThese responses include:")
    print("  - Your exact Z-plane pole implementation")
    print("  - Tanh saturation coloration")
    print("  - Adaptive gain behavior")
    print("  - All DSP quirks and numerical characteristics")

if __name__ == '__main__':
    main()
