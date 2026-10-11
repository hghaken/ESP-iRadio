# 05 - Layout guidelines

## Board and mechanics

- Start from the Olimex outline (from their Rev.D KiCad file) placed on a fab layer, so the
  carrier's headers line up exactly. Place the Olimex board so that:
  - its **USB connector** and **Li-Po JST connector** point to a board edge and stay reachable
    (the owner plugs a power bank into the USB connector daily, and the PC for USB updates);
  - its **reset button** stays reachable (or leave a hole above it);
  - its **WROVER antenna** sticks out past the carrier edge, or the carrier has **no copper on
    any layer** (and no parts) under the antenna and ~10 mm around it. The radio already fights
    for WiFi throughput next to Bluetooth: any loss at the antenna hurts (see `AGENTS.md`, "WiFi
    and Bluetooth share one 2.4 GHz radio").
- Display: on the front (or on a cable), its header above or below the Olimex board; encoder
  shaft through the front panel next to it. Agree the front layout with the owner first.
- Jack and speaker terminals on one board edge (the back of the enclosure).
- microSD socket at an edge where the card can be inserted with the board in the enclosure.
- 4 mounting holes, one per corner, clear of tracks (keep-out 1 mm around the screw head).
- Height: the Olimex board on female headers sits ~8.5-11 mm above the carrier; parts under it
  must stay below that (only 0805 passives there).

## Routing

- 2 layers: top for signals and short power, bottom as solid ground plane. Stitch ground vias
  around the board edge and next to signal vias.
- SPI (`SPI_SCK`, `SPI_MOSI`, `SPI_MISO`, CS lines): the display runs at 26.7 MHz. Keep them short
  (< 80 mm), over unbroken ground, no stubs; series resistors 22-33 Ohm footprints at the ESP32 end
  of `SPI_SCK` and `SPI_MOSI` (0 Ohm by default) in case of ringing.
- I2S (`I2S_BCK` 2.8 MHz, `I2S_LRCK`, `I2S_DATA`): short, over ground, away from the analog
  output and the speaker traces.
- Analog line out (`LINE_L`, `LINE_R`, `AGND`): short, guarded by ground, away from the amplifier
  output and the SPI clock.
- Amplifier: `+VAMP` and the speaker outputs >= 0.8 mm (1 mm preferred), the supply caps right at
  the chip/module pins, outputs straight to the terminals.
- Encoder lines can be thin (0.25 mm) but keep them off the I2S lines.

## Fab rules (JLCPCB/PCBWay standard 2-layer)

- Track/space 0.15/0.15 mm minimum (use 0.25 mm signals by default), via 0.3 mm drill / 0.6 mm pad.
- Board edge clearance 0.3 mm for copper.
- Silkscreen: connector pin names, "ESP iRadio", revision (Rev A), date, polarity marks on the
  battery JST (if option B of `04-power.md`), "+" on the speaker terminals.
- Run DRC with the fab's rule set; export Gerbers (F.Cu, B.Cu, F/B.Mask, F/B.Silkscreen,
  Edge.Cuts, F/B.Paste for assembly) and the Excellon drill file.
