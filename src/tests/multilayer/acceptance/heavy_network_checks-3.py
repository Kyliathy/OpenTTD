"""Freeze actual heavy local/native-server twins, own-resave next actions and independent admission refusals.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9DC
Earlier producers are exact dated report/save boundaries. Current sources, executable bytes, terminal logs and every physical owner are audited separately.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity

SHARED = runpy.run_path(str(Path(__file__).with_name("heavy_network_checks-1.py")))
REJECTS = runpy.run_path(str(Path(__file__).with_name("heavy_network_checks-2.py")))
CYCLE = runpy.run_path(str(Path(__file__).with_name("heavy_cycle_checks-2.py")))
ROLE = runpy.run_path(str(Path(__file__).with_name("heavy_role_checks-2.py")))
AUDIT = CYCLE["AUDIT"]; PHYSICAL = SHARED["PHYSICAL"]
SCOPES = ("focused", "native-a", "native-b", "reload-mining", "reload-service", "reload-descent", "reload-orbital", "rejects")
SOURCES = (*CYCLE["SOURCES"], "src/tests/heavy_network_native.cpp", "src/tests/heavy_joined_native.cpp", "src/tests/heavy_role_native.cpp",
           "src/tests/multilayer/acceptance/heavy_network_checks-1.py", "src/tests/multilayer/acceptance/heavy_network_checks-2.py", "src/tests/multilayer/acceptance/heavy_network_checks-3.py", "src/tests/multilayer/acceptance/heavy_network_checks-4.py",
           "src/tests/multilayer/acceptance/heavy_role_checks-1.py", "src/tests/multilayer/acceptance/heavy_role_checks-2.py",
           "src/network/network_command.cpp", "src/command_func.h", "src/command.cpp", "src/command_type.h")


def qualify(saved, case, stage):
    """Independently qualify physical packets, immutable role bills, work, allowance and original receiver stock.

    @param saved Exact accepted native/ordinary/own-resave identity.
    @param case Original mining, contained service, Surface descent or Core return.
    @param stage Initial command or completed actual native next action.
    @return Parsed original physical state for exact continuation/twin comparison.
    """
    if case == "mining":
        return ROLE["qualify"](saved, "approach" if stage == "initial" else "partial")
    phases = {"service": ("resumed", "serviced"), "descent": ("descent", "surface-recovered"), "orbital": ("orbital-blocked", "orbital-returned")}
    return CYCLE["qualify"](saved, phases[case][stage == "complete"])


def process_evidence(native, configuration, references):
    """Bind each actual current process to unchanged executable bytes and its retained terminal log.

    @param native Original native invocation report.
    @param configuration Matching current product/test configuration.
    @param references Shared exact direct artifact identities.
    @return Actual terminal identity, including ordinary console byte equivalence where applicable.
    """
    if Path(native["executable"]["path"]).name == "openttd_console.exe":
        return CYCLE["JOINED_AUDIT"]["process_evidence"](native, configuration, references)
    assert ROOT / native["executable"]["path"] == ROOT / "build" / configuration / "openttd_test.exe"; scenario = native["scenario"]
    filename = "focused.log" if scenario == "heavy-cycle-focused" else "heavy-joined.log" if scenario.startswith(("heavy-network-full-", "heavy-network-preview-")) else "heavy-network.log"
    result = identity(ROOT / "logs/space" / scenario / native["run_id"] / filename); AUDIT["inspect_references"](result, references); return result


def configuration(name, selectors, references):
    """Audit complete exact local/server membership and every actual own-resave next result.

    @param name Matching current executable and original input configuration.
    @param selectors Eight explicit completed acceptance scopes in task order.
    @param references Shared retained artifact identities; historical graphs stop at dated boundaries.
    @return Qualified configuration and all actual states for cross-build and local/server comparison.
    """
    selectors = [p.resolve() for p in selectors]; paths = {}; proofs = {}; snapshots = {}; states = {}; processes = {}; reports = []; refused = set()
    totals = dict(focused_assertions=0, focused_cases=0, native_assertions=0, resumed_assertions=0, preview_assertions=0)
    # Explicit scopes exclude smoke/failed selectors and duplicated cases; every native or malformed process keeps its original terminal and executable membership.
    for scope, selector in zip(SCOPES, selectors, strict=True):
        selected = SHARED["read"](selector); assert selected["configuration"] == name and selected["complete"] and selected["scope"] == scope
        assert len(selected["reports"]) == (1 if scope == "focused" else 2 if scope.startswith("reload-") else 4)
        AUDIT["inspect_references"](selected, references)
        for member in selected["reports"]:
            path = ROOT / member; proof = SHARED["read"](path); assert proof["configuration"] == name; reports.append(identity(path)); AUDIT["inspect_references"](proof, references)
            for child in proof["native_reports"]:
                assert child["path"] not in processes; native = SHARED["read"](ROOT / child["path"]); AUDIT["inspect_references"](native, references)
                expected_exit = 1 if native["scenario"].startswith("heavy-network-full-") else 0; assert native["configuration"] == name and native["exit_status"] == expected_exit
                processes[child["path"]] = dict(report=child, terminal=process_evidence(native, name, references), intended_full_refusal=bool(expected_exit))
            if scope == "focused":
                totals["focused_assertions"] = proof["assertions"]; totals["focused_cases"] = proof["cases"]; assert proof["cases"] == 20
            elif scope.startswith("native-"):
                case, mode = proof["case"], proof["mode"]; key = case, mode; assert key not in paths and proof["procurement_acceptance"] is False
                index, predecessor, source = SHARED["boundary"](name, case); assert (proof["entry_index"], proof["predecessor"], proof["source"]) == (index, predecessor, source)
                paths[key] = identity(path); proofs[key] = proof; totals["native_assertions"] += proof["assertions"]
                # Both the command checkpoint and uninterrupted result must independently retain their actual original physical owners; expected older results are bound only by their exact dated report/save membership.
                for stage, field in (("initial", "after"), ("complete", "complete")):
                    states[stage, case, mode] = proof[field]; snapshots[stage, case, mode] = qualify(proof[field], case, stage)
                    if (case, stage) in SHARED["EXPECTED"]:
                        _, report, saved = SHARED["boundary"](name, case, SHARED["EXPECTED"][case, stage]); assert proof["expected"][stage] == dict(proof=report, save=saved); SHARED["compare"](saved, proof[field], (name, case, mode, stage))
            elif scope.startswith("reload-"):
                case, mode = proof["case"], proof["mode"]; key = case, mode; original = proofs[key]
                assert proof["proof"] == paths[key] and proof["source"] == original["after"] and proof["expected"] == original["complete"]
                assert qualify(proof["ordinary"], case, "initial") == snapshots["initial", case, mode] and qualify(proof["resumed"], case, "complete") == snapshots["complete", case, mode]
                SHARED["compare"](proof["source"], proof["ordinary"], (name, key, "ordinary")); SHARED["compare"](proof["expected"], proof["resumed"], (name, key, "resumed"))
                states["ordinary", case, mode] = proof["ordinary"]; states["resumed", case, mode] = proof["resumed"]; totals["resumed_assertions"] += proof["assertions"]
            else:
                kind = proof["kind"]; assert kind not in refused; refused.add(kind); case, field = REJECTS["CASES"][kind]
                assert proof["proof"] == paths[case, "local"] and proof["source"] == proofs[case, "local"][field] and proof["preview_reason"]; totals["preview_assertions"] += proof["assertions"]
    assert set(paths) == {(case, mode) for case in SHARED["CASES"] for mode in SHARED["MODES"]} and refused == set(REJECTS["CASES"])
    assert len(states) == 32 and len(processes) == 41 and sum(p["intended_full_refusal"] for p in processes.values()) == 4; twins = 0
    # For each actual command, uninterrupted result, ordinary copy and own-resave action, local and native-server bytes must converge across every physical owner and separate CDIM metadata.
    for stage in ("initial", "complete", "ordinary", "resumed"):
        for case in SHARED["CASES"]:
            SHARED["compare"](states[stage, case, "local"], states[stage, case, "network"], (name, stage, case, "local/server")); twins += len(PHYSICAL)
    return dict(configuration=name, save_version=437, selectors=[identity(p) for p in selectors], reports=reports, processes=list(processes.values()), ordinary_checkpoints=8, own_resave_actions=8,
                independent_full_copied_pairs=4, local_network_comparisons=twins, local_network_metadata_comparisons=16, **totals), states


def dated_boundaries():
    """Freeze every completed heavy producer's exact dated index without claiming its overwritten executable graph is current.

    @return Immutable producer index identities through original role, paid supply/descent/service, joined validation and actual cycle.
    """
    members = dict(SHARED["CYCLE"]["JOINED"]["INDEXES"]); members.update(SHARED["INDEXES"])
    members.update(tuning=("heavy-mining-tuning", "5b81340501efe115b5d5963dd01f1ac6e3034c59b53ad7ab80414edba883edd7"), commissioning=("heavy-commissioning", "bbe592959ba830cbfd24ba3c84f4f0d5b78743c4defd64c6d498c0e5ae42ffde"),
                   joined=("heavy-joined", "255a3cf940bc341a0145e1b0283cd12d56f298f09be3e1bf4288c4a7be5d3dfe"))
    result = {}
    # Dated index content is immutable evidence; current source/binary checks apply only to this consumer's actual processes and sources.
    for name, (stem, digest) in members.items():
        selected = identity(ROOT / ("logs/" + stem + "-verification.json")); assert selected["sha256"] == digest; result[name] = selected
    return result


def main():
    """Publish the heavy aggregate only after both configurations, local/server twins and actual independent readers agree."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--release", nargs=8, type=Path, required=True); parser.add_argument("--debug", nargs=8, type=Path, required=True); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); target = args.output.resolve(); assert not target.exists(); references = {}; release, left = configuration("RelWithDebInfo", args.release, references); debug, right = configuration("Debug", args.debug, references)
    assert left.keys() == right.keys(); comparisons = 0
    # Actual paid inputs must converge across native builds as well as dispatch modes; CDIM remains configured metadata rather than an additional physical owner.
    for key in left:
        SHARED["compare"](left[key], right[key], key); comparisons += len(PHYSICAL)
    result = dict(_documentation="Owner: r2mlvs11-orbital-stations-and-manufacturing.md Group9DC/original9.1; original paid heavy mining, service, last-shell Surface descent and podless precise Core return through actual native server serialization/frame admission/PostFromNet. Own-resave and independent full/copied malformed role/content/progress/owner proofs pass. Useful processor receivers/outputs remain10-12; inherited starting-content procurement and later artwork are unverified.",
                  configurations=[release, debug], dated_producer_boundaries=dated_boundaries(), sources=[identity(ROOT / name) for name in dict.fromkeys(SOURCES)], direct_references=list(references.values()),
                  exact_owner_families=[f.decode("ascii") for f in PHYSICAL], cross_configuration_comparisons=comparisons, configured_metadata_comparisons=len(left), procurement_acceptance=False)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(target.relative_to(ROOT).as_posix(), identity(target)["sha256"], comparisons, flush=True)


if __name__ == "__main__":
    main()
