# ESP32 Internet Radio

Battery-powered internet radio on an Olimex ESP32-DevKit-LiPo (WROVER, PSRAM): streams AAC/MP3 internet
radio over WiFi and plays it on Bluetooth headphones or a Bluetooth speaker (A2DP source), with a small colour TFT status
display (1.69" IPS, ST7789, 280x240: station logo, station, artist and song with scrolling for long lines,
Bluetooth link quality, buffer, WiFi signal, battery; high-contrast colours, no flicker; brightness,
night mode and idle dimming; a progress screen during firmware updates) and a web page for control,
configuration (up to 60 stations), diagnostics and firmware updates.

- Firmware: Arduino (C++) with PlatformIO. Build with `pio run`; update the running radio over WiFi with
  `pio run -t upload` (or upload `firmware.bin` on the web page). Serial flashing (`pio run -e usb -t upload`)
  is only needed once, for the OTA partition table. The firmware uses ~85% of an OTA slot (env `radio`) or ~74% (env `idf`); older
  builds are kept as git tags and in the git-ignored `backup/firmware/` for a quick rollback over WiFi.
- Live log: the web page has a "Log" section with an on/off switch that shows everything the radio
  also prints on the USB serial port (last ~64 KB kept, download as a text file), so the USB
  connector can stay on a power bank.
- Experimental: env `idf` builds Arduino as an ESP-IDF component with an own SDK configuration
  (`scripts/build_idf.ps1`, see `docs/framework-build.md`): classic Bluetooth only, no BLE/SPP/HFP,
  Bluetooth heap in PSRAM. ~74% of the OTA slot instead of ~85%, ~28 KB more free internal RAM. This
  is the build that runs on the radio. A larger TCP window was tried and did not work next to Bluetooth.
- First boot: join the access point `ESP-iRadio-Setup`, pick your WiFi network; afterwards use
  `http://radio.local/`.
- Bluetooth devices: up to 4 remembered (headphones, speakers), one plays at a time; play/pause on the device mutes, twice = next station, three times = previous. Pair a new
  one from the web page (device in pairing mode, type its name). Automatic mode tries them in the
  order you set (arrows); otherwise pick one with "Use" (restart, ~30 s). The radio reconnects
  with WiFi held off until the page finishes. "Try again" pages again and wipes nothing.
- SD card (microSD breakout on the SPI bus, CS GPIO4): daily log and measurement files with a chart
  on the web page, settings backup/restore, 64x64 station logos, copies of working firmware to roll
  back without a PC. Built in, switched off (`SD_ENABLED` in `src/config.h`) until the reader is wired.
- WiFi: the radio reconnects to the mesh point it last used. A minute after every connect, and
  whenever the signal stays under a threshold on the web page (default -65 dBm), it looks for a
  point at least 12 dB stronger and moves there. The "Search" list shows every mesh point (channel,
  BSSID) with a "Switch to this access point" button. Opening the page does not scan
  for networks unless "Search networks when the page opens" is on. Setup mode always scans.
- Battery: ~14 h on a 2500 mAh Li-Po; level shown on the display and the web page.
- Stations: plain `http://` AAC-LC or MP3 streams (no TLS). About 25 Dutch stations (NPO, Radio 10,
  Veronica, Qmusic, Arrow) are verified; the URLs and the rules for finding working ones are in
  AGENTS.md under "Stations". Stations that are too heavy, HE-AAC only or https-only (Joe, NPO
  Sterren.nl, RadioNL, Team FM) run through Liquidsoap relays on a NAS (`nas/`) that re-encode
  them to AAC-LC 96 kbps on the own Icecast server.

All project knowledge - hardware, wiring, build and update commands, and the pitfalls learned the hard way
(internal RAM, flash budget, WiFi/Bluetooth coexistence, headphone pairing, brownouts) - lives in
[AGENTS.md](AGENTS.md).

## Hardware documentation

Not included in this repository (third-party documents); get them from the makers:
- Olimex ESP32-DevKit-LiPo: product page and user manual at https://www.olimex.com/Products/IoT/ESP32/ESP32-DevKit-LiPo/ ,
  schematics and KiCad files at https://github.com/OLIMEX/ESP32-DevKit-LiPo
  (`docs/olimex-jumpers-bottom.png` is drawn from that KiCad layout).
- 1.69" ST7789 IPS module (240x280, e.g. ZJY169S0800TG01) and a generic SPI microSD breakout.
- Sony WH-1000XM6 help guide: https://helpguide.sony.net/

## Credits and licence

Built with PlatformIO and the Arduino core for the ESP32, and these libraries:
[ESP32-A2DP](https://github.com/pschatzmann/ESP32-A2DP) (Apache-2.0),
[arduino-audio-tools](https://github.com/pschatzmann/arduino-audio-tools) (GPL-3.0),
[arduino-libhelix](https://github.com/pschatzmann/arduino-libhelix) (GPL-3.0, Helix AAC/MP3 decoders),
[Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library) (BSD) and
[Adafruit ST7735 and ST7789](https://github.com/adafruit/Adafruit-ST7735-Library) (MIT).
The NAS relays use [Liquidsoap](https://www.liquidsoap.info/) and [Icecast](https://icecast.org/).

This project is licensed under the GNU General Public License v3.0, see [LICENSE](LICENSE).
Station URLs are examples of public Dutch broadcasters; host names such as `icecast.example.com`
and `my-nas` stand for your own servers.
