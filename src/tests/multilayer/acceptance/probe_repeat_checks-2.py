"""Qualify paid descent custody, exhausted allowance and earned service across saved next actions.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group12
Original packet, shell, job, manifest and Surface owners remain authoritative throughout.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity

HERE = Path(__file__).parent
LEAF = runpy.run_path(str(HERE / "probe_repeat_checks-1.py"))
DECODER = runpy.run_path(str(HERE / "probe_repeat_checks-3.py"))
AUDIT = LEAF["AUDIT"]
SOURCES = tuple(dict.fromkeys((*DECODER["BASE"]["SOURCES"], "src/space_descent.cpp", "src/space_manifest.cpp", "src/space_manifest.h", "src/space_pod.cpp",
                             "src/space_transition.cpp", "src/space_order_actions.cpp", "src/space_departure.h", "src/saveload/space_manifest_sl.cpp", "src/saveload/space_descent_sl.cpp",
                             "src/tests/probe_repeat_native.cpp", "src/tests/descent_custody.cpp", "src/tests/probe_order_actions.cpp",
                             *(f"src/tests/multilayer/acceptance/probe_repeat_checks-{n}.py" for n in (1, 2, 3)))))
EXPECTED = {"first-pending": (2, 1, 0, 0, 1, 20), "first-recovered": (1, 0, 0, 0, 1, 20),
            "second-pending": (1, 1, 0, 0, 0, 40), "exhausted": (0, 0, 0, 0, 0, 40),
            "resupplied": (0, 0, 0, 0, 2, 40), "service-partial": (0, 0, 74, 0, 2, 40),
            "service-complete": (2, 0, 0, 1, 2, 40), "third-assigned": (2, 0, 0, 1, 2, 40)}


def configuration(name, selectors, references):
    """Qualify exact action ancestry, ordinary saves and actual next actions for one current build.

    @param name Native executable configuration.
    @param selectors Exact native-a/native-b/own/resume/focused selector paths.
    @param references Direct checked identities; historical indexes remain dated boundaries.
    @return Qualified current reports, checkpoints and decoded state.
    """
    rows = {}; proofs = []; focused = None
    # Explicit ancestry prevents replaying a command during an unchanged resave or substituting a newer world for the paid custody boundary.
    for scope, path in zip(LEAF["SCOPES"], selectors):
        selected = LEAF["read"](path); AUDIT["inspect"](selected, references)
        assert selected["complete"] and selected["configuration"] == name and selected["scope"] == scope and len(selected["reports"]) == len(LEAF["SCOPES"][scope])
        for phase, relative in zip(LEAF["SCOPES"][scope], selected["reports"]):
            path = ROOT / relative; proof = LEAF["read"](path); AUDIT["inspect"](proof, references)
            assert proof["complete"] and proof["configuration"] == name and proof["scope"] == scope and proof["phase"] == phase and not proof["added_stock"]
            assert proof["dated_index"] == identity(LEAF["ENTRY"]) and len(proof["native_reports"]) == 1
            process = LEAF["read"](ROOT / proof["native_reports"][0]["path"]); AUDIT["inspect"](process, references)
            assert process["exit_status"] == 0 and process["executable"] == identity(ROOT / "build" / name / "openttd_test.exe")
            assert process["argv"][1:] == [LEAF["TAGS"] if scope == "focused" else "[.probe-repeat-native]"]
            proof["_path"] = identity(path); proofs.append(proof)
            if scope == "focused":
                focused = proof; continue
            assert proof["source"] in process["inputs"] and proof["after"] in process["artifacts"]
            category = "native" if scope.startswith("native") else scope; parent_category = "native" if category in ("native", "own") else "own"
            parent_phase = phase if category == "own" else LEAF["PARENTS"].get(phase); parent = rows.get(parent_category + "/" + str(parent_phase))
            if parent:
                assert proof["parent"] == parent["_path"] and proof["source"] == parent["after"]
            else:
                assert phase == "first-pending" and proof["parent"] is None and proof["source"] == LEAF["predecessor"](name)[1]
            rows[category + "/" + phase] = proof
            if category == "own":
                AUDIT["compare"](proof["source"], proof["after"])
            elif category == "resume":
                AUDIT["compare"](rows["native/" + phase]["after"], proof["after"])
    assert len(proofs) == len({p["native_reports"][0]["path"] for p in proofs}) == 15 and focused
    checkpoints = {key: proof["after"] for key, proof in rows.items()}; decoded = {key: DECODER["snapshot"](saved) for key, saved in checkpoints.items()}
    initial = DECODER["snapshot"](LEAF["predecessor"](name)[1]); original_pods = initial["manifests"]["16"]["pods"]
    # Two actual finite holds finance every return. Recovery expends each shell; the replacement flight grants no quota, and completed paid work alone replaces allowance.
    for key, state in decoded.items():
        phase = key.split("/")[1]; quota, pending, ticks, services, pod_count, extracted = EXPECTED[phase]; manifest = state["manifests"]["16"]
        assert (manifest["quota"], manifest["pending"], manifest["ticks"], manifest["services"] - initial["manifests"]["16"]["services"], len(manifest["pods"]), state["extracted"]) == EXPECTED[phase]
        assert state["quantity"] == 0 and state["reserve"] == 1760 - extracted and state["conserved_ore"] == extracted and state["next_pod"] == initial["next_pod"]
        assert state["next_descent"] == initial["next_descent"] + (1 if extracted == 20 else 2) and len(state["descents"]) == pending
        assert manifest["supplies"][1:] == ([1, 1] if phase in ("resupplied", "service-partial") else [0, 0]) and state["phase"] == (1 if phase == "third-assigned" else 4)
        if pending:
            descent, = state["descents"]; assert [descent["pod"], descent["generation"]] == original_pods[0 if extracted == 20 else 1]
            assert descent["remaining"] == (111 if phase == "first-pending" else 148)
            assert descent["job"] == initial["next_descent"] + (0 if extracted == 20 else 1)
            if phase == "first-pending":
                assert descent["packets"] == [packet["packet"] for packet in initial["packets"]]
        assert len(state["pods"]) == 10 - (0 if phase == "first-pending" else 1 if phase in ("first-recovered", "second-pending") else 2)
        assert all(pod["generation"] == initial["pods"][identifier]["generation"] and pod["capital"] == initial["pods"][identifier]["capital"] > 0 for identifier, pod in state["pods"].items())
        if phase in ("resupplied", "service-partial", "service-complete", "third-assigned"):
            assert state["manifests"]["19"]["burned"] == initial["manifests"]["19"]["burned"] + 15 and all(pod not in original_pods for pod in manifest["pods"])
    return dict(configuration=name, processes=15, focused_assertions=focused["assertions"], focused_cases=focused["cases"], native_assertions=sum(p["assertions"] for p in proofs if p is not focused),
                selectors=[identity(path) for path in selectors], reports=[p["_path"] for p in proofs], checkpoints=checkpoints, decoded_checkpoints=decoded, physical_owner_comparisons=180, metadata_comparisons=12)


def main():
    """Freeze both builds only after paid custody, service and exact saved next actions qualify."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path, nargs=5, required=True); parser.add_argument("--debug", type=Path, nargs=5, required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); assert not args.output.exists(); references = {}
    release = configuration("RelWithDebInfo", [path.resolve() for path in args.release], references); debug = configuration("Debug", [path.resolve() for path in args.debug], references)
    # Original, unchanged and resumed paid checkpoints retain identical physical owners between configurations; reload cannot grant another service or cargo handoff.
    for key, saved in release["checkpoints"].items():
        AUDIT["compare"](saved, debug["checkpoints"][key])
    assert release["decoded_checkpoints"] == debug["decoded_checkpoints"]
    args.output.write_text(json.dumps(dict(_documentation=__doc__, save_version=438, configurations=[release, debug], sources=[identity(ROOT / path) for path in SOURCES],
                                         direct_references=list(references.values()), dated_producer_indexes=[identity(LEAF["ENTRY"])],
                                         cross_configuration_physical_comparisons=420, cross_configuration_metadata_comparisons=28,
                                         actual_paid_descent=True, actual_same_rocket_shell_replacement=True, earned_onboard_service=True, inherited_starting_content_procurement=False), indent=2) + "\n", encoding="utf-8")
    print(identity(args.output.resolve()), flush=True)


if __name__ == "__main__":
    main()
