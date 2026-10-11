# Env idf only: two small edits to Bluedroid's A2DP source in the PlatformIO ESP-IDF package.
#
# 1. SBC bitpool (v1.08). Bluedroid's source capability is a compile-time constant
# (BTA_AV_CO_SBC_MAX_BITPOOL 53 in components/bt/host/bluedroid/btc/profile/std/a2dp/bta_av_co.c,
# SBC "high quality", ~328 kbit/s at 44.1 kHz joint stereo); the bitpool used is min(source max,
# sink max). Next to the JBL Charge 5 the WiFi stream collapsed every few minutes (2026-10-11)
# while the headphones were fine, even with the radio as link master; less Bluetooth airtime was
# the next lever. 35 is the A2DP spec's "middle quality" for 44.1 kHz joint stereo (~229 kbit/s,
# ~30 % less airtime). It did not cure the stalls (docs/wifi-stall-investigation.md).
#
# 2. MTU log line (v1.10). btc_a2dp_source_enc_update() runs when the stream starts and sets the
# media packet size from the sink's L2CAP MTU; one "(diag) a2dp stream" log line shows the
# peer MTU, the MTU used, the bitpool and the SBC frame length, so the JBL and the headphones can
# be compared (how many frames, and so which baseband packets, per media packet). It is an
# ESP_LOGE: the self-built IDF has CONFIG_LOG_DEFAULT_LEVEL 1 (error), which compiles ESP_LOGW out.
#
# The env idf compiles ESP-IDF from the PlatformIO package (C:/pio/packages/framework-espidf), so
# this script edits those files before every build; the prebuilt env radio (libbt.a) has neither.
# A package reinstall restores the originals; the next idf build puts both edits back.
Import("env")

import re
from pathlib import Path

BITPOOL = 35

pkg = Path(env.PioPlatform().get_package_dir("framework-espidf"))
a2dp = pkg / "components/bt/host/bluedroid/btc/profile/std/a2dp"

# --- 1. bitpool ---
src = a2dp / "bta_av_co.c"
text = src.read_text(encoding="utf-8")
new = re.sub(r"#define BTA_AV_CO_SBC_MAX_BITPOOL\s+\d+", "#define BTA_AV_CO_SBC_MAX_BITPOOL  %d" % BITPOOL, text, count=1)
if new == text and ("BTA_AV_CO_SBC_MAX_BITPOOL  %d" % BITPOOL) not in text:
    raise SystemExit("patch_idf_bitpool: define not found in %s" % src)
if new != text:
    src.write_text(new, encoding="utf-8")
    print("SBC max bitpool set to %d in %s" % (BITPOOL, src))
else:
    print("SBC max bitpool already %d" % BITPOOL)

# --- 2. MTU log line ---
MARKER = "/* iradio: mtu diag */"
src = a2dp / "btc_a2dp_source.c"
text = src.read_text(encoding="utf-8")
OLD_LOG = 'ESP_LOGW("BT_AV", "(diag) a2dp stream'
if MARKER in text and OLD_LOG in text:
    src.write_bytes(text.replace(OLD_LOG, 'ESP_LOGE("BT_AV", "(diag) a2dp stream').encode("utf-8"))
    print("A2DP MTU log line upgraded to ESP_LOGE in %s" % src)
elif MARKER in text:
    print("A2DP MTU log line already in place")
else:
    inc_old = '#include "sbc_encoder.h"\n'
    anchor = """        /* make sure we reinitialize encoder with new settings */
        SBC_Encoder_Init(&(a2dp_source_local_param.btc_aa_src_cb.encoder));
    }
}
"""
    log = """        /* make sure we reinitialize encoder with new settings */
        SBC_Encoder_Init(&(a2dp_source_local_param.btc_aa_src_cb.encoder));
        %s
        ESP_LOGE("BT_AV", "(diag) a2dp stream: peer mtu %%d, used mtu %%d, bitpool %%d (sink %%d-%%d), sbc frame %%d bytes, %%d kbit/s",
                 pUpdateAudio->MinMtuSize, a2dp_source_local_param.btc_aa_src_cb.TxAaMtuSize,
                 pstrEncParams->s16BitPool, pUpdateAudio->MinBitPool, pUpdateAudio->MaxBitPool,
                 pstrEncParams->u16PacketLength, pstrEncParams->u16BitRate);
    }
}
""" % MARKER
    crlf = "\r\n" in text
    body = text.replace("\r\n", "\n")
    if body.count(inc_old) != 1 or body.count(anchor) != 1:
        raise SystemExit("patch_idf_bitpool: MTU anchor not found in %s" % src)
    body = body.replace(inc_old, inc_old + '#include "esp_log.h"\n', 1).replace(anchor, log, 1)
    if crlf:
        body = body.replace("\n", "\r\n")
    src.write_bytes(body.encode("utf-8"))
    print("A2DP MTU log line added to %s" % src)


