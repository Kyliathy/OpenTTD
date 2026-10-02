# Sprite groups

Action `0x02` builds the chain. Action `0x01` only records spritesets. Action `0x03` points an entity at a chain. Resolution walks the chain later, through `ResolverObject`.

## Pool

`SpriteGroup` is a pool item (`src/newgrf_spritegroup.h` lines 39–44):

```text
SpriteGroupPool = Pool<SpriteGroup, SpriteGroupID, 1024, PoolType::Data>
```

`SpriteGroupID` is a `PoolID` with end `1 << 30` and invalid `0xFFFFFFFF` (line 39). The pool is named `"SpriteGroup"` in `src/newgrf_spritegroup.cpp` line 18. `PoolType::Data` is excluded from `InitializeGame`'s `PoolBase::Clean(PoolType::Normal)`. `ResetNewGRFData` is what calls `_spritegroup_pool.CleanPool()` (`src/newgrf.cpp` line 503). See [archi-newgrf.md](archi-newgrf.md).

During one file, Action `0x02` also stores the group under a GRF-local id in `GrfProcessingState::spritegroups`, an array of `MAX_SPRITEGROUP + 1` pointers (`src/newgrf/newgrf_internal.h` lines 86 and 114). That array is cleared between files (`ClearDataForNextFile`, lines 117–127). The pool object is what Action `0x03` keeps.

`NewSpriteGroup` (`src/newgrf/newgrf_act2.cpp` lines 759–775) reads `feature`, `set-id`, and `type`, then `ReadSpriteGroup`.

## Group kinds

`ReadSpriteGroup` (lines 693–755) switches on `type`:

| `type` | Class | Scope and size, from the bits of `type` |
| --- | --- | --- |
| `0x81`, `0x82` | `DeterministicSpriteGroup` | byte; self if bit 1 is clear, parent if bit 1 is set (`ReadDeterministicSpriteGroup` lines 348–355) |
| `0x85`, `0x86` | `DeterministicSpriteGroup` | word; same scope bit |
| `0x89`, `0x8A` | `DeterministicSpriteGroup` | dword; same scope bit |
| `0x80` | `RandomizedSpriteGroup` | self (`ReadRandomizedSpriteGroup`: bit 1 clear, bit 2 clear) |
| `0x83` | `RandomizedSpriteGroup` | parent (bit 1 set) |
| `0x84` | `RandomizedSpriteGroup` | relative only when the feature is `Aircraft` or lower; otherwise the scope stays self. Bit 2 still reads `count` (`src/newgrf/newgrf_act2.cpp` lines 474–478) |
| `0x80` and above, any other value | none | logged as a reserved type and skipped (lines 721–723) |
| below `0x80` | depends on the feature | the byte is a count of entries, not a variational type (comment at lines 765–769) |

For `type < 0x80` the feature selects the leaf (`ReadSpriteGroup` lines 726–749):

| Features | Leaf |
| --- | --- |
| Trains, road vehicles, ships, aircraft, stations, canals, cargoes, airports, rail types, road types, tram types, badges | `RealSpriteGroup` via `ReadRealSpriteGroup` |
| Houses, airport tiles, objects, industry tiles, road stops | `TileLayoutSpriteGroup` via `ReadTileLayoutSpriteGroup` |
| Industries | `IndustryProductionSpriteGroup` via `ReadIndustryProductionSpriteGroup` |
| anything else | skipped |

`ResultSpriteGroup` is not read as its own Action `0x02` type. `ReadRealSpriteGroup` creates one when a "real" group would have a single spriteset, and returns that instead of a `RealSpriteGroup` (the "one result" and "same result" paths around lines 527–550). `CallbackResultSpriteGroup` is the in-chain form of a callback return value (`src/newgrf_spritegroup.h` lines 233–246).

`ResolverResult` (lines 29–34) is one of: failure (`std::monostate`), `CallbackResult`, `ResultSpriteGroup`, `TileLayoutSpriteGroup`, or `IndustryProductionSpriteGroup`.

## Deterministic groups

`DeterministicSpriteGroup` (`src/newgrf_spritegroup.h` lines 192–207) stores:

- `var_scope`: `VarSpriteGroupScope::Self` or `Parent` (line 115–121). Relative scope is used by randomized groups.
- `size`: `DeterministicSpriteGroupSize::Byte`, `Word`, or `DWord`.
- `adjusts`: a list of `DeterministicSpriteGroupAdjust`.
- `ranges`: inclusive `[low, high]` intervals, each pointing at a `DeterministicSpriteGroupResult`.
- `default_result` and `error_group` (the first range, kept for an unavailable variable).

`DeterministicSpriteGroup::Resolve` (`src/newgrf_spritegroup.cpp` lines 190–249) starts `last_value` at 0 and walks `adjusts`:

