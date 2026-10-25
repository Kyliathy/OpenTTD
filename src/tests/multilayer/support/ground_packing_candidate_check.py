"""Verify actual current Ground candidates against immutable original shipping bytes.

@file ground_packing_candidate_check.py Requires an explicit current source-to-pack run.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17DE.
Fault inputs use current report roles so real selection/normalization checks run.
Normal acceptance requires completion; --sources-only explicitly debugs verified
suppliers without accepting an incomplete executor result.
"""

import argparse
import copy
import json
from pathlib import Path
import sys
import unittest

from artifact_paths import RunContext, contained, identifier, repository_root
from source_input_guard import no_old_proofs

ROOT = repository_root()
sys.path.insert(0, str(ROOT / "media/multilayer"))
import regenerate_ground_sources as source
from evidence import identity

RUN_ID = None
SOURCES_ONLY = False
RECIPE = "ground-derived-inputs"


class GroundPackingCandidates(unittest.TestCase):
    """Exercise real complete packing and independent source/absence/anchor refusal gates."""

    @classmethod
    def setUpClass(cls):
        """Read exact supplied source identities from the completed shared executor ledger.

        @param cls Active explicitly selected current-run packing checks.
        @return None; no latest-run lookup or earlier workstation receipt is accepted.
        """
        path = ROOT / "mlm/reviews/rebuild" / RECIPE / RUN_ID / "reports/rebuild.json"
        state = json.loads(path.read_text(encoding="utf-8"))
        # Normal acceptance requires a complete recipe. Explicit source-only
        # debugging may exercise a failed packing leaf's verified suppliers,
        # without changing or accepting that incomplete executor result.
        if state["run_id"] != RUN_ID or (not SOURCES_ONLY and (state["complete"] is not True or state["status"] != "verified")):
            raise ValueError("Ground packing checks require the explicit completed current recipe")
        declarations = state["signature"]["declarations"]
        cls.manifests = {}
        for requirement in declarations["ground-derived-inputs"]["requires"]:
            name = requirement["recipe"]
            if state["nodes"][name]["status"] != "verified":
                raise ValueError("Ground source supplier is incomplete")
            for asset in declarations[name]["original_parameters"]["source_assets"]:
                cls.manifests[asset] = state["nodes"][name]["outputs"]["manifest"]

    def context(self, scenario, run_id=None):
        """Choose a contained packing/check scenario sharing the explicitly supplied source run.

        @param self Active packing candidate check.
        @param scenario Exact generated check scenario, distinct from source producers.
        @param run_id Optional deliberate foreign-run identity for a refusal case.
        @return Explicit generated output context; this method creates no directories.
        """
        return RunContext(ROOT, "multilayer-art", scenario, run_id or RUN_ID)

    def fault_report(self, reference, label, document):
        """Write a disposable coherent fault input under its real current report owner.

        @param self Active source-contract fault check.
        @param reference Actual current raw/typed source identity whose role is under test.
        @param label Fixed test-case filename suffix, never a producer output filename.
        @param document Detached mutated metadata; original report bytes remain untouched.
        @return Actual fault-file identity for the real current input selector.
        """
        context = self.context("ground-packing-faults")
        original = source.selected_path(reference, context, "reports")
        path = contained(original.parent / ("fault-ground-packing-" + label + ".json"), original.parent)
        document["_documentation"] = "Owner: r2fc-file-cleanup3.md17DE; generated current-role negative fixture."
        source.shared.write_document(path, document)
        return identity(path)

    def test_actual_packing_repeats_exact_canonical_shipping_bytes(self):
        """Execute the original47-family packer twice with all old-proof I/O blocked.

        @param self Active complete source-to-packing comparison.
        @return None; canonical PNG/NFO/header supply independent expected bytes.
        """
        first, repeat = self.context("ground-packing-check"), self.context("ground-packing-check-repeat")
        with no_old_proofs(ROOT):
            a = source.produce_packing(first, self.manifests)
            b = source.produce_packing(repeat, self.manifests)
        self.assertEqual((a["pose_count"], a["native_start"], a["native_sprite_bound"]), (14254, 7791, 22045))
        self.assertEqual(a["pose_count"], b["pose_count"])
        self.assertFalse(a["native_proof"])
        self.assertFalse(a["publishes_source"])
        scope = source.read_scope()
        # Compare actual candidate bytes and unchanged independently shipped
        # bytes, rather than deriving a replacement expectation from this run.
        for role in ("png", "nfo", "header"):
            name = Path(scope[role]).name
            self.assertEqual((first.data / name).read_bytes(), (ROOT / scope[role]).read_bytes())
            self.assertEqual((first.data / name).read_bytes(), (repeat.data / name).read_bytes())

    def test_missing_foreign_and_changed_source_identities_refuse(self):
        """Refuse incomplete/foreign/stale manifests before an old cache can stand in.

        @param self Active selected source identity boundary check.
        @return None; source, original reports and canonical shipping bytes stay unchanged.
        """
        missing = dict(self.manifests)
        missing.pop(next(iter(missing)))
        with no_old_proofs(ROOT):
            with self.assertRaisesRegex(ValueError, "every exact selected physical source"):
                source.produce_packing(self.context("ground-packing-missing"), missing)
            with self.assertRaisesRegex(ValueError, "foreign invocation"):
                source.produce_packing(self.context("ground-packing-foreign", RUN_ID + "-foreign"), self.manifests)
            changed = copy.deepcopy(self.manifests)
            changed[next(iter(changed))]["sha256"] = "0" * 64
            with self.assertRaisesRegex(ValueError, "identity changed"):
                source.produce_packing(self.context("ground-packing-stale"), changed)

    def test_coherent_source_producer_signed_anchor_and_absence_faults_refuse(self):
        """Reject rehashed metadata faults even while original PNG pixels remain exact.

        @param self Active independent physical/provenance comparison.
        @return None; all mutated reports are disposable current-role fixtures.
        """
        asset = "facility.rail-station"
        envelope = json.loads((ROOT / self.manifests[asset]["path"]).read_text(encoding="utf-8"))
        changed = copy.deepcopy(envelope)
        changed["source_producer"]["sha256"] = "0" * 64
        manifests = dict(self.manifests)
        manifests[asset] = self.fault_report(self.manifests[asset], "producer", changed)
        with no_old_proofs(ROOT):
            with self.assertRaisesRegex(ValueError, "producer/family/approval changed"):
                source.produce_packing(self.context("ground-packing-bad-producer"), manifests)
        raw = envelope["raw_source_reports"][asset]
        original = json.loads((ROOT / raw["path"]).read_text(encoding="utf-8"))
        # Both raw and envelope hashes are recomputed. Independent physical
        # expectations, rather than mere file-integrity checks, must catch each fault.
        for label in ("anchor", "absence"):
            document = copy.deepcopy(original)
            if label == "anchor":
                row = next(row for row in document["sprites"] if not row["structurally_absent"])
                row["native_packed_anchor"][0] += 1
                expected = "independent original physical poses"
            else:
                row = next(row for row in document["sprites"] if row["structurally_absent"])
                row["structurally_absent"] = False
                expected = "contradicts structural absence"
            changed = copy.deepcopy(envelope)
            changed["raw_source_reports"][asset] = self.fault_report(raw, label + "-raw", document)
            changed["raw_source_report"] = changed["raw_source_reports"][asset]
            manifests = dict(self.manifests)
            manifests[asset] = self.fault_report(self.manifests[asset], label + "-envelope", changed)
            with no_old_proofs(ROOT):
                with self.assertRaisesRegex(ValueError, expected):
                    source.produce_packing(self.context("ground-packing-bad-" + label), manifests)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--run-id", required=True)
    parser.add_argument("--recipe", type=identifier, default="ground-derived-inputs", help="Explicit completed graph owning the Ground suppliers; no latest-run lookup.")
    parser.add_argument("--case", choices=("all", "repeat", "refusals"), default="all")
    parser.add_argument("--sources-only", action="store_true", help="Debug verified suppliers from an incomplete packing run; never accepts that recipe")
    arguments = parser.parse_args()
    RUN_ID = arguments.run_id
    RECIPE = arguments.recipe
    SOURCES_ONLY = arguments.sources_only
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(GroundPackingCandidates)
    if arguments.case != "all":
        names = ["test_actual_packing_repeats_exact_canonical_shipping_bytes"] if arguments.case == "repeat" else [
            "test_missing_foreign_and_changed_source_identities_refuse",
            "test_coherent_source_producer_signed_anchor_and_absence_faults_refuse"]
        suite = unittest.defaultTestLoader.loadTestsFromNames(["GroundPackingCandidates." + name for name in names], sys.modules[__name__])
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    raise SystemExit(0 if result.wasSuccessful() else 1)
