# Link graph MCF solver

Parent: [archi-linkgraph.md](archi-linkgraph.md).

The schedule runs these handlers after demand has been written, in this order:

| Index | Type | What the constructor / `Run` does |
| --- | --- | --- |
| 2 | `MCFHandler<MCF1stPass>` | `MCF1stPass::MCF1stPass` runs the first pass to completion |
| 3 | `FlowMapper(false)` | Turns path trees into `FlowStatMap`s and deletes the paths. Does not scale |
| 4 | `MCFHandler<MCF2ndPass>` | `MCF2ndPass::MCF2ndPass` assigns remaining demand onto flows from the first pass |
| 5 | `FlowMapper(true)` | Maps paths again, then `FlowStat::ScaleToMonthly` |

`MCFHandler::Run` only constructs `Tpass(job)`. The pass constructors do the work. `MultiCommodityFlow` stores `job` and `max_saturation`, initialised from `job.Settings().short_path_saturation` (default 80, range 0–250).

`MCF1stPass`’s comment says this calculation is exponential in the number of nodes, with small constant factors, and names three ways to spend less time: lower `accuracy` (more flow per loop), lower `short_path_saturation` (this pass stops sooner), and a longer recalculation interval (the game tolerates a longer run). It does not describe the problem as NP-hard. That wording exists only in `docs/linkgraph.md`.

Both passes stop their outer loop when `job.IsJobAborted()` is true.

## Dijkstra

`MultiCommodityFlow::Dijkstra<Tannotation, Tedge_iterator>` is a binary-heap-free Dijkstra over a `std::set` ordered by `Tannotation::Comparator`. One annotation object is allocated per node. The source annotation is constructed with `source = true` (`Path` then has distance 0, capacity `UINT_MAX`, free capacity `INT_MAX`). Every other node starts disconnected (`distance == UINT_MAX`).

For each settled node the edge iterator yields destinations. An edge whose destination equals the source node is skipped. The comment says that is “not a real edge but a consumption sign”.

Capacity used for the annotation is the edge capacity, multiplied by `max_saturation / 100` when `max_saturation != UINT_MAX`. A product of 0 is raised to 1.

The annotation distance of an edge is:

- For `CargoClass::Passengers`, `Mail`, or `Express`: `TravelTime() + Ticks::DAY_TICKS` if `TravelTime()` is non-zero, otherwise `(DistanceMaxPlusManhattan + 1) * Ticks::DAY_TICKS`. The comment says in-between stops are punished with a 1-tile or 1-day penalty, and that passengers, mail, and express prefer the fastest route.
- For every other cargo: `DistanceMaxPlusManhattan + 1`. The comment says those classes prefer the shortest route.

`TravelTime()` is `travel_time_sum / capacity` on the copied edge.

If `dest->IsBetter(source, capacity, capacity - edge.Flow(), distance_anno)` the destination is re-forked from the source annotation and reinserted into the set.

`DistanceAnnotation::IsBetter` (first pass):

- A disconnected `base` is never better. A disconnected `this` is better than a connected `base`.
- If the new residual and the base residual are both positive: the base is better when `this` has no residual, or when both have residual and `base.distance + dist < this.distance`.
- If the new residual is not positive: the base is worse when `this` still has residual; if both are exhausted, the shorter distance wins.

`CapacityAnnotation::IsBetter` (second pass): compare `GetCapacityRatio` of the min residuals and min capacities. `GetCapacityRatio` is `Clamp(free, PATH_CAP_MIN_FREE, PATH_CAP_MAX_FREE) * 16 / max(total, 1)`. A higher ratio wins. Equal ratios prefer the shorter distance unless `base` is disconnected.

`Greater` orders equal annotation values by `NodeID` so two different paths with the same annotation are not treated as the same set key. `DistanceAnnotation`’s comparator prefers the smaller distance. `CapacityAnnotation`’s comparator prefers the larger ratio.

Edge iterators:

- `GraphEdgeIterator` walks `NodeAnnotation::edges` in stored order. `MCF1stPass` uses it, so the first pass can use every copied edge.
- `FlowEdgeIterator` walks the `FlowStat` shares for the path’s origin station and maps the `via` station back to a node id. `MCF2ndPass` uses it. The header comment says the second pass does not create paths along edges the first pass never visited, and that this is why it skips cycle detection. After the first `FlowMapper`, those visits exist as flow shares, and the path list has been cleared.

## Pushing flow

`MultiCommodityFlow::PushFlow`:

```
flow = Clamp(node.DemandTo(to) / accuracy, 1, node.UnsatisfiedDemandTo(to))
flow = path->AddFlow(flow, job, max_saturation)
node.SatisfyDemandTo(to, flow)
```

