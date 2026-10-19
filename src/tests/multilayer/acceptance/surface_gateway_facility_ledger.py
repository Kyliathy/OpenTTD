"""Decode the actual paid Surface launch center and its exclusive freight link.

@file surface_gateway_facility_ledger.py
@see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups15BIEQK/15BIEQM.
This initial facility contract keeps all launch stock on original vehicles.
It cannot admit later material transfers or paid hull consumption implicitly.
"""
import struct

from space_preview_checks import chunks
from surface_regional_ledger import TABLE, observe as observe_regional


def facility(path, allow_craft=False):
    """@param path Complete actual native save. @param allow_craft Explicit paid-assembly consumer; default requires original empty slots.
    @return Independently decoded native center/hub and exact slot references, or absent original owners.
    """
    data = chunks(path)[1]; centers, hubs, slots = [data[key][3:] for key in (b"LCTR", b"LCHB", b"LCOP")]
    next_center, count = struct.unpack(">QH", centers[:10]); next_member, hub_count = struct.unpack(">QH", hubs[:10])
    if count == 0:
        assert len(centers) == len(hubs) == 10 and hub_count == 0 and slots == bytes(1)
        return dict(center=None, next_center=next_center, next_member=next_member)
    assert count == hub_count == 1 and len(centers) == 25 and len(hubs) == 31 and len(slots) >= 58
    station, generation, anchor, rotation = struct.unpack(">HQIB", centers[10:])
    center, center_generation, members, member, member_generation = struct.unpack(">HQBHQ", hubs[10:])
    assert (center, center_generation) == (station, generation) and members == 1 and member == 0
    assert 0 < generation < next_center and 0 < member_generation < next_member
    assert anchor == 2890 and rotation == 1 and station not in (0, 20)
    assert slots[0] == 1
    native = struct.unpack(">HQIQIQHQIQB", slots[1:58])
    assert len(slots) == 58 + 12 * native[10]
    assert native[:2] == (station, generation)
    # Native VehicleID reserves its20bit pool sentinel even though transport
    # writes four bytes. Empty slots must retain that exact native identity.
    empty = native[2:] == (0xFFFFF, 0, 0xFFFFF, 0, 0xFFFF, 0, 0xFFFFF, 0, 0)
    if not allow_craft:
        assert empty and len(slots) == 58
    else:
        # Paid assembly permits only its workshop and active rocket pad.
        # A completed rocket remains in the normal one-entry pad queue until
        # actual launch preparation; service/pods and extra queued hulls remain excluded.
        assert native[4:8] == (0xFFFFF, 0, 0xFFFF, 0) and native[10] <= 1
    row = TABLE(path, b"STNN")[station]
    assert row["normal[0].base[0].owner"] == 0 and row["normal[0].base[0].xy"] == anchor
    assert row["facilities"] == row["normal[0].base[0].facilities"] == 1 << 5
    cells = [(45 + dy) * 64 + 10 + dx for dy in range(4) for dx in range(3)]
    # All twelve genuine station cells must name this same allocated native
    # station; a sign, joined Village station or painted footprint cannot qualify.
    for tile in cells:
        assert data[b"MAPT"][3 + tile] >> 4 == 5 and data[b"MAPO"][3 + tile] & 31 == 0
        assert int.from_bytes(data[b"MAP2"][3 + 2 * tile:5 + 2 * tile], "big") == station
    assert data[b"MAPT"][3 + 2826] >> 4 == 6
    result = dict(center=station, generation=generation, anchor=anchor, rotation=rotation, member=member, member_generation=member_generation,
                  next_center=next_center, next_member=next_member, cells=cells, empty_slots=empty)
    if allow_craft:
        result["workshop"] = native[2:4]; result["pad"] = native[8:10]
        if native[10]:
            result["queue"] = [struct.unpack_from(">IQ", slots, 58)]
    return result


def observe(path):
    """@param path Actual complete native facility checkpoint. @return Original strict128unit stock plus native paid center and exclusive hub."""
    result = observe_regional(path); result["launch_facility"] = facility(path)
    return result
