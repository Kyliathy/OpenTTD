/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file tbm_acquisition.cpp Pure whole-consist preparation, unpublished acquisition with rollback, and the role contract.
 * Production commissioning stays guarded: nothing here charges money, reserves a job or publishes a project.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs4-tunnel-boring-machines.md, Group 2B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs3mp-f-solid-starter-contract.md (Plan 4 consumer obligations)
 */

#include "stdafx.h"
#include "tbm.h"
#include "train.h"
#include "train_cmd.h"
#include "engine_base.h"
#include "engine_func.h"
#include "articulated_vehicles.h"
#include "cargotype.h"
#include "company_base.h"
#include "company_func.h"
#include "vehicle_func.h"
#include "settings_type.h"
#include "layer_portal_map.h"
#include "core/math_func.hpp"
#include "timer/timer_game_calendar.h"

#include "safeguards.h"

/**
 * Length of one native part, by the rule ConsistChanged applies when no NewGRF length callback exists.
 * Native TBM definitions carry no GRF or callbacks; acquisition still verifies the built length afterwards.
 * @param engine Native definition.
 * @return Part length in vehicle units (1..VEHICLE_LENGTH).
 */
static uint32_t GetTBMPartLength(const Engine &engine)
{
	return VEHICLE_LENGTH - std::min<uint>(engine.VehInfo<RailVehicleInfo>().shorten_factor, VEHICLE_LENGTH - 1);
}

/**
 * Check that a hopper definition can join this commissioning consist.
 * @param engine Resolved native hopper, or Invalid.
 * @param model Profile model the cutter names for this role.
 * @param label Dedicated spoil cargo this role must carry.
 * @param source Surface assembly end of the pair.
 * @param destination Rock-side housing where the consist works.
 * @return Whether the company may build it now and it carries the right cargo on both layers.
 */
static bool IsTBMHopperUsable(EngineID engine, uint16_t model, CargoLabel label, TileIndex source, TileIndex destination)
{
	// The existing engine owner decides company availability (introduction, railtype); the portal adds no bypass.
	if (engine == EngineID::Invalid() || !IsEngineBuildable(engine, VehicleType::Train, _current_company)) return false;
	const auto cargo = Engine::Get(engine)->GetDefaultCargoType();
	if (!IsValidCargoType(cargo) || CargoSpec::Get(cargo)->label != label) return false;
	const auto capabilities = GetMultiLayerVehicleCapabilities();
	const auto it = std::ranges::find(capabilities, model, &LayerVehicleCapability::model_id);
	// The profile owns where a hopper may operate: it must serve both the assembly end and the work end.
	return it != capabilities.end() && it->supported_layers.Test(GetTileLayer(source)) && it->supported_layers.Test(GetTileLayer(destination));
}

/**
 * Planner history size for a consist of this length.
 * A consist touches at most one more tile than its length covers, whatever its alignment.
 * @param length Whole consist length in vehicle units.
 * @return ceil(length / TILE_SIZE) + 1, at least 3; zero when more than the planner's 16 points would be needed.
 */
static uint8_t GetTBMPlanningPoints(uint32_t length)
{
	const uint32_t points = std::max<uint32_t>(3, CeilDiv(length, TILE_SIZE) + 1);
	return points > 16 ? 0 : static_cast<uint8_t>(points);
}

/**
 * Size a consist from its definitions, before any site or company check.
 * @param model_id Cutter model; it names its own Earth and Rock hopper designs.
 * @param consist Requested wagon counts.
 * @return Engines, parts, length, planner history and capacities, or a refusal.
 */
