"""Run native non-paying packet custody and its nearby ordinary freight controls.

@file surface_supply_checks-2.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CB
Declared component stock is not purchased cargo, a driven construction delivery
or ordinary-loader acceptance. Each native test child is bounded to110seconds.
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

TAGS = "[surface-supply-custody],[layer-processor-delivery],[layer-baseline-receiver]"
SOURCES = ("src/source_type.h", "src/economy.cpp", "src/subsidy.cpp", "src/fios.h",
           "src/cargopacket.h", "src/cargopacket.cpp", "src/cargoaction.cpp",
           "src/saveload/saveload.h", "src/saveload/cargo_source_sl.h",
           "src/saveload/cargopacket_sl.cpp", "src/saveload/CMakeLists.txt",
           "src/tests/surface_supply_custody.cpp", "src/tests/CMakeLists.txt",
           "src/tests/processor_fixture.h", "src/tests/mine_fixture.h", "src/tests/excavation_fixture.h",
           "src/tests/layer_processor_delivery.cpp", "src/tests/layer_baseline_receiver.cpp")


def read(path):
    """Read one explicit proof. @param path Repository-owned JSON. @return Its exact parsed content."""
    return json.loads(path.read_text(encoding="utf-8"))


def execute(configuration):
    """Run one actual focused native test build with its own original process/log evidence.

    @param configuration Independently compiled Debug or RelWithDebInfo executable.
    @return Native component report identity; this does not authorize procurement.
    """
    context = create_run_context("space", "surface-supply-custody")
    context.prepare_runtime()
    executable = ROOT / "build" / configuration / "openttd_test.exe"
    log = context.logs / "surface-supply-custody.log"
    with log.open("wb") as sink:
        result = run_native(context, [str(executable), TAGS], cwd=ROOT / "build",
                            env=dict(os.environ, OTTD_TEST_CONFIGURATION=configuration),
                            stdout=sink, stderr=subprocess.STDOUT, timeout=110)
    matched = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases?\)", log.read_text(errors="replace"))
    assert result.returncode == 0 and matched, log
    target = context.reports / "surface-supply-custody.json"
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
    @return Both-build purpose handoff with runtime/save/delivery consumers still open.
    """
    configurations = []
    # Each independently compiled native executable must pass its own real
    # custody/payment and ordinary control cases; a dated binary cannot qualify it.
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
                saved_purpose_era=439, company_supply_source_type=4,
                runtime_procurement_acceptance=False, ordinary_loader_acceptance=False,
                open_consumers=["Plan6 13CC paid order/quota/persistence/lifecycle",
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
