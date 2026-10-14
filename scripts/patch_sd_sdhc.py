# Both envs: one fix in the Arduino SD library (libraries/SD/src/sd_diskio.cpp) of the
# PlatformIO package framework-arduinoespressif32, reapplied before every build.
#
# ff_sd_initialize() decides between byte addressing (SDSC, CARD_SD) and block addressing
# (SDHC/SDXC, CARD_SDHC) from CMD8 and the CCS bit of the OCR. A new 16 GB card (2026-10-14)
# ended up as CARD_SD (no OCR line was logged, so it took the SD v1 path), so the library used
# byte addresses (sector << 9):
# sector 0 still read right, every other sector came from the wrong place (f_mount: "no valid
# FAT volume") and every write ended in an address/parameter error (CMD13 R1 0x33). The card's
# CSD is version 2.0, which only block-addressed cards have, and its size (31,129,600 sectors)
# is far beyond the 2 GB SDSC limit. The patch logs the OCR and switches such a card to
# CARD_SDHC once the size is known. A package reinstall restores the original; the next build
# puts the patch back.
Import("env")

from pathlib import Path

MARK = "/* iradio: sdhc fix */"
pkg = Path(env.PioPlatform().get_package_dir("framework-arduinoespressif32"))
src = pkg / "libraries/SD/src/sd_diskio.cpp"
text = src.read_text(encoding="utf-8")
if MARK in text:
    print("SD SDHC fix already in", src)
else:
    nl = "\r\n" if "\r\n" in text else "\n"
    t = text.replace("\r\n", "\n")
    old_ocr = """        if (!sdTransaction(pdrv, READ_OCR, 0, &resp)) {
            if (resp & (1 << 30)) {"""
    new_ocr = """        if (!sdTransaction(pdrv, READ_OCR, 0, &resp)) {
            log_w("OCR %08X", resp); """ + MARK + """
            if (resp & (1 << 30)) {"""
    old_size = """    card->sectors = sdGetSectorsCount(pdrv);

    if (card->frequency > 25000000) {"""
    new_size = """    card->sectors = sdGetSectorsCount(pdrv);
    """ + MARK + """
    if (card->type == CARD_SD && card->sectors > 4194304) {
        log_w("card without the OCR CCS bit but %lu sectors: block addressing (SDHC)", (unsigned long)card->sectors);
        card->type = CARD_SDHC;
    }

    if (card->frequency > 25000000) {"""
    if t.count(old_ocr) != 1 or t.count(old_size) != 1:
        raise SystemExit("patch_sd_sdhc: anchor not found in %s" % src)
    t = t.replace(old_ocr, new_ocr).replace(old_size, new_size)
    src.write_text(t.replace("\n", nl), encoding="utf-8", newline="")
    print("SD SDHC fix applied to", src)
