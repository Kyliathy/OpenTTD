/*
 * This file is part of OpenTTD, licensed under the GNU GPL version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */
/** @file space_vehicle.cpp Native orbital probe ownership, deterministic pursuit and conserved extraction. */
#include "stdafx.h"
#include "space_vehicle.h"
#include "space_vehicle_cmd.h"
#include "command_func.h"
#include "asteroid.h"
#include "engine_base.h"
#include "cargo_type.h"
#include "cargotype.h"
#include "company_base.h"
#include "company_func.h"
#include "core/backup_type.hpp"
#include "group.h"
#include "map_func.h"
#include "multilayer_state.h"
#include "vehicle_func.h"
#include "window_func.h"
#include "table/strings.h"
#include <set>
#include "safeguards.h"

static constexpr uint16_t PROBE_ENGINE_LOCAL_ID = 0xF00F;

uint32_t GetSpaceVehicleCount()
{
	uint32_t count = 0; for ([[maybe_unused]] const auto *vehicle : SpaceVehicle::Iterate()) ++count; return count;
}

EngineID GetSpaceProbeEngine()
{
	const auto &required = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content;
	return _engine_mngr.GetID(VehicleType::Space, PROBE_ENGINE_LOCAL_ID, required.package.grfid);
}

bool InitialiseSpaceEngineDefinition()
{
	const auto &required = *GetMultiLayerContentProfile(MultiLayerGameplayProfileID::MultilayerIndustry)->required_content;
	if (CheckMultiLayerContentIdentity(required, _grfconfig) != MultiLayerContentIdentityError::None) return false;
	BuildCargoLabelMap();
	const CargoType cargo = GetCargoTypeByLabel(CargoLabel{"NORE"});
	if (!IsValidCargoType(cargo)) return false;
	auto id = GetSpaceProbeEngine();
	Engine *engine = Engine::GetIfValid(id);
	if (engine != nullptr && (engine->type != VehicleType::Space || engine->grf_prop.local_id != PROBE_ENGINE_LOCAL_ID)) return false;
	if (engine == nullptr) {
		if (id == EngineID::Invalid()) {
			if (!Engine::CanAllocateItem()) return false;
			engine = Engine::Create(VehicleType::Space, PROBE_ENGINE_LOCAL_ID);
			_engine_mngr.SetID(VehicleType::Space, PROBE_ENGINE_LOCAL_ID, required.package.grfid, 0, engine->index);
		} else {
			engine = Engine::CreateAtIndex(id, VehicleType::Space, PROBE_ENGINE_LOCAL_ID);
		}
	}
	engine->info.base_intro = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1990}, 0, 1);
	engine->intro_date = engine->info.base_intro;
	engine->info.base_life = TimerGameCalendar::Year{0xFF};
	engine->info.lifelength = TimerGameCalendar::Year{30};
	engine->info.climates = {LandscapeType::Temperate};
	engine->info.cargo_type = cargo;
	engine->info.cargo_label = CargoLabel{"NORE"};
	engine->info.cargo_age_period = Ticks::CARGO_AGING_TICKS;
	engine->info.misc_flags.Set(EngineMiscFlag::NoDefaultCargoMultiplier);
	engine->info.extra_flags.Set({ExtraEngineFlag::NoNews, ExtraEngineFlag::NoPreview});
	engine->info.string_id = STR_JUST_RAW_STRING;
	engine->name = "Mining probe";
	engine->reliability = 0xFFFF;
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
	if (!CleaningPool()) ReleaseSpaceMiningSlot(*this);
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

