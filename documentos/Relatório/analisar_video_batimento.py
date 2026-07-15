from pathlib import Path

import cv2
import numpy as np
import matplotlib.pyplot as plt


VIDEO = Path(r"C:\Users\jpedr\Desktop\VIDEOS_simulador_broncoscopia\P1120310.MP4")
OUT_DIR = Path(r"C:\Users\jpedr\Documents\GiitHub\Delta\tmp_video_analysis")
OUT_DIR.mkdir(parents=True, exist_ok=True)


def metadata():
    cap = cv2.VideoCapture(str(VIDEO))
    fps = cap.get(cv2.CAP_PROP_FPS)
    frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    cap.release()
    return fps, frames, frames / fps, width, height


def make_contact_sheet():
    fps, frames, duration, _, _ = metadata()
    cap = cv2.VideoCapture(str(VIDEO))
    times = np.linspace(0, duration, 24, endpoint=False)
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

    rows = [np.hstack(thumbs[i : i + 4]) for i in range(0, len(thumbs), 4)]
    sheet = np.vstack(rows)
    out = OUT_DIR / "contacto_video_batimento.jpg"
    if not cv2.imwrite(str(out), sheet):
        raise RuntimeError(f"Could not write {out}")
    return out


def moving_average(values, window):
    if window <= 1:
        return values
    kernel = np.ones(window, dtype=float) / window
    return np.convolve(values, kernel, mode="same")


def motion_signal():
    fps, frames, duration, _, _ = metadata()
    cap = cv2.VideoCapture(str(VIDEO))
    prev = None
    values = []
    times = []
    frame_idx = 0

    # Central/right crop keeps the delta links and bronchial model, reducing
    # irrelevant background while keeping the moving parts visible.
    crop = (120, 980, 260, 1780)  # y1, y2, x1, x2

    while True:
        ok, frame = cap.read()
        if not ok:
            break
        y1, y2, x1, x2 = crop
        roi = frame[y1:y2, x1:x2]
        gray = cv2.cvtColor(roi, cv2.COLOR_BGR2GRAY)
        gray = cv2.resize(gray, (360, 220), interpolation=cv2.INTER_AREA)
        gray = cv2.GaussianBlur(gray, (5, 5), 0)
        if prev is not None:
            diff = cv2.absdiff(gray, prev)
            values.append(float(np.mean(diff)))
            times.append(frame_idx / fps)
        prev = gray
        frame_idx += 1

    cap.release()
    values = np.array(values, dtype=float)
    times = np.array(times, dtype=float)
    smooth = moving_average(values, max(3, int(round(fps * 0.18))))

    np.savez(OUT_DIR / "batimento_motion_signal.npz", times=times, raw=values, smooth=smooth, fps=fps)

    fig, ax = plt.subplots(figsize=(14, 4.5), dpi=160)
    ax.plot(times, smooth, linewidth=0.8)
    ax.set_title("Sinal global de movimento no vídeo")
    ax.set_xlabel("Tempo [s]")
    ax.set_ylabel("Diferença média entre frames")
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    out = OUT_DIR / "batimento_motion_signal.png"
    fig.savefig(out)
    plt.close(fig)
    return out


def estimate_active_blocks():
    data = np.load(OUT_DIR / "batimento_motion_signal.npz")
    times = data["times"]
    raw = data["raw"]
    smooth = data["smooth"]
    fps = float(data["fps"])

    active = smooth > 0.75
    max_gap = int(round(1.5 * fps))
    runs = []
    i = 0
    while i < len(active):
        value = active[i]
        j = i
        while j < len(active) and active[j] == value:
            j += 1
        runs.append((value, i, j))
        i = j

    filled = active.copy()
    for value, i, j in runs:
        if (not value) and (j - i) <= max_gap and i > 0 and j < len(filled) and filled[i - 1] and filled[j]:
            filled[i:j] = True

    segments = []
    i = 0
    while i < len(filled):
        if not filled[i]:
            i += 1
            continue
        j = i
        while j < len(filled) and filled[j]:
            j += 1
        if (j - i) / fps > 4:
            segments.append((times[i], times[j - 1], i, j))
        i = j

    print("\nBlocos ativos detetados:")
    for idx, (start, end, i, j) in enumerate(segments, 1):
        signal = raw[i:j] - np.mean(raw[i:j])
        window = np.hanning(len(signal))
        spectrum = np.abs(np.fft.rfft(signal * window))
        freqs = np.fft.rfftfreq(len(signal), d=1 / fps)
        mask = (freqs >= 0.7) & (freqs <= 4.5)
        top_idx = np.argsort(spectrum[mask])[-8:][::-1]
        top_bpm = freqs[mask][top_idx] * 60
        print(
            f"{idx:02d}: {start:7.2f}s - {end:7.2f}s "
            f"({end - start:5.2f}s) | picos: "
            + ", ".join(f"{v:6.1f}" for v in top_bpm[:5])
            + " bpm"
        )


if __name__ == "__main__":
    fps, frames, duration, width, height = metadata()
    print(f"fps={fps:.6f}")
    print(f"frames={frames}")
    print(f"duration_s={duration:.3f}")
    print(f"size={width}x{height}")
    print(f"contact_sheet={make_contact_sheet()}")
    print(f"motion_signal={motion_signal()}")
    estimate_active_blocks()
