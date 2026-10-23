"""Refresh actual useful capsule worlds and declared directional topology presentation originals.

@file selected_tube_movement_gallery.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84CC.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Useful producers remain their exact dated owners; topology fixtures disclose prepared floor and portals separately.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True
from geothermal_machinery_raster import ROOT, identity, verify_art_references
from capsule_useful_model_audit import MODELS
from runner import current_run_context

PURCHASE_RECEIPT = ROOT / "logs/graphics/selected-purchase/20261018-native-01/verification.json"


def jobs_for(configuration, part, purchase_receipt=None):
    """Select exact original writers for one bounded useful-route or topology observation matrix.

    @param configuration Current Debug or optimized native reader, always opposite its retained original writer.
    @param part Useful worlds for all five models, or physical plain/joined directional presentation fixtures.
    @param purchase_receipt Optional exact current sealed purchase identity;default retains the original immutable selection.
    @return Existing native observer commands and exact dated producer/original identities; no native builders run.
    """
    jobs = []
    if part == "useful":
        for spec in MODELS:  # The original five-model acceptance owner supplies each actual useful route, date and immutable raster lineage; no generic capsule or prepared route can replace a model.
            model, name = spec[:2]; path = ROOT / "logs/tubes" / spec[7] / "20261018-native-01" / spec[8]
            assert identity(path)["sha256"] == spec[9]
            receipt = json.loads(path.read_text(encoding="utf-8"))
            selected = [row for row in receipt["records"] if row["configuration"] == configuration and row.get("focus") == "raster"]
            assert len(selected) == 1; writer = selected[0]["writer"]; assert identity(ROOT / writer["path"]) == writer
            job = {"name": f"useful-{model}", "model": model, "script": f"src/tests/multilayer/{name}_useful_raster.py",
                   "arguments": ["--config", configuration, "--writer-report", writer["path"]], "writer": writer,
                   "sealed_receipt": identity(path), "fixture_classification": receipt["fixture_classification"]}
            if model == 13:
                #84CB already observed the same actual moving32RFMT Forge in
                # both current binaries. Reuse that exact native/pixel/purity
                # report rather than earning another unnecessary opportunity.
                if purchase_receipt is None:
                    chosen = PURCHASE_RECEIPT
                    assert identity(chosen)["sha256"] == "e723b623ee673f48fc0a729ae862a6ee567f79b12dadd134043abb27812376b4"
                else:
                    chosen = (ROOT / purchase_receipt["path"]).resolve()
                    assert chosen.is_relative_to(ROOT / "logs") and identity(chosen) == purchase_receipt
                purchase = json.loads(chosen.read_text(encoding="utf-8"))
                assert purchase["native_version"] == 434 and purchase["native_cases"] == 4
                if purchase_receipt is not None:
                    verify_art_references(purchase, {})
                    assert purchase["binaries"][configuration] == identity(ROOT / f"build/{configuration}/openttd_test.exe")
                current = next(row for row in purchase["reports"] if row["configuration"] == configuration and row["world"] == "forge")
                job["reused_current_report"] = current["native_report"]; job["reused_receipt"] = identity(chosen)
            jobs.append(job)
        assert len(jobs) == 5
    else:
        for family, layout in (("travel-art", "plain"), ("joins-art", "joined")):  # Existing accepted paid-command topology fixtures supply genuine turns/crossing/junction geometry, explicitly separate from the original useful cargo sources.
            path = ROOT / f"logs/tubes/{family}/20261018-native-01/verification.json"; receipt = json.loads(path.read_text(encoding="utf-8"))
            for row in receipt["reports"]:  # Each current model/view matrix reuses its exact accepted opposite-writer original; never invoke an old builder to refresh the screenshot.
                if row["configuration"] != configuration or row["focus"] != "raster":
                    continue
                writer = row["native"]["writer"]; assert identity(ROOT / writer["path"]) == writer
                jobs.append({"name": f"{layout}-{row['model']}", "model": row["model"], "layout": layout, "script": "src/tests/multilayer/tube_travel_sprite_checks.py",
                             "arguments": ["--config", configuration, "--focus", "raster", "--model", str(row["model"]), "--layout", layout, "--writer-report", writer["path"]],
                             "writer": writer, "sealed_receipt": identity(path), "fixture_classification": receipt["fixture_classifications"]})
        assert len(jobs) == 7
    return jobs


def capture(configuration, part, output, purchase_receipt=None):
    """Run sequential actual native observers and immediately retain every explicit completed result.

    @param configuration Current actual native Debug or optimized reader configuration.
    @param part Bounded useful or topology observation matrix; no feature producer is recreated.
    @param output Absent progress/final report under repository logs.
    @param purchase_receipt Explicit current sealed purchase file for shared Forge observation,never a latest-run guess.
    @return None; any failed observer, stale original/current source or binary refuses a complete passing matrix.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    purchase_ref = identity(ROOT / purchase_receipt) if purchase_receipt is not None else None
    context = current_run_context("graphics", "selected-tube-movement-gallery"); jobs = jobs_for(configuration, part, purchase_ref)
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84CC.2; exact useful originals and separately labelled topology presentation fixtures;no new world builder or public activation",
              "configuration": configuration, "part": part, "controller": identity(__file__), "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"),
              "public": identity(ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"), "jobs": jobs, "observations": [], "complete": False}
    if purchase_ref is not None:
        result["purchase_receipt"] = purchase_ref
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    for index, job in enumerate(jobs):  # One native observation at a time preserves the requested sequential execution and records useful progress before the next process starts.
        log_identity = None
        if "reused_current_report" in job:
            path = ROOT / job["reused_current_report"]["path"]; assert identity(path) == job["reused_current_report"]
        else:
            process = subprocess.run([sys.executable, "-B", str(ROOT / job["script"]), *job["arguments"]], cwd=ROOT, capture_output=True, timeout=110, check=False)
            log = context.logs / f"{configuration}-{part}-{index:02}-{job['name']}.log"; log.write_bytes(process.stdout + process.stderr); assert process.returncode == 0, log
            log_identity = identity(log); path = Path(process.stdout.decode("utf-8").splitlines()[-1])
        report = json.loads(path.read_text(encoding="utf-8")); verify_art_references(report, {})
        assert report["configuration"] == configuration and report["binary"] == result["binary"] and report["controller"] == identity(ROOT / job["script"])
        for source in report["sources"]:  # Current pixels must belong to the exact current implementation/fixture owners, independently of the dated accepted producer receipt.
            assert identity(ROOT / source["path"]) == source
        result["observations"].append({"job": job["name"], "report": identity(path), "controller_log": log_identity})
        output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(f"{configuration} {part} {index + 1}/{len(jobs)} {job['name']}: passed", flush=True)
    assert identity(ROOT / result["public"]["path"]) == result["public"]
    result["complete"] = True; output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"report": identity(output), "observations": len(jobs)}), flush=True)


def main():
    """Select one bounded current useful-model or actual-topology observation matrix.

    @return None; all original inputs and frame identities remain explicit and immutable.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--part", choices=("useful", "topology"), required=True); parser.add_argument("--output", required=True)
    parser.add_argument("--purchase-receipt", help="exact current sealed purchase closure for shared Forge observation")
    args = parser.parse_args(); capture(args.config, args.part, args.output, args.purchase_receipt)


if __name__ == "__main__":
    main()
