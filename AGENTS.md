# AGENTS.md

## Project
ESP32-InternetRadio: a battery-powered internet radio built from an Olimex ESP32 board, a small
1.69" colour IPS display (ST7789, since 2026-10-10) and Sony Bluetooth headphones. The radio
streams internet radio stations over WiFi, sends the audio to the headphones over Bluetooth A2DP,
shows station/status info on the display and is configured/controlled through a web page served by
the ESP32.

Current state (2026-10-11): working and in daily use. Everything is in English (web page, display,
messages, docs) and the project is public as hghaken/ESP-iRadio. WiFi setup via captive portal; the radio
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
copies) runs again since 2026-10-14 on a separate microSD breakout (a second, good 16 GB card; 27 station
logos on it). Calibrated battery measurement (~14 h measured on the former 2500 mAh cell; a 5000 mAh cell since 2026-10-11, expected ~28 h), power guard against brownout
loops. Station list up to 60 entries (NVS blob); about 25 public Dutch stations verified and in
use (see "Stations"). Next to active Bluetooth the radio gets ~9-16 KB/s: 96 kbps AAC is fine,
128 kbps MP3 is borderline, 192/320 kbps stutters. Stations that are too heavy or only offer
HE-AAC are re-encoded to AAC-LC 96 kbps by Liquidsoap relays on the user's NAS (Joe, NPO
Sterren.nl, RadioNL, Team FM; `nas/`) and played from the own Icecast.
Firmware v1.38 (2026-10-15: microSD breakout, logo layout, rotary encoder, line output; v1.17 was the JBL fix). Open problem: with the JBL Charge 5 the WiFi stream starves
now and then (minutes to hours into a link; the headphones never show it); investigation,
workarounds and diagnostics in `docs/wifi-stall-investigation.md` and under "Zombie WiFi" in
Pitfalls.
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
Line output since 2026-10-14 (v1.31): a PCM5102A DAC module on I2S plays when the Bluetooth
toggle is off, and since 2026-10-15 (v1.37) a MAX98357A speaker amplifier on the same I2S lines
plays with it (web checkbox "Speaker"; see `docs/speaker-output.md`).
Public repository: GitHub hghaken/ESP-iRadio (branch `master`, GPL-3.0). Host names such as
`icecast.example.com`, `my-nas` and 192.168.1.x stand for the owner's own servers.

Hardware:
- Olimex ESP32-DevKit-LiPo with ESP32-WROVER module (rev D), p/n 0000099739, GTIN 4260696351831.
  Chip ESP32-D0WD-V3 rev 3.1, 4 MB flash, 4 MB PSRAM.
- Li-Po battery 3.7 V 5000 mAh (JST-PH) since 2026-10-11; before that 2500 mAh (LP785060), which
  ran ~14 h. Charged by the board's on-board charger (BL4054B, ~455 mA set by Rprog 2.2k on the
  Olimex schematic): an empty 5000 mAh cell needs ~11-12 h on USB, so the power bank keeps it topped
  up rather than filling it quickly. The percentage is derived from the voltage, so the larger cell
  needs no firmware change; `BAT_CAL_PERMILLE` was calibrated on the old cell - check it against a
  multimeter once.
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
- `scripts/flash_usb.ps1` - USB update (through the firmware with progress bar, esptool fallback),
  see Commands. `scripts/usb_update.py` is its firmware path.
- `scripts/patch_idf_bitpool.py` - env idf only (patches the framework-espidf package before
  each build; part 3 adds the v1.15 Bluetooth TX counters in btc_a2dp_source.c, bta_av_aact.c,
  l2c_link.c and btm_pm.c, part 4 the v1.17 full-media-packets rule): sets Bluedroid's SBC source max bitpool
  (`BTA_AV_CO_SBC_MAX_BITPOOL` in the framework-espidf package) to 35 before each build (v1.08,
  did not cure the JBL stalls; set it back to 53 when the investigation closes), and adds one
  `ESP_LOGE` "(diag) a2dp stream: peer mtu ..." line where the stream starts (v1.11). The self-built
  IDF logs at level error (`CONFIG_LOG_DEFAULT_LEVEL` 1): `ESP_LOGW`/`ESP_LOGI` inside IDF
  components compile to nothing (v1.10 lost its line that way).
- `scripts/patch_sd_sdhc.py` - both envs, before every build: fixes the Arduino SD library
  (`libraries/SD/src/sd_diskio.cpp` in the framework-arduinoespressif32 package) for cards that
  come out of the init as byte-addressed SDSC although they are SDHC: a card with more than
  4,194,304 sectors (> 2 GB) is switched to block addressing, with a log line. Without it the
  faulty 16 GB card of 2026-10-14 read sector 0 only ("no valid FAT volume") and every write failed
  (CMD13 R1 0x33), also at 1 MHz. `sdcard.cpp` logs `[sd] diag:` lines (card type, sectors,
  partition and boot sector) after a failed mount. `sdBegin()` tries a second time after 200 ms
  (v1.33): after esptool's hard reset the card once answered `GO_IDLE_STATE failed` and stayed
  unread until the next restart.
