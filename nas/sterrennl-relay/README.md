# NPO Sterren.nl relay on the NAS (Liquidsoap in Docker)

Same setup as `../joe-relay` (see the README there for the full explanation). Fetches
NPO Sterren.nl as MP3 192 kbps (48 kHz), re-encodes it to AAC-LC 96 kbps (44.1 kHz stereo)
and offers it on the own Icecast as hidden mount `/sterrennl.aac`, with titles.

| | Sterren.nl directly | Via this relay |
|---|---|---|
| Format | MP3 192 kbps 48 kHz (24 KB/s, stutters next to Bluetooth) or HE-AAC 64 kbps (dull) | AAC-LC 96 kbps 44.1 kHz (12 KB/s) |
| Titles | yes, already "artist - song" | yes, passed on unchanged |

## On the NAS
- Folder: `/volume1/docker/sterrennl-relay` with `sterrennl.liq`, `docker-compose.yml` and `.env`
  (the Icecast source password, readable by the owner only, not in git).
- Start: `cd /volume1/docker/sterrennl-relay && sudo docker compose up -d`.
- `icecast.xml` has a mount block for this:

  ```xml
  <mount type="normal">
      <mount-name>/sterrennl.aac</mount-name>
      <burst-size>262144</burst-size>
      <hidden>1</hidden>
  </mount>
  ```

## On the radio
```
NPO Sterren.nl|http://icecast.example.com:8888/sterrennl.aac
```
