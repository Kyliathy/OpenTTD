"""Run only the pure Surface seed admission and its existing bootstrap contract.

@file surface_supply_checks-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13C.1
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15B
Declared station/component inputs supply no runtime purchase, quota, paid cargo,
delivery, save or continuous-chain acceptance. Each child is bounded to110seconds.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess

from space_preview_checks import ROOT, identity
from artifact_paths import create_run_context
from runner import run_native

TAGS = "[surface-supply-admission],[multilayer-bootstrap]"
SOURCES = ("src/surface_supply.h", "src/surface_supply.cpp", "src/tests/surface_supply.cpp",
           "src/multilayer_bootstrap.h", "src/multilayer_bootstrap.cpp", "src/CMakeLists.txt", "src/tests/CMakeLists.txt")


def read(path):
    """Read one explicit repository-owned proof.

    @param path Exact native membership/report path. @return Original parsed JSON.
    """
    return json.loads(path.read_text(encoding="utf-8"))


def execute(configuration):
    """Run the complete focused admission selection against one current test build.

    @param configuration Independently compiled native Debug or RelWithDebInfo executable.
    @return Exact current bounded component report identity, never a procurement claim.
    """
    context = create_run_context("space", "surface-supply-admission")
    context.prepare_runtime()
    executable = ROOT / "build" / configuration / "openttd_test.exe"
    log = context.logs / "surface-supply-admission.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(executable), TAGS], cwd=ROOT / "build",
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration),
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    matched = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(errors="replace"))
    assert result.returncode == 0 and matched, log
    target = context.reports / "surface-supply-admission.json"
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration,
                                    assertions=int(matched[1]), cases=int(matched[2]), tags=TAGS,
                                    executable=identity(executable), terminal=identity(log),
                                    sources=[identity(ROOT / name) for name in SOURCES] + [identity(Path(__file__))],
                                    native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                                    component_only=True, runtime_procurement_acceptance=False), indent=2) + "\n", encoding="utf-8")
    return identity(target)


def bind(members):
    """Bind both actual current component passes while keeping runtime consumers open.

    @param members Exactly two explicit current configuration memberships.
    @return Current pure-admission handoff; it grants no order/cargo/quota state or runtime entry.
    """
    configurations = []
    # Each native build must pass its own actual station and numerical policy
    # tests. No dated binary or another configuration supplies the new producer.
    for member in members:
        reference = read(member)["report"]
        assert identity(ROOT / reference["path"]) == reference
        proof = read(ROOT / reference["path"])
        assert proof["complete"] and proof["component_only"] and not proof["runtime_procurement_acceptance"]
        for original in [proof["executable"], proof["terminal"], *proof["sources"], *proof["native_processes"]]:
            assert identity(ROOT / original["path"]) == original
        configurations.append(dict(report=reference, **proof))
    assert len(configurations) == 2 and {row["configuration"] for row in configurations} == {"RelWithDebInfo", "Debug"}
    return dict(_documentation=__doc__, complete=True, configurations=configurations,
                public_exports=["PrepareSurfaceSupplyOrder", "CaptureSurfaceSupplyOrderView"],
                supported_seed=["STEL", "RFMT", "ELEC", "BATY"], runtime_procurement_acceptance=False,
                open_consumers=["Plan6 13C.2 settlement/non-paying custody", "Plan6 13C.3 persistence/lifecycle", "Plan6 13C.4 native delivery", "Plan11 15B genuine entry"])


def main():
    """Run one focused build or freeze exactly two supplied members under the repository log owner."""
    parser = argparse.ArgumentParser(description=__doc__)
    scope = parser.add_mutually_exclusive_group(required=True)
    scope.add_argument("--configuration", choices=("RelWithDebInfo", "Debug"))
    scope.add_argument("--bind", nargs=2, type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    assert output.is_relative_to(ROOT / "logs") and not output.exists()
    result = bind([ROOT / path for path in args.bind]) if args.bind else dict(_documentation=__doc__, report=execute(args.configuration))
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(identity(output), flush=True)


if __name__ == "__main__":
    main()
