import numpy as np
import matplotlib.pyplot as plt

# =========================
# PARAMETROS DA CURVA
# =========================
a = 5
b = 2
xc = 5
yc = 5
alpha_deg = 45

n_curva = 300
n_pontos = 10

# =========================
# FUNCAO DA CURVA
# =========================
def curva(u, a, b, xc, yc, alpha_deg):
    alpha = np.radians(alpha_deg)

    x = xc + a * np.cos(u) * np.cos(alpha) - b * np.sin(u) * np.sin(alpha)
    y = yc + a * np.cos(u) * np.sin(alpha) + b * np.sin(u) * np.cos(alpha)

    return x, y

# =========================
# CURVA COMPLETA
# =========================
u_curva = np.linspace(0, 2 * np.pi, n_curva)
x_curva, y_curva = curva(u_curva, a, b, xc, yc, alpha_deg)

# =========================
# PONTOS DESTACADOS
# =========================
u_pontos = np.linspace(0, 2 * np.pi, n_pontos, endpoint=False)
x_pontos, y_pontos = curva(u_pontos, a, b, xc, yc, alpha_deg)

# =========================
# DADOS 3D SEM CONVERSAO
# =========================
x_curva_3d = x_curva
y_curva_3d = y_curva
z_curva_3d = np.zeros_like(x_curva)

x_pontos_3d = x_pontos
y_pontos_3d = y_pontos
z_pontos_3d = np.zeros_like(x_pontos)

# =========================
# UMA SO JANELA COM 2 SUBPLOTS
# =========================
fig = plt.figure(figsize=(14, 6))

# subplot 2D
ax1 = fig.add_subplot(1, 2, 1)
ax1.plot(x_curva, y_curva, color="blue", linewidth=2, label="curva 2D")
ax1.scatter(x_pontos, y_pontos, color="red", s=70, label="pontos 2D")
ax1.scatter([xc], [yc], color="green", s=80, label="centro")
ax1.axhline(0, color="gray", linewidth=0.8)
ax1.axvline(0, color="gray", linewidth=0.8)
ax1.grid(True)
ax1.axis("equal")
ax1.set_xlabel("X")
ax1.set_ylabel("Y")
ax1.set_title("Curva 2D original")
ax1.legend()

# subplot 3D
ax2 = fig.add_subplot(1, 2, 2, projection='3d')

ax2.plot(x_curva_3d, y_curva_3d, z_curva_3d, color="black", linewidth=1.5, label="trajetoria 3D")
ax2.scatter(x_pontos_3d, y_pontos_3d, z_pontos_3d, color="magenta", s=70, label="pontos 3D")

ax2.set_xlabel("X", color="red")
ax2.set_ylabel("Y", color="green")
ax2.set_zlabel("Z", color="blue")
ax2.set_title("Curva no espaco 3D")
ax2.legend()
ax2.view_init(elev=35, azim=-85)

# =========================
# LIMITES SIMETRICOS
# =========================
max_x = max(abs(np.min(x_curva_3d)), abs(np.max(x_curva_3d)))
max_y = max(abs(np.min(y_curva_3d)), abs(np.max(y_curva_3d)))
max_z = 2  # como agora z=0, damos alguma profundidade manual

limite = max(max_x, max_y, max_z) + 2

ax2.set_xlim(-limite, limite)
ax2.set_ylim(-limite, limite)
ax2.set_zlim(-limite, limite)

# =========================
# EIXOS VERMELHOS NO MEIO
# =========================
# eixo X
ax2.plot([-limite, limite], [0, 0], [0, 0], color="red", linewidth=2)

# eixo Y
ax2.plot([0, 0], [-limite, limite], [0, 0], color="green", linewidth=2)

# eixo Z
ax2.plot([0, 0], [0, 0], [-limite, limite], color="blue", linewidth=2)

#plot3d
plt.tight_layout()
plt.show()
# =========================
# CONVERSAO APENAS DEPOIS DOS PLOTS
# =========================
x_manip = x_pontos
y_manip = -y_pontos
z_manip = np.zeros_like(x_pontos)

pontos_strings = [
    f"X={x:.3f}, Y={y:.3f}, Z={z:.3f}"
    for x, y, z in zip(x_manip, y_manip, z_manip)
]
print("Pontos convertidos para o manipulador:")
for s in pontos_strings:
    print(s)
    

print("Pontos convertidos para o manipulador:")

x_string = "X: " + ", ".join(f"{x:.3f}" for x in x_manip)
y_string = "Y: " + ", ".join(f"{y:.3f}" for y in y_manip)
z_string = "Z: " + ", ".join(f"{z:.3f}" for z in z_manip)

print(x_string)
print(y_string)
print(z_string)