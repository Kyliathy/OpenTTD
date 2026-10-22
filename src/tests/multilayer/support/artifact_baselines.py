"""Retain bounded independent semantic witnesses before disposable trees vanish.

@file artifact_baselines.py Historical tube, phase-pair and prior-stage observations.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group9.2.
No application, screenshot gallery or generated expected baseline is retained.
Historical paths inside copied receipts are facts, not live dependency requests.
"""

import json
from pathlib import Path

from artifact_inventory import checked_path, file_identity
from artifact_paths import repository_root
from artifact_preservation import preserve_file, publish_document

TUBE_OWNER = "src/tests/fixtures/checkpoints/underground"
PHASE_INDEX = "mlm/reviews/space/phase-pair-index/20261014-123013-61cbad237c/reports/phase-pair-index.json"
SEMANTIC_PREFIXES = ("FEEDBACK_RESULT ", "MOTION_SNAPSHOT ", "MOTION_TRACE ", "MOTION_RESULT ",
                     "PACE_SEED ", "PACE_LOADED ", "PACE_FRACTION ", "PACE_CUT ")
PRIOR_MARKERS = ("legacy=false editor=true", "Unsupported or invalid multilayer metadata",
                 "Missing required multilayer metadata chunk", "Unknown multilayer policy flags",
                 "MLVS_GEOLOGY generator=1", "MLVS_GEOLOGY_LOADED generator=")


def retained_witness(original, destination, root, expected=None):
    """Retain exact independent history with an explicit live-reference boundary.

    @param original Exact existing source spelling, never a latest-run selection.
    @param destination Durable fixture owner for the original immutable bytes.
    @param root Explicit source checkout.
    @param expected Optional recorded SHA256; None records the current original.
    @return Original/retained identity pair, both matching the same exact bytes.
    """
    source = checked_path(original, root)
    digest = expected or file_identity(source)[0]
    retained = preserve_file(source, destination, digest, root)
    return dict(original=dict(path=source.relative_to(root).as_posix(), sha256=digest), retained=retained)


def retain_underground_history(root):
    """Protect both paused source epochs and completed Group9's independent facts.

    @param root Explicit checkout with the original paused underground work.
    @return Historical contract with only its declared retained inputs live.
    """
    original_owner = "logs/underground-tubes/20261021-models-01"
    witnesses = []
    epochs = {}
    # The two source snapshots are different epochs. Combining them would hide
    # the provisional definition edits that have never been built or qualified.
    for label, name in (("before_edit", "before.json"), ("partial_definitions", "before-definitions.json")):
        witness = retained_witness(f"{original_owner}/{name}", f"{TUBE_OWNER}/history/{name}", root)
        witnesses.append(witness)
        epochs[label] = witness["retained"]
    qualified_path = "logs/underground-tubes/20261021-terminal-01/qualification.json"
    qualified = json.loads(checked_path(qualified_path, root).read_text(encoding="utf-8"))
    witnesses.append(retained_witness(qualified_path, f"{TUBE_OWNER}/history/group9-qualification.json", root))
    configurations = {}
    for name, row in qualified["configurations"].items():
        # Preserve actual terminal/process observations without copying the old
        # executable. Its quoted identity remains a non-dereferenced past fact.
        for field, reference in (("terminal", row["observation"]["terminal"]), ("execution", row["execution"])):
            suffix = ".log" if field == "terminal" else ".json"
            witness = retained_witness(reference["path"], f"{TUBE_OWNER}/history/{field}-{reference['sha256']}{suffix}", root, reference["sha256"])
            witnesses.append(witness)
        configurations[name] = dict(observation=row["observation"], historical_tested_image=row["tested_image"])
    # Exact historical source witnesses explain qualified packet/terminal rules;
    # they do not replace or compile over the current provisional fleet source.
    for logical, reference in qualified["retained_sources"].items():
        destination = f"{TUBE_OWNER}/history/sources/{reference['sha256'][:16]}-{Path(logical).name}"
        witnesses.append(retained_witness(reference["path"], destination, root, reference["sha256"]))
    result = dict(_documentation="Owner: r2fc-file-cleanup3.md9/r2mlvs17; only live_retained_inputs.retained are physical dependencies; nested old paths are history.",
        schema_version=1, kind="paused_underground_history", source_epochs=epochs,
        status=dict(group9="historically_complete", group10="provisional_not_built_or_tested", feature_work_paused=True),
        configurations=configurations, qualification=qualified, live_retained_inputs=witnesses,
        fresh_build_qualified=False, source_preservation_pending=True)
    publish_document(f"{TUBE_OWNER}/history.json", result, root)
    return result


