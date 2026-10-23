"""Drive the exact original paid Shield world through real useful freight and whole empty returns.

@file shield_useful_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15ECC.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Finite extraction continues; prepared floor and Connected shafts remain explicit boundary inputs.
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
from geothermal_tube_binding_files import world_chunks
from shield_source_files import source_environment
from shield_route_prepare import SOURCES as PREPARE_SOURCES

PREPARE_RECEIPT = "logs/tubes/shield-route/20261018-native-01/prepare-verification.json"
PREPARE_SHA256 = "8f09b96720da64daacadaaf5aff59686a07e57d0a93aed98951f1a4aa3763201"
SOURCES = (*PREPARE_SOURCES, "src/vehicle.cpp", "src/economy.cpp", "src/cargopacket.cpp")


def original_route_environment(prepared, receipt):
    """Transport original source roles and actual paid route references without constructing participants.

    @param prepared Exact accepted preparation report for the opposite native configuration.
    @param receipt Immutable preparation receipt preserving the original finite-source lineage.
    @return Native borrowed-loader environment and independent ready/reserve/minimum-receipt facts.
    """
    source_path = ROOT / receipt["source_receipt"]["path"]
    assert identity(source_path) == receipt["source_receipt"]
    source = json.loads(source_path.read_text(encoding="utf-8"))["source"]
    point = prepared["prepared"]
    environment = source_environment(source)
    values = (point["tick"], point["cash"], point["loan"], source["reserve_before"], source["units"], source["income"])
    environment.update(
        OTTD_SHIELD_ROUTE_PARTS=",".join(map(str, point["parts"][2:])),
        OTTD_SHIELD_ROUTE_REFERENCES=",".join(map(str, (*point["stations"], point["power"]))),
        OTTD_SHIELD_USEFUL_VALUES=",".join(map(str, values)),
    )
    return environment


def validate_result(result, scenes, returned, point):
    """Check useful native accounting and physical observations independently of cross-build equality.

    @param result Exact final native route, reserve, stock and accounting observations.
    @param scenes First actual loaded states of both conventional layer crossings and local capsule.
    @param returned Genuine whole empty home arrivals after useful intake and payment.
    @param point Accepted original still-stopped paid world facts.
    @return None; missing movement, stock loss or duplicated intake prevents acceptance.
    """
    assert result["year"] == 1985 and result["original_tick"] == point["tick"]
    assert 0 < result["route_ticks"] < 24000
    assert result["tick"] == point["tick"] + result["route_ticks"] + 148
    assert result["delivered"] == result["receiver"] >= point["stock"]
    assert result["income"] > 0 and result["cash"] > 0 and result["loan"] == 0 and result["consumed"] > 0
    assert result["reserve"] + result["extracted"] == point["reserve"] + point["stock"]
    assert result["extracted"] == result["dispatched"] + result["source_buffer"]
    assert result["extracted"] == result["source_buffer"] + result["waiting"] + result["receiver"]
    assert len(scenes) == 5 and {row["scene"] for row in scenes} == set(range(5))
    assert len(returned) == 3 and {row["role"] for row in returned} == set(range(3))
    # Partial native loads are legitimate observations. Every scene must precede
    # the completed run, and every whole empty return follows actual final-leg
    # movement; initial storage cannot impersonate a useful freight return.
    for row in scenes:
        assert row["cargo"] > 0 and point["tick"] < row["tick"] <= result["tick"] - 148
    for row in returned:
        assert max(scene["tick"] for scene in scenes) < row["tick"] <= result["tick"] - 148


def main():
    """Continue an exact opposite ready world through native useful freight, conserving ongoing extraction.

    @return None; failed native ownership, payment, movement or return checks prevent a useful report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    parser.add_argument("--prepare-report", type=Path, required=True)
    args = parser.parse_args()
    receipt = ROOT / PREPARE_RECEIPT
    assert identity(receipt)["sha256"] == PREPARE_SHA256
    accepted = json.loads(receipt.read_text(encoding="utf-8"))
    selected = identity(args.prepare_report)
    assert selected in [row["report"] for row in accepted["records"] if row.get("focus") == "route-prepare"]
    prepared = json.loads(args.prepare_report.read_text(encoding="utf-8"))
    assert prepared["configuration"] != args.config
    original = prepared["native"]["output"]
    source = ROOT / original["path"]
    assert identity(source) == original
    context = current_run_context("tubes", "shield-route")
    context.prepare_runtime()
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15ECC; actual original Shield useful freight.\nshield_useful_check\n", encoding="utf-8")
    environment = original_route_environment(prepared, accepted)
    environment["OTTD_SHIELD_ROUTE_INPUT"] = str(source)
    output, proof = native(context, args.config, "[.tube-shield-useful-native]", "shield-useful", environment)
    names = ("year", "original_tick", "tick", "route_ticks", "delivered", "receiver", "income", "cash", "loan", "consumed",
             "reserve", "extracted", "dispatched", "source_buffer", "waiting", "packets", "quarter_history")
    pattern = "SHIELD_USEFUL " + " ".join(name + r"=(\d+)" for name in names)
    pattern += r" scenes=5 returns=3 empty_ticks=148 path=([^\r\n]+)"
    match = re.search(pattern, output)
    assert match is not None
    result = dict(zip(names, map(int, match.groups()[:len(names)])))
    scenes = [dict(zip(("scene", "tick", "cargo", "tile"), map(int, row)))
              for row in re.findall(r"SHIELD_ROUTE_SCENE scene=(\d+) tick=(\d+) cargo=(\d+) tile=(\d+)", output)]
    returned = [dict(zip(("role", "tick"), map(int, row)))
                for row in re.findall(r"SHIELD_ROUTE_RETURN role=(\d+) tick=(\d+)", output)]
    validate_result(result, scenes, returned, prepared["prepared"])
    saved = Path(match[len(names) + 1])
    assert len(world_chunks(saved)) == 126 and identity(source) == original
    proof.update(input=original, output=identity(saved))
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15ECC; actual original Shield useful native freight and whole empty returns with continuing finite extraction",
        "configuration": args.config, "focus": "shield-useful", "native": proof,
        "result": result, "scenes": scenes, "returned": returned,
        "prepare_report": selected, "prepare_receipt": identity(receipt), "controller": identity(__file__),
        "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"), "sources": [identity(ROOT / name) for name in SOURCES],
        "fixture_classification": "Exact original paid native world; full common clocks/fronts/industry work and real producer dispatch/earned station service carry finite Deep RARE through conventional Deep/Underground feed, local powered Shield and conventional Surface ELEC intake/payment. Original reserve, all stock/packets and native current/historical accounts conserve ongoing extraction. Three whole empty returns and148 native opportunities cannot replay income/intake; genuine mine queues may remain. Prepared floor/machinery/Connected shafts remain boundary inputs. No RFMT/Electronics manufacturing, operating profit, source freeze, packet/rating grant, pose/order/clock/cash repair or TBM/bootstrap claim.",
    }
    path = context.output("reports", f"shield-useful-{args.config}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
