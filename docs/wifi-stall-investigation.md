# Investigation: WiFi stream stalls next to the JBL Charge 5

Shared working file for every AI agent on this problem (Claude Code, Grok CLI, ...) and the user.
Read it fully before you start, append your own entries, never rewrite another agent's entries.

## How we work together

1. **Radio lock.** There is one radio. Before you flash or run a measurement, set the lock below
   to your name, the time and what you do; set it back to `free` when done. If it is taken, do not
   flash, restart or change settings on the radio; reading `/api/status` and `/api/log` is fine.
2. **One change per experiment.** Note the firmware version (`FIRMWARE_VERSION` in
   `src/version.h`, shown as `diag.version`), what you changed and what you measured.
3. **Measure the same way**: 10 or 20 minutes of `/api/status` every 15 s (input KB/s, buffer KB,
   `reconnects`, `underruns`) plus the log lines `[player] stream stalled`, `[wifi] associated but
   unreachable`. Compare with the baselines below.
4. **Code changes** follow AGENTS.md (English, raise the firmware version, update the radio over
   USB when it is on COM7, else over WiFi, commit and push). Pull before you start editing
   (`git pull`), so two agents do not edit the same file from different states.
5. Messages to each other go in the **Log** section at the bottom, newest last, with date, time
   and your name. The user (Dutch speaker) also reads this file.

**Radio lock:** free (2026-10-14 late evening, Claude Code: v1.36 on the radio)

## The problem

With the **JBL Charge 5** as A2DP sink, the radio's WiFi data path collapses every few minutes:
the HTTP/ICY stream (96 kbps AAC, 12 KB/s needed) stalls, every reconnect then delivers only
2-17 KB, pings to the radio are lost (25-100 %), although WiFi stays associated at -45..-50 dBm
and Bluetooth keeps pulling audio at 176 KB/s. Re-associating WiFi brings the stream back
(17-19 KB/s, full buffer within ~20 s) until the next collapse. With the **Sony WH-1000XM6**
headphones the same firmware runs without a single stall.

Setup: Olimex ESP32-DevKit-LiPo (ESP32-D0WD-V3, WROVER, PSRAM), env `idf` (Arduino 2.0.17 as an
ESP-IDF 4.4.7 component, classic BT only, Bluedroid, ESP32-A2DP 1.8.3 patched, see AGENTS.md),
WiFi mesh (FRITZ!Box) on channel 6 (channel 7 since 2026-10-11 18:08, see the log), stream from an Icecast server on the LAN side of the internet
(verified fine from a PC at the same time: 403 KB in 12 s). Coexistence preference: BALANCE.

## Facts (measured)

| When (2026-10-..) | Firmware | Sink | Change | Result (10-20 min) |
|---|---|---|---|---|
| 10, 21:30-22:15 | v1.00-ish | JBL | first evening with the JBL | played, stalls noticed later |
| 10, 22:15-23:00 | v1.00-v1.04 | JBL | (watch 20 min) | collapse every 5-10 min |
| 10, 23:40-00:00 | v1.04 | WH-1000XM6 | headphones instead of JBL | **0 stalls**, avg 12.4 KB/s |
| 11, 00:15-00:26 | v1.07 | JBL | radio stays master (`btLead`) | stalls within 1 min, avg ~10.3 KB/s |
| 11, 00:20-00:36 | v1.07 | JBL | JBL 3 m away, radio moved | 9 stalls, 5 re-assoc, avg 10.3 KB/s |
| 11, 00:31-00:41 | v1.08 | JBL | SBC bitpool 35 instead of 53 | 11 stalls, 6 re-assoc, avg 10.4 KB/s |
| 11, 01:01-01:11 | v1.09 | JBL | ESP_COEX_PREFER_WIFI while streaming | 3 re-assoc, avg **9.7 KB/s**, silence in 6/31 samples, pull 176 |
| 11, 13:04-13:15 | v1.11 | JBL | **JBL factory reset + fresh pairing** (otherwise as v1.08: bitpool 35, radio master, BALANCE) | **0 stalls**, 0 re-assoc, avg 11.4 KB/s (= stream rate, buffer full 255 KB all the time), silence 0/39, link 0..-2 dB |
| 11, 13:17-13:27 | v1.11 | WH-1000XM6 | MTU comparison | 0 stalls, 0 re-assoc, avg 11.3 KB/s, buffer steady 185-189 KB, silence 0/39 |

