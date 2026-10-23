"""Measure retained Electronics construction originals without publishing any source coverage.

@file register-electronics-stage.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups42B/42C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Disposable authoring helper: requests/results remain in this cache; immutable
originals and exact executed provenance are retained under the source owner.
"""

import hashlib
import json
from pathlib import Path
import sys

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_processor_base as plant


def digest(path):
    """Hash an actual retained source or reference.

    @param path Existing repository or permitted generated-image path.
    @return Exact byte SHA256.
    """
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main(request_path):
    """Review real crop/native boundaries after explicit per-component visual inspection.

    @param request_path Cache JSON with exact executed prompt, references and visual observations.
    @return None; write one versioned provenance document without changing public/source records.
    """
    request = json.loads(request_path.read_text(encoding="utf-8"))
    raw = ROOT / request["path"]
    with Image.open(raw) as image:
        rgba = image.convert("RGBA")
    contract = plant.processor_contract("facility.electronics-works")
    candidate = {key: request[key] for key in ("id", "path", "cache_path", "prompt", "construction_stage", "variants")}
    candidate.update(_documentation={"owner": "r2mlvs14-remaining-assets-and-runtime-integration.md Groups42B/42C",
                                     "architecture": "archi-map-layers.md", "scope": "Actual built-in construction output and per-component native/model review; no coverage/runtime claim"},
                     date="2026-10-21", tool="image_gen.imagegen", sha256=digest(raw), image_size=list(rgba.size),
                     references=[{"path": row["path"], "sha256": digest(ROOT / row["path"]), "role": row["role"]}
                                 for row in request["references"]],
                     processor_asset_id="facility.electronics-works", alpha_measure_threshold=16,
                     components=[], measurements=[], selection={"status": "pending_native_component_review"}, excluded_variants=[])
    if request["construction_stage"] == 3:
        candidate["completed_empty"] = True
    else:
        candidate["unfinished_empty"] = True
    # Each independent complete rectangle keeps its own measured bounds and
    # native registration. Nothing clips, erases or repaints a failed original.
    for variant, window in zip(request["variants"], request["windows"]):
        if window is None:
            window = [0, 0, *rgba.size]
        crop = rgba.crop(window)
        bounds = crop.getchannel("A").point(lambda value: 255 if value >= 16 else 0).getbbox()
        if bounds is None:
            raise ValueError("empty construction source")
        left, top, right, bottom = bounds
        height = round((bottom - top) * 62 / (right - left))
        candidate["components"].append({"id": f"cell-{variant}", "variant": variant, "world_cell": contract["native_cells"][variant],
                                         "direction": "none", "native_size": [64, 64], "anchor": [-31, -33], "resampling": "BOX",
                                         "alpha_crop_threshold": 16, "whole_original_uniform_scale": True, "source_window": window,
                                         "measured_alpha16_bbox": list(bounds), "native_box": [1, 64 - height, 63, 64],
                                         "construction_stage": request["construction_stage"]})
        candidate["measurements"].append({"variant": variant, "source_window": window, "measured_alpha16_bbox": list(bounds),
                                          "minimum_alpha16_clearance": min(left, top, crop.width - right, crop.height - bottom),
                                          "opaque_magenta_pixels": sum(alpha >= 16 and red > 220 and green < 60 and blue > 220
                                                                        for red, green, blue, alpha in crop.get_flattened_data())})
    record = json.loads(plant.RECORD.read_text(encoding="utf-8"))
    record["candidates"].append(candidate)
    approved, refused = [], {}
    # Model observations are supplied only after visual inspection. Actual
    # native contacts and envelope can still refuse a visually useful model.
    for variant in request["variants"]:
        try:
            sprite = plant.mine.original_cell(record, {"variant": variant, "source": candidate["id"], "component": f"cell-{variant}"}, contract)
            plant.mine.validate_cell(sprite, variant)
            if not request["model_review"].get(str(variant)):
                raise ValueError("no explicit model inventory review")
            approved.append(variant)
        except ValueError as error:
            refused[str(variant)] = str(error)
    candidate["selection"] = {"status": "approved_native_components" if approved else "rejected", "approved_variants": approved,
                               "refused_variants": refused, "model_review": request["model_review"]}
    candidate["excluded_variants"] = [int(value) for value in refused]
    target = ROOT / "media/multilayer/source/ground" / (candidate["id"] + ".json")
    if target.exists():
        raise ValueError("immutable candidate provenance already exists")
    target.write_text(json.dumps(candidate, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"id": candidate["id"], "selection": candidate["selection"], "measurements": candidate["measurements"]}))


if __name__ == "__main__":
    main(Path(sys.argv[1]))
