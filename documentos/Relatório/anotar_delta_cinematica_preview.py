from pathlib import Path
from math import atan2, cos, sin, radians

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "documentos" / "Relatório" / "figuras" / "Imagem lateral do Manipulador Delta.png"
OUT_DIR = ROOT / "tmp_video_analysis"
OUT_DIR.mkdir(parents=True, exist_ok=True)
OUT = OUT_DIR / "delta_cinematica_anotada_preview_v3.png"
CLEAN_OUT = OUT_DIR / "delta_cinematica_base_limpa_preview.png"

SCALE = 3


def sc(p):
    return tuple(int(round(v * SCALE)) for v in p)


def font(size, bold=False):
    candidates = [
        r"C:\Windows\Fonts\arialbd.ttf" if bold else r"C:\Windows\Fonts\arial.ttf",
        r"C:\Windows\Fonts\calibrib.ttf" if bold else r"C:\Windows\Fonts\calibri.ttf",
    ]
    for candidate in candidates:
        if Path(candidate).exists():
            return ImageFont.truetype(candidate, size * SCALE)
    return ImageFont.load_default()


def text(draw, xy, value, fill, size=22, bold=True, anchor=None):
    draw.text(sc(xy), value, fill=fill, font=font(size, bold), anchor=anchor)


def line(draw, pts, fill, width=4):
    draw.line([sc(p) for p in pts], fill=fill, width=width * SCALE, joint="curve")


def dashed_line(draw, p1, p2, fill, width=3, dash=13, gap=9):
    x1, y1 = p1
    x2, y2 = p2
    dx = x2 - x1
    dy = y2 - y1
    length = (dx * dx + dy * dy) ** 0.5
    if length <= 0:
        return
    ux, uy = dx / length, dy / length
    d = 0
    while d < length:
        e = min(d + dash, length)
        line(draw, [(x1 + ux * d, y1 + uy * d), (x1 + ux * e, y1 + uy * e)], fill, width)
        d += dash + gap


def arrow(draw, p1, p2, fill, width=4, head=15):
    line(draw, [p1, p2], fill, width)
    ang = atan2(p2[1] - p1[1], p2[0] - p1[0])
    left = (p2[0] - head * cos(ang - radians(30)), p2[1] - head * sin(ang - radians(30)))
    right = (p2[0] - head * cos(ang + radians(30)), p2[1] - head * sin(ang + radians(30)))
    draw.polygon([sc(p2), sc(left), sc(right)], fill=fill)


def arc(draw, center, radius, start_deg, end_deg, fill, width=4):
    x, y = center
    bbox = (x - radius, y - radius, x + radius, y + radius)
    draw.arc(sc_bbox(bbox), start=start_deg, end=end_deg, fill=fill, width=width * SCALE)


def sc_bbox(bbox):
    return tuple(int(round(v * SCALE)) for v in bbox)


def remove_existing_annotations(img):
    """Remove the previous coloured annotations from the report image.

    The file in the report is not the clean render; it already contains coloured
    axes and labels. This keeps the black CAD lines and wipes only saturated
    annotation colours, plus the old dashed construction line.
    """
    px = img.load()
    w, h = img.size
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y][:3]
            maxc = max(r, g, b)
            minc = min(r, g, b)
            saturated = maxc - minc > 24 and maxc > 105
            blue_green = b > 120 or g > 135
            red_or_orange = r > 135 and (g < 170 or b < 120)
            if saturated and (blue_green or red_or_orange):
                px[x, y] = (255, 255, 255)

    draw = ImageDraw.Draw(img)
    # Old black dashed construction line visible in the current report image.
    for offset in (-5, -3, -1, 1, 3, 5):
        draw.line([(386 + offset, 141), (459 + offset, 363)], fill=(255, 255, 255), width=3)
    return img


def main():
    img = Image.open(SRC).convert("RGB")
    img = remove_existing_annotations(img)
    img.save(CLEAN_OUT)

    img = img.resize((img.width * SCALE, img.height * SCALE), Image.Resampling.LANCZOS)
    draw = ImageDraw.Draw(img)

    # Anchor points adjusted to the visible joints in the clean side view.
    origin = (408, 141)
    top_joint = (442, 141)
    servo_axis = (407, 393)
    lower_joint = (462, 360)
    arm_mid = (430, 248)
    right_upper = (600, 216)
    right_elbow = (807, 315)
    right_servo = (836, 390)

    blue = (25, 55, 220)
    green = (0, 155, 80)
    red = (185, 20, 28)
    orange = (230, 125, 0)
    black = (15, 15, 15)
    gray = (105, 105, 105)

    # Reference axes, placed at the platform centre.
    arrow(draw, origin, (545, 141), blue, width=4, head=16)
    arrow(draw, origin, (408, 45), green, width=4, head=16)
    arrow(draw, origin, (360, 174), black, width=3, head=12)
    text(draw, (552, 130), "X", blue, size=24, anchor="lm")
    text(draw, (416, 40), "Z", green, size=24, anchor="lm")
    text(draw, (351, 185), "Y", black, size=22, anchor="mm")

    # Main projected length used by the lateral IK.
    arrow(draw, origin, servo_axis, red, width=4, head=0)
    dashed_line(draw, origin, lower_joint, gray, width=3, dash=13, gap=9)
    text(draw, (377, 268), "L2p", red, size=23, anchor="rm")

    # Active and passive links.
    line(draw, [top_joint, lower_joint], red, width=5)
    text(draw, (477, 238), "L2", red, size=22, anchor="lm")
    line(draw, [servo_axis, lower_joint], orange, width=5)
    text(draw, (438, 407), "L1", orange, size=21, anchor="lm")

    # Extra dimensions visible on the inspiration drawing.
    line(draw, [right_upper, right_elbow], black, width=5)
    text(draw, (713, 227), "L2", black, size=23, anchor="mm")
    line(draw, [right_elbow, right_servo], black, width=5)
    text(draw, (853, 357), "L1", black, size=21, anchor="lm")
    arrow(draw, origin, top_joint, gray, width=3, head=10)
    text(draw, (425, 121), "L3", gray, size=18, anchor="mm")

    # Local construction arrows and angles.
    arrow(draw, servo_axis, lower_joint, orange, width=4, head=12)
    text(draw, (426, 426), "Y'", orange, size=19, anchor="mm")

    arc(draw, servo_axis, 48, 300, 354, orange, width=4)
    text(draw, (454, 382), "theta", orange, size=17, anchor="lm")

    arc(draw, lower_joint, 56, 197, 255, red, width=4)
    text(draw, (424, 337), "phi", red, size=17, anchor="rm")

    arc(draw, origin, 55, 62, 90, black, width=4)
    text(draw, (440, 91), "omega", black, size=17, anchor="lm")

    arc(draw, lower_joint, 43, 244, 307, green, width=4)
    text(draw, (481, 329), "gamma", green, size=17, anchor="lm")

    # Compact legend, kept outside the mechanism and without opaque boxes.
    lx, ly = 46, 45
    text(draw, (lx, ly), "theta -> angulo do servo", green, size=15, anchor="lm")
    text(draw, (lx, ly + 24), "phi -> angulo intermedio", red, size=15, anchor="lm")
    text(draw, (lx, ly + 48), "omega -> angulo auxiliar", black, size=15, anchor="lm")
    text(draw, (lx, ly + 72), "gamma -> geometria local", orange, size=15, anchor="lm")

    img = img.resize((img.width // SCALE, img.height // SCALE), Image.Resampling.LANCZOS)
    img.save(OUT)
    print(OUT)


if __name__ == "__main__":
    main()
