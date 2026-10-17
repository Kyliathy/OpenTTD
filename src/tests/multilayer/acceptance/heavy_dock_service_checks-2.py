"""Pair independent clean full/copied refusals for both actual heavy Dock berth orderings.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group 9CCCC
Only invocation-owned malformed copies change. Native SPST/SPOR/CAPA framing selects each original paid job, source and physical packet.
"""
import argparse
import json
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, identity, read_stream
from runner import current_run_context

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_dock_service_checks-1.py")))
LAYOUT = SHARED["CORE"]["LAYOUT"]
PACKETS = runpy.run_path(str(Path(__file__).with_name("heavy_descent_checks-2.py")))
CASES = {"heavy-slot": "heavy-partial", "second-slot": "legacy-partial", "duplicate-occupant": "heavy-partial", "second-role": "legacy-partial",
         "second-work": "legacy-partial", "second-metal": "legacy-partial", "job-alias": "heavy-partial", "source-generation": "heavy-partial"}


def damage(context, source, kind):
    """Break one actual Dock invariant in a newly owned malformed save.

    @param context Invocation-owned malformed-artifact directories.
    @param source Exact accepted first- or second-berth heavy partial job.
    @param kind Independent physical berth, immutable receipt, packet or paid source violation.
    @return New malformed native save preserving every unrelated owner.
    """
    version, raw = read_stream(source); assert version == 436; layout = LAYOUT["structure_rows"](raw, version); dock = layout["rows"][4]
    assert dock["generation"] == 5 and dock["kind"] == 7; first, second = dock["jobs"]; changed = bytearray(raw)
    if kind in ("heavy-slot", "second-slot"):
        offset = LAYOUT["approach_slot_offset"](raw); assert raw[offset] == (0 if kind == "heavy-slot" else 1); changed[offset] = 1 - raw[offset]
    elif kind == "duplicate-occupant":
        changed[second["start"]:second["start"] + 12] = raw[first["start"]:first["start"] + 12]
    elif kind == "second-role":
        assert second["model"] == 21; offset = second["model_offset"]; changed[offset:offset + 2] = (65535).to_bytes(2, "big")
    elif kind == "second-work":
        assert second["model"] == 21; offset = second["start"] + 12; changed[offset:offset + 2] = (297).to_bytes(2, "big")
    elif kind == "second-metal":
        assert second["model"] == 21; part = second["parts"][0]; assert part["quantity"] == 2
        # A genuine heavy input may own one two-unit packet or two one-unit packets. Remove exactly one physical unit without forging a zero-count CAPA row that could crash native reconstruction.
        if part["count"] == 1:
            position = part["packet_offsets"][0]; packet = int.from_bytes(raw[position:position + 4], "big"); offset = PACKETS["packet_count_offset"](raw, packet)
            assert int.from_bytes(raw[offset:offset + 2], "big") == 2; changed[offset:offset + 2] = (1).to_bytes(2, "big")
        else:
            assert part["count"] == 2; changed[part["count_offset"]:part["count_offset"] + 2] = (1).to_bytes(2, "big")
            payload = bytearray(changed[layout["body"]:layout["end"]]); offset = part["packet_offsets"][-1] - layout["body"]; del payload[offset:offset + 6]
            changed = LAYOUT["replace_family"](raw, b"SPST", payload)
    elif kind == "job-alias":
        left, right = first["parts"][0]["packet_offsets"][0], second["parts"][0]["packet_offsets"][0]; changed[right:right + 4] = raw[left:left + 4]
    else:
        assert kind == "source-generation"; offset = first["end"] + 4; assert int.from_bytes(raw[offset:offset + 2], "big") == 0 and int.from_bytes(raw[offset + 2:offset + 10], "big") == 1
        changed[offset + 2:offset + 10] = (2).to_bytes(8, "big")
    assert changed != raw; target = context.saves / ("malformed-heavy-dock-" + kind + ".sav")
    target.write_bytes(b"OTTN" + struct.pack(">I", version << 16) + changed); return target


def verify(configuration, proofs, kind):
    """Require independent clean full startup and detached valid-world preview to refuse one same-file fault.

    @param configuration Matching current test build.
    @param proofs Explicit current native phase-to-report mapping.
    @param kind Exact independent Dock boundary.
    @return Immutable paired-reader refusal report.
    """
    selected = proofs[CASES[kind]]; proof = SHARED["CORE"]["read"](selected); assert proof["configuration"] == configuration
    source = ROOT / proof["after"]["path"]; assert identity(source) == proof["after"]; context = current_run_context("space", "heavy-dock-refusal-" + kind)
    malformed = damage(context, source, kind); full = context.for_feature("space", "heavy-dock-full-" + kind); preview = context.for_feature("space", "heavy-dock-preview-" + kind)
    SHARED["native"](full, configuration, "corrupt", malformed, reject=True); assertions, _ = SHARED["native"](preview, configuration, "corrupt", source, corrupt=malformed)
    assert identity(source) == proof["after"]; target = context.reports / "heavy-dock-refusal.json"
    target.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group 9CCCC; independent clean full/copied Dock berth, immutable job and actual packet/source refusal with active-world authority, money and RNG unchanged.",
                                     configuration=configuration, kind=kind, proof=identity(selected), source=proof["after"], malformed=identity(malformed), assertions=assertions,
                                     native_reports=[identity(path) for owner in (full, preview) for path in sorted(owner.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return target


def main():
    """Run four independent paired Dock refusals per focused burst from exact complete native membership."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"), required=True)
    parser.add_argument("--selector", nargs=3, type=Path, required=True); parser.add_argument("--part", choices=("berths", "inputs"), required=True); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); assert not args.output.exists(); proofs = {}; reports = []
    # Every candidate consumes its exact actual original role ordering, never an unrelated successful save or relabelled paid frame.
    for selector in args.selector:
        selected = SHARED["CORE"]["read"](selector); assert selected["configuration"] == args.configuration and selected["complete"] and selected["scope"].startswith("native-")
        for member in selected["reports"]:
            path = ROOT / member; phase = SHARED["CORE"]["read"](path)["phase"]; assert phase not in proofs; proofs[phase] = path
    assert set(proofs) == {"focused"} | set(SHARED["PARENTS"])
    # Each independent violation gets both real reader processes; four pairs bound the active burst without weakening complete fault membership.
    for kind in tuple(CASES)[:4] if args.part == "berths" else tuple(CASES)[4:]:
        reports.append(verify(args.configuration, proofs, kind)); print(args.configuration, kind, flush=True)
    args.output.write_text(json.dumps(dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group 9CCCC; explicit independent clean Dock reader membership.",
                                          configuration=args.configuration, scope="rejects-" + args.part, complete=True, reports=[path.relative_to(ROOT).as_posix() for path in reports]), indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
