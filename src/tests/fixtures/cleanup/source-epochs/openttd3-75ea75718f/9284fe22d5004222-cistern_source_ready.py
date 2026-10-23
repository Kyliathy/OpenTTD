"""Inspect and advance the exact original useful chemical source through ordinary native loading.

@file cistern_source_ready.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DBD.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Every reader uses original completed haulage output; no producer reconstruction or Cistern movement occurs.
"""
import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/support"))
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from runner import current_run_context
from local_electrical_feed_checks import identity, native
from geothermal_tube_work_files import compare_chunks
from cistern_source_haul import SOURCES as HAUL_SOURCES, STOCK_RECEIPT
from cistern_calendar_files import compare_capsule_calendar

HAUL_RECEIPT = "logs/tubes/cistern-source/20261018-native-01/haul-verification.json"
SOURCES = (*HAUL_SOURCES, "src/saveload/afterload.cpp", "src/saveload/industry_sl.cpp",
           "src/saveload/company_sl.cpp", "src/saveload/vehicle_sl.cpp", "src/saveload/layer_processor_sl.cpp",
           "src/saveload/engine_sl.cpp", "src/transport_tube.h", "src/transport_tube_engine.cpp", "src/tests/multilayer/cistern_calendar_files.py")


def main():
    """Read one exact opposite completed source twice through distinct untouched and next-opportunity processes.

    @return None; changed originals, lost loaded owners, altered histories or nondeterministic complete resaves prevent acceptance.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--haul-report", type=Path, required=True)
    args = parser.parse_args()
    receipt = ROOT / HAUL_RECEIPT
    assert identity(receipt)["sha256"] == "31c37be8f6f7cf061eee94c8e66ff8790774c9937c3097b6bfb3d49f4a5688b1"
    history = json.loads(receipt.read_text(encoding="utf-8")); selected = identity(args.haul_report)
    assert selected in [row["report"] for row in history["records"]]
    haul = json.loads(args.haul_report.read_text(encoding="utf-8")); assert haul["configuration"] != args.config
    result = haul["result"]; original = haul["native"]["output"]; source = ROOT / original["path"]; assert identity(source) == original
    stock_receipt = ROOT / STOCK_RECEIPT; assert identity(stock_receipt) == history["stock_receipt"]
    stocks = json.loads(stock_receipt.read_text(encoding="utf-8")); carrier_receipt = ROOT / stocks["carrier_receipt"]["path"]
    assert identity(carrier_receipt) == stocks["carrier_receipt"]
    carrier = json.loads(carrier_receipt.read_text(encoding="utf-8"))
    context = current_run_context("tubes", "cistern-source"); context.prepare_runtime()
    environment = {
        "OTTD_CISTERN_SOURCE_INPUT": str(source),
        "OTTD_CISTERN_STOCK_PARTS": ",".join(map(str, result["parts"])),
        "OTTD_CISTERN_STOCK_INDUSTRIES": ",".join(map(str, result["industries"])),
        "OTTD_CISTERN_STOCK_STATIONS": ",".join(map(str, result["stations"])),
        "OTTD_CISTERN_STOCK_DEPOSIT": str(carrier["source_contracts"]["salt_deposit"]),
        "OTTD_CISTERN_STOCK_TANKER_ENGINE": str(carrier["carrier"]["original_engine"]),
        "OTTD_CISTERN_READY_VALUES": ",".join(str(result[key]) for key in ("tick", "oil", "salt", "chemicals", "unused_oil", "unused_salt", "income", "cash", "loan", "reserve", "quarter_history")),
    }
    observations = []
    # Each mode loads the original accepted world in a separate real process.
    # The native mode executes exactly zero or one shared opportunity; repeated
    # resaves cannot disguise a reconstruction, a raw replay or hidden work.
    for step in (0, 1):
        process = context.for_process(f"ready-{step}")
        process.prepare_runtime()
        startup = process.runtime_dir / "scripts/game_start.scr"; startup.parent.mkdir(parents=True, exist_ok=True)
        startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15DBD; original ready-source native inspection.\ncistern_ready_check\n", encoding="utf-8")
        output, proof = native(process, args.config, "[.tube-cistern-source-ready-native]", f"ready-{step}", dict(environment, OTTD_CISTERN_READY_STEP=str(step)))
        names = ("step", "tick", "chemicals", "unused_oil", "unused_salt", "income", "cash", "loan", "reserve", "work", "revision", "digest")
        pattern = "CISTERN_READY " + " ".join(name + r"=(\d+)" for name in names) + r" before=([^\r\n]+) first=([^\r\n]+) repeat=([^\r\n]+)"
        match = re.search(pattern, output); assert match is not None
        observed = dict(zip(names, map(int, match.groups()[:12])))
        assert observed["step"] == step and observed["tick"] == result["tick"] + step
        for key in ("chemicals", "unused_oil", "unused_salt", "income", "loan", "reserve"):
            assert observed[key] == result[key]
        before, first, repeat = map(Path, match.groups()[12:])
        anchors = [dict(zip(("model", "engine", "intro_date", "flags", "company_avail"), map(int, row))) for row in re.findall(r"CISTERN_CAPSULE_DATE model=(\d+) engine=(\d+) intro_date=(\d+) flags=(\d+) company_avail=(\d+)", output)]
        load = compare_capsule_calendar(source, before, anchors, result["year"])
        assert compare_chunks(first, repeat, None) == 126
        if step == 0:
            assert compare_chunks(before, first, None) == 126
        proof.update(input=original, before=identity(before), output=identity(first), repeat=identity(repeat), observed=observed, original_load=load, capsule_definitions=anchors)
        observations.append(proof)
    assert compare_chunks(ROOT / observations[0]["before"]["path"], ROOT / observations[1]["before"]["path"], None) == 126
    assert identity(source) == original
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15DBD; exact original useful chemical source reload/full-native next opportunity and pure repeated native output",
        "configuration": args.config, "focus": "source-ready", "native": observations,
        "haul_report": selected, "haul_receipt": identity(receipt), "controller": identity(__file__),
        "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"), "sources": [identity(ROOT / name) for name in SOURCES],
        "fixture_classification": "Both processes load the exact accepted completed haulage output. All125 non-ENGN original chunks, including GLOG, remain exact before work; only the precise public five-capsule definition date/publication compatibility change is admitted inside ENGN, with every other byte exact. Untouched and full-driver next states preserve inventory/accounts/whole storage; complete repeated saves remain exact. No fixture repair, source reconstruction, raw/output grant, Cistern carriage or TBM/bootstrap claim.",
    }
    path = context.output("reports", f"cistern-source-ready-{args.config}.json"); path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
