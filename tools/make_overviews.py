#!/usr/bin/env python3
"""
Writes docs/LAYOUTS.md (letter grids of all front layouts) and
docs/SOUNDS.md (number -> sound of the Wortuhr_mp3 sound package).

    python3 tools/make_overviews.py [mp3-ordner-oder-zip]

The layouts are read from the "Layout Front" comments in
include/WordClockTypes/*.hpp, the names from webpage/language/de.js.
The sound list needs the mp3 files (folder or zip) only for the names; the
files themselves are not part of the repository.
"""

import json
import os
import re
import sys
import zipfile

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
DOCS = os.path.join(ROOT, "docs")


# --------------------------------------------------------------------------
# Layouts
# --------------------------------------------------------------------------

def layout_names():
    """key ("de-10-11-clock") -> UI name ("🇩🇪 10 × 11 Uhr")."""
    text = open(os.path.join(ROOT, "webpage/language/de.js"), encoding="utf-8").read()
    return dict(re.findall(r'"([a-z]{2}-[0-9a-z-]+)":\s*"([^"]+)"', text))


def layout_keys():
    """lower-case variable name ("de10x11clock") -> (enum value, key)."""
    text = open(os.path.join(ROOT, "include/WordClockState.h"), encoding="utf-8").read()
    out = {}
    for name, num, var, key in re.findall(r'X\((\w+),\s*(\d+),\s*(\w+),\s*"([^"]+)"\)', text):
        out[var.lstrip("_").lower()] = (int(num), key, name)
    return out


def read_grid(path):
    text = open(path, encoding="utf-8").read()
    m = re.search(r"Layout Front(.*?)\*/", text, re.S)
    if not m:
        return None
    rows = []
    for line in m.group(1).splitlines():
        line = line.lstrip(" *")
        m2 = re.match(r"^\s*([0-9X])\s*\|\s?(.*)$", line)
        if m2:
            rows.append(m2.group(2).rstrip())
        elif re.match(r"^\s*X\s*\|", line):
            rows.append(line.split("|", 1)[1].rstrip())
    return rows


# Short notes for the German layouts
NOTES = {
    "de-10-11-uhr-rechts": "Wie „10 × 11“, aber **UHR ganz rechts unten** "
                           "(…F M U H R statt …F U H R Y). Voreingestellt "
                           "in diesem Fork.",
    "de-10-11-clock": "Das klassische QlockTwo-Layout: VOR und NACH getrennt "
                      "in Zeile 4, Stunden der Reihe nach (EINS, ZWEI, DREI …), "
                      "UHR unten rechts.",
    "de-10-11": "Standard-Layout von ESPWortuhr: NACH/VOR in Zeile 4, "
                "Stunden durcheinander (ZWÖLF in Zeile 5), UHR in der "
                "letzten Zeile in der Mitte.",
    "de-10-11-alt": "Wie Standard, mit HAPPY BIRTHDAY in Zeile 4/5.",
    "de-10-11-alt-frame": "Wie „Alternativ“, zusätzlich ein LED-Rahmen für "
                          "Sekunden.",
    "de-10-11-nero": "Variante mit anderen Füllbuchstaben, UHR ganz unten rechts.",
    "de-10-11-nero-frame": "Wie „Nero“, zusätzlich ein LED-Rahmen für Sekunden.",
    "de-10-11-mrrioes": "VIERTEL und ZWANZIG in eigenen Zeilen, Stunden der "
                        "Reihe nach, kein DREIVIERTEL.",
    "de-10-11-bayerisch": "Bayerisch: „ETZ IST“, „HOIB“, „ZWOA“ …",
    "de-10-11-schwaebisch": "Schwäbisch: „ES ISCH“, „HALB“, „OIS“ …",
    "de-11-11": "Standard 10 × 11 plus eine Zeile mit FUNK/UHR "
                "(Funkuhr-Anzeige).",
    "de-11-11-v2": "Stunden der Reihe nach, zusätzliche Zeile mit FUNK/UHR.",
    "de-11-11-v3": "Standard plus Zeile mit Sonderzeichen (panbachi).",
    "de-11-11-schwaebisch": "Schwäbisch, 11 Zeilen.",
    "de-13-13": "Minutengenaue Anzeige (z. B. „DREI UHR SIEBENUNDZWANZIG“).",
    "de-16-18": "Große Front mit Minuten, Tageszeit (MORGENS, ABENDS …).",
    "de-16-8": "Breite Front mit Minuten in Ziffern.",
    "de-22-11-weather": "10 × 11 plus Wetterfeld (REGEN, WOLKEN, SCHNEE …).",
    "de-08-08": "Kleine Uhr 8 × 8.",
    "de-08-08-viertel": "Kleine Uhr 8 × 8, nur Viertelstunden.",
}


