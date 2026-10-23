"""Deliver original paid-world capsule cargo through real conventional Surface freight.

@file tube_tbm_delivery.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group16C.2.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This phase loads exact earned cargo;excavation,funding and production fixtures are forbidden.
"""

import argparse
import json
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
from construction_provider_return import ROOT, identity
from tube_tbm_useful import OWNER_KEYS, SOURCES, native_loaded
from runner import current_run_context

PREPARATION = ("logs/tubes/tbm-final-use/20261019-native-01/useful-prepare-verification.json",
               "13d536cf642e679636c17fbf28d30a0d91380029e87709ed44404bdd9e8126a2")
USEFUL_KEYS = ("capsule", "freight", "wagon", "mine", "factory", "source", "opening", "loaded")


def deliver(configuration, preparation_report):
    """Earn useful Surface intake and whole empty returns from exact original loaded cargo.

    @param configuration Current reader,opposite the accepted actual cargo-ready writer.
    @param preparation_report Exact report selected by the sealed16C.1 native preparation.
    @return Useful-final/full-next original file report after complete actual native success.
    """
    receipt_path = ROOT / PREPARATION[0]
    receipt_ref = identity(receipt_path)
    assert receipt_ref["sha256"] == PREPARATION[1]
    receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    selected = identity(preparation_report)
    record = next(row for row in receipt["records"].values() if row["report"] == selected)
    assert record["configuration"] != configuration
    original = record["output"]
    source = ROOT / original["path"]
    assert identity(source) == original
    context = current_run_context("tubes", "tbm-final-use")
    tag = "[.tube-tbm-useful-delivery]"
    environment = {"OTTD_TUBE_TBM_INPUT": str(source),
                   "OTTD_TUBE_TBM_OWNERS": ",".join(str(record["original_owners"][key]) for key in OWNER_KEYS),
                   "OTTD_TUBE_TBM_USEFUL": ",".join(str(record["observations"][key]) for key in USEFUL_KEYS)}
    raw, proof = native_loaded(context, configuration, tag, environment, "useful-delivery")
    match = re.search(r"TUBE_TBM_USEFUL_DELIVERED ticks=(\d+) useful=(\d+) income=(\d+) extracted=(\d+) remaining=(\d+) energy=(\d+) invoice=(\d+) capsule_return=1 freight_return=1 crossed_surface=1", raw)
    assert match is not None
    observations = dict(zip(("ticks", "useful", "income", "extracted", "remaining", "energy", "invoice"), map(int, match.groups())))
    assert min(observations.values()) > 0
    assert observations["useful"] >= record["observations"]["loaded"]
    assert observations["remaining"] + observations["extracted"] == record["observations"]["opening"]
    outputs = {}
    # Each original full world and its untouched next native opportunity are
    # retained separately;another final or a reconstructed next step cannot
    # supply later ordinary-file/reservation acceptance.
    for stage in ("useful-final", "useful-final-next"):
        saved = re.search(rf"TUBE_TBM_USEFUL_SAVE stage={stage} path=([^\r\n]+)", raw)
        assert saved is not None
        saved_path = Path(saved[1]).resolve()
        assert saved_path == context.output("saves", f"tube-tbm-{stage}.sav").resolve()
        outputs[stage] = identity(saved_path)
    assert identity(source) == original
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md16C.2;original earned finite cargo,actual curved capsule transfer/conventional Surface acceptance/native income/paid power and whole empty returns;original completed paid spoil leg retained independently",
        "configuration": configuration, "focus": "delivery", "tag": tag, "native_version": 434,
        "native": proof, "observations": observations, "original_input": original,
        "preparation_receipt": receipt_ref, "preparation_report": selected,
        "original_owners": record["original_owners"], "useful_participants": record["observations"],
        "outputs": outputs, "controller": identity(__file__),
        "native_loader": identity(ROOT / "src/tests/multilayer/tube_tbm_useful.py"),
        "binary": identity(ROOT / f"build/{configuration}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES],
    }
    path = context.output("reports", f"tube-tbm-{configuration}-useful-delivery.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return path


def main():
    """Select the sealed original cargo-ready report and current opposite native reader.

    @return None;altered inputs,unearned transfer,missing Surface intake or incomplete returns refuse.
    """
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--preparation-report", type=Path, required=True)
    args = parser.parse_args()
    print(deliver(args.config, args.preparation_report), flush=True)


if __name__ == "__main__":
    main()
