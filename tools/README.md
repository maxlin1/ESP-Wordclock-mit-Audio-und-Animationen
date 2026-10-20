# Tools

## Bau-Dateien (make_front.py)

`make_front.py` erzeugt aus einem Layout die Dateien zum Bau einer Uhr: Front
(SVG/DXF, Schablonenschrift für Fräse und Laser), Lichtgitter als Platte und
als Steckgitter, Rückwand mit LED-Positionen und für den 3D-Drucker STL/3MF.
Siehe [docs/BAUANLEITUNG.md](../docs/BAUANLEITUNG.md).

```sh
pip install fonttools shapely ezdxf manifold3d trimesh
python3 tools/make_front.py --list
python3 tools/make_front.py de-10-11-nero --size 450 --letter 22 --tool 1.5
```

## ClockType simulator

`clock_type_simulator.py` renders the front matrix definitions from
`include/WordClockTypes/*.hpp`. It parses the layout comment and the
`show(FrontWord)` coordinate mappings, so it is useful for checking ClockType
edits without flashing hardware.

Examples:

```sh
python3 tools/clock_type_simulator.py --list
python3 tools/clock_type_simulator.py DE16x18 --word day_noon --active-letters --show-inactive
python3 tools/clock_type_simulator.py DE16x18 --time 12:00 --active-letters
python3 tools/clock_type_simulator.py DE10x11 --time 22:45 --use-45
python3 tools/clock_type_simulator.py EN10x11 --available-words
```

The simulator warns about words that a layout does not map, matching the
firmware behavior where unknown `FrontWord` values simply do not light pixels.

## ClockType GUI

`clock_type_gui.py` starts a local web GUI around the simulator.

```sh
python3 tools/clock_type_gui.py
```

Then open `http://127.0.0.1:8765/`.
