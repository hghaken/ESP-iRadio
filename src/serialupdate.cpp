#include "serialupdate.h"

#include <Arduino.h>
#include <Update.h>

#include "display.h"
#include "player.h"
#include "settings.h"
#include "weblog.h"

static const uint32_t DATA_BAUD = 921600;
static const size_t CHUNK = 1024;

static void finishFailed(const char* why) {
  Update.abort();
  Serial.printf("\n#E %s\n", why);
  Serial.flush();
  Serial.updateBaudRate(115200);
  weblogSerialMute(false);
  Log.printf("[usb] update failed: %s\n", why);
  displaySetUpdate(UpdatePhase::Failed, 0, 0, "via USB");
  playerHoldBluetooth(false);
  playerPlay(settings.lastStation);
}

static void runUpdate(size_t size, const String& md5) {
  if (size < 100000 || size > 0x1F0000) {
    Serial.print("\n#E bad size\n");
    return;
  }
  Log.printf("[usb] update over USB: %u bytes\n", (unsigned)size);
  weblogSerialMute(true);  // keep our own log lines out of the data channel
  playerShutdown();
  playerHoldBluetooth(true);
  displaySetUpdate(UpdatePhase::Receiving, 0, size, "via USB");
  if (!Update.begin(size)) {
    finishFailed("begin");
    return;
  }
  if (md5.length() == 32) Update.setMD5(md5.c_str());
  uint8_t* buf = (uint8_t*)malloc(CHUNK);
  if (!buf) {
    finishFailed("no memory");
    return;
  }
  while (Serial.available()) Serial.read();  // nothing else may precede the data
  Serial.printf("\n#R %u\n", (unsigned)DATA_BAUD);
  Serial.flush();
  Serial.updateBaudRate(DATA_BAUD);

  size_t done = 0;
  bool ok = true;
  const char* why = "";
  while (done < size) {
    size_t want = size - done < CHUNK ? size - done : CHUNK;
    size_t got = 0;
    uint32_t last = millis();
    while (got < want) {
      int n = Serial.available();
      if (n > 0) {
        got += Serial.read(buf + got, (size_t)n < want - got ? (size_t)n : want - got);
        last = millis();
      } else if (millis() - last > 10000) {
        ok = false;
        why = "timeout";
        break;
      } else {
        delay(1);
      }
    }
    if (!ok) break;
    if (Update.write(buf, want) != want) {
      ok = false;
      why = "write";
      break;
    }
    done += want;
    displaySetUpdate(UpdatePhase::Receiving, done, size, "via USB");
    Serial.printf("#K %u\n", (unsigned)done);  // the PC sends the next chunk only after this
  }
  free(buf);
  if (ok && !Update.end(true)) {
    ok = false;
    why = Update.errorString();
  }
  if (!ok) {
    finishFailed(why);
    return;
  }
  Serial.print("#D\n");
  Serial.flush();
  displaySetUpdate(UpdatePhase::Done, size, size, "via USB");
  delay(1500);  // let the display show it
  ESP.restart();
}

void serialUpdateLoop() {
  static String line;
  while (Serial.available()) {
    int c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (line.startsWith("IRADIO-UPDATE ")) {
        int sp = line.indexOf(' ', 14);
        size_t size = line.substring(14, sp < 0 ? line.length() : sp).toInt();
        String md5 = sp < 0 ? String("") : line.substring(sp + 1);
        md5.trim();
        line = "";
        runUpdate(size, md5);
        return;
      }
      line = "";
    } else if (line.length() < 80) {
      line += (char)c;
    } else {
      line = "";  // not our command (e.g. noise when the cable is plugged in)
    }
  }
}
