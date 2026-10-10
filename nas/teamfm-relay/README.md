# Team FM relay on the NAS (Liquidsoap in Docker)

Same setup as `../joe-relay` (see the README there for the full explanation). Fetches Team FM
via StreamTheWorld as MP3 192 kbps (44.1 kHz), re-encodes it to AAC-LC 96 kbps (44.1 kHz
stereo) and offers it on the own Icecast as hidden mount `/teamfm.aac`, with titles.

| | Team FM directly | Via this relay |
|---|---|---|
| Format | MP3 192 kbps 44.1 kHz (24 KB/s, stutters next to Bluetooth) | AAC-LC 96 kbps 44.1 kHz (12 KB/s) |
| Titles | yes, already "artist - song" | yes, passed on unchanged |

The audiostreamen.nl links point to https (`mediacp`, `stream4`) or to the
Drenthe-Groningen regional broadcast (`:8010/stream`); the source here is StreamTheWorld's
http entrance for `TEAMFM`.

## On the NAS
- Folder: `/volume1/docker/teamfm-relay` with `teamfm.liq`, `docker-compose.yml` and `.env`
  (the Icecast source password, readable by the owner only, not in git).
- Start: `cd /volume1/docker/teamfm-relay && sudo docker compose up -d`.
- `icecast.xml` has a mount block for this:

  ```xml
  <mount type="normal">
      <mount-name>/teamfm.aac</mount-name>
      <burst-size>262144</burst-size>
      <hidden>1</hidden>
  </mount>
  ```

## On the radio
```
Team FM|http://icecast.example.com:8888/teamfm.aac
```