TBMConsistDimensions GetTBMConsistDimensions(uint16_t model_id, const TBMConsistRequest &consist)
{
	TBMConsistDimensions result;
	const auto *model = GetTBMCutterModel(model_id);
	result.cutter_engine = GetTBMEngine(model_id);
	if (model == nullptr || result.cutter_engine == EngineID::Invalid()) {
		result.error = TBMAcquisitionError::UnavailableModel;
		return result;
	}
	result.earth_engine = GetTBMEngine(model->earth_wagon_model);
	result.rock_engine = GetTBMEngine(model->rock_wagon_model);
	if (result.earth_engine == EngineID::Invalid() || result.rock_engine == EngineID::Invalid()) {
		result.error = TBMAcquisitionError::UnavailableModel;
		return result;
	}
	result.error = TBMAcquisitionError::InvalidConsist;
	// The portal owner requires separate positive Earth and Rock capacity, so both roles need a wagon.
	if (consist.earth_wagons == 0 || consist.rock_wagons == 0) return result;
	const Engine &cutter = *Engine::Get(result.cutter_engine);
	const Engine &earth = *Engine::Get(result.earth_engine);
	const Engine &rock = *Engine::Get(result.rock_engine);
	const uint32_t cutter_parts = 1 + CountArticulatedParts(result.cutter_engine);
	const uint32_t earth_parts = 1 + CountArticulatedParts(result.earth_engine);
	const uint32_t rock_parts = 1 + CountArticulatedParts(result.rock_engine);
	result.vehicle_parts = cutter_parts + consist.earth_wagons * earth_parts + consist.rock_wagons * rock_parts;
	// The cutter's support part reuses the cutter definition, and native hoppers have no articulated parts.
	result.consist_length = cutter_parts * GetTBMPartLength(cutter) + consist.earth_wagons * earth_parts * GetTBMPartLength(earth) + consist.rock_wagons * rock_parts * GetTBMPartLength(rock);
	if (result.consist_length > _settings_game.vehicle.max_train_length * TILE_SIZE) return result;
	result.planning_points = GetTBMPlanningPoints(result.consist_length);
	if (result.planning_points == 0) return result;
	result.earth_capacity_t = uint64_t{consist.earth_wagons} * GetCapacityOfArticulatedParts(result.earth_engine)[earth.GetDefaultCargoType()];
	result.rock_capacity_t = uint64_t{consist.rock_wagons} * GetCapacityOfArticulatedParts(result.rock_engine)[rock.GetDefaultCargoType()];
	result.error = TBMAcquisitionError::None;
	return result;
}

/**
 * Measure an actual assigned consist from its chain.
 * @param front Actual front engine.
 * @param project Assignment whose immutable layout must match the chain.
 * @return Dimensions, or InvalidConsist when the layout or planner history does not fit.
 */
TBMConsistDimensions MeasureTBMConsist(const Train &front, const TBMProject &project)
{
	TBMConsistDimensions result;
	const auto layout = InspectTBMConsist(front, project);
	if (layout.error != TBMBlockedReason::None) return result;
	const auto *model = GetTBMCutterModel(project.model_id);
	if (model == nullptr) return result;
	result.cutter_engine = GetTBMEngine(project.model_id);
	result.earth_engine = GetTBMEngine(model->earth_wagon_model);
	result.rock_engine = GetTBMEngine(model->rock_wagon_model);
	uint32_t earth_units = 0, rock_units = 0;
	// Each exchangeable unit contributes all of its own parts' real capacity to its role.
	for (const auto &unit : layout.exchangeable_units) {
		const Train *head = Train::Get(unit.vehicle);
		uint64_t capacity = 0;
		for (const Train *part = head; part != nullptr && part != head->GetNextUnit(); part = part->Next()) capacity += part->cargo_cap;
		if (unit.role == TBMConsistRole::EarthWagon) {
			++earth_units;
			result.earth_capacity_t += capacity;
		} else {
			++rock_units;
			result.rock_capacity_t += capacity;
		}
	}
	// Separate positive Earth and Rock capacity is required for any area work, exactly as at commissioning.
	if (earth_units == 0 || rock_units == 0) return result;
	for (const Train *part = &front; part != nullptr; part = part->Next()) ++result.vehicle_parts;
	result.consist_length = front.gcache.cached_total_length;
	result.planning_points = GetTBMPlanningPoints(result.consist_length);
	if (result.planning_points == 0) return result;
	result.error = TBMAcquisitionError::None;
	return result;
}

/**
 * Prepare a complete commissioning consist without allocating, charging or reserving anything.
 * @param request Portal request whose proposal carries the canonical route, requested floor and exchange path.
 * @param consist Requested Earth/Rock wagon counts.
 * @return Complete plan, or a refusal with every world state unchanged.
 */
