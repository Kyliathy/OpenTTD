"""Qualify coordinated paid construction, processing, local assembly and product command replay.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md task8C.4
Dated native producers remain separate accepted scenarios. Current paid-world twins prove actual product dispatch and conserved next actions.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity, read_stream
from launch_center_checks import riff_location

HERE = Path(__file__).parent
RUN = runpy.run_path(str(HERE / "probe_product_checks-2.py"))
RETURNS = runpy.run_path(str(HERE / "processor_returns_checks-4.py"))
AUDIT = RUN["AUDIT"]
DATED = {"probe-product-contract": "c637da7e4728965c5240af99bf2e026dec789c87616dddddb2f85a76be54d2e2",
         "processor-returns": RUN["ENTRY_SHA"],
         "probe-role-commissioning": "76b24987f597a158599ea8609f6d3fbe6f30b40805e172471cb08b8ea821f6b9",
         "orbital-core": "d901a9cb257411e01a8093a68e7b33a34dfa7fd3f09330efa30a1383051964e6",
         "orbital-assembly": "9b8665ff4f0ebaa0e80457811729d72a3320d5cd1c906602cb7a917948cb9327",
         "assembly-facility": "87994a5020094d09eb0cb5e7b763487edd54cd22cd009c17206e53f8cad20a8d",
         "manufacturing-reservation": "44aa6aa8862405f8df0ee32b4a08a6d59d9392c3288f82f0c9922a593a913a33",
         "manufacturing-materialization": "a52bdfb62a387d6a351256e8704273cc76cb70f51a09c73a2fdefcbde0a0e557"}
SOURCES = tuple(dict.fromkeys((*RETURNS["SOURCES"], "src/tests/probe_product_replay_native.cpp", "src/tests/launch_production.cpp",
                             "src/tests/space_content.cpp", "src/tests/manufacturing_purchase.cpp", "src/tests/manufacturing_reservation.cpp",
                             "src/saveload/space_production_sl.cpp", "src/network/network_command.cpp",
                             *(f"src/tests/multilayer/acceptance/probe_product_checks-{n}.py" for n in (1, 2, 3)))))


def bind(reference, references):
    """Verify one original dated report/save without reopening its historical executable graph.

    @param reference Exact accepted direct identity.
    @param references Current checked direct reference set.
    @return Original path for read-only semantic inspection.
    """
    path = ROOT / reference["path"]; assert identity(path) == reference
    assert references.get(reference["path"], reference) == reference; references[reference["path"]] = reference
    return path


def dated_scenarios(name, indexes, references):
    """Bind actual Earth-only bootstrap, interrupted construction and later paid local probe assembly.

    @param name Matching producer configuration.
    @param indexes Independently pinned immutable producer indexes.
    @param references Checked direct identities, excluding recursive historical binaries/sources.
    @return Original report/save memberships and independently decoded local factory receipt.
    """
    reports = []; commissioning = indexes["probe-role-commissioning"]
    original = []
    for reference in commissioning["reports"]:
        proof = json.loads(bind(reference, references).read_text(encoding="utf-8"))
        if proof["configuration"] == name:
            assert proof["completed_task"] == "8B.1" and proof["cases"] == 8 and proof["assertions"] == 980
            original.append(reference)
            for milestone in proof["paid_milestones"]:
                bind(milestone, references)
    assert len(original) == 1; reports.extend(original)
    bootstrap = indexes["orbital-assembly"]["reports"][name]["bootstrap"]
    proof = json.loads(bind(bootstrap, references).read_text(encoding="utf-8")); reports.append(bootstrap)
    assert proof["configuration"] == name and proof["no_station_utility_prerequisite"] and proof["duplicate_public_work_refused"] and proof["one_work_tick_for_two_builders"]
    for key in ("input", "receipt", "frame"):
        bind(proof[key], references)
    core, = [r for r in indexes["orbital-core"]["configurations"][name]["reports"] if Path(r["path"]).name == "orbital-core-verification.json"]
    proof = json.loads(bind(core, references).read_text(encoding="utf-8")); reports.append(core)
    bind(proof["input"], references); bind(proof["completed"], references)
    # Real activation and saved next actions retain the same actual completed frame; no newer native load is allowed to supply a free first station.
    for checkpoint in proof["checkpoints"]:
        bind(checkpoint["checkpoint"], references); bind(checkpoint["continued"], references)
    materialization, = [c for c in indexes["manufacturing-materialization"]["configurations"] if c["configuration"] == name]
    complete = None
    for phase in ("halfway", "before", "complete"):
        reference, = [r for r in materialization["reports"] if f"/manufacturing-materialization-verify-{phase}-15/" in r["path"]]
        proof = json.loads(bind(reference, references).read_text(encoding="utf-8")); reports.append(reference)
        assert proof["configuration"] == name and proof["model"] == 15 and proof["phase"] == phase
        bind(proof["source"], references); bind(proof["after"], references)
        if phase == "complete":
            complete = proof["after"]
    version, raw = read_stream(ROOT / complete["path"]); assert version == 427
    _, start, _, end = riff_location(raw, b"SVST"); assert start + 1 + raw[start] * 67 == end; local = []
    # Original427 has fixed67-byte paid receipts, before replacement transport existed. Only model15 with a positive original factory incarnation/serial proves local commissioning rather than a carried Earth hull.
    for p in range(start + 1, end, 67):
        if int.from_bytes(raw[p + 41:p + 43], "big") == 15 and int.from_bytes(raw[p + 51:p + 59], "big") != 0:
            assert int.from_bytes(raw[p + 43:p + 47], "big") == 100000 and raw[p + 48] == 1 and raw[p + 37:p + 39] == bytes(2)
            local.append(dict(vehicle=int.from_bytes(raw[p:p + 4], "big"), generation=int.from_bytes(raw[p + 4:p + 12], "big"),
                              factory=int.from_bytes(raw[p + 49:p + 51], "big"), factory_generation=int.from_bytes(raw[p + 51:p + 59], "big"),
                              serial=int.from_bytes(raw[p + 59:p + 67], "big"), capital=100000))
    receipt, = local; assert receipt["serial"] > 0
    return dict(reports=reports, local_probe_output=complete, local_probe_receipt=receipt, inherited_starting_content_procurement=False)


def configuration(name, selectors, indexes, references):
    """Prove exact current command twins and retain original interrupted scenario producers.

    @param name Current native executable configuration.
    @param selectors Three exact bounded native-a/native-b/focused selectors.
    @param indexes Immutable pinned original scenario indexes.
    @param references Checked direct identities for this complete acceptance.
    @return Qualified current processes and retained original scenario membership.
    """
    proofs = []; native = {}; focused = None
    original, = [c for c in indexes["processor-returns"]["configurations"] if c["configuration"] == name]
    for scope, path in zip(RUN["SCOPES"], selectors):
        selector = json.loads(path.read_text(encoding="utf-8")); AUDIT["inspect"](selector, references)
        assert selector["complete"] and selector["configuration"] == name and selector["scope"] == scope
        for relative in selector["reports"]:
            path = ROOT / relative; proof = json.loads(path.read_text(encoding="utf-8")); AUDIT["inspect"](proof, references)
            assert proof["complete"] and proof["configuration"] == name and not proof["added_stock"] and len(proof["native_reports"]) == 1
            process = json.loads((ROOT / proof["native_reports"][0]["path"]).read_text(encoding="utf-8")); AUDIT["inspect"](process, references)
            assert process["exit_status"] == 0 and process["executable"] == identity(ROOT / "build" / name / "openttd_test.exe")
            assert process["argv"][1:] == [RUN["TAGS"] if scope == "focused" else "[.probe-product-replay-native]"]
            terminal = (ROOT / proof["terminal"]["path"]).read_text(encoding="utf-8", errors="replace")
            assert f"All tests passed ({proof['assertions']} assertions in {proof['cases']} test case" in terminal
            proof["_path"] = identity(path); proofs.append(proof)
            if scope == "focused":
                focused = proof; continue
            action = proof["action"]; assert action in RUN["SCOPES"][scope]
            assert proof["source"] == original["checkpoints"]["native/" + RUN["ACTIONS"][action]] and proof["source"] in process["inputs"] and proof["after"] in process["artifacts"]
            assert chunks(ROOT / proof["after"]["path"])[0] == 438
            key = action + ("/network" if proof["network"] else "/local"); assert key not in native; native[key] = proof
    assert len(proofs) == len({p["native_reports"][0]["path"] for p in proofs}) == 9 and len(native) == 8 and focused
    for action in RUN["ACTIONS"]:
        AUDIT["compare"](native[action + "/local"]["after"], native[action + "/network"]["after"])
        if action in ("processing", "release", "handling"):
            expected = {"processing": "steel-stock", "release": "released", "handling": "handling"}[action]
            AUDIT["compare"](native[action + "/local"]["after"], original["checkpoints"]["native/" + expected])
        else:
            state = RETURNS["DECODE"]["snapshot"](native[action + "/local"]["after"])
            assert state["modules"]["2"]["STEL"] == 10 and state["modules"]["3"]["GOOD"] == 4
            assert all(c["quantity"] == 0 for c in state["carriers"].values())
    return dict(configuration=name, selectors=[identity(p) for p in selectors], reports=[p["_path"] for p in proofs], processes=9,
                focused_assertions=focused["assertions"], focused_cases=focused["cases"], native_assertions=sum(p["assertions"] for p in proofs if p is not focused),
                local_network_physical_comparisons=120, local_network_metadata_comparisons=8,
                original_next_action_physical_comparisons=90, original_next_action_metadata_comparisons=6,
                checkpoints={k: p["after"] for k, p in native.items()}, dated_scenarios=dated_scenarios(name, indexes, references))


def main():
    """Publish task8C.4 only after both current executable twins and exact prior scenario owners qualify."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path, nargs=3, required=True); parser.add_argument("--debug", type=Path, nargs=3, required=True)
    parser.add_argument("--output", type=Path, required=True); args = parser.parse_args(); assert not args.output.exists(); references = {}; indexes = {}; dated = []
    for name, digest in DATED.items():
        reference = identity(ROOT / "logs" / (name + "-verification.json")); assert reference["sha256"] == digest
        indexes[name] = json.loads(bind(reference, references).read_text(encoding="utf-8")); dated.append(reference)
    release = configuration("RelWithDebInfo", [p.resolve() for p in args.release], indexes, references)
    debug = configuration("Debug", [p.resolve() for p in args.debug], indexes, references)
    for key, saved in release["checkpoints"].items():
        AUDIT["compare"](saved, debug["checkpoints"][key])
    assert release["dated_scenarios"]["local_probe_receipt"] == debug["dated_scenarios"]["local_probe_receipt"]
    args.output.write_text(json.dumps(dict(_documentation=__doc__, completed_tasks=["8C.4"], save_version=438, configurations=[release, debug],
                                          sources=[identity(ROOT / p) for p in SOURCES], direct_references=list(references.values()), dated_producer_indexes=dated,
                                          cross_configuration_physical_comparisons=240, cross_configuration_metadata_comparisons=16,
                                          actual_native_command_dispatch=True, multiplayer_socket_integration=False,
                                          inherited_starting_content_procurement=False), indent=2) + "\n", encoding="utf-8")
    print(identity(args.output.resolve()), flush=True)


if __name__ == "__main__":
    main()
