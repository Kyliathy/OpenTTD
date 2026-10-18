"""Verify original robot/hauler materialization across current ordinary save boundaries.

@file orbital_save_checks-5.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group14.2
Only original paid16/17/21 jobs are loaded. Native completion/retry executes
the existing producer; inherited declared starting content remains outside procurement acceptance.
"""
import argparse
import json
import os
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity, read_stream
from artifact_paths import create_run_context
from launch_center_checks import riff_location

HERE = Path(__file__).parent
MODULE = runpy.run_path(str(HERE / "orbital_save_checks-2.py"))
SELECT = runpy.run_path(str(HERE / "orbital_save_checks-4.py"))
NATIVE = runpy.run_path(str(HERE / "manufacturing_materialization_checks-1.py"))
TABLES = runpy.run_path(str(HERE / "heavy_joined_checks-2.py"))
read, accepted_report = SELECT["read"], SELECT["accepted_report"]


def compare(source, saved, configuration):
    """Compare every owner while qualifying each unstarted legacy Engine row separately.

    @param source Original paid native save path. @param saved Actual current reconstructed/resumed path.
    @param configuration Matching qualified native build. @return Exact owner checks and bounded catalog repair references.
    """
    physical = [family for family in MODULE["PHYSICAL"] if family != b"ENGN"]
    result = MODULE["UPGRADE"]["compare_upgrade"](source, saved, configuration, physical, MODULE["PARSER"]["structures"])
    old_version, before = MODULE["chunks"](source)
    _, after = MODULE["chunks"](saved)
    if before[b"ENGN"] == after[b"ENGN"]:
        result["exact_families"].append("ENGN")
        return result
    assert old_version < 429
    _, producer = SELECT["producer"](configuration, "orbital-processor-verification")
    reference, = [ref for ref in producer["reports"] if ref["path"].endswith("/orbital-processor-inspected.json")]
    proof = accepted_report(reference)
    baseline_old, baseline_new = [ROOT / proof[key]["path"] for key in ("source", "after")]
    assert identity(baseline_old) == proof["source"] and identity(baseline_new) == proof["after"]
    streams = [read_stream(path)[1] for path in (source, saved, baseline_old, baseline_new)]
    catalogs = [{row["index"]: raw[row["payload"]:row["end"]] for row in TABLES["table_rows"](raw, b"ENGN")} for raw in streams]
    left, right, permitted_old, permitted_new = catalogs
    migration_owner, replacement = SELECT["producer"](configuration, "manufacturing-replacement-verification")
    migration_ref, = [ref for ref in replacement["reports"] if "/manufacturing-replacement-verify-main-upgrade-clone-16/" in ref["path"]]
    migration = accepted_report(migration_ref)
    allowed = [(permitted_old, permitted_new)]
    additional = []
    for key in ("source", "after"):
        path = ROOT / migration[key]["path"]
        assert identity(path) == migration[key]
        raw = read_stream(path)[1]
        additional.append({row["index"]: raw[row["payload"]:row["end"]] for row in TABLES["table_rows"](raw, b"ENGN")})
    allowed.append(tuple(additional))
    freight_owner, freight = SELECT["producer"](configuration, "processor-freight-verification")
    freight_ref, = [ref for ref in freight["reports"] if "/processor-freight-native/" in ref["path"] and accepted_report(ref).get("phase") == "inspected"]
    freight_proof = accepted_report(freight_ref)
    freight_catalogs = []
    # The original paid17 freight migration qualifies its own model-dependent
    # initialized row. A model16 catalog cannot supply a different role's Engine
    # defaults even when both products happened to use the same numeric index.
    for key in ("source", "after"):
        path = ROOT / freight_proof[key]["path"]
        assert identity(path) == freight_proof[key]
        raw = read_stream(path)[1]
        freight_catalogs.append({row["index"]: raw[row["payload"]:row["end"]] for row in TABLES["table_rows"](raw, b"ENGN")})
    allowed.append(tuple(freight_catalogs))
    assert set(left) == set(right)
    changed = []
    # A previously initialized product Engine keeps its exact original row.
    # Every changed row must independently equal the already qualified old-Core
    # unstarted row and its exact initialized output; other history is immutable.
    for engine in left:
        if left[engine] == right[engine]:
            continue
        assert any(engine in old and engine in new and left[engine] == old[engine] and right[engine] == new[engine] for old, new in allowed)
        changed.append(engine)
    assert changed
    result["engine_repair"] = dict(report=reference, source=proof["source"], after=proof["after"], exact_repaired_rows=changed,
                                  later_product_migration=dict(index=migration_owner, report=migration_ref, source=migration["source"], after=migration["after"]))
    result["engine_repair"]["hauler_migration"] = dict(index=freight_owner, report=freight_ref, source=freight_proof["source"], after=freight_proof["after"])
    result["native_legacy_upgrades"].append("ENGN: each changed row equals the accepted unstarted catalog repair; initialized rows exact")
    return result


