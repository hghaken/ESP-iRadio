# Joe (DPG Media) -> AAC-LC 96 kbps, 44.1 kHz stereo for ESP iRadio, sent to the own Icecast.
#
# Why: Joe only offers HE-AAC (the radio's Helix decoder plays just the dull 24 kHz core) and
# MP3 128 kbps at 48 kHz (needs 16 KB/s, borderline next to Bluetooth). Liquidsoap decodes the
# HE-AAC stream in full (incl. SBR) and re-encodes it as plain AAC-LC 96 kbps (12 KB/s), the
# format the radio plays best. Song titles are passed on as ICY metadata.

settings.log.stdout.set(true)
settings.log.file.set(false)
settings.log.level.set(3)

# Source: HE-AAC 96 kbps (http, no TLS needed; https would work here too).
source_url = "http://audio-streaming.joe.nl/Joe_nl_high.aac"

joe = input.http(source_url)

# Joe (like Qmusic) sends "Song - ARTIST" in a single title field. Split on the last " - " and
# pass artist and title separately; Icecast then gets "ARTIST - Song", the order the radio
# expects (artist on the first display line). Titles without " - " (jingles) and metadata that
# already has an artist are left alone.
def swap_song_artist(m) =
  parts = string.split(separator=" - ", m["title"])
  if m["artist"] == "" and list.length(parts) >= 2 then
    rev = list.rev(parts)
    [("artist", list.hd(rev)), ("title", string.concat(separator=" - ", list.rev(list.tl(rev))))]
  else
    []
  end
end
joe = metadata.map(swap_song_artist, joe)

# Silence while the source is down, so the Icecast mount (and the radio) stay connected.
joe = mksafe(joe)

output.icecast(
  %ffmpeg(format="adts", %audio(codec="aac", b="96k", samplerate=44100, channels=2)),
  host="127.0.0.1",
  port=8888,
  password=environment.get("ICECAST_SOURCE_PASSWORD"),
  mount="/joe.aac",
  format="audio/aac",
  name="Joe (relay)",
  description="Joe converted to AAC-LC 96 kbps for ESP iRadio",
  public=false,
  send_icy_metadata=true,
  joe
)