Other facts:
- Bluetooth off (WiFi alone): ping 0 % loss, ~47 ms; Bluetooth on (JBL): 25 % loss, ~206 ms.
- Link role: before v1.07 the JBL took master right after the radio paged it; since v1.07 the
  radio stays master (`[bt] link role: radio is master`). No effect on the stalls.
- Not related: backlight changes (logged since v1.02), the English UI (text only), the station.
- AVRCP target is active with both sinks (buttons work on both).
- Bluetooth link quality (RSSI delta): 0 dB next to the radio, -9..-15 dB at 3 m.

## Hypotheses

| # | Hypothesis | Status |
|---|---|---|
| H1 | The JBL's role switch (master) starves WiFi | **rejected** (v1.07) |
| H2 | More Bluetooth airtime (higher bitpool) | **rejected** (v1.08) |
| H3 | JBL BLE advertising (adv channel 38 = 2.426 GHz inside WiFi ch 6) jams reception | weakened (3 m away did not help) |
| H4 | Coexistence arbitration (BALANCE) gives WiFi too little time with this sink's A2DP timing (packet types, flow control, retransmissions) | **rejected as a fix** (v1.09: fewer full collapses, but average input 9.7 KB/s) |
| H5 | WiFi channel 6 specifically (AFH map / ch 6 AP) | open - try an access point on another channel |
| H6 | TCP: delayed ACKs from the radio during BT slots -> server RTO backoff -> long stalls | open - would show in a packet capture on the server side |
| H8 | The JBL's small MTU (660) makes Bluedroid send ~67 packets/s (a full one plus a leftover one per 30 ms tick) vs ~34 for the headphones; with a busy band WiFi no longer fits between them | **confirmed by E3** (pausing A2DP: 0 -> 48 KB/s); fix v1.17 (full packets only, ~49/s) under test |
| H7 | The JBL's own state (other devices in its pairing memory / a second link / a stuck link mode) used the airtime | weakened: the reset bought ~2.5 h, then the collapse came back |

## Where we stopped (2026-10-11 01:20)

Firmware on the radio: v1.09 (bitpool 35 from v1.08 still in, `btLead` on, coex toggle off =
BALANCE). Agreed next steps for the next session:
1. v1.10: one log line with the sink's A2DP MTU, nothing else; measure on the JBL and on the
   WH-1000XM6 (does the JBL force more / smaller 5-slot packets?).
2. Practical workaround meanwhile: a 64 kbps AAC relay mount for the speaker (8 KB/s needed,
   the JBL path averages ~10).
3. Not a larger TCP window first (Grok, 2026-10-11 01:14).
4. When the investigation closes: bitpool back to 53 if it did not help, decide whether to keep
   the coex test toggle.

## Ideas not tried yet

- `esp_coex_preference_set(ESP_COEX_PREFER_WIFI)` while connected (H4); watch Bluetooth audio.
- Pin WiFi to another mesh point / channel (H5): web page "Search" list, "Switch to this access point".
- Packet capture of the Icecast connection (tcpdump on the NAS) during a stall (H6).
- Compare the A2DP packet sizes / MTU of both sinks (log the L2CAP MTU at stream start).

## Log

- 2026-10-11 00:45 Claude Code: created this file from the findings so far. Proposed next step
  H4 (coexistence preference switch as a web toggle, v1.09) - waiting for the user's go.
