# Reapplies the A2DP source reconnect fix to the gitignored ESP32-A2DP 1.8.3 copy.
# pio pkg install restores the upstream file; the next build puts this back.
# Six patches, all in BluetoothA2DPSource.cpp (reconnect, RSSI, media start, diag lines,
# AVRCP target, stack-ready hook); the first two changes:
# - a connect attempt that is still open after two heartbeats (20 s) is cancelled
#   with esp_a2d_source_disconnect before any new page
# - the unconnected heartbeat does not page when auto-reconnect is off
Import("env")

from pathlib import Path

MARKER = "connect timeout, source disconnect"

OLD_UNCONNECTED = """  case BT_APP_HEART_BEAT_EVT: {
    esp_a2d_connect(peer_bd_addr);
    s_a2d_state = APP_AV_STATE_CONNECTING;
    s_connecting_heatbeat_count = 0;
    break;
  }"""

NEW_UNCONNECTED = """  case BT_APP_HEART_BEAT_EVT: {
    // Do not page when auto-reconnect is off, or when there is no stored peer.
    if (reconnect_status != AutoReconnect || !has_last_connection()) break;
    memcpy(peer_bd_addr, last_connection, ESP_BD_ADDR_LEN);
    esp_a2d_connect(peer_bd_addr);
    s_a2d_state = APP_AV_STATE_CONNECTING;
    s_connecting_heatbeat_count = 0;
    break;
  }"""

OLD_TIMEOUT = """    /**
     * Switch state to APP_AV_STATE_UNCONNECTED
     * when connecting lasts more than 2 heart beat intervals.
     */
    if (++s_connecting_heatbeat_count >= 2) {
      s_a2d_state = APP_AV_STATE_UNCONNECTED;
      s_connecting_heatbeat_count = 0;
    }"""

NEW_TIMEOUT = """    // Cancel the open page before a later heartbeat starts another one.
    if (++s_connecting_heatbeat_count >= 2) {
      s_connecting_heatbeat_count = 0;
      s_a2d_state = APP_AV_STATE_DISCONNECTING;
      esp_err_t err = esp_a2d_source_disconnect(peer_bd_addr);
      ESP_LOGW(BT_AV_TAG, "connect timeout, source disconnect -> %d", (int)err);
      if (err != ESP_OK) s_a2d_state = APP_AV_STATE_UNCONNECTED;
    }"""

OLD_DISCONNECTING = """  /* handle the events of intrest in disconnecing state */
  switch (event) {
  case ESP_A2D_CONNECTION_STATE_EVT: {
    a2d = (esp_a2d_cb_param_t *)(param);
    if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
      ESP_LOGI(BT_AV_TAG, "a2dp disconnected");
      s_a2d_state = APP_AV_STATE_UNCONNECTED;
    }
    break;
  }
  case ESP_A2D_AUDIO_STATE_EVT:
  case ESP_A2D_AUDIO_CFG_EVT:
  case ESP_A2D_MEDIA_CTRL_ACK_EVT:
  case BT_APP_HEART_BEAT_EVT:
    break;"""

NEW_DISCONNECTING = """  /* handle the events of intrest in disconnecing state */
  switch (event) {
  case ESP_A2D_CONNECTION_STATE_EVT: {
    a2d = (esp_a2d_cb_param_t *)(param);
    if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
      ESP_LOGI(BT_AV_TAG, "a2dp disconnected");
      s_a2d_state = APP_AV_STATE_UNCONNECTED;
    }
    break;
  }
  case ESP_A2D_AUDIO_STATE_EVT:
  case ESP_A2D_AUDIO_CFG_EVT:
  case ESP_A2D_MEDIA_CTRL_ACK_EVT:
    break;
  case BT_APP_HEART_BEAT_EVT:
    // Disconnect event never arrived. Allow a new page on a later beat.
    if (++s_connecting_heatbeat_count >= 2) {
      s_connecting_heatbeat_count = 0;
      s_a2d_state = APP_AV_STATE_UNCONNECTED;
      ESP_LOGW(BT_AV_TAG, "source disconnect timed out");
    }
    break;"""


# Third change: publish the link RSSI (relative to the golden range) that the
# controller reports after esp_bt_gap_read_rssi_delta(), for the display/diagnostics.
RSSI_MARKER = "g_bt_rssi_delta"

OLD_INCLUDE = '#include "BluetoothA2DPSource.h"\n'
NEW_INCLUDE = """#include "BluetoothA2DPSource.h"
// Patched in by scripts/patch_a2dp_reconnect.py: last RSSI delta, 127 = none yet.
volatile int8_t g_bt_rssi_delta = 127;
"""

OLD_MODE_CHG = """  case ESP_BT_GAP_MODE_CHG_EVT:
    ESP_LOGI(BT_AV_TAG, "ESP_BT_GAP_MODE_CHG_EVT mode:%d","""
