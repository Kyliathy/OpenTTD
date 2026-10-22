"""Verify exact retained generation inputs without reopening historical proof trees.

@file space_source_ancestry_check.py Protect original Space/rocket provenance.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17C.
The existing generation record and frozen launch handoff provide independent
original identities; these checks neither generate artwork nor update approvals.
"""

import copy
import hashlib
import json
import sys
import unittest

from artifact_paths import repository_root
from source_input_guard import no_old_proofs

ROOT = repository_root()
sys.path.insert(0, str(ROOT / "media/multilayer"))
import convert_space_art as art


def classify_inputs():
    """Trace selected source-policy facts without dereferencing historical products.

    @return Rebuildable measured retained-input closure and explicit historical identities.
    """
    roots = ["media/multilayer/source/space/launch-center-state-art-v2.json",
             "media/multilayer/source/space/cargo-rocket-state-art-v1.json",
             "media/multilayer/source/space/launch-fleet-source-handoff-v1.json"]
    queue, seen, retained, historical, mismatched = list(roots), set(), {}, {}, []
    # Follow exact selected document references once. Old review/native products
    # remain historical facts and cannot expand the current source prerequisite set.
    while queue:
        name = queue.pop()
        if name in seen:
            continue
        seen.add(name)
        pending = [("", json.loads((ROOT / name).read_text(encoding="utf-8")))]
        # Nested geometry/fixture contracts carry explicit path/hash pairs; inspect
        # their roles rather than interpreting every old hash as a current input.
        while pending:
            field, node = pending.pop()
            if isinstance(node, dict):
                # Only actual identity records participate; ordinary source paths
                # and descriptive strings cannot grant implicit input authority.
                if isinstance(node.get("path"), str) and isinstance(node.get("sha256"), str):
                    path = node["path"]
                    # These two named predecessor fields intentionally freeze old
                    # epochs. Current prefixes/pixels, not those mutable root files,
                    # are independently checked by source and packing validation.
                    old_epoch = ((name == roots[2] and field == "predecessor_payload_original")
                                 or (name == "src/tests/multilayer/cargo-rocket-source-fixture.json" and field == "predecessor_record"))
                    if old_epoch:
                        historical[name + "/" + field] = dict(path=path, sha256=node["sha256"], role="historical_predecessor_epoch")
                    elif path.startswith(("media/", "src/tests/")):
                        current = ROOT / path
                        # Required retained source/expectation bytes must exist
                        # unchanged; a historical success flag cannot excuse drift.
                        if not current.is_file() or art.identity(current)["sha256"] != node["sha256"]:
                            mismatched.append(path)
                        else:
                            retained[path] = node["sha256"]
                            # The generation record is already the original owner;
                            # do not preload unrelated families through its root.
                            if path.endswith(".json") and path != art.RECORD.relative_to(ROOT).as_posix():
                                queue.append(path)
                    elif path.startswith(("logs/", "mlm/", "test-cache/", "build/")):
                        historical[path] = dict(path=path, sha256=node["sha256"], role="historical_product")
                pending.extend((key, value) for key, value in node.items())
            elif isinstance(node, list):
                pending.extend((field, value) for value in node)
    return dict(_documentation=__doc__, roots=roots, verified_retained_inputs=retained,
                historical_references=historical, mismatched_retained_inputs=sorted(set(mismatched)))


