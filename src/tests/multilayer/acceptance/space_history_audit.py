"""Validate existing Space history through exact retained storage aliases only.

@file space_history_audit.py No native execution, runtime restoration or EXE copies.
@see docs/zz-reach2/upgrades/2026-10/r2fc-file-cleanup3.md Group7.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md GZVEEH.
The unchanged original verifier remains authoritative. A private call namespace
selects byte-identical retained console images for its existing PE comparison.
This is historical validation, never fresh application qualification.
"""

import argparse
import hashlib
import json
from pathlib import Path
import runpy
import subprocess
import sys
import time
from types import FunctionType
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[4]
SUPPORT = ROOT / "src/tests/multilayer/support"
sys.path.insert(0, str(SUPPORT))
from artifact_inventory import checked_path, file_identity
from artifact_paths import contained, owned_root


def identity(path):
    """Read one exact repository-owned identity without changing the source file.

    @param path Existing regular input or newly published audit artifact.
    @return Repository-relative path and full SHA256.
    """
    selected = checked_path(path, ROOT)
    return dict(path=selected.relative_to(ROOT).as_posix(), sha256=file_identity(selected)[0])


def verified_aliases(products, read_identity, root=ROOT):
    """Derive flat original-verifier aliases from an exact retained-product authority.

    @param products Parsed original source/retained pair manifest, already hash-qualified.
    @param read_identity Actual byte reader, optionally the original invocation-local cache.
    @param root Explicit contained checkout for isolated validation tests.
    @return Flat existing-verifier rows plus original path/hash to retained identity mapping.
    @throws ValueError For disagreeing authorities, corrupt archives or redirected paths.
    """
    rows, aliases, names = [], {}, {}
    # A storage alias proves only an already declared source/retained equality.
    # It cannot invent a historical source epoch or substitute another native image.
    for product in products["products"]:
        source, retained = product["source"], product["retained"]
        checked_path(source["path"], root)
        target = checked_path(retained["path"], root)
        if source["sha256"] != retained["sha256"]:
            raise ValueError(f"retained source identity disagrees: {source['path']}")
        if read_identity(target) != retained:
            raise ValueError(f"retained source bytes changed: {retained['path']}")
        canonical = str(target).casefold()
        # A filename cannot certify competing identities even if several logical
        # names legitimately share the same byte-identical retained payload.
        if canonical in names and names[canonical] != retained["sha256"]:
            raise ValueError(f"conflicting retained storage alias: {retained['path']}")
        names[canonical] = retained["sha256"]
        key = (source["path"], source["sha256"])
        aliases.setdefault(key, []).append(retained)
        rows.append(dict(source=source["path"], path=retained["path"], sha256=source["sha256"]))
    return rows, aliases


def select_retained(reference, aliases, read_identity, root=ROOT):
    """Resolve only an explicitly declared logical path/hash, never a basename match.

    @param reference Original exact identity from an actual historical process report.
    @param aliases Mapping derived solely from its hash-bound retained-product manifest.
    @param read_identity Actual byte reader used for every selected physical alias.
    @param root Explicit source checkout used by the supplied identity reader.
    @return Current original identity or a byte-identical explicitly declared retained identity.
    @throws ValueError If neither the original nor its declared exact archive can qualify.
    """
    original = checked_path(reference["path"], root)
    # Original locations retain precedence, but changed bytes cannot satisfy a
    # historical identity merely because the path still exists.
    if original.is_file() and read_identity(original) == reference:
        return reference
    selected = aliases.get((reference["path"], reference["sha256"]), [])
    if not selected:
        raise ValueError(f"missing exact historical storage alias: {reference['path']}")
    for candidate in selected:
        if read_identity(checked_path(candidate["path"], root)) != candidate:
            raise ValueError(f"changed historical storage alias: {candidate['path']}")
    return selected[0]


def private_call(function, replacements):
    """Reuse unchanged original function code with explicitly isolated storage dependencies.

    @param function Existing original verifier/main function; its code is never rewritten.
    @param replacements Selected globals for this private invocation only.
    @return Function sharing the original code/defaults and a copied globals dictionary.
    """
    namespace = {**function.__globals__, **replacements}
    return FunctionType(function.__code__, namespace, function.__name__,
        function.__defaults__, function.__closure__)