NEW_MODE_CHG = """  case ESP_BT_GAP_READ_RSSI_DELTA_EVT:
    if (param->read_rssi_delta.stat == ESP_BT_STATUS_SUCCESS)
      g_bt_rssi_delta = param->read_rssi_delta.rssi_delta;
    break;

  case ESP_BT_GAP_MODE_CHG_EVT:
    ESP_LOGI(BT_AV_TAG, "ESP_BT_GAP_MODE_CHG_EVT mode:%d","""


# Third patch: the upstream state machine only starts the media stream on the next
# heartbeat tick (every 10 s). In that silent gap WiFi returns and the fresh link drops
# with disc_rsn=1 (seen three times in a row, 2026-10-09). Ask for the stream at once.
START_MARKER = "// Start the media stream at once (patched)"
OLD_CONNECTED = """      ESP_LOGI(BT_AV_TAG, "a2dp connected");
      s_a2d_state = APP_AV_STATE_CONNECTED;
      s_media_state = APP_AV_MEDIA_STATE_IDLE;
"""
NEW_CONNECTED = """      ESP_LOGI(BT_AV_TAG, "a2dp connected");
      s_a2d_state = APP_AV_STATE_CONNECTED;
      s_media_state = APP_AV_MEDIA_STATE_IDLE;
      // Start the media stream at once (patched): do not wait up to 10 s for the
      // next heartbeat; the ACK arrives in the connected state and starts it.
      esp_a2d_media_ctrl(ESP_A2D_MEDIA_CTRL_CHECK_SRC_RDY);
"""

# Fourth patch: diagnostics. The ACL result code (256 = ok, 260 = page timeout) and the
# A2DP disconnect reason (0 = normal, 1 = link loss) are the first things to read when the
# headphones do not connect. Upstream logs them without the values, at INFO/DEBUG level
# (invisible at CORE_DEBUG_LEVEL 2). These lines used to be added by hand.
DIAG_MARKER = "(diag)"
OLD_ACL_LOG = """    ESP_LOGI( BT_AV_TAG,"ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT");"""
NEW_ACL_LOG = """    ESP_LOGW( BT_AV_TAG,"ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT stat=%d handle=%d (diag)", (int)param->acl_conn_cmpl_stat.stat, (int)param->acl_conn_cmpl_stat.handle);"""
OLD_STATE_LOG = """    ESP_LOGD(BT_AV_TAG, "%s ESP_A2D_CONNECTION_STATE_EVT: %s", __func__,
             to_str(a2d->conn_stat.state));"""
NEW_STATE_LOG = """    ESP_LOGW(BT_AV_TAG, "%s ESP_A2D_CONNECTION_STATE_EVT: %s disc_rsn=%d (diag)", __func__,
             to_str(a2d->conn_stat.state), (int)a2d->conn_stat.disc_rsn);"""


# Fifth patch: AVRCP target, so the buttons of the speaker / headphones reach the radio
# (play/pause, next, previous). Upstream only starts the AVRCP controller. The handler is
# radio_avrc_tg_cb() in src/player.cpp (declared here at block scope, C++ linkage).
AVRC_MARKER = "avrc tg init"  # v2: with init diagnostics
AVRC_V1 = """    void radio_avrc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);
    esp_avrc_tg_init();
    esp_avrc_tg_register_callback(radio_avrc_tg_cb);
    {
      esp_avrc_psth_bit_mask_t cmd_set = {0};
      esp_avrc_tg_get_psth_cmd_filter(ESP_AVRC_PSTH_FILTER_ALLOWED_CMD, &cmd_set);
      esp_avrc_tg_set_psth_cmd_filter(ESP_AVRC_PSTH_FILTER_SUPPORTED_CMD, &cmd_set);
    }
"""
OLD_AVRC = """    // initialize AVRCP controller
    esp_avrc_ct_init();
    esp_avrc_ct_register_callback(ccall_bt_app_rc_ct_cb);
"""
NEW_AVRC = """    // initialize AVRCP controller
    esp_avrc_ct_init();
    esp_avrc_ct_register_callback(ccall_bt_app_rc_ct_cb);

    // AVRCP target (patched): receive the sink's buttons. Handled in src/player.cpp.
    void radio_avrc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);
    {
      esp_err_t e1 = esp_avrc_tg_init();
      esp_err_t e2 = esp_avrc_tg_register_callback(radio_avrc_tg_cb);
      esp_avrc_psth_bit_mask_t cmd_set = {0};
      esp_err_t e3 = esp_avrc_tg_get_psth_cmd_filter(ESP_AVRC_PSTH_FILTER_ALLOWED_CMD, &cmd_set);
      esp_err_t e4 = esp_avrc_tg_set_psth_cmd_filter(ESP_AVRC_PSTH_FILTER_SUPPORTED_CMD, &cmd_set);
      ESP_LOGW(BT_AV_TAG, "avrc tg init %d %d %d %d, cmds %08x %08x (diag)", e1, e2, e3, e4,
               (unsigned)cmd_set.bits[0], (unsigned)cmd_set.bits[1]);
    }
"""


