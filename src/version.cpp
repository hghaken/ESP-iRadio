// Build stamp. scripts/patch_a2dp_reconnect.py writes include/build_stamp.h with the
// current time before every build, so this file is recompiled and the stamp is fresh
// (a __DATE__ inside web.cpp only changed when web.cpp itself was recompiled).
#include "version.h"

#include "build_stamp.h"

const char* const buildStamp = BUILD_STAMP;
const char* const firmwareVersion = FIRMWARE_VERSION;
