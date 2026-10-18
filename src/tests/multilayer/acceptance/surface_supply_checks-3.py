"""Run detached order metadata, canonical prices and existing pure native admission.

@file surface_supply_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CCB
All order/station/company records are declared component inputs. No live owner,
purchase, stock, refund or save activation is provided. Children are bounded to110seconds.
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

TAGS = "[surface-supply-state],[surface-supply-admission],[multilayer-bootstrap]"
SOURCES = ("src/surface_supply.h", "src/surface_supply.cpp", "src/surface_supply_order.h", "src/surface_supply_order.cpp",
           "src/multilayer_bootstrap.h", "src/multilayer_bootstrap.cpp", "src/CMakeLists.txt", "src/tests/CMakeLists.txt",
           "src/tests/surface_supply_order.cpp", "src/tests/surface_supply.cpp")


def read(path):
    """Read one explicit proof. @param path Repository-owned JSON. @return Its exact parsed content."""
    return json.loads(path.read_text(encoding="utf-8"))


def execute(configuration):
    """Run one actual focused native test build with its own original process/log evidence.

    @param configuration Independently compiled Debug or RelWithDebInfo executable.
    @return Native component report identity; this does not authorize procurement.
    """
    context = create_run_context("space", "surface-supply-order-model")
    context.prepare_runtime()
    executable = ROOT / "build" / configuration / "openttd_test.exe"
    log = context.logs / "surface-supply-order-model.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(executable), TAGS], cwd=ROOT / "build",
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration),
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    matched = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(errors="replace"))
    assert result.returncode == 0 and matched, log
    target = context.reports / "surface-supply-order-model.json"
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True, configuration=configuration,
                                    tags=TAGS, assertions=int(matched[1]), cases=int(matched[2]),
                                    executable=identity(executable), terminal=identity(log),
                                    sources=[identity(ROOT / name) for name in SOURCES] + [identity(Path(__file__))],
                                    native_processes=[identity(path) for path in sorted(context.reports.glob("native-*.json"))],
                                    component_only=True, runtime_procurement_acceptance=False,
                                    ordinary_loader_acceptance=False), indent=2) + "\n", encoding="utf-8")
    return identity(target)


def bind(members):
    """Bind exactly two native component passes against their original current sources.

    @param members Explicit Debug and RelWithDebInfo membership files.
    @return Both-build detached model handoff with runtime/save/delivery consumers still open.
    """
    configurations = []
    # Each independently compiled native executable must pass its own real
    # order-record/price and original admission cases; a dated binary cannot qualify it.
    for member in members:
        reference = read(member)["report"]
        assert identity(ROOT / reference["path"]) == reference
        proof = read(ROOT / reference["path"])
        assert proof["complete"] and proof["component_only"]
        assert not proof["runtime_procurement_acceptance"] and not proof["ordinary_loader_acceptance"]
        # Original bytes bind the exact tested producer. None of these records
        # supplies stock, purchase history or a replacement process to gameplay.
        for original in [proof["executable"], proof["terminal"], *proof["sources"], *proof["native_processes"]]:
            assert identity(ROOT / original["path"]) == original
        configurations.append(dict(report=reference, **proof))
    assert len(configurations) == 2 and {row["configuration"] for row in configurations} == {"RelWithDebInfo", "Debug"}
    return dict(_documentation=__doc__, complete=True, configurations=configurations,
                native_order_owner_implemented=False, save_activation_implemented=False,
                runtime_procurement_acceptance=False, ordinary_loader_acceptance=False,
                open_consumers=["Plan6 13CCC sole native owner/save/preview", "Plan6 13CCD paid command/lifecycle",
                                "Plan6 13CD driven construction/ordinary-loader acceptance",
                                "Plan11 15B unassisted Surface entry"])


def main():
    """Run one focused build or freeze two explicit members under the repository log owner."""
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