def ordinary_boundary(configuration, source):
    """Reconstruct one paid materialization milestone through the current ordinary product.

    @param configuration Qualified current application. @param source Original immutable paid checkpoint.
    @return Exact native boundary report with all thirty physical owners and metadata compared.
    """
    context = create_run_context("space", "orbital-save-factory-boundary")
    context.prepare_runtime()
    application = identity(ROOT / "build" / configuration / "openttd.exe")
    assert application == read(ROOT / "logs/orbital-art-ordinary-binary-verification.json")["builds"][configuration]["application"]
    previous = os.environ.get("OTTD_TEST_CONFIGURATION")
    os.environ["OTTD_TEST_CONFIGURATION"] = configuration
    try:
        MODULE["ordinary"](context, source, configuration)
    finally:
        if previous is None:
            os.environ.pop("OTTD_TEST_CONFIGURATION", None)
        else:
            os.environ["OTTD_TEST_CONFIGURATION"] = previous
    saved = context.saves / "ordinary-resave.sav"
    comparison = compare(ROOT / source["path"], saved, configuration)
    assert identity(ROOT / source["path"]) == source
    target = context.reports / "orbital-save-factory-boundary.json"
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration, source=source,
                                    after=identity(saved), application=application, comparison=comparison,
                                    terminal=identity(context.logs / "ordinary.log"),
                                    native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))]), indent=2) + "\n", encoding="utf-8")
    return identity(target)


def predecessor(configuration, model, phase):
    """Resolve the exact original paid model's last-work or completed checkpoint.

    @param configuration Matching original producer configuration.
    @param model Retained robot16, hauler17 or heavy miner21. @param phase Original before/complete milestone.
    @return Immutable producer, original report and native save identities.
    """
    name = "heavy-commissioning-verification" if model == 21 else "manufacturing-materialization-verification"
    owner, row = SELECT["producer"](configuration, name)
    directory = "/heavy-commissioning-" + phase + "/" if model == 21 else "/manufacturing-materialization-verify-" + phase + "-" + str(model) + "/"
    reference, = [ref for ref in row["reports"] if directory in ref["path"]]
    proof = accepted_report(reference)
    assert proof["configuration"] == configuration
    assert identity(ROOT / proof["after"]["path"]) == proof["after"]
    return owner, reference, proof["after"]


def snapshot(path):
    """Join an independently decoded factory job with original native hull IDs and physical positions.

    @param path Actual native save. @return Exact factory request, fleet and sole job-to-product receipt.
    """
    version, raw = read_stream(path)
    records = MODULE["PARSER"]["structures"](raw, version)[0]
    (site, factory), = [(site, row) for site, row in records["rows"].items() if row["kind"] == 9]
    job = factory["module"]["manufacture"]
    assert job is not None
    hulls = {}
    # Actual type6 native rows supply physical identity/pose; neither an empty
    # Engine slot nor a saved product reference can stand in for a real hull.
    for row in TABLES["table_rows"](raw, b"VEHS"):
        type_field, = [field for field in row["fields"] if field["path"] == "type"]
        if int.from_bytes(raw[type_field["position"]:type_field["end"]], "big") != 6:
            continue
        pose = {}
        for axis in ("x", "y"):
            field, = [field for field in row["fields"] if field["path"].endswith(".position." + axis)]
            pose[axis] = int.from_bytes(raw[field["position"]:field["end"]], "big", signed=True)
        hulls[row["index"]] = pose
    # The original fixed paid body owns its factory serial. Original jobs here
    # have no replacement intention, so every current record has its real68-byte
    # extent; absent provenance cannot materialize a second product on load.
    _, start, _, end = riff_location(raw, b"SVST")
    if version >= 429:
        assert start + 1 + raw[start] * 68 == end
        products = []
        for position in range(start + 1, end, 68):
            assert raw[position + 67] == 0
            factory_id = int.from_bytes(raw[position + 49:position + 51], "big")
            factory_generation = int.from_bytes(raw[position + 51:position + 59], "big")
            serial = int.from_bytes(raw[position + 59:position + 67], "big")
            if (factory_id, factory_generation, serial) == (site, factory["generation"], job["serial"]):
                products.append(dict(vehicle=int.from_bytes(raw[position:position + 4], "big"),
                                     generation=int.from_bytes(raw[position + 4:position + 12], "big")))
    else:
        products = None  # Original427 is compared after the independently admitted current upgrade.
    return dict(factory=site, factory_generation=factory["generation"], x=factory["x"], y=factory["y"], job=job, hulls=hulls, products=products)


