"""Run isolated actual geothermal/tube producer and source-integration boundaries.

@file geothermal_tube_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Groups6BB-6BD.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Each focus owns one native process and actual nonzero Catch counts; a source
declaration or fabricated utility balance cannot establish an integration pass.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
from runner import current_run_context, run_native

FOCUSES = {
    "controls-source": "[tube-geothermal-controls-source]",
    "controls-switch": "[tube-geothermal-controls-switch]",
    "controls-guidance": "[tube-geothermal-controls-guidance]",
    "controls-stale": "[tube-geothermal-controls-stale]",
    "external-controls": "[transport-tube-controls]",
    "external-controls-construction": "[transport-tube-controls-construction]",
    "external-controls-fleet": "[transport-tube-controls-fleet]",
    "external-controls-stale": "[transport-tube-controls-stale]",
    "facility-readiness": "Tube public facility view retains native identity and exact capsule cargo admission",
    "terminal-readiness": "Tube public loading terminal uses actual station platform and cargo owner",
    "external-readiness": "Tube public power readiness changes only with actual service finance or layout",
    "command-source": "[tube-geothermal-command-source]",
    "command-switch": "[tube-geothermal-command-switch]",
    "command-refusal": "[tube-geothermal-command-refusal]",
    "command-wire": "[tube-geothermal-command-wire]",
    "external-command-service": "[transport-tube-commands]",
    "external-command-refusal": "[transport-tube-command-refusal]",
    "external-command-binding": "[transport-tube-command-binding]",
    "external-command-owner": "[transport-tube-command-owner]",
    "external-command-wire": "[transport-tube-command-serialization]",
    "switch": "[tube-geothermal-switch]",
    "switch-inlet": "[tube-geothermal-switch-inlet]",
    "switch-refusal": "[tube-geothermal-switch-refusal]",
    "work-motion": "[tube-geothermal-work-motion]",
    "work-aggregate": "[tube-geothermal-work-aggregate]",
    "work-loss": "[tube-geothermal-work-loss]",
    "work-late-loss": "[tube-geothermal-work-late-loss]",
    "binding": "[tube-geothermal-binding]",
    "binding-refusal": "[tube-geothermal-binding-refusal]",
    "binding-topology": "[tube-geothermal-binding-topology]",
    "binding-retirement": "[tube-geothermal-binding-retirement]",
    "binding-wait": "[tube-geothermal-binding-wait]",
    "recipient-company": "[utility-local-feed-recipient-company]",
    "recipient": "[utility-local-feed-recipient]",
    "recipient-refusal": "[utility-local-feed-recipient-refusal]",
    "recipient-transfer": "[utility-local-feed-recipient-transfer]",
    "source-state": "[tube-geothermal-source-state]",
    "source-owner": "[tube-geothermal-source-owner]",
    "source-supply": "[tube-geothermal-source-supply]",
    "source-refusal": "[tube-geothermal-source-refusal]",
    "source-lifecycle": "[tube-geothermal-source-lifecycle]",
    "utility-company": "[utility-power-company]",
    "generator-company": "[geothermal-generator-operating-company]",
    "heat-company": "[utility-heat-company]",
    "path-lifecycle": "[utility-power-path-lifecycle]",
    "local-feed": "[utility-power-local-feed]",
    "local-feed-refusal": "[utility-power-local-feed-refusal]",
    "local-feed-loss": "[utility-power-local-feed-loss]",
    "local-feed-wire": "[utility-power-local-feed-wire]",
    "paid-path": "[utility-power-path-transfer]",
    "shared-lineage": "[utility-power-lineage]",
    "generator-service": "[geothermal-generator-service]",
    "magma-service": "[geothermal-magma-service]",
    "paid-riser": "[geothermal-riser-link]",
    "actual-export": "[geothermal-service-connection]",
    "utility-mutation": "[utility-power-mutation]",
    "heat-conversion": "[utility-conversion-atomic]",
    "tube-meter": "[transport-tube-meter]",
    "tube-recovery": "[transport-tube-power-recovery]",
    "external-power": "[transport-tube-power]",
}


def identity(path):
    """Identify one current producer, binary or retained result.

    @param path Existing repository-owned artifact.
    @return Relative path, byte length and SHA256.
    """
    path = Path(path).resolve()
    return {"path": path.relative_to(ROOT).as_posix(), "bytes": path.stat().st_size,
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def main():
    """Run one focused native producer or integration boundary in isolation.

    @return None; timeout, empty selection or failed native prerequisites abort acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--focus", choices=tuple(FOCUSES), required=True)
    args = parser.parse_args()
    context = current_run_context("tubes", "geothermal-source")
    context.prepare_runtime()
    binary = ROOT / f"build/{args.config}/openttd_test.exe"
    result = run_native(context, [binary, FOCUSES[args.focus]], cwd=ROOT / "build",
                        env=dict(os.environ, OTTD_TEST_CONFIGURATION=args.config),
                        capture_output=True, timeout=110, check=False)
    output = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    log = context.logs / f"{args.config}-{args.focus}.log"
    log.write_text(output + f"\nNATIVE_EXIT_CODE={result.returncode}\n", encoding="utf-8")
    assert result.returncode == 0 and "All tests passed" in output, log
    counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", output)
    assert counts and int(counts[1]) > 0 and int(counts[2]) > 0, log
    report = {"_documentation": "r2mlvs17-underground-transport-tubes.md Groups6BB-6BD; actual native producer/integration boundary; prepared fixtures are not paid-world bootstrap",
              "configuration": args.config, "focus": args.focus, "tag": FOCUSES[args.focus],
              "case_runs": int(counts[2]), "assertions": int(counts[1]), "binary": identity(binary),
              "log": identity(log), "controller": identity(__file__),
              "sources": [identity(ROOT / name) for name in (
                  "src/transport_tube.h", "src/transport_tube_runtime.cpp", "src/transport_tube_topology.cpp",
                  "src/transport_tube_power.cpp", "src/transport_tube_cmd.cpp", "src/transport_tube_gui.cpp",
                  "src/utility_power.h", "src/utility_power.cpp", "src/multilayer_utility.h", "src/multilayer_utility.cpp",
                  "src/utility_power_transfer.h", "src/utility_power_lineage.cpp", "src/saveload/utility_power_sl.cpp",
                  "src/transport_tube_geothermal.h", "src/transport_tube_geothermal.cpp",
                  "src/tests/geothermal_tube_power_fixture.h", "src/tests/transport_tube_geothermal.cpp",
                  "src/tests/utility_power_local_feed_registration.cpp",
                  "src/tests/geothermal_tube_capsule_fixture.h", "src/tests/transport_tube_geothermal_binding.cpp",
                  "src/tests/transport_tube_geothermal_binding_load.cpp",
                  "src/tests/geothermal_tube_work_fixture.h", "src/tests/transport_tube_geothermal_work.cpp",
                  "src/tests/transport_tube_geothermal_work_loss.cpp",
                  "src/tests/transport_tube_geothermal_switch.cpp",
                  "src/tests/geothermal_tube_switch_fixture.h", "src/tests/transport_tube_geothermal_commands.cpp", "src/transport_tube_cmd.h", "src/command_type.h",
                  "src/tests/transport_tube_geothermal_controls.cpp", "src/tests/transport_tube_controls.cpp",
                  "src/tests/transport_tube_readiness.cpp",
                  "src/transport_tube_gui.h", "src/transport_tube_readiness.cpp", "src/underground_gui.cpp",
                  "src/widgets/underground_widget.h", "src/lang/english.txt",
                  "src/saveload/saveload.h", "src/tests/utility_power_local_feed.cpp",
                  "src/saveload/transport_tube_vehicle_sl.cpp", "src/saveload/vehicle_sl.cpp", "src/saveload/afterload.cpp", "src/multilayer_state.cpp",
                  "src/geothermal_generator.cpp", "src/geothermal_generator.h", "src/geothermal_electrical.cpp",
                  "src/geothermal_electrical.h", "src/geothermal_riser.cpp", "src/geothermal_utility.cpp",
                  "src/tests/transport_tube_power_fixture.h", "src/tests/transport_tube_meter_fixture.h",
                  "src/tests/transport_tube_meter.cpp", "src/tests/transport_tube_recovery.cpp",
                  "src/tests/geothermal_generator_operating_fixture.h", "src/train_cmd.cpp", "src/vehicle.cpp")]}
    path = context.output("reports", f"geothermal-tube-source-{args.config}-{args.focus}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"{args.config}-{args.focus}: {counts[1]} assertions / {counts[2]} cases\n{path}", flush=True)


if __name__ == "__main__":
    main()