# --- 3. Bluetooth TX diagnostics (v1.15, review experiments E2 + E4) ---
# Counters for the A2DP send path, logged every 30 s as "(diag) a2dp tx": SBC frames dropped
# because the TX queue overflowed (btc), media packets handed to AVDTP and packets dropped
# because L2CAP did not move data (bta_av), the highest L2CAP queue depth seen, and the ACL
# packets the controller reported as completed (l2c). Plus one "(diag) link mode" line per HCI
# mode change (0 active, 1 hold, 2 sniff, 3 park). Log only: no behaviour changes.
BT = pkg / "components/bt/host/bluedroid"


def patch(path, marker, pairs):
    text = path.read_text(encoding="utf-8")
    if marker in text:
        print("%s already patched" % path.name)
        return
    crlf = "\r\n" in text
    body = text.replace("\r\n", "\n")
    for old, new in pairs:
        if body.count(old) != 1:
            raise SystemExit("patch_idf_bitpool: anchor not found in %s: %r" % (path, old[:60]))
        body = body.replace(old, new, 1)
    if crlf:
        body = body.replace("\n", "\r\n")
    path.write_bytes(body.encode("utf-8"))
    print("TX diagnostics added to %s" % path.name)


patch(a2dp / "btc_a2dp_source.c", "/* iradio: tx diag */", [
    ('#include "sbc_encoder.h"\n',
     '#include "sbc_encoder.h"\n#include "esp_timer.h"\n/* iradio: tx diag */\n'
     'volatile uint32_t iradio_q_drop, iradio_av_drop, iradio_av_writes, iradio_l2c_bufs_max, iradio_cmpl_pkts;\n'),
    ("""    while (fixed_queue_length(a2dp_source_local_param.btc_aa_src_cb.TxAaQ) > (MAX_OUTPUT_A2DP_SRC_FRAME_QUEUE_SZ - nb_frame)) {
        osi_free(fixed_queue_dequeue(a2dp_source_local_param.btc_aa_src_cb.TxAaQ, 0));
    }
""",
     """    while (fixed_queue_length(a2dp_source_local_param.btc_aa_src_cb.TxAaQ) > (MAX_OUTPUT_A2DP_SRC_FRAME_QUEUE_SZ - nb_frame)) {
        osi_free(fixed_queue_dequeue(a2dp_source_local_param.btc_aa_src_cb.TxAaQ, 0));
        iradio_q_drop++;
    }
    {
        static int64_t iradio_next = 0;
        int64_t now = esp_timer_get_time();
        if (now >= iradio_next) {
            if (iradio_next)
                ESP_LOGE("BT_AV", "(diag) a2dp tx 30s: queued %u, av drops %u, frame drops %u, l2c bufs max %u, acl completed %u, txq %d",
                         (unsigned)iradio_av_writes, (unsigned)iradio_av_drop, (unsigned)iradio_q_drop,
                         (unsigned)iradio_l2c_bufs_max, (unsigned)iradio_cmpl_pkts,
                         (int)fixed_queue_length(a2dp_source_local_param.btc_aa_src_cb.TxAaQ));
            iradio_av_writes = iradio_av_drop = iradio_q_drop = iradio_l2c_bufs_max = iradio_cmpl_pkts = 0;
            iradio_next = now + 30000000;
        }
    }
"""),
])