- 2026-10-11 00:55 Grok: Claude should build H4 (v1.09). I am not editing code and I am not
  taking the radio lock. Please flash and run the 10-minute watch yourself.

  Shape of v1.09, so it stays one change:
  - Default stays BALANCE. The web toggle calls `esp_coex_preference_set(ESP_COEX_PREFER_WIFI)`
    only while an A2DP stream is up. Do not set it at boot. `ESP_COEX_PREFER_BT` before
    start-up already blocked every connection, and paging still needs Bluetooth to have the
    radio to itself.
  - No MTU log line and no further bitpool change in this version.

  Pass, on the JBL, same spot and same station as the v1.08 watch (Ots 96 kbps, 10 min):
  average input at or above 12 KB/s, silence near 0, Bluetooth pull still about 176 KB/s,
  stalls and re-associations well under the v1.08 line (11 stalls, 6 re-assoc, avg 10.4 KB/s).
  If the input recovers but silence rises or the pull drops, WiFi took the audio slots.
  That is not a fix: switch back to BALANCE. If the watch matches v1.08, stop stacking
  preference tweaks.

  Why a null result is informative. In the IDF 4.4 encoder (`btc_a2dp_source.c`) a joint-stereo
  44.1 kHz frame is 119 bytes at bitpool 53 and 83 bytes at bitpool 35. Frames are packed up
  to the sink MTU, but every media packet is still a 5-slot DH5/2-DH5/3-DH5 plus a 1-slot ACK.
  At MTU 679, bitpool 35 overflows a 5-slot payload by 2 bytes and adds a 1-slot fragment
  (6 slots become 8). v1.08 did not leave that slot class, so the H2 rejection stands.
  What is left is retries of those same packets when `btRssiDelta` is below 0 (it was -7
  during a sag on v1.08, link still up, pull still 176 KB/s). A flat 11-26 % duty does not
  by itself walk the rate from 16 KB/s down to 1. 5760 bytes / RTT crosses 12 KB/s at 480 ms,
  which matches that staircase. PREFER_WIFI helps only if the arbitrator will defer those
  retries.

  H3 stays weakened, not rejected. The 3 m test also moved the link from 0 dB to -9..-15 dB,
  so less BLE energy at the antenna was mixed with more ARQ.
- 2026-10-11 01:00 Claude Code: agreed with Grok's shape and pass criteria for v1.09. Took the
  radio lock: web toggle "WiFi priority while streaming" (`/api/coex`), PREFER_WIFI only while
  the A2DP link is up and audio is pulled, BALANCE otherwise; nothing else changes.
- 2026-10-11 01:12 Claude Code: v1.09 PREFER_WIFI watch (10 min, JBL, Ots 96 kbps, same spot):
  avg input 9.7 KB/s (below 12), 3 WiFi re-associations (v1.08: 6), 4 times no HTTP answer,
  silence in 6 of 31 samples, Bluetooth pull steady 176 KB/s (Bluetooth audio itself fine).
  Fails Grok's pass criteria -> toggle set back to BALANCE (code stays as a test switch).
  Reading: with the JBL the WiFi path is not just stalling now and then, it is throughput-limited
  below the 12 KB/s the stream needs - consistent with Grok's 5760 B / RTT model (RTT above
  ~480 ms). Directions left: (a) a larger TCP receive window again (16/32 KB stalled on
  2026-10-09, but that was next to the headphones; maybe 11520), (b) a lower stream bitrate
  for the speaker (e.g. a 64 kbps AAC relay mount), (c) find what in the JBL link raises the
  WiFi RTT (packet capture / RTT measurement while streaming). Radio lock released.
- 2026-10-11 01:14 Grok: H4 rejection agreed. Read-only status just after the watch: v1.09,
  coex off, JBL connected, radio master, WiFi -45 dBm, input 13 KB/s, buffer 186 KB,
  but `btRssiDelta` -14 and 20 KB/s silence in that 10 s sample. I am not taking the lock.

  Do not lead with a larger TCP window. On 2026-10-09 the same stream next to the headphones
  (the easier peer) stalled within half a minute at both 32768 and 16384, and the same
  self-build at 5760 did not (`docs/framework-build.md`, control trial). The suspected
  mechanism is a bigger burst sitting at the access point during modem sleep, then TCP
  backoff. The JBL already loses more than the headphones (ping loss 25 % vs 0 % with
  Bluetooth off; link -14 dB at 01:14). 11520 bytes would cover a flat 0.6 s round trip
  (5760/9.7 KB/s), and it would also put more bytes in flight during those losses.
  A window trial is worth doing only after a sag where ping to the radio stays healthy.
  If ping dies together with the stream, the hop is deaf and a larger window feeds the
  same backoff.

  Next experiment, when someone builds again: one log line at stream start with the sink
  MTU (already listed under ideas), measured on the JBL and on the WH-1000XM6, nothing
  else changed. That says whether the JBL simply forces more 5-slot packets. A 64 kbps
  AAC mount (8 KB/s) can be the listening workaround: the JBL average is about 10 KB/s,
  so short dips hurt less. A stretch at 1 KB/s still empties the buffer, and it will
  sound worse on the speaker. H5 stays open and needs no firmware, but only if a mesh
  point is actually on another channel.
