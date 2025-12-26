import numpy as np
import matplotlib.pyplot as plt
from scipy import signal
from scipy.signal import freqz, group_delay

# =========================
# FIR Filter Design Parameters
# =========================
NUM_TAPS = 64
SAMPLE_RATE = 44100.0  # Hz (original sample rate)
TARGET_SAMPLE_RATE = 16000.0  # Hz (target after resampling)
NYQUIST = SAMPLE_RATE / 2.0
TARGET_NYQUIST = TARGET_SAMPLE_RATE / 2.0  # 8000 Hz

# For anti-aliasing before downsampling from 44.1kHz to 16kHz:
# Target Nyquist is 8kHz, so we need to remove all content above ~7.5kHz
# Set cutoff at ~0.9 of target Nyquist to allow for transition band
CUTOFF_FREQ = 7200.0  # Hz (safely below 8kHz target Nyquist)

# =========================
# Design FIR Filter
# =========================
# Using a Kaiser window for good stopband attenuation
# firwin automatically normalizes cutoff by Nyquist frequency
h = signal.firwin(
    NUM_TAPS,
    CUTOFF_FREQ,
    window=('kaiser', 8.0),  # Beta=8 gives ~80dB stopband attenuation
    fs=SAMPLE_RATE
)

print("=" * 60)
print("FIR FILTER DESIGN SUMMARY")
print("=" * 60)
print(f"Number of taps: {len(h)}")
print(f"Original sample rate: {SAMPLE_RATE} Hz")
print(f"Target sample rate: {TARGET_SAMPLE_RATE} Hz (resampling ratio: {TARGET_SAMPLE_RATE/SAMPLE_RATE:.4f})")
print(f"Target Nyquist: {TARGET_NYQUIST} Hz")
print(f"Cutoff frequency: {CUTOFF_FREQ} Hz ({CUTOFF_FREQ/TARGET_NYQUIST:.3f} * target Nyquist)")
print(f"DC gain (sum of taps): {np.sum(h):.6f}")
print(f"Expected group delay: {(len(h) - 1) / 2:.1f} samples ({(len(h) - 1) / 2 / SAMPLE_RATE * 1000:.3f} ms)")
print("=" * 60)

# Print coefficients in C++ array format
print("\nC++ array format:")
print("constexpr float FIR_TAPS[{}] = {{".format(NUM_TAPS))
for i in range(0, len(h), 4):
    coeffs = ", ".join([f"{c:13.10f}f" for c in h[i:min(i+4, len(h))]])
    print(f"    {coeffs},")
print("};")
print()

# =========================
# Frequency Analysis
# =========================
w, H = freqz(h, worN=16384, fs=SAMPLE_RATE)
mag = np.abs(H)
mag_db = 20 * np.log10(np.maximum(mag, 1e-12))
phase = np.unwrap(np.angle(H))

# Group delay
wg, gd = group_delay((h, 1), fs=SAMPLE_RATE)

# Step response
step_length = NUM_TAPS * 3
step_input = np.ones(step_length)
step_response = signal.lfilter(h, 1, step_input)

# =========================
# Create Comprehensive Plots
# =========================
fig = plt.figure(figsize=(16, 12))
fig.suptitle(f'{NUM_TAPS}-Tap FIR Anti-Aliasing Filter: {SAMPLE_RATE/1000:.1f} kHz → {TARGET_SAMPLE_RATE/1000:.1f} kHz',
             fontsize=14, fontweight='bold')

# 1) Impulse Response
ax1 = plt.subplot(3, 3, 1)
plt.stem(h, basefmt=" ", linefmt='C0-', markerfmt='C0o')
plt.title("Impulse Response")
plt.xlabel("Tap Index")
plt.ylabel("Amplitude")
plt.grid(True, alpha=0.3)
plt.axhline(0, color='k', linewidth=0.5)