CommandCost CmdSetSpaceMission(DoCommandFlags flags, VehicleID id, uint32_t target)
{
	auto *vehicle = SpaceVehicle::GetIfValid(id);
	if (vehicle == nullptr || vehicle->owner != _current_company || vehicle->vehstatus.Test(VehState::Crashed) || GetCurrentMultiLayerProfileSelection().id != MultiLayerGameplayProfileID::MultilayerIndustry) return CMD_ERROR;
	const bool recall = target == INVALID_ORBITAL_BODY.base();
	const auto *body = recall ? nullptr : GetMutableAsteroidBody(OrbitalBodyID{target});
	if (!recall && (body == nullptr || body->remaining_reserve == 0 || vehicle->cargo.TotalCount() >= vehicle->cargo_cap)) return CMD_ERROR;
	const CargoType cargo = body == nullptr ? vehicle->cargo_type : GetCargoTypeByLabel(GetMultiLayerAsteroidDefinitions()[to_underlying(body->resource_class)].cargo);
	if (!IsValidCargoType(cargo) || (!recall && vehicle->cargo.TotalCount() != 0 && vehicle->cargo_type != cargo)) return CMD_ERROR;
	if (flags.Test(DoCommandFlag::Execute)) {
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

void SpaceVehicle::GetImage([[maybe_unused]] Direction direction, [[maybe_unused]] EngineImageType image_type, VehicleSpriteSeq *result) const
{
	result->Set(SPR_SPACE_PREVIEW_PROBE + (this->mission.phase == SpaceMissionPhase::Mining ? 1 : 0));
}
void SpaceVehicle::UpdateDeltaXY() { this->bounds = {{-4, -4, 0}, {8, 8, 8}, {0, 0, 0}}; }
void SpaceVehicle::MarkDirty() { this->UpdateViewport(true, true); SetWindowClassesDirty(WindowClass::SpaceSurvey); }
void SpaceVehicle::UpdateSpacePosition()
{
	const TileIndex origin = TileXYL(0, 0, MapLayer::Space);
	this->x_pos = static_cast<int64_t>(this->position.x) * TILE_SIZE / SPACE_Q16 + TILE_SIZE / 2;
	this->y_pos = static_cast<int64_t>(this->position.y) * TILE_SIZE / SPACE_Q16 + TileY(origin) * TILE_SIZE + TILE_SIZE / 2;
	this->z_pos = 0;
	this->tile = TileVirtXY(this->x_pos, this->y_pos);
	this->direction = Direction::NE;
	this->UpdateDeltaXY();
	this->GetImage(this->direction, EngineImageType::OnMap, &this->sprite_cache.sprite_seq);
	this->UpdatePositionAndViewport();
}

Money SpaceVehicle::GetRunningCost() const { return (this->mission.phase == SpaceMissionPhase::Mining ? 500 : 100) * CalendarTime::DAYS_IN_YEAR * 256; }
void SpaceVehicle::OnNewCalendarDay() { AgeVehicle(this); }
void SpaceVehicle::OnNewEconomyDay()
{
	EconomyAgeVehicle(this);
	if ((++this->day_counter & 7) == 0) DecreaseVehicleValue(this);
}

bool SpaceVehicle::Tick()
{
	++this->tick_counter;
	auto *body = GetMutableAsteroidBody(this->mission.target);
	const bool stopped = this->vehstatus.Test(VehState::Stopped);
	const bool mining = this->mission.phase == SpaceMissionPhase::Mining && !stopped;
	const uint16_t accrual = this->mission.cost_remainder + (mining ? 500 : 100);
	const uint16_t cost = accrual / 74;
	this->mission.cost_remainder = accrual % 74;
	SubtractMoneyFromCompany(this->owner, CommandCost(ExpensesType::Other, cost));
	this->profit_this_year -= Money{cost} * 256;
	if (this->mission.phase == SpaceMissionPhase::Mining && body != nullptr && body->mining_slot == this->index) {
		this->position = {body->motion.position.x + this->mission.attachment_offset.x, body->motion.position.y + this->mission.attachment_offset.y};
	}
	if (stopped) { this->UpdateSpacePosition(); return true; }
	this->cur_speed = 0;
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
	if (!Engine::IsValidID(engine) || !Vehicle::CanAllocateItem(3)) return false;
	const Money total_cost = Engine::Get(engine)->GetCost() * 3;
	if (GetAvailableMoney(_local_company) < total_cost) return false;
	AutoRestoreBackup company(_current_company, _local_company);
	if (!RetireSpaceSurveyPreview()) return false;
	for (uint32_t i = 0; i < 3; ++i) {
		SpaceVehicle *vehicle = SpaceVehicle::Create();
		vehicle->owner = _local_company;
		vehicle->engine_type = engine;
		vehicle->unitnumber = GetFreeUnitNumber(VehicleType::Space);
		Company::Get(_local_company)->freeunits[VehicleType::Space].UseID(vehicle->unitnumber);
		vehicle->cargo_cap = 20;
		vehicle->cargo_type = Engine::Get(engine)->GetDefaultCargoType();
		vehicle->vcache.cached_cargo_age_period = Ticks::CARGO_AGING_TICKS;
		vehicle->value = Engine::Get(engine)->GetCost();
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

bool ValidateSpaceVehicles()
{
	uint32_t count = 0;
	const auto &field = GetAsteroidField();
	std::set<CargoPacketID> packet_owners;
	for (const auto *vehicle : SpaceVehicle::Iterate()) {
		if (++count > MAX_SPACE_VEHICLES || !Company::IsValidID(vehicle->owner) || vehicle->engine_type != GetSpaceProbeEngine() || vehicle->cargo_cap != 20 || vehicle->cargo.TotalCount() > 20 ||
			vehicle->orders != nullptr || !vehicle->current_order.IsType(OT_NOTHING) || vehicle->Previous() != nullptr || vehicle->Next() != nullptr || vehicle->PreviousShared() != nullptr || vehicle->NextShared() != nullptr || vehicle->mission.travel_remainder >= 74 || vehicle->mission.cost_remainder >= 74 ||
			vehicle->mission.work_ticks >= 74 || vehicle->mission.attachment_offset != SpacePoint{SPACE_Q16, -SPACE_Q16 / 2} || to_underlying(vehicle->mission.phase) > to_underlying(SpaceMissionPhase::Holding) || !field.surveys.empty() || field.generation_version == 0) return false;
		for (auto point : {vehicle->position, vehicle->home}) if (point.x < 0 || point.y < 0 || point.x > static_cast<int32_t>(field.width - 1) * SPACE_Q16 || point.y > static_cast<int32_t>(field.height - 1) * SPACE_Q16) return false;
		const int x = static_cast<int64_t>(vehicle->position.x) * TILE_SIZE / SPACE_Q16 + TILE_SIZE / 2;
		const int y = static_cast<int64_t>(vehicle->position.y) * TILE_SIZE / SPACE_Q16 + TileY(TileXYL(0, 0, MapLayer::Space)) * TILE_SIZE + TILE_SIZE / 2;
		if (vehicle->x_pos != x || vehicle->y_pos != y || vehicle->z_pos != 0 || vehicle->tile != TileVirtXY(x, y)) return false;
		const auto *body = GetMutableAsteroidBody(vehicle->mission.target);
		if (vehicle->mission.phase == SpaceMissionPhase::Approach && (body == nullptr || vehicle->mission.approach_ticks_left == 0)) return false;
		if (vehicle->mission.phase == SpaceMissionPhase::Mining && (body == nullptr || body->mining_slot != vehicle->index || vehicle->position != SpacePoint{body->motion.position.x + vehicle->mission.attachment_offset.x, body->motion.position.y + vehicle->mission.attachment_offset.y})) return false;
		if (body != nullptr && (vehicle->mission.phase == SpaceMissionPhase::Mining || vehicle->mission.phase == SpaceMissionPhase::Approach) &&
			vehicle->cargo_type != GetCargoTypeByLabel(GetMultiLayerAsteroidDefinitions()[to_underlying(body->resource_class)].cargo)) return false;
		for (const auto *packet : *vehicle->cargo.Packets()) {
			if (!packet_owners.insert(packet->index).second) return false;
			const auto source = packet->GetSource();
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
	// Until an actual descent owner is delivered, every extracted packet has exactly
	// one native probe owner. A forged orphan/Surface packet cannot invent a handoff.
	for (const auto *packet : CargoPacket::Iterate()) if (packet->GetSource().type == SourceType::Asteroid && !packet_owners.contains(packet->index)) return false;
	return true;
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
	return digest;
}
