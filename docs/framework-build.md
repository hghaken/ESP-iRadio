# Own framework build: larger TCP window (plan, 2026-10-09)

## Why
Next to active Bluetooth the radio only takes in 9 to 16 KB/s. That is enough for My Radio
(AAC 96 kbps, 12 KB/s), but tight for 128 kbps stations (16 KB/s, for example Qmusic and Radio 10).
For 320 kbps (40 KB/s) it is far too little.

The cause is the TCP receive window. The prebuilt Arduino libraries fix it at
5760 bytes. The radio may then receive only 5760 bytes unacknowledged, and every round takes as long
as the ping time. That ping time averages 350 to 670 ms, with peaks up to 1.9 s, because WiFi sleeps
and shares the antenna with Bluetooth.

| Window | At 0.4 s ping | At 0.7 s ping |
|---|---|---|
| 5760 B (now) | 14 KB/s | 8 KB/s |
| 16384 B | 40 KB/s | 23 KB/s |
| 32768 B | 80 KB/s | 46 KB/s |

The window is set in the compiled lwIP library. There is no setting to change it at run time. It
can only be changed by building the framework yourself with an own `sdkconfig`.

## Approach
- PlatformIO builds Arduino 2.0.17 as a component of ESP-IDF 4.4.7. That is the same IDF version
  the prebuilt libraries were made with. The platform pins that combination itself.
- A new environment `idf` goes into `platformio.ini`, next to `radio`. The `radio` environment, the
  OTA update and the backups stay exactly as they are.
- `sdkconfig.defaults` is the complete sdkconfig of the prebuilt Arduino, with only the
  changes of phase 1. That way everything else behaves the same.
- ESP-IDF 4.4 cannot build in a folder with a space, and the project folder is called "_AI Projects".
  So a script mirrors the required files to `C:\pio\iradio-idf` and builds there:

  ```
  .\scripts\build_idf.ps1
  ```

  The firmware ends up in `C:\pio\iradio-idf\.pio\build\idf\firmware.bin`. The resolved
  configuration is copied back as `sdkconfig.idf` into the project, so it is in git.

- Uploading from `idf` is blocked for now, so that this build cannot end up on the radio by
  accident.

## Phase 1: only the window
| Setting | Was | Becomes | Why |
|---|---|---|---|
| `CONFIG_LWIP_TCP_WND_DEFAULT` | 5760 | 32768 | the actual goal |
| `CONFIG_LWIP_TCP_RECVMBOX_SIZE` | 6 | 32 | enough slots for the packets of a full window |
| `CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP` | off | on | the extra buffers in PSRAM, not in the scarce internal RAM |
| `CONFIG_BLE_MESH` | on | off | never used; replaces `src/bt_mesh_stubs.c` |
| `CONFIG_ESP32_REV_MIN` | 0 | 3 | the chip is rev 3.1; old workarounds drop out |

## Phase 2: later, one by one
- Classic Bluetooth only (`CONFIG_BTDM_CTRL_MODE_BR_EDR_ONLY`, BLE off). That frees internal RAM
  and saves ~130 KB flash. Do check that ESP32-A2DP still starts properly.
- SPP and HFP off, ~70 KB flash.
- The Bluedroid heap in PSRAM (`CONFIG_BT_ALLOCATION_FROM_SPIRAM_FIRST`).

## Status (2026-10-09): phase 1 builds
| | radio (now on the radio) | idf (phase 1) |
|---|---|---|
| Firmware | 1,661,161 bytes (81.8 %) | 1,636,289 bytes (80.5 %) |
| Static RAM | 62,984 bytes | 62,920 bytes |
| TCP window | 5760 | 32768 |

`sdkconfig.idf` was compared with the prebuilt Arduino sdkconfig. The only differences are the
intended changes and the consequences of chip revision 3: the PSRAM workarounds for old chips
drop out. In addition, settings of components we do not use are missing, such as camera,
RainMaker, Insights, LittleFS and DSP. Exceptions and the stack check are still on in this build,
as in the prebuilt libraries. Turning them off via sdkconfig will later save another ~50 KB.

