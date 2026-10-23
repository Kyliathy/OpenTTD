"""Accept real retained-town repair and paid source replacement in isolated native files.

@file first_plant_town_recovery_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15BBC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Every mutation belongs to the ordinary executable and original paid first-plant world.
"""
import argparse
import importlib
import json
from pathlib import Path

from first_plant_service_checks import ROOT, identity, complete_chunks, native, saved, verify_energy
from first_plant_magma_power import verify_native_power
from first_plant_checks import saved_table
from runner import current_run_context


def native_acceptance(config, kind, original, context):
    """Run one bounded real recovery family without substituting a fixture world.

    @param config Successfully built native configuration.
    @param kind Actual terminal loss/rebuild or paid2000 source replacement.
    @param original Immutable genuine partial1980 or complete unbound2000 construction input.
    @param context Current repository-owned report/artifact home.
    @return Exact native child identities, checkpoints and original owner reconciliation.
    """
    original = original.resolve()
    original_identity = identity(original)
    proofs = []
    stage = "generation" if kind == "terminal" else "magma"

    def run(action, source, boundary=None, command=False):
        """Record one successful isolated ordinary native scenario process.

        @param action Exact native action over the supplied retained world.
        @param source Actual immutable prior checkpoint.
        @param boundary Explicit physical interruption, if requested.
        @param command Require actual synchronized connection/recovery marker.
        @return Actual successful native report; no simulation state is synthesized.
        """
        report, proof = native(config, action, source, boundary=boundary, stage=stage, town_command=command)
        proofs.append(proof)
        return report

    if kind == "terminal":
        interrupted = run("boundary", original, boundary="terminal")
        blocked = saved(interrupted, "blocked")
        repaired = run("repair", blocked, boundary="terminal", command=True)
        checkpoints = {"blocked": blocked, "repaired": saved(repaired, "repaired"), "complete": saved(repaired, "complete")}
        energy = verify_energy(checkpoints["complete"])
        before = next(row for row in saved_table(original, b"UPLD").values() if row["type"] == 1 and row["town_terminal"] != 65535)
        after = energy["actual_town_service"]
        assert before["id"] == after["id"] and before["town"] == after["town"]
        continued = run("operate", checkpoints["repaired"])
        assert complete_chunks(checkpoints["complete"]) == complete_chunks(saved(continued, "complete")), "Repaired original diverged after ordinary load."
    else:
        wired = run("wire", original, command=True)
        operation = run("operate", saved(wired, "connected"))
        checkpoints = {"wired": saved(wired, "wired"), "connected": saved(wired, "connected"),
                       "partial": saved(operation, "partial-generation"), "complete": saved(operation, "generation-complete")}
        energy = verify_native_power(checkpoints["complete"])
        before_towns = [row for row in saved_table(original, b"UPLD").values() if row["type"] == 1 and row["town"] != 65535]
        after_towns = [row for row in saved_table(checkpoints["complete"], b"UPLD").values() if row["type"] == 1 and row["town"] != 65535]
        assert len(before_towns) == len(after_towns) == 1
        assert before_towns[0]["id"] == after_towns[0]["id"]
        # Replaying the original wired checkpoint must use the SAME actual
        # atomic command, preserving its one-publication operation sequence.
        connected = run("wire", checkpoints["wired"], command=True)
        assert complete_chunks(checkpoints["connected"]) == complete_chunks(saved(connected, "connected"))
        continued = run("operate", checkpoints["partial"])
        assert complete_chunks(checkpoints["complete"]) == complete_chunks(saved(continued, "generation-complete"))

    # First and repeated ordinary native loads preserve every complete chunk,
    # including settled shares, retired edges, paid stock and native town data.
    for name, checkpoint in checkpoints.items():
        previous = checkpoint
        for iteration in range(2):
            loaded = run("inspect", previous)
            previous = saved(loaded, "inspect")
            assert complete_chunks(checkpoint) == complete_chunks(previous), (kind, name, iteration)
    assert identity(original) == original_identity
    path = context.output("reports", f"town-recovery-{kind}-{config}.json")
    report = {"_documentation": "r2mlvs18 Group15BBC; archi-map-layers.md; actual retained-town recovery, immutable original and exact native continuation",
              "configuration": config, "kind": kind, "controller": identity(__file__), "input": original_identity,
              "reports": proofs, "checkpoints": {name: identity(value) for name, value in checkpoints.items()},
              "native_energy": energy, "original_town_account_retained": True,
              "repeated_loads_and_continuation_all_complete_chunks_equal": True}
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return path


def production_acceptance(config, parent_path, context):
    """Load/resave accepted current recovery worlds with the normal game.

    @param config Successfully built normal-game configuration.
    @param parent_path Exact successful native recovery report.
    @param context Current repository-owned report home.
    @return Retained normal-production evidence path.
    """
    parent = json.loads(parent_path.read_text())
    assert parent["configuration"] == config
    ordinary = importlib.import_module("first_plant_service_checks-2").ordinary
    proofs = []
    # Each actual normal executable gets an isolated config/startup/output;
    # the repeat observes its own resave, never overwriting a native original.
    for name, checkpoint in parent["checkpoints"].items():
        proofs.append(ordinary(config, f"recovery-{parent['kind']}-{name}", ROOT / checkpoint["path"]))
    proofs.append(ordinary(config, f"recovery-{parent['kind']}-repeat", ROOT / proofs[-1]["output"]["path"]))
    path = context.output("reports", f"town-recovery-production-{parent['kind']}-{config}.json")
    path.write_text(json.dumps({"_documentation": "r2mlvs18 Group15BBC; archi-map-layers.md; actual normal-game recovery loading",
                                "configuration": config, "parent": identity(parent_path), "controller": identity(__file__),
                                "ordinary_processes": proofs}, indent=2) + "\n", encoding="utf-8")
    return path


def main():
    """Dispatch one bounded recovery or production burst.

    @return None; an absent/failed native prerequisite refuses acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--phase", choices=("native", "production"), default="native")
    parser.add_argument("--kind", choices=("terminal", "source"), required=True)
    parser.add_argument("--input", type=Path, required=True, help="Original native world, or accepted parent report for production.")
    args = parser.parse_args()
    context = current_run_context("geothermal", "town-recovery")
    if args.phase == "native":
        path = native_acceptance(args.config, args.kind, args.input, context)
    else:
        path = production_acceptance(args.config, args.input, context)
    print(f"Accepted {args.config} {args.kind} {args.phase}\n{path}", flush=True)


if __name__ == "__main__":
    main()
