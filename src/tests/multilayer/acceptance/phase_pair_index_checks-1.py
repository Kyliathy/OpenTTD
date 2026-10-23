"""Bind actual launch/descent/handling phase pairs to both native configurations.

@file phase_pair_index_checks-1.py Read-only comparison of exact saved owners and financial replay witnesses.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group22, explicit current progress and retained history aliases.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group17.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from runner import current_run_context
from artifact_paths import contained, resolve_input_path

EXACT = runpy.run_path(str(Path(__file__).with_name("orbital_core_checks-1.py")))["EXACT"]
CONFIGURATIONS = ("RelWithDebInfo", "Debug")


def history_aliases():
    """Load the declared immutable historical graph without selecting a latest run.

    @return Exact original path/hash to retained path/hash aliases from Group9 preservation.
    """
    path = ROOT / "src/tests/fixtures/space/phase-history/history.json"
    document = json.loads(path.read_text(encoding="utf-8"))
    if document["kind"] != "historical_phase_pairs":
        raise ValueError("phase history requires its retained original graph")
    aliases = {}
    # Each exact old identity has a declared retained owner. An equal basename
    # or a convenient current report cannot replace a missing historical edge.
    for row in document["live_retained_inputs"]:
        original, retained = row["original"], row["retained"]
        if original["sha256"] != retained["sha256"]:
            raise ValueError("phase history alias changed original payload identity")
        aliases[(original["path"], original["sha256"])] = retained
    return aliases


def bound_path(reference, aliases):
    """Resolve one exact report/save identity through its declared retained role.

    @param reference Original path/hash from an accepted report, or explicit current report identity.
    @param aliases Independently retained exact original-to-durable graph edges.
    @return Contained actual payload path with the original SHA256 intact.
    """
    retained = aliases.get((reference["path"], reference["sha256"]))
    if retained is not None:
        path = contained(ROOT / retained["path"], ROOT / "src/tests/fixtures/space/phase-history")
    elif reference in aliases.values():
        path = contained(ROOT / reference["path"], ROOT / "src/tests/fixtures/space/phase-history")
    elif Path(reference["path"]).suffix.lower() in (".sav", ".scn"):
        path = resolve_input_path(reference["path"], ROOT)
    else:
        path = contained(ROOT / reference["path"], ROOT / "mlm/reviews")
    if identity(path)["sha256"] != reference["sha256"]:
        raise ValueError("selected phase report/save identity changed")
    return path


def selected_reference(current, historical, aliases):
    """Select an explicit current progress report or its exact declared historical root.

    @param current Optional operator-selected current generated report path.
    @param historical Exact old root key in the independently retained graph.
    @param aliases Original-to-retained path/hash edges; no directory scan is permitted.
    @return Actual current/retained report identity.
    """
    if current is not None:
        return identity(contained(current.resolve(), ROOT / "mlm/reviews"))
    references = [dict(path=path, sha256=digest) for path, digest in aliases if path == historical]
    if len(references) != 1:
        raise ValueError("selected historical phase root is missing or ambiguous: " + historical)
    return identity(bound_path(references[0], aliases))


def read_bound(reference, aliases):
    """Read a hash-bound original report or saved artifact without inferring a latest run.

    @param reference Exact repository path and immutable SHA256 identity.
    @param aliases Declared retained historical graph; current reports retain their exact paths.
    @return Parsed original report, after validating its actual bytes.
    """
    path = bound_path(reference, aliases)
    return json.loads(path.read_text(encoding="utf-8"))


def compare_owners(first, second, label, aliases):
    """Compare canonical owners from independent native configurations.

    @param first Exact saved identity from the first accepted execution.
    @param second Exact saved identity from the repeated accepted execution.
    @param label Declared scenario/phase for meaningful refusal diagnostics.
    @param aliases Exact retained report/save aliases, preserving original path/hash facts.
    """
    paths = [bound_path(reference, aliases) for reference in (first, second)]
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
    for suffix in ("release", "debug"):
        parser.add_argument("--probe-" + suffix, type=Path)
        parser.add_argument("--descent-" + suffix, type=Path)
        parser.add_argument("--descent-" + suffix + "-extra", type=Path)
    parser.add_argument("--module-index", type=Path)
    args = parser.parse_args()
    aliases = history_aliases()
    reports = []; pairs = []; owners = {}; ledgers = {}
    # The exact progress files name every original paid phase/report; both configurations must repeat the same physical transition and financial outcome.
    for configuration in CONFIGURATIONS:
        suffix = "release" if configuration == "RelWithDebInfo" else "debug"
        progress = selected_reference(getattr(args, "probe_" + suffix), f"logs/paid-probe-phase-{suffix}-progress.json", aliases)
        reports.append(progress); proof = read_bound(progress, aliases); assert proof["configuration"] == configuration
        assert len(proof["steps"]) == 6
        for step in proof["steps"]:
            row = read_bound(step["report"], aliases); assert row["configuration"] == configuration and row["phase"] == step["phase"]
            reports.append(identity(bound_path(step["report"], aliases))); label = "probe/" + row["phase"]
            owners.setdefault(label, {})[configuration] = (row["before"], row["after"]); ledgers.setdefault(label, {})[configuration] = row["normalized_ledger"]
            pairs.append({"kind": "genuine_paid_probe", "configuration": configuration, "phase": row["phase"], "before": row["before"], "after": row["after"]})
        # Reuse actual paid descent/service and replacement/retarget evidence; no previous native suite is rerun for this index comparison.
        for extra in ("", "-extra"):
            progress = selected_reference(getattr(args, "descent_" + suffix + extra.replace("-", "_")), f"logs/descent-custody-{suffix}{extra}-progress.json", aliases)
            reports.append(progress); proof = read_bound(progress, aliases); assert proof["configuration"] == configuration
            for step in proof["steps"]:
                row = read_bound(step["report"], aliases); reports.append(identity(bound_path(step["report"], aliases))); assert row["configuration"] == configuration
                label = "descent/" + row["phase"]; owners.setdefault(label, {})[configuration] = (row["source"], row["complete"])
                pairs.append({"kind": "genuine_paid_descent_service", "configuration": configuration, "phase": row["phase"], "before": row["source"], "after": row["complete"]})
        handling = (args.handling_release if configuration == "RelWithDebInfo" else args.handling_debug).resolve()
        reference = identity(handling); row = read_bound(reference, aliases); reports.append(reference)
        assert row["configuration"] == configuration and row["fixture_kind"] == "seeded_packet_handling_boundary"
        owners.setdefault("handling/completion", {})[configuration] = (row["before"], row["after"])
        pairs.append({"kind": row["fixture_kind"], "configuration": configuration, "phase": "completion", "before": row["before"], "after": row["after"]})
    module_proof = selected_reference(args.module_index, "logs/module-flight-verification.json", aliases)
    reports.append(module_proof); module = read_bound(module_proof, aliases)
    prior = read_bound(module["prior_positive_release"], aliases); reports.append(identity(bound_path(module["prior_positive_release"], aliases)))
    points = {configuration: {} for configuration in CONFIGURATIONS}
    # Only the hash-bound original positive reports supply module phases; failure reports and historical source-code snapshots do not become new successful gameplay producers.
    for reference in [*prior["reports"], *module["reports"]]:
        name = Path(reference["path"]).name
        if name not in ("orbital-module-flight-prepare.json", "orbital-module-flight-complete.json"):
            continue
        row = read_bound(reference, aliases); reports.append(identity(bound_path(reference, aliases))); target = points[row["configuration"]]
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
            compare_owners(configurations[CONFIGURATIONS[0]][edge], configurations[CONFIGURATIONS[1]][edge], (label, edge), aliases)
    for label, configurations in ledgers.items():
        assert configurations[CONFIGURATIONS[0]] == configurations[CONFIGURATIONS[1]], label
    # Publish actual current/retained save owners in the new index. Original
    # paths/hashes remain in the immutable reports reached through the alias graph.
    for pair in pairs:
        for edge in ("before", "after"):
            pair[edge] = identity(bound_path(pair[edge], aliases))
    report = current_run_context("space", "phase-pair-index").reports / "phase-pair-index.json"
    report.write_text(json.dumps({"_documentation": "Owner: r2mlvs9 Group17; original paid probe, module and finite-cargo descent/service phase pairs with explicit seeded handling characterization.26 canonical owners and normalized probe ledgers agree across independent configurations; original ordinary final acceptance/payment remains separate.",
        "save_version": 423, "reports": reports, "pairs": pairs, "pair_scenarios": len(owners), "pairs_per_configuration": len(owners), "exact_owner_families": len(EXACT),
        "normalized_probe_ledgers": ledgers, "no_cargo_income": True}, indent=2) + "\n", encoding="utf-8")
    print(f"Phase pair index: {report.relative_to(ROOT)} ({len(owners)} pairs per configuration)", flush=True)


if __name__ == "__main__":
    main()
