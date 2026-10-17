"""Qualify actual paid recall exits, retained service owners and unchanged saved recovery boundaries.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group13
Original partial/full/service inputs remain immutable dated producers; no stock or movement is installed.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity

HERE = Path(__file__).parent
LEAF = runpy.run_path(str(HERE / "probe_exit_checks-1.py"))
GEOMETRY = runpy.run_path(str(HERE / "probe_return_checks-2.py"))
DECODER = runpy.run_path(str(HERE / "probe_repeat_checks-3.py"))
AUDIT = LEAF["AUDIT"]
SOURCES = tuple(dict.fromkeys((*GEOMETRY["SOURCES"], "src/asteroid_gui.cpp", "src/vehicle_cmd.cpp", "src/space_orders.cpp", "src/space_order_actions.cpp",
                             "src/launch_operations.cpp", "src/space_manifest.cpp", "src/saveload/space_order_sl.cpp", "src/saveload/space_production_sl.cpp",
                             "src/tests/probe_exit_native.cpp", "src/tests/probe_extraction.cpp", "src/tests/probe_order_actions.cpp",
                             "src/tests/multilayer/acceptance/probe_repeat_checks-3.py",
                             *(f"src/tests/multilayer/acceptance/probe_exit_checks-{n}.py" for n in (1, 2)))))


def configuration(name, selectors, references):
    """Qualify exact actual recall/service inputs and unchanged ordinary resaves for one current build.

    @param name Current native executable configuration.
    @param selectors Exact native/own/focused selector paths in that order.
    @param references Checked direct evidence, with historical producer indexes retained only as dated boundaries.
    @return Qualified current process counts, actual saved states and independently decoded owners.
    """
    rows = {}; proofs = []; focused = None
    # Every case uses its own explicit paid producer; a partial attached world cannot borrow a full hold, replacement parts or progress from another scenario.
    for scope, path in zip(LEAF["SCOPES"], selectors):
        selected = LEAF["read"](path); AUDIT["inspect"](selected, references)
        assert selected["complete"] and selected["configuration"] == name and selected["scope"] == scope and len(selected["reports"]) == len(LEAF["SCOPES"][scope])
        for phase, relative in zip(LEAF["SCOPES"][scope], selected["reports"]):
            path = ROOT / relative; proof = LEAF["read"](path); AUDIT["inspect"](proof, references)
            assert proof["complete"] and proof["configuration"] == name and proof["scope"] == scope and proof["phase"] == phase and not proof["added_stock"]
            assert proof["dated_index"] == LEAF["predecessor"](name, phase)[0] and len(proof["native_reports"]) == 1
            process = LEAF["read"](ROOT / proof["native_reports"][0]["path"]); AUDIT["inspect"](process, references)
            assert process["exit_status"] == 0 and process["executable"] == identity(ROOT / "build" / name / "openttd_test.exe")
            assert process["argv"][1:] == [LEAF["TAGS"] if scope == "focused" else "[.probe-exit-native]"]
            proof["_path"] = identity(path); proofs.append(proof)
            if scope == "focused":
                focused = proof; continue
            assert proof["source"] in process["inputs"] and proof["after"] in process["artifacts"]
            if scope == "own":
                parent = rows["native/" + phase]; assert proof["parent"] == parent["_path"] and proof["source"] == parent["after"]; AUDIT["compare"](proof["source"], proof["after"])
            else:
                assert proof["parent"] is None and proof["source"] == LEAF["predecessor"](name, phase)[1]
            rows[scope + "/" + phase] = proof
    assert len(proofs) == len({p["native_reports"][0]["path"] for p in proofs}) == 7 and focused
    checkpoints = {key: proof["after"] for key, proof in rows.items()}; decoded = {key: DECODER["snapshot"](saved) for key, saved in checkpoints.items()}
    # Native cancellation releases only its lease/uncommitted interval. Actual packet IDs, quantities, source tiles, finite extraction and paid service/pod history survive travel and reload.
    for key, state in decoded.items():
        phase = key.split("/")[1]; source = LEAF["predecessor"](name, phase)[1]; initial = DECODER["snapshot"](source); geometry = GEOMETRY["snapshot"](source)
        assert state["quantity"] == initial["quantity"] and state["reserve"] == initial["reserve"] and state["extracted"] == initial["extracted"]
        assert state["work"] == 0 and state["phase"] == 4 and state["lease"] == 1048575 and not state["descents"]
        assert state["manifests"] == initial["manifests"] and state["pods"] == initial["pods"] and state["next_pod"] == initial["next_pod"] and state["next_descent"] == initial["next_descent"]
        assert state["conserved_ore"] == initial["conserved_ore"]
        assert [(p["packet"], p["quantity"], p["tile"]) for p in state["packets"]] == [(p["packet"], p["quantity"], p["tile"]) for p in initial["packets"]]
        assert all(b["age"] >= a["age"] for a, b in zip(initial["packets"], state["packets"]))
        assert sum((p - a)**2 for p, a in zip(state["position"], geometry["anchor"])) <= (20 * 65536)**2
        if phase == "service":
            AUDIT["compare"](source, checkpoints[key])
        if phase == "partial":
            assert initial["work"] == 37 and initial["quantity"] == 2 and state["position"] != initial["position"]
    return dict(configuration=name, processes=7, focused_assertions=focused["assertions"], focused_cases=focused["cases"], native_assertions=sum(p["assertions"] for p in proofs if p is not focused),
                selectors=[identity(path) for path in selectors], reports=[p["_path"] for p in proofs], checkpoints=checkpoints, decoded_checkpoints=decoded,
                physical_owner_comparisons=150, metadata_comparisons=10)


def main():
    """Freeze current native exits only after shared command refusals and exact physical saved owners agree in both builds."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path, nargs=3, required=True); parser.add_argument("--debug", type=Path, nargs=3, required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); assert not args.output.exists(); references = {}
    release = configuration("RelWithDebInfo", [path.resolve() for path in args.release], references); debug = configuration("Debug", [path.resolve() for path in args.debug], references)
    # Matching original and ordinary-resaved exits must retain exact physical owners between builds; presentation and explicit cancellation cannot mint another batch or service credit.
    for key, saved in release["checkpoints"].items():
        AUDIT["compare"](saved, debug["checkpoints"][key])
    assert release["decoded_checkpoints"] == debug["decoded_checkpoints"]
    dated = [identity(ROOT / path) for path, _, _ in LEAF["ENTRIES"].values()]
    args.output.write_text(json.dumps(dict(_documentation=__doc__, save_version=438, configurations=[release, debug], sources=[identity(ROOT / path) for path in SOURCES],
                                         direct_references=list(references.values()), dated_producer_indexes=dated, cross_configuration_physical_comparisons=180,
                                         cross_configuration_metadata_comparisons=12, shared_mining_recall=True, paid_cargo_and_service_preserved=True,
                                         inherited_starting_content_procurement=False), indent=2) + "\n", encoding="utf-8")
    print(identity(args.output.resolve()), flush=True)


if __name__ == "__main__":
    main()
