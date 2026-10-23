"""Seal current portal/tube consumer boundaries against exact retained useful/runtime handoffs.

@file portal_consumer_report.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md Group12B.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs3-sealed-portals-and-deep-rail.md Group12C; shared exact native matrix verification.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Native boundary inputs do not replace the separate original paid/bootstrap/final-use evidence.
"""

import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
from portal_consumer_return import ROOT, SOURCES, TUBE_CASES
from construction_provider_return import identity

CONFIGURATIONS = ("Debug", "RelWithDebInfo")
HANDOFFS = {
    "five-model-useful-files": ("logs/tubes/forge-files/20261018-native-01/five-model-verification.json",
                                "28158c1b1aa03cefc608da94fa7cf7b25ef85f4c116269b8b57161b9cde53d60"),
    "selected-gallery": ("logs/graphics/selected-guidance/20261019-native-01/publication-verification.json",
                         "799dfe322bf03fc3e2e9238dee4a6cbd3ee5283a8bcb149e8f8eb436355c253e"),
}


def checked(reference, cache):
    """Require exact current bytes for one retained source, executable, save or log.

    @param reference Repository-relative path, byte count and SHA256 selected by its owner.
    @param cache Per-acceptance actual identities, avoiding repeated large executable reads.
    @return Contained existing path after complete identity verification.
    """
    path = (ROOT / reference["path"]).resolve()
    assert path.is_relative_to(ROOT)
    if path not in cache:
        cache[path] = identity(path)
    assert cache[path] == reference, reference
    return path


def verify_matrix(selection, required_cases):
    """Verify one explicit current both-reader consumer matrix without publishing a receipt.

    @param selection Complete exact report/build-log index already parsed by its owning acceptance phase.
    @param required_cases Required focus-to-single-native-case mapping from the existing consumer runner.
    @return Per-reader counts, complete verified records and shared actual-identity cache.
    """
    assert selection["complete"] is True
    cache, seen, records = {}, set(), []
    expected = {(cfg, focus) for cfg in CONFIGURATIONS for focus in required_cases}
    summary = {cfg: {"cases": 0, "assertions": 0} for cfg in CONFIGURATIONS}
    # Every required consumer obligation must pass independently in both real
    # readers; an unrelated focus cannot fill a missing commissioning boundary.
    for row in selection["records"]:
        cfg, focus = row["configuration"], row["focus"]
        key = cfg, focus
        assert key in expected and key not in seen, key
        seen.add(key)
        path = ROOT / row["report"]
        report = json.loads(path.read_text(encoding="utf-8"))
        assert (report["configuration"], report["focus"], report["tag"]) == (cfg, focus, required_cases[focus])
        assert report["native_version"] == 434
        assert report["controller"]["path"] == "src/tests/multilayer/portal_consumer_return.py"
        assert report["boundary_runner"]["path"] == "src/tests/multilayer/construction_provider_return.py"
        assert report["binary"]["path"] == f"build/{cfg}/openttd_test.exe"
        assert [ref["path"] for ref in report["sources"]] == list(SOURCES)
        # A dated executable or changed test/source cannot supply a fresh
        # consumer pass; every current shared identity is checked once exactly.
        for ref in [report["controller"], report["boundary_runner"], report["binary"], *report["sources"]]:
            checked(ref, cache)
        raw = checked(report["native"]["log"], cache).read_text(encoding="utf-8")
        assert "NATIVE_EXIT_CODE=0" in raw
        assert f"Filters: {required_cases[focus]}" in raw
        counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", raw)
        assert counts is not None
        assertions, cases = int(counts[1]), int(counts[2])
        assert cases == report["native"]["cases"] == 1
        assert assertions == report["native"]["assertions"] and assertions > 0
        summary[cfg]["cases"] += cases
        summary[cfg]["assertions"] += assertions
        records.append({"report": identity(path), **report})
    assert seen == expected, expected - seen
    return summary, records, cache


def accept(selection_path, output_path):
    """Verify both current readers and bind exact retained five-model useful/intermodal history.

    @param selection_path Explicit complete report/build-log index; no latest-file discovery.
    @param output_path Absent accepted receipt beneath repository logs.
    @return Complete sealed receipt, preserving every native and retained owner distinction.
    """
    selection_path = ROOT / selection_path
    selection = json.loads(selection_path.read_text(encoding="utf-8"))
    summary, records, cache = verify_matrix(selection, TUBE_CASES)
    handoffs, documents = {}, {}
    # Completed native useful routes/files/raster and the current gallery are
    # retained sealed upstream evidence. Their dated binaries/runtime copies
    # are not reopened or falsely relabelled as this current boundary matrix.
    for role, (name, sha256) in HANDOFFS.items():
        path = ROOT / name
        reference = identity(path)
        assert reference["sha256"] == sha256, role
        handoffs[role] = reference
        documents[role] = json.loads(path.read_text(encoding="utf-8"))
    models = documents["five-model-useful-files"]["models"]
    assert [row["model"] for row in models] == [9, 10, 11, 12, 13]
    original_finals = []
    # Both original writers for all five actual models must retain useful
    # delivery, positive native freight income, whole empty returns and the
    # exact saved inputs already accepted by their owning file/raster audit.
    for model in models:
        assert set(model["original_finals"]) == set(CONFIGURATIONS)
        for cfg, final in model["original_finals"].items():
            assert final["useful_units"] > 0 and final["native_freight_income"] > 0 and final["whole_empty_returns"] > 0
            checked(final["original_final"], cache)
            original_finals.append({"model": model["model"], "configuration": cfg, **final})
    review = documents["selected-gallery"]
    assert review["technical_acceptance"] == "verified"
    assert review["native_frames"] == 2209
    checked(review["public_manifest"], cache)
    checked(review["selection"], cache)
    build_logs = {cfg: identity(ROOT / selection["build_logs"][cfg]) for cfg in CONFIGURATIONS}
    result = {
        "_documentation": "r2mlvs3-sealed-portals-and-deep-rail.md12B; current actual commissioning/domain/shaft/floor/interchange acceptance and sealed original five-model useful history;paid TBM conversion/final-use remains with Plan4/Plan17",
        "controller": identity(__file__), "selection": identity(selection_path),
        "summary": summary, "native_cases": sum(row["cases"] for row in summary.values()),
        "native_assertions": sum(row["assertions"] for row in summary.values()),
        "native_version": 434, "build_logs": build_logs, "records": records,
        "retained_handoffs": handoffs, "original_useful_finals": original_finals,
        "artwork": identity(ROOT / "media/baseset/openttd.grf"),
        "classification": "Current tests use disclosed prepared floor/material participants;original retained useful routes prove physical intermodal carriage independently,not full paid TBM final use.",
        "remaining": "Plan3-12C original-portal/date-valid risers before12.1 closure;master10.8 remaining Plan4 Group15/21 actual paid final use and human/network/operator gates remain pending.",
    }
    output = (ROOT / output_path).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"receipt": identity(output), "summary": summary}), flush=True)
    return result


def main():
    """Select exact completed consumer observations and an absent retained closure.

    @return None; incomplete/stale/duplicate matrices or missing original inputs refuse.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--selection", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    accept(args.selection, args.output)


if __name__ == "__main__":
    main()
