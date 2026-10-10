# AGENTS.md

## Project
ESP32-InternetRadio: a battery-powered internet radio built from an Olimex ESP32 board, a small
1.69" colour IPS display (ST7789, since 2026-10-10) and Sony Bluetooth headphones. The radio
streams internet radio stations over WiFi, sends the audio to the headphones over Bluetooth A2DP,
shows station/status info on the display and is configured/controlled through a web page served by
the ESP32.

Current state (2026-10-10): working and in daily use. WiFi setup via captive portal; the radio
remembers the last mesh access point and moves to a point of the same network that is at least
12 dB stronger, checked a minute after every connect and while the signal is weak (the web scan
list also offers a manual hop). Bluetooth gets the radio to itself while connecting (at boot and in
paging windows). Bluetooth devices: up to 4 remembered (now a JBL Charge 5 speaker and the
WH-1000XM6 headphones), one plays at a time; automatic mode tries them in the user's order (JBL
first), or pick one by hand; new ones are paired from the web page. Web page: live diagnostics
(incl. Bluetooth link quality and build stamp), live log window (toggle), "try again", Bluetooth
on/off toggle, firmware updates over WiFi (OTA) with a progress screen on the display, battery and
USB voltage, SD card section. The 1.69" IPS display (ST7789, 280x240) shows a station logo,
station, artist and song (scrolling when too long, Title Case), a status line "BT good  buf NN s
AAC", IP + WiFi dBm and battery, updated live without flicker; 180-degree flip; brightness,
night mode and idle dimming. SD card support (log, measurements, settings backup, logos, firmware
copies) is built in but off (`SD_ENABLED 0`) until the ordered microSD breakout is wired. Calibrated battery measurement (~14 h on the 2500 mAh cell), power guard against brownout
loops. Station list up to 60 entries (NVS blob); about 25 public Dutch stations verified and in
use (see "Stations"). Next to active Bluetooth the radio gets ~9-16 KB/s: 96 kbps AAC is fine,
128 kbps MP3 is borderline, 192/320 kbps stutters. Stations that are too heavy or only offer
HE-AAC are re-encoded to AAC-LC 96 kbps by Liquidsoap relays on the user's NAS (Joe, NPO
Sterren.nl, RadioNL, Team FM; `nas/`) and played from the own Icecast.
Size/speed optimisation done 2026-10-09 in three tagged steps (`opt-1-fixes`, `opt-2-resampler`,
`opt-3-flash`; baseline `opt-0-baseline`): firmware 90.6% -> 81.8% of the OTA slot, min free heap
while playing ~52 KB (was 37-41). Verified on the radio: My Radio (AAC 44.1 kHz) steady, NPO
Radio 2 and Qmusic Foute Uur (MP3 48 kHz, fixed-point resampler) sound right, headphones reconnect
in ~1 s after an update.
Since 2026-10-09 19:52 the radio runs the self-built framework, phase 2c plus the live log (env `idf`, Arduino
2.0.17 as an ESP-IDF 4.4.7 component, prebuilt TCP window, rev 3, classic Bluetooth only, no BLE /
BLE Mesh / SPP / HFP, Bluedroid heap in PSRAM first; tag `opt-5-idf-phase2` + later commits): 70.3% flash (was
81.8% prebuilt) and ~84 KB free internal RAM while playing, min ~80 KB (was ~57 / 52), verified
5 min per step on My Radio. A larger TCP window (16/32 KB) stalled next to Bluetooth and was
dropped. Mind which env you
upload: `pio run -t upload` (env `radio`) puts the prebuilt build back; the `idf` build goes on
with `.\scripts\build_idf.ps1` plus the curl upload under Commands.
Display + rotary encoder parts (1.69" IPS ST7789, EC11) and a PCM5102A DAC + PAM8403 amp are on
order; see `docs/`.
Public repository: GitHub hghaken/ESP-iRadio (branch `master`, GPL-3.0). Host names such as
`icecast.example.com`, `my-nas` and 192.168.1.x stand for the owner's own servers.

Hardware:
- Olimex ESP32-DevKit-LiPo with ESP32-WROVER module (rev D), p/n 0000099739, GTIN 4260696351831.
  Chip ESP32-D0WD-V3 rev 3.1, 4 MB flash, 4 MB PSRAM.
- Li-Po battery 3.7 V 2500 mAh (JST-PH, LP785060), charged by the board's on-board charger.
- Display since 2026-10-10 evening: 1.69" IPS module ZJY169S0800TG01, ST7789V2, 240x280 (used
  as 280x240), 4-wire SPI, pins GND VCC SCL SDA RES DC CS BLK, no SD slot, rounded corners.
  The final display (user's decision 2026-10-10). Earlier, now removed from the code (see git
  history): the Arduino Robot LCD (1.77" 160x128 with a microSD slot, 2026-10-10 day; last
  commit with it 7dd3f69) and a WeAct 2.13" black/white/red e-paper (until 2026-10-10).
- Sony WH-1000XM6 headphones (Bluetooth A2DP sink; LE Audio off / "Classic audio").
- Development PC connection: USB-serial CH340 (COM7 since 2026-10-07, was COM6). Only needed for the
  serial log and as fallback; updates go over WiFi. Since 2026-10-09 a 10,000 mAh USB power bank
  usually occupies that connector; the same log is on the web page (section "Log").

## Structure
- `platformio.ini` - build config: `[common]`, env `radio` (default, OTA upload via curl) and env `usb`
  (serial upload on COM7). Partition table `partitions_ota.csv`. `extra_scripts` runs
  `scripts/patch_a2dp_reconnect.py` before every build.
- Experimental env `idf` (not on the radio yet, upload disabled): Arduino 2.0.17 built as an
  ESP-IDF 4.4.7 component with an own SDK configuration, mainly for a 32 KB TCP window. Files:
  `sdkconfig.defaults` (the prebuilt Arduino sdkconfig plus the phase-1 changes, header explains),
  `sdkconfig.idf` (resolved config, generated, committed for review), `CMakeLists.txt` and
  `src/CMakeLists.txt` (only read by `idf`), `scripts/build_idf.ps1` (builds it, see Commands).
  Plan, status and test plan: `docs/framework-build.md`.