def make_layouts():
    names = layout_names()
    keys = layout_keys()
    entries = []
    for fn in sorted(os.listdir(os.path.join(ROOT, "include/WordClockTypes"))):
        if not fn.endswith(".hpp") or fn == "ClockType.hpp":
            continue
        path = os.path.join(ROOT, "include/WordClockTypes", fn)
        src = open(path, encoding="utf-8").read()
        m = re.search(r"class (\w+)_t\b", src)
        if not m:
            continue
        info = keys.get(m.group(1).lower())
        if not info:
            continue
        grid = read_grid(path)
        if not grid:
            continue
        num, key, enum = info
        entries.append((key, names.get(key, key), grid, fn))

    def order(e):
        key = e[0]
        if key == "de-10-11-uhr-rechts":
            return (0, "")
        return (1 if key.startswith("de-") else 2, key)

    entries.sort(key=order)

    out = ["# Übersicht der Layouts (Frontplatten)", "",
           "Auswahl auf der Webseite der Uhr unter **Einstellungen → Layout**. "
           "Die Buchstaben entsprechen der Frontplatte von vorne gesehen. "
           "Erzeugt mit `python3 tools/make_overviews.py` aus "
           "`include/WordClockTypes`.", "",
           "Die 4 Minuten-LEDs (Ecken) sind nicht eingezeichnet.", ""]

    out += ["## Deutsch", ""]
    de = [e for e in entries if e[0].startswith("de-")]
    other = [e for e in entries if not e[0].startswith("de-")]

    out += ["| Layout | Besonderheit |", "|---|---|"]
    for key, name, grid, fn in de:
        out.append(f"| [{name}](#{key}) | {NOTES.get(key, '')} |")
    out.append("")

    def block(key, name, grid, fn):
        lines = [f'<a id="{key}"></a>', f"### {name}", ""]
        if key in NOTES:
            lines += [NOTES[key], ""]
        lines += ["```"] + grid + ["```", f"<sub>`{fn}`</sub>", ""]
        return lines

    for e in de:
        out += block(*e)

    out += ["## Andere Sprachen", ""]
    for e in other:
        out += block(*e)

    with open(os.path.join(DOCS, "LAYOUTS.md"), "w", encoding="utf-8") as f:
        f.write("\n".join(out))
    print("docs/LAYOUTS.md:", len(entries), "layouts")


# --------------------------------------------------------------------------
# Sounds
# --------------------------------------------------------------------------

def sound_files(source):
    names = []
    if source.endswith(".zip"):
        with zipfile.ZipFile(source) as z:
            names = [os.path.basename(n) for n in z.namelist()]
    else:
        for _, _, files in os.walk(source):
            names += files
    out = {}
    for n in names:
        m = re.match(r"^(\d{4})_(.*)\.mp3$", n, re.I)
        if m:
            out[int(m.group(1))] = m.group(2)
    return out


def named_sounds():
    """Names of the sounds as shown on the clock's web page (extras.js)."""
    text = open(os.path.join(ROOT, "webpage/extras.js"), encoding="utf-8").read()
    return {int(n): s for n, s in re.findall(r"\[(\d+), \"([^\"]+)\"\]", text)}


GONG_SETS = [("Sound 1", 0), ("Sound 2", 20), ("Sound 3", 40), ("Sound 4", 60),
             ("Kuckuck", 80), ("Westminster 1", 100), ("Bing", 120),
             ("Kirche", 140), ("Westminster 2", 160), ("Alte Uhr", 180),
             ("Ding", 200), ("Hund und Katz", 220), ("Tod", 240)]

RANDOM = [(3001, "Geburtstag", "720–726"), (3002, "Weihnachten", "710–719"),
          (3003, "Silvester", "730–733"), (3004, "Natur", "761–769"),
          (3005, "Tiere", "811–839"), (3006, "Musik", "841–869, 871–897")]


def pretty(name):
    s = name.replace("_", " ")
    for a, b in (("aeuss", "äuß"), ("ae", "ä"), ("oe", "ö"), ("ueb", "üb"),
                 ("uend", "ünd"), ("uer", "ür"), ("ruen", "rün"), ("tuer", "tür")):
        s = s.replace(a, b)
    return s[:1].upper() + s[1:]


