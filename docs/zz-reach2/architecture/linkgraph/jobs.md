# Link graph jobs

Parent: [archi-linkgraph.md](archi-linkgraph.md).

A `LinkGraphJob` is one calculation of one `LinkGraph` component. It is a pool item (`LinkGraphJobPool _link_graph_job_pool`, first chunk 32).

## Snapshot

`LinkGraphJob(LinkGraphJobID, const LinkGraph &)` copies three things and then starts nothing by itself:

- `link_graph` — full copy of the component, **including** `index`. The comment says the index is copied on purpose so the finished job can find the live component again.
- `settings` — `_settings_game.linkgraph` at spawn. Handlers call `Settings()` and do not re-read the global settings.
- `join_date` — `TimerGameEconomy::date + (recalc_time / EconomyTime::SECONDS_PER_DAY)`.

The copy is `const`. The comment on the member says it must not be modified after the job starts. Save/load uses a second constructor that default-constructs an empty graph and then the saveload code writes `join_date`, `link_graph.index`, the graph body, and settings through `const_cast`-style address arithmetic (`GetLinkGraphJobDesc` in `src/saveload/linkgraph_sl.cpp`). The bare constructor’s comment says those members are brutally const-cast for that reason.

`SpawnThread` is called by `LinkGraphSchedule::SpawnNext` (normal play) or `SpawnAll` (after load). It calls `StartNewThread` with the name `"ottd:linkgraph"` and `LinkGraphSchedule::Run`. If that returns false, `Run` is called on the current thread. The comment in `SpawnThread` says that inline path will hang a bit, and that a finer `Step` API was not implemented.

`LinkGraphSchedule::instance` and `Path::invalid_path` are both created at startup. Their comments say a lazy first use would race the cargo-distribution threads.

## Annotations built in the worker

`InitHandler` → `LinkGraphJob::Init` runs on the worker, not in the constructor, so the main thread does not pay for it at spawn. For each copied node it emplaces a `NodeAnnotation`:

- `undelivered_supply` starts as `BaseNode::supply`.
- `edges` are `EdgeAnnotation`s pointing at the copied `BaseEdge`s. `flow` starts at 0.
- `demands` has one `DemandAnnotation` per node in the component, initially 0. Demand is all-pairs; it is not limited to existing edges.
- `paths` and `flows` start empty.

`NodeAnnotation::DeliverSupply` subtracts from `undelivered_supply` and adds the same amount to both `demand` and `unsatisfied_demand` toward the destination. `SatisfyDemandTo` only reduces `unsatisfied_demand`. `EdgeAnnotation::AddFlow` / `RemoveFlow` adjust the planned flow on that edge.

## Paths

`Path` is one leg of a path tree. `MCF` code subclasses it as `DistanceAnnotation` and `CapacityAnnotation` (those types live in `mcf.cpp`). Fields that matter for the architecture:

- `parent`, `origin`, `node`, `num_children` — tree structure. `Fork` attaches this leg to a base path and sets capacity, free capacity, and distance from the base plus the new leg.
- `capacity` — minimum edge capacity along the path.
- `free_capacity` — minimum residual (`capacity - flow`) for the current Dijkstra run.
- `flow` — flow assigned to this leg.

`Path::AddFlow(new_flow, job, max_saturation)` walks to the parent first. When `max_saturation != UINT_MAX` the usable capacity of the parent edge is `edge.base.capacity * max_saturation / 100`, and `new_flow` is clamped to what is left. The edge’s planned flow increases by whatever was actually pushed. A leg whose flow goes from 0 to non-zero is pushed to the front of the parent node’s `paths` list. The comment on `NodeAnnotation::paths` says the list is ordered so that legs with `flow == 0` sit at the back. `FlowMapper` relies on that: it breaks out of the path loop at the first zero-flow leg.

`Path::invalid_path` is a static `Path(INVALID_NODE, true)` used by cycle elimination as a “already resolved” marker. It is not a real route.

