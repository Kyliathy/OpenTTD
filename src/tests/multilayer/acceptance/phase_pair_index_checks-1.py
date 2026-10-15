"""Bind actual launch/descent/handling phase pairs to both native configurations.

@file phase_pair_index_checks-1.py Read-only comparison of exact saved owners and financial replay witnesses.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group17.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from runner import current_run_context

EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"]
CONFIGURATIONS = ("RelWithDebInfo", "Debug")


def read_bound(reference):
    """Read a hash-bound original report or saved artifact without inferring a latest run.

    @param reference Exact repository path and immutable SHA256 identity.
    @return Parsed original report, after validating its actual bytes.
    """
    path = ROOT / reference["path"]
    assert identity(path) == reference, path
    return json.loads(path.read_text(encoding="utf-8"))


def compare_owners(first, second, label):
    """Compare canonical owners from independent native configurations.

    @param first Exact saved identity from the first accepted execution.
    @param second Exact saved identity from the repeated accepted execution.
    @param label Declared scenario/phase for meaningful refusal diagnostics.
    """
    paths = [ROOT / reference["path"] for reference in (first, second)]
    assert all(identity(path) == reference for path, reference in zip(paths, (first, second))), label
    left_version, left = chunks(paths[0]); right_version, right = chunks(paths[1])
    assert left_version == right_version, label
    if left_version != 423:
        # Historical genuine inputs retain their original schema/identity; only current native outputs claim the complete26-family contract.
        assert first["sha256"] == second["sha256"], label
        return
    for family in EXACT:
        assert left[family] == right[family], (label, family)


def main():
    """Compare only declared accepted phase manifests, including canonical company ledgers and just-completed delivery receipts."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--handling-release", type=Path, required=True)
    parser.add_argument("--handling-debug", type=Path, required=True)
    args = parser.parse_args()
    reports = []; pairs = []; owners = {}; ledgers = {}
    # The exact progress files name every original paid phase/report; both configurations must repeat the same physical transition and financial outcome.
    for configuration in CONFIGURATIONS:
        suffix = "release" if configuration == "RelWithDebInfo" else "debug"
        progress = ROOT / f"logs/paid-probe-phase-{suffix}-progress.json"
        reports.append(identity(progress)); proof = json.loads(progress.read_text(encoding="utf-8")); assert proof["configuration"] == configuration
        assert len(proof["steps"]) == 6
        for step in proof["steps"]:
            row = read_bound(step["report"]); assert row["configuration"] == configuration and row["phase"] == step["phase"]
            reports.append(step["report"]); label = "probe/" + row["phase"]
            owners.setdefault(label, {})[configuration] = (row["before"], row["after"]); ledgers.setdefault(label, {})[configuration] = row["normalized_ledger"]
            pairs.append({"kind": "genuine_paid_probe", "configuration": configuration, "phase": row["phase"], "before": row["before"], "after": row["after"]})
        # Reuse actual paid descent/service and replacement/retarget evidence; no previous native suite is rerun for this index comparison.
        for extra in ("", "-extra"):
            progress = ROOT / f"logs/descent-custody-{suffix}{extra}-progress.json"
            reports.append(identity(progress)); proof = json.loads(progress.read_text(encoding="utf-8")); assert proof["configuration"] == configuration
            for step in proof["steps"]:
                row = read_bound(step["report"]); reports.append(step["report"]); assert row["configuration"] == configuration
                label = "descent/" + row["phase"]; owners.setdefault(label, {})[configuration] = (row["source"], row["complete"])
                pairs.append({"kind": "genuine_paid_descent_service", "configuration": configuration, "phase": row["phase"], "before": row["source"], "after": row["complete"]})
        handling = (args.handling_release if configuration == "RelWithDebInfo" else args.handling_debug).resolve()
        reference = identity(handling); row = read_bound(reference); reports.append(reference)
        assert row["configuration"] == configuration and row["fixture_kind"] == "seeded_packet_handling_boundary"
        owners.setdefault("handling/completion", {})[configuration] = (row["before"], row["after"])
        pairs.append({"kind": row["fixture_kind"], "configuration": configuration, "phase": "completion", "before": row["before"], "after": row["after"]})
    module_proof = ROOT / "logs/module-flight-verification.json"; reports.append(identity(module_proof)); module = json.loads(module_proof.read_text())
    prior = read_bound(module["prior_positive_release"]); reports.append(module["prior_positive_release"])
    points = {configuration: {} for configuration in CONFIGURATIONS}
    # Only the hash-bound original positive reports supply module phases; failure reports and historical source-code snapshots do not become new successful gameplay producers.
    for reference in [*prior["reports"], *module["reports"]]:
        name = Path(reference["path"]).name
        if name not in ("orbital-module-flight-prepare.json", "orbital-module-flight-complete.json"):
            continue
        row = read_bound(reference); reports.append(reference); target = points[row["configuration"]]
        fields = ("preburn", "ascent", "delivery") if name.endswith("prepare.json") else ("approach", "unloaded", "turnaround", "complete")
        for phase in fields:
            assert phase not in target, phase
            target[phase] = row[phase]
    order = ("preburn", "ascent", "delivery", "approach", "unloaded", "turnaround", "complete")
    for configuration in CONFIGURATIONS:
        assert set(points[configuration]) == set(order)
        for first, second in zip(order, order[1:]):
            label = "module/" + first + "->" + second; before, after = points[configuration][first], points[configuration][second]
            owners.setdefault(label, {})[configuration] = (before, after)
            pairs.append({"kind": "genuine_paid_module_freight", "configuration": configuration, "phase": label, "before": before, "after": after})
    # Both sides of every actual pair include financial, inventory, packet-age/provenance, reservation and exact-generation owners; paths are excluded from equality.
    for label, configurations in owners.items():
        assert set(configurations) == set(CONFIGURATIONS), label
        for edge in (0, 1):
            compare_owners(configurations[CONFIGURATIONS[0]][edge], configurations[CONFIGURATIONS[1]][edge], (label, edge))
    for label, configurations in ledgers.items():
        assert configurations[CONFIGURATIONS[0]] == configurations[CONFIGURATIONS[1]], label
    report = current_run_context("space", "phase-pair-index").reports / "phase-pair-index.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group17; original paid probe, module and finite-cargo descent/service phase pairs with explicit seeded handling characterization.26 canonical owners and normalized probe ledgers agree across independent configurations; original ordinary final acceptance/payment remains separate.",
        "save_version": 423, "reports": reports, "pairs": pairs, "pair_scenarios": len(owners), "pairs_per_configuration": len(owners), "exact_owner_families": len(EXACT),
        "normalized_probe_ledgers": ledgers, "no_cargo_income": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Phase pair index: {report.relative_to(ROOT)} ({len(owners)} pairs per configuration)", flush=True)


if __name__ == "__main__":
    main()
