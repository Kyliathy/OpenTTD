"""Print the length and SHA-256 of every map plane chunk (MAPT to MAP8) of an OpenTTD save.

Written for docs/zz-reach2/upgrades/2026-10/map-layers/r2rvm-review-map.md (task 2.2, master-plan task 1.7).
The container, gamma lengths and chunk types it reads are described in
docs/zz-reach2/architecture/savegame/archi-savegame.md.

Usage: python savegame_plane_digest.py <save> [--tiles N]
       python savegame_plane_digest.py <save> --portals --size-x N --size-y N
@see docs/zz-reach2/upgrades/2026-10/r2mlm13-layer-art-review-and-new-sprites.md

With --tiles N only the first N tiles of each plane are hashed: N bytes of an
8-bit plane, 2N bytes of MAP2 and MAP8. That makes the surface plane of a
layered save comparable with the single plane of a pre-epic save.
"""

import argparse
import hashlib
import lzma
import sys
import zlib

#Bytes one tile takes in each map plane chunk, in the order map_sl.cpp saves them.
PLANE_BYTES = {
    b"MAPT": 1, b"MAPH": 1, b"MAPO": 1, b"MAP2": 2, b"M3LO": 1,
    b"M3HI": 1, b"MAP5": 1, b"MAPE": 1, b"MAP7": 1, b"MAP8": 2,
}

#ChunkType values from saveload.h. Riff carries a 28-bit length; the others are gamma-length records.
CHUNK_RIFF = 0
CHUNK_RECORDS = (1, 2, 3, 4)


class Reader:
    """Sequential reader over the decompressed chunk stream."""

    def __init__(self, data):
        """Start at the first byte.

        @param data Decompressed chunk stream.
        """
        self.data = data
        self.pos = 0

    def take(self, count):
        """Read a run of bytes.

        @param count How many bytes to read.
        @return The bytes.
        """
        #A chunk that claims more bytes than the stream holds means the parse went wrong.
        if self.pos + count > len(self.data):
            raise ValueError(f"read of {count} bytes at offset {self.pos} runs past the end of the stream")
        chunk = self.data[self.pos:self.pos + count]
        self.pos += count
        return chunk

    def byte(self):
        """Read one byte.

        @return The byte value.
        """
        return self.take(1)[0]

    def gamma(self):
        """Read a simple gamma length, exactly as SlReadSimpleGamma does.

        @return The decoded value.
        """
        i = self.byte()
        #Each leading 1 bit announces one more byte, up to the 32-bit form.
        if i & 0x80:
            i &= ~0x80
            if i & 0x40:
                i &= ~0x40
                if i & 0x20:
                    i &= ~0x20
                    if i & 0x10:
                        if i & 0x08:
                            raise ValueError("unsupported gamma")
                        i = self.byte()
                    i = (i << 8) | self.byte()
                i = (i << 8) | self.byte()
            i = (i << 8) | self.byte()
        return i


def read_stream(path):
    """Read a save and decompress its chunk stream.

    @param path Save file.
    @return The savegame version and the decompressed chunk stream.
    """
    with open(path, "rb") as f:
        raw = f.read()
    tag = raw[0:4]
    version = int.from_bytes(raw[4:6], "big")
    payload = raw[8:]
    #The tag names the compressor (SaveLoadFormatTag in saveload.cpp). LZO saves are not supported.
    if tag == b"OTTX":
        return version, lzma.decompress(payload)
    if tag == b"OTTZ":
        return version, zlib.decompress(payload)
    if tag == b"OTTN":
        return version, payload
    raise ValueError(f"unsupported save format {tag!r}; save with savegame_format lzma, zlib or none")


def list_portals(planes, size_x, size_y):
    """Print stable machine-readable scene coordinates for each portal end.

    @param planes Map-plane payloads keyed by chunk id.
    @param size_x Tiles across one layer.
    @param size_y Tiles down one layer.
    @return None.
    """
    if size_x <= 0 or size_y <= 0:
        raise ValueError("map dimensions must be positive")
    types, fields = planes.get(b"MAPT"), planes.get(b"MAP5")
    if types is None or fields is None or len(types) != len(fields):
        raise ValueError("MAPT and MAP5 must both exist and have matching lengths")
    plane = size_x * size_y
    if len(types) % plane or not 1 <= len(types) // plane <= 4:
        raise ValueError("map-plane lengths do not match the supplied dimensions and 1..4 layers")
    names = ("NE", "SE", "SW", "NW")
    for index, tile_type in enumerate(types):
        #Only LayerPortal tiles expose these m5 fields; other structures share the byte.
        if tile_type >> 4 != 11:
            continue
        m5 = fields[index]
        transport = (m5 >> 2) & 3
        if transport not in (0, 1):
            raise ValueError(f"portal {index} has invalid transport {transport}")
        layer, position = divmod(index, plane)
        y, x = divmod(position, size_x)
        print(f"portal tile {index} layer {layer} x {x} y {y} shaft {names[m5 & 3]} "
              f"door {names[(m5 & 3) ^ 2]} transport {'rail' if transport == 0 else 'road'} target {(m5 >> 5) & 3}")


def main():
    """Hash the map planes or list portal scenes without changing the save.

    @return Process exit code.
    """
    parser = argparse.ArgumentParser(description="Hash the map planes of an OpenTTD save.")
    parser.add_argument("save", help="path of the .sav file")
    parser.add_argument("--tiles", type=int, default=None, help="hash only the first N tiles of each plane")
    parser.add_argument("--portals", action="store_true", help="list each portal end instead of hashing")
    parser.add_argument("--size-x", type=int, help="tiles across one plane, required with --portals")
    parser.add_argument("--size-y", type=int, help="tiles down one plane, required with --portals")
    args = parser.parse_args()
    if args.portals and (args.size_x is None or args.size_y is None or args.tiles is not None):
        parser.error("--portals requires --size-x and --size-y, and cannot be combined with --tiles")

    version, stream = read_stream(args.save)
    #Keep the original digest output, but portal mode emits only parseable scene lines.
    if not args.portals:
        print(f"file {args.save} version {version}")
    planes = {}
    reader = Reader(stream)
    while True:
        chunk_id = reader.take(4)
        #Four zero bytes end the chunk stream.
        if chunk_id == b"\0\0\0\0":
            break
        m = reader.byte()
        chunk_type = m & 0x0F
        if chunk_type == CHUNK_RIFF:
            #The high nibble of the type byte holds length bits 24 to 27.
            length = ((m >> 4) << 24) | (reader.byte() << 16) | (reader.byte() << 8) | reader.byte()
            payload = reader.take(length)
            #Only map planes are reported. Other RIFF chunks are skipped.
            if chunk_id in PLANE_BYTES:
                if args.portals:
                    if chunk_id in (b"MAPT", b"MAP5"):
                        planes[chunk_id] = payload
                else:
                    part = payload if args.tiles is None else payload[:args.tiles * PLANE_BYTES[chunk_id]]
                    print(f"{chunk_id.decode()} length {length} hashed {len(part)} sha256 {hashlib.sha256(part).hexdigest()}")
            continue
        if chunk_type not in CHUNK_RECORDS:
            raise ValueError(f"chunk {chunk_id!r} has unknown type {chunk_type}")
        #Array and table chunks are records with a gamma length (bytes + 1). Gamma 0 ends the chunk.
        while True:
            length = reader.gamma()
            if length == 0:
                break
            reader.take(length - 1)
    if args.portals:
        list_portals(planes, args.size_x, args.size_y)
    return 0


if __name__ == "__main__":
    sys.exit(main())