TBMAcquisitionPlan PrepareTBMAcquisition(const LayerPortalCommissionRequest &request, const TBMConsistRequest &consist)
{
	TBMAcquisitionPlan plan;
	plan.consist = consist;
	/**
	 * Finish a refusal.
	 * @param error Stable refusal.
	 * @param tile Failing location, if any.
	 * @return The plan carrying the refusal.
	 */
	const auto refuse = [&plan](TBMAcquisitionError error, TileIndex tile = INVALID_TILE) {
		plan.error = error;
		plan.failure_tile = tile;
		return plan;
	};

	/* Portal: the issued proof covers commissioning validation, source support, the pocket and the exit heading. */
	plan.proof = IssueLayerPortalBootstrap(request);
	if (plan.proof.error != LayerPortalWorkError::None) {
		plan.portal_error = plan.proof.error;
		return refuse(TBMAcquisitionError::Portal, request.portal);
	}

	/* Models and dimensions: the cutter's physical tuning names its own Earth and Rock hopper designs. */
	const uint16_t model_id = request.proposal.model_id;
	const auto dimensions = GetTBMConsistDimensions(model_id, consist);
	plan.cutter_engine = dimensions.cutter_engine;
	plan.earth_engine = dimensions.earth_engine;
	plan.rock_engine = dimensions.rock_engine;
	plan.vehicle_parts = dimensions.vehicle_parts;
	plan.consist_length = dimensions.consist_length;
	plan.planning_points = dimensions.planning_points;
	plan.earth_capacity_t = dimensions.earth_capacity_t;
	plan.rock_capacity_t = dimensions.rock_capacity_t;
	if (dimensions.error != TBMAcquisitionError::None) return refuse(dimensions.error);
	const auto *model = GetTBMCutterModel(model_id);
	if (!IsTBMCutterAvailable(model_id, GetTileLayer(plan.proof.destination), GetCurrentMultiLayerProfileSelection(), TimerGameCalendar::date)) return refuse(TBMAcquisitionError::UnavailableModel, plan.proof.destination);
	// Both hoppers must be buildable now and carry their dedicated cargo on both layers; one missing role refuses the whole consist.
	if (!IsTBMHopperUsable(plan.earth_engine, model->earth_wagon_model, CargoLabel{"ERTH"}, plan.proof.source, plan.proof.destination) || !IsTBMHopperUsable(plan.rock_engine, model->rock_wagon_model, CargoLabel{"ROCK"}, plan.proof.source, plan.proof.destination)) return refuse(TBMAcquisitionError::UnavailableModel);
	const Engine &cutter = *Engine::Get(plan.cutter_engine);
	const Engine &earth = *Engine::Get(plan.earth_engine);
	const Engine &rock = *Engine::Get(plan.rock_engine);
	const uint32_t points = plan.planning_points;

	/* Route: it must replay exactly from the housing with this consist's history, or it was planned for another train. */
	plan.job = request.proposal;
	if (plan.job.route.empty()) {
		plan.geology_error = ExcavationError::InvalidRoute;
		return refuse(TBMAcquisitionError::Geology, plan.proof.destination);
	}
	ExcavationPlanningPose pose{plan.proof.destination, plan.proof.exit_heading, std::vector<TileIndex>(points, plan.proof.destination)};
	// Replaying every accepted segment proves each sweep covers this train's actual trailing parts, not a shorter one's.
	for (const auto &cut : plan.job.route) {
		const auto step = AdvanceExcavationPlanningPose(pose, cut.heading);
		if (step.error != ExcavationError::None) {
			plan.geology_error = step.error; // An illegal turn or boundary is the planner's refusal, not a consist mismatch.
			return refuse(TBMAcquisitionError::Geology, step.failure_tile);
		}
		if (step.cut != cut) return refuse(TBMAcquisitionError::InvalidConsist, cut.cutter_position);
		pose = step.next;
	}

	/* Geology: freeze the pass at the actual capacities, then quote the reservation without allocating it. */
	ExcavationPassTerms terms;
	terms.earth_capacity_t = plan.earth_capacity_t;
	terms.rock_capacity_t = plan.rock_capacity_t;
	if (request.proposal.pass_terms.has_value()) terms.exchange_path = request.proposal.pass_terms->exchange_path;
	const auto pass = PrepareExcavationPass(plan.job, terms);
	if (pass.error != ExcavationError::None) {
		plan.geology_error = pass.error;
		return refuse(TBMAcquisitionError::Geology, pass.failure_tile);
	}
	plan.job.pass_terms = pass.terms;
	const auto reservation = ReserveExcavationJob(plan.job, false);
	if (reservation.error != ExcavationError::None) {
		plan.geology_error = reservation.error;
		return refuse(TBMAcquisitionError::Geology, reservation.failure_tile);
	}
	plan.prospective_job = reservation.id;

	/* Pool, unit number and the per-part site estimates, in the order the builder needs them. */
	if (!Vehicle::CanAllocateItem(plan.vehicle_parts)) return refuse(TBMAcquisitionError::VehicleLimit);
	plan.unit_number = GetFreeUnitNumber(VehicleType::Train);
	if (plan.unit_number == UINT16_MAX) return refuse(TBMAcquisitionError::UnitLimit);
	// Each distinct definition must pass the real site primitive's estimate (railtype, power, model permission).
	for (const Engine *engine : {&cutter, &earth, &rock}) {
		Vehicle *none = nullptr;
		if (BuildRailVehicleForTBM(DoCommandFlags{}, request, engine, &none).Failed()) return refuse(TBMAcquisitionError::UnavailableModel, request.portal);
	}

	/* Funds: the machinery price from its physical model plus every hopper at the engine owner's price. */
	plan.machinery_cost = model->purchase_cost;
	plan.wagon_cost = earth.GetCost() * consist.earth_wagons + rock.GetCost() * consist.rock_wagons;
	if (GetAvailableMoneyForCommand() < plan.machinery_cost + plan.wagon_cost) return refuse(TBMAcquisitionError::InsufficientFunds);

	plan.error = TBMAcquisitionError::None;
	return plan;
}

