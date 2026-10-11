# 04 - Power

## Sources

| Net | Present when | Max current for the carrier | Notes |
|---|---|---|---|
| `+3V3` | always (battery or USB) | ~200-300 mA spare (estimate; the ESP32 with WiFi + Bluetooth already takes 150-200 mA average and 400+ mA peaks from the same regulator) | Olimex on-board regulator. Check its part and rating in the Olimex Rev.D schematic |
| `+5V_OLI` | when the Olimex micro-USB is powered (or when the carrier feeds it, see below) | - | the Olimex header's +5V pin. **0 V on battery only**: the Olimex board has no boost converter (verify in its schematic) |
| `VBUS_C` / `+5V_USBC` | when a USB-C charger or power bank is plugged into the carrier | up to 3 A (USB-C at 5 V with 5.1k CC pull-downs; less from a weak source) | new on the carrier (decision D5) |
| `VBAT` | always | Li-Po 3.0-4.2 V, 5000 mAh | only on the Olimex JST-PH connector, not on the headers; not used by the carrier |

## "USB-C PD version" (decision D5)

A USB-C sink with 5.1k pull-downs on CC1 and CC2 receives **5 V, up to 3 A** from any USB-C
charger or power bank, PD-capable or not: no PD controller chip is needed for 5 V. A PD controller
(e.g. CH224K, HUSB238, STUSB4500) only makes sense to ask for 9/12/15/20 V, which this board does
not need: the amplifier and the Olimex board run on 5 V (PAM8302A/PAM8403: 2.0-5.5 V, absolute
maximum 6 V). **Recommendation: plain USB-C 5 V sink.** If the owner still wants PD (open question
2 in `01-requirements.md`), use a fixed-5V or higher-voltage request with a buck converter to 5 V
behind it; that adds parts and gives no audible benefit here.

## Power tree (recommended)

```
USB-C VBUS --TVS-- polyfuse 2-3 A --+-- +5V_USBC --+-- +VAMP --> amplifier (bulk 220-470 uF + 10 uF + 1 uF)
                                    |              |
                                    |              +-- ideal diode (U3) --> +5V_OLI (Olimex +5V pin)   [open question 3]
                                    |
Olimex micro-USB --> (on the Olimex board) --> +5V_OLI --> Olimex charger + 3.3 V regulator --> +3V3
Li-Po (JST on Olimex) --------------------------------------------------------------------^
```

- **Amplifier** (`+VAMP` = `+5V_USBC`): the speaker works only with USB-C power (decision D5); on
  battery only the amplifier is unpowered, `AMP_SD` low does not matter then.
- **Feeding the radio from USB-C** (open question 3, recommended): one cable then powers and charges
  everything. It needs an ideal diode (or a power mux) between `+5V_USBC` and the Olimex +5V pin so
  that (a) USB-C does not push current back into a PC on the Olimex micro-USB and (b) the
  micro-USB does not feed the USB-C connector. Check in the Olimex Rev.D schematic how its +5V pin
  connects to its USB VBUS (a diode or not; the radio measured 4.54 V on its USB sense from a power
  bank, which suggests a diode drop already) and to the charger input. Without that path, the
  radio itself keeps running from its micro-USB / battery as today.
- **Never** put the amplifier on `+3V3`: its peaks would brown out the ESP32 (the radio already had
  brownout loops on a weak supply, see `AGENTS.md` "Brownouts").

## Loads

| Load | Supply | Typical | Peak |
|---|---|---|---|
| Display ST7789 1.69" (controller + backlight) | `+3V3` | 5-30 mA (backlight PWM) | 40 mA |
| microSD card | `+3V3` | 20-50 mA while writing | 100 mA |
| Encoder pull-ups | `+3V3` | < 1 mA | |
| PCM5102A | `+3V3` | ~15 mA | 25 mA |
| Amplifier, mono 4 Ohm at 5 V | `+VAMP` | 5-10 mA idle, 50-200 mA at room volume | ~0.6-0.8 A at full power |
| Olimex board via its +5V pin (if fed from USB-C) | `+5V_OLI` | 150-250 mA radio + up to ~0.45 A Li-Po charging | ~0.8 A |

USB-C budget with everything on: ~1.6 A peak; a 2 A or 3 A source is comfortable, a 1.5 A power
bank is borderline at full speaker volume while charging.

## Decoupling

- Display: 10 uF at J3.
- microSD: 10 uF + 100 nF at the socket.
- PCM5102A: per the TI datasheet (per supply pin), optional ferrite bead into AVDD.
- Amplifier: per the Diodes Inc. datasheet; bulk 220-470 uF on `+VAMP` near it.
- USB-C: 10 uF after the polyfuse.

## Grounding

- One ground plane on the bottom layer, no splits. Keep the amplifier's output current loop
  (`+VAMP` -> amplifier -> speaker -> back) compact and on its own corner of the board, away from
  the DAC's outputs and the jack.
- The DAC's AGND/DGND/CPGND pins go straight to the plane next to the chip; the jack sleeve joins
  the plane near the DAC.
- The amplifier output is bridged (BTL): no speaker wire to ground.
