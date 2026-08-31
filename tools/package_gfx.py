#!/usr/bin/env python3
"""Package the graphical native agenda and a writable seed database as D64."""

from pathlib import Path
import struct
import sys

SECTORS = [0] + [21] * 17 + [19] * 7 + [18] * 6 + [17] * 5


def ts_offset(track, sector):
    return (sum(SECTORS[1:track]) + sector) * 256


def petscii_name(name, length=16):
    return name.upper().encode("ascii")[:length].ljust(length, b"\xa0")


def make_d64(files):
    img = bytearray(174848)
    free = {(t, s) for t in range(1, 36) for s in range(SECTORS[t])}
    free -= {(18, 0), (18, 1)}
    bam = ts_offset(18, 0)
    img[bam:bam + 4] = bytes([18, 1, 0x41, 0])
    img[bam + 0x90:bam + 0xA0] = petscii_name("C77 GFX AGENDA")
    img[bam + 0xA2:bam + 0xA4] = b"77"
    img[bam + 0xA5:bam + 0xA7] = b"2A"
    directory = ts_offset(18, 1)
    img[directory:directory + 2] = b"\x00\xff"
    entries = []
    for name, data, ftype in files:
        chain = []
        remaining = data
        while remaining:
            t, s = min(free)
            free.remove((t, s))
            chain.append((t, s))
            chunk, remaining = remaining[:254], remaining[254:]
            off = ts_offset(t, s)
            img[off + 2:off + 2 + len(chunk)] = chunk
        final_size = len(data) % 254 or 254
        for ix, (t, s) in enumerate(chain):
            off = ts_offset(t, s)
            if ix + 1 < len(chain):
                img[off:off + 2] = bytes(chain[ix + 1])
            else:
                img[off:off + 2] = bytes([0, final_size + 1])
        entries.append((name, chain[0], len(chain), ftype))
    for ix, (name, (t, s), blocks, ftype) in enumerate(entries):
        off = directory + 2 + ix * 32
        img[off] = 0x80 | ftype
        img[off + 1:off + 3] = bytes([t, s])
        img[off + 3:off + 19] = petscii_name(name)
        img[off + 28:off + 30] = struct.pack("<H", blocks)
    for t in range(1, 36):
        available = [s for s in range(SECTORS[t]) if (t, s) in free]
        off = bam + 4 * t
        img[off] = len(available)
        mask = sum(1 << s for s in available)
        img[off + 1:off + 4] = mask.to_bytes(3, "little")
    return img


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: package_gfx.py PROGRAM.prg OUTPUT.d64")
    program = Path(sys.argv[1]).read_bytes()
    seed = b"20260831\r0\r"
    image = make_d64([("C77 AGENDA GFX", program, 2), ("AGENDA.DAT", seed, 1)])
    Path(sys.argv[2]).write_bytes(image)
    print(f"PRG: {len(program)} bytes; D64: {len(image)} bytes")


if __name__ == "__main__":
    main()
