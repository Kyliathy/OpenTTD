"""Verify only the precise native capsule publication metadata migration in original Cistern source files.

@file cistern_calendar_files.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group15DCBA.
@see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
This reader never changes a save, a registry, world time, availability or any native quantity.
"""
from datetime import date
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/tests/multilayer/utilities"))
from geothermal_tube_work_files import compare_chunks, world_chunks, chunks, read_stream
from geology_generation_fixture import Reader

YEARS = {9: 1970, 10: 1975, 11: 1980, 12: 1985, 13: 2005}
FIELDS = (("intro_date", 5, 4), ("age", 5, 4), ("reliability", 4, 2), ("reliability_spd_dec", 4, 2),
          ("reliability_start", 4, 2), ("reliability_max", 4, 2), ("reliability_final", 4, 2),
          ("duration_phase_1", 4, 2), ("duration_phase_2", 4, 2), ("duration_phase_3", 4, 2),
          ("flags", 2, 1), ("preview_asked", 4, 2), ("preview_company", 2, 1), ("preview_wait", 2, 1),
          ("company_avail", 4, 2), ("company_hidden", 4, 2), ("name", 26, 0))


def engine_rows(path):
    """Read the exact current native engine table schema and lossless complete field byte ranges.

    @param path Original or ordinarily loaded actual native434 world.
    @return Exact ENGN table, decoded fields and their raw byte ranges; any schema/transport drift fails explicitly.
    """
    table = next(row for row in chunks(read_stream(path)[1]) if row["name"] == b"ENGN")
    assert table["kind"] == 3
    header = Reader(table["rows"][0]); descriptors = []
    # Native schema names/types, rather than guessed offsets, authorize the
    # narrow field comparison. Any additional or reordered saved owner must be
    # reviewed instead of being silently omitted by this compatibility gate.
    while True:
        kind = header.byte()
        if kind == 0:
            break
        descriptors.append((header.take(header.gamma()).decode("ascii"), kind))
    assert descriptors == [(name, kind) for name, kind, width in FIELDS] and header.pos == len(table["rows"][0])
    decoded = []
    # Every complete original row participates, including its variable name
    # and all historical reliability/preview/preferences fields. Retaining raw
    # ranges prevents an unrelated changed engine field from being ignored.
    for raw in table["rows"][1:]:
        fields, ranges = {}, {}
        if raw:
            reader = Reader(raw)
            for name, kind, width in FIELDS:
                start = reader.pos
                if width:
                    fields[name] = int.from_bytes(reader.take(width), "big", signed=kind == 5)
                else:
                    fields[name] = reader.take(reader.gamma())
                ranges[name] = (start, reader.pos)
            assert reader.pos == len(raw)
        decoded.append((fields, ranges))
    return table, decoded


def compare_capsule_calendar(original, loaded, anchors, year):
    """Require all original world owners exact except five explicitly bounded canonical native capsule publication fields.

    @param original Exact immutable pre-fix original chemical-source world, without reconstruction.
    @param loaded Its actual normal application resave after public engine restoration.
    @param anchors Five native model/engine/canonical-date/flags/availability observations after ordinary loading.
    @param year Actual original saved calendar year, used only to verify date-valid publication.
    @return Precise ENGN migration and unchanged owner evidence; no bytes are rewritten or normalized in either input.
    """
    before, after = world_chunks(original), world_chunks(loaded)
    assert before.keys() == after.keys() and len(before) == 126
    assert compare_chunks(original, loaded, before.keys() - {b"ENGN"}) == 125
    old_table, old_rows = engine_rows(original); new_table, new_rows = engine_rows(loaded)
    assert old_table["rows"][0] == new_table["rows"][0] and len(old_rows) == len(new_rows)
    models = {row["model"]: row for row in anchors}
    assert len(anchors) == len(models) == 5 and models.keys() == YEARS.keys()
    engines = {row["engine"]: row for row in anchors}; assert len(engines) == 5
    changes = []
    # Preserve complete ordinary engine rows and every unrelated field in all
    # five originals. Only the exact catalogue date and a formerly pending,
    # now-date-valid native public introduction may differ after normal load.
    for index, (old, new) in enumerate(zip(old_rows, new_rows)):
        raw_old, raw_new = old_table["rows"][index + 1], new_table["rows"][index + 1]
        if index not in engines:
            assert raw_old == raw_new, index
            continue
        anchor = engines[index]; model = anchor["model"]; canonical = date(YEARS[model], 1, 1).toordinal() + 365
        assert anchor["intro_date"] == canonical and old[0] and new[0]
        expected = bytearray(raw_old)
        values = {"intro_date": canonical, "flags": old[0]["flags"], "company_avail": old[0]["company_avail"]}
        # EngineFlag Available is bit0 and ExclusivePreview bit1. Native
        # originals have no exclusive preview; a future model never acquires
        # availability just because an older source file was loaded today.
        if YEARS[model] <= year and not (values["flags"] & 1):
            values["flags"] = (values["flags"] & ~2) | 1
            values["company_avail"] = 0xFFFF
        for name, value in values.items():
            start, end = old[1][name]; assert old[1][name] == new[1][name]
            expected[start:end] = value.to_bytes(end - start, "big", signed=name == "intro_date")
        assert bytes(expected) == raw_new, (model, index)
        assert new[0]["flags"] == anchor["flags"] and new[0]["company_avail"] == anchor["company_avail"]
        if raw_old != raw_new:
            changes.append({"model": model, "engine": index,
                            "before": {key: old[0][key] for key in values}, "after": values,
                            "all_other_engine_bytes_exact": True})
    assert set(engines) <= set(range(len(old_rows)))
    return {"unchanged_complete_chunks": 125, "engine_rows": len(old_rows), "changed_capsule_rows": changes,
            "ordinary_engines_exact": True, "all_other_native_engine_fields_exact": True}
