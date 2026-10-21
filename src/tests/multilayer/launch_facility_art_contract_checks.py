"""Focused native launch-center source contract, guides and corruption checks.

@file launch_facility_art_contract_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group61B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md task16.2.
Run --native CONFIG after building the focused openttd_test target in that
configuration, then run without arguments. --capture-before is a one-time
pre-edit preservation capture. Guides/proofs rebuild beneath logs/proofs;
none of their pixels are authored gameplay artwork or runtime acceptance.
"""

import argparse
from copy import deepcopy
import hashlib
import json
import os
from pathlib import Path
import re
import sys
import unittest

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
sys.path.insert(0, str(ROOT / "src/tests"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from tools import launch_facility_art_contract as launch
import multilayer_sprite_contract as shared
from multilayer.support.artifact_paths import create_run_context
from multilayer.support.runner import run_native

PROOFS = ROOT / "logs/proofs/mlm/native-art/space/launch-center-contract-v2"
BRIEF = ROOT / "media/multilayer/source/space/launch-center-source-contract.json"
CONFIGURATIONS = ("RelWithDebInfo", "Debug")
OWNERS = ("src/launch_site.h", "src/launch_center.cpp", "src/launch_operations.cpp", "src/station_cmd.cpp", "src/landscape.h",
          "src/space_production.h", "src/space_manifest.h", "src/space_pod.h",
          "src/space_transition.h", "src/tests/launch_site.cpp", "src/tests/launch_production.cpp")
PROTECTED = ("media/baseset/openttd.grf", "media/multilayer/source/space/space-generation-record.json",
             "media/baseset/openttd/space_final.nfo", "media/baseset/openttd/orbital_final.nfo",
             "media/multilayer/contracts/multilayer_geometry_templates.svg")


def digest(path):
    """Identify exact producer, source or retained proof bytes.

    @param path Explicit existing file, never a latest-run lookup.
    @return Lowercase SHA256 digest.
    """
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def identity(path):
    """Bind an existing repository artifact to its exact retained contents.

    @param path Absolute existing repository path.
    @return Repository-relative path and SHA256 object.
    """
    return dict(path=path.relative_to(ROOT).as_posix(), sha256=digest(path))


def read_json(path):
    """Read an explicitly selected UTF8 descriptor or proof.

    @param path Existing JSON path.
    @return Decoded JSON value.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def capture_before():
    """Retain the predecessor before the one-family public contract edit.

    @return None; refuse to overwrite the original preservation capture.
    """
    PROOFS.mkdir(parents=True, exist_ok=True)
    before = dict(_documentation="Plan14Group61B pre-edit preservation capture; rebuild with --capture-before before applying the scoped correction.",
                  manifest=read_json(shared.MANIFEST), protected={name: digest(ROOT / name) for name in PROTECTED})
    with (PROOFS / "before.json").open("x", encoding="utf-8") as sink:
        sink.write(json.dumps(before, indent=2) + "\n")


def expected_trace():
    """Project public layouts into the compiled fixture's exact trace order.

    @return Four native rotation/footprint/zone records.
    """
    layouts = launch.launch_facility_contract()["native_rotations"]
    return [dict(rotation=layouts[view]["rotation"], footprint=layouts[view]["footprint"],
                 zones=[layouts[view]["zones"][zone] for zone in launch.ZONES]) for view in launch.VIEWS]


def verify_native_proof(proof):
    """Refuse stale, partial, failed or substituted native geometry evidence.

    @param proof Explicit one-configuration process, source and terminal receipt.
    @return None; raise ValueError when any required native identity differs.
    """
    # Both configuration identity and exact source membership are necessary:
    # an old geometry-only log cannot authorize this public art correction.
    if proof.get("configuration") not in CONFIGURATIONS or not proof.get("complete"):
        raise ValueError("missing successful native configuration")
    sources = proof.get("sources", [])
    if [item["path"] for item in sources] != list(OWNERS):
        raise ValueError("missing native owner identity")
    # Every recorded executable, source and terminal must still be the exact
    # invocation inputs; path names alone cannot qualify a stale producer.
    for item in [proof["application"], proof["terminal"], *sources, *proof["native_processes"]]:
        if identity(ROOT / item["path"]) != item:
            raise ValueError("stale native proof identity: " + item["path"])
    configuration = proof["configuration"]
    if proof["application"]["path"] != f"build/{configuration}/openttd_test.exe":
        raise ValueError("wrong native executable")
    output = (ROOT / proof["terminal"]["path"]).read_text(encoding="utf-8-sig")
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases\)", output)
    # This bounded producer has exactly four selected cases. Its final exit
    # sentinel and all four traces must agree, including non-square layouts.
    if passed is None or not output.rstrip().endswith("NATIVE_EXIT_CODE=0"):
        raise ValueError("failed or incomplete native terminal")
    if tuple(map(int, passed.groups())) != (proof["assertions"], 4) or proof.get("cases") != 4:
        raise ValueError("wrong native case/assertion evidence")
    traces = [json.loads(line.removeprefix("LAUNCH_ART_LAYOUT ")) for line in output.splitlines()
              if line.startswith("LAUNCH_ART_LAYOUT ")]
    if traces != expected_trace():
        raise ValueError("native/public rotated zones differ")
    projections = [json.loads(line.removeprefix("LAUNCH_ART_PROJECTION ")) for line in output.splitlines()
                   if line.startswith("LAUNCH_ART_PROJECTION ")]
    expected_projection = launch.launch_facility_contract()["native_projection"]
    if projections != [{key: expected_projection[key] for key in ("x_step", "y_step")}]:
        raise ValueError("native/public viewport basis differs")
    if len(proof["native_processes"]) != 1:
        raise ValueError("wrong native invocation count")
    process = read_json(ROOT / proof["native_processes"][0]["path"])
    # The artifact catalogue must name this exact successful executable and
    # build, preventing a proof assembled from unrelated native invocations.
    if process.get("exit_status") != 0 or process.get("executable") != proof["application"] or process.get("configuration") != configuration:
        raise ValueError("native process identity or success changed")


def native_run(configuration):
    """Run only launch-site and launch-assembly contract producer cases.

    @param configuration Explicit built Debug or RelWithDebInfo application.
    @return None; publish a fresh proof only after actual successful completion.
    """
    context = create_run_context("space", "launch-center-contract-geometry")
    context.prepare_runtime()
    application = ROOT / f"build/{configuration}/openttd_test.exe"
    terminal = context.logs / "native.log"
    with terminal.open("w", encoding="utf-8") as sink:
        result = run_native(context, [str(application), "[launch-site],[launch-assembly-contract]"],
                            cwd=ROOT / "build", stdout=sink, stderr=sink, timeout=110,
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration))
        sink.write(f"\nNATIVE_EXIT_CODE={result.returncode}\n")
    output = terminal.read_text(encoding="utf-8")
    passed = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases\)", output)
    # An exit failure or missing Catch summary leaves the invocation available
    # for diagnosis and cannot be published as a successful source contract.
    if result.returncode != 0 or passed is None:
        raise RuntimeError("native producer failed: " + str(terminal))
    processes = sorted(context.reports.glob("native-*.json"))
    proof = dict(_documentation="Plan14Group61B actual native twelve-cell/four-rotation producer; source/packing/runtime remain separate.",
                 complete=True, configuration=configuration, application=identity(application), terminal=identity(terminal),
                 assertions=int(passed.group(1)), cases=int(passed.group(2)),
                 native_processes=[identity(path) for path in processes], sources=[identity(ROOT / path) for path in OWNERS])
    verify_native_proof(proof)
    PROOFS.mkdir(parents=True, exist_ok=True)
    with (PROOFS / f"native-{configuration}.json").open("x", encoding="utf-8") as sink:
        sink.write(json.dumps(proof, indent=2) + "\n")
    print(f"{configuration}: {proof['assertions']} assertions / {proof['cases']} native cases")


def draw_guides():
    """Rebuild labelled floor guides for genuine physical source rotations.

    @return Paths of four 1024-square layout references, never gameplay art.
    """
    outputs = []
    colours = {"pad": "#758aa0", "workshop": "#b9a578", "recovery": "#9ba997", "storage": "#b59eac"}
    # A fixed 2:1 floor projection gives every rotated native cell the same
    # contact spacing. Guide colours and labels identify ownership only.
    for view, layout in launch.launch_facility_contract()["native_rotations"].items():
        canvas = Image.new("RGBA", (1024, 1024))
        draw = ImageDraw.Draw(canvas)
        draw.rectangle((24, 24, 999, 999), outline="#ff00ff", width=6)
        width, height = layout["footprint"]
        origin_x = 512 + (width - height) * 48
        # Every zone cell is drawn independently so an author can compare its
        # actual row-major position, rather than guess a rectangular silhouette.
        for zone, (x0, y0, zone_width, zone_height) in layout["zones"].items():
            for y in range(y0, y0 + zone_height):
                for x in range(x0, x0 + zone_width):
                    px, py = origin_x + 96 * (y - x), 300 + 48 * (x + y)
                    draw.polygon([(px, py), (px + 96, py + 48), (px, py + 96), (px - 96, py + 48)],
                                 fill=colours[zone], outline="#343c46", width=3)
                    draw.text((px - 30, py + 43), f"{zone} {x},{y}", fill="black")
        draw.text((64, 80), f"{view}: native rotation {layout['rotation']}; {width}x{height}; LAYOUT ONLY", fill="white")
        path = PROOFS / f"launch-center-{view}-layout.png"
        canvas.save(path)
        outputs.append(path)
    return outputs


class LaunchFacilityContractChecks(unittest.TestCase):
    """Protect native zone geometry, public ownership and retained predecessor art."""

    def setUp(self):
        """Load one explicitly scoped public family and its preserved predecessor.

        @param self Active independent contract test.
        @return None.
        """
        self.document = read_json(shared.MANIFEST)
        self.row = next(row for row in self.document["assets"] if row["asset_id"] == launch.ASSET)

    def test_public_and_generator_agree_on_twelve_cells(self):
        """The existing manifest and regeneration path require identical native geometry.

        @param self Active public contract test.
        @return None.
        """
        self.assertEqual(shared.validate_manifest(self.document, [launch.ASSET]), 1536)
        rebuilt = shared.launch_center_record()
        launch.validate_launch_facility_contract(rebuilt)
        self.assertEqual(self.row, rebuilt)

    def test_each_rotated_zone_partitions_actual_world_cells(self):
        """All four exact zone partitions occupy twelve distinct row-major world cells.

        @param self Active rotated geometry test.
        @return None.
        """
        # Zone-specific counts and full coverage prevent a plausible-looking
        # model from exchanging pad/recovery or consuming a thirteenth cell.
        for layout in self.row["native_rotations"].values():
            actual = set()
            for zone, (x, y, width, height) in layout["zones"].items():
                cells = {(cx, cy) for cy in range(y, y + height) for cx in range(x, x + width)}
                self.assertEqual(len(cells), dict(pad=2, workshop=4, recovery=2, storage=4)[zone])
                self.assertFalse(actual & cells)
                actual |= cells
            width, height = layout["footprint"]
            self.assertEqual(actual, {(x, y) for y in range(height) for x in range(width)})
            self.assertEqual(len(actual), 12)

    def test_wrong_geometry_or_invented_state_owners_refuse(self):
        """Independent malformed contracts cannot publish changed layouts or gameplay owners.

        @param self Active corruption test.
        @return None.
        """
        mutations = dict(footprint=[4, 4], tile_variants=16, native_size=[64, 80], anchor=[-32, -32],
                         directions=["NE"], frames=1, parts=["body"], native_state_owner="art timer",
                         native_inventory_policy="decorative free stock", native_occupant_policy="baked free rocket",
                         native_frame_policy={}, native_water_policy="north edge only", native_freight_policy="baked train station")
        # Each isolated mutation must fail through the public scoped validator;
        # testing only the helper would miss an omitted shared integration hook.
        for key, value in mutations.items():
            with self.subTest(field=key):
                broken = deepcopy(self.document)
                row = next(row for row in broken["assets"] if row["asset_id"] == launch.ASSET)
                row[key] = value
                with self.assertRaises(ValueError):
                    shared.validate_manifest(broken, [launch.ASSET])
        bad = deepcopy(self.row)
        bad["native_rotations"]["NE"]["zones"]["recovery"] = [0, 0, 1, 2]
        with self.assertRaises(ValueError):
            launch.validate_launch_facility_contract(bad)
        bad = deepcopy(self.row)
        bad["native_projection"]["x_step"] = [32, 16]
        bad["native_projection"]["y_step"] = [-32, 16]
        with self.assertRaises(ValueError):
            launch.validate_launch_facility_contract(bad)

    def test_both_actual_native_geometry_proofs_are_current(self):
        """Both compiled configurations prove the same complete physical layouts.

        @param self Active compiled producer proof test.
        @return None.
        """
        for configuration in CONFIGURATIONS:
            verify_native_proof(read_json(PROOFS / f"native-{configuration}.json"))

    def test_stale_failed_and_partial_native_evidence_refuse(self):
        """Tampered native receipts cannot authorize an art contract.

        @param self Active native proof corruption test.
        @return None.
        """
        original = read_json(PROOFS / "native-RelWithDebInfo.json")
        for field, value in (("complete", False), ("configuration", "unknown"), ("sources", []), ("assertions", 1)):
            broken = deepcopy(original)
            broken[field] = value
            with self.assertRaises(ValueError):
                verify_native_proof(broken)
        broken = deepcopy(original)
        broken["application"]["sha256"] = "0" * 64
        with self.assertRaises(ValueError):
            verify_native_proof(broken)

    def test_other_public_families_and_published_art_are_preserved(self):
        """The launch-only correction preserves147 other families and existing source/payload bytes.

        @param self Active predecessor preservation test.
        @return None.
        """
        before = read_json(PROOFS / "before.json")
        other_before = [row for row in before["manifest"]["assets"] if row["asset_id"] != launch.ASSET]
        other_after = [row for row in self.document["assets"] if row["asset_id"] != launch.ASSET]
        self.assertEqual(len(other_before), 147)
        self.assertEqual(other_before, other_after)
        self.assertEqual({key: value for key, value in before["manifest"].items() if key != "assets"},
                         {key: value for key, value in self.document.items() if key != "assets"})
        self.assertEqual(before["protected"], {name: digest(ROOT / name) for name in PROTECTED})

    def test_brief_retains_planned_art_and_real_native_owners(self):
        """A verified placement brief remains distinct from selected source and runtime delivery.

        @param self Active model brief test.
        @return None.
        """
        self.assertEqual(read_json(BRIEF), launch.source_brief())
        self.assertEqual(self.row["source"]["status"], "planned")
        self.assertIsNone(self.row["source"]["path"])
        self.assertIsNone(self.row["packed"]["offset"])

    def test_guides_reproduce_without_clipping_their_reference_frames(self):
        """All four labelled reference diagrams rebuild byte-identically with real alpha margins.

        @param self Active guide reproducibility test.
        @return None.
        """
        first = {path.name: digest(path) for path in draw_guides()}
        self.assertEqual(first, {path.name: digest(path) for path in draw_guides()})
        for name in first:
            with Image.open(PROOFS / name) as guide:
                self.assertEqual(guide.size, (1024, 1024))
                self.assertIsNone(guide.getchannel("A").crop((0, 0, 24, 1024)).getbbox())


def main():
    """Capture, run a selected native producer, or verify this bounded contract group.

    @return None; failing checks exit nonzero without changing source/runtime claims.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    actions = parser.add_mutually_exclusive_group()
    actions.add_argument("--capture-before", action="store_true")
    actions.add_argument("--native", choices=CONFIGURATIONS)
    args = parser.parse_args()
    if args.capture_before:
        capture_before()
        return
    if args.native:
        native_run(args.native)
        return
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(LaunchFacilityContractChecks)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if not result.wasSuccessful():
        raise SystemExit(1)
    proof_paths = [PROOFS / f"native-{configuration}.json" for configuration in CONFIGURATIONS]
    report = dict(_documentation="Plan14Group61B contract/placement only; originals, source coverage, packing and ordinary-game gates remain open.",
                  complete=True, tests=result.testsRun, preserved_families=147, protected_files=5,
                  required_source_slots=1536, native=[identity(path) for path in proof_paths],
                  brief=identity(BRIEF), manifest=identity(shared.MANIFEST),
                  guides=[identity(path) for path in draw_guides()],
                  sources=[identity(Path(__file__).resolve()), identity(ROOT / "media/multilayer/tools/launch_facility_art_contract.py"),
                           identity(ROOT / "src/tests/multilayer_sprite_contract.py")],
                  source_coverage=False, packed=False, runtime=False)
    (PROOFS / "verification.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("Launch-center contract: 8 focused cases; 147 other families and 5 published artifacts preserved")


if __name__ == "__main__":
    main()
