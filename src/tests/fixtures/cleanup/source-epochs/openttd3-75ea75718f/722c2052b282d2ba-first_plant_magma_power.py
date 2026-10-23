"""Reconcile original two-model power and ordinary paid-Magma generation files.

@file first_plant_magma_power.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8FCA-8FCB.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Group15BBC atomic source-replacement continuation.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs1-f-utility-and-transfer-contracts.md Group19.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
Historical unused generation is never interpreted as a spendable battery.
"""
import argparse
import importlib
import json
from pathlib import Path

from first_plant_checks import saved_table
from first_plant_service_checks import ROOT, identity, complete_chunks, native, saved
from runner import current_run_context


def verify_native_power(path, magma_ticks=75, magma_use=9990):
    """Reconcile actual original Heat, both native models and the unique town effect.

    @param path Genuine completed same-world2000 useful-service original or resave.
    @param magma_ticks Actual expected committed model ticks, including its one real warmup and any explicit later recovery.
    @param magma_use Expected settled original Magma electricity, including a recovery only when that case requests one.
    @return Actual native ledgers and bounded useful effect; no quantity is supplied to the game.
    """
    generator_rows = saved_table(path, b"GPGD")
    assert len(generator_rows) == 2
    generators = {row["model"]: row for row in generator_rows.values()}
    assert set(generators) == {0, 1}
    rows = list(saved_table(path, b"UPLD").values())
    producers = {row["id"]: row for row in rows if row["type"] == 0}
    assert len(producers) == 3
    collectors = saved_table(path, b"GCBD")
    assert len(collectors) == 1
    collector = next(iter(collectors.values()))
    heat = producers[collector["heat_producer"]]
    assert heat["domain"] == 1 and heat["a"] == 1000000 and heat["c"] == 0
    converted_heat = 0
    outputs = {}
    # Each actual machinery record names its own original shared output.
    # Their thermal use must reconcile together against the ONE collector;
    # neither generator may claim the whole vent's lifetime independently.
    for model, efficiency, watts_j, daily_cost in [(0, 15, 4050, 20), (1, 35, 9450, 35)]:
        state = generators[model]
        assert state["materials_committed"] == 1
        assert state["work_ticks"] == (592 if model == 0 else 888)
        output = producers[state["electrical_producer"]]
        ticks = state["active_service_ticks"]
        heat_j = ticks * 27000
        assert output["domain"] == 0 and output["c"] == 0
        assert output["lifetime_generated"] == ticks * watts_j
        assert output["lifetime_generated"] * 100 == heat_j * efficiency
        assert output["lifetime_consumed"] == (9990 if model == 0 else magma_use)
        assert state["service_remainder"] == ticks * daily_cost % 74
        converted_heat += heat_j
        outputs[model] = output
    assert heat["lifetime_consumed"] == converted_heat
    assert generators[0]["active_service_ticks"] == 10285
    assert generators[1]["active_service_ticks"] == magma_ticks
    services = [row for row in rows if row["type"] == 1 and row["town_terminal"] != 65535]
    assert len(services) == 1
    service = services[0]
    assert (service["id"], service["town"], service["town_terminal"]) == (3, 0, 3)
    assert (service["town_bonus"], service["town_applied"], service["town_suspended"]) == (25, 1, 0)
    # Native growth_rate is the reset counter (interval minus one). Retain
    # the real paid town's original baseline11269 and its single25% benefit.
    growth_rate = saved_table(path, b"CITY")[0]["growth_rate"]
    assert growth_rate == ((11269 + 1) * 100 + 124) // 125 - 1 == 9015
    return {"native_generators": generators, "original_heat": heat, "original_outputs": outputs,
            "actual_town_service": service, "native_town_growth_counter": growth_rate,
            "heat_consumed_total": converted_heat,
            "electrical_generated_total": sum(row["lifetime_generated"] for row in outputs.values()),
            "useful_electricity_total": sum(row["lifetime_consumed"] for row in outputs.values()),
            "magma_warmup_ticks": 1, "magma_useful_service_ticks": magma_ticks - 1,
            "magma_service_invoice": magma_ticks * 35 // 74}


def main():
    """Run one bounded ledger, native-file, continuation or normal-production burst.

    @return None; publish exact actual proof identities or refuse incomplete evidence.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--wire-report", type=Path, required=True)
    parser.add_argument("--operation-report", type=Path, required=True)
    parser.add_argument("--phase", choices=("ledger", "files", "continuation", "boundaries", "production"), required=True)
    parser.add_argument("--boundary-report", type=Path, help="Accepted actual date/view/recovery proof, adding those real ordinary originals to the production burst.")
    args = parser.parse_args()
    wire, operation = [json.loads(p.read_text()) for p in [args.wire_report, args.operation_report]]
    config = wire["configuration"]
    assert operation["configuration"] == config
    context = current_run_context("geothermal", "first-plant-magma-power")
    checkpoints = {name: saved(wire, name) for name in ["wired", "connected"]}
    checkpoints.update({name: saved(operation, name) for name in ["partial-generation", "generation-complete"]})
    energy = verify_native_power(checkpoints["generation-complete"])
    proofs = []
    boundary_outputs = {}
    if args.phase == "files":
        # First and repeated ordinary native loads retain every whole chunk,
        # including unbound/bound topology and real partially settled service.
        for name, original in checkpoints.items():
            previous = original
            for iteration in range(2):
                report, proof = native(config, "inspect", previous, stage="magma")
                previous = saved(report, "inspect")
                assert complete_chunks(original) == complete_chunks(previous), (name, iteration)
                proofs.append(dict(proof, checkpoint=name, load=iteration + 1))
    elif args.phase == "continuation":
        # A wired but not yet operated native file binds actual sources once;
        # an interrupted37-use original completes without a second effect,
        # repurchase, fabricated heat or manually advanced utility interval.
        for name, action, target in [("wired", "wire", "connected"), ("partial-generation", "operate", "generation-complete")]:
            report, proof = native(config, action, checkpoints[name], stage="magma",
                                   town_command=bool(wire.get("town_command", False)) and action == "wire")
            output = saved(report, target)
            assert complete_chunks(checkpoints[target]) == complete_chunks(output), name
            proofs.append(dict(proof, checkpoint=name, complete_chunks_equal=True))
    elif args.phase == "boundaries":
        view, proof = native(config, "view", checkpoints["connected"], stage="magma")
        proofs.append(proof)
        # Actual main-view Surface/Deep selection must leave every saved
        # native chunk unchanged at both real useful-service checkpoints.
        for name in ["partial-generation", "generation-complete"]:
            assert complete_chunks(checkpoints[name]) == complete_chunks(saved(view, name)), name
        blocked, proof = native(config, "boundary", checkpoints["generation-complete"], boundary="date", stage="magma")
        proofs.append(proof)
        blocked_path = saved(blocked, "blocked-date")
        previous = blocked_path
        # Both ordinary reloads retain a real already-paid model while its
        # calendar is1999. Historical purchases and machine identity are not
        # erased to hide the unavailable state or renew its settled energy.
        for iteration in range(2):
            checked, proof = native(config, "inspect", previous, stage="magma")
            previous = saved(checked, "inspect")
            assert complete_chunks(blocked_path) == complete_chunks(previous)
            proofs.append(proof)
        recovered, proof = native(config, "retry", blocked_path, stage="magma")
        proofs.append(proof)
        recovered_path = saved(recovered, "retried")
        verify_native_power(recovered_path, magma_ticks=76, magma_use=10125)
        checked, proof = native(config, "inspect", recovered_path, stage="magma")
        assert complete_chunks(recovered_path) == complete_chunks(saved(checked, "inspect"))
        proofs.append(proof)
        boundary_outputs = {"blocked_date": identity(blocked_path), "retried": identity(recovered_path)}
    elif args.phase == "production":
        if args.boundary_report:
            boundaries = json.loads(args.boundary_report.read_text())
            assert boundaries["configuration"] == config
            checkpoints.update({name: ROOT / boundaries[name]["path"] for name in ["blocked_date", "retried"]})
        ordinary = importlib.import_module("first_plant_service_checks-2").ordinary
        # The normal game has no scenario observer. Its startup loader's one
        # possible CargoDist wait flag uses the existing strict field check.
        for name, original in checkpoints.items():
            proofs.append(ordinary(config, f"magma-{name}", original))
        proofs.append(ordinary(config, "magma-power-repeat", ROOT / proofs[-1]["output"]["path"]))
    result = {"_documentation": "r2mlvs18 Groups8FCA-8FCB; same original paid world, two native models and unique useful town service",
              "configuration": config, "phase": args.phase, "controller": identity(__file__),
              "parents": [identity(args.wire_report), identity(args.operation_report)],
              "energy": energy, "reports": proofs,
              "complete_chunks_compared": args.phase in ("files", "continuation", "boundaries"), **boundary_outputs}
    if args.boundary_report:
        result["ordinary_boundary_parent"] = identity(args.boundary_report)
    path = context.output("reports", f"magma-power-{config}-{args.phase}.json")
    path.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
