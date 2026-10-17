"""Reconcile delivered processor transports for the construction-probe product consumer.

@see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md task8C.3
Only exact dated producer indexes and their original saves supply acceptance; historical binaries are not requalified.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, identity

HERE = Path(__file__).parent
DECODE = runpy.run_path(str(HERE / "processor_returns_checks-3.py"))
FLOW = runpy.run_path(str(HERE / "processor_returns_checks-4.py"))["material_flow"]
PRODUCERS = {"orbital-processor": "45ddf32a5b13afccea72e1af6753cfe87cfcea5119d351ba063f9ee875406747",
             "processor-freight": "6b6479c4e5ee1c8ba31b3329253c67fc487bc56036171229e793efb95c5a9fc0",
             "processor-returns": "0a2006417589f3b28cd5149ee1cbad417aa0a15f2104cb95d48c77bf5f45c6d2"}


def main():
    """Freeze task8C.3 after original ore, construction, hauler and useful-return producers agree."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); assert not args.output.exists(); producers = {}; configurations = []
    for name, digest in PRODUCERS.items():
        path = ROOT / "logs" / (name + "-verification.json"); reference = identity(path); assert reference["sha256"] == digest
        producers[name] = (reference, json.loads(path.read_text(encoding="utf-8")))
    for configuration in ("RelWithDebInfo", "Debug"):
        selected = {}
        for name, (_, index) in producers.items():
            selected[name], = [c for c in index["configurations"] if c["configuration"] == configuration]
        returns = selected["processor-returns"]
        decoded = {key: DECODE["snapshot"](saved) for key, saved in returns["checkpoints"].items() if key.startswith("native/")}
        FLOW(decoded)
        # Exact original module construction consumes eighteen of the first twenty Steel; the other two fund Goods. Later fresh production/export has disjoint physical owners and cannot replay either consumption.
        original = selected["orbital-processor"]["checkpoints"]["native/ice-batch"]
        assert DECODE["snapshot"](original) == decoded["native/inspected"]
        freight = selected["processor-freight"]; assert freight["full_copied_refusal_pairs"] == 3 and freight["focused_cases"] == 12
        configurations.append(dict(configuration=configuration, processing_input=original,
                                   product_checkpoints={k: v for k, v in returns["checkpoints"].items() if k.startswith("native/")},
                                   hauler_checkpoints=freight["checkpoints"], native_explicit_ore_unload=True,
                                   distinct_product_construction_and_export=True, linked_train_lorry_collection=True))
    args.output.write_text(json.dumps(dict(_documentation=__doc__, completed_tasks=["8C.3"], save_version=438, configurations=configurations,
                                          dated_producer_indexes=[p[0] for p in producers.values()],
                                          sources=[identity(Path(__file__).resolve())], inherited_starting_content_procurement=False), indent=2) + "\n", encoding="utf-8")
    print(identity(args.output.resolve()), flush=True)


if __name__ == "__main__":
    main()
