"""Characterize the real original liquid carrier before the1980 Cistern source.

@file cistern_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The empty transport world and native carrier prerequisite earn no Salt, Oil, Chemicals or useful freight claim.
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

SOURCES = ("src/tests/tube_cistern_source.cpp", "src/tests/tube_cistern_source_fixture.h", "src/tests/CMakeLists.txt", "src/tests/excavation_fixture.h",
           "src/tests/layer_save_fixture.h", "src/underground_vehicle.cpp", "src/newgrf.cpp", "src/engine.cpp",
           "src/multilayer_vehicle_catalogue.cpp", "src/multilayer_recipe.cpp", "media/multilayer/build_content.py")


def main():
    """Require actual ordinary content startup and native original tanker purchase/refit before source development.

    @return None; missing liquid semantics, a rejected native carrier or a failed ordinary load prevents this prerequisite report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    args = parser.parse_args()
    context = current_run_context("tubes", "cistern-source")
    context.prepare_runtime()
    output, seed_proof = native(context, args.config, "[tube-cistern-source-template]", "empty-world")
    match = re.search(r"CISTERN_SEED year=1980 vehicles=0 industries=0 packets=0 useful_source=false path=([^\r\n]+)", output)
    assert match is not None
    seed = Path(match[1]); original = identity(seed)
    assert len(world_chunks(seed)) == 126
    startup = context.runtime_dir / "scripts/game_start.scr"
    startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15DB; original carrier prerequisite only.\ncistern_source_check\n", encoding="utf-8")
    output, carrier = native(context, args.config, "[.tube-cistern-carrier-native]", "original-carrier",
                             {"OTTD_CISTERN_SOURCE_INPUT": str(seed)})
    match = re.search(r"CISTERN_CARRIER year=1980 original_engine=(\d+) head=(\d+) tanker=(\d+) cargo=(\d+) capacity=(\d+) packets=0 useful_source=false path=([^\r\n]+)", output)
    assert match is not None
    fields = dict(zip(("original_engine", "head", "tanker", "cargo", "capacity"), map(int, match.groups()[:5])))
    assert fields["head"] != fields["tanker"] and fields["capacity"] > 0
    source_match = re.search(r"CISTERN_SOURCES oil_type=(\d+) chemical_type=(\d+) salt_deposit=(\d+) x=(\d+) y=(\d+) useful_source=false", output)
    assert source_match is not None
    source_contracts = dict(zip(("oil_type", "chemical_type", "salt_deposit", "salt_x", "salt_y"), map(int, source_match.groups())))
    saved = Path(match[6]); assert len(world_chunks(saved)) == 126
    assert identity(seed) == original
    seed_proof["output"] = original
    carrier.update(input=original, output=identity(saved))
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15DB; actual native liquid-carrier prerequisite only; source production/transport gate remains pending",
        "configuration": args.config, "focus": "carrier-contract", "native": [seed_proof, carrier], "carrier": fields, "source_contracts": source_contracts,
        "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES],
        "fixture_classification": "An empty prepared world initializes ordinary native application content. Explicit author funds purchase a real original tanker and native locomotive without a cargo grant. The tanker refit mask/liquid properties come from the delivered package. No useful source, physical portal crossing or freight route is claimed yet.",
        "remaining": "15DB must earn finite Salt/original Oil, drive physical inputs to CHEM, retain actual output/unused inputs and earn complete empty returns in both builds before dependent Cistern routing.",
    }
    path = context.output("reports", f"cistern-carrier-{args.config}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(path, flush=True)


if __name__ == "__main__":
    main()
