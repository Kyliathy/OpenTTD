"""Qualify per-resource native ownership, exact saved phase continuations and original repeated paid returns.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group17
Original raw resources and paid shell/work histories remain separate from service parts and processed goods.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity

HERE = Path(__file__).parent
LEAF = runpy.run_path(str(HERE / "probe_conservation_checks-1.py"))
FLOW = runpy.run_path(str(HERE / "probe_conservation_checks-3.py"))
STATE = runpy.run_path(str(HERE / "probe_repeat_checks-3.py"))
AUDIT = LEAF["AUDIT"]
SOURCES = tuple(dict.fromkeys((*STATE["BASE"]["SOURCES"], "src/launch_center.h", "src/space_descent.cpp", "src/space_pod.cpp",
                             "src/space_manifest.cpp", "src/saveload/space_descent_sl.cpp", "src/saveload/space_manifest_sl.cpp", "src/saveload/space_pod_sl.cpp",
                             "src/saveload/station_sl.cpp", "src/tests/probe_conservation_native.cpp",
                             "src/tests/multilayer/acceptance/probe_repeat_checks-3.py",
                             *(f"src/tests/multilayer/acceptance/probe_conservation_checks-{n}.py" for n in (1, 2, 3)))))


def configuration(name, selectors, references):
    """Qualify exact original and own-save phase inputs, unique resource holdings and matching real next decisions.

    @param name Current native build configuration.
    @param selectors Explicit native and own-save-resume selectors in prerequisite order.
    @param references Independently checked direct report/process/save identities.
    @return Qualified original process counts and every conserved own/next checkpoint.
    """
    rows = {}
    proofs = []
    checkpoints = {}
    # Each phase must use its exact original producer, and its repeated continuation must start at that same invocation's unchanged own save.
    for scope, selector in zip(LEAF["SCOPES"], selectors):
        selected = LEAF["read"](selector)
        AUDIT["inspect"](selected, references)
        assert selected["complete"] and selected["configuration"] == name and selected["scope"] == scope
        assert len(selected["reports"]) == len(LEAF["PHASES"])
        # Every semantic checkpoint requires its own successful native process and actual saved outputs; selector membership alone cannot earn a simulation step.
        for phase, relative in zip(LEAF["PHASES"], selected["reports"]):
            path = ROOT / relative
            proof = LEAF["read"](path)
            AUDIT["inspect"](proof, references)
            assert proof["complete"] and proof["configuration"] == name and proof["scope"] == scope and proof["phase"] == phase
            assert proof["opportunities"] == 1 and not proof["added_stock"] and len(proof["native_reports"]) == 1
            assert proof["dated_index"] == LEAF["predecessor"](name, phase)[0]
            process = LEAF["read"](ROOT / proof["native_reports"][0]["path"])
            AUDIT["inspect"](process, references)
            assert process["exit_status"] == 0 and process["executable"] == identity(ROOT / "build" / name / "openttd_test.exe")
            assert process["argv"][1:] == ["[.probe-conservation-native]"]
            assert proof["source"] in process["inputs"] and proof["own"] in process["artifacts"] and proof["after"] in process["artifacts"]
            terminal = (ROOT / proof["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
            assert f"All tests passed ({proof['assertions']} assertions in {proof['cases']} test case" in terminal
            if scope == "native":
                assert proof["parent"] is None and proof["source"] == LEAF["predecessor"](name, phase)[1]
            else:
                parent = rows["native/" + phase]
                assert proof["parent"] == parent["_path"] and proof["source"] == parent["own"]
                AUDIT["compare"](parent["after"], proof["after"])
            AUDIT["compare"](proof["source"], proof["own"])
            proof["_path"] = identity(path)
            rows[scope + "/" + phase] = proof
            proofs.append(proof)
            checkpoints[scope + "/" + phase + "/own"] = proof["own"]
            checkpoints[scope + "/" + phase + "/next"] = proof["after"]
    assert len(proofs) == len({p["native_reports"][0]["path"] for p in proofs}) == 10
    decoded = {}
    # Independently decode every physical holding and finite reserve before interpreting a saved mission phase. All three resources participate, including the unchanged classes.
    for key, saved in checkpoints.items():
        scope, phase, stage = key.split("/")
        flow = FLOW["inventory"](saved)
        state = STATE["snapshot"](saved)
        initial = STATE["snapshot"](LEAF["predecessor"](name, phase)[1])
        after = stage == "next"
        assert state["elapsed"] == initial["elapsed"] + int(after)
        assert state["quantity"] == initial["quantity"] and state["extracted"] == initial["extracted"] and state["reserve"] == initial["reserve"]
        assert state["manifests"] == initial["manifests"] and state["pods"] == initial["pods"]
        assert state["next_pod"] == initial["next_pod"] and state["next_descent"] == initial["next_descent"]
        assert state["work"] == initial["work"] + int(after and phase in ("attach", "mine"))
        if phase == "descent":
            assert len(state["descents"]) == 1 and state["descents"][0]["remaining"] == 111 - int(after)
        decoded[key] = dict(flow=flow, state=state)
    return dict(configuration=name, processes=10, native_assertions=sum(p["assertions"] for p in proofs),
                selectors=[identity(p) for p in selectors], reports=[p["_path"] for p in proofs], checkpoints=checkpoints,
                decoded_checkpoints=decoded, physical_owner_comparisons=450, metadata_comparisons=30)


def cycles(references):
    """Reconcile exact accepted repeated-return phases through their original packets and paid shell owners.

    @param references Checked direct dated index/save identities, without reopening historical executables or source graphs.
    @return Separate resource equations and original packet holdings for every accepted paid repeat checkpoint.
    """
    entry, _ = LEAF["predecessor"]("RelWithDebInfo", "descent")
    AUDIT["inspect"](entry, references)
    result = []
    # Original paid cycles include split descent packets, Surface recovery, shell replacement, actual parts consumption and the following rendezvous. None can reset finite reserve or duplicate a holding.
    for selected in LEAF["read"](ROOT / entry["path"])["configurations"]:
        checkpoints = {}
        for key, saved in selected["checkpoints"].items():
            AUDIT["inspect"](saved, references)
            flow = FLOW["inventory"](saved)
            state = STATE["snapshot"](saved)
            assert state["conserved_ore"] == state["extracted"]
            checkpoints[key] = dict(save=saved, flow=flow, manifests=state["manifests"], pods=state["pods"], descents=state["descents"])
        first = checkpoints["native/first-pending"]
        final = checkpoints["native/third-assigned"]
        assert first["flow"]["resources"]["NORE"]["descent"] == 20
        assert final["flow"]["resources"]["NORE"]["surface_stock"] == 40
        assert first["manifests"]["16"]["services"] == 1 and final["manifests"]["16"]["services"] == 2
        assert final["manifests"]["16"]["quota"] == 2 and final["manifests"]["16"]["supplies"][1:] == [0, 0]
        result.append(dict(configuration=selected["configuration"], checkpoints=checkpoints))
    assert {p["configuration"] for p in result} == {"RelWithDebInfo", "Debug"}
    return result


def main():
    """Freeze current exact phase decisions and per-resource paid conservation only after both native builds agree."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path, nargs=2, required=True)
    parser.add_argument("--debug", type=Path, nargs=2, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    references = {}
    release = configuration("RelWithDebInfo", [p.resolve() for p in args.release], references)
    debug = configuration("Debug", [p.resolve() for p in args.debug], references)
    # All original/resumed own and next saves must agree across builds, including actual packet age, cost/travel/work remainders, leases and independent descent custody.
    for key, saved in release["checkpoints"].items():
        AUDIT["compare"](saved, debug["checkpoints"][key])
    assert release["decoded_checkpoints"] == debug["decoded_checkpoints"]
    repeated = cycles(references)
    dated = [identity(ROOT / path) for path in dict.fromkeys(row[0] for row in LEAF["ENTRIES"].values())]
    args.output.write_text(json.dumps(dict(_documentation=__doc__, save_version=438, configurations=[release, debug], repeated_paid_cycles=repeated,
                                         sources=[identity(ROOT / path) for path in SOURCES], direct_references=list(references.values()), dated_producer_indexes=dated,
                                         cross_configuration_physical_comparisons=600, cross_configuration_metadata_comparisons=40,
                                         per_body_and_resource_conserved=True, exclusive_packet_owners=True, raw_ore_consumed=0, raw_ore_disposed=0,
                                         added_stock=False, inherited_starting_content_procurement=False), indent=2) + "\n", encoding="utf-8")
    print(identity(args.output.resolve()), flush=True)


if __name__ == "__main__":
    main()
