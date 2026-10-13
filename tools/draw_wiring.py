#!/usr/bin/env python3
"""
Draws pics/extras/schaltbild-esp32.svg in the style of pics/Schematic.png:
ESP32 D1 mini, DFPlayer Mini, BME280, LED strip and the parts of the
Wortuhr_mp3 board (Manfred Hofmann, Platine_Plan_Wortuhr_mp3.pdf).

    python3 tools/draw_wiring.py
"""

import os

W, H = 1780, 1330
FONT = "'Droid Sans', 'Open Sans', 'Segoe UI', Helvetica, Arial, sans-serif"

out = []
segments = []  # drawn wire segments (net, x1, y1, x2, y2) for crossing hops


def add(s):
    out.append(s)


# --------------------------------------------------------------------------
# basic shapes
# --------------------------------------------------------------------------

WIRE = {
    "5v": ("#c4161c", "#7d0e12"),
    "gnd": ("#2b2b2b", "#000000"),
    "3v3": ("#e8850c", "#8f5207"),
    "led": ("#2fb344", "#1c6b29"),
    "tx": ("#1c7ed6", "#0f4c82"),
    "rx": ("#14b8a6", "#0b6e63"),
    "busy": ("#7048e8", "#3f2a85"),
    "sda": ("#ae3ec9", "#682579"),
    "scl": ("#9c6b1c", "#5c3f10"),
    "spk": ("#868e96", "#495057"),
    "ldr": ("#868e96", "#495057"),
}
HOP = 9


def hop_points(net, a, b):
    """Crossings of segment a-b with earlier segments of other nets."""
    (x1, y1), (x2, y2) = a, b
    hits = []
    for n, sx1, sy1, sx2, sy2 in segments:
        if n == net:
            continue
        if y1 == y2 and sx1 == sx2:  # horizontal over vertical
            lo, hi = sorted((x1, x2))
            slo, shi = sorted((sy1, sy2))
            if lo + HOP < sx1 < hi - HOP and slo < y1 < shi:
                hits.append(sx1)
        elif x1 == x2 and sy1 == sy2:  # vertical over horizontal
            lo, hi = sorted((y1, y2))
            slo, shi = sorted((sx1, sx2))
            if lo + HOP < sy1 < hi - HOP and slo < x1 < shi:
                hits.append(sy1)
    return sorted(set(hits))


def wire(net, pts, dots=()):
    d = f"M{pts[0][0]} {pts[0][1]}"
    for a, b in zip(pts, pts[1:]):
        hits = hop_points(net, a, b)
        (x1, y1), (x2, y2) = a, b
        if y1 == y2:
            step = 1 if x2 > x1 else -1
            if step < 0:
                hits = hits[::-1]
            for hx in hits:
                d += f" L{hx - step * HOP} {y1} A{HOP} {HOP} 0 0 {1 if step > 0 else 0} {hx + step * HOP} {y1}"
        else:
            step = 1 if y2 > y1 else -1
            if step < 0:
                hits = hits[::-1]
            for hy in hits:
                d += f" L{x1} {hy - step * HOP} A{HOP} {HOP} 0 0 {0 if step > 0 else 1} {x1} {hy + step * HOP}"
        d += f" L{x2} {y2}"
    for a, b in zip(pts, pts[1:]):
        segments.append((net, a[0], a[1], b[0], b[1]))
    col, dark = WIRE[net]
    dash = ' stroke-dasharray="14 7"' if net == "ldr" else ""
    add(f'<path d="{d}" fill="none" stroke="{dark}" stroke-width="10" '
        f'stroke-linejoin="round" stroke-linecap="round"/>')
    add(f'<path d="{d}" fill="none" stroke="{col}" stroke-width="6.5" '
        f'stroke-linejoin="round" stroke-linecap="round"{dash}/>')
    for x, y in dots:
        add(f'<circle cx="{x}" cy="{y}" r="7.5" fill="{dark}"/>')


def pin(x, y, used=True):
    if used:
        add(f'<circle cx="{x}" cy="{y}" r="10" fill="#8ad08a" stroke="#3c8d3c" stroke-width="3"/>')
    else:
        add(f'<circle cx="{x}" cy="{y}" r="9" fill="#e06666" stroke="#b03a3a" stroke-width="2.5"/>')


