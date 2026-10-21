"""Bind launch-center source-state mapping to its actual native receipt owners.

@file launch_facility_state_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group61E.1.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-z-z-z-z-z-q-launch-center-source.md.
Default execution checks current producer receipts read-only. Pass --native to
rerun only the seven pure native logistics/flight/descent contract cases in both
existing builds. No saved world is loaded or advanced. Rebuildable proof JSONs
live under logs/proofs/mlm/native-art/space/launch-center-state-v1; native output
logs live under logs/launch-center-state-native-<configuration>.log.
"""

import argparse
from copy import deepcopy
import json
import re
import subprocess
import unittest

from launch_facility_art_contract_checks import ROOT, PROOFS as GEOMETRY_PROOFS, read_json, verify_native_proof
from tools import launch_facility_art_contract as contract
from tools import launch_facility_base as base

MODEL = ROOT / "media/multilayer/source/space/launch-center-state-model-v1.json"
PROOFS = ROOT / "logs/proofs/mlm/native-art/space/launch-center-state-v1"
NATIVE_ARGUMENTS = ["[space-logistics-contract]", "--reporter", "console", "--durations", "yes"]


def checked_native(proof, model):
    """Require current real executables, source inputs and native terminal results.

    @param proof Explicit Debug or RelWithDebInfo native contract receipt.
    @param model Curated source-state mapping with exact native owner identities.
    @return None.
    @throws ValueError On stale inputs, changed native scope or missing success evidence.
    """
    configuration = proof["configuration"]
    # These pure cases qualify receipt/phase/custody boundaries. Paid-game
    # captures or another test filter cannot silently replace that narrow scope.
    if (configuration not in ("Debug", "RelWithDebInfo") or not proof["complete"]
            or proof["arguments"] != NATIVE_ARGUMENTS or proof["assertions"] != 183
            or proof["cases"] != 7 or proof["paid_world_loaded"] or proof["simulation_ticks"] != 0
            or proof["application"]["path"] != f"build/{configuration}/openttd_test.exe"
            or proof["sources"] != model["native_inputs"]):
        raise ValueError("launch state producer scope changed")
    # Every exact input is observable. A current-looking log alone cannot
    # bless changed code, another executable or an edited native result.
    for reference in [proof["application"], proof["terminal"]] + proof["sources"]:
        if base.identity(ROOT / reference["path"]) != reference:
            raise ValueError("launch state producer identity changed")
    path = ROOT / proof["terminal"]["path"]
    encoding = "utf-16" if path.read_bytes().startswith(b"\xff\xfe") else "utf-8"
    text = path.read_text(encoding=encoding)
    if not re.search(r"All tests passed \(183 assertions in 7 test cases\)", text):
        raise ValueError("launch state producer lacks actual success output")


def rebuild_native(model):
    """Run the bounded original native receipt/phase/custody test filter.

    @param model Curated source mapping whose actual owner files are bound.
    @return None; fail before source acceptance if either native build refuses.
    """
    PROOFS.mkdir(parents=True, exist_ok=True)
    # Independent processes preserve each build's actual assertions and log;
    # neither native process loads a paid world or uses a fixture observer.
    for configuration in ("Debug", "RelWithDebInfo"):
        application = ROOT / f"build/{configuration}/openttd_test.exe"
        terminal = ROOT / f"logs/launch-center-state-native-{configuration}.log"
        with terminal.open("w", encoding="utf-8") as output:
            subprocess.run([str(application)] + NATIVE_ARGUMENTS, cwd=ROOT,
                           stdout=output, stderr=subprocess.STDOUT, check=True, timeout=110)
        proof = dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Group61E.1; actual pure native state/receipt contracts only.",
                     complete=True, configuration=configuration, application=base.identity(application),
                     arguments=NATIVE_ARGUMENTS, terminal=base.identity(terminal), assertions=183, cases=7,
                     sources=model["native_inputs"], paid_world_loaded=False, simulation_ticks=0)
        checked_native(proof, model)
        (PROOFS / f"native-{configuration}.json").write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


