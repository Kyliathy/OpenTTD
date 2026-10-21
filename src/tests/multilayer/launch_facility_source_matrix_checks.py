"""Qualify and optionally publish complete launch-center indexed source coverage.

@file launch_facility_source_matrix_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group61F.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-z-z-z-z-z-q-launch-center-source.md.
Run with --publish to update only the qualified launch-center public source
fields after checks pass. Source atlas, manifest and verification are rebuilt
under logs/proofs/mlm/native-art/space/launch-center-source-v1. PNG decoding is
source evidence; packed payloads and gameplay presentation remain separate.
"""

import argparse
from copy import deepcopy
import hashlib
import json
import unittest

from PIL import Image

from launch_facility_art_contract_checks import ROOT, PROOFS as GEOMETRY_PROOFS, read_json, verify_native_proof
from launch_facility_state_checks import checked_native, PROOFS as STATE_PROOFS
import convert_launch_center_source as source

FIXTURE = ROOT / "src/tests/multilayer/launch-center-source-fixture.json"
NE_FIXTURE = ROOT / "src/tests/multilayer/launch-center-ne-base-fixture.json"


def signature(value):
    """Compare exact decoded public metadata independently of harmless JSON layout.

    @param value JSON-compatible original public row or envelope.
    @return Stable semantic SHA256.
    """
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def verify_public_handoff(report, reference):
    """Require the current public source handoff without granting packed/runtime activation.

    @param report Qualified complete indexed source manifest.
    @param reference Exact stored source-manifest identity.
    @return None.
    @throws ValueError When public/source-family publication differs from qualified evidence.
    """
    public = read_json(source.PUBLIC)
    row = next(row for row in public["assets"] if row["asset_id"] == source.contract.ASSET)
    family = read_json(source.RECORD)["families"][source.contract.ASSET]
    # The public family owns one source handoff; it still grants no SpriteID,
    # native renderer, unhidden Surface actor or ordinary-game acceptance.
    if (row["source"]["status"] != "authored" or not row["authoring"]["coverage_complete"]
            or row["authoring"]["atlas_manifest"] != reference["path"]
            or row["authoring"]["atlas_manifest_sha256"] != reference["sha256"]
            or row["authoring"]["native_sha256"] != report["atlas"]["sha256"]
            or row["authoring"]["runtime_activated"] or row["packed"]["offset"] is not None
            or family["source_manifest"] != reference or not family["source_coverage"]):
        raise ValueError("launch source publication differs from its qualified handoff")


