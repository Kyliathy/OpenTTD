/*
 * This file is part of OpenTTD, licensed under the GNU GPL version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file space_vehicle.cpp Native orbital probe ownership, deterministic pursuit and conserved extraction.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BG; read-only source-authored probe presentation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13K; original heavy/retained/hauler art ownership.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group21; opt-in operation diagnostics never govern simulation.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group20; generation-bound synchronized mission admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs8-space-world-and-construction.md Group11; shared approach admission.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group1; retained asteroid cargo in Core stock.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs9-launch-centers-and-rockets.md Group14; cargo recall ends inside its exact owned drop circle.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group8BC; actual new Engine definitions publish native availability before replacement policy queries them.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group9C; per-model extraction retains the original finite-resource transaction.
 */
#include "stdafx.h"
#include "space_work_cost.h"
#include "space_mining_inventory.h"
#include "space_departure.h"
#include "space_mining_tuning.h"
#include "space_vehicle.h"
#include "space_sprite.h"
#include "orbital_sprite.h"
#include "space_replacement.h"
#include "space_approach.h"
#include "space_manifest.h"
#include "space_pod.h"
#include "space_structure.h"
#include "orbital_service.h"
#include "orbital_manufacturing.h"
#include "space_transition.h"
#include "launch_operations.h"
#include "multilayer_space_content.h"
#include "space_vehicle_cmd.h"
#include "command_func.h"
#include "network/network.h"
#include "asteroid.h"
#include "engine_base.h"
#include "engine_func.h"
#include "cargo_type.h"
#include "cargotype.h"
#include "company_base.h"
#include "station_base.h"
#include "company_func.h"
#include "core/backup_type.hpp"
#include "group.h"
#include "map_func.h"
#include "landscape.h"
#include "multilayer_state.h"
#include "vehicle_func.h"
#include "window_func.h"
#include "table/strings.h"
#include <set>
#include "safeguards.h"

/** @param index Actual Vehicle pool slot; paid commissioning and generation issuance remain separate owner transactions. */
SpaceVehicle::SpaceVehicle(VehicleID index) : SpecializedVehicleBase(index) {}

/** @return Actual native Space hull count, including retained inactive paid objects. */
uint32_t GetSpaceVehicleCount()
{
	uint32_t count = 0; for ([[maybe_unused]] const auto *vehicle : SpaceVehicle::Iterate()) ++count; return count;
}

/** @param model Delivered native role. @return Its selected-profile engine mapping, or invalid for unavailable roles. */
EngineID GetSpaceEngine(uint16_t model)
{
	// Only the delivered native role catalogue can resolve an engine; future manufacturing cannot acquire a free definition here.
	if (model != 14 && model != 15 && model != 16 && model != 17 && model != 19 && model != 20 && model != SPACE_HEAVY_MINER_MODEL) return EngineID::Invalid();
	const auto &required = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content;
	return _engine_mngr.GetID(VehicleType::Space, 0xF000 + model, required.package.grfid);
}
/** @return Existing small mining-probe engine mapping; this query creates no hull or catalogue entry. */
EngineID GetSpaceProbeEngine() { return GetSpaceEngine(15); }
/** @param vehicle Actual native hull. @return Canonical selected-profile role, or an invalid sentinel for an unrelated engine. */
uint16_t GetSpaceVehicleModel(const SpaceVehicle &vehicle)
{
	const auto *engine = Engine::GetIfValid(vehicle.engine_type);
	// A missing or ordinary engine cannot lend a Space role to this hull.
	if (engine == nullptr || engine->type != VehicleType::Space) return UINT16_MAX;
	const auto model = engine->VehInfo<SpaceVehicleInfo>().model;
	return GetSpaceEngine(model) == engine->index ? model : UINT16_MAX;
}

/** Restore a delivered role's catalogue definition without commissioning any vehicle.
 * @param model Native role requested by an actual producer or retained save.
 * @return Whether the selected package and engine mapping admit the definition.
 */
bool EnsureSpaceEngineDefinition(uint16_t model)
{
	// Catalogue reconstruction accepts only delivered roles; manufacturing plans cannot manufacture a definition by numeric coincidence.
	if (model != 14 && model != 15 && model != 16 && model != 17 && model != 19 && model != 20 && model != SPACE_HEAVY_MINER_MODEL) return false;
	const auto *definition = GetSpaceVehicleConstruction(model);
	const auto &required = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content;
	// Exact paid construction data and selected package identity must both exist before mutating an engine catalogue entry.
	if (definition == nullptr || CheckMultiLayerContentIdentity(required, _grfconfig) != MultiLayerContentIdentityError::None) return false;
	BuildCargoLabelMap();
	const CargoLabel label = model == 14 || model == 16 || model == 17 || model == 20 ? CargoLabel{"STEL"} : CargoLabel{"NORE"};
	const CargoType cargo = GetCargoTypeByLabel(label);
	if (!IsValidCargoType(cargo)) return false;
	const uint16_t local = 0xF000 + model;
	const auto id = GetSpaceEngine(model);
	bool created = false; ///< Only a genuinely new runtime mapping needs native startup; estimates and retained reconstruction never reset catalogue history.
	Engine *engine = Engine::GetIfValid(id);
	// A pre-existing mapping cannot overwrite an unrelated vehicle family or local content identity.
	if (engine != nullptr && (engine->type != VehicleType::Space || engine->grf_prop.local_id != local)) return false;
	if (engine == nullptr) {
		if (id == EngineID::Invalid()) {
			if (!Engine::CanAllocateItem()) return false;
			engine = Engine::Create(VehicleType::Space, local);
			_engine_mngr.SetID(VehicleType::Space, local, required.package.grfid, 0, engine->index);
			created = true;
		} else engine = Engine::CreateAtIndex(id, VehicleType::Space, local);
	}
	engine->info.base_intro = TimerGameCalendar::ConvertYMDToDate(definition->introduction_year, 0, 1);
	engine->intro_date = engine->info.base_intro;
	engine->info.base_life = TimerGameCalendar::Year{0xFF};
	engine->info.lifelength = TimerGameCalendar::Year{30};
	engine->info.climates = {LandscapeType::Temperate};
	engine->info.cargo_type = cargo; engine->info.cargo_label = label;
	engine->info.cargo_age_period = Ticks::CARGO_AGING_TICKS;
	engine->info.misc_flags.Set(EngineMiscFlag::NoDefaultCargoMultiplier);
	engine->info.extra_flags.Set({ExtraEngineFlag::NoNews, ExtraEngineFlag::NoPreview});
	engine->info.string_id = STR_JUST_RAW_STRING;
	engine->name = model == 14 ? "Cargo rocket" : model == 15 ? "Mining probe" : model == 16 ? "Heavy construction robot" :
		model == 17 ? "Orbital cargo hauler" : model == 19 ? "Survey probe" : model == SPACE_HEAVY_MINER_MODEL ? "Heavy mining robot" : "Construction probe";
	auto &info = engine->VehInfo<SpaceVehicleInfo>();
	const auto &budget = GetMultiLayerSpaceBudget();
	const auto *miner = GetSpaceMiningTuning(model);
	info.model = model;
	const auto mass = model == 14 ? budget.rocket_payload_sixteenths : miner != nullptr ? miner->hold_sixteenths : budget.probe_hold_sixteenths;
	info.capacity = model == 16 || model == 19 ? 0 : mass / 16;
	info.purchase_cost = definition->capital;
	// Actual commissioning can create a definition after global catalogue startup. Publish its native company availability without inventing private masks or resetting saved existing Engines.
	if (created) StartupNewEngine(engine);
	engine->reliability = 0xFFFF;
	return true;
}