def audit(output):
    """Run unchanged legacy semantic checks using verified retained storage selections.

    @param output Explicit audit JSON path under test-cache; no durable seal is published here.
    @return Actual historical audit result, with the original CLI failure recorded separately.
    """
    started = time.monotonic()
    output = contained(checked_path(output, ROOT), owned_root(ROOT, "test-cache"))
    dependencies = json.loads((ROOT / "src/tests/fixtures/cleanup/space-dependencies.json").read_text(encoding="utf-8"))
    bound_path = checked_path(dependencies["root"]["path"], ROOT)
    if identity(bound_path) != dependencies["root"]:
        raise ValueError("live bound root advanced; stop historical sealing")
    # Source helpers and independently recorded expectations must remain the
    # captured epoch before consumed adapters can change for the new foundation.
    for reference in dependencies["current_observer_sources"]:
        if identity(ROOT / reference["path"]) != reference:
            raise ValueError(f"original observer/reader changed: {reference['path']}")
    bound = json.loads(bound_path.read_text(encoding="utf-8"))
    foundation_path = checked_path(bound["foundation"]["path"], ROOT)
    if identity(foundation_path) != bound["foundation"]:
        raise ValueError("original immediate foundation changed")
    foundation = json.loads(foundation_path.read_text(encoding="utf-8"))
    products_path = checked_path(foundation["products"]["path"], ROOT)
    if identity(products_path) != foundation["products"]:
        raise ValueError("original retained-product authority changed")
    products = json.loads(products_path.read_text(encoding="utf-8"))
    current_products_path = checked_path(bound["products"]["path"], ROOT)
    if identity(current_products_path) != bound["products"]:
        raise ValueError("original final retained-product authority changed")
    current_products = json.loads(current_products_path.read_text(encoding="utf-8"))
    # The immediate foundation protects its imported source epochs; the final
    # manifest additionally owns this group's two actual ordinary console images.
    # Both authorities are original hash-bound inputs, not new archive guesses.
    storage_products = dict(products=[*products["products"], *current_products["products"]])
    binder_path = Path(__file__).with_name("surface_entry_construction_cash_checks-3.py")
    # Importing the original entry point is explicit and cannot start any child
    # process. Its main selects metadata only; cash/native execute is never called.
    with patch.object(subprocess, "Popen", side_effect=RuntimeError("historical audit forbids child/native execution")) as child_guard:
        legacy = runpy.run_path(str(binder_path))
        cache = legacy["IdentityCache"]()
        rows, aliases = verified_aliases(storage_products, cache)
        alias_path = output.with_name("space-storage-aliases.json")
        alias_path.write_text(json.dumps(dict(_documentation=__doc__,
            kind="verified_existing_storage_aliases_not_new_source_epochs",
            authorities=[foundation["products"], bound["products"]], products=rows), indent=2) + "\n", encoding="utf-8")
        original_bind = legacy["bind"]
        static = original_bind.__globals__["STATIC"]
        original_ordinary = static["verify_ordinary_application"]
        ordinary_selections = []

        def verify_ordinary(actual, application):
            """Use the original full-byte PE projection verifier on exact retained bytes.

            @param actual Unchanged original runtime identity from its historical report.
            @param application Unchanged original GUI application identity from the wrapper.
            @return None; no new executable is copied, linked or executed.
            """
            launched = select_retained(actual, aliases, cache)
            source = select_retained(application, aliases, cache)
            original_ordinary(launched, source)
            ordinary_selections.append(dict(original=actual, selected=launched, application=application))

        # Private globals supply only the storage-aware ordinary selector. The
        # original verify/bind bodies, checks and loaded module globals stay intact.
        selected_bind = private_call(original_bind,
            dict(STATIC={**static, "verify_ordinary_application": verify_ordinary}))
        selected_main = private_call(legacy["main"], dict(bind=selected_bind,
            VERSIONS=[*legacy["VERSIONS"], alias_path]))
        report_path = ROOT / "logs/cleanup-space-storage-bind-20261021.json"
        sys.argv = [str(binder_path), "--windows", "32", "--output", report_path.relative_to(ROOT).as_posix()]
        selected_main()
        cache.assert_unchanged()
    if child_guard.called:
        raise ValueError("historical audit attempted a prohibited child process")
    result = json.loads(report_path.read_text(encoding="utf-8"))
    if result["configurations"] != bound["configurations"]:
        raise ValueError("recomputed final observations/identities differ from the original proof")
    # These exact process/comparison counts establish that all33original stages
    # and the two actual ordinary restores passed; a partial traversal cannot seal.
    if (result["native_processes"] != 68 or result["cross_build_gameplay_comparisons"] != 4080
            or result["ordinary_static_reloads"] != 2):
        raise ValueError("historical validation did not cover both complete original branches")
    for key in ("actual_cash", "startup_reserve", "working_floor_policy", "capital_scope", "measured_cash_flow"):
        if result[key] != bound[key]:
            raise ValueError(f"historical closed policy or cash flow changed: {key}")
    configurations = []
    # Record independent per-family expectations from the actual protected save;
    # no changed implementation or newly generated world supplies a baseline.
    for row in result["configurations"]:
        save = checked_path(row["output"]["path"], ROOT)
        version, chunks = legacy["CURRENT"]["chunks"](save)
        if len(chunks) != 121:
            raise ValueError("actual original checkpoint does not contain all121families")
        configurations.append(dict(**row, save_version=version,
            chunk_sha256={name.decode("ascii"): hashlib.sha256(data).hexdigest() for name, data in chunks.items()}))
    audit_result = dict(_documentation=__doc__, schema_version=1, kind="validated_historical_space_facts",
        complete=True, fresh_application_ran=False, gameplay_ticks_executed=0, child_process_attempts=0,
        original_root=dependencies["root"], immediate_foundation=bound["foundation"],
        source_epochs=bound["historical_versions"], retained_product_authority=foundation["products"],
        final_retained_product_authority=bound["products"],
        storage_aliases=identity(alias_path), storage_bound_result=identity(report_path),
        original_cli_failure=identity(ROOT / "test-cache/cleanup/binder-original.log"),
        original_cli_status="failed_on_advanced_live_source_src_cargopacket_cpp",
        adapted_verifier_status="passed_unchanged_original_code_with_explicit_exact_storage_aliases",
        original_verifier=identity(ROOT / "src/tests/multilayer/acceptance/surface_original_evidence.py"),
        binder=identity(binder_path), auditor=identity(Path(__file__)), configurations=configurations,
        ordinary_projection_selections=ordinary_selections, native_processes=68,
        cross_build_gameplay_comparisons=4080, original_ordinary_processes=2,
        closed_policy=dependencies["closed_policy"], storage_validation_seconds=round(time.monotonic()-started, 3))
    output.write_text(json.dumps(audit_result, indent=2) + "\n", encoding="utf-8")
    return audit_result


def main():
    """Run one explicit metadata-only audit into a new disposable report.

    @return None; failures retain diagnostics and never become a valid checkpoint seal.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    result = audit(args.output)
    print("Historical Space facts verified; no native execution:", result["storage_validation_seconds"], "seconds")


if __name__ == "__main__":
    main()