- `scripts/patch_a2dp_reconnect.py` - reapplies six patches onto the git-ignored ESP32-A2DP
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
- Line output (in `src/player.*`, since v1.28/v1.30): `lineTask` (core 1, prio 5) pulls PCM from
  `pcmBuf` through the same `pullPcm()` as `btData()` (flush, silence padding, software volume,
  `btPulled` counters) and writes it to I2S_NUM_0 (master TX, 44.1 kHz, 16-bit slots by default
  since v1.31, they sounded the same as 32-bit slots; web checkbox "Line output: 32-bit I2S slots"
  since v1.32, NVS `l32`, `/api/line32 on=`, also in the SD backup; APLL, 4 x 256-frame DMA = 4 KB internal RAM, driver
  installed on first use).
  Pins `I2S_BCK_PIN` 27, `I2S_LRCK_PIN` 14, `I2S_DATA_PIN` 13 in `config.h`. It follows the
  Bluetooth toggle: off = `lineActive` (set after the link is down, cleared before Bluetooth starts,
  so only one reader of `pcmBuf`), on = I2S stopped. `outputUp()` (line out or Bluetooth link)
  replaces the link check in `PcmSink`, `resetPipeline()` and `decodeTask`. Display status
  "Line out  buf NN s  AAC", `lineOut` in `/api/status`, Diagnostics "Output". Test endpoint
  `/api/linetest` (tone=0/1: 440 Hz at about -12 dB, ignores the volume; fmt=i2s/msb; bits=16/32,
  32 = slots of 64 x fs via `i2s_write_expand`;
  not stored). Module: GeeDIY PCM5102MK 2.0 (header VCC GND GND LRCK DATA BCK; three AMS1117-3.3,
  so VCC = 5 V; SCK and XSMT handled on the board; FMT jumper on I2S; FLT solder bridge N/L as
  shipped); its maker's site has no page for it. First test 2026-10-14: only crackling until the
  DATA wire went to the right pin; the 440 Hz tone isolated that.
  Speaker (v1.37): a MAX98357A I2S amplifier on the same three lines plays with the line output
  when the web checkbox "Speaker" is on (NVS `spk`, default on, `/api/speaker on=`, SD backup
  `speaker=`). `ampSet()` in `lineTask` drives `AMP_SD_PIN` (GPIO2) low = off and releases it
  (input) = on, the board's 1 Mohm pull-up then selects (L+R)/2; never driven high (3.3 V would
  select the left channel only). On only while I2S runs: switched on after I2S starts and off
  before it stops or the driver is reinstalled. GPIO2 is driven low from `playerBegin()`. Display
  status "Speaker  buf ..." while it plays, `speaker` / `speakerOn` in `/api/status`, Diagnostics
  "Output: line out + speaker".
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
  play/pause once = mute on/off (`playerSetMuted`, status "Muted" on the display,
  "muted" on the web page; moving the web volume or changing station unmutes), twice = next
  station, three times = previous station (`stepStation`, saves `lastStation`). Play presses are
  counted in the Bluetooth task and acted on in `playerLoop()` after 0.6 s. The JBL Charge 5 sends
  PLAY (0x44) for one press and FORWARD (0x4B) for two; three presses arrive as FORWARD + PLAY,
  so a FORWARD waits 0.9 s (`TRIPLE_MS`) and FORWARD + PLAY counts as previous. BACKWARD (0x4C,
  a swipe back on the Sony) is previous at once. Verified on the WH-1000XM6 too (2026-10-11): a
  double tap sends PLAY (0x44) or PAUSE (0x46) alternately (both toggle mute), swipe forward
  FORWARD (0x4B), swipe back BACKWARD (0x4C). Every button press also calls `displayWake()` (a dimmed screen comes back). Volume keys stay on the speaker. Pitfall:
  `esp_avrc_tg_init()` is asynchronous, so the patch's `esp_avrc_tg_get_psth_cmd_filter()` right
  after it fails with 259 (ESP_ERR_INVALID_STATE) and leaves the supported command set empty
  (every button refused); the filter is set in the target CONNECTION_STATE event instead
  (`[avrc] button filter: ... cmds 0000 0000 1fff 0078`). All target events log as `[avrc]`.
- `src/battery.*` - battery / USB sensing on GPIO35 / GPIO39 (see "Battery measurement").
  "Battery fitted" (web checkbox, NVS `batf`, `/api/batfitted on=`, default on, v1.36): off =
  no battery reading at all (`batteryRead()` reports invalid, so the display shows only "USB" and
  the power guard skips its battery checks). Without a cell the pin reads the charger output
  (~3.96 V on USB, shown as ~70 %); measured 2026-10-14 it cannot be told apart from a real cell
  (the same 30-125 mV ADC noise within 100 ms with and without one).
