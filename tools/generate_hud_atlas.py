"""
Generates the Touch HUD Font & Icon Atlas for Split/Second Velocity (SSV).
Creates:
  - tools/hud_atlas_preview.png (visual preview)
  - include/touch_hud_atlas.h (embedded C++ header with pixels and UV metadata)
"""
import os
import math
from PIL import Image, ImageDraw, ImageFont

ATLAS_W = 512
ATLAS_H = 256

atlas = Image.new('L', (ATLAS_W, ATLAS_H), 0)
draw = ImageDraw.Draw(atlas)

# -------------------------------------------------------------------------
# 1. Font Glyphs (ASCII 32 .. 126) - Proportional Packed
# -------------------------------------------------------------------------
font_path = 'C:/Windows/Fonts/segoeuiz.ttf' # Segoe UI Bold Italic
if not os.path.exists(font_path):
    font_path = 'C:/Windows/Fonts/arialbi.ttf'
if not os.path.exists(font_path):
    font_path = 'C:/Windows/Fonts/arialbd.ttf'

font_size = 20
font = ImageFont.truetype(font_path, font_size)

GLYPH_START_ASCII = 32
GLYPH_END_ASCII = 126
NUM_GLYPHS = GLYPH_END_ASCII - GLYPH_START_ASCII + 1

glyph_metrics = [] # (char, u0, v0, u1, v1, w, h, adv)

curr_x = 4
curr_y = 2
ROW_H = 26

