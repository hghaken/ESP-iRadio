# 04 - Power

> **Status 2026-10-15:** phase 1 power tree (Olimex regulator, USB-C into the Olimex +5V pin).
> Superseded by "Power" in `07-integrated-board.md` (USB-C -> BQ24074 power path -> VSYS ->
> AP7361C / LP5907, cell protection, on/off switch on the LDO enable).

## Sources

| Net | Present when | Max current for the carrier | Notes |
|---|---|---|---|
| `+3V3` | always (battery or USB) | ~200-300 mA spare (estimate; the ESP32 with WiFi + Bluetooth already takes 150-200 mA average and 400+ mA peaks from the same regulator) | Olimex on-board regulator. Check its part and rating in the Olimex Rev.D schematic |
| `+5V_OLI` | when the Olimex micro-USB is powered, or when USB-C on the carrier feeds it through U3 (decision D7) | - | the Olimex header's +5V pin. **0 V on battery only**: the Olimex board has no boost converter (verify in its schematic) |
| `VBUS_C` / `+5V_USBC` | when a USB-C charger or power bank is plugged into the carrier | up to 3 A (USB-C at 5 V with 5.1k CC pull-downs; less from a weak source) | new on the carrier (decision D5) |
| `VBAT` | always | Li-Po 3.0-4.2 V, 5000 mAh | only on the Olimex JST-PH connector, not on the headers; not used by the carrier |

## USB-C input (decisions D5 and D7)

A plain USB-C sink: 5.1k pull-downs on CC1 and CC2 make any USB-C charger or power bank (PD-capable
or not) deliver **5 V, up to 3 A**. No PD controller: higher PD voltages are not needed (the
amplifier and the Olimex board run on 5 V; the MAX98357A allows 2.5-5.5 V, 6 V absolute
maximum).

## Power tree (recommended)

```
USB-C VBUS --TVS-- polyfuse 2-3 A --+-- +5V_USBC --+-- +VAMP --> amplifier (bulk 220-470 uF + 10 uF + 100 nF)
                                    |              |
                                    |              +-- ideal diode (U3) --> +5V_OLI (Olimex +5V pin)   [D7]
                                    |
Olimex micro-USB --> (on the Olimex board) --> +5V_OLI --> Olimex charger + 3.3 V regulator --> +3V3
Li-Po (JST on Olimex) --------------------------------------------------------------------^
```

- **Amplifier** (`+VAMP` = `+5V_USBC`): the speaker works only with USB-C power (decision D5); on
  battery only the amplifier is unpowered, `AMP_SD` low does not matter then.
- **USB-C powers the radio and charges the Li-Po** (decision D7): `+5V_USBC` feeds the Olimex +5V
  pin through the ideal diode U3, so one cable powers and charges everything. U3 stops the Olimex
  micro-USB (a PC during USB updates) from feeding the USB-C connector.
  **Verified 2026-10-11 in the Olimex Rev.D netlist:** the micro-USB VBUS (`/+5V_USB`) reaches the
  board's `+5V` net only through a 1N5819 Schottky (D1), so USB-C on the +5V pin cannot push current
  back into a PC (this diode also explains the 4.54 V the radio reads from a power bank). The `+5V`
  net (header EXT1 pin 19) feeds the BL4054B charger (U1 VCC), the WPM2015 P-MOSFET power path
  (FET1: battery cut off from the load while 5 V is present), the 3.3 V regulator through another
  1N5819 (D2), the charge LED and the USB-voltage divider (R15 220k, GPIO39). So the D7 path works
  as planned.
- The radio's USB voltage reading (GPIO39, `diag.usbMv`) then shows the USB-C supply too, so the
  firmware's "USB" indication and the power guard keep working.
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