class LaunchStateMappingChecks(unittest.TestCase):
    """Preserve native state/custody ownership while authoring facility source parts."""

    @classmethod
    def setUpClass(cls):
        """Load explicit immutable state mapping and the existing public family.

        @param cls Current focused source-state mapping test class.
        @return None.
        """
        cls.model = read_json(MODEL)
        cls.public = read_json(ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json")
        cls.row = next(row for row in cls.public["assets"] if row["asset_id"] == contract.ASSET)

    def test_actual_native_state_and_geometry_producers_remain_current(self):
        """Both real builds qualify existing state receipts and actual rotated geometry.

        @param self Active cross-producer input and actual native result check.
        @return None.
        """
        # Existing native owners separately qualify geometry and state receipts;
        # a source-only mapping never replaces either compiled behavior check.
        for configuration in ("Debug", "RelWithDebInfo"):
            checked_native(read_json(PROOFS / f"native-{configuration}.json"), self.model)
            geometry = read_json(GEOMETRY_PROOFS / f"native-{configuration}.json")
            verify_native_proof(geometry)
            self.assertEqual(geometry["assertions"], 305)

    def test_public_contract_and_four_selected_bases_stay_exact(self):
        """State mapping consumes the single public family and selected genuine source views.

        @param self Active public source geometry and immutable selection check.
        @return None.
        """
        contract.validate_launch_facility_contract(self.row)
        self.assertEqual(self.model["asset_id"], contract.ASSET)
        frozen = base.bound_json(self.model["native_contract"])
        self.assertEqual(frozen["contract"], contract.launch_facility_contract())
        selection = base.bound_json(self.model["selected_bases"])
        self.assertEqual(set(selection["selected_views"]), set(contract.VIEWS))
        # State cues may change declared parts later; the selected equipment
        # model, original transforms and ready/empty whole views stay frozen.
        for direction, reference in selection["selected_views"].items():
            recipe = base.bound_json(reference)
            self.assertEqual(recipe["directions"], [direction])
            self.assertEqual(recipe["status"], "selected_base")
            base.bound_json(recipe["component_model"])

    def test_actual_zone_cells_and_custody_contacts_cover_twelve_cells(self):
        """Every state retains actual native zones and first-cell custody anchors.

        @param self Active rotated zone coverage and original contact check.
        @return None.
        """
        layouts = contract.launch_facility_contract()["native_rotations"]
        # A copied visual center is not the native surface_tile. Every view
        # must retain its genuine zone's first real world cell and full extent.
        for direction, layout in layouts.items():
            view = self.model["native_views"][direction]
            self.assertEqual(view["footprint"], layout["footprint"])
            self.assertEqual(view["native_rotation"], layout["rotation"])
            observed = []
            for name, rect in layout["zones"].items():
                x, y, width, height = rect
                zone = view["zones"][name]
                self.assertEqual(zone["rect"], rect)
                self.assertEqual(zone["custody_world_cell"], [x, y])
                cells = [[cx, cy] for cy in range(y, y + height) for cx in range(x, x + width)]
                self.assertEqual(zone["world_cells"], cells)
                observed.extend(map(tuple, cells))
            self.assertEqual(len(observed), 12)
            self.assertEqual(len(set(observed)), 12)

    def test_no_fictional_facility_clock_actor_or_source_acceptance(self):
        """Placement aliases and separate native actors grant no unearned source/runtime state.

        @param self Active construction/frame and public ownership boundary check.
        @return None.
        """
        self.assertEqual(self.model["states"], self.row["states"])
        self.assertEqual(self.model["frames"], self.row["frames"])
        self.assertEqual(self.model["construction"]["scope"], "placement_preview_only")
        self.assertFalse(self.model["construction"]["facility_timer"])
        self.assertEqual(self.model["frame_policy"]["aliases"], [0, 0, 0, 0])
        assets = {row["asset_id"] for row in self.public["assets"]}
        self.assertTrue(set(self.model["occupant_visibility"]["separate_assets"]).issubset(assets))
        self.assertEqual(set(self.model["occupant_visibility"]["forbidden_baked_pixels"]),
                         {"rocket", "probe", "pod", "cargo", "people", "road", "track", "water"})
        self.assertTrue(self.model["state_mapping_complete"])
        for field in ("state_art_complete", "source_coverage", "packed", "runtime"):
            self.assertFalse(self.model[field])

    def test_changed_native_scope_input_or_receipt_identity_refuses(self):
        """A re-labelled native proof cannot authorize new producers or untested states.

        @param self Active native evidence corruption refusal check.
        @return None.
        """
        proof = read_json(PROOFS / "native-Debug.json")
        mutations = dict(arguments=["*"], assertions=184, cases=8, simulation_ticks=1,
                         application=dict(proof["application"], sha256="0" * 64),
                         terminal=dict(proof["terminal"], sha256="0" * 64), sources=proof["sources"][:-1])
        # Mutate one receipt boundary at a time so another valid guard cannot
        # conceal stale executables, expanded tests or substituted owner files.
        for field, value in mutations.items():
            broken = deepcopy(proof)
            broken[field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                checked_native(broken, self.model)


def main():
    """Qualify only the active source-state mapping with reproducible local evidence.

    @return None; exit nonzero without a passing proof when a required boundary fails.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", action="store_true", help="Rerun seven pure native contract cases in both existing builds.")
    arguments = parser.parse_args()
    model = read_json(MODEL)
    if arguments.native:
        rebuild_native(model)
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(LaunchStateMappingChecks))
    if not result.wasSuccessful():
        raise SystemExit(1)
    proof = dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Group61E.1; source mapping only, state art and source/packing/runtime coverage remain open.",
                 status="passed", cases=result.testsRun, model=base.identity(MODEL),
                 test=base.identity(ROOT / "src/tests/multilayer/launch_facility_state_checks.py"),
                 native_producers={configuration: base.identity(PROOFS / f"native-{configuration}.json")
                                   for configuration in ("Debug", "RelWithDebInfo")},
                 state_art_complete=False, source_coverage=False, packed=False, runtime=False)
    (PROOFS / "mapping-verification.json").write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