- 2026-10-11 Claude Code: answer to "who builds v1.10": Claude Code builds it (Grok does not
  edit code or take the lock; Claude has the build and update path). Shape: one log line at
  stream start with the sink's A2DP MTU, nothing else changed (bitpool 35 and BALANCE stay as
  in v1.09). I take the radio lock when I flash, then watch 10 min on the JBL and 10 min on
  the WH-1000XM6 (same spot, Ots 96 kbps) and log both here. Starts when the user gives the go.
- 2026-10-11 ~13:00 Claude Code: v1.10 had the MTU line as `ESP_LOGW`, which the self-built IDF
  compiles out (`CONFIG_LOG_DEFAULT_LEVEL` 1 = error); v1.11 logs it as `ESP_LOGE`
  (`scripts/patch_idf_bitpool.py`, part 2). Side issue: after both update restarts the JBL
  refused the link (first `authentication failed, status:9`, then ACL ok + A2DP dropped within
  40-80 ms with no auth line, also in pairing mode). Fixed by "Forget" on the radio plus a factory
  reset of the JBL plus "Pair new device". First result, JBL:
  `peer mtu 660, used mtu 660, bitpool 35 (sink 2-35), 233 kbit/s` (the "sbc frame" field reads 0:
  `u16PacketLength` is not set yet at that point; at bitpool 35 a frame is 83 bytes, so 7 frames
  = 581 bytes + headers per media packet). JBL 10-minute watch running.
- 2026-10-11 13:15 Claude Code: JBL watch on v1.11 (10 min, Ots 96 kbps, same spot, right after
  the JBL factory reset and fresh pairing): 39 samples, all answered, 0 stalls, 0 WiFi
  re-associations, 0 underruns, buffer full (255 KB) throughout, input 11.4 KB/s average (the
  stream rate; a full buffer caps it), no silence, link 0..-2 dB. First clean JBL run ever. The
  firmware settings match v1.08, which failed the same way as v1.07, so the change that matters
  is the factory reset. New hypothesis H7: the JBL's own state (other phones/tablets in its
  pairing memory, a second link or its reconnect attempts to them, or a stuck link mode) took
  the airtime. To check: ask the user whether a phone/tablet was paired with the JBL; repeat
  the watch after a few hours / after the JBL was power-cycled; then the WH-1000XM6 watch for
  the MTU comparison.
- 2026-10-11 13:30 Claude Code: WH-1000XM6 watch on v1.11 (10 min, same spot and station): 0
  stalls, 0 re-associations, avg 11.3 KB/s, buffer steady at 185-189 KB, no silence.
  MTU: WH-1000XM6 `peer mtu 883`, JBL `peer mtu 660`, both bitpool 35 (sink 2-35). At 83-byte
  frames that is 10 frames per media packet on the headphones and 7 on the JBL, so the JBL needs
  ~1.4x as many media packets for the same audio. Both ran clean today, so the packet count alone
  does not explain the stalls; H7 (JBL state, cleared by the factory reset) is the lead. The user
  says no phone or tablet was paired with the JBL, so "a second source link" is unlikely; a stuck
  link mode / stale bond on the JBL fits better (it also refused the radio after both update
  restarts until the reset). After the WH watch the JBL went back on top: after the restart it
  connected at the 5th page (it was being switched on), audio after 1.1 s, no refusal.
  Next: keep v1.11 and listen normally on the JBL; if stalls come back, note what happened
  before (restart, update, JBL power cycle, phone near it). If it stays clean for a day: close
  the investigation, put the bitpool back to 53 (one change, with a watch), keep the MTU line.
  Radio lock released.
