from pathlib import Path
from math import atan2, cos, sin, radians

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "documentos" / "Imagens e Videos" / "datasheet" / "Captura de ecrã 2026-07-14 111649.png"
OUT_DIR = ROOT / "tmp_video_analysis"
OUT_DIR.mkdir(parents=True, exist_ok=True)
OUT = OUT_DIR / "delta_cinematica_limpa_anotada_v2.png"

SCALE = 4


def sc(p):
    return tuple(int(round(v * SCALE)) for v in p)


def sc_box(box):
    return tuple(int(round(v * SCALE)) for v in box)


def font(size, bold=False):
    names = [
        r"C:\Windows\Fonts\arialbd.ttf" if bold else r"C:\Windows\Fonts\arial.ttf",
        r"C:\Windows\Fonts\calibrib.ttf" if bold else r"C:\Windows\Fonts\calibri.ttf",
    ]
    for name in names:
        path = Path(name)
        if path.exists():
            return ImageFont.truetype(path, size * SCALE)
    return ImageFont.load_default()


def line(draw, pts, color, width=3):
    draw.line([sc(p) for p in pts], fill=color, width=width * SCALE, joint="curve")


def arrow(draw, p1, p2, color, width=3, head=12):
    line(draw, [p1, p2], color, width)
    if head <= 0:
        return
    angle = atan2(p2[1] - p1[1], p2[0] - p1[0])
    left = (p2[0] - head * cos(angle - radians(28)), p2[1] - head * sin(angle - radians(28)))
    right = (p2[0] - head * cos(angle + radians(28)), p2[1] - head * sin(angle + radians(28)))
    draw.polygon([sc(p2), sc(left), sc(right)], fill=color)


def double_arrow(draw, p1, p2, color, width=3, head=10):
    arrow(draw, p1, p2, color, width, head)
    arrow(draw, p2, p1, color, width, head)


def dashed(draw, p1, p2, color, width=2, dash=10, gap=7):
    x1, y1 = p1
    x2, y2 = p2
    dx = x2 - x1
    dy = y2 - y1
    length = (dx * dx + dy * dy) ** 0.5
    if length == 0:
        return
    ux = dx / length
    uy = dy / length
    pos = 0
    while pos < length:
        end = min(pos + dash, length)
        a = (x1 + ux * pos, y1 + uy * pos)
        b = (x1 + ux * end, y1 + uy * end)
        line(draw, [a, b], color, width)
        pos += dash + gap


def text(draw, p, value, color, size=18, bold=True, anchor="mm"):
    draw.text(sc(p), value, font=font(size, bold), fill=color, anchor=anchor)


def arc(draw, center, radius, start, end, color, width=3):
    x, y = center
    draw.arc(sc_box((x - radius, y - radius, x + radius, y + radius)), start, end, fill=color, width=width * SCALE)


def small_dot(draw, p, color, radius=3):
    x, y = p
    draw.ellipse(sc_box((x - radius, y - radius, x + radius, y + radius)), fill=color)


def main():
    img = Image.open(SRC).convert("RGB")
    img = img.resize((img.width * SCALE, img.height * SCALE), Image.Resampling.LANCZOS)
    draw = ImageDraw.Draw(img)

    blue = (20, 65, 220)
    green = (0, 145, 80)
    red = (185, 20, 25)
    orange = (230, 120, 0)
    black = (20, 20, 20)
    gray = (95, 95, 95)

    # Geometric anchor points, manually matched to the clean side-view render.
    origin = (326, 98)
    x_axis_end = (420, 98)
    z_axis_end = (326, 39)
    y_axis_end = (294, 122)

    top_arm_joint = (300, 98)
    lower_arm_joint = (373, 273)
    servo_axis = (378, 292)
    platform_joint = (349, 270)

    right_passive_a = (416, 96)
    right_passive_b = (579, 219)
    right_servo_axis = (597, 277)

    # Coordinate system.
    arrow(draw, origin, x_axis_end, blue, width=3, head=11)
    arrow(draw, origin, z_axis_end, green, width=3, head=11)
    arrow(draw, origin, y_axis_end, black, width=2, head=9)
    text(draw, (427, 96), "X", blue, 19, anchor="lm")
    text(draw, (327, 31), "Z", green, 19, anchor="mm")
    text(draw, (288, 130), "Y", black, 16, anchor="mm")
    small_dot(draw, origin, green, radius=2)

    # Construction lines used in the lateral inverse-kinematics triangle.
    dashed(draw, origin, servo_axis, gray, width=2, dash=10, gap=6)
    dashed(draw, origin, lower_arm_joint, gray, width=2, dash=10, gap=6)
    double_arrow(draw, origin, servo_axis, red, width=3, head=9)
    text(draw, (297, 196), "L2p", red, 17, anchor="rm")

    # Main arm under analysis.
    double_arrow(draw, top_arm_joint, lower_arm_joint, red, width=4, head=9)
    text(draw, (356, 186), "L2", red, 17, anchor="lm")
    line(draw, [servo_axis, lower_arm_joint], orange, width=4)

    # Horizontal/platform offset.
    double_arrow(draw, origin, top_arm_joint, gray, width=2, head=8)
    text(draw, (313, 82), "L3", gray, 13, anchor="mm")
    arrow(draw, servo_axis, platform_joint, orange, width=3, head=9)
    text(draw, (350, 302), "Y'", orange, 15, anchor="mm")

    # Extra dimensions on the visible right arm, matching the inspiration figure.
    double_arrow(draw, right_passive_a, right_passive_b, black, width=3, head=9)
    text(draw, (504, 149), "L2", black, 18, anchor="mm")
    double_arrow(draw, right_passive_b, right_servo_axis, black, width=3, head=9)
    text(draw, (614, 249), "L1", black, 17, anchor="lm")

    # Angle references.
    arc(draw, servo_axis, 40, 257, 332, orange, width=3)
    text(draw, (412, 281), "theta", orange, 14, anchor="lm")

    arc(draw, lower_arm_joint, 42, 195, 258, red, width=3)
    text(draw, (332, 252), "phi", red, 14, anchor="rm")

    arc(draw, origin, 43, 82, 131, black, width=3)
    text(draw, (357, 62), "omega", black, 15, anchor="lm")

    arc(draw, lower_arm_joint, 33, 237, 302, green, width=3)
    text(draw, (392, 247), "gamma", green, 14, anchor="lm")

    # Compact legend outside the mechanism.
    text(draw, (38, 42), "theta -> Theta", green, 13, anchor="lm")
    text(draw, (38, 63), "phi -> Phi", red, 13, anchor="lm")
    text(draw, (38, 84), "omega -> Omega", black, 13, anchor="lm")
    text(draw, (38, 105), "gamma -> Gamma", orange, 13, anchor="lm")

    img = img.resize((img.width // SCALE, img.height // SCALE), Image.Resampling.LANCZOS)
    img.save(OUT)
    print(OUT)


if __name__ == "__main__":
    main()
