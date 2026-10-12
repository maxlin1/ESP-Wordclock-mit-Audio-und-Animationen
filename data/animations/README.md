# Animationen

Die Dateien `ani_*.json` in diesem Ordner stammen aus dem Projekt
**Wortuhr_mp3** von Manfred Hofmann (früher
`github.com/manfred-hofmann/Wortuhr_ESP8266`) und stehen unter der
**GNU General Public License v3**.

Beim Bauen der Firmware wandelt `tools/convert_animations.py` alle Dateien in
`include/Extras/AnimationData.gen.h` um. Die Animationen werden damit fest in
die Firmware eingebaut, ein Dateisystem auf der Uhr ist nicht nötig.

Eigene Animationen: eine Datei `ani_NAME.json` im selben Format hier ablegen
und die Firmware neu bauen. Format (10 Zeilen × 11 Spalten, Farben als
`#RRGGBB`):

```json
{
 "Name": "HERZ",
 "Loops": 6,
 "Laufmode": 1,
 "Frame_0": {
  "Delay": 200,
  "Zeile_0": ["#000000", "#FF0000", "..."],
  "...": "...",
  "Zeile_9": ["..."]
 }
}
```

- `Loops`: wie oft die Animation abgespielt wird
- `Laufmode`: 0 = vorwärts, 1 = vorwärts und rückwärts, 2 = zufällige Bilder
- `Delay`: Anzeigedauer des Bildes in Millisekunden; ein Bild mit `Delay` 0
  beendet die Animation
- Animationen mit dem Namen `Zhhmm` (z. B. `Z0800`) laufen automatisch zu
  dieser Uhrzeit, wenn „Uhrzeit-Animationen“ eingeschaltet ist.