## Thread flags

| Flag | Store | Documented leeway |
| --- | --- | --- |
| `job_completed` | `true` at the end of `LinkGraphSchedule::Run`, `memory_order_release` | `IsJobCompleted` may return an incorrect value. The `Run` comment says a reader may see a stale value and that the worst case is pausing longer than necessary. The join is the real synchronisation |
| `job_aborted` | `AbortJob`, `memory_order_release` | `IsJobAborted` may spuriously return false. It must not spuriously return true |

`Run` checks `IsJobAborted()` before every handler. `MCF1stPass` and `MCF2ndPass` also check it in their outer loops. An abort returns without storing `job_completed`.

`IsScheduledToBeJoined` is `join_date <= TimerGameEconomy::date`. That test is on the simulation thread. It does not look at the worker.

## Join and publication

`LinkGraphSchedule::JoinNext` deletes the front job once it is scheduled to be joined. `~LinkGraphJob`:

1. `JoinThread()` if the `std::thread` is joinable.
2. If `CleaningPool()`, return. No station or flow updates. This is the shutdown / new-game path after `AbortJob`.
3. `assert(!IsJobAborted())`. An aborted job is only valid to destroy while the pool is cleaning.
4. If the copied `link_graph.index` is no longer a live `LinkGraph`, return. That happens when this component was merged into another and deleted.
5. Otherwise, for each node, write flows into the station’s `GoodsEntry` for this cargo.

Per node, publication skips or erases work when the world moved on during the run:

- Station gone → `EraseFlows` for that station id (drops flows inside the **job** that originated there).
- `GoodsEntry::link_graph` or `GoodsEntry::node` no longer matches this job → same erase, then continue.
- For each job edge with non-zero flow, if the destination station is gone, its node id does not match, the live edge is gone, or the live edge’s `LastUpdate()` is `INVALID_DATE`, flows via that station are deleted. Origins whose entire `FlowStat` disappeared are also erased from the station’s existing flows, which the comment says avoids cycles between old and new flows.
- If the live edge’s `last_unrestricted_update` is `INVALID_DATE`, the edge is fully restricted and `flows.RestrictFlows(to)` is called on the **job** flows before they are swapped in.

The station’s existing `FlowStatMap` is then reconciled with the job map:

- Origin present in both → `SwapShares`, then the job entry is erased.
- Origin present only on the station, and live distribution for this cargo is not `Manual` → `FlowStat::Invalidate` (every share becomes 1, so routing can continue, but the weights no longer dominate link-usage stats). The comment says the shares are not deleted, because that could leave cargo with no route.
- Origin present only on the station, and live distribution **is** `Manual` → shares are swapped into a temporary `FlowStat`, the map entry is erased, and `RerouteCargo` is called for each remaining share.
- Origins that exist only on the job are inserted.

This manual-vs-automatic test uses `_settings_game.linkgraph` at join time, not `job.Settings()`.

Empty goods data is cleared. `InvalidateWindowData(WindowClass::StationView, station, cargo)` runs for each station that was updated.

`JoinNext` then, if the link-graph id is still valid, dequeues and re-queues that component.

## Who joins, and when the sim can see the result

`OnTick_LinkGraph` calls `JoinNext` on dedicated servers and on clients. Pause control runs only for `!_networking || _network_server` (`StateGameLoop`). See the pause diagram in [archi-linkgraph.md](archi-linkgraph.md).

`CallVehicleTicks` runs before `CallLandscapeTick` → `OnTick_LinkGraph`. Cargo handling in that tick still uses the flows from the previous publication. The new shares are visible after `~LinkGraphJob` returns, which is still inside that landscape tick, so subsequent ticks observe them.

There is no API that installs flows from the worker. The worker’s last write to shared game state is the `job_completed` flag (and it only reads the job). The handler contract in `ComponentHandler` forbids touching anything outside the job.
