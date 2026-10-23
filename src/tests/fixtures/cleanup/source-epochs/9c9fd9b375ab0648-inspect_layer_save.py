"""Inspect layered OpenTTD saves without changing files or launching the game.

Preserves the supplied-save diagnostic from build/inspect_review_save.py, with
explicit dimensions, inferred layer count and no record-export side effects.
Use Python -B so importing the shared reader does not create bytecode files.

Usage: python -B inspect_layer_save.py <save> --size-x 256 --size-y 256
       python -B inspect_layer_save.py <save> --size-x 256 --size-y 256 --summary-only
@see docs/zz-reach2/architecture/save-game.md
@see docs/zz-reach2/architecture/savegame/archi-savegame.md
"""

import argparse
from collections import Counter
from pathlib import Path

from savegame_plane_digest import CHUNK_RECORDS, CHUNK_RIFF, PLANE_BYTES, Reader, list_portals, read_stream


LAYER_NAMES = ("Surface", "Underground", "Outer Space", "Deep Underground")
TILE_NAMES = {
    0: "clear", 1: "rail", 2: "road", 3: "house", 4: "trees",
    5: "station", 6: "water", 7: "void", 8: "industry",
    9: "tunnel/bridge", 10: "object", 11: "layer portal",
}


def read_chunks(stream):
    """Collect map payloads and record lengths from a decompressed save.

    @param stream Decompressed bytes returned by the shared save reader.
    @return Map payloads and array/table record lengths keyed by chunk ID.
    """
    reader = Reader(stream)
    planes = {}
    records = {}
    while True:
        chunk = reader.take(4)
        if chunk == bytes(4):
            return planes, records
        mode = reader.byte()
        chunk_type = mode & 15
        if chunk_type == CHUNK_RIFF:
            length = ((mode >> 4) << 24) | (reader.byte() << 16) | (reader.byte() << 8) | reader.byte()
            payload = reader.take(length)
            if chunk in PLANE_BYTES:
                planes[chunk] = payload
            continue
        if chunk_type not in CHUNK_RECORDS:
            raise ValueError(f"chunk {chunk!r} has unsupported type {chunk_type}")
        lengths = []
        while True:
            length = reader.gamma()
            if not length:
                break
            reader.take(length - 1)
            lengths.append(length - 1)
        records[chunk] = lengths


def inspect_tiles(planes, size_x, size_y, summary_only):
    """Report each layer and its rail, station and portal tile fields.

    @param planes Map payloads keyed by chunk ID.
    @param size_x Tiles across one plane, supplied by the caller.
    @param size_y Tiles down one plane, supplied by the caller.
    @param summary_only Whether to omit individual tile and portal listings.
    @return None.
    """
    if size_x <= 0 or size_y <= 0:
        raise ValueError("map dimensions must be positive")
    missing = set(PLANE_BYTES) - set(planes)
    if missing:
        raise ValueError(f"missing map chunks: {', '.join(sorted(chunk.decode() for chunk in missing))}")
    plane_size = size_x * size_y
    tile_count = len(planes[b"MAPT"])
    layers, remainder = divmod(tile_count, plane_size)
    if remainder or not 1 <= layers <= len(LAYER_NAMES):
        raise ValueError("MAPT length does not match the supplied dimensions and 1..4 layers")
    for chunk, bytes_per_tile in PLANE_BYTES.items():
        if len(planes[chunk]) != tile_count * bytes_per_tile:
            raise ValueError(f"{chunk.decode()} length disagrees with MAPT")
    print(f"dimensions {size_x} x {size_y} layers {layers} tiles {tile_count}")
    for layer in range(layers):
        start = layer * plane_size
        types = planes[b"MAPT"][start:start + plane_size]
        counts = Counter(tile_type >> 4 for tile_type in types)
        totals = ", ".join(f"{TILE_NAMES.get(kind, str(kind))}={count}" for kind, count in sorted(counts.items()))
        print(f"layer {layer} {LAYER_NAMES[layer]}: {totals}")
        if summary_only:
            continue
        for position, tile_type in enumerate(types):
            kind = tile_type >> 4
            if kind not in (1, 5, 11):
                continue
            tile = start + position
            m2 = int.from_bytes(planes[b"MAP2"][tile * 2:tile * 2 + 2], "big")
            print(f"tile {tile} layer {layer} x {position % size_x} y {position // size_x} "
                  f"type {TILE_NAMES[kind]} m2 {m2} m3 {planes[b'M3LO'][tile]} "
                  f"m4 {planes[b'M3HI'][tile]} m5 {planes[b'MAP5'][tile]}")
    if not summary_only:
        list_portals(planes, size_x, size_y)


def main():
    """Read one save and print its inventory with no write operations.

    @return None.
    """
    parser = argparse.ArgumentParser(description="Read-only layer, track, station and portal inventory.")
    parser.add_argument("save", type=Path, help="lzma, zlib or uncompressed OpenTTD save")
    parser.add_argument("--size-x", type=int, required=True, help="tiles across one plane")
    parser.add_argument("--size-y", type=int, required=True, help="tiles down one plane")
    parser.add_argument("--summary-only", action="store_true", help="omit per-tile and portal listings")
    args = parser.parse_args()
    version, stream = read_stream(args.save)
    planes, records = read_chunks(stream)
    print(f"file {args.save} version {version}")
    for chunk in (b"MAPS", b"VEHS", b"STNN", b"STNS", b"ROAD", b"ORDR", b"ORDL", b"BKOR"):
        if chunk not in records:
            continue
        lengths = records[chunk]
        print(f"{chunk.decode()} records {len(lengths)} first lengths {lengths[:8]}")
    inspect_tiles(planes, args.size_x, args.size_y, args.summary_only)


if __name__ == "__main__":
    main()