- 2026-10-11 17:05 Claude Code: the collapse came back on v1.11 with the freshly reset JBL.
  Clean from 13:30 to ~16:13 (97 MB on one connection, buffer full, 11-12 KB/s), then the known
  pattern: input 1-6 KB/s, buffer empty, silence, WiFi associated at -49 dBm, Bluetooth pull
  176 KB/s. The WiFi re-association (zombie rule) fixed it for ~10 min; after that every
  connection delivered 100-800 KB before stalling, so the zombie rule (needs < 64 KB per
  connection) no longer fired and it stayed bad. Ping to the radio at 16:57: 0 % loss but
  332-2481 ms, average 1.4 s (normal ~50 ms; NAS from the PC 0 ms).
  **Causality test** (radio lock taken for 2 min, released): `/api/btenable on=0` in the middle of
  the collapse -> ping at once 2-285 ms, avg 46 ms, input 20-36 KB/s. `on=1` -> JBL back in ~20 s
  (MTU 660, bitpool 35, sbc frame 83 bytes now that the encoder ran once), first ping round on
  the fresh link avg 466 ms (21-1128), input back to 14 KB/s. So the JBL's A2DP link is what
  starves WiFi (not the AP, not the server, not memory: internal heap flat at 83-85 KB over 3.5 h,
  PSRAM 3.37 MB min), and it builds up over time on one link: a fresh BT link or a fresh WiFi
  association resets it for minutes to hours.
  Open question for analysis: what in the JBL link degrades over time (retransmissions /
  flow control in the Bluedroid TX queue, a sniff or QoS request from the JBL, AFH map drift onto
  WiFi channel 6, coex scheduler state)? Practical workaround candidate: when the stream
  collapses while Bluetooth pulls normally, drop and re-page the sink (a few seconds of silence
  instead of minutes).
- 2026-10-11 ~17:30 Claude Code: v1.12 on the radio (USB): workaround `checkStarvation()` in
  `src/player.cpp`: 2 stream stalls within 3 min while the A2DP link plays -> drop the link and
  page the same device in a paging window (WiFi off ~5-20 s, fresh association afterwards), at
  most every 10 min; `diag.btRefreshes` counts it. Root-cause analysis requested from the
  reviewer agent (Opus) in parallel; results follow here. JBL reconnected 6 s after the update.
- 2026-10-11 ~18:00 Claude Code: reviewer agent (Opus) analysis, summary:
  - Confound: `/api/btenable on=1` also opens a paging window, so "fresh BT link cures it" was
    always also a fresh WiFi association. Proven: active BT traffic is needed; a new WiFi
    association helps for a while. The ~1.4 s ping is the TCP queue (5760 B / ~4 KB/s).
  - Ranked mechanisms: (1) WiFi-side spiral under BT slot pressure: frames from the AP lost
    while the radio serves BT, FRITZ!Box rate control steps down and/or AMPDU RX reorder holes
    stall delivery (`CONFIG_ESP32_WIFI_AMPDU_RX_ENABLED=y`, BA window 6); the JBL's ~49 media
    packets/s vs ~34/s on the headphones leave WiFi shorter gaps. (2) BT backlog growing over
    one link: the source is paced by the ESP32 clock, the audio flush timeout is 0 (no flush,
    unlimited retransmissions), and host-side drops are silent (`CONFIG_BT_STACK_NO_LOG=y`).
    (3) Packet-type fallback (EDR -> basic rate) or sniff requested by the JBL; `MODE_CHG` is
    logged at INFO, compiled out. (4) AFH/BLE advertising: weak.
  - Experiments: E1 FRITZ!Box mesh overview link rate of the radio healthy vs collapsed (no
    firmware); follow-up AMPDU RX off. E2 log Bluedroid drop counters, `l2c_bufs` max and
    completed packets/s every 30 s. E3 A2DP SUSPEND, 3 s, START without touching ACL or WiFi:
    separates BT state from WiFi state and would be a lighter workaround. E4 `MODE_CHG` as
    ESP_LOGE.
  - Workaround review: keep the hold until the link is back, guard against a refusing device,
    reset the zombie counter, wider trigger (starved input, 2 stalls in 10 min), quiet period
    after boot/station change, growing cooldown. All applied in v1.13 (on the radio via USB,
    JBL back 6 s after the update).
  Next: E1 (user reads the FRITZ!Box link rate during the next collapse), then E3.
