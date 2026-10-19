"""Bind both native bank receipts before later focused sources relink their products.

@file surface_entry_checks-6.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BC.1.
Only credit is accepted here; paid factories and physical freight remain separate.
"""
import argparse
import json
from pathlib import Path

from space_preview_checks import ROOT, chunks, identity


def verify(reference, retained=None):
    """Check one original input, using an explicit retained executable when relinked.

    @param reference Exact original path and digest.
    @param retained Optional immutable copy of that exact tested executable.
    @return Actual verified path.
    """
    path = ROOT / (retained or reference["path"])
    assert identity(path)["sha256"] == reference["sha256"], path
    return path


def bind():
    """@return Both-build credit proof with exact native process and bank ownership."""
    snapshots, entries, processes = [], [], set()
    # Both configurations must borrow from the identical original game, then
    # restore without a second loan. Archive aliases preserve tested products
    # when subsequent factory test sources relink the shared focused target.
    for configuration in ("RelWithDebInfo", "Debug"):
        membership = ROOT / f"logs/surface-entry-credit-{configuration}.json"
        proof = json.loads(membership.read_text())
        assert proof["complete"] and proof["configuration"] == configuration
        archive = f"test-cache/surface-entry/credit-products/{configuration}/openttd_surface_supply_test.exe"
        verify(proof["native"]["application"], archive)
        for reference in proof["sources"]:
            verify(reference)
        for operation in (proof["native"], proof["ordinary"]):
            for key in ("source", "terminal"):
                verify(operation[key])
            if "output" in operation:
                verify(operation["output"])
                verify(operation["application"])
            for reference in operation.get("snapshots", []):
                verify(reference)
            for reference in operation["native_processes"]:
                child = json.loads(verify(reference).read_text())
                assert child["exit_status"] == 0
                retained = archive if child["executable"] == proof["native"]["application"] else None
                verify(child["executable"], retained)
                for artifact in child["inputs"] + child["artifacts"]:
                    verify(artifact)
                processes.add(reference["path"])
        assert proof["native"]["final_cash"] == proof["native"]["final_debt"] == 2000000
        assert proof["ordinary"]["money"] == proof["ordinary"]["loan"] == 2000000
        snapshots.append(chunks(verify(proof["native"]["snapshots"][-1]))[1])
        entries.append(dict(membership=identity(membership), retained_application=identity(ROOT / archive)))
    assert snapshots[0] == snapshots[1]
    assert len(processes) == 4
    return dict(_documentation=__doc__, complete=True, configurations=entries, native_processes=4,
                exact_cross_configuration_families=len(snapshots[0]), cash=2000000, debt=2000000,
                expense=0, supplied_stock=False, source=identity(Path(__file__)))


def main():
    """Publish a fresh immediate bank acceptance receipt without native reruns."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    target = (ROOT / args.output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.write_text(json.dumps(bind(), indent=2) + "\n", encoding="utf-8")
    print(target.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
