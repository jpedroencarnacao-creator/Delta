from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


OUT_DIR = Path("figuras/resultados")
OUT_DIR.mkdir(parents=True, exist_ok=True)

axes_data = {
    "x": {
        "title": "Eixo X",
        "intended": [-30, -25, -20, -15, -10, -5, 0, 5, 10, 15, 20, 25, 30],
        "measured": [-26, -21.5, -18, -13.5, -10, -4, 0, 5, 8, 12, 15.5, 19, 19.4],
        "limits": (-30, 30),
        "ticks": np.arange(-30, 31, 5),
        "jumps_labels": [
            "-30->-25", "-25->-20", "-20->-15", "-15->-10", "-10->-5", "-5->0",
            "0->5", "5->10", "10->15", "15->20", "20->25", "25->30",
        ],
        "jumps": [4.5, 3.5, 4.5, 3.5, 6.0, 4.0, 5.0, 3.0, 4.0, 3.5, 3.5, 0.4],
        "color": "#3f8f3a",
    },
    "y": {
        "title": "Eixo Y",
        "intended": [-30, -25, -20, -15, -10, -5, 0, 5, 10, 15, 20, 25, 30],
        "measured": [-22.5, -18.5, -15, -11, -7.5, -3.5, 0, 5, 8.5, 12, 16.5, 20.5, 24.5],
        "limits": (-30, 30),
        "ticks": np.arange(-30, 31, 5),
        "jumps_labels": [
            "-30->-25", "-25->-20", "-20->-15", "-15->-10", "-10->-5", "-5->0",
            "0->5", "5->10", "10->15", "15->20", "20->25", "25->30",
        ],
        "jumps": [4.0, 3.5, 4.0, 3.5, 4.0, 3.5, 5.0, 4.5, 3.5, 4.5, 4.0, 4.0],
        "color": "#2287a7",
    },
    "z": {
        "title": "Eixo Z",
        "intended": [45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100, 105],
        "measured": [45.8, 50, 54, 58, 63.5, 68, 72.5, 78, 84, 86.5, 90, 95, 98.5],
        "limits": (45, 105),
        "ticks": np.arange(45, 106, 5),
        "jumps_labels": [
            "45->50", "50->55", "55->60", "60->65", "65->70", "70->75",
            "75->80", "80->85", "85->90", "90->95", "95->100", "100->105",
        ],
        "jumps": [4.2, 4.0, 4.0, 5.5, 4.5, 4.5, 5.5, 6.0, 2.5, 3.5, 5.0, 3.5],
        "color": "#c67955",
    },
}


def set_common_style(ax):
    ax.grid(True, color="#d9d9d9", linewidth=0.8)
    ax.set_axisbelow(True)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)


for axis, data in axes_data.items():
    fig, ax = plt.subplots(figsize=(7.2, 5.0), dpi=180)
    intended = np.array(data["intended"], dtype=float)
    measured = np.array(data["measured"], dtype=float)
    lo, hi = data["limits"]

    ax.plot([lo, hi], [lo, hi], color="#404040", linewidth=1.6, linestyle="--", label="Valor pretendido")
    ax.plot(intended, measured, color=data["color"], marker="o", linewidth=2.2, label="Valor obtido")
    ax.set_title(f"Comparação entre deslocamento pretendido e obtido - {data['title']}")
    ax.set_xlabel("Valor pretendido [mm]")
    ax.set_ylabel("Valor obtido [mm]")
    ax.set_xlim(lo, hi)
    ax.set_ylim(lo, hi)
    ax.set_xticks(data["ticks"])
    ax.set_yticks(data["ticks"])
    ax.legend(frameon=False, loc="upper left")
    set_common_style(ax)
    fig.tight_layout()
    fig.savefig(OUT_DIR / f"teste_delta_linear_{axis}.png")
    plt.close(fig)

    fig, ax = plt.subplots(figsize=(8.4, 4.6), dpi=180)
    x = np.arange(len(data["jumps"]))
    ax.bar(x, data["jumps"], color=data["color"], alpha=0.82)
    ax.axhline(5.0, color="#404040", linewidth=1.6, linestyle="--", label="Salto pretendido: 5 mm")
    ax.set_title(f"Tamanho dos saltos medidos - {data['title']}")
    ax.set_xlabel("Transição pretendida [mm]")
    ax.set_ylabel("Salto medido [mm]")
    ax.set_xticks(x)
    ax.set_xticklabels(data["jumps_labels"], rotation=45, ha="right")
    ax.set_ylim(0, max(7.0, max(data["jumps"]) + 0.8))
    ax.legend(frameon=False, loc="upper right")
    set_common_style(ax)
    fig.tight_layout()
    fig.savefig(OUT_DIR / f"teste_delta_saltos_{axis}.png")
    plt.close(fig)


heartbeat_intended = np.array([60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180, 190, 200], dtype=float)
heartbeat_measured = np.array([57.1, 66.5, 77.2, 90.7, 100.3, 109.8, 119.1, 128.3, 138.1, 146.7, 156.1, 174.5, 182.9, 188.4, 192.0], dtype=float)

fig, ax = plt.subplots(figsize=(7.6, 5.0), dpi=180)
ax.plot([60, 200], [60, 200], color="#404040", linewidth=1.6, linestyle="--", label="Frequência pretendida")
ax.plot(
    heartbeat_intended,
    heartbeat_measured,
    color="#b84a62",
    marker="o",
    linewidth=2.2,
    label="Frequência medida",
)
ax.set_title("Comparação entre frequência cardíaca pretendida e medida")
ax.set_xlabel("Frequência configurada [bpm]")
ax.set_ylabel("Frequência medida [bpm]")
ax.set_xlim(55, 205)
ax.set_ylim(55, 205)
ax.set_xticks(np.arange(60, 201, 10))
ax.set_yticks(np.arange(60, 201, 20))
ax.legend(frameon=False, loc="upper left")
set_common_style(ax)
fig.tight_layout()
fig.savefig(OUT_DIR / "teste_batimento_frequencia.png")
plt.close(fig)


respiration_intended = np.array([12, 13, 14, 15, 16, 17, 18, 19, 20], dtype=float)
respiration_measured = np.array([12.0, 12.9, 14.0, 14.9, 15.9, 16.8, 17.8, 18.8, 19.8], dtype=float)

fig, ax = plt.subplots(figsize=(7.6, 5.0), dpi=180)
ax.plot([12, 20], [12, 20], color="#404040", linewidth=1.6, linestyle="--", label="Frequência pretendida")
ax.plot(
    respiration_intended,
    respiration_measured,
    color="#2f7f75",
    marker="o",
    linewidth=2.2,
    label="Frequência medida",
)
ax.set_title("Comparação entre frequência respiratória pretendida e medida")
ax.set_xlabel("Frequência configurada [resp/min]")
ax.set_ylabel("Frequência medida [resp/min]")
ax.set_xlim(11.5, 20.5)
ax.set_ylim(11.5, 20.5)
ax.set_xticks(np.arange(12, 21, 1))
ax.set_yticks(np.arange(12, 21, 1))
ax.legend(frameon=False, loc="upper left")
set_common_style(ax)
fig.tight_layout()
fig.savefig(OUT_DIR / "teste_respiracao_frequencia.png")
plt.close(fig)
