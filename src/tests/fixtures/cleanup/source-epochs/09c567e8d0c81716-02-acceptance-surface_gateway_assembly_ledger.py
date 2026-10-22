"""Conserve actual station stock and inputs embodied by genuinely paid native hulls.

@file surface_gateway_assembly_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15BIEQM/15BIEQN/15C.
SVST describes the existing paid Vehicle, never another physical inventory.
The original delivery observer remains strict unless assembly is explicit.
"""
import struct

from space_preview_checks import chunks
from surface_regional_ledger import TABLE


def production(path, center, allow_payload=False, allow_mission=False, allow_core=False, allow_resupply=False):
    """@param path Actual native paid assembly checkpoint. @param center Independently decoded actual center/slot references.
    @param allow_payload Explicit ground payload consumer; default keeps original stored/assembling empty-hold assembly constraints.
    @param allow_mission Explicit first mission consumer; admits only original in-transit rocket/orbital constructor receipts.
    @param allow_core Explicit first-Core consumer; alone permits an actual constructor blueprint order/approach.
    @param allow_resupply Explicit later Core Goods resupply; default still requires empty rocket freight and initial constructor freight bounds.
    @return Existing original Earth Rocket14/Constructor20paid receipts, cross-checked with real Vehicle and issued SPOR generations.
    """
    data = chunks(path)[1]; raw = data[b"SVST"][3:]; count = raw[0]; assert count <= 2 and len(raw) == 1 + 68 * count
    issued = data[b"SPOR"][3:]; next_generation, orders = struct.unpack_from(">QB", issued)
    assert orders == count
    generations = {}
    if allow_core:
        from surface_gateway_core_ledger import orders as core_orders
        generations = core_orders(path)["generations"]
    else:
        assert len(issued) == 9 + 23 * count
        for index in range(count):
            offset = 9 + 23 * index; vehicle, generation = struct.unpack_from(">IQ", issued, offset)
            assert issued[offset + 12:offset + 23] == bytes(11) and 0 < generation < next_generation
            generations[vehicle] = generation
    vehicles = TABLE(path, b"VEHS"); result = []
    # Each native receipt belongs to one actual paid body and one issued
    # generation. Empty copied records or unrelated construction jobs cannot
    # justify subtracting any component from the original station stock.
    for index in range(count):
        values = struct.unpack_from(">IQBHQHQIHHHIBBHQQB", raw, 1 + index * 68)
        vehicle, generation, placement, home, home_generation, origin, origin_generation, tile, remaining, content, model, capital, paused, deployed, factory, factory_generation, serial, replacement = values
        assert generations[vehicle] == generation and model in (14, 20)
        assert placement in ((0, 2, 4, 5) if allow_mission else (1, 2, 4) if allow_payload else (1, 2))
        assert (home, home_generation) == (origin, origin_generation) == (center["center"], center["generation"])
        assert capital == (400000 if model == 14 else 100000) and content == 3 and paused == replacement == 0
        assert deployed == (int(model == 20 and placement == 0) if allow_mission else 0)
        assert factory == 0xFFFF and factory_generation == serial == 0 and tile in center["cells"]
        assert remaining in range(1, 149) if placement == 1 else remaining == 0
        row = vehicles[vehicle]; assert row["type"] == 6 and row["space[0].common[0].owner"] == 0
        # Native resale value may depreciate through real days. The immutable
        # SVSTcapital receipt, rather than changing resale value, proves payment.
        assert 0 <= row["space[0].common[0].value"] <= capital
        if not allow_payload and not allow_mission:  # Only explicit loaded custody may retain real freight in the paid native body.
            assert not row["space[0].common[0].cargo.packets"]
        reference = (vehicle, generation)
        if placement == 1:
            assert tuple(center["workshop"]) == reference
        elif model == 14 and placement == 2:  # The recovered/stored rocket alone owns its queued pad admission.
            assert [tuple(item) for item in center.get("queue", [])] == [reference]
            assert tuple(center["pad"]) == (0xFFFFF, 0)
        elif model == 14:
            assert allow_mission and placement == 5 and tuple(center["pad"]) == reference and not center.get("queue")
        result.append(dict(vehicle=vehicle, generation=generation, model=model, placement=placement, remaining=remaining, content=content, capital=capital, tile=tile))
        if allow_mission:
            result[-1]["deployed"] = bool(deployed)
    assert len({craft["model"] for craft in result}) == len(result)
    assert {key for key, row in vehicles.items() if row["type"] == 6} == generations.keys()
    if not any(craft["placement"] == 1 for craft in result):
        assert tuple(center["workshop"]) == (0xFFFFF, 0)
    if not any(craft["placement"] in (2, 5) and craft["model"] == 14 for craft in result):
        assert tuple(center["pad"]) == (0xFFFFF, 0) and not center.get("queue")
    # No initial fuel, service cargo, passenger, shell or consumed-flight
    # history belongs to this assembly producer. Its constructor manifest may
    # receive only the actual once-issued initial return allowance.
    if allow_payload or allow_mission:  # Explicit payload/mission consumers validate their actual native compartments; the default assembly branch stays empty.
        from surface_gateway_payload_ledger import supplies
        supplies(path, result, allow_mission=allow_mission, allow_resupply=allow_resupply)
        return result
    supply = data[b"SPSU"][3:]; expected = [craft for craft in result if craft["model"] == 20]
    assert supply[0] == len(expected)
    if expected:
        assert len(supply) == 65
        vehicle, generation = struct.unpack_from(">IQ", supply, 1); assert (vehicle, generation) == (expected[0]["vehicle"], expected[0]["generation"])
        assert supply[13:37] == struct.pack(">IQIQ", 0xFFFFF, 0, 0xFFFFF, 0)
        assert int.from_bytes(supply[39:41], "big") == 0 and supply[41] == 2 and supply[42:] == bytes(23)
        assert int.from_bytes(supply[37:39], "big") < 185
    else:
        assert supply == bytes(1)
    return result


def observe(path):
    """@param path Actual native original assembly world. @return Strict physical plus genuinely embodied material/craft/slot accounting."""
    from surface_gateway_material_ledger import observe as material
    return material(path, phase="assembly")
