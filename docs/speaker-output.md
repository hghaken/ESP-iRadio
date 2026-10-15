# Speaker and line output: PCM5102A DAC + PAM8403 amplifier (plan, 2026-10-09)

**Status 2026-10-14:** the line output works (firmware v1.30, `lineTask` in `src/player.cpp`).
The module that arrived is a GeeDIY "PCM5102MK 2.0", not the purple GY-PCM5102 described below:
6-pin header VCC, GND, GND, LRCK, DATA, BCK; VCC = 5 V (three AMS1117-3.3 on the board); SCK
and XSMT are handled on the board; FMT is a jumper (set to I2S), FLT a solder bridge (N/L, left
as shipped); RCA and 3.5 mm outputs. Wiring: VCC 5V, GND, BCK GPIO27, LRCK GPIO14, DATA GPIO13.
The firmware uses 16-bit I2S slots (32-bit slots sounded the same). Test tone: `curl -X POST
http://radio.local/api/linetest -d "tone=1"` (back with `tone=0`). The PAM8403 is not wired yet.

**Speaker amplifier, 2026-10-14: two boards on hand, nothing wired yet.**

1. *PAM8403 mini board with volume pot* (green, "rout - +, lout + -, power - +, input L G B"):
   analog input from the DAC's line output (tap the 3.5 mm plug: tip L, ring R, sleeve ground,
   or the RCA pins), power + to 5V, power - to GND, speaker 4-8 ohm on lout +/- (bridged outputs:
   never a speaker wire to ground, never both channels on one speaker; for true mono feed L and
   R each through 10k into input L). **No shutdown pin**, so the ESP32 cannot switch it off
   (~10 mA idle; silent while Bluetooth is on because I2S stops).
2. *MAX98357A I2S mono amplifier* (Adafruit-style breakout, pins LRC BCLK DIN GAIN SD GND Vin,
   speaker +/- on top, "default 9 dB gain & (L+R)/2 out"): digital input, its own DAC. It can
   share the I2S lines with the PCM5102A (one transmitter, two receivers; both play the same,
   the software volume applies to both, the 16/32-bit slot setting suits both):
   LRC -> GPIO14, BCLK -> GPIO27, DIN -> GPIO13, Vin -> 5V (2.5-5.5 V), GND -> GND, GAIN open
   (9 dB; to GND 12 dB, to Vin 6 dB), speaker 4-8 ohm >= 3 W on +/- (never to ground).
   SD: the board pulls it up through 1 Mohm, which with the chip's internal pull-down gives the
   "(L+R)/2, on" level. Plan: SD -> GPIO2, firmware drives GPIO2 **low** = amplifier off and
   releases it (**input**) = on; never drive it high with 3.3 V (that selects the left channel
   only). GPIO2 is low at boot, so no pop at power-on. Peak current with speaker and DAC on 5V up
   to ~0.5-1 A (fine on the power bank, maybe not on a PC port).

Recommendation given: the MAX98357A for a single built-in speaker (no analog wiring, can be
switched off). Built 2026-10-15 (v1.37): web checkbox "Speaker" (on = the MAX98357A plays with
the line output, off = line output only; both silent while Bluetooth is on); GPIO2 low = off,
released = on, switched on after I2S starts and off before it stops. Wired on the breadboard
the same day (GAIN open, 9 dB) and heard playing next to the line output. The KiCad design still has the analog route (PCM5102APWR + PAM8302A); a
MAX98357A there would save the PAM8302A and its input network.

Goal: besides the Bluetooth headphones, add (1) a line output on RCA/3.5 mm for an amplifier or active
speakers and (2) a small speaker directly on the radio. The Bluetooth switch in the GUI chooses:
Bluetooth on = headphones, off = DAC output (line + speaker).

Ordered (2026-10-09): PCM5102A DAC module (the purple "GY-PCM5102" board) and a PAM8403
amplifier module (2 x 3 W class D).

## Why these two
- **PCM5102A**: a real line output (2.1 Vrms, 112 dB SNR, own low-pass filter, no MCLK needed),
  I2S from the ESP32. Supplies the signal for the RCA connectors and for the amplifier.
- **PAM8403**: 2 x 3 W at 5 V into 4 Ω (roughly 1 W per channel from the battery, 3.7 V), line-level input,
  ~10 mA idle. Sits behind the DAC's line output.
- A MAX98357A (I2S + amplifier in one) was dropped: it only has a balanced speaker output, no line
  output; an RCA plug on it would short it out.

