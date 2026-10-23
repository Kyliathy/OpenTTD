"""Verify exact native tube presentation reports and publish an immutable closure.

@file tube_sprite_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group13B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

from PIL import Image

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from geothermal_tube_binding_files import world_chunks
from geology_generation_fixture import read_stream
from geology_save_fixture import chunks
from local_electrical_feed_checks import identity


def verify_references(value, cache):
    """Verify every retained identity without substituting a later input or binary.

    @param value Report subtree containing source, native log, original input and output identities.
    @param cache Exact current identities already computed for this closure invocation.
    """
    if isinstance(value, dict):
        if {"path", "bytes", "sha256"} <= value.keys():
            path = value["path"]
            if path not in cache:
                cache[path] = identity(ROOT / path)
            assert cache[path] == value, path
        # Independently retain each nested owner, including inputs shared by
        # multiple readers; a passing native count cannot replace its log/hash.
        for nested in value.values():
            verify_references(nested, cache)
    elif isinstance(value, list):
        for nested in value:
            verify_references(nested, cache)


def compare_worlds(first, second):
    """Require every current native chunk to match without normalizing any owner.

    @param first Exact ordinary current native world identity.
    @param second Matching repeated or opposite-build world identity.
    @return Exact number of complete native chunks.
    """
    left, right = world_chunks(ROOT / first["path"]), world_chunks(ROOT / second["path"])
    assert left == right, [name.decode() for name in left if left[name] != right.get(name)]
    assert len(left) == 126
    return len(left)


def compare_reservation_fixture(ordinary, diagnostic):
    """Account for the declared native reservation fixture and require a visible client cue.

    @param ordinary Actual blackout scene without the diagnostic setup.
    @param diagnostic Same original world after native TryReserveRailTrack setup and cue selection.
    @return Exact single-field native map difference and actual visible marker pixel counts.
    """
    original = ROOT / ordinary["before"]["path"]
    reserved = ROOT / diagnostic["before"]["path"]
    before, after = world_chunks(original), world_chunks(reserved)
    assert before.keys() == after.keys()
    changed = [name for name in before if before[name] != after[name]]
    assert changed == [b"MAP2"], changed
    # Native rail_map.h stores Track+1 in m2 bits8..10. map_sl.cpp writes
    # that original U16 in big-endian tile order. Predict the one declared
    # field exactly; never normalize the map, vehicle or quantity transport.
    left = next(row["payload"] for row in chunks(read_stream(original)[1]) if row["name"] == b"MAP2")
    right = next(row["payload"] for row in chunks(read_stream(reserved)[1]) if row["name"] == b"MAP2")
    tile = diagnostic["tile"]
    assert tile == ordinary["tile"]
    assert diagnostic["actual_reserved_tracks"] == 2
    offset = tile * 2
    prior = int.from_bytes(left[offset:offset + 2], "big")
    assert prior & 0x0F00 == 0
    expected = bytearray(left)
    expected[offset:offset + 2] = (prior | 0x0200).to_bytes(2, "big")
    assert bytes(expected) == right, "Native reservation changed more than its declared original tile field."
    pixels = []
    # Compare the physical interior of this tile, beyond adjacent facility
    # ground/roof overlap. A no-op renderer cannot pass solely by toggling the
    # client setting or showing a reservation cue on another nearby tile.
    for zoom, index, scale in ((1, 0, 2), (2, 2, 1)):
        first = Image.open(ROOT / ordinary["frames"][index]["path"]).convert("RGB")
        second = Image.open(ROOT / diagnostic["frames"][index]["path"]).convert("RGB")
        box = (320 - 8 * scale, 180 - 4 * scale, 320 + 8 * scale, 180 + 4 * scale)
        changed_pixels = sum(a != b for a, b in zip(first.crop(box).get_flattened_data(), second.crop(box).get_flattened_data()))
        assert changed_pixels > 0, "Actual native reservation cue is absent from the guideway interior."
        pixels.append({"zoom": zoom, "native_box": list(box), "visible_marker_pixels": changed_pixels})
    return {"original": ordinary["before"], "diagnostic": diagnostic["before"], "unchanged_complete_chunks": 125,
            "changed_chunk": "MAP2", "tile": tile, "prior_m2": prior, "reserved_m2": prior | 0x0200,
            "native_fixture": "TryReserveRailTrack on original Track::Y, not a queued-route persistence claim", "visible_cue": pixels}


def main():
    """Verify explicit focused reports, cross-build worlds/pixels and four build outputs.

    @return None; refusal leaves the requested closure path absent.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reports", nargs="+", required=True, help="Exact fourteen current native report paths.")
    parser.add_argument("--output", required=True, help="New immutable JSON closure beneath repository logs.")
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs")
    assert not output.exists()
    cache, grid, reports = {}, {}, []
    summary = {name: {"cases": 0, "assertions": 0} for name in ("Debug", "RelWithDebInfo")}
    scopes = {(name, 0) for name in ("geometry", "ends", "external", "geothermal", "raster")} | {("raster", 3), ("reservation", 3)}
    # Explicit paths prevent accidentally combining a prior failed/stale
    # matrix with the current binaries, controller or retained original writer.
    for report_path in args.reports:
        path = ROOT / report_path
        report = json.loads(path.read_text(encoding="utf-8"))
        verify_references(report, cache)
        cfg, focus, case = report["configuration"], report["focus"], report["case"]
        key = (cfg, focus, case)
        assert cfg in summary
        assert (focus, case) in scopes
        assert key not in grid
        proof = report["native"]
        log = (ROOT / proof["log"]["path"]).read_text(encoding="utf-8")
        counts = re.search(r"All tests passed \((\d+) assertions in (\d+) test case", log)
        assert counts is not None
        assert (int(counts[1]), int(counts[2])) == (proof["assertions"], proof["cases"])
        assert "NATIVE_EXIT_CODE=0" in log
        summary[cfg]["cases"] += proof["cases"]
        summary[cfg]["assertions"] += proof["assertions"]
        grid[key] = proof
        reports.append({"report": identity(path), **report})
        if focus in ("raster", "reservation"):
            compare_worlds(proof["before"], proof["after"])
    assert set(grid) == {(cfg, focus, case) for cfg in summary for focus, case in scopes}
    world_pairs, frame_pairs, reservations = [], [], []
    # Each same-scene pair uses independent original writer inputs and
    # opposite reader builds. Require complete state and every RGB byte.
    for focus, case in (("raster", 0), ("raster", 3), ("reservation", 3)):
        left, right = grid["Debug", focus, case], grid["RelWithDebInfo", focus, case]
        for boundary in ("before", "after"):
            count = compare_worlds(left[boundary], right[boundary])
            world_pairs.append({"focus": focus, "case": case, "boundary": boundary, "left": left[boundary], "right": right[boundary], "complete_chunks_exact": count})
        assert len(left["frames"]) == len(right["frames"]) == 4
        for first, second in zip(left["frames"], right["frames"]):
            a, b = Image.open(ROOT / first["path"]).convert("RGB"), Image.open(ROOT / second["path"]).convert("RGB")
            assert a.size == b.size == (640, 360)
            assert a.tobytes() == b.tobytes()
            frame_pairs.append({"left": first, "right": second, "rgb_sha256": hashlib.sha256(a.tobytes()).hexdigest(), "pixels_exact": True})
    for cfg in summary:
        reservations.append({"configuration": cfg, **compare_reservation_fixture(grid[cfg, "raster", 3], grid[cfg, "reservation", 3])})
    build_names = ("debug-test-build-10.log", "debug-game-build-2.log", "optimized-test-build-4.log", "optimized-game-build-2.log")
    builds = [identity(output.parent / name) for name in build_names]
    binaries = [identity(ROOT / f"build/{cfg}/{binary}.exe") for cfg in summary for binary in ("openttd", "openttd_test")]
    packed = identity(ROOT / "media/baseset/openttd.grf")
    assert packed["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    result = {"_documentation": "r2mlvs17-underground-transport-tubes.md Group13B; exact native guideway rendering; declared art boundary fixtures are separate from useful-route/queued persistence acceptance",
              "controller": identity(Path(__file__).resolve()), "summary": summary,
              "native_cases": sum(row["cases"] for row in summary.values()), "native_assertions": sum(row["assertions"] for row in summary.values()),
              "reports": reports, "cross_build_worlds": world_pairs, "cross_build_frames": frame_pairs, "reservation_fixtures": reservations,
              "exact_identities_verified": len(cache), "builds": builds, "binaries": binaries, "packed_art": packed, "native_version": 434,
              "game_build_reuse": "Production source/library unchanged after both game builds; final updates rebuild only test observer documentation and declared reservation setup.",
              "observed_fixture_failures": [identity(ROOT / "logs/tubes/guideway-art/20261017-213844-ab57745565/geometry.log"),
                                             identity(ROOT / "logs/tubes/guideway-art/20261017-215046-4c03be6f11/raster-3.log")],
              "remaining": "13C capsule/facility/conduit rendering,13D complete visual/picking acceptance,13E authoritative files,14 lifecycle,15 useful routes; runtime gallery and human feedback pending"}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary, "world_pairs": len(world_pairs), "frame_pairs": len(frame_pairs)}))


if __name__ == "__main__":
    main()