/** @return Whether the default miner and already saved native role mappings can be reconstructed without unsolicited entries. */
bool InitialiseSpaceEngineDefinition()
{
	if (!EnsureSpaceEngineDefinition(15)) return false;
	// Restore already saved mappings without appending unsolicited engines to old worlds.
	for (uint16_t model : {14, 16, 17, 19, 20, 21}) if (GetSpaceEngine(model) != EngineID::Invalid() && !EnsureSpaceEngineDefinition(model)) return false;
	return true;
}

/** @param vehicle Actual hull relinquishing only its own body lease/work; a distinct static service berth remains separately owned. */
void ReleaseSpaceMiningSlot(SpaceVehicle &vehicle)
{
	auto *body = GetMutableAsteroidBody(vehicle.mission.target);
	// Releasing one hull must never revoke another hull's real extraction lease.
	if (body != nullptr && body->mining_slot == vehicle.index) body->mining_slot = VehicleID::Invalid();
	vehicle.mission.work_ticks = 0;
	// Releasing an actual body attachment also releases its approach; a distinct static berth must remain owned until departure.
	if (vehicle.space_orders.approach && vehicle.space_orders.approach->target.kind == SpaceOrderTargetKind::Body) ReleaseSpaceApproach(vehicle);
}

/** Release actual manifest, mining and commissioning references before native Vehicle teardown; global pool cleanup retains its own container policy. */
SpaceVehicle::~SpaceVehicle()
{
	ForgetSpaceManifest(*this, CleaningPool());
	if (!CleaningPool()) { ReleaseSpaceMiningSlot(*this); ForgetLaunchCraft(*this); }
	this->PreDestructor();
}

/** Round distance upwards and freeze the assignment deadline; pursuit cannot extend its own budget.
 * @param position Actual starting plane point. @param target Current authorized attachment point at assignment.
 * @return Bounded native tick allowance, including one arrival margin.
 */
uint32_t GetSpaceMiningApproachTicks(SpacePoint position, SpacePoint target)
{
	const uint64_t squared = SpaceDistanceSquared(position, target);
	uint64_t lo = 0, hi = 1ULL << 30;
	// Integer ceiling-root search converts the actual initial distance to a deterministic deadline without floating-point drift.
	while (lo < hi) {
		const uint64_t mid = (lo + hi) / 2;
		if (mid * mid < squared) lo = mid + 1; else hi = mid;
	}
	const auto &tuning = GetMultiLayerMiningProbeTuning();
	const uint64_t closing = tuning.closing_tiles_per_day * SPACE_Q16;
	return static_cast<uint32_t>(std::max<uint64_t>(tuning.minimum_approach_ticks,
		(lo * GetMultiLayerSpaceTuning().ticks_per_economy_day + closing - 1) / closing + tuning.approach_margin_ticks));
}

/** Assign the native mining/recall controller only when no paid Core berth retains this craft.
 * @param craft Actual orbital hull. @param target Monotonic body ID, or invalid for recall.
 * @param execute False estimates without changing work or target. @return Mission admission or unchanged refusal.
 */
