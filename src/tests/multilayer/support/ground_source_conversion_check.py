"""Verify one actual Ground material or paired fleet source against independent facts.

@file ground_source_conversion_check.py Each explicit family is a focused burst.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Groups17DB-17DDC.
No generated candidate becomes an expected baseline or a current native proof.
"""

import argparse
import copy
import json
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

import PIL
from artifact_paths import RunContext, create_run_context, repository_root
from source_input_guard import no_old_proofs

ROOT = repository_root()
sys.path.insert(0, str(ROOT / "media/multilayer"))
import regenerate_ground_sources as source
from evidence import identity

ASSET = None
RUN_ID = None


def context_for(run_id):
    """Reserve or join the explicit shared source-check invocation.

    @param run_id Exact primary, repeat or refusal invocation identity.
    @return Context owning this family's generated images, data and reports.
    """
    scenario = "source-" + ASSET.replace(".", "-").lower()
    if not (ROOT / "mlm/runtime/.runs" / run_id).is_dir():
        return create_run_context("multilayer-art", scenario, run_id=run_id)
    return RunContext(ROOT, "multilayer-art", scenario, run_id)


def reusable_source(context):
    """Prepare only actual current source prerequisites selected by this original producer.

    @param context Explicit Ground source invocation sharing reconstructed reusable parts.
    @return Exact current M1/conduit prerequisite reports; never an old cache or another run.
    """
    if ASSET == "facility.utility-riser":
        donor = RunContext(ROOT, context.feature, "source-geothermal-conduit", context.run_id)
        path = donor.reports / "derived.json"
        if not path.is_file():
            source.produce_source("geothermal.conduit", donor)
        return {"conduit-source": identity(path)}
    if ASSET == "geothermal.vent":
        stage = "hardness"
    elif ASSET in source.STATIONS or ASSET in source.TUBE_INFRA or ASSET in source.ENERGY:
        # These pure owners reconstruct only original media. Their M1-derived
        # service model is explicit original reuse, with no cached prerequisite.
        return {}
    else:
        stage = "base" if ASSET.startswith("vehicle.") else "facility"
    donor = RunContext(ROOT, context.feature, "m1-source-" + stage, context.run_id)
    path = donor.reports / "derived.json"
    if not path.is_file():
        source.shared.produce(stage, donor)
    return {stage + "-source": identity(path)}


