"""Seal current paid geothermal source/state/frame mappings with exact native owner and RGB comparisons.

@file selected_geothermal_gallery_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84CD.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Existing producer/selector/payload owners retain authority; this verifier publishes no runtime flags or gameplay quantities.
"""
import argparse
import json
from pathlib import Path
import re
import sys

from PIL import Image

sys.dont_write_bytecode = True
from selected_geothermal_gallery import ROOT, CASES, original_for, identity, verify_art_references
from selected_natural_vent_pixels import natural_vent_pixels
from geothermal_machinery_interaction import inspect as inspect_machinery
from geothermal_conduit_raster import inspect as inspect_conduit
from selected_control_raster import inspect as inspect_controls
from tube_sprite_report import compare_worlds


def bind_frames(families, public, asset, frames, context):
    """Map actual native frames and original state evidence to one existing public family.

    @param families Accumulated evidence mappings, not a sprite or simulation registry.
    @param public Existing exact public family objects indexed by asset_id.
    @param asset Actual native machinery/widget/utility/natural-source family.
    @param frames Hash-verified native PNG identities from the same ordinary invocation.
    @param context Actual case/configuration/state-owner and rendering mode information.
    @return None; public offsets and counts are borrowed without modification.
    """
    family = public[asset]
    row = families.setdefault(asset, {"asset_id": asset, "native_offset": family["packed"]["native_offset"], "sprite_count": family["packed"]["sprite_count"], "frames": {}, "native_views": []})
    row["frames"].update({frame["path"]: frame for frame in frames}); row["native_views"].append(context)


