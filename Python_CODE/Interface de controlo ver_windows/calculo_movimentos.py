"""
calculo_movimentos.py — Cálculo e geração de pontos de movimento para o ESP32.

Ficheiro separado do RB_PI4B_Main_WD.py, chamado quando o utilizador
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
# PARÂMETROS — TODAS AS VARIÁVEIS MODIFICÁVEIS ESTÃO AQUI EM CIMA
# ===========================================================================

# --- Curva 1: Elipse Rotacionada ---
# Usada por: Respiração, Batimento Cardíaco
DEFAULTS_CURVA1 = {
    'a':           5,       # semi-eixo maior
    'b':           2,       # semi-eixo menor
    'xc':          3.6,     # centro X da elipse
    'yc':          3.6,     # centro Y da elipse
    'alpha_deg':   45,      # rotação da elipse em graus
    'n_curva':     300,     # nº de pontos para desenhar a curva contínua
    'n_pontos':    10,      # nº de pontos de passagem enviados ao ESP32
    'sentido':     'ccw',   # 'ccw' = anti-horário | 'cw' = horário
}

# --- Curva 2: Parábola aberta para Z+ (plano ZX, 1º quadrante) ---
# Usada por: Tosse
DEFAULTS_CURVA2 = {
    'altura':      20,      # altura máxima Z da parábola
    'largura':     12,      # largura máxima X da parábola
    'n_curva':     300,     # nº de pontos para desenhar a curva contínua
    'n_pontos':    10,      # nº de pontos de passagem enviados ao ESP32
    'theta_deg':   0,       # inclinação da curva no espaço 3D (graus)
}

# --- Curva 3: Circunferência centrada na origem (plano XY) ---
# Usada por: Vibração Tosse
DEFAULTS_CURVA3 = {
    'raio':        2,       # raio da circunferência
    'n_curva':     300,     # nº de pontos para desenhar a curva contínua
    'n_pontos':    10,      # nº de pontos de passagem enviados ao ESP32
    'sentido':     'ccw',   # 'ccw' = anti-horário | 'cw' = horário
}


# ===========================================================================
# FUNÇÕES DE CÁLCULO
# ===========================================================================

def calcular_curva1(params=None):
    """Curva 1: elipse rotacionada no plano XY.
    Devolve (x_curva, y_curva, x_pontos, y_pontos)."""
    p = {**DEFAULTS_CURVA1, **(params or {})}
    alpha = np.radians(p['alpha_deg'])

    def elipse(u):
        x = p['xc'] + p['a'] * np.cos(u) * np.cos(alpha) - p['b'] * np.sin(u) * np.sin(alpha)
        y = p['yc'] + p['a'] * np.cos(u) * np.sin(alpha) + p['b'] * np.sin(u) * np.cos(alpha)
        return x, y

    u_curva  = np.linspace(0, 2 * np.pi, p['n_curva'])
    x_curva, y_curva = elipse(u_curva)

    u_pontos = np.linspace(0, 2 * np.pi, p['n_pontos'], endpoint=False)
    x_pontos, y_pontos = elipse(u_pontos)

    # ponto de início = o mais próximo da origem
    idx = np.argmin(x_pontos ** 2 + y_pontos ** 2)
    x_pontos = np.roll(x_pontos, -idx)
    y_pontos = np.roll(y_pontos, -idx)

    if p['sentido'] == 'cw':
        x0, y0   = x_pontos[0], y_pontos[0]
        x_pontos = np.concatenate(([x0], x_pontos[1:][::-1]))
        y_pontos = np.concatenate(([y0], y_pontos[1:][::-1]))

    return x_curva, y_curva, x_pontos, y_pontos


def calcular_curva2(params=None):
    """Curva 2: parábola no 1º quadrante, plano ZX.
    Devolve (x_curva, z_curva, x_pontos, z_pontos)."""
    p = {**DEFAULTS_CURVA2, **(params or {})}

    def parabola(x):
        return (p['altura'] / p['largura'] ** 2) * x ** 2

    x_curva  = np.linspace(0, p['largura'], p['n_curva'])
    z_curva  = parabola(x_curva)
    x_pontos = np.linspace(0, p['largura'], p['n_pontos'])
    z_pontos = parabola(x_pontos)

    return x_curva, z_curva, x_pontos, z_pontos


def calcular_curva3(params=None):
    """Curva 3: circunferência centrada na origem, plano XY.
    Usa a mesma fórmula da elipse com a=b=raio e sem rotação.
    Devolve (x_curva, y_curva, x_pontos, y_pontos)."""
    p = {**DEFAULTS_CURVA3, **(params or {})}
    r = p['raio']

    def circunferencia(u):
        return r * np.cos(u), r * np.sin(u)

    u_curva  = np.linspace(0, 2 * np.pi, p['n_curva'])
    x_curva, y_curva = circunferencia(u_curva)

    u_pontos = np.linspace(0, 2 * np.pi, p['n_pontos'], endpoint=False)
    x_pontos, y_pontos = circunferencia(u_pontos)

    # ponto de início = o mais próximo do eixo X positivo (ângulo 0)
    # para circunferência centrada na origem, isso é simplesmente o primeiro ponto
    if p['sentido'] == 'cw':
        x0, y0   = x_pontos[0], y_pontos[0]
        x_pontos = np.concatenate(([x0], x_pontos[1:][::-1]))
        y_pontos = np.concatenate(([y0], y_pontos[1:][::-1]))

    return x_curva, y_curva, x_pontos, y_pontos


def calcular_3d_curva1(x_pontos, y_pontos):
    """Curva 1 em 3D: z=0, movimento no plano XY."""
    return x_pontos, y_pontos, np.zeros_like(x_pontos)


def calcular_3d_curva2(x_pontos, z_pontos, theta_deg=0):
    """Curva 2 em 3D: rotação theta em torno do eixo X."""
    theta = np.radians(theta_deg)
    return x_pontos, z_pontos * np.sin(theta), z_pontos * np.cos(theta)


def calcular_3d_curva3(x_pontos, y_pontos):
    """Curva 3 em 3D: z=0, movimento no plano XY (igual à Curva 1)."""
    return x_pontos, y_pontos, np.zeros_like(x_pontos)


# ===========================================================================
# GERAÇÃO DOS GRÁFICOS (imagens base64, sem janela)
# ===========================================================================

def _fig_to_base64(fig):
    """Converte uma figura matplotlib para string base64 PNG."""
    buf = io.BytesIO()
    fig.savefig(buf, format='png', bbox_inches='tight', dpi=100)
    buf.seek(0)
    encoded = base64.b64encode(buf.read()).decode('utf-8')
    plt.close(fig)
    return 'data:image/png;base64,' + encoded


def _fig_2d(plot_fn, title):
    """Cria uma figura 2D com um único subplot."""
    fig, ax = plt.subplots(figsize=(5, 4))
    plot_fn(ax)
    ax.set_title(title, fontsize=11)
    ax.grid(True)
    plt.tight_layout()
    return _fig_to_base64(fig)


def _fig_3d(plot_fn, title):
    """Cria uma figura 3D com um único subplot."""
    fig = plt.figure(figsize=(5, 4))
    ax  = fig.add_subplot(111, projection='3d')
    plot_fn(ax)
    ax.set_title(title, fontsize=11)
    plt.tight_layout()
    return _fig_to_base64(fig)


def _eixos_3d(ax, lim):
    """Desenha os eixos X (vermelho), Y (verde), Z (azul) num subplot 3D."""
    ax.plot([-lim, lim], [0, 0], [0, 0], color='red',   linewidth=1.2)
    ax.plot([0, 0], [-lim, lim], [0, 0], color='green', linewidth=1.2)
    ax.plot([0, 0], [0, 0], [-lim, lim], color='blue',  linewidth=1.2)
    ax.set_xlim(-lim, lim); ax.set_ylim(-lim, lim); ax.set_zlim(-lim, lim)
    ax.set_xlabel('X', color='red')
    ax.set_ylabel('Y', color='green')
    ax.set_zlabel('Z', color='blue')


# ===========================================================================
# FUNÇÃO PRINCIPAL — chamada pelo RB_PI4B_Main_WD.py
# ===========================================================================

def gerar_graficos(params_c1=None, params_c2=None, params_c3=None):
    """Calcula as três curvas e gera gráficos individuais por secção de movimento.

    Devolve um dicionário com:
      'graficos'  — dict com imagens base64 por gráfico:
                    c1_2d, c1_3d                      (Curva 1 — Elipse)
                    respiracao_2d, respiracao_3d       (usa Curva 1)
                    batimento_2d,  batimento_3d        (usa Curva 1)
                    tosse_2d,      tosse_3d            (usa Curva 2)
                    vibracao_tosse_2d, vibracao_tosse_3d (usa Curva 3)
      'pontos_c1' — dict {x, y, z} da Curva 1
      'pontos_c2' — dict {x, y, z} da Curva 2
      'pontos_c3' — dict {x, y, z} da Curva 3
    """
    p1 = {**DEFAULTS_CURVA1, **(params_c1 or {})}
    p2 = {**DEFAULTS_CURVA2, **(params_c2 or {})}
    p3 = {**DEFAULTS_CURVA3, **(params_c3 or {})}

    # --- calcular curvas ---
    x_c1, y_c1, xp1, yp1   = calcular_curva1(p1)
    x_c2, z_c2, xp2, zp2   = calcular_curva2(p2)
    x_c3, y_c3, xp3, yp3   = calcular_curva3(p3)

    xp1_3d, yp1_3d, zp1_3d = calcular_3d_curva1(xp1, yp1)
    xp2_3d, yp2_3d, zp2_3d = calcular_3d_curva2(xp2, zp2, p2['theta_deg'])
    xp3_3d, yp3_3d, zp3_3d = calcular_3d_curva3(xp3, yp3)

    graficos = {}

    # -----------------------------------------------------------------------
    # Curva 1 — 2D
    # -----------------------------------------------------------------------
    def _plot_c1_2d(ax):
        ax.plot(x_c1, y_c1, color='blue', linewidth=2, label='curva 1')
        ax.scatter(xp1, yp1, color='red', s=60, label='pontos')
        ax.scatter([p1['xc']], [p1['yc']], color='green', s=70, label='centro')
        for i, (x, y) in enumerate(zip(xp1, yp1)):
            ax.text(x + 0.1, y + 0.1, str(i), fontsize=8)
        ax.axhline(0, color='gray', linewidth=0.6)
        ax.axvline(0, color='gray', linewidth=0.6)
        ax.axis('equal')
        ax.set_xlabel('X'); ax.set_ylabel('Y')
        ax.legend(fontsize=8)

    graficos['c1_2d'] = _fig_2d(_plot_c1_2d, 'Curva 1 — 2D')

    # -----------------------------------------------------------------------
    # Curva 1 — 3D
    # -----------------------------------------------------------------------
    def _plot_c1_3d(ax):
        lim = max(abs(x_c1).max(), abs(y_c1).max(), 2) + 2
        ax.plot(x_c1, y_c1, np.zeros_like(x_c1), color='black', linewidth=1.5, label='curva 1')
        ax.scatter(xp1_3d, yp1_3d, zp1_3d, color='magenta', s=50, label='pontos')
        _eixos_3d(ax, lim)
        ax.legend(fontsize=8); ax.view_init(elev=35, azim=-85)

    graficos['c1_3d'] = _fig_3d(_plot_c1_3d, 'Curva 1 — 3D')

    # Respiração e Batimento partilham a Curva 1
    graficos['respiracao_2d'] = graficos['c1_2d']
    graficos['respiracao_3d'] = graficos['c1_3d']
    graficos['batimento_2d']  = graficos['c1_2d']
    graficos['batimento_3d']  = graficos['c1_3d']

    # -----------------------------------------------------------------------
    # Curva 2 — 2D  (Tosse)
    # -----------------------------------------------------------------------
    def _plot_c2_2d(ax):
        ax.plot(x_c2, z_c2, color='orange', linewidth=2, label='curva 2')
        ax.scatter(xp2, zp2, color='purple', s=60, label='pontos')
        ax.axhline(0, color='gray', linewidth=0.6)
        ax.axvline(0, color='gray', linewidth=0.6)
        ax.set_aspect('equal', adjustable='box')
        ax.set_xlim(0, p2['largura'] + 1)
        ax.set_ylim(0, p2['altura'] + 1)
        ax.set_xlabel('X'); ax.set_ylabel('Z')
        ax.legend(fontsize=8)

    graficos['tosse_2d'] = _fig_2d(_plot_c2_2d, 'Curva 2 — 2D (plano ZX)')

    # -----------------------------------------------------------------------
    # Curva 2 — 3D  (Tosse)
    # -----------------------------------------------------------------------
    def _plot_c2_3d(ax):
        x_3d = x_c2
        y_3d = z_c2 * np.sin(np.radians(p2['theta_deg']))
        z_3d = z_c2 * np.cos(np.radians(p2['theta_deg']))
        lim  = max(abs(x_3d).max(), abs(z_3d).max(), 2) + 2
        ax.plot(x_3d, y_3d, z_3d, color='black', linewidth=1.5, label='curva 2')
        ax.scatter(xp2_3d, yp2_3d, zp2_3d, color='purple', s=50, label='pontos')
        _eixos_3d(ax, lim)
        ax.legend(fontsize=8); ax.view_init(elev=10, azim=-85)

    graficos['tosse_3d'] = _fig_3d(_plot_c2_3d, f'Curva 2 — 3D (θ={p2["theta_deg"]}°)')

    # -----------------------------------------------------------------------
    # Curva 3 — 2D  (Vibração Tosse — circunferência)
    # -----------------------------------------------------------------------
    def _plot_c3_2d(ax):
        ax.plot(x_c3, y_c3, color='teal', linewidth=2, label=f'curva 3 (r={p3["raio"]})')
        ax.scatter(xp3, yp3, color='darkorange', s=60, label='pontos')
        ax.scatter([0], [0], color='black', s=60, marker='+', label='origem')
        for i, (x, y) in enumerate(zip(xp3, yp3)):
            ax.text(x + 0.08, y + 0.08, str(i), fontsize=8)
        ax.axhline(0, color='gray', linewidth=0.6)
        ax.axvline(0, color='gray', linewidth=0.6)
        ax.axis('equal')
        ax.set_xlabel('X'); ax.set_ylabel('Y')
        ax.legend(fontsize=8)

    graficos['vibracao_tosse_2d'] = _fig_2d(_plot_c3_2d, 'Curva 3 — 2D (circunferência)')

    # -----------------------------------------------------------------------
    # Curva 3 — 3D  (Vibração Tosse — circunferência)
    # -----------------------------------------------------------------------
    def _plot_c3_3d(ax):
        lim = p3['raio'] + 2
        ax.plot(x_c3, y_c3, np.zeros_like(x_c3), color='teal', linewidth=1.5, label='curva 3')
        ax.scatter(xp3_3d, yp3_3d, zp3_3d, color='darkorange', s=50, label='pontos')
        _eixos_3d(ax, lim)
        ax.legend(fontsize=8); ax.view_init(elev=35, azim=-85)

    graficos['vibracao_tosse_3d'] = _fig_3d(_plot_c3_3d, 'Curva 3 — 3D (circunferência)')

    # -----------------------------------------------------------------------
    # Devolver resultados
    # -----------------------------------------------------------------------
    return {
        'graficos': graficos,
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
        'pontos_c3': {
            'x': xp3_3d.tolist(),
            'y': yp3_3d.tolist(),
            'z': zp3_3d.tolist(),
        },
    }