class GroundSourceTests(unittest.TestCase):
    """Exercise genuine source producers without reading old proofs or publishing media."""

    def test_actual_source_repeats_independent_pixels_and_poses(self):
        """Run the original source/guide producer twice with old proof access blocked.

        @param self Current focused material-source test instance.
        @return None after independent original pixels, poses and repeated images agree.
        """
        context, repeat = context_for(RUN_ID), context_for(RUN_ID + "-repeat")
        with no_old_proofs(ROOT):
            first = source.produce_source(ASSET, context, dependencies=reusable_source(context))
            second = source.produce_source(ASSET, repeat, dependencies=reusable_source(repeat))
        expected = source.load_facts()["source_pose_contracts"][ASSET]
        self.assertEqual(first["native_source"]["sha256"], expected["original_atlas"]["sha256"])
        self.assertEqual(first["pose_count"], expected["pose_count"])
        required = {ASSET, ASSET.replace("vehicle.", "purchase.", 1)} if ASSET.startswith("vehicle.") else {ASSET}
        if ASSET in source.CONTROLS:
            required = set(source.CONTROLS)
        self.assertEqual(set(first["family_pose_counts"]), required)
        # A fleet body and its purchase preview share a producer, while each
        # retains an independent original physical matrix expectation.
        for asset, count in first["family_pose_counts"].items():
            self.assertEqual(count, source.load_facts()["source_pose_contracts"][asset]["pose_count"])
        original = {path.name: identity(path)["sha256"] for path in context.images.iterdir()}
        repeated = {path.name: identity(path)["sha256"] for path in repeat.images.iterdir()}
        self.assertEqual(original, repeated)
        self.assertFalse(first["native_proof"])
        self.assertFalse(first["publishes_source"])
        foreign = context_for(RUN_ID + "-foreign")
        with self.assertRaises(ValueError):
            source.selected_path(first["manifest"], foreign, "reports")
        summary = dict(_documentation=__doc__, asset_id=ASSET, run_id=RUN_ID, manifest=first["manifest"],
                       exact_native_pixels=True, exact_physical_poses=True, exact_repeat=True,
                       old_proof_io_refused=True, generated_images=len(original), native_proof=False)
        output = ROOT / "test-cache/cleanup" / ("ground-source-" + ASSET.replace(".", "-").lower() + "-verification.json")
        output.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")

    def test_encoder_original_and_historical_geometry_refusals(self):
        """Refuse altered inputs/tools and prevent historical art from qualifying native code.

        @param self Current focused material-source test instance.
        @return None; all mutations are detached or mocked, leaving originals intact.
        """
        with patch.object(PIL, "__version__", "0.0-unit-mismatch"):
            with self.assertRaisesRegex(ValueError, "toolchain"):
                source.read_scope()
        owner = source.ground_mine_contract if ASSET.startswith("mine.") else source.ground_processor_contract
        if ASSET in source.STATIONS:
            owner = source.ground_station_contract if ASSET == "facility.rail-station" else source.ground_terminal_contract
        original = json.loads(owner.DESCRIPTOR.read_text(encoding="utf-8"))
        changed = copy.deepcopy(original)
        if ASSET in source.STATIONS:
            changed["layouts"][0]["graphics"] = 999
        else:
            changed["families"][0]["cells"][0]["cell"] = [9, 9]
        with no_old_proofs(ROOT):
            with self.assertRaisesRegex(ValueError, "original native descriptor|original station source descriptor"):
                owner.validate_geometry(changed, source_only=True)
        # Source-only mode is scoped and restored even when a converter fails;
        # strict native defaults cannot inherit this historical source authority.
        reader = owner.load_geometry
        with self.assertRaisesRegex(RuntimeError, "test failure"):
            with source.historical_geometry(asset=ASSET, related=source.related_converters(source.converter(ASSET)[0])):
                owner.load_geometry()
                raise RuntimeError("test failure")
        self.assertIs(owner.load_geometry, reader)
        facts = source.load_facts()
        changed = copy.deepcopy(facts)
        changed["selected_public_facts"][ASSET]["source"]["sha256"] = "0" * 64
        with patch("ground_source_facts.load_facts", return_value=changed), no_old_proofs(ROOT):
            with self.assertRaisesRegex(ValueError, "source contract changed"):
                source.verify_facts()
        module, record_path = source.converter(ASSET)
        record = source.read_record(record_path)
        changed_record = copy.deepcopy(record)
        # Alter only the copied original identities. The actual producer must
        # reject its original pixel ancestry before publishing a current report.
        for candidate in changed_record["candidates"]:
            candidate["sha256"] = "0" * 64
        primary = context_for(RUN_ID)
        context = RunContext(ROOT, primary.feature, primary.scenario + "-fault", primary.run_id)
        parent_reader = Path.read_text

        def changed_service_original(path, *args, **kwargs):
            """Inject a detached hash fault into the actual reused M1 service original.

            @param path Actual source path requested by the original pure producer.
            @param args Original read options passed through unchanged.
            @param kwargs Original encoding/error options passed through unchanged.
            @return Actual source text, except this one explicitly selected in-memory fault.
            """
            text = parent_reader(path, *args, **kwargs)
            if path == module.facilities.RECORD:
                parent = json.loads(text)
                # Service source comes from this exact M1 model. Mutating an
                # unused tube candidate would not test its real source ancestry.
                for family in parent["families"]:
                    family["sha256"] = "0" * 64
                return json.dumps(parent)
            return text

        with no_old_proofs(ROOT):
            dependencies = reusable_source(context)
            service_reader = changed_service_original if ASSET == "facility.tube-service" else parent_reader
            with patch.object(source, "read_record", return_value=changed_record), patch.object(Path, "read_text", service_reader):
                with self.assertRaisesRegex(ValueError, "original|source.*changed|identity|ancestry changed"):
                    source.produce_source(ASSET, context, dependencies=dependencies)
        self.assertFalse((context.reports / "derived.json").exists())

    def test_selected_physical_contract_and_input_refusals(self):
        """Refuse coherent metadata drift, missing inputs and duplicated paired atlases.

        @param self Current focused fleet check; all faults use disposable generated copies.
        @return None after independent physical expectations reject edited current reports.
        """
        # Conventional facilities have signed native compensation and relative
        # glass children. A coherent anchor edit must fail independently of PNGs.
        if ASSET in source.STATIONS:
            context = context_for(RUN_ID)
            report = json.loads((context.reports / "derived.json").read_text(encoding="utf-8"))
            family = next(row for row in json.loads(source.handoff.PUBLIC.read_text(encoding="utf-8"))["assets"] if row["asset_id"] == ASSET)
            changed = copy.deepcopy(report)
            row = next(row for row in changed["sprites"] if not row["structurally_absent"])
            row["native_packed_anchor"][0] += 1
            path = context.reports / "fault-native-anchor.json"
            source.shared.write_document(path, changed)
            with no_old_proofs(ROOT):
                with self.assertRaisesRegex(ValueError, "independent original physical poses"):
                    source.decode_source(family, source.read_scope(), source.load_facts(), changed, identity(path))
            return
        if ASSET in source.TUBE_INFRA or ASSET in source.ENERGY:
            if ASSET == "facility.utility-riser":
                module, _record = source.converter(ASSET)
                with self.assertRaisesRegex(ValueError, "both selected conduit"):
                    module.geometry_contact(None, [], conduit_report={})
            context = context_for(RUN_ID)
            report = json.loads((context.reports / "derived.json").read_text(encoding="utf-8"))
            family = next(row for row in json.loads(source.handoff.PUBLIC.read_text(encoding="utf-8"))["assets"] if row["asset_id"] == ASSET)
            changed = copy.deepcopy(report)
            rows = changed["atlases"][0]["sprites"] if "atlases" in changed else changed["sprites"]
            rows[0]["alias_of"] = {"state": "foreign-topology"}
            path = context.reports / "fault-physical-topology.json"
            source.shared.write_document(path, changed)
            with no_old_proofs(ROOT):
                with self.assertRaisesRegex(ValueError, "independent original physical poses"):
                    source.decode_source(family, source.read_scope(), source.load_facts(), changed, identity(path))
                if ASSET in source.load_facts()["derived_source_inputs"]:
                    facts = copy.deepcopy(source.load_facts())
                    facts["derived_source_inputs"][ASSET]["historical_derived_source"]["sha256"] = "0" * 64
                    with self.assertRaisesRegex(ValueError, "historical generated-source role"):
                        source.decode_source(family, source.read_scope(), facts, report, report["raw_source_report"])
            return
        if not ASSET.startswith("vehicle."):
            self.skipTest("paired atlas-list physical faults cover fleet source producers")
        context = context_for(RUN_ID)
        report = json.loads((context.reports / "derived.json").read_text(encoding="utf-8"))
        family = next(row for row in json.loads(source.handoff.PUBLIC.read_text(encoding="utf-8"))["assets"] if row["asset_id"] == ASSET)
        with no_old_proofs(ROOT):
            changed = copy.deepcopy(report)
            changed["atlases"].append(copy.deepcopy(changed["atlases"][0]))
            with self.assertRaisesRegex(ValueError, "duplicate native atlas"):
                source.native_references(changed)
            changed = copy.deepcopy(report)
            changed["atlases"][0]["sprites"][0]["alias_of"] = {"state": "foreign-load"}
            path = context.reports / "fault-physical-alias.json"
            source.shared.write_document(path, changed)
            with self.assertRaisesRegex(ValueError, "independent original physical poses"):
                source.decode_source(family, source.read_scope(), source.load_facts(), changed, identity(path))
            native = dict(report["native_source"], sha256="0" * 64)
            with self.assertRaisesRegex(ValueError, "identity changed"):
                source.selected_path(native, context, "images")
            native = dict(report["native_source"])
            native["path"] = str(Path(native["path"]).with_name("absent-source.png"))
            with self.assertRaises(FileNotFoundError):
                source.selected_path(native, context, "images")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--asset", required=True)
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--case", choices=("all", "refusals", "contract"), default="all")
    arguments = parser.parse_args()
    ASSET, RUN_ID = arguments.asset, arguments.run_id
    selected = [__file__]
    if arguments.case == "refusals":
        selected.append("GroundSourceTests.test_encoder_original_and_historical_geometry_refusals")
    if arguments.case == "contract":
        selected.append("GroundSourceTests.test_selected_physical_contract_and_input_refusals")
    unittest.main(argv=selected, verbosity=2)
