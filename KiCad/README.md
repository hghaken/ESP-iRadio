# KiCad: PCB for the ESP iRadio

This folder holds everything an AI agent (Claude, Grok, ...) or a person needs to design a printed
circuit board for this radio in **KiCad 9**. The KiCad project is `esp-iradio/` (schematic Rev A
draft: one A3 sheet in Elektuur/Elektor style; no board layout yet). Its status, decisions and
libraries are in `esp-iradio/NOTES.md`.

## What the board is

Today the radio is a hand-wired stack: an **Olimex ESP32-DevKit-LiPo** (ESP32-WROVER, Li-Po charger,
USB) with jumper wires to a 1.69" ST7789 display. A rotary encoder, a microSD socket, a PCM5102A
DAC (line out) and a PAM8403 amplifier (speaker) are ordered and planned.

**Phase 1 (design this first): a carrier board.** The Olimex board plugs in on two 1x19 pin
headers; the carrier holds the display, the encoder, the microSD socket, a PCM5102A DAC chip
(line out on a 3.5 mm jack), a mono class-D amplifier chip for one speaker, and a USB-C power
input. The firmware needs no pin changes: every net in `02-pinout-and-nets.md` matches
`src/config.h` and the docs. 2-layer board, assembled by JLCPCB; the ESP32 module and its charger
stay a finished product.

Owner decisions (2026-10-11, details in `01-requirements.md`): chips on the board (no audio
modules), display on the board, a 3D-printed case designed around the board, one mono speaker,
amplifier powered from a USB-C input, JLCPCB with SMT assembly.

**Phase 2 (later, optional): an integrated board** with the ESP32-WROVER-E module, USB-UART, Li-Po
charger and regulator on the PCB itself. Only after phase 1 works; notes in `01-requirements.md`.

## Files (read in this order)

| File | Content |
|---|---|
| `01-requirements.md` | Scope, functions, constraints, phase 1 vs phase 2, open questions for the owner |
| `02-pinout-and-nets.md` | Every GPIO and connector pin with its net name; forbidden and strapping pins |
| `03-parts-and-footprints.md` | Bill of materials with KiCad symbols/footprints to use or to create |
| `04-power.md` | Power tree, currents, the amplifier supply question, decoupling, grounding |
| `05-layout-guidelines.md` | Board outline, antenna keep-out, audio and SPI routing, mechanics, fab rules |
| `06-checklist-and-bring-up.md` | Design review checklist (ERC/DRC/firmware match) and first power-up test plan |

## Sources in this repository

- `AGENTS.md` (project root): the whole project, pin tables, pitfalls. Sections "Display wiring",
  "Battery measurement", "Hardware".
- `src/config.h`: the pins the firmware uses (`TFT_*`, `SD_CS`, `BAT_SENSE_PIN`, `PWR_SENSE_PIN`).
- `docs/speaker-output.md`: PCM5102A + PAM8403 plan (pins, jumpers, currents).
- `docs/display-research.md`: rotary encoder choice and pins.
- `docs/olimex-jumpers-bottom.png`: the three solder jumpers for battery/USB sensing on the Olimex board.
- Olimex ESP32-DevKit-LiPo **Rev.D** open hardware (KiCad sources, take the exact header
  positions, board outline and mounting holes from there):
  https://github.com/OLIMEX/ESP32-DevKit-LiPo/tree/master/HARDWARE

## Deliverables (when the design is done)

1. KiCad project `KiCad/esp-iradio/` (`.kicad_pro`, `.kicad_sch`, `.kicad_pcb`, project-local
   symbol/footprint libraries in `KiCad/esp-iradio/lib/`).
2. Fabrication output in `KiCad/esp-iradio/fab/`: Gerbers + drill files, BOM (CSV with LCSC
   codes) and pick-and-place (CPL) file in JLCPCB's format.
