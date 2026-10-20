# Übersicht der Layouts (Frontplatten)

Auswahl auf der Webseite der Uhr unter **Anzeigeoptionen → Front**. Die Buchstaben entsprechen der Frontplatte von vorne gesehen. Erzeugt mit `python3 tools/make_overviews.py` aus `include/WordClockTypes`.

Die 4 Minuten-LEDs (Ecken) sind nicht eingezeichnet.

## Welches Layout habe ich?

Entscheidend ist nur, **wo die Wörter stehen**. Die Buchstaben dazwischen (Füllbuchstaben) sind bei jeder Frontplatte anders und spielen keine Rolle. Deshalb zeigt die Tabelle nur die Wort-Buchstaben, Füllbuchstaben sind `·`.

1. Vergleiche die **unterste Zeile** deiner Front mit der Tabelle – vor allem, **wo UHR steht** (ganz rechts oder eine Stelle davor) und wo die Stunden stehen.
2. Passen noch mehrere, vergleiche die **4. Zeile** (VOR/NACH).
3. Kontrolle: Mit dem richtigen Layout leuchten genau die Wörter. Leuchtet z. B. „M U H“ statt „U H R“, ist es das falsche – dann das Layout mit UHR an der anderen Stelle wählen.

Beispiel: Eine Front mit der untersten Zeile `BSECHSFMUHR` hat SECHS links und UHR ganz rechts → Wortmuster `·SECHS··UHR` → **10 × 11 Nero**.

