"""Rebuild and verify launch-center state material and actual cell source parts.

@file launch_facility_state_art_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group61E.2.
@see docs/zz-reach2/upgrades/2026-10/r2mlsp-z-z-z-z-z-q-launch-center-source.md.
Outputs, source review contacts and stored parts are reproducible under
logs/proofs/mlm/native-art/space/launch-center-state-v1/art. Native state/geometry
proofs are consumed read-only; no full regression or game advancement occurs.
"""

from copy import deepcopy
import hashlib
import json
import unittest

from PIL import Image, ImageDraw

from launch_facility_art_contract_checks import ROOT, read_json
from launch_facility_state_checks import checked_native, PROOFS as STATE_PROOFS
from tools import launch_facility_states as states

base = states.base
POLICY = ROOT / "media/multilayer/source/space/launch-center-state-art-v1.json"
PROOFS = STATE_PROOFS / "art"


def write_json(path, document):
    """Persist explicit reproducible source evidence with stable serialization.

    @param path Repository-local output JSON path.
    @param document Source artifact or verification metadata.
    @return None.
    """
    path.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")


def rebuild_sources(policy):
    """Rebuild the three physical source states per genuine view and their saved cuts.

    @param policy Explicit original-material reuse policy.
    @return Per-view source fixtures used independently by stored-part tests.
    """
    fixtures = {}
    sheet = Image.new("RGB", (1920, 1920), "#29323c")
    draw = ImageDraw.Draw(sheet)
    details = Image.new("RGB", (576, 672), "#29323c")
    detail_labels = ImageDraw.Draw(details)
    # Each row retains one genuine physical camera. Construction is an exact
    # ready alias; still frames are not duplicated into artificial motion poses.
    for row_index, direction in enumerate(states.CONTACTS):
        images, owners, cells, evidence = states.compose_states(policy, direction)
        directory = PROOFS / direction
        directory.mkdir(parents=True, exist_ok=True)
        documents = {}
        # The only authored state differences are whole genuine glass material
        # pixels inside the existing workshop. All source hardware stays fixed.
        for column, state in enumerate(("ready", "working", "blocked")):
            image = images[state]
            image.save(directory / f"{state}-native.png")
            pieces = states.cut_state(image, cells, direction, state)
            rows = []
            for metadata, part in pieces:
                path = directory / f"{state}-cell-{metadata['tile_variant']:02d}-{metadata['part']}.png"
                part.save(path)
                rows.append(dict(metadata, image=base.identity(path)))
            document = dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Group61E.2; source state and explicit current foreground absence only.",
                            direction=direction, state=state, frame=0, foreground_policy=states.FOREGROUND_POLICY,
                            native_sha256=evidence["state_sha256"][state],
                            whole=base.identity(directory / f"{state}-native.png"), parts=rows)
            write_json(directory / f"{state}-parts.json", document)
            documents[state] = document
            panel = base.geometry_review(image, direction).resize((640, 448), Image.Resampling.NEAREST)
            x, y = column * 640, row_index * 480
            sheet.paste(panel, (x, y + 24))
            draw.text((x + 12, y + 8), f"{direction} {state}: source only; diagnostic floor", fill="white")
            # This nearest-neighbor diagnostic enlarges the exact source
            # glazing pixels; it neither paints art nor changes registrations.
            cx, cy = states.CONTACTS[direction]
            detail = image.convert("RGB").crop((cx - 7, cy - 6, cx + 9, cy + 6)).resize((192, 144), Image.Resampling.NEAREST)
            dx, dy = column * 192, row_index * 168
            details.paste(detail, (dx, dy + 24))
            detail_labels.text((dx + 8, dy + 8), f"{direction} {state}: exact glass", fill="white")
        (directory / "cell-ownership.bin").write_bytes(cells)
        (directory / "component-ownership.bin").write_bytes(owners)
        write_json(directory / "source.json", dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Group61E.2; original material and ready-base ancestry.",
                                                   policy=base.identity(POLICY), evidence=evidence,
                                                   cell_ownership=base.identity(directory / "cell-ownership.bin"),
                                                   component_ownership=base.identity(directory / "component-ownership.bin")))
        fixtures[direction] = dict(images=images, owners=owners, cells=cells, evidence=evidence, documents=documents)
    sheet.save(PROOFS / "state-contact.png")
    details.save(PROOFS / "status-detail-contact.png")
    return fixtures


