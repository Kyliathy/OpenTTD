"""Verify ordinary/resumed typed action ownership and independent malformed service refusals.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group8B
All continuations load their own actual ordinary resave. Malformed offsets follow bounded native family/row framing and exact original identities.
"""
import argparse
import json
import os
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, chunks, identity, ordinary, read_stream, Reader, CHUNK_RIFF, CHUNK_RECORDS
from launch_center_checks import riff_location
from runner import current_run_context

SHARED = runpy.run_path(str(Path(__file__).with_name("probe_role_checks-2.py")))
EXACT = runpy.run_path(str(Path(__file__).with_name("manufacturing_replacement_checks-1.py")))["EXACT"]
KINDS = ("old-version", "foreign-center", "wrong-action", "wrong-domain", "unstarted", "deadline", "work", "target-generation", "pending", "allowance")


def station_owner_offset(raw, station):
    """Locate native normal.base.owner through its real bounded table schema and exact Station index.

    @param raw Decompressed accepted native stream.
    @param station Exact original LaunchCenter Station index from the current instruction.
    @return Absolute one-byte native owner position; no search for coincidental cargo/name bytes is used.
    """
    def header(reader, depth=0):
        """Read one native nested table description without interpreting gameplay owners.

        @param reader Bounded original table-header extent.
        @param depth Current nested schema depth, bounded against accidental recursive framing.
        @return Ordered exact field type/name/subtable descriptors.
        """
        assert depth < 16; fields = []
        # Native headers list all field tags/names before their nested descriptions; retaining that order is essential to locate the actual base.owner field.
        while True:
            tag = reader.byte()
            if tag == 0:
                break
            count = reader.gamma(); assert count < 4096 and len(fields) < 1024
            fields.append(dict(tag=tag, name=reader.take(count).decode("utf-8")))
        for field in fields:
            if field["tag"] & 15 == 11:
                field["children"] = header(reader, depth + 1)
        return fields

    reader = Reader(raw)
    # Only real native framing selects STNN. Other RIFF/table payloads are skipped by their declared extents, including strings that happen to contain the same four bytes.
    while True:
        family = reader.take(4); assert family != bytes(4)
        mode = reader.byte(); kind = mode & 15
        if kind == CHUNK_RIFF:
            length = ((mode >> 4) << 24) | (reader.byte() << 16) | (reader.byte() << 8) | reader.byte(); reader.take(length)
        elif kind in CHUNK_RECORDS:
            if family != b"STNN":
                while True:
                    length = reader.gamma()
                    if length == 0:
                        break
                    reader.take(length - 1)
                continue
            assert kind == 3; extent = reader.gamma(); assert extent > 1
            description = Reader(reader.take(extent - 1)); fields = header(description); assert description.pos == len(description.data)
            assert [field["name"] for field in fields[:2]] == ["facilities", "normal"]
            assert fields[0]["tag"] == 2 and fields[1]["tag"] == 27
            normal = fields[1]["children"]; assert normal[0]["name"] == "base" and normal[0]["tag"] == 27
            number = 0
            # Dense table indices include empty row markers. Skip each complete earlier station rather than selecting a first/newest or normal/waypoint alias.
            while True:
                extent = reader.gamma(); assert extent != 0
                start = reader.pos; body = Reader(reader.take(extent - 1))
                if number != station:
                    number += 1; continue
                assert extent > 1; body.byte(); assert body.gamma() == 1 and body.gamma() == 1
                # Only the original base prefix precedes owner. Native scalar widths and the saved name's actual length determine the offset even for custom UTF-8 station names.
                for field in normal[0]["children"]:
                    if field["name"] == "owner":
                        assert field["tag"] == 2 and body.pos < len(body.data)
                        return start + body.pos
                    tag = field["tag"]; primitive = tag & 15
                    if primitive == 10:
                        body.take(body.gamma())
                    else:
                        width = {1: 1, 2: 1, 3: 2, 4: 2, 5: 4, 6: 4, 7: 8, 8: 8, 9: 2}[primitive]
                        body.take(width * (body.gamma() if tag & 16 else 1))
                raise AssertionError("Missing original native Station owner")
        else:
            raise AssertionError("Unsupported native framing")