def make_sounds(source):
    files = sound_files(source)
    named = named_sounds()
    out = ["# Übersicht der Sounds (microSD-Karte des DFPlayers)", "",
           "Die Nummer ist der Anfang des Dateinamens im Ordner `mp3` der "
           "SD-Karte (`0720_happy_birthday.mp3` = Nr. 720). Diese Nummern "
           "verwendet man bei Ereignissen, bei „Sound abspielen (Nr.)“ in "
           "Home Assistant und im MQTT-Befehl `play`.", "",
           "Die mp3-Dateien selbst sind **nicht** im Repository (Urheberrecht, "
           "siehe README).", ""]

    out += ["## Zufällig aus einer Kategorie", "",
            "| Nr. | Kategorie | spielt zufällig eine der Nummern |",
            "|---|---|---|"]
    out += [f"| {n} | {c} | {r} |" for n, c, r in RANDOM]
    out.append("")

    out += ["## Stundengong", "",
            "Pro Klang 13 Dateien: *Basis* + Stunde (1–12 Gongschläge), "
            "*Basis* + 0 um Mitternacht. Bei „nur ein Gongschlag“ wird "
            "*Basis* + 1 gespielt.", "",
            "| Klang | Nummern | Beispiel (3 Uhr) |", "|---|---|---|"]
    for name, base in GONG_SETS:
        out.append(f"| {name} | {base:04d}–{base + 12:04d} | {base + 3:04d} |")
    out += ["| Vicki (Ansage) | 1000–1024 | 1003 |",
            "| Hans (Ansage) | 2000–2024 | 2003 |", ""]

    out += ["## Effekte, Musik und Tiere (700–899)", "", "| Nr. | Sound |", "|---|---|"]
    for n in sorted(k for k in files if 700 <= k < 900):
        out.append(f"| {n} | {named.get(n, pretty(files[n]))} |")
    out.append("")

    out += ["## Sprachansagen", "",
            "Vicki ab **1000**, Hans ab **2000** – gleiche Reihenfolge, Hans "
            "also immer Vicki-Nummer + 1000.", "",
            "| Vicki | Hans | Inhalt |", "|---|---|---|",
            "| 1000–1023 | 2000–2023 | Uhrzeit-Ansage 0–23 Uhr (1024 = 0 Uhr) |"]
    for n in sorted(k for k in files if 1025 <= k < 1100):
        word = re.sub(r"^vicki_", "", files[n])
        out.append(f"| {n} | {n + 1000} | {pretty(word)} |")
    out += ["| 1101–1131 | 2101–2131 | Tag: „ersten“ … „einunddreißigsten“ |",
            "| 1141–1147 | 2141–2147 | Wochentag: Sonntag … Samstag |",
            "| 1151–1162 | 2151–2162 | Monat: Januar … Dezember |"]
    for n in sorted(k for k in files if 1163 <= k < 1200):
        word = re.sub(r"^vicki_", "", files[n])
        out.append(f"| {n} | {n + 1000} | {pretty(word)} |")
    out += ["| 1200–1299 | 2200–2299 | Zahlen 0–99 |",
            "| 1301–1309 | 2301–2309 | 100, 200 … 900 |",
            "| 1311–1319 | 2311–2319 | 1000, 2000 … 9000 |"]
    for n in sorted(k for k in files if 1400 <= k < 1600):
        word = re.sub(r"^vicki_", "", files[n])
        out.append(f"| {n} | {n + 1000} | {pretty(word)} |")
    out.append("")
    out += ["Diese Firmware nutzt davon Uhrzeit, Datum (Tag, Wochentag, Monat, "
            "Jahr), Raumklima (Innentemperatur, Luftfeuchte, plus/minus, "
            "Komma, Grad, Prozent) und die Zahlen. Wetter-, Spiel- und "
            "Mondphasen-Ansagen stammen aus Manfreds Firmware und werden "
            "hier nicht verwendet.", ""]

    with open(os.path.join(DOCS, "SOUNDS.md"), "w", encoding="utf-8") as f:
        f.write("\n".join(out))
    print("docs/SOUNDS.md:", len(files), "files")


if __name__ == "__main__":
    os.makedirs(DOCS, exist_ok=True)
    make_layouts()
    if len(sys.argv) > 1:
        make_sounds(sys.argv[1])
