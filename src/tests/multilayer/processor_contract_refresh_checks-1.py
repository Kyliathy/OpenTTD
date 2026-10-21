"""Run current native processor boundaries without rewriting historical geometry.

@file processor_contract_refresh_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group41B.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Command-body funding quotes/geometry and injected states are source-contract evidence,
not normal-game construction, transported bootstrap or runtime art acceptance.
"""
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
sys.path.insert(0, str(Path(__file__).with_name("support")))
from tools.ground_station_contract import digest, source_digest
from tools import ground_processor_contract as contract
from tools import ground_processor_qualification as qualification
from artifact_paths import RunContext, create_run_context, identifier
from runner import run_native

LOGS = ROOT / "logs/graphics/processor-source/20261021-requalification-01"
TAGS = "[layer-processor-definition],[layer-processor-presentation],[layer-processor]"


def identity(path):
    """Identify an existing repository authority without modifying it.

    @param path Actual source, application, original descriptor or native proof file.
    @return Repository-relative path and exact byte SHA256.
    """
    path = Path(path).resolve()
    assert path.is_relative_to(ROOT) and path.is_file()
    return dict(path=path.relative_to(ROOT).as_posix(), sha256=digest(path))


def preserve_originals():
    """Verify every frozen completed asset/application and qualify only the intentional helper edit.

    @return Exact baseline reference and original/current helper transition.
    """
    before_path = LOGS / "before.json"
    before = json.loads(before_path.read_text(encoding="utf-8"))
    transition = None
    # Completed sources, all public/packed metadata, payloads and existing
    # applications stay exact. Only the archived helper gains epoch admission.
    for row in before["files"]:
        if row["path"] == "media/multilayer/tools/ground_processor_contract.py":
            archived = ROOT / "test-cache/processor-source/20261021-requalification-before-edit" / row["path"]
            assert digest(archived) == row["sha256"]
            transition = dict(original=row, retained=identity(archived), current=identity(ROOT / row["path"]))
        else:
            assert identity(ROOT / row["path"]) == row, row["path"]
    assert transition is not None
    return identity(before_path), transition


def execute(configuration, recover_run=None):
    """Run or recover one exact current seven-case native processor qualification.

    @param configuration Isolated compiled application configuration.
    @param recover_run Exact successful invocation to inspect without repeating it.
    @return Current native-owner qualification preserving the original descriptor.
    """
    original = json.loads(contract.DESCRIPTOR.read_text(encoding="utf-8"))
    native_sources = {name: source_digest(ROOT / name) for name in contract.OWNERS}
    build_sources = {name: source_digest(ROOT / name) for name in qualification.BUILD_OWNERS}
    build_log = LOGS / f"build-{configuration}.log"
    # Windows PowerShell redirects native build output as UTF16; retain its
    # original bytes and decode that container before checking actual linkage.
    build_bytes = build_log.read_bytes()
    build_text = build_bytes.decode("utf-16" if build_bytes.startswith(b"\xff\xfe") else "utf-8-sig", errors="replace")
    assert "openttd_processor_contract_test.vcxproj ->" in build_text
    # A source edited during or after the rebuild cannot silently qualify an
    # older executable. This group changes no native implementation source.
    for name in set(contract.OWNERS) | set(qualification.BUILD_OWNERS):
        assert (ROOT / name).stat().st_mtime_ns <= build_log.stat().st_ctime_ns, name
    before, transition = preserve_originals()
    application = ROOT / f"build/{configuration}/openttd_processor_contract_test.exe"
    attempt = LOGS / f"attempt-{configuration}.json"
    if recover_run is None:
        assert not attempt.exists(), "recover the exact original native invocation"
        context = create_run_context("graphics", "processor-contract-current")
        attempt.write_text(json.dumps(dict(run_id=context.run_id, application=identity(application),
            historical_descriptor=identity(contract.DESCRIPTOR), native_source_sha256=native_sources,
            build_source_sha256=build_sources), indent=2)+"\n", encoding="utf-8")
    else:
        recorded = json.loads(attempt.read_text(encoding="utf-8"))
        assert recorded["run_id"] == recover_run and recorded["application"] == identity(application)
        assert recorded["native_source_sha256"] == native_sources and recorded["build_source_sha256"] == build_sources
        context = RunContext(ROOT, "graphics", "processor-contract-current", identifier(recover_run))
    log = context.logs / "native.log"
    if recover_run is None:
        with log.open("w", encoding="utf-8") as stream:
            result = run_native(context, [str(application), TAGS], cwd=ROOT / "build", stdout=stream,
                                stderr=subprocess.STDOUT, timeout=110)
            stream.write(f"\nNATIVE_EXIT_CODE={result.returncode}\n")
        assert result.returncode == 0, log
    output = log.read_text(encoding="utf-8-sig")
    summary = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases\)", output)
    assert summary is not None and int(summary[2]) == 7 and output.count("All tests passed") == 1
    assert output.rstrip().endswith("NATIVE_EXIT_CODE=0") and output.count("NATIVE_EXIT_CODE=") == 1
    traces = [json.loads(line.removeprefix("PROCESSOR_ART_GEOMETRY ")) for line in output.splitlines()
              if line.startswith("PROCESSOR_ART_GEOMETRY ")]
    assert traces == original["families"]
    report_path, = context.reports.glob("native-*.json")
    report = json.loads(report_path.read_text(encoding="utf-8"))
    assert report["exit_status"] == 0 and report["executable"] == identity(application)
    retained = LOGS / f"bin/{configuration}/openttd_processor_contract_test.exe"
    retained.parent.mkdir(parents=True, exist_ok=True)
    # Recorder recovery may reach an already retained successful application;
    # inspect those exact bytes rather than replacing them or rerunning tests.
    if retained.exists():
        assert digest(retained) == digest(application)
    else:
        shutil.copy2(application, retained)
    assert digest(retained) == digest(application)
    assert native_sources == {name: source_digest(ROOT / name) for name in contract.OWNERS}
    assert build_sources == {name: source_digest(ROOT / name) for name in qualification.BUILD_OWNERS}
    assert preserve_originals() == (before, transition)
    value = dict(_documentation=dict(owner="docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group41B",
        architecture="docs/zz-reach2/architecture/map-layers/archi-map-layers.md",
        scope="Current native command-body funding quotes/geometry and injected status boundaries; no paid procurement, runtime art or transported bootstrap claim"),
        schema_version=1, complete=True, historical_descriptor=identity(contract.DESCRIPTOR),
        native_source_sha256=native_sources, build_source_sha256=build_sources, families=traces,
        native_proof=dict(binary=identity(retained), executed_binary=identity(application), process_executable=report["executable"],
            log=identity(log), process=identity(report_path), exit_code=0, assertions=int(summary[1]), test_cases=7),
        preservation=dict(before=before, artifacts=1196, helper_transition=transition), build_log=identity(build_log), caller=identity(Path(__file__)))
    qualification.validate_qualification(original, contract.OWNERS, value)
    return value


def main():
    """Publish one separately qualified current epoch after actual native success and preservation."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "RelWithDebInfo"), default="Debug")
    parser.add_argument("--recover-run")
    args = parser.parse_args()
    assert not qualification.QUALIFICATION.exists()
    value = execute(args.configuration, args.recover_run)
    qualification.QUALIFICATION.write_text(json.dumps(value, indent=2)+"\n", encoding="utf-8")
    print(qualification.QUALIFICATION.relative_to(ROOT).as_posix(), value["native_proof"]["assertions"], "assertions / 7 cases", flush=True)


if __name__ == "__main__":
    main()
