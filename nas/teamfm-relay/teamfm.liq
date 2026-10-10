# Team FM -> AAC-LC 96 kbps, 44.1 kHz stereo for ESP iRadio, sent to the own Icecast.
#
# Why: Team FM (StreamTheWorld) only offers MP3 192 kbps 44.1 kHz (24 KB/s, too much next to
# Bluetooth). It is re-encoded as plain AAC-LC 96 kbps (12 KB/s). Titles already come as
# "Artist - Song", so they are passed on unchanged.

settings.log.stdout.set(true)
settings.log.file.set(false)
settings.log.level.set(3)

# StreamTheWorld's http entrance; it redirects to an http host on port 80.
source_url = "http://playerservices.streamtheworld.com/api/livestream-redirect/TEAMFM.mp3"

teamfm = input.http(source_url)

# Silence while the source is down, so the Icecast mount (and the radio) stay connected.
teamfm = mksafe(teamfm)

output.icecast(
  %ffmpeg(format="adts", %audio(codec="aac", b="96k", samplerate=44100, channels=2)),
  host="127.0.0.1",
  port=8888,
  password=environment.get("ICECAST_SOURCE_PASSWORD"),
  mount="/teamfm.aac",
  format="audio/aac",
  name="Team FM (relay)",
  description="Team FM converted to AAC-LC 96 kbps for ESP iRadio",
  public=false,
  send_icy_metadata=true,
  teamfm
)
