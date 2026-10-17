"""Freeze actual original433 descent input before a later heavy receipt changes SPDJ.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CBBA
Native commands/ticks and original paid assets supply the partial/recovered packets; bounded Surface fuel fixture input remains explicit.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, read_stream
from launch_center_checks import riff_location

SHARED = runpy.run_path(str(Path(__file__).with_name("legacy_descent_checks-1.py")))
AUDIT = SHARED["AUDIT"]; EXACT = SHARED["EXACT"]
SOURCES = (
    "src/launch_center.h", "src/space_descent.cpp", "src/saveload/space_descent_sl.cpp", "src/saveload/saveload.h",
    "src/space_orders.cpp", "src/space_vehicle.cpp", "src/space_manifest.cpp", "src/space_pod.cpp",
    "src/tests/legacy_descent_native.cpp", "src/tests/CMakeLists.txt",
) + tuple(f"src/tests/multilayer/acceptance/legacy_descent_checks-{number}.py" for number in (1, 2))


def qualify(saved):
    """Prove that the actual partial input has the original current433 descent row and real20t packet owner.

    @param saved Exact native or independently ordinary/resumed partial save identity.
    @return Original sender identity and receipt dimensions for the later format consumer.
    """
    source = ROOT / saved["path"]; assert identity(source) == saved; version, raw = read_stream(source); assert version == 433
    _, body, length, end = riff_location(raw, b"SPDJ"); count = int.from_bytes(raw[body + 8:body + 10], "big"); assert count == 1
    row = body + 10; quantity = int.from_bytes(raw[row + 49:row + 51], "big"); work = int.from_bytes(raw[row + 51:row + 53], "big"); packets = raw[row + 56]
    assert quantity == 20 and work == 111 and packets > 0 and row + 57 + 6 * packets == end and length == 10 + 57 + 6 * packets
    return dict(sender=int.from_bytes(raw[row + 36:row + 40], "big"), generation=int.from_bytes(raw[row + 40:row + 48], "big"), quantity=quantity,
                remaining_ticks=work, packet_references=packets, row_fixed_bytes=57, source_version=version)


def configuration(name, selectors, references):
    """Audit one actual433 producer's explicit focused/native/ordinary/resumed evidence.

    @param name Matching current build configuration.
    @param selectors Exact native and reconstruction selectors in order.
    @param references Shared current artifact identity audit, stopping at accepted dated producer report/save boundaries.
    @return Qualified configuration row and exact saved physical states.
    """
    reports = []; processes = {}; states = {}; totals = dict(focused_assertions=0, focused_cases=0, native_assertions=0, resumed_assertions=0)
    # Complete fixed scope membership prevents unrelated success or an omitted ordinary/next-action checkpoint from replacing actual old descent evidence.
    for scope, selector, expected in zip(("native", "reloads"), selectors, (6, 5)):
        selected = SHARED["read"](selector); assert selected["configuration"] == name and selected["scope"] == scope and selected["complete"] and len(selected["reports"]) == expected
        AUDIT["inspect_references"](identity(selector), references)
        for member in selected["reports"]:
            path = ROOT / member; proof = SHARED["read"](path); assert proof["configuration"] == name; reports.append(identity(path))
            AUDIT["inspect_references"](reports[-1], references); AUDIT["inspect_references"](proof, references)
            for child in proof["native_reports"]:
                native = SHARED["read"](ROOT / child["path"]); assert native["configuration"] == name and native["exit_status"] == 0 and child["path"] not in processes
                AUDIT["inspect_references"](native, references); processes[child["path"]] = child
            if scope == "native":
                if proof["phase"] == "focused":
                    totals["focused_assertions"] = proof["assertions"]; totals["focused_cases"] = proof["cases"]
                else:
                    assert proof["save_version"] == 433 and proof["procurement_acceptance"] is False
                    assert proof["declared_surface_fuel_fixture"] == (proof["phase"] == "starting-fuel")
                    states["native", proof["phase"]] = proof["after"]; totals["native_assertions"] += proof["assertions"]
            else:
                assert proof["exact_owner_families"] == len(EXACT); states["ordinary", proof["phase"]] = proof["ordinary"]; totals["resumed_assertions"] += proof["assertions"]
                if "resumed" in proof:
                    states["resumed", proof["phase"]] = proof["resumed"]
    assert len(processes) == 15 and len(states) == 14 and totals["focused_assertions"] > 0
    actual = qualify(states["native", "partial"]); assert actual["sender"] == 16
    assert qualify(states["ordinary", "partial"]) == actual and qualify(states["resumed", "returning"]) == actual
    return dict(configuration=name, save_version=433, reports=reports, selectors=[identity(path) for path in selectors], processes=list(processes.values()),
                ordinary_checkpoints=5, resumed_continuations=4, legacy_descent=states["native", "partial"], legacy_receipt=actual, **totals), states


def main():
    """Freeze one new immutable dual-build legacy boundary after all current physical owner comparisons pass."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--release", nargs=2, type=Path, required=True)
    parser.add_argument("--debug", nargs=2, type=Path, required=True); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); target = args.output.resolve(); assert not target.exists(); references = {}
    release, left = configuration("RelWithDebInfo", [path.resolve() for path in args.release], references)
    debug, right = configuration("Debug", [path.resolve() for path in args.debug], references); assert left.keys() == right.keys(); comparisons = 0
    # Every actual packet, fee, hull, finite body, original shell and quota owner must converge across builds, including recovery from each process's own ordinary resave.
    for key in left:
        before, after = chunks(ROOT / left[key]["path"])[1], chunks(ROOT / right[key]["path"])[1]
        for family in EXACT:
            assert before[family] == after[family], (key, family); comparisons += 1
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CBBA; genuine actual433 small-miner descent before any heavy receipt/wire mutation. A separate one-trip Surface starting-fuel fixture remains explicit; procurement acceptance is false.",
                  configurations=[release, debug], exact_owner_families=len(EXACT), cross_configuration_comparisons=comparisons,
                  sources=[identity(ROOT / path) for path in SOURCES], direct_references=list(references.values()), entry_index=identity(ROOT / "logs/heavy-supply-custody-verification.json"), procurement_acceptance=False)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(json.dumps(dict(index=identity(target), owners=len(EXACT), comparisons=comparisons)), flush=True)


if __name__ == "__main__":
    main()