# --- 4. Full media packets only (v1.17) ---
# Every 30 ms tick Bluedroid encodes the frames due (~10.3 SBC frames at 44.1 kHz) and packs them
# into media packets of at most the sink's MTU. With the JBL Charge 5 (MTU 660, 83-byte frames at
# bitpool 35: 7 frames per packet) that is a full packet plus a packet with the 3-4 left over,
# ~67 packets/s, two back to back per tick; the WH-1000XM6 (MTU 883: 10 frames) needs ~34/s.
# Next to the JBL WiFi starved while the band was busy (2026-10-11, pausing the A2DP stream
# brought WiFi back at once). Here the frames due are rounded down to whole packets; the rest
# stays in the PCM counter for the next tick (one tick of extra latency at most), so the JBL gets
# ~49 full packets/s with the same audio and the headphones are unchanged.
patch(a2dp / "btc_a2dp_source.c", "/* iradio: full packets */", [
    ("""        result = a2dp_source_local_param.btc_aa_src_cb.media_feeding_state.pcm.counter / pcm_bytes_per_frame;
""",
     """        result = a2dp_source_local_param.btc_aa_src_cb.media_feeding_state.pcm.counter / pcm_bytes_per_frame;
        { /* iradio: full packets */
            UINT32 fl = a2dp_source_local_param.btc_aa_src_cb.encoder.u16PacketLength;
            UINT32 mtu = a2dp_source_local_param.btc_aa_src_cb.TxAaMtuSize;
            if (fl > 0 && mtu > fl) {
                UINT32 fpp = (mtu - 1) / fl;  /* frames per packet, as the packing loop fills them */
                if (fpp > 15) fpp = 15;
                if (fpp > 1 && result >= fpp) result -= result % fpp;
            }
        }
"""),
])

patch(BT / "bta/av/bta_av_aact.c", "/* iradio: tx diag */", [
    ("    p_scb->l2c_bufs = (UINT8)L2CA_FlushChannel (p_scb->l2c_cid, L2CAP_FLUSH_CHANS_GET);\n",
     "    p_scb->l2c_bufs = (UINT8)L2CA_FlushChannel (p_scb->l2c_cid, L2CAP_FLUSH_CHANS_GET);\n"
     "    { /* iradio: tx diag */ extern volatile uint32_t iradio_l2c_bufs_max;\n"
     "      if (p_scb->l2c_bufs > iradio_l2c_bufs_max) iradio_l2c_bufs_max = p_scb->l2c_bufs; }\n"),
    ("            AVDT_WriteReqOpt(p_scb->avdt_handle, p_buf, timestamp, m_pt, opt);\n",
     "            AVDT_WriteReqOpt(p_scb->avdt_handle, p_buf, timestamp, m_pt, opt);\n"
     "            { extern volatile uint32_t iradio_av_writes; iradio_av_writes++; }\n"),
    ("                    bta_av_co_audio_drop(p_scb->hndl);\n",
     "                    bta_av_co_audio_drop(p_scb->hndl);\n"
     "                    { extern volatile uint32_t iradio_av_drop; iradio_av_drop++; }\n"),
])

patch(BT / "stack/l2cap/l2c_link.c", "/* iradio: tx diag */", [
    ("        STREAM_TO_UINT16 (num_sent, p);\n",
     "        STREAM_TO_UINT16 (num_sent, p);\n"
     "        { /* iradio: tx diag */ extern volatile uint32_t iradio_cmpl_pkts; iradio_cmpl_pkts += num_sent; }\n"),
])

patch(BT / "stack/btm/btm_pm.c", "/* iradio: mode diag */", [
    ("#include <stddef.h>\n", '#include <stddef.h>\n#include "esp_log.h"\n'),
    ("""void btm_pm_proc_mode_change (UINT8 hci_status, UINT16 hci_handle, UINT8 mode, UINT16 interval)
{
    tACL_CONN   *p;
    tBTM_PM_MCB *p_cb = NULL;
    int yy;
    tBTM_PM_STATE  old_state;
    tL2C_LCB        *p_lcb;
""",
     """void btm_pm_proc_mode_change (UINT8 hci_status, UINT16 hci_handle, UINT8 mode, UINT16 interval)
{
    tACL_CONN   *p;
    tBTM_PM_MCB *p_cb = NULL;
    int yy;
    tBTM_PM_STATE  old_state;
    tL2C_LCB        *p_lcb;
    /* iradio: mode diag */
    ESP_LOGE("BT_PM", "(diag) link mode: status %d, handle %d, mode %d, interval %d", hci_status, hci_handle, mode, interval);
"""),
])
