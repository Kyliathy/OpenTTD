"""Reject independently malformed heavy native mission fields in both actual readers.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9C
All mutations target invocation-owned copies through decoded native framing and exact hull identity.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, Reader, CHUNK_RIFF, CHUNK_RECORDS, read_stream, identity
from runner import current_run_context
from launch_center_checks import riff_location

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_mining_runtime_checks-1.py")))
ACTIONS = runpy.run_path(str(Path(__file__).with_name("probe_role_checks-3.py")))["action_rows"]
KINDS = ("future-runtime", "future-orders", "extraction-work", "unknown-phase", "missing-native-body", "changed-queue-body")


def future_orders(raw):
    """Add only a typed Visit to the original idle431 hull, isolating future order admission from mining history.

    @param raw Exact immutable commissioning-only431 native stream.
    @return Copy with the selected SPOR row reframed; every other family remains byte-preserved.
    """
    start, body, _, end = riff_location(raw, b"SPOR"); position = body + 9; count = raw[body + 8]
    # Exact native row framing selects Vehicle22/generation8; a queue/name/cargo byte pattern cannot impersonate that paid original.
    for _ in range(count):
        row = position; entries = raw[row + 21]; position += 22 + 14 * entries; present = raw[position]; assert present <= 1; position += 1 + 30 * present
        assert position <= end
        if raw[row:row + 12] != (22).to_bytes(4, "big") + (8).to_bytes(8, "big"):
            continue
        assert entries == 0; fields = native_fields(raw, 22)
        # Idle native fields eliminate a mining-progress rejection as the reason this candidate fails; only its newly introduced typed21 order is future capability.
        for field, expected in (("space.mission.phase", 4), ("space.mission.work_ticks", 0), ("space.extracted_units", 0)):
            offset, width = fields[field]; assert int.from_bytes(raw[offset:offset + width], "big") == expected
        prefix = bytearray(raw[row:row + 22]); prefix[12:21] = bytes(9); prefix[21] = 1
        replacement = prefix + bytes(14) + bytes(1)  # Visit(0), Body(0), actual body0, generation0; no borrowed factory approach.
        payload = raw[body:row] + replacement + raw[position:end]; assert len(payload) < 2**24
        return raw[:start] + b"SPOR" + len(payload).to_bytes(4, "big") + payload + raw[end:]
    raise AssertionError("Missing original idle commissioning hull")


def native_fields(raw, vehicle):
    """Locate actual native Space scalar offsets without matching arbitrary payload bytes.

    @param raw Decompressed original native stream.
    @param vehicle Exact paid original native VehicleID.
    @return Fully qualified native field paths to original absolute offsets and widths.
    """
    def header(reader, depth=0):
        """Decode one bounded original nested native schema.

        @param reader Original exact table-header extent.
        @param depth Nested description depth, capped against recursive malformed framing.
        @return Ordered type/name/child tuples.
        """
        assert depth < 16; fields = []
        # Native descriptions list scalar names first and child descriptions afterwards; preserving both orders avoids invented fixed offsets.
        while True:
            tag = reader.byte()
            if tag == 0:
                break
            size = reader.gamma(); assert size < 4096 and len(fields) < 1024
            fields.append([tag, reader.take(size).decode("utf-8"), None])
        for field in fields:
            if field[0] & 15 == 11:
                field[2] = header(reader, depth + 1)
        return fields

    def walk(reader, fields, absolute, prefix=""):
        """Read the selected original vehicle's actual nested value extents.

        @param reader Exact bounded original sparse row after its native VehicleID.
        @param fields Actual serialized field definitions.
        @param absolute Original row's stream offset.
        @param prefix Actual ancestor field path.
        @return Scalar field offset/width map; containers remain byte-preserved.
        """
        result = {}
        # Each actual length/count admits its complete value before the next field; optional inactive vehicle structs cannot shift a guessed Space offset.
        for tag, name, children in fields:
            path = prefix + name; primitive = tag & 15
            if primitive == 11:
                count = reader.gamma(); assert count <= 65536
                for _ in range(count):
                    result.update(walk(reader, children, absolute, path + "."))
            elif primitive == 10:
                reader.take(reader.gamma())
            else:
                width = {1: 1, 2: 1, 3: 2, 4: 2, 5: 4, 6: 4, 7: 8, 8: 8, 9: 2}[primitive]
                count = reader.gamma() if tag & 16 else 1
                result[path] = (absolute + reader.pos, width * count); reader.take(width * count)
        return result

    reader = Reader(raw)
    # The actual VEHS chunk framing and sparse native row ID select the owner; names inside packets or strings never participate in selection.
    while True:
        family = reader.take(4); assert family != bytes(4); mode = reader.byte(); kind = mode & 15
        if kind == CHUNK_RIFF:
            length = ((mode >> 4) << 24) | (reader.byte() << 16) | (reader.byte() << 8) | reader.byte(); reader.take(length)
            continue
        assert kind in CHUNK_RECORDS
        if family != b"VEHS":
            while True:
                extent = reader.gamma()
                if extent == 0:
                    break
                reader.take(extent - 1)
            continue
        assert kind == 4; extent = reader.gamma(); description = Reader(reader.take(extent - 1)); fields = header(description)
        assert description.pos == len(description.data)
        while True:
            extent = reader.gamma(); assert extent != 0; absolute = reader.pos; row = Reader(reader.take(extent - 1)); index = row.gamma()
            if index != vehicle:
                continue
            result = walk(row, fields, absolute); assert row.pos == len(row.data); return result


def damage(context, source, kind):
    """Change one original mission invariant in a fresh malformed native copy.

    @param context Explicit current malformed-artifact owner.
    @param source Exact accepted partial heavy mining checkpoint, or original idle431 for the isolated future-orders fault.
    @param kind Independent source-version, work, phase or target violation.
    @return New malformed input with original native framing preserved.
    """
    version, raw = read_stream(source)
    if kind == "future-orders":
        assert version == 431; changed = bytearray(future_orders(raw))
    else:
        assert version == 432; changed = bytearray(raw)
        fields = native_fields(raw, 22); work, width = fields["space.mission.work_ticks"]; assert width == 2 and int.from_bytes(raw[work:work + width], "big") == 37
    if kind == "future-orders":
        pass  # Its exact431 source and newly inserted Visit already isolate this admission boundary.
    elif kind == "future-runtime":
        version = 431
    elif kind == "extraction-work":
        changed[work:work + 2] = (74).to_bytes(2, "big")
    elif kind == "unknown-phase":
        position, width = fields["space.mission.phase"]; assert width == 1; changed[position] = 255
    elif kind == "missing-native-body":
        position, width = fields["space.mission.target"]; assert width == 4; changed[position:position + width] = (2**32 - 2).to_bytes(4, "big")
    else:
        assert kind == "changed-queue-body"; _, instruction, _ = ACTIONS(raw, dict(vehicle=22, generation=8))
        current = int.from_bytes(raw[instruction + 2:instruction + 6], "big"); replacement = 0 if current != 0 else 1
        changed[instruction + 2:instruction + 6] = replacement.to_bytes(4, "big")
    assert changed != raw or version != 432
    target = context.saves / ("malformed-heavy-mining-" + kind + ".sav"); target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed); return target


def verify(configuration, proof_path, kind):
    """Reject the same malformed copy independently in native startup and copied preview.

    @param configuration Matching completed current build.
    @param proof_path Exact original accepted partial mining report.
    @param kind Independent malformed mission invariant.
    @return Immutable paired-refusal proof path.
    """
    proof = SHARED["read"](proof_path); assert proof["configuration"] == configuration and proof["phase"] == "working"
    source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    malformed_source = source; predecessor = identity(proof_path)
    if kind == "future-orders":
        predecessor, selected = SHARED["entry"](configuration); malformed_source = ROOT / selected["path"]; assert identity(malformed_source) == selected
    context = current_run_context("space", "heavy-mining-runtime-refusal-" + kind); malformed = damage(context, malformed_source, kind)
    full = context.for_feature("space", "heavy-mining-runtime-full-" + kind); preview = context.for_feature("space", "heavy-mining-runtime-preview-" + kind)
    SHARED["native"](full, configuration, "corrupt", malformed, reject=True)
    assertions = SHARED["native"](preview, configuration, "corrupt", source, corrupt=malformed); assert identity(source) == proof["after"]
    target = context.reports / "heavy-mining-runtime-refusal.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9C; independently exercised full/copied heavy mission refusal with active-world owners retained.",
                                     configuration=configuration, kind=kind, proof=identity(proof_path), source=proof["after"], malformed_source=identity(malformed_source), predecessor=predecessor, malformed=identity(malformed), assertions=assertions,
                                     native_reports=[identity(path) for owner in (full, preview) for path in sorted(owner.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8"); return target


def main():
    """Run only the active heavy native mission malformed-admission burst."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--selector", type=Path, required=True); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); assert not args.output.exists(); selected = SHARED["read"](args.selector)
    assert selected["configuration"] == args.configuration and selected["complete"]
    working = [ROOT / path for path in selected["reports"] if SHARED["read"](ROOT / path)["phase"] == "working"]; assert len(working) == 1; reports = []
    # Each field fault receives its own independent actual full reader and valid-world copied reader; no prior refusal substitutes another boundary.
    for kind in KINDS:
        reports.append(verify(args.configuration, working[0], kind)); print(args.configuration, kind, flush=True)
    args.output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9C; explicit independently paired refusal membership.",
                                          configuration=args.configuration, scope="rejects", complete=True, reports=[path.relative_to(ROOT).as_posix() for path in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
