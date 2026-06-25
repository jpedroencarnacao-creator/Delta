"""
calculo_movimentos.py — Cálculo e geração de pontos de movimento para o ESP32.

Ficheiro separado do RB_PI4B_Main_WD.py, chamado por ele quando o utilizador
clica em "Calcular" dentro da aba "Configurações Movimentos".

Não faz plt.show() — devolve as figuras como imagens base64 para serem
exibidas diretamente na aba HTML, sem janelas externas.
"""

import io
import base64
import numpy as np
import matplotlib
matplotlib.use('Agg')   # backend sem janela — essencial para uso em Flask/web
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D   # noqa: F401 — necessário para projection='3d'


# ===========================================================================
# PARÂMETROS DAS CURVAS (modificáveis pelo utilizador via aba)
# ===========================================================================

# --- Curva 1: Elipse Rotacionada ---
DEFAULTS_CURVA1 = {
    'a':           5,       # semi-eixo maior
    'b':           2,       # semi-eixo menor
    'xc':          3.6,     # centro X
    'yc':          3.6,     # centro Y
    'alpha_deg':   45,      # rotação em graus
    'n_curva':     300,     # pontos para desenhar a curva contínua
    'n_pontos':    10,      # pontos de passagem enviados ao ESP32
    'sentido':     'ccw',   # 'ccw' (anti-horário) ou 'cw' (horário)
}

# --- Curva 2: Parábola aberta para Z+ (plano ZX, 1º quadrante) ---
DEFAULTS_CURVA2 = {
    'altura':      20,      # altura máxima Z
    'largura':     12,      # largura máxima X
    'n_curva':     300,
    'n_pontos':    10,
    'theta_deg':   0,       # inclinação da curva no espaço 3D
}


# ===========================================================================
# FUNÇÕES DE CÁLCULO
# ===========================================================================

def calcular_curva1(params=None):
    """Calcula os pontos da Curva 1 (elipse rotacionada).
    Devolve (x_curva, y_curva, x_pontos, y_pontos) como arrays numpy."""
    p = {**DEFAULTS_CURVA1, **(params or {})}

    alpha = np.radians(p['alpha_deg'])

    def elipse(u):
        x = p['xc'] + p['a'] * np.cos(u) * np.cos(alpha) - p['b'] * np.sin(u) * np.sin(alpha)
        y = p['yc'] + p['a'] * np.cos(u) * np.sin(alpha) + p['b'] * np.sin(u) * np.cos(alpha)
        return x, y

    # curva contínua para o plot
    u_curva = np.linspace(0, 2 * np.pi, p['n_curva'])
    x_curva, y_curva = elipse(u_curva)

    # pontos de passagem
    u_pontos = np.linspace(0, 2 * np.pi, p['n_pontos'], endpoint=False)
    x_pontos, y_pontos = elipse(u_pontos)

    # ponto de início = o mais próximo da origem
    idx_inicio = np.argmin(x_pontos ** 2 + y_pontos ** 2)
    x_pontos = np.roll(x_pontos, -idx_inicio)
    y_pontos = np.roll(y_pontos, -idx_inicio)

    # inverter sentido se horário
    if p['sentido'] == 'cw':
        x0, y0 = x_pontos[0], y_pontos[0]
        x_pontos = np.concatenate(([x0], x_pontos[1:][::-1]))
        y_pontos = np.concatenate(([y0], y_pontos[1:][::-1]))

    return x_curva, y_curva, x_pontos, y_pontos


def calcular_curva2(params=None):
    """Calcula os pontos da Curva 2 (parábola no 1º quadrante, plano ZX).
    Devolve (x_curva, z_curva, x_pontos, z_pontos) como arrays numpy."""
    p = {**DEFAULTS_CURVA2, **(params or {})}

    def parabola(x):
        return (p['altura'] / p['largura'] ** 2) * x ** 2

    x_curva  = np.linspace(0, p['largura'], p['n_curva'])
    z_curva  = parabola(x_curva)
    x_pontos = np.linspace(0, p['largura'], p['n_pontos'])
    z_pontos = parabola(x_pontos)

    return x_curva, z_curva, x_pontos, z_pontos


def calcular_3d_curva1(x_pontos, y_pontos):
    """Converte pontos 2D da Curva 1 para 3D (z=0 no plano XY)."""
    return x_pontos, y_pontos, np.zeros_like(x_pontos)


def calcular_3d_curva2(x_pontos, z_pontos, theta_deg=0):
    """Converte pontos 2D da Curva 2 para 3D com rotação theta."""
    theta = np.radians(theta_deg)
    y_3d = z_pontos * np.sin(theta)
    z_3d = z_pontos * np.cos(theta)
    return x_pontos, y_3d, z_3d


# ===========================================================================
# GERAÇÃO DOS GRÁFICOS (devolve imagem base64, sem janela)
# ===========================================================================

def _fig_to_base64(fig):
    """Converte uma figura matplotlib para string base64 PNG."""
    buf = io.BytesIO()
    fig.savefig(buf, format='png', bbox_inches='tight', dpi=100)
    buf.seek(0)
    encoded = base64.b64encode(buf.read()).decode('utf-8')
    plt.close(fig)
    return 'data:image/png;base64,' + encoded