- `src/encoder.*` - EC11 rotary encoder (A GPIO32, B GPIO33, button GPIO25, common pins to GND,
  internal pull-ups; `ENC_*_PIN` in `config.h`), on the breadboard since 2026-10-14 (v1.26):
  turning = volume 1 % per detent (unmutes; NVS `vol` written 2 s after the last turn; the
  display shows a volume bar instead of the status line for 2.5 s, `displayShowVolume()`),
  short press = mute on/off (bar shows "Muted"), long press (0.7 s, acted on while held) = station
  chooser (`displayShowChooser()`: "Choose station", name, "n / N" instead of station, artist and
  song; no scrolling meanwhile): turning browses (wraps), a short press plays and stores it as
  `lastStation`, a long press or 10 s without input cancels. Every input calls `displayWake()`.
  The display task looks at once after encoder feedback (`poke`) instead of at its 250 ms check.
  Decoding: quadrature in an IRAM interrupt on both edges (pins read from `GPIO_IN1_REG`); a
  detent counts when A and B are back at rest (both high) with a net of 2+ steps in one direction
  since the last rest, which keeps the count exact despite bounce and missed edges (raw / 4
  drifted in the v1.24 test). `ENC_REVERSE` makes clockwise positive on this wiring. Button
  debounced 30 ms in `encoderLoop()`. `diag.enc` (Diagnostics "Encoder") shows the counters.
  Test 2026-10-14: 20 clicks right by hand read 19, 20 left came back to exactly 0; ~10
  bounce steps per minute of turning without RC filter. With the RC filter on the breadboard
  (2026-10-15: 10k pull-up to 3V3 + 100 nF to GND on A, B and the switch, as R10-R12/C6-C8 in
  the KiCad schematic; internal pull-ups stay on): 20 right = exactly +20, 20 left back to 0,
  fast back and forth back to 0, raw steps net 0, 0 bad steps; the user finds it smooth now.