def reload(configuration, proof_path, completion=None):
    """Require exact ordinary reconstruction and optional actual next action from that reconstructed checkpoint.

    @param configuration Matching current game/test build.
    @param proof_path Exact accepted current native action report.
    @param completion Optional matching uninterrupted next action whose exact predecessor is proof_path.
    @return New accepted ordinary/resumed proof report path.
    """
    proof = json.loads(proof_path.read_text(encoding="utf-8"))
    assert proof["configuration"] == configuration and proof["save_version"] == 430 and "after" in proof
    scope = f'{proof["branch"]}-{proof["phase"]}'; context = current_run_context("space", "probe-order-reload-" + scope)
    target = context.reports / "probe-order-reload.json"; assert not target.exists()
    source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    owner = context.for_feature("space", "probe-order-ordinary-" + scope)
    previous = os.environ.get("OTTD_TEST_CONFIGURATION"); os.environ["OTTD_TEST_CONFIGURATION"] = configuration
    try:
        ordinary(owner, {"name": "probe-order-" + scope, "snapshot": proof["after"]}, configuration)
    finally:
        if previous is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous
    resaved = owner.saves / "ordinary-resave.sav"; assert chunks(resaved)[0] == 430
    before, after = chunks(source)[1], chunks(resaved)[1]
    # Native physical packets/work, allowance, source/incarnation, company and Engine/group/rule owners must reconstruct exactly; queue digests alone cannot prove a once-only paid action.
    for family in EXACT:
        assert before[family] == after[family], (scope, family)
    result = dict(_documentation="Owner: r2mlvs10-asteroids-and-mining-probes.md Group8B; actual ordinary reconstruction and resaved next typed action across thirty native owner families.",
                  configuration=configuration, save_version=430, phase=proof["phase"], branch=proof["branch"], proof=identity(proof_path),
                  source=proof["after"], ordinary=identity(resaved), exact_owner_families=len(EXACT))
    owners = [owner]
    if completion is not None:
        accepted = json.loads(completion.read_text(encoding="utf-8"))
        assert accepted["configuration"] == configuration and accepted["original"] == proof["original"] and accepted["predecessor"] == identity(proof_path)
        resumed_owner = context.for_feature("space", "probe-order-resumed-" + scope)
        assertions, metadata = SHARED["native"](resumed_owner, configuration, accepted["phase"], resaved, proof["original"])
        resumed = resumed_owner.saves / f'probe-orders-{accepted["phase"]}-430.sav'
        uninterrupted = ROOT / accepted["after"]["path"]; assert identity(uninterrupted) == accepted["after"]
        before, after = chunks(uninterrupted)[1], chunks(resumed)[1]
        # Resume actual saved work/return claims, never its uninterrupted predecessor. The next real native opportunity must converge with every original paid owner intact.
        for family in EXACT:
            assert before[family] == after[family], ("resumed", scope, family)
        for key, value in metadata.items():
            assert accepted[key] == value, (scope, key)
        result.update(completion=identity(completion), resumed_input=identity(resaved), resumed=identity(resumed), assertions=assertions)
        owners.append(resumed_owner)
    result["native_reports"] = [identity(path) for item in owners for path in sorted(item.reports.glob("native-*.json"))]
    assert identity(source) == proof["after"]; target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return target


def action_rows(raw, original):
    """Locate one exact actual queue/manifest through bounded native RIFF rows.

    @param raw Decompressed native chunk stream.
    @param original Exact accepted actual paid hull incarnation.
    @return Original SPOR row/current instruction and SPSU row offsets.
    """
    key = original["vehicle"].to_bytes(4, "big") + original["generation"].to_bytes(8, "big")
    found = {}
    # Parse each real row's complete variable extent before selecting its original incarnation; matching bytes in cargo/strings cannot impersonate a queue or manifest owner.
    for family in (b"SPOR", b"SPSU"):
        _, start, length, end = riff_location(raw, family); cursor = start
        if family == b"SPOR":
            assert length >= 9; cursor += 8
        count = raw[cursor]; cursor += 1; assert count <= 64
        for _ in range(count):
            row = cursor
            if family == b"SPOR":
                assert cursor + 22 <= end
                current, entries = raw[cursor + 12], raw[cursor + 21]; assert current <= entries <= 32
                cursor += 22 + 14 * entries; assert cursor + 1 <= end
                present = raw[cursor]; cursor += 1; assert present <= 1; cursor += 30 * present
                if raw[row:row + 12] == key:
                    assert family not in found and current < entries
                    found[family] = (row, row + 22 + 14 * current)
            else:
                assert cursor + 61 <= end; pods = raw[cursor + 60]; assert pods <= 8; cursor += 61 + 10 * pods
                # Compartment packet counts bound every native reference; this parser never changes a packet or invents a service part.
                for _ in range(3):
                    assert cursor + 1 <= end; packets = raw[cursor]; assert packets <= 128; cursor += 1 + 6 * packets
                if raw[row:row + 12] == key:
                    assert family not in found; found[family] = row
            assert cursor <= end
        assert cursor == end
    assert len(found) == 2
    return *found[b"SPOR"], found[b"SPSU"]