def gerar_graficos(params_c1=None, params_c2=None):
    """Calcula as duas curvas e gera o gráfico 2x2.
    Devolve um dicionário com:
      - 'img': string base64 do gráfico completo
      - 'pontos_c1': dict com x, y, z dos pontos da Curva 1
      - 'pontos_c2': dict com x, y, z dos pontos da Curva 2
    """
    p1 = {**DEFAULTS_CURVA1, **(params_c1 or {})}
    p2 = {**DEFAULTS_CURVA2, **(params_c2 or {})}

    # calcular curvas
    x_c1, y_c1, xp1, yp1        = calcular_curva1(p1)
    x_c2, z_c2, xp2, zp2        = calcular_curva2(p2)
    xp1_3d, yp1_3d, zp1_3d      = calcular_3d_curva1(xp1, yp1)
    xp2_3d, yp2_3d, zp2_3d      = calcular_3d_curva2(xp2, zp2, p2['theta_deg'])

    # ----- figura 2x2 -----
    fig = plt.figure(figsize=(13, 9))

    # 1 — Curva 1 2D
    ax1 = fig.add_subplot(2, 2, 1)
    ax1.plot(x_c1, y_c1, color='blue', linewidth=2, label='curva 1')
    ax1.scatter(xp1, yp1, color='red', s=70, label='pontos')
    ax1.scatter([p1['xc']], [p1['yc']], color='green', s=80, label='centro')
    for i, (x, y) in enumerate(zip(xp1, yp1)):
        ax1.text(x + 0.12, y + 0.12, str(i), color='black', fontsize=9)
    ax1.axhline(0, color='gray', linewidth=0.8)
    ax1.axvline(0, color='gray', linewidth=0.8)
    ax1.grid(True)
    ax1.axis('equal')
    ax1.set_xlabel('X')
    ax1.set_ylabel('Y')
    ax1.set_title('Curva 1 — 2D')
    ax1.legend(fontsize=8)

    # 2 — Curva 1 3D
    ax2 = fig.add_subplot(2, 2, 2, projection='3d')
    ax2.plot(x_c1, y_c1, np.zeros_like(x_c1), color='black', linewidth=1.5, label='curva 1')
    ax2.scatter(xp1_3d, yp1_3d, zp1_3d, color='magenta', s=70, label='pontos')
    lim1 = max(abs(x_c1).max(), abs(y_c1).max(), 2) + 2
    ax2.set_xlim(-lim1, lim1)
    ax2.set_ylim(-lim1, lim1)
    ax2.set_zlim(-lim1, lim1)
    ax2.plot([-lim1, lim1], [0, 0], [0, 0], color='red',   linewidth=1.5)
    ax2.plot([0, 0], [-lim1, lim1], [0, 0], color='green', linewidth=1.5)
    ax2.plot([0, 0], [0, 0], [-lim1, lim1], color='blue',  linewidth=1.5)
    ax2.set_xlabel('X', color='red')
    ax2.set_ylabel('Y', color='green')
    ax2.set_zlabel('Z', color='blue')
    ax2.set_title('Curva 1 — 3D')
    ax2.legend(fontsize=8)
    ax2.view_init(elev=35, azim=-85)

    # 3 — Curva 2 2D
    ax3 = fig.add_subplot(2, 2, 3)
    ax3.plot(x_c2, z_c2, color='orange', linewidth=2, label='curva 2')
    ax3.scatter(xp2, zp2, color='purple', s=70, label='pontos')
    ax3.axhline(0, color='gray', linewidth=0.8)
    ax3.axvline(0, color='gray', linewidth=0.8)
    ax3.grid(True)
    ax3.set_aspect('equal', adjustable='box')
    ax3.set_xlim(0, p2['largura'] + 1)
    ax3.set_ylim(0, p2['altura'] + 1)
    ax3.set_xlabel('X')
    ax3.set_ylabel('Z')
    ax3.set_title('Curva 2 — 2D (plano ZX)')
    ax3.legend(fontsize=8)

    # 4 — Curva 2 3D
    ax4 = fig.add_subplot(2, 2, 4, projection='3d')
    x_c2_3d = x_c2
    y_c2_3d = z_c2 * np.sin(np.radians(p2['theta_deg']))
    z_c2_3d = z_c2 * np.cos(np.radians(p2['theta_deg']))
    ax4.plot(x_c2_3d, y_c2_3d, z_c2_3d, color='black', linewidth=1.5, label='curva 2')
    ax4.scatter(xp2_3d, yp2_3d, zp2_3d, color='purple', s=70, label='pontos')
    lim2 = max(abs(x_c2_3d).max(), abs(z_c2_3d).max(), 2) + 2
    ax4.set_xlim(-lim2, lim2)
    ax4.set_ylim(-lim2, lim2)
    ax4.set_zlim(-lim2, lim2)
    ax4.plot([-lim2, lim2], [0, 0], [0, 0], color='red',   linewidth=1.5)
    ax4.plot([0, 0], [-lim2, lim2], [0, 0], color='green', linewidth=1.5)
    ax4.plot([0, 0], [0, 0], [-lim2, lim2], color='blue',  linewidth=1.5)
    ax4.set_xlabel('X', color='red')
    ax4.set_ylabel('Y', color='green')
    ax4.set_zlabel('Z', color='blue')
    ax4.set_title(f'Curva 2 — 3D (θ={p2["theta_deg"]}°)')
    ax4.legend(fontsize=8)
    ax4.view_init(elev=10, azim=-85)

    plt.tight_layout()
    img_b64 = _fig_to_base64(fig)

    return {
        'img': img_b64,
        'pontos_c1': {
            'x': xp1_3d.tolist(),
            'y': yp1_3d.tolist(),
            'z': zp1_3d.tolist(),
        },
        'pontos_c2': {
            'x': xp2_3d.tolist(),
            'y': yp2_3d.tolist(),
            'z': zp2_3d.tolist(),
        },
    }
