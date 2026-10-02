# Link graph demands

Parent: [archi-linkgraph.md](archi-linkgraph.md).

`DemandHandler::Run` constructs a `DemandCalculator` and discards it. The calculator’s constructor performs the whole calculation. The class comment says it has state and is recreated for every call. `DemandHandler` itself keeps no state; `ComponentHandler` requires that, because handlers run on different threads.

Demand is written only into `LinkGraphJob::NodeAnnotation::demands` via `DeliverSupply`. It is a matrix over nodes, not a property of edges. Edges come later, when the MCF passes look for paths.

## Inputs

From the job snapshot (`job.Settings()`, `job.Cargo()`, node fields):

| Field | Source |
| --- | --- |
| `accuracy` | `settings.accuracy` (default 16, range 2–64) |
| `mod_dist` | `settings.demand_distance` (default 100, range 0–255). If the value is greater than 100, it is replaced with `100 + ((mod_dist - 100) * (mod_dist - 100)) / 12`. The comment lists 100→100, 150→308, 200→933, 255→2102 |
| `base_distance` | `IntSqrt(DistanceMaxPlusManhattan(TileXY(0,0), TileXY(Map::MaxX(), Map::MaxY())))` |
| Distribution | `settings.GetDistributionType(cargo)` |

`Symmetric` constructs `SymmetricScaler(settings.demand_size)` and calls `CalcDemand`. `Asymmetric` does the same with `AsymmetricScaler`. Any other value, including `Manual`, returns without writing demand. `demand_size` (default 100, range 0–100) is unused in the asymmetric path.

`base_distance` reads the map size. That is outside the job object. The handler comment says a handler must not read or write data outside the component because that can desync. Map extent is the same on every peer; the demand math below otherwise uses only the job.

Node `supply` is whatever had been accumulated on the component when it was copied. Node `demand` on `BaseNode` is the acceptance flag at `AddNode` and later `acceptance / 8` from `UpdateStationAcceptance`. The scaler treats `base.demand > 0` as “this node accepts”.

## Symmetric and asymmetric scalers

`AsymmetricScaler::EffectiveSupply` returns `from.base.supply`. `HasDemandLeft` is `to.base.demand > 0`. `AddNode` and `SetDemandPerNode` are empty.

`SymmetricScaler` sums every node’s supply in `AddNode`. `SetDemandPerNode(num_demands)` sets `demand_per_node` to `max(supply_sum / num_demands, 1)`. Effective supply from `from` toward `to` is:

```
max(from.base.supply * max(1, to.base.supply) * mod_size / 100 / demand_per_node, 1)
```

`mod_size` is `demand_size`. `HasDemandLeft` is `(to.base.supply == 0 || to.undelivered_supply > 0) && to.base.demand > 0`. A station that supplies something only keeps accepting, in this mode, while it still has undelivered supply.

`SymmetricScaler::SetDemands` also writes a reverse demand when `job[from_id].base.demand > 0`:

- `demand_back = demand_forw * mod_size / 100`, clamped to `job[to_id].undelivered_supply`.
- If that clamp hit, `demand_forw` is recomputed as `max(1, demand_back * 100 / mod_size)`.
- Reverse direction is delivered first (`to` → `from`), then the forward direction.

`Scaler::SetDemands` is `job[from_id].DeliverSupply(to_id, demand_forw)`.

The setting help text for `demand_size` says that 0% makes symmetric distribution behave like asymmetric distribution. In the formula, `mod_size == 0` makes `EffectiveSupply` fall through to 1 (because of the outer `max(..., 1)`), and `SetDemands` sets `demand_back` to 0. That is what the code does; the help text is the player-facing description.

## `CalcDemand` loop

`CalcDemand` builds two `std::queue<NodeID>`s: nodes with `supply > 0`, and nodes with `base.demand > 0`. If either count is zero it returns.

`scaler.SetDemandPerNode(num_demands)` runs once. A local `chance` starts at 0.

While both queues are non-empty:

1. Pop a supply node `from_id`.
2. For up to `num_demands` iterations, pop a demand node `to_id`. If `from_id == to_id`, push it back onto the demand queue. If that was the last node in both queues, return.
3. `supply = scaler.EffectiveSupply(...)` (the local variable shadows the node field; it is the scaler’s figure).
4. Distance scaling, with `divisor_scale = 16`:
   - `scaled_distance` starts as `base_distance`.
   - If `mod_dist > 0`, `distance = DistanceMaxPlusManhattan` of the two nodes’ `xy`, and `scaled_distance = max(0, base_distance + ((distance - base_distance) * mod_dist) / 1024)`.
   - `divisor = 16 + (accuracy * scaled_distance * 16) / (base_distance * 2)`. The code asserts `divisor >= 16`.
5. Forward demand:
   - If `divisor <= supply * 16`, `demand_forw = (supply * 16) / divisor`. The comment says this keeps pairs whose effective supply over the accuracy divisor is at least 1, and drops pairs that are too small or too far.
   - Otherwise increment `chance`. When `chance > accuracy * num_demands * num_supplies`, `demand_forw = 1`. The comment says this eventually distributes leftover supply to the other nodes.
6. `demand_forw` is clamped to `undelivered_supply` of `from_id`.
7. `scaler.SetDemands(...)`.
8. If the destination still has demand left, it is pushed back and `num_demands` stays; otherwise `num_demands` decreases.
9. If `from_id` still has undelivered supply, the inner loop breaks (the rest of the demand queue is left as it is). After the inner loop, a node with remaining supply is pushed back onto the supply queue; a node that is exhausted decrements `num_supplies`.

`chance` is a counter in this call. It is not `Random`.

No distances are cached on the component. Each pair evaluates `DistanceMaxPlusManhattan` when `mod_dist > 0`.
