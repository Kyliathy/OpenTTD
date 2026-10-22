"""Retain original conventional-facility geometry without qualifying current native code.

@file station_source_facts.py Exact source-era records, never a regenerated image baseline.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group17DD.
"""

import argparse
import json
from pathlib import Path
import subprocess
import sys

from artifact_paths import contained, repository_root

ROOT = repository_root()
sys.path.insert(0, str(ROOT / "media/multilayer"))
from evidence import identity
from ground_source_facts import load_facts

FIXTURE = ROOT / "src/tests/fixtures/graphics/station-source-geometry.json"
FIXTURE_SHA256 = "d25dafc14ea73e8e836b8097128b79700c6a3e27cfb6e55cc90e5c345b986303"
EPOCH = "395103b5672c992aad21ccc928cac24e886c294e"
OWNERS = {"facility.rail-station": "station", "facility.road-terminal": "terminal"}


def original_facts():
    """Capture only original descriptors/records after independent history/report verification.

    @return Compact immutable identities; requires original witnesses only during preservation.
    """
    facts = load_facts()
    public = json.loads((ROOT / "media/multilayer/contracts/multilayer_sprite_manifest.json").read_text(encoding="utf-8"))
    rows = {row["asset_id"]: row for row in public["assets"]}
    result = {}
    # Descriptor/record bytes must precede cleanup implementation. Original
    # scoped source reports independently bind the fitting/state expectations.
    for asset, name in OWNERS.items():
        descriptor = ROOT / ("media/multilayer/contracts/conventional-" + name + "-geometry.json")
        record = ROOT / ("media/multilayer/source/ground/" + name + "-generation-record.json")
        for path in (descriptor, record):
            historical = subprocess.run(["git", "--no-optional-locks", "show", EPOCH + ":" + path.relative_to(ROOT).as_posix()],
                                        cwd=ROOT, capture_output=True, check=True).stdout
            if json.loads(historical) != json.loads(path.read_text(encoding="utf-8")):
                raise ValueError("station source input differs from original pre-cleanup Git epoch")
        original = facts["source_pose_contracts"][asset]["original_report"]
        report_path = ROOT / original["path"]
        if identity(report_path) != original:
            raise ValueError("station original scoped report changed before fact preservation")
        report = json.loads(report_path.read_text(encoding="utf-8"))
        family = json.loads(record.read_text(encoding="utf-8"))["families"][asset]
        # Source geometry and state ownership cannot be inferred from a new
        # producer. Preserve only facts agreed by original records and reports.
        if report["equipment_geometry"] != family["state_component_geometry"] or report["native_state_policy"] != rows[asset]["native_state_policy"]:
            raise ValueError("station original report/record physical state expectations differ")
        result[asset] = dict(descriptor=identity(descriptor), record=identity(record), original_report=original,
                             native_anchor_renderer_sha256=rows[asset]["native_anchor_renderer_sha256"])
    return dict(_documentation=__doc__, schema_version=1, kind="original_station_source_geometry",
                original_git_epoch=EPOCH, scope=facts["scope"], families=result, native_proof=False)


def export_facts():
    """Preserve indispensable original identities once; never update an existing baseline.

    @return Actual fixture identity after historical-only original witness checks.
    """
    encoded = (json.dumps(original_facts(), indent=2) + "\n").encode("utf-8")
    if FIXTURE.exists():
        if FIXTURE.read_bytes() != encoded:
            raise ValueError("original station source expectation cannot be replaced")
    else:
        FIXTURE.write_bytes(encoded)
    return identity(FIXTURE)


def load_originals():
    """Read exact retained source-era identities without old reports or Git access.

    @return Immutable original descriptor/record/renderer expectations.
    """
    if not FIXTURE.is_file():
        raise ValueError("original station geometry expectation is missing; restore retained bytes")
    if identity(FIXTURE)["sha256"] != FIXTURE_SHA256:
        raise ValueError("original station geometry expectation changed; no automatic baseline update")
    document = json.loads(FIXTURE.read_text(encoding="utf-8"))
    if document["scope"] != load_facts()["scope"]:
        raise ValueError("original station geometry scope changed")
    return document


def verify_original(document, role, asset=None, path=None):
    """Verify exact source-era geometry or original selection without current native authority.

    @param document Parsed immutable descriptor or generation record supplied by its owner.
    @param role Explicit descriptor or record identity being checked.
    @param asset Existing station/terminal family, required for original records.
    @param path Actual descriptor file selected by its existing native owner.
    @return Independently retained original family facts; default native gates stay separate.
    """
    families = load_originals()["families"]
    if asset is None:
        matches = [row for row in families.values() if ROOT / row["descriptor"]["path"] == Path(path)]
        if len(matches) != 1:
            raise ValueError("unknown original station geometry owner")
        row = matches[0]
    else:
        row = families[asset]
    reference = row[role]
    original = contained(ROOT / reference["path"], ROOT / "media")
    # Neither an edited file nor a detached coherent mutation can borrow
    # historical source authority by keeping only a plausible descriptor name.
    if identity(original) != reference or document != json.loads(original.read_text(encoding="utf-8")):
        raise ValueError("original station source " + role + " changed")
    return row


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--export", action="store_true")
    args = parser.parse_args()
    print(json.dumps(export_facts() if args.export else load_originals(), indent=2))