def damage(context, source, original, kind):
    """Create one independent malformed active Return/Service copy without mutating its accepted source.

    @param context Unique malformed-file output owner.
    @param source Exact actual partial onboard-service checkpoint.
    @param original Same accepted original hull incarnation.
    @param kind One independent authority/progress/version invariant to violate.
    @return Explicit malformed copy consumed independently by full load and copied preview.
    """
    assert kind in KINDS; context.prepare_runtime(); version, raw = read_stream(source); assert version == 430
    queue, instruction, manifest = action_rows(raw, original)
    assert raw[instruction] == 5 and raw[instruction + 1] == 3 and int.from_bytes(raw[manifest + 38:manifest + 40], "big") == 111
    changed = bytearray(raw)
    if kind == "old-version":
        version = 429
    elif kind == "foreign-center":
        station = int.from_bytes(raw[instruction + 2:instruction + 6], "big")
        offset = station_owner_offset(raw, station); assert raw[offset] < 15
        changed[offset] = 16  # Native OWNER_NONE is not an owned operational service target; every original hull/anchor/part remains unchanged.
    elif kind == "wrong-action":
        changed[instruction] = 0
    elif kind == "wrong-domain":
        changed[instruction + 1] = 1
    elif kind == "unstarted":
        changed[queue + 13] = 0
    elif kind == "deadline":
        changed[queue + 15:queue + 19] = bytes(4)
    elif kind == "work":
        changed[queue + 19:queue + 21] = struct.pack(">H", 1)
    elif kind == "target-generation":
        changed[instruction + 6:instruction + 14] = (int.from_bytes(raw[instruction + 6:instruction + 14], "big") + 1).to_bytes(8, "big")
    elif kind == "pending":
        changed[manifest + 42] = 1
    else:
        changed[manifest + 40] = 1
    target = context.saves / f"probe-order-{kind}.sav"; assert not target.exists()
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed)
    return target


def reject(configuration, proof_path, kind):
    """Require full native startup and independent copied preview to refuse one malformed active service owner.

    @param configuration Matching current game/test build.
    @param proof_path Exact accepted partial onboard-service report.
    @param kind One independent malformed ownership/progress/version boundary.
    @return Current two-process accepted rejection report.
    """
    proof = json.loads(proof_path.read_text(encoding="utf-8")); assert proof["configuration"] == configuration and proof["phase"] == "service-partial"
    source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    context = current_run_context("space", "probe-order-reject-" + kind); malformed = damage(context, source, proof["original"], kind)
    full = context.for_feature("space", "probe-order-full-" + kind)
    SHARED["native"](full, configuration, "service-partial", malformed, proof["original"], reject=True)
    preview = context.for_feature("space", "probe-order-preview-" + kind)
    SHARED["native"](preview, configuration, "service-partial", source, proof["original"], corrupt=malformed)
    assert identity(source) == proof["after"]
    result = dict(_documentation="Owner: r2mlvs10-asteroids-and-mining-probes.md Group8B; independent full native load and copied preview refusal with unchanged open-world authority.",
                  configuration=configuration, kind=kind, proof=identity(proof_path), source=proof["after"], malformed=identity(malformed),
                  native_reports=[identity(path) for item in (full, preview) for path in sorted(item.reports.glob("native-*.json"))])
    target = context.reports / "probe-order-reject.json"; assert not target.exists(); target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Select one current ordinary/resumed or independent malformed boundary without searching latest outputs."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--proof", type=Path, required=True)
    parser.add_argument("--completion", type=Path)
    parser.add_argument("--kind", choices=KINDS)
    args = parser.parse_args(); assert args.kind is None or args.completion is None
    result = reject(args.configuration, args.proof.resolve(), args.kind) if args.kind else reload(args.configuration, args.proof.resolve(), args.completion.resolve() if args.completion else None)
    print(result.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
