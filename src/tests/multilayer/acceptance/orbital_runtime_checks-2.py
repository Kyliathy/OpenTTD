"""Independently malformed native/copy processing ownership fixtures.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10C
Only copied transport bytes are altered; no fake native module, cargo, consumer or paid progress is installed.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, identity, read_stream
from launch_center_checks import riff_location

RUN = runpy.run_path(str(Path(__file__).with_name("orbital_runtime_checks-1.py")))["run"]


def offsets(raw):
    """Locate original processing fields solely for independent malformed-file construction.

    @param raw Actual original SPRC payload from the explicit valid ordinary file.
    @return Exact original field offsets and record boundaries; this decoder creates no runtime ownership.
    """
    position = 0

    def field(width):
        """Read one admitted original scalar and retain its position.

        @param width Exact protocol byte count.
        @return Original position, unsigned quantity and encoded width.
        """
        nonlocal position
        assert position + width <= len(raw)
        result = position, int.from_bytes(raw[position:position + width], "little"), width
        position += width
        return result

    def counters():
        """Read original stable label/counter pairs for targeted corruptions.

        @return Original per-label scalar positions, without interpreting them as live cargo or work.
        """
        nonlocal position
        result = {}
        # Every original key identifies one transported historical counter; duplicate original input would invalidate the purported positive source.
        for _ in range(field(1)[1]):
            name = raw[position:position + 4].decode("ascii")
            position += 4
            assert name not in result
            result[name] = field(8)
        return result

    field(8)
    counters()
    count = field(2)
    rows = []
    # Independently decode only the transport layout needed for malicious edits; every quantity remains detached metadata until the actual native/copy reader refuses it.
    for _ in range(count[1]):
        start = position
        values = {name: field(width) for name, width in (("site", 2), ("generation", 8), ("choice", 4), ("source", 2), ("source_generation", 8),
                                                      ("consumer", 4), ("revision", 8), ("baseline", 8), ("enabled", 1), ("last_tick", 8), ("paid_lifetime", 8), ("discarded", 8))}
        receipt = {}
        if field(1)[1]:
            receipt = {name: field(width) for name, width in (("recipe", 4), ("consumer", 4), ("work", 4), ("power", 8),
                                                            ("paid", 8), ("sequence", 8), ("interval", 8), ("interval_paid", 8))}
        maps = {name: counters() for name in ("batches", "consumed", "produced", "residue")}
        rows.append(dict(start=start, end=position, fields=values, receipt=receipt, maps=maps))
    assert position == len(raw) and count[1] == 1
    return count, rows


def damage(context, source, fault):
    """Alter one independent original ownership/work boundary and retain an actual ordinary reader candidate.

    @param context Exact repository-owned malformed-candidate artifact destination.
    @param source Explicit valid original partial processing fixture.
    @param fault Exact semantic fault or required-family absence.
    @return Original-version native file with only that processing family changed.
    """
    version, stream = read_stream(source)
    assert version == 438
    start, body, length, end = riff_location(stream, b"SPRC")
    raw = bytearray(stream[body:body + length])
    count, rows = offsets(raw)
    row = rows[0]

    def replace(location, value):
        """Replace a scalar only at its independently decoded original field.

        @param location Actual original position/value/width tuple.
        @param value Explicit malformed unsigned replacement.
        """
        position, _, width = location
        raw[position:position + width] = value.to_bytes(width, "little")

    if fault == "paid":
        replace(row["receipt"]["paid"], row["receipt"]["paid"][1] + 750)
    elif fault == "history":
        replace(row["maps"]["consumed"]["NORE"], 4)
    elif fault == "residue":
        replace(row["maps"]["residue"]["OSTL"], 32)
    elif fault == "future-opportunity":
        replace(row["receipt"]["interval"], (1 << 64) - 1)
    elif fault == "consumer":
        replace(row["fields"]["consumer"], 0)
        replace(row["receipt"]["consumer"], 0)
    elif fault == "duplicate-owner":
        raw.extend(raw[row["start"]:row["end"]])
        replace(count, 2)
    else:
        assert fault == "missing"
    if fault == "missing":
        changed = stream[:start] + stream[end:]
    else:
        size = len(raw)
        header = bytes([(size >> 24) << 4, (size >> 16) & 255, (size >> 8) & 255, size & 255])
        changed = stream[:start + 4] + header + raw + stream[end:]
    target = context.saves / ("malformed-orbital-recipe-" + fault + ".sav")
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return target


def main():
    """Execute one actual full/copy refusal pair in a bounded burst and retain exact source/report membership."""
    from runner import current_run_context
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--fault", choices=("paid", "history", "residue", "future-opportunity", "consumer", "duplicate-owner", "missing"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    context = current_run_context("space", "orbital-recipe-refusal")
    context.prepare_runtime()
    source = args.source.resolve()
    candidate = damage(context, source, args.fault)
    copied = RUN(args.configuration, "file", "partial", source=source, corrupt=candidate)
    full = RUN(args.configuration, "file", "partial", source=candidate, reject=True)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, configuration=args.configuration, fault=args.fault, complete=True,
                                          source=identity(source), candidate=identity(candidate), reports=[copied.relative_to(ROOT).as_posix(), full.relative_to(ROOT).as_posix()]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
