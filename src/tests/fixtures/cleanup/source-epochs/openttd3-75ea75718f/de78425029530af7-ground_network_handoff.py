"""Publish the selected ground/tube/riser real-peer acceptance checkpoint.

@file ground_network_handoff.py
@see r2mlvs3mp-master-plan.md11.3 and r2mlvs15-savegame-determinism-and-lifecycle.md.
Consumes immutable dated child receipts;does not reinterpret earlier binaries as current builds.
"""

import argparse
import json
import sys

sys.dont_write_bytecode = True
from ground_network import ROOT
from construction_provider_return import identity

RECEIPTS = {
    "transport": ("transport-verification.json", "0dead2b120c824ac33f97427fbda25e1c4c5d8b7cefd4200da27e6956f780771"),
    "complete-state": ("trace-verification.json", "ec3a2db9bd75abbd1f5efc328e4310b2431a03160cde886df7ddb68f05c29397"),
    "company-commands": ("company-verification.json", "189c805c2dd591b4223f8797155ab9aa6bea6f4158837cb90ebbf1067f2647f3"),
    "active-excavation-wagons": ("active-verification.json", "778fbc37a72486d7773141157d2c0e059a3c3c8fcd043cc377d38fdd18d059aa"),
    "tube-thermal-jobs": ("delivered-verification.json", "4585dc35329ff2991cc0b1d917f73c57aabe46d519d0723be585f5a8318951c7"),
    "pause": ("pause-verification.json", "adacced921a804d405289ea77ae9371036a684f4d57e6a6a96d0ba6108c94b95"),
    "starting-time-modes": ("clock-verification.json", "8ac59fbcc2e0b82e0b376aa83822cfc6caa8035ee68c66d1f4b25e5037d72b58"),
    "tube-power-loss": ("power-loss-verification.json", "2739375a1c6143fdfaf63c5f2962af2f4ac38a3d0dad0f860b71c3d9060c5ea6"),
    "paid-cargo-recovery": ("power-recovery-verification.json", "89e1de5ab7ba59e1afebc4712cd72e87d63234e719b7e7f83976fe7c3d0b4b97"),
    "thermal-riser-recovery": ("thermal-riser-verification.json", "f423e8a1d61e63a0394dbfc3350d2652e2f4e2de41279f66c3f106b8b2a86037"),
}


def publish(output):
    """Bind every independently accepted selected scenario without reopening closed groups.

    @param output New immutable checkpoint receipt under repository logs.
    @return Selected master11.3 acceptance and explicit remaining whole-plan limits.
    """
    selected = {}
    cases = assertions = 0
    for scope, (name, expected) in RECEIPTS.items():
        path = ROOT / "logs/network/paid-ground/20261019-native-01" / name
        reference = identity(path)
        assert reference["sha256"] == expected, scope
        child = json.loads(path.read_text(encoding="utf-8"))
        selected[scope] = {"receipt": reference, "native_cases": child.get("native_cases", 0),
                           "native_assertions": child.get("native_assertions", 0)}
        cases += selected[scope]["native_cases"]
        assertions += selected[scope]["native_assertions"]
    assert len(selected) == 10 and cases == 60
    assert all(row["native_cases"] > 0 and row["native_assertions"] > 0 for scope, row in selected.items() if scope != "transport")
    result = {"_documentation": "r2mlvs3mp-master-plan.md11.3 selected ground/tube/riser real-host/client acceptance;dated exact complete-state/command/join/pause/time/failure evidence,not bootstrap/orbital/human acceptance",
              "controller": identity(__file__), "scope": "selected-ground-tube-riser", "native_version": 434,
              "accepted": True, "receipts": selected, "native_cases": cases, "native_assertions": assertions,
              "comparison": "Every native gameplay owner retained;125 exact whole peer chunks plus all195 gameplay settings;only seven declared native NoNetworkSync unit preferences local;pure/cross-build pairs cover all126chunks.",
              "dated_proof": "Each child binds its actual historical executable/source/config/content/input identities. Later observer additions do not replace or reopen those closed invocations.",
              "remaining": ["master11.4 genuine normal-newgame bootstrap", "master11.6 representative/max-map performance",
                            "master11.7 human art acceptance", "whole mixed orbital/direct-processing/vent-reuse/date scenarios",
                            "target whole-plan closure and demo acceptance/fixture cleanup"]}
    target = (ROOT / output).resolve()
    assert target.is_relative_to(ROOT / "logs") and not target.exists()
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"verification": identity(target), "native_cases": cases, "native_assertions": assertions}), flush=True)
    return result


def main():
    """Publish only a complete exact selected network checkpoint."""
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    publish(args.output)


if __name__ == "__main__":
    main()
