# R2MLVS3 - Sealed Portals and Deep Rail

Status: proposed. Parent: [R2MLVS master](r2mlvs-multilayer-vehicles-and-structures.md). Depends on r2mlvs2.

## Outcome and reading

New portal exits begin blocked; a controlled rail commissioning path admits a TBM. Excavated Deep Underground gains rail access through Underground from the proposed 1 January 1980 calendar unlock. Read master AD-3/4 and I-1/2/4/8/14; [map layers](../../architecture/map-layers/archi-map-layers.md), `src/layer_portal_map.h`, `src/layer_portal_cmd.cpp`, `src/map_layer.cpp`, `src/pathfinder/follow_track.hpp`, `src/signal.cpp`, `src/rail.cpp`, `src/timer/timer_game_calendar.h` and rail/road controllers.

## Group 1 - Portal state and connection matrix

- [ ] 1.1 Extend the profile-specific construction/pair matrix to Underground <-> Deep rail. Keep Surface <-> Underground rail/road and existing road elevators. Preserve layer IDs and the direct elevator's untouched intervening tile.
- [ ] 1.2 Define Sealed, Commissioning and Connected portal status, paired ownership and the work-pocket boundary. Save only authoritative fields and derive consistent per-end connectivity.
- [ ] 1.3 Add an Underground rail destination picker and accurate command quotes. Validate both endpoints, geology, footprint, reservations and vehicles before any paired mutation.
- [ ] 1.4 Make new portals into solid rock sealed. When both approaches were already excavated, validate a separate connection/commissioning step instead of granting excavation credit or reopening rock.
- [ ] 1.5 Give ordinary pathfinding/signals no traversable edge through a sealed end; the assigned TBM controller receives a narrowly scoped construction route. Normal trains cannot follow behind it into solid rock.
- [ ] 1.6 Add a shared calendar availability check: before 1 January 1980, refuse every new-profile pair touching Deep, including direct Surface/Deep road elevators, commissioning/activation and vehicle entry. Locked-layer preview is allowed, but Deep construction/excavation/industry operations are not. Keep explicit saved legacy/editor policies separate from ordinary play.

## Group 2 - Commissioning and bootstrap

- [ ] 2.1 Add a portal-associated TBM commissioning facility on the accessible side. Validate model/depth, track compatibility, purchase/assembly area, spoil wagons and the required Surface origin station.
- [ ] 2.2 Define source-side initial wagon exchange and cutter entry/reversal while sealed. Capacity must permit the first safe boring segment; allow return and unloading before any underground bay exists.
- [ ] 2.3 Enable the normal portal only after excavated approach, compatible track and work reservations permit it. Preserve train-part positions and reservations if the consist spans layers during a state change.
- [ ] 2.4 Apply blocked-exit behavior to road tunnels/elevators. They can connect excavated approaches but cannot commission TBMs or clear rock. Existing legacy portals retain prior behavior under their saved profile.
- [ ] 2.5 Handle cancellation, demolition, conversion, ownership transfer and source-side obstruction during commissioning without orphaning the TBM/project or half-editing the pair.
- [ ] 2.6 Do not treat tube guideways as eligible rail commissioning infrastructure or allow tube capsules through rail portals/elevators. Supply tube/utility interchanges on excavated accessible ground; a geothermal electrical riser may attach only to an already commissioned, date-valid portal through r2mlvs17.

## Group 3 - Controls, art and acceptance

- [ ] 3.1 Show sealed face, TBM-only commissioning and connected traffic states in portal preview, tooltip and sprites. Update road/elevator far-side errors to name excavation as the missing step.
- [ ] 3.2 Test every allowed/refused layer pair, all four directions, Deep build tools, both-end removal/conversion and attempted road/elevator commissioning.
- [ ] 3.3 Run a TBM first-entry and retreat scenario, normal train refusal before opening, then freight crossing after opening. Test reversal and signals with a consist spanning each new rail pair.
- [ ] 3.4 Save/reload in Sealed, Commissioning and Connected states, including occupied work pockets and legacy version-370 rail/road portals.
- [ ] 3.5 Test 31 December 1979 refusal and 1 January 1980 success through every Deep rail/elevator path, both build directions, scripting and commissioning. Check a legacy pre-1980 save still follows its explicit compatibility mode and changing economy speed cannot unlock technology early.

## Completion conditions

Creating a tunnel never produces an immediately usable unexcavated network. A first TBM can safely enter and retreat without an underground depot. Deep rail and both elevator access routes obey the 1980 calendar unlock in the new profile; tube tracks cannot commission a TBM. Existing legacy routes retain their documented compatibility behavior.
