"""Verify current useful/topology pixels and map actual rendered capsule/casing IDs to public ranges.

@file selected_tube_movement_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84CC.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
No selected runtime/gallery flag is published by this source-preserving evidence verifier.
"""
import argparse
import json
from pathlib import Path
import sys

from PIL import Image

sys.dont_write_bytecode = True
from selected_tube_movement_gallery import ROOT, identity, jobs_for, verify_art_references
from selected_tube_gallery_report import frames_by_name
from selected_tube_end_pixels import end_cap_pixels
from tube_sprite_report import compare_worlds
from mole_useful_raster_report import count_native
sys.path.insert(0, str(ROOT / "media/multilayer"))
import activate_m1_art as activation


def views_for(report, part):
    """Expose actual native pure boundaries without changing either producer's public record shape.

    @param report Current useful-route or physical topology capture report.
    @param part Explicit useful or topology matrix owning this observation.
    @return Actual useful inspection point or eight physically earned native headings.
    """
    return [report["view"]] if part == "useful" else report["native"]["views"]


def body_base(view, model, public, packed):
    """Bind one actual rendered body ID to the existing source pose and native range.

    @param view Native actual model/load/direction/frame evidence, not an inferred model from similar pixels.
    @param model Actual canonical model identity from its accepted original writer.
    @param public Existing sole public family manifest.
    @param packed Existing exact decoded native pose table.
    @return Loaded sprite base and matching public body family; all frames must agree on one base.
    """
    asset = f"vehicle.model-{model:02}"; family = next(row for row in public["assets"] if row["asset_id"] == asset)
    state = family["states"][view.get("state", 1)]; direction = family["directions"][view["view"]]
    pose = next(row for row in packed["sprites"] if (row["asset_id"], row["state"], row["direction"], row["frame"]) == (asset, state, direction, view["frame"]))
    base = view["sprite"] - pose["native_offset"]; assert base > 0
    return base, family


