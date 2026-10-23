"""Verify original paid Magma construction through ordinary isolated loads and continuations.

@file first_plant_magma_files.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CE.
@see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups8FCA-8FCB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
No helper supplies a replacement world, physical stock, labour or energy.
"""
import argparse
import importlib
import json
from pathlib import Path

from first_plant_service_checks import ROOT, identity, complete_chunks, native, saved
from runner import current_run_context


def main():
    """Run one bounded native original/continuation or ordinary-production burst for actual purchased construction.

    @return None; retain exact ordinary native source/binary/checkpoint evidence or fail.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--funding-report", type=Path, required=True)
    parser.add_argument("--delivery-report", type=Path, required=True)
    parser.add_argument("--phase", choices=("originals", "continuation", "production"), required=True)
    args = parser.parse_args()
    funding = json.loads(args.funding_report.read_text())
    delivery = json.loads(args.delivery_report.read_text())
    config = funding["configuration"]
    assert delivery["configuration"] == config
    context = current_run_context("geothermal", "first-plant-magma-files")
    stages = {name: saved(funding, name) for name in ("funded", "purchased")}
    stages.update({name: saved(delivery, name) for name in ("outbound", "partial-construction", "construction-complete")})
    proofs = []
    if args.phase == "originals":
        # Separate actual processes must preserve every complete native chunk
        # on first and repeated load, including dormant1980 history and the
        # genuinely transported52-unit second bill at its unfinished phases.
        for name, original in stages.items():
            previous = original
            for iteration in range(2):
                report, proof = native(config, "inspect", previous, stage="magma")
                output = saved(report, "inspect")
                assert complete_chunks(original) == complete_chunks(output), (name, iteration)
                proofs.append(dict(proof, checkpoint=name, load=iteration + 1))
                previous = output
    elif args.phase == "continuation":
        # A genuinely funded file buys its missing obligation once; a real
        # partial-work file buys nothing. Both must reproduce the exact whole
        # uninterrupted native world, including original orders and expenses.
        for name in ("funded", "partial-construction"):
            report, proof = native(config, "deliver", stages[name], stage="magma")
            output = saved(report, "construction-complete")
            assert complete_chunks(stages["construction-complete"]) == complete_chunks(output), name
            proofs.append(dict(proof, checkpoint=name, complete_chunks_equal=True))
    else:
        # Reuse the already-verified normal-game startup validator. Its
        # chronological burst suffix is a valid importlib module name, while
        # the normal Python import statement cannot spell a hyphenated name.
        ordinary = importlib.import_module("first_plant_service_checks-2").ordinary
        for name, original in stages.items():
            proofs.append(ordinary(config, f"magma-{name}", original))
        complete = proofs[-1]
        proofs.append(ordinary(config, "magma-complete-repeat", ROOT / complete["output"]["path"]))
    result = {"_documentation": "r2mlvs6 Group13CE; r2mlvs18 Groups8FCA-8FCB; actual paid construction ordinary-file acceptance",
              "configuration": config, "phase": args.phase, "controller": identity(__file__),
              "shared_native_controller": identity(ROOT / "src/tests/multilayer/utilities/first_plant_service_checks.py"),
              "shared_game_controller": identity(ROOT / "src/tests/multilayer/utilities/first_plant_service_checks-2.py"),
              "parents": [identity(args.funding_report), identity(args.delivery_report)], "reports": proofs,
              "comparison": "Every complete native chunk" if args.phase != "production" else "Strict complete owners and clock fields; only the native startup LinkGraph pause bit may be added before the first frame"}
    path = context.output("reports", f"magma-construction-{config}-{args.phase}.json")
    path.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
