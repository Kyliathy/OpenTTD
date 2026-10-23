"""Produce an actual1985 finite Deep RARE stock through ordinary native source funding and work.

@file shield_source_checks.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15EB.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
The real sealed carrier stays stopped/empty; undispatched source stock is not useful delivery or Electronics manufacture.
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

SOURCES = ("src/tests/tube_shield_source.cpp", "src/tests/CMakeLists.txt", "src/tests/excavation_fixture.h", "src/tests/layer_save_fixture.h",
           "src/tests/tube_useful_tick_fixture.h", "src/industry_cmd.cpp", "src/layer_industry_definitions.cpp", "src/layer_industry_mine.cpp", "src/layer_industry_processor.cpp",
           "src/multilayer_vehicle_catalogue.cpp", "src/multilayer_recipe.cpp", "src/underground_vehicle.cpp", "src/transport_tube_engine.cpp",
           "src/engine.cpp", "src/newgrf.cpp", "src/economy.cpp", "src/multilayer_industry.h", "media/multilayer/build_content.py")


def main():
    """Load a newly authored1985 empty native world, then fund real owners and earn20 conserved finite RARE.

    @return None; wrong dates/definitions, rejected native purchases, changed originals or missing finite conservation prevent a source report.
    """
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--config", choices=("Debug", "RelWithDebInfo"), required=True)
    args = parser.parse_args(); context = current_run_context("tubes", "shield-source"); context.prepare_runtime()
    output, author = native(context, args.config, "[tube-shield-source-template]", "empty-world")
    match = re.search(r"SHIELD_SEED year=1985 vehicles=0 industries=0 packets=0 useful_source=false path=([^\r\n]+)", output)
    assert match is not None
    seed = Path(match[1]); original = identity(seed); assert len(world_chunks(seed)) == 126
    startup = context.runtime_dir / "scripts/game_start.scr"; startup.parent.mkdir(parents=True, exist_ok=True)
    startup.write_text("# @see r2mlvs17-underground-transport-tubes.md15EB; actual native finite RARE source only.\nshield_source_stock_check\n", encoding="utf-8")
    output, stock = native(context, args.config, "[.tube-shield-source-stock-native]", "source-stock", {"OTTD_SHIELD_SOURCE_INPUT": str(seed)})
    names = ("tick", "opportunities", "units", "reserve_before", "reserve", "extracted", "dispatched", "cash", "loan", "income")
    pattern = "SHIELD_STOCK year=1985 " + " ".join(name + r"=(\d+)" for name in names)
    pattern += r" parts=([\d,]+) industries=([\d,]+) deposit=(\d+) home=(\d+) cargo=(\d+) pickup_x=(\d+) pickup_y=(\d+) packets=0 path=([^\r\n]+)"
    match = re.search(pattern, output); assert match is not None
    result = dict(zip(names, map(int, match.groups()[:10])))
    result.update(parts=list(map(int, match[11].split(","))), industries=list(map(int, match[12].split(","))),
                  deposit=int(match[13]), home=int(match[14]), cargo=int(match[15]), pickup_x=int(match[16]), pickup_y=int(match[17]), year=1985)
    assert len(result["parts"]) == len(set(result["parts"])) == len(result["industries"]) == len(set(result["industries"])) == 2
    assert result["tick"] == result["opportunities"] and 0 < result["opportunities"] < 2048
    assert result["units"] == result["extracted"] == result["reserve_before"] - result["reserve"] == 20
    assert result["dispatched"] == result["loan"] == result["income"] == 0 and result["cash"] > 0
    saved = Path(match[18]); assert len(world_chunks(saved)) == 126 and identity(seed) == original
    author.update(output=original); stock.update(input=original, output=identity(saved))
    report = {
        "_documentation": "r2mlvs17-underground-transport-tubes.md15EB.1; actual native1985 finite RARE stock/real stopped sealed carrier/empty native ELEC receiver; ordinary source-file/useful route acceptance remains pending",
        "configuration": args.config, "focus": "stock-producer", "native": [author, stock], "source": result,
        "controller": identity(__file__), "binary": identity(ROOT / f"build/{args.config}/openttd_test.exe"),
        "sources": [identity(ROOT / name) for name in SOURCES],
        "fixture_classification": "New1985 author date/funding precedes all purchases/work. Ordinary application loading owns original native cargo/engine/industry definitions and prices. Commands fund the actual finite Deep RARE mine, Surface ELEC machinery and real Deep depot/4+6 sealed carrier; completed machinery/prepared floor remain disclosed boundaries. Complete native clocks/vehicles plus one ordinary industry dispatcher earn20 original reserve-backed RARE into its native industry buffer. Carrier remains whole stopped/empty; no station/packet/service/rating grant, dispatch, useful intake/payment, RFMT/Electronics output, loaded date/cash repair or TBM/bootstrap claim.",
    }
    path = context.output("reports", f"shield-source-stock-{args.config}.json")
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8"); print(path, flush=True)


if __name__ == "__main__":
    main()
