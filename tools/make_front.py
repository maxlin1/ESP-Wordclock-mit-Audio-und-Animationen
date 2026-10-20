#!/usr/bin/env python3
"""
Bau-Dateien für eine Wortuhr aus einem Layout der Firmware erzeugen.

Erzeugt für Fräse, Laser und 3D-Drucker:

  front.svg / front.dxf          Frontplatte: Außenkontur, Buchstaben,
                                 Löcher für die Minuten-LEDs (+ Befestigung)
  lichtgitter.svg / .dxf         Lichtgitter als Platte (Laser, mehrere Lagen)
  steckgitter.svg / .dxf         Lichtgitter aus Streifen zum Zusammenstecken
                                 (Pappe, Sperrholz – Laser oder Plotter)
  rueckwand.svg / .dxf           Rückwand mit LED-Positionen und Reihenfolge
  3d/front.stl                   Frontplatte zum Drucken (mit Buchstaben-Löchern)
  3d/buchstaben.stl              Buchstaben als Einsätze (weiß, bündig)
  3d/diffusor.stl                dünne Streuscheibe hinter der Front
  3d/lichtgitter.stl             Lichtgitter
  3d/rueckwand.stl               Rückwand mit LED-Positionen und Kabelloch
  3d/wortuhr.3mf                 alles zusammen (Bambu Studio, PrusaSlicer …)
  vorschau.svg                   Vorschau aller Teile
  info.txt                       Maße und Hinweise

Beispiele:

  # Edelstahl-Front 450 x 450 mm für die Fräse, Schablonenschrift
  python3 tools/make_front.py de-10-11-nero --size 450 --pitch 33.33 \\
      --font stencil --out bau/nero-450

  # kleine Uhr 8 x 8 für den 3D-Drucker (LED-Streifen 60 LEDs/m)
  python3 tools/make_front.py de-08-08 --pitch 16.67 --out bau/8x8-3d

  python3 tools/make_front.py --list

Benötigt: pip install fonttools shapely ezdxf manifold3d trimesh
"""

import argparse
import math
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
FONTS = {
    "stencil": os.path.join(ROOT, "tools/fonts/AllertaStencil-Regular.ttf"),
    "normal": os.path.join(ROOT, "tools/fonts/Lato-Bold.ttf"),
}

# ---------------------------------------------------------------------------
# Layouts of the firmware
# ---------------------------------------------------------------------------


def layouts():
    """key -> (name, letter grid, hpp file)."""
    state = open(os.path.join(ROOT, "include/WordClockState.h"), encoding="utf-8").read()
    keys = {var.lstrip("_").lower(): key
            for var, key in re.findall(r'X\(\w+,\s*\d+,\s*(\w+),\s*"([^"]+)"\)', state)}
    lang = open(os.path.join(ROOT, "webpage/language/de.js"), encoding="utf-8").read()
    names = dict(re.findall(r'"([a-z]{2}-[0-9a-z-]+)":\s*"([^"]+)"', lang))
    out = {}
    folder = os.path.join(ROOT, "include/WordClockTypes")
    for fn in sorted(os.listdir(folder)):
        if not fn.endswith(".hpp"):
            continue
        text = open(os.path.join(folder, fn), encoding="utf-8").read()
        m = re.search(r"class (\w+)_t\b", text)
        g = re.search(r"Layout Front(.*?)\*/", text, re.S)
        if not m or not g or m.group(1).lower() not in keys:
            continue
        rows = []
        for line in g.group(1).splitlines():
            line = line.lstrip(" *")
            r = re.match(r"^\s*[0-9X]\s*\|\s?(.*)$", line)
            if r and r.group(1).strip():
                rows.append(r.group(1).split())
        key = keys[m.group(1).lower()]
        name = re.sub("[\U0001F1E6-\U0001F1FF]+ ?", "", names.get(key, key))
        out[key] = (name, rows, fn)
    return out