def accept(selection, output):
    """Verify every explicit current original scene, source witness, complete native world and paired RGB frame.

    @param selection Exact completed14-case index from selected_geothermal_gallery.py.
    @param output Absent immutable receipt beneath repository logs; no public or generated runtime is promoted.
    @return None; stale/false source/state/family, missing case or any native owner/pixel mismatch refuses acceptance.
    """
    selection = ROOT / selection; document = json.loads(selection.read_text(encoding="utf-8")); assert document["complete"]
    public_path = ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"
    public = {row["asset_id"]: row for row in json.loads(public_path.read_text(encoding="utf-8"))["assets"]}
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    grid, families, cache, native_paths, field_identities = {}, {}, {}, set(), set()
    totals = {cfg: {"cases": 0, "assertions": 0} for cfg in ("Debug", "RelWithDebInfo")}
    source_pixels = {"machinery": 0, "conduit": 0, "controls": 0, "natural_vent": 0}
    bases, natural_states, topology, repeats = set(), set(), {}, []
    for name in document["reports"]:  # Every actual construction/connection/service/removal/solid/retired original is required in each current reader; similarly named older captures cannot fill the matrix.
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_art_references(report, cache)
        cfg, case = report["configuration"], report["case"]; key = (cfg, case)
        assert cfg in totals and case in CASES and key not in grid and report["complete"]
        assert report["controller"] == identity(ROOT / "src/tests/multilayer/selected_geothermal_gallery.py")
        assert report["natural_vent_verifier"] == identity(ROOT / "src/tests/multilayer/selected_natural_vent_pixels.py")
        original, scene = original_for(cfg, case); assert report["original"] == original and report["scene"] == scene
        native_path = ROOT / report["native_report"]["path"]; native = json.loads(native_path.read_text(encoding="utf-8")); verify_art_references(native, cache)
        assert str(native_path) not in native_paths; native_paths.add(str(native_path))
        assert native["binary"] == identity(ROOT / f"build/{cfg}/openttd_test.exe") and native["input"] == original["input"]
        assert native["all_complete_native_chunks_equal"] and compare_worlds(native["before"], native["after"]) == 126
        raw = (ROOT / native["log"]["path"]).read_text(encoding="utf-8")
        match = re.search(r"All tests passed \((\d+) assertions in (\d+) test case", raw)
        assert match is not None and int(match[2]) == native["case_runs"] == 1 and int(match[1]) == native["assertions"] and "NATIVE_EXIT_CODE=0" in raw
        totals[cfg]["cases"] += 1; totals[cfg]["assertions"] += native["assertions"]
        vent = natural_vent_pixels(native); assert vent == report["natural_vent"]
        field = vent["native_field"]; field_identities.add(tuple(field[key] for key in ("id", "layer", "x", "y", "width", "height", "capacity")))
        natural_states.add((vent["state"], vent["frame"])); source_pixels["natural_vent"] += vent["original_exact_pixels"]
        context = {"configuration": cfg, "case": case, "report": identity(path), "original": original, "state_owner": "actual saved natural vent,rock floor,synchronized tick and native armed/cancelled survey"}
        bind_frames(families, public, "geothermal.vent", [vent[key] for key in ("active", "inactive", "cancelled")], context)
        actual = {row["path"]: row for row in native["frames"]}
        expected = {"machinery", "conduit"} if case in CASES[:4] else {"conduit"} if case == "heat-removed" else set()
        if case in ("ready", "magma-working"):
            expected.add("controls")
        assert report["roles"].keys() == expected
        for role, reference in report["roles"].items():  # Fresh original-pixel/native-state verification reuses the same completed ordinary report; neither a cached success boolean nor three copies of one process increase acceptance coverage.
            proof_path = ROOT / reference["report"]["path"]; proof = json.loads(proof_path.read_text(encoding="utf-8")); verify_art_references(proof, cache)
            assert proof["native_report"] == report["native_report"] and proof["native"] == native
            fresh = inspect_machinery(native, case) if role == "machinery" else inspect_conduit(native) if role == "conduit" else inspect_controls(native)
            for field, value in fresh.items():  # Every stored witness must reproduce its exact current source projection, full original pixel counts and actual original-owner state.
                assert proof[field] == value, "selected geothermal witness differs from its actual native source: " + role + "/" + field
            if role == "machinery":
                witnesses = proof["opaque_source_witnesses"]; source_pixels[role] += sum(row["original_exact_contour_pixels"] for row in witnesses)
                bases.update(row["native_base"] for row in witnesses)
            elif role == "conduit":
                # The conduit producer records opaque/hidden counts separately.
                # Its fixed original floor witness is complete with bodies
                # hidden; retain that established metric without counting the
                # smaller partially occluded opaque witness a second time.
                witnesses = proof["source_witnesses"]; source_pixels[role] += sum(row["exact_original_pixels"]["hidden"] for row in witnesses)
                bases.update(row["native_base"] for row in witnesses)
                topology[cfg, case] = sum(len(row["domains"]) for row in proof["topology"])
            else:
                source_pixels[role] += sum(row["original_exact_pixels"] for row in proof["source_witnesses"])
            repeats.extend({"configuration": cfg, "case": case, "role": role, **row} for row in proof["complete_repeat_frame_pairs"])
            for record in proof["records"]:  # Native renderer records identify actual machinery/layers/domains/widgets; each PNG must be the exact producer-declared counterpart of that record's own PPM.
                frames = []
                for name in record["paths"]:
                    png = Path(name).with_suffix(".png"); reference = actual[png.relative_to(ROOT).as_posix()]
                    assert reference["sha256"] == identity(png)["sha256"]; frames.append(identity(png))
                details = {"configuration": cfg, "case": case, "report": identity(proof_path), "original": original,
                           "layer": record["layer"], "mode": record["mode"], "state_owner": "actual original paid machinery,physical installed domain/native neighbours or native widget policy"}
                if role == "machinery":
                    assert record["native_industry_window"] and record["surface_same_owner_refused"]
                    bind_frames(families, public, record["asset_id"], frames, {**details, "industry": record["industry"], "cells": record["cells"]})
                elif role == "conduit":
                    bind_frames(families, public, "geothermal.conduit", frames, {**details, "tile": record["tile"], "domains": record["domains"]})
                else:
                    for widget in record["widgets"]:  # Both actual selected tool families retain readable native widgets and their original enabled/disabled/pressed policy, without granting gameplay permission.
                        bind_frames(families, public, widget["asset_id"], frames, {**details, "widget": widget})
        grid[key] = {"report": identity(path), **report, "native": native}
    assert grid.keys() == {(cfg, case) for cfg in totals for case in CASES}
    assert len(field_identities) == 1 and next(iter(field_identities))[-1] == 1000000 and len(bases) == 1
    assert {state for state, phase in natural_states} == {"surveyed", "excavated"}
    expected_families = {"facility.vent-collector", "facility.vent-powerhouse", "facility.magma-powerhouse", "facility.utility-riser", "geothermal.conduit", "geothermal.vent", "control.tube", "control.geothermal"}
    assert families.keys() == expected_families
    for cfg in totals:  # Actual original paid replacement ports earn50->55 installed cells;the paid Heat removal leaves54 and remains removed during later useful Magma service,with no repair or erased energy history inferred from a picture.
        assert [topology[cfg, case] for case in ("funded", "ready", "vent-working", "heat-removed", "magma-working")] == [50, 55, 55, 54, 54]
    pairs, worlds = [], []
    for case in CASES:  # Independent current readers must preserve every complete126-chunk before/after world and every original native RGB frame,including hidden bodies,cancelled overlays and actual disabled-widget checkerboards.
        a, b = grid["Debug", case]["native"], grid["RelWithDebInfo", case]["native"]
        assert compare_worlds(a["before"], b["before"]) == compare_worlds(a["after"], b["after"]) == 126
        worlds.append({"case": case, "before": [a["before"], b["before"]], "after": [a["after"], b["after"]], "complete_chunks_exact": 126})
        first, second = {Path(row["path"]).name: row for row in a["frames"]}, {Path(row["path"]).name: row for row in b["frames"]}
        assert first.keys() == second.keys()
        for name, frame in first.items():
            other = second[name]; p, q = Image.open(ROOT / frame["path"]).convert("RGB"), Image.open(ROOT / other["path"]).convert("RGB")
            assert p.size == q.size and p.tobytes() == q.tobytes(), (case, name)
            pairs.append({"case": case, "left": identity(ROOT / frame["path"]), "right": identity(ROOT / other["path"]), "complete_rgb_exact": True})
    for family in families.values():  # The final evidence mappings borrow exact existing public ranges;public runtime activation remains a separate84D step after the aggregate selected gallery passes.
        family["frames"] = list(family["frames"].values())
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84CD; current original paid/natural-source gallery mappings and complete native/pixel purity,not runtime publication",
              "controller": identity(__file__), "selection": identity(selection), "reports": list(grid.values()), "families": list(families.values()),
              "summary": totals, "native_cases": sum(row["cases"] for row in totals.values()), "native_assertions": sum(row["assertions"] for row in totals.values()),
              "complete_cross_reader_frame_pairs": pairs, "complete_repeat_frame_pairs": repeats, "complete_cross_reader_world_pairs": worlds,
              "exact_original_pixels": source_pixels, "natural_states_observed": sorted(natural_states), "loaded_sprite_base": next(iter(bases)), "native_version": 434,
              "public_manifest": identity(public_path), "packed_grf": identity(ROOT / "media/baseset/openttd.grf"),
              "remaining": "Return84C.1 aggregate selected family validation,84C.2 native contact sheet/gallery then84D publication;whole master/human/operator/network gates remain pending."}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "families": len(families), "cases": result["native_cases"], "assertions": result["native_assertions"], "rgb_pairs": len(pairs), "source_pixels": source_pixels}), flush=True)


def main():
    """Verify one exact complete current geothermal observation index and seal an absent receipt.

    @return None; no native process or source/public mutation is performed.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--selection", required=True); parser.add_argument("--output", required=True)
    args = parser.parse_args(); accept(args.selection, args.output)


if __name__ == "__main__":
    main()