class SourceAncestryTests(unittest.TestCase):
    """Keep retained original input aliases exact, contained and historically honest."""

    def test_selected_source_fixture_closure_remains_independent_and_retained(self):
        """Verify retained source/geometry/pixel fixtures and write a rebuildable inventory.

        @param self Active source-fact check; historical products are classified without reads.
        @return None; diagnostic inventory belongs under test-cache and is disposable.
        """
        with no_old_proofs(ROOT):
            summary = classify_inputs()
        self.assertEqual(summary["mismatched_retained_inputs"], [])
        self.assertGreater(len(summary["verified_retained_inputs"]), 250)
        destination = ROOT / "test-cache/cleanup/launch-source-input-classification.json"
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")

    def test_selected_original_ancestry_uses_media_without_old_proof_io(self):
        """Check all28aliases and every admitted launch/rocket original against actual bytes.

        @param self Active focused ancestry check; actual original files remain read-only.
        @return None; blocked old storage cannot satisfy a selected original input.
        """
        document = art.read_record()
        aliases = document["retained_reference_inputs"]
        self.assertEqual(len(aliases), 28)
        references = {row["path"]: row for candidate in document["candidates"]
                      if candidate["asset_id"] in ("vehicle.model-14", "facility.launch-center")
                      for row in candidate.get("references", candidate.get("reference_inputs", [])) if row["path"].startswith("logs/")}
        self.assertEqual(set(references), set(aliases))
        with no_old_proofs(ROOT):
            # Every actual prompt-input image retains its independently recorded
            # digest; durable relocation cannot replace or regenerate its pixels.
            for original, reference in references.items():
                path = art.source_reference(reference, document)
                self.assertEqual(art.identity(path), aliases[original])
            # Rejected drafts retain provenance but grant no native source view.
            # All admitted original/component candidates must resolve without logs.
            for candidate in document["candidates"]:
                if candidate["asset_id"] in ("vehicle.model-14", "facility.launch-center") and not candidate["status"].startswith("rejected"):
                    self.assertEqual(art.source_candidate(candidate["key"], document), candidate)

    def test_original_candidate_and_exclusion_prefixes_keep_frozen_identities(self):
        """Compare original metadata to its independently frozen launch source handoff.

        @param self Active provenance check; fresh alias metadata lies outside signed prefixes.
        @return None; original receipts, registrations and approval facts stay byte-semantic exact.
        """
        document = art.read_record()
        scope = json.loads((ROOT / "media/multilayer/launch-pack-scope.json").read_text(encoding="utf-8"))
        handoff_path = ROOT / scope["source_handoff"]["path"]
        self.assertEqual(art.identity(handoff_path), scope["source_handoff"])
        handoff = json.loads(handoff_path.read_text(encoding="utf-8"))
        # Frozen prefix hashes predate this cleanup and cannot be approved by
        # newly copied inputs or by a freshly regenerated actual output.
        for field, count, digest in (("candidates", "candidate_count", "candidate_prefix_sha256"),
                                     ("excluded_sources", "exclusion_count", "exclusion_prefix_sha256")):
            encoded = json.dumps(document[field][:handoff[count]], sort_keys=True, separators=(",", ":")).encode("utf-8")
            self.assertEqual(hashlib.sha256(encoded).hexdigest(), handoff[digest])

    def test_changed_missing_and_escaped_aliases_refuse(self):
        """Refuse stale input identities, a removed alias and another media owner.

        @param self Active fault check; mutations affect detached metadata only.
        @return None; no historical path fallback or changed original is accepted.
        """
        original = art.read_record()
        alias = next(iter(original["retained_reference_inputs"]))
        reference = dict(path=alias, sha256=original["retained_reference_inputs"][alias]["sha256"])
        document = copy.deepcopy(original)
        document["retained_reference_inputs"][alias]["sha256"] = "0" * 64
        with self.assertRaisesRegex(ValueError, "identity changed"):
            art.source_reference(reference, document)
        document = copy.deepcopy(original)
        document["retained_reference_inputs"][alias]["path"] = "media/multilayer/source/space/space-generation-record.json"
        with self.assertRaisesRegex(ValueError, "escapes"):
            art.source_reference(reference, document)
        document = copy.deepcopy(original)
        del document["retained_reference_inputs"][alias]
        with no_old_proofs(ROOT):
            with self.assertRaisesRegex(ValueError, "escapes"):
                art.source_reference(reference, document)


if __name__ == "__main__":
    unittest.main(verbosity=2)