class LaunchSourceMatrixChecks(unittest.TestCase):
    """Require real coverage, direct native aliases and protected earlier sources/payloads."""

    report = None

    @classmethod
    def setUpClass(cls):
        """Load exact saved atlas and immutable accepted source/preservation expectations.

        @param cls Current focused source-closure test class.
        @return None.
        """
        cls.fixture = read_json(FIXTURE)
        with Image.open(ROOT / cls.report["atlas"]["path"]) as image:
            cls.atlas = image.copy()
        cls.decoded = source.decode(cls.report, cls.atlas)

    def test_all_required_positions_have_exact_native_source_coverage(self):
        """All1536 logical positions retain actual geometry and explicit still aliases.

        @param self Active full source coverage and canonical/alias count check.
        @return None.
        """
        self.assertEqual(len(self.decoded), 1536)
        self.assertEqual(self.report["logical_pose_count"], 1536)
        self.assertEqual(self.report["canonical_part_count"] + self.report["explicit_static_alias_count"], 1536)
        self.assertEqual(self.report["missing_pose_count"], 0)
        self.assertTrue(self.report["coverage_complete"])
        self.assertFalse(self.report["packed"])
        self.assertFalse(self.report["runtime_activated"])
        self.assertEqual(source.base.identity(ROOT / self.report["atlas"]["path"]), self.report["atlas"])

    def test_all_decoded_states_reassemble_frozen_original_pixels(self):
        """Actual decoded atlas pixels reproduce every accepted whole state without drift.

        @param self Active source decoding, exact state ancestry and original reassembly check.
        @return None.
        """
        grouped = {}
        # Check all logical entries, including every declared frame alias.
        # Reconstruct each distinct state once after pixel-identical frame checks.
        for row, image in self.decoded:
            grouped.setdefault((row["direction"], row["state"], row["frame"]), []).append((row, image))
        for direction in source.contract.VIEWS:
            evidence = self.report["evidence"][direction]
            self.assertEqual(evidence["state_sha256"], self.fixture["approved_whole_pixels"][direction])
            reference = self.report["ownership"][direction]["cells"]
            self.assertEqual(source.base.identity(ROOT / reference["path"]), reference)
            cells = (ROOT / reference["path"]).read_bytes()
            front_reference = self.report["ownership"][direction]["foreground"]
            self.assertEqual(source.base.identity(ROOT / front_reference["path"]), front_reference)
            front = (ROOT / front_reference["path"]).read_bytes()
            self.assertEqual(hashlib.sha256(cells).hexdigest(), evidence["original"]["cell_ownership_sha256"])
            for state in source.states.STATES:
                pieces = grouped[direction, state, 0]
                expected = self.fixture["approved_whole_pixels"][direction][state]
                whole = source.occupied.reassemble_state(pieces, cells, front, expected, direction, state)
                self.assertEqual(hashlib.sha256(whole.tobytes()).hexdigest(), expected)
                # Every later logical frame is the exact still, including
                # structurally absent fronts. Equal counts alone are insufficient.
                for frame in (1, 2, 3):
                    for original, alias in zip(pieces, grouped[direction, state, frame]):
                        self.assertEqual(original[1].tobytes(), alias[1].tobytes())
                        self.assertEqual(original[0]["native_sha256"], alias[0]["native_sha256"])

    def test_native_state_geometry_and_original_material_inputs_remain_current(self):
        """Source completeness consumes current native producers and genuine original materials.

        @param self Active cross-producer state/geometry and immutable material ancestry check.
        @return None.
        """
        policy = source.base.bound_json(self.report["policy"])
        model = source.checked_occupied_policy(policy)
        mapping = source.base.bound_json(model["state_mapping"])
        selection = source.base.bound_json(model["selected_bases"])
        checkpoint = source.base.bound_json(model["status_art_checkpoint"])
        for configuration in ("Debug", "RelWithDebInfo"):
            checked_native(read_json(STATE_PROOFS / f"native-{configuration}.json"), mapping)
            verify_native_proof(read_json(GEOMETRY_PROOFS / f"native-{configuration}.json"))
        # Every physical pose comes from a frozen genuine view. Material
        # reuse retains original prompts/references without borrowing other jobs.
        for direction in source.contract.VIEWS:
            self.assertEqual(self.report["evidence"][direction]["base_recipe"], selection["selected_views"][direction])
            for state in source.states.MATERIALS:
                plane, ancestry = source.states.material(checkpoint, direction, state)
                self.assertEqual(ancestry, self.report["evidence"][direction]["materials"][state])

    def test_missing_duplicate_cross_view_and_self_aliases_refuse(self):
        """Aliases cannot fabricate coverage or move an actual cell into another native view.

        @param self Active logical coverage and direct alias corruption refusal check.
        @return None.
        """
        # Isolate each invalid declaration rather than stacking corruptions;
        # otherwise a second failure could conceal a missing alias guard.
        for mutation in ("missing", "duplicate", "cross_view", "self_alias"):
            broken = deepcopy(self.report)
            if mutation == "missing":
                broken["sprites"].pop()
            elif mutation == "duplicate":
                broken["sprites"][-1] = deepcopy(broken["sprites"][0])
            elif mutation == "cross_view":
                row = next(row for row in broken["sprites"] if row["alias_of"] is not None)
                row["alias_of"]["direction"] = "NW" if row["direction"] != "NW" else "NE"
            else:
                row = next(row for row in broken["sprites"] if row["alias_of"] is None)
                row["alias_of"] = source.pose(row)
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                source.decode(broken, self.atlas)

    def test_changed_anchor_opacity_palette_or_source_pixels_refuse(self):
        """A source report cannot bless shifted geometry, invented fronts or modified PNGs.

        @param self Active decoded native geometry/pixel/absence corruption check.
        @return None.
        """
        # Valid-looking PNGs cannot inherit a substituted foreground model or
        # an unqualified completion header while keeping the original atlas.
        for field, value in dict(occupied_foregrounds_complete=False,
                                 foreground_model=dict(self.report["foreground_model"], sha256="0" * 64)).items():
            broken = deepcopy(self.report)
            broken[field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                source.decode(broken, self.atlas)
        for field, value in dict(anchor=[0, 0], native_layout=9, native_origin=[999, 999], structurally_absent=True).items():
            broken = deepcopy(self.report)
            row = next(row for row in broken["sprites"] if row["part"] == "back")
            row[field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                source.decode(broken, self.atlas)
        atlas = self.atlas.copy()
        x, y = next((x, y) for y in range(atlas.height) for x in range(atlas.width) if atlas.getpixel((x, y)))
        atlas.putpixel((x, y), 1 if atlas.getpixel((x, y)) != 1 else 2)
        with self.assertRaises(ValueError):
            source.decode(self.report, atlas)
        atlas = self.atlas.copy()
        palette = atlas.getpalette()
        palette[0] ^= 1
        atlas.putpalette(palette)
        with self.assertRaises(ValueError):
            source.decode(self.report, atlas)

    def test_other_public_families_payload_and_accepted_ne_art_remain_exact(self):
        """One completed family cannot revise earlier sources, namespace or paid native assets.

        @param self Active147-family, immutable NE and published payload preservation check.
        @return None.
        """
        public = read_json(source.PUBLIC)
        self.assertEqual(signature({name: value for name, value in public.items() if name != "assets"}),
                         self.fixture["public_envelope_sha256"])
        other = {row["asset_id"]: signature(row) for row in public["assets"] if row["asset_id"] != source.contract.ASSET}
        self.assertEqual(other, self.fixture["other_public_rows"])
        row = next(row for row in public["assets"] if row["asset_id"] == source.contract.ASSET)
        self.assertEqual({name: value for name, value in row.items() if name not in ("source", "authoring")},
                         {name: value for name, value in self.fixture["launch_before"].items() if name not in ("source", "authoring")})
        # Existing published files stay byte exact. Only this explicitly
        # admitted family's source record may append closure metadata.
        for name, expected in self.fixture["protected_payload"].items():
            if name != source.RECORD.relative_to(ROOT).as_posix():
                self.assertEqual(hashlib.sha256((ROOT / name).read_bytes()).hexdigest(), expected)
        fixture = read_json(NE_FIXTURE)
        for reference in [fixture["recipe"], fixture["component_model"], fixture["rotation_model"]] + fixture["artifacts"]:
            self.assertEqual(source.base.identity(ROOT / reference["path"]), reference)


def main():
    """Qualify full decoded source coverage before optionally publishing this one family.

    @return None; exit nonzero and retain public source metadata on failed checks.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--publish", action="store_true", help="Publish qualified launch-center source fields after checks pass.")
    mode.add_argument("--verify-published", action="store_true", help="Verify saved qualified artifacts and current public handoff without rebuilding or writing source metadata.")
    arguments = parser.parse_args()
    report = read_json(source.OUTPUT / "source-manifest.json") if arguments.verify_published else source.derive()
    LaunchSourceMatrixChecks.report = report
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(LaunchSourceMatrixChecks))
    if not result.wasSuccessful():
        raise SystemExit(1)
    reference = source.base.identity(source.OUTPUT / "source-manifest.json")
    if arguments.publish:
        source.publish(report)
        # Publication is scoped to one family. Recheck protected rows/payloads
        # after the metadata writes, not merely against their pre-write state.
        LaunchSourceMatrixChecks("test_other_public_families_payload_and_accepted_ne_art_remain_exact").test_other_public_families_payload_and_accepted_ne_art_remain_exact()
    if arguments.publish or arguments.verify_published:
        verify_public_handoff(report, reference)
    proof = dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Group61F; complete decoded indexed PNG source, not GRF decoding or game presentation.",
                 status="passed", cases=result.testsRun, source_manifest=reference,
                 public_source_published=arguments.publish or arguments.verify_published,
                 public_manifest=source.base.identity(source.PUBLIC), source_record=source.base.identity(source.RECORD),
                 fixture=source.base.identity(FIXTURE), policy=report["policy"], atlas=report["atlas"],
                 test=source.base.identity(ROOT / "src/tests/multilayer/launch_facility_source_matrix_checks.py"),
                 converter=source.base.identity(ROOT / "media/multilayer/convert_launch_center_source.py"),
                 helper=source.base.identity(ROOT / "media/multilayer/tools/launch_facility_states.py"),
                 occupied_helper=source.base.identity(ROOT / "media/multilayer/tools/launch_facility_foregrounds.py"),
                 occupied_foregrounds_complete=True,
                 logical_pose_count=1536, canonical_part_count=report["canonical_part_count"],
                 explicit_static_alias_count=report["explicit_static_alias_count"],
                 source_coverage=True, packed=False, runtime=False)
    source.write_json(source.OUTPUT / "verification.json", proof)


if __name__ == "__main__":
    main()
