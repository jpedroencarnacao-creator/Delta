import numpy as np
import matplotlib.pyplot as plt

# =========================================================
# CURVA 1 - ELIPSE ROTACIONADA
# =========================================================
a1 = 5
b1 = 2
xc1 = 3.6
yc1 = 3.6
alpha_deg1 = 45
n_curva1 = 300
n_pontos1 = 10
sentido_curva1 = "ccw"   # "ccw" ou "cw"

def curva1(u, a, b, xc, yc, alpha_deg):
    alpha = np.radians(alpha_deg)
    x = xc + a * np.cos(u) * np.cos(alpha) - b * np.sin(u) * np.sin(alpha)
    y = yc + a * np.cos(u) * np.sin(alpha) + b * np.sin(u) * np.cos(alpha)
    return x, y

u_curva1 = np.linspace(0, 2 * np.pi, n_curva1)
x_curva1, y_curva1 = curva1(u_curva1, a1, b1, xc1, yc1, alpha_deg1)

u_pontos1 = np.linspace(0, 2 * np.pi, n_pontos1, endpoint=False)
x_pontos1, y_pontos1 = curva1(u_pontos1, a1, b1, xc1, yc1, alpha_deg1)

dist2 = x_pontos1**2 + y_pontos1**2
idx_inicio = np.argmin(dist2)

x_pontos1 = np.roll(x_pontos1, -idx_inicio)
y_pontos1 = np.roll(y_pontos1, -idx_inicio)

if sentido_curva1 == "cw":
    x0 = x_pontos1[0]
    y0 = y_pontos1[0]
    x_rest = x_pontos1[1:][::-1]
    y_rest = y_pontos1[1:][::-1]
    x_pontos1 = np.concatenate(([x0], x_rest))
    y_pontos1 = np.concatenate(([y0], y_rest))

# =========================================================
# CURVA 2 - PARABOLA ABERTA PARA Z+
# apenas no 1º quadrante, no plano ZX
# =========================================================
altura2 = 20
largura2 = 12
n_curva2 = 300
n_pontos2 = 10
theta_deg = 0   # inclinacao da curva 2

def curva2(x, altura, largura):
    z = (altura / (largura ** 2)) * (x ** 2)
    return z

x_curva2 = np.linspace(0, largura2, n_curva2)
z_curva2 = curva2(x_curva2, altura2, largura2)

x_pontos2 = np.linspace(0, largura2, n_pontos2)
z_pontos2 = curva2(x_pontos2, altura2, largura2)

# =========================================================
# DADOS 3D
# =========================================================
x_curva1_3d = x_curva1
y_curva1_3d = y_curva1
z_curva1_3d = np.zeros_like(x_curva1)

x_pontos1_3d = x_pontos1
y_pontos1_3d = y_pontos1
z_pontos1_3d = np.zeros_like(x_pontos1)

theta = np.radians(theta_deg)

x_curva2_3d = x_curva2
y_curva2_3d = z_curva2 * np.sin(theta)
z_curva2_3d = z_curva2 * np.cos(theta)

x_pontos2_3d = x_pontos2
y_pontos2_3d = z_pontos2 * np.sin(theta)
z_pontos2_3d = z_pontos2 * np.cos(theta)

# =========================================================
# FIGURA 2x2
# =========================================================
fig = plt.figure(figsize=(14, 10))

# CURVA 1 - 2D
ax1 = fig.add_subplot(2, 2, 1)
ax1.plot(x_curva1, y_curva1, color='blue', linewidth=2, label='curva 1')
ax1.scatter(x_pontos1, y_pontos1, color='red', s=70, label='pontos curva 1')
ax1.scatter([xc1], [yc1], color='green', s=80, label='centro')
ax1.axhline(0, color='gray', linewidth=0.8)
ax1.axvline(0, color='gray', linewidth=0.8)
ax1.grid(True)
ax1.axis('equal')
ax1.set_xlabel('X')
ax1.set_ylabel('Y')
ax1.set_title('Curva 1 - 2D')
ax1.legend()

# CURVA 1 - 3D
ax2 = fig.add_subplot(2, 2, 2, projection='3d')
ax2.plot(x_curva1_3d, y_curva1_3d, z_curva1_3d, color='black', linewidth=1.5, label='curva 1')
ax2.scatter(x_pontos1_3d, y_pontos1_3d, z_pontos1_3d, color='magenta', s=70, label='pontos curva 1')
for i, (x, y) in enumerate(zip(x_pontos1, y_pontos1)):
    ax1.text(x + 0.12, y + 0.12, str(i), color='black', fontsize=10)
