"""Identify available old-save types using oldloader.cpp's actual name checksum."""

from pathlib import Path

for path in sorted(Path("!kit/TT").glob("*.sv*")):
    data = path.read_bytes()[:49]
    for size, kind in ((41, "TTO"), (49, "TTD")):
        checksum = 0
        for byte in data[:size - 2]:
            checksum = (checksum + (byte if byte < 128 else byte - 256)) & 0xffff
            checksum = ((checksum << 1) | (checksum >> 15)) & 0xffff
        if checksum ^ 0xaaaa == int.from_bytes(data[size - 2:size], "little"):
            print(path, kind, data[:size - 2].split(bytes(1))[0])
            break
    else:
        print(path, "unknown")