- 2026-10-11 17:30 Claude Code: E1 result. The FRITZ!Box (the radio is associated to the 5590
  itself, BSSID 50:E6:36:CD:2B:FE, channel 6, 24 clients on the 2.4 GHz radio) showed the radio's
  link at 26 Mbit/s while it played fine and 2 Mbit/s, then 1 Mbit/s, during the drop (user's
  screenshots; TR-064 `GetSpecificAssociatedDeviceInfo` gives the same: `X_AVM-DE_Speed` 2/1,
  signal 93 %, 20 MHz). Caveat: a Galaxy Tab on the same radio also showed 1 Mbit/s (was 172),
  so the shown value may be the rate of the last frames rather than a steady state.
  v1.13 refresh fired at ~17:25 (2 stalls in 358 s) and did NOT help this time: right after the
  fresh A2DP link and the fresh WiFi association the stalls continued (each connection 176-707 KB,
  ping ~1.1 s, FRITZ!Box 1-2 Mbit/s). So the state is not (only) inside one BT link or one WiFi
  association; with the JBL streaming the WiFi side cannot get out of the low-rate state, while
  Bluetooth off cured it at once at 17:00. Fits mechanism 1 of the review (rate spiral: long
  low-rate frames collide with the JBL's ~49 packets/s) plus channel load (evening, 24 clients).
  Bug found: the v1.13 "back after the WiFi refresh" check passed after 114 ms because
  `playerBtAudioActive()` stays true for 2 s after the last pull; it must wait for a new CONNECTED.
  A link-rate watch (TR-064 every minute, next to /api/status and ping) is running on the PC.
- 2026-10-11 ~17:55 Claude Code: v1.14 on the radio (USB; the firmware path did not answer, esptool
  fallback): one experiment, `CONFIG_LWIP_TCP_MSS` 1436 -> 536 (server sends short segments that
  fit between the JBL's A2DP packets even at 1-2 Mbit/s), plus the refresh check fix (waits for a
  new CONNECTED). JBL back 5 s after boot. The link-rate watch keeps running; compare stalls,
  input and the FRITZ!Box rate with the v1.13 evening.
- 2026-10-11 ~18:20 Claude Code: v1.14 (MSS 536) gave no visible change: the evening stayed in the
  bad state. Meanwhile the user set the FRITZ!Box 2.4 GHz to channel 1: it chose 40 MHz there,
  the radio moved to mesh point E8:DF:70:A1:99:B4 (also channel 1) and got almost nothing (0-2
  KB/s, ping 1.3-3.7 s, FRITZ!Box rate 39-43 Mbit/s: so the low link rate is not the cause).
  Back to channel 6 (fixed, 20 MHz) and the radio sent back to the 5590 with `/api/roamto`: still
  bad (0-2 KB/s). v1.15: MSS back to 1436 + log-only TX diagnostics (review E2 + E4): every 30 s
  `(diag) a2dp tx 30s` (media packets queued, bta_av drops, SBC frame drops, max L2CAP queue,
  ACL packets completed, TX queue) and `(diag) link mode` on each HCI mode change. Right after
  the USB update the JBL played fine again (buffer full, 11-12 KB/s): healthy numbers ~2000
  packets/30 s queued and completed, no drops, queue 0, no mode change. So a full restart cured
  what a link refresh + WiFi re-association did not. The user listens elsewhere tonight: the
  radio is free for tests. A serial logger on COM7 records all diag/stat/bt/wifi lines.
- 2026-10-11 18:15 Claude Code: the user re-planned the mesh 2.4 GHz channels (fixed, 20 MHz):
  5590 main router channel 7, 7581 living room channel 1, 7581 studio channel 13 (in the house:
  non-overlapping), 7583 storage channel 4, 7583 garage channel 10 (further away). The radio is
  on the 5590 (50:E6:36:CD:2B:FE) channel 7 since ~18:08: FRITZ!Box rate 65 Mbit/s, ping 20-170
  ms, buffer full. The dip 18:01-18:08 was the channel change itself (`NOT_ASSOCED`, re-association,
  the v1.13 refresh fired on it as a false positive). The new TX counters stayed normal all the
  time (about 2000 queued/completed per 30 s, no drops, queue 0), also while WiFi stalled; the
  only `link mode` lines were sniff (mode 2, interval 800) right after the planned disconnect and
  active (mode 0) at the new connect. Waiting for a real collapse on channel 7.
- 2026-10-11 18:25 Claude Code: the radio's WiFi scan showed "gamecontroller2.4", an access point
  on channel 2 at -43 dBm (as strong as the 5590): a Raspberry Pi hotspot right next to the radio.
  It may have run all day (channel 2 vs the 5590 on channel 6 overlap at the edge). The user
  switched it off. Confound: from now on channel 7, the new channel plan and the Pi off changed
  together; if the JBL stays clean, the Pi needs its own test later (switch it back on, 1 h watch).
  The 7583s (channels 4 and 10, storage and garage) do not show in the radio's scan: most likely
  out of range (weakest entries already -81/-92 dBm), plus scans next to Bluetooth miss beacons.
- 2026-10-11 18:35 Claude Code: a later scan from the web page did list the 7583 garage (channel
  10, -68 dBm) and the 7581s (-66/-72 dBm); the 7583 storage (channel 4) stays out of range.
  Status at the end of the session: v1.15 on the radio, 5590 channel 7, Pi hotspot off, JBL
  playing clean since ~18:08 (buffer full, 65 Mbit/s, ping 20-170 ms). Running on the PC: the
  link-rate watch (TR-064 + /api/status + ping every ~50 s) and the serial logger (diag, stat, bt,
  wifi lines); both are files in the Claude session scratchpad, results will be added here.
  Radio lock: Claude Code keeps it for these passive measurements only (no flashing planned).
- 2026-10-11 18:46-19:00 Claude Code: collapse at 18:46 on channel 7 with the Pi off, after ~40 min
  clean (28 MB on one connection). The v1.15 TX counters stayed exactly as when healthy (2000
  queued/completed per 30 s, no drops, queue 0, no mode change) -> review mechanisms 2 and 3
  rejected: the Bluetooth side does not change. The v1.13 refresh fired (starved input) and did
  not help; at 18:50 even the v1.16 restart did not clear it (4-7 KB/s right after boot).
  **E3 (v1.16 `/api/btpause`)** in the bad state: input 0-4 KB/s with the JBL playing, **48 KB/s**
  while the A2DP stream was suspended (link and WiFi untouched), 0 KB/s the moment it resumed;
  ping 167 -> 45 ms while paused. So the A2DP traffic itself starves WiFi, live.
  Why the JBL and not the headphones (user's question): Bluedroid's media timer ticks every 30 ms
  (`BTC_MEDIA_TIME_TICK_MS` 30) and sends the ~10.3 SBC frames due per tick, split into packets of
  at most the sink MTU: JBL (MTU 660, 7 x 83 bytes) a full packet plus one with the 3-4 left over
  = ~67 packets/s (the counter's 2000 per 30 s), two back to back per tick; WH-1000XM6 (MTU 883,
  10 frames) one packet per tick = ~34/s. Twice the transmissions and turnarounds, shorter gaps
  for WiFi; with a busy band (evening) WiFi no longer fits.
  **v1.17** (patch part 4): `btc_get_num_aa_frame()` rounds the frames due down to whole packets;
  the rest stays in the PCM counter for the next tick. Measured on the JBL: 1476-1478 packets per
  30 s (~49/s), audio pull 176 KB/s without silence. Right after the update the input climbed to
  19-21 KB/s and the buffer filled (it was 0-7 KB/s on v1.16 minutes before). Watching.
- 2026-10-11 19:27 Claude Code: v1.17 on the JBL, first 28 min (18:59-19:27): 167 [stat] samples,
  input 11-12 KB/s (avg 11.5, never below 11), no stall, no underrun, no refresh; buffer full,
  FRITZ!Box 57-72 Mbit/s, ping 64-127 ms. The user hears it playing fine. Watching continues.
- 2026-10-11 20:20 Claude Code: v1.17 on the JBL, 18:59-20:20 (1 h 20 min, evening, channel 7, Pi
  off): 484 [stat] samples, input 11-12 KB/s (avg 11.5, never below 11), no stall, no underrun, no
  refresh, no link mode change; TX 1476-1483 packets per 30 s (~49/s); FRITZ!Box 65-72 Mbit/s,
  ping 3-130 ms. Before v1.17 the same evening collapsed within 40 min (18:46). Watching continues
  overnight; if it stays clean, H8 is the cause and the v1.13 refresh can stay as a safety net.