/** @return The assembled front engine, or nullptr when nothing is held. */
Train *TBMAcquisition::GetFront() const
{
	return this->held ? Train::GetIfValid(this->front) : nullptr;
}

/** Delete every held part and restore the captured random seeds; a no-op when nothing is held. */
void TBMAcquisition::Rollback()
{
	if (!this->held) return;
	Train *front = Train::GetIfValid(this->front);
	// Delete only the chain this owner built; a vanished or changed head means there is nothing of ours left to delete.
	if (front != nullptr && front->IsFrontEngine() && front->owner == this->project.company) delete front;
	RestoreRandomSeeds(this->seeds);
	this->front = VehicleID::Invalid();
	this->project = {};
	this->held = false;
}

/**
 * Hand the parts to a publishing owner; afterwards this object no longer rolls back.
 * @return The prepared project; its id and job remain for the publishing owner to allocate.
 */
TBMProject TBMAcquisition::Release()
{
	TBMProject project = std::move(this->project);
	this->project = {};
	this->front = VehicleID::Invalid();
	this->held = false;
	return project;
}

/**
 * Build the whole consist as unpublished parts after proving the plan is still current.
 * @param request The same portal request the plan was prepared from.
 * @param plan Accepted successful preparation.
 * @param out Empty owner; on success it holds the parts, on failure it stays empty with everything rolled back.
 * @return None, Stale, a fresh preparation refusal, or AllocationFailure after a complete rollback.
 */
