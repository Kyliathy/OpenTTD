# Tunnel and bridge tiles

`TileType::TunnelBridge` is a tunnel mouth or a bridge ramp. The bridge deck itself is not a tile of this type. Tiles under the deck keep their own `TileType`, and `type` bits 3..2 on those tiles record the bridge axis ([archi-landscape.md](archi-landscape.md)). Accessors are split across `src/tunnelbridge_map.h`, `src/bridge_map.h`, and `src/tunnel_map.h`.

## Ramp versus tunnel

| Field | Bits | Accessor |
| --- | --- | --- |
| Bridge ramp | `m5` bit 7 set | `IsBridge` (`src/bridge_map.h` lines 24–27) |
| Tunnel mouth | `m5` bit 7 clear | `IsTunnel` (`src/tunnel_map.h` lines 23–26) |
| Direction | `m5` bits 1..0 | `GetTunnelBridgeDirection`, `GB(m5, 0, 2)` (`src/tunnelbridge_map.h` lines 26–29). `DiagDirection`: `NE = 0`, `SE = 1`, `SW = 2`, `NW = 3` |
| Transport | `m5` bits 3..2 | `GetTunnelBridgeTransportType`, `GB(m5, 2, 2)` (lines 39–42) |
| Rail PBS reservation | `m5` bit 4 | `HasTunnelBridgeReservation` (lines 91–95). Asserts the transport type is rail |
| Snow or desert | `m7` bit 5 | `HasTunnelBridgeSnowOrDesert` (lines 52–55) |

`TransportType` (`src/transport_type.h` lines 17–28) is `Rail`, `Road`, `Water`, `Air`, in that order, so the two-bit field is 0, 1, 2, 3. The comment on lines 18–23 says the values are tied to the bridge and tunnel representation and that the road and rail values must not change. The HTML's 0 = railway, 1 = road, 2 = water matches `Rail`, `Road`, and `Water`. `MakeAqueductBridgeRamp` passes `TransportType::Water` (`src/bridge_map.h` lines 182–184).

`m1` bits 4..0 are the owner. `m1` bit 7 is the docking mark, used for aqueduct ends (`SetDockingTile` allows `TunnelBridge`, `src/water_map.h` lines 367–370). The HTML says the same.

## Bridge type

`GetBridgeType` (`src/bridge_map.h` lines 56–59) is `GB(m6, 2, 4)`, so `m6` bits 5..2, and only on a bridge ramp (`IsBridgeTile`). `MakeBridgeRamp` (lines 128–140) writes that field and clears `m6` bits 7..6. `MAX_BRIDGES` is 13 (`src/bridge.h` line 18), so the legal spec indices are 0–12. The HTML stores the type in the same four bits. Its speed and material table is `BridgeSpec` data (year, length, price, speed live on `BridgeSpec`, `src/bridge.h` lines 27–40), not extra tile bits. This note does not restate those speeds.

`MakeBridgeRamp` also sets `m5` to `1 << 7 | transport << 2 | direction` (line 136), which is the bridge bit, the transport type, and the direction above.

A tunnel does not use the bridge-type field. `MakeRoadTunnel` / `MakeRailTunnel` (`src/tunnel_map.h` lines 51–64 and 74–80) leave `m5` bit 7 clear and clear `m6` bits 7..2.

## Road, tram, and rail on the ramp

`MayHaveRoad` is true when the transport type is road (`src/road_map.cpp` lines 30–31). Then the road accessors apply:

| Field | Bits | Meaning |
| --- | --- | --- |
| Road type | `m4` 5..0 | `GetRoadTypeRoad` |
| Tram type | `m8` 11..6 | `GetRoadTypeTram` |
| Road owner | `m7` 4..0 | `GetRoadOwner` for road. A tunnel/bridge tile is not a normal road tile, so the owner comes from `m7` (`src/road_map.h` lines 244–247) |
| Tram owner | `m3` 7..4 | `GetRoadOwner` for tram, with the `OWNER_NONE` / `OWNER_TOWN` remap |

`GetRailType` reads `m8` bits 5..0 on a rail tunnel or rail bridge (`MakeRailBridgeRamp` calls `SetRailType`, `src/bridge_map.h` lines 170–173). On a road bridge those bits are not the rail type; tram type occupies bits 11..6 and the rail-type accessor is the other use of bits 5..0, selected by transport type.

The HTML's road type, tram type, road owner, tram owner, and rail type placements match these accessors.
