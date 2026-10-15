# Grok review, reply to the round 6 outcome

Checked commit `8ba0d64` (docs and comments only: `07`, `05`, `06`, `NOTES.md`, `gen_radio.py`, `mkpcb.py`, `pcbrules.py`). The board file and both schematic sheets are not in that commit. No new route.

The twelve points match the files:

- CN1 order, hole drill 2.7 mm, and round M2.5 standoffs about 4 mm across are in `07`, `05`, `NOTES.md`, `mkpcb.py` and `pcbrules.py`.
- `gen_radio.py` now says J3 pin 3 CLK.
- CN2's own order is in `07`, with the warning never to tie the sockets pin for pin. J7 stays open. The old "duplicates CN1" sentence is gone from the live docs (it remains only in the archived round 6 text, with the correction above it).
- The first-module check (GPIO5 selects the glass, GPIO26 is the backlight; swap pins 7 and 8 only if the module disagrees) is in `07`, `NOTES.md` and the `06` checklist.
- Backlight current from `+3V3`, glass front about 14 mm, and PWM at 20 kHz or above are in `07`.
- The encoder row no longer offers C470754 as the shaft for this height. The footprint stays the Alps 20 mm shaft until the case thickness is known.

Two sentences to align, still not a layout change:

1. The coax exit. `07` and `NOTES.md` say the cable "leaves through the ~11 mm gap". The module outline starts at x 0.5, so the cable cannot leave past the left edge. It runs in that gap and comes out below the module, past y 36.9. Add that, so the case slot is cut under the bottom edge of the module.
2. `NOTES.md` still lists two narrow necks in the B.Cu pour under "still to do". The round 6 DRC said those necks are gone and the remaining warnings are silkscreen. Drop that bullet unless a fresh DRC shows them again. The `06` checkbox "review of the 90 x 60 board (round 6)" can be ticked: this reply closes it.
