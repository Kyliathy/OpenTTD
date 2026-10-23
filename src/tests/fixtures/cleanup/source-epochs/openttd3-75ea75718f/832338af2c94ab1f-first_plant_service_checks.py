"""Verify actual original first-plant Surface service in isolated native processes.

@file first_plant_service_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group9B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group8FC.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15BBB synchronized connection acceptance.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Every mutation belongs to the real executable and ordinary retained world.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.dont_write_bytecode = True
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from runner import current_run_context
from save_tables import chunks, read_stream, saved_table


def identity(path):
    """Identify one exact original or current report.

    @param path Existing repository-owned artifact.
    @return Relative path, size and SHA256; no artifact is changed.
    """
    path = Path(path).resolve()
    return {"path": path.relative_to(ROOT).as_posix(), "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def complete_chunks(path):
    """Read every complete native owner, including unknown ordinary fields.

    @param path Genuine ordinary native original/resave.
    @return Ordered version/chunk hashes; this creates no runtime owner.
    """
    version, stream = read_stream(Path(path))
    return version, [(row["name"].decode(), hashlib.sha256(row["raw"]).hexdigest()) for row in chunks(stream)]


def native(config, action, source=None, boundary=None, focus=None, stage="generation", town_command=False):
    """Run one focused native case with its own ordinary startup and diagnostics.

    @param config Successfully built selected configuration.
    @param action Native file action, or None for an isolated existing owner boundary.
    @param source Exact retained original file for ordinary afterload.
    @param boundary One actual physical/owner interruption.
    @param focus Existing producer tag, never combined with other isolated cases.
    @param stage Actual native file owner; existing Surface-service callers retain their generation owner by default.
    @param town_command Explicit first-connection command acceptance during actual populated-town preparation.
    @return Actual successful child report and its immutable report identity.
    """
    command = [sys.executable, str(ROOT / "src/tests/multilayer/utilities/first_plant_checks.py"), "--config", config]
    if focus:
        command += ["--stage", "focused", "--focus", focus]
    else:
        command += ["--stage", stage, "--action", action, "--input", str(Path(source).resolve())]
    if boundary:
        command += ["--boundary", boundary]
    if town_command:
        command += ["--town-command"]
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=130, check=False)
    print(result.stdout, end="", flush=True)
    assert result.returncode == 0, result.stdout + result.stderr
    report_path = Path(result.stdout.strip().splitlines()[-1])
    report = json.loads(report_path.read_text())
    assert report["case_runs"] > 0 and report["assertions"] > 0
    return report, identity(report_path)


def saved(report, stage):
    """Resolve an actual native writer checkpoint.

    @param report Successful native child proof.
    @param stage Exact emitted original stage.
    @return Existing absolute repository-owned save path.
    """
    return ROOT / report["saves"][stage]["path"]


def verify_energy(path):
    """Reconcile the real collector, exact conversion, service and useful town use.

    @param path Actual completed original Surface-service file.
    @return Observed native ledgers, with no invented energy/cash/storage account.
    """
    generators = saved_table(path, b"GPGD")
    assert len(generators) == 1
    generator = next(iter(generators.values()))
    rows = list(saved_table(path, b"UPLD").values())
    output = next(row for row in rows if row["type"] == 0 and row["id"] == generator["electrical_producer"])
    heat = next(row for row in rows if row["type"] == 0 and row["domain"] == 1)
    assert output["lifetime_consumed"] == 9990 and output["c"] == 0
    assert heat["lifetime_consumed"] * 15 == output["lifetime_generated"] * 100
    assert generator["active_service_ticks"] * 4050 == output["lifetime_generated"]
    assert generator["service_remainder"] == generator["active_service_ticks"] * 20 % 74
    town = next(row for row in rows if row["type"] == 1 and row["town_terminal"] != 65535)
    assert town["town_bonus"] == 25 and town["town_applied"] == 1 and town["town_suspended"] == 0
    return {"generator": generator, "original_heat": heat, "original_electricity": output, "actual_town_service": town}


def main():
    """Accept one configuration's complete physical-service burst.

    @return None; retain every child identity, original, boundary and exact continuation.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--town-command", action="store_true", help="Exercise the actual synchronized connection before native use and file continuation.")
    args = parser.parse_args()
    context = current_run_context("geothermal", "first-plant-service")
    reports = []
    # This is a sequential controller, not a combined native test process.
    # A failing isolated case stops dependent acceptance immediately.
    def run(action, source=None, boundary=None, focus=None):
        """Retain one successful isolated report in this acceptance burst.

        @param action Actual native action, None for a focused producer boundary.
        @param source Genuine ordinary original, unchanged by the controller.
        @param boundary Optional physical interruption.
        @param focus Optional isolated existing producer tag.
        @return Native successful child evidence.
        """
        report, proof = native(args.config, action, source, boundary, focus,
                               town_command=args.town_command and action == "prepare")
        reports.append(proof)
        return report
    prepared = run("prepare", args.input)
    connected = saved(prepared, "connected")
    operation = run("operate", connected)
    partial, complete = saved(operation, "partial"), saved(operation, "complete")
    # Both repeat loads must preserve every complete owner, including town
    # state and native utility lineage; no whitelist hides a service replay.
    for original in (connected, partial, complete):
        first = run("inspect", original)
        second = run("inspect", saved(first, "inspect"))
        assert complete_chunks(original) == complete_chunks(saved(first, "inspect"))
        assert complete_chunks(original) == complete_chunks(saved(second, "inspect"))
    resumed = run("operate", partial)
    assert complete_chunks(complete) == complete_chunks(saved(resumed, "complete")), "Interrupted native service diverged from its original continuation."
    ledger = verify_energy(complete)
    report = {"_documentation": "r2mlvs12 Group9B; r2mlvs18 Group8FC; original native Surface service, scoped physical/owner acceptance",
              "configuration": args.config, "input": identity(args.input), "controller": identity(__file__),
              "town_command": args.town_command,
              "reports": reports, "connected": identity(connected), "partial": identity(partial), "complete": identity(complete),
              "native_energy": ledger, "continued_complete_chunks_equal": True}
    report_path = context.output("reports", f"first-plant-service-{args.config}.json")
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Surface service accepted in {args.config}: {len(reports)} isolated reports\n{report_path}", flush=True)


if __name__ == "__main__":
    main()
