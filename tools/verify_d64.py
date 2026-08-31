#!/usr/bin/env python3
"""Perform structural checks on the generated 1541 disk image."""

from pathlib import Path
import sys

SECTORS = [0] + [21] * 17 + [19] * 7 + [18] * 6 + [17] * 5


def offset(t, s):
    assert 1 <= t <= 35 and 0 <= s < SECTORS[t]
    return (sum(SECTORS[1:t]) + s) * 256


def chain(image, t, s):
    seen = set()
    data = bytearray()
    while t:
        assert (t, s) not in seen, "cyclic sector chain"
        seen.add((t, s))
        p = offset(t, s)
        nt, ns = image[p], image[p + 1]
        if nt:
            data += image[p + 2:p + 256]
        else:
            assert 1 <= ns <= 255
            data += image[p + 2:p + 1 + ns]
        t, s = nt, ns
    return bytes(data), seen


def main():
    image = Path(sys.argv[1]).read_bytes()
    assert len(image) == 174848, "not a standard 35-track D64"
    directory = offset(18, 1)
    names = []
    occupied = set()
    for ix in range(8):
        p = directory + 2 + ix * 32
        if not image[p]:
            continue
        name = image[p + 3:p + 19].rstrip(b"\xa0").decode("ascii")
        content, sectors = chain(image, image[p + 1], image[p + 2])
        assert not occupied.intersection(sectors), "cross-linked files"
        occupied.update(sectors)
        names.append((name, len(content)))
    program_size = len(Path("C77_AGENDA_GFX.prg").read_bytes())
    assert names[0] == ("C77 AGENDA GFX", program_size)
    assert names[1] == ("AGENDA.DAT", 11)
    assert names[0][1] > 2 and image[offset(image[directory+3], image[directory+4])+2:][:2] == b"\x01\x08"
    print("D64 OK:", ", ".join(f"{name} ({size} bytes)" for name, size in names))


if __name__ == "__main__":
    main()