3. For the 3D-printed case: a STEP of the assembled board and a DXF of the outline and holes in
   `KiCad/esp-iradio/case/`.
4. A short `KiCad/esp-iradio/NOTES.md`: design decisions, deviations from these documents, revision
   history (Rev A, B, ...).
5. If a pin changes: update `src/config.h`, `AGENTS.md` and `02-pinout-and-nets.md` in the same
   step. The firmware and the board must never disagree.

## Tooling on the owner's PC (2026-10-11)

- **KiCad 9.0.9** (`C:\Program Files\KiCad\9.0`). Chosen over EasyEDA Pro for git-friendly text
  files and AI access; JLCPCB output comes from plugins.
- **KiCad MCP server** for Claude Code: mixelpixx/KiCAD-MCP-Server v2.8.2 (branch `stable`, MIT),
  cloned to `C:\Users\Admin\mcp-servers\KiCAD-MCP-Server`. Python: a venv in that folder (`venv\`,
  made from KiCad's own Python 3.11 with `--system-site-packages`, so `pcbnew` imports and the KiCad
  install itself stays untouched) with `requirements.txt`; the server finds the venv by itself.
  Node 24, `npm install` + `npm run build` -> `dist\index.js`. Registered in Claude Code at user
  scope as `kicad` (`claude mcp add kicad --scope user -e KICAD_AUTO_LAUNCH=false ... -- node
  ...\dist\index.js`); the tools appear in a new Claude Code session. Update: `git pull` in that
  folder, `npm run build`, `venv\Scripts\python -m pip install -r requirements.txt`.
- Pitfall: the server writes new sheets in the KiCad 10 file format, which KiCad 9 skips
  without an error (empty netlist). Its templates are patched to KiCad 9 on this PC; details and
  the fix in `esp-iradio/NOTES.md` ("Tooling pitfall"). Check every change with
  `kicad-cli sch export netlist` (component count) and `kicad-cli sch erc`.
- Optional, for live sync with an open KiCad window: KiCad -> Preferences -> Plugins -> enable
  the KiCad API (IPC). The file-based tools work without it.
- To install in KiCad (Plugin and Content Manager), owner's choice: **kicad-jlcpcb-tools** (LCSC
  part search with stock, `LCSC` fields, JLCPCB BOM/CPL export) and **easyeda2kicad** (symbol,
  footprint and 3D model of an LCSC part).

## Commands (`kicad-cli`, run inside `esp-iradio/`)

`C:\Program Files\KiCad\9.0\bin\kicad-cli.exe`:
- ERC: `kicad-cli sch erc --severity-all -o erc.rpt esp-iradio.kicad_sch`
- Netlist (check the component count after every schematic change):
  `kicad-cli sch export netlist --format kicadsexpr -o esp-iradio.net esp-iradio.kicad_sch`
- Schematic PDF/SVG: `kicad-cli sch export pdf esp-iradio.kicad_sch` (or `export svg -o <dir>`)
- DRC: `kicad-cli pcb drc esp-iradio.kicad_pcb`
- Gerbers/drill: `kicad-cli pcb export gerbers esp-iradio.kicad_pcb -o fab/`,
  `kicad-cli pcb export drill esp-iradio.kicad_pcb -o fab/`; JLCPCB BOM/CPL via the JLCPCB Tools
  plugin in the KiCad GUI.
- STEP for the case: `kicad-cli pcb export step esp-iradio.kicad_pcb -o case/esp-iradio.step`

Close the schematic/board in KiCad (or reload afterwards) before an agent edits the files: KiCad
overwrites external changes; `*.lck` files mean KiCad has them open. Only one agent session works
on the KiCad project at a time.

## Rules for agents

- Do not invent part numbers, LCSC codes or module dimensions: look them up (datasheet, supplier
  page) or mark them `TBD` and list them under open questions.
- Ask the owner before ordering anything or choosing between options marked "owner decides".
- Everything in this folder is English; talk to the owner in Dutch.