ax2.set_xlabel('X', color='red')
ax2.set_ylabel('Y', color='green')
ax2.set_zlabel('Z', color='blue')
ax2.set_title('Curva 1 - 3D')
ax2.legend()
ax2.view_init(elev=35, azim=-85)

max_x1 = max(abs(np.min(x_curva1_3d)), abs(np.max(x_curva1_3d)))
max_y1 = max(abs(np.min(y_curva1_3d)), abs(np.max(y_curva1_3d)))
max_z1 = 2
limite1 = max(max_x1, max_y1, max_z1) + 2

ax2.set_xlim(-limite1, limite1)
ax2.set_ylim(-limite1, limite1)
ax2.set_zlim(-limite1, limite1)

ax2.plot([-limite1, limite1], [0, 0], [0, 0], color='red', linewidth=2)
ax2.plot([0, 0], [-limite1, limite1], [0, 0], color='green', linewidth=2)
ax2.plot([0, 0], [0, 0], [-limite1, limite1], color='blue', linewidth=2)

# CURVA 2 - 2D
ax3 = fig.add_subplot(2, 2, 3)
ax3.plot(x_curva2, z_curva2, color="orange", linewidth=2, label="curva 2")
ax3.scatter(x_pontos2, z_pontos2, color="purple", s=70, label="pontos curva 2")
ax3.axhline(0, color="gray", linewidth=0.8)
ax3.axvline(0, color="gray", linewidth=0.8)
ax3.grid(True)
ax3.set_aspect('equal', adjustable='box')
ax3.set_xlim(0, largura2 + 1)
ax3.set_ylim(0, altura2 + 1)
ax3.set_xlabel("X")
ax3.set_ylabel("Z")
ax3.set_title("Curva 2 - 2D (1º quadrante, plano ZX)")
ax3.legend()

# CURVA 2 - 3D
ax4 = fig.add_subplot(2, 2, 4, projection='3d')
ax4.plot(x_curva2_3d, y_curva2_3d, z_curva2_3d, color='black', linewidth=1.5, label='curva 2')
ax4.scatter(x_pontos2_3d, y_pontos2_3d, z_pontos2_3d, color='purple', s=70, label='pontos curva 2')
ax4.set_xlabel('X', color='red')
ax4.set_ylabel('Y', color='green')
ax4.set_zlabel('Z', color='blue')
ax4.set_title(f'Curva 2 - 3D (theta = {theta_deg}°)')
ax4.legend()
ax4.view_init(elev=10, azim=-85)

max_x2 = max(abs(np.min(x_curva2_3d)), abs(np.max(x_curva2_3d)))
max_y2 = max(abs(np.min(y_curva2_3d)), abs(np.max(y_curva2_3d)))
max_z2 = max(abs(np.min(z_curva2_3d)), abs(np.max(z_curva2_3d)))
limite2 = max(max_x2, max_y2, max_z2) + 2

ax4.set_xlim(-limite2, limite2)
ax4.set_ylim(-limite2, limite2)
ax4.set_zlim(-limite2, limite2)

ax4.plot([-limite2, limite2], [0, 0], [0, 0], color='red', linewidth=2)
ax4.plot([0, 0], [-limite2, limite2], [0, 0], color='green', linewidth=2)
ax4.plot([0, 0], [0, 0], [-limite2, limite2], color='blue', linewidth=2)

plt.tight_layout()
plt.show()

# =========================================================
# CONVERSAO APENAS DEPOIS DOS PLOTS
# =========================================================
x1_manip = x_pontos1
y1_manip = y_pontos1
z1_manip = np.zeros_like(x_pontos1)

x2_manip = x_pontos2_3d
y2_manip = y_pontos2_3d
z2_manip = z_pontos2_3d

print('\nCurva 1 convertida para o manipulador:')
print('X: ' + ', '.join(f'{x:.3f}' for x in x1_manip))
print('Y: ' + ', '.join(f'{y:.3f}' for y in y1_manip))
print('Z: ' + ', '.join(f'{z:.3f}' for z in z1_manip))

print('\nCurva 2 convertida para o manipulador:')
print('X: ' + ', '.join(f'{x:.3f}' for x in x2_manip))
print('Y: ' + ', '.join(f'{y:.3f}' for y in y2_manip))
print('Z: ' + ', '.join(f'{z:.3f}' for z in z2_manip))