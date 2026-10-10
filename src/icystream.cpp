#include "icystream.h"

#include "weblog.h"

bool IcyStream::begin(const String& url) {
  end();
  return open(url, 2);
}

void IcyStream::end() {
  client.stop();
  metaInt = untilMeta = 0;
  lostConn = false;
  icyBr = 0;
  mime = "";
}

bool IcyStream::open(const String& url, int redirectsLeft) {
  if (!url.startsWith("http://")) {
    Log.println("[icy] only plain http:// is supported");
    return false;
  }
  int hostStart = 7;
  int pathStart = url.indexOf('/', hostStart);
  String hostPort = pathStart < 0 ? url.substring(hostStart) : url.substring(hostStart, pathStart);
  String path = pathStart < 0 ? "/" : url.substring(pathStart);
  int port = 80;
  int colon = hostPort.indexOf(':');
  String host = hostPort;
  if (colon >= 0) {
    host = hostPort.substring(0, colon);
    port = hostPort.substring(colon + 1).toInt();
  }

  client.setTimeout(timeoutMs / 1000);
  if (!client.connect(host.c_str(), port, timeoutMs)) {
    Log.printf("[icy] connect to %s:%d failed\n", host.c_str(), port);
    return false;
  }
  client.print("GET " + path + " HTTP/1.0\r\nHost: " + host + "\r\nIcy-MetaData: 1\r\n"
               "User-Agent: ESP-iRadio\r\nConnection: close\r\n\r\n");

  String line;
  if (!readLine(line) || line.indexOf(' ') < 0) {
    Log.println("[icy] no status line");
    client.stop();
    return false;
  }
  int status = line.substring(line.indexOf(' ') + 1).toInt();  // "HTTP/1.0 200 OK" or "ICY 200 OK"
  String location;
  while (readLine(line) && line.length()) {
    int sep = line.indexOf(':');
    if (sep < 0) continue;
    String key = line.substring(0, sep), value = line.substring(sep + 1);
    key.toLowerCase();
    value.trim();
    if (key == "content-type") mime = value;
    else if (key == "icy-metaint") metaInt = value.toInt();
    else if (key == "icy-name") icyName = value;
    else if (key == "icy-br") icyBr = value.toInt();
    else if (key == "location") location = value;
  }
  if (status >= 300 && status < 400 && location.length() && redirectsLeft > 0) {
    client.stop();
    Log.printf("[icy] redirect to %s\n", location.c_str());
    return open(location, redirectsLeft - 1);
  }
  if (status != 200) {
    Log.printf("[icy] HTTP %d\n", status);
    client.stop();
    return false;
  }
  untilMeta = metaInt;
  Log.printf("[icy] %s, %s, metaint %d\n", icyName.c_str(), mime.c_str(), metaInt);
  return true;
}

// Reads one header line (without CR/LF); false on timeout/disconnect.
bool IcyStream::readLine(String& line) {
  line = "";
  uint32_t start = millis();
  while (millis() - start < (uint32_t)timeoutMs) {
    if (!client.available()) {
      if (!client.connected()) return false;
      delay(2);
      continue;
    }
    char c = client.read();
    if (c == '\n') return true;
    if (c != '\r') line += c;
    if (line.length() > 1024) return false;
  }
  return false;
}

bool IcyStream::readFully(uint8_t* data, size_t len) {
  size_t got = 0;
  uint32_t start = millis();
  while (got < len && millis() - start < (uint32_t)timeoutMs) {
    int avail = client.available();
    if (avail <= 0) {
      if (!client.connected()) return false;
      delay(2);
      continue;
    }
    int r = client.read(data + got, min((size_t)avail, len - got));
    if (r <= 0) return false;  // read error: -1 must not be added to a size_t
    got += r;
  }
  return got == len;
}

// Metadata block: 1 length byte (x16), then "StreamTitle='...';StreamUrl='...';" padded with zeros.
// Returns false when the block could not be read completely; the stream position is
// then unknown (the next bytes are not audio), so the caller must drop the connection.
bool IcyStream::readMetadata() {
  uint8_t lenByte;
  if (!readFully(&lenByte, 1)) return false;
  int len = lenByte * 16;
  if (len == 0) return true;
  uint8_t* meta = (uint8_t*)malloc(len + 1);  // > 256 bytes: lands in PSRAM
  if (meta == nullptr) {
    uint8_t skip[64];  // no memory: still consume the block to stay in step
    while (len > 0) {
      int n = min(len, (int)sizeof(skip));
      if (!readFully(skip, n)) return false;
      len -= n;
    }
    return true;
  }
  bool ok = readFully(meta, len);
  if (ok) {
    meta[len] = 0;
    String s((char*)meta);
    int a = s.indexOf("StreamTitle='");
    if (a >= 0) {
      a += 13;
      int b = s.indexOf("';", a);
      if (b < 0) b = s.length();
      if (onTitle) onTitle(s.substring(a, b));
    }
  }
  free(meta);
  return ok;
}

size_t IcyStream::readBytes(uint8_t* data, size_t len) {
  if (lostConn) return 0;
  uint32_t start = millis();
  while (client.available() <= 0) {
    if (!client.connected() || millis() - start > 50) return 0;
    delay(5);
  }
  if (metaInt > 0 && untilMeta == 0) {
    if (!readMetadata()) {
      Log.println("[icy] metadata block incomplete, dropping the connection");
      client.stop();
      lostConn = true;
      return 0;
    }
    untilMeta = metaInt;
  }
  size_t want = len;
  if (metaInt > 0) want = min(want, (size_t)untilMeta);
  int avail = client.available();
  if (avail <= 0) return 0;
  int n = client.read(data, min((size_t)avail, want));
  if (n <= 0) return 0;
  if (metaInt > 0) untilMeta -= n;
  return n;
}