def execute(configuration, model):
    """Round-trip both real milestones, resume the final work and refuse duplicate materialization.

    @param configuration Qualified current product/test configuration.
    @param model Original independently paid native recipe16/17/21.
    @return Current ordinary and resumed receipts with exact hull/berth/material conservation.
    """
    owner, before_report, before = predecessor(configuration, model, "before")
    _, complete_report, complete = predecessor(configuration, model, "complete")
    ordinary_before = ordinary_boundary(configuration, before)
    ordinary_complete = ordinary_boundary(configuration, complete)
    left, right = [snapshot(ROOT / accepted_report(ref)["after"]["path"]) for ref in (ordinary_before, ordinary_complete)]
    job, settled = left["job"], right["job"]
    assert job["model"] == settled["model"] == model
    assert job["serial"] == settled["serial"] and job["berth"] == settled["berth"] == 0
    assert job["phase"] == 2 and settled["phase"] == 4 and settled["work"] == job["work"] + 1
    assert all(quantity == 0 for quantity in job["consumed"])
    assert [slot["quantity"] for slot in job["inputs"]] == settled["consumed"]
    assert all(slot["quantity"] == 0 and not slot["references"] for slot in settled["inputs"])
    product = dict(vehicle=settled["product"], generation=settled["product_generation"])
    assert left["products"] == [] and right["products"] == [product]
    assert set(right["hulls"]) - set(left["hulls"]) == {product["vehicle"]}
    assert set(left["hulls"]) - set(right["hulls"]) == set()
    assert right["hulls"][product["vehicle"]] == dict(x=right["x"] * 65536, y=(right["y"] + 2) * 65536)
    context = create_run_context("space", "orbital-save-materialization")
    expected_binary = read(ROOT / "logs/orbital-art-ordinary-binary-verification.json")["builds"][configuration]["test_application"]
    assert identity(ROOT / expected_binary["path"]) == expected_binary
    reconstructed = accepted_report(ordinary_before)["after"]
    resumed, assertions = NATIVE["native"](context, configuration, "complete", model, ROOT / reconstructed["path"])
    control_context = create_run_context("space", "orbital-save-materialization-control")
    control, control_assertions = NATIVE["native"](control_context, configuration, "complete", model, ROOT / before["path"])
    # Current uninterrupted and save-interrupted materialization must converge
    # byte-for-byte. A newly created current hull has content3 provenance; an
    # already completed original427 hull keeps2. Comparing dated executables'
    # product provenance would incorrectly erase this intentional distinction.
    comparison = compare(control, resumed, configuration)
    assert snapshot(resumed) == right
    repeated_context = create_run_context("space", "orbital-save-materialization-repeat")
    repeated, repeated_assertions = NATIVE["native"](repeated_context, configuration, "repeat", model, resumed)
    retry = snapshot(repeated)
    assert retry["job"] == settled and retry["products"] == [product] and retry["hulls"] == right["hulls"]
    references = [identity(path) for folder in (context, control_context, repeated_context) for path in sorted(folder.reports.glob("native-*.json"))]
    target = context.reports / "orbital-save-materialization.json"
    result = dict(_documentation=__doc__, complete=True, configuration=configuration, model=model, producer=owner,
                  original_before=before_report, original_complete=complete_report, ordinary_before=ordinary_before,
                  ordinary_complete=ordinary_complete, before=left, settled=right, resumed=identity(resumed),
                  repeated=identity(repeated), control=identity(control), assertions=assertions + control_assertions + repeated_assertions, comparison=comparison,
                  provenance_scope="New current products use content3; ordinary restore preserves already completed original content2 hulls.",
                  executable=expected_binary, native_processes=references, initial_content_procurement_acceptance=False,
                  sources=[identity(Path(__file__)), identity(ROOT / "src/tests/manufacturing_materialization_native.cpp")])
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return identity(target)


def main():
    """Run one bounded model/configuration or bind the six completed explicit members."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    parser.add_argument("--model", type=int, choices=(16, 17, 21))
    parser.add_argument("--bind", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    if args.bind:
        results = []
        # Every actual build/model pair must qualify its own physical product;
        # a small robot or another build cannot supply a hauler/heavy identity.
        for configuration, label in (("RelWithDebInfo", "release"), ("Debug", "debug")):
            for model in (16, 17, 21):
                member = ROOT / "logs" / (f"orbital-save-{label}-materialization-{model}-2.json")
                reference = read(member)["report"]
                proof = accepted_report(reference)
                assert proof["complete"] and proof["configuration"] == configuration and proof["model"] == model
                results.append(dict(membership=identity(member), report=reference, **proof))
        result = dict(_documentation=__doc__, complete=True, configurations=results, initial_content_procurement_acceptance=False)
    else:
        assert args.configuration is not None and args.model is not None
        result = dict(_documentation=__doc__, report=execute(args.configuration, args.model))
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(identity(output), flush=True)


if __name__ == "__main__":
    main()