`accuracy` is `job.Settings().accuracy`. Lower accuracy makes the quotient larger, so each successful push satisfies more of the demand. `AddFlow` may return less than requested when `max_saturation` caps the edge (see [jobs.md](jobs.md)). `SatisfyDemandTo` subtracts only the amount actually pushed.

## First pass (`MCF1stPass`)

`max_saturation` stays at `short_path_saturation`.

Repeat until there is nothing left to do or the job is aborted:

For each source that is not finished, run `Dijkstra<DistanceAnnotation, GraphEdgeIterator>`. For every destination with unsatisfied demand:

- If `path->GetFreeCapacity() > 0`, `PushFlow` with `this->max_saturation`. If demand remains, another loop is required (`more_loops`).
- Else if unsatisfied demand still equals the original demand for that pair **and** `GetFreeCapacity() > INT_MIN`, `PushFlow` once with `max_saturation == UINT_MAX`. The comment says that if nothing has been assigned yet, any valid path is allowed once.
- If demand remains, the source is not finished.

Then `CleanupPaths` drops the root and any zero-flow legs that have no children, and detaches legs that hung directly off the root.

The outer condition is `(more_loops || EliminateCycles()) && !aborted`.

`EliminateCycles` searches from every node. `EliminateCycles(path, origin, next)`:

- If `path[next]` is already `Path::invalid_path`, the branch was resolved with no cycle.
- If `path[next]` is null, flows of paths with this origin that share the same next hop are summed onto one leg (the others are reduced to 0 and moved to the back of the path list). The search then recurses through those next hops. If no cycle was found, `path[next]` becomes `Path::invalid_path`. If a cycle was found, it becomes null so the branch can be searched again.
- If `path[next]` is already a path, the node was visited and the walk is a cycle. `FindCycleFlow` takes the minimum leg flow around the cycle. If that is non-zero, `EliminateCycle` subtracts it from each leg and from the corresponding edge annotation, and a zero-flow leg is moved to the back of its parent’s path list.

## Second pass (`MCF2ndPass`)

The constructor sets `this->max_saturation = UINT_MAX` before searching, which disables the artificial saturation cap in `Dijkstra` and in `AddFlow`.

While any demand remains and the job is not aborted, for each unfinished source:

- `Dijkstra<CapacityAnnotation, FlowEdgeIterator>` from that source.
- For each destination, if unsatisfied demand is positive and `GetFreeCapacity() > INT_MIN`, `PushFlow` with `UINT_MAX`.
- A source with nothing left is marked finished.
- `CleanupPaths` as in the first pass.

The header comment says this pass saturates paths with the most capacity left first, does not create paths on edges the first pass never visited, and therefore does no cycle detection. Accuracy is used the same way, through `PushFlow`.

## `FlowMapper`

`FlowMapper::Run` walks every node and every path until `GetFlow()` is 0.

For a leg from `prev` (the node that owns the path list) to `via` (the leg’s node), with `origin` from `path->GetOrigin()`:

- `node.flows.AddFlow(origin, via, flow)` — the comment says this marks the flow for local consumption at the leg’s node.
- If `prev` is not the origin, `prev_node.flows.PassOnFlow(origin, via, flow)`. `PassOnFlow` adds the share toward `via` and an equal share toward `StationID::Invalid()`, which stands for “passed on, not consumed here”.
- If `prev` is the origin, `prev_node.flows.AddFlow(origin, via, flow)`.

Then each node’s `FlowStatMap::FinalizeLocalConsumption` subtracts the invalid (`StationID::Invalid()`) share from the local station’s share. The comment on `PassOnFlow` says negative flows are avoided by recording the passed-on amount as invalid and subtracting it at the end.

If this mapper was constructed with `scale == true` (only the last handler):

```
runtime = JoinDate() - Settings().recalc_time / CalendarTime::SECONDS_PER_DAY - LastCompression() + 1
```

`JoinDate() - recalc_time / SECONDS_PER_DAY` is the spawn date, because that is how `join_date` was built. `CalendarTime::SECONDS_PER_DAY` and `EconomyTime::SECONDS_PER_DAY` are both 2. The comment says the `+ 1` matches `LinkGraph::Monthly` and avoids dividing by zero when the spawn date equals the last compression date. Each `FlowStat::ScaleToMonthly(runtime)` replaces shares so the running total is `max(previous + 1, old_key * 30 / runtime)`.

Every path object is then `delete`d and `paths` is cleared, so the next MCF pass does not see the previous path tree. The second pass reads the flows instead, through `FlowEdgeIterator`.

The finished `FlowStatMap` stays on the job until `~LinkGraphJob` swaps it into the station. That step is in [jobs.md](jobs.md).