What was needed to make it build (also in AGENTS.md):
- **Space in the path.** A `subst` drive or junction does not work: PlatformIO resolves it back to
  the real path, CMake does not, and then the source paths no longer match. Hence the mirror folder.
- **Project name.** PlatformIO generates its own `CMakeLists.txt` with the folder name as project
  name. That has a space, so there is now an own version with `project(esp_iradio)`.
- **`build_flags`.** In this mode they apply to all IDF components. So `idf` has its own
  flags without `-fno-exceptions`, because asio needs exceptions.
- **IDF's `-Werror=all`.** That turns harmless warnings in audio-tools into errors. It is
  switched off via `build_unflags`.
- **`build_src_filter`.** That does not work with IDF. `src/CMakeLists.txt` excludes the Mesh stubs.

## Test on the radio (2026-10-09, 17:04-17:10): phase 1 disappoints
My Radio, 4 minutes per build. Icecast had just received a burst of 256 KB and a queue of 1 MB.

| | idf phase 1 | radio build back (same Icecast) |
|---|---|---|
| Input per 10 s | 1 to 28 KB/s, often 3 to 5 | 7 to 20 KB/s, mostly 10 to 15 |
| Stalls (5 s of nothing) | 8 in 4 minutes | 0 |
| Buffer empty | 3 times | 0 times, buffer full (255 KB) |
| Free internal RAM (lowest) | 66 KB | 52 KB |
| Headphones | connects in 1.1 s, stays connected | same |

After every reconnect the burst came in quickly, and then the connection stalled within half a
minute. At the same time the PC got 25 KB/s from the same server, so the server is fine. The
radio build was put back (`backup/firmware/3-flash_c78bbed.bin`).

Possible causes, not yet tested:
1. A window of 32 KB lets the server send up to ~22 packets at once. While the ESP32 sleeps
   (modem sleep), the access point has to hold them. If packets get lost, TCP waits
   longer and longer before retrying. That fits stalls of 5 s.
2. `CONFIG_SPIRAM_TRY_ALLOCATE_WIFI_LWIP`: the WiFi receive buffers in PSRAM may be slower
   next to Bluetooth.

## Trial 1 (17:20-17:27): window 16384, buffers internal again - also bad
On My Radio no data came in for 5 s four times, in about 3 minutes. The buffer ran empty once, the
lowest free RAM was 61 KB, the input kept dropping to 3 to 4 KB/s. Then the headphones dropped.
On the stable build they stayed unreachable afterwards too (`stat=260`); so they were probably
switched off. Backup of this trial: `backup/firmware/5-idf-trial1-wnd16k.bin`.

Conclusion: a smaller window and internal buffers do not help. The pattern (fast first,
then nothing within half a minute) is the same in both trials. So it may not be
the window at all, but the self-built IDF itself.

Next trial, as a control: the same `idf` build with the old window of 5760 and otherwise
everything as in trial 1. If that stutters too, the problem is in the build itself and not in the
window. Suspects are then chip revision 3, which pulls in different code, or differences
in the WiFi and coexistence libraries. If that control plays stably, any larger
window is the problem next to Bluetooth and modem sleep. Then this route is a dead end.

## Control trial (17:53-18:03): self-build with the old window - stable
My Radio, 5 minutes with the headphones connected. Compared with the prebuilt build
(`3-flash`) from this afternoon:

| | self-build, window 5760 | prebuilt (`3-flash`) |
|---|---|---|
| Input per 10 s | 7 to 19 KB/s, mostly 11 to 13 | 7 to 20 KB/s |
| Stalls / buffer empty | 0 / 0 | 0 / 0 |
| Buffer | 206 to 255 KB, mostly full | same |
| Free internal RAM | 64 to 67 KB, lowest 61 KB | 56 to 57 KB, lowest 52 KB |
| Ping to the radio | average 177 ms | 339 to 666 ms earlier today |
| Firmware | 80.5 % | 81.8 % |

