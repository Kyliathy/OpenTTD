"""Exercise explicit SPOR policy/VEHS intent in bounded actual native applications.

@file space_mining_wire_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group23.
Declared transport fixtures retain real original hulls, work, leases and cargo;
setting copied order intent is not evidence of a player-issued gameplay command.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from savegame_tables import Reader, WIDTHS, chunks, read_stream, table_header
from asteroid_cargo_wire_checks import gamma
from asteroid_field_wire_checks import riff

CASES = ("pinned", "missing-mineral", "old-payload", "phase", "presence", "departure",
         "retry", "route", "travel", "wrong-mineral", "future-previous", "policy", "orphan-lease", "wrong-effective")


def native_missions(path):
    """Read original native mission, stop and contained-intent scalars without loading gameplay.

    @param path Actual genuine source or native resave. @return Its version and original Space IDs/scalars.
    """
    version, stream = read_stream(path)
    owner = next(chunk for chunk in chunks(stream) if chunk["name"] == b"VEHS")
    fields = table_header(Reader(owner["rows"][0]))
    result = {}
    # Decode full row extents before selecting mission scalars; ordinary
    # vehicle variants and packet lists retain their own original framing.
    for index, row in enumerate(owner["rows"][1:]):
        if not row:
            continue
        reader = Reader(row)
        identity = reader.gamma() if owner["kind"] == 4 else index
        offsets = locations(reader, fields)
        assert reader.pos == len(row)
        if "space.mission.phase" not in offsets:
            continue
        result[identity] = {key: value for key, (_, _, value) in offsets.items()
                            if key.startswith(("space.mission.", "space.position.", "space.home."))
                            or key in ("space.extracted_units", "space.common.vehstatus")}
    return version, result


def order_rows(path):
    """Read exact original SPOR queue fields and policy by genuine source era.

    @param path Actual native source/resave. @return Issuance header and canonical per-ID original fields, or old absence.
    """
    version, stream = read_stream(path)
    owner = next((chunk for chunk in chunks(stream) if chunk["name"] == b"SPOR"), None)
    if owner is None:
        assert version < 410
        return None
    payload = owner["payload"]; position = 9; result = {}
    # Preserve raw queue/cursor/work/generation and approach bytes. Only each
    # version443 policy scalar is separated from the original14-byte order.
    for _ in range(payload[8]):
        prefix = payload[position:position + 22]; position += 22
        orders = []
        for _ in range(prefix[21]):
            order = payload[position:position + 14]; position += 14
            policy = payload[position] if version >= 443 else 0
            position += int(version >= 443); orders.append((order, policy))
        approach = b"\0"
        if version >= 420:
            extent = 1 + (30 if payload[position] else 0)
            approach = payload[position:position + extent]; position += extent
        result[int.from_bytes(prefix[:4], "big")] = (prefix, tuple(orders), approach)
    assert position == len(payload)
    return payload[:9], result


def compare(source, target, policy_edit=False):
    """Verify genuine migration/resave preserves every original mission/stop discriminator.

    @param source Genuine old file or accepted current checkpoint.
    @param target Actual current native resave. @param policy_edit Whether exactly one real ReturnNearest edit is expected. @return Bounded exact scalar evidence.
    """
    before_version, before = native_missions(source)
    after_version, after = native_missions(target)
    assert after_version >= 443 and before.keys() == after.keys()
    inert = {"phase": 0, "mineral": 0, "previous_body": 2**32 - 1,
             "effective_present": 0, "effective_kind": 0, "effective_id": 2**32 - 1, "effective_generation": 0,
             "departure_present": 0, "departure_x": 0, "departure_y": 0, "retry_ticks": 0,
             "waiting": 0, "route_cursor": 255, "travel_ticks": 0}
    # Only absent old continuation columns may be initialized. Original pose,
    # mission, partial work, stop flag and extraction history stay byte-exact.
    for identity, original in before.items():
        expected = dict(original)
        if before_version < 443:
            expected.update({"space.mission.continuation." + key: value for key, value in inert.items()})
        assert expected == after[identity], (identity, expected, after[identity])
    old_orders, new_orders = order_rows(source), order_rows(target)
    assert new_orders is not None
    changed = 0
    if old_orders is None:
        assert not policy_edit and new_orders[1].keys() == before.keys()
        for identity, (prefix, orders, approach) in new_orders[1].items():
            assert int.from_bytes(prefix[4:12], "big") == identity + 1
            assert prefix[12:] == bytes(10) and not orders and approach == b"\0"
    else:
        assert old_orders[0] == new_orders[0] and old_orders[1].keys() == new_orders[1].keys()
        # A native policy edit may change one existing Mine's one byte only;
        # every original target, cursor, generation, wait and approach persists.
        for identity, (prefix, orders, approach) in old_orders[1].items():
            new_prefix, new_entries, new_approach = new_orders[1][identity]
            assert prefix == new_prefix and approach == new_approach and len(orders) == len(new_entries)
            for (old, policy), (new, next_policy) in zip(orders, new_entries):
                assert old == new
                if policy != next_policy:
                    assert policy_edit and old[0] == 2 and next_policy == 1
                    changed += 1
        assert changed == int(policy_edit)
    return {"source_version": before_version, "saved_version": after_version,
            "space_hulls": len(before), "mission_and_stop_preserved": True, "original_queues_preserved": True,
            "expected_policy_edits": changed, "legacy_inert_defaults": before_version < 443}


def header_bytes(fields):
    """Encode only the original explicit table descriptors, recursively in native order.

    @param fields Actual named original descriptors. @return Complete native header bytes.
    """
    own = b"".join(bytes([kind]) + gamma(len(name.encode())) + name.encode() for name, kind, _ in fields) + b"\0"
    # Native nested headers follow the complete parent descriptor list; inline
    # nesting would shift the actual common vehicle and cargo owner schemas.
    return own + b"".join(header_bytes(nested) for _, _, nested in fields if nested is not None)


def locations(reader, fields, prefix=""):
    """Locate exact scalar extents through the original nested VEHS schema.

    @param reader Complete bounded native row. @param fields Its actual descriptors.
    @param prefix Original parent field path. @return Named scalar offset/width/value map.
    """
    result = {}
    # Native variants and compartment lists retain their real lengths. Parsing
    # every original field avoids guessing offsets across another cargo owner.
    for name, kind, nested in fields:
        key = prefix + name
        primitive = kind & 15
        if primitive == 11:
            for _ in range(reader.gamma()):
                result.update(locations(reader, nested, key + "."))
        elif primitive == 10:
            reader.take(reader.gamma())
        else:
            count = reader.gamma() if kind & 16 else 1
            width = WIDTHS[primitive]
            offset = reader.pos
            raw = reader.take(count * width)
            if not kind & 16:
                result[key] = (offset, width, int.from_bytes(raw, "big"))
    return result


def set_scalar(row, fields, key, value):
    """Change exactly one located native scalar without changing its width.

    @param row Invocation-owned mutable row. @param fields Actual located fields.
    @param key Exact original scalar path. @param value Explicit fixture value.
    """
    offset, width, _ = fields[key]
    row[offset:offset + width] = value.to_bytes(width, "big")


def configure_spor(payload, vehicle, body, invalid=False):
    """Set declared current policy on an actual empty or already pinned single-Mine row.

    @param payload Actual current SPOR. @param vehicle Original paid native ID.
    @param body Original actual active body. @param invalid Whether policy is deliberately unknown.
    @return Complete new payload preserving all other original queues/generations.
    """
    position = 9
    rows = []
    # Every queue retains its exact original variable extent and optional
    # approach. Only the selected empty original queue gains declared intent.
    for _ in range(payload[8]):
        start = position
        count = payload[start + 21]
        position += 22 + 15 * count
        position += 1 + (30 if payload[position] else 0)
        row = payload[start:position]
        if int.from_bytes(row[:4], "big") == vehicle:
            if count == 0:
                prefix = bytearray(row[:22]); prefix[12:22] = bytes(10)
                prefix[13] = 1; prefix[21] = 1
                order = bytes([2, 0]) + body.to_bytes(4, "big") + bytes(8) + bytes([255 if invalid else 2])
                row = bytes(prefix) + order + row[22:]
            else:
                # A previously admitted pinned fixture already owns this exact
                # single Mine. Preserve its original queue/work/approach bytes.
                assert count == 1 and row[12] == 0 and row[22:24] == bytes([2, 0]) and int.from_bytes(row[24:28], "big") == body
                changed = bytearray(row); changed[36] = 255 if invalid else 2; row = bytes(changed)
        rows.append(row)
    assert position == len(payload)
    return payload[:9] + b"".join(rows)


def fixture(source, target, case):
    """Create one complete declared policy checkpoint or narrowly malformed copy.

    @param source Genuine442 file for old-payload, current443 mining file otherwise.
    @param target New invocation-owned repository file. @param case Explicit transport case.
    """
    version, stream = read_stream(source)
    assert (version == 442 if case == "old-payload" else version >= 443) and not target.exists()
    owners = chunks(stream)
    vehs = next(owner for owner in owners if owner["name"] == b"VEHS")
    fields = table_header(Reader(vehs["rows"][0]))
    space = next(nested for name, _, nested in fields if name == "space")
    rows = list(vehs["rows"])
    selected = None
    if case == "old-payload":
        assert not any(name.startswith("mission.continuation.") for name, _, _ in space)
        space.append(["mission.continuation.phase", 2, None])
    # Select actual active mining bytes, not a fabricated hull or pool ordinal.
    for index, original in enumerate(rows[1:], 1):
        if not original:
            continue
        reader = Reader(original)
        identity = reader.gamma() if vehs["kind"] == 4 else index - 1
        offsets = locations(reader, table_header(Reader(vehs["rows"][0])))
        assert reader.pos == len(original)
        if "space.mission.phase" not in offsets:
            continue
        if case == "old-payload":
            # Space is the final original variant. Its added future scalar is
            # deliberately skipped by generic old tables, but must still refuse.
            rows[index] = original + b"\0"
        elif selected is None and offsets["space.mission.phase"][2] == 2:
            selected = index, identity, offsets
    replacements = {}
    if case != "old-payload":
        assert selected is not None, "choose actual working mining checkpoint with empty original queues"
        index, vehicle, offsets = selected
        row = bytearray(rows[index]); body = offsets["space.mission.target"][2]
        asteroid = next(owner["payload"] for owner in owners if owner["name"] == b"ASTR")
        body_rows = [asteroid[start:start + 40] for start in range(51, 51 + 40 * int.from_bytes(asteroid[27:31], "big"), 40)]
        resource = next(record[4] for record in body_rows if int.from_bytes(record[:4], "big") == body)
        mineral = int.from_bytes((b"NORE", b"WICE", b"RARE")[resource], "big")
        set_scalar(row, offsets, "space.mission.continuation.mineral", mineral)
        set_scalar(row, offsets, "space.mission.continuation.previous_body", body)
        changed = {"phase": ("phase", 255), "presence": ("effective_present", 2), "departure": ("departure_x", 1),
                   "retry": ("retry_ticks", 75), "route": ("route_cursor", 33), "travel": ("travel_ticks", 2**24 + 1),
                   "wrong-mineral": ("mineral", int.from_bytes(b"WICE" if resource != 1 else b"NORE", "big")),
                   "future-previous": ("previous_body", 2**32 - 2)}
        if case in changed:
            key, value = changed[case]; set_scalar(row, offsets, "space.mission.continuation." + key, value)
        elif case == "orphan-lease":
            set_scalar(row, offsets, "space.mission.phase", 4)
        elif case == "wrong-effective":
            for key, value in (("phase", 2), ("route_cursor", 1), ("effective_present", 1), ("effective_id", 2**32 - 2)):
                set_scalar(row, offsets, "space.mission.continuation." + key, value)
        elif case == "missing-mineral":
            for field in space:
                if field[0] == "mission.continuation.mineral":
                    field[0] = "x" + field[0][1:]
        rows[index] = bytes(row)
        spor = next(owner["payload"] for owner in owners if owner["name"] == b"SPOR")
        replacements[b"SPOR"] = riff(b"SPOR", configure_spor(spor, vehicle, body, case == "policy"))
    rows[0] = header_bytes(fields)
    replacements[b"VEHS"] = b"VEHS" + bytes([vehs["kind"]]) + b"".join(gamma(len(row) + 1) + row for row in rows) + b"\0"
    target.write_bytes(b"OTTN" + (version << 16).to_bytes(4, "big") +
                       b"".join(replacements.get(owner["name"], owner["raw"]) for owner in owners) + bytes(4))


def main():
    """Run one focused native checkpoint or paired refusal, using separate disposable processes.

    @return First native failure, or zero after the selected actual loading checks.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True)
    parser.add_argument("--case", choices=CASES)
    parser.add_argument("--run")
    parser.add_argument("--target", help="Read-only original mission/stop/intent comparison with a native resave")
    parser.add_argument("--policy-edit", action="store_true", help="Read-only comparison expects exactly one ReturnNearest policy byte to change")
    args = parser.parse_args()
    source = (ROOT / args.source).resolve()
    if args.target:
        print(json.dumps(compare(source, (ROOT / args.target).resolve(), args.policy_edit)))
        return 0
    assert args.case and args.run
    directory = ROOT / "test-cache/space-fixes/mining-wire-cases"
    directory.mkdir(parents=True, exist_ok=True)
    target = (directory / (args.run + ".sav")).resolve()
    assert source.is_relative_to(ROOT) and target.parent == directory.resolve()
    fixture(source, target, args.case)
    # Full refusals tear down their own isolated application; preview checks
    # valid-corrupt-valid against the separate unchanged accepted world.
    for mode in (("accepted",) if args.case == "pinned" else ("preview", "full")):
        command = [sys.executable, str(Path(__file__).with_name("space_fixes_checks.py")),
                   "--source", (target if mode == "accepted" else source).relative_to(ROOT).as_posix(), "--cargo-origin", "--run", args.run + "-" + mode]
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