## Wiring
### PCM5102A module (GY-PCM5102)
| Module | ESP32 | Note |
|---|---|---|
| VIN | 5V (USB) or BAT | the module has its own 3.3 V regulator; 5 V is safest, BAT (3.7 V) works on most units |
| GND | GND | |
| LCK (LRCK) | GPIO14 | I2S word select, 44.1 kHz |
| DIN | GPIO13 | I2S data |
| BCK | GPIO27 | I2S bit clock (2.8 MHz) |
| SCK | **GND** | master clock not used: tie to ground, otherwise no sound. Most purple boards have a solder jumper on the back for this ("SCK" to GND), otherwise use a wire |

Solder jumpers on the back of the purple board (H = high, L = low; often already set this way from the factory):
| Jumper | Setting | Meaning |
|---|---|---|
| 1 FLT | L | normal filter |
| 2 DEMP | L | de-emphasis off |
| 3 XSMT | **H** | sound on (L = mute) |
| 4 FMT | L | I2S format (H = left-justified) |

Output: 3.5 mm socket plus solder pads **L, R, G** next to the socket. RCA connectors: L → centre pin of
the red/white RCA, R → centre pin of the other, G → both RCA grounds. Shielded cable, keep it short.

### PAM8403 amplifier module
| Module | Connection | Note |
|---|---|---|
| +5V / VCC | BAT (3.7 V) or 5V (USB) | not on 3V3: the 3.3 V regulator of the Olimex board cannot supply the current peaks |
| GND | GND | common ground with the DAC |
| L IN / R IN / GND | pads L / R / G of the DAC | inputs have their own coupling capacitors; mono speaker: use only L, or L and R on two speakers |
| L+ / L- (R+ / R-) | speaker(s) 4-8 Ω, 2-3 W | **balanced**: never connect a speaker wire to ground; never both outputs to one speaker |
| SD (shutdown, if the board has that pin) | GPIO2 | high = on; the firmware switches the amplifier off while Bluetooth is on. Board without SD pin: amplifier stays on (~10 mA) or power it through a small MOSFET on GPIO2 |

Boards with a potentiometer: set it to ~3/4 and adjust the volume further on the radio (software).

### Pins on the ESP32
Used after this extension: display (Arduino Robot LCD, since 2026-10-10) 5/18/21/22/23 +
backlight 26 + SD card 4 (CS) and 19 (MISO), encoder 32/33/25 (planned), battery 35/39, I2S 27/14/13,
amplifier SD **2** (was 19, but that is now the SD card's MISO). GPIO2 is a strapping pin
that is low at start-up: the amplifier then stays off, which is what we want. Not available:
16/17 (PSRAM), 0/12/15 (strapping), 1/3 (USB serial), 6-11 (flash). After that everything is taken.
GPIO13/14 are briefly undefined during start-up; at worst that gives a tick from the speaker.

## Current and playing time
DAC ~15 mA. PAM8403 idle ~10 mA, at room volume on average 50-150 mA, peaks ~0.5 A at 3.7 V.
When listening through the speaker, expect a third to a half less playing time than with the
headphones (~14 h on the former 2500 mAh cell, ~28 h expected on the 5000 mAh cell). Line output only (amplifier off): hardly any difference.

## Firmware plan
- Output choice in `PcmSink`: the same 44.1 kHz 16-bit stereo PCM goes to (a) the Bluetooth buffer or
  (b) the I2S output (`i2s_write`, DMA, ~8 kB internal RAM for DMA buffers). Software volume stays.
- The choice follows the Bluetooth switch: on = headphones (I2S silent, amplifier off via GPIO2);
  off = DAC (line output + amplifier on). Possibly a separate "speaker" button later.
- The decoder must then also run without headphones (now it waits for the Bluetooth link).
- I2S on the ESP32 (IDF 4.4): `i2s_driver_install(I2S_NUM_0)` with `I2S_MODE_MASTER | I2S_MODE_TX`,
  44100 Hz, 16 bit, stereo, `I2S_COMM_FORMAT_STAND_I2S`, pins BCK 27 / WS 14 / DOUT 13, no MCLK.
  The A2DP library does not use I2S (source, not sink), so no conflict. ~5-10 kB flash.
- For a sample rate other than 44.1 kHz, `PcmSink` already resamples (linear); I2S stays at 44.1 kHz.
- Without active Bluetooth, WiFi is relaxed (no coexistence problems): then the 320 kbps stream
  probably also plays without stuttering.

## Order of work
1. Bluetooth switch in the GUI (done, 2026-10-09).
2. Connect PCM5102A + PAM8403 according to the tables (SCK to ground, XSMT high, SD on GPIO2).
3. I2S output in `PcmSink` + switching on the Bluetooth switch; test via OTA, first with the
   line output (headphones in the 3.5 mm socket), then the amplifier.
