# 04 - Power

## Sources (from the Olimex ESP32-DevKit-LiPo)

| Net | Present when | Max current for the carrier | Notes |
|---|---|---|---|
| `+3V3` | always (battery or USB) | ~200-300 mA spare (estimate; the ESP32 with WiFi + Bluetooth already takes 150-200 mA average and 400+ mA peaks from the same regulator) | Olimex on-board regulator. Check its part and rating in the Olimex Rev.D schematic |
| `+5V_USB` | only when USB is powered (power bank or charger) | limited by the USB source and the board's input protection | **0 V on battery only**: the Olimex board has no boost converter (verify in the schematic) |
| `VBAT` | always | Li-Po 3.0-4.2 V, 5000 mAh | **not on the headers**: only on the Olimex JST-PH connector |

## Loads on the carrier

| Load | Supply | Typical | Peak |
|---|---|---|---|
| Display ST7789 1.69" (controller + backlight) | `+3V3` | 5-30 mA (backlight PWM) | 40 mA |
| microSD card | `+3V3` | 20-50 mA while writing | 100 mA (insertion / write bursts) |
| Encoder pull-ups | `+3V3` | < 1 mA | |
| PCM5102A (module or chip) | `+3V3` | ~15 mA | 25 mA |
| PAM8403 2 x 3 W | `+VAMP` | 10 mA idle, 50-150 mA at room volume | ~0.5 A per channel at full power into 4 Ohm |

`+3V3` total on the carrier: ~100-200 mA worst case, acceptable for the Olimex regulator as long
as the amplifier is **not** on `+3V3` (its peaks would brown out the ESP32: the radio already had
brownout loops on a weak supply, see `AGENTS.md` "Brownouts").

## Amplifier supply `+VAMP` (owner decides, see `01-requirements.md` question 5)

| Option | How | Pro | Contra |
|---|---|---|---|
| A: USB 5 V | `+VAMP` = `+5V_USB` | trivial, full 2 x 3 W | no speaker on battery only; the power bank must deliver ~1 A |
| B: battery | Li-Po plugs into a JST-PH on the carrier; a short JST-PH cable carries `VBAT` on to the Olimex JST; `+VAMP` = `VBAT` | speaker works on battery; ~1 W per channel at 3.7 V | the carrier sits in the charge path: traces >= 1 mm, keep the cable short; reverse-polarity risk on JST (check the battery's pin order) |
| C: both | ideal-diode OR (e.g. two Schottky diodes or an LM66100-class ideal diode) of `+5V_USB` and `VBAT` | always on, louder on USB | more parts |

Recommendation: **B** (the radio is a battery radio), with:
- a 1.5-2 A polyfuse or fuse in the `VBAT` branch to the amplifier only (not in the path to the
  Olimex board);
- 470 uF bulk + 10 uF + 1 uF close to the amplifier;
- a slide switch or jumper in the amplifier branch, so the amplifier can be cut off completely
  (its shutdown pin already cuts it to < 1 uA when `AMP_SD` is low).

## Decoupling

- Display: 10 uF at J3.
- microSD: 10 uF + 100 nF at the socket.
- DAC chip variant: per the TI datasheet (10 uF + 100 nF per supply pin).
- Amplifier: per the Diodes Inc. datasheet (see above).

## Grounding

- One ground plane on the bottom layer. Split it only logically: keep the amplifier's output
  current loop (supply -> PAM8403 -> speaker -> back) compact and away from the DAC's analog
  output and the jack.
- `AGND` of the DAC/jack joins `GND` at one point near the DAC (module: its G pad).
- The PAM8403 outputs are bridged (BTL): no speaker wire to ground, no shared return with the line
  output.