- `scripts/patch_a2dp_reconnect.py` - reapplies five patches onto the git-ignored ESP32-A2DP
  copy (cancel a stuck page; do not page when auto-reconnect is off; publish the RSSI delta; start
  the media stream right after connecting instead of on the next 10 s heartbeat; diag log lines
  with the ACL status and disconnect reason; AVRCP target init + `radio_avrc_tg_cb` for the
  device's buttons, with an in-place upgrade of its first version).
- `partitions_ota.csv` - two OTA app slots of 1984 KB, nvs at the stock offset, coredump at the end.
- `src/config.h` - pins, default stations, buffer sizes, battery calibration.
- `src/main.cpp` - boot (PSRAM malloc limit, settings, power guard, display, Bluetooth, WiFi or setup
  AP, web server, autoplay), `loop()` (web, battery, Bluetooth paging window while the headphones are still down, mesh roam while they are connected and RSSI is under the threshold, WiFi retry every 20 s while down or "zombie": associated with an IP but 3+ stream connect failures in a row -> `WiFi.disconnect` + fresh association; a `[wifi] status ...` log line every 10 s).
- `src/bt_mesh_stubs.c` - empty BLE Mesh dispatch handlers, saves ~120 KB flash (see "Flash budget").
- `src/settings.*` - persistent settings in NVS (Preferences namespace `radio`); `settingsSaveVolume()`
  writes only the volume key (the slider must not rewrite everything). The station list
  (max `MAX_STATIONS` = 60) is a blob under key `stlist`: NVS strings are capped at 4000 bytes,
  which 20 StreamTheWorld lines already approach (the old string key `stations` is read once and
  removed on the next save).
- `src/icystream.*` - own minimal HTTP/ICY stream client (plain http, redirects, ICY title metadata).
  `player.cpp` turns every title into Title Case (`titleCase()`: all-caps and all-lower words get a
  capital first letter, mixed-case words such as "McCartney" are kept, non-ASCII bytes untouched).
  Replaces audio-tools' URLStream, which linked WiFiClientSecure/mbedTLS and switched WiFi modem
  sleep off. TLS would not fit in RAM anyway.
- `src/player.*` - audio pipeline: `netTask` (ICY stream -> 256 KB PSRAM buffer) and `decodeTask`
  (AAC/MP3 Helix decoder -> `PcmSink` -> 64 KB PSRAM PCM buffer); `PcmSink` resamples anything that is
  not 44.1 kHz (linear interpolation in Q16 fixed point: the ESP32 has no double FPU) and stops waiting
  for space when the link is down; the Bluetooth stack pulls PCM from that buffer in `btData()` and
  gets silence when it is empty. Software volume is applied in `btData()`. Station changes use a
  generation counter + handshake so buffers are only reset when the decoder is idle; with the link
  up, `btData()` (the only reader) drains `pcmBuf` itself via the `pcmFlush` flag instead of a reset
  from another core. `playerBtConnected()` is the cheap link check for hot paths (`loop()`, web
  handlers); `playerStatus()` copies the whole struct and calls Bluedroid. Also: forget
  headphones (NVS wipe + bonds, address kept), quiet-page request, clean shutdown, Bluetooth failure counter, 10 s `[stat]` line.
  Remembered Bluetooth devices (since 2026-10-10; headphones WH-1000XM6 and a JBL Charge 5
  speaker): up to 4 `{name[32], bda[6]}` records in NVS namespace `btdev` (blob `list` in
  priority order, int `sel` = last connected, informational). Only one A2DP sink at a time
  (Bluedroid source limit). The top device is handed to the library with
  `set_auto_reconnect(addr)` before `start_raw()`, so the boot pages it directly; manual mode
  (automatic off) always uses the top one (the library otherwise uses its own NVS copy, namespace
  `connected_bda`). A device is added and selected when it connects (`onBtState` sets a flag,
  `playerLoop()` in `loop()` writes NVS; the name comes from the discovery callback, else the
  existing entry, else `btTarget`; a new device goes on top in manual mode, last in automatic
  mode). Switching ("Use", `/api/btuse i=`: moves it to the top) and pairing a new one
  ("Pair new device", `/api/btpair name=`: selection -1, library address cleared, boot
  discovers by name) save and restart, reusing the proven boot path; "Forget" (`/api/btdel i=`)
  removes that bond and entry (restart only if it was connected or the list is now empty). "Erase all pairings" (`/api/btforget`) also empties the list. Existing installs: the headphones were
  recorded as device 0 on the first connect after the update. JBL Charge 5 verified the same
  evening: found by name, ACL `stat=256`, legacy PIN pairing accepted (SSP stays off), audio
  ~1 s after connect, link quality 0 dB (ideal) next to the radio, 2 bonds kept side by side.
  Automatic mode (`btdev`/`auto`, web checkbox, `/api/btauto on=`, order with `/api/btup i=`,
  default list order = priority; the user chose JBL Charge 5 first, WH-1000XM6 second): `applySelection()` pages the top
  device; `onBtState` DISCONNECTED after a failed page calls `autoNext()` (next address via
  `set_auto_reconnect(addr)`, picked up by the library's 10 s heartbeat), after a dropped link
  it pages the same device once more first (`linkWasUp`). Planned disconnects set `autoHold`
  (in `sourceDisconnect()`). The boot wait allows 12 s more per extra candidate and counts the
  page as done only after every candidate had one (`pageFailures`). Pairing a new device
  (`btdev`/`pair`) bypasses the list for that boot. One sink at a time: while the speaker plays,
  turning the headphones on does not switch. Seen 2026-10-10: one `authentication failed,
  status:9` from the JBL on the first reconnect (bond deleted, re-paired by itself on a later
  page); after an OTA restart the JBL answered `stat=267` (0x0B, connection already exists),
  then 260 / 287 for ~2 min - it keeps paging the radio itself after a link loss while the ESP32
  is non-connectable (library default) - and connected after ~145 s. With the AVRCP target in
  place (next item) the JBL came back in ~3 s after later updates.
  Buttons on the Bluetooth device (AVRCP target, fifth library patch + `radio_avrc_tg_cb()`):
  play/pause once = mute on/off (`playerSetMuted`, status "Muted (play button)" on the display,
  "muted" on the web page; moving the web volume or changing station unmutes), twice = next
  station, three times = previous station (`stepStation`, saves `lastStation`). Play presses are
  counted in the Bluetooth task and acted on in `playerLoop()` after 0.6 s. The JBL Charge 5 sends
  PLAY (0x44) for one press and FORWARD (0x4B) for two; three presses arrive as FORWARD + PLAY,
  so a FORWARD waits 0.9 s (`TRIPLE_MS`) and FORWARD + PLAY counts as previous. BACKWARD (0x4C,
  e.g. a swipe on the Sony) is previous at once. Every button press also calls `displayWake()` (a dimmed screen comes back). Volume keys stay on the speaker. Pitfall:
  `esp_avrc_tg_init()` is asynchronous, so the patch's `esp_avrc_tg_get_psth_cmd_filter()` right
  after it fails with 259 (ESP_ERR_INVALID_STATE) and leaves the supported command set empty
  (every button refused); the filter is set in the target CONNECTION_STATE event instead
  (`[avrc] button filter: ... cmds 0000 0000 1fff 0078`). All target events log as `[avrc]`.
- `src/battery.*` - battery / USB sensing on GPIO35 / GPIO39 (see "Battery measurement").
- `src/power.h` - `powerBrownouts()` for the diagnostics (guard itself lives in `main.cpp`).
- `src/display.*` - status screen on the 1.69" ST7789 IPS (`Adafruit_ST7789::init(240, 280)`:
  generic init with INVON - IPS panels need it - and the 20-row offset; rotation 1, or 3 when
  flipped; pixel data at 26.7 MHz), own task. Layout constants for 280x240 at the top of the
  file; wide side margins because the glass has rounded corners. The whole frame is drawn into
  a `GFXcanvas16` (131 KB, PSRAM) and pushed in one go when anything changed (checked every
  250 ms): no flicker. Top to bottom: dark red header bar "ESP iRadio" + Bluetooth state
  (`BT: on / searching / off`, FreeSans Bold 9 pt, white), station (FreeSans Bold 12 pt, white),
  artist (FreeSans 12 pt, yellow), song (FreeSans 12 pt, cyan), each a `ScrollLine` strip: text
  wider than its area scrolls (2 s pause, ~25 px/s to the end, 2 s pause, back) and only that
  strip (~18 KB) is pushed every 40 ms; scrolling can be switched off (NVS `scroll`,
  `/api/scroll`). A 64x64 station logo from the SD card sits left of station and artist (those
  strips narrow; GFX has no clipping, so `drawLine()` clears the logo column and redraws the
  logo after scrolled text). Then status "BT good  buf NN s  AAC" (green, red on error; buffer
  seconds in 5 s steps so the frame is not re-sent every second), IP + WiFi dBm and battery
  (white, FreeSans 9 pt). Only full-intensity colours on black (mid greys faded on the earlier
  TN panel). Backlight on GPIO26 via LEDC PWM (10 bit, 5 kHz, duty = percent squared for
  perceived brightness, fades ~1 s, `backlightTick()` every 40 ms): the lowest that applies of
  the normal level (`bl`, 5-100 %), night mode (`ngt`, window `ngtf`-`ngtt` in minutes, may
  cross midnight, level `ngtl`, 0 = off; needs the SNTP clock) and idle dimming (`idlm`
  minutes without a new title/station/Bluetooth/error state, level `idll`, 0 = off). Buffer
  seconds, WiFi signal and battery do not count as activity; `/api/backlight` (fields b, night,
  from=HH:MM, to, nl, idle, il; any subset) and `displayWake()` restart the idle timer.
  `/api/status` `backlight.now` is the level right now. Web: System -> Brightness,
  Night mode, Dim after. Tested 2026-10-10: 30 %, night window 5 %, idle 1 min to off.
  Update screen: `displaySetUpdate(phase, bytes, total, source)` replaces the normal screen
  while firmware is written (OTA upload in `web.cpp`: total from `clientContentLength()`; SD
  restore in `sdcard.cpp`): "Updating firmware", source, progress bar, kB and percent, then
  "Klaar - de radio herstart" or "Mislukt" (red, back to normal after 10 s); it wakes a dimmed
  screen. Seen working on the radio for an OTA upload (2026-10-10). GPIO4 (SD chip select) is held high in `displayBegin()`.
- `src/sdcard.*` - microSD card (CS GPIO4, shares VSPI with the display; `SD_ENABLED 0` in
  `config.h` until the planned separate breakout is wired; until 2026-10-10 the card sat in the
  Robot LCD's slot, where everything below was verified; Arduino's
  SPI transactions take the bus lock, so the display task and the card take turns). Mounted at
  4 MHz in `setup()` right after the display; everything else runs in `sdLoop()` from `loop()`
  in small steps (no own task: internal RAM). Without a card all of it is skipped. On the card:
  - `/log/YYYY-MM-DD.log`: the web log ring (`weblogRead`) appended every 30 s with a
    `--- HH:MM:SS (uptime N s) ---` marker and a `===== opgestart =====` line per boot; waits up
    to 3 min for the clock, else `no-date.log`; 90 files kept.
  - `/stats/YYYY-MM-DD.csv`: one line per minute (state, station, input, buffer, underruns,
    reconnects, Bluetooth rate/link, WiFi RSSI/BSSID/channel, heap, battery, USB); 365 kept. The
    web page draws a chart of one column for a chosen day (canvas, no library).
  - `/backup/settings.txt` (key=value, no WiFi password; `wifi_ssid` is informational) and
    `/backup/stations.txt` (`Name|URL`): written 10 s after every `settingsSave()` /
    `settingsSaveVolume()` and at boot when missing. "Restore from SD" (`/api/sdrestore`) applies
    both (WiFi untouched) and restarts.
  - `/logos/<fnv1a(name)>.565`: 40x40 RGB565 little-endian (3200 bytes), keyed by FNV-1a of the
    station name (the page computes the same hash). The browser scales the chosen image on a
    canvas and uploads raw pixels (`/api/logo?i=N`, multipart), so the radio decodes no
    JPG/PNG. `sdLoop()` loads the current station's logo into a PSRAM buffer (mutex);
    `display.cpp` copies it and draws it at x 3, y 19, with station and artist strips narrowed to
    the right of it (song strip full width). GFX has no clipping, so `drawLine()` clears the
    logo column and redraws the logo after drawing scrolled text.
  - `/firmware/<build>.bin`: copy of the running image (`ESP.getSketchSize()` bytes,
    `esp_partition_read` into a 4 KB internal buffer, 4 KB per `sdLoop()` pass, ~6 s) made
    automatically after 10 min uptime or with "Keep current firmware"; newest 5 kept;
    verified byte-identical to the built `firmware.bin` on 2026-10-10. "Restore"
    (`/api/fwrestore f=`) stops audio, writes it to the other OTA slot with `Update` and
    restarts.
  Time: `connectWifi()` starts SNTP once (`configTzTime`, CET/CEST, nl.pool.ntp.org /
  pool.ntp.org / fritz.box). Dates are formatted with `snprintf`, not `strftime`: the prebuilt
  framework (env `radio`) links newlib's strftime into IRAM and the link overflowed IRAM by
  1.2 KB. Free space is never asked for: `f_getfree` can scan the whole FAT (tens of seconds on
  128 GB) after a power cut. FAT16/FAT32 only: ESP-IDF 4.4's FatFs has no exFAT, so a 64 GB+
  card as sold logs `f_mount failed: (13) There is no valid FAT volume`. "Format SD card" (`/api/sdformat`) first tries a plain mount and only then the library's
  `format_if_empty` (`f_mkfs` runs only on that error 13): a card with FAT is never wiped, boot
  never formats. `diag.sdFormat` / `diag.sdFirmware` hold "busy" or the outcome; the web server
  does not answer during `f_mkfs`, so the page counts seconds itself and polls afterwards.
  Downloads (`/api/sdfile?p=`, only below those five folders, no `..`) block `loop()` while
  they stream and are slow next to Bluetooth. Web section "SD card". The card in the radio
  since 2026-10-10: 128 GB SDXC, formatted FAT32 by the radio (43 s, 123.7 GB). Costs: env
  `idf` 74.8% flash with SD enabled (72.8% with `SD_ENABLED 0`), env `radio` 86.1%.
- `src/version.*` - build stamp (`buildStamp`) from the generated, git-ignored `include/build_stamp.h`
  that the pre-build script writes every build (PlatformIO rebuilds on content, not timestamps; a
  `__DATE__` inside web.cpp only changed when web.cpp recompiled).
- `src/web.*` - web page + JSON API (`/api/status` incl. a `diag` object, `/api/play`, `/api/stop`,
  `/api/volume`, `/api/stations`, `/api/bt`, `/api/btretry` (WiFi off, page the stored headphones, nothing wiped), `/api/btforget` (wipe all local bonds and the device list, address kept, restart), `/api/btuse` / `/api/btpair` / `/api/btdel` / `/api/btauto` / `/api/btup` (device list, see `src/player.*`), `/api/btenable` (on=1/0: Bluetooth toggle, NVS `bten`; off = auto-reconnect off, source link dropped, no paging windows, boot wait skipped, stack not even started when off at boot; header shows BT: uit / zoekt / aan), `/api/btidentity` (experiments),
  `/api/flip` (on=1/0: screen rotated 180 degrees, NVS `flip`), `/api/scroll` (on=1/0: scroll long lines, NVS `scroll`, default on), `/api/backlight` (see `src/display.*`), `/api/log` (live log, see `src/weblog.*`), `/api/wifi`, `/api/roam` (mesh RSSI threshold, 0 = off, no restart), `/api/autoscan` (scan WiFi networks when the page opens; default off, no restart; setup mode always scans), `/api/scan` (every access point, also each mesh point, with channel and BSSID; `same`/`cur` mark points of the current network; a scan while associated often misses the own point, so it is added from the link), `/api/roamto` (bssid=, ch=: hop to that point of the current network, done in `loop()`), `/api/sd` (card listing JSON), `/api/sdfile` (download), `/api/logo` / `/api/logodel`, `/api/sdbackup` / `/api/sdrestore`, `/api/fwsave` / `/api/fwrestore`, `/api/sdformat` (see `src/sdcard.*`), `/api/update` (OTA), `/api/restart`); captive portal in setup mode;
  "Diagnostics" section with live rates, buffers, decoder, WiFi AP, memory, battery pins, reset reason, Bluetooth link quality and bond count on the radio (`btBonds`),
  running OTA slot and build time; "Firmware update" upload; OTA stall guard task.
- `src/memlog.h` - `logMem()` helper printing internal/PSRAM heap.
- `src/weblog.*` - live log for the web page: a 64 KB ring buffer in PSRAM (`WEBLOG_BYTES`) that
  always runs, fed by three sources: own messages (`Log.printf/println/print`, a Print tee to
  Serial; never use `Serial.print*` directly for log lines), Arduino/library `log_x` and the
  Bluetooth "(diag)" lines (they end in `ets_printf`, captured on ROM output channel 2 with
  `esp_rom_install_channel_putc(2, ...)`; the hook is IRAM and skips characters while the flash
  cache is off), and ESP-IDF's own `ESP_LOGx` (`esp_log_set_vprintf`). `/api/log?since=N&max=M`
  returns text plus headers `X-Log-Next`, `X-Log-More`, `X-Log-Skipped`, `X-Log-Restart`. Web
  page: "Log" section with a "Show live log" checkbox (polls every 1.5 s only while on, choice
  kept in the browser's localStorage), "Clear window" and "Download log". Costs ~4 KB flash,
  64 KB PSRAM, no internal RAM. Lost on restart.
- `docs/` - reference drawings (battery jumper locations), `display-research.md` (display + rotary
  encoder research; the user ordered a 1.69" IPS ST7789 and an EC11) and `speaker-output.md`
  (line + speaker output plan: PCM5102A I2S DAC on GPIO27/14/13 (SCK grounded, XSMT high) feeding
  RCA/3.5 mm and a PAM8403 amp on BAT, amp shutdown on GPIO19; output follows the Bluetooth toggle;
  parts ordered 2026-10-09), `framework-build.md` (self-built framework: plan, trials, results) and
  `icecast-burst-per-mount.xml` (proposed config for the user's own Icecast server, passwords as
  placeholders).
- `nas/joe-relay/` - Liquidsoap relay on the user's Synology NAS (DS1522+, DSM 7.4, SSH host
  `my-nas`, Docker via `sudo /usr/local/bin/docker`): takes Joe's HE-AAC stream, decodes it in full
  and re-encodes AAC-LC 96 kbps 44.1 kHz stereo to the own Icecast as hidden mount `/joe.aac`
  (titles passed on; Joe's "Song - ARTIST" is split on the last " - " and sent as
  "ARTIST - Song", the order the radio's display expects). Runs since 2026-10-09 as container `joe-relay` from
  `/volume1/docker/joe-relay` (image `savonet/liquidsoap:v2.4.6`, host network, restart
  unless-stopped). `.env` there holds the Icecast source password (created on the NAS from
  `icecast.xml`, mode 600, never in git); `.env.example` is the template. Icecast runs as
  container `IceCast-4th-stream` with `/volume1/docker/icecast/icecast.xml` mounted at
  `/etc/icecast.xml`; after editing it, reload with `sudo docker kill -s HUP IceCast-4th-stream`
  (no restart, listeners stay). README.md in that folder: setup and how to add more stations.
  Restarting a relay container drops its listeners for ~7 s (the radio then reconnects and may
  underrun once): do it when nobody listens to that mount. Edit `icecast.xml` in place (`cat new >
  icecast.xml`, backup in `/volume1/docker/icecast/backup/`): it is a single-file bind mount, a
  replaced file (new inode) would not reach the container.
- `nas/sterrennl-relay/` - same for NPO Sterren.nl since 2026-10-09: source MP3 192 kbps 48 kHz
  (`http://icecast.omroep.nl/radio2-sterrennl-mp3`), hidden mount `/sterrennl.aac`, container
  `sterrennl-relay` from `/volume1/docker/sterrennl-relay`. Titles already "Artist - Song", no
  swap.
- `nas/radionl-relay/` - same for RadioNL since 2026-10-09: its only stream is MP3 192 kbps
  44.1 kHz (`http://stream.radionl.fm/radionl`), hidden mount `/radionl.aac`, container
  `radionl-relay`. Titles "ARTIST - SONG" (upper case, the radio's Title Case fixes that), no
  swap. RadioNL inserts a ~10 s pre-roll ad on every connect; via the relay the radio hears it
  only when the NAS reconnects.
- `nas/teamfm-relay/` - same for Team FM since 2026-10-09: source the StreamTheWorld http
  entrance `.../livestream-redirect/TEAMFM.mp3` (MP3 192 kbps 44.1 kHz; the audiostreamen.nl links
  go to https or to the Drenthe-Groningen regional stream), hidden mount `/teamfm.aac`, container
  `teamfm-relay`. Titles already "Artist - Song", no swap.
- `AGENTS.md` (this file), `README.md` (short GitHub intro), `CLAUDE.md` (Claude Code workflow only,
  includes this file), `.claude/`.

## Technology
- Arduino (C++) with PlatformIO, NOT ESPHome: ESPHome cannot act as a Bluetooth A2DP source
  (decided with the user 2026-10-06, deviation from the brief).
- Platform `espressif32@6.12.0` = Arduino core 2.0.17 / ESP-IDF 4.4. Libraries pinned in
  `platformio.ini`: ESP32-A2DP v1.8.3, arduino-audio-tools v1.0.1 (library.json says 0.9.9),
  arduino-libhelix (git head), Adafruit GFX 1.12 + Adafruit ST7735 and ST7789 Library 1.11 (GxEPD2
  until 2026-10-10). Newer ESP32-A2DP/audio-tools need
  Arduino 3.x; see "Pitfalls" for why that was abandoned.
- Firmware (2026-10-10 late evening: ST7789 display with backlight control and update screen,
  Bluetooth device list with automatic mode, SD code compiled in but `SD_ENABLED 0`): env `radio`
  (prebuilt libraries) 85.0% of the 1984 KB OTA slot; env `idf` phase 2c (self-built, on the
  radio) 73.8%. With `SD_ENABLED 1` the SD code links in fully (~+45 KB more). See "Flash
  budget" under Pitfalls and `docs/framework-build.md`.
- Control: web page only; no buttons, no Home Assistant integration.
- Tools on the dev PC: PlatformIO 6.1.19 (`pio`), Python 3.13, esptool 5.3.1, git, gh (GitHub CLI).

## Commands
Run from the project root:
- Build: `pio run`
- Update the `idf` build (upload is disabled in that env on purpose): build with
  `.\scripts\build_idf.ps1`, Bluetooth off via `/api/btenable on=0`, then
  `curl -F "firmware=@C:/pio/iradio-idf/.pio/build/idf/firmware.bin" http://radio.local/api/update`,
  wait for the new `diag.build`, Bluetooth back on.
- Update over WiFi (default env `radio`): `pio run -t upload` posts `firmware.bin` to
  `http://radio.local/api/update` with curl. The radio disconnects Bluetooth, stops playback, writes the
  inactive OTA slot and restarts cleanly; the headphones reconnect during the boot wait. The web page has
  the same function (System -> "Firmware update", choose `.pio/build/radio/firmware.bin`).
  Do this with a decent WiFi signal; a broken upload is caught by a 30 s stall guard (restart).
  `/api/status` `diag.partition` and `diag.build` show which build runs; slots alternate app0/app1.
  Since 2026-10-10 (build 21:38 and later) an upload works with Bluetooth on: at upload start
  `playerHoldBluetooth(true)` turns auto-reconnect off and `loop()` opens no paging window while
  `webUploadActive()` (before, a window took WiFi off mid-upload: stuck at ~23 %, curl error 28);
  a failed upload resumes paging. Older procedure, still safe: first `curl -X POST
  http://radio.local/api/btenable -d on=0`,
  upload, wait until `/api/status` shows the new build, then `on=1` (that opens a paging window at
  once, so the API is silent for up to 30 s; the headphones reconnect in ~1 s and audio follows).
  A curl timeout on an otherwise healthy radio: just run the upload again.
- Flash over USB (ask the user first; every USB flash is a hard reset): `pio run -e usb -t upload`
  (COM7). Only for the first install of the OTA partition table or when OTA is broken.
- Live log without USB: web page, section "Log", "Show live log", or
  `curl "http://radio.local/api/log?since=0&max=65536"` (everything kept since boot, up to 64 KB).
  Since 2026-10-09 a USB power bank sits in the board's USB connector, so COM7 is only there
  when the PC cable is plugged back in.
- Serial monitor: `pio device monitor` (COM7, 115200). Only one program can hold the port. For a
  log file while working: start it hidden (`Start-Process pio -ArgumentList "device monitor -p COM7
  -b 115200 --quiet" -RedirectStandardOutput <file> -WindowStyle Hidden`); it survives OTA restarts.
  If the port is listed but opening it fails ("cannot find the file specified"), the CH340 has
  dropped off: replug the USB cable.
- Chip/flash info (read-only): `python -m esptool --port COM7 flash-id`
- Live status: `curl http://radio.local/api/status` (or the IP shown on the display).
- Experimental framework build: `.\scripts\build_idf.ps1` (PowerShell). ESP-IDF 4.4 cannot build
  in a path with a space, so it mirrors the project to `C:\pio\iradio-idf` (robocopy, the mirror
  keeps its own `.pio`) and runs `pio run -e idf` there; the first build compiles all of ESP-IDF
  (~5 min), later ones ~3 min. Output `C:\pio\iradio-idf\.pio\build\idf\firmware.bin`.
- Backups / rollback: before risky steps, tag the commit (`opt-0-baseline` .. `opt-3-flash` exist)
  and copy `.pio/build/radio/firmware.bin` to the git-ignored `backup/firmware/<n>-<name>_<sha>.bin`.
  Roll back without building: Bluetooth off via `/api/btenable on=0`, then
  `curl -F "firmware=@backup/firmware/<file>.bin" http://radio.local/api/update` (tested 2026-10-09).
Toolchains and frameworks live in `C:/pio` (see `core_dir` in `platformio.ini`).

## Using the radio
- First boot (no WiFi configured): join the open access point `ESP-iRadio-Setup`, browse to
  192.168.4.1 (captive portal), pick the network and save -> it restarts. Bluetooth stays off in
  setup mode so the access point works.
- Normal use: `http://radio.local/` (or the IP shown on the display). One "Nu" card with now
  playing, play/stop, volume and the station list (`Name|URL` per line; saving it stops the stream, finds the current station again in the
  new list by URL, else by name, and resumes it; the last station and play/stop state are stored
  in NVS and restored at boot), Bluetooth name, WiFi, mesh roam threshold (dBm, 0 = off), "Search networks when the page opens" (default off), diagnostics, live log ("Show live log", "Download log"), firmware update, restart.
- Bluetooth devices (web section "Bluetooth devices"): one plays at a time. New device: put it
  in pairing mode (WH-1000XM6: hold power ~7 s; JBL Charge 5: press the Bluetooth button until
  the light flashes fast), type its name prefix (e.g. `JBL Charge 5`) and "Pair new device": the radio restarts, connects to the first discovered device whose name starts with
  it and remembers it. Switch with "Use" (restart, ~30 s). Afterwards it reconnects by itself. At boot WiFi stays off until the first
  inquiry (12.8 s) and the first page have finished (connected, page ended, or about 36 s at
  most). Later, while the headphones are still down, `loop()` turns WiFi off for up to 30 s
  (a failed inquiry counts; it does not wait for 3 disconnects), at most every 45 s, or at
  once via "Try again". That button wipes nothing. The headphones
  accept two devices at a time: switch Bluetooth off on phone/tablet if they never connect.
  "Erase all pairings" wipes the stored address and bonds on the radio only and does not change
  the Bluetooth address. Remove "ESP iRadio" in the Sony app first, then put the headphones in
  pairing mode; otherwise they keep the old link key and refuse the new attempt.
- Battery: shown bottom right on the display and under Diagnostics. Empty cell -> "Battery empty - charge it"
  and deep sleep; charging an empty cell on USB -> "Charging battery, please wait" until 3.55 V. Use a real
  charger (>= 1 A) for an empty battery, not a PC port.

## Conventions
- AGENTS.md and code comments in English; communication with the user in Dutch.
- WiFi credentials are entered via the web page and stored in NVS; they are never hard-coded, printed,
  returned by the API, or committed. Any future compile-time secrets go in git-ignored `secrets.h`.
- Commit messages: short, imperative. Push to `origin master` after each working step.
- Source files use CRLF line endings (git warns "LF will be replaced by CRLF"). Scripted edits that
  match text across line breaks must normalize CRLF to LF first and write back with the original
  endings. Write such edit scripts to a file (not a shell heredoc): heredocs in some agent shells
  turn `\n` in the script into a real line break, which then lands inside C string literals.
  Note that the GFX fonts are ASCII only: `toAscii()` in `display.cpp` strips accents before drawing.

## Pitfalls (learned the hard way, 2026-10-06 .. 10-10)
- Ask the user before: flashing over USB, erasing flash, changing partition tables, or anything that may
  wipe stored settings (NVS). OTA updates: after every code change build and upload at once, without
  asking (user's standing instruction, 2026-10-09; an update stops playback for ~1 min and restarts).
- **Internal RAM is the scarce resource.** The ESP32 has ~180 KB internal heap after static data;
  Bluedroid needs ~70-90 KB and WiFi/lwIP ~45-60 KB of it. `main.cpp` calls
  `heap_caps_malloc_extmem_enable(256)` first thing in `setup()` so every `malloc()` above 256 bytes
  prefers PSRAM (the prebuilt core's limit is 4 KB). Without it the A2DP connection runs out of memory.
  After boot + WiFi + Bluetooth + streaming ~52-65 KB internal is free with the prebuilt build
  (env `radio`) and ~84 KB (min ~80) with the self-built phase-2c build (env `idf`: classic
  Bluetooth only, Bluedroid heap in PSRAM first); watch `[mem]` lines and
  `/api/status` (`heap`, `minHeap`) when adding features. Keep own buffers in PSRAM (audio-tools'
  default allocator and libhelix already do).
- **Flash budget (OTA).** Each OTA slot is 1984 KB (app partitions must be 64 KB aligned, so the last
  64 KB of the 4 MB cannot join a slot). Since 2026-10-09 the firmware is ~1.66 MB (81.8%) thanks to
  build flags and a link trick, measured one by one: `-fno-exceptions` (unflag `-fexceptions`) -47 KB,
  no `-mfix-esp32-psram-cache-issue` (rev 0/1 silicon only; this chip is rev 3.1) -7.6 KB, unflag
  `-fstack-protector` -6 KB, and `src/bt_mesh_stubs.c` -120 KB: the prebuilt framework has
  `CONFIG_BLE_MESH=y`, so Bluedroid's BTC dispatch table drags in the whole Mesh stack through 11
  `btc_ble_mesh_*_handler` functions; empty definitions in our own object stop that (Mesh is never
  started). A "multiple definition" link error there means a library object is needed after all:
  delete the file. Further options measured but not applied: ROM newlib-nano printf -85 KB (breaks
  `%f`/`%lld`), LTO -19 KB extra, `CORE_DEBUG_LEVEL` 0 -37 KB (loses all logs), HE-AAC/SBR out of
  libhelix ~32 KB (needs a patch step). Removing BLE/GATT/HFP or WPA3 needs Arduino as an IDF
  component with an own sdkconfig. Before that, two things kept the firmware at ~1.83 MB: (1) `#undef
  HAS_IOSTRAM` right after `#include "AudioConfig.h"` in `player.cpp` - audio-tools otherwise includes
  `<iostream>` (Collections/BitVector.h) and the C++ locale machinery costs ~200 KB; include specific
  audio-tools headers, not the umbrella `AudioTools.h`. (2) No TLS: the own `IcyStream` instead of
  audio-tools' URLStream (WiFiClientSecure/mbedTLS, ~160 KB). Check the size line of every build.
  Bluedroid alone is ~550 KB prebuilt (includes BLE/GATT, BLE Mesh, HFP, SPP); the self-built env
  `idf` drops those (~240 KB less, see `docs/framework-build.md`). libhelix ~105 KB.
- **WiFi and Bluetooth share one 2.4 GHz radio - this drives most of the design:**
  - Bluetooth *paging* (connecting to the headphones) mostly times out while WiFi is associated,
    even an idle WiFi: `ACL stat=260` (HCI 0x04 page timeout; success is 256) in the diag line, the
    headphones never answer although inquiry finds them. It looked like the headphones refusing
    (two factory resets were done for nothing). Solution: give Bluetooth the radio to itself.
    Boot: Bluetooth starts first. `setup()` keeps WiFi off until the first inquiry (12.8 s) and
    the first page have finished, then waits until audio is pulled (+1 s, max 15 s) if the link is up. Cap about 36 s so missing
    headphones do not hold WiFi off. Later, while still not connected (a failed inquiry counts,
    not only 3 `DISCONNECTED` events), or when the web page asks, `loop()` opens a paging window
    (WiFi off for up to 30 s, automatic opens at most every 45 s). A connect attempt still open
    after 20 s is cancelled with `esp_a2d_source_disconnect` before the next page. The library
    heartbeat does not page while auto-reconnect is off. "Erase all pairings" is separate:
    it deletes only the local bond.
  - A WiFi scan burst right after a fresh Bluetooth link drops that link. So WiFi (re)connects
    straight to the access point remembered in NVS (`bssid`/`chan`, `connectWifi(true)`).
    The all-channel fallback is skipped while the headphones are not connected. After three
    such misses one scan is allowed, so a moved access point does not leave the radio
    unreachable. No remembered AP yet: the scan still runs, it is the only way on.
    After a paging window that ends in a link, WiFi returns once audio is pulled (+1 s, max 15 s). `/api/scan` (the "Search" button) is
    refused while Bluetooth is on and the headphones are not connected yet; with the Bluetooth
    toggle off (e.g. right after an OTA update) and in setup mode it scans. The page used to
    scan by itself 1.5 s after load, which crackles audio. That now follows `wifiAutoScan`
    (web checkbox, default off, NVS `autoscan`, `/api/autoscan`, no restart). Setup mode
    still scans on open. The manual Search button is unchanged.
  - Bluetooth must start before WiFi. With WiFi first (tried) the stream starved to 1-6 KB/s as soon
    as A2DP audio ran. With Bluetooth first it holds 11-14 KB/s next to audio.
  - Keep WiFi modem sleep on (`WIFI_PS_MIN_MODEM`): the driver aborts when it is off while Bluetooth
    runs ("Should enable WiFi modem sleep..."). `WIFI_PS_MAX_MODEM` made a -72 dBm link unusable.
    Max BR/EDR TX power (`esp_bredr_tx_power_set(P9, P9)`), coexistence left at BALANCE;
    `esp_coex_preference_set(ESP_COEX_PREFER_BT)` before start-up prevented any connection.
  - Throughput next to A2DP audio is ~30 KB/s at best: the 96 kbps AAC stream (12 KB/s) is fine,
    320 kbps MP3 (40 KB/s) is not. OTA uploads fail with audio active - the update handler
    disconnects Bluetooth first.
  - Mesh WiFi: the SSID is served by several access points. `WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN)`
    + `setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL)` pick the strongest when a scan does run.
    While the headphones are connected and RSSI has stayed under `settings.wifiRoamDbm`
    (web page, default -65, 0 = off, stored as NVS `roam`) for two 30 s checks, `loop()`
    scans that SSID once. It switches only when another BSSID is at least 12 dB stronger,
    then remembers that AP. Nothing better: next scan after 5 min. After a hop: 2 min.
    Settle check (2026-10-10): because reconnects go straight to the remembered AP, the radio
    stayed on a point at -58..-66 dBm (never two checks under -65) while another point gave
    -46. Now, a minute after every successful `connectWifi()`, once the headphones are up
    (or Bluetooth is off), one roam scan runs (at most every 30 min, `[wifi] settle check`).
    The web scan list shows each mesh point; "Switch to this access point" hops by hand.
    The scan crackles the audio for a few seconds (seen 2026-10-07); it does not run while
    the headphones are still connecting. A dropped association reconnects to the remembered
    access point. Bad placement (next to the PC, Li-Po over the antenna) costs 15-20 dB.
- **Stale `is_connected()` (2026-10-08):** audio was flowing at 176 KB/s while the library's
  `is_connected()` stayed false, so `loop()` kept opening paging windows every 45 s (web page
  unreachable, stream stalls, display "Waiting for headphones"). `btLinkUp()` in `player.cpp` now also
  counts a Bluetooth audio pull in the last 2 s as connected; everything uses that. Root cause not
  found (USB was not attached).
- **Zombie WiFi (2026-10-09):** after a paging window WiFi came back "connected" with an IP, but every
  socket got `errno 118 Host is unreachable`, the web page was dead and the old retry never fired
  (status was WL_CONNECTED). `loop()` now counts stream connect failures (`playerStreamFailures()`,
  counted only while `WiFi.status()` is connected, reset on a successful stream connect and on every
  `connectWifi()`; failures inside a paging window must not count or the detector fires after every
  window) and after 3 in a row does `WiFi.disconnect(true,false)` + reconnect to the remembered AP;
  log `[wifi] associated but unreachable, reconnecting`. Seen working the same
  day (stream back within 5 s). The reconnect logs `timeout when WiFi un-init` and `ESPmDNS addService
  failed` (service already registered); `radio.local` still resolves.
- **Fresh link drops when WiFi returns (2026-10-09, fixed):** three times in a row the link came up
  (`Connected`), WiFi came back 3-4 s later, and ~10 s after the connect the link dropped with
  `disc_rsn=1` (link loss) before any audio had been pulled. Cause: upstream ESP32-A2DP only starts
  the media stream on its next heartbeat tick (every 10 s), and a fresh ACL link without traffic
  does not survive WiFi's connect burst. Two fixes: the third patch in
  `scripts/patch_a2dp_reconnect.py` sends `ESP_A2D_MEDIA_CTRL_CHECK_SRC_RDY` right in the connected
  transition (audio now runs ~1 s after connect, log `[bt] audio running after N ms`), and
  `waitForBtAudio()` in `main.cpp` keeps WiFi off (boot and paging window) until the stack pulls
  audio, plus 1 s, at most 15 s.
- **`authentication failed, status:9` (2026-10-09):** the headphones answered the page (ACL ok) but
  rejected authentication; Bluedroid then deletes the local bond (`btBonds` went 1 -> 0) and every
  further attempt fails the same way until the headphones are put in pairing mode (then the fresh
  pairing went through at once). Seen after the fresh-link drops above; most likely the headphones
  dropped a bond whose link was lost before the first audio. Remedy: pairing mode, not a reset.
- **Paging window side effects:** `WiFi.mode(WIFI_OFF)` logged `timeout when WiFi un-init, type=4`
  and the page inside that window still timed out; the stream meanwhile logs "Host is unreachable"
  retries. A clean restart (`/api/restart`) connected the headphones in 1.2 s. Open: stop the stream
  before the window and try `esp_wifi_stop()`/`esp_wifi_start()` instead of a full un-init. That
  restart came back with reset reason 5 (interrupt watchdog) instead of 3 - also open.
- **Bluetooth link quality:** `esp_bt_gap_read_rssi_delta()` every 30 s (player status); the answer
  arrives in the library's GAP callback, published through `g_bt_rssi_delta` by the third patch in
  `scripts/patch_a2dp_reconnect.py`. Value = dB relative to the golden receive range (0 = ideal,
  negative = weak); shown as good (>= -5) / fair (>= -15) / weak on the screen and under Diagnostics.
- **Headphone quirks (WH-1000XM6):** two devices at a time (switch phone/tablet Bluetooth off when in
  doubt); pairing mode lasts 5 min; after a hard reset of the radio the first reconnect attempts fail
  for the paging reason above, not because of the headphones. LE Audio must stay off ("Classic").
  A one-sided bond delete (radio forgets, Sony still has the link key for the same address) makes
  the headphones reject every attempt until the device is removed in the Sony app or factory-reset.
  Do not bump `btIdentity` to "fix" that: each address occupies one of the two slots.
  SSP stays off (the library's PIN path). Do not enable it, and do not use its `ESP_BT_IO_CAP_IO`.
  `authentication failed, status:9` right after an ACL connect is the headphones rejecting the
  bond (see above), not an IO-capability problem. A page timeout (`stat=260`) is not an auth
  failure. Do not set `ESP_COEX_PREFER_BT`.
  Planned shutdown and "Erase all pairings" call `esp_a2d_source_disconnect` and wait
  until the link is down (about 3 s) before restart or `remove_bond_device`.
  `BluetoothA2DPCommon::disconnect()` calls `esp_a2d_sink_disconnect` and is not used.
- **Stream buffering.** Icecast bursts ~64 KB on connect, so playback starts at once with ~5 s cushion.
  After an underrun the pre-buffer threshold doubles (up to 192 KB of the 256 KB buffer); a station
  change resets it to 64 KB. IcyStream drops the connection when a metadata block cannot be read
  completely (`lost()`, log `[icy] metadata block incomplete`), and `netTask` reconnects at once:
  otherwise audio bytes were read as metadata lengths until the next reconnect. A stall
  (no data for 5 s) reconnects *without* resetting the decoder: it keeps playing what is buffered and
  resyncs on the new frames. A full buffer (headphones off) is not a stall. The `[stat]` line every
  10 s shows input rate, Bluetooth pull rate (176 KB/s = real time), silence padding, buffer level
  and heap - the first thing to read when something sounds wrong (also under Diagnostics on the page).
- **The Bluetooth data callback must never block**: `pcmBuf` read wait is 0 and it pads with silence.
- **Timers:** compare `millis()` deadlines wrap-safe, `(int32_t)(millis() - deadline) >= 0`; a plain
  `millis() >= deadline` stalls for ~49 days after the 49.7-day wrap (fixed 2026-10-09 in the
  stream retry, the WiFi retry and the restart timer).
- **Brownouts / empty battery.** After ~14 h on battery the radio sat in a brownout reboot loop (every
  boot died when Bluetooth started, every 10 s, flashing the e-paper), also on a PC USB port (500 mA
  cannot charge an empty cell *and* feed the radios). `powerGuard()` in `main.cpp` runs before the
  radios: on USB with the cell < 3.5 V it charges first (display message, re-checked every 30 s, until
  3.55 V); after a brownout reset it waits 1-3 min; on battery only with the cell < 3.45 V it shows
  "Battery empty - charge it" and deep-sleeps 15 min. `[power]` log line at boot shows reset reason.
- audio-tools objects allocate in their constructors (from PSRAM) - create them in `playerBegin()`,
  not as globals: PSRAM is not ready when global constructors run (boot loop otherwise).
- Arduino 3.x (pioarduino) was tried first and abandoned: ESP32-A2DP 1.8.11 needed hooks
  (`btInUse()`/`bleInUse()`) because core 3.3 releases BT memory in `initArduino()`, its
  `btStartMode(CLASSIC)` releases Bluedroid's BLE host memory and crashes `esp_bluedroid_init()`,
  and the baseline heap is ~10 KB smaller. Not worth revisiting.
- **Framework build (env `idf`) pitfalls, 2026-10-09:** a `subst` drive or junction does not get
  around the space in the project path (PlatformIO resolves it back to the real path, CMake keeps
  the alias, object paths come out wrong like `rc\battery.cpp`); PlatformIO's generated
  `CMakeLists.txt` uses the folder name as project name, which breaks on the space; `build_flags`
  reach every IDF component (no `-fno-exceptions` there, asio needs exceptions; set exceptions
  and stack checks in sdkconfig instead); IDF's `-Werror=all` turns audio-tools warnings into
  errors (`build_unflags = -Werror=all`); `build_src_filter` is ignored (use `src/CMakeLists.txt`);
  PowerShell 5.1 with `$ErrorActionPreference = "Stop"` aborts on git's harmless stderr output.
  Never delete the old sdkconfig deprecated-alias block back into `sdkconfig.defaults`: it repeats
  old values (e.g. `CONFIG_TCP_WND_DEFAULT=5760`) under old names.
- `core_dir = C:/pio` in `platformio.ini` is required: the frameworks contain paths that exceed the
  Windows 260-char limit under the default `~/.platformio`. Don't remove it.
- Don't edit `platformio.ini` with PowerShell 5.1 `Set-Content -Encoding utf8` (adds a BOM;
  PlatformIO then fails with "File contains no section headers"). Running `pio` from Git Bash with
  output redirected can crash on a cp1252 encode error in its progress bar; use PowerShell for uploads.
- A2DP carries 44.1 kHz 16-bit stereo; other rates are resampled in `PcmSink` (linear). Codec is
  chosen from the Content-Type (`aac`/`mp4` -> AAC, `mpeg`/`mp3` -> MP3), else from the URL.
- **ST7789 1.69" (2026-10-10):** Adafruit's `init(240, 280)` works as is: colours right with its
  INVON, offset right (white test frame visible on all four sides), no noise at 26.7 MHz over
  ~15 cm jumper wires. The former Robot LCD needed its own init at 4 MHz (Adafruit's 32 MHz init
  gave stripes); that code is in git history (commit 7dd3f69).
- **`playerStatus()` before the player is ready (2026-10-10):** `playerBegin()` creates the lock
  first and the Bluetooth source (`a2dp`) last. Once the display task polled every 40 ms it hit
  that gap and crashed in `btLinkUp()` (LoadProhibited, boot loop; recovered by a USB flash).
  `playerStatus()` now returns an empty status until `a2dp` exists. Any new task that calls
  player functions early must expect that.
- The setup access point is open (no password) and the web page has no login; it is meant for the home LAN.
- Diagnostics: two `ESP_LOGW(... (diag))` lines in the library copy of
  `ESP32-A2DP/src/BluetoothA2DPSource.cpp` log the ACL status (`stat=256` ok, `260` page timeout)
  and the A2DP `disc_rsn`. Since 2026-10-09 they are the fourth patch of
  `scripts/patch_a2dp_reconnect.py`, so every library copy (radio, usb, idf mirror) gets them on
  every build. `CORE_DEBUG_LEVEL` is 2.
- The A2DP patches in that same file (cancel a stuck page, do not page when auto-reconnect is
  off, publish the RSSI delta, start the media stream right after connecting, AVRCP target) are
  reapplied by `scripts/patch_a2dp_reconnect.py` before every build (`extra_scripts` in
  `platformio.ini`). `pio pkg install` restores the upstream file; the next build puts all five
  patches back, the diag lines included. A changed patch needs a new marker plus an upgrade path
  for copies that already hold the old version (see the AVRCP patch).
- **Learned 2026-10-10 (one line each; details in the sections named):**
  - Audio dropping out "now and then" was a far mesh point, not the display: -58..-66 dBm on one
    AP gave ~11 KB/s input for a 12 KB/s stream and the buffer drained; another AP gave -46 dBm and
    a full buffer. The radio never left the remembered AP because RSSI rarely stayed under the
    roam threshold twice -> settle check a minute after each connect (`main.cpp`, Mesh WiFi).
  - An ESP32 scan while associated often does not list its own access point; `/api/scan` adds it.
  - The web "Search" button refused to scan whenever the headphones were not connected, also with
    Bluetooth switched off (e.g. after an OTA) - it now only refuses while a link is being built.
  - OTA with Bluetooth on stalled at ~23 %: the update handler drops the link, then `loop()`
    opened a paging window (WiFi off 30 s) mid-upload. Fixed with `webUploadActive()` and
    `playerHoldBluetooth()`; a curl wait loop that never sees the radio for ~30 s is usually a
    paging window, not a crash. Reset reason 1 in `diag` = power was cut (e.g. rewiring).
  - Env `radio` (prebuilt framework) puts newlib's `strftime` in IRAM: adding it overflowed
    `iram0_0_seg` by 1.2 KB. Use `snprintf` on `localtime_r` fields (`sdcard.cpp`).
  - SD: FAT only (no exFAT in IDF 4.4 FatFs, 64 GB+ cards need formatting); never ask for free
    space (`f_getfree` can scan the whole FAT); the web server blocks during `f_mkfs` and file
    downloads. A "MP3-TF-16P" board is a DFPlayer Mini (its own MP3 chip owns the card) and cannot
    serve as a card reader; use a plain SPI microSD breakout.
  - Robot LCD: Adafruit's ST77xx init at 32 MHz gave stripes (needed Arduino's own init at
    4 MHz); its TN panel faded mid greys towards the bottom (hence full-intensity colours only).
  - GFX has no clipping: scrolled text overwrote the logo column (redraw the logo after it).
  - Web page JS: an element id (`stations`) is also a global variable; do not reuse element ids
    as JS variable names. Browser range inputs fill to the thumb centre, so 0 showed blue and 100
    grey; the page styles its own track (`--p` from `rangeFill()`).
  - Bluetooth devices: one A2DP sink at a time. JBL Charge 5 accepts the legacy PIN pairing; it
    once rejected the bond on the first reconnect (`authentication failed, status:9`, bond
    deleted, re-paired by itself later); after a link loss it pages the radio itself for ~2 min
    (`stat=267` connection exists, then 260/287) because the ESP32 is non-connectable; with the
    AVRCP target in place it came back in ~3 s.
  - AVRCP: `esp_avrc_tg_init()` is asynchronous; setting the passthrough filter right after it
    fails (259) and leaves every button refused. Set it in the target CONNECTION_STATE event.
    The JBL sends PLAY for 1 press, FORWARD for 2, FORWARD + PLAY for 3 (no BACKWARD).
  - Do not "restore" settings the user changed on the web page (e.g. the device order) to match
    an earlier request; ask first.

## Display wiring (1.69" ST7789 IPS, VSPI, since 2026-10-10 evening)
| Display pin | ESP32 | Note |
|-------------|-------|------|
| GND | GND | |
| VCC | 3V3 | 3.3 V logic; do not use 5 V on the signal side |
| SCL | GPIO18 | VSPI SCK |
| SDA | GPIO23 | VSPI MOSI |
| RES | GPIO21 | |
| DC  | GPIO22 | |
| CS  | GPIO5  | strapping pin, fine for CS |
| BLK | GPIO26 | backlight, high = on, LEDC PWM |
Planned microSD breakout on the same bus (until then `SD_ENABLED 0` in `config.h`): CS GPIO4,
SCK GPIO18, MOSI GPIO23, MISO GPIO19, VCC 3V3 (or 5V for a module with its own regulator and
level shifter), GND. Then set `SD_ENABLED 1`.

Former displays (Robot LCD, e-paper) used the same CS/DC/RST/BL pins; the e-paper had BUSY on
GPIO4, the Robot LCD its SD chip select there.
Avoid GPIO16/17 (PSRAM on WROVER),
GPIO6-11 (flash), GPIO0/12/15 (strapping), GPIO1/3 (USB serial). GPIO2 (strapping, low at boot) is
reserved for the amplifier shutdown of the planned speaker output (was GPIO19, now MISO).

## Stations
My Radio (user's own Icecast 2.4.4 server, `http://icecast.example.com:8888/`) is what the user
listens to most; its 96 kbps AAC mount (12 KB/s) plays steadily next to Bluetooth, so the TCP-window
limit below only matters for 128 kbps and up. Since 2026-10-09 the server has
`<burst-size>262144</burst-size>` and `<queue-size>1048576</queue-size>` (were 65535 / 524288), so a
(re)connect fills the radio's 256 KB buffer at once (~20 s cushion). Since 2026-10-09 (evening)
the burst is set per mount, as in `docs/icecast-burst-per-mount.xml`: 256 KB only for `/96.aac`
(the radio), ~4 s for the other mounts (160/96/32 KB), global default back to 65536, so browsers
and apps do not run 21 s behind live. Verified from the PC: the first 1.5 s brought 276 KB on
`/96.aac`, 217 KB on `/320.mp3`, 120 KB on `/192.ogg`, 33 KB on `/64.opus`. That file has placeholders instead of the
passwords; never put the real server passwords in the repo. Other suggestions given: a
`fallback-mount` per mount so listeners stay connected while the encoder restarts, `/admin/`
restricted to the LAN, one listen socket if the encoders use the Icecast protocol. Four mounts
(verified 2026-10-06):
| Mount       | Format                         | Use on ESP32 |
|-------------|--------------------------------|--------------|
| `/96.aac`   | AAC-LC, 44.1 kHz stereo, ADTS  | default station, verified for hours |
| `/320.mp3`  | MP3 320 kbps, 48 kHz           | plays (resampled) but stutters: needs 40 KB/s, WiFi next to BT gives ~30. A 128 kbps 44.1 kHz MP3 mount (16 KB/s) would fit |
| `/192.ogg`  | Ogg Vorbis 192 kbps            | not supported (heavy decoder) |
| `/64.opus`  | Opus 64 kbps in Ogg, 48 kHz    | not supported (heavy decoder + resampling) |

Public Dutch stations, verified 2026-10-09 (all plain http; the radio has no TLS). Rules of thumb:
- **https-only links do not work.** Most stations also serve the same mount over http; try `http://`
  first. Only the AAC-LC and MP3 formats play; `audio/aacp` (HE-AAC, ADTS core at 24 kHz) decodes
  to a dull 24 kHz core on Helix, so avoid `aachigh`/`aaclow`/`...AAC.aac` variants.
- **StreamTheWorld** (Radio 10, Veronica, Team FM, ...): use
  `http://playerservices.streamtheworld.com/api/livestream-redirect/<MOUNT>.mp3`. It redirects to an
  http host on port 80 (the livestream config only lists https ports, but http works). The mount
  without extension returns FLV (unusable), `<MOUNT>AAC.aac` is HE-AAC. The current mount names come
  from the station's own player: the Veronica site loads its theme channels from
  `https://api.mediahuisradio.nl/umbraco/delivery/api/v2/content?filter=contentType:themeChannel&brand=veronica`
  (`stationKey` dab01/dab02/web06/web07 -> mounts `DAB01_MP3` etc.); the old `SRGSTR09..12` mounts
  are gone (404). StreamTheWorld sends the ICY title only on song changes, so the first title can
  take minutes.
- **NPO** (`icecast.omroep.nl`): `http://icecast.omroep.nl/<zender>-sb-mp3` (MP3 96 kbps, 48 kHz,
  resampled) is the right pick; `-bb-mp3` is 192 kbps (borderline next to Bluetooth), `-bb-aac` is
  HE-AAC.
- **Qmusic**: every link redirects to `http://audio-streaming.qmusic.nl/Qmusic_nl_<x>.mp3` (MP3 128
  kbps, 48 kHz, 16 KB/s). Titles do come (`icy-metaint: 1024` when the request sends
  `Icy-MetaData: 1`, as IcyStream does). Borderline next to Bluetooth: the input averaged 13-14
  KB/s on 2026-10-09 and the buffer ran dry every 1-2 minutes (see the TCP window note under Open
  points). AAC variants are HE-AAC.
- **Arrow Classic Rock**: `http://stream.player.arrow.nl/arrowcr` (MP3 96 kbps).

| Station | URL | Format |
|---|---|---|
| NPO Radio 2 / 5 | `http://icecast.omroep.nl/radio2-sb-mp3`, `.../radio5-sb-mp3` | MP3 96, 48 kHz |
| Radio 10 (+ 60s/70s, 80s, 90s, Love Songs, Disco Classics, Non-Stop, Top 4000) | `.../livestream-redirect/RADIO10.mp3`, `TLPSTR18/20/22/04/23/15/24.mp3` | MP3 128, 44.1 kHz |
| Radio Veronica, 90s, 80s, Rock Radio, Top 3000 | `.../livestream-redirect/VERONICA.mp3`, `DAB02_MP3.mp3`, `DAB01_MP3.mp3`, `WEB07_MP3.mp3`, `WEB06_MP3.mp3` | MP3 128, 44.1 kHz |
| Team FM Overijssel | `.../livestream-redirect/TEAMFMOVERIJSSEL.mp3` | MP3 192, 44.1 kHz (24 KB/s, borderline) |
| Qmusic (+ Nederlandstalig, Foute Uur, Non-Stop, Summer, Top 40) | `http://audio-streaming.qmusic.nl/Qmusic_nl_live.mp3`, `_nederlandstalig`, `_fouteuur`, `_nonstop`, `_thema`, `_top40` | MP3 128, 48 kHz, borderline (needs 16 KB/s) |
| Arrow Classic Rock | `http://stream.player.arrow.nl/arrowcr` | MP3 96 |
| Joe (relay) | `http://icecast.example.com:8888/joe.aac` | AAC-LC 96, 44.1 kHz, from Joe's HE-AAC 96 |
| NPO Sterren.nl (relay) | `http://icecast.example.com:8888/sterrennl.aac` | AAC-LC 96, from MP3 192 48 kHz |
| RadioNL (relay) | `http://icecast.example.com:8888/radionl.aac` | AAC-LC 96, from MP3 192 44.1 kHz |
| Team FM (relay) | `http://icecast.example.com:8888/teamfm.aac` | AAC-LC 96, from MP3 192 44.1 kHz |

Relays on the NAS (`nas/<name>-relay/`, one Liquidsoap container each): the right tool when a
station is https-only, HE-AAC only (the radio plays just the dull 24 kHz core) or heavier than
96 kbps. Each costs ~3-6 % of one NAS core and ~70 MB RAM, and fetches its source all the time.
New relay: copy a folder, change source URL, mount and name, add a hidden `<mount>` block with
`burst-size` 262144 to `icecast.xml` (backup first, edit in place), reload Icecast with HUP,
create `.env` on the NAS from `icecast.xml`, `sudo docker compose up -d`, check format
(AAC-LC 44.1 kHz) and title order (swap "Song - ARTIST" like `joe.liq` when needed).

## Battery measurement
Implemented in `src/battery.*`: GPIO35 (ADC1, 470k/470k divider -> x2, calibration `BAT_CAL_PERMILLE 989`
from a multimeter check on 2026-10-09: 4.09 V at the Li-Po vs pin 2067 mV; was 986 from 3.900 V vs
3.957 V) for the battery, GPIO39 for external-power sense and, since 2026-10-09, the USB input
voltage (`PWR_CAL_PERMILLE 1457` = divider x calibration in one factor: multimeter 4.54 V from the
power bank vs pin ~3117 mV; matches a 10k/22k divider). The GPIO39 pin is then near the top of the
ADC range (11 dB): a real 5 V supply would read too low (the reading flattens above ~4.6-4.8 V).
16-sample average with `analogReadMilliVolts` (eFuse-calibrated). Shown bottom right on the screen
("4.1V 88% USB", re-read every 30 s) and in `/api/status` (`batteryMv`, `batteryPct`, `usb`, `usbMv`, raw pin values under `diag`);
Diagnostics shows "Battery 4.09 V, 88% (pin ...), USB 4.54 V (pin ...)".
Power setup since 2026-10-09: a 10,000 mAh USB power bank on the board's USB connector; it charges
the Li-Po and feeds the radio (estimate 40-50 h together with the Li-Po). The Li-Po bridges a power
bank swap or auto-off (many banks switch off below ~50-100 mA; the radio draws 150-200 mA).
Hardware prerequisite (Olimex user manual, p. 8): the three SMT jumpers **BAT_PWR_E1, PWR_SENS_E1 and
BAT_SENS_E1 are open as shipped** - "Close them (solder the pads together) to enable both functions,
on pins GPI35 and GPI39". All three are soldered since 2026-10-06; `docs/olimex-jumpers-bottom.png`
shows where they are (bottom view, generated from the KiCad Rev.D layout). With open jumpers (or no
battery) the pins float (~0.3 V) and the firmware reports "no reading". While charging over USB the
reading is the charger voltage (~4.0-4.2 V), not the resting state of charge. First full run on
battery: the whole night + day (~14 h) until the cell was empty.

## Open points
- Planned: line + speaker output with PCM5102A + PAM8403 (ordered; see `docs/speaker-output.md`):
  `PcmSink` writes to I2S when the Bluetooth toggle is off; decoder must then run without a link.
- 1.69" ST7789: judge the normal layout near the rounded corners on the real panel (photo);
  the space under the battery line is still free (e.g. a volume or buffer bar). Add the EC11 rotary
  encoder (GPIO32/33, button GPIO25, PCNT via ESP32Encoder) with a menu; see
  `docs/display-research.md`. Backlight dimming, night mode and idle dimming are done; the
  encoder should also count as activity (`displayWake()`).
- SD card (done 2026-10-10: log, measurements, settings backup, logos, firmware copies). The user
  does not want music files on it. Since the ST7789 replaced the Robot LCD there is no card
  reader: `SD_ENABLED 0` in `config.h` until the planned microSD breakout (CS GPIO4) is wired. Possible next: song history ("wat was dat liedje?"),
  `stations.txt` import from the card.
- Repeat the battery run with the power guard in place and read `/api/status` `diag` (underruns,
  reconnects, min heap) before the battery runs out.
- Optional: more NAS relays for 128 kbps stations that sometimes stall (Qmusic, Radio 10); see the
  relay notes under "Stations".
- Paging window: stop the stream first and use `esp_wifi_stop()` instead of `WiFi.mode(WIFI_OFF)`
  (un-init timeouts, pages still failing inside the window); find why `is_connected()` went stale
  (needs a log of the moment it happens: the web page's live log now keeps the last 64 KB).
- Framework build with a 32 KB TCP window: phase 1 builds (env `idf`, 80.5% flash) but failed on
  the radio (2026-10-09): repeated 5 s stalls and underruns on My Radio, while the radio build
  next to it was stable; rolled back. Suspects: 32 KB in flight vs. modem sleep (losses, TCP
  backoff) or WiFi/lwIP buffers in PSRAM. Trial 1 (window 16384, buffers internal) stalled the
  same way. Control build `idf` with the old 5760 window (2026-10-09 17:48) was stable for 5 min
  with ~9 KB more free internal RAM (min 61 KB) and 80.5% flash: the self-built IDF is fine, a
  larger TCP window is what stalls next to Bluetooth + modem sleep. Dead end for 128/320 kbps;
  the control build stays on the radio as a soak test. Details: `docs/framework-build.md`.
  Phase 2 (same evening), each step 5 min on My Radio: 2a classic Bluetooth only (BR/EDR,
  BLE/GATT/SMP off) 73.3% flash, ~82 KB free; 2b also SPP/HFP off 70.0%, stable on the retest
  (the first try lost the headphones, which turned out to be unreachable on 2a too); 2c Bluedroid
  heap in PSRAM first 70.1%, ~84 KB free (min ~80). 2c is on the radio. `scripts/build_idf.ps1`
  deletes the mirror's `sdkconfig.idf` when `sdkconfig.defaults` is newer: ESP-IDF otherwise
  keeps the old values.
  Background: Arduino as an ESP-IDF component with an own sdkconfig. The
  prebuilt lwIP TCP window is 5760 bytes (`CONFIG_LWIP_TCP_WND_DEFAULT`); with the ~400-500 ms
  round trip seen next to Bluetooth that alone caps a stream near 12-30 KB/s, which would explain
  the 320 kbps stutter. Own sdkconfig: larger TCP window, lwIP buffers in PSRAM, BLE/GATT/HFP/SPP off
  (-100-200 KB flash). Hard to reverse; ask the user first. Cheap alternatives: DTIM 1 on the mesh
  access points (lower latency with modem sleep; the FRITZ!Box UI does not seem to offer it), larger
  `burst-size` on the own Icecast server. Measured 2026-10-09: ping to the radio while streaming
  averages 350-670 ms with peaks of 1-1.9 s, and 5760 B / RTT matches the 9-14 KB/s input seen on
  128 kbps stations. "Prioritized" for the radio in the FRITZ!Box did not help (it only shapes the
  internet line, the delay is on the WiFi hop where WiFi and Bluetooth share the antenna).
