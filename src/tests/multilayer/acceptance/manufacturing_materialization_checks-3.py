"""Reject one malformed paid work/completion/provenance boundary through independent native full load and copied preview.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8
Bounded427 inspection only locates existing native fields; it creates no gameplay inventory, fleet or utility authority.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from runner import current_run_context
from space_preview_checks import ROOT, identity, read_stream, Reader, CHUNK_RIFF, CHUNK_RECORDS

native = runpy.run_path(str(Path(__file__).with_name("manufacturing_materialization_checks-1.py")))["native"]
KINDS = ("old-version", "work", "energy", "interval", "phase", "product-generation", "consumption", "factory", "serial", "duplicate-origin")


def layout(raw):
    """Locate exact original SPST completion and SVST orbital-origin offsets through bounded native framing.

    @param raw Complete uncompressed native stream, excluding its eight-byte file header.
    @return Completion fields, its exact product's commissioning row and all other native commissioning rows.
    """
    reader = Reader(raw); families = {}
    # Chunk framing supplies the only authority for offsets; a coincidental cargo/string byte pattern cannot select a native owner.
    while True:
        name = reader.take(4)
        if name == bytes(4):
            break
        mode = reader.byte(); kind = mode & 15
        if kind == CHUNK_RIFF:
            length = ((mode >> 4) << 24) | (reader.byte() << 16) | (reader.byte() << 8) | reader.byte()
            if name in (b"SPST", b"SVST"):
                assert name not in families
                families[name] = (reader.pos, reader.pos + length)
            reader.take(length)
        elif kind in CHUNK_RECORDS:
            while True:
                length = reader.gamma()
                if not length:
                    break
                reader.take(length - 1)
        else:
            raise AssertionError("unsupported native framing")
    assert set(families) == {b"SPST", b"SVST"}
    position, end = families[b"SPST"]

    def read(width):
        """Read only inside the admitted current native family.

        @param width Nonnegative exact field extent.
        @return Its unsigned big-endian integer value.
        """
        nonlocal position
        assert 0 <= width <= end - position
        value = int.from_bytes(raw[position:position + width], "big"); position += width
        return value

    def packets(maximum=64):
        """Skip one native physical packet list after its whole quantity and reference-count bounds.

        @param maximum Exact containing native slot's reference limit.
        """
        read(2); count = read(2); assert count <= maximum; read(6 * count)

    def service():
        """Skip one complete original31-byte service state and its two bounded quantity/reference lists."""
        read(31); packets(1); packets(1)

    read(8); count = read(2); read(32); assert count <= (end - position) // 62
    jobs = []
    # Walk every original construction/facility/stock/service owner before its append-only manufacturing payload; preserve exact old work receipts afterward.
    for _ in range(count):
        read(2); read(8); read(1); kind = read(1); read(17)
        for _ in range(2):
            read(2); packets()
        present = read(1); assert present <= 1
        if present:
            read(4); sparse = read(1); assert sparse <= 64
            for _ in range(sparse):
                read(1); packets()
            read(17); service(); read(4); read(10); service(); read(8)
            has_job = read(1); assert has_job <= 1
            if has_job:
                fields = {}
                for name, width in (("phase", 1), ("model", 2), ("serial", 8), ("source", 10), ("capital", 4), ("berth", 1), ("reservation", 8)):
                    fields[name] = position; read(width)
                for _ in range(4):
                    packets()
                for name, width in (("work", 4), ("interval", 8), ("energy", 8), ("product", 12), ("consumption", 8)):
                    fields[name] = position; read(width)
                if kind == 9:
                    jobs.append(fields)
        read(20)
    assert position == end and len(jobs) == 1
    job = jobs[0]; product = raw[job["product"]:job["product"] + 12]
    position, end = families[b"SVST"]; count = read(1); assert end - position == 67 * count and count <= 64
    rows = []
    # Every native hull keeps its exact67-byte original-plus-orbital commissioning record; locate the product by its full incarnation rather than pool-row index.
    for _ in range(count):
        start = position; craft = raw[start:start + 12]; read(67); rows.append((start, craft))
    matching = [start for start, craft in rows if craft == product]; assert len(matching) == 1
    return job, matching[0], rows


def damage(context, source, kind):
    """Change exactly one semantic work, consumption or original-provenance boundary in an owned native copy.

    @param context Explicit malformed-copy artifact owner.
    @param source Exact genuine accepted completed checkpoint.
    @param kind One independent bounded malformed field or duplicate provenance fault.
    @return New uncompressed malformed save, with all original input bytes unchanged.
    """
    version, raw = read_stream(source); assert version == 427
    fields, product, rows = layout(raw); changed = bytearray(raw)
    if kind == "old-version":
        version = 426
    elif kind == "work":
        value = int.from_bytes(raw[fields["work"]:fields["work"] + 4], "big")
        changed[fields["work"]:fields["work"] + 4] = struct.pack(">I", value + 1)
    elif kind == "energy":
        value = int.from_bytes(raw[fields["energy"]:fields["energy"] + 8], "big")
        changed[fields["energy"]:fields["energy"] + 8] = struct.pack(">Q", value - 1)
    elif kind == "interval":
        changed[fields["interval"]:fields["interval"] + 8] = bytes([255]) * 8
    elif kind == "phase":
        changed[fields["phase"]] = 3
    elif kind == "product-generation":
        changed[fields["product"] + 4:fields["product"] + 12] = bytes([255]) * 8
    elif kind == "consumption":
        model = int.from_bytes(raw[fields["model"]:fields["model"] + 2], "big"); last = 3 if model in (16, 17) else 1
        offset = fields["consumption"] + 2 * last; changed[offset:offset + 2] = bytes(2)
    elif kind == "factory":
        changed[product + 51:product + 59] = bytes([255]) * 8
    elif kind == "serial":
        value = int.from_bytes(raw[product + 59:product + 67], "big")
        changed[product + 59:product + 67] = struct.pack(">Q", value + 1)
    else:
        assert kind == "duplicate-origin"
        candidates = [start for start, _ in rows if start != product and int.from_bytes(raw[start + 41:start + 43], "big") == 20]
        assert candidates; other = candidates[0]
        # Forge only provenance on a different real already paid orbital builder: its native Engine/model/capital/placement stay valid, isolating duplicate factory-job rejection.
        empty_center = struct.pack(">HQ", 65535, 0)
        changed[other + 13:other + 33] = empty_center * 2; changed[other + 33:other + 37] = bytes([255]) * 4
        changed[other + 49:other + 67] = raw[product + 49:product + 67]
    target = context.saves / ("manufacture-malformed-" + kind + ".sav")
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return target


def verify(configuration, proof_path, kind):
    """Require independent full/copied-preview rejection while original current-world owners remain unchanged.

    @param configuration Completed native configuration matching the exact accepted proof.
    @param proof_path Exact accepted complete-phase report, never a latest-run lookup.
    @param kind One independent paid work/completion/provenance fault.
    @return Accepted hash-bound rejection report.
    """
    proof = json.loads(proof_path.read_text(encoding="utf-8"))
    assert proof["configuration"] == configuration and proof["phase"] == "complete" and proof["save_version"] == 427
    model = proof["model"]; source = ROOT / proof["after"]["path"]; original = identity(source); assert original == proof["after"]
    context = current_run_context("space", f"manufacturing-materialization-reject-{kind}-{model}")
    malformed = damage(context, source, kind)
    full = context.for_feature("space", f"manufacturing-materialization-full-{kind}-{model}")
    native(full, configuration, "repeat", model, malformed, reject=True)
    preview = context.for_feature("space", f"manufacturing-materialization-preview-{kind}-{model}")
    native(preview, configuration, "repeat", model, source, corrupt=malformed)
    assert identity(source) == original
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group8; independent native paid-work, completion material and original factory/job provenance refusal.",
                  configuration=configuration, model=model, kind=kind, proof=identity(proof_path), source=original, malformed=identity(malformed),
                  native_reports=[identity(path) for owner in (full, preview) for path in sorted(owner.reports.glob("native-*.json"))])
    report = context.reports / "manufacturing-materialization-rejection.json"
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Select one exact completed original-world proof and one independent malformed boundary."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--proof", type=Path, required=True)
    parser.add_argument("--kind", choices=KINDS, required=True)
    args = parser.parse_args()
    print(verify(args.configuration, args.proof.resolve(), args.kind).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
