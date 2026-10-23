"""Verify selected real geothermal checkpoints in bounded ordinary native processes.

@file geothermal_selected_save_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group17B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Inputs are exact retained paid-world originals. Independent replay is explicitly
different from the still-required real host/client join.
"""

import argparse
import importlib
import json
from pathlib import Path

from first_plant_service_checks import ROOT, identity, complete_chunks, native, saved
from first_plant_checks import verify_original, saved_table
from runner import current_run_context


def compare_native_load(source, output, startup_linkgraph=False):
    """Validate every raw owner, with explicit field-checked old-format initialization.

    @param source Exact original or previous real native resave.
    @param output Actual isolated ordinary loader output.
    @param startup_linkgraph Permit only the field-checked normal startup CargoDist wait bit.
    @return Explicit migration/initialization classification and complete chunk count.
    """
    _, _, initialized = verify_original(source, output, startup_linkgraph=startup_linkgraph)
    old_version, before = complete_chunks(source)
    new_version, after = complete_chunks(output)
    before, after = dict(before), dict(after)
    assert before.keys() == after.keys(), "Native loading added or removed a complete owner family"
    exceptions = set()
    # Genuine430 absence gains only the three native431 town references.
    # The shared validator above checks every old field and each absent default.
    if old_version == 430 and new_version == 431:
        exceptions.add("UPLD")
    created = set(saved_table(output, b"PLYR")) - set(saved_table(source, b"PLYR"))
    # Only actual ordinary company initialization admits face/RNG changes;
    # verify_original validates all money/identity/clock fields individually.
    if initialized or created:
        exceptions.update(("PLYR", "DATE"))
    if startup_linkgraph:
        old_pause = saved_table(source, b"DATE")[0]["pause_mode"]
        new_pause = saved_table(output, b"DATE")[0]["pause_mode"]
        if new_pause == (old_pause | 64):
            exceptions.add("DATE")
    changed = {key for key in before if before[key] != after[key]}
    assert changed <= exceptions, sorted(changed - exceptions)
    return {"source_version": old_version, "loaded_version": new_version,
            "complete_chunks": len(before), "changed_complete_chunks": sorted(changed),
            "field_checked_format_migration": old_version != new_version,
            "native_manager_initialization": initialized, "native_new_company": sorted(created),
            "all_other_complete_chunks_exact": True}


def main():
    """Execute one selected checkpoint or one model replay, keeping each burst bounded.

    @return None; missing originals, native failures or any unexplained owner difference abort acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--phase", choices=("reload", "replay", "production"), required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--case", required=True)
    args = parser.parse_args()
    manifest = json.loads(args.manifest.read_text())
    case = manifest["cases"][args.case]
    original = ROOT / case["input"]["path"]
    assert identity(original) == case["input"], "A retained paid-world original changed"
    context = current_run_context("geothermal", "selected-saves")
    proofs, outputs, classifications = [], {}, []

    def run(action, source):
        """Retain one actual ordinary native child and its invocation-owned checkpoints.

        @param action Existing native inspect/operate/view action.
        @param source Original or real preceding child checkpoint.
        @return Successful original native report; no test-owned world state is injected.
        """
        result, proof = native(args.config, action, source, stage=case["stage"])
        proofs.append(proof)
        return result

    if args.phase == "reload":
        first = saved(run("inspect", original), "inspect")
        classifications.append(compare_native_load(original, first))
        second = saved(run("inspect", first), "inspect")
        assert complete_chunks(first) == complete_chunks(second), "Second native load changed a complete owner"
        outputs.update(first=identity(first), second=identity(second))
    elif args.phase == "replay":
        assert "partial_stage" in case, "Only a real connected operating model has a replay boundary"
        baseline = run("operate", original)
        visible = run("view", original)
        # Compare every complete owner at the same two actual useful-service
        # points; camera selection never provides a grant or advances a tick.
        for name in (case["partial_stage"], case["complete_stage"]):
            normal, viewed = saved(baseline, name), saved(visible, name)
            assert complete_chunks(normal) == complete_chunks(viewed), (args.case, name, "view")
            outputs[name] = identity(normal)
            outputs[name + "-view"] = identity(viewed)
        partial = saved(baseline, case["partial_stage"])
        continued = saved(run("operate", partial), case["complete_stage"])
        assert complete_chunks(continued) == complete_chunks(saved(baseline, case["complete_stage"])), "Interrupted replay changed the next original allocation/production step"
        outputs["continued"] = identity(continued)
    else:
        ordinary = importlib.import_module("first_plant_service_checks-2").ordinary
        version, _ = complete_chunks(original)
        label = "old430" if version == 430 else "selected-" + args.case
        first = ordinary(args.config, label, original)
        second = ordinary(args.config, "selected-repeat-" + args.case, ROOT / first["output"]["path"])
        # Normal startup may add only the CargoDist wait bit before its first
        # frame. The existing ordinary validator checks every clock field and
        # all actual owners, including explicit old-format/company exceptions.
        outputs.update(first=first["output"], second=second["output"])
        classifications.extend((dict(ordinary=first, complete_chunks=compare_native_load(original, ROOT / first["output"]["path"], True)),
                                dict(ordinary=second, complete_chunks=compare_native_load(ROOT / first["output"]["path"], ROOT / second["output"]["path"], True))))
    assert identity(original) == case["input"], "Acceptance changed its supplied input"
    report = {"_documentation": "r2mlvs18-geothermal-energy-and-industry.md Group17B; actual selected native saves/replay; real host/client join remains uncompleted",
              "configuration": args.config, "phase": args.phase, "case": args.case,
              "manifest": identity(args.manifest), "controller": identity(__file__), "input": identity(original),
              "native_reports": proofs, "outputs": outputs, "load_classifications": classifications,
              "all_original_inputs_unchanged": True, "actual_network_join": False}
    path = context.output("reports", f"selected-save-{args.config}-{args.phase}-{args.case}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
