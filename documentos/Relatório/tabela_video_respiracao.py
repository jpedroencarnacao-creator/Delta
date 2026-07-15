from pathlib import Path

import numpy as np


DATA = Path(r"C:\Users\jpedr\Documents\GiitHub\Delta\tmp_video_analysis\respiracao_motion_signal.npz")


def fft_peak(times, raw, start, end, min_bpm=6, max_bpm=30):
    mask_t = (times >= start) & (times <= end)
    signal = raw[mask_t]
    fps = 1 / np.median(np.diff(times))
    signal = signal - np.mean(signal)
    window = np.hanning(len(signal))
    spectrum = np.abs(np.fft.rfft(signal * window))
    freqs = np.fft.rfftfreq(len(signal), d=1 / fps)
    mask_f = (freqs >= min_bpm / 60) & (freqs <= max_bpm / 60)
    idx = np.argmax(spectrum[mask_f])
    return float(freqs[mask_f][idx] * 60)


def autocorr_peak(times, raw, start, end, min_bpm=6, max_bpm=30):
    mask_t = (times >= start) & (times <= end)
    signal = raw[mask_t]
    fps = 1 / np.median(np.diff(times))
    signal = signal - np.mean(signal)
    corr = np.correlate(signal, signal, mode="full")[len(signal) - 1 :]
    min_lag = int(round(fps * 60 / max_bpm))
    max_lag = int(round(fps * 60 / min_bpm))
    max_lag = min(max_lag, len(corr) - 1)
    if max_lag <= min_lag:
        return float("nan")
    lag = min_lag + int(np.argmax(corr[min_lag:max_lag]))
    return float(60 * fps / lag)


def main():
    data = np.load(DATA)
    times = data["times"]
    raw = data["raw"]

    segments = [
        (8.59, 33.60),
        (45.61, 68.85),
        (85.20, 128.14),
        (141.41, 169.60),
        (182.22, 212.48),
        (224.84, 249.77),
        (267.28, 297.56),
        (311.78, 340.56),
        (351.58, 381.88),
    ]

    print("| Patamar | Intervalo [s] | Duração [s] | Frequência FFT [rpm] | Frequência autocorr. [rpm] | Valor adotado [rpm] | Período [s] |")
    print("|---:|---:|---:|---:|---:|---:|---:|")
    for idx, (start, end) in enumerate(segments, 1):
        fft = fft_peak(times, raw, start, end)
        auto = autocorr_peak(times, raw, start, end)
        adopted = fft
        period = 60 / adopted
        print(
            f"| {idx} | {start:.2f}-{end:.2f} | {end-start:.2f} | "
            f"{fft:.1f} | {auto:.1f} | {adopted:.1f} | {period:.3f} |"
        )


if __name__ == "__main__":
    main()