- Variable `0x7E` resolves `adjust.subroutine` and takes its `CallbackResult`, or `UINT16_MAX` if the subgroup did not return one (lines 200–203).
- Variable `0x7B` reads variable `adjust.parameter`, passing `last_value` as that variable's parameter (line 207).
- Any other variable goes through `GetVariable` (lines 54–76). Variables `0x0C`, `0x10`, `0x18`, and `0x1C` are the callback id, callback parameter 1, callback parameter 2, and `last_value`. Variable `0x5F` is random bits in the high byte and triggers in the low byte. Variable `0x7D` is a temporary register. Variable `0x7F` is `GRFFile::GetParam`. Variables below `0x40` try `GetGlobalVariable` before the feature scope.

The adjust is then evaluated at the group's width (`EvalAdjustT`, lines 144–181). The first adjust's operation is forced to `Add` at load time (`src/newgrf/newgrf_act2.cpp` line 365). Later adjusts use `DeterministicSpriteGroupAdjustOperation` (`src/newgrf_spritegroup.h` lines 138–162): add, subtract, signed and unsigned min/max, signed and unsigned division and modulo, multiply, and, or, xor, store to a temporary register (`Sto`), return the operand (`Rst`), store to persistent storage (`Stop`), rotate, signed and unsigned compare, and shifts. Division or modulo by zero returns `last_value` unchanged (lines 163–166). Rotate and shifts mask the count to 5 bits (lines 174–179).

An unavailable variable stops the chain and resolves `error_group` (lines 212–215).

The final value picks the first range where `low <= value <= high`. Lists longer than 4 ranges use `lower_bound` on `high` (lines 231–243). If the chosen result has `calculated_result`, the return is `GB(value, 0, 15)` as a `CallbackResult` (lines 246–248). Otherwise the result's `SpriteGroup` is resolved.

`ScopeResolver::StorePSA` is the persistent store used by `Stop` (lines 112–116). Industry scopes allocate a `PersistentStorage` for that (`src/newgrf_industries.cpp` lines 450–454). That pool is a normal pool (`src/newgrf_storage.h` line 193), so it is wiped with the world, unlike the sprite-group pool.

## Randomized groups

`RandomizedSpriteGroup` (`src/newgrf_spritegroup.h` lines 215–230) stores `var_scope`, `cmp_mode` (`Any` or `All`), `triggers`, `count`, `lowest_randbit`, and `groups`. `ReadRandomizedSpriteGroup` logs an error when `num_groups` is not a power of two (`HasExactlyOneBit`, `src/newgrf/newgrf_act2.cpp` lines 487–488) and still builds the group. Bit 7 of the trigger byte selects `All` versus `Any` (line 483).

`RandomizedSpriteGroup::Resolve` (`src/newgrf_spritegroup.cpp` lines 253–270):

- When the callback is `CBID_RANDOM_TRIGGER` (`0x01`), the group's trigger mask is compared with `GetWaitingRandomTriggers`. `Any` matches a non-zero overlap; `All` matches the whole mask. A match records the overlapping triggers in `used_random_triggers` and ORs a reseed mask into `reseed[var_scope]`: `(groups.size() - 1) << lowest_randbit`.
- The group index is always `(GetRandomBits() & mask) >> lowest_randbit`, including during a trigger callback. `Resolve` does not draw a new random value.

Vehicle bits come from `Vehicle::random_bits` (`src/newgrf_engine.cpp` lines 312–314). Writing new bits is `DoTriggerVehicleRandomisation` (lines 1273–1291): it resolves with `CBID_RANDOM_TRIGGER`, calls `Random()`, and merges those bits into `v->random_bits` under `GetReseedSum()`. Industry self-scope bits come from `Industry::random` (`src/newgrf_industries.cpp` lines 430–432). Industry parent scope is the related town (`src/newgrf_industries.h` lines 55–57); `TownScopeResolver` leaves `GetRandomBits` at the base, which returns 0.

## Real groups and the resolver

`RealSpriteGroup` has two vectors, `loaded` and `loading` (`src/newgrf_spritegroup.h` lines 107–108). Its `Resolve` asks the feature resolver (`src/newgrf_spritegroup.cpp` lines 278–283). The default `ResolverObject::ResolveReal` returns `loaded[0]`, else `loading[0]` (lines 123–128).

`VehicleResolverObject::ResolveReal` (`src/newgrf_engine.cpp` lines 1025–1046) picks `loaded` when the vehicle is in motion and `loading` when it is loading, then indexes by `cargo.StoredCount() * totalsets / cargo_cap`.

`ResolverObject::DoResolve` (`src/newgrf_spritegroup.h` lines 342–348) clears temporary-register changes, zeroes `last_value`, `used_random_triggers`, and `reseed`, then resolves `root_spritegroup`. `ResolveCallback` returns the `CallbackResult` alternative, or `CALLBACK_FAILED` (lines 423–431). `Resolve<T>()` returns a typed leaf pointer, or `nullptr`.

Temporary registers are a process-wide `TemporaryStorageArray<int32_t, 0x110>` (`ResolverObject::temp_store`, line 324). `DoResolve` clears the changes at entry, so one callback does not see another callback's registers. `GetRegister` / `SetRegister` require the index to be below `0x110` (lines 359–372).
