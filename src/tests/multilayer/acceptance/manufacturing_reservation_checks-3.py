"""Reject one malformed manufacturing reservation through independent full load and copied preview.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group7
The bounded426 SPST parser is an inspection tool and creates no gameplay owner or cargo registry.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from runner import current_run_context
from space_preview_checks import ROOT, identity, read_stream, Reader, CHUNK_RIFF, CHUNK_RECORDS

native = runpy.run_path(str(Path(__file__).with_name("manufacturing_reservation_checks-1.py")))["native"]
KINDS = ("old-version", "serial", "source-generation", "missing-source", "role", "capital", "berth", "interval", "phase", "material", "packet-alias", "unused-input")


def layout(raw):
    """Locate exact job-field offsets by parsing native framing and every bounded426 contained owner.

    @param raw Complete uncompressed native stream, excluding its eight-byte file header.
    @return Assembly job offsets, including all four quantity/packet lists.
    """
    reader = Reader(raw); extent = None
    # Native framing, rather than a byte-pattern search, identifies SPST even when another cargo/string contains the same four letters.
    while True:
        name = reader.take(4)
        if name == bytes(4):
            break
        mode = reader.byte(); kind = mode & 15
        if kind == CHUNK_RIFF:
            length = ((mode >> 4) << 24) | (reader.byte() << 16) | (reader.byte() << 8) | reader.byte()
            if name == b"SPST":
                assert extent is None
                extent = (reader.pos, reader.pos + length)
            reader.take(length)
        elif kind in CHUNK_RECORDS:
            while True:
                length = reader.gamma()
                if not length:
                    break
                reader.take(length - 1)
        else:
            raise AssertionError("unsupported native framing")
    assert extent is not None
    position, end = extent

    def read(width):
        """Advance only within the admitted native family.

        @param width Exact nonnegative byte count to decode or skip.
        @return Big-endian integer value.
        """
        nonlocal position
        assert 0 <= width <= end - position
        value = int.from_bytes(raw[position:position + width], "big"); position += width
        return value

    def packets(maximum=64):
        """Read one bounded physical packet-reference count and its exact payload.

        @param maximum Native per-container reference capacity.
        @return Quantity-field offset, reference-payload offset and actual reference count.
        """
        quantity = position; read(2); count = read(2); assert count <= maximum
        references = position; read(6 * count)
        return (quantity, references, count)

    def service():
        """Skip the complete existing39-byte service owner plus its at-most-two actual part references."""
        read(31); packets(1); packets(1)

    read(8); count = read(2); read(32); assert count <= (end - position) // 62
    assemblies = []
    # Every static row retains its old construction prefix, role-bounded facility/stock and append-only manufacturing payload before exact work receipts.
    for _ in range(count):
        read(2); read(8); read(1); kind = read(1); read(17)
        for _ in range(2):
            read(2); packets()
        present = read(1); assert present <= 1
        if present:
            read(4); sparse = read(1); assert sparse <= 64
            for _ in range(sparse):
                read(1); packets()
            read(17); service(); read(4); read(10); service()
            issued = position; read(8); has_job = read(1); assert has_job <= 1
            if has_job:
                fields = {"issued": issued}
                for name, width in (("phase", 1), ("model", 2), ("serial", 8), ("source", 10), ("capital", 4), ("berth", 1), ("interval", 8)):
                    fields[name] = position; read(width)
                fields["inputs"] = [packets() for _ in range(4)]
                if kind == 9:
                    assemblies.append(fields)
        read(20)
    assert position == end and len(assemblies) == 1
    return assemblies[0]


def damage(context, source, kind, model):
    """Change one semantic boundary in an owned copy, preserving the entire original checkpoint.

    @param context Exact malformed-copy artifact owner.
    @param source Genuine accepted reserved checkpoint.
    @param kind One independent phase, identity, recipe, material or epoch fault.
    @param model Genuine immutable recipe, used to select its final meaningful input.
    @return New uncompressed malformed native save with unchanged chunk framing.
    """
    version, raw = read_stream(source); assert version == 426
    fields = layout(raw); changed = bytearray(raw)
    if kind == "old-version":
        version = 425
    elif kind == "serial":
        changed[fields["serial"]:fields["serial"] + 8] = bytes(8)
    elif kind == "source-generation":
        changed[fields["source"] + 2:fields["source"] + 10] = bytes([255]) * 8
    elif kind == "missing-source":
        changed[fields["source"]:fields["source"] + 10] = struct.pack(">HQ", 65535, 0)
    elif kind == "role":
        changed[fields["model"]:fields["model"] + 2] = struct.pack(">H", 14)
    elif kind == "capital":
        changed[fields["capital"]:fields["capital"] + 4] = bytes(4)
    elif kind == "berth":
        changed[fields["berth"]] = 1
    elif kind == "interval":
        changed[fields["interval"]:fields["interval"] + 8] = bytes([255]) * 8
    elif kind == "phase":
        changed[fields["phase"]] = 0
    elif kind == "material":
        index = 3 if model in (16, 17) else 1
        offset = fields["inputs"][index][0]; changed[offset:offset + 2] = bytes(2)
    elif kind == "packet-alias":
        first, second = fields["inputs"][:2]; assert first[2] > 0 and second[2] > 0
        changed[second[1]:second[1] + 4] = raw[first[1]:first[1] + 4]
    else:
        assert kind == "unused-input" and model in (15, 19, 20)
        offset, _, count = fields["inputs"][3]; assert count == 0
        changed[offset:offset + 2] = struct.pack(">H", 1)
    target = context.saves / ("manufacture-malformed-" + kind + ".sav")
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return target


def verify(configuration, proof_path, kind):
    """Require independent full-load and copied-preview refusal with unchanged accepted current-world owners.

    @param configuration Completed native build matching the proof.
    @param proof_path Exact accepted reserved/restore report, never a guessed latest artifact.
    @param kind One independent bounded fault.
    @return Accepted refusal report binding original, malformed copy and both native process reports.
    """
    proof = json.loads(proof_path.read_text(encoding="utf-8"))
    assert proof["configuration"] == configuration and proof["phase"] in ("reserved", "restore") and proof["save_version"] == 426
    model = proof["model"]; source = ROOT / proof["after"]["path"]; original = identity(source); assert original == proof["after"]
    context = current_run_context("space", f"manufacturing-reservation-reject-{kind}-{model}")
    malformed = damage(context, source, kind, model)
    full = context.for_feature("space", f"manufacturing-reservation-full-{kind}-{model}")
    native(full, configuration, "reserved", model, malformed, reject=True)
    preview = context.for_feature("space", f"manufacturing-reservation-preview-{kind}-{model}")
    native(preview, configuration, "reserved", model, source, corrupt=malformed)
    assert identity(source) == original
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group7; independent full/copied-preview manufacturing identity, recipe, packet and epoch refusals.",
                  configuration=configuration, model=model, kind=kind, proof=identity(proof_path), source=original, malformed=identity(malformed),
                  native_reports=[identity(path) for owner in (full, preview) for path in sorted(owner.reports.glob("native-*.json"))])
    report = context.reports / "manufacturing-reservation-rejection.json"
    report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return report


def main():
    """Select one exact accepted proof and bounded malformed boundary."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), default="RelWithDebInfo")
    parser.add_argument("--proof", type=Path, required=True)
    parser.add_argument("--kind", choices=KINDS, required=True)
    args = parser.parse_args()
    print(verify(args.configuration, args.proof.resolve(), args.kind).relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
