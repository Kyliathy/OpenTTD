"""Seal current tube structure observations with complete native owner and RGB comparisons.

@file selected_tube_gallery_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84CC.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Actual existing scene producers own pixels, native ranges and simulation; this file verifies their explicit current evidence.
"""
import argparse
import json
from pathlib import Path
import sys

from PIL import Image

sys.dont_write_bytecode = True
from selected_tube_gallery import ROOT, identity, structure_jobs, verify_art_references
from tube_sprite_report import compare_worlds
from mole_useful_raster_report import count_native


def exact_pixels(value):
    """Count actual source-pixel witnesses already verified by the named current native observers.

    @param value Producer's packed_pixels subtree, excluding historical receipt and unrelated native accounting metadata.
    @return Sum of independent RGB-exact authored source witnesses at their original native zooms.
    """
    if isinstance(value, dict):
        # The older conduit observer calls its exact authored count 'pixels';
        # both names belong only to this verified packed_pixels subtree.
        return value.get("rgb_pixels_exact", 0) + value.get("pixels", 0) + sum(exact_pixels(row) for key, row in value.items() if key not in ("rgb_pixels_exact", "pixels"))
    if isinstance(value, list):
        return sum(exact_pixels(row) for row in value)
    return 0


def frames_by_name(rows):
    """Retain exact native frame identities across existing producer record shapes.

    @param rows Actual frame records; conduit observations wrap their identity in file beside zoom/path-row metadata.
    @return Unique native basenames mapped to unchanged identity records; no path guessing or frame substitution occurs.
    """
    identities = [row.get("file", row) for row in rows]
    result = {Path(row["path"]).name: row for row in identities}
    assert len(result) == len(identities), "native frame basenames are ambiguous within one exact scene"
    return result


def accept(inputs, output):
    """Accept every exact selected structure scene in both current native readers.

    @param inputs Exact completed Debug/optimized observation matrices from selected_tube_gallery.py.
    @param output Absent immutable receipt under repository logs; no runtime/public/gallery flags are published here.
    @return None; any stale source/binary, incomplete matrix, native owner difference or changed pixel refuses closure.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    grid, selections, cache = {}, {}, {}; totals = {cfg: {"cases": 0, "assertions": 0} for cfg in ("Debug", "RelWithDebInfo")}
    pixels = 0
    for name in inputs:  # Both explicitly named matrices are mandatory; a same-sized family substitution or implicit latest run cannot authorize the selected current gallery.
        path = ROOT / name; matrix = json.loads(path.read_text(encoding="utf-8")); cfg = matrix["configuration"]
        assert cfg in totals and cfg not in selections and matrix["complete"]
        assert matrix["jobs"] == structure_jobs(cfg)
        assert matrix["controller"] == identity(ROOT / "src/tests/multilayer/selected_tube_gallery.py")
        assert matrix["binary"] == identity(ROOT / f"build/{cfg}/openttd_test.exe")
        assert matrix["public"] == identity(ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json")
        verify_art_references(matrix, cache); selections[cfg] = {"matrix": identity(path), **matrix}
        assert len(matrix["observations"]) == len(matrix["jobs"])
        for job, observation in zip(matrix["jobs"], matrix["observations"]):  # Each current report must belong to its exact accepted scene/state command and original writer, rather than borrowing a different family's successful output.
            assert observation["job"] == job["name"]
            report_path = ROOT / observation["report"]["path"]; report = json.loads(report_path.read_text(encoding="utf-8")); verify_art_references(report, cache)
            assert report["configuration"] == cfg and report["binary"] == matrix["binary"]
            assert report["controller"] == identity(ROOT / job["script"])
            native = report["native"]; assert native["writer"] == job["writer"] and native["complete_chunks_exact"] == 126
            assert compare_worlds(native["before"], native["after"]) == 126
            count_native(native, totals[cfg]); pixels += exact_pixels(native["packed_pixels"])
            key = (cfg, job["name"]); assert key not in grid; grid[key] = {"report": observation["report"], **report}
    assert selections.keys() == totals.keys()
    frames, repeats, worlds = [], [], []
    for job in selections["Debug"]["jobs"]:  # Current independent readers must agree on every complete world boundary and every native RGB frame for all powered/dark/reserved/service/facility/conduit scenes.
        label = job["name"]; a, b = grid["Debug", label]["native"], grid["RelWithDebInfo", label]["native"]
        assert compare_worlds(a["before"], b["before"]) == compare_worlds(a["after"], b["after"]) == 126
        worlds.append({"scene": label, "before": [a["before"], b["before"]], "after": [a["after"], b["after"]], "complete_chunks_exact": 126})
        left, right = frames_by_name(a["frames"]), frames_by_name(b["frames"])
        assert left.keys() == right.keys() and len(left) > 0
        for basename, first in left.items():  # Complete repeat canvases remain evidence too; pairing by exact native basename preserves zoom, path location and repetition rather than choosing similar-looking pixels.
            second = right[basename]; x, y = Image.open(ROOT / first["path"]).convert("RGB"), Image.open(ROOT / second["path"]).convert("RGB")
            assert x.size == y.size and x.tobytes() == y.tobytes(), (label, basename)
            frames.append({"scene": label, "left": first, "right": second, "complete_rgb_exact": True})
        for cfg, native in (("Debug", a), ("RelWithDebInfo", b)):  # Every pair of original native repetitions must independently match in each reader; cross-reader agreement alone cannot hide unstable drawing.
            indexed = frames_by_name(native["frames"])
            for basename, first in indexed.items():
                if not basename.endswith("-0.png"):
                    continue
                second = indexed[basename.removesuffix("-0.png") + "-1.png"]
                x, y = Image.open(ROOT / first["path"]).convert("RGB"), Image.open(ROOT / second["path"]).convert("RGB")
                assert x.size == y.size and x.tobytes() == y.tobytes(), (cfg, label, basename)
                repeats.append({"configuration": cfg, "scene": label, "first": first, "second": second, "complete_rgb_exact": True})
    assert pixels > 0
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84CC.1; current original presentation scene matrix,not a new useful route or a public runtime promotion",
              "controller": identity(__file__), "matrices": list(selections.values()), "reports": list(grid.values()), "summary": totals,
              "native_cases": sum(row["cases"] for row in totals.values()), "native_assertions": sum(row["assertions"] for row in totals.values()),
              "complete_cross_reader_world_pairs": worlds, "complete_cross_reader_frame_pairs": frames, "complete_repeat_frame_pairs": repeats,
              "exact_original_source_pixels": pixels, "native_version": 434,
              "remaining": "84CC.2 all five original useful model/topology mapping,current84CD paid geothermal scenes,return84C.1/.2 gallery and84D publication;whole master/human/operator/network remain pending."}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "cases": result["native_cases"], "assertions": result["native_assertions"], "rgb_pairs": len(frames), "exact_source_pixels": pixels}), flush=True)


def main():
    """Verify the exact current pair of tube structure matrices without changing source or public metadata.

    @return None; failed or incomplete native evidence cannot produce a sealed receipt.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--inputs", nargs=2, required=True); parser.add_argument("--output", required=True)
    args = parser.parse_args(); accept(args.inputs, args.output)


if __name__ == "__main__":
    main()
