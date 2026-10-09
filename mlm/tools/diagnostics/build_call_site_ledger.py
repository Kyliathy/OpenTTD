"""Fill the r2mlm3 call-site ledger from the current sources.

Run from the repository root. Rewrites the ledger table in the plan.
@see docs/zz-reach2/upgrades/2026-10/r2mlm3-layer-call-site-migration.md
"""

import os
import re

PLAN = "docs/zz-reach2/upgrades/2026-10/r2mlm3-layer-call-site-migration.md"
SKIP_EXACT = {"src/map_func.h", "src/map.cpp", "src/tile_map.h"}
PAT = re.compile(
    r"Map::Size\(\)|Map::Iterate\(\)|Map::SizeY\(\)|Map::MaxY\(\)|Map::LogY\(\)|"
    r"RandomTileSeed|RandomTile\(|Map::WrapToMap|TilePlaneY\(|TileXYL\(|"
    r"IterateLayer\(|ProjectToSurface\(|WrapToLayer\(|PlaneSize\(\)|IsTileInLayer\(|"
    r"ClosestTownFromTile|CalcClosestTownFromTile|GetTownRadiusGroup|_town_kdtree|_station_kdtree"
)
MIGRATED_MARKERS = ("IterateLayer", "TileXYL", "TilePlaneY", "ProjectToSurface", "WrapToLayer", "PlaneSize", "IsTileInLayer")
VIEW_GROUP = {
    "viewport.cpp": "r2mlm5 Groups 2 and 3",
    "smallmap_gui.cpp": "r2mlm5 Group 6",
    "screenshot.cpp": "r2mlm5 Group 7",
    "terraform_gui.cpp": "r2mlm5 Group 3",
}


def classify(path, line):
    """Pick a class, a status and a short note for one source line.

    @param path Source path relative to the repository.
    @param line The source line.
    @return Class, status and note.
    """
    name = os.path.basename(path)
    if name in VIEW_GROUP:
        return "C7", "migrated", f"View code. Migrated by {VIEW_GROUP[name]}."
    if path.endswith("saveload/map_sl.cpp"):
        return "C8", "deferred", "Map chunk. Migrated by r2mlm4 Group 5."
    if "InverseRemapCoords2" in line or "GetSlopePixelZOutsideMap" in line:
        return "C7", "deferred", "Viewport coordinate clamp. Migrated by r2mlm5 Group 3."
    if "Map::Size()" in line and any(token in line for token in (">=", "assert", "CMD_ERROR", "return false", "return {")):
        return "C4", "kept", "Validity bound. Map::Size() covers every layer."

    migrated = any(marker in line for marker in MIGRATED_MARKERS)
    if "ProjectToSurface" in line or "ClosestTownFromTile" in line or "CalcClosestTownFromTile" in line or "GetTownRadiusGroup" in line or "_town_kdtree" in line:
        cls, note = "C5", "Town association. The projection lives in the town query functions."
    elif name in ("script_map.cpp", "script_tilelist.cpp", "script_object.hpp", "newgrf_commons.cpp", "newgrf_house.cpp", "newgrf_airport.cpp", "network_game_info.cpp", "network_admin.cpp", "network_client.cpp", "console_cmds.cpp", "openttd.cpp") or "newgrf_actd.cpp" in path:
        cls, note = "C9", "Scripts, NewGRF and network reports stay on one plane."
    elif "station_kdtree" in path or name in ("tilearea_orthogonal.cpp", "tilearea_spiral.cpp", "tilearea_spiral.h", "tilearea_diagonal.cpp", "tilearea_diagonal.h", "station_gui.cpp", "station_cmd.cpp") or "_station_kdtree" in line:
        cls, note = "C6", "The search stays in the origin tile's layer."
    elif name == "afterload.cpp" and "Map::Iterate" in line:
        cls, note = "C1", "The conversion repairs every stored tile. Old files hold one plane."
    elif name in ("company_sl.cpp", "station_sl.cpp", "town_sl.cpp", "waypoint_sl.cpp", "economy.cpp", "newgrf_railtype.cpp", "newgrf_roadtype.cpp") or (name == "road_cmd.cpp" and "Map::Iterate" in line):
        cls, note = "C1", "Ownership, label or cache walk over every stored tile."
    elif name in ("cheat_gui.cpp", "settings_table.cpp") and "Map::Iterate" in line:
        cls, note = "C1", "A height anywhere in storage blocks the limit change."
    elif "RandomTile" in line:
        cls, note = "C2", "RandomTileSeed already wraps onto the surface."
    elif "Map::Size()" in line and any(token in line for token in (">=", "assert", "CMD_ERROR", "return false", "return {")):
        cls, note = "C4", "Validity bound. Map::Size() covers every layer."
    elif migrated and name in ("tunnel_map.cpp", "industry_cmd.cpp", "object_cmd.cpp", "water_cmd.cpp", "afterload.cpp", "sound.cpp", "aircraft_cmd.cpp", "effectvehicle.cpp", "disaster_vehicle.cpp", "terraform_cmd.cpp"):
        if "TilePlaneY" in line and "TileXYL" not in line and "IterateLayer" not in line:
            cls, note = "C3", "The edge or neighbour is measured inside the origin layer."
        else:
            cls, note = "C2", "Surface content, written explicitly."
    elif migrated:
        cls, note = "C2", "Surface content, written explicitly."
    elif "Map::SizeY" in line or "Map::MaxY" in line or "Map::LogY" in line:
        if path.startswith("src/network") or name in ("console_cmds.cpp", "script_map.cpp", "newgrf_airport.cpp", "openttd.cpp"):
            cls, note = "C9", "SizeY, MaxY and LogY are already one plane."
        else:
            cls, note = "C3", "SizeY, MaxY and LogY are already one plane."
    elif "Map::Size()" in line:
        cls, note = "C4", "Reviewed as a quantity or a storage bound."
    elif "Map::Iterate" in line:
        cls, note = "C1", "Walks every stored tile."
    else:
        cls, note = "C6", "Spatial query."

    status = "migrated" if migrated else "kept"
    return cls, status, note


