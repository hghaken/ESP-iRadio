# Joe relay on the NAS (Liquidsoap in Docker)

Fetches Joe, re-encodes it to AAC-LC 96 kbps (44.1 kHz stereo) and offers it on your own
Icecast as `/joe.aac`. The radio plays that as steadily as My Radio, with the full sound and
with titles.

| | Joe directly | Via this relay |
|---|---|---|
| Format on the radio | HE-AAC (dull, only the 24 kHz core) or MP3 128 kbps 48 kHz | AAC-LC 96 kbps 44.1 kHz |
| Needed next to Bluetooth | 16 KB/s (tight) | 12 KB/s (stable) |
| Titles | yes | yes (passed on) |

## Files
- `joe.liq` - the Liquidsoap script (source, re-encode, to Icecast).
- `docker-compose.yml` - the container (Liquidsoap 2.4.6, host network, restarts by itself).
- `.env.example` - example for the password. Copy it to `.env` on the NAS and fill it in.

## 1. Prepare Icecast (`icecast.xml`)
One more source is added. Raise `<sources>`:

```xml
<sources>10</sources>
```

And add a mount block next to the other four:

```xml
<mount type="normal">
    <mount-name>/joe.aac</mount-name>
    <burst-size>262144</burst-size>
    <hidden>1</hidden>
</mount>
```

`hidden` keeps the relayed commercial station off the public status page; it is for your own
use only. Restart Icecast afterwards.

## 2. Start the container (Synology Container Manager)
1. On the NAS, create a folder, for example `/volume1/docker/joe-relay`.
2. Put `joe.liq` and `docker-compose.yml` in it.
3. Copy `.env.example` to `.env` in the same folder and fill in Icecast's source password.
4. Container Manager: Project, Create, choose that folder as the path, use the existing
   `docker-compose.yml`, and start.
5. Check the container's log. A good sign is a message that the connection to Icecast
   succeeded (something like "Connection setup was successful"). An error about an unknown parameter
   means the Liquidsoap version is different; report the error line.

If the image tag `v2.4.6` does not exist, pick the newest `v2.4.x` tag of `savonet/liquidsoap`
in Container Manager and put that in `docker-compose.yml`.

## 3. Check
- Icecast status page (as admin, the mount is hidden): `/joe.aac` with `audio/aac`, 96 kbps.
- From a PC: `ffprobe http://icecast.example.com:8888/joe.aac` reports `aac (LC)`, 44100 Hz,
  stereo.
- On the radio, in the station list:

  ```
  Joe|http://icecast.example.com:8888/joe.aac
  ```

## Load
The container pulls Joe all the time (~12 KB/s), even when nobody is listening. Re-encoding costs
roughly 5 to 10 % of one processor core on an ordinary NAS. If the source drops out, Liquidsoap
sends silence and the mount stays up; the radio stays connected.

## More stations
For another station: copy the folder, change `source_url`, `mount` and `name` in the `.liq` file,
and the service name and `container_name` in `docker-compose.yml`. Every station gets its own
mount block in `icecast.xml`, and `<sources>` must stay large enough.
