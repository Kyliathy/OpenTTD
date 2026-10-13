/*
 * This file is part of OpenTTD, licensed under the GNU GPL version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file space_vehicle.cpp Native orbital probe ownership, deterministic pursuit and conserved extraction.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group1; retained asteroid cargo in Core stock.
 */
#include "stdafx.h"
#include "space_vehicle.h"
#include "space_manifest.h"
#include "space_pod.h"
#include "space_structure.h"
#include "orbital_service.h"
#include "space_transition.h"
#include "launch_operations.h"
#include "multilayer_space_content.h"
#include "space_vehicle_cmd.h"
#include "command_func.h"
#include "asteroid.h"
#include "engine_base.h"
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

SpaceVehicle::SpaceVehicle(VehicleID index) : SpecializedVehicleBase(index) {}

uint32_t GetSpaceVehicleCount()
{
	uint32_t count = 0; for ([[maybe_unused]] const auto *vehicle : SpaceVehicle::Iterate()) ++count; return count;
}

EngineID GetSpaceEngine(uint16_t model)
{
	if (model != 14 && model != 15 && model != 19 && model != 20) return EngineID::Invalid();
	const auto &required = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content;
	return _engine_mngr.GetID(VehicleType::Space, 0xF000 + model, required.package.grfid);
}
EngineID GetSpaceProbeEngine() { return GetSpaceEngine(15); }
uint16_t GetSpaceVehicleModel(const SpaceVehicle &vehicle)
{
	const auto *engine = Engine::GetIfValid(vehicle.engine_type);
	if (engine == nullptr || engine->type != VehicleType::Space) return UINT16_MAX;
	const auto model = engine->VehInfo<SpaceVehicleInfo>().model;
	return GetSpaceEngine(model) == engine->index ? model : UINT16_MAX;
}

