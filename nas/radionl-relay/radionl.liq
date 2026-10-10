# RadioNL -> AAC-LC 96 kbps, 44.1 kHz stereo for ESP iRadio, sent to the own Icecast.
#
# Why: RadioNL has a single stream, MP3 192 kbps 44.1 kHz (24 KB/s, too much next to
# Bluetooth). It is re-encoded as plain AAC-LC 96 kbps (12 KB/s). Bonus: RadioNL inserts a
# ~10 s pre-roll ad on every (re)connect; the NAS reconnects rarely, the radio often (station
# change, stall, paging window), so the radio hears far fewer of those.

settings.log.stdout.set(true)
settings.log.file.set(false)
settings.log.level.set(3)

source_url = "http://stream.radionl.fm/radionl"

radionl = input.http(source_url)

# Silence while the source is down, so the Icecast mount (and the radio) stay connected.
radionl = mksafe(radionl)

output.icecast(
  %ffmpeg(format="adts", %audio(codec="aac", b="96k", samplerate=44100, channels=2)),
  host="127.0.0.1",
  port=8888,
  password=environment.get("ICECAST_SOURCE_PASSWORD"),
  mount="/radionl.aac",
  format="audio/aac",
  name="RadioNL (relay)",
  description="RadioNL converted to AAC-LC 96 kbps for ESP iRadio",
  public=false,
  send_icy_metadata=true,
  radionl
)