# 2) Magnitude Response (Full Range)
ax2 = plt.subplot(3, 3, 2)
plt.plot(w, mag_db, linewidth=1.5)
plt.axvline(CUTOFF_FREQ, color='r', linestyle='--', linewidth=1.5, label=f'Cutoff: {CUTOFF_FREQ} Hz')
plt.axvline(TARGET_NYQUIST, color='m', linestyle='--', linewidth=1.5, alpha=0.7, label=f'Target Nyquist: {TARGET_NYQUIST} Hz')
plt.axhline(-3, color='orange', linestyle=':', alpha=0.7, label='-3 dB')
plt.axhline(-6, color='y', linestyle=':', alpha=0.7, label='-6 dB')
plt.title("Magnitude Response (Full)")
plt.xlabel("Frequency (Hz)")
plt.ylabel("Magnitude (dB)")
plt.ylim(-120, 5)
plt.xlim(0, NYQUIST)
plt.grid(True, alpha=0.3)
plt.legend(fontsize=7)

# 3) Magnitude Response (Passband Detail)
ax3 = plt.subplot(3, 3, 3)
passband_freq = CUTOFF_FREQ * 1.5
passband_mask = w <= passband_freq
plt.plot(w[passband_mask], mag_db[passband_mask], linewidth=1.5)
plt.axvline(CUTOFF_FREQ, color='r', linestyle='--', linewidth=1.5, label=f'Cutoff: {CUTOFF_FREQ} Hz')
plt.axhline(-3, color='orange', linestyle=':', alpha=0.7, label='-3 dB')
plt.axhline(-1, color='g', linestyle=':', alpha=0.7, label='-1 dB')
plt.axhline(-0.1, color='c', linestyle=':', alpha=0.7, label='-0.1 dB')
plt.title("Magnitude Response (Passband)")
plt.xlabel("Frequency (Hz)")
plt.ylabel("Magnitude (dB)")
plt.ylim(-3, 1)
plt.xlim(0, passband_freq)
plt.grid(True, alpha=0.3)
plt.legend(fontsize=8)

# 4) Magnitude Response (Linear Scale)
ax4 = plt.subplot(3, 3, 4)
plt.plot(w, mag, linewidth=1.5)
plt.axvline(CUTOFF_FREQ, color='r', linestyle='--', linewidth=1.5, label=f'Cutoff: {CUTOFF_FREQ} Hz')
plt.axhline(1/np.sqrt(2), color='orange', linestyle=':', alpha=0.7, label='-3 dB (0.707)')
plt.title("Magnitude Response (Linear)")
plt.xlabel("Frequency (Hz)")
plt.ylabel("Magnitude")
plt.xlim(0, NYQUIST)
plt.ylim(0, 1.1)
plt.grid(True, alpha=0.3)
plt.legend(fontsize=8)

# 5) Phase Response
ax5 = plt.subplot(3, 3, 5)
plt.plot(w, np.degrees(phase), linewidth=1.5)
plt.title("Phase Response (Unwrapped)")
plt.xlabel("Frequency (Hz)")
plt.ylabel("Phase (degrees)")
plt.xlim(0, NYQUIST)
plt.grid(True, alpha=0.3)

# 6) Group Delay
ax6 = plt.subplot(3, 3, 6)
plt.plot(wg, gd, linewidth=1.5)
expected_delay = (NUM_TAPS - 1) / 2
plt.axhline(expected_delay, color='r', linestyle='--', linewidth=1.5,
            label=f'Expected: {expected_delay:.1f} samples')
plt.title("Group Delay")
plt.xlabel("Frequency (Hz)")
plt.ylabel("Delay (samples)")
plt.xlim(0, NYQUIST)
plt.grid(True, alpha=0.3)
plt.legend(fontsize=8)

# 7) Step Response
ax7 = plt.subplot(3, 3, 7)
plt.plot(step_response, linewidth=1.5)
plt.axhline(1.0, color='r', linestyle='--', alpha=0.7, label='Target: 1.0')
plt.title("Step Response")
plt.xlabel("Sample Index")
plt.ylabel("Amplitude")
plt.grid(True, alpha=0.3)
plt.legend(fontsize=8)