for i in range(NUM_GLYPHS):
    c = chr(GLYPH_START_ASCII + i)
    if c == ' ':
        glyph_metrics.append((c, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 8.0))
        continue

    bbox = draw.textbbox((0, 0), c, font=font)
    bw = bbox[2] - bbox[0]
    bh = bbox[3] - bbox[1]
    
    # 2px padding on each side to avoid bleeding during bilinear sampling
    pad_x = 2
    gw = bw + pad_x * 2
    
    if curr_x + gw >= ATLAS_W - 4:
        curr_x = 4
        curr_y += ROW_H

    # Draw glyph
    draw_x = curr_x + pad_x - bbox[0]
    draw_y = curr_y + max(0, (ROW_H - bh) // 2) - bbox[1]
    draw.text((draw_x, draw_y), c, font=font, fill=255)

    u0 = float(curr_x) / float(ATLAS_W)
    v0 = float(curr_y) / float(ATLAS_H)
    u1 = float(curr_x + gw) / float(ATLAS_W)
    v1 = float(curr_y + ROW_H) / float(ATLAS_H)
    adv = float(bw + 2)

    glyph_metrics.append((c, u0, v0, u1, v1, float(gw), float(ROW_H), adv))
    curr_x += gw + 2

# -------------------------------------------------------------------------
# 2. Vector Icons (y: 110..255)
# 16 icons, 64x64 each, 2 rows of 8 icons
# Row 0: y = 120..184
# Row 1: y = 188..252
# -------------------------------------------------------------------------
ICON_SIZE = 64

def get_icon_rect(idx):
    col = idx % 8
    row = idx // 8
    x = col * ICON_SIZE
    y = 120 + row * ICON_SIZE
    return x, y, x + ICON_SIZE, y + ICON_SIZE

icon_uvs = []

def draw_icon_supersampled(idx, draw_func):
    x0, y0, x1, y1 = get_icon_rect(idx)
    ss = 4
    high = Image.new('L', (ICON_SIZE * ss, ICON_SIZE * ss), 0)
    hdraw = ImageDraw.Draw(high)
    draw_func(hdraw, ICON_SIZE * ss)
    icon_img = high.resize((ICON_SIZE, ICON_SIZE), Image.Resampling.LANCZOS)
    atlas.paste(icon_img, (x0, y0))
    u0 = x0 / float(ATLAS_W)
    v0 = y0 / float(ATLAS_H)
    u1 = x1 / float(ATLAS_W)
    v1 = y1 / float(ATLAS_H)
    icon_uvs.append((u0, v0, u1, v1))

# Icon 0: Gas Chevrons (▲▲)
def draw_gas_chevrons(d, s):
    cx = s / 2.0
    w = s * 0.36
    h = s * 0.20
    th = s * 0.08
    y1 = s * 0.32
    d.polygon([(cx - w, y1 + h), (cx, y1), (cx + w, y1 + h),
               (cx + w - th, y1 + h + th*0.5), (cx, y1 + th*1.3), (cx - w + th, y1 + h + th*0.5)], fill=255)
    y2 = s * 0.56
    d.polygon([(cx - w, y2 + h), (cx, y2), (cx + w, y2 + h),
               (cx + w - th, y2 + h + th*0.5), (cx, y2 + th*1.3), (cx - w + th, y2 + h + th*0.5)], fill=255)

draw_icon_supersampled(0, draw_gas_chevrons)

# Icon 1: Brake Chevrons (▼▼)
def draw_brake_chevrons(d, s):
    cx = s / 2.0
    w = s * 0.36
    h = s * 0.20
    th = s * 0.08
    y1 = s * 0.24
    d.polygon([(cx - w, y1), (cx, y1 + h), (cx + w, y1),
               (cx + w - th, y1 - th*0.5), (cx, y1 + h - th*1.3), (cx - w + th, y1 - th*0.5)], fill=255)
    y2 = s * 0.48
    d.polygon([(cx - w, y2), (cx, y2 + h), (cx + w, y2),
               (cx + w - th, y2 - th*0.5), (cx, y2 + h - th*1.3), (cx - w + th, y2 - th*0.5)], fill=255)

draw_icon_supersampled(1, draw_brake_chevrons)

# Icon 2: Steer Left Arrow (◀)
def draw_steer_left(d, s):
    cx = s * 0.48
    cy = s * 0.50
    w = s * 0.26
    h = s * 0.36
    th = s * 0.09
    pts = [
        (cx - w, cy),
        (cx + w, cy - h),
        (cx + w, cy - h + th*1.4),
        (cx - w + th*2.0, cy),
        (cx + w, cy + h - th*1.4),
        (cx + w, cy + h)
    ]
    d.polygon(pts, fill=255)
    cx2 = cx + s * 0.16
    d.polygon([
        (cx2 - w*0.4, cy),
        (cx2 + w*0.4, cy - h*0.5),
        (cx2 + w*0.4, cy - h*0.5 + th*1.2),
        (cx2 - w*0.4 + th*1.4, cy),
        (cx2 + w*0.4, cy + h*0.5 - th*1.2),
        (cx2 + w*0.4, cy + h*0.5)
    ], fill=255)

draw_icon_supersampled(2, draw_steer_left)

# Icon 3: Steer Right Arrow (▶)
def draw_steer_right(d, s):
    cx = s * 0.52
    cy = s * 0.50
    w = s * 0.26
    h = s * 0.36
    th = s * 0.09
    pts = [
        (cx + w, cy),
        (cx - w, cy - h),
        (cx - w, cy - h + th*1.4),
        (cx + w - th*2.0, cy),
        (cx - w, cy + h - th*1.4),
        (cx - w, cy + h)
    ]
    d.polygon(pts, fill=255)
    cx2 = cx - s * 0.16
    d.polygon([
        (cx2 + w*0.4, cy),
        (cx2 - w*0.4, cy - h*0.5),
        (cx2 - w*0.4, cy - h*0.5 + th*1.2),
        (cx2 + w*0.4 - th*1.4, cy),
        (cx2 - w*0.4, cy + h*0.5 - th*1.2),
        (cx2 - w*0.4, cy + h*0.5)
    ], fill=255)

draw_icon_supersampled(3, draw_steer_right)

# Icon 4: Drift / Handbrake (Curved Skid Marks + Caliper)
def draw_drift(d, s):
    cx = s * 0.5
    cy = s * 0.5
    for offset in [-s*0.14, s*0.14]:
        bbox = (cx + offset - s*0.28, cy - s*0.35, cx + offset + s*0.28, cy + s*0.35)
        d.arc(bbox, start=120, end=320, fill=255, width=int(s*0.065))
    d.ellipse((cx - s*0.13, cy - s*0.13, cx + s*0.13, cy + s*0.13), outline=255, width=int(s*0.05))
    d.ellipse((cx - s*0.045, cy - s*0.045, cx + s*0.045, cy + s*0.045), fill=255)

draw_icon_supersampled(4, draw_drift)

# Icon 5: Split/Second Tactical Powerplay 1 (Crosshairs + Corner Brackets + Bold '1')
def draw_powerplay1(d, s):
    cx = s * 0.5
    cy = s * 0.5
    r = s * 0.42
    d.ellipse((cx - r, cy - r, cx + r, cy + r), outline=255, width=int(s*0.05))
    th = int(s * 0.05)
    tick = s * 0.09
    d.line([(cx - r - tick, cy), (cx - r + tick, cy)], fill=255, width=th)
    d.line([(cx + r - tick, cy), (cx + r + tick, cy)], fill=255, width=th)
    d.line([(cx, cy - r - tick), (cx, cy - r + tick)], fill=255, width=th)
    d.line([(cx, cy + r - tick), (cx, cy + r + tick)], fill=255, width=th)
    f = ImageFont.truetype(font_path, int(s * 0.50))
    bb = d.textbbox((0,0), "1", font=f)
    d.text((cx - (bb[2]-bb[0])//2 - bb[0], cy - (bb[3]-bb[1])//2 - bb[1]), "1", font=f, fill=255)

draw_icon_supersampled(5, draw_powerplay1)

# Icon 6: Split/Second Cataclysm Powerplay 2 (Detonation Spikes + Dual Rings + Bold '2')
def draw_powerplay2(d, s):
    cx = s * 0.5
    cy = s * 0.5
    r1 = s * 0.43
    r2 = s * 0.33
    th = int(s * 0.04)
    d.ellipse((cx - r1, cy - r1, cx + r1, cy + r1), outline=255, width=th)
    d.ellipse((cx - r2, cy - r2, cx + r2, cy + r2), outline=255, width=int(s*0.03))
    for angle_deg in range(0, 360, 45):
        rad = math.radians(angle_deg)
        x_in = cx + math.cos(rad) * r2
        y_in = cy + math.sin(rad) * r2
        x_out = cx + math.cos(rad) * (r1 + s*0.05)
        y_out = cy + math.sin(rad) * (r1 + s*0.05)
        d.line([(x_in, y_in), (x_out, y_out)], fill=255, width=int(s*0.05))
    f = ImageFont.truetype(font_path, int(s * 0.48))
    bb = d.textbbox((0,0), "2", font=f)
    d.text((cx - (bb[2]-bb[0])//2 - bb[0], cy - (bb[3]-bb[1])//2 - bb[1]), "2", font=f, fill=255)

draw_icon_supersampled(6, draw_powerplay2)

# Icon 7: Camera (Camera Body + Lens Circle + Flash)
def draw_camera(d, s):
    cx = s * 0.5
    cy = s * 0.52
    w = s * 0.36
    h = s * 0.24
    r = s * 0.06
    d.rounded_rectangle((cx - w, cy - h, cx + w, cy + h), radius=int(r), outline=255, width=int(s*0.05))
    d.rounded_rectangle((cx - w*0.4, cy - h - s*0.07, cx + w*0.1, cy - h + 2), radius=int(s*0.03), fill=255)
    lens_r = s * 0.12
    d.ellipse((cx - lens_r, cy - lens_r, cx + lens_r, cy + lens_r), outline=255, width=int(s*0.045))
    d.ellipse((cx - s*0.04, cy - s*0.04, cx + s*0.04, cy + s*0.04), fill=255)

draw_icon_supersampled(7, draw_camera)

# Icon 8: Reset (Circular Rewind Arrow ⟲)
def draw_reset(d, s):
    cx = s * 0.50
    cy = s * 0.50
    r = s * 0.28
    th = int(s * 0.07)
    bbox = (cx - r, cy - r, cx + r, cy + r)
    d.arc(bbox, start=50, end=310, fill=255, width=th)
    rad = math.radians(50)
    ax = cx + math.cos(rad) * r
    ay = cy + math.sin(rad) * r
    ah = s * 0.16
    d.polygon([(ax - ah*0.6, ay - ah*0.4), (ax + ah*0.8, ay), (ax, ay + ah*0.8)], fill=255)

draw_icon_supersampled(8, draw_reset)

# Icon 9: Settings / Edit (6-Tooth Gear ⚙)
def draw_gear(d, s):
    cx = s * 0.50
    cy = s * 0.50
    r_outer = s * 0.36
    r_inner = s * 0.25
    r_hole = s * 0.12
    pts = []
    num_teeth = 6
    for i in range(num_teeth * 2):
        angle = (i * math.pi) / num_teeth
        r = r_outer if (i % 2 == 0) else r_inner
        pts.append((cx + math.cos(angle - 0.15) * r, cy + math.sin(angle - 0.15) * r))
        pts.append((cx + math.cos(angle + 0.15) * r, cy + math.sin(angle + 0.15) * r))
    d.polygon(pts, fill=255)
    d.ellipse((cx - r_hole, cy - r_hole, cx + r_hole, cy + r_hole), fill=0)

draw_icon_supersampled(9, draw_gear)

# Icon 10: Hamburger Handle (≡ - 3 Rounded Bars)
def draw_hamburger(d, s):
    cx = s * 0.50
    cy = s * 0.50
    w = s * 0.32
    h = s * 0.05
    spacing = s * 0.14
    for dy in [-spacing, 0, spacing]:
        d.rounded_rectangle((cx - w, cy + dy - h, cx + w, cy + dy + h), radius=int(h), fill=255)

draw_icon_supersampled(10, draw_hamburger)

# Icon 11: Checkmark (✔ - Save / Done)
def draw_checkmark(d, s):
    th = int(s * 0.08)
    p0 = (s * 0.22, s * 0.52)
    p1 = (s * 0.42, s * 0.72)
    p2 = (s * 0.78, s * 0.30)
    d.line([p0, p1, p2], fill=255, width=th, joint="curve")

draw_icon_supersampled(11, draw_checkmark)

# Icon 12: Plus Sign (+)
def draw_plus(d, s):
    cx = s * 0.50
    cy = s * 0.50
    L = s * 0.26
    th = int(s * 0.07)
    d.line([(cx - L, cy), (cx + L, cy)], fill=255, width=th)
    d.line([(cx, cy - L), (cx, cy + L)], fill=255, width=th)

draw_icon_supersampled(12, draw_plus)

# Icon 13: Minus Sign (−)
def draw_minus(d, s):
    cx = s * 0.50
    cy = s * 0.50
    L = s * 0.26
    th = int(s * 0.07)
    d.line([(cx - L, cy), (cx + L, cy)], fill=255, width=th)

draw_icon_supersampled(13, draw_minus)

# Icon 14: Opacity Dial
def draw_opacity(d, s):
    cx = s * 0.50
    cy = s * 0.50
    r = s * 0.30
    d.ellipse((cx - r, cy - r, cx + r, cy + r), outline=255, width=int(s*0.05))
    d.pieslice((cx - r, cy - r, cx + r, cy + r), start=270, end=90, fill=255)

draw_icon_supersampled(14, draw_opacity)

# Icon 15: Cross / Cancel (X)
def draw_cancel(d, s):
    cx = s * 0.50
    cy = s * 0.50
    L = s * 0.22
    th = int(s * 0.07)
    d.line([(cx - L, cy - L), (cx + L, cy + L)], fill=255, width=th)
    d.line([(cx - L, cy + L), (cx + L, cy - L)], fill=255, width=th)

draw_icon_supersampled(15, draw_cancel)

os.makedirs("tools", exist_ok=True)
preview_path = "tools/hud_atlas_preview.png"
atlas.save(preview_path)
print(f"[OK] Generated {preview_path} ({ATLAS_W}x{ATLAS_H})")

# -------------------------------------------------------------------------
# 4. Generate C++ Header (include/touch_hud_atlas.h)
# -------------------------------------------------------------------------
header_path = "include/touch_hud_atlas.h"
pixels = list(atlas.convert('L').tobytes())

rle_data = []
curr_val = pixels[0]
curr_count = 1
for px in pixels[1:]:
    if px == curr_val and curr_count < 255:
        curr_count += 1
    else:
        rle_data.append((curr_count, curr_val))
        curr_val = px
        curr_count = 1
rle_data.append((curr_count, curr_val))

print(f"Raw pixels: {len(pixels)} bytes, RLE pairs: {len(rle_data)} ({len(rle_data)*2} bytes, {len(rle_data)*200/len(pixels):.1f}% of raw)")

with open(header_path, "w") as f:
    f.write("#pragma once\n")
    f.write("// Auto-generated by tools/generate_hud_atlas.py. DO NOT EDIT DIRECTLY.\n\n")
    f.write("#include <cstdint>\n")
    f.write("#include <cstddef>\n\n")
    f.write("namespace SSV {\n\n")
    f.write(f"constexpr int HUD_ATLAS_WIDTH = {ATLAS_W};\n")
    f.write(f"constexpr int HUD_ATLAS_HEIGHT = {ATLAS_H};\n")
    f.write(f"constexpr int HUD_GLYPH_START = {GLYPH_START_ASCII};\n")
    f.write(f"constexpr int HUD_GLYPH_COUNT = {NUM_GLYPHS};\n\n")
    
    f.write("enum class HudIconId {\n")
    f.write("    GasChevrons = 0,\n")
    f.write("    BrakeChevrons = 1,\n")
    f.write("    SteerLeft = 2,\n")
    f.write("    SteerRight = 3,\n")
    f.write("    Drift = 4,\n")
    f.write("    TacticalP1 = 5,\n")
    f.write("    CataclysmP2 = 6,\n")
    f.write("    Camera = 7,\n")
    f.write("    Reset = 8,\n")
    f.write("    Settings = 9,\n")
    f.write("    Hamburger = 10,\n")
    f.write("    Checkmark = 11,\n")
    f.write("    Plus = 12,\n")
    f.write("    Minus = 13,\n")
    f.write("    Opacity = 14,\n")
    f.write("    Cancel = 15,\n")
    f.write("    Count\n")
    f.write("};\n\n")

    f.write("struct AtlasRect {\n")
    f.write("    float u0, v0, u1, v1;\n")
    f.write("    float width, height, advance;\n")
    f.write("};\n\n")

    f.write(f"// ASCII 32 .. 126 glyph metrics\n")
    f.write(f"inline const AtlasRect g_GlyphMetrics[{NUM_GLYPHS}] = {{\n")
    for c, u0, v0, u1, v1, w, h, adv in glyph_metrics:
        esc_c = c.replace('\\', '\\\\').replace('\'', '\\\'')
        f.write(f"    {{ {u0:.6f}f, {v0:.6f}f, {u1:.6f}f, {v1:.6f}f, {w:.1f}f, {h:.1f}f, {adv:.1f}f }}, // '{esc_c}'\n")
    f.write("};\n\n")

    f.write(f"// 16 Vector Icons UVs\n")
    f.write("inline const AtlasRect g_IconUVs[16] = {\n")
    for i, (u0, v0, u1, v1) in enumerate(icon_uvs):
        f.write(f"    {{ {u0:.6f}f, {v0:.6f}f, {u1:.6f}f, {v1:.6f}f, 64.0f, 64.0f, 64.0f }},\n")
    f.write("};\n\n")

    f.write(f"constexpr size_t HUD_ATLAS_RLE_COUNT = {len(rle_data)};\n")
    f.write("inline const uint8_t g_HudAtlasRLE[] = {\n")
    for i in range(0, len(rle_data), 16):
        chunk = rle_data[i:i+16]
        hex_str = ", ".join(f"0x{cnt:02X}, 0x{val:02X}" for cnt, val in chunk)
        f.write(f"    {hex_str},\n")
    f.write("};\n\n")

    f.write("inline void DecompressHudAtlas(uint8_t* outPixels) {\n")
    f.write("    size_t outIdx = 0;\n")
    f.write(f"    for (size_t i = 0; i < sizeof(g_HudAtlasRLE); i += 2) {{\n")
    f.write("        uint8_t count = g_HudAtlasRLE[i];\n")
    f.write("        uint8_t val = g_HudAtlasRLE[i + 1];\n")
    f.write("        for (uint8_t k = 0; k < count; ++k) {\n")
    f.write("            outPixels[outIdx++] = val;\n")
    f.write("        }\n")
    f.write("    }\n")
    f.write("}\n\n")
    f.write("} // namespace SSV\n")

print(f"[OK] Generated {header_path}")
