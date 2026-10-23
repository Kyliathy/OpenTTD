"""Retain actual loaded reservation queues and paid-grid recovery in the original paid world.

@file tube_tbm_queue.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group16D.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Only native purchases,orders,path signals and paid source commands create queue/recovery.
"""

import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
from construction_provider_return import ROOT, identity
from tube_tbm_useful import OWNER_KEYS, SOURCES as USEFUL_SOURCES, native_loaded
from tube_tbm_delivery import USEFUL_KEYS
from runner import current_run_context

DELIVERY = ("logs/tubes/tbm-final-use/20261019-native-01/useful-delivery-verification.json",
            "9420fa1df3b943b6acb1fc8d4a1d44f7f103a57a500f6896defeeca36bd09662")
SOURCES = (*USEFUL_SOURCES, "src/tests/tube_tbm_queue_fixture.h", "src/pbs.cpp", "src/transport_tube_stock.cpp")
STAGES = ("queued", "starved", "restored", "recovered")


def observe(configuration, delivery_report):
    """Earn actual competing queue,closed-source hold and paid useful recovery from an exact useful original.

    @param configuration Current native reader,opposite the accepted actual useful writer.
    @param delivery_report Explicit useful-final report already sealed by16C.
    @return All original queue/starved/restored/recovered worlds and untouched full-next baselines.
    """
    receipt_path = ROOT / DELIVERY[0]
    receipt_ref = identity(receipt_path)
    assert receipt_ref["sha256"] == DELIVERY[1]
    receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    selected = identity(delivery_report)
    record = next(row for row in receipt["records"].values() if row["report"] == selected)
    assert record["configuration"] != configuration
    original = record["outputs"]["useful-final"]
    source = ROOT / original["path"]
    assert identity(source) == original
    context = current_run_context("tubes", "tbm-final-use")
    participants = dict(record["useful_participants"], loaded=0)
    tag = "[.tube-tbm-queue-recovery]"
    environment = {"OTTD_TUBE_TBM_INPUT": str(source),
                   "OTTD_TUBE_TBM_OWNERS": ",".join(str(record["original_owners"][key]) for key in OWNER_KEYS),
                   "OTTD_TUBE_TBM_USEFUL": ",".join(str(participants[key]) for key in USEFUL_KEYS)}
    raw, proof = native_loaded(context, configuration, tag, environment, "queue-recovery")
    match = re.search(r"TUBE_TBM_QUEUE_RECOVERY queue_ticks=(\d+) recovery_ticks=(\d+) peer=(\d+) winner=(\d+) waiting=(\d+) queued_units=(\d+) useful=(\d+) old_source=(\d+) fresh_source=(\d+) old_energy=(\d+) fresh_energy=(\d+)", raw)
    assert match is not None
    observations = dict(zip(("queue_ticks", "recovery_ticks", "peer", "winner", "waiting", "queued_units", "useful", "old_source", "fresh_source", "old_energy", "fresh_energy"), map(int, match.groups())))
    assert observations["queued_units"] > 0
    assert observations["useful"] >= record["observations"]["useful"] + observations["queued_units"]
    assert observations["winner"] != observations["waiting"]
    assert observations["fresh_source"] != observations["old_source"]
    assert observations["fresh_energy"] > 0 and observations["old_energy"] >= record["observations"]["energy"]
    outputs = {}
    # Retain every actual physical queue/loss/repair/recovered boundary and
    # its next whole native opportunity separately. No later reconstruction
    # may stand in for these original packets,reservations or consumed rights.
    for stage in STAGES:
        for suffix in ("", "-next"):
            label = stage + suffix
            saved = re.search(rf"TUBE_TBM_USEFUL_SAVE stage={label} path=([^\r\n]+)", raw)
            assert saved is not None
            saved_path = Path(saved[1]).resolve()
            assert saved_path == context.output("saves", f"tube-tbm-{label}.sav").resolve()
            outputs[label] = identity(saved_path)
    assert identity(source) == original
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md16D.1;actual paid same-floor second capsule/native PBS queue,retired external service,intact loaded power hold,fresh paid reconnection and resumed useful Surface freight;full ordinary file replay remains16D.2",
        "configuration": configuration, "focus": "queue", "tag": tag, "native_version": 434,
        "native": proof, "observations": observations, "original_input": original,
        "delivery_receipt": receipt_ref, "delivery_report": selected,
        "original_owners": record["original_owners"], "useful_participants": participants,
        "outputs": outputs, "controller": identity(__file__),
        "native_loader": identity(ROOT / "src/tests/multilayer/tube_tbm_useful.py"),
        "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES],
    }
    path = context.output("reports", f"tube-tbm-{configuration}-queue.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return path


def main():
    """Select one sealed actual useful original and current opposite native queue observer.

    @return None;missing physical queue,intact hold,paid repair or useful recovery refuses.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--delivery-report", type=Path, required=True)
    args = parser.parse_args()
    print(observe(args.config, args.delivery_report), flush=True)


if __name__ == "__main__":
    main()
