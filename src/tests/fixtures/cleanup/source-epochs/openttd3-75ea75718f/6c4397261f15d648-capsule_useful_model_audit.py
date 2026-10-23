"""Audit the five original useful capsule routes without rebuilding their accepted worlds.

@file capsule_useful_model_audit.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15FEE/original15.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Historical receipts bind original reports/files; their nested binary/source hashes are dated history.
Only the explicit calendar phase launches current native processes. Art and world files remain immutable.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys
from types import SimpleNamespace

from PIL import Image

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from runner import current_run_context
from local_electrical_feed_checks import identity, native
from mole_useful_raster_report import count_native
from tube_sprite_report import compare_worlds
from tube_travel_sprite_checks import match_travel_pixels

CONFIGURATIONS = ("Debug", "RelWithDebInfo")
CALENDAR_CASES = (
    ("Native capsule startup honors all five exact launch dates", "startup"),
    ("Native calendar publication uses each original capsule launch without a preview year", "monthly"),
    ("Saved native capsule dates reconcile without changing other owners", "restore"),
)
# These are sealed actual moving-world receipts, rather than an atlas-only
# gallery or reconstructed route. Their exact file/producer lineage supplies
# each original source, useful delivery, ordinary-file and rendering owner.
MODELS = (
    (9, "mole", 1970, "COAL", 20, 55, 2, "useful-files", "mole-raster-verification.json", "01fd0ec36cdee3b46bbeb90d6ddbd154c6baf7b5f47de798a40b8169ae58ae03"),
    (10, "crate", 1975, "GOOD", 39, 844, 3, "crate-files", "raster-verification.json", "1eccc847047f0acf7bdc81b799438bb1cb78d3d915982d91c292aeb5ad0d9326"),
    (11, "cistern", 1980, "CHEM", 20, 427, 3, "cistern-files", "raster-verification.json", "7e7644befeafaf8606da5f7fa7bd89d47c03e27c87b6469ae24e2e86e4c8e462"),
    (12, "shield", 1985, "RARE", 24, 798, 3, "shield-files", "raster-verification.json", "55604bdd99e0782ea2304a68f8aef6a2e0519e36bd711b7e663a140d9289846e"),
    (13, "forge", 2005, "RFMT", 32, 323, 3, "forge-files", "raster-verification.json", "702d5c16efd02cadcaffaaedaf7e5afe098af67bc642ca2104620f1d5e7c1d25"),
)


def artifact(reference, cache):
    """Require one exact retained original, without consulting dated executable identities.

    @param reference Repository-relative path, original byte count and SHA256.
    @param cache Already verified immutable artifact identities for this invocation.
    @return The verified original identity.
    """
    path = reference["path"]
    assert (ROOT / path).resolve().is_relative_to(ROOT)
    if path not in cache:
        cache[path] = identity(ROOT / path)
    assert cache[path] == reference, path
    return cache[path]


def original_processes(proofs, cache):
    """Check retained successful diagnostics and real world inputs/outputs only.

    @param proofs One native process or a list of independently run processes.
    @param cache Exact original artifacts already verified.
    @return None; stale logs/worlds or mismatched native counts refuse the audit.
    """
    rows = proofs if isinstance(proofs, list) else [proofs]
    # Successful counts cannot replace original diagnostic/world identity;
    # intentionally exclude dated controller, source and executable references.
    for proof in rows:
        for key in ("log", "input", "output"):
            if isinstance(proof.get(key), dict):
                artifact(proof[key], cache)
        count_native(proof, {"cases": 0, "assertions": 0})


def accepted_receipt(reference, receipts, cache):
    """Follow sealed receipt links while preserving the scope of each original acceptance.

    @param reference Exact receipt identity inherited from an already sealed parent.
    @param receipts Loaded accepted JSON keyed by original repository path.
    @param cache Exact report/log/world identities already verified.
    @return Original accepted JSON, with no fresh claim about its historical binaries.
    """
    artifact(reference, cache)
    path = reference["path"]
    if path in receipts:
        return receipts[path]
    data = json.loads((ROOT / path).read_text(encoding="utf-8"))
    receipts[path] = data
    # Producer/preparation/source receipt links retain actual upstream work.
    # Do not walk arbitrary identities: historical code/binaries were valid at
    # their own closure and may have changed in subsequent authorized groups.
    for key, value in data.items():
        if key.endswith("_receipt"):
            accepted_receipt(value, receipts, cache)
    for record in data["records"]:
        artifact(record["report"], cache)
        original_processes(record["native"], cache)
        # Native writers retain the original loaded/final saves separately
        # from process output. Every selected world must still be available.
        for point in record.get("checkpoints", []):
            artifact(point["input"], cache)
    return data


def contour(name, state, direction, frame, cache):
    """Hash the original opaque shape independently of palette and sprite translation.

    @param name Original capsule family basename.
    @param state Actual observed or common reference load state.
    @param direction Original named view direction.
    @param frame Original animation frame index.
    @param cache Verified original manifest/atlas identities.
    @return Original atlas row and normalized boolean contour signature; no image is written.
    """
    path = ROOT / f"logs/proofs/mlm/native-art/ground/{name}-source-manifest.json"
    manifest = json.loads(path.read_text(encoding="utf-8")); atlas = manifest["atlases"][0]
    artifact(identity(path), cache)
    assert identity(ROOT / atlas["path"])["sha256"] == atlas["sha256"]
    row = next(row for row in atlas["sprites"] if (row["state"], row["direction"], row["frame"]) == (state, direction, frame))
    x, y, width, height = row["box"]
    piece = Image.open(ROOT / atlas["path"]).crop((x, y, x + width, y + height))
    mask = piece.point([0] + [255] * 255, mode="L"); bounds = mask.getbbox(); assert bounds is not None
    shape = mask.crop(bounds)
    digest = hashlib.sha256(str(shape.size).encode("ascii") + shape.tobytes()).hexdigest()
    return {"manifest": identity(path), "atlas": identity(ROOT / atlas["path"]), "state": state,
            "direction": direction, "frame": frame, "normalized_size": list(shape.size), "contour_sha256": digest}


def runtime_views(receipt, name, model, cache):
    """Recheck original runtime captures against unchanged atlas pixels and complete worlds.

    @param receipt Sealed actual useful-route raster receipt for one model.
    @param name Original capsule family basename.
    @param model Actual native capability identity9..13.
    @param cache Verified immutable report, world and capture identities.
    @return Both original configurations' capture identities and independent silhouette signatures.
    """
    views = {}; rgb = {}
    # Both actual build configurations must independently retain the loaded
    # moving capsule, correct-layer picking and every complete render owner.
    for record in receipt["records"]:
        if record.get("focus") != "raster":
            continue
        cfg = record["configuration"]; assert cfg in CONFIGURATIONS and cfg not in views
        view = record["view"]; assert view["model"] == model and view["speed"] > 0
        assert view["native_picking"] and view["wrong_layer_refused"]
        assert record["native_steps"] == record["end_tick"] - record["start_tick"] > 0
        assert compare_worlds(view["before"], view["after"]) == compare_worlds(view["after"], record["native"]["output"]) == 126
        artifact(view["before"], cache); artifact(view["after"], cache)
        assert len(view["frames"]) == 4
        actual_rgb = {}
        # Full640x360 RGB frames include company remaps and all scenery.
        # Repeats and opposite builds cannot hide changes behind hull metrics.
        for capture in view["frames"]:
            artifact(capture["file"], cache)
            image = Image.open(ROOT / capture["file"]["path"]).convert("RGB"); assert image.size == (640, 360)
            key = capture["zoom"], capture["repeat"]; assert key not in actual_rgb
            actual_rgb[key] = hashlib.sha256(image.tobytes()).hexdigest()
        assert actual_rgb.keys() == {(zoom, repeat) for zoom in (1, 2) for repeat in range(2)}
        assert all(actual_rgb[zoom, 0] == actual_rgb[zoom, 1] for zoom in (1, 2))
        home = (ROOT / view["frames"][0]["file"]["path"]).parent
        assert match_travel_pixels(SimpleNamespace(images=home), model, view) == view["packed_pixels"]
        state = ("empty", "partial", "full")[view.get("state", 1)]
        observed = contour(name, state, ("N", "NE", "E", "SE", "S", "SW", "W", "NW")[view["view"]], view["frame"], cache)
        views[cfg] = {"report": record["report"], "model": model, "load_state": state,
                      "original_frames": view["frames"], "observed_contour": observed,
                      "packed_pixels": view["packed_pixels"], "complete_chunks_pure": 126}
        rgb[cfg] = actual_rgb
    assert views.keys() == set(CONFIGURATIONS) and rgb["Debug"] == rgb["RelWithDebInfo"]
    assert views["Debug"]["observed_contour"] == views["RelWithDebInfo"]["observed_contour"]
    return views


def useful_worlds(receipt, spec, receipts):
    """Verify each real launch-year useful delivery and its whole empty return evidence.

    @param receipt Actual useful-route raster receipt with exact ordinary-file lineage.
    @param spec Original model, family, introduction year, cargo, useful intake and native income contract.
    @param receipts Already verified sealed receipt graph.
    @return Original final worlds, native useful amounts and exact file-acceptance limitations.
    """
    model, name, year, cargo, units, income, returns, *_ = spec
    files = receipts[receipt["file_receipt"]["path"]]
    writer = receipts[files["writer_receipt"]["path"]] if "writer_receipt" in files else files
    finals = {}; deep = []
    # Only original step0 is the useful completed world. The untouched next
    # opportunity can cross a day/quarter or spend more power and is a baseline,
    # never a replacement for the original final intake/account observation.
    for record in writer["records"]:
        if record.get("phase") != "write":
            continue
        cfg = record["configuration"]; assert cfg in CONFIGURATIONS and cfg not in finals
        points = {point["scene"]: point for point in record["checkpoints"] if point["step"] == 0}
        final = points[max(points)]; snapshot = final.get("snapshot", final)
        log = (ROOT / record["native"]["log"]["path"]).read_text(encoding="utf-8")
        useful = re.search(r"^" + name.upper() + r"_USEFUL (.+)$", log, re.MULTILINE); assert useful is not None
        values = dict(re.findall(r"(\w+)=(\d+)", useful[1].split("snapshot=")[0]))
        assert int(values["year"]) == year and int(values["delivered"]) == units and int(values["income"]) == income
        assert int(values.get("returns", values.get("empty_returns", "0"))) == returns
        if name == "mole":
            assert final["calendar"] == 719528 and final["factory_wait"] == final["delivered"] == units
            assert final["cap_count"] == final["wagon_count"] == 0 and final["income"] == income
        else:
            assert snapshot["calendar_year"] == year
            # All retained train parts must be empty after the accepted whole
            # returns; native packet provenance and remaining source inventory
            # remain owned by their original sealed source/useful receipts.
            assert all(value == 0 for key, value in snapshot.items() if re.fullmatch(r"part\d+_units", key))
        if name == "crate":
            assert snapshot["total_delivered"] == units and snapshot["current_income"] == income
            assert snapshot["quarter0_income"] == 426 and snapshot["total_income"] == 1270
        if name == "shield":
            assert points[0]["snapshot"]["part0_layer"] == points[0]["snapshot"]["part1_layer"] == 3
            assert points[0]["snapshot"]["part1_units"] > 0
            assert points[1]["snapshot"]["part0_layer"] == points[1]["snapshot"]["part1_layer"] == 1
            assert points[1]["snapshot"]["part1_units"] > 0
            assert points[2]["snapshot"]["part2_layer"] == 1 and points[2]["snapshot"]["part2_units"] == units
            assert points[3]["snapshot"]["part0_layer"] == 3 and points[3]["snapshot"]["part1_units"] == 0
            assert points[4]["snapshot"]["part4_layer"] == 0 and points[4]["snapshot"]["part4_units"] == units
            assert snapshot["part0_layer"] == 3 and snapshot["part2_layer"] == snapshot["part3_layer"] == 1
            deep.append({"configuration": cfg, "writer": record["report"], "scenes": [points[i]["input"] for i in range(6)],
                         "contract": "Same conventional parts carry finite Deep RARE to Underground, transfer to local Shield, and return empty from Underground to Deep. Separate conventional parts carry the actual tube cargo from Underground to Surface ELEC. Capsules stay on Underground."})
        finals[cfg] = {"writer": record["report"], "original_final": final["input"], "tick": final["tick"],
                       "year": year, "cargo": cargo, "useful_units": units, "native_freight_income": income, "whole_empty_returns": returns}
    assert finals.keys() == set(CONFIGURATIONS)
    return {"model": model, "family": name, "launch_year": year, "files": receipt["file_receipt"], "original_finals": finals,
            "file_contract": files["continuation_contract"], "first_load_authority_chunks": len(files.get("complete_owner_chunks", [])) or None,
            "fixture_classification": receipt["fixture_classification"], "conventional_deep_chain": deep}


def calendar(configuration):
    """Run only the three exact current five-model date boundaries in isolated native processes.

    @param configuration Actual Debug or optimized native test binary.
    @return None; print the invocation-owned calendar report path after all focused cases pass.
    """
    context = current_run_context("tubes", "forge-files"); context.prepare_runtime(); processes = []
    # Startup, live publication and saved-definition restoration are separate
    # current authorities. Each exact case gets a process to prevent fixture
    # leakage and retains future-model locks as well as launch-day admission.
    for case, label in CALENDAR_CASES:
        _, proof = native(context, configuration, case, "five-model-" + label)
        processes.append({"case": case, **proof})
    result = {"_documentation": "r2mlvs17-underground-transport-tubes.md15FEE; current exact launch dates for all five original native models",
              "configuration": configuration, "focus": "five-model-calendar", "controller": identity(__file__),
              "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"), "native": processes,
              "sources": [identity(ROOT / name) for name in ("src/tests/transport_tube_calendar.cpp", "src/transport_tube_model.h", "src/transport_tube.cpp")]}
    path = context.output("reports", f"five-model-calendar-{configuration}.json")
    path.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(path, flush=True)


def audit(calendar_reports, output):
    """Close only original15.2 from exact five-model producers/files/captures and current date gates.

    @param calendar_reports Both independently successful current native calendar reports.
    @param output Absent immutable audit receipt under repository logs.
    @return None; any changed original, incomplete model/Deep chain or duplicate contour prevents closure.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    cache, receipts, models, calendars = {}, {}, [], {}; summary = {cfg: {"cases": 0, "assertions": 0} for cfg in CONFIGURATIONS}
    # Current calendar proof is deliberately fresh. Historical producer code
    # and executables remain dated identities in their own accepted receipts.
    for name in calendar_reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); cfg = report["configuration"]
        assert cfg in CONFIGURATIONS and cfg not in calendars and report["focus"] == "five-model-calendar"
        for key in ("controller", "binary"):
            artifact(report[key], cache)
        for reference in report["sources"]:
            artifact(reference, cache)
        assert [row["case"] for row in report["native"]] == [row[0] for row in CALENDAR_CASES]
        original_processes(report["native"], cache)
        for proof in report["native"]:
            count_native(proof, summary[cfg])
        calendars[cfg] = {"report": identity(path), **report}
    assert calendars.keys() == set(CONFIGURATIONS) and summary["Debug"] == summary["RelWithDebInfo"]
    # Ascending original model/date order matches the plan. Rechecking sealed
    # artifacts and pixels is read-only and never reruns source/route builders.
    for spec in MODELS:
        model, name, _, _, _, _, _, folder, filename, digest = spec
        path = ROOT / f"logs/tubes/{folder}/20261018-native-01/{filename}"; reference = identity(path); assert reference["sha256"] == digest
        receipt = accepted_receipt(reference, receipts, cache)
        views = runtime_views(receipt, name, model, cache)
        models.append({**useful_worlds(receipt, spec, receipts), "raster_receipt": reference, "runtime_views": views,
                       "common_partial_contour": contour(name, "partial", "SW", 0, cache)})
    assert len({row["common_partial_contour"]["contour_sha256"] for row in models}) == 5
    assert len({row["runtime_views"]["Debug"]["observed_contour"]["contour_sha256"] for row in models}) == 5
    assert len(models[3]["conventional_deep_chain"]) == 2
    result = {"_documentation": "r2mlvs17-underground-transport-tubes.md15FEE/15F.1/original15.2; actual five-model route/date/file/runtime audit",
              "controller": identity(__file__), "models": models, "current_calendar": calendars, "current_native_summary": summary,
              "accepted_receipts": [cache[path] for path in receipts], "original_artifacts_verified": len(cache),
              "distinct_original_observed_contours": 5, "distinct_common_partial_contours": 5,
              "native_version": 434, "packed_art": identity(ROOT / "media/baseset/openttd.grf"),
              "scope": "Actual original Coal20/Goods39/Chemicals20/RARE24/RFMT32 useful routes at1970/1975/1980/1985/2005; both builds, native income and whole empty returns, ordinary original files, exact real runtime hull/interior/rim pixels and full RGB/picking/render purity. Shield includes both conventional Deep/Underground directions and Surface delivery. Older Mole/Crate first-load authority/GLOG exceptions remain exactly their accepted file contracts; current date gates do not rewrite their historical worlds or prices.",
              "remaining": "Group16 paid TBM corridor/cavern conversion and bootstrap; later geothermal/lifecycle integration, player/operator/network acceptance and runtime gallery/human feedback remain pending. No profit/balance or missing-coinput manufactured-output claim."}
    assert result["packed_art"]["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "models": len(models), "current_calendar": summary,
                      "sealed_receipts": len(receipts), "original_artifacts_verified": len(cache), "distinct_contours": 5}), flush=True)


def main():
    """Select one bounded current calendar burst or the read-only original five-model audit.

    @return None; incompatible arguments or failed semantic obligations stop before acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--phase", choices=("calendar", "audit"), required=True)
    parser.add_argument("--config", choices=CONFIGURATIONS)
    parser.add_argument("--calendar-reports", nargs=2)
    parser.add_argument("--output")
    args = parser.parse_args()
    if args.phase == "calendar":
        assert args.config is not None
        calendar(args.config)
    else:
        assert args.calendar_reports is not None and args.output is not None
        audit(args.calendar_reports, args.output)


if __name__ == "__main__":
    main()