Conclusion:
- The self-build itself is fine. It plays as stably as the prebuilt version and has
  ~9 KB more free internal RAM.
- Any larger TCP window, 16 or 32 KB, gives stalls next to Bluetooth and modem sleep. That route
  to make 128 and 320 kbps stations more stable is therefore a dead end.
- The probable cause: with more data in flight, the access point loses packets while
  the radio sleeps. TCP then waits longer and longer before a new attempt. This is not proven.

The control build (`backup/firmware/6-idf-control-wnd5760.bin`) stays on the radio for now
as a soak test. Phase 2 (BLE off, SPP/HFP off, Bluedroid in PSRAM) can build on it
for more RAM and flash. That is a separate choice.

## Phase 2 (2026-10-09 evening)
| Step | Change | Flash | Free internal RAM (My Radio) | Result |
|---|---|---|---|---|
| control | self-build, old window | 80.5 % | ~66 KB, lowest 61 | stable |
| 2a | classic Bluetooth only, BLE off | 73.3 % | ~82 KB, lowest 77 | 5 min stable, connects in 1 s |
| 2b | also SPP and HFP off | 70.0 % | ~82 KB, lowest 77 | stable (second try) |
| **2c** | **Bluetooth heap in PSRAM first** | **70.1 %** | **~84 KB, lowest 80** | **stable, on the radio** |

On the first try with 2b the headphones dropped after 40 s. After that they stayed unreachable
even with 2a, although that build had connected in 1 s an hour earlier. So the headphones were off
or out of range. The repeat with diagnostic lines was stable for 5 minutes.

Each step: connected in ~1.1 s, 5 minutes without silence or link loss, buffer mostly full.
With every build there is one reconnect right after start-up, when WiFi comes back after the paging window.
Compared with the prebuilt build (81.8 % flash, lowest free RAM 52 KB), phase 2 yields
~240 KB flash and ~28 KB internal RAM. That is room for the new screen and the
speaker output.

Backups in `backup/firmware`: `7-idf-2a-bredr-only.bin`, `8-idf-2b-no-spp-hfp.bin`,
`9-idf-2b-retest-diag.bin` and `10-idf-2c-bt-psram.bin`. Git tag: `opt-5-idf-phase2`. The
diag lines (`stat=`, `disc_rsn=`) have been part of the patch script since the 2b repeat.

Open: make env `idf` the default (still `radio` now), after a longer soak test.

## Test plan
1. The build must succeed, and the firmware must fit in the OTA slot (1984 KB).
2. The sdkconfig that comes out (`sdkconfig.idf`) must really contain the new values.
3. Only then, after the user agrees: OTA to the radio with a backup ready. The radio
   stays reachable, because the OTA code and the partitions are the same.
4. Measure on the radio:
   - On My Radio: `[stat]` input, buffer, `minHeap` and whether the headphones stay connected.
   - On Qmusic (128 kbps) and `/320.mp3`: input and interruptions over 10 minutes.
   - The ping time to the radio.
5. If it disappoints: go back with the backup from `backup/firmware` (see AGENTS.md).

## Risks
- **Internal RAM.** A full window of 32 KB has to live somewhere. With lwIP in PSRAM that should not cost
  internal RAM, but that has to show in `minHeap`. If it goes wrong, try 16384 first.
- **WiFi next to Bluetooth.** More data per round means more airtime for WiFi. That can make the
  Bluetooth link tighter. The Bluetooth pull must stay at 176 KB/s, without silence.
- **Build time.** The whole framework is built from source. The first time that takes long,
  after that it is faster.
- **Rolling back.** That is easy: `radio` stays the default and the backups are ready. Only
  once `idf` is proven on the radio does it become the default.
