# Env idf only: limits the SBC bitpool the radio offers as an A2DP source.
#
# Bluedroid's source capability is a compile-time constant (BTA_AV_CO_SBC_MAX_BITPOOL 53 in
# components/bt/host/bluedroid/btc/profile/std/a2dp/bta_av_co.c, SBC "high quality", ~328 kbit/s
# at 44.1 kHz joint stereo); the bitpool used is min(source max, sink max). Next to the JBL
# Charge 5 the WiFi stream collapsed every few minutes (2026-10-11) while the headphones were fine,
# even with the radio as link master; less Bluetooth airtime is the next lever. 35 is the A2DP
# spec's "middle quality" for 44.1 kHz joint stereo (~229 kbit/s, ~30 % less airtime).
#
# The env idf compiles ESP-IDF from the PlatformIO package (C:/pio/packages/framework-espidf), so
# this script edits that file before every build; the prebuilt env radio (libbt.a) keeps 53.
# A package reinstall restores 53; the next idf build puts the value back.
Import("env")

import re
from pathlib import Path

BITPOOL = 35

pkg = env.PioPlatform().get_package_dir("framework-espidf")
src = Path(pkg) / "components/bt/host/bluedroid/btc/profile/std/a2dp/bta_av_co.c"
text = src.read_text(encoding="utf-8")
new = re.sub(r"#define BTA_AV_CO_SBC_MAX_BITPOOL\s+\d+", "#define BTA_AV_CO_SBC_MAX_BITPOOL  %d" % BITPOOL, text, count=1)
if new == text and ("BTA_AV_CO_SBC_MAX_BITPOOL  %d" % BITPOOL) not in text:
    raise SystemExit("patch_idf_bitpool: define not found in %s" % src)
if new != text:
    src.write_text(new, encoding="utf-8")
    print("SBC max bitpool set to %d in %s" % (BITPOOL, src))
else:
    print("SBC max bitpool already %d" % BITPOOL)