bool EnsureSpaceEngineDefinition(uint16_t model)
{
	if (model != 14 && model != 15 && model != 19 && model != 20) return false;
	const auto *definition = GetSpaceVehicleConstruction(model);
	const auto &required = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content;
	if (definition == nullptr || CheckMultiLayerContentIdentity(required, _grfconfig) != MultiLayerContentIdentityError::None) return false;
	BuildCargoLabelMap();
	const CargoLabel label = model == 14 || model == 20 ? CargoLabel{"STEL"} : CargoLabel{"NORE"};
	const CargoType cargo = GetCargoTypeByLabel(label);
	if (!IsValidCargoType(cargo)) return false;
	const uint16_t local = 0xF000 + model;
	const auto id = GetSpaceEngine(model);
	Engine *engine = Engine::GetIfValid(id);
	if (engine != nullptr && (engine->type != VehicleType::Space || engine->grf_prop.local_id != local)) return false;
	if (engine == nullptr) {
		if (id == EngineID::Invalid()) {
			if (!Engine::CanAllocateItem()) return false;
			engine = Engine::Create(VehicleType::Space, local);
			_engine_mngr.SetID(VehicleType::Space, local, required.package.grfid, 0, engine->index);
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
	engine->name = model == 14 ? "Cargo rocket" : model == 15 ? "Mining probe" : model == 19 ? "Survey probe" : "Construction probe";
	auto &info = engine->VehInfo<SpaceVehicleInfo>();
	info.model = model; info.capacity = model == 14 ? 30 : model == 19 ? 0 : 20; info.purchase_cost = definition->capital;
	engine->reliability = 0xFFFF;
	return true;
}

bool InitialiseSpaceEngineDefinition()
{
	if (!EnsureSpaceEngineDefinition(15)) return false;
	// Restore already saved mappings without appending unsolicited engines to old worlds.
	for (uint16_t model : {14, 19, 20}) if (GetSpaceEngine(model) != EngineID::Invalid() && !EnsureSpaceEngineDefinition(model)) return false;
	return true;
}

void ReleaseSpaceMiningSlot(SpaceVehicle &vehicle)
{
	auto *body = GetMutableAsteroidBody(vehicle.mission.target);
	if (body != nullptr && body->mining_slot == vehicle.index) body->mining_slot = VehicleID::Invalid();
	vehicle.mission.work_ticks = 0;
}

SpaceVehicle::~SpaceVehicle()
{
	ForgetSpaceManifest(*this, CleaningPool());
	if (!CleaningPool()) { ReleaseSpaceMiningSlot(*this); ForgetLaunchCraft(*this); }
	this->PreDestructor();
}

/** Round distance upwards; freeze the adaptive deadline when assigning, never extend it during pursuit. */
static uint32_t ApproachDeadline(SpacePoint position, SpacePoint target)
{
	const uint64_t squared = SpaceDistanceSquared(position, target);
	uint64_t lo = 0, hi = 1ULL << 30;
	while (lo < hi) {
		const uint64_t mid = (lo + hi) / 2;
		if (mid * mid < squared) lo = mid + 1; else hi = mid;
	}
	return static_cast<uint32_t>(std::max<uint64_t>(1480, (lo * 74 + 3 * SPACE_Q16 - 1) / (3 * SPACE_Q16) + 148));
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
	if (vehicle->construction.placement != SpacePlacement::Orbital || (vehicle->manifest && vehicle->manifest->service_ticks != 0) || vehicle->engine_type != GetSpaceProbeEngine() || vehicle->vehstatus.Test(VehState::Crashed) || GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return CMD_ERROR;
	const bool recall = target == INVALID_ORBITAL_BODY.base();
	if (!recall && GetSpaceMiningBlock(*vehicle) != SpaceOrderBlock::None) return CMD_ERROR;
	const auto *body = recall ? nullptr : GetMutableAsteroidBody(OrbitalBodyID{target});
	if (!recall && (body == nullptr || body->remaining_reserve == 0 || vehicle->cargo.TotalCount() >= vehicle->cargo_cap)) return CMD_ERROR;
	const CargoType cargo = body == nullptr ? vehicle->cargo_type : GetCargoTypeByLabel(GetMultiLayerAsteroidDefinitions()[to_underlying(body->resource_class)].cargo);
	if (!IsValidCargoType(cargo) || (!recall && vehicle->cargo.TotalCount() != 0 && vehicle->cargo_type != cargo)) return CMD_ERROR;
	if (execute) {
		ReleaseSpaceMiningSlot(*vehicle);
		vehicle->mission.target = recall ? INVALID_ORBITAL_BODY : body->id;
		vehicle->mission.phase = recall ? SpaceMissionPhase::Return : SpaceMissionPhase::Approach;
		vehicle->mission.approach_ticks_left = recall ? 0 : ApproachDeadline(vehicle->position, body->motion.position);
		vehicle->cargo_type = cargo;
		vehicle->vehstatus.Reset(VehState::Stopped);
		vehicle->MarkDirty();
	}
	return CommandCost{};
}

CommandCost CmdSetSpaceMission(DoCommandFlags flags, VehicleID id, uint32_t target)
{
	auto *vehicle = SpaceVehicle::GetIfValid(id);
	if (vehicle == nullptr || vehicle->owner != _current_company) return CMD_ERROR;
	const auto result = AssignSpaceMiningMission(*vehicle, target, flags.Test(DoCommandFlag::Execute));
	if (result.Succeeded() && flags.Test(DoCommandFlag::Execute)) vehicle->space_orders = {};
	return result;
}

void SpaceVehicle::GetImage([[maybe_unused]] Direction direction, [[maybe_unused]] EngineImageType image_type, VehicleSpriteSeq *result) const
{
	result->Set(SPR_SPACE_PREVIEW_PROBE + (this->mission.phase == SpaceMissionPhase::Mining ? 1 : 0));
}
void SpaceVehicle::UpdateDeltaXY() { this->bounds = {{-4, -4, 0}, {8, 8, 8}, {0, 0, 0}}; }
void SpaceVehicle::MarkDirty() { this->UpdateViewport(true, true); SetWindowClassesDirty(WindowClass::SpaceSurvey); }
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
Money SpaceVehicle::GetRunningCost() const { if (this->construction.placement != SpacePlacement::Orbital) return 0; return (IsSpaceMiningWorkTick(*this) || IsSpaceSurveyWorkTick(*this) || IsSpaceServiceWorkTick(*this) || IsOrbitalServiceWorkTick(*this) || IsSpaceConstructionWorkTick(*this) ? 500 : 100) * CalendarTime::DAYS_IN_YEAR * 256; }
void SpaceVehicle::OnNewCalendarDay() { AgeVehicle(this); }
void SpaceVehicle::OnNewEconomyDay()
{
	EconomyAgeVehicle(this);
	if ((++this->day_counter & 7) == 0) DecreaseVehicleValue(this);
}

/** @return Whether this hull survives its native synchronized tick; held service craft accrue only actual eligible upkeep. */
bool SpaceVehicle::Tick()
{
	if (this->construction.placement != SpacePlacement::Orbital) return true;
	++this->tick_counter;
	auto *body = GetMutableAsteroidBody(this->mission.target);
	const bool stopped = this->vehstatus.Test(VehState::Stopped);
	const bool mining = IsSpaceMiningWorkTick(*this);
	const bool servicing = IsSpaceServiceWorkTick(*this) || IsOrbitalServiceWorkTick(*this);
	const uint16_t accrual = this->mission.cost_remainder + (mining || servicing || IsSpaceSurveyWorkTick(*this) || IsSpaceConstructionWorkTick(*this) ? 500 : 100);
	const uint16_t cost = accrual / 74;
	this->mission.cost_remainder = accrual % 74;
	SubtractMoneyFromCompany(this->owner, CommandCost(ExpensesType::Other, cost));
	this->profit_this_year -= Money{cost} * 256;
	// A waiting/paused Core job keeps its craft at the reserved berth while charging only the predicate's actual upkeep.
	if (HasOrbitalServiceAssignment(*this) || servicing || (this->manifest && this->manifest->service_ticks != 0)) { this->UpdateSpacePosition(); return true; }
	if (this->mission.phase == SpaceMissionPhase::Mining && body != nullptr && body->mining_slot == this->index) {
		this->position = {body->motion.position.x + this->mission.attachment_offset.x, body->motion.position.y + this->mission.attachment_offset.y};
	}
	if (stopped) { this->UpdateSpacePosition(); return true; }
	this->cur_speed = 0;
	if (TickSpaceOrders(*this)) { this->UpdateSpacePosition(); return true; }
	if ((this->mission.phase == SpaceMissionPhase::Approach || this->mission.phase == SpaceMissionPhase::Mining) && GetSpaceMiningBlock(*this) != SpaceOrderBlock::None) {
		ReleaseSpaceMiningSlot(*this); this->mission.phase = SpaceMissionPhase::Holding; this->mission.approach_ticks_left = 0; this->UpdateSpacePosition(); return true;
	}
	// Starting a queued Mine may have changed the target this tick.
	body = GetMutableAsteroidBody(this->mission.target);
	switch (this->mission.phase) {
		case SpaceMissionPhase::Approach: {
			if (body == nullptr || body->remaining_reserve == 0 || this->mission.approach_ticks_left == 0) {
				ReleaseSpaceMiningSlot(*this); this->mission.phase = SpaceMissionPhase::Return; break;
			}
			--this->mission.approach_ticks_left;
			const SpacePoint attachment{body->motion.position.x + this->mission.attachment_offset.x, body->motion.position.y + this->mission.attachment_offset.y};
			this->cur_speed = 4;
			if (MoveSpaceSurveyTowards(this->position, attachment, this->mission.travel_remainder) && body->mining_slot == VehicleID::Invalid() && body->survey_slot == INVALID_SPACE_SURVEY) {
				body->mining_slot = this->index; this->mission.phase = SpaceMissionPhase::Mining; this->mission.approach_ticks_left = 0;
			} else if (this->mission.approach_ticks_left == 0) {
				this->mission.phase = SpaceMissionPhase::Return;
			}
			break;
		}
		case SpaceMissionPhase::Mining:
			if (body == nullptr || body->mining_slot != this->index || body->remaining_reserve == 0 || this->cargo.TotalCount() >= this->cargo_cap) {
				ReleaseSpaceMiningSlot(*this); this->mission.phase = SpaceMissionPhase::Return; break;
			}
			if (++this->mission.work_ticks >= 74) {
				const uint16_t amount = std::min<uint32_t>({2, body->remaining_reserve, this->cargo_cap - this->cargo.TotalCount()});
				if (!CargoPacket::CanAllocateItem()) { this->mission.work_ticks = 73; break; }
				CargoPacket *packet = CargoPacket::Create(StationID::Invalid(), amount, Source{static_cast<SourceID>(body->id.base()), SourceType::Asteroid});
				const TileIndex extraction_tile = TileXYL((body->motion.position.x + SPACE_Q16 / 2) / SPACE_Q16, (body->motion.position.y + SPACE_Q16 / 2) / SPACE_Q16, MapLayer::Space);
				packet->UpdateLoadingTile(extraction_tile); // Current body coordinates; before Append, which may merge/delete the packet.
				this->cargo.Append(packet);
				body->remaining_reserve -= amount; this->extracted_units += amount; this->mission.work_ticks = 0;
				if (body->remaining_reserve == 0 || this->cargo.TotalCount() == this->cargo_cap) {
					ReleaseSpaceMiningSlot(*this); this->mission.phase = SpaceMissionPhase::Return;
				}
			}
			break;
		case SpaceMissionPhase::Return:
			this->cur_speed = 4;
			if (MoveSpaceSurveyTowards(this->position, this->home, this->mission.travel_remainder)) this->mission.phase = SpaceMissionPhase::Holding;
			break;
		case SpaceMissionPhase::Idle:
		case SpaceMissionPhase::Holding: break; // Hold packets in orbit until an actual descent producer exists.
	}
	this->UpdateSpacePosition();
	return true;
}

bool PrepareNativeSpaceMiningDemo()
{
	if (!Company::IsValidID(_local_company) || GetAsteroidField().bodies.size() < 3 || GetSpaceVehicleCount() != 0) return false;
	const EngineID engine = GetSpaceProbeEngine();
	if (!Engine::IsValidID(engine) || !Vehicle::CanAllocateItem(3) || UINT64_MAX - GetNextSpaceVehicleGeneration() < 3) return false;
	const Money total_cost = Money{40000} * 3; // Historical P2 demonstration purchase, never the production assembly path.
	if (GetAvailableMoney(_local_company) < total_cost) return false;
	AutoRestoreBackup company(_current_company, _local_company);
	if (!RetireSpaceSurveyPreview()) return false;
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
	auto add = [&](const CargoPacket *packet, CargoType cargo) {
		const auto source = packet->GetSource(); if (source.type != SourceType::Asteroid) return true;
		if (!owned.insert(packet->index).second || source.id >= field.bodies.size() || !IsValidTile(packet->GetSourceTile()) ||
			GetTileLayer(packet->GetSourceTile()) != MapLayer::Space || packet->GetFirstStation() != StationID::Invalid()) return false;
		return cargo == GetCargoTypeByLabel(GetMultiLayerAsteroidDefinitions()[to_underlying(field.bodies[source.id].resource_class)].cargo);
	};
	auto station_list = [&](const StationCargoList &list, CargoType cargo) {
		for (const auto &[hop, packets] : *list.Packets()) { (void)hop; for (const auto *packet : packets) if (!add(packet, cargo)) return false; } return true;
	};
	for (const auto *s : Station::Iterate()) {
		for (CargoType cargo : EnumRange(NUM_CARGO)) if (s->goods[cargo].HasData() && !station_list(s->goods[cargo].GetData().cargo, cargo)) return false;
		if (s->launch_hub) for (const auto &job : s->launch_hub->handling) if (!station_list(job->packets, job->cargo)) return false;
	}
	for (const auto *v : Vehicle::Iterate()) for (const auto *packet : *v->cargo.Packets()) if (!add(packet, v->cargo_type)) return false;
	for (const auto *v : SpaceVehicle::Iterate()) if (v->manifest) for (uint i = 0; i < SPACE_SUPPLY_COUNT; ++i) if (!station_list(v->manifest->supplies[i], GetSpaceSupplyCargo(static_cast<SpaceSupply>(i)))) return false;
	for (const auto *pod : SpacePod::Iterate()) if (pod->descent && !station_list(pod->descent->packets, pod->descent->cargo)) return false;
	// Asteroid provenance remains accountable when freight is stored in an actual completed Core, separately from job material.
	for (const auto *site : SpaceStructure::Iterate()) {
		for (uint i = 0; i < 2; ++i) if (!station_list(site->job.inputs[i], GetCargoTypeByLabel(GetSpaceConstructionDefinition(site->state.kind)->inputs[i].label))) return false;
		if (site->module) for (CargoType cargo : EnumRange(NUM_CARGO)) if (!station_list(site->module->stock[cargo], cargo)) return false;
		if (site->module) for (uint i = 0; i < 2; ++i) if (!station_list(site->module->service.inputs[i], GetSpaceSupplyCargo(static_cast<SpaceSupply>(i + 1)))) return false;
	}
	for (const auto *packet : CargoPacket::Iterate()) if (packet->GetSource().type == SourceType::Asteroid && !owned.contains(packet->index)) return false;
	return true;
}

bool ValidateSpaceVehicles()
{
	if (!ValidateAsteroidKnowledge() || !ValidateNativeSpaceOrders() || !ValidateSpaceProduction() || !ValidateLaunchOperations() || !ValidateSpaceManifests() || !ValidateSpacePods() || !ValidateSpaceTransitions() || !ValidateSpaceDescents() || !ValidateSpaceStructures()) return false;
	uint32_t count = 0;
	const auto &field = GetAsteroidField();
	std::set<CargoPacketID> packet_owners;
	for (const auto *vehicle : SpaceVehicle::Iterate()) {
		if (++count > MAX_SPACE_VEHICLES || !Company::IsValidID(vehicle->owner) || GetSpaceVehicleModel(*vehicle) == UINT16_MAX || vehicle->cargo.TotalCount() > vehicle->cargo_cap ||
			vehicle->orders != nullptr || !vehicle->current_order.IsType(OT_NOTHING) || vehicle->Previous() != nullptr || vehicle->Next() != nullptr || vehicle->PreviousShared() != nullptr || vehicle->NextShared() != nullptr || vehicle->mission.travel_remainder >= 74 || vehicle->mission.cost_remainder >= 74 ||
			vehicle->mission.work_ticks >= 74 || vehicle->mission.attachment_offset != SpacePoint{SPACE_Q16, -SPACE_Q16 / 2} || to_underlying(vehicle->mission.phase) > to_underlying(SpaceMissionPhase::Holding)) return false;
		if (vehicle->construction.placement != SpacePlacement::Orbital) continue;
		const auto model = GetSpaceVehicleModel(*vehicle);
		if ((model != 15 && model != 19 && model != 20) || vehicle->cargo_cap != GetSpaceFreightCapacity(model, vehicle->cargo_type) ||
			(vehicle->construction.content_version == 0 && !field.surveys.empty())) return false;
		for (auto point : {vehicle->position, vehicle->home}) if (point.x < 0 || point.y < 0 || point.x > static_cast<int32_t>(Map::SizeX() - 1) * SPACE_Q16 || point.y > static_cast<int32_t>(Map::SizeY() - 1) * SPACE_Q16) return false;
		const int x = static_cast<int64_t>(vehicle->position.x) * TILE_SIZE / SPACE_Q16 + TILE_SIZE / 2;
		const int y = static_cast<int64_t>(vehicle->position.y) * TILE_SIZE / SPACE_Q16 + TileY(TileXYL(0, 0, MapLayer::Space)) * TILE_SIZE + TILE_SIZE / 2;
		if (vehicle->x_pos != x || vehicle->y_pos != y || vehicle->z_pos != 0 || vehicle->tile != TileVirtXY(x, y)) return false;
		const auto *body = GetMutableAsteroidBody(vehicle->mission.target);
		if (vehicle->mission.phase == SpaceMissionPhase::Approach && (body == nullptr || vehicle->mission.approach_ticks_left == 0)) return false;
		if (vehicle->mission.phase == SpaceMissionPhase::Mining && (body == nullptr || body->mining_slot != vehicle->index || vehicle->position != SpacePoint{body->motion.position.x + vehicle->mission.attachment_offset.x, body->motion.position.y + vehicle->mission.attachment_offset.y})) return false;
		if (body != nullptr && (vehicle->mission.phase == SpaceMissionPhase::Mining || vehicle->mission.phase == SpaceMissionPhase::Approach) &&
			vehicle->cargo_type != GetCargoTypeByLabel(GetMultiLayerAsteroidDefinitions()[to_underlying(body->resource_class)].cargo)) return false;
		for (const auto *packet : *vehicle->cargo.Packets()) {
			const auto source = packet->GetSource();
			if (source.type != SourceType::Asteroid) { if (model != 20 || vehicle->construction.content_version == 0) return false; continue; }
			if (!packet_owners.insert(packet->index).second) return false;
			if (source.type != SourceType::Asteroid || source.id >= field.bodies.size()) return false;
			if (!IsValidTile(packet->GetSourceTile()) || GetTileLayer(packet->GetSourceTile()) != MapLayer::Space || packet->GetFirstStation() != StationID::Invalid()) return false;
			if (vehicle->cargo_type != GetCargoTypeByLabel(GetMultiLayerAsteroidDefinitions()[to_underlying(field.bodies[source.id].resource_class)].cargo)) return false;
		}
		if (body != nullptr && body->mining_slot == vehicle->index && vehicle->mission.phase != SpaceMissionPhase::Mining) return false;
	}
	for (const auto &body : field.bodies) if (body.mining_slot != VehicleID::Invalid()) {
		const auto *vehicle = SpaceVehicle::GetIfValid(body.mining_slot);
		if (vehicle == nullptr || vehicle->mission.phase != SpaceMissionPhase::Mining || vehicle->mission.target != body.id) return false;
	}
	return ValidateAsteroidPacketCustody();
}

uint64_t ComputeSpaceVehicleDigest()
{
	uint64_t digest = 14695981039346656037ULL;
	auto hash = [&](uint64_t value) { for (int shift = 56; shift >= 0; shift -= 8) { digest ^= static_cast<uint8_t>(value >> shift); digest *= 1099511628211ULL; } };
	auto point = [&](SpacePoint p) { hash(static_cast<uint32_t>(p.x)); hash(static_cast<uint32_t>(p.y)); };
	for (const auto *vehicle : SpaceVehicle::Iterate()) {
		hash(vehicle->index.base()); hash(vehicle->owner.base()); hash(vehicle->engine_type.base()); hash(vehicle->unitnumber);
		point(vehicle->position); point(vehicle->home); point(vehicle->mission.attachment_offset);
		hash(vehicle->mission.target.base()); hash(to_underlying(vehicle->mission.phase)); hash(vehicle->mission.approach_ticks_left);
		hash(vehicle->mission.work_ticks); hash(vehicle->mission.travel_remainder); hash(vehicle->mission.cost_remainder); hash(vehicle->extracted_units);
		hash(vehicle->cargo_type); hash(vehicle->cargo_cap); hash(vehicle->cargo_age_counter); hash(vehicle->cargo.TotalCount());
		hash(static_cast<int64_t>(vehicle->profit_this_year)); hash(static_cast<int64_t>(Company::Get(vehicle->owner)->money));
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