def text(x, y, s, size=14, color="#ffffff", anchor="start", weight="normal",
         rotate=None, family=FONT):
    tr = f' transform="rotate({rotate} {x} {y})"' if rotate is not None else ""
    add(f'<text x="{x}" y="{y}" font-family="{family}" font-size="{size}" '
        f'fill="{color}" text-anchor="{anchor}" font-weight="{weight}"{tr}>{s}</text>')


def note(x, y, w, h, lines, size=30):
    fold = 22
    add(f'<path d="M{x} {y} H{x + w - fold} L{x + w} {y + fold} V{y + h} H{x} Z" fill="#fde8c3"/>')
    add(f'<rect x="{x}" y="{y + h - 22}" width="{w}" height="22" fill="#f8b84e"/>')
    add(f'<path d="M{x + w - fold} {y} V{y + fold} H{x + w} Z" fill="#f8b84e"/>')
    for i, s in enumerate(lines):
        text(x + 24, y + 44 + i * (size + 8), s, size, "#6d6d6d")


def resistor(x, y, bands, label, vertical=False, label_side=1):
    """Through-hole resistor centred on (x, y), length 70."""
    if vertical:
        add(f'<g transform="rotate(90 {x} {y})">')
    add(f'<rect x="{x - 36}" y="{y - 3}" width="72" height="6" fill="#9aa0a6"/>')
    add(f'<path d="M{x - 26} {y - 11} q-6 0 -6 6 v10 q0 6 6 6 h52 q6 0 6 -6 v-10 '
        f'q0 -6 -6 -6 z" fill="#e7c48d" stroke="#a9884f" stroke-width="2"/>')
    for i, c in enumerate(bands):
        bx = x - 20 + i * 10 if i < 3 else x + 18
        add(f'<rect x="{bx}" y="{y - 11}" width="5" height="22" fill="{c}"/>')
    if vertical:
        add("</g>")
        text(x + 20 * label_side, y + 5, label, 17, "#333",
             "start" if label_side > 0 else "end", "bold")
    else:
        text(x, y - 18 if label_side > 0 else y + 32, label, 17, "#333", "middle", "bold")


RED, BLK, BRN, ORN, YEL, VIO, WHT, GLD = ("#d62828", "#222", "#7b4a1e", "#f08c00",
                                          "#f5d000", "#7d3cb5", "#f8f8f8", "#c9a227")

