"""Verify retained Tube terminal history separately from paused provisional source.

@file qualify_tube_terminals.py Historical Group9 facts grant no fresh native qualification.
@see docs/zz-reach2/upgrades/2026-10/r2fc2-file-cleanup3.md Group20C.
@see docs/zz-reach2/architecture/archi-historical-debug.md.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group9.
"""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
CHECKPOINT = "src/tests/fixtures/checkpoints/underground/checkpoint.json"
TAGS = {"transport-tube-model", "transport-tube-construction", "transport-tube-readiness",
        "transport-tube-platform-geometry", "transport-tube-meter", "transport-tube-interchange-geometry",
        "transport-tube-interchange", "transport-tube-interchange-delivery"}


def exact(row, root=ROOT):
    """Check one durable retained payload without following historical aliases.

    @param row Exact durable repository path and independently retained SHA256.
    @param root Checkout containing the original checkpoint, source epochs and transcripts.
    @return Contained hash-verified payload path.
    """
    path = (root / row["path"]).resolve()
    if not path.is_relative_to(root.resolve()):
        raise ValueError("tube retained input escapes the selected checkout")
    if hashlib.sha256(path.read_bytes()).hexdigest() != row["sha256"]:
        raise ValueError("tube retained input changed: " + row["path"])
    return path


def verify_checkpoint(root=ROOT):
    """Verify bounded historical terminal scope without old applications or generated logs.

    @param root Checkout owning all durable Tube history and provisional source payloads.
    @return Historical results and precise current/original source-epoch differences.
    """
    checkpoint = json.loads((root / CHECKPOINT).read_text(encoding="utf-8"))
    history = json.loads(exact(checkpoint["history"], root).read_text(encoding="utf-8"))
    expected_status = dict(group9="historically_complete", group10="provisional_not_built_or_tested",
                           feature_work_paused=True)
    # Historical terminal success and provisional model work have separate
    # authority; neither a new compile nor copied image may collapse that boundary.
    if (checkpoint["status"] != expected_status or history["status"] != expected_status
            or checkpoint["fresh_build_qualified"] is not False
            or checkpoint["feature_work_paused"] is not True):
        raise ValueError("tube checkpoint lost its historical/provisional boundary")
    aliases = {}
    # Dereference only the declared durable owners. Old paths and executable
    # hashes inside retained receipts remain dated facts, never live dependencies.
    for row in history["live_retained_inputs"]:
        path = exact(row["retained"], root)
        if row["original"]["sha256"] != row["retained"]["sha256"]:
            raise ValueError("tube retained alias changed original payload identity")
        aliases[row["original"]["path"]] = path
    epochs = [exact(row, root) for row in checkpoint["source_epochs"].values()]
    if len(set(epochs)) != 2:
        raise ValueError("tube before/partial-definition source epochs were merged")
    for row in checkpoint["provisional_sources"]:
        exact(row["retained"], root)
    qualification = history["qualification"]
    # Completed terminal work exercised model9 only. Route, final art and the
    # five-model task remain explicitly open after historical reconstruction.
    if (qualification["passed"] is not True or qualification["operating_models"] != [9]
            or qualification["all_five_models"] is not False or qualification["final_art"] is not False
            or qualification["tbm_cleared_cross_layer_useful_route"] is not False
            or qualification["whole_underground_complete"] is not False):
        raise ValueError("tube historical qualification expanded its original terminal scope")
    # Both actual historical processes retain their exact eight-tag completion
    # transcript. Checking the retained transcript requires no historical EXE.
    for configuration in ("Debug", "RelWithDebInfo"):
        observation = history["configurations"][configuration]["observation"]
        tags = {tag.strip("[]") for tag in observation["filter"].split(",")}
        text = aliases[observation["terminal"]["path"]].read_text(encoding="utf-8")
        if (observation["exit_code"] != 0 or observation["cases"] != 24
                or observation["assertions"] != 8687 or tags != TAGS
                or "All tests passed (8687 assertions in 24 test cases)" not in text):
            raise ValueError("tube historical terminal matrix changed: " + configuration)
    differences = []
    # Compare live source with the original executed epoch solely to explain
    # fresh-run refusal; changed source must never relabel historical execution.
    for name, original in qualification["retained_sources"].items():
        path = root / name
        actual = hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None
        if actual != original["sha256"]:
            differences.append(dict(path=name, historical_sha256=original["sha256"], current_sha256=actual))
    art = {}
    # Protect original authored pixels, palettes and shipping GRFs across both
    # baseline epochs, while changed producer code remains a separate source epoch.
    for path in epochs:
        baseline = json.loads(path.read_text(encoding="utf-8"))
        for row in baseline["files"]:
            # Media payloads carry irreplaceable original bytes; Python producer
            # migrations are source changes and cannot be mistaken for lost art.
            if row["path"].startswith("media/") and Path(row["path"]).suffix.lower() in (".png", ".grf", ".pal"):
                exact(row, root)
                art[row["path"]] = row["sha256"]
    return dict(_documentation=__doc__, kind="historical_tube_terminal_check", complete=True,
        historical_terminal_qualified=True, cases_per_configuration=24, assertions_per_configuration=8687,
        configurations=["Debug", "RelWithDebInfo"], operating_models=[9], all_five_models=False,
        current_native_qualified=False, feature_work_paused=True, no_executable_archive=True,
        tbm_cleared_cross_layer_useful_route=False, human_acceptance=False,
        source_epoch_differences=differences, retained_input_count=len(history["live_retained_inputs"]),
        protected_art_files=len(art), checkpoint=CHECKPOINT)


def require_original_source(root=ROOT):
    """Refuse legacy native execution when its original terminal epoch is unavailable.

    @param root Explicit checkout selected by the legacy terminal caller.
    @return None only when all original executed sources still match exactly.
    """
    result = verify_checkpoint(root)
    if result["source_epoch_differences"]:
        names = ", ".join(row["path"] for row in result["source_epoch_differences"])
        raise ValueError("original Tube terminal source epoch differs: " + names
                         + "; Group10 remains provisional and paused; historical Group9 evidence is retained")


def main():
    """Inspect retained history or publish its compact result under a cleanable review run.

    @return None; altered durable inputs refuse without rebuilding provisional feature code.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--run-id", help="Optional explicit fresh review run; omit for read-only inspection")
    args = parser.parse_args()
    result = verify_checkpoint()
    if args.run_id:
        import sys
        sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
        from artifact_paths import create_run_context
        context = create_run_context("underground-tubes", "terminal-history", run_id=args.run_id, root=ROOT)
        target = context.reports / "history.json"
        with target.open("x", encoding="utf-8") as stream:
            stream.write(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
