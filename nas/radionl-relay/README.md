# RadioNL relay on the NAS (Liquidsoap in Docker)

Same setup as `../joe-relay` (see the README there for the full explanation). Fetches RadioNL
as MP3 192 kbps (44.1 kHz), re-encodes it to AAC-LC 96 kbps (44.1 kHz stereo) and offers it
on the own Icecast as hidden mount `/radionl.aac`, with titles.

| | RadioNL directly | Via this relay |
|---|---|---|
| Format | MP3 192 kbps 44.1 kHz (24 KB/s, stutters next to Bluetooth) | AAC-LC 96 kbps 44.1 kHz (12 KB/s) |
| Ad on connect | ~10 s on every (re)connect of the radio | only when the NAS reconnects |

## On the NAS
- Folder: `/volume1/docker/radionl-relay` with `radionl.liq`, `docker-compose.yml` and `.env`
  (the Icecast source password, readable by the owner only, not in git).
- Start: `cd /volume1/docker/radionl-relay && sudo docker compose up -d`.
- `icecast.xml` has a mount block for this:

  ```xml
  <mount type="normal">
      <mount-name>/radionl.aac</mount-name>
      <burst-size>262144</burst-size>
      <hidden>1</hidden>
  </mount>
  ```

## On the radio
```
RadioNL|http://icecast.example.com:8888/radionl.aac
```