# 8) Stopband Attenuation Detail
ax8 = plt.subplot(3, 3, 8)
stopband_start = CUTOFF_FREQ * 1.2
stopband_mask = w >= stopband_start
plt.plot(w[stopband_mask], mag_db[stopband_mask], linewidth=1.5)
plt.axhline(-40, color='orange', linestyle=':', alpha=0.7, label='-40 dB')
plt.axhline(-60, color='y', linestyle=':', alpha=0.7, label='-60 dB')
plt.axhline(-80, color='g', linestyle=':', alpha=0.7, label='-80 dB')
plt.title("Stopband Attenuation")
plt.xlabel("Frequency (Hz)")
plt.ylabel("Magnitude (dB)")
plt.xlim(stopband_start, NYQUIST)
plt.ylim(-120, 0)
plt.grid(True, alpha=0.3)
plt.legend(fontsize=8)

# 9) Frequency Response Summary Table
ax9 = plt.subplot(3, 3, 9)
ax9.axis('off')

# Find key frequencies
idx_dc = 0
idx_cutoff = np.argmin(np.abs(w - CUTOFF_FREQ))
idx_nyquist = -1

# Calculate metrics
passband_ripple = np.max(mag_db[w <= CUTOFF_FREQ * 0.9]) - np.min(mag_db[w <= CUTOFF_FREQ * 0.9])
stopband_atten = -np.max(mag_db[w >= CUTOFF_FREQ * 1.5])

info_text = f"""
FILTER CHARACTERISTICS

Number of Taps: {NUM_TAPS}
Original Sample Rate: {SAMPLE_RATE/1000:.1f} kHz
Target Sample Rate: {TARGET_SAMPLE_RATE/1000:.1f} kHz
Target Nyquist: {TARGET_NYQUIST/1000:.1f} kHz
Cutoff Frequency: {CUTOFF_FREQ} Hz

PERFORMANCE METRICS

DC Gain: {mag_db[idx_dc]:.3f} dB
Gain at Cutoff: {mag_db[idx_cutoff]:.3f} dB
Passband Ripple: {passband_ripple:.3f} dB
Stopband Atten: {stopband_atten:.1f} dB

GROUP DELAY

Expected: {expected_delay:.1f} samples
Actual (avg): {np.mean(gd[wg <= CUTOFF_FREQ]):.2f} samples
Time Delay: {expected_delay/SAMPLE_RATE*1000:.3f} ms

TRANSITION BAND

-3 dB freq: {w[np.argmin(np.abs(mag_db + 3))]:.0f} Hz
-6 dB freq: {w[np.argmin(np.abs(mag_db + 6))]:.0f} Hz
-40 dB freq: {w[np.argmin(np.abs(mag_db + 40))]:.0f} Hz
"""

plt.text(0.1, 0.5, info_text, fontsize=9, family='monospace',
         verticalalignment='center', transform=ax9.transAxes)

plt.tight_layout()
plt.savefig('/Users/anthonyhong/Desktop/self/code/autolumemac/rtautolume/scripts/fir_filter_analysis.png',
            dpi=150, bbox_inches='tight')
print(f"Plot saved to: scripts/fir_filter_analysis.png")
plt.show()

# =========================
# Additional Analysis
# =========================
print("\nFREQUENCY RESPONSE AT KEY POINTS:")
print("-" * 60)
for freq_hz in [100, 1000, 5000, 7200, 8000, 10000, 15000, 20000]:
    if freq_hz > NYQUIST:
        continue
    idx = np.argmin(np.abs(w - freq_hz))
    marker = ""
    if abs(freq_hz - CUTOFF_FREQ) < 1:
        marker = " (CUTOFF)"
    elif abs(freq_hz - TARGET_NYQUIST) < 1:
        marker = " (TARGET NYQUIST)"
    print(f"{freq_hz:6.0f} Hz: {mag_db[idx]:7.2f} dB  ({mag[idx]:.4f} linear){marker}")
