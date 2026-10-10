# Display research: menu with rotary encoder, low power (2026-10-07)

Status 2026-10-08: the three-colour e-paper is still fitted. This document is research, not a
completed conversion. The firmware still refreshes that screen only after a stable change.

Background: the 2.13" three-colour e-paper refreshes in ~15 s with flashing and cannot update a
single line; unsuitable for a menu you step through with a rotary knob. Wanted: a display that
responds immediately, draws little current and suits the ESP32 radio (SPI, 3.3 V, limited RAM/flash).

## Budget
While playing, the radio draws ~150-200 mA (WiFi + Bluetooth + decoder). A display of 10-20 mA
therefore costs 5-10 % of the playing time (~1 hour of the ~14). Consumption below 20 mA is "good
enough"; response speed, readability (also in the dark?) and simplicity matter more.

## Candidates

| Type | Example | Consumption | Response | Readable | Price | Notes |
|---|---|---|---|---|---|---|
| E-paper black/white 2.13" (SSD1680) | WeAct 2.13" B/W, same connector as the current one | ~0 mA idle, ~20 mA while refreshing | partial 0.5-0.75 s (GxEPD2 measurements: `GxEPD2_213_BN` 750 ms, `_B74` 500 ms), full 4 s | only with light | €5-8 | Best for battery and a static image, but every encoder step feels 0.5 s late. Ghosting; an occasional full refresh is needed. Not for a menu. |
| **OLED mono 2.42" 128x64 (SSD1309, SPI)** | generic 2.42" yellow/white/blue | 5-15 mA for text (black costs nothing), 20-30 mA full white; sleep ~10 µA | immediate (100+ Hz) | anywhere, also in the dark | €10-15 | **Recommended.** Big enough for station list + title + bar, Adafruit_GFX/U8g2, 1 kB framebuffer. Limit burn-in risk: dim after 1 min, screen off after 10 min, no fixed white areas. |
| OLED mono 1.3" 128x64 (SH1106, I2C/SPI) | generic 1.3" | 5-15 mA | immediate | anywhere | €3-5 | Same as above but small; 4 lines of text. Cheapest way in, same code. |
| Sharp Memory LCD 2.7" 400x240 (LS027B7DH01) | Adafruit breakout (with 5 V boost and level shifter) | 0.05-0.2 mW (= ~0.05 mA), almost nothing even while refreshing | immediate (up to 30 Hz) | excellent in bright daylight, indoors mediocre, not in the dark (no backlight) | €40-50 | The consumption champion and sharp, but expensive, 12 kB framebuffer in internal RAM (which we are short of), and unreadable in the evening without a lamp. The 1.3" LS013B7DH03 (128x128, 3 V) is the small/cheap variant (~€20). |
| IPS TFT 1.69"/2.0" 240x280/320 (ST7789, SPI) | generic IPS module with BLK pin | controller ~4 mA + backlight 20-40 mA (PWM-dimmable, off = ~0) | immediate | anywhere, colour | €6-10 | Nicest picture, highest consumption. Acceptable with the backlight dimmed/switched off automatically after a minute; graphics cost CPU (we need it for audio) and flash (TFT_eSPI ~50 kB). |

Not suitable: three-colour/four-colour e-paper (slow), e-paper with "fast refresh" via a custom LUT
(unofficial, ghosting), colour OLED 1.5" SSD1351 (25-40 mA, expensive), character LCD (HD44780: 5 V,
backlight 30+ mA, ugly).

## Recommendation
1. **Mono OLED 2.42" SSD1309 over SPI** as the main choice: immediate, cheap, readable at night, 5-15 mA,
   and the existing Adafruit_GFX code (fonts, text) stays usable. Reuse the e-paper's VSPI bus
   (SCK 18, MOSI 23) with its own CS/DC/RST; the e-paper can come off.
2. Alternative if the radio is mostly used in daylight/outdoors: Sharp Memory LCD 2.7".
3. The current three-colour e-paper stays useful as a "static" screen (station, battery) if you do
   *not* want a menu; not for a menu.

## Rotary encoder
- Type: EC11 (with push button), 20 or 24 steps per revolution, preferably a module with pull-ups and
  100 nF debouncing (e.g. "KY-040", or a bare EC11 with its own 10 k pull-up and 100 nF to GND per pin).
- Pins (free on this board, with internal pull-up): **GPIO32 (A), GPIO33 (B), GPIO25 (button)**.
  Also free: 13, 14, 19, 26, 27. Do not use: 34/36/39 (no pull-up, 35/39 are battery sensing),
  0/2/12/15 (strapping), 16/17 (PSRAM), 1/3 (USB serial), 4/5/18/21/22/23 (display).
- Read with the **PCNT hardware counter** (library `ESP32Encoder`, `madhephaestus/ESP32Encoder`):
  counts in hardware, so no missed steps because of WiFi/Bluetooth interrupts, with a built-in filter
  against contact bounce. Button: interrupt or polling in `loop()` with 30 ms debounce, long press =
  back.
- Menu idea: turning = scroll through stations (visible at once, switch station only after 1 s of
  stillness), pressing = menu (volume, station, re-pair Bluetooth, WiFi info, battery), long press =
  back/off. The same functions remain available through the web page.

## Impact on the firmware
- Flash: Adafruit_SSD1306/SH110X ~10-20 kB extra (GFX is already there); U8g2 ~40-60 kB. Budget is ~200 kB.
- RAM: OLED 1 kB framebuffer; Sharp 12 kB internal (tight but feasible); do not use a framebuffer
  for the TFT (115 kB), draw directly.
- Display task: the current "draw only after 3 s of calm" logic goes away; draw on every change
  (encoder step, title) and add a dim/off timer for the OLED.

## Sources
- GxEPD2 drivers (`GxEPD2_213_BN.h`, `_B74.h`): partial 500-750 ms, full 3.6-4.1 s.
- Waveshare 2.13" B/W: 0.3 s partial / 2 s full (factory measurement), 26 mW while refreshing:
  https://www.waveshare.com/wiki/2.13inch_e-Paper_HAT_Manual
- Sharp LS027B7DH01: 50-175 µW, 400x240, SPI, 5 V (A variant): https://cgi.global.sharp/products/device/lineup/selection/lcd/mobile/index.html ,
  Adafruit breakout: https://learn.adafruit.com/adafruit-sharp-memory-display-breakout/programming?view=all
- SSD1306/SSD1309 OLED consumption (3-8 mA text, 25-30 mA full white): https://preamp.org/static/ssd1306-oled-current-consumption/index.html ,
  https://docs.soldered.com/ssd1306/overview/
- ST7789 IPS 1.3": controller ~4 mA, backlight 20-40 mA, BLK PWM: https://done.land/components/humaninterface/display/tft/st7789/