TBMAcquisitionError AcquireTBMConsist(const LayerPortalCommissionRequest &request, const TBMAcquisitionPlan &plan, TBMAcquisition &out)
{
	if (out.IsHeld()) return TBMAcquisitionError::AllocationFailure; // One owner holds one consist; a second would orphan the first.
	const auto fresh = PrepareTBMAcquisition(request, plan.consist);
	if (fresh.error != TBMAcquisitionError::None) return fresh.error;
	if (fresh != plan) return TBMAcquisitionError::Stale;

	std::vector<const Engine *> wagons;
	wagons.insert(wagons.end(), plan.consist.earth_wagons, Engine::Get(plan.earth_engine));
	wagons.insert(wagons.end(), plan.consist.rock_wagons, Engine::Get(plan.rock_engine));
	SavedRandomSeeds seeds;
	SaveRandomSeeds(&seeds);
	Train *front = nullptr;
	// The builder deletes its own partial parts on refusal; only the random seeds remain for this owner to restore.
	if (BuildTBMConsistAtSite(request, Engine::Get(plan.cutter_engine), plan.machinery_cost, plan.unit_number, wagons, &front).Failed() || front == nullptr) {
		RestoreRandomSeeds(seeds);
		return TBMAcquisitionError::AllocationFailure;
	}
	out.front = front->index;
	out.seeds = seeds;
	out.held = true;

	TBMProject &project = out.project;
	project.company = request.proposal.company;
	project.cutter = front->index;
	project.model_id = request.proposal.model_id;
	// The permanent prefix is the cutter and its articulated support, in chain order; the first wagon ends it.
	for (const Train *part = front; part != nullptr; part = part->Next()) {
		if (part != front && !part->IsArticulatedPart()) break;
		project.permanent_parts.push_back(part->index);
	}
	project.heading = plan.proof.exit_heading;
	project.origin = request.proposal.origin;
	project.portal = plan.proof.pair_key;
	project.exchange_bay = plan.proof.source; // Source-side commissioning exchange until a Plan 5 bay exists.
	project.phase = TBMPhase::Commissioning;
	project.blocked = TBMBlockedReason::None;

	/* Verify the built consist against the plan; any difference rolls everything back. */
	const auto layout = InspectTBMConsist(*front, project);
	bool matches = layout.error == TBMBlockedReason::None && layout.exchangeable_units.size() == size_t{plan.consist.earth_wagons} + plan.consist.rock_wagons;
	// Earth units come first, then Rock, exactly as requested; a reordered or mislabelled unit is a failed assembly.
	for (size_t i = 0; matches && i < layout.exchangeable_units.size(); ++i) {
		const auto expected = i < plan.consist.earth_wagons ? TBMConsistRole::EarthWagon : TBMConsistRole::RockWagon;
		matches = layout.exchangeable_units[i].role == expected;
	}
	const CargoType earth_cargo = Engine::Get(plan.earth_engine)->GetDefaultCargoType();
	const CargoType rock_cargo = Engine::Get(plan.rock_engine)->GetDefaultCargoType();
	uint32_t parts = 0;
	uint64_t earth_t = 0, rock_t = 0;
	// Count every actual part and its real capacity, so a definition change between quote and build is caught.
	for (const Train *part = front; part != nullptr; part = part->Next()) {
		++parts;
		if (part->cargo_type == earth_cargo) earth_t += part->cargo_cap;
		if (part->cargo_type == rock_cargo) rock_t += part->cargo_cap;
	}
	// Every prepared quantity must hold on the real train before anything can rely on it.
	if (!matches || parts != plan.vehicle_parts || front->gcache.cached_total_length != plan.consist_length || earth_t != plan.earth_capacity_t || rock_t != plan.rock_capacity_t || front->unitnumber != plan.unit_number) {
		out.Rollback();
		return TBMAcquisitionError::AllocationFailure;
	}
	return TBMAcquisitionError::None;
}

/**
 * Classify one actual part against a project's immutable assignment.
 * @param project Canonical assignment: company, cutter and ordered permanent parts.
 * @param part Any actual train part.
 * @return Its role in this company's assigned consist, or Invalid.
 */
TBMConsistRole GetTBMConsistRole(const TBMProject &project, const Train &part)
{
	// Only parts of this company's assigned chain have a role; a stray or foreign part never inherits one.
	if (project.cutter == VehicleID::Invalid() || part.owner != project.company || part.First() == nullptr || part.First()->index != project.cutter) return TBMConsistRole::Invalid;
	const auto fixed = std::ranges::find(project.permanent_parts, part.index);
	if (fixed != project.permanent_parts.end()) {
		if (fixed != project.permanent_parts.begin()) return TBMConsistRole::PermanentSupport;
		return part.index == project.cutter ? TBMConsistRole::Cutter : TBMConsistRole::Invalid;
	}
	const Train *unit = part.GetFirstEnginePart();
	// A spoil role belongs to a whole dedicated wagon unit; unlisted machinery or a mixed-cargo part stays Invalid.
	if (unit == nullptr || !unit->IsWagon() || unit->IsEngine() || !IsValidCargoType(unit->cargo_type) || part.cargo_type != unit->cargo_type) return TBMConsistRole::Invalid;
	const auto label = CargoSpec::Get(unit->cargo_type)->label;
	if (label == CargoLabel{"ERTH"}) return TBMConsistRole::EarthWagon;
	if (label == CargoLabel{"ROCK"}) return TBMConsistRole::RockWagon;
	return TBMConsistRole::Invalid;
}
