"""Verify actual installed geothermal branches beneath native paid-world bodies, without new source pixels or infrastructure.

@file geothermal_conduit_raster.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BC.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The ordinary application loader/compositor supplies every frame and complete unchanged native owner; all paid originals stay sealed.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from PIL import Image

sys.dont_write_bytecode = True
from geothermal_machinery_raster import ROOT, identity, original, verify_art_references, RECEIPTS
from geothermal_conduit_checks import CASES as FOCUSED_CASES, SOURCES as SELECTOR_SOURCES
from runner import current_run_context
from tube_sprite_report import compare_worlds
from mole_useful_raster_report import count_native

CASES = ("funded", "ready", "vent-working", "heat-removed", "magma-working")
SOURCES = SELECTOR_SOURCES + ("src/tests/geothermal_window_raster.cpp", "src/tests/geothermal_conduit_view_fixture.h", "src/tests/native_art_render_fixture.h",
                              "src/tests/CMakeLists.txt", "src/tests/multilayer/geothermal_conduit_raster.py")


def paid_original(configuration, case):
    """Resolve an exact sealed opposite-writer paid reconnection, operation or removal checkpoint.

    @param configuration Current ordinary native reader, distinct from the original writer.
    @param case Named actual funding/connection/conversion/paid Heat-isolation stage.
    @return Opaque receipt/report/checkpoint identities and the native observer scene; never generate replacement infrastructure.
    """
    if case != "heat-removed":
        return original(configuration, case)
    name, sha = RECEIPTS["service"]; receipt = ROOT / name; assert identity(receipt)["sha256"] == sha
    history = json.loads(receipt.read_text(encoding="utf-8")); record = next(row for row in history["records"] if row["configuration"] != configuration)
    point = next(point for point in record["checkpoints"] if point["scene"] == 5 and point["step"] == 0)
    assert identity(ROOT / point["input"]["path"]) == point["input"]
    return {"receipt": identity(receipt), "report": record["report"], "input": point["input"]}, "vent-tap"


def source_pixels(row, family, source, atlas, palette):
    """Project all original installed branches in native Heat-before-electricity and NE/SE/SW/NW order.

    @param row Actual selected native physical cell and issued domain/adjacency/sprite records, centred in its real viewport.
    @param family Existing sole public geothermal.conduit family contract.
    @param source Exact approved per-pose source manifest; no raster extraction or source editing occurs here.
    @param atlas Original approved indexed native atlas.
    @param palette Unmodified native original-DOS RGB palette.
    @return Every opaque final composed source pixel at its fixed actual world projection and the single compiled sprite base.
    """
    assert row["zoom"] in (1, 2); scale = 1 if row["zoom"] == 2 else 2; pixels, bases = {}, set()
    for domain in row["domains"]:  # Real installed lanes compose separately; electrical source variant1 never aliases shared enum0 or an absent Heat lane.
        variant = domain["variant"]
        for direction, neighbour in enumerate(domain["neighbours"]):
            state = 1 if neighbour and neighbour["owner"] == domain["owner"] else 2
            pose = next(pose for pose in source["sprites"] if (pose["state"], pose["direction"], pose["tile_variant"]) == (family["states"][state], family["directions"][direction], variant))
            ordinal = (state * 4 + direction) * 2 + variant; bases.add(domain["sprites"][direction] - family["packed"]["native_offset"] - ordinal)
            x, y, width, height = pose["box"]; patch = atlas.crop((x, y, x + width, y + height)); anchor = pose["anchor"]
            left, top = 320 + anchor[0] * scale, 180 + (anchor[1] - 16) * scale
            for index, colour in enumerate(patch.get_flattened_data()):
                if colour == 0:
                    continue
                px, py = index % width, index // width
                for dy in range(scale):
                    for dx in range(scale):
                        point = left + px * scale + dx, top + py * scale + dy
                        assert 0 <= point[0] < 640 and 0 <= point[1] < 360
                        pixels[point] = palette[colour]
    assert len(bases) == 1 and pixels
    return pixels, bases.pop()


def inspect(native):
    """Independently validate every issued paid neighbour and actual whole-view repetition/source/body composition.

    @param native Current ordinary paid-world observer report with exact complete saves, native log and producer-bound PNG frames.
    @return Complete real topology, fixed projected source witnesses, whole-frame repetitions and original-pixel body-occlusion evidence.
    """
    raw = (ROOT / native["log"]["path"]).read_text(encoding="utf-8")
    topology = [json.loads(line) for line in re.findall(r"^GEO_CONDUIT_TOPOLOGY (.+)$", raw, re.M)]; assert len(topology) == 1; topology = topology[0]
    frames = [json.loads(line) for line in re.findall(r"^GEO_CONDUIT_FRAME (.+)$", raw, re.M)]; assert frames and topology
    installed = {(row["x"], row["y"], row["layer"], domain["variant"]): (row, domain) for row in topology for domain in row["domains"]}
    assert len({row["tile"] for row in topology}) == len(topology)
    for row in topology:  # Native topology comes from the bounded original shared ledger, never a screenshot, geometric proximity or a draft route.
        assert row["layer"] in (1, 3) and [domain["variant"] for domain in row["domains"]] in ([0], [1], [0, 1])
        for domain in row["domains"]:
            assert domain["variant"] != 0 or row["layer"] == 3
            for direction, (dx, dy) in enumerate(((-1, 0), (0, 1), (1, 0), (0, -1))):
                other = installed.get((row["x"] + dx, row["y"] + dy, row["layer"], domain["variant"]))
                expected = None if other is None else {"id": other[1]["id"], "owner": other[1]["owner"], "tile": other[0]["tile"]}
                assert domain["neighbours"][direction] == expected, (row["tile"], direction)
    public_path = ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"; public = json.loads(public_path.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == "geothermal.conduit"); source_path = ROOT / family["source"]["manifest"]
    source = json.loads(source_path.read_text(encoding="utf-8")); atlas_path = ROOT / source["path"]; assert identity(atlas_path)["sha256"] == family["source"]["sha256"]
    palette_path = ROOT / "src/table/palettes.h"; text = palette_path.read_text(encoding="utf-8").split("static const Palette _palette = {")[1].split("\n};")[0]
    palette = [(0, 0, 0)] + [tuple(map(int, rgb)) for rgb in re.findall(r"M\(\s*(\d+),\s*(\d+),\s*(\d+)\)", text)]; assert len(palette) == 256
    atlas = Image.open(atlas_path); assert atlas.mode == "P"; grid, repetitions, witnesses, bases = {}, [], [], set()
    for row in frames:
        key = row["tile"], row["zoom"], row["mode"]; assert key not in grid; grid[key] = row
        original_row = next(cell for cell in topology if cell["tile"] == row["tile"])
        assert all(row[field] == value for field, value in original_row.items()) and len(row["paths"]) == 2
        first, second = [Path(name).with_suffix(".png") for name in row["paths"]]
        with Image.open(first) as a, Image.open(second) as b:
            assert a.size == b.size == (640, 360) and a.convert("RGB").tobytes() == b.convert("RGB").tobytes()
        repetitions.append({"tile": row["tile"], "layer": row["layer"], "type": row["type"], "zoom": row["zoom"], "mode": row["mode"], "first": identity(first), "second": identity(second), "complete_rgb_exact": True})
    selected = {row["tile"] for row in frames}; assert grid.keys() == {(tile, zoom, mode) for tile in selected for zoom in (1, 2) for mode in ("opaque", "hidden")}
    for tile in sorted(selected):  # Geometry-selected real floors prove exact original branches; native body differences separately prove their ground ordering without repainting or masking a screenshot.
        for zoom in (1, 2):
            row = grid[tile, zoom, "opaque"]; projected, base = source_pixels(row, family, source, atlas, palette); bases.add(base)
            scale = 1 if zoom == 2 else 2
            # The complete original canvas is projected and checked for
            # screenshot clipping above. Its fixed inner floor diamond is
            # the independent visibility witness: neighbouring native floor
            # cells and their branches legitimately overlap outer contacts.
            # This geometry is chosen before reading either native image;
            # no mismatching screenshot pixel is removed or normalized.
            pixels = {point: colour for point, colour in projected.items()
                      if abs((point[0] - 320) // scale) + 2 * abs((point[1] - 180) // scale + 1) < 24}
            assert pixels
            images = {mode: Image.open(Path(grid[tile, zoom, mode]["paths"][0]).with_suffix(".png")).convert("RGB") for mode in ("opaque", "hidden")}
            exact = {mode: sum(image.getpixel(point) == colour for point, colour in pixels.items()) for mode, image in images.items()}
            revealed = [point for point, colour in pixels.items() if images["hidden"].getpixel(point) == colour and images["opaque"].getpixel(point) != colour]
            if row["type"] in (0, 1, 8):
                assert exact["hidden"] == len(pixels), (tile, zoom, row["type"], exact, len(pixels))
            else:
                assert exact["hidden"] > 0, (tile, zoom, "actual native transport/body detail covers every source branch")
            witnesses.append({"tile": tile, "layer": row["layer"], "type": row["type"], "domains": [domain["variant"] for domain in row["domains"]], "zoom": zoom,
                              "complete_original_projected_pixels": len(projected), "fixed_original_floor_witness_pixels": len(pixels), "exact_original_pixels": exact, "body_revealed_original_pixels": len(revealed),
                              "complete_fixed_floor_witness_exact": row["type"] in (0, 1, 8), "native_base": base,
                              "opaque_frame": identity(Path(row["paths"][0]).with_suffix(".png")), "hidden_frame": identity(Path(grid[tile, zoom, "hidden"]["paths"][0]).with_suffix(".png"))})
    assert len(bases) == 1
    return {"topology": topology, "records": frames, "source_witnesses": witnesses, "complete_repeat_frame_pairs": repetitions,
            "source_manifest": identity(source_path), "source_atlas": identity(atlas_path), "palette": identity(palette_path), "public_manifest": identity(public_path)}


def capture(configuration, case, native_report=None):
    """Observe one exact opposite-writer paid source and verify original installed branches at both native zooms.

    @param configuration Current actual native Debug or optimized reader.
    @param case Sealed paid funding/connection/conversion scenario, never a replacement prepared-floor builder.
    @param native_report Optional explicit current same-binary ordinary observation; avoids repeating a diagnostic capture after verifier-only changes.
    @return None; publish a new report only after all exact native/source/owner checks pass.
    """
    selected, scene = paid_original(configuration, case); context = current_run_context("graphics", "geothermal-conduit-raster"); controller_log = None
    if native_report is None:
        script = ROOT / "src/tests/multilayer/utilities/geothermal_window_raster_checks.py"
        process = subprocess.run([sys.executable, str(script), "--config", configuration, "--scene", scene, "--input", str(ROOT / selected["input"]["path"])], cwd=ROOT,
                                 env=dict(os.environ, OTTD_GEO_CONDUIT_CAPTURE="1"), capture_output=True, timeout=110, check=False)
        log = context.logs / f"{configuration}-{case}-controller.log"; log.write_text((process.stdout + process.stderr).decode("utf-8", errors="replace"), encoding="utf-8")
        assert process.returncode == 0, log
        path = Path(process.stdout.decode("utf-8").splitlines()[-1]); controller_log = identity(log)
    else:
        path = ROOT / native_report
    native = json.loads(path.read_text(encoding="utf-8")); cache = {}; verify_art_references(native, cache)
    assert native["configuration"] == configuration and native["input"] == selected["input"] and native["binary"] == identity(ROOT / f"build/{configuration}/openttd_test.exe")
    assert native["case_runs"] == 1 and native["all_complete_native_chunks_equal"] and compare_worlds(native["before"], native["after"]) == 126
    proof = inspect(native); assert identity(ROOT / selected["input"]["path"]) == selected["input"]
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84BC.2; actual original paid geothermal installed branches and native body occlusion,not a new utility/source owner",
              "configuration": configuration, "case": case, "original": selected, "native_report": identity(path), "native": native, **proof,
              "controller": identity(__file__), "controller_log": controller_log, "sources": [identity(ROOT / name) for name in SOURCES],
              "remaining": "Paired-reader/four-target84BC.2 acceptance,84BD,84B.2/84C/84D;whole84.3/master10.7/human/network/operator gates remain pending."}
    output = context.output("reports", f"geothermal-conduit-raster-{configuration}-{case}.json"); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8"); print(output, flush=True)


def accept(focused_reports, raster_reports, build_logs, output):
    """Seal current native selectors, every original paid topology transition and both complete reader/rendering matrices.

    @param focused_reports Exact current Debug/optimized seven-tag native conduit bursts.
    @param raster_reports Ten exact current paid-world reports, one per configuration and selected original scenario.
    @param build_logs Four successful distinct current native game/test target diagnostics.
    @param output Absent immutable acceptance JSON beneath repository logs.
    @return None; stale source/binary, changed pixels/world, missing paid transition or failed native target prevents closure.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    cache, focused, reports = {}, {}, {}; totals = {cfg: {"cases": 0, "assertions": 0} for cfg in ("Debug", "RelWithDebInfo")}
    for name in focused_reports:  # Every actual native tag/source/binary must be current; the earlier selector-only receipt stays an opaque historical checkpoint.
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_art_references(report, cache); cfg = report["configuration"]
        assert cfg in totals and cfg not in focused and tuple(row["case"] for row in report["native"]) == FOCUSED_CASES
        assert report["binary"] == identity(ROOT / f"build/{cfg}/openttd_test.exe")
        for proof in report["native"]:
            count_native(proof, totals[cfg])
        focused[cfg] = {"report": identity(path), **report}
    assert focused.keys() == totals.keys()
    for name in raster_reports:
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_art_references(report, cache); cfg, case = report["configuration"], report["case"]
        assert cfg in totals and case in CASES and (cfg, case) not in reports
        assert report["original"] == paid_original(cfg, case)[0] and report["native"]["binary"] == focused[cfg]["binary"]
        assert compare_worlds(report["native"]["before"], report["native"]["after"]) == 126
        proof = inspect(report["native"]); assert proof["source_witnesses"] == report["source_witnesses"]
        totals[cfg]["cases"] += report["native"]["case_runs"]; totals[cfg]["assertions"] += report["native"]["assertions"]
        reports[cfg, case] = {"report": identity(path), **report}
    assert reports.keys() == {(cfg, case) for cfg in totals for case in CASES} and totals["Debug"] == totals["RelWithDebInfo"]
    transitions, frames = [], []
    for cfg in totals:  # The original paid replacement reconnects five fresh ports, then isolates one Heat port; subsequent source flow cannot restore that removed wire.
        topology = {case: {(row["x"], row["y"], row["layer"], domain["variant"]): domain for row in reports[cfg, case]["topology"] for domain in row["domains"]} for case in CASES}
        funded, ready, flowing, removed, final = [topology[case] for case in CASES]
        assert funded.keys() < ready.keys() and ready == flowing and removed.keys() < ready.keys() and removed == final
        missing = ready.keys() - removed.keys(); assert len(missing) == 1 and next(iter(missing))[3] == 0
        for key in funded:
            assert funded[key]["id"] == ready[key]["id"]
        for key in removed:
            assert removed[key]["id"] == ready[key]["id"]
        added = ready.keys() - funded.keys(); assert len(added) == 5
        assert all(ready[key]["id"] > max(domain["id"] for domain in funded.values()) for key in added)
        key = next(iter(missing))
        witnesses = [row for case in CASES for row in reports[cfg, case]["source_witnesses"]]
        assert {row["layer"] for row in witnesses} == {1, 3}
        assert {(row["layer"], row["type"]) for row in witnesses} >= {(1, 0), (1, 1), (1, 8), (1, 11), (3, 0), (3, 1), (3, 8)}
        assert any(row["domains"] == [0, 1] and row["complete_fixed_floor_witness_exact"] for row in witnesses)
        for layer in (1, 3):
            assert any(row["layer"] == layer and row["type"] == 8 and row["body_revealed_original_pixels"] > 0 for row in witnesses)
        assert any(row["type"] == 11 and row["body_revealed_original_pixels"] > 0 for row in witnesses)
        transitions.append({"configuration": cfg, "funded_surviving_cells": len(funded), "fresh_paid_ports": [{"coordinate": list(key), "id": ready[key]["id"]} for key in sorted(added)],
                            "removed_heat_coordinate": list(key), "terminal_removed_heat_identity": ready[key]["id"], "removed_port_absent_during_other_generator_service": True,
                            "retained_other_cell_identities_exact": True, "repair_boundary": "Actual paid remove/fresh repair/nonreused identity is the isolated topology test;paid original replacement earns five fresh ports before service and retains the later removed Heat port as absent."})
    for case in CASES:  # Both independent ordinary readers preserve every saved owner and the entire corresponding RGB frame, without a colour tolerance or screenshot normalization.
        left, right = reports["Debug", case], reports["RelWithDebInfo", case]; assert left["topology"] == right["topology"]
        for stage in ("before", "after"):
            assert compare_worlds(left["native"][stage], right["native"][stage]) == 126
        first = {(row["tile"], row["zoom"], row["mode"]): row for row in left["complete_repeat_frame_pairs"]}
        second = {(row["tile"], row["zoom"], row["mode"]): row for row in right["complete_repeat_frame_pairs"]}; assert first.keys() == second.keys()
        for key in first:
            a, b = first[key], second[key]
            with Image.open(ROOT / a["first"]["path"]) as image_a, Image.open(ROOT / b["first"]["path"]) as image_b:
                assert image_a.size == image_b.size and image_a.convert("RGB").tobytes() == image_b.convert("RGB").tobytes()
            frames.append({"case": case, "tile": key[0], "zoom": key[1], "mode": key[2], "left": a["first"], "right": b["first"], "complete_rgb_exact": True})
    builds, targets = [], set()
    for name in build_logs:  # Require all four distinct actual target completions; a duplicated log or mere compile line cannot establish a current game binary.
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert all(message not in log for message in ("error C", "fatal error", "error MSB", "warning C", "warning LNK"))
        found = {(cfg, target) for cfg in totals for target in ("openttd", "openttd_test") if f"{target}.vcxproj -> " in log and f"\\{cfg}\\{target}.exe" in log}
        assert len(found) == 1 and not (found & targets); targets.update(found); builds.append(identity(path))
    assert targets == {(cfg, target) for cfg in totals for target in ("openttd", "openttd_test")}
    grf = identity(ROOT / "media/baseset/openttd.grf"); assert grf["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84BC.2; actual paid installed branches,source-defined fixed floor witnesses/native body occlusion,whole-world/render purity;no new utility owner",
              "controller": identity(__file__), "focused": list(focused.values()), "rasters": list(reports.values()), "builds": builds, "summary": totals, "paid_topology_transitions": transitions,
              "native_cases": sum(row["cases"] for row in totals.values()), "native_assertions": sum(row["assertions"] for row in totals.values()), "native_version": 434,
              "complete_cross_reader_world_pairs": len(CASES) * 2, "complete_cross_reader_frame_pairs": frames,
              "complete_repeat_frame_pairs": sum(len(row["complete_repeat_frame_pairs"]) for row in reports.values()),
              "exact_original_floor_pixels": sum(w["exact_original_pixels"]["hidden"] for row in reports.values() for w in row["source_witnesses"]),
              "packed_grf": grf, "exact_identities_verified": len(cache), "binaries": [identity(ROOT / f"build/{cfg}/{target}.exe") for cfg in totals for target in ("openttd", "openttd_test")],
              "visibility_contract": "Original full source canvases project unclipped. Fixed inner native-floor diamond witnesses precede pixel inspection;adjacent native floors/branches legitimately overlap outer contacts. Hidden industry/building bodies reveal exact floor pixels;native portal rails remain visible. No source cropping,painted mask,pixel search or colour tolerance.",
              "remaining": "84BD actual icons,return84B.2,84C current selected gallery,84D publication;whole84.3/master10.7/human/network/operator gates remain pending."}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": totals, "cross_reader_frame_pairs": len(frames), "repeat_pairs": result["complete_repeat_frame_pairs"], "exact_floor_pixels": result["exact_original_floor_pixels"]}), flush=True)


def main():
    """Select one bounded paid-world conduit capture without regenerating its original producer.

    @return None; all selected native/source/geometry/purity boundaries must pass before reporting success.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--phase", choices=("capture", "accept"), default="capture")
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo")); parser.add_argument("--case", choices=CASES); parser.add_argument("--native-report")
    parser.add_argument("--focused-reports", nargs=2); parser.add_argument("--raster-reports", nargs=10); parser.add_argument("--build-logs", nargs=4); parser.add_argument("--output"); args = parser.parse_args()
    if args.phase == "capture":
        assert args.config and args.case; capture(args.config, args.case, args.native_report)
    else:
        assert args.focused_reports and args.raster_reports and args.build_logs and args.output; accept(args.focused_reports, args.raster_reports, args.build_logs, args.output)


if __name__ == "__main__":
    main()
