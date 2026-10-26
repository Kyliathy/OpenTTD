"""Run one bounded malformed CAPA preview/full-load pair using original native owners.

@file asteroid_cargo_wire_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group21.
Only named CAPA bytes in invocation-owned test-cache copies change. Original
versions, unrelated chunks and immutable retained input saves stay intact.
"""
import argparse
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from savegame_tables import Reader, WIDTHS, chunks, read_stream, table_header

CASES = ("missing-body", "missing-mineral", "dual", "sentinel", "empty-mineral",
         "wrong-class", "future-body", "ordinary-payload", "old-payload", "truncated")


def gamma(value):
    """Encode the original native bounded simple-gamma framing.

    @param value Original/changed row extent, below2**32.
    @return Portable native length bytes; no gameplay value is inferred.
    """
    assert 0 <= value < 2**32
    # Each prefix announces one more framing byte, exactly as the original
    # reader does. Complete extents preserve all unrelated owner boundaries.
    for width, bits, prefix in ((1, 7, 0), (2, 14, 0x80), (3, 21, 0xC0), (4, 28, 0xE0)):
        if value < 1 << bits:
            result = bytearray(value.to_bytes(width, "big"))
            result[0] |= prefix
            return bytes(result)
    return b"\xf0" + value.to_bytes(4, "big")


def packet_fields(row, fields, sparse):
    """Locate actual flat CAPA scalars through their original named schema.

    @param row Complete original native packet row.
    @param fields Original table header descriptors.
    @param sparse Whether the row begins with its native pool index.
    @return Field-name to original byte-offset/width/value mapping.
    """
    reader = Reader(row)
    if sparse:
        reader.gamma()
    result = {}
    # CAPA has only original scalar fields. Refuse unexpected arrays/structs
    # rather than guessing offsets into another future packet representation.
    for name, kind, nested in fields:
        assert not nested and not kind & 16
        width = WIDTHS[kind & 15]
        offset = reader.pos
        result[name] = (offset, width, int.from_bytes(reader.take(width), "big"))
    assert reader.pos == len(row)
    return result


def malformed(source, destination, case):
    """Create one narrowly malformed original-version packet family.

    @param source Immutable actual native save,441 or genuinely older forold-payload.
    @param destination New invocation-owned repository test-cache file.
    @param case One explicit source-schema/origin failure.
    @return Original version, unchanged by this fixture mutation.
    """
    version, raw = read_stream(source)
    assert case in CASES and (version < 441 if case == "old-payload" else version >= 441)
    owners = chunks(raw)
    capa, = [owner for owner in owners if owner["name"] == b"CAPA"]
    assert capa["kind"] in (3, 4)
    fields = table_header(Reader(capa["rows"][0]))
    rows = list(capa["rows"])
    if case == "old-payload":
        assert not any(name.startswith("asteroid_") for name, _, _ in fields)
        fields.extend([["asteroid_body", 6, None], ["asteroid_mineral", 6, None]])
        # Deliberately forge newer payload keys in an actual older stream; its
        # genuine header/version and every unrelated family remain original.
        rows[1:] = [row + bytes(8) if row else row for row in rows[1:]]
    elif case.startswith("missing-"):
        wanted = "asteroid_body" if case == "missing-body" else "asteroid_mineral"
        fields = [["x" + name[1:] if name == wanted else name, kind, nested] for name, kind, nested in fields]
    else:
        selected = None
        # Select actual natural freight through its saved type. Pool holes and
        # ordinary cargo cannot lend an invented packet identity to this test.
        for index, row in enumerate(rows[1:], 1):
            if not row:
                continue
            offsets = packet_fields(row, fields, capa["kind"] == 4)
            if offsets["source_type"][2] == 3:
                selected = index, offsets
                break
        assert selected is not None, "select a source with real retained asteroid cargo"
        index, offsets = selected
        changed = bytearray(rows[index])

        def put(name, value):
            """Change exactly one admitted original scalar.

            @param name Exact CAPA schema field. @param value Deliberately malformed bounded unsigned value.
            """
            offset, width, _ = offsets[name]
            changed[offset:offset + width] = value.to_bytes(width, "big")

        if case == "dual":
            put("source_id", 0)
        elif case == "sentinel":
            put("asteroid_body", 2**32 - 1)
        elif case == "empty-mineral":
            put("asteroid_mineral", 0)
        elif case == "wrong-class":
            put("asteroid_mineral", int.from_bytes(b"WICE" if offsets["asteroid_mineral"][2] == int.from_bytes(b"NORE", "big") else b"NORE", "big"))
        elif case == "future-body":
            put("asteroid_body", 2**32 - 2)
        elif case == "ordinary-payload":
            put("source_type", 0)
            put("source_id", 7)
        elif case == "truncated":
            del changed[-4:]
        rows[index] = bytes(changed)
    assert all(nested is None for _, _, nested in fields)
    rows[0] = b"".join(bytes([kind]) + gamma(len(name.encode())) + name.encode() for name, kind, _ in fields) + b"\0"
    replacement = b"CAPA" + bytes([capa["kind"]]) + b"".join(gamma(len(row) + 1) + row for row in rows) + b"\0"
    # Rebuild only the chosen declared chunk, retaining every other original
    # byte and the genuine file version. Output cannot overwrite any input.
    assert not destination.exists()
    destination.write_bytes(b"OTTN" + (version << 16).to_bytes(4, "big") + b"".join(replacement if owner is capa else owner["raw"] for owner in owners) + bytes(4))
    return version


def main():
    """Run one explicit native refusal pair under the existing110second per-process harness.

    @return First native failure, or zero when both real loading modes refuse.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, help="Valid441 Space fixes checkpoint used as the open world")
    parser.add_argument("--old-source", help="Genuine older immutable source forold-payload")
    parser.add_argument("--case", choices=CASES, required=True)
    parser.add_argument("--run", required=True)
    args = parser.parse_args()
    source = (ROOT / args.source).resolve()
    original = (ROOT / args.old_source).resolve() if args.case == "old-payload" else source
    target_dir = ROOT / "test-cache/space-fixes/cargo-wire-cases"
    target_dir.mkdir(parents=True, exist_ok=True)
    target = (target_dir / (args.run + ".sav")).resolve()
    assert target.parent == target_dir.resolve() and original.is_relative_to(ROOT)
    malformed(original, target, args.case)
    # Each invocation is a separate bounded original application, so a failed
    # destructive load cannot erase the accepted world used by another check.
    for mode in ("preview", "full"):
        command = [sys.executable, str(Path(__file__).with_name("space_fixes_checks.py")),
                   "--source", source.relative_to(ROOT).as_posix(), "--cargo-origin", "--cargo-bad", target.relative_to(ROOT).as_posix(),
                   "--run", args.run + "-" + mode]
        if mode == "full":
            command.append("--cargo-full")
        result = subprocess.run(command, cwd=ROOT, check=False)
        if result.returncode:
            return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
