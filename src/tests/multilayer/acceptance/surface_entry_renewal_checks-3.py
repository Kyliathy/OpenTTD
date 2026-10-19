"""Restore genuinely renewed freight and compare identical original annual work.

@file surface_entry_renewal_checks-3.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15BIEE.3.
Ordinary applications retain every native owner. The replay branch supplies no
cash or materials to the original selected world, which remains the continuation.
"""
import argparse
import json
from pathlib import Path
import runpy

from space_preview_checks import ROOT, chunks, identity
from surface_propellant_ledger import observe

RELOAD = runpy.run_path(str(Path(__file__).with_name("surface_entry_fuel_checks-12.py")))
DRIVER = runpy.run_path(str(Path(__file__).with_name("surface_entry_renewal_checks-1.py")))["execute"]


def execute():
    """@return Actual both-build ordinary restore and exact annual original/resaved continuation evidence."""
    foundation_path = ROOT / "logs/surface-entry-freight-refresh-purchase-verification.json"
    foundation = json.loads(foundation_path.read_text(encoding="utf-8")); assert foundation["complete"] and foundation["actual_renewal_complete"]
    processes, physical, summaries, rows_by_build = set(), [], [], []
    for entry in foundation["configurations"]:
        configuration = entry["configuration"]; source = ROOT / entry["output"]["path"]
        assert identity(source) == entry["output"]
        restored, restore = RELOAD["ORDINARY"](configuration, source, "surface-freight-refresh-restore-" + configuration.lower())
        RELOAD["REVISION"](source, restored)
        original, resaved = [chunks(path)[1] for path in (source, restored)]
        pause = RELOAD["restore_pause"](source, restored)
        assert original.keys() == resaved.keys() and all(original[key] == resaved[key] for key in original if key not in (b"GLOG", b"DATE"))
        assert observe(source) == observe(restored)
        restore_path = ROOT / f"logs/surface-entry-freight-refresh-restored-{configuration}.json"
        RELOAD["publish"](restore_path, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="restore",
                                            operation=restore, original_pause=pause))
        receipts, branches, rows = [], [], []
        # Original and ordinary branches earn exactly the same27000tick native
        # work. Compare all gameplay bytes, RNG and financial ledgers; only the
        # first branch may advance the selected genuine capital lineage.
        for predecessor, suffix in ((ROOT / entry["receipt"]["path"], "original"), (restore_path, "ordinary")):
            operation = DRIVER(configuration, "earn", predecessor)
            path = ROOT / f"logs/surface-entry-freight-refresh-continued-{configuration}-{suffix}.json"
            RELOAD["publish"](path, dict(_documentation=__doc__, complete=True, configuration=configuration, phase="earn", operation=operation))
            receipts.append(identity(path)); branches.append(chunks(ROOT / operation["output"]["path"])[1]); rows.append(operation["rows"])
            processes.update(row["path"] for row in operation["native_processes"])
        assert branches[0].keys() == branches[1].keys() and all(branches[0][key] == branches[1][key] for key in branches[0] if key != b"GLOG")
        assert rows[0] == rows[1]
        normalized = dict(resaved); normalized[b"DATE"] = original[b"DATE"]
        physical.append([normalized] + branches); rows_by_build.append(rows)
        processes.update(row["path"] for row in restore["native_processes"])
        selected = json.loads((ROOT / receipts[0]["path"]).read_text(encoding="utf-8"))["operation"]
        site = json.loads((ROOT / "logs/surface-entry-launch-site-prepared-verification.json").read_text(encoding="utf-8"))["site"]; final_map = branches[0]
        assert final_map[b"MAPT"][3 + site["canal"]] >> 4 == 6
        assert final_map[b"MAPO"][3 + site["canal"]] & 31 == 0 and (final_map[b"MAPO"][3 + site["canal"]] >> 5) & 3 == 1
        x, y = site["anchor"] % 64, site["anchor"] // 64
        width = 4 if site["rotation"] % 2 == 0 else 3; height = 3 if width == 4 else 4
        # Native work may grow real trees, but this prepared complete land
        # footprint must still satisfy its flat clear/tree geometry. Water
        # retains its original owned Canal class, without a fabricated hub.
        assert all(final_map[b"MAPT"][3 + (y + dy) * 64 + x + dx] >> 4 in (0, 4) for dy in range(height) for dx in range(width))
        assert len({final_map[b"MAPH"][3 + (y + dy) * 64 + x + dx] for dy in range(height + 1) for dx in range(width + 1)}) == 1
        summaries.append(dict(configuration=configuration, restore=identity(restore_path), continuations=receipts, output=selected["output"],
                              company=selected["final"]["company"], plant=selected["final"]["plant"], fuel=selected["final"]["propellant_truck"],
                              original_pause=pause, renewed_income=selected["rows"]["earned"], delivered=selected["rows"]["delivered"]))
        print(configuration, "ordinary renewed-freight restore and exact27000tick continuation passed", flush=True)
    assert rows_by_build[0] == rows_by_build[1]
    for first, second in zip(*physical, strict=True):
        assert first.keys() == second.keys() and all(first[key] == second[key] for key in first if key != b"GLOG")
    assert len(processes) == 6
    return dict(_documentation=__doc__, complete=True, actual_renewal_complete=True, prepared_site=True, foundation=identity(foundation_path), configurations=summaries,
                native_processes=6, cross_build_gameplay_comparisons=360, exact_native_replay_gameplay_comparisons=240,
                actual_launch_complete=False, sources=[identity(Path(__file__)), identity(Path(__file__).with_name("surface_entry_renewal_checks-1.py")),
                                                       identity(Path(__file__).with_name("surface_propellant_ledger.py")), identity(ROOT / "src/tests/surface_entry_launch.cpp")])


def main():
    """Retain useful original renewed freight while keeping physical supplies and full launch funding open."""
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument("--output", type=Path, required=True)
    output = (ROOT / parser.parse_args().output).resolve(); RELOAD["publish"](output, execute())
    print(output.relative_to(ROOT).as_posix(), flush=True)


if __name__ == "__main__":
    main()
