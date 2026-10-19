"""Bounded single-state replay of accepted ordinary purchased-cargo evidence.

@file surface_supply_delivery_checks-2-1.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Group13CD.2.
The initial Debug batch took124seconds; future ordinary bursts select one state
and execute four bounded children. The accepted original batch remains immutable.
"""

import argparse
import json
from pathlib import Path
import runpy

SHARED = runpy.run_path(str(Path(__file__).with_name("surface_supply_delivery_checks-2.py")))
ROOT, checked, ordinary, compare = (SHARED[name] for name in ("ROOT", "checked", "ordinary", "compare"))
identity = SHARED["identity"]


def execute(accepted, state):
    """Replay one actual accepted state without rerunning its sibling groups.

    @param accepted Immutable actual ordinary batch receipt, including original native inputs/outputs.
    @param state Exactly one pending, travelling or full-receiver boundary.
    @return Original new process evidence and exact comparisons against accepted native owners.
    """
    assert accepted["complete"] and accepted["phase"] == "ordinary"
    entry, = [row for row in accepted["evidence"] if row["state"] == state]
    first_old, second_old, continued_old, repeated_old = entry["processes"]
    configuration = accepted["configuration"]
    source = checked(first_old["source"])
    first, first_proof = ordinary(configuration, source, state + "-bounded-load")
    compare(source, first, initial=True)
    compare(checked(first_old["output"]), first)
    second, second_proof = ordinary(configuration, first, state + "-bounded-resave")
    compare(first, second)
    continued, continued_proof = ordinary(configuration, first, state + "-bounded-continue", True)
    repeated, repeated_proof = ordinary(configuration, second, state + "-bounded-resaved-continue", True)
    compare(continued, repeated)
    # Compare with actual previously accepted native bytes, including original
    # money/receipts/cargo/history. No handwritten stock or timing model can
    # substitute for the selected state's physical continuation.
    compare(checked(continued_old["output"]), continued)
    compare(checked(repeated_old["output"]), repeated)
    return dict(state=state, owners=entry["owners"],
                processes=[first_proof, second_proof, continued_proof, repeated_proof])


def main():
    """Select one bounded ordinary state and retain a fresh explicit receipt."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--accepted", type=Path, required=True)
    parser.add_argument("--state", choices=("paid-pending", "loaded-deep", "full-refusal-return"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    original = (ROOT / args.accepted).resolve()
    accepted = json.loads(original.read_text(encoding="utf-8"))
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    evidence = execute(accepted, args.state)
    target.write_text(json.dumps(dict(_documentation=__doc__, complete=True,
                                     configuration=accepted["configuration"], phase="bounded-ordinary",
                                     accepted=identity(original), evidence=evidence,
                                     harness=identity(Path(__file__))), indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
