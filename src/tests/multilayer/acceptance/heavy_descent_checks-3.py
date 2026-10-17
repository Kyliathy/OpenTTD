"""Freeze current heavy descent evidence after both bounded builds and independent reader checks pass.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB
Exact dated433 supply/legacy reports and saves remain input boundaries; current receipts, native work and original packet owners supply new acceptance.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, read_stream
from launch_center_checks import riff_location

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_descent_checks-1.py")))
FAULTS = runpy.run_path(str(Path(__file__).with_name("heavy_descent_checks-2.py")))["KINDS"]
AUDIT = SHARED["AUDIT"]; EXACT = SHARED["EXACT"]
SOURCES = (
    "src/launch_center.h", "src/space_descent.cpp", "src/space_manifest.h", "src/space_manifest.cpp",
    "src/fios.h", "src/fios_gui.cpp", "src/saveload/cargopacket_sl.cpp", "src/saveload/space_descent_sl.cpp",
    "src/saveload/space_manifest_sl.cpp", "src/saveload/saveload.h", "src/saveload/vehicle_sl.cpp", "src/space_orders.cpp", "src/space_pod.cpp",
    "src/tests/heavy_descent.cpp", "src/tests/heavy_descent_native.cpp", "src/tests/descent_custody.cpp", "src/tests/CMakeLists.txt",
) + tuple(f"src/tests/multilayer/acceptance/heavy_descent_checks-{number}.py" for number in (1, 2, 3))


def qualify(saved, legacy=False):
    """Check actual serialized partial provenance independently of cached report labels.

    @param saved Exact actual current partial save identity.
    @param legacy Whether this is the genuine433 job upgraded with explicitly absent new provenance.
    @return Exact original role, mass, work and quantity snapshot.
    """
    source = ROOT / saved["path"]; assert identity(source) == saved; version, raw = read_stream(source); assert version == 434
    _, body, length, end = riff_location(raw, b"SPDJ"); row = body + 10; assert int.from_bytes(raw[body + 8:body + 10], "big") == 1
    quantity = int.from_bytes(raw[row + 49:row + 51], "big"); work = int.from_bytes(raw[row + 51:row + 53], "big"); packets = raw[row + 61]
    model = int.from_bytes(raw[row + 56:row + 58], "big"); weight = raw[row + 58]; mass = int.from_bytes(raw[row + 59:row + 61], "big")
    assert packets > 0 and row + 62 + 6 * packets == end and length == 10 + 62 + 6 * packets and work == 111
    assert (model, weight, mass, quantity) == ((65535, 0, 0, 20) if legacy else (21, 16, 640, 40))
    return dict(sender=int.from_bytes(raw[row + 36:row + 40], "big"), generation=int.from_bytes(raw[row + 40:row + 48], "big"), sender_model=model,
                unit_weight=weight, freight_mass=mass, quantity=quantity, remaining_ticks=work, packet_references=packets, row_fixed_bytes=62)


def configuration(name, selectors, references):
    """Audit exact native, ordinary/resumed and both independently paired refusal scopes.

    @param name Matching completed build configuration.
    @param selectors Exact four current scope selectors, in acceptance order.
    @param references Shared direct retained artifact identity audit.
    @return Qualified configuration row and actual physical states for build comparison.
    """
    reports = []; processes = {}; states = {}; faults = set(); phases = set(); edges = set()
    totals = dict(focused_assertions=0, focused_cases=0, native_assertions=0, resumed_assertions=0, preview_assertions=0)
    # Explicit complete membership prevents a partial rerun or another success from replacing one required original checkpoint or full/copied refusal.
    for scope, selector, expected in zip(("native", "reloads", "rejects-receipts", "rejects-joins"), selectors, (8, 8, 6, 6)):
        selected = SHARED["read"](selector); assert selected["configuration"] == name and selected["scope"] == scope and selected["complete"] and len(selected["reports"]) == expected
        AUDIT["inspect_references"](identity(selector), references)
        for member in selected["reports"]:
            path = ROOT / member; proof = SHARED["read"](path); assert proof["configuration"] == name; reports.append(identity(path))
            AUDIT["inspect_references"](reports[-1], references); AUDIT["inspect_references"](proof, references)
            for child in proof["native_reports"]:
                native = SHARED["read"](ROOT / child["path"]); assert native["configuration"] == name and child["path"] not in processes
                intended = scope.startswith("rejects-") and native["scenario"].startswith("heavy-descent-full-")
                assert native["exit_status"] == (1 if intended else 0); AUDIT["inspect_references"](native, references)
                if intended:
                    log = ROOT / "logs/space" / native["scenario"] / native["run_id"] / "heavy-descent.log"; evidence = log.read_text(encoding="utf-8", errors="replace")
                    assert "Broken savegame" in evidence and "Fatal test error" not in evidence and "Assertion failed" not in evidence
                    AUDIT["inspect_references"](identity(log), references)
                processes[child["path"]] = dict(report=child, intended_refusal=intended)
            if scope == "native":
                if proof["phase"] == "focused":
                    totals["focused_assertions"] = proof["assertions"]; totals["focused_cases"] = proof["cases"]; assert proof["cases"] == 7
                else:
                    assert proof["save_version"] == 434 and proof["procurement_acceptance"] is False and proof["sender_destruction_fixture"] == (proof["phase"] == "sender-lost")
                    assert proof["phase"] not in phases; phases.add(proof["phase"]); states["native", proof["phase"], ""] = proof["after"]; totals["native_assertions"] += proof["assertions"]
            elif scope == "reloads":
                assert proof["exact_owner_families"] == len(EXACT); next_phase = SHARED["read"](ROOT / proof["completion"]["path"])["phase"] if "completion" in proof else ""
                key = (proof["phase"], next_phase); assert key not in edges; edges.add(key); states[("ordinary",) + key] = proof["ordinary"]
                if "resumed" in proof:
                    assert proof["resumed_input"] == proof["ordinary"]; states[("resumed",) + key] = proof["resumed"]; totals["resumed_assertions"] += proof["assertions"]
            else:
                assert proof["kind"] not in faults; faults.add(proof["kind"]); totals["preview_assertions"] += proof["assertions"]
    assert phases == set(SHARED["PARENTS"]) and faults == set(FAULTS)
    expected_edges = {(parent, child) for child, parent in SHARED["PARENTS"].items() if parent is not None} | {(phase, "") for phase in phases if phase not in SHARED["PARENTS"].values()}
    assert edges == expected_edges and len(processes) == 45 and len(states) == 20 and sum(row["intended_refusal"] for row in processes.values()) == 12
    heavy = qualify(states["native", "partial", ""]); orphan = qualify(states["native", "sender-lost", ""]); legacy = qualify(states["native", "legacy-upgrade", ""], True)
    assert heavy == orphan and heavy["sender"] == 22 and legacy["sender"] == 16
    return dict(configuration=name, save_version=434, reports=reports, selectors=[identity(path) for path in selectors], processes=list(processes.values()),
                ordinary_checkpoints=8, resumed_continuations=5, refusal_pairs=12, heavy_receipt=heavy, legacy_receipt=legacy, **totals), states


def main():
    """Freeze current dual-build identity and physical conservation only after every new scoped proof passes."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--release", nargs=4, type=Path, required=True)
    parser.add_argument("--debug", nargs=4, type=Path, required=True); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); target = args.output.resolve(); assert not target.exists(); references = {}
    release, left = configuration("RelWithDebInfo", [path.resolve() for path in args.release], references)
    debug, right = configuration("Debug", [path.resolve() for path in args.debug], references); assert left.keys() == right.keys(); comparisons = 0
    # All actual packet identities/ages/fees, hulls, finite resources, original quota and paid work must converge across compilers and each own-resave continuation.
    for key in left:
        before, after = chunks(ROOT / left[key]["path"])[1], chunks(ROOT / right[key]["path"])[1]
        for family in EXACT:
            assert before[family] == after[family], (key, family); comparisons += 1
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9CBB; actual40t paid-pod descent, immutable sender-loss provenance and genuine433 small recovery. Receiver full/loss unit scenarios and sender destruction fixture are explicit. Surface starting procurement remains unverified.",
                  configurations=[release, debug], exact_owner_families=len(EXACT), cross_configuration_comparisons=comparisons,
                  sources=[identity(ROOT / path) for path in SOURCES], direct_references=list(references.values()),
                  entry_indexes=[identity(ROOT / value[0]) for value in SHARED["ENTRIES"].values()], procurement_acceptance=False,
                  copied_scope="Same-file SPDJ arithmetic, paid/native sender/company/quota/target, pod custody and original CAPA identity/counts. Arbitrary GRF weight reconstruction and broader ordinary/hub packet exclusivity remain joined9CD scope.")
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(json.dumps(dict(index=identity(target), owners=len(EXACT), comparisons=comparisons)), flush=True)


if __name__ == "__main__":
    main()
