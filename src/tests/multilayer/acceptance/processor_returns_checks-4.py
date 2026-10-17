"""Freeze paid product return and train/lorry collection after exact native replay qualification.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12B
Historical Surface stock remains distinct from actual processed exports; no descent/handling earns delivery income.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity

HERE = Path(__file__).parent
LEAF = runpy.run_path(str(HERE / "processor_returns_checks-1.py"))
SCOPED = runpy.run_path(str(HERE / "processor_returns_checks-2.py"))
DECODE = runpy.run_path(str(HERE / "processor_returns_checks-3.py"))
AUDIT = runpy.run_path(str(HERE / "orbital_runtime_checks-3.py"))
FREIGHT = runpy.run_path(str(HERE / "processor_freight_checks-3.py"))
SOURCES = tuple(dict.fromkeys((*FREIGHT["SOURCES"], "src/tests/processor_returns_native.cpp", "src/space_pod.cpp", "src/space_pod.h",
                             "src/space_descent.cpp", "src/launch_center.cpp", "src/launch_center.h", "src/launch_hub.cpp", "src/surface_handling.cpp",
                             "src/saveload/launch_center_sl.cpp", "src/saveload/space_pod_sl.cpp", "src/saveload/space_descent_sl.cpp",
                             "src/tests/surface_collection_boundaries.cpp", "src/tests/space_logistics.cpp",
                             "src/tests/multilayer/acceptance/orbital_processor_checks-3.py",
                             *(f"src/tests/multilayer/acceptance/processor_returns_checks-{n}.py" for n in (1, 2, 3, 4)))))


def qualify(configuration, path, references):
    """Qualify an actual current native process and exact ordinary source/output membership.

    @param configuration Exact native executable configuration.
    @param path Explicit retained semantic report.
    @param references Checked direct identities, with dated producers remaining immutable boundaries.
    @return Original qualified report with its own identity attached.
    """
    proof = LEAF["read"](path); AUDIT["inspect"](proof, references)
    assert proof["complete"] and proof["configuration"] == configuration and not proof["added_stock"]
    assert proof["dated_index"] == identity(LEAF["ENTRY"]) and len(proof["native_reports"]) == 1
    process = LEAF["read"](ROOT / proof["native_reports"][0]["path"]); AUDIT["inspect"](process, references)
    assert process["configuration"] == configuration and process["exit_status"] == 0
    assert process["executable"] == identity(ROOT / "build" / configuration / "openttd_test.exe")
    assert process["argv"][1:] == [LEAF["TAGS"] if proof["scope"] == "focused" else "[.processor-returns-native]"]
    terminal = (ROOT / proof["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
    assert f"All tests passed ({proof['assertions']} assertions in {proof['cases']} test case" in terminal
    if proof["after"]:
        assert proof["source"] in process["inputs"] and proof["after"] in process["artifacts"]
        assert chunks(ROOT / proof["after"]["path"])[0] == 438
    proof["_path"] = identity(path)
    return proof


def material_flow(decoded):
    """Prove original physical products retain one chosen owner through flights and collection.

    @param decoded Independently decoded native/own/resumed checkpoint mapping.
    """
    phases = LEAF["PHASES"]; initial = decoded["native/inspected"]; final = decoded["native/collected"]
    assert initial["stations"]["4"] == dict(STEL=36, GOOD=64, PROP=38)
    members = final["hubs"]["4"]; assert len(members) == 2
    rail, = [m for m in members if final["facilities"][str(m)] == 1]
    lorry, = [m for m in members if final["facilities"][str(m)] == 2]
    historical = set(initial["hubs"]["4"]) - {4}; assert historical.isdisjoint(members)
    # Each actual source, carrier, descent and handling reservation contributes once. Historical Surface stock remains separate from the newly emitted source-invalid processor packets.
    for key, state in decoded.items():
        phase = key.split("/")[1]; step = phases.index(phase); fresh = step >= phases.index("steel-stock")
        recovered = step >= phases.index("recovered"); released = phase == "released"
        assert state["next_pod"] == 11 and len(state["pods"]) == (8 if recovered else 10)
        assert state["pods"]["0"] == initial["pods"]["0"] and state["pods"]["1"] == initial["pods"]["1"]
        assert all(p["capital"] == 2000 for p in state["pods"].values())
        assert state["provenance"].get("STEL/65535/0/65535", 0) == (10 if fresh else 0)
        assert state["provenance"]["GOOD/65535/0/65535"] == 4
        assert state["provenance"]["STEL/4/0/65535"] == 36 and state["provenance"]["GOOD/4/0/65535"] == 64
        assert state["feeder"]["STEL"] == 360 and state["feeder"]["GOOD"] == 640
        assert state["stations"]["4"]["PROP"] == (38 if step < 3 else 23 if step == 3 else 8)
        steel = state["recipes"][2]; batches = 15 if fresh else 10
        assert steel["batches"] == {"OSTL": batches} and steel["consumed"] == {"NORE": batches * 4}
        assert steel["produced"] == {"STEL": batches * 2} and steel["residue"] == {"OSTL": batches * 32} and steel["paid"] == batches * 55500
        assert state["recipes"][3]["batches"] == {"OGDS": 1} and state["recipes"][3]["produced"] == {"GOOD": 4}
        assert state["modules"]["4"] == initial["modules"]["4"] and state["modules"]["3"].get("RARE", 0) == 19
        assert state["modules"]["2"].get("STEL", 0) == (0 if not fresh else 10 if step < 5 else 8)
        assert state["modules"]["3"].get("GOOD", 0) == (4 if step < 5 else 0)
        assert state["carriers"]["18"]["quantity"] == (2 if 5 <= step <= 6 else 0)
        assert state["carriers"]["19"]["quantity"] == (4 if 5 <= step <= 6 else 0)
        expected_surface = (36, 64) if step < 6 else (2, 4) if phase == "recovered" else (0, 0)
        assert tuple(state["stations"]["4"][label] for label in ("STEL", "GOOD")) == expected_surface
        if step >= 6:
            assert state["hubs"]["4"] == members
            assert sum(state["stations"][str(m)]["STEL"] - initial["stations"][str(m)]["STEL"] for m in historical) == 36
            assert sum(state["stations"][str(m)]["GOOD"] - initial["stations"][str(m)]["GOOD"] for m in historical) == 64
            for member in (rail, lorry):
                assert state["stations"][str(member)] == dict(STEL=1 if phase == "collected" else 0, GOOD=2 if phase == "collected" else 0, PROP=0)
        assert len(state["descents"]) == (2 if released else 0)
        for descent in state["descents"]:
            assert descent["model"] == 20 and descent["remaining"] > 0 and descent["mass"] == 32
            assert (descent["sender"], descent["sender_generation"], descent["label"], descent["quantity"], descent["weight"], descent["pod"], descent["generation"]) in (
                (18, 4, "STEL", 2, 16, 2, 3), (19, 5, "GOOD", 4, 8, 3, 4))
            assert state["pods"][str(descent["pod"])]["custody"] == 5
        assert len(state["handling"]) == (4 if phase == "handling" else 0)
        if phase == "handling":
            assert sorted((j["member"], j["label"], j["quantity"]) for j in state["handling"]) == sorted((m, l, n) for m in members for l, n in (("STEL", 1), ("GOOD", 2)))
            assert all(j["center"] == 4 and j["reverse"] == 0 and j["remaining"] > 0 for j in state["handling"])


def configuration(name, selectors, references):
    """Qualify exact bounded membership, same-phase resaves and genuine resumed next actions.

    @param name Current native build.
    @param selectors Seven explicit original selectors in scope order.
    @param references Checked current direct identity graph.
    @return Frozen independently decoded configuration evidence.
    """
    native, own, resumed, proofs = {}, {}, {}, []; focused = None
    for scope, path in zip(SCOPED["SCOPES"], selectors):
        selector = LEAF["read"](path); AUDIT["inspect"](selector, references)
        assert selector["configuration"] == name and selector["complete"] and selector["scope"] == scope
        operation, phases = SCOPED["SCOPES"][scope]; assert len(selector["reports"]) == len(phases)
        for phase, relative in zip(phases, selector["reports"]):
            proof = qualify(name, ROOT / relative, references); proofs.append(proof)
            if scope == "focused":
                focused = proof; continue
            target = native if scope.startswith("native-") else own if scope.startswith("own-") else resumed
            assert proof["phase"] == phase and proof["scope"] == operation and phase not in target; target[phase] = proof
            parent = native.get(LEAF["PARENTS"].get(phase)) if target is native else native[phase] if target is own else own[LEAF["PARENTS"][phase]]
            if parent:
                assert proof["parent"] == parent["_path"] and proof["source"] == parent["after"]
            else:
                assert phase == "inspected" and proof["parent"] is None and proof["source"] == LEAF["predecessor"](name)[2]
            if target is own:
                AUDIT["compare"](proof["source"], proof["after"])
            elif target is resumed:
                AUDIT["compare"](native[phase]["after"], proof["after"])
    assert len(native) == 11 and len(own) == 10 and len(resumed) == 9 and focused
    assert len(proofs) == len({p["native_reports"][0]["path"] for p in proofs}) == 31
    checkpoints = {operation + "/" + phase: p["after"] for operation, rows in (("native", native), ("own", own), ("resumed", resumed)) for phase, p in rows.items()}
    decoded = {key: DECODE["snapshot"](saved) for key, saved in checkpoints.items()}; material_flow(decoded)
    return dict(configuration=name, selectors=[identity(p) for p in selectors], reports=[p["_path"] for p in proofs], processes=31,
                focused_assertions=focused["assertions"], focused_cases=focused["cases"], native_assertions=sum(p["assertions"] for p in proofs if p is not focused),
                ordinary_own_resaves=10, own_continuations=9, physical_owner_comparisons=570, metadata_comparisons=38,
                checkpoints=checkpoints, decoded_checkpoints=decoded)


def main():
    """Publish both current configurations only after conserved products and byte-exact continuations pass."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path, nargs=7, required=True)
    parser.add_argument("--debug", type=Path, nargs=7, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); target = args.output.resolve(); assert not target.exists(); references = {}
    release = configuration("RelWithDebInfo", [p.resolve() for p in args.release], references)
    debug = configuration("Debug", [p.resolve() for p in args.debug], references)
    assert release["checkpoints"].keys() == debug["checkpoints"].keys()
    for key, saved in release["checkpoints"].items():
        AUDIT["compare"](saved, debug["checkpoints"][key])
    assert release["decoded_checkpoints"] == debug["decoded_checkpoints"]
    freight = ROOT / "logs/processor-freight-verification.json"
    assert identity(freight)["sha256"] == "6b6479c4e5ee1c8ba31b3329253c67fc487bc56036171229e793efb95c5a9fc0"
    result = dict(_documentation=__doc__, save_version=438, configurations=[release, debug], sources=[identity(ROOT / p) for p in SOURCES],
                  direct_references=list(references.values()), dated_producer_indexes=[identity(LEAF["ENTRY"]), identity(freight)],
                  cross_configuration_physical_comparisons=900, cross_configuration_metadata_comparisons=60,
                  native_paid_product_returns=True, native_linked_train_lorry_collection=True, inherited_starting_content_procurement=False)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), identity(target)["sha256"], flush=True)


if __name__ == "__main__":
    main()
