"""Reconcile the original paid deployment flight with actual cargo and burned fuel.

@file surface_gateway_mission_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15C.
SPFL is the same rocket's saved transition, not a second vehicle or inventory.
Only the explicit first-mission consumer admits flight and deployed placement.
"""
import struct

from space_preview_checks import chunks
from surface_gateway_material_ledger import observe as material


def observe(path, allow_core=False):
    """@param path Actual first mission checkpoint. @param allow_core Explicit later original Core consumer; default still refuses every blueprint/job.
    @return Exact sole-owner physical/embodied/burned quantities and native saved flight history.
    """
    result = material(path, phase="core" if allow_core else "mission"); data = chunks(path)[1]
    if not allow_core:
        assert data[b"SPST"][3:] == struct.pack(">QHQQQQ", 1, 0, 0, 0, 0, 0), "First deployment must not create a blueprint or construction history"
    rocket, = [craft for craft in result["paid_craft"] if craft["model"] == 14]
    constructor, = [craft for craft in result["paid_craft"] if craft["model"] == 20]
    assert (rocket["vehicle"], rocket["generation"]) == (54, 1)
    assert (constructor["vehicle"], constructor["generation"]) == (38, 2)
    assert constructor["placement"] in (0, 4)
    freight = [packet for packet in result["cohort"] if tuple(packet["owner"]) == ("vehicle", constructor["vehicle"])]
    reserved = result["core"]["row"]["inputs"][0]["quantity"] if allow_core and result["core"] is not None else 0
    assert sum(packet["count"] for packet in freight) + reserved == 16 and all(packet["label"] == "STEL" for packet in freight)
    raw = data[b"SPFL"][3:]; assert raw[0] in (0, 1) and len(raw) == 1 + 84 * raw[0]
    manifest = result["manifests"][rocket["vehicle"]]; burned = manifest["fuel_burned"]
    assert result["totals"] == {"STEL": 20, "GOOD": 26, "PROP": 42 - burned}
    flight = None
    if raw[0]:
        values = struct.unpack_from(">IQBBBHQHQBIQiiHBBQQQ", raw, 1)
        vehicle, generation, phase, blocked, layer, source, source_generation, recovery, recovery_generation, kind, target, target_generation, x, y, remaining, remainder, flags, started, completed, canceled = values
        assert (vehicle, generation) == (54, 1) and blocked == 0 and started == 1 and canceled == 0 and remainder < 74
        assert phase in range(6) and completed == int(phase == 0)
        if phase == 0:
            assert (source, source_generation, recovery, recovery_generation) == (65535, 0, 65535, 0)
            assert kind == 0 and target == 0xFFFFFFFF and target_generation == x == y == remaining == flags == 0
            assert rocket["placement"] == 2 and constructor["placement"] == 0 and layer == 0 and burned == 15
        else:
            assert (source, source_generation) == (recovery, recovery_generation) == (21, 1)
            assert (kind, target, target_generation) == (3, 21, 1) and rocket["placement"] == 5
            assert layer == (2 if phase in (3, 4) else 0)
            expected_burn = 0 if phase == 1 else 10 if phase in (2, 3) else 15
            assert burned == expected_burn and flags == (0 if phase == 1 else 1 if phase in (2, 3) else 7)
            assert constructor["placement"] == (4 if phase in (1, 2, 3) else 0)
        flight = dict(vehicle=vehicle, generation=generation, phase=phase, blocked=blocked, layer=layer,
                      source=(source, source_generation), recovery=(recovery, recovery_generation),
                      destination=(kind, target, target_generation), departure=(x, y), remaining=remaining,
                      pad_remainder=remainder, flags=flags, started=started, completed=completed, canceled=canceled)
    else:
        assert rocket["placement"] == 2 and constructor["placement"] == 4 and burned == 0
    result["flight"] = flight; result["fuel_burned"] = burned
    return result
