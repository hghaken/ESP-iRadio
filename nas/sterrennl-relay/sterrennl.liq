# NPO Sterren.nl -> AAC-LC 96 kbps, 44.1 kHz stereo for ESP iRadio, sent to the own Icecast.
#
# Why: NPO offers Sterren.nl only as MP3 192 kbps at 48 kHz (24 KB/s, too much next to
# Bluetooth) and HE-AAC 64 kbps (the radio plays just the dull 24 kHz core). The MP3 is the best
# source; it is re-encoded as plain AAC-LC 96 kbps (12 KB/s). Titles already come as
# "Artist - Song", so they are passed on unchanged.

settings.log.stdout.set(true)
settings.log.file.set(false)
settings.log.level.set(3)

source_url = "http://icecast.omroep.nl/radio2-sterrennl-mp3"

sterren = input.http(source_url)

# Silence while the source is down, so the Icecast mount (and the radio) stay connected.
sterren = mksafe(sterren)

output.icecast(
  %ffmpeg(format="adts", %audio(codec="aac", b="96k", samplerate=44100, channels=2)),
  host="127.0.0.1",
  port=8888,
  password=environment.get("ICECAST_SOURCE_PASSWORD"),
  mount="/sterrennl.aac",
  format="audio/aac",
  name="NPO Sterren.nl (relay)",
  description="NPO Sterren.nl converted to AAC-LC 96 kbps for ESP iRadio",
  public=false,
  send_icy_metadata=true,
  sterren
)
