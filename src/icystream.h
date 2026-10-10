// Minimal HTTP/ICY (Shoutcast/Icecast) stream client, plain HTTP only.
// Replaces audio-tools' URLStream, which links in WiFiClientSecure/mbedTLS (~250 KB
// of flash we cannot afford next to a second OTA slot, and TLS would not fit in the
// ~45 KB of free internal RAM anyway) and which switches WiFi modem sleep off.
#pragma once
#include <Arduino.h>
#include <WiFiClient.h>

class IcyStream {
 public:
  typedef void (*TitleCallback)(const String& title);

  // Connects and reads the response headers. Follows one redirect. False on failure.
  bool begin(const String& url);
  void end();
  // Returns stream bytes without the ICY metadata; 0 when nothing arrived within
  // ~50 ms or the connection is gone.
  size_t readBytes(uint8_t* data, size_t len);
  // True when readBytes() had to drop the connection (a metadata block could not be
  // read completely, so audio and metadata are out of step): reconnect.
  bool lost() const { return lostConn; }
  const String& contentType() const { return mime; }
  int bitrateKbps() const { return icyBr; }  // from the icy-br header, 0 if absent
  void setTitleCallback(TitleCallback cb) { onTitle = cb; }
  void setTimeout(int ms) { timeoutMs = ms; }

 private:
  bool open(const String& url, int redirectsLeft);
  bool readLine(String& line);
  bool readFully(uint8_t* data, size_t len);
  bool readMetadata();

  WiFiClient client;
  String mime, icyName;
  TitleCallback onTitle = nullptr;
  int timeoutMs = 8000;
  int icyBr = 0;          // icy-br header (kbps), 0 if absent
  int metaInt = 0;        // bytes of audio between metadata blocks (0 = none)
  int untilMeta = 0;      // audio bytes left before the next metadata block
  bool lostConn = false;  // dropped by readBytes(), see lost()
};
