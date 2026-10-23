"""Accept exact current native conduit selector boundaries without promoting unverified runtime-gallery metadata.

@file geothermal_conduit_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group84BC.1.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Actual paid-world branch/domain/occlusion pixels and four-target integration remain84BC.2.
"""
import argparse
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
from geothermal_conduit_checks import ROOT, CASES, identity
from geothermal_machinery_raster import verify_art_references
from mole_useful_raster_report import count_native


def accept(reports, build_logs, output):
    """Seal both current actual native topology/owner selector bursts and the unchanged approved physical sprite contract.

    @param reports Exact Debug/optimized current isolated native conduit reports.
    @param build_logs Successful current native test-target build diagnostics in those configurations.
    @param output Absent immutable selector-only acceptance receipt beneath repository logs.
    @return None; stale binary/source identities, missing tags or an altered source/payload contract prevent closure.
    """
    output = (ROOT / output).resolve(); assert output.is_relative_to(ROOT / "logs") and not output.exists()
    cache, records = {}, {}; counts = {cfg: {"cases": 0, "assertions": 0} for cfg in ("Debug", "RelWithDebInfo")}
    for name in reports:  # Each current actual native tag/binary/source is checked freshly; no dated generator/riser receipt is reinterpreted as current conduit evidence.
        path = ROOT / name; report = json.loads(path.read_text(encoding="utf-8")); verify_art_references(report, cache)
        cfg = report["configuration"]; assert cfg in counts and cfg not in records and tuple(row["case"] for row in report["native"]) == CASES
        assert report["binary"] == identity(ROOT / f"build/{cfg}/openttd_test.exe")
        for proof in report["native"]:
            count_native(proof, counts[cfg])
        records[cfg] = {"report": identity(path), **report}
    assert records.keys() == counts.keys() and counts["Debug"] == counts["RelWithDebInfo"]
    public_path = ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json"; public = json.loads(public_path.read_text(encoding="utf-8"))
    family = next(row for row in public["assets"] if row["asset_id"] == "geothermal.conduit")
    assert family["directions"] == ["NE", "SE", "SW", "NW"] and family["states"] == ["construction", "connected", "unconnected"]
    assert family["native_domains"] == ["Heat", "Electricity"] and family["frames"] == 1 and family["tile_variants"] == 2 and family["parts"] == ["body"]
    assert family["packed"]["native_offset"] == 17823 and family["packed"]["sprite_count"] == 24
    atlas = identity(ROOT / family["source"]["path"]); assert atlas["sha256"] == family["source"]["sha256"]
    grf = identity(ROOT / family["packed"]["payload"]); assert grf["sha256"] == family["packed"]["sha256"] == "6d3d2dc0e9e8ede6c07931a6fc446573af48f90fca5d65122e252ea827efc92a"
    builds, configurations = [], set()
    for name in build_logs:  # Both final test targets, not a prior successful source compile or a duplicated log, must reach their own executable.
        path = ROOT / name; raw = path.read_bytes(); log = raw.decode("utf-16" if raw.startswith(b"\xff\xfe") else "utf-8")
        assert "error C" not in log and "fatal error" not in log and "error MSB" not in log and "warning C" not in log and "warning LNK" not in log
        found = {cfg for cfg in counts if "openttd_test.vcxproj -> " in log and f"\\{cfg}\\openttd_test.exe" in log}
        assert len(found) == 1 and not (found & configurations); configurations.update(found); builds.append(identity(path))
    assert configurations == counts.keys()
    result = {"_documentation": "r2mlvs14-remaining-assets-and-runtime-integration.md84BC.1; current native installed-domain selector/callback boundaries only,not paid pixel/gallery acceptance",
              "controller": identity(__file__), "records": list(records.values()), "summary": counts, "builds": builds,
              "native_cases": sum(row["cases"] for row in counts.values()), "native_assertions": sum(row["assertions"] for row in counts.values()),
              "public_manifest": identity(public_path), "source_manifest": identity(ROOT / family["source"]["manifest"]), "source_atlas": atlas, "packed_grf": grf,
              "native_version": 434, "exact_identities_verified": len(cache),
              "remaining": "84BC.2 actual paid-source branch/domain/removal/layer/ground-occlusion pixels and four-target acceptance,84BD icons,84B.2/84C/84D;full84.3/master10.7/human/network/operator gates remain pending."}
    output.parent.mkdir(parents=True, exist_ok=True); output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": counts}), flush=True)


def main():
    """Accept one complete current two-configuration conduit selector burst.

    @return None; both exact native reports/build logs and a new output destination are required.
    """
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--reports", nargs=2, required=True)
    parser.add_argument("--build-logs", nargs=2, required=True); parser.add_argument("--output", required=True); args = parser.parse_args(); accept(args.reports, args.build_logs, args.output)


if __name__ == "__main__":
    main()
