"""Verify exact original native stages and ordinary static loading without replaying work.

@file surface_original_evidence.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CK.
Callers supply explicit observers, ordered receipts and before-edit manifests.
This helper verifies artifacts; it owns no gameplay inventory, money or time.
Group15CSGZVEC may explicitly provide invocation-local unchanged-file identity
reuse; every historical caller retains the original uncached default behavior.
"""
import json
from pathlib import Path
import runpy
import shutil

from space_preview_checks import ROOT, chunks, identity
from surface_gateway_replay import RESTORE

DIRECTORY = Path(__file__).resolve().parent
STATIC = runpy.run_path(str(DIRECTORY / "surface_entry_salt_window_checks-4.py"))


def verify(reference, versions=(), identity_reader=None):
    """Verify current bytes or an exact authority from explicitly selected before-edit manifests.

    @param reference Original path/hash identity from an actual receipt.
    @param versions Caller-selected immutable native/source version manifests.
    @param identity_reader Optional explicit read-only identity provider; None preserves original uncached reads.
    @return Whether the original path still contains these exact current bytes.
    """
    read_identity = identity if identity_reader is None else identity_reader
    path = ROOT / reference["path"]
    if path.is_file():
        if read_identity(path) == reference:
            return True
    selected = []
    # A caller's explicit pre-edit path and hash must both match. Arbitrary
    # historical artifacts cannot authorize stale current sources or processes.
    for manifest in versions:
        for row in json.loads(manifest.read_text(encoding="utf-8"))["products"]:
            if row["source"] == reference["path"] and row["sha256"] == reference["sha256"]:
                selected.append(row)
    assert selected, reference
    assert all(read_identity(ROOT / row["path"])["sha256"] == reference["sha256"] for row in selected)
    return False


def restore(configuration, receipt, observer, scenario, documentation):
    """Load/resave only the selected actual original through the ordinary application.

    @param configuration Independently linked complete ordinary game application.
    @param receipt Actual current original receipt; its output is never replaced by this copy.
    @param observer Caller-specific current native ownership contract.
    @param scenario Unique ordinary-run label for this explicit group.
    @param documentation Caller file/plan provenance recorded with the proof.
    @return Static ordinary process, exact source and preserved actual owners.
    """
    predecessor = json.loads(receipt.read_text(encoding="utf-8"))
    assert predecessor["complete"] and predecessor["configuration"] == configuration
    original = predecessor["operation"]["output"]
    verify(original)
    source = ROOT / original["path"]
    restored, operation = RESTORE["ORDINARY"](configuration, source, scenario + "-" + configuration.lower())
    RESTORE["REVISION"](source, restored)
    pause = RESTORE["restore_pause"](source, restored)
    first, last = [chunks(path)[1] for path in (source, restored)]
    assert first.keys() == last.keys() and len(first) == 121
    assert all(first[key] == last[key] for key in first if key not in (b"GLOG", b"DATE"))
    assert observer(source) == observer(restored)
    return dict(_documentation=documentation, complete=True, configuration=configuration, phase="restore",
                receipt=identity(receipt), operation=operation, original_pause=pause, source=identity(Path(__file__)))