CommandCost AssignSpaceMiningMission(SpaceVehicle &craft, uint32_t target, bool execute)
{
	// Explicit Core service holds this hull's physical berth; mining/recall cannot move it out during reserved work.
	if (HasOrbitalServiceAssignment(craft)) return CMD_ERROR;
	auto *vehicle = &craft;
	// Only this deployed, unstalled native miner in the active Industry profile may begin work; dormant carried hulls cannot mine during launch.
	const auto model = GetSpaceVehicleModel(craft);
	if (vehicle->construction.placement != SpacePlacement::Orbital || (vehicle->manifest && vehicle->manifest->service_ticks != 0) || GetSpaceMiningTuning(model) == nullptr || vehicle->vehstatus.Test(VehState::Crashed) || GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return CMD_ERROR;
	const bool recall = target == INVALID_ORBITAL_BODY.base();
	// Exhausted pods or service allowance block new extraction, while recall remains available to retain and recover cargo.
	if (!recall && GetSpaceMiningBlock(*vehicle) != SpaceOrderBlock::None) return CMD_ERROR;
	const auto *body = recall ? nullptr : GetMutableAsteroidBody(OrbitalBodyID{target});
	// A new extraction needs a surviving finite body and real hold space; recall must remain available with a full load or depleted target.
	if (!recall && (body == nullptr || body->remaining_reserve == 0)) return CMD_ERROR;
	const CargoType cargo = body == nullptr ? vehicle->cargo_type : GetCargoTypeByLabel(GetMultiLayerAsteroidDefinitions()[to_underlying(body->resource_class)].cargo);
	// Existing loaded packets cannot be relabelled by assigning a different asteroid resource; recall preserves the current cargo type.
	if (!IsValidCargoType(cargo) || (!recall && vehicle->cargo.TotalCount() != 0 && vehicle->cargo_type != cargo)) return CMD_ERROR;
	const auto capacity = GetSpaceFreightCapacity(model, cargo);
	// The selected body's physical unit weight determines this miner's hold; changing resource cannot retain an oversized previous refit or relabel loaded packets.
	if (!recall && (capacity == 0 || vehicle->cargo.TotalCount() >= capacity)) return CMD_ERROR;
	// Querying a new pursuit must prove the shared approach can admit this hull before replacing any current lease.
	if (!recall && !RequestSpaceApproach(craft, {SpaceOrderTargetKind::Body, target, 0}, false)) return CMD_ERROR;
	if (execute) {
		const auto previous = vehicle->space_orders.approach;
		ReleaseSpaceMiningSlot(*vehicle);
		ReleaseSpaceApproach(*vehicle);
		if (!recall) {
			// Reusing the same actual body preserves common-phase admission and original priority; a new target requests a new slot.
			if (previous && previous->target == SpaceOrderTarget{SpaceOrderTargetKind::Body, target, 0}) vehicle->space_orders.approach = previous;
			else { const bool requested = RequestSpaceApproach(craft, {SpaceOrderTargetKind::Body, target, 0}, true); assert(requested); (void)requested; }
		}
		// Recall ends extraction through Return and released leases, but retains the last natural body as departure geometry/history until a new assignment replaces it. Clearing it here would bypass the shared safe exit while still inside that body's envelope.
		if (!recall) vehicle->mission.target = body->id;
		vehicle->mission.phase = recall ? SpaceMissionPhase::Return : SpaceMissionPhase::Approach;
		const SpacePoint approach = recall ? vehicle->position : SpacePoint{body->motion.position.x + vehicle->mission.attachment_offset.x,
			body->motion.position.y + vehicle->mission.attachment_offset.y};
		vehicle->mission.approach_ticks_left = recall ? 0 : GetSpaceMiningApproachTicks(vehicle->position, approach);
		vehicle->cargo_type = cargo;
		if (!recall) vehicle->cargo_cap = capacity;
		vehicle->vehstatus.Reset(VehState::Stopped);
		SetPendingSpaceReplacementResume(GetSpaceCraftReference(*vehicle), true); // Explicit native mission admission starts this exact original; keep the one pending paid producer's eventual running intent consistent with that command.
		vehicle->MarkDirty();
	}
	return CommandCost{};
}

/** @copydoc IsSpaceMiningStateSupported */
bool IsSpaceMiningStateSupported(uint16_t model, SpaceMissionPhase phase, uint16_t work_ticks, uint32_t extracted_units, bool heavy_runtime)
{
	if (phase > SpaceMissionPhase::Holding) return false;
	const auto *miner = GetSpaceMiningTuning(model);
	const bool active = phase == SpaceMissionPhase::Approach || phase == SpaceMissionPhase::Mining || phase == SpaceMissionPhase::Return;
	// Navigation-only, survey and construction roles never acquire extraction work/history through forged native mission fields.
	if (miner == nullptr) return !active && work_ticks == 0 && extracted_units == 0;
	if (work_ticks >= miner->extraction_ticks) return false;
	// Save431 commissioned an idle paid21 only; neither a historical queue nor forged progress may grant its later runtime capability.
	if (model == SPACE_HEAVY_MINER_MODEL && !heavy_runtime && (active || work_ticks != 0 || extracted_units != 0)) return false;
	return true;
}

/** @param flags Query or synchronized execution. @param id Actual owned native miner. @param target Body ID, or invalid for cargo/empty recall.
 * @return Mission admission or unchanged refusal; the legacy command deliberately replaces the existing queue.
 */
static CommandCost SetSpaceMission(DoCommandFlags flags, VehicleID id, uint32_t target)
{
	auto *vehicle = SpaceVehicle::GetIfValid(id);
	// Mission edits require the actual command actor's hull, preventing foreign recovery or target reassignment.
	if (vehicle == nullptr || vehicle->owner != _current_company) return CMD_ERROR;
	// This legacy command removes the queue. A podless heavy craft cannot borrow its old future module Unload and then discard that sole return route; explicit recall remains available.
	if (GetSpaceVehicleModel(*vehicle) == SPACE_HEAVY_MINER_MODEL && target != INVALID_ORBITAL_BODY.base() && (!vehicle->manifest || vehicle->manifest->pods.empty())) return CMD_ERROR;
	const bool execute = flags.Test(DoCommandFlag::Execute) && !flags.Test(DoCommandFlag::QueryCost);
	const auto result = AssignSpaceMiningMission(*vehicle, target, execute);
	// The legacy mission command clears instructions, preserving the one actual approach request already admitted by that command.
	if (result.Succeeded() && execute) {
		const auto approach = vehicle->space_orders.approach;
		vehicle->space_orders = {}; vehicle->space_orders.approach = approach;
	}
	return result;
}

/** Preserve the historical offline mission argument layout.
 * @param flags Native query or execution flags.
 * @param id Original owned miner's native pool ID.
 * @param target Original body ID or UINT32_MAX for cargo-preserving Recall.
 * @return Original offline admission; multiplayer refuses this generation-free entry point.
 */
CommandCost CmdSetSpaceMission(DoCommandFlags flags, VehicleID id, uint32_t target)
{
	// Historical callers retain their original offline layout. Live multiplayer must carry an incarnation, including commands originating at the server.
	if (_networking) return CMD_ERROR;
	return SetSpaceMission(flags, id, target);
}

/** Admit only the hull incarnation originally selected by the player.
 * @param flags Native query or execution flags.
 * @param id Original owned miner's native pool ID.
 * @param generation Original monotonic hull incarnation, checked at authoritative execution.
 * @param target Original body ID or UINT32_MAX for cargo-preserving Recall.
 * @return Shared mission admission after identity validation, or unchanged stale-reference refusal.
 */
CommandCost CmdSetSpaceMissionChecked(DoCommandFlags flags, VehicleID id, uint64_t generation, uint32_t target)
{
	// The native pool may reuse an ID before a delayed command executes; only the originally selected incarnation may lose its queue or extraction lease.
	if (ResolveSpaceCraft({id, generation}) == nullptr) return CMD_ERROR;
	return SetSpaceMission(flags, id, target);
}

/** @param direction Native presentation direction. @param image_type Native sprite use. @param result Receives the existing role/phase sprite sequence. */
void SpaceVehicle::GetImage(Direction direction, EngineImageType image_type, VehicleSpriteSeq *result) const
{
	const auto view = image_type == EngineImageType::OnMap ? GetSpaceCraftArtDirection(*this) : direction;
	if (const auto pose = GetSpaceCraftArtPose(*this, view)) {
		if (const auto sprite = ResolveSpaceSprite(*pose)) { result->Set(*sprite); return; }
	}
	if (const auto pose = GetOrbitalCraftArtPose(*this, view)) {
		if (const auto sprite = ResolveOrbitalSprite(*pose)) { result->Set(*sprite); return; }
	}
	// The original rocket retains its separate Plan9 source/integration gate;
	// unrecognized legacy roles keep the explicit earlier preview without new capability.
	result->Set(SPR_SPACE_PREVIEW_PROBE + (this->mission.phase == SpaceMissionPhase::Mining ? 1 : 0));
}
/** Refresh the existing native sprite bounds for selection and viewport invalidation. */
void SpaceVehicle::UpdateDeltaXY() { this->bounds = {{-4, -4, 0}, {8, 8, 8}, {0, 0, 0}}; }
/** Invalidate this physical hull's viewport and Space information window after authoritative movement/state changes. */
void SpaceVehicle::MarkDirty() { this->UpdateViewport(true, true); SetWindowClassesDirty(WindowClass::SpaceSurvey); }
/** Project the actual saved Surface/Space custody and Q16 point to native tile/viewport fields, then synchronize the same carried paid shells. */
void SpaceVehicle::UpdateSpacePosition()
{
	if (GetSpaceVehicleLayer(*this) == MapLayer::Surface) {
		this->tile = this->construction.surface_tile;
		this->x_pos = TileX(this->tile) * TILE_SIZE + TILE_SIZE / 2;
		this->y_pos = TileY(this->tile) * TILE_SIZE + TILE_SIZE / 2;
		this->z_pos = GetTilePixelZ(this->tile);
		this->direction = Direction::NE; this->UpdateDeltaXY();
		this->GetImage(this->direction, EngineImageType::OnMap, &this->sprite_cache.sprite_seq);
		this->UpdatePositionAndViewport(); SyncSpacePods(*this); return;
	}
	const TileIndex origin = TileXYL(0, 0, MapLayer::Space);
	this->x_pos = static_cast<int64_t>(this->position.x) * TILE_SIZE / SPACE_Q16 + TILE_SIZE / 2;
	this->y_pos = static_cast<int64_t>(this->position.y) * TILE_SIZE / SPACE_Q16 + TileY(origin) * TILE_SIZE + TILE_SIZE / 2;
	this->z_pos = 0;
	this->tile = TileVirtXY(this->x_pos, this->y_pos);
	this->direction = Direction::NE;
	this->UpdateDeltaXY();
	this->GetImage(this->direction, EngineImageType::OnMap, &this->sprite_cache.sprite_seq);
	this->UpdatePositionAndViewport();
	SyncSpacePods(*this);
}

/** @return Published annual upkeep using the same actual service eligibility as native work and per-tick expense. */
Money SpaceVehicle::GetRunningCost() const
{
	if (this->construction.placement != SpacePlacement::Orbital) return 0;
	const auto &budget = GetMultiLayerSpaceBudget();
	const auto *miner = GetSpaceMiningTuning(GetSpaceVehicleModel(*this));
	const bool active = IsSpaceMiningWorkTick(*this) || IsSpaceSurveyWorkTick(*this) || IsSpaceServiceWorkTick(*this) ||
		IsOrbitalServiceWorkTick(*this) || IsSpaceConstructionWorkTick(*this);
	const auto active_rate = miner != nullptr ? miner->active_per_day : budget.active_per_day;
	const auto idle_rate = miner != nullptr ? miner->idle_per_day : budget.idle_per_day;
	return (active ? active_rate : idle_rate) * CalendarTime::DAYS_IN_YEAR * 256;
}
/** Apply canonical calendar aging to this existing native hull. */
void SpaceVehicle::OnNewCalendarDay() { AgeVehicle(this); }
/** Apply canonical economy age and periodic resale depreciation without changing cargo or paid commissioning history. */
void SpaceVehicle::OnNewEconomyDay()
{
	EconomyAgeVehicle(this);
	if ((++this->day_counter & 7) == 0) DecreaseVehicleValue(this);
}

/** @return Whether this hull survives its native synchronized tick; held service craft accrue only actual eligible upkeep. */
bool SpaceVehicle::Tick()
{
	RecordSpaceWork(SpaceWorkKind::CraftTicks);
	if (this->construction.placement != SpacePlacement::Orbital) return true;
	const auto &budget = GetMultiLayerSpaceBudget();
	const auto &tuning = GetMultiLayerMiningProbeTuning();
	const auto day = GetMultiLayerSpaceTuning().ticks_per_economy_day;
	++this->tick_counter;
	auto *body = GetMutableAsteroidBody(this->mission.target);
	const bool stopped = this->vehstatus.Test(VehState::Stopped);
	const bool mining = IsSpaceMiningWorkTick(*this);
	const bool servicing = IsSpaceServiceWorkTick(*this) || IsOrbitalServiceWorkTick(*this);
	const auto *miner = GetSpaceMiningTuning(GetSpaceVehicleModel(*this));
	const auto active_rate = miner != nullptr ? miner->active_per_day : budget.active_per_day;
	const auto idle_rate = miner != nullptr ? miner->idle_per_day : budget.idle_per_day;
	const uint16_t accrual = this->mission.cost_remainder +
		(mining || servicing || IsSpaceSurveyWorkTick(*this) || IsSpaceConstructionWorkTick(*this) ? active_rate : idle_rate);
	const uint16_t cost = accrual / day;
	this->mission.cost_remainder = accrual % day;
	SubtractMoneyFromCompany(this->owner, CommandCost(ExpensesType::Other, cost));
	this->profit_this_year -= Money{cost} * 256;
	// A waiting/paused Core job keeps its craft at the reserved berth while charging only the predicate's actual upkeep.
	// Paid service retains its actual physical berth/position and prevents navigation from moving its still-accountable manifest.
	if (HasOrbitalServiceAssignment(*this) || servicing || (this->manifest && this->manifest->service_ticks != 0)) { this->UpdateSpacePosition(); return true; }
	// An attached miner follows only the body lease it actually owns; another hull's attachment cannot move this vehicle.
	if (this->mission.phase == SpaceMissionPhase::Mining && body != nullptr && body->mining_slot == this->index) {
		this->position = {body->motion.position.x + this->mission.attachment_offset.x, body->motion.position.y + this->mission.attachment_offset.y};
	}
	if (stopped) { this->UpdateSpacePosition(); return true; }
	this->cur_speed = 0;
	if (TickSpaceOrders(*this)) { this->UpdateSpacePosition(); return true; }
	// Losing paid service or pod admission must end extraction without consuming another finite resource batch.
	if ((this->mission.phase == SpaceMissionPhase::Approach || this->mission.phase == SpaceMissionPhase::Mining) && GetSpaceMiningBlock(*this) != SpaceOrderBlock::None) {
		ReleaseSpaceMiningSlot(*this); this->mission.phase = SpaceMissionPhase::Holding; this->mission.approach_ticks_left = 0; this->UpdateSpacePosition(); return true;
	}
	// Starting a queued Mine may have changed the target this tick.
	body = GetMutableAsteroidBody(this->mission.target);
	switch (this->mission.phase) {
		case SpaceMissionPhase::Approach: {
			// Losing the finite target or spending the frozen pursuit allowance exits extraction while retaining every held packet for return.
			if (body == nullptr || body->remaining_reserve == 0 || this->mission.approach_ticks_left == 0) {
				ReleaseSpaceMiningSlot(*this); this->mission.phase = SpaceMissionPhase::Return; break;
			}
			// Retained old missions acquire no free attachment; new and migrated pursuits use the same saved shared reservation.
			if (!RequestSpaceApproach(*this, {SpaceOrderTargetKind::Body, body->id.base(), 0}, true)) break;
			SpacePoint approach;
			if (GetSpaceApproachDestination(*this, approach) != SpaceOrderBlock::None) {
				this->cur_speed = tuning.travel_tiles_per_day;
				MoveSpaceSurveyTowards(this->position, approach, this->mission.travel_remainder);
				break;
			}
			--this->mission.approach_ticks_left;
			const SpacePoint attachment{body->motion.position.x + this->mission.attachment_offset.x, body->motion.position.y + this->mission.attachment_offset.y};
			this->cur_speed = tuning.travel_tiles_per_day;
			MoveSpaceSurveyTowards(this->position, attachment, this->mission.travel_remainder);
			const uint64_t tolerance = tuning.approach_tolerance_q16;
			// Physical arrival and exclusive mining/survey availability must agree before converting the shared request into actual attachment.
			if (SpaceDistanceSquared(this->position, attachment) <= tolerance * tolerance && body->mining_slot == VehicleID::Invalid() && body->survey_slot == INVALID_SPACE_SURVEY) {
				this->position = attachment;
				body->mining_slot = this->index; this->mission.phase = SpaceMissionPhase::Mining; this->mission.approach_ticks_left = 0;
			} else if (this->mission.approach_ticks_left == 0) {
				this->mission.phase = SpaceMissionPhase::Return;
			}
			break;
		}
		case SpaceMissionPhase::Mining:
			// Extraction ends on lost custody, finite depletion or a full physical hold; none of these exits deletes mined cargo.
			if (body == nullptr || body->mining_slot != this->index || body->remaining_reserve == 0 || this->cargo.TotalCount() >= this->cargo_cap) {
				ReleaseSpaceMiningSlot(*this); this->mission.phase = SpaceMissionPhase::Return; break;
			}
			// Only the actual admitted mining role supplies extraction quantities; both sizes retain the same native finite reserve and packet transaction.
			if (miner == nullptr) { ReleaseSpaceMiningSlot(*this); this->mission.phase = SpaceMissionPhase::Holding; break; }
			if (++this->mission.work_ticks >= miner->extraction_ticks) {
				RecordSpaceWork(SpaceWorkKind::ExtractionAttempts); const auto extracted_before = this->extracted_units;
				// The shared native participant preflights complete output before reserve debit; refusal retains all material and the last earned partial interval for actual retry.
				if (!CommitSpaceMiningExtraction(*this, *body)) this->mission.work_ticks = miner->extraction_ticks - 1;
				else RecordSpaceWork(SpaceWorkKind::ExtractionUnits, this->extracted_units - extracted_before);
			}
			break;
		case SpaceMissionPhase::Return: {
			const auto *center = ResolveLaunchCenter(this->construction.home);
			// A cargo-only return to a real paid home needs only its inclusive drop region; empty and legacy coordinate-only recalls keep their precise destination.
			const bool cargo_return = this->cargo.TotalCount() != 0 && center != nullptr && center->owner == this->owner;
			const SpacePoint destination = cargo_return ? *GetLaunchCenterPlaneReference(center->launch_center->tile) : this->home;
			const auto departure = GetSpaceMiningDepartureDestination(*this, destination);
			if (!departure) break;
			if (*departure != this->position) {
				// Leave the original body through its shared clear staging policy before choosing circle entry. Only actual travel moves the retained hold; blocked geometry earns no free exit or cargo handoff.
				this->cur_speed = tuning.travel_tiles_per_day;
				MoveSpaceSurveyTowards(this->position, *departure, this->mission.travel_remainder);
				break;
			}
			if (cargo_return && IsInsideLaunchCenterDropZone(this->position, destination)) { this->mission.phase = SpaceMissionPhase::Holding; break; }
			this->cur_speed = tuning.travel_tiles_per_day;
			const bool arrived = MoveSpaceSurveyTowards(this->position, destination, this->mission.travel_remainder);
			// Stop on the same actual movement opportunity that enters the permitted circle, avoiding an extra step toward the service anchor.
			if (arrived || (cargo_return && IsInsideLaunchCenterDropZone(this->position, destination))) this->mission.phase = SpaceMissionPhase::Holding;
			break;
		}
		case SpaceMissionPhase::Idle:
		case SpaceMissionPhase::Holding: break; // Hold packets in orbit until an actual descent producer exists.
	}
	this->UpdateSpacePosition();
	return true;
}

/** @return Whether the historical P2 demonstration purchase completed; this explicit legacy fixture is not material-paid production or an ordinary-load fallback. */
bool PrepareNativeSpaceMiningDemo()
{
	// Historical explicit P2 preparation needs its own empty fleet and three actual bodies; ordinary loading cannot bootstrap this demonstration.
	if (!Company::IsValidID(_local_company) || GetAsteroidField().bodies.size() < 3 || GetSpaceVehicleCount() != 0) return false;
	const EngineID engine = GetSpaceProbeEngine();
	// Catalogue, pool and generation capacity must admit the entire historical purchase before any demonstration hull is created.
	if (!Engine::IsValidID(engine) || !Vehicle::CanAllocateItem(3) || UINT64_MAX - GetNextSpaceVehicleGeneration() < 3) return false;
	const Money total_cost = Money{40000} * 3; // Historical P2 demonstration purchase, never the production assembly path.
	if (GetAvailableMoney(_local_company) < total_cost) return false;
	AutoRestoreBackup company(_current_company, _local_company);
	if (!RetireSpaceSurveyPreview()) return false;
	// Issue three native hulls for this explicitly funded legacy demonstration only; actual material-paid launch production uses its separate owner.
	for (uint32_t i = 0; i < 3; ++i) {
		SpaceVehicle *vehicle = SpaceVehicle::Create();
		if (!RegisterSpaceVehicleIdentity(*vehicle)) { delete vehicle; return false; }
		vehicle->owner = _local_company;
		vehicle->engine_type = engine;
		vehicle->unitnumber = GetFreeUnitNumber(VehicleType::Space);
		Company::Get(_local_company)->freeunits[VehicleType::Space].UseID(vehicle->unitnumber);
		vehicle->cargo_cap = 20;
		vehicle->cargo_type = Engine::Get(engine)->GetDefaultCargoType();
		vehicle->vcache.cached_cargo_age_period = Ticks::CARGO_AGING_TICKS;
		vehicle->value = Money{40000}; // Preserve the explicitly staged P2 fixture convention.
		vehicle->build_year = TimerGameCalendar::year;
		vehicle->max_age = Engine::Get(engine)->GetLifeLengthInDays();
		vehicle->reliability = 0xFFFF;
		const auto target = GetAsteroidField().bodies[i].motion.position;
		vehicle->home = vehicle->position = {target.x - static_cast<int32_t>(4 + i) * SPACE_Q16, target.y - 6 * SPACE_Q16};
		vehicle->UpdateSpacePosition();
		GroupStatistics::CountEngine(vehicle, 1);
		GroupStatistics::CountVehicle(vehicle, 1);
		CmdSetSpaceMission(DoCommandFlag::Execute, vehicle->index, i);
	}
	SubtractMoneyFromCompany(_local_company, CommandCost(ExpensesType::NewVehicles, total_cost));
	return ValidateSpaceVehicles();
}

/** Audit asteroid provenance through actual vehicles, construction, Core stock, descent and Surface hauling.
 * @return Whether each extracted packet retains its original body/cargo identity in exactly one physical container.
 */
static bool ValidateAsteroidPacketCustody()
{
	const auto &field = GetAsteroidField(); std::set<CargoPacketID> owned;
	/** @param packet Actual packet in one inspected physical container. @param cargo That container's declared cargo.
     * @return Whether asteroid provenance is unique and retains its original valid resource identity.
     */
	auto add = [&](const CargoPacket *packet, CargoType cargo) {
		const auto source = packet->GetSource(); if (source.type != SourceType::Asteroid) return true;
		// No second container, missing natural source, wrong extraction plane or invented pickup station may legitimize an asteroid packet.
		if (!owned.insert(packet->index).second || source.id >= field.bodies.size() || !IsValidTile(packet->GetSourceTile()) ||
			GetTileLayer(packet->GetSourceTile()) != MapLayer::Space || packet->GetFirstStation() != StationID::Invalid()) return false;
		return cargo == GetCargoTypeByLabel(GetMultiLayerAsteroidDefinitions()[to_underlying(field.bodies[source.id].resource_class)].cargo);
	};
	/** @param list Actual station-style packet owner. @param cargo Declared cargo for this owner. @return Whether all routed fragments retain unique valid provenance. */
	auto station_list = [&](const StationCargoList &list, CargoType cargo) {
		// Inspect every routing bucket; a next-hop split cannot hide duplicated physical ownership.
		for (const auto &[hop, packets] : *list.Packets()) { (void)hop; for (const auto *packet : packets) if (!add(packet, cargo)) return false; } return true;
	};
	// All Surface inventory and handling owners participate in the same custody audit, preventing a returned packet from remaining in two containers.
	for (const auto *s : Station::Iterate()) {
		// Only existing goods data owns packets; inactive cargo slots must not create an inventory during validation.
		for (CargoType cargo : EnumRange(NUM_CARGO)) if (s->goods[cargo].HasData() && !station_list(s->goods[cargo].GetData().cargo, cargo)) return false;
		if (s->launch_hub) for (const auto &job : s->launch_hub->handling) if (!station_list(job->packets, job->cargo)) return false;
	}
	// Ordinary carriers, native supply compartments and paid descent shells all retain the same production provenance after transport.
	for (const auto *v : Vehicle::Iterate()) for (const auto *packet : *v->cargo.Packets()) if (!add(packet, v->cargo_type)) return false;
	for (const auto *v : SpaceVehicle::Iterate()) if (v->manifest) for (uint i = 0; i < SPACE_SUPPLY_COUNT; ++i) if (!station_list(v->manifest->supplies[i], GetSpaceSupplyCargo(static_cast<SpaceSupply>(i)))) return false;
	// Only a retained descent job owns return packets; a paid empty shell cannot supply a second cargo inventory.
	for (const auto *pod : SpacePod::Iterate()) if (pod->descent && !station_list(pod->descent->packets, pod->descent->cargo)) return false;
	// Asteroid provenance remains accountable when freight is stored in an actual completed Core, separately from job material.
	for (const auto *site : SpaceStructure::Iterate()) {
		for (uint i = 0; i < 2; ++i) if (!station_list(site->job.inputs[i], GetCargoTypeByLabel(GetSpaceConstructionDefinition(site->state.kind)->inputs[i].label))) return false;
		if (site->module) for (CargoType cargo : EnumRange(NUM_CARGO)) if (!station_list(site->module->stock[cargo], cargo)) return false;
		// Both contained service jobs retain their actual cargo provenance; a second Dock berth cannot hide a mislabelled asteroid packet.
		if (site->module) for (const auto *job : site->module->GetServiceJobs()) for (uint i = 0; i < 2; ++i) if (!station_list(job->inputs[i], GetSpaceSupplyCargo(static_cast<SpaceSupply>(i + 1)))) return false;
	}
	// Every physically allocated asteroid packet must have exactly one inspected owner; an unreferenced remainder is lost custody, not new reserve.
	for (const auto *packet : CargoPacket::Iterate()) if (packet->GetSource().type == SourceType::Asteroid && !owned.contains(packet->index)) return false;
	return true;
}

/** @return Whether every actual native hull, role, generation, queue, cargo owner and paid production dependency is consistent. */
bool ValidateSpaceVehicles()
{
	// Every independent native owner must validate before this fleet can borrow its bodies, paid receipts, queues or cargo dependencies.
	if (!ValidateAsteroidKnowledge() || !ValidateNativeSpaceOrders() || !ValidateSpaceProduction() || !ValidateLaunchOperations() || !ValidateSpaceManifests() || !ValidateSpacePods() || !ValidateSpaceTransitions() || !ValidateSpaceDescents() || !ValidateSpaceStructures()) return false;
	uint32_t count = 0;
	const auto &field = GetAsteroidField();
	std::set<CargoPacketID> packet_owners;
	// Audit each actual pool hull against bounded ownership, ordinary-order exclusion and deterministic physical mission/accounting fields.
	for (const auto *vehicle : SpaceVehicle::Iterate()) {
		if (!IsSpaceMiningStateSupported(GetSpaceVehicleModel(*vehicle), vehicle->mission.phase, vehicle->mission.work_ticks, vehicle->extracted_units, true)) return false;
		if (++count > MAX_SPACE_VEHICLES || !Company::IsValidID(vehicle->owner) || GetSpaceVehicleModel(*vehicle) == UINT16_MAX || vehicle->cargo.TotalCount() > vehicle->cargo_cap ||
			vehicle->orders != nullptr || !vehicle->current_order.IsType(OT_NOTHING) || vehicle->Previous() != nullptr || vehicle->Next() != nullptr || vehicle->PreviousShared() != nullptr || vehicle->NextShared() != nullptr || vehicle->mission.travel_remainder >= 74 || vehicle->mission.cost_remainder >= 74 ||
			vehicle->mission.work_ticks >= 74 || vehicle->mission.attachment_offset != SpacePoint{SPACE_Q16, -SPACE_Q16 / 2} || to_underlying(vehicle->mission.phase) > to_underlying(SpaceMissionPhase::Holding)) return false;
		if (vehicle->construction.placement != SpacePlacement::Orbital) continue;
		const auto model = GetSpaceVehicleModel(*vehicle);
		// Only delivered deployed roles may use their canonical freight capacity; legacy staged hulls cannot inherit a later survey fleet's authority.
		if ((model != 15 && model != 16 && model != 17 && model != 19 && model != 20 && model != SPACE_HEAVY_MINER_MODEL) || vehicle->cargo_cap != GetSpaceFreightCapacity(model, vehicle->cargo_type) ||
			(vehicle->construction.content_version == 0 && !field.surveys.empty())) return false;
		// Actual and remembered plane positions must stay inside the world; stacked coordinates cannot wrap a mission to another layer.
		for (auto point : {vehicle->position, vehicle->home}) if (point.x < 0 || point.y < 0 || point.x > static_cast<int32_t>(Map::SizeX() - 1) * SPACE_Q16 || point.y > static_cast<int32_t>(Map::SizeY() - 1) * SPACE_Q16) return false;
		const int x = static_cast<int64_t>(vehicle->position.x) * TILE_SIZE / SPACE_Q16 + TILE_SIZE / 2;
		const int y = static_cast<int64_t>(vehicle->position.y) * TILE_SIZE / SPACE_Q16 + TileY(TileXYL(0, 0, MapLayer::Space)) * TILE_SIZE + TILE_SIZE / 2;
		// Native viewport/tile projection must agree with the same physical point used by release, avoiding a different unloading provenance tile.
		if (vehicle->x_pos != x || vehicle->y_pos != y || vehicle->z_pos != 0 || vehicle->tile != TileVirtXY(x, y)) return false;
		const auto *body = GetMutableAsteroidBody(vehicle->mission.target);
		// Pursuit and attachment require their exact surviving body, remaining approach budget and actual exclusive lease/offset.
		if (vehicle->mission.phase == SpaceMissionPhase::Approach && (body == nullptr || vehicle->mission.approach_ticks_left == 0)) return false;
		// Attachment cannot survive a foreign lease or a position detached from the body's current motion.
		if (vehicle->mission.phase == SpaceMissionPhase::Mining && (body == nullptr || body->mining_slot != vehicle->index || vehicle->position != SpacePoint{body->motion.position.x + vehicle->mission.attachment_offset.x, body->motion.position.y + vehicle->mission.attachment_offset.y})) return false;
		// An active extraction/pursuit retains its body's actual resource label rather than changing an already loaded hold's meaning.
		if (body != nullptr && (vehicle->mission.phase == SpaceMissionPhase::Mining || vehicle->mission.phase == SpaceMissionPhase::Approach) &&
			vehicle->cargo_type != GetCargoTypeByLabel(GetMultiLayerAsteroidDefinitions()[to_underlying(body->resource_class)].cargo)) return false;
		// Check every actual held fragment, preserving the distinction between construction freight and finite asteroid extraction.
		for (const auto *packet : *vehicle->cargo.Packets()) {
			const auto source = packet->GetSource();
			// Surface-origin construction freight belongs only to a materially commissioned construction probe, never a legacy mining grant.
			if (!CanRetainSpaceCargoPacket(vehicle->construction, *packet)) return false;
			if (source.type != SourceType::Asteroid) continue; // Paid construction freight keeps its real Surface source; only actual asteroid packets enter the native resource checks below.
			if (!packet_owners.insert(packet->index).second) return false;
			// The source remains a valid natural body identity before its resource label can authorize held inventory.
			if (source.type != SourceType::Asteroid || source.id >= field.bodies.size()) return false;
			// Extraction keeps its actual Space production tile and never invents a first pickup station during descent or handling.
			if (!IsValidTile(packet->GetSourceTile()) || GetTileLayer(packet->GetSourceTile()) != MapLayer::Space || packet->GetFirstStation() != StationID::Invalid()) return false;
			if (vehicle->cargo_type != GetCargoTypeByLabel(GetMultiLayerAsteroidDefinitions()[to_underlying(field.bodies[source.id].resource_class)].cargo)) return false;
		}
		// A body lease cannot remain assigned after its hull exits Mining; recall/release must relinquish extraction separately from cargo custody.
		if (body != nullptr && body->mining_slot == vehicle->index && vehicle->mission.phase != SpaceMissionPhase::Mining) return false;
	}
	// Every assigned natural lease must resolve back to the same active native miner, closing the reverse ownership check.
	for (const auto &body : field.bodies) if (body.mining_slot != VehicleID::Invalid()) {
		const auto *vehicle = SpaceVehicle::GetIfValid(body.mining_slot);
		// Missing hulls, inactive extraction or a different target cannot leave an actionable slot behind.
		if (vehicle == nullptr || vehicle->mission.phase != SpaceMissionPhase::Mining || vehicle->mission.target != body.id) return false;
	}
	return ValidateOrbitalManufacturingProductionLinks(CaptureSpaceStructures(), CaptureSpaceProduction(), GetNextSpaceStructureGeneration(), GetNextSpaceVehicleGeneration()) && ValidateAsteroidPacketCustody();
}

/** @return Deterministic current native fleet/accounting digest for interrupted replay and pure-quote validation; rendering grants no ownership. */
uint64_t ComputeSpaceVehicleDigest()
{
	uint64_t digest = 14695981039346656037ULL;
	/** @param value Canonical integer encoded in fixed byte order so replay digests do not depend on host representation. */
	auto hash = [&](uint64_t value) { for (int shift = 56; shift >= 0; shift -= 8) { digest ^= static_cast<uint8_t>(value >> shift); digest *= 1099511628211ULL; } };
	/** @param p Actual saved plane point; both coordinates enter the same deterministic digest. */
	auto point = [&](SpacePoint p) { hash(static_cast<uint32_t>(p.x)); hash(static_cast<uint32_t>(p.y)); };
	// Include each hull's actual physical mission, ownership and financial history in stable native pool order.
	for (const auto *vehicle : SpaceVehicle::Iterate()) {
		hash(vehicle->index.base()); hash(vehicle->owner.base()); hash(vehicle->engine_type.base()); hash(vehicle->unitnumber);
		point(vehicle->position); point(vehicle->home); point(vehicle->mission.attachment_offset);
		hash(vehicle->mission.target.base()); hash(to_underlying(vehicle->mission.phase)); hash(vehicle->mission.approach_ticks_left);
		hash(vehicle->mission.work_ticks); hash(vehicle->mission.travel_remainder); hash(vehicle->mission.cost_remainder); hash(vehicle->extracted_units);
		hash(vehicle->cargo_type); hash(vehicle->cargo_cap); hash(vehicle->cargo_age_counter); hash(vehicle->cargo.TotalCount());
		hash(static_cast<int64_t>(vehicle->profit_this_year)); hash(static_cast<int64_t>(Company::Get(vehicle->owner)->money));
		// Packet identity, age and feeder/source history make cloned/reset cargo visible even when the hold quantity matches.
		for (const auto *packet : *vehicle->cargo.Packets()) {
			hash(packet->index.base()); hash(packet->Count()); hash(packet->GetPeriodsInTransit()); hash(to_underlying(packet->GetSource().type)); hash(packet->GetSource().id);
			hash(packet->GetFirstStation().base()); hash(static_cast<int64_t>(packet->GetFeederShare()));
			hash(packet->GetSourceTile().base());
		}
	}
	hash(ComputeSpaceOrdersDigest());
	hash(ComputeLaunchProductionDigest());
	hash(ComputeSpaceManifestDigest());
	hash(ComputeSpaceTransitionDigest());
	hash(ComputeSpaceDescentDigest());
	return digest;
}