- `src/power.h` - `powerBrownouts()` for the diagnostics (guard itself lives in `main.cpp`).
- `src/display.*` - status screen on the 1.69" ST7789 IPS (`Adafruit_ST7789::init(240, 280)`:
  generic init with INVON - IPS panels need it - and the 20-row offset; rotation 1, or 3 when
  flipped; pixel data at 26.7 MHz), own task. Layout constants for 280x240 at the top of the
  file; wide side margins because the glass has rounded corners. The whole frame is drawn into
  a `GFXcanvas16` (131 KB, PSRAM) and pushed in one go when anything changed (checked every
  250 ms): no flicker. Top to bottom: dark red header bar "ESP iRadio" + Bluetooth state
  (`BT: on / searching / off`, FreeSans Bold 9 pt, white), station (FreeSans Bold 12 pt, white),
  artist (FreeSans 12 pt, yellow), song (FreeSans 12 pt, cyan), all on black (the dark blue bar behind artist and song went in v1.23), each a `ScrollLine` strip: text
  wider than its area scrolls (2 s pause, ~25 px/s to the end, 2 s pause, back) and only that
  strip (~18 KB) is pushed every 40 ms; scrolling can be switched off (NVS `scroll`,
  `/api/scroll`). A 64x64 station logo from the SD card sits left of station, artist and song,
  centred vertically on the three lines (x 14, rows 53-116, since v1.22; all three strips narrow; GFX has no clipping, so `drawLine()` clears the logo column and redraws the
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
- `src/sdcard.*` - microSD card (CS GPIO4, shares VSPI with the display; `SD_ENABLED 1` in
  `config.h`; since 2026-10-14 a plain 3.3 V breakout without regulator (4x 10k pull-ups) on
  breadboard wires with a 16 GB SDHC card (the second one; OCR C0FF8000). The first 16 GB card was faulty: it reports no
  SDHC bit (OCR), and after the addressing fix every write left it busy (`sdWait(): Wait
  Failed`, CRC/token errors), while the 128 GB card on the same wiring wrote 27 logos without
  an error. Station logos come from the TuneIn catalogue (`opml.radiotime.com/Search.ashx`,
  `logoq` images), scaled to 64x64 RGB565 on the PC and posted to `/api/logo?i=N`; until 2026-10-10 the card sat in the
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
    `display.cpp` copies it and draws it at x 14, y 53 (centred on the three text lines), with the
    station, artist and song strips narrowed to the right of it. GFX has no clipping, so `drawLine()` clears the
    logo column and redraws the logo after drawing scrolled text.
  - History (v1.38, 2026-10-15; everything kept a year, `pruneBefore()` removes files whose
    date name is older than a year):
    - `/events/YYYY-MM.log`: one line per event with its wall-clock time: boot with firmware and
      reset reason (crashes named), WiFi connected / reconnecting / moved to another access
      point, Bluetooth connected / link down / switched on or off / reconnected for WiFi
      (starvation refresh), play / stop, stream stalled / lost / underrun / connect failed (first
      of a series) / connected again, updates over WiFi and USB (start, done, failed), firmware
      restore, brownout waits, crash dumps. `sdEvent(fmt, ...)` (in `sdcard.h`) can be called
      from any task: lines go into an 8 KB PSRAM buffer under a spinlock (`millis()` + text) and
      `writeEvents()` writes them every 30 s, the time worked out from `millis()` once the clock
      is set (before that it waits up to 3 min, then "uptime N s"). `sdFlushEvents()` writes them
      before a planned restart (web restart, updates, firmware restore).
    - `/stations/YYYY-MM-DD.csv` and `/stations/months/YYYY-MM.csv`: per station minutes
      listened (playing, sound on an output, not muted), minutes streamed, KB in, stream kbps,
      underruns, reconnects, failed connects. `stMinute()` counts once a minute from the player's
      running totals (`PlayerStatus` `inBytes`, `connectFails`, `underruns`, `reconnects`); the
      day file is rewritten every 10 minutes (and read back after a restart), the month file is
      summed from that month's day files.
    - `/crash/YYYY-MM-DD_HHMMSS.elf`: ESP-IDF's ELF core dump (coredump partition, sdkconfig
      `CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH`), copied once the clock is set (or after 3 min), its
      summary (task, PC, cause, backtrace, ELF SHA) written to the events, then erased from
      flash. First run 2026-10-15 found an old dump from the 2026-10-10 display-task crash
      (task display, cause 28 LoadProhibited). Decode with `xtensa-esp32-elf-gdb` / `idf.py
      coredump-info` against the matching firmware ELF.
    Web page, section SD card: "Stations: listening and stream quality" (today, a month, the last
    12 months or one of the last 30 days: table with listened, streamed, average input KB/s
    against what the stream needs, underruns, reconnects, failed connects), "Events" (per month,
    newest first) and download links for the events, station months and crash dumps.
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
- `src/serialupdate.*` - firmware update over USB by the running firmware (protocol in the
  header): `IRADIO-UPDATE <size> <md5>` at 115200, `#R 921600`, then 1 KB chunks stop-and-wait
  (`#K <bytes>` after each flash write, so nothing arrives while the flash cache is off and the
  UART interrupt may be held off), `#D` + restart or `#E <reason>` (playback resumes). Our own log
  lines to the serial port are muted meanwhile (`weblogSerialMute`); `Serial.setRxBufferSize(2048)`
  before `Serial.begin`. The PC opens the port with DTR/RTS low so the board is not reset.
- `src/version.*` - firmware version `FIRMWARE_VERSION` in `version.h` (v1.01 .. v1.38 on 2026-10-15),
  shown bottom left on the display, under Diagnostics -> Firmware and as `diag.version`. Raise it
  by 0.01 with every firmware change (in the same commit), before building and uploading. Also the build stamp (`buildStamp`) from the generated, git-ignored `include/build_stamp.h`
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
  (line + speaker output: PCM5102A I2S DAC on GPIO27/14/13, working since 2026-10-14, feeding
  RCA/3.5 mm, and the MAX98357A speaker amplifier with shutdown on GPIO2; output follows the
  Bluetooth toggle), `framework-build.md` (self-built framework: plan, trials, results) and
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
- `KiCad/` - PCB in KiCad 9: the project `KiCad/esp-iradio/` (schematic Rev B of 2026-10-15 for
  the integrated board on two A3 sheets in Elektuur style, `esp-iradio.kicad_sch` "radio" +
  `power_usb.kicad_sch`, generated by the scripts in `tools/` (README there: regenerate, ERC,
  netlist check `intcheck.py`; stop regenerating once the board is edited by hand); project
  libraries in `lib/` incl. `lcsc` parts made with easyeda2kicad (STEP models git-ignored);
  status, decisions and open checks in its `NOTES.md`; Rev A, the carrier board for the Olimex
  module, is in git history; first layout of Rev B on 2026-10-15: 80 x 60 mm, 4 layers, generated
  and autorouted by the scripts in `tools/` (Freerouting), reviewed by Grok and Claude, hand edits
  by `tools/touchup.py`, DRC clean, renders in `renders/`) and the design input: `README.md` (overview, phase 1
  carrier board for the Olimex board with display, EC11 encoder, microSD, PCM5102A line out and
  MAX98357A speaker amplifier; phase 2 integrated board later), `01-requirements.md` (incl. open
  questions for the owner), `02-pinout-and-nets.md` (every header pin and connector, must match
  `src/config.h`), `03-parts-and-footprints.md`, `04-power.md`, `05-layout-guidelines.md` (antenna
  keep-out, routing, fab rules), `06-checklist-and-bring-up.md`, `07-integrated-board.md` (phase 2,
  chosen 2026-10-15: everything on one board - ESP32-WROVER-IE-N16R8 with an external antenna on
  its U.FL, USB-C + CH340C, BQ24074 power-path charger, cell protection, 3.3 V LDO with the on/off
  switch on its EN, the bare display panel soldered by its 12-pin 0.7 mm FPC, every part from the
  JLCPCB stock). Owner decisions 2026-10-11: DAC
  and amplifier as chips (PCM5102APWR, mono amplifier MAX98357A since 2026-10-15, was PAM8302A),
  display on the board,
  3D-printed case around the board (STEP/DXF export), one mono speaker of 4 or 8 Ohm (user's
  choice), a plain USB-C 5 V input (no PD) that powers the amplifier and, through an ideal diode
  into the Olimex +5V pin, also runs the radio and charges the Li-Po; JLCPCB with SMT assembly. A pin change on the board means
  updating `src/config.h`, this file and `KiCad/02-pinout-and-nets.md` together.
- `LICENSE` - GPL-3.0 (arduino-audio-tools is GPL-3.0).
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
- Control: web page, the rotary encoder and the Bluetooth device's buttons; no Home Assistant integration.
- Tools on the dev PC: PlatformIO 6.1.19 (`pio`), Python 3.13, esptool 5.3.1, git, gh (GitHub CLI);
  KiCad 9.0.9 with kicad-cli; for the PCB autorouter Freerouting 1.9.0 on a portable Java 21 in
  `C:\Users\Admin\tools\` (see `KiCad/esp-iradio/tools/README.md`).

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
- **When the radio is connected to the PC (COM7 present), always update over USB, not WiFi**
  (user's instruction 2026-10-11): `.\scripts\flash_usb.ps1` (default: the idf build;
  `-Bin .pio\build\radio\firmware.bin` for the prebuilt one; `-Rom` forces esptool). It stops a
  `pio device monitor` that holds COM7, then asks the running firmware to take the image
  (`scripts/usb_update.py`, `src/serialupdate.*`, since v1.06): the firmware writes the other OTA
  slot like a WiFi update and the display shows the progress bar ("via USB"); 1.5 MB in ~39 s.
  When the firmware does not answer (older than v1.06, crashed, boot loop) it falls back to
  esptool: app to app0 (0x10000), otadata (0xe000) erased so app0 boots, reset (1.5 MB in 13 s,
  no progress on the display: the ROM loader runs then). Settings and pairings in NVS stay
  either way. A WiFi upload had timed out because the radio kept opening paging windows with no
  device paired. Seen 2026-10-11..14: the firmware path often timed out in the first KB ("radio:
  timeout"): a byte lost at 921600 baud through the CH340 left a chunk incomplete and the radio
  waited 10 s. Since v1.34 an incomplete chunk is dropped after 1 s and acknowledged at the old
  position again, so the PC resends it (up to 10 times per chunk); three updates in a row then
  went through with the progress bar. esptool (`-Rom`) shows no progress on the display (the ROM
  loader runs then).
- Full flash over USB (bootloader + partition table + app; ask the user first): `pio run -e usb -t
  upload` (COM7). Only for the first install of the OTA partition table.
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
- Volume: the web slider sets the radio's software volume (applied in `btData()`), stored in NVS
  (`vol`, only that key) and restored at boot. The speaker's or headphones' own volume buttons
  are not controlled by the radio (no AVRCP absolute volume); mute via the play button is not
  stored (a restart plays with sound).
- Battery: shown bottom right on the display and under Diagnostics. Empty cell -> "Battery empty - charge it"
  and deep sleep; charging an empty cell on USB -> "Charging battery, please wait" until 3.55 V. Use a real
  charger (>= 1 A) for an empty battery, not a PC port.

## Working with several agents
More than one AI agent may work on this project (Claude Code, Grok CLI, ...). Open
investigations have a shared working file in `docs/` that every agent reads first and appends to;
it has a **radio lock** line: only the agent named there flashes, restarts or tests the radio.
Current: `docs/wifi-stall-investigation.md` (WiFi stalls next to the JBL Charge 5). Run `git pull`
before editing and commit small steps, so agents do not overwrite each other.

## Conventions
- Everything in the repository is English: web page, display texts, messages, docs and code
  comments (since 2026-10-11, for the public repo). Communication with the user is in Dutch.
- WiFi credentials are entered via the web page and stored in NVS; they are never hard-coded, printed,
  returned by the API, or committed. Any future compile-time secrets go in git-ignored `secrets.h`.
- Commit messages: short, imperative. Push to `origin master` after each working step.
- Every firmware change raises `FIRMWARE_VERSION` in `src/version.h` by 0.01 (v1.01 -> v1.02).
  Documentation-only commits do not.
- Source files use CRLF line endings (git warns "LF will be replaced by CRLF"). Scripted edits that
  match text across line breaks must normalize CRLF to LF first and write back with the original
  endings. Write such edit scripts to a file (not a shell heredoc): heredocs in some agent shells
  turn `\n` in the script into a real line break, which then lands inside C string literals.
  Note that the GFX fonts are ASCII only: `toAscii()` in `display.cpp` strips accents before drawing.

## Pitfalls (learned the hard way, 2026-10-06 .. 10-10)
- Ask the user before: erasing flash, changing partition tables, a full USB flash, or anything that may
  wipe stored settings (NVS). Updates: after every code change build and update at once, without
  asking (user's standing instruction, 2026-10-09): over USB with `scripts/flash_usb.ps1` when the
  radio is on COM7 (2026-10-11), over WiFi otherwise.
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
    The user's mesh (2026-10-11 evening, fixed channels, 20 MHz): FRITZ!Box 5590 main router
    channel 7 (BSSID ..2B:FE, the radio's point, -43..-50 dBm), 7581 living room channel 1
    (..99:B4), 7581 studio channel 13 (..76:4D), 7583 storage channel 4 and 7583 garage
    channel 10 (..B1:A6) further away. With the 5590 on channel 1 the box picked 40 MHz and the
    radio got almost nothing: keep 2.4 GHz at 20 MHz. A changed AP channel makes the radio
    re-associate (`NOT_ASSOCED`, about a minute of dropouts). Scans next to Bluetooth miss
    beacons, so a list may lack a point that a second scan shows. A Raspberry Pi hotspot
    ("gamecontroller2.4", channel 2, -43 dBm) stood next to the radio until 2026-10-11 18:20.
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
  log `[wifi] associated but unreachable, reconnecting`. Since 2026-10-11 a stall also counts:
  a connection that stalls before it delivered 64 KB increments `streamFailures`, which is now
  cleared only after 64 KB on one connection (not on connect). Seen that day: after 12 min of
  normal play the data stopped with no WiFi event (still associated at -45 dBm, 25-100 % ping
  loss, 0.3-0.8 KB/s); every reconnect succeeded and stalled after a few KB, so the old check
  (connect failures only) never fired and the radio played silence. A restart fixed it; the new
  rule re-associates WiFi after such stalls (threshold 2 since v1.03: with 3 the 20 s buffer
  often ran dry first). A 20-minute watch the same evening: the collapse came back every 5-10 min
  (a connection runs 0.4-2.8 MB, stalls, every reconnect then gets 2-17 KB); re-association
  brought 17-19 KB/s and a full buffer within ~20 s. Not correlated with backlight changes. It
  started the evening the JBL Charge 5 (plus AVRCP) was in use; the headphones did not show it
  the day before. Root cause (Bluetooth airtime with the speaker, access point or ESP32 WiFi
  driver) narrowed down by an A/B test on 2026-10-11: 20 min with the WH-1000XM6 (same firmware
  v1.04, same access point, night mode on) gave 0 stalls, 0 WiFi re-associations, average
  12.4 KB/s; the JBL Charge 5 collapsed every 5-10 min. So it comes with the speaker. Suspects:
  more Bluetooth airtime (higher SBC bitpool / packet types) or a role switch (JBL becoming
  master). Tried the same night, each with a 10-minute watch, none helped (6-9 stalls, average
  input ~10.4 KB/s): v1.07 radio stays master (`btLead`, role measured: the JBL took master
  before, the radio keeps it now), JBL 3 m away and the radio moved, v1.08 SBC bitpool 35
  instead of 53 (`scripts/patch_idf_bitpool.py`, env idf only), v1.09 `ESP_COEX_PREFER_WIFI`
  while streaming (web test toggle `/api/coex`, not stored, BALANCE otherwise): fewer full
  collapses but average input 9.7 KB/s. Reading (with Grok CLI, see
  `docs/wifi-stall-investigation.md`): next to the JBL the WiFi path is throughput-limited below
  the 12 KB/s the stream needs (5760-byte TCP window / RTT above ~480 ms), not just stalling now
  and then. Not a larger TCP window first (it stalled on 2026-10-09 and makes a deaf spell worse).
  v1.11 (2026-10-11 midday): after both update restarts the JBL refused the radio (`authentication
  failed, status:9`, then ACL ok and A2DP dropped within 40-80 ms, also in pairing mode); "Forget"
  on the radio + a factory reset of the JBL + "Pair new device" fixed that, and the next
  10-minute watch on the JBL was the first clean one (0 stalls, buffer full, 11.4 KB/s), as clean
  as the headphones (0 stalls, 11.3 KB/s). Lead: the JBL's own state (stale bond / stuck link
  mode), cleared by the factory reset (H7). If the JBL stalls again or refuses after a restart,
  try that reset first. A2DP MTU (v1.11 log line `(diag) a2dp stream`): JBL 660, WH-1000XM6 883.
  The reset only bought ~2.5 h: the collapse came back at 16:13. Turning Bluetooth off in the
  middle of it brought ping from ~1.4 s to 46 ms and the input to 20-36 KB/s at once, so the
  JBL's A2DP link starves WiFi, building up over time on one link (that test also gave a fresh
  WiFi association, so a WiFi-side spiral is not ruled out). Workaround since v1.12, revised in
  v1.13 after a review (`checkStarvation()` in `player.cpp`): while the link plays, two stream
  stalls within 10 min or 30 s of starved input (`slowPeriods`: input under 70 % of the stream
  rate, buffer under half and falling) -> `sourceDisconnect()`, page the same device again in a
  paging window (WiFi off, then a fresh association). Not in the first 3 min after boot or a
  station change; cooldown 10 min doubling to 60 min (reset after 2 h quiet); the planned
  disconnect's `autoHold` lasts until the link is back (cleared on CONNECTED), so automatic
  mode does not move to another device; stream failures are reset so the zombie rule does not
  fire right before the window; when the device does not play within 60 s (e.g. it refuses the
  bond) the refresh stays off until a restart. Log `[bt] WiFi starved next to the link`, `[bt]
  back after the WiFi refresh`; count in `diag.btRefreshes` (Diagnostics: "reconnected Nx for
  WiFi"). The zombie rule alone did not catch it: those connections delivered 100-800 KB before
  stalling. Reviewer's next experiments (docs/wifi-stall-investigation.md): FRITZ!Box link rate
  of the radio healthy vs collapsed, A2DP SUSPEND/START instead of a full reconnect, Bluedroid
  drop/queue counters.
  E1 done 2026-10-11: the FRITZ!Box showed the radio's link at 26 Mbit/s while it played fine and
  1-2 Mbit/s during the drop, and a refresh at that moment did not help (stalls went on right
  after the fresh link and association): a WiFi rate spiral next to the JBL's ~49 A2DP packets/s.
  v1.14 tried `CONFIG_LWIP_TCP_MSS` 1436 -> 536 (no visible change, back to 1436 in v1.15) and
  fixed the refresh check (it waits for a new CONNECTED; `playerBtAudioActive()` stays true for
  2 s after the old link's last pull). Changing the FRITZ!Box to channel 1 made it worse (the box
  picked 40 MHz there; the radio also moved to another mesh point): back on channel 6, then the user moved all mesh points to a new plan (5590 on channel 7, see Mesh WiFi).
  v1.15 adds log-only Bluetooth TX diagnostics (`scripts/patch_idf_bitpool.py` part 3, env idf):
  every 30 s `(diag) a2dp tx 30s: queued N, av drops, frame drops, l2c bufs max, acl completed,
  txq` and a `(diag) link mode` line per HCI mode change. Healthy on the JBL: ~2000 media
  packets per 30 s (~67/s, about 5 SBC frames each), no drops, queue empty, every packet
  completed. A radio restart cured the bad state when a link refresh did not (once; at 18:50
  the state survived a restart too).
  The counters stayed exactly the same when the WiFi side collapsed (18:46): no BT backlog, no
  mode change. v1.16 adds a test endpoint `/api/btpause` (s=2..60: `esp_a2d_media_ctrl` SUSPEND,
  then START; link and WiFi untouched): in the bad state the input went from 0-4 KB/s to 48 KB/s
  while paused and back to 0 the moment audio resumed. Cause found: Bluedroid sends a packet
  every 30 ms tick with the frames due (~10.3) and splits what does not fit the sink MTU into a
  second, small packet; the JBL (MTU 660, 7 frames per packet) got ~67 packets/s, two back to
  back per tick, the headphones (MTU 883, 10 frames) ~34. v1.17 (`scripts/patch_idf_bitpool.py`
  part 4, in `btc_get_num_aa_frame()`): the frames due are rounded down to whole packets, the
  rest stays in the PCM counter for the next tick, so the JBL gets ~49 full packets/s (measured
  1477 per 30 s) with the same audio; the headphones are unchanged. Seen working the same
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
microSD breakout on the same bus (since 2026-10-14, `SD_ENABLED 1`): CS GPIO4, SCK GPIO18,
MOSI GPIO23, MISO GPIO19, VCC 3V3 (the module has no regulator; a module with an AMS1117 and
level shifter needs 5V), GND. Module pin order: 3V3, CS, MOSI, CLK, MISO, GND.

Line output DAC (GeeDIY PCM5102MK 2.0, since 2026-10-14): VCC -> 5V (EXT1 pin 19; the Olimex 5V
pin is only powered from USB), GND -> GND, BCK -> GPIO27 (EXT1 pin 11), LRCK -> GPIO14 (EXT1 pin
12), DATA -> GPIO13 (EXT1 pin 15). EC11 encoder: A GPIO32 (EXT1 7), B GPIO33 (EXT1 8), C GND,
switch GPIO25 (EXT1 9) and GND.

Former displays (Robot LCD, e-paper) used the same CS/DC/RST/BL pins; the e-paper had BUSY on
GPIO4, the Robot LCD its SD chip select there.
Avoid GPIO16/17 (PSRAM on WROVER),
GPIO6-11 (flash), GPIO0/12/15 (strapping), GPIO1/3 (USB serial). GPIO2 (strapping, low at boot) is
the MAX98357A speaker amplifier's SD (shutdown) since v1.37 (was GPIO19, now MISO).
MAX98357A: LRC -> GPIO14, BCLK -> GPIO27, DIN -> GPIO13 (parallel to the DAC), GAIN open (9 dB),
SD -> GPIO2 (EXT2 pin 15), GND, Vin -> 5V, speaker 4-8 ohm on its +/- (never to ground).

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
the Li-Po and feeds the radio (estimate 50-60 h together with the 5000 mAh Li-Po). The Li-Po bridges a power
bank swap or auto-off (many banks switch off below ~50-100 mA; the radio draws 150-200 mA).
Hardware prerequisite (Olimex user manual, p. 8): the three SMT jumpers **BAT_PWR_E1, PWR_SENS_E1 and
BAT_SENS_E1 are open as shipped** - "Close them (solder the pads together) to enable both functions,
on pins GPI35 and GPI39". All three are soldered since 2026-10-06; `docs/olimex-jumpers-bottom.png`
shows where they are (bottom view, generated from the KiCad Rev.D layout). With open jumpers (or no
battery) the pins float (~0.3 V) and the firmware reports "no reading". While charging over USB the
reading is the charger voltage (~4.0-4.2 V), not the resting state of charge. First full run on
battery: the whole night + day (~14 h) until the 2500 mAh cell was empty; with the 5000 mAh cell
(2026-10-11) about twice that is expected, not measured yet.

## Open points
- Speaker: the MAX98357A plays (wired and heard on 2026-10-15, v1.37; the "Speaker" checkbox
  silences and restores it while the line output keeps playing, also verified).
  The PAM8403 mini board is a spare (analog, no shutdown pin). Details and wiring in
  `docs/speaker-output.md`. Maybe a separate output choice instead of the Bluetooth toggle. The
  KiCad schematic uses the MAX98357A since 2026-10-15.
- PCB (integrated board, `KiCad/07-integrated-board.md`): schematic Rev B done 2026-10-15 (two
  sheets, ERC and netlist checked), reviewed the same evening by Grok and Claude
  (`grok-bevindingen.md`, `claude-bevindingen.md` in the repository root) and the agreed changes
  worked in. The board must be powered from a USB source of at least 1.5 A (owner's choice: the
  charger runs in ILIM mode, which has no input-voltage regulation for weak PC ports). First
  layout done the same night (80 x 60 mm, 4 layers, autorouted, DRC 0 errors / 0 unconnected / 0
  parity issues), reviewed by Grok and Claude, hand edits applied. Still open
  (`KiCad/esp-iradio/NOTES.md`, "Layout"): a last look at the routing by the owner,
  PJ-320D pin order and MSK12C02 ON position on samples, short-shaft EC11, display FPC pin 1 and
  backlight resistor from the real panel, then JLCPCB BOM/CPL and STEP/DXF for the case. The
  firmware needs a few changes for that board (`PWR_CAL_PERMILLE` ~2500, charge state on
  GPIO34/36, backlight PWM >= 20 kHz, a 16 MB partition table); pins stay.
- 1.69" ST7789: judge the normal layout near the rounded corners on the real panel (photo);
  the space under the battery line is still free (e.g. a volume or buffer bar). The EC11 rotary
  encoder works (volume, mute, station chooser; `src/encoder.*`); maybe a settings menu later; see
  `docs/display-research.md`. Backlight dimming, night mode and idle dimming are done; the
  encoder should also count as activity (`displayWake()`).
- SD card (done 2026-10-10: log, measurements, settings backup, logos, firmware copies; history
  of events, station statistics and crash dumps since 2026-10-15). The user does not want music
  files on it. Card reader again since 2026-10-14 (separate breakout). Possible next: song
  history ("wat was dat liedje?") and favourites, `stations.txt` import from the card.
- Repeat the battery run (now with the 5000 mAh cell; the SD measurement files will show the
  discharge curve once the card reader is in) with the power guard in place and read `/api/status` `diag` (underruns,
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