def stored_pieces(document):
    """Load exact stored source PNGs independently of the in-memory cutter.

    @param document One physical state/view parts receipt.
    @return Stored metadata and copied indexed image pairs.
    @throws ValueError When any stored source identity changes.
    """
    pieces = []
    # Serialization is part of the source handoff: saved palette and pixel
    # bytes must survive without silently changing anchors or absence metadata.
    for row in document["parts"]:
        path = ROOT / row["image"]["path"]
        if base.identity(path) != row["image"]:
            raise ValueError("stored launch state PNG changed")
        with Image.open(path) as image:
            pieces.append((row, image.copy()))
    return pieces


class LaunchStateArtChecks(unittest.TestCase):
    """Require exact genuine material, fixed hardware, native ownership and static aliases."""

    fixtures = {}

    @classmethod
    def setUpClass(cls):
        """Read current explicit state policy and its immutable native owner mapping.

        @param cls Active bounded state-source test class.
        @return None.
        """
        cls.policy = read_json(POLICY)
        cls.mapping, cls.selection = states.checked_policy(cls.policy)

    def test_current_native_owners_and_selected_ready_bases_stay_exact(self):
        """Status materials consume existing native jobs and immutable base selections.

        @param self Active source-to-original-native producer boundary check.
        @return None.
        """
        for configuration in ("Debug", "RelWithDebInfo"):
            checked_native(read_json(STATE_PROOFS / f"native-{configuration}.json"), self.mapping)
        # Source state authoring never changes the accepted ready hardware;
        # each physical view is still bound to its reviewed composition recipe.
        for direction, fixture in self.fixtures.items():
            self.assertEqual(fixture["evidence"]["base_recipe"], self.selection["selected_views"][direction])
            self.assertEqual(fixture["evidence"]["state_sha256"]["ready"], fixture["evidence"]["original"]["native_sha256"])
        self.assertEqual(self.policy["status"], "selected_state_parts")
        self.assertTrue(self.policy["state_art_complete"])
        for field in ("source_coverage", "packed", "runtime"):
            self.assertFalse(self.policy[field])

    def test_only_three_existing_workshop_glass_pixels_change(self):
        """Complete source hardware and opacity survive each genuine material overlay.

        @param self Active native material visibility and protected-hardware check.
        @return None.
        """
        # Compare every pixel, including transparency and the other three
        # native zones. Whole authentic material is copied without masking.
        for direction, fixture in self.fixtures.items():
            ready = fixture["images"]["ready"]
            for state in states.MATERIALS:
                image = fixture["images"][state]
                changed = [[index % image.width, index // image.width] for index, pair in
                           enumerate(zip(ready.get_flattened_data(), image.get_flattened_data())) if pair[0] != pair[1]]
                self.assertEqual(changed, fixture["evidence"]["changed_pixels"][state])
                self.assertEqual(len(changed), 3)
                self.assertTrue(all(bool(a) == bool(b) for a, b in zip(ready.get_flattened_data(), image.get_flattened_data())))
                plane, ancestry = states.material(self.policy, direction, state)
                material_points = [[index % image.width, index // image.width] for index, value in
                                   enumerate(plane.get_flattened_data()) if value]
                self.assertEqual(changed, material_points)
                self.assertEqual(ancestry, fixture["evidence"]["materials"][state])
                self.assertTrue(all(ready.getpixel(tuple(point)) in states.BLUE_GLASS for point in changed))
                self.assertTrue(all(image.getpixel(tuple(point)) == plane.getpixel(tuple(point)) for point in changed))
            self.assertNotEqual(fixture["images"]["working"].tobytes(), fixture["images"]["blocked"].tobytes())

    def test_stored_state_parts_reassemble_actual_original_pixels(self):
        """Every physical state retains twenty-four real native cell positions.

        @param self Active stored source-state round-trip check.
        @return None.
        """
        # Stored PNGs and metadata are checked independently for all genuine
        # views; a successful empty NE base cannot qualify another state/view.
        for direction, fixture in self.fixtures.items():
            for state, document in fixture["documents"].items():
                pieces = stored_pieces(document)
                whole = states.reassemble_state(pieces, fixture["cells"], document["native_sha256"], direction, state)
                self.assertEqual(whole.tobytes(), fixture["images"][state].tobytes())
                self.assertEqual(len(pieces), 24)
                self.assertEqual(sum(row["part"] == "front" and row["structurally_absent"] for row, image in pieces), 12)

    def test_construction_and_declared_still_frames_are_explicit_aliases(self):
        """No placement or animation alias fabricates an incomplete building or work clock.

        @param self Active source construction and still-frame alias check.
        @return None.
        """
        for direction, fixture in self.fixtures.items():
            self.assertEqual(fixture["images"]["construction"].tobytes(), fixture["images"]["ready"].tobytes())
        self.assertEqual(self.policy["frame_aliases"], [0, 0, 0, 0])
        # Check a nonzero logical frame through the real cutter/verifier;
        # retagging aliases cannot change state pixels or become an art clock.
        fixture = self.fixtures["NE"]
        image = fixture["images"]["working"]
        pieces = states.cut_state(image, fixture["cells"], "NE", "working", 3)
        result = states.reassemble_state(pieces, fixture["cells"], fixture["evidence"]["state_sha256"]["working"], "NE", "working", 3)
        self.assertEqual(result.tobytes(), image.tobytes())

    def test_moved_glass_changed_material_and_invented_foreground_policy_refuse(self):
        """Rehashing cannot move cues onto machinery or authorize new visible actors.

        @param self Active original material/registration and source authority refusal check.
        @return None.
        """
        mutations = dict(glass_contacts=dict(states.CONTACTS, NE=[121, 103]), permitted_underlay=[0, 1],
                         frame_aliases=[0, 1, 2, 3], construction_alias="partial", foreground_policy="new_actors_visible",
                         state_mapping=dict(self.policy["state_mapping"], sha256="0" * 64))
        # Corrupt each independent policy field so no secondary invalid field
        # can conceal a missing geometry, native-owner or foreground guard.
        for field, value in mutations.items():
            broken = deepcopy(self.policy)
            broken[field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                states.checked_policy(broken)
        broken = deepcopy(self.policy)
        broken["materials"]["working"]["original"]["sha256"] = "0" * 64
        with self.assertRaises(ValueError):
            states.material(broken, "NE", "working")
        broken = deepcopy(self.policy)
        broken["materials"]["working"]["uniform_scale"] = [1, 128]
        with self.assertRaises(ValueError):
            states.material(broken, "NE", "working")

    def test_unmasked_material_overflow_and_false_stored_state_refuse(self):
        """Extra source pixels and relabelled PNGs cannot inherit a valid state proof.

        @param self Active material footprint and serialized pose refusal check.
        @return None.
        """
        fixture = self.fixtures["NE"]
        plane, ancestry = states.material(self.policy, "NE", "working")
        plane.putpixel((0, 0), 1)
        with self.assertRaises(ValueError):
            states.with_material(fixture["images"]["ready"], fixture["owners"], "NE", plane)
        document = fixture["documents"]["working"]
        pieces = stored_pieces(document)
        for field, value in dict(state="blocked", frame=1, foreground_policy="invented_actor_front").items():
            broken = deepcopy(pieces)
            broken[0][0][field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                states.reassemble_state(broken, fixture["cells"], document["native_sha256"], "NE", "working")
        broken = deepcopy(pieces)
        row, image = broken[1]
        image.putpixel((31, 48), 1)
        row.update(opaque_pixels=1, structurally_absent=False, native_sha256=hashlib.sha256(image.tobytes()).hexdigest())
        with self.assertRaises(ValueError):
            states.reassemble_state(broken, fixture["cells"], document["native_sha256"], "NE", "working")


def main():
    """Rebuild only the active authored states and qualify their stored source artifacts.

    @return None; exit nonzero without publishing successful source evidence on failure.
    """
    PROOFS.mkdir(parents=True, exist_ok=True)
    policy = read_json(POLICY)
    LaunchStateArtChecks.fixtures = rebuild_sources(policy)
    result = unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(LaunchStateArtChecks))
    if not result.wasSuccessful():
        raise SystemExit(1)
    proof = dict(_documentation="r2mlvs14-remaining-assets-and-runtime-integration.md Group61E.2; physical source states, source closure/packing/runtime remain separate.",
                 status="passed", cases=result.testsRun, policy=base.identity(POLICY),
                 contact=base.identity(PROOFS / "state-contact.png"),
                 detail_contact=base.identity(PROOFS / "status-detail-contact.png"),
                 sources={direction: base.identity(PROOFS / direction / "source.json") for direction in states.CONTACTS},
                 helper=base.identity(ROOT / "media/multilayer/tools/launch_facility_states.py"),
                 test=base.identity(ROOT / "src/tests/multilayer/launch_facility_state_art_checks.py"),
                 source_coverage=False, packed=False, runtime=False)
    write_json(STATE_PROOFS / "state-art-verification.json", proof)


if __name__ == "__main__":
    main()
