"""Focused refusal cases for imported original identities and canonical aliases.

@file donor_input_checks.py No native processes or cleanup operations are used.
@see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space.md Group 2.
@see docs/zz-reach2/architecture/archi-historical-debug.md Storage ownership.
Test files remain under test-cache/donor-input-checks; no recursive deletion.
"""

import copy
import hashlib
import json
from pathlib import Path
import unittest
import uuid

from verify_donor_inputs import verify_retained_inputs


class DonorInputChecks(unittest.TestCase):
    """Exercise exact bytes, owner containment and donor namespace refusal."""

    def setUp(self):
        """Create a fresh disposable fixture without removing prior test output.

        @return None; each case owns an isolated test-cache subdirectory.
        """
        self.root = Path(__file__).resolve().parents[3] / "test-cache/donor-input-checks" / uuid.uuid4().hex
        path = self.root / "src/tests/fixtures/legacy/original.sav"
        path.parent.mkdir(parents=True)
        (self.root / "CMakeLists.txt").write_text("# @see r2fc-file-cleanup3.md; isolated ownership marker, never built.\n", encoding="utf-8")
        path.write_bytes(b"independent original bytes")
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        self.namespace = "donor/openttd3-75ea75718f/"
        self.original = "build/save/original.sav"
        self.row = dict(role="save", destination=path.relative_to(self.root).as_posix(), sha256=digest, bytes=path.stat().st_size, original_paths=[self.original])
        self.document = dict(schema_version=1, kind="donor_input_preservation", namespace=self.namespace, payloads=[self.row])
        self.catalogue = dict(_documentation="@see r2fc-file-cleanup3.md; Group2 isolated test fixture.", schema_version=2,
                              inputs={self.namespace + self.original: dict(path=self.row["destination"], sha256=digest)})
        self.publish_catalogue()

    def publish_catalogue(self):
        """Write only this test's explicit immutable-input alias mapping.

        @return None; the real checkout's catalogue is never touched.
        """
        (self.root / "src/tests/fixtures/multilayer-inputs.json").write_text(json.dumps(self.catalogue), encoding="utf-8")

    def test_retained_alias_requires_no_donor_directory(self):
        """Resolve original bytes when no donor filesystem exists in the fixture.

        @return None; only the current fixture root can supply the bytes.
        """
        self.assertEqual(verify_retained_inputs(self.root, document=self.document)["payloads"], 1)

    def test_changed_bytes_refuse(self):
        """Reject a same-named payload whose content no longer matches its seal.

        @return None.
        """
        (self.root / self.row["destination"]).write_bytes(b"different")
        with self.assertRaisesRegex(ValueError, "retained input changed"):
            verify_retained_inputs(self.root, document=self.document)

    def test_alias_rebinding_refuses(self):
        """Reject a donor alias redirected to another baseline identity.

        @return None.
        """
        self.catalogue["inputs"][self.namespace + self.original]["sha256"] = "0" * 64
        self.publish_catalogue()
        with self.assertRaisesRegex(ValueError, "retained donor alias changed"):
            verify_retained_inputs(self.root, document=self.document)

    def test_wrong_owner_refuses(self):
        """Reject a generated path masquerading as a durable original.

        @return None.
        """
        self.row["destination"] = "test-cache/borrowed.sav"
        with self.assertRaisesRegex(ValueError, "wrong owner"):
            verify_retained_inputs(self.root, document=self.document)

    def test_foreign_namespace_refuses(self):
        """Reject a contract that could replace destination fixture keys.

        @return None.
        """
        self.document["namespace"] = "src/tests/fixtures/"
        with self.assertRaisesRegex(ValueError, "alias namespace"):
            verify_retained_inputs(self.root, document=self.document)

    def test_scopes_do_not_borrow_other_payloads(self):
        """Keep a source-only check independent from an unrelated missing save.

        @return None; ordinary all-scope checking must still reject that save.
        """
        document = copy.deepcopy(self.document)
        document["payloads"][0]["destination"] = "src/tests/fixtures/legacy/absent.sav"
        self.assertEqual(verify_retained_inputs(self.root, scope="sources", document=document)["payloads"], 0)
        with self.assertRaisesRegex(ValueError, "retained input is missing"):
            verify_retained_inputs(self.root, document=document)


if __name__ == "__main__":
    unittest.main()