# Sixth patch: a hook into the radio once the stack is up and before the first page
# (radio_bt_stack_ready() in src/player.cpp: link policy / role switch setting).
READY_MARKER = "radio_bt_stack_ready"
OLD_READY = """      ESP_LOGW(BT_AV_TAG, "avrc tg init %d %d %d %d, cmds %08x %08x (diag)", e1, e2, e3, e4,
               (unsigned)cmd_set.bits[0], (unsigned)cmd_set.bits[1]);
    }
"""
NEW_READY = """      ESP_LOGW(BT_AV_TAG, "avrc tg init %d %d %d %d, cmds %08x %08x (diag)", e1, e2, e3, e4,
               (unsigned)cmd_set.bits[0], (unsigned)cmd_set.bits[1]);
    }
    // Patched: the radio's own Bluetooth setup, before the first page (src/player.cpp).
    void radio_bt_stack_ready();
    radio_bt_stack_ready();
"""


def lf(text):
    return text.replace("\r\n", "\n")


def replace_once(text, old, new, path):
    old, new = lf(old), lf(new)
    found = text.count(old)
    if found != 1:
        raise SystemExit("patch_a2dp_reconnect: expected 1 match in %s, found %d" % (path, found))
    return text.replace(old, new, 1)


def patch(path):
    raw = path.read_bytes()
    crlf = b"\r\n" in raw
    text = lf(raw.decode("utf-8"))
    changed = False
    if MARKER in text:
        print("A2DP reconnect patch already in %s" % path)
    else:
        text = replace_once(text, OLD_UNCONNECTED, NEW_UNCONNECTED, path)
        text = replace_once(text, OLD_TIMEOUT, NEW_TIMEOUT, path)
        text = replace_once(text, OLD_DISCONNECTING, NEW_DISCONNECTING, path)
        if MARKER not in text:
            raise SystemExit("patch_a2dp_reconnect: marker missing after edit of %s" % path)
        changed = True
        print("A2DP reconnect patch applied to %s" % path)
    if RSSI_MARKER in text:
        print("A2DP RSSI patch already in %s" % path)
    else:
        text = replace_once(text, OLD_INCLUDE, NEW_INCLUDE, path)
        text = replace_once(text, OLD_MODE_CHG, NEW_MODE_CHG, path)
        changed = True
        print("A2DP RSSI patch applied to %s" % path)
    if START_MARKER in text:
        print("A2DP media-start patch already in %s" % path)
    else:
        text = replace_once(text, OLD_CONNECTED, NEW_CONNECTED, path)
        changed = True
        print("A2DP media-start patch applied to %s" % path)
    if DIAG_MARKER in text:
        print("A2DP diag patch already in %s" % path)
    else:
        text = replace_once(text, OLD_ACL_LOG, NEW_ACL_LOG, path)
        text = replace_once(text, OLD_STATE_LOG, NEW_STATE_LOG, path)
        changed = True
        print("A2DP diag patch applied to %s" % path)
    if AVRC_MARKER in text:
        print("A2DP AVRCP target patch already in %s" % path)
    elif lf(AVRC_V1) in text:  # first version without diagnostics: upgrade in place
        new_block = lf(NEW_AVRC)[lf(NEW_AVRC).index("    void radio_avrc_tg_cb"):]
        old_block = lf(AVRC_V1)
        text = replace_once(text, old_block, new_block, path)
        changed = True
        print("A2DP AVRCP target patch upgraded in %s" % path)
    else:
        text = replace_once(text, OLD_AVRC, NEW_AVRC, path)
        changed = True
        print("A2DP AVRCP target patch applied to %s" % path)
    if READY_MARKER in text:
        print("A2DP stack-ready hook already in %s" % path)
    else:
        text = replace_once(text, OLD_READY, NEW_READY, path)
        changed = True
        print("A2DP stack-ready hook applied to %s" % path)
    if changed:
        out = text.replace("\n", "\r\n") if crlf else text
        path.write_bytes(out.encode("utf-8"))


# Fresh build stamp: PlatformIO rebuilds on content, not on timestamps, so write a
# generated header with the current time (git-ignored) that src/version.cpp includes.
import time
inc = Path(env["PROJECT_DIR"]) / "include"
inc.mkdir(exist_ok=True)
(inc / "build_stamp.h").write_text('#define BUILD_STAMP "%s"\n' % time.strftime("%Y-%m-%d %H:%M"), encoding="utf-8")

libdeps = Path(env["PROJECT_DIR"]) / ".pio" / "libdeps"
found_any = False
for cpp in sorted(libdeps.glob("*/ESP32-A2DP/src/BluetoothA2DPSource.cpp")):
    found_any = True
    patch(cpp)
if not found_any:
    print("A2DP reconnect patch: library not installed yet, skipped")