def accept(inputs, output):
    """Seal all five actual useful worlds and every current directional/joined presentation boundary.

    @param inputs Four exact completed useful/topology matrices covering both current configurations.
    @param output Absent immutable receipt under repository logs; public/runtime/source pixels remain unchanged.
    @return None; missing/changed model, native source, pixel, complete world or actual casing/range prevents acceptance.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    public = json.loads(activation.PUBLIC.read_text(encoding="utf-8"))
    selection = activation.EvidenceSet(ROOT / "logs/proofs/mlm/native-art/selected-runtime-20261018/evidence.json")
    packed = json.loads(selection.output("packed").read_text(encoding="utf-8"))
    assert identity(ROOT / packed["packed_grf"]["path"])["sha256"] == packed["packed_grf"]["sha256"]
    totals = {cfg: {"cases": 0, "assertions": 0} for cfg in ("Debug", "RelWithDebInfo")}
    matrices, grid, cache, bases, families, caps = {}, {}, {}, set(), {}, []
    exact = 0
    for name in inputs:  # Every exact reader/part pair is mandatory; dated original acceptance can supply lineage but cannot replace current rendered pixels or a missing native model.
        path = ROOT / name; matrix = json.loads(path.read_text(encoding="utf-8")); cfg, part = matrix["configuration"], matrix["part"]
        assert cfg in totals and part in ("useful", "topology") and (cfg, part) not in matrices and matrix["complete"]
        purchase_ref = matrix.get("purchase_receipt")
        if purchase_ref is not None:
            verify_art_references(purchase_ref, cache)
        assert matrix["jobs"] == jobs_for(cfg, part, purchase_ref) and matrix["controller"] == identity(ROOT / "src/tests/multilayer/selected_tube_movement_gallery.py")
        assert matrix["binary"] == identity(ROOT / f"build/{cfg}/openttd_test.exe") and matrix["public"] == identity(activation.PUBLIC)
        verify_art_references(matrix, cache); matrices[cfg, part] = {"matrix": identity(path), **matrix}
        assert len(matrix["observations"]) == len(matrix["jobs"])
        for job, observation in zip(matrix["jobs"], matrix["observations"]):  # Each current report retains the exact original writer and actual source/paid route category; neither generic frames nor a recreated route can fill the matrix.
            assert observation["job"] == job["name"]
            path = ROOT / observation["report"]["path"]; report = json.loads(path.read_text(encoding="utf-8")); verify_art_references(report, cache)
            assert report["configuration"] == cfg and report["binary"] == matrix["binary"] and report["controller"] == identity(ROOT / job["script"])
            native = report["native"]; count_native(native, totals[cfg])
            writer = report["writer"] if part == "useful" else native["writer"]; assert writer == job["writer"]
            if part == "useful":
                assert report["native_steps"] > 0 and report["consumed_j"] > 0
            else:
                assert native["native_ticks"] > 0 and native["used_j"] > 0
                assert {row["view"] for row in native["views"]} == set(range(8))
            model = job["model"]; views = views_for(report, part)
            for view in views:  # Every physically earned native pose must retain correct/wrong-layer picking, its complete pure126-chunk boundary and exact body/casing source witnesses.
                assert view["speed"] > 0 and view["native_picking"] and view["wrong_layer_refused"] and view["complete_chunks_exact"] == 126
                assert compare_worlds(view["before"], view["after"]) == 126
                base, body = body_base(view, model, public, packed); bases.add(base)
                current = [body]
                for rim in view["rims"]:  # Actual native foreground IDs identify each visible casing family through the one existing public range table; source filenames and broad tube prefixes cannot substitute a different family.
                    offset = rim["front"] - base
                    family = next(row for row in public["assets"] if row.get("packed", {}).get("native_offset") is not None and row["packed"]["native_offset"] <= offset < row["packed"]["native_offset"] + row["packed"]["sprite_count"])
                    assert family["asset_id"].startswith("tube."); current.append(family)
                frames = [row["file"] for row in view["frames"]]
                exact += sum(row["exact_pixels"] for row in view["packed_pixels"]["matches"])
                for family in current:  # Direct frame links retain the current actual consumer/public range and original useful-vs-presentation boundary for every observed family.
                    asset = family["asset_id"]
                    row = families.setdefault(asset, {"asset_id": asset, "native_offset": family["packed"]["native_offset"], "sprite_count": family["packed"]["sprite_count"], "frames": {}, "native_views": []})
                    row["frames"].update({frame["path"]: frame for frame in frames})
                    row["native_views"].append({"configuration": cfg, "part": part, "model": model, "view": view["view"], "report": observation["report"], "original_writer": writer,
                                                "state_owner": "actual native packet load,physical heading,original source/service and renderer", "fixture_classification": job["fixture_classification"]})
            if job.get("layout") == "joined":
                witnesses = end_cap_pixels(report, public, packed, base); caps.extend({"configuration": cfg, "model": model, **row} for row in witnesses)
                family = next(row for row in public["assets"] if row["asset_id"] == "tube.end")
                row = families.setdefault("tube.end", {"asset_id": "tube.end", "native_offset": family["packed"]["native_offset"], "sprite_count": family["packed"]["sprite_count"], "frames": {}, "native_views": []})
                row["frames"].update({witness["frame"]["path"]: witness["frame"] for witness in witnesses})
                row["native_views"].append({"configuration": cfg, "part": part, "model": model, "view": 3, "report": observation["report"], "original_writer": writer,
                                            "state_owner": "actual paid original collinear X branch and logged native neighbour publication;full original cap parts at fixed endpoint", "fixture_classification": job["fixture_classification"]})
            grid[cfg, job["name"]] = {"report": observation["report"], **report}
    assert matrices.keys() == {(cfg, part) for cfg in totals for part in ("useful", "topology")} and len(bases) == 1
    expected = {asset for asset in activation.SELECTED_RUNTIME_ADDITIONS if asset.startswith("vehicle.") or (asset.startswith("tube.") and asset != "tube.conduit")}
    assert families.keys() == expected, "current native family-to-frame mapping is incomplete"
    pairs, repetitions, worlds = [], [], []
    for part in ("useful", "topology"):  # Independent readers must agree on the full original world and entire RGB raster at every actual useful/directional inspection point.
        for job in matrices["Debug", part]["jobs"]:
            left, right = grid["Debug", job["name"]], grid["RelWithDebInfo", job["name"]]
            a, b = views_for(left, part), views_for(right, part); assert len(a) == len(b)
            for x, y in zip(a, b):
                assert x["view"] == y["view"] and compare_worlds(x["before"], y["before"]) == compare_worlds(x["after"], y["after"]) == 126
                worlds.append({"job": job["name"], "view": x["view"], "before": [x["before"], y["before"]], "after": [x["after"], y["after"]], "complete_chunks_exact": 126})
                first, second = frames_by_name(x["frames"]), frames_by_name(y["frames"]); assert first.keys() == second.keys()
                for basename, frame in first.items():  # Both native zooms and every complete repetition retain all background/company/foreground pixels across readers, not just the source contour used for family identity.
                    other = second[basename]; p, q = Image.open(ROOT / frame["path"]).convert("RGB"), Image.open(ROOT / other["path"]).convert("RGB")
                    assert p.size == q.size and p.tobytes() == q.tobytes(), (job["name"], basename)
                    pairs.append({"job": job["name"], "left": frame, "right": other, "complete_rgb_exact": True})
                for cfg, view in (("Debug", x), ("RelWithDebInfo", y)):  # Repetition is verified independently inside each reader so cross-reader agreement cannot hide unstable native rendering.
                    indexed = frames_by_name(view["frames"])
                    for basename, frame in indexed.items():
                        if not basename.endswith("-0.png"):
                            continue
                        other = indexed[basename.removesuffix("-0.png") + "-1.png"]
                        p, q = Image.open(ROOT / frame["path"]).convert("RGB"), Image.open(ROOT / other["path"]).convert("RGB")
                        assert p.size == q.size and p.tobytes() == q.tobytes()
                        repetitions.append({"configuration": cfg, "job": job["name"], "first": frame, "second": other, "complete_rgb_exact": True})
    for row in families.values():  # Publish direct verified frame identities through this evidence receipt only; the actual public runtime/gallery activation still belongs to84D.
        row["frames"] = list(row["frames"].values())
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84CC.2; current actual useful/physical topology frames,exact public range mappings and complete native purity;public runtime promotion pending",
              "controller": identity(__file__), "endpoint_verifier": identity(ROOT / "src/tests/multilayer/selected_tube_end_pixels.py"), "matrices": list(matrices.values()), "reports": list(grid.values()),
              "families": list(families.values()), "end_cap_witnesses": caps, "loaded_sprite_base": next(iter(bases)), "summary": totals,
              "native_cases": sum(row["cases"] for row in totals.values()), "native_assertions": sum(row["assertions"] for row in totals.values()),
              "complete_cross_reader_frame_pairs": pairs, "complete_repeat_frame_pairs": repetitions, "complete_cross_reader_world_pairs": worlds,
              "exact_body_and_rim_pixels": exact, "exact_original_end_cap_pixels": sum(row["original_exact_pixels"] for row in caps), "native_version": 434,
              "packed_grf": packed["packed_grf"], "remaining": "84CD current original paid geothermal evidence,return84C.1 aggregate/84C.2 gallery then84D publication;whole master/human/operator/network gates remain pending."}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "families": len(families), "cases": result["native_cases"], "assertions": result["native_assertions"], "rgb_pairs": len(pairs), "exact_end_cap_pixels": result["exact_original_end_cap_pixels"]}), flush=True)


def main():
    """Verify four explicit current movement matrices and seal an absent evidence receipt.

    @return None; no source pixel, native world, public range or activation status is changed.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--inputs", nargs=4, required=True); parser.add_argument("--output", required=True)
    args = parser.parse_args(); accept(args.inputs, args.output)


if __name__ == "__main__":
    main()
