import re
import matplotlib.pyplot as plt
import numpy as np

FSAMP = 8000  # ADC sampling rate

# --- Load audio samples ---
with open("audio_samples.txt", "r") as f:
    audio_data = f.read()

# Parse the audio samples
samples = [int(x) for x in re.findall(r"Buffer\[\d+\]: (\d+)", audio_data)]
samples = np.array(samples)

# Time axis for waveform
time = np.arange(len(samples)) / FSAMP

# --- Load FFT data ---
with open("fft_data.txt", "r") as f:
    fft_data = f.read()

pattern = r"Bin (\d+): Freq (\d+)-(\d+) Hz, Amplitude: ([\d\.]+)"
matches = re.findall(pattern, fft_data)

freqs = [(int(f_start) + int(f_end)) / 2 for _, f_start, f_end, _ in matches]
amps = [float(amp) for _, _, _, amp in matches]

# Convert amplitudes to dB
amps_db = 20 * np.log10(np.array(amps) + 1e-6)

# --- Plot waveform and FFT ---
fig, axs = plt.subplots(2, 1, figsize=(12, 8))

# Audio waveform
axs[0].plot(time, samples, color="darkgreen")
axs[0].set_title("Audio Waveform")
axs[0].set_xlabel("Time (s)")
axs[0].set_ylabel("Amplitude")
axs[0].grid(True, alpha=0.3)

# FFT spectrum
axs[1].plot(freqs, amps_db, color="royalblue")
axs[1].set_title("FFT Spectrum (Log Amplitude)")
axs[1].set_xlabel("Frequency (Hz)")
axs[1].set_ylabel("Amplitude (dB)")
axs[1].set_ylim(bottom=0)  # start y-axis at 0
axs[1].grid(True, alpha=0.3)

plt.tight_layout()
plt.show()
