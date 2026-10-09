"""Write a NewGRF that adds one road type, "Test road", so road conversion can be tried without downloading a set.

Written for docs/zz-reach2/upgrades/2026-10/map-layers/r2rvm2-review-map-2.md (task 17.8, scenario S7: converting the road
of one elevator end needs a second road type). NewGRF loading is described in
docs/zz-reach2/architecture/newgrf/archi-newgrf.md; the actions written here are read by
src/newgrf/newgrf_act8.cpp, src/newgrf/newgrf_act4.cpp and src/newgrf/newgrf_act0_roadtypes.cpp.

Usage: python -B src/tests/multilayer/support/road_type_fixture.py [--out PATH]

The default output is build/newgrf/test_road_type.grf, where a game started with a -c config in build/ finds it.
The file is GRF container version 1 and holds pseudo sprites only, so no GRF tool is needed:
- Action 8 names the set;
- Action 4 defines the three strings of the new road type;
- an Action 0 creates road type TSRD, which starts as a copy of the default road (AllocateRoadType), with its
  own name and a 60 km/h limit;
- a second Action 0 makes the default road introduce TSRD and lets road vehicles drive on it. Without it no
  company could build TSRD, because a road type is available only through the vehicles' road types.
TSRD has no sprites of its own, so it looks like the default road; the land area information window and the
road construction menu tell the two apart.
"""

import argparse
import pathlib
import struct
import sys

#Unique ID of this set. A first byte of 0xFF is reserved for OpenTTD's own files.
GRFID = b"MLR\x01"
#NewGRF version 8, the current one; version 7 or later reads word string IDs in Action 4.
GRF_VERSION = 8
#GrfSpecFeature::RoadTypes in src/newgrf.h.
FEATURE_ROAD_TYPES = 0x12
LABEL_TEST = b"TSRD"
LABEL_ROAD = b"ROAD"
#Generic string IDs of a NewGRF start at 0xD000 (newgrf_act4.cpp, default case).
STRING_BASE = 0xD000
#Language 0x7F is "any language"; bit 7 marks the IDs as generic words.
LANGUAGE_ANY_GENERIC = 0xFF
#Road type speed limits count half km/h (road_gui.cpp divides max_speed by 2).
SPEED_LIMIT = 120

#Road type properties (newgrf_act0_roadtypes.cpp).
PROP_LABEL = 0x08
PROP_TOOLBAR_CAPTION = 0x09
PROP_MENU_TEXT = 0x0A
PROP_POWERED_LIST = 0x0F
PROP_SPEED_LIMIT = 0x14
PROP_INTRODUCED_LIST = 0x19
PROP_NAME = 0x1B

#Strings in Action 4 order; the property that uses each one gets STRING_BASE + its index.
STRINGS = [
    (PROP_NAME, "Test road"),
    (PROP_TOOLBAR_CAPTION, "Test Road Construction"),
    (PROP_MENU_TEXT, "Test road construction"),
]


def word(value):
    """Little-endian 16-bit value.

    @param value Number from 0 to 0xFFFF.
    @return Two bytes.
    """
    return struct.pack("<H", value)


def text(value):
    """Zero-terminated string. ASCII reads the same in the TTD and the UTF-8 encoding.

    @param value ASCII text.
    @return The bytes, ending in 0.
    """
    return value.encode("ascii") + b"\x00"


def action8():
    """Action 8: version, GRFID, name and description of the set.

    @return Pseudo sprite bytes.
    """
    return (bytes([0x08, GRF_VERSION]) + GRFID + text("Test road type (multi-layered map scenarios)")
            + text("One extra road type, TSRD, drawn like the default road, for road conversion tests."))


def action4():
    """Action 4: the generic strings of the new road type.

    @return Pseudo sprite bytes.
    """
    data = bytes([0x04, FEATURE_ROAD_TYPES, LANGUAGE_ANY_GENERIC, len(STRINGS)]) + word(STRING_BASE)
    for _, value in STRINGS:
        data += text(value)
    return data


def action0(local_id, properties):
    """Action 0 for one road type.

    @param local_id Local ID of the road type inside this set; the label property maps it to a real road type.
    @param properties List of (property, value bytes). The label must come first, because the reservation stage
        allocates or finds the road type from it before any other property is applied.
    @return Pseudo sprite bytes.
    """
    #The local ID is an extended byte; IDs below 0xFF are one byte.
    data = bytes([0x00, FEATURE_ROAD_TYPES, len(properties), 1, local_id])
    for prop, value in properties:
        data += bytes([prop]) + value
    return data


def build_sprites():
    """All pseudo sprites of the file, after the leading sprite count.

    @return List of pseudo sprite bytes.
    """
    test_properties = [(PROP_LABEL, LABEL_TEST)]
    for index, (prop, _) in enumerate(STRINGS):
        test_properties.append((prop, word(STRING_BASE + index)))
    test_properties.append((PROP_SPEED_LIMIT, word(SPEED_LIMIT)))

    #The label list properties add to the existing lists, so the default road keeps its own entries.
    road_properties = [
        (PROP_LABEL, LABEL_ROAD),
        (PROP_POWERED_LIST, bytes([1]) + LABEL_TEST),
        (PROP_INTRODUCED_LIST, bytes([1]) + LABEL_TEST),
    ]
    return [action8(), action4(), action0(0, test_properties), action0(1, road_properties)]


def container_v1(sprites):
    """GRF container version 1: a size word, the type byte 0xFF, then the data of each pseudo sprite, and a 0 word.

    @param sprites Pseudo sprite bytes, without the leading sprite count.
    @return The whole file.
    """
    out = bytearray()
    #The first pseudo sprite holds the number of sprites that follow; the loader checks for size 4 and type 0xFF.
    for data in [struct.pack("<I", len(sprites))] + sprites:
        if len(data) > 0xFFFF:
            raise ValueError("a container version 1 sprite must be shorter than 64 KiB")
        out += word(len(data)) + b"\xFF" + data
    out += word(0)
    return bytes(out)


def main():
    """Write the file and print where it went.

    @return Process exit code.
    """
    root = pathlib.Path(__file__).resolve().parents[4]
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--out", type=pathlib.Path, default=root / "build" / "newgrf" / "test_road_type.grf",
                        help="output .grf file (default: build/newgrf/test_road_type.grf)")
    args = parser.parse_args()

    data = container_v1(build_sprites())
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(data)
    print(f"{args.out}: {len(data)} bytes, GRFID {GRFID.hex().upper()}, road type {LABEL_TEST.decode()}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
