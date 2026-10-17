"""Qualify native exit tails, original paid cancellations and guarded recovery identities.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group16
Original paid inputs remain immutable dated producers; isolated final-unit reserve is labeled in its component case and earns actual native work.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity

HERE = Path(__file__).parent
LEAF = runpy.run_path(str(HERE / "probe_boundary_checks-1.py"))
GEOMETRY = runpy.run_path(str(HERE / "probe_return_checks-2.py"))
DECODER = runpy.run_path(str(HERE / "probe_repeat_checks-3.py"))
AUDIT = LEAF["AUDIT"]
SOURCES = tuple(dict.fromkeys((*GEOMETRY["SOURCES"], "src/asteroid_gui.cpp", "src/vehicle_cmd.cpp", "src/space_orders.cpp", "src/space_order_actions.cpp",
                             "src/launch_operations.cpp", "src/space_manifest.cpp", "src/saveload/space_order_sl.cpp", "src/saveload/space_production_sl.cpp",
                             "src/tests/probe_boundary_native.cpp", "src/tests/probe_extraction.cpp", "src/tests/probe_order_actions.cpp", "src/tests/descent_custody.cpp", "src/station_cmd.cpp", "src/space_transition.cpp",
                             "src/tests/multilayer/acceptance/probe_repeat_checks-3.py",
                             *(f"src/tests/multilayer/acceptance/probe_boundary_checks-{n}.py" for n in (1, 2)))))


def configuration(name, selectors, references):
    """Qualify original paid cancellation/allowance inputs and unchanged ordinary resaves for one current build.

    @param name Current native executable configuration.
    @param selectors Exact native/own/focused selector paths in that order.
    @param references Checked direct evidence, with historical producer indexes retained only as dated boundaries.
    @return Qualified current process counts, actual saved states and independently decoded owners.
    """
    rows = {}; proofs = []; focused = None
    # Every selected paid boundary uses its exact producer; final-unit work belongs only to the declared component and cannot substitute paid procurement.
    for scope, path in zip(LEAF["SCOPES"], selectors):
        selected = LEAF["read"](path); AUDIT["inspect"](selected, references)
        assert selected["complete"] and selected["configuration"] == name and selected["scope"] == scope and len(selected["reports"]) == len(LEAF["SCOPES"][scope])
        # Every native/refreshed save requires its own retained original process and exact artifact identity; a successful selector alone is insufficient.
        for phase, relative in zip(LEAF["SCOPES"][scope], selected["reports"]):
            path = ROOT / relative; proof = LEAF["read"](path); AUDIT["inspect"](proof, references)
            assert proof["complete"] and proof["configuration"] == name and proof["scope"] == scope and proof["phase"] == phase and not proof["added_stock"]
            assert proof["dated_index"] == LEAF["predecessor"](name, phase)[0] and len(proof["native_reports"]) == 1
            process = LEAF["read"](ROOT / proof["native_reports"][0]["path"]); AUDIT["inspect"](process, references)
            assert process["exit_status"] == 0 and process["executable"] == identity(ROOT / "build" / name / "openttd_test.exe")
            assert process["argv"][1:] == [LEAF["TAGS"] if scope == "focused" else "[.probe-boundary-native]"]
            terminal = (ROOT / proof["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
            assert f"All tests passed ({proof['assertions']} assertions in {proof['cases']} test case" in terminal
            proof["_path"] = identity(path); proofs.append(proof)
            if scope == "focused":
                focused = proof; continue
            assert proof["source"] in process["inputs"] and proof["after"] in process["artifacts"]
            if scope == "own":
                parent = rows["native/" + phase]; assert proof["parent"] == parent["_path"] and proof["source"] == parent["after"]; AUDIT["compare"](proof["source"], proof["after"])
            else:
                assert proof["parent"] is None and proof["source"] == LEAF["predecessor"](name, phase)[1]
            rows[scope + "/" + phase] = proof
    assert len(proofs) == len({p["native_reports"][0]["path"] for p in proofs}) == 13 and focused
    checkpoints = {key: proof["after"] for key, proof in rows.items()}; decoded = {key: DECODER["snapshot"](saved) for key, saved in checkpoints.items()}
    # Cancellation ends only the sender's uncommitted interval. Actual emitted packets, finite debit, paid shell history and independent descent reservations remain with their original owners.
    for key, state in decoded.items():
        phase = key.split("/")[1]
        source = LEAF["predecessor"](name, phase)[1]
        initial = DECODER["snapshot"](source)
        assert state["quantity"] == initial["quantity"] and state["reserve"] == initial["reserve"] and state["extracted"] == initial["extracted"]
        assert state["work"] == 0 and state["lease"] == 1048575
        assert state["manifests"] == initial["manifests"] and state["pods"] == initial["pods"]
        assert state["next_pod"] == initial["next_pod"] and state["next_descent"] == initial["next_descent"] and state["conserved_ore"] == initial["conserved_ore"]
        assert [(p["packet"], p["quantity"], p["tile"]) for p in state["packets"]] == [(p["packet"], p["quantity"], p["tile"]) for p in initial["packets"]]
        assert all(b["age"] >= a["age"] for a, b in zip(initial["packets"], state["packets"]))
        if phase in ("exhausted", "home"):
            AUDIT["compare"](source, checkpoints[key])
        else:
            assert state["phase"] in (3, 4) and state["elapsed"] == initial["elapsed"] + 75
        if phase == "pending":
            assert len(state["descents"]) == len(initial["descents"]) == 1
            before, after = initial["descents"][0], state["descents"][0]
            assert after == dict(before, remaining=before["remaining"] - 75) and after["remaining"] == 36
        else:
            assert state["descents"] == initial["descents"]

    return dict(configuration=name, processes=13, focused_assertions=focused["assertions"], focused_cases=focused["cases"], native_assertions=sum(p["assertions"] for p in proofs if p is not focused),
                selectors=[identity(path) for path in selectors], reports=[p["_path"] for p in proofs], checkpoints=checkpoints, decoded_checkpoints=decoded,
                physical_owner_comparisons=240, metadata_comparisons=16)


def retarget(references):
    """Retain the accepted original paid alternate-recovery command boundary without replaying its historical binary graph.

    @param references Checked direct report, index and original paid save identities.
    @return Exact original paid retarget reports and completed saves for both configurations.
    """
    entry = ROOT / "logs/descent-custody-verification.json"
    assert identity(entry)["sha256"] == "370e4e14d6d9e77c1ade61d2b2cc52c2d4c263215abdd00d3ba6cc3594bf7723"
    AUDIT["inspect"](identity(entry), references)
    result = []
    # These dated reports own the actual purchased alternate and explicit retarget. Only their direct saved boundaries are consumed, not an obsolete executable/source graph.
    for report in LEAF["read"](entry)["reports"]:
        if Path(report["path"]).name != "descent-custody.json":
            continue
        proof = LEAF["read"](ROOT / report["path"])
        if proof["phase"] != "retarget":
            continue
        assert proof["conserved_mined_units"] == 20 and proof["actual_tick_expense_checked"]
        selected = dict(configuration=proof["configuration"], report=report, completed=proof["complete"], own=proof["ordinary_resave"])
        AUDIT["inspect"](selected, references)
        result.append(selected)
    assert {p["configuration"] for p in result} == {"RelWithDebInfo", "Debug"} and len(result) == 2
    return result


def main():
    """Freeze native boundaries only after real cancellation, original cargo custody and guarded recovery agree in both builds."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path, nargs=3, required=True); parser.add_argument("--debug", type=Path, nargs=3, required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); assert not args.output.exists(); references = {}
    release = configuration("RelWithDebInfo", [path.resolve() for path in args.release], references); debug = configuration("Debug", [path.resolve() for path in args.debug], references)
    # Matching original and ordinary-resaved exits must retain exact physical owners between builds; presentation and explicit cancellation cannot mint another batch or service credit.
    for key, saved in release["checkpoints"].items():
        AUDIT["compare"](saved, debug["checkpoints"][key])
    assert release["decoded_checkpoints"] == debug["decoded_checkpoints"]
    dated = [identity(ROOT / path) for path in dict.fromkeys(row[0] for row in LEAF["ENTRIES"].values())]
    paid_retarget = retarget(references)
    args.output.write_text(json.dumps(dict(_documentation=__doc__, save_version=438, configurations=[release, debug], sources=[identity(ROOT / path) for path in SOURCES],
                                         direct_references=list(references.values()), dated_producer_indexes=dated, cross_configuration_physical_comparisons=360,
                                         cross_configuration_metadata_comparisons=24, shared_mining_recall=True, paid_cargo_and_service_preserved=True, body_removal_command_supported=False,
                                         exhausted_body_ids_retained=True, paid_explicit_retarget_producers=paid_retarget,
                                         inherited_starting_content_procurement=False), indent=2) + "\n", encoding="utf-8")
    print(identity(args.output.resolve()), flush=True)


if __name__ == "__main__":
    main()
