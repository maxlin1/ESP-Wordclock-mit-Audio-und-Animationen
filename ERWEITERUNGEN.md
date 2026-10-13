# Wortuhr mit Sound, Animationen und Home Assistant

Dieser Fork von [ESPWortuhr](https://github.com/ESPWortuhr/Multilayout-ESP-Wordclock)
ergänzt die Funktionen, die man von der Wortuhr von Manfred Hofmann
(*Wortuhr_mp3*) kennt:

- **Sound** mit einem DFPlayer Mini: Stundengong (pro Wochentag wählbar),
  Zeit-, Datums- und Raumklima-Ansage mit den Sprechern Vicki und Hans,
  Lautstärke nach Tageszeit, Nachtruhe
- **die 58 Animationen** von Manfred (Herz, Feuerwerk, Kerze, Pacman …),
  Uhrzeit-Animationen (`Z0800` usw.)
- **Ereignisse** wie Geburtstage, Weihnachten und Silvester mit Lauftext,
  Sound und Animationen – die `events.json` der alten Uhr kann importiert
  werden
- **BME280/BMP280** für Temperatur, Luftfeuchte und Luftdruck
- **Home Assistant** per MQTT mit automatischer Erkennung (Discovery)

Die Erweiterungen gibt es nur für den **ESP32**. Auf dem ESP8266 verhält
sich die Firmware wie das Original-Projekt.

Die SD-Karte des DFPlayers bleibt unverändert: Die Firmware benutzt dieselbe
Nummerierung wie Manfreds Sound-Paket (Ordner `mp3`, `0001_….mp3` usw.).

---

## 1. Hardware: ESP32 D1 mini statt WeMos D1 mini

Ein **ESP32 D1 mini** hat dieselbe Steckerbelegung wie der WeMos D1 mini (die
jeweils innere Stiftreihe) und passt auf die vorhandene Platine. Die Firmware
ist dafür voreingestellt:

| Funktion | WeMos D1 mini (alt) | ESP32 D1 mini (neu) |
|---|---|---|
| LED-Daten | D8 | GPIO 5 |
| DFPlayer TX → ESP | D3 | GPIO 17 |
| ESP → DFPlayer RX | D5 | GPIO 18 |
| DFPlayer BUSY | D0 | GPIO 26 |
| BME280 SDA | D2 | GPIO 21 |
| BME280 SCL | D1 | GPIO 22 |

Die LED-Kette ist so eingestellt wie bei der alten Uhr: 4 Minuten-LEDs am
**Anfang** der Kette, danach die Buchstaben in Schlangenlinien, oben links
beginnend (`MINUTE_LEDS_FIRST` in `include/ExtrasConfig.h`).

Der alte WeMos D1 mini mit der bisherigen Firmware bleibt als Reserve übrig.

## 2. Firmware zum ersten Mal aufspielen (per USB, im Browser)

1. Unter **Releases** die Datei **`wortuhr-esp32-komplett.bin`** herunterladen.
2. In Chrome oder Edge **https://web.esphome.io** öffnen.
3. ESP32 per USB anschließen → **Connect** → Port auswählen.
4. **Install** → die heruntergeladene Datei auswählen → installieren.
   (Wird nach einer Adresse gefragt: `0x0`.)

Alternativ mit esptool:

```
esptool.py --chip esp32 write_flash 0x0 wortuhr-esp32-komplett.bin
```

## 3. Einrichten

1. Nach dem Start öffnet die Uhr ein WLAN **„Connect_to_Wordclock“**. Damit
   verbinden und das eigene WLAN eintragen.
2. Die Webseite der Uhr öffnen (`http://<IP-der-Uhr>`).
3. Prüfen: **Layout** „🇩🇪 10 × 11 Uhr“, **LED-Typ** „WS2812 GRB“.
4. **Smart Home → MQTT**: Broker (z. B. die IP von Home Assistant), Benutzer,
   Passwort eintragen, MQTT aktivieren, „Sende HA Discovery“ anklicken.
5. Die Erweiterungen sind in die normale Webseite der Uhr eingebaut, die
   Einstellungen werden sofort gespeichert:
   - **Funktionen → Übergang → „Zur vollen Stunde“**: Stundengong (Klang,
     auch pro Wochentag), Animation zur vollen Stunde
   - **Funktionen → Animationen & Ansagen**: Animationen und Töne abspielen,
     Uhrzeit, Datum und Raumklima ansagen
   - **Ereignisse**: Geburtstage, Feiertage usw. mit Uhrzeit, Wiederholung,
     Text, Ton und Animationen; „events.json importieren“ übernimmt die
     Ereignisse der alten Uhr
   - **Farben → Automatischer Farbwechsel**: neue Farbe alle 5 Minuten,
     jede Stunde oder jeden Tag (sanft überblendet) oder fließend sehr
     langsam durch alle Farben
   - **Einstellungen → Sound**: Lautstärke, Nachtruhe, Sprecher, Anschlüsse
     des MP3-Players
   - **Einstellungen → Raumklima**: Messwerte, Höhe über dem Meer (für den
     Luftdruck), Temperatur-Korrektur

## 4. Updates

Neue Versionen erscheinen unter **Releases** als
**`wortuhr-esp32-update.bin`**. Einspielen über WLAN: auf der Webseite der Uhr
unter **Einstellungen → Firmware-Update** die Datei auswählen und
**„Update starten“** klicken. Die Seite zeigt den Fortschritt und lädt sich
nach dem Neustart der Uhr selbst neu. (Notfalls geht es weiterhin über die
einfache Seite `http://<IP-der-Uhr>:81/update`.)

Neuerungen vom Original-Projekt holt man auf GitHub mit **„Sync fork“** in
diesen Fork; danach wird die Firmware automatisch neu gebaut.

## 5. Home Assistant

Nach dem Verbinden erscheint die Uhr als Gerät mit zusätzlichen Entitäten:

| Entität | Typ | Funktion |
|---|---|---|
| Wortuhr | Licht | an/aus, Helligkeit, Farbe, Modus (Wortuhr, Sekunden, Lauftext …) |
| Helligkeit | Zahl | 1–100 % (wirkt nicht, solange „Helligkeit automatisch“ an ist) |
| Sound | Schalter | alle Töne an/aus |
| Lautstärke | Zahl | 0–100 % |
| Stundengong | Schalter | Gong zur vollen Stunde |
| Gong-Klang | Auswahl | Klang für alle Wochentage |
| Sprecher | Auswahl | Vicki / Hans |
| Nachtruhe | Schalter | keine Töne in der eingestellten Zeit |
| Animation zur vollen Stunde | Auswahl | Aus, Uhrzeit-Animationen (`Zhhmm`), Zufall oder eine bestimmte Animation |
| Ereignisse | Schalter | Geburtstage usw. |
| Animation abspielen | Auswahl | spielt die gewählte Animation |
| Sound abspielen (Nr.) | Zahl | spielt die mp3 mit dieser Nummer |
| Uhrzeit / Datum / Raumklima ansagen | Taste | Ansage |
| Sound/Animation stoppen | Taste | stoppt alles |
| Temperatur, Luftfeuchtigkeit, Luftdruck | Sensoren | vom BME280 |

Eigene Nachricht mit Sound und Animationen (z. B. in einer Automation):

```yaml
action: mqtt.publish
data:
  topic: ESPWordclock/extras/event/set   # "ESPWordclock" = MQTT-Topic der Uhr
  payload: >
    {"text": "Essen ist fertig!", "sound": 701, "pre": "HERZ", "post": "HERZ"}
```

Weitere MQTT-Befehle (Topic jeweils `<Topic der Uhr>/extras/<Befehl>/set`):

| Befehl | Payload |
|---|---|
| `play` | Nummer der mp3, z. B. `720`; zufällig aus einer Kategorie: `3001` Geburtstag, `3002` Weihnachten, `3003` Silvester, `3004` Natur, `3005` Tiere, `3006` Musik |
| `animation` | Name, z. B. `FEUERWERK` |
| `announce` | `time`, `date` oder `climate` |
| `stop` | beliebig |
| `volume` | `0`–`100` |
| `color_auto` | `Aus`, `Alle 5 Minuten`, `Stündlich`, `Täglich`, `Fließend` |
| `hour_animation` | `Aus`, `Uhrzeit-Animationen`, `Zufall` oder Name |
| `sound`, `chime`, `quiet`, `events` | `ON` / `OFF` |

Für Skripte gibt es außerdem den Aufruf der alten Uhr:
`http://<IP-der-Uhr>:81/setEvent?text=Hallo&audio=701&preani=HERZ&postani=HERZ`

## 6. Zurück zur alten Uhr

Das Flash-Abbild der alten Uhr (`wortuhr_backup.bin`) lässt sich jederzeit
zurückspielen – auf den alten WeMos D1 mini:

```
esptool.py --port <PORT> write_flash 0x0 wortuhr_backup.bin
```

## Lizenz

Der Code des Original-Projekts steht unter der BSD-3-Clause-Lizenz. Die
Animationen in `data/animations` stammen aus Manfred Hofmanns Wortuhr_mp3 und
stehen unter der GPLv3 – damit gilt für die gebaute Firmware dieses Forks die
GPLv3.