| Layout | 4. Zeile | unterste Zeile | UHR steht |
|---|---|---|---|
| [🇩🇪 10 × 11 Nero](#de-10-11-nero) | `··NACHVOR··` | `·SECHS··UHR` | **ganz rechts** |
| [🇩🇪 10 × 11](#de-10-11) | `··NACHVOR··` | `·SECHS·UHR·` | 1 Stelle vor dem Rand |
| [🇩🇪 10 × 11 Alternativ](#de-10-11-alt) | `NACHAPPYVOR` | `SECHS·UHR··` | 2 Stellen vor dem Rand |
| [🇩🇪 10 × 11 Bayerisch](#de-10-11-bayerisch) | `··NOCHVOR··` | `·SECHSE····` | nicht unten |
| [🇩🇪 10 × 11 Uhr](#de-10-11-clock) | `VOR····NACH` | `ZEHNEUN·UHR` | **ganz rechts** |
| [🇩🇪 10 × 11 mrrioes](#de-10-11-mrrioes) | `·NACH·HALB·` | `·ZWÖLF·UHR·` | 1 Stelle vor dem Rand |
| [🇩🇪 10 × 11 Schwäbisch](#de-10-11-schwaebisch) | `NACH····VOR` | `SIEBNEVIERE` | nicht unten |

Fronten mit anderer Größe (8 × 8, 11 × 11, 13 × 13 …) erkennt man schon an der Anzahl der Zeilen und Buchstaben.

## Deutsch

| Layout | Besonderheit |
|---|---|
| [🇩🇪 10 × 11 Nero](#de-10-11-nero) | Wie „10 × 11“, aber **UHR ganz rechts unten** (…L U H R). Passt auch zu Fronten mit anderen Füllbuchstaben, z. B. …F M U H R. Voreingestellt in diesem Fork. |
| [🇩🇪 8 × 8](#de-08-08) | Kleine Uhr 8 × 8. |
| [🇩🇪 8 × 8 Viertelstunden](#de-08-08-viertel) | Kleine Uhr 8 × 8, nur Viertelstunden. |
| [🇩🇪 10 × 11](#de-10-11) | Standard-Layout von ESPWortuhr: NACH/VOR in Zeile 4, Stunden durcheinander (ZWÖLF in Zeile 5), UHR in der letzten Zeile in der Mitte. |
| [🇩🇪 10 × 11 Alternativ](#de-10-11-alt) | Wie Standard, mit HAPPY BIRTHDAY in Zeile 4/5. |
| [🇩🇪 10 × 11 Bayerisch](#de-10-11-bayerisch) | Bayerisch: „ETZ IST“, „HOIB“, „ZWOA“ … |
| [🇩🇪 10 × 11 Uhr](#de-10-11-clock) | Das klassische QlockTwo-Layout: VOR und NACH getrennt in Zeile 4, Stunden der Reihe nach (EINS, ZWEI, DREI …), UHR unten rechts. |
| [🇩🇪 10 × 11 mrrioes](#de-10-11-mrrioes) | VIERTEL und ZWANZIG in eigenen Zeilen, Stunden der Reihe nach, kein DREIVIERTEL. |
| [🇩🇪 10 × 11 Schwäbisch](#de-10-11-schwaebisch) | Schwäbisch: „ES ISCH“, „HALB“, „OIS“ … |
| [🇩🇪 11 × 11](#de-11-11) | Standard 10 × 11 plus eine Zeile mit FUNK/UHR (Funkuhr-Anzeige). |
| [🇩🇪 11 × 11 Schwäbisch](#de-11-11-schwaebisch) | Schwäbisch, 11 Zeilen. |
| [🇩🇪 11 × 11 Version 2](#de-11-11-v2) | Stunden der Reihe nach, zusätzliche Zeile mit FUNK/UHR. |
| [🇩🇪 11 × 11 (panbachi)](#de-11-11-v3) | Standard plus Zeile mit Sonderzeichen (panbachi). |
| [🇩🇪 13 × 13](#de-13-13) | Minutengenaue Anzeige (z. B. „DREI UHR SIEBENUNDZWANZIG“). |
| [🇩🇪 16 × 18](#de-16-18) | Große Front mit Minuten, Tageszeit (MORGENS, ABENDS …). |
| [🇩🇪 16 × 8](#de-16-8) | Breite Front mit Minuten in Ziffern. |
| [🇩🇪 22 × 11 Wetter](#de-22-11-weather) | 10 × 11 plus Wetterfeld (REGEN, WOLKEN, SCHNEE …). |

<a id="de-10-11-nero"></a>
### 🇩🇪 10 × 11 Nero

Wie „10 × 11“, aber **UHR ganz rechts unten** (…L U H R). Passt auch zu Fronten mit anderen Füllbuchstaben, z. B. …F M U H R. Voreingestellt in diesem Fork.

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
E S J I S T L F Ü N F
Z E H N Z W A N Z I G
D R E I V I E R T E L
T F N A C H V O R J M
H A L B X Z W Ö L F T
Z W E I N S I E B E N
L D R E I C I F Ü N F
E L F N E U N V I E R
U A C H T Z E H N S I
C S E C H S T L U H R
```

</td><td>

```
E S · I S T · F Ü N F
Z E H N Z W A N Z I G
D R E I V I E R T E L
· · N A C H V O R · ·
H A L B · Z W Ö L F ·
Z W E I N S I E B E N
· D R E I · · F Ü N F
E L F N E U N V I E R
· A C H T Z E H N · ·
· S E C H S · · U H R
```

</td></tr></table>

<sub>`DE10x11.nero.hpp`</sub>

<a id="de-08-08"></a>
### 🇩🇪 8 × 8

Kleine Uhr 8 × 8.

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
F Ü N F Z E H N
V O R N A C H E
H A L B D R E I
A C H T Z E H N
E L F S E C H S
V I E R N E U N
Z W Ö L F Ü N F
E I S I E B E N
```

</td><td>

```
F Ü N F Z E H N
V O R N A C H E
H A L B D R E I
A C H T Z E H N
E L F S E C H S
V I E R N E U N
Z W Ö L F Ü N F
E I S I E B E N
```

</td></tr></table>

<sub>`DE08x08.hpp`</sub>

<a id="de-08-08-viertel"></a>
### 🇩🇪 8 × 8 Viertelstunden

Kleine Uhr 8 × 8, nur Viertelstunden.

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
V I E R T E L N
H A L B V O R A
Z E H N E U N C
D R E I N S B H
Z W Ö L F Ü N F
E I S I E B E N
S E C H S E L F
V I E R A C H T
```

</td><td>

```
V I E R T E L N
H A L B V O R A
Z E H N E U N C
D R E I N S · H
Z W Ö L F Ü N F
E I S I E B E N
S E C H S E L F
V I E R A C H T
```

</td></tr></table>

<sub>`DE08x08.viertel.hpp`</sub>

<a id="de-10-11"></a>
### 🇩🇪 10 × 11

Standard-Layout von ESPWortuhr: NACH/VOR in Zeile 4, Stunden durcheinander (ZWÖLF in Zeile 5), UHR in der letzten Zeile in der Mitte.

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
E S K I S T R F Ü N F
Z E H N Z W A N Z I G
D R E I V I E R T E L
T G N A C H V O R U M
H A L B G Z W Ö L F J
Z W E I N S I E B E N
K D R E I R H F Ü N F
E L F N E U N V I E R
N A C H T Z E H N B X
U S E C H S F U H R Y
```

</td><td>

```
E S · I S T · F Ü N F
Z E H N Z W A N Z I G
D R E I V I E R T E L
· · N A C H V O R · ·
H A L B · Z W Ö L F ·
Z W E I N S I E B E N
· D R E I · · F Ü N F
E L F N E U N V I E R
· A C H T Z E H N · ·
· S E C H S · U H R ·
```

</td></tr></table>

<sub>`DE10x11.hpp`</sub>

<a id="de-10-11-alt"></a>
### 🇩🇪 10 × 11 Alternativ

Wie Standard, mit HAPPY BIRTHDAY in Zeile 4/5.

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
E S K I S T L F Ü N F
Z E H N Z W A N Z I G
D R E I V I E R T E L
N A C H A P P Y V O R
H A L B I R T H D A Y
D R Z W Ö L F Ü N F X
Z E H N E U N D R E I
Z W E I N S I E B E N
E L F V I E R A C H T
S E C H S I U H R Y E
```

</td><td>

```
E S · I S T · F Ü N F
Z E H N Z W A N Z I G
D R E I V I E R T E L
N A C H A P P Y V O R
H A L B I R T H D A Y
· · Z W Ö L F Ü N F ·
Z E H N E U N D R E I
Z W E I N S I E B E N
E L F V I E R A C H T
S E C H S · U H R · ·
```

</td></tr></table>

<sub>`DE10x11.alternative.hpp`</sub>

<a id="de-10-11-bayerisch"></a>
### 🇩🇪 10 × 11 Bayerisch

Bayerisch: „ETZ IST“, „HOIB“, „ZWOA“ …

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
E T Z K I S T F Ü N F
Z E H N E Z W A N Z G
D R E I V I E R T L E
J T N O C H V O R X M
H O I B E P Z D R E I
Z W O A N S S I E M E
K A C H T E F Ü N F E
N E I N E V I E R E Q
Z W O I F E Z E H N E
Y S E C H S E F U H R
```

</td><td>

```
E T Z · I S · F Ü N F
Z E H N · Z W A N Z G
D R E I V I E R T L ·
· · N O C H V O R · ·
H O I B E · · D R E I
Z W O A N S S I E M E
· A C H T E F Ü N F E
N E I N E V I E R E ·
Z W O I F E Z E H N E
· S E C H S E · · · ·
```

</td></tr></table>

<sub>`DE10x11.bayerisch.hpp`</sub>

<a id="de-10-11-clock"></a>
### 🇩🇪 10 × 11 Uhr

Das klassische QlockTwo-Layout: VOR und NACH getrennt in Zeile 4, Stunden der Reihe nach (EINS, ZWEI, DREI …), UHR unten rechts.

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
E S K I S T A F Ü N F
Z E H N Z W A N Z I G
D R E I V I E R T E L
V O R F U N K N A C H
H A L B A E L F Ü N F
E I N S X A M Z W E I
D R E I A U J V I E R
S E C H S N L A C H T
S I E B E N Z W Ö L F
Z E H N E U N K U H R
```

</td><td>

```
E S · I S T · F Ü N F
Z E H N Z W A N Z I G
D R E I V I E R T E L
V O R · · · · N A C H
H A L B · E L F Ü N F
E I N S · · · Z W E I
D R E I · · · V I E R
S E C H S · · A C H T
S I E B E N Z W Ö L F
Z E H N E U N · U H R
```

</td></tr></table>

<sub>`DE10x11.2clock.hpp`</sub>

<a id="de-10-11-mrrioes"></a>
### 🇩🇪 10 × 11 mrrioes

VIERTEL und ZWANZIG in eigenen Zeilen, Stunden der Reihe nach, kein DREIVIERTEL.

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
E S K I S T Ä F Ü N F
Z E H N V I E R T E L
Z W A N Z I G K V O R
P N A C H G H A L B M
E I N S R Z W E I P Y
K D R E I Y V I E R F
F Ü N F Q S E C H S K
M S I E B E N A C H T
N E U N Z E H N E L F
B Z W Ö L F J U H R X
```

</td><td>

```
E S · I S T · F Ü N F
Z E H N V I E R T E L
Z W A N Z I G K V O R
· N A C H · H A L B ·
E I N S · Z W E I · ·
· D R E I · V I E R ·
F Ü N F · S E C H S ·
· S I E B E N A C H T
N E U N Z E H N E L F
· Z W Ö L F · U H R ·
```

</td></tr></table>

<sub>`DE10x11.mrrioes.hpp`</sub>

<a id="de-10-11-schwaebisch"></a>
### 🇩🇪 10 × 11 Schwäbisch

Schwäbisch: „ES ISCH“, „HALB“, „OIS“ …

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
E S K I S C H F U N K
D R E I V I E R T L A
Z E H N B I E F Ü N F
N A C H G E R T V O R
H A L B X F Ü N F E I
O I S E C H S E L F E
Z W O I E A C H T E D
D R E I E Z W Ö L F E
Z E H N E U N E U H L
S I E B N E V I E R E
```

</td><td>

```
E S · I S C H · · · ·
D R E I V I E R T L ·
Z E H N · · · F Ü N F
N A C H · · · · V O R
H A L B · F Ü N F E ·
O I S E C H S E L F E
Z W O I E A C H T E ·
D R E I E Z W Ö L F E
Z E H N E U N E · · ·
S I E B N E V I E R E
```

</td></tr></table>

<sub>`DE10x11.schwaebisch.hpp`</sub>

<a id="de-11-11"></a>
### 🇩🇪 11 × 11

Standard 10 × 11 plus eine Zeile mit FUNK/UHR (Funkuhr-Anzeige).

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
E S K I S T R F Ü N F
Z E H N Z W A N Z I G
D R E I V I E R T E L
T G N A C H V O R U M
H A L B G Z W Ö L F J
Z W E I N S I E B E N
K D R E I R H F Ü N F
E L F N E U N V I E R
N A C H T Z E H N B X
U S E C H S F U E R Y
W A S D F U N K U H R
```

</td><td>

```
E S · I S T · F Ü N F
Z E H N Z W A N Z I G
D R E I V I E R T E L
· · N A C H V O R · ·
H A L B · Z W Ö L F ·
Z W E I N S I E B E N
· D R E I · · F Ü N F
E L F N E U N V I E R
· A C H T Z E H N · ·
· S E C H S · · · · ·
· · · · · · · · U H R
```

</td></tr></table>

<sub>`DE11x11.hpp`</sub>

<a id="de-11-11-schwaebisch"></a>
### 🇩🇪 11 × 11 Schwäbisch

Schwäbisch, 11 Zeilen.

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
S T I S C H A F E M F
Z E H N Z W A N O C H
V O R I V I E R T L E
B C H A L B E R A C H
E D R E I V I E R T L
O I S R S I E B E N E
Z W O I K S E C H S E
D R E I S Z W E L F E
V I E R E N F E M F E
A C H D E P N E I N E
Z E H N E U N E L F E
```

</td><td>

```
S T I S C H · F E M F
Z E H N Z W A N O C H
V O R · V I E R T L E
· · H A L B E R · · ·
· D R E I V I E R T L
O I S · S I E B E N E
Z W O I · S E C H S E
D R E I S Z W E L F E
V I E R E · F E M F E
A C H D E · N E I N E
Z E H N E · · E L F E
```

</td></tr></table>

<sub>`DE11x11.schwaebisch.hpp`</sub>

<a id="de-11-11-v2"></a>
### 🇩🇪 11 × 11 Version 2

Stunden der Reihe nach, zusätzliche Zeile mit FUNK/UHR.

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
E S K I S T A F Ü N F
Z E H N D A U V O R G
N A C H V I E R T E L
H A L B V O R N A C H
E I N S K U R Z W E I
D R E I A U J V I E R
F Ü N F T O S E C H S
S I E B E N L A C H T
A N E U N M H Z E H N
Z W Ö L F D T F E L F
W A S D F U N K U H R
```

</td><td>

```
E S · I S T · F Ü N F
Z E H N · · · V O R ·
N A C H V I E R T E L
H A L B V O R N A C H
E I N S · · · Z W E I
D R E I · · · V I E R
F Ü N F · · S E C H S
S I E B E N · A C H T
· N E U N · · Z E H N
Z W Ö L F · · · E L F
· · · · · · · · U H R
```

</td></tr></table>

<sub>`DE11x11.v2.hpp`</sub>

<a id="de-11-11-v3"></a>
### 🇩🇪 11 × 11 (panbachi)

Standard plus Zeile mit Sonderzeichen (panbachi).

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
E S K I S T L F Ü N F
Z E H N Z W A N Z I G
D R E I V I E R T E L
T G N A C H V O R J M
H A L B X Z W Ö L F P
Z W E I N S I E B E N
K D R E I R H F Ü N F
E L F N E U N V I E R
W A C H T Z E H N R S
B S E C H S F M U H R
E V * X * N * S * P I
```

</td><td>

```
E S · I S T · F Ü N F
Z E H N Z W A N Z I G
D R E I V I E R T E L
· · N A C H V O R · ·
H A L B · Z W Ö L F ·
Z W E I N S I E B E N
· D R E I · · F Ü N F
E L F N E U N V I E R
· A C H T Z E H N · ·
· S E C H S · · U H R
· · * · * · * · * · ·
```

</td></tr></table>

<sub>`DE11x11.v3.hpp`</sub>

<a id="de-13-13"></a>
### 🇩🇪 13 × 13

Minutengenaue Anzeige (z. B. „DREI UHR SIEBENUNDZWANZIG“).

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
F E S A I S T B S E C H S
Z E H N N U L L Z W Ö L F
E L F Z W E I S I E B E N
V I E R L E I N A N E U N
D R E I A C H T B F Ü N F
U H R N S E C H S V I E R
E I N S F Ü N F Z W E I B
N E U N A C H T G D R E I
S I E B E N F A B L A B N
U N D B G Z W A N Z I G F
Z W Ö L F A F Ü N F Z I G
V I E R Z I G B Z E H N L
A D R E I S S I G E L F B
```

</td><td>

```
· E S · I S T · S E C H S
Z E H N N U L L Z W Ö L F
E L F Z W E I S I E B E N
V I E R · E I N · N E U N
D R E I A C H T · F Ü N F
U H R · S E C H S V I E R
E I N S F Ü N F Z W E I ·
N E U N A C H T · D R E I
S I E B E N · · · · · · ·
U N D · · Z W A N Z I G ·
Z W Ö L F · F Ü N F Z I G
V I E R Z I G · Z E H N ·
· D R E I S S I G E L F ·
```

</td></tr></table>

<sub>`DE13x13.hpp`</sub>

<a id="de-16-18"></a>
### 🇩🇪 16 × 18

Große Front mit Minuten, Tageszeit (MORGENS, ABENDS …).

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
E S A I S T O V I E R T E L E I N S
D R E I N E R S E C H S I E B E N E
E L F Ü N F N E U N V I E R A C H T
N U L L Z W E I N Z W Ö L F Z E H N
U N D O Z W A N Z I G V I E R Z I G
D R E I S S I G F Ü N F Z I G U H R
M I N U T E N I V O R U N D N A C H
E I N D R E I V I E R T E L H A L B
S I E B E N E U N U L L Z W E I N E
F Ü N F S E C H S N A C H T V I E R
D R E I N S U N D A E L F E Z E H N
Z W A N Z I G G R A D R E I S S I G
V I E R Z I G Z W Ö L F Ü N F Z I G
M I N U T E N U H R E F R Ü H V O R
A B E N D S M I T T E R N A C H T S
M O R G E N S W A R M M I T T A G S
```

</td><td>

```
E S · I S T · · · · · · · · E I N ·
D R E I · · · S E C H S I E B E N ·
E L F Ü N F N E U N V I E R A C H T
N U L L Z W E I · Z W Ö L F Z E H N
U N D · Z W A N Z I G · · · · · · ·
· · · · · · · · · · · · · · · U H R
· · · · · · · · V O R U N D N A C H
· · · D R E I V I E R T E L H A L B
S I E B E N E U N · · · Z W E I N E
F Ü N F S E C H S · A C H T V I E R
D R E I · · U N D · E L F · Z E H N
Z W A N Z I G · · · D R E I S S I G
V I E R Z I G Z W Ö L F Ü N F Z I G
M I N U T E N · · · · F R Ü H · · ·
A B E N D S M I T T E R N A C H T S
M O R G E N S · · · · M I T T A G S
```

</td></tr></table>

<sub>`DE16x18.hpp`</sub>

<a id="de-16-8"></a>
### 🇩🇪 16 × 8

Breite Front mit Minuten in Ziffern.

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
E S H I S T C W L A N Z E I T X
Z W A N Z I G H A L B O Z E H N
F Ü N F V I E R T E L T N A C H
V O R E H A L B D Z E H N E L F
Z W E I D R E I N E U N V I E R
A C H T E I N S E C H S F Ü N F
S I E B E N Z W Ö L F T A U H R
U + D 1 2 3 4 P M I N U T E N X
```

</td><td>

```
E S · I S T · · · · · · · · · ·
Z W A N Z I G · · · · · Z E H N
F Ü N F V I E R T E L · N A C H
V O R · H A L B · Z E H N E L F
Z W E I D R E I N E U N V I E R
A C H T E I N S E C H S F Ü N F
S I E B E N Z W Ö L F · · U H R
· + · 1 2 3 4 · M I N U T E N ·
```

</td></tr></table>

<sub>`DE16x8.hpp`</sub>

<a id="de-22-11-weather"></a>
### 🇩🇪 22 × 11 Wetter

10 × 11 plus Wetterfeld (REGEN, WOLKEN, SCHNEE …).

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
E S K I S T L F Ü N F
Z E H N Z W A N Z I G
D R E I V I E R T E L
T G N A C H V O R J M
H A L B Q Z W Ö L F P
Z W E I N S I E B E N
K D R E I R H F Ü N F
E L F N E U N V I E R
W A C H T Z E H N B X
B S E C H S F U H R M
       X X X X
M O R G E N X F R Ü H
A B E N D M I T T A G
N A C H T S C H N E E
K L A R W A R N U N G
R E G E N W O L K E N
O G E W I T T E R B N
U N T E R Z Ü B E R K
Y M I N U S A N U L L
F Ü N F Z E H N U N D
H N Z W A N Z I G J T
D R E I ẞ I G O ° C X
```

</td><td>

```
E S · I S T · F Ü N F
Z E H N Z W A N Z I G
· · · · V I E R T E L
· · N A C H V O R · ·
H A L B · Z W Ö L F ·
Z W E I N S I E B E N
· D R E I · · F Ü N F
E L F N E U N V I E R
· A C H T Z E H N · ·
· S E C H S · U H R ·
· · · ·
M O R G E N · F R Ü H
A B E N D M I T T A G
N A C H T S C H N E E
K L A R W A R N U N G
R E G E N W O L K E N
· G E W I T T E R · ·
U N T E R · Ü B E R ·
· M I N U S · N U L L
F Ü N F Z E H N U N D
· · Z W A N Z I G · ·
D R E I ẞ I G · ° C ·
```

</td></tr></table>

<sub>`DE22x11.weather.hpp`</sub>

## Andere Sprachen

<a id="bn-9-8"></a>
### 🇧🇩 9 × 8

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
এ খ ন ত এ ক দু ই টা
তি ন য় দ শ চা র পাঁ চ
ছ য সা ত আ ট বা রো ফ
এ গা রো বে জে প য় ত্রি শ
বি শ পঁ চি শ চৈ তি দ শ
পঁ য় তা ল্লি শ ফা বি পাঁ চ
প ঞ্চা ন্ন চ ল্লি শ পো নে রো
বা জে ত প ঞ্চা শ মি নি ট
```

</td><td>

```
· · · · · · · · · · ·
· · · · · · · · · · · া ঁ চ
· · · · · · · · · · · ফ
· · · · · · · · · · · ় ত ্ র ি শ
· · · · · · · · · · · ি দ শ
· · · · · · · · · · · ফ া ব ি প · · চ
· · · · · · · · · · · ল ি শ প ো ন ে র ো
· · · · · · · · · · · ম ি · ি ট
```

</td></tr></table>

<sub>`BN9x8.hpp`</sub>

<a id="ch-10-11"></a>
### 🇨🇭 10 x 11

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
E S K I S C H U F Ü F
V I E R T U N F Z Ä Ä
Z W Ä N Z G S E V O R
A B C H A U B I E C M
E I S Z W Ö I S D R Ü
V I E R I F Ü F I S T
S Ä C H S I S I B N I
A C H T I N Ü N I E L
Z Ä N I E C H E U F I
Z W Ö U F I E N U H R
```

</td><td>

```
E S · I S C H · F Ü F
V I E R T U · · Z Ä Ä
Z W Ä N Z G · · V O R
A B · H A U B I · · ·
E I S Z W Ö I · D R Ü
V I E R I F Ü F I · ·
S Ä C H S I S I B N I
A C H T I N Ü N I · ·
Z Ä N I · · · E U F I
Z W Ö U F I · · · · ·
```

</td></tr></table>

<sub>`CH10x11.hpp`</sub>

<a id="ch-10-11-v2"></a>
### 🇨🇭 10 x 11 Version 2

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
E S K I S C H U F Ü F
V I E R T U N F Z Ä Ä
Z W Ä N Z G S E V O R
A B C H A U B I E C M
E I S Z W Ö I S D R Ü
V I E R I F Ü F I S T
S Ä C H S I S I B N I
A C H T I N Ü N I E L
Z Ä N I E C H E U F I
Z W Ö U F I E N G S I
```

</td><td>

```
E S · I S C H · F Ü F
V I E R T U · · Z Ä Ä
Z W Ä N Z G · · V O R
A B · H A U B I · · ·
E I S Z W Ö I · D R Ü
V I E R I F Ü F I · ·
S Ä C H S I S I B N I
A C H T I N Ü N I · ·
Z Ä N I · · · E U F I
Z W Ö U F I · · G S I
```

</td></tr></table>

<sub>`CH10x11v2.hpp`</sub>

<a id="ch-10-11-zh"></a>
### 🇨🇭 10 x 11 Züridütsch

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
E  S  K  I  S  C  H  A  Z  Ä  Ä
Z  W  Ä  N  Z  G  W  F  O  I  F
V  I  E  R  T  E  L  M  A  B  N
V  O  R  P  H  A  L  B  I  E  D
Ä  I  S  Ü  S  Ä  C  H  S  I  G
Z  W  Ä  I  L  F  Ö  I  F  I  Ä
D  R  Ü  Ü  C  Z  W  Ö  L  F  I
S  I  B  N  I  B  E  L  F  I  M
A  C  H  T  I  H  N  Ü  Ü  N  I
Z  Ä  Ä  N  I  X  V  I  E  R  I
```

</td><td>

```
E S · I S C H · Z Ä Ä
Z W Ä N Z G · F O I F
V I E R T E L · A B ·
V O R · H A L B I · ·
Ä I S · S Ä C H S I ·
Z W Ä I · F Ö I F I ·
D R Ü Ü · Z W Ö L F I
S I B N I · E L F I ·
A C H T I · N Ü Ü N I
Z Ä Ä N I · V I E R I
```

</td></tr></table>

<sub>`CH10x11ZH.hpp`</sub>

<a id="en-08-08"></a>
### 🇬🇧 8 × 8

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
T W F I F T H A
E N V E E N L F
T Y P A S T O T
T W E L V E L E
F O U R N V E N
S I X E I G H T
S E V E N O N E
T H R E E T W O
```

</td><td>

```
T W F I F T H A
E N V E E N L F
T Y P A S T O T
T W E L V E L E
F O U R N V E N
S I X E I G H T
S E V E N O N E
T H R E E T W O
```

</td></tr></table>

<sub>`EN08x08.hpp`</sub>

<a id="en-10-11"></a>
### 🇬🇧 10 × 11

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
I T L I S A S A M P M
A C Q U A R T E R D C
T W E N T Y F I V E X
H A L F S T E N F T O
P A S T E R U N I N E
O N E S I X T H R E E
F O U R F I V E T W O
E I G H T E L E V E N
S E V E N T W E L V E
T E N S E O'C L O C K
```

</td><td>

```
· T L · S A · · · · ·
· C · U A R T E R D ·
· W E N T Y F I V E X
· A L F S · E N F · O
· A S T E · · · I N E
· N E S I X T H R E E
· O U R F I V E T W O
· I G H T E L E V E N
· E V E N T W E L V E
· E N S · · ' C L O C K
```

</td></tr></table>

<sub>`EN10x11.hpp`</sub>

<a id="en-11-12"></a>
### 🇬🇧 11 × 12

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
I T L I S A S T H P M A
A C F I F T E E N D C O
T W E N T Y F I V E X W
T H I R T Y X T E N X W
M I N U T E S E T O U R
P A S T O R U F O U R T
S E V E N X T W E L V E
N I N E F I V E C T W O
E I G H T F E L E V E N
S I X T H R E E O N E G
T E N S E Z O C L O C K
```

</td><td>

```
I T · I S · · · · · · ·
· · F I F T E E N · · ·
T W E N T Y F I V E · ·
T H I R T Y · T E N · ·
M I N U T E S · T O · ·
P A S T · · · F O U R ·
S E V E N · T W E L V E
N I N E F I V E · T W O
E I G H T · E L E V E N
S I X T H R E E O N E ·
T E N · · · O C L O C K
```

</td></tr></table>

<sub>`EN11x12.hpp`</sub>

<a id="es-08-08-cuarto"></a>
### 🇪🇸 8 × 8 Viertelstunden

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
S E I S I E T E
C I N C O C H O
N U E V E U N A
C U A T R O D O
T R E S O N C E
D O S D I E Z Y
M E N O S C U A
M E D I A R T O
```

</td><td>

```
S E I S I E T E
C I N C O C H O
N U E V E U N A
C U A T R O D O
T R E S O N C E
D O S D I E Z Y
M E N O S C U A
M E D I A R T O
```

</td></tr></table>

<sub>`ES08x08.cuarto.hpp`</sub>

<a id="es-10-11"></a>
### 🇪🇸 10 × 11

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
E S O N E L A S U N A
D O S I T R E S O R E
C U A T R O C I N C O
S E I S A S I E T E N
O C H O N U E V E Y O
L A D I E Z S O N C E
D O C E L Y M E N O S
O V E I N T E D I E Z
V E I N T I C I N C O
M E D I A C U A R T O
```

</td><td>

```
E S O N · L A S U N A
D O S · T R E S · · ·
C U A T R O C I N C O
S E I S · S I E T E ·
O C H O N U E V E · ·
· · D I E Z · O N C E
D O C E · Y M E N O S
· V E I N T E D I E Z
V E I N T I C I N C O
M E D I A C U A R T O
```

</td></tr></table>

<sub>`ES10x11.hpp`</sub>

<a id="fr-10-11"></a>
### 🇫🇷 10 x 11

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
I L N E S T O D E U X
Q U A T R E T R O I S
N E U F U N E S E P T
H U I T S I X C I N Q
M I D I X M I N U I T
O N Z E R H E U R E S
M O I N S O L E D I X
E T R Q U A R T P M D
V I N G T - C I N Q U
E T S D E M I E P A M
```

</td><td>

```
I L · E S T · D E U X
Q U A T R E T R O I S
N E U F U N E S E P T
H U I T S I X C I N Q
M I D I X M I N U I T
O N Z E · H E U R E S
M O I N S · L E D I X
E T · Q U A R T · · ·
V I N G T - C I N Q ·
E T · D E M I · · · ·
```

</td></tr></table>

<sub>`FR10x11.hpp`</sub>

<a id="hu-10-10"></a>
### 🇭🇺 10 x 10

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
Ö T B T Í Z J A S U
P E R C C E L E W O
M Ú L T A M Ú L V A
H Á R O M U F É L J
N E G Y E D X H A T
N É G Y A H Á R O M
N Y O L C Ö T H É T
T I Z E N E G Y E B
T I Z E N K E T T Ő
K I L E N C Q Ó R A
```

</td><td>

```
Ö T · T Í Z · · · ·
P E R C C E L · · ·
M Ú L T · M Ú L V A
H Á R O M · F É L ·
N E G Y E D · H A T
N É G Y · H Á R O M
N Y O L C Ö T H É T
T I Z E N E G Y · ·
T I Z E N K E T T Ő
K I L E N C · Ó R A
```

</td></tr></table>

<sub>`HU10x10.hpp`</sub>

<a id="it-10-11"></a>
### 🇮🇹 10 × 11

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
S O N O R L E B O R E
È R Ľ U N A S D U E Z
T R E O T T O N O V E
D I E C I U N D I C I
D O D I C I S E T T E
Q U A T T R O C S E I
C I N Q U E A M E N O
E C U N O Q U A R T O
V E N T I C I N Q U E
D I E C I P M E Z Z A
```

</td><td>

```
S O N O · L E · O R E
È · Ľ U N A · D U E ·
T R E O T T O N O V E
D I E C I U N D I C I
D O D I C I S E T T E
Q U A T T R O · S E I
C I N Q U E · M E N O
E · U N · Q U A R T O
V E N T I C I N Q U E
D I E C I · M E Z Z A
```

</td></tr></table>

<sub>`IT10x11.hpp`</sub>

<a id="nl-10-11"></a>
### 🇳🇱 10 × 11

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
H E T L I S H N U W S
T W I N T I G T I E N
V I J F B Y K W A R T
V O O R P M O V E R U
H A L F I T W E E N N
A C H T D R I E Z E S
Z E V E N E G E N T O
T W A A L F A T I E N
V I E R V I J F E L F
U U R A G E W E E S T
```

</td><td>

```
H E T · I S · N U · ·
T W I N T I G T I E N
V I J F · · K W A R T
V O O R · · O V E R ·
H A L F · T W E E N ·
A C H T D R I E Z E S
Z E V E N E G E N · ·
T W A A L F · T I E N
V I E R V I J F E L F
U U R · G E W E E S T
```

</td></tr></table>

<sub>`NL10x11.hpp`</sub>

<a id="ro-10-11"></a>
### 🇷🇴 10 x 11

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
E S T E Z O R A P M O
D O U Ă N S P R E A M
U N S P R E Z E C E L
N O U Ă O P T Ş A S E
P A T R U N U T R E I
Ş A P T E C I N C I A
Ş I B T R E I Z E C I
F Ă R Ă O Z E C E U N
D O U Ă Z E C I V Ş I
C I N C I U S F E R T
```

</td><td>

```
E S T E · O R A · · ·
D O U Ă · S P R E · ·
U N S P R E Z E C E ·
N O U Ă O P T Ş A S E
P A T R U N U T R E I
Ş A P T E C I N C I ·
Ş I · T R E I Z E C I
F Ă R Ă · Z E C E U N
D O U Ă Z E C I · Ş I
C I N C I · S F E R T
```

</td></tr></table>

<sub>`RO10x11.hpp`</sub>

<a id="ru-10-11"></a>
### 🇷🇺 10 × 11

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
О Д И Н П Я Т Ь Д В А
Д Е Ш Е С Т Ь В Я Т Ь
В О Ч Е C E M Ь Т Р И
Т Ы Д В Е Р Е С Я Т Ь
Н А Д Ц А Т Ь Ч А С А
Ч А С О В Д С О Р О К
Т Р И Д В А Д П Я Т Ь
П Я Т Н А Д Е Ц А Т Ь
A M Д Е С Я Т С Я Т Ь
П Я Т Ь Я Р М И Н У Т
```

</td><td>

```
О Д И Н П Я Т Ь Д В А
Д Е Ш Е С Т Ь В Я Т Ь
В О Ч Е C E M Ь Т Р И
Т Ы Д В Е Р Е С Я Т Ь
Н А Д Ц А Т Ь Ч А С А
Ч А С О В · С О Р О К
Т Р И Д В А Д П Я Т Ь
П Я Т Н А Д · Ц А Т Ь
· · Д Е С Я Т С Я Т Ь
П Я Т Ь · · М И Н У Т
```

</td></tr></table>

<sub>`RU10x11.hpp`</sub>

<a id="se-10-11"></a>
### 🇸🇪 10 × 11

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
K L O C K A N T Ä R K
F E M Y I S T I O N I
K V A R T Q I E N Z O
T J U G O L I V I P M
Ö V E R K A M H A L V
E T T U S V L X T V Å
T R E M Y K Y F Y R A
F E M S F L O R S E X
S J U Å T T A I N I O
T I O E L V A T O L V
```

</td><td>

```
K L O C K A N · Ä R ·
F E M · · · T I O · ·
K V A R T · · · · · ·
T J U G O · · · I · ·
Ö V E R · · · H A L V
E T T · · · · · T V Å
T R E · · · · F Y R A
F E M · · · · · S E X
S J U Å T T A · N I O
T I O E L V A T O L V
```

</td></tr></table>

<sub>`SE10x11.hpp`</sub>

<a id="tr-10-11"></a>
### 🇹🇷 10 × 11

<table><tr><th>Front</th><th>Nur die Wörter</th></tr><tr><td>

```
S A A T R O N U Ü Ç Ü
B İ R İ A L T I Y I D
İ K İ Y İ D O K U Z U
D Ö R D Ü Y E D İ Y İ
S E K İ Z İ Y A R I M
D Ö R T A M S B E Ş İ
K P M O T U Z K I R K
E L L İ O N Y İ R M İ
B U Ç U K Ç E Y R E K
B E Ş M G E C İ Y O R
```

</td><td>

```
S A A T · O N U Ü Ç Ü
B İ R İ A L T I Y I ·
İ K İ Y İ D O K U Z U
D Ö R D Ü Y E D İ Y İ
S E K İ Z İ · · · · ·
D Ö R T A M · B E Ş İ
· · · O T U Z K I R K
E L L İ O N Y İ R M İ
B U Ç U K Ç E Y R E K
B E Ş · G E C İ Y O R
```

</td></tr></table>

<sub>`TR10x11.hpp`</sub>
