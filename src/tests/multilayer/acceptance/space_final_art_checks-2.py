"""Qualify ordinary-game final Space screenshots against exact packed source pixels.

@file space_final_art_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BH
Matching observes original indexed screenshot pixels. Transparency leaves the
actual native background visible; it never authorizes another rendered pose.
"""
import argparse
import hashlib
import json
from pathlib import Path
import runpy

from PIL import Image
from space_preview_checks import ROOT, identity

LEAF = runpy.run_path(str(Path(__file__).with_name("space_final_art_checks-1.py")))
EXPECTED = {
    "selection": ("vehicle.model-15", "full"),
    "scan": ("vehicle.model-19", "scanning"),
    "known": ("asteroid.nickel", "free"),
    "construction": ("vehicle.model-20", "empty"),
    "mining": ("vehicle.model-15", "mining"),
    "pod": ("vehicle.model-18", "descending"),
    "attachment": ("vehicle.model-15", "attached"),
    "nickel": ("asteroid.nickel", None),
    "ice": ("asteroid.ice", None),
    "rich": ("asteroid.rare-earth", None),
}


def positions(native, sprite):
    """Locate an actual native sprite by every opaque source palette index.

    @param native Original complete indexed engine screenshot.
    @param sprite Exact cropped indexed packed sheet cell, with index0 transparency.
    @return Native top-left positions where all original opaque pixels match.
    """
    assert native.mode == sprite.mode == "P"
    data, pixels = native.tobytes(), sprite.tobytes()
    width, height = native.size
    sw, sh = sprite.size
    runs = []
    # A longest opaque row cheaply identifies candidates in the complete native field; every candidate still requires all opaque source pixels to agree.
    for y in range(sh):
        x = 0
        while x < sw:
            if pixels[y * sw + x] == 0:
                x += 1
                continue
            end = x + 1
            while end < sw and pixels[y * sw + end] != 0:
                end += 1
            runs.append((pixels[y * sw + x:y * sw + end], x, y))
            x = end
    run, rx, ry = max(runs, key=lambda entry: len(entry[0]))
    opaque = [(offset // sw, offset % sw, value) for offset, value in enumerate(pixels) if value]
    matches = []
    start = data.find(run)
    # Candidate rows cannot wrap around the screenshot boundary; transparent source pixels deliberately retain the world's actual background and neighbours.
    while start >= 0:
        x, y = start % width - rx, start // width - ry
        if 0 <= x <= width - sw and 0 <= y <= height - sh:
            if all(data[(y + py) * width + x + px] == value for py, px, value in opaque):
                matches.append([x, y, sw, sh])
        start = data.find(run, start + 1)
    return matches


def native_matches(native, manifest, sheet, asset, state):
    """Bind visible native pixels to one required final family and genuine state.

    @param native Exact indexed screenshot written by the actual engine.
    @param manifest Accepted packed pose/range/source identities.
    @param sheet Original current indexed packed sheet.
    @param asset Required actual family, never a preview substitute.
    @param state Required authored state, or None for actual natural-body class coverage.
    @return All exact native placements, retaining explicit aliases without claiming distinct frames.
    """
    matches = []
    # Only this actual required family/state qualifies the view. Static aliases may share pixels, but cannot substitute another family or an unearned work state.
    for row in manifest["sprites"]:
        if row["asset_id"] != asset or (state is not None and row["state"] != state):
            continue
        x, y, width, height = row["box"]
        sprite = sheet.crop((x, y, x + width, y + height))
        assert hashlib.sha256(sprite.tobytes()).hexdigest() == row["pixel_sha256"]
        for box in positions(native, sprite):
            matches.append(dict(native_offset=row["native_offset"], asset_id=asset, state=row["state"],
                                frame=row["frame"], direction=row["direction"], tile_variant=row["tile_variant"],
                                native_box=box, source_box=row["box"], source_pixel_sha256=row["pixel_sha256"]))
    return matches


def qualify(selector, configuration, manifest, sheet):
    """Qualify exact ordinary captures, paid ancestry and expected visible final sprites.

    @param selector Explicit complete capture-membership file.
    @param configuration Matching independently built ordinary game.
    @param manifest Accepted final native ranges and original packed sheet cells.
    @param sheet Exact current indexed final source sheet.
    @return Original current reports with independently verified native matches.
    """
    selected = LEAF["read"](selector)
    assert selected["configuration"] == configuration
    records = []
    seen = set()
    # Every declared state/view has one exclusive current native process and an immutable accepted gameplay source; no missing state can inherit another capture.
    for relative in selected["selectors"]:
        membership = LEAF["read"](ROOT / relative)
        path = ROOT / membership["report"]["path"]
        assert identity(path) == membership["report"]
        proof = LEAF["read"](path)
        assert proof["complete"] and proof["configuration"] == configuration
        key = proof["phase"], proof["view"]
        assert key not in seen
        seen.add(key)
        assert (proof["dated_producer"], proof["source"]) == LEAF["predecessor"](configuration, proof["phase"])
        assert not any(proof[field] for field in ("added_stock", "granted_work", "granted_pose"))
        references = {}
        LEAF["AUDIT"]["inspect"](proof, references)
        LEAF["AUDIT"]["compare"](proof["source"], proof["after"])
        process = LEAF["read"](ROOT / proof["native_process"]["path"])
        assert process["original_application"] == identity(ROOT / "build" / configuration / "openttd.exe")
        assert proof["capture"] in process["artifacts"] and proof["after"] in process["artifacts"]
        matches = []
        with Image.open(ROOT / proof["capture"]["path"]) as native:
            assert list(native.size) == proof["dimensions"]
            assert hashlib.sha256(native.convert("RGB").tobytes()).hexdigest() == proof["rgb_pixel_sha256"]
            if proof["view"] == "field":
                asset, state = EXPECTED[proof["phase"]]
                matches = native_matches(native, manifest, sheet, asset, state)
                assert matches, (configuration, proof["phase"], "required final native sprite not visible", proof["capture"])
        records.append(dict(report=identity(path), **proof, exact_native_matches=matches))
    expected = {(phase, "field") for phase in EXPECTED} | {(phase, "inspector") for phase in list(EXPECTED)[:6]}
    assert seen == expected
    return records


def qualify_live(selector, configuration, manifest, sheet):
    """Verify actual ordinary construction progress independently of paused-load review.

    @param selector Explicit one-process live construction capture membership.
    @param configuration Actual independently built ordinary game.
    @param manifest Accepted final pose/source/range identities.
    @param sheet Exact current indexed final source sheet.
    @return Complete live record, real earned output and exact constructing pixels.
    """
    member = LEAF["read"](selector)
    path = ROOT / member["report"]["path"]
    assert identity(path) == member["report"]
    proof = LEAF["read"](path)
    assert proof["complete"] and proof["configuration"] == configuration
    assert proof["phase"] == "construction" and proof["view"] == "field" and proof["captured_live"]
    assert (proof["dated_producer"], proof["source"]) == LEAF["predecessor"](configuration, "construction")
    LEAF["AUDIT"]["inspect"](proof, {})
    LEAF["AUDIT"]["compare"](proof["source"], proof["after"])
    process = LEAF["read"](ROOT / proof["native_process"]["path"])
    assert process["original_application"] == identity(ROOT / "build" / configuration / "openttd.exe")
    assert proof["capture"] in process["artifacts"] and proof["earned_after"] in process["artifacts"]
    before = LEAF["AUDIT"]["chunks"](ROOT / proof["after"]["path"])[1]
    after = LEAF["AUDIT"]["chunks"](ROOT / proof["earned_after"]["path"])[1]
    assert before[b"SPST"] != after[b"SPST"], "ordinary loop earned no native construction progress"
    assert not any(proof[field] for field in ("added_stock", "granted_work", "granted_pose"))
    commands = proof["live_console_commands"]
    assert len(commands) == 3 and commands[0] == "pause"
    assert commands[1].startswith('save "') and commands[2] == 'screenshot giant "space-final-construction"'
    with Image.open(ROOT / proof["capture"]["path"]) as native:
        assert hashlib.sha256(native.convert("RGB").tobytes()).hexdigest() == proof["rgb_pixel_sha256"]
        matches = native_matches(native, manifest, sheet, "vehicle.model-20", "constructing")
    assert matches, (configuration, "genuine native constructing pixels absent")
    return dict(report=identity(path), **proof, exact_native_matches=matches)


def main():
    """Freeze both ordinary-game configurations only after original owners and exact final pixels agree."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--release", type=Path, required=True)
    parser.add_argument("--debug", type=Path, required=True)
    parser.add_argument("--live-release", type=Path, required=True)
    parser.add_argument("--live-debug", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    manifest_path = ROOT / "logs/proofs/mlm/native-art/space/packed-space-manifest.json"
    manifest = LEAF["read"](manifest_path)
    sheet_path = ROOT / "media/baseset/openttd/space_final.png"
    sheet = Image.open(sheet_path)
    release = qualify(args.release, "RelWithDebInfo", manifest, sheet)
    debug = qualify(args.debug, "Debug", manifest, sheet)
    live = [qualify_live(args.live_release, "RelWithDebInfo", manifest, sheet), qualify_live(args.live_debug, "Debug", manifest, sheet)]
    debug_rows = {(row["phase"], row["view"]): row for row in debug}
    # Identical physical state must produce identical final game pixels across configurations, including the actual ordinary status/selection windows.
    for row in release:
        peer = debug_rows[row["phase"], row["view"]]
        assert row["packed_acceptance"] == peer["packed_acceptance"] == release[0]["packed_acceptance"]
        LEAF["AUDIT"]["compare"](row["after"], peer["after"])
        assert row["rgb_pixel_sha256"] == peer["rgb_pixel_sha256"], (row["phase"], row["view"], "configuration pixels differ")
        assert row["exact_native_matches"] == peer["exact_native_matches"]
    result = dict(_documentation=__doc__, complete=True, configurations=[dict(configuration="RelWithDebInfo", captures=release), dict(configuration="Debug", captures=debug)],
                  manifest=identity(manifest_path), sheet=identity(sheet_path), packed_acceptance=release[0]["packed_acceptance"],
                  sources=[identity(Path(__file__)), identity(Path(__file__).with_name("space_final_art_checks-1.py"))],
                  live_construction=live, ordinary_native_captures=34, paused_load_captures=32,
                  required_field_states=10, exact_cross_configuration_pixel_comparisons=16,
                  physical_owner_families=30, metadata_families=2, added_stock=False, granted_work=False, granted_pose=False,
                  visual_review_complete=False, personal_review_complete=False)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(identity(args.output.resolve()), flush=True)


if __name__ == "__main__":
    main()
