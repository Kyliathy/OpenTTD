"""Continue an exact original paid TBM/tube world through native useful freight.

@file tube_tbm_useful.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group16C.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The ordinary native loader retains the original earned floor and completed spoil leg.
"""

import argparse
import json
import os
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
from construction_provider_return import ROOT, identity
from tube_tbm_return import SOURCES as CLEARANCE_SOURCES
from runner import current_run_context, run_native

SOURCES = (*CLEARANCE_SOURCES, "src/tests/tube_tbm_useful_fixture.h", "src/tests/tube_useful_tick_fixture.h",
           "src/layer_industry_mine.cpp", "src/transport_tube_cmd.cpp", "src/transport_tube_power.cpp",
           "src/transport_tube_runtime.cpp", "src/transport_tube_engine.cpp", "src/order_cmd.cpp",
           "src/engine.cpp", "src/saveload/engine_sl.cpp", "src/landscape.cpp")
OWNER_KEYS = ("site", "portal", "cutter", "hauler", "earth_receiver", "rock_receiver", "job", "depot0", "depot1", "cash")


def native_loaded(context, configuration, tag, environment, label="useful-prepare"):
    """Run one bounded real application-load observer without unrelated runtime imports.

    @param context Explicit original-world diagnostic/runtime owner.
    @param configuration Exact current native Debug or optimized test binary.
    @param tag One explicit single compiled native file/continuation case.
    @param environment Immutable original input and independently retained native identities.
    @param label Explicit isolated phase diagnostic basename.
    @return Native stdout and exact passing log/case/assertion identity.
    """
    context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md16C;original paid-world native continuation.\ntube_tbm_file_check\n", encoding="utf-8")
    result = run_native(context, [ROOT / f"build/{configuration}/openttd_test.exe", tag], cwd=ROOT / "build",
                        env=dict(os.environ, **environment), capture_output=True, timeout=110, check=False)
    raw = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    log = context.logs / f"{label}.log"
    log.write_text(raw + f"\nNATIVE_EXIT_CODE={result.returncode}\n", encoding="utf-8")
    assert result.returncode == 0 and "All tests passed" in raw, log
    counts = re.search(r"All tests passed \((\d+) assertions? in (\d+) test cases?\)", raw)
    assert counts is not None
    assertions, cases = map(int, counts.groups())
    assert cases == 1 and assertions > 0
    print(f"native useful preparation: {assertions} assertions / {cases} case", flush=True)
    return raw, {"log": identity(log), "assertions": assertions, "cases": cases}


def prepare(configuration, clearance_report, clearance_receipt, clearance_sha256):
    """Earn first native mine cargo in a capsule on the accepted original paid floor.

    @param configuration Current reader,opposite the exact original construction writer.
    @param clearance_report Explicit original report already sealed by Group16B.
    @param clearance_receipt Exact Group16BB catalogue-complete paid construction handoff.
    @param clearance_sha256 Independently retained SHA256 of that explicit immutable receipt.
    @return New native loaded-ready report,with unchanged immutable construction input.
    """
    receipt_path = ROOT / clearance_receipt
    receipt_identity = identity(receipt_path)
    assert receipt_identity["sha256"] == clearance_sha256
    receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    assert set(receipt["catalogue_probes"]) == {"Debug", "RelWithDebInfo"}
    selected = identity(clearance_report)
    record = next(row for row in receipt["records"].values() if row["report"] == selected)
    assert record["configuration"] != configuration
    original = record["original_ready"]
    source = ROOT / original["path"]
    assert identity(source) == original
    context = current_run_context("tubes", "tbm-final-use")
    tag = "[.tube-tbm-useful-prepare]"
    environment = {"OTTD_TUBE_TBM_INPUT": str(source),
                   "OTTD_TUBE_TBM_OWNERS": ",".join(str(record["owners"][key]) for key in OWNER_KEYS)}
    raw, proof = native_loaded(context, configuration, tag, environment)
    match = re.search(r"TUBE_TBM_USEFUL_PREPARED ticks=(\d+) capsule=(\d+) freight=(\d+) wagon=(\d+) mine=(\d+) factory=(\d+) source=(\d+) loaded=(\d+) extracted=(\d+) remaining=(\d+) cash=(\d+) opening=(\d+) excavation_loss=(\d+)", raw)
    assert match is not None
    observations = dict(zip(("ticks", "capsule", "freight", "wagon", "mine", "factory", "source", "loaded", "extracted", "remaining", "cash", "opening", "excavation_loss"), map(int, match.groups())))
    assert observations["loaded"] > 0
    assert observations["extracted"] >= observations["loaded"]
    assert observations["opening"] == observations["remaining"] + observations["extracted"]
    assert 0 < observations["cash"] < record["owners"]["cash"]
    saved = re.search(r"TUBE_TBM_USEFUL_SAVE stage=loaded-ready path=([^\r\n]+)", raw)
    assert saved is not None
    saved_path = Path(saved[1]).resolve()
    assert saved_path == context.output("saves", "tube-tbm-loaded-ready.sav").resolve()
    assert identity(source) == original
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md16C.1;ordinary opposite-reader original paid TBM floor,actual paid mine/machinery/shaft/utility/fleet and native finite production/loading;no recreated floor,prepared stock or private arrivals",
        "configuration": configuration, "focus": "prepare", "tag": tag, "native_version": 434,
        "native": proof, "observations": observations, "original_input": original,
        "original_owners": record["owners"], "original_paid_observations": record["metrics"],
        "clearance_receipt": receipt_identity, "clearance_report": selected,
        "output": identity(saved_path), "controller": identity(__file__),
        "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES],
    }
    path = context.output("reports", f"tube-tbm-{configuration}-useful-prepare.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return path


def main():
    """Select the exact accepted original construction report and current opposite reader.

    @return None;changed original inputs,native funding/production/loading failure cannot pass.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--clearance-report", type=Path, required=True)
    parser.add_argument("--clearance-receipt", type=Path, required=True)
    parser.add_argument("--clearance-sha256", required=True)
    args = parser.parse_args()
    print(prepare(args.config, args.clearance_report, args.clearance_receipt, args.clearance_sha256), flush=True)


if __name__ == "__main__":
    main()