def bind(foundation_path, versions, names, static_name, observer, sources, retained_directory=None, identity_reader=None):
    """Verify ordered original stages, both configurations and ordinary static copies.

    @param foundation_path Completed immediately preceding group's exact proof.
    @param versions Explicit before-edit native/source manifests; no historical wildcard.
    @param names Ordered native receipt suffixes after logs/surface-entry-, before configuration.
    @param static_name Ordinary receipt suffix for each selected current original.
    @param observer Caller-specific actual current custody/material observation.
    @param sources Current caller/helper authorities additionally retained with their hashes.
    @param retained_directory Fresh test-cache destination, or None for verification only.
    @param identity_reader Optional explicit read-only identity provider; None preserves every original uncached read.
    @return Exact native/cross-build/static acceptance and current original observations.
    """
    read_identity = identity if identity_reader is None else identity_reader

    def check_reference(reference, selected_versions):
        """@param reference Exact original identity. @param selected_versions Explicit immutable epochs. @return Current-byte equality or an exact archived match."""
        return verify(reference, selected_versions, identity_reader=read_identity)

    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    assert foundation["complete"]
    check_reference(foundation["products"], versions)
    previous = json.loads((ROOT / foundation["products"]["path"]).read_text(encoding="utf-8"))
    changed = set()
    for product in previous["products"]:
        check_reference(product["retained"], versions)
        if not check_reference(product["source"], versions):
            changed.add(product["source"]["path"])
    products, processes, worlds, diagnostics, configurations = {}, set(), [], [], []
    for configuration in ("RelWithDebInfo", "Debug"):
        origin, = [row for row in foundation["configurations"] if row["configuration"] == configuration]
        predecessor = origin["receipt"]
        check_reference(predecessor, versions)
        stages, rows = [], []
        # Each stage must consume precisely its preceding original output.
        # Static copies and unrelated successful processes cannot substitute
        # for a paid command, earlier partial work or current physical stock.
        for name in names:
            path = ROOT / f"logs/surface-entry-{name}-{configuration}.json"
            receipt = json.loads(path.read_text(encoding="utf-8"))
            assert receipt["complete"] and receipt["configuration"] == configuration
            operation = receipt["operation"]
            assert operation["receipt"] == predecessor
            assert operation["source"] == json.loads((ROOT / predecessor["path"]).read_text(encoding="utf-8"))["operation"]["output"]
            for field in ("source", "before", "output", "application", "terminal", "retained_harness"):
                check_reference(operation[field], versions)
            for reference in operation["sources"] + [operation["application"]]:
                if check_reference(reference, versions):
                    products[reference["path"]] = reference
            for reference in operation["native_processes"]:
                check_reference(reference, versions)
                report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
                assert report["exit_status"] == 0 and report["inputs"] == [operation["source"]]
                assert report["executable"] == operation["application"]
                processes.add(reference["path"])
            for field, recorded in (("before", "initial"), ("output", "final")):
                observed = observer(ROOT / operation[field]["path"])
                assert json.loads(json.dumps(observed)) == operation[recorded]
            first, last = [chunks(ROOT / operation[field]["path"])[1] for field in ("before", "output")]
            if receipt["phase"].endswith("-query"):
                assert first == last and operation["initial"] == operation["final"]
            stages.append(last)
            rows.append(operation["rows"])
            predecessor = read_identity(path)
        final = observer(ROOT / operation["output"]["path"])
        static_path = ROOT / f"logs/surface-entry-{static_name}-{configuration}.json"
        restored = json.loads(static_path.read_text(encoding="utf-8"))
        assert restored["complete"] and restored["receipt"] == predecessor
        ordinary = restored["operation"]
        assert ordinary["source"] == operation["output"]
        for field in ("source", "output", "application", "terminal"):
            check_reference(ordinary[field], versions)
        for reference in ordinary["native_processes"]:
            check_reference(reference, versions)
            report = json.loads((ROOT / reference["path"]).read_text(encoding="utf-8"))
            assert report["exit_status"] == 0 and report["inputs"] == [operation["output"]]
            STATIC["verify_ordinary_application"](report["executable"], ordinary["application"])
            products[report["executable"]["path"]] = report["executable"]
            processes.add(reference["path"])
        products[ordinary["application"]["path"]] = ordinary["application"]
        assert observer(ROOT / ordinary["output"]["path"]) == final
        normalized = dict(chunks(ROOT / ordinary["output"]["path"])[1])
        normalized[b"DATE"] = last[b"DATE"]
        stages.append(normalized)
        worlds.append(stages)
        diagnostics.append(rows)
        configurations.append(dict(configuration=configuration, receipt=predecessor, output=operation["output"],
                                   observation=final, ordinary=read_identity(static_path)))
    assert len(processes) == 2 * (len(names) + 1) and diagnostics[0] == diagnostics[1]
    comparisons = 0
    for first, second in zip(*worlds, strict=True):
        assert first.keys() == second.keys() and len(first) == 121
        assert all(first[key] == second[key] for key in first if key != b"GLOG")
        comparisons += len(first) - 1
    result = dict(complete=True, foundation=read_identity(foundation_path), historical_versions=[read_identity(path) for path in versions],
                  configurations=configurations, native_processes=len(processes), cross_build_gameplay_comparisons=comparisons,
                  ordinary_static_reloads=2, source=read_identity(Path(__file__)))
    if retained_directory is not None:
        retained_directory.mkdir(parents=True, exist_ok=False)
        copies = [product for product in previous["products"] if product["source"]["path"] not in changed]
        existing = {product["source"]["path"] for product in copies}
        for path in [Path(__file__)] + list(sources) + list(versions):
            reference = read_identity(path)
            products[reference["path"]] = reference
        # Keep unchanged prior authorities at their exact retained locations.
        # Copy only new current tested authorities; historical versions remain
        # qualified by the caller's explicit immutable before-edit manifests.
        for index, reference in enumerate(products.values()):
            if reference["path"] in existing:
                continue
            source = ROOT / reference["path"]
            target = retained_directory / f"{index:03d}-{source.parent.name}-{source.name}"
            shutil.copyfile(source, target)
            assert read_identity(target)["sha256"] == reference["sha256"]
            copies.append(dict(source=reference, retained=read_identity(target)))
            existing.add(reference["path"])
        manifest = retained_directory / "retained.json"
        manifest.write_text(json.dumps(dict(_documentation=__doc__, predecessor=foundation["products"], products=copies), indent=2) + "\n", encoding="utf-8")
        result.update(products=read_identity(manifest), retained_products=len(copies))
    return result