# ---------------------------------------------------------------------------
# Glyphs -> polygons
# ---------------------------------------------------------------------------


class Glyphs:
    def __init__(self, path, steps=8):
        from fontTools.ttLib import TTFont
        self.font = TTFont(path)
        self.glyphs = self.font.getGlyphSet()
        self.cmap = self.font.getBestCmap()
        self.steps = steps
        self.cap = self._cap_height()
        self.cache = {}

    def _cap_height(self):
        os2 = self.font["OS/2"]
        cap = getattr(os2, "sCapHeight", 0) or 0
        if cap <= 0:
            cap = self.outline("H").bounds[3]
        return cap

    def outline(self, ch):
        """Shapely geometry of a character in font units (y up)."""
        from fontTools.pens.basePen import BasePen
        from shapely.geometry import Polygon
        from shapely.ops import unary_union

        if ch in self.cache:
            return self.cache[ch]
        name = self.cmap.get(ord(ch))
        if name is None:
            raise SystemExit(f"Zeichen '{ch}' fehlt in der Schrift")
        rings = []
        steps = self.steps

        class Pen(BasePen):
            def __init__(self, gs):
                super().__init__(gs)
                self.ring = []

            def _moveTo(self, p):
                self.ring = [p]

            def _lineTo(self, p):
                self.ring.append(p)

            def _curveToOne(self, p1, p2, p3):
                p0 = self.ring[-1]
                for i in range(1, steps + 1):
                    t = i / steps
                    a = (1 - t) ** 3
                    b = 3 * (1 - t) ** 2 * t
                    c = 3 * (1 - t) * t ** 2
                    d = t ** 3
                    self.ring.append((a * p0[0] + b * p1[0] + c * p2[0] + d * p3[0],
                                      a * p0[1] + b * p1[1] + c * p2[1] + d * p3[1]))

            def _qCurveToOne(self, p1, p2):
                p0 = self.ring[-1]
                for i in range(1, steps + 1):
                    t = i / steps
                    self.ring.append(((1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t ** 2 * p2[0],
                                      (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t ** 2 * p2[1]))

            def _closePath(self):
                if len(self.ring) >= 3:
                    rings.append(self.ring)
                self.ring = []

            _endPath = _closePath

        self.glyphs[name].draw(Pen(self.glyphs))
        geom = Polygon()
        for ring in rings:  # even-odd
            p = Polygon(ring).buffer(0)
            geom = geom.symmetric_difference(p)
        geom = unary_union(geom)
        self.cache[ch] = geom
        return geom


def place(geom, cx, cy, scale):
    """Font units (y up) -> mm (y down), centred horizontally on cx and the
    cap height centred vertically on cy."""
    from shapely import affinity
    minx, _, maxx, _ = geom.bounds
    g = affinity.translate(geom, -(minx + maxx) / 2, 0)
    g = affinity.scale(g, scale, -scale, origin=(0, 0))
    return affinity.translate(g, cx, cy)


# ---------------------------------------------------------------------------
# Geometry of the clock
# ---------------------------------------------------------------------------


class Clock:
    def __init__(self, args, grid):
        self.a = args
        self.rows = len(grid)
        self.cols = max(len(r) for r in grid)
        self.grid = grid
        px, py = args.pitch, args.pitch_y or args.pitch
        self.px, self.py = px, py
        mw, mh = self.cols * px, self.rows * py
        if args.size:
            self.w = args.size[0]
            self.h = args.size[1] if len(args.size) > 1 else args.size[0]
        else:
            border = args.border if args.border is not None else max(px, py)
            self.w, self.h = mw + 2 * border, mh + 2 * border
        if self.w < mw or self.h < mh:
            raise SystemExit(f"Front {self.w:g} x {self.h:g} mm ist kleiner als die "
                             f"LED-Matrix {mw:g} x {mh:g} mm")
        self.x0 = (self.w - mw) / 2
        self.y0 = (self.h - mh) / 2
        self.letter_h = args.letter or 0.5 * min(px, py)

    def cell(self, r, c):
        return self.x0 + (c + 0.5) * self.px, self.y0 + (r + 0.5) * self.py

    def minute_leds(self):
        """Corners: top left, top right, bottom right, bottom left (like the
        minute LEDs 1-4)."""
        if self.a.no_minutes:
            return []
        off = self.a.minute_offset
        if off is None:
            off = min(self.x0, self.y0) / 2
        return [(off, off), (self.w - off, off), (self.w - off, self.h - off), (off, self.h - off)]

    def mount_holes(self):
        if not self.a.mount_holes:
            return []
        off = self.a.mount_offset
        pts = [(off, off), (self.w - off, off), (self.w - off, self.h - off), (off, self.h - off)]
        # not on top of a minute LED
        mins = self.minute_leds()
        return [p for p in pts if all(math.dist(p, m) > self.a.minute_hole + self.a.mount_holes for m in mins)]


def letters(clock, glyphs):
    from shapely.ops import unary_union
    scale = clock.letter_h / glyphs.cap
    out = []
    for r, row in enumerate(clock.grid):
        for c, ch in enumerate(row):
            if ch in ("·", "-", "_"):
                continue
            cx, cy = clock.cell(r, c)
            # centre the cap height in the cell (baseline below the centre)
            g = place(glyphs.outline(ch), cx, cy + clock.letter_h / 2, scale)
            out.append(g)
    return unary_union(out)


def circle(x, y, d, n=48):
    from shapely.geometry import Point
    return Point(x, y).buffer(d / 2, n // 4)


def rect(x, y, w, h):
    from shapely.geometry import box
    return box(x, y, x + w, y + h)


# ---------------------------------------------------------------------------
# Writers
# ---------------------------------------------------------------------------


def rings(geom):
    polys = getattr(geom, "geoms", [geom])
    for p in polys:
        if p.is_empty:
            continue
        yield list(p.exterior.coords)
        for i in p.interiors:
            yield list(i.coords)


def svg_path(geom):
    d = []
    for ring in rings(geom):
        d.append("M" + " L".join(f"{x:.3f} {y:.3f}" for x, y in ring[:-1]) + " Z")
    return " ".join(d)


def write_svg(path, w, h, layers, note=""):
    """layers: list of (id, geom, style)."""
    out = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{w:g}mm" height="{h:g}mm" '
           f'viewBox="0 0 {w:g} {h:g}">']
    if note:
        out.append(f"<!-- {note} -->")
    for lid, geom, style in layers:
        if geom is None or geom.is_empty:
            continue
        out.append(f'<g id="{lid}"><path d="{svg_path(geom)}" fill-rule="evenodd" {style}/></g>')
    out.append("</svg>")
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(out))


def write_dxf(path, h, layers):
    import ezdxf
    doc = ezdxf.new("R2010")
    doc.header["$INSUNITS"] = 4  # mm
    msp = doc.modelspace()
    for name, geom, color in layers:
        if geom is None or geom.is_empty:
            continue
        doc.layers.add(name, color=color)
        for ring in rings(geom):
            pts = [(x, h - y) for x, y in ring[:-1]]  # DXF: y up
            msp.add_lwpolyline(pts, close=True, dxfattribs={"layer": name})
    doc.saveas(path)


CUT = 'fill="none" stroke="#e00000" stroke-width="0.2"'
ENGRAVE = 'fill="#000000" stroke="none"'
MARK = 'fill="none" stroke="#0050e0" stroke-width="0.3"'


# ---------------------------------------------------------------------------
# Parts
# ---------------------------------------------------------------------------


def front_parts(clock, glyphs):
    outline = rect(0, 0, clock.w, clock.h)
    text = letters(clock, glyphs)
    mins = None
    from shapely.ops import unary_union
    if clock.minute_leds():
        mins = unary_union([circle(x, y, clock.a.minute_hole) for x, y in clock.minute_leds()])
    mounts = None
    if clock.mount_holes():
        mounts = unary_union([circle(x, y, clock.a.mount_holes) for x, y in clock.mount_holes()])
    return outline, text, mins, mounts


def grid_plate(clock):
    """Plate with one square opening per letter (walls between)."""
    from shapely.ops import unary_union
    wall = clock.a.wall
    holes = [rect(clock.x0 + c * clock.px + wall / 2, clock.y0 + r * clock.py + wall / 2,
                  clock.px - wall, clock.py - wall)
             for r in range(clock.rows) for c in range(clock.cols)]
    for x, y in clock.minute_leds():
        holes.append(circle(x, y, clock.a.minute_hole + 2))
    return rect(0, 0, clock.w, clock.h).difference(unary_union(holes))


def egg_crate(clock):
    """Strips that slot into each other (half-lap joints)."""
    from shapely.ops import unary_union
    t = clock.a.strip_thickness
    hgt = clock.a.grid_height
    gap = 4.0
    parts = []
    y = 0.0
    # horizontal strips: slots from the top, vertical strips: from the bottom
    for kind, count, length, step in (("h", clock.rows + 1, clock.cols * clock.px, clock.px),
                                     ("v", clock.cols + 1, clock.rows * clock.py, clock.py)):
        for _ in range(count):
            strip = rect(0, y, length + t, hgt)
            slots = []
            n = int(round(length / step))
            for i in range(n + 1):
                x = i * step
                if kind == "h":
                    slots.append(rect(x, y, t, hgt / 2))
                else:
                    slots.append(rect(x, y + hgt / 2, t, hgt / 2))
            parts.append(strip.difference(unary_union(slots)))
            y += hgt + gap
    geom = unary_union(parts)
    return geom, clock.cols * max(clock.px, clock.py) + t, y


def back_marks(clock):
    """LED positions (circle + cross) and the wiring order as a line."""
    from shapely.geometry import LineString
    from shapely.ops import unary_union
    marks = []
    order = []
    meander = not clock.a.no_meander
    for r in range(clock.rows):
        cols = range(clock.cols)
        if meander and r % 2 == 1:
            cols = reversed(range(clock.cols))
        for c in cols:
            x, y = clock.cell(r, c)
            marks.append(circle(x, y, 5).exterior.buffer(0.15))
            marks.append(LineString([(x - 3, y), (x + 3, y)]).buffer(0.15))
            marks.append(LineString([(x, y - 3), (x, y + 3)]).buffer(0.15))
            order.append((x, y))
    path = LineString(order).buffer(0.25) if len(order) > 1 else None
    for x, y in clock.minute_leds():
        marks.append(circle(x, y, clock.a.minute_hole).exterior.buffer(0.15))
    # start arrow at the first LED of the matrix
    sx, sy = order[0]
    start = circle(sx, sy, 2.5)
    return unary_union(marks), path, start


# ---------------------------------------------------------------------------
# 3D
# ---------------------------------------------------------------------------


def cross_section(geom):
    from manifold3d import CrossSection, FillRule
    polys = []
    for ring in rings(geom):
        polys.append([(x, -y) for x, y in ring[:-1]])  # y up for 3D
    return CrossSection(polys, FillRule.EvenOdd)


def mesh_of(manifold):
    import numpy as np
    import trimesh
    m = manifold.to_mesh()
    return trimesh.Trimesh(vertices=np.array(m.vert_properties)[:, :3],
                           faces=np.array(m.tri_verts), process=True)


def make_3d(clock, outline, text, mins, outdir):
    from manifold3d import Manifold
    import trimesh

    from shapely import affinity

    # The front is printed with its visible side on the print bed, so the
    # front parts are mirrored: seen from the front the text reads correctly.
    def mirror(g):
        return affinity.scale(g, -1, 1, origin=(clock.w / 2, 0)) if g is not None else None

    outline_m, text_m, mins_m = mirror(outline), mirror(text), mirror(mins)

    t = clock.a.front_thickness
    d = clock.a.diffusor
    holes = text_m if mins_m is None else text_m.union(mins_m)
    front = cross_section(outline_m.difference(holes)).extrude(t)
    inserts = cross_section(text_m.buffer(-clock.a.insert_clearance)).extrude(t)
    diffusor = cross_section(outline_m).extrude(d).translate((0, 0, t))
    grid2d = mirror(grid_plate(clock)).intersection(
        rect(clock.x0 - clock.a.wall, clock.y0 - clock.a.wall,
             clock.cols * clock.px + 2 * clock.a.wall, clock.rows * clock.py + 2 * clock.a.wall))
    grid = cross_section(grid2d).extrude(clock.a.grid_height).translate((0, 0, t + d))

    # back plate: LED positions engraved 0.4 mm, cable hole, two hanging holes
    marks, _, _ = back_marks(clock)
    back2d = outline.difference(circle(clock.w / 2, clock.h - clock.y0 / 2, 8))
    for x in (clock.w * 0.25, clock.w * 0.75):
        back2d = back2d.difference(circle(x, clock.y0 / 2, 4))
    back = cross_section(back2d).extrude(clock.a.back_thickness)
    back = back - cross_section(marks.intersection(back2d)).extrude(0.4).translate(
        (0, 0, clock.a.back_thickness - 0.4))

    os.makedirs(outdir, exist_ok=True)
    meshes = {
        "front": mesh_of(front),
        "buchstaben": mesh_of(inserts),
        "diffusor": mesh_of(diffusor),
        "lichtgitter": mesh_of(grid),
    }
    for name, mesh in meshes.items():
        mesh.export(os.path.join(outdir, f"{name}.stl"))
    mesh_of(back).export(os.path.join(outdir, "rueckwand.stl"))
    scene = trimesh.Scene()
    for name, mesh in meshes.items():
        scene.add_geometry(mesh, node_name=name, geom_name=name)
    try:
        scene.export(os.path.join(outdir, "wortuhr.3mf"))
    except Exception as e:  # 3MF export needs optional packages
        print("Hinweis: 3MF nicht erzeugt:", e)
    return meshes


# ---------------------------------------------------------------------------
# Checks
# ---------------------------------------------------------------------------


def narrow_letters(clock, glyphs, tool):
    """Letters with strokes narrower than the milling tool (the tool cannot
    get into them, so they would be missing or incomplete)."""
    if tool <= 0:
        return []
    scale = clock.letter_h / glyphs.cap
    r = tool / 2 * 0.98
    bad = []
    for ch in sorted({c for row in clock.grid for c in row}):
        if ch in ("·", "-", "_"):
            continue
        g = place(glyphs.outline(ch), 0, 0, scale)
        for part in getattr(g, "geoms", [g]):
            inner = part.buffer(-r)
            if inner.is_empty or len(getattr(inner, "geoms", [inner])) > 1:
                bad.append(ch)
                break
    return bad


def islands(geom):
    """Number of loose inner parts (e.g. the inside of an O) for a front that
    is cut through."""
    return sum(len(p.interiors) for p in getattr(geom, "geoms", [geom]))


# ---------------------------------------------------------------------------


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("layout", nargs="?", help="Layout, z. B. de-10-11-nero (siehe --list)")
    ap.add_argument("--list", action="store_true", help="alle Layouts anzeigen")
    ap.add_argument("--out", help="Ausgabeordner (Standard: bau/<layout>)")
    ap.add_argument("--pitch", type=float, default=33.33,
                    help="LED-Abstand in mm (30 LEDs/m = 33.33, 60 LEDs/m = 16.67)")
    ap.add_argument("--pitch-y", type=float, help="LED-Abstand senkrecht, falls anders")
    ap.add_argument("--size", type=float, nargs="+", help="Außenmaß der Front in mm: B [H]")
    ap.add_argument("--border", type=float, help="Rand um die Matrix in mm (ohne --size)")
    ap.add_argument("--letter", type=float, help="Buchstabenhöhe in mm (Standard: halber LED-Abstand)")
    ap.add_argument("--font", default="stencil", help="stencil, normal oder Pfad zu einer .ttf/.otf")
    ap.add_argument("--minute-hole", type=float, default=5.0, help="Loch für die Minuten-LEDs in mm")
    ap.add_argument("--minute-offset", type=float, help="Abstand der Minuten-LEDs von der Ecke in mm")
    ap.add_argument("--no-minutes", action="store_true", help="keine Minuten-LEDs in den Ecken")
    ap.add_argument("--mount-holes", type=float, default=0, help="Befestigungslöcher (Durchmesser mm, 0 = keine)")
    ap.add_argument("--mount-offset", type=float, default=10, help="Abstand der Befestigungslöcher vom Rand")
    ap.add_argument("--tool", type=float, default=0, help="Fräserdurchmesser in mm (prüft zu enge Stellen)")
    ap.add_argument("--wall", type=float, default=1.2, help="Wandstärke des Lichtgitters in mm")
    ap.add_argument("--grid-height", type=float, default=15, help="Höhe des Lichtgitters in mm")
    ap.add_argument("--strip-thickness", type=float, default=1.0, help="Materialstärke für das Steckgitter in mm")
    ap.add_argument("--front-thickness", type=float, default=1.6, help="3D-Druck: Stärke der Front in mm")
    ap.add_argument("--diffusor", type=float, default=0.6, help="3D-Druck: Stärke der Streuscheibe in mm")
    ap.add_argument("--back-thickness", type=float, default=2.0, help="3D-Druck: Stärke der Rückwand in mm")
    ap.add_argument("--insert-clearance", type=float, default=0.1, help="3D-Druck: Spiel der Buchstaben-Einsätze")
    ap.add_argument("--no-meander", action="store_true", help="LED-Reihen nicht in Schlangenlinie verlegt")
    ap.add_argument("--no-3d", action="store_true", help="keine 3D-Dateien erzeugen")
    a = ap.parse_args()

    all_layouts = layouts()
    if a.list or not a.layout:
        for key, (name, grid, fn) in all_layouts.items():
            print(f"{key:24} {len(grid):2} x {max(len(r) for r in grid):2}  {name}")
        return
    if a.layout not in all_layouts:
        raise SystemExit(f"Layout '{a.layout}' unbekannt, siehe --list")
    name, grid, fn = all_layouts[a.layout]
    out = a.out or os.path.join("bau", a.layout)
    os.makedirs(out, exist_ok=True)

    clock = Clock(a, grid)
    font = FONTS.get(a.font, a.font)
    glyphs = Glyphs(font)
    outline, text, mins, mounts = front_parts(clock, glyphs)

    # --- front
    write_svg(os.path.join(out, "front.svg"), clock.w, clock.h, [
        ("aussenkontur", outline.exterior.buffer(0.01), CUT),
        ("buchstaben", text, ENGRAVE),
        ("minuten-leds", mins, CUT),
        ("befestigung", mounts, CUT),
    ], f"{name} – {clock.w:g} x {clock.h:g} mm")
    write_dxf(os.path.join(out, "front.dxf"), clock.h, [
        ("AUSSENKONTUR", outline, 1),
        ("BUCHSTABEN", text, 7),
        ("MINUTEN_LEDS", mins, 3),
        ("BEFESTIGUNG", mounts, 5),
    ])

    # --- light grid as a plate and as slotted strips
    plate = grid_plate(clock)
    write_svg(os.path.join(out, "lichtgitter.svg"), clock.w, clock.h, [("lichtgitter", plate, CUT)])
    write_dxf(os.path.join(out, "lichtgitter.dxf"), clock.h, [("LICHTGITTER", plate, 1)])
    crate, cw, ch = egg_crate(clock)
    write_svg(os.path.join(out, "steckgitter.svg"), cw, ch, [("steckgitter", crate, CUT)])
    write_dxf(os.path.join(out, "steckgitter.dxf"), ch, [("STECKGITTER", crate, 1)])

    # --- back plate with LED positions
    marks, path, start = back_marks(clock)
    write_svg(os.path.join(out, "rueckwand.svg"), clock.w, clock.h, [
        ("aussenkontur", outline.exterior.buffer(0.01), CUT),
        ("led-positionen", marks, ENGRAVE),
        ("verdrahtung", path, 'fill="#0050e0" fill-opacity="0.35" stroke="none"'),
        ("start", start, 'fill="#e00000"'),
    ])
    write_dxf(os.path.join(out, "rueckwand.dxf"), clock.h, [
        ("AUSSENKONTUR", outline, 1), ("LED_POSITIONEN", marks, 7), ("VERDRAHTUNG", path, 5)])

    # --- preview
    write_svg(os.path.join(out, "vorschau.svg"), clock.w, clock.h, [
        ("platte", outline, 'fill="#2a2d33"'),
        ("buchstaben", text, 'fill="#f4f1e8"'),
        ("minuten-leds", mins, 'fill="#f4f1e8"'),
        ("befestigung", mounts, 'fill="#888"'),
    ])

    # --- 3D
    meshes = None
    if not a.no_3d:
        meshes = make_3d(clock, outline, text, mins, os.path.join(out, "3d"))

    # --- info
    info = [
        f"Layout:            {name} ({a.layout}, {fn})",
        f"Matrix:            {clock.rows} Zeilen x {clock.cols} Spalten = {clock.rows * clock.cols} LEDs"
        + (" + 4 Minuten-LEDs" if clock.minute_leds() else ""),
        f"LED-Abstand:       {clock.px:g} x {clock.py:g} mm",
        f"Front:             {clock.w:g} x {clock.h:g} mm (Matrix {clock.cols * clock.px:g} x {clock.rows * clock.py:g} mm)",
        f"Buchstabenhöhe:    {clock.letter_h:g} mm, Schrift {os.path.basename(font)}",
    ]
    if clock.minute_leds():
        info.append("Minuten-LEDs:      " + ", ".join(f"({x:.1f}|{y:.1f})" for x, y in clock.minute_leds())
                    + f" mm, Loch {a.minute_hole:g} mm")
    n_islands = islands(text)
    info.append(f"Innenteile:        {n_islands} (fallen beim Durchschneiden heraus"
                + (" – für Fräse/Laser die Schablonenschrift nehmen)" if n_islands else ")"))
    if a.tool:
        bad = narrow_letters(clock, glyphs, a.tool)
        info.append(f"Fräser {a.tool:g} mm:" + " " * max(1, 7 - len(f"{a.tool:g}"))
                    + ("passt für alle Buchstaben (Innenecken werden mit "
                       f"Radius {a.tool / 2:g} mm gerundet)" if not bad else
                       f"zu dick für {' '.join(bad)} – kleineren Fräser oder größere Buchstaben"))
    if meshes:
        size = meshes["front"].extents
        bed = "passt auf 256 x 256 mm (z. B. Bambu Lab A1)" if max(size[:2]) <= 256 else \
            "zu groß für 256 x 256 mm – kleineren LED-Abstand oder geteilt drucken"
        info.append(f"3D-Druck:          {size[0]:.0f} x {size[1]:.0f} mm, {bed}")
    with open(os.path.join(out, "info.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(info) + "\n")
    print("\n".join(info))
    print(f"\nDateien in {out}/")


if __name__ == "__main__":
    main()
