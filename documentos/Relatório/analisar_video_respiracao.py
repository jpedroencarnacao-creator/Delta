from pathlib import Path

import cv2
import numpy as np
import matplotlib.pyplot as plt


VIDEO = Path(r"C:\Users\jpedr\Desktop\VIDEOS_simulador_broncoscopia\P1120312.MP4")
OUT_DIR = Path(r"C:\Users\jpedr\Documents\GiitHub\Delta\tmp_video_analysis")
OUT_DIR.mkdir(parents=True, exist_ok=True)


def metadata():
    cap = cv2.VideoCapture(str(VIDEO))
    if not cap.isOpened():
        raise RuntimeError(f"Could not open video: {VIDEO}")
    fps = cap.get(cv2.CAP_PROP_FPS)
    frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    cap.release()
    return fps, frames, frames / fps, width, height


def moving_average(values, window):
    if window <= 1:
        return values
    kernel = np.ones(window, dtype=float) / window
    return np.convolve(values, kernel, mode="same")


def make_contact_sheet():
    fps, frames, duration, _, _ = metadata()
    cap = cv2.VideoCapture(str(VIDEO))
    times = np.linspace(0, duration, 30, endpoint=False)
    thumbs = []
    for t in times:
        cap.set(cv2.CAP_PROP_POS_MSEC, float(t * 1000))
        ok, frame = cap.read()
        if not ok:
            continue
        frame = cv2.resize(frame, (320, 180))
        cv2.putText(
            frame,
            f"{t:6.1f}s",
            (8, 24),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.7,
            (0, 0, 255),
            2,
            cv2.LINE_AA,
        )
        thumbs.append(frame)
    cap.release()

    rows = [np.hstack(thumbs[i : i + 5]) for i in range(0, len(thumbs), 5)]
    sheet = np.vstack(rows)
    out = OUT_DIR / "contacto_video_respiracao.jpg"
    if not cv2.imwrite(str(out), sheet):
        raise RuntimeError(f"Could not write {out}")
    return out


def motion_signal():
    fps, _, _, width, height = metadata()
    cap = cv2.VideoCapture(str(VIDEO))

    # The camera position changed for this test. A wide central crop keeps the
    # bronchial model/manipulator visible while reducing borders and table area.
    x1, x2 = int(width * 0.10), int(width * 0.90)
    y1, y2 = int(height * 0.10), int(height * 0.88)

    prev = None
    values = []
    times = []
    frame_idx = 0
    while True:
        ok, frame = cap.read()
        if not ok:
            break
        roi = frame[y1:y2, x1:x2]
        gray = cv2.cvtColor(roi, cv2.COLOR_BGR2GRAY)
        gray = cv2.resize(gray, (420, 240), interpolation=cv2.INTER_AREA)
        gray = cv2.GaussianBlur(gray, (7, 7), 0)
        if prev is not None:
            diff = cv2.absdiff(gray, prev)
            values.append(float(np.mean(diff)))
            times.append(frame_idx / fps)
        prev = gray
        frame_idx += 1
    cap.release()

    raw = np.array(values, dtype=float)
    times = np.array(times, dtype=float)

    # Respiratory movement is slower and more continuous than the heartbeat;
    # smooth over roughly half a second to suppress small mechanical vibration.
    smooth = moving_average(raw, max(3, int(round(fps * 0.55))))
    np.savez(OUT_DIR / "respiracao_motion_signal.npz", times=times, raw=raw, smooth=smooth, fps=fps)

    fig, ax = plt.subplots(figsize=(14, 4.5), dpi=160)
    ax.plot(times, smooth, linewidth=0.9)
    ax.set_title("Sinal global de movimento no video de respiracao")
    ax.set_xlabel("Tempo [s]")
    ax.set_ylabel("Diferenca media entre frames")
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    out = OUT_DIR / "respiracao_motion_signal.png"
    fig.savefig(out)
    plt.close(fig)
    return out


def strongest_frequencies(start, end, min_bpm=4, max_bpm=60):
    data = np.load(OUT_DIR / "respiracao_motion_signal.npz")
    times = data["times"]
    raw = data["raw"]
    fps = float(data["fps"])
    mask_t = (times >= start) & (times <= end)
    signal = raw[mask_t]
    if len(signal) < int(4 * fps):
        return []
    signal = signal - np.mean(signal)
    window = np.hanning(len(signal))
    spectrum = np.abs(np.fft.rfft(signal * window))
    freqs = np.fft.rfftfreq(len(signal), d=1 / fps)
    mask_f = (freqs >= min_bpm / 60) & (freqs <= max_bpm / 60)
    if not np.any(mask_f):
        return []
    idxs = np.argsort(spectrum[mask_f])[-6:][::-1]
    bpm = freqs[mask_f][idxs] * 60
    return [float(v) for v in bpm]


def estimate_active_blocks():
    data = np.load(OUT_DIR / "respiracao_motion_signal.npz")
    times = data["times"]
    smooth = data["smooth"]
    fps = float(data["fps"])

    baseline = np.percentile(smooth, 25)
    spread = np.percentile(smooth, 90) - baseline
    threshold = baseline + 0.25 * spread
    active = smooth > threshold

    # Fill short pauses and split only at long gaps, because each respiratory
    # setting may include a slow inhale/exhale and the operator pauses between settings.
    max_gap = int(round(3.0 * fps))
    filled = active.copy()
    i = 0
    while i < len(active):
        j = i
        while j < len(active) and active[j] == active[i]:
            j += 1
        if not active[i] and (j - i) <= max_gap and i > 0 and j < len(active) and filled[i - 1] and filled[j]:
            filled[i:j] = True
        i = j

    segments = []
    i = 0
    while i < len(filled):
        if not filled[i]:
            i += 1
            continue
        j = i
        while j < len(filled) and filled[j]:
            j += 1
        duration = (j - i) / fps
        if duration >= 6:
            segments.append((float(times[i]), float(times[j - 1]), i, j))
        i = j

    print("\nBlocos ativos detetados:")
    for idx, (start, end, _, _) in enumerate(segments, 1):
        peaks = strongest_frequencies(start, end)
        peaks_txt = ", ".join(f"{v:5.1f}" for v in peaks[:5]) if peaks else "sem pico claro"
        print(f"{idx:02d}: {start:7.2f}s - {end:7.2f}s ({end - start:5.2f}s) | picos: {peaks_txt} rpm")


if __name__ == "__main__":
    fps, frames, duration, width, height = metadata()
    print(f"fps={fps:.6f}")
    print(f"frames={frames}")
    print(f"duration_s={duration:.3f}")
    print(f"size={width}x{height}")
    print(f"contact_sheet={make_contact_sheet()}")
    print(f"motion_signal={motion_signal()}")
    estimate_active_blocks()