def collect():
    """Walk src and return ledger rows.

    @return Rows of path, line, apis, class, status and note.
    """
    rows = []
    for dirpath, _, files in os.walk("src"):
        for fn in files:
            if not fn.endswith((".h", ".hpp", ".cpp")):
                continue
            path = os.path.join(dirpath, fn).replace("\\", "/")
            if path.startswith("src/tests/") or path in SKIP_EXACT or os.path.basename(path).startswith("map_layer"):
                continue
            text = open(path, encoding="utf-8", errors="replace").read().splitlines()
            for index, line in enumerate(text, 1):
                # A comment that only names an API is not a call site.
                stripped = line.strip()
                if stripped.startswith("//") or stripped.startswith("*") or stripped.startswith("/*"):
                    continue
                found = PAT.findall(line)
                if not found:
                    continue
                apis = ", ".join(dict.fromkeys(item.rstrip("(") for item in found))
                cls, status, note = classify(path, line)
                rows.append((f"{path}:{index}", apis, cls, status, note))
    rows.sort(key=lambda row: (row[0].split(":")[0], int(row[0].split(":")[1])))
    return rows


def main():
    """Replace the ledger table in the plan."""
    rows = collect()
    counts = {}
    for row in rows:
        counts[row[2]] = counts.get(row[2], 0) + 1
    lines = [
        "| File:line | API | Class | Status | Note |",
        "| --- | --- | --- | --- | --- |",
    ]
    for path, apis, cls, status, note in rows:
        safe = note.replace("|", "/")
        lines.append(f"| {path} | {apis} | {cls} | {status} | {safe} |")
    summary = ", ".join(f"{cls} {counts[cls]}" for cls in sorted(counts))
    lines.append("")
    lines.append(f"Counts: {summary}. Total {len(rows)}.")
    block = "\n".join(lines) + "\n"

    raw = open(PLAN, "rb").read()
    newline = "\r\n" if b"\r\n" in raw else "\n"
    plan = raw.decode("utf-8")
    start = plan.index("| File:line | API | Class | Status | Note |")
    end = plan.index("## Implementation Tasks")
    updated = plan[:start] + block.replace("\n", newline) + newline + plan[end:]
    open(PLAN, "w", encoding="utf-8", newline="").write(updated)
    print(summary, "total", len(rows))


if __name__ == "__main__":
    main()
