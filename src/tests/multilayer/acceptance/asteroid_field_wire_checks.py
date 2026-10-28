"""Check native ASTR/SPKN framing, migration and declared sparse field fixtures.

@file asteroid_field_wire_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group22.
Only named families in new test-cache copies change. Synthetic aggregate exit
history tests transport admission; it is never evidence of executed departures.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from savegame_tables import chunks, read_stream, saved_owner_table

CASES = ("truncated", "oversized", "count", "duplicate", "sentinel", "future",
         "cursor", "revision", "credit", "counter", "timer", "optional",
         "knowledge-missing", "knowledge-duplicate", "knowledge-mask", "sparse", "empty", "retired-origin")


def put(payload, offset, value, width=4):
    """Change one bounded explicitly located transport scalar.

    @param payload Invocation-owned mutable bytes. @param offset Exact field offset.
    @param value Deliberate fixture value. @param width Original scalar byte width.
    """
    assert offset + width <= len(payload)
    payload[offset:offset + width] = value.to_bytes(width, "big")


def riff(name, payload):
    """Frame the original RIFF owner with its checked complete payload.

    @param name Four-byte original family. @param payload Complete fixture bytes.
    @return Portable native framing without altering any unrelated family.
    """
    assert len(name) == 4 and len(payload) < 2**28
    length = len(payload)
    return name + bytes([(length >> 24) << 4]) + (length & 0xFFFFFF).to_bytes(3, "big") + payload


def field_parts(payload, version):
    """Separate exact original body, preview and versioned field trailers.

    @param payload Complete actual ASTR bytes. @param version Genuine native version.
    @return Header, original rows, original previews, starter metadata and lifecycle.
    """
    count = int.from_bytes(payload[27:31], "big")
    surveys = int.from_bytes(payload[31:35], "big")
    width = 40 if version >= 405 else 36
    end = 51 + count * width
    survey_end = end + surveys * 34
    starter_end = survey_end + (26 if version >= 424 else 0)
    assert len(payload) == starter_end + (62 if version >= 442 else 0)
    return payload[:51], payload[51:end], payload[end:survey_end], payload[survey_end:starter_end], payload[starter_end:]


def compare(source, target):
    """Require native migration to preserve original field records and all renewal words.

    @param source Genuine old native file or declared current sparse fixture.
    @param target Actual current native resave. @return Exact migration evidence.
    """
    old_version, before = read_stream(source)
    version, after = read_stream(target)
    assert version >= 442
    old = next(owner["payload"] for owner in chunks(before) if owner["name"] == b"ASTR")
    new = next(owner["payload"] for owner in chunks(after) if owner["name"] == b"ASTR")
    old_parts, new_parts = field_parts(old, old_version), field_parts(new, version)
    expected_bodies = old_parts[1]
    if old_version < 405:
        # Genuine preview-era bodies predate native mining leases. Preserve
        # every original byte and add only the original invalid VehicleID.
        expected_bodies = b"".join(expected_bodies[index:index + 36] + (2**20 - 1).to_bytes(4, "big")
                                   for index in range(0, len(expected_bodies), 36))
    assert old_parts[0] == new_parts[0] and expected_bodies == new_parts[1] and old_parts[2] == new_parts[2], "original body/motion/lease/preview bytes changed"
    if old_version >= 424:
        assert old_parts[3] == new_parts[3], "original starter history changed"
    trailer = new_parts[4]
    if old_version >= 442:
        assert old_parts[4] == trailer, "renewal/cursor/RNG checkpoint changed"
    else:
        assert trailer[:2] == b"\0\1"
        assert int.from_bytes(trailer[2:6], "big") == int.from_bytes(old[27:31], "big")
        assert trailer[6] == 1 and trailer[7:46] == bytes(39)
        assert trailer[46:54] != bytes(8) and trailer[54:62] != bytes(8)
    return {"source_version": old_version, "saved_version": version,
            "bodies": int.from_bytes(new[27:31], "big"), "original_records_preserved": True,
            "complete_lifecycle_preserved": old_version >= 442}


def fixture(source, target, case):
    """Create one precisely declared current field/disclosure transport case.

    @param source Valid current native file. @param target New repository test-cache copy.
    @param case One malformed family or aggregate-history sparse acceptance case.
    """
    version, stream = read_stream(source)
    assert version >= 442 and not target.exists()
    owners = chunks(stream)
    original = next(owner["payload"] for owner in owners if owner["name"] == b"ASTR")
    payload = bytearray(original)
    footer = len(payload) - 62
    count = int.from_bytes(payload[27:31], "big")
    next_id = int.from_bytes(payload[footer + 2:footer + 6], "big")
    knowledge = None
    if case == "truncated":
        payload.pop()
    elif case == "oversized":
        payload = bytearray(139 + 40 * 14336 + 34 * 64 + 1)
    elif case == "count":
        put(payload, 27, 2**32 - 1)
    elif case in ("duplicate", "sentinel", "future"):
        put(payload, 91 if case == "duplicate" else 51,
            0 if case == "duplicate" else 2**32 - 1 if case == "sentinel" else next_id)
    elif case in ("cursor", "revision", "credit", "counter", "timer", "optional"):
        offset, value, width = {"cursor": (2, 0, 4), "revision": (0, 2, 2),
                                "credit": (31, 1, 8), "counter": (7, 1, 8),
                                "timer": (42, 2223, 4), "optional": (39, 2, 1)}[case]
        put(payload, footer + offset, value, width)
    elif case.startswith("knowledge-"):
        identity = next_id if case == "knowledge-missing" else 0
        mask = 2**63 if case == "knowledge-mask" else 1
        row = identity.to_bytes(4, "big") + mask.to_bytes(8, "big")
        knowledge = (2 if case == "knowledge-duplicate" else 1).to_bytes(2, "big") + row * (2 if case == "knowledge-duplicate" else 1)
    elif case == "retired-origin":
        assert int.from_bytes(payload[31:35], "big") == 0
        selected = next(index for index in range(count) if int.from_bytes(payload[51 + 40 * index:55 + 40 * index], "big") == 0)
        start = 51 + 40 * selected
        body = payload[start:start + 40]
        assert int.from_bytes(body[32:36], "big") == 2**32 - 1 and int.from_bytes(body[36:40], "big") == 2**20 - 1
        packets = saved_owner_table(source, b"CAPA")
        assert any(row["source_type"] == 3 and row["asteroid_body"] == 0 for row in packets.values())
        # Declare one component retirement while preserving every original
        # native packet/hull/work record. This tests typed history admission;
        # actual pre-exit participant release remains the later runtime gate.
        del payload[start:start + 40]; footer -= 40
        put(payload, 27, count - 1)
        put(payload, 18, max(1, int.from_bytes(payload[18:26], "big")), 8)
        size, mineral = body[5], body[4]
        old_count = int.from_bytes(payload[footer + 7 + size * 8:footer + 15 + size * 8], "big")
        credit = int.from_bytes(payload[footer + 31:footer + 39], "big")
        put(payload, footer + 7 + size * 8, old_count + 1, 8)
        put(payload, footer + 31, credit + (1, 3, 5)[size], 8)
        put(payload, footer + 39, 1, 1); put(payload, footer + 40, size, 1); put(payload, footer + 41, mineral, 1)
        if credit == 0:
            put(payload, footer + 42, 741)
        old_knowledge = next(owner["payload"] for owner in owners if owner["name"] == b"SPKN")
        rows = [old_knowledge[index:index + 12] for index in range(2, len(old_knowledge), 12) if int.from_bytes(old_knowledge[index:index + 4], "big") != 0]
        knowledge = len(rows).to_bytes(2, "big") + b"".join(rows)
    elif case == "empty":
        assert next_id == count and int.from_bytes(payload[31:35], "big") == 0
        # This declared component fixture requires a source with no live mining
        # lease, actionable body order or natural packet custody. The native
        # joins must still refuse if the chosen source contains any such owner.
        sizes = [payload[51 + 40 * index + 5] for index in range(count)]
        last_class = payload[51 + 40 * (count - 1) + 4]
        del payload[51:51 + 40 * count]
        footer -= 40 * count
        put(payload, 27, 0)
        put(payload, 18, 1, 8)
        for size in range(3):
            put(payload, footer + 7 + size * 8, sizes.count(size), 8)
        put(payload, footer + 31, sum((1, 3, 5)[size] for size in sizes), 8)
        put(payload, footer + 39, 1, 1)
        put(payload, footer + 40, sizes[-1], 1)
        put(payload, footer + 41, last_class, 1)
        put(payload, footer + 42, 741)
        knowledge = bytes(2)
    elif case == "sparse":
        assert next_id == count and int.from_bytes(payload[31:35], "big") == 0
        records = [payload[index:index + 40] for index in range(51, 51 + 40 * count, 40)]
        retired = next(record for record in records if int.from_bytes(record[:4], "big") >= 3 and record[5] == 1
                       and int.from_bytes(record[32:36], "big") == 2**32 - 1 and int.from_bytes(record[36:40], "big") == 2**20 - 1)
        retired_id = int.from_bytes(retired[:4], "big")
        assert all(row["source_type"] != 3 or row["asteroid_body"] != retired_id for row in saved_owner_table(source, b"CAPA").values())
        # One unleased original Medium supplies the initial credit. Alternating
        # Small/Medium creation and exit leaves one known paid Small and two
        # credits. This declared aggregate history preserves every paid asset
        # and original packet, without claiming any executed departure/spawn.
        body = bytearray(next(record for record in records if record[4] == 0 and record[5] == 0))
        put(body, 0, 70000)
        put(body, 32, 2**32 - 1)
        put(body, 36, 2**20 - 1)  # Original VehicleID::Invalid(), distinct from the natural-body U32 sentinel.
        payload[51:51 + 40 * count] = b"".join(record for record in records if int.from_bytes(record[:4], "big") != retired_id) + body
        put(payload, 18, 1, 8)
        spawned = 70001 - count; assert spawned % 2 == 1
        put(payload, footer + 2, 70001)
        put(payload, footer + 7, (spawned - 1) // 2, 8)
        put(payload, footer + 15, (spawned + 1) // 2, 8)
        put(payload, footer + 31, 2, 8)
        put(payload, footer + 39, 1, 1)
        put(payload, footer + 40, 1, 1)
        put(payload, footer + 41, retired[4], 1)
        put(payload, footer + 42, 741)
        company = min(saved_owner_table(source, b"PLYR"))
        knowledge = b"\0\1" + (70000).to_bytes(4, "big") + (1 << company).to_bytes(8, "big")
    else:
        raise ValueError(case)
    replacements = {b"ASTR": riff(b"ASTR", payload)}
    if knowledge is not None:
        replacements[b"SPKN"] = riff(b"SPKN", knowledge)
    # All original families and the genuine native version remain unchanged;
    # only the explicitly declared component owners are replaced in this copy.
    target.write_bytes(b"OTTN" + (version << 16).to_bytes(4, "big") +
                       b"".join(replacements.get(owner["name"], owner["raw"]) for owner in owners) + bytes(4))


def main():
    """Run one bounded native pair or read-only migration comparison.

    @return First native failure, or zero when all selected checks pass.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True)
    parser.add_argument("--target", help="Read-only comparison target instead of creating a fixture")
    parser.add_argument("--case", choices=CASES)
    parser.add_argument("--run")
    args = parser.parse_args()
    source = (ROOT / args.source).resolve()
    assert source.is_relative_to(ROOT)
    if args.target:
        print(json.dumps(compare(source, (ROOT / args.target).resolve())))
        return 0
    assert args.run and args.case
    directory = ROOT / "test-cache/space-fixes/field-wire-cases"
    directory.mkdir(parents=True, exist_ok=True)
    target = (directory / (args.run + ".sav")).resolve()
    assert target.parent == directory.resolve()
    fixture(source, target, args.case)
    # Separate disposable applications isolate deliberate full-load teardown.
    # Preview runs valid-corrupt-valid and verifies original owners/RNG/clock.
    for mode in (("accepted",) if args.case in ("sparse", "empty", "retired-origin") else ("preview", "full")):
        command = [sys.executable, str(Path(__file__).with_name("space_fixes_checks.py")),
                   "--source", (target if mode == "accepted" else source).relative_to(ROOT).as_posix(),
                   "--cargo-origin", "--run", args.run + "-" + mode]
        if mode != "accepted":
            command.extend(["--cargo-bad", target.relative_to(ROOT).as_posix()])
        if mode == "full":
            command.append("--cargo-full")
        result = subprocess.run(command, cwd=ROOT, check=False)
        if result.returncode:
            return result.returncode
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
