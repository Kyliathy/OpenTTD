"""Track the genuine first Core blueprint and its exclusive delivered Steel job.

@file surface_gateway_core_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CB.
Native SPSTjob packet lists alone own deliveries; metadata owns no extra stock.
Current440 retains the exact438 SPSTlayout, verified against its live writer.
"""
from pathlib import Path
import runpy
import struct

from space_preview_checks import ROOT, chunks, read_stream

DIRECTORY = Path(__file__).resolve().parent
LAYOUT = runpy.run_path(str(DIRECTORY / "orbital_save_checks-1.py"))["structures"]


def blueprint(path, allow_construction=False):
    """@param path Actual440 native original world. @param allow_construction Explicit later real work/frame/activation consumer; the default stays Steel-only and unworked.
    @return Sole actual paid Core/job and exclusive packet refs, or None before purchase.
    """
    version, raw = read_stream(path); assert version == 440
    # This selects the unchanged SPSTfield layout, not a different source
    # version: no original save header or byte is rewritten to emulate history.
    records, _ = LAYOUT(raw, 438)
    assert records["count"] in (0, 1) and records["history"] == [0, 0, 0, 0]
    if records["count"] == 0:
        assert records["next"] == 1
        return None
    (site, row), = records["rows"].items()
    assert records["next"] == 2 and row["generation"] == 1
    assert row["owner"] == 0 and row["kind"] == 6 and (row["x"], row["y"]) == (10, 48)
    assert row["capital"] == 700000 and row["content"] == 3
    if not allow_construction:
        assert row["phase"] == 0 and row["module"] is None
        assert row["work"] == row["fraction"] == row["remainder"] == row["tick"] == row["interval"] == 0
        assert row["consumed"] == [0, 0] and row["inputs"][0]["quantity"] in (0, 16) and row["inputs"][1] == dict(quantity=0, references=[])
    else:
        assert row["phase"] in (0, 1) and row["fraction"] == row["remainder"] == row["interval"] == 0
        if row["phase"] == 0:
            assert 0 <= row["work"] < 740 and row["module"] is None and row["consumed"] == [0, 0]
            assert row["inputs"][0]["quantity"] == 16 and row["inputs"][1]["quantity"] in (0, 24)
            assert bool(row["tick"]) == bool(row["work"])
        else:
            assert row["work"] == 740 and row["tick"] > 0 and row["consumed"] == [16, 24]
            assert row["inputs"] == [dict(quantity=0, references=[]), dict(quantity=0, references=[])]
    return dict(site=site, row=row)


def orders(path, allow_construction=False):
    """@param path Actual native Core consumer checkpoint. @param allow_construction Explicit real work/completed-order consumer; earlier defaults retain their initial queue bounds.
    @return Complete issued original queues/approaches and paid craft generations.
    """
    raw = chunks(path)[1][b"SPOR"][3:]; next_generation, count = struct.unpack_from(">QB", raw)
    assert count == 2; offset, rows, generations = 9, {}, {}
    core = blueprint(path, allow_construction=allow_construction)
    # Each saved native queue belongs to one original paid hull. Walk actual
    # variable entries and optional approaches, rejecting unrelated targets or
    # hidden instructions rather than skipping their declared byte extents.
    for _ in range(count):
        vehicle, generation, cursor, started, blocked, timer, work, size = struct.unpack_from(">IQBBBIHB", raw, offset); offset += 22
        assert vehicle in (38, 54) and vehicle not in rows and 0 < generation < next_generation and size <= 1
        entries = [struct.unpack_from(">BBIQ", raw, offset + index * 14) for index in range(size)]; offset += 14 * size
        present = raw[offset]; offset += 1; assert present in (0, 1)
        approach = None
        if present:
            approach = struct.unpack_from(">BIQQBii", raw, offset); offset += 30
        assert work == 0 and started in (0, 1)
        if allow_construction:
            assert cursor in (0, 1) and blocked in (0, 1, 9, 10, 11)
            if cursor == 1:
                assert blocked == 1 and started == timer == 0
        else:
            assert cursor == 0 and blocked in (0, 9)
        if entries:
            assert vehicle == 38 and core is not None and entries == [(3, 2, core["site"], core["row"]["generation"])]
            if approach is not None:
                assert approach[:3] == (1, core["site"], core["row"]["generation"]) and approach[4] == 0
        else:
            assert started == blocked == timer == 0 and approach is None
        rows[vehicle] = dict(generation=generation, cursor=cursor, started=started, blocked=blocked, timer=timer, work=work, entries=entries, approach=approach)
        generations[vehicle] = generation
    assert offset == len(raw)
    return dict(next=next_generation, generations=generations, queues=rows)


def observe(path):
    """@param path Actual native original Core world. @return Complete sole-owner cargo/flight/Core/job/order and actual company books."""
    from surface_gateway_mission_ledger import observe as mission
    result = mission(path, allow_core=True); result["space_orders"] = orders(path)
    assert result["flight"]["phase"] == 0 and result["flight"]["completed"] == 1 and result["fuel_burned"] == 15
    return result
