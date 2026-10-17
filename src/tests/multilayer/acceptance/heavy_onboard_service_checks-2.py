"""Pair independent actual full/copied refusals for heavy onboard source, bill and queue ownership.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CC
Only invocation-owned malformed copies change. Exact native SPSU/SPOR/CAPA framing selects original paid roles and physical part quantities.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, identity, read_stream
from runner import current_run_context

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_onboard_service_checks-1.py")))
MANIFEST_ROW = runpy.run_path(str(Path(__file__).with_name("heavy_supply_custody_checks-2.py")))["manifest_row"]
PACKET_OFFSET = runpy.run_path(str(Path(__file__).with_name("heavy_descent_checks-2.py")))["packet_count_offset"]
ACTION_ROWS = runpy.run_path(str(Path(__file__).with_name("probe_role_checks-3.py")))["action_rows"]
KINDS = ("future-progress", "future-history", "overwork", "allowance", "earth-location", "legacy-budget",
         "metal-shortage", "electronics-shortage", "missing-part-packet", "duplicate-part-packet", "unreached-order", "unmarked-target-loss")


def slots(raw):
    """Locate original heavy compartments from exact native row/variable-pod framing.

    @param raw Original decompressed current stream.
    @return Original manifest offset and per-compartment raw packet-ID field offsets.
    """
    row = MANIFEST_ROW(raw); position = row + 61 + 10 * raw[row + 60]; result = []
    # Every compartment keeps its original native count and routing fields; arbitrary matching bytes cannot impersonate an original service packet.
    for _ in range(3):
        count = raw[position]; assert count <= 128; result.append([position + 1 + 6 * index for index in range(count)]); position += 1 + 6 * count
    return row, result


def damage(context, source, kind):
    """Change one actual native source or service invariant in a newly owned malformed input.

    @param context Exact invocation-owned malformed-artifact destination.
    @param source Accepted partial, completed, reached-order or marked target-loss checkpoint.
    @param kind Independent source, work, quota, physical part or queue violation.
    @return New malformed file preserving original surrounding native framing.
    """
    version, raw = read_stream(source); assert version == 435; changed = bytearray(raw); row, compartments = slots(raw)
    if kind in ("future-progress", "future-history"):
        # SPSU/SPST/SPDJ framing is unchanged435 versus434: only unavailable heavy work/history becomes future capability, not misinterpreted new bytes or a pretend legacy producer.
        assert int.from_bytes(raw[row + 52:row + 60], "big") == (1 if kind == "future-history" else 0); version = 434
    elif kind == "overwork":
        changed[row + 38:row + 40] = (297).to_bytes(2, "big")
    elif kind == "allowance":
        assert raw[row + 40] == 0; changed[row + 40] = 1
    elif kind == "earth-location":
        assert raw[row + 43] == 1; changed[row + 43] = 0
    elif kind == "legacy-budget":
        legacy = MANIFEST_ROW(raw, 16, 2); assert int.from_bytes(raw[legacy + 38:legacy + 40], "big") == 0
        changed[legacy + 38:legacy + 40] = (149).to_bytes(2, "big")
    elif kind in ("metal-shortage", "electronics-shortage"):
        index = 1 if kind == "metal-shortage" else 2; assert len(compartments[index]) == 1
        position = compartments[index][0]; packet = int.from_bytes(raw[position:position + 4], "big"); offset = PACKET_OFFSET(raw, packet)
        assert int.from_bytes(raw[offset:offset + 2], "big") == 2; changed[offset:offset + 2] = (1).to_bytes(2, "big")
    elif kind == "missing-part-packet":
        assert len(compartments[1]) == 1; position = compartments[1][0]; changed[position:position + 4] = (2**20 - 2).to_bytes(4, "big")
    elif kind == "duplicate-part-packet":
        assert len(compartments[1]) == len(compartments[2]) == 1; left, right = compartments[1][0], compartments[2][0]; changed[right:right + 4] = raw[left:left + 4]
    else:
        queue, _, _ = ACTION_ROWS(raw, dict(vehicle=22, generation=8)); assert raw[queue + 13] == 1
        if kind == "unreached-order":
            # Keep a structurally valid queued-but-unstarted ReturnService while the forged timer already claims work: only physical reached-action ownership should authorize it.
            changed[queue + 13:queue + 21] = bytes(8)
        else:
            # SPOR writes cursor/started/blocked at12/13/14 before the four-byte timer and two-byte work receipt. Remove only the original loss marker; preserve every earned action/work field.
            assert kind == "unmarked-target-loss" and raw[queue + 14] != 0; changed[queue + 14] = 0
    assert changed != raw or version != 435
    target = context.saves / ("malformed-heavy-onboard-" + kind + ".sav"); target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed); return target


def verify(configuration, proofs, kind):
    """Require independent clean full startup and valid-world copied preview to reject the same candidate.

    @param configuration Matching actual build.
    @param proofs Explicit accepted phase-to-report mapping.
    @param kind Independent malformed service invariant.
    @return Immutable paired-refusal report path.
    """
    phase = "complete" if kind == "future-history" else "ordered" if kind == "unreached-order" else "target-lost" if kind == "unmarked-target-loss" else "partial"
    selected = proofs[phase]; proof = SHARED["read"](selected); assert proof["configuration"] == configuration
    source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]
    context = current_run_context("space", "heavy-onboard-refusal-" + kind); malformed = damage(context, source, kind)
    full = context.for_feature("space", "heavy-onboard-full-" + kind); preview = context.for_feature("space", "heavy-onboard-preview-" + kind)
    SHARED["native"](full, configuration, "corrupt", malformed, reject=True); assertions = SHARED["native"](preview, configuration, "corrupt", source, corrupt=malformed)
    assert identity(source) == proof["after"]; target = context.reports / "heavy-onboard-refusal.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CC; independent clean full/copied service admission with actual active-world owners, money and RNG preserved.",
                                     configuration=configuration, kind=kind, proof=identity(selected), source=proof["after"], malformed=identity(malformed), assertions=assertions,
                                     native_reports=[identity(path) for owner in (full, preview) for path in sorted(owner.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Select six independently paired receipt or packet/queue faults through exact accepted native membership."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--selector", type=Path, required=True); parser.add_argument("--output", type=Path, required=True); parser.add_argument("--part", choices=("receipts", "joins"), required=True)
    args = parser.parse_args(); assert not args.output.exists(); selected = SHARED["read"](args.selector); assert selected["configuration"] == args.configuration and selected["complete"]
    proofs = {SHARED["read"](ROOT / path)["phase"]: ROOT / path for path in selected["reports"]}; reports = []
    # Small explicit six-pair collectors preserve each independent reader boundary without turning a focused active-group burst into a full regression.
    for kind in KINDS[:6] if args.part == "receipts" else KINDS[6:]:
        reports.append(verify(args.configuration, proofs, kind)); print(args.configuration, kind, flush=True)
    args.output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CC; explicit independent clean full/copied refusal membership.",
                                          configuration=args.configuration, scope="rejects-" + args.part, complete=True, reports=[path.relative_to(ROOT).as_posix() for path in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
