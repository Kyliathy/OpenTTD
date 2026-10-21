"""Run the existing pure rocket/manifest native producer in isolated proof files.

@file cargo_rocket_native_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62E.1.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md saved native flight contracts.
--native executes seven existing pure cases in both unchanged binaries. No paid
world is loaded and no simulation tick is earned. Accepted launch receipts stay
untouched; new terminals live in logs/cargo-rocket-state-native-*.log.
"""

import argparse
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "media/multilayer"))
from tools import cargo_rocket_art as art
from tools import cargo_rocket_directions as directions

OUTPUT = ROOT / "logs/proofs/mlm/native-art/space/cargo-rocket-state-v1"
ARGUMENTS = ["[space-logistics-contract]", "--reporter", "console", "--durations", "yes"]
CONFIGURATIONS = ("Debug", "RelWithDebInfo")
FOUNDATION = ROOT / "logs/proofs/mlm/native-art/space/launch-center-state-v1"
READY = ROOT / "media/multilayer/source/space/cargo-rocket-ready-qualification-v1.json"


def native_sources():
    """Bind the unchanged current native receipt/flight/custody producer inputs.

    @return Exact accepted native sources plus the immutable rocket brief inputs.
    @throws ValueError On stale actual owner bytes or unqualified ready inputs.
    """
    model = art.read_json(art.MODEL)
    art.checked_model(model)
    foundation = art.read_json(FOUNDATION / "native-Debug.json")
    ready = art.read_json(READY)
    # Completed ready source is a prerequisite, not a replacement for current
    # native phase/custody behavior. The following contract adds no runtime art.
    if not ready["ready_views_qualified"] or not ready["ready_fit_qualified"]:
        raise ValueError("rocket ready source qualification absent")
    references = {row["path"]: row for row in [*foundation["sources"], *model["native_inputs"]]}
    # Inputs must remain the accepted producer's exact bytes before the same
    # binaries run again; a fresh-looking log cannot qualify uncompiled changes.
    for reference in references.values():
        if art.source.identity(ROOT / reference["path"]) != reference:
            raise ValueError("native rocket source owner changed")
    return list(references.values())


def checked_native(proof):
    """Require the real narrow producer result and exact current input/log identities.

    @param proof Explicit new rocket-native receipt, never a paid-world fixture.
    @return None.
    @throws ValueError On changed scope, old output, source or binary identity.
    """
    configuration = proof["configuration"]
    # This source gate is the existing pure seven-case producer only. Hidden
    # game fixtures or synthetic metadata cannot substitute for its actual run.
    if (configuration not in CONFIGURATIONS or not proof["complete"]
            or proof["arguments"] != ARGUMENTS or proof["assertions"] != 183 or proof["cases"] != 7
            or proof["paid_world_loaded"] or proof["simulation_ticks"] != 0
            or proof["application"]["path"] != f"build/{configuration}/openttd_test.exe"
            or proof["terminal"]["path"] != f"logs/cargo-rocket-state-native-{configuration}.log"
            or proof["sources"] != native_sources() or proof["ready_qualification"] != art.source.identity(READY)):
        raise ValueError("native rocket producer scope changed")
    foundation = art.read_json(FOUNDATION / f"native-{configuration}.json")
    if proof["application"] != foundation["application"]:
        raise ValueError("native rocket executable differs from accepted producer")
    for reference in [proof["application"], proof["terminal"], *proof["sources"]]:
        if art.source.identity(ROOT / reference["path"]) != reference:
            raise ValueError("native rocket producer input/output changed")
    terminal = (ROOT / proof["terminal"]["path"]).read_text(encoding="utf-8")
    # Actual native validation exercises paid model14/phase/burn receipts and
    # cargo custody; a summary alone cannot hide a different filtered case set.
    if (not re.search(r"All tests passed \(183 assertions in 7 test cases\)", terminal)
            or "Native rocket phases require one round trip and consistent burn receipts" not in terminal):
        raise ValueError("native rocket producer lacks actual focused success output")


def rebuild_native():
    """Execute both existing binaries into new rocket-owned terminals and receipts.

    @return None; accepted launch/native/source outputs are never overwritten.
    """
    sources = native_sources()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    # Each configuration earns its own actual result and immutable identity;
    # these pure cases neither load original paid worlds nor tick their owners.
    for configuration in CONFIGURATIONS:
        application = ROOT / f"build/{configuration}/openttd_test.exe"
        expected = art.read_json(FOUNDATION / f"native-{configuration}.json")
        if art.source.identity(application) != expected["application"]:
            raise ValueError("native rocket executable changed before focused run")
        terminal = ROOT / f"logs/cargo-rocket-state-native-{configuration}.log"
        print("Running pure native rocket/manifest producer: " + configuration, flush=True)
        with terminal.open("w", encoding="utf-8") as output:
            subprocess.run([str(application), *ARGUMENTS], cwd=ROOT, stdout=output,
                           stderr=subprocess.STDOUT, check=True, timeout=55)
        proof = dict(_documentation=__doc__, complete=True, configuration=configuration,
                     application=art.source.identity(application), arguments=ARGUMENTS,
                     terminal=art.source.identity(terminal), assertions=183, cases=7,
                     sources=sources, ready_qualification=art.source.identity(READY),
                     paid_world_loaded=False, simulation_ticks=0)
        checked_native(proof)
        (OUTPUT / f"native-{configuration}.json").write_text(json.dumps(proof, indent=2)+"\n", encoding="utf-8")


def main():
    """Run or verify only the native/source prerequisites for the active rocket group.

    @return Zero after both actual native receipts and protected ready inputs pass.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", action="store_true")
    args = parser.parse_args()
    if args.native:
        rebuild_native()
    for configuration in CONFIGURATIONS:
        checked_native(art.read_json(OUTPUT / f"native-{configuration}.json"))
    art.checked_predecessor(art.source.read_record(), art.read_json(art.PUBLIC))
    # Exact immutable heading fixtures also protect every accepted body during
    # phase preparation, including the original NE source and all genuine yaws.
    for direction in art.DIRECTIONS:
        directions.direction_view(direction)
    print("Both native producers passed183assertions/7cases; eight ready bodies and completed-source/payload inputs preserved.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
