#!/usr/bin/env python3
"""Generates the Svitlo app icon (an original design: an amber lens aperture
on a dark tile).

Outputs, relative to the project root:
    resources/icons/svitlo.png    1024x1024, used as the window icon
    resources/icons/Svitlo.icns   macOS app bundle icon

Requires Pillow and numpy:  pip install pillow numpy
Run from anywhere:          python3 packaging/icon/make_icon.py
"""

import math
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

SS = 4            # supersampling factor
N = 1024 * SS     # working canvas size

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT_DIR = os.path.join(ROOT, "resources", "icons")


def s(v):
    """Scale a 1024-space coordinate to the working canvas."""
    return v * SS


def linear_gradient(size, c0, c1, angle_deg):
    """RGBA gradient image from c0 to c1 along angle_deg."""
    w, h = size
    a = math.radians(angle_deg)
    y, x = np.mgrid[0:h, 0:w].astype(np.float32)
    t = (x / w - 0.5) * math.cos(a) + (y / h - 0.5) * math.sin(a)
    t = (t - t.min()) / (t.max() - t.min())
    c0 = np.array(c0, np.float32)
    c1 = np.array(c1, np.float32)
    img = c0 + (c1 - c0) * t[..., None]
    return Image.fromarray(img.clip(0, 255).astype(np.uint8), "RGBA")


def radial_glow(size, centre, radius, colour):
    w, h = size
    y, x = np.mgrid[0:h, 0:w].astype(np.float32)
    d = np.sqrt((x - centre[0]) ** 2 + (y - centre[1]) ** 2) / radius
    alpha = np.clip(1.0 - d, 0, 1) ** 2.2 * colour[3]
    img = np.zeros((h, w, 4), np.float32)
    img[..., :3] = colour[:3]
    img[..., 3] = alpha
    return Image.fromarray(img.astype(np.uint8), "RGBA")


def mask_from(draw_fn):
    m = Image.new("L", (N, N), 0)
    draw_fn(ImageDraw.Draw(m))
    return m


def build():
    canvas = Image.new("RGBA", (N, N), (0, 0, 0, 0))

    # --- Tile: macOS icon grid (824px body inside 1024, ~22% corner radius).
    body = (s(100), s(100), s(924), s(924))
    radius = s(185)

    shadow = mask_from(lambda d: d.rounded_rectangle(
        (body[0], body[1] + s(14), body[2], body[3] + s(14)), radius, fill=110))
    shadow = shadow.filter(ImageFilter.GaussianBlur(s(18)))
    canvas.paste(Image.new("RGBA", (N, N), (0, 0, 0, 255)), (0, 0), shadow)

    tile_mask = mask_from(lambda d: d.rounded_rectangle(body, radius, fill=255))
    tile = linear_gradient((N, N), (52, 54, 59, 255), (20, 21, 23, 255), 90)
    canvas.paste(tile, (0, 0), tile_mask)

    # Faint top highlight on the tile edge.
    rim = mask_from(lambda d: d.rounded_rectangle(body, radius, outline=255, width=s(3)))
    rim_col = linear_gradient((N, N), (255, 255, 255, 60), (255, 255, 255, 0), 90)
    canvas.paste(rim_col, (0, 0), Image.fromarray(
        (np.array(rim, np.float32) * np.array(rim_col.split()[3], np.float32) / 255).astype(np.uint8)))

    # --- Aperture.
    cx = cy = s(512)
    R = s(292)          # outer radius of the blades
    r_open = s(104)     # radius of the hexagonal opening
    blades = 6
    rot = math.radians(-12)

    ring_mask = mask_from(lambda d: d.ellipse((cx - R, cy - R, cx + R, cy + R), fill=255))
    amber = linear_gradient((N, N), (255, 206, 92, 255), (222, 120, 38, 255), 55)

    # Opening polygon.
    verts = [(cx + r_open * math.cos(rot + i * 2 * math.pi / blades),
              cy + r_open * math.sin(rot + i * 2 * math.pi / blades)) for i in range(blades)]
    hole = mask_from(lambda d: d.polygon(verts, fill=255))
    blades_mask = Image.fromarray(np.clip(
        np.array(ring_mask, np.int16) - np.array(hole, np.int16), 0, 255).astype(np.uint8))
    canvas.paste(amber, (0, 0), blades_mask)

    # Blade seams: from each opening vertex, along the next edge's direction,
    # out to the rim — gives the familiar pinwheel of an iris diaphragm.
    seams = Image.new("L", (N, N), 0)
    sd = ImageDraw.Draw(seams)
    for i in range(blades):
        x0, y0 = verts[i]
        x1, y1 = verts[(i + 1) % blades]
        dx, dy = x1 - x0, y1 - y0
        L = math.hypot(dx, dy)
        dx, dy = dx / L, dy / L
        # Solve |P0 + t*D - C| = R for t > 0.
        px, py = x0 - cx, y0 - cy
        b = px * dx + py * dy
        c = px * px + py * py - R * R
        t = -b + math.sqrt(b * b - c)
        sd.line((x0, y0, x0 + dx * (t + s(4)), y0 + dy * (t + s(4))), fill=255, width=s(13))
    seams = Image.fromarray(np.minimum(np.array(seams), np.array(ring_mask)))
    canvas.paste(Image.new("RGBA", (N, N), (24, 25, 27, 255)), (0, 0), seams)

    # Soft shading on the blades: darker towards the rim.
    yy, xx = np.mgrid[0:N, 0:N].astype(np.float32)
    dist = np.sqrt((xx - cx) ** 2 + (yy - cy) ** 2) / R
    shade_alpha = (np.clip((dist - 0.55) / 0.45, 0, 1) ** 1.6 * 70).astype(np.uint8)
    shade_alpha = np.minimum(shade_alpha, np.array(blades_mask))
    canvas.paste(Image.new("RGBA", (N, N), (90, 30, 0, 255)), (0, 0), Image.fromarray(shade_alpha))

    # Thin dark outer rim of the lens.
    outer = mask_from(lambda d: d.ellipse((cx - R, cy - R, cx + R, cy + R), outline=255, width=s(10)))
    canvas.paste(Image.new("RGBA", (N, N), (24, 25, 27, 255)), (0, 0), outer)

    # --- Light through the opening ("svitlo" = light).
    canvas.alpha_composite(radial_glow((N, N), (cx, cy), s(150), (255, 236, 190, 255)))
    core = mask_from(lambda d: d.ellipse((cx - s(30), cy - s(30), cx + s(30), cy + s(30)), fill=255))
    core = core.filter(ImageFilter.GaussianBlur(s(6)))
    canvas.paste(Image.new("RGBA", (N, N), (255, 251, 240, 255)), (0, 0), core)

    return canvas.resize((1024, 1024), Image.LANCZOS)


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    icon = build()
    png_path = os.path.join(OUT_DIR, "svitlo.png")
    icns_path = os.path.join(OUT_DIR, "Svitlo.icns")
    icon.save(png_path)
    # Pillow writes all standard ICNS sizes (16..1024) from the 1024 image.
    icon.save(icns_path, format="ICNS")
    print("wrote", png_path)
    print("wrote", icns_path)


if __name__ == "__main__":
    main()
