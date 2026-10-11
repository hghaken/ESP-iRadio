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
