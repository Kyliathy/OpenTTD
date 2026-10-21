"""Run the bounded selected-ground source, packing and generated-type qualification.

@file run_ground_pack_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79D.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Rebuild receipts under logs and dry conversions/header objects under test-cache.
Successful staging writes only the three new append inputs and their own source report.
The current GRF/public manifest/application files remain byte-exact.
"""

import argparse
import json
from pathlib import Path
import subprocess
import sys
import time

from ground_pack_check_support import ROOT, ground, handoff


def execute(command, log):
    """Capture an actual subprocess result without PowerShell stderr reinterpretation.

    @param command Explicit argument vector, never interpolated shell code.
    @param log Repository logs path for both normal and error output.
    @return Actual command, exit status, elapsed time and exact log identity.
    @throws ValueError If a required qualification command fails.
    """
    started = time.perf_counter()
    with log.open("wb") as stream:
        process = subprocess.run(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT, check=False)
    result = {"command": [str(arg) for arg in command], "exit_code": process.returncode,
              "seconds": round(time.perf_counter() - started, 3),
              "log": {"path": log.relative_to(ROOT).as_posix(), "sha256": handoff.digest(log)}}
    if process.returncode != 0:
        raise ValueError("ground qualification failed; inspect " + str(log))
    return result


def stage(report, files):
    """Stage only new append-owned inputs after all deterministic and compile checks pass.

    @param report Fresh source-derived report, including exact output byte identities.
    @param files Fresh exact PNG/NFO/header output mapping from the qualified converter.
    @return None; no current public packed binding or GRF metadata is published.
    """
    scope, before, _public = handoff.read_scope()
    handoff.validate_preserved_files(scope, before)
    expected = {handoff.bound(scope[key]) for key in ("png", "nfo", "header")}
    if set(files) != expected:
        raise ValueError("ground staging attempts a path outside its new append inputs")
    # Only these new own files can be staged. A prior owner at any destination
    # must not be silently overwritten, even after conversion tests succeed.
    for path, data in files.items():
        if path.exists() and path.read_bytes() != data:
            raise ValueError("ground staging destination already contains different bytes: " + str(path))
    for path, data in files.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
    target = handoff.bound(scope["report"])
    encoded = json.dumps(report, indent=2) + "\n"
    if target.exists() and target.read_text(encoding="utf-8") != encoded:
        raise ValueError("ground own report destination already contains different metadata")
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(encoded, encoding="utf-8")
    handoff.validate_preserved_files(scope, before)


def main():
    """Run two fresh conversions, both bounded suites and the isolated native type fixture.

    @return None; retain a truthful aggregate receipt and optionally stage the new inputs.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", required=True, help="Repository logs run directory")
    parser.add_argument("--cmake", required=True, help="Actual installed CMake executable")
    parser.add_argument("--stage-inputs", action="store_true", help="Stage qualified own PNG/NFO/header/report only")
    args = parser.parse_args()
    output = handoff.bound(args.run)
    if not output.is_relative_to(ROOT / "logs"):
        raise ValueError("ground qualification diagnostics belong in repository logs")
    output.mkdir(parents=True, exist_ok=True)
    scope, before, _public = handoff.read_scope()
    commands = []
    # Each converter runs in a separate Python process with fresh document and
    # image caches. Neither receives bytes or cached poses from the other run.
    for suffix in ("a", "b"):
        commands.append(execute([sys.executable, "media/multilayer/pack_ground_art.py", "--derive",
                                 "test-cache/ground-pack/conversion-" + suffix], output / ("conversion-" + suffix + ".log")))
    suites = [("source", "ground_source_adapter_checks-1.py"), ("packing", "ground_packing_checks-2.py")]
    for name, script in suites:
        commands.append(execute([sys.executable, "src/tests/multilayer/" + script, "--report",
                                 (output / (name + "-checks.json")).relative_to(ROOT).as_posix()], output / (name + "-checks.log")))
    fixture = "src/tests/multilayer/ground_header_fixture"
    build = "test-cache/ground-pack/header-fixture-build"
    commands.append(execute([args.cmake, "-S", fixture, "-B", build, "-G", "Visual Studio 17 2022", "-A", "x64"],
                            output / "header-configure.log"))
    commands.append(execute([args.cmake, "--build", build, "--config", "Debug", "--target", "ground_header_contract", "--", "/nodeReuse:false"],
                            output / "header-build.log"))
    report, files = ground.derive()
    # The fresh converter used for staging must still match both separately
    # checked processes; a changed input during qualification invalidates staging.
    for path, data in files.items():
        for suffix in ("a", "b"):
            if (ROOT / "test-cache/ground-pack" / ("conversion-" + suffix) / path.name).read_bytes() != data:
                raise ValueError("ground sources or conversion changed during qualification")
    encoded = json.dumps(report, indent=2) + "\n"
    for suffix in ("a", "b"):
        if (ROOT / "test-cache/ground-pack" / ("conversion-" + suffix) / "packed-ground-manifest.json").read_text(encoding="utf-8") != encoded:
            raise ValueError("ground conversion report changed during qualification")
    handoff.validate_preserved_files(scope, before)
    if args.stage_inputs:
        stage(report, files)
    inputs = [Path(__file__), ROOT / "src/tests/multilayer/ground_pack_check_support.py",
              ROOT / fixture / "CMakeLists.txt", ROOT / fixture / "ground_header_contract.cpp",
              ROOT / "media/multilayer/pack_ground_art.py", ROOT / "media/multilayer/tools/ground_art_handoff.py", handoff.SCOPE]
    receipts = [output / (name + "-checks.json") for name, _script in suites]
    qualification = {"_documentation": __doc__, "schema_version": 1, "passed": True, "commands": commands,
                     "focused_cases": sum(json.loads(path.read_text(encoding="utf-8"))["cases"] for path in receipts),
                     "inputs": [{"path": path.relative_to(ROOT).as_posix(), "sha256": handoff.digest(path)} for path in inputs + receipts],
                     "outputs": report["outputs"], "pose_count": report["pose_count"], "preserved_file_count": len(before["files"]),
                     "staged_inputs": args.stage_inputs, "native_loading_claimed": False, "runtime_activated": False}
    target = handoff.bound(scope["qualification"])
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(qualification, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"passed": True, "focused_cases": qualification["focused_cases"], "pose_count": report["pose_count"],
                      "staged_inputs": args.stage_inputs, "qualification": target.relative_to(ROOT).as_posix()}))


if __name__ == "__main__":
    main()