# --------------------------------------------------------------------------
# background grid
# --------------------------------------------------------------------------
add(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">')
add('<title>Schaltbild: ESP32 D1 mini mit DFPlayer Mini, BME280 und LED-Streifen</title>')
add('<defs><pattern id="g" width="20" height="20" patternUnits="userSpaceOnUse">'
    '<path d="M20 0H0V20" fill="none" stroke="#ececec" stroke-width="1"/></pattern>'
    '<pattern id="G" width="100" height="100" patternUnits="userSpaceOnUse">'
    '<rect width="100" height="100" fill="url(#g)"/>'
    '<path d="M100 0H0V100" fill="none" stroke="#dedede" stroke-width="1.2"/></pattern></defs>')
add(f'<rect width="{W}" height="{H}" fill="#fafafa"/><rect width="{W}" height="{H}" fill="url(#G)"/>')

# --------------------------------------------------------------------------
# geometry
# --------------------------------------------------------------------------
# ESP32 D1 mini (landscape). Inner pin rows = D1-mini compatible.
EX0, EX1, EY0, EY1 = 120, 760, 500, 810
TOP_Y, BOT_Y = 575, 735
PX = [210 + 60 * i for i in range(8)]
top_pins = ["3V3", "IO5", "IO23", "IO19", "IO18", "IO26", "IO36", "RST"]
top_d1 = ["3V3", "D8", "D7", "D6", "D5", "D0", "A0", "RST"]
bot_pins = ["5V", "GND", "IO16", "IO17", "IO21", "IO22", "RX", "TX"]
bot_d1 = ["5V", "G", "D4", "D3", "D2", "D1", "RX", "TX"]
P = {name: (x, TOP_Y) for name, x in zip(top_pins, PX)}
P.update({name: (x, BOT_Y) for name, x in zip(bot_pins, PX)})

# DFPlayer Mini, turned so that the VCC/RX/TX/SPK row is on top
DX0, DX1, DY0, DY1 = 800, 1190, 200, 380
D_TOP_Y, D_BOT_Y = 222, 358
DPX = [835 + 44 * i for i in range(8)]
d_top = ["SPK2", "GND", "SPK1", "DAC_L", "DAC_R", "TX", "RX", "VCC"]
d_bot = ["IO1", "GND ", "IO2", "ADK1", "ADK2", "USB+", "USB−", "BUSY"]
DP = {n: (x, D_TOP_Y) for n, x in zip(d_top, DPX)}
DP.update({n: (x, D_BOT_Y) for n, x in zip(d_bot, DPX)})

# BME280 (pins at the bottom)
BX0, BX1, BY0, BY1 = 1330, 1530, 470, 660
BPX = [1365 + 40 * i for i in range(4)]
b_pins = ["VIN", "GND", "SCL", "SDA"]
BP = {n: (x, BY1 - 18) for n, x in zip(b_pins, BPX)}

# LED strip
SX0, SX1, SY0, SY1 = 360, 1470, 1040, 1150
S_GND, S_DIN, S_5V = 1062, 1095, 1128
SPX = 392

GND_Y, V5_Y = 905, 945

# --------------------------------------------------------------------------
# notes (drawn first, below everything)
# --------------------------------------------------------------------------
note(235, 250, 195, 150, ["ESP32", "D1 mini"])
note(1320, 190, 300, 130, ["DFPlayer Mini", "(MP3-Modul)"], 28)
note(1550, 470, 172, 140, ["BME280", "(Sensor)"], 28)
note(360, 1190, 1110, 92, ["LED-Streifen WS2812B · 114 LEDs (4 Minuten-LEDs zuerst)"], 28)
note(1500, 1100, 255, 110, ["Netzteil", "5 V / ≥ 4 A"], 26)

# --------------------------------------------------------------------------
# modules
# --------------------------------------------------------------------------
# ESP32 board
add(f'<rect x="{EX0}" y="{EY0}" width="{EX1 - EX0}" height="{EY1 - EY0}" rx="34" fill="#2d2d2d" stroke="#1a1a1a" stroke-width="3"/>')
for cx, cy in [(EX0 + 34, EY0 + 34), (EX0 + 34, EY1 - 34), (EX1 - 34, EY0 + 34), (EX1 - 34, EY1 - 34)]:
    add(f'<circle cx="{cx}" cy="{cy}" r="17" fill="#fafafa"/>')
# USB connector
add(f'<rect x="{EX0 - 22}" y="618" width="62" height="74" rx="6" fill="#c8c8c8" stroke="#8a8a8a" stroke-width="3"/>')
add(f'<rect x="{EX0 - 12}" y="636" width="34" height="38" rx="3" fill="#9a9a9a"/>')
# buttons
add('<rect x="132" y="560" width="26" height="26" rx="4" fill="#d9d9d9"/><circle cx="145" cy="573" r="8" fill="#444"/>')
# ESP32 module with antenna
add('<rect x="300" y="628" width="350" height="62" rx="4" fill="#cfd3d6" stroke="#8d9398" stroke-width="3"/>')
text(475, 655, "ESP32-WROOM-32", 19, "#555", "middle", "bold")
text(475, 678, "WiFi · Bluetooth", 14, "#666", "middle")
add('<path d="M665 640 h70 v10 h-60 v10 h60 v10 h-60 v10 h60 v10 h-70 z" fill="none" stroke="#c9a227" stroke-width="5"/>')
# outer pin rows (not used)
for i in range(10):
    x = PX[0] + 60 * i - 0
    if x > EX1 - 50:
        break
    add(f'<circle cx="{x}" cy="{TOP_Y - 40}" r="7" fill="#555" stroke="#777" stroke-width="2"/>')
    add(f'<circle cx="{x}" cy="{BOT_Y + 40}" r="7" fill="#555" stroke="#777" stroke-width="2"/>')
USED = {"3V3", "IO5", "IO18", "IO26", "5V", "GND", "IO17", "IO21", "IO22"}
for n, (x, y) in P.items():
    pin(x, y, n in USED)
for n, d1, x in zip(top_pins, top_d1, PX):
    text(x, TOP_Y + 30, n, 15, "#ffffff", "middle", "bold")
    if d1 != n:
        text(x, TOP_Y + 47, f"({d1})", 13, "#9fd49f", "middle")
for n, d1, x in zip(bot_pins, bot_d1, PX):
    text(x, BOT_Y - 36, n, 15, "#ffffff", "middle", "bold")
    if d1 not in (n, "G"):
        text(x, BOT_Y - 19, f"({d1})", 13, "#9fd49f", "middle")

# DFPlayer
add(f'<rect x="{DX0 - 34}" y="246" width="44" height="88" rx="5" fill="#c9ccd0" stroke="#8d9398" stroke-width="3"/>')
text(DX0 - 12, 290, "microSD", 13, "#555", "middle", rotate=-90)
add(f'<rect x="{DX0}" y="{DY0}" width="{DX1 - DX0}" height="{DY1 - DY0}" rx="10" fill="#1f5c99" stroke="#143d66" stroke-width="3"/>')
add('<rect x="925" y="262" width="62" height="56" rx="4" fill="#222"/>')
text(1085, 285, "DFPlayer", 22, "#ffffff", "middle", "bold")
text(1085, 310, "Mini", 18, "#cfe3f7", "middle")
for n, (x, y) in DP.items():
    pin(x, y, n in {"SPK2", "SPK1", "TX", "RX", "VCC", "GND", "BUSY"})
for n, x in zip(d_top, DPX):
    text(x, D_TOP_Y + 26, n, 12, "#ffffff", "middle")
for n, x in zip(d_bot, DPX):
    text(x, D_BOT_Y - 17, n.strip(), 12, "#ffffff", "middle")

# BME280
add(f'<rect x="{BX0}" y="{BY0}" width="{BX1 - BX0}" height="{BY1 - BY0}" rx="10" fill="#6b2fa0" stroke="#4a1f70" stroke-width="3"/>')
add(f'<circle cx="{BX0 + 24}" cy="{BY0 + 24}" r="12" fill="#fafafa"/><circle cx="{BX1 - 24}" cy="{BY0 + 24}" r="12" fill="#fafafa"/>')
add('<rect x="1410" y="510" width="40" height="40" rx="3" fill="#c0c4c8" stroke="#8d9398" stroke-width="2"/>')
add('<circle cx="1442" cy="518" r="3" fill="#555"/>')
text(1430, 580, "BME280", 20, "#ffffff", "middle", "bold")
for n, (x, y) in BP.items():
    pin(x, y, True)
    text(x, y - 20, n, 13, "#ffffff", "middle")

# speaker
add('<rect x="590" y="150" width="34" height="96" rx="4" fill="#cfd3d6" stroke="#6c757d" stroke-width="3"/>')
add('<path d="M590 160 L545 120 V276 L590 236 Z" fill="#adb5bd" stroke="#6c757d" stroke-width="3"/>')
add('<circle cx="560" cy="198" r="10" fill="#868e96"/>')
text(585, 305, "Lautsprecher", 17, "#495057", "middle", "bold")
text(585, 325, "3 W, 4–8 Ω", 15, "#6c757d", "middle")

# LED strip
add(f'<rect x="{SX0}" y="{SY0}" width="{SX1 - SX0}" height="{SY1 - SY0}" fill="#25306b"/>')
for y, lab in [(S_GND, "GND"), (S_DIN, "Din"), (S_5V, "+5V")]:
    pin(SPX, y, True)
    text(SPX + 22, y + 5, lab, 14, "#ffffff")
add(f'<path d="M{SPX + 58} {S_DIN - 9} l12 9 l-12 9 z" fill="#ffffff"/>')
for lx in (560, 860, 1160):
    add(f'<rect x="{lx - 28}" y="{SY0 + 28}" width="56" height="56" rx="4" fill="#eeeeee" stroke="#999" stroke-width="2"/>')
    add(f'<circle cx="{lx}" cy="{SY0 + 56}" r="17" fill="#ffffff" stroke="#bbb" stroke-width="2"/>')
    add(f'<rect x="{lx - 6}" y="{SY0 + 48}" width="12" height="16" fill="#555"/>')
    text(lx - 48, SY0 + 56, "WS2812B", 13, "#ffffff", "middle", rotate=-90)
for lx in (710, 1010, 1310):
    for dy, lab in [(-33, "+5V"), (0, "Din"), (33, "GND")]:
        add(f'<rect x="{lx - 16}" y="{SY0 + 55 + dy - 8}" width="32" height="16" rx="8" fill="#e0b080"/>')
add(f'<path d="M1395 {SY0 - 14} l-26 {SY1 - SY0 + 28} M1415 {SY0 - 14} l-26 {SY1 - SY0 + 28}" stroke="#222" stroke-width="5"/>')
text(SX1 - 12, SY1 - 10, "… 114 LEDs", 14, "#cfd6ff", "end")

# power supply jack
JX0, JY0 = 1560, 1010
add(f'<rect x="{JX0}" y="{JY0}" width="150" height="70" rx="8" fill="#2b2b2b" stroke="#000" stroke-width="3"/>')
add(f'<circle cx="{JX0 + 120}" cy="{JY0 + 40}" r="17" fill="#555" stroke="#888" stroke-width="3"/>')
add(f'<circle cx="{JX0 + 120}" cy="{JY0 + 40}" r="6" fill="#bbb"/>')
text(JX0 + 45, JY0 + 60, "DC 5 V", 14, "#ffffff", "middle", "bold")
pin(JX0 + 30, JY0 + 18); pin(JX0 + 80, JY0 + 18)
text(JX0 + 30, JY0 + 44, "+", 18, "#ffffff", "middle", "bold")
text(JX0 + 80, JY0 + 44, "−", 18, "#ffffff", "middle", "bold")

# --------------------------------------------------------------------------
# wires (order matters for crossing hops: later wires hop over earlier ones)
# --------------------------------------------------------------------------
# rails
wire("gnd", [(150, GND_Y), (1762, GND_Y)])
wire("5v", [(40, V5_Y), (1740, V5_Y)])
# power input: GND direct, +5V through a Schottky diode (reverse polarity)
wire("gnd", [(1640, JY0 + 18), (1640, GND_Y)], dots=[(1640, GND_Y)])
wire("5v", [(1590, JY0 + 18), (1590, V5_Y)], dots=[(1590, V5_Y)])
add(f'<rect x="1579" y="955" width="22" height="42" rx="4" fill="#222"/>')
add(f'<rect x="1579" y="958" width="22" height="7" fill="#cfcfcf"/>')
text(1570, 975, "Diode", 15, "#333", "end", "bold")
text(1570, 993, "Schottky", 13, "#555", "end")

# capacitor 1800 uF between the rails
CX = 1180
add(f'<rect x="{CX - 10}" y="{V5_Y + 18}" width="46" height="66" rx="8" fill="#1d3f8f" stroke="#0e2457" stroke-width="2"/>')
add(f'<rect x="{CX + 20}" y="{V5_Y + 18}" width="12" height="66" fill="#9fb3e0"/>')
wire("5v", [(CX, V5_Y + 18), (CX, V5_Y)], dots=[(CX, V5_Y)])
wire("gnd", [(CX + 26, V5_Y + 18), (CX + 26, GND_Y)], dots=[(CX + 26, GND_Y)])
text(CX - 18, V5_Y + 44, "1800 µF", 15, "#333", "end", "bold")
text(CX - 18, V5_Y + 62, "6,3 V", 13, "#555", "end")

# ESP supply
wire("gnd", [P["GND"], (P["GND"][0], GND_Y)], dots=[(P["GND"][0], GND_Y)])
wire("5v", [P["5V"], (P["5V"][0], V5_Y)], dots=[(P["5V"][0], V5_Y)])

# LED strip
wire("gnd", [(SPX, S_GND), (330, S_GND), (330, GND_Y)], dots=[(330, GND_Y)])
wire("5v", [(SPX, S_5V), (40, S_5V), (40, V5_Y)])

# 3,3 V to the BME280
wire("3v3", [P["3V3"], (P["3V3"][0], 70), (1300, 70), (1300, 690), (BP["VIN"][0], 690), BP["VIN"]])

# LED data IO5 -> Din
wire("led", [P["IO5"], (P["IO5"][0], 470), (65, 470), (65, S_DIN), (SPX, S_DIN)])

# IO18 (D5) -> DFPlayer RX
wire("tx", [P["IO18"], (P["IO18"][0], 108), (DP["RX"][0], 108), DP["RX"]])

# DFPlayer TX -> IO17 (D3)
wire("rx", [P["IO17"], (P["IO17"][0], 860), (1245, 860), (1245, 150), (DP["TX"][0], 150), DP["TX"]])

# BUSY -> 2k2 -> IO26 (D0)
wire("busy", [P["IO26"], (P["IO26"][0], 465), (DP["BUSY"][0], 465), DP["BUSY"]])
resistor(700, 465, [RED, RED, RED, GLD], "2,2 kΩ")

# I2C
wire("scl", [P["IO22"], (P["IO22"][0], 800), (BP["SCL"][0], 800), BP["SCL"]])
wire("sda", [P["IO21"], (P["IO21"][0], 825), (BP["SDA"][0], 825), BP["SDA"]])

# DFPlayer VCC / GND (top row) and BME280 GND
wire("5v", [DP["VCC"], (DP["VCC"][0], 40), (1740, 40), (1740, V5_Y)])
wire("gnd", [DP["GND"], (DP["GND"][0], 22), (1762, 22), (1762, GND_Y)])
wire("gnd", [BP["GND"], (BP["GND"][0], 710), (1280, 710), (1280, GND_Y)], dots=[(1280, GND_Y)])

# speaker with 3,9 Ohm series resistor
wire("spk", [DP["SPK1"], (DP["SPK1"][0], 175), (624, 175)])
resistor(745, 175, [ORN, WHT, GLD, GLD], "3,9 Ω")
wire("spk", [DP["SPK2"], (DP["SPK2"][0], 222 + 0), (DP["SPK2"][0], 222)])
wire("spk", [DP["SPK2"], (DP["SPK2"][0], 235), (624, 235)])

# --------------------------------------------------------------------------
# LDR (optional) as an inset with labelled ends
# --------------------------------------------------------------------------
note(30, 1178, 310, 140, [], 26)
text(52, 1212, "LDR (optional)", 22, "#6d6d6d")
LY = 1258
add(f'<path d="M48 {LY} H290" stroke="#9aa0a6" stroke-width="4"/>')
add(f'<rect x="88" y="{LY - 16}" width="44" height="32" rx="16" fill="#c9302c" stroke="#7a1b18" stroke-width="2"/>')
add(f'<path d="M98 {LY - 8} h8 v16 h8 v-16 h8 v16" fill="none" stroke="#f3d2a0" stroke-width="3"/>')
resistor(225, LY, [YEL, VIO, RED, GLD], "4,7 kΩ")
add(f'<circle cx="165" cy="{LY}" r="6" fill="#555"/><path d="M165 {LY} V{LY + 14}" stroke="#9aa0a6" stroke-width="4"/>')
add(f'<circle cx="48" cy="{LY}" r="5" fill="#555"/><circle cx="290" cy="{LY}" r="5" fill="#555"/>')
text(40, LY + 28, "3V3", 14, "#333", "start", "bold")
text(165, LY + 32, "IO36 (A0)", 14, "#333", "middle", "bold")
text(298, LY + 28, "GND", 14, "#333", "end", "bold")

add(f'</svg>')

path = os.path.join(os.path.dirname(__file__), "..", "pics", "extras", "schaltbild-esp32.svg")
with open(path, "w", encoding="utf-8") as f:
    f.write("\n".join(out))
print("written", os.path.normpath(path))