def retain_phase_history(root):
    """Retain the original paid/seeded phase distinction and exact compact reports.

    @param root Explicit checkout containing the accepted phase-pair index.
    @return Historical report identities and disclosed missing/changed old edges.
    """
    owner = "src/tests/fixtures/space/phase-history"
    index = json.loads(checked_path(PHASE_INDEX, root).read_text(encoding="utf-8"))
    witnesses = [retained_witness(PHASE_INDEX, f"{owner}/original-index.json", root)]
    unresolved = []
    # These are the index's typed report edges, not every path-shaped JSON
    # string. Missing old reports remain explicit history gaps, never fresh proof.
    for reference in index["reports"]:
        source = checked_path(reference["path"], root)
        if not source.is_file():
            unresolved.append(dict(reference=reference, status="missing_historical_report"))
            continue
        if file_identity(source)[0] != reference["sha256"]:
            unresolved.append(dict(reference=reference, status="changed_historical_report"))
            continue
        destination = f"{owner}/reports/{reference['sha256'][:16]}-{source.name}"
        witnesses.append(retained_witness(source, destination, root, reference["sha256"]))
    result = dict(_documentation="Owner: r2fc-file-cleanup3.md9/r2mlvs9 Group17; independent original phase observations; no financial replay authorization.",
        schema_version=1, kind="historical_phase_pairs", live_retained_inputs=witnesses,
        unresolved_historical_reports=unresolved, pairs=index["pairs"], normalized_probe_ledgers=index["normalized_probe_ledgers"],
        seeded_kind="seeded_packet_handling_boundary", genuine_kinds=["genuine_paid_probe", "genuine_paid_module_freight", "genuine_paid_descent_service"],
        fresh_application_ran=False, gameplay_ticks_executed=0)
    publish_document(f"{owner}/history.json", result, root)
    return result


def retain_prior_stage_observations(root):
    """Extract only actual prior-stage comparison lines from original native logs.

    @param root Explicit checkout; only named feedback/motion/profile/geology logs qualify.
    @return Compact independent semantic fixture, not an executed-run substitute.
    """
    records = []
    # No unbounded transcript archive is retained. These concrete producers use
    # the listed lines for reload/pacing/editor/refusal comparisons; other log
    # output is disposable and future live proof requires explicit preparation.
    for source in sorted((root / "logs").rglob("*.log")):
        if not source.name.startswith(("r2tbmf", "r2mlvs1-h-", "r2mlvs2-g-")):
            continue
        checked_path(source, root)
        before = file_identity(source)[0]
        lines = source.read_text(encoding="utf-8", errors="replace").splitlines()
        selected = [line for line in lines if line.startswith(SEMANTIC_PREFIXES) or any(marker in line for marker in PRIOR_MARKERS)]
        if file_identity(source)[0] != before:
            raise ValueError(f"native prior-stage log changed while reading: {source}")
        if selected:
            records.append(dict(original=dict(path=source.relative_to(root).as_posix(), sha256=before), lines=selected))
    result = dict(_documentation="Owner: r2fc-file-cleanup3.md9/21; independent original comparison lines; not current-run acceptance or paid scope.",
                  schema_version=1, kind="historical_prior_stage_observations", records=records,
                  retained_full_transcripts=False, gameplay_ticks_executed=0)
    publish_document("src/tests/fixtures/multilayer/prior-stage-observations.json", result, root)
    return result


def main():
    """Explicitly promote the named independent baseline families without replay.

    @return None; prints compact counts, with no native build or simulation.
    """
    root = repository_root()
    underground = retain_underground_history(root)
    phase = retain_phase_history(root)
    prior = retain_prior_stage_observations(root)
    print(json.dumps(dict(underground_witnesses=len(underground["live_retained_inputs"]),
        phase_witnesses=len(phase["live_retained_inputs"]), phase_history_gaps=len(phase["unresolved_historical_reports"]),
        prior_observations=len(prior["records"])), indent=2))


if __name__ == "__main__":
    main()
