# Grok recheck, round 3 done

Checked commit `717a85c` ("USBLC6 VBUS behind the fuse"). Exported a netlist with KiCad 9 `kicad-cli` and ran `tools/intcheck.py`. Result: 107 nets, 39 WROVER pins, `OK`. ERC was not run again.

## USBLC6 pin 5 is on VIN

From the exported netlist, not only from the generator:

- `/Power, charger and USB/VBUS` is only J201 A4B9, J201 B4A9 and F201 pin 1.
- `/Power, charger and USB/VIN` is F201 pin 2, D201 pin 1 (cathode), U202 pin 5, U201 pin 13 (IN), R203 pin 1, C201 pin 1 and TP201. D201 pin 2 (anode) is on GND.
- U202 pins 1 and 3 are still on the connector D+ and D−. Pins 6 and 4 still go to the CH340C.

The two `VIN` labels in `power_usb.kicad_sch` sit on the fused rail (116.84, 40.64) and on U202 pin 5 (60.96, 91.44). `07`, `NOTES.md` and `tools/README.md` describe the same split, including the limit that the USBLC6 clamp conducts before the SMAJ.

The apart-check now uses the full sheet path and treats a missing net as an error. VBUS/VIN, VSYS/VBAT and GND/BATN are present and separate, which is why `intcheck.py` passes. The single-node lines it prints are the pins that are meant to be open (GPIO12, GPIO15, ITERM, GAIN_SLOT, the CH340C handshake pins, and the FS8205A drain pins).

No further schematic change from this recheck.

## Still open

J5 pin order on a sample, which way SW203 is ON, J4 against a manufacturer drawing, short-shaft EC11 for SW1, FPC pin 1 and R22 from the panel Vf, BQ24074 and WROVER thermal vias, "+"/"-" silk on J202 and J6.

Firmware for this board only, not for the radio that is running: `PWR_CAL_PERMILLE` ~2500, GPIO34/36 polled, backlight PWM at least 20 kHz, 16 MB table on a full USB flash.
