/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_construction_trace.cpp Actual UI-to-queue-to-work diagnosis in retained demos.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups4-7.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "space_construction_trace.h"
#include "../asteroid.h"
#include "../space_structure.h"
#include "../space_vehicle.h"
#include "../space_vehicle_cmd.h"
#include "../space_structure_cmd.h"
#include "../vehicle_cmd.h"
#include "../command_func.h"
#include "../space_manifest.h"
#include "../space_approach.h"
#include "../orbital_station.h"
#include "../space_construction_gui.h"
#include "../strings_func.h"
#include "../table/strings.h"
#include "../orbital_service.h"
#include "../utility_power.h"
#include "../company_func.h"
#include "../company_base.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../openttd.h"
#include "../timer/timer_game_tick.h"
#include "../window_func.h"
#include "../window_gui.h"
#include "../widgets/space_structure_widget.h"
#include <thread>
#include "../safeguards.h"

/** Read-only assertion snapshot; no duplicate simulation/cargo account is installed. */
struct ConstructionAuthoritySnapshot {
	uint64_t fleet = ComputeSpaceVehicleDigest(); ///< Native hulls, queues and leases.
	uint64_t structures = ComputeSpaceStructureDigest(); ///< Actual jobs, packets and receipts.
	uint64_t field = ComputeAsteroidFieldDigest(GetAsteroidField()); ///< Sole field motion and resource owner.
	uint64_t power = ComputeUtilityPowerDigest(); ///< Original shared utility accounts.
	uint64_t manifests = ComputeSpaceManifestDigest(); ///< Real held supplies and service custody.
	uint64_t tick = TimerGameTick::counter; ///< No command estimate may simulate an opportunity.
	Money cash = Company::Get(_current_company)->money; ///< Actual company account.
	Randomizer random = _random; ///< Existing synchronized global stream.
	/** Assert that pure or refused operations preserved all observed native owners. */
	void CheckUnchanged() const
	{
		CHECK(ComputeSpaceVehicleDigest() == this->fleet);
		CHECK(ComputeSpaceStructureDigest() == this->structures);
		CHECK(ComputeAsteroidFieldDigest(GetAsteroidField()) == this->field);
		CHECK(ComputeUtilityPowerDigest() == this->power);
		CHECK(ComputeSpaceManifestDigest() == this->manifests);
		CHECK(TimerGameTick::counter == this->tick);
		CHECK(Company::Get(_current_company)->money == this->cash);
		CHECK(_random.state[0] == this->random.state[0]);
		CHECK(_random.state[1] == this->random.state[1]);
	}
};

/**
 * Report the distinct instruction, approach, material and work prerequisites.
 * @param phase Observation boundary; it never names synthetic progress.
 * @param craft Exact native builder selected by the real construction window.
 * @param site Exact retained blueprint selected by that window.
 */
static void ReportConstructionBoundary(std::string_view phase, const SpaceVehicle &craft, const SpaceStructure &site)
{
	const auto &queue = craft.space_orders;
	const auto *core = GetOrbitalConstructionCore(site.state);
	SpacePoint destination;
	const auto approach = GetSpaceApproachDestination(craft, destination);
	const auto visual = GetSpaceConstructionVisualWork(craft);
	fmt::print("CONSTRUCTION phase={} tick={} native_tick={} paused={} builder={}/{} site={}/{} cursor={} started={} stopped={} service={} service_assignment={} cargo_type={} cargo={} approach_block={} at_selected_berth={} inputs_ready={} work={} fraction={} remainder={} last_work_tick={} visual_work={}\n",
		phase, GetAsteroidField().elapsed_ticks, TimerGameTick::counter, _pause_mode.Any(), craft.index.base(), craft.generation, site.index.base(), site.generation,
		queue.cursor, queue.started, craft.vehstatus.Test(VehState::Stopped), craft.manifest ? craft.manifest->service_ticks : 0,
		HasOrbitalServiceAssignment(craft), craft.cargo_type, craft.cargo.TotalCount(), to_underlying(approach), GetSpaceStructureBerth(craft, site).has_value(),
		HasOrbitalConstructionInputs(site), site.job.work_ticks, site.job.work_fraction, site.job.work_remainder, site.job.last_work_tick,
		visual ? static_cast<int64_t>(*visual) : -1);
	fmt::print("CONSTRUCTION_QUEUE blocked={} timer={} work={} crashed={}\n", to_underlying(queue.blocked), queue.timer_ticks,
		queue.work_ticks, craft.vehstatus.Test(VehState::Crashed));
	if (queue.approach) {
		const auto &request = *queue.approach;
		fmt::print("CONSTRUCTION_APPROACH kind={} target={}/{} request_tick={} slot={} staging={},{}\n", to_underlying(request.target.kind),
			request.target.id, request.target.generation, request.request_tick, request.slot, request.staging.x, request.staging.y);
	}
	// Every configured action is logged, including completed/earlier Visits that
	// a native edit restarts. An Idle mining mission alone cannot describe these.
	for (size_t i = 0; i < queue.entries.size(); ++i) {
		const auto &order = queue.entries[i];
		fmt::print("CONSTRUCTION_ORDER index={} action={} kind={} target={}/{} current={}\n", i, to_underlying(order.action),
			to_underlying(order.target.kind), order.target.id, order.target.generation, i == queue.cursor);
	}
	// Physical availability and consumed receipts stay separate from the hold.
	// A failed material boundary may not be bypassed with displayed quantities.
	for (size_t i = 0; i < site.job.inputs.size(); ++i) {
		fmt::print("CONSTRUCTION_INPUT slot={} total={} available={} consumed={}\n", i, site.job.inputs[i].TotalCount(), site.job.inputs[i].AvailableCount(), site.job.consumed[i]);
	}
	if (core != nullptr) {
		const auto grant = GetUtilityAllocation(core->module->assembly_consumer);
		fmt::print("CONSTRUCTION_CORE id={} generation={} active={} panel_enabled={} watts={} consumer={} unspent_grant_j={}\n", core->index.base(), core->generation,
			IsOrbitalModuleOperational(*core), core->module->power_enabled, GetOrbitalModulePanelRate(*core), core->module->assembly_consumer.base(),
			grant ? grant->granted_energy_j : 0);
		// Log genuine stock before delivery/service scenarios so missing supplies
		// cannot be confused with an execution or presentation regression.
		for (const auto label : {CargoLabel{"STEL"}, CargoLabel{"GOOD"}, CargoLabel{"RFMT"}, CargoLabel{"ELEC"}}) {
			const auto cargo = GetCargoTypeByLabel(label);
			if (!IsValidCargoType(cargo)) continue;
			fmt::print("CONSTRUCTION_STOCK cargo={} total={} available={}\n", cargo,
				core->module->stock[cargo].TotalCount(), core->module->stock[cargo].AvailableCount());
		}
	}
}

/**
 * Assert an explanation while proving repeated queries leave every native owner unchanged.
 * @param craft Original native builder, including its exact current queue.
 * @param expected First false prerequisite or current real work receipt.
 * @param unpaused Inspect the next running opportunity, or retain the actual paused state.
 */
void CheckConstructionReadiness(const SpaceVehicle &craft, OrbitalConstructionBlock expected, bool unpaused)
{
	AutoRestoreBackup pause(_pause_mode, unpaused ? PauseModes{} : _pause_mode);
	const ConstructionAuthoritySnapshot before;
	// Repeated GUI refreshes may read the same receipt, but must not allocate a
	// new grant, consume a packet, select a worker or earn another work step.
	for (uint repeat = 0; repeat < 3; ++repeat) {
		const auto readiness = GetOrbitalConstructionReadiness(craft);
		INFO("readiness=" << uint{to_underlying(readiness.block)} << " expected=" << uint{to_underlying(expected)});
		CHECK(readiness.block == expected);
		if (craft.space_orders.cursor < craft.space_orders.entries.size()) {
			const auto &order = craft.space_orders.entries[craft.space_orders.cursor];
			if (order.action == SpaceOrderAction::Construct) CHECK(readiness.target == SpaceStructureRef{SpaceStructureID{static_cast<uint16_t>(order.target.id)}, order.target.generation});
		}
	}
	before.CheckUnchanged();
}

/**
 * Advance only the normal native loop and count actual field opportunities.
 * @param ticks Number of synchronized unpaused simulation ticks to observe.
 * @param craft Builder whose blocked-state transitions are reported.
 * @param site Blueprint whose packet/progress owner remains authoritative.
 * @param report Emit the final boundary; one-tick predicate waits report their named endpoint instead.
 */
void AdvanceConstructionTrace(uint ticks, const SpaceVehicle &craft, const SpaceStructure &site, bool report)
{
	AutoRestoreBackup pause(_pause_mode, PauseModes{});
	const auto target = GetAsteroidField().elapsed_ticks + ticks;
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
	auto blocked = craft.space_orders.blocked;
	// Rendering, elapsed wall time and direct work callbacks grant no progress;
	// only the complete native schedule may change approach/material admission.
	while (GetAsteroidField().elapsed_ticks < target) {
		REQUIRE(std::chrono::steady_clock::now() < deadline);
		StateGameLoop();
		if (craft.space_orders.blocked != blocked) {
			blocked = craft.space_orders.blocked;
			ReportConstructionBoundary("block-transition", craft, site);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	if (report) ReportConstructionBoundary("after-unpaused", craft, site);
}

/**
 * Wait for a real physical transition using the complete native schedule.
 * @param craft Actual builder whose approach/work remains under native control.
 * @param site Actual observed blueprint.
 * @param reached Read-only endpoint predicate; never injects state.
 * @param phase Descriptive endpoint recorded in the evidence log.
 */
void AwaitConstructionBoundary(const SpaceVehicle &craft, const SpaceStructure &site, const std::function<bool()> &reached, std::string_view phase)
{
	// A fixed simulation budget makes stuck admission fail visibly; checking the
	// endpoint after each native tick also preserves the exact delivery boundary.
	for (uint elapsed = 0; !reached(); ++elapsed) {
		REQUIRE(elapsed < 1500);
		AdvanceConstructionTrace(1, craft, site, false);
	}
	ReportConstructionBoundary(phase, craft, site);
}

/**
 * Issue the player's explicit Clear and single-site instruction sequence.
 * @param craft Actual owned hull; a successful edit starts it under native rules.
 * @param target Actual exact site incarnation.
 * @param action Visit or Construct, retaining their distinct target domains.
 */
void OrderConstructionCraft(SpaceVehicle &craft, const SpaceStructure &target, SpaceOrderAction action)
{
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Clear, uint8_t{0},
		SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(craft.index, craft.generation, SpaceOrderEdit::Insert, uint8_t{0}, action,
		action == SpaceOrderAction::Construct ? SpaceOrderTargetKind::Blueprint : SpaceOrderTargetKind::Structure, uint32_t{target.index.base()}, target.generation));
}

/**
 * Free the Core approach through actual travel, preserving every other hull.
 * @param builder Sole builder used by the current focused case.
 */
void MoveOtherConstructionCraftAway(const SpaceVehicle &builder)
{
	// Earlier checkpoints leave another paid builder at the single Core berth.
	// Send it on a real Visit so the selected builder can later load or service.
	for (auto *other : SpaceVehicle::Iterate()) {
		if (other == &builder || other->owner != builder.owner || GetSpaceVehicleModel(*other) != 20) continue;
		REQUIRE_FALSE(HasOrbitalServiceAssignment(*other));
		REQUIRE(Command<Commands::EditSpaceOrder>::Post(other->index, other->generation, SpaceOrderEdit::Clear, uint8_t{0},
			SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
		REQUIRE(Command<Commands::EditSpaceOrder>::Post(other->index, other->generation, SpaceOrderEdit::Insert, uint8_t{0},
			SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
	}
}

/**
 * Deliver real returned stock in two trips and observe the final delivery tick.
 * @param builder Actual empty builder in the retained cancelled/replacement tour.
 * @param site Fresh paid replacement blueprint with no progress or inputs.
 */
static void CheckConstructionDelivery(SpaceVehicle &builder, SpaceStructure &site)
{
	auto *core = GetOrbitalConstructionCore(site.state);
	REQUIRE(core != nullptr);
	ReportConstructionBoundary("delivery-loaded", builder, site);
	REQUIRE(site.job.work_ticks == 0);
	REQUIRE(site.job.inputs[0].TotalCount() == 0);
	REQUIRE(site.job.inputs[1].TotalCount() == 0);
	REQUIRE(builder.cargo.TotalCount() == 0);
	MoveOtherConstructionCraftAway(builder);
	REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(core->index, core->generation, false));
	const auto *recipe = GetSpaceConstructionDefinition(site.state.kind);
	REQUIRE(recipe != nullptr);
	// The fixture's returned packets are loaded by the normal handling command.
	// Deliver each canonical label independently; neither cargo may substitute.
	for (size_t slot = 0; slot < recipe->inputs.size(); ++slot) {
		const auto cargo = GetCargoTypeByLabel(recipe->inputs[slot].label);
		const auto quantity = static_cast<uint16_t>(recipe->inputs[slot].quantity);
		REQUIRE(core->module->stock[cargo].AvailableCount() >= quantity);
		OrderConstructionCraft(builder, *core, SpaceOrderAction::Visit);
		/** @return Whether ordinary travel has reached the actual stock-handling berth. */
		const auto at_stock = [&] { return GetSpaceStructureBerth(builder, *core).has_value(); };
		AwaitConstructionBoundary(builder, site, at_stock, "at-stock");
		REQUIRE(Command<Commands::MoveOrbitalFreight>::Post(core->index, core->generation, builder.index, builder.generation, cargo, quantity, true));
		OrderConstructionCraft(builder, site, SpaceOrderAction::Construct);
		/** @return Whether the original packet owner has received this complete recipe label. */
		const auto delivered = [&] { return site.job.inputs[slot].AvailableCount() == quantity; };
		AwaitConstructionBoundary(builder, site, delivered, "material-delivered");
		CHECK(builder.cargo.TotalCount() == 0);
		CHECK(site.job.work_ticks == 0);
		CHECK(site.job.last_work_tick == 0);
		if (slot == 0) {
			CheckConstructionReadiness(builder, OrbitalConstructionBlock::MaterialsMissing);
			AdvanceConstructionTrace(5, builder, site);
			CHECK(site.job.work_ticks == 0);
		}
	}
	REQUIRE(HasOrbitalConstructionInputs(site));
	const auto delivered_tick = TimerGameTick::counter;
	const ConstructionAuthoritySnapshot delivered;
	GetOrbitalConstructionReadiness(builder);
	delivered.CheckUnchanged();
	AdvanceConstructionTrace(1, builder, site);
	CHECK(site.job.work_ticks == 1);
	CHECK(site.job.last_work_tick == delivered_tick + 1);
	CheckConstructionReadiness(builder, OrbitalConstructionBlock::Working);
	CHECK(site.job.consumed == std::array<uint16_t, 2>{0, 0});
	CHECK(ValidateSpaceVehicles());
	fmt::print("CONSTRUCTION_DELIVERY real_packets=true missing_material_wait=true final_delivery_work=0 next_opportunity_work=1\n");
}

/**
 * Interrupt a real fractional job through power, stop, berth and service owners.
 * @param builder Original working builder in the retained power-loss checkpoint.
 * @param site Actual supplied Storage job with earned fractional work.
 */
static void CheckConstructionInterruptions(SpaceVehicle &builder, SpaceStructure &site)
{
	auto *core = GetOrbitalConstructionCore(site.state);
	REQUIRE(core != nullptr);
	REQUIRE(HasOrbitalConstructionInputs(site));
	REQUIRE(site.job.work_fraction != 0);
	ReportConstructionBoundary("interruptions-loaded", builder, site);
	const auto original = static_cast<const OrbitalConstructionProgress &>(site.job);
	const auto spent = GetUtilityPowerProducer(core->module->producer)->consumed_lifetime_j;
	REQUIRE_FALSE(core->module->power_enabled);
	CheckConstructionReadiness(builder, OrbitalConstructionBlock::Paused, false);
	// This older checkpoint predates saved approach requests. Its original job
	// survives, while the ordinary owner must readmit the existing builder first.
	CheckConstructionReadiness(builder, OrbitalConstructionBlock::ApproachBusy);
	AdvanceConstructionTrace(5, builder, site);
	CheckConstructionReadiness(builder, OrbitalConstructionBlock::PowerUnavailable);
	CHECK(static_cast<const OrbitalConstructionProgress &>(site.job) == original);
	CHECK(GetUtilityPowerProducer(core->module->producer)->consumed_lifetime_j == spent);
	// Return the other craft's real reserved parts before moving it away. This
	// explicitly cancelled service is independent of the tested builder's job.
	const auto held = core->module->service.craft;
	REQUIRE(held != GetSpaceCraftReference(builder));
	REQUIRE(Command<Commands::ServiceOrbitalProbe>::Post(core->index, core->generation, held.vehicle, held.generation, OrbitalServiceControl::Cancel));
	MoveOtherConstructionCraftAway(builder);
	REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(core->index, core->generation, false));
	AdvanceConstructionTrace(5, builder, site);
	CHECK(site.job.work_ticks == original.work_ticks + 5);
	CHECK(site.job.work_fraction == original.work_fraction);
	CHECK(site.job.work_remainder == original.work_remainder);
	CheckConstructionReadiness(builder, OrbitalConstructionBlock::Working);
	{
		AutoRestoreBackup pause(_pause_mode, PauseModes{});
		const ConstructionAuthoritySnapshot committed;
		CHECK_FALSE(IsSpaceConstructionWorkTick(builder));
		CHECK_FALSE(CommitOrbitalConstructionStep(builder));
		committed.CheckUnchanged();
	}
	const auto earned = static_cast<const OrbitalConstructionProgress &>(site.job);
	REQUIRE(Command<Commands::StartStopVehicle>::Post(builder.tile, builder.index, true));
	CheckConstructionReadiness(builder, OrbitalConstructionBlock::Stopped);
	AdvanceConstructionTrace(5, builder, site);
	CHECK(static_cast<const OrbitalConstructionProgress &>(site.job) == earned);
	REQUIRE(Command<Commands::StartStopVehicle>::Post(builder.tile, builder.index, true));
	// The shared approach owner may withdraw admission. Keeping coordinates and
	// the Construct order proves position alone cannot authorize more work.
	ReleaseSpaceApproach(builder);
	CheckConstructionReadiness(builder, OrbitalConstructionBlock::ApproachBusy);
	CHECK(static_cast<const OrbitalConstructionProgress &>(site.job) == earned);
	OrderConstructionCraft(builder, *core, SpaceOrderAction::Visit);
	CheckConstructionReadiness(builder, OrbitalConstructionBlock::NoOrder);
	/** @return Whether native travel has reached the Core's actual service berth. */
	const auto service_arrived = [&] { return GetSpaceStructureBerth(builder, *core).has_value(); };
	AwaitConstructionBoundary(builder, site, service_arrived, "service-arrival");
	CHECK(static_cast<const OrbitalConstructionProgress &>(site.job) == earned);
	const auto services = builder.manifest->services_completed;
	REQUIRE(Command<Commands::ServiceOrbitalProbe>::Post(core->index, core->generation, builder.index, builder.generation, OrbitalServiceControl::Start));
	CheckConstructionReadiness(builder, OrbitalConstructionBlock::ServiceHeld);
	REQUIRE(Command<Commands::ServiceOrbitalProbe>::Post(core->index, core->generation, builder.index, builder.generation, OrbitalServiceControl::Pause));
	AdvanceConstructionTrace(5, builder, site);
	CHECK(static_cast<const OrbitalConstructionProgress &>(site.job) == earned);
	CHECK(builder.manifest->services_completed == services);
	REQUIRE(Command<Commands::ServiceOrbitalProbe>::Post(core->index, core->generation, builder.index, builder.generation, OrbitalServiceControl::Resume));
	/** @return Whether the real contained service owner has completed and released this hull. */
	const auto service_finished = [&] { return !HasOrbitalServiceAssignment(builder); };
	AwaitConstructionBoundary(builder, site, service_finished, "service-finished");
	CHECK(builder.manifest->services_completed == services + 1);
	CHECK(static_cast<const OrbitalConstructionProgress &>(site.job) == earned);
	OrderConstructionCraft(builder, site, SpaceOrderAction::Construct);
	/** @return Whether the original job has earned its next actual whole work step. */
	const auto work_resumed = [&] { return site.job.work_ticks > earned.work_ticks; };
	AwaitConstructionBoundary(builder, site, work_resumed, "work-resumed");
	CHECK(site.job.work_ticks == earned.work_ticks + 1);
	CHECK(site.job.work_fraction == earned.work_fraction);
	CHECK(site.job.work_remainder == earned.work_remainder);
	CheckConstructionReadiness(builder, OrbitalConstructionBlock::Working);
	CHECK(site.job.consumed == std::array<uint16_t, 2>{0, 0});
	CHECK(ValidateSpaceVehicles());
	fmt::print("CONSTRUCTION_INTERRUPTION power=true stopped=true berth=true service=true fractional_work_preserved=true real_service_completed=true\n");
}

/**
 * Verify estimates, stale targets, active edits and stopped presentation.
 * @param builder Actual supplied builder that has just earned native work.
 * @param site Its actual paid Storage blueprint and retained partial progress.
 * @param window Real construction window whose paint path must not start a hull.
 */
static void CheckConstructionQueueEdits(SpaceVehicle &builder, SpaceStructure &site, Window &window)
{
	/** @param flags Native execution/estimate flags. @param craft_generation Exact or deliberately stale hull incarnation. @param site_generation Exact or deliberately stale site incarnation. @return Native command admission. */
	auto edit = [&](DoCommandFlags flags, uint64_t craft_generation, uint64_t site_generation) {
		return CmdEditSpaceOrder(flags, builder.index, craft_generation, SpaceOrderEdit::Replace, 0,
			SpaceOrderAction::Construct, SpaceOrderTargetKind::Blueprint, site.index.base(), site_generation);
	};
	const ConstructionAuthoritySnapshot original;
	CHECK(edit({}, builder.generation, site.generation).Succeeded());
	CHECK(edit({DoCommandFlag::Execute, DoCommandFlag::QueryCost}, builder.generation, site.generation).Succeeded());
	original.CheckUnchanged();
	CHECK(edit({DoCommandFlag::Execute}, builder.generation + 1, site.generation).Failed());
	CHECK(edit({DoCommandFlag::Execute}, builder.generation, site.generation + 1).Failed());
	original.CheckUnchanged();
	const auto work = static_cast<const OrbitalConstructionProgress &>(site.job);
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(builder.index, builder.generation, SpaceOrderEdit::Replace, uint8_t{0},
		SpaceOrderAction::Construct, SpaceOrderTargetKind::Blueprint, uint32_t{site.index.base()}, site.generation));
	CHECK(static_cast<const OrbitalConstructionProgress &>(site.job) == work);
	REQUIRE(Command<Commands::StartStopVehicle>::Post(builder.tile, builder.index, true));
	REQUIRE(builder.vehstatus.Test(VehState::Stopped));
	const ConstructionAuthoritySnapshot stopped;
	{
		DrawPixelInfo empty{};
		AutoRestoreBackup clipping(_cur_dpi, &empty);
		window.OnPaint();
	}
	CHECK(edit({}, builder.generation, site.generation).Succeeded());
	CHECK(edit({DoCommandFlag::Execute, DoCommandFlag::QueryCost}, builder.generation, site.generation).Succeeded());
	stopped.CheckUnchanged();
	REQUIRE(builder.vehstatus.Test(VehState::Stopped));
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(builder.index, builder.generation, SpaceOrderEdit::Replace, uint8_t{0},
		SpaceOrderAction::Construct, SpaceOrderTargetKind::Blueprint, uint32_t{site.index.base()}, site.generation));
	CHECK_FALSE(builder.vehstatus.Test(VehState::Stopped));
	CHECK(static_cast<const OrbitalConstructionProgress &>(site.job) == work);
	AdvanceConstructionTrace(5, builder, site);
	CHECK(site.job.work_ticks > work.work_ticks);
	fmt::print("CONSTRUCTION_QUEUE_CONTRACT estimates_pure=true stale_rejected=true paint_keeps_stopped=true explicit_edit_starts=true partial_work_retained=true\n");
}

/**
 * Keep the actual service owner in control when a queue edit is attempted.
 * @param craft Original service-held builder in the retained service checkpoint.
 * @param site Existing owned Core, providing an otherwise valid Visit destination.
 */
static void CheckConstructionServiceHold(SpaceVehicle &craft, const SpaceStructure &site)
{
	REQUIRE(HasOrbitalServiceAssignment(craft));
	SpacePoint point;
	const SpaceOrderTarget target{SpaceOrderTargetKind::Structure, site.index.base(), site.generation};
	REQUIRE(ResolveSpaceNavigationTarget(craft, target, point) == SpaceOrderBlock::None);
	const ConstructionAuthoritySnapshot before;
	// The target and role are valid; each refusal must come from the existing
	// service custody boundary without resetting its timer or moving its parts.
	for (DoCommandFlags flags : {DoCommandFlags{}, DoCommandFlags{DoCommandFlag::Execute}, DoCommandFlags{DoCommandFlag::Execute, DoCommandFlag::QueryCost}}) {
		CHECK(CmdEditSpaceOrder(flags, craft.index, craft.generation, SpaceOrderEdit::Insert, 0,
			SpaceOrderAction::Visit, target.kind, target.id, target.generation).Failed());
		before.CheckUnchanged();
	}
	fmt::print("CONSTRUCTION_QUEUE_CONTRACT service_held=true edit_refused=true supplies_and_receipts_unchanged=true\n");
}

/**
 * Remove an empty paid blueprint and buy a genuine replacement in its pool slot.
 * @param builder Real travelling builder whose configured old target must persist.
 * @param site Empty, untouched Storage from the retained factory checkpoint; deleted by the native command.
 */
static void CheckConstructionTargetReuse(SpaceVehicle &builder, SpaceStructure &site)
{
	REQUIRE(site.job.work_ticks == 0);
	REQUIRE(site.job.inputs[0].TotalCount() == 0);
	REQUIRE(site.job.inputs[1].TotalCount() == 0);
	AdvanceConstructionTrace(74, builder, site);
	const auto reference = GetSpaceStructureReference(site);
	const auto state = site.state;
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(builder.index, builder.generation, SpaceOrderEdit::Clear, uint8_t{0},
		SpaceOrderAction::Visit, SpaceOrderTargetKind::Body, uint32_t{0}, uint64_t{0}));
	REQUIRE(Command<Commands::EditSpaceOrder>::Post(builder.index, builder.generation, SpaceOrderEdit::Insert, uint8_t{0},
		SpaceOrderAction::Construct, SpaceOrderTargetKind::Blueprint, uint32_t{reference.structure.base()}, reference.generation));
	const auto order = builder.space_orders.entries.front();
	const auto cargo = builder.cargo.TotalCount();
	const auto cash = Company::Get(_current_company)->money;
	REQUIRE(Command<Commands::RemoveOrbitalModule>::Post(reference.structure, reference.generation));
	CHECK(ResolveSpaceStructure(reference) == nullptr);
	CHECK(builder.space_orders.blocked == SpaceOrderBlock::TargetRemoved);
	CHECK(Company::Get(_current_company)->money == cash);
	const auto tile = TileXYL(state.x, state.y, MapLayer::Space);
	const auto quote = CmdBuildSpaceBlueprint({}, tile, state.kind);
	REQUIRE(quote.Succeeded());
	REQUIRE(Command<Commands::BuildSpaceBlueprint>::Post(tile, state.kind));
	auto *replacement = SpaceStructure::GetIfValid(reference.structure);
	REQUIRE(replacement != nullptr);
	CHECK(replacement->generation > reference.generation);
	CHECK(Company::Get(_current_company)->money == cash - quote.GetCost());
	CHECK(builder.space_orders.entries.front() == order);
	CHECK(builder.space_orders.blocked == SpaceOrderBlock::TargetRemoved);
	const ConstructionAuthoritySnapshot replaced;
	CHECK(CmdEditSpaceOrder({DoCommandFlag::Execute}, builder.index, builder.generation, SpaceOrderEdit::Replace, 0,
		SpaceOrderAction::Construct, order.target.kind, order.target.id, order.target.generation).Failed());
	replaced.CheckUnchanged();
	AdvanceConstructionTrace(1, builder, *replacement);
	CHECK(builder.space_orders.entries.front() == order);
	CHECK(builder.space_orders.blocked == SpaceOrderBlock::TargetRemoved);
	CHECK(builder.cargo.TotalCount() == cargo);
	CHECK(replacement->job.work_ticks == 0);
	CHECK(ValidateSpaceVehicles());
	fmt::print("CONSTRUCTION_QUEUE_CONTRACT removed_target=true reused_slot={} old_generation={} new_generation={} paid_replacement=true old_order_unresolved=true\n",
		reference.structure.base(), reference.generation, replacement->generation);
}

/**
 * Capture an actual material wait, explicit remedy, resumed work and completed recipe.
 * @param builder Real empty construction hull from the cancelled/replacement tour.
 * @param site Actual empty paid replacement job beside the retained stocked Core.
 */
static void CheckConstructionPresentation(SpaceVehicle &builder, SpaceStructure &site)
{
	auto *core = GetOrbitalConstructionCore(site.state);
	REQUIRE(core != nullptr);
	MoveOtherConstructionCraftAway(builder);
	REQUIRE(Command<Commands::SetOrbitalPanelPaused>::Post(core->index, core->generation, false));
	OrderConstructionCraft(builder, site, SpaceOrderAction::Construct);
	AdvanceConstructionTrace(148, builder, site);
	CheckConstructionReadiness(builder, OrbitalConstructionBlock::MaterialsMissing);
	CaptureConstructionViews(builder, site, "waiting");
	ShowSpaceConstructionWindow(GetSpaceStructureReference(site));
	auto *window = FindWindowById(WindowClass::SpaceConstruction, 0);
	REQUIRE(window != nullptr);
	REQUIRE_FALSE(window->IsWidgetDisabled(WID_SC_RESERVE));
	window->OnClick({}, WID_SC_RESERVE, 1);
	REQUIRE(HasOrbitalConstructionInputs(site));
	CHECK(site.job.work_ticks == 0);
	CaptureConstructionViews(builder, site, "reserved");
	AdvanceConstructionTrace(1, builder, site);
	CHECK(site.job.work_ticks == 1);
	CheckConstructionReadiness(builder, OrbitalConstructionBlock::Working);
	CaptureConstructionViews(builder, site, "working");
	AdvanceConstructionTrace(300, builder, site);
	REQUIRE(site.state.phase == SpaceStructurePhase::FrameComplete);
	CheckConstructionReadiness(builder, OrbitalConstructionBlock::Completed);
	const auto text = GetSpaceConstructionMaterialText(site, &builder);
	CHECK(std::ranges::find(text, GetString(STR_SPACE_CONSTRUCTION_REMAINING, 0, 0)) != text.end());
	CHECK(std::ranges::find(text, GetString(STR_SPACE_CONSTRUCTION_CONSUMED, 8, 8)) != text.end());
	CaptureConstructionViews(builder, site, "complete");
	const auto core_text = GetSpaceConstructionMaterialText(*core, &builder);
	CHECK(std::ranges::find(core_text, GetString(STR_SPACE_CONSTRUCTION_REMAINING, 0, 0)) != core_text.end());
	CaptureConstructionViews(builder, *core, "completed-core");
	CHECK(ValidateSpaceVehicles());
	fmt::print("CONSTRUCTION_PRESENTATION actual_wait=true explicit_reservation_click=true next_tick_work=true completed_consumption=true both_views_pure=true\n");
}

/** @copydoc RunSpaceConstructionTrace */
void RunSpaceConstructionTrace(std::string_view scenario)
{
	// Replay cases have their own bounded preparation and completion assertions;
	// they still use the same native application and construction tick helpers.
	if (scenario.starts_with("checkpoint-") || scenario.starts_with("reload-") || scenario == "workers") {
		RunSpaceConstructionReplay(scenario);
		return;
	}
	REQUIRE((scenario == "waiting" || scenario == "working" || scenario == "queue" || scenario == "service" || scenario == "removed" || scenario == "delivery" || scenario == "interruptions" || scenario == "presentation"));
	const bool supplied = scenario == "working" || scenario == "queue";
	SpaceVehicle *builder = nullptr;
	SpaceStructure *site = nullptr;
	// Match the window's first owned orbital builder, retaining its exact real
	// incarnation instead of creating another hull for a convenient test state.
	for (auto *craft : SpaceVehicle::Iterate()) {
		// The service case must address the actual held hull, not whichever idle
		// builder happens to occupy the lowest native pool index.
		if (craft->owner == _local_company && GetSpaceVehicleModel(*craft) == 20 && craft->construction.placement == SpacePlacement::Orbital &&
			(scenario != "service" || HasOrbitalServiceAssignment(*craft))) { builder = craft; break; }
	}
	// Both inputs contain the same paid Storage geometry; its own generation and
	// packet owner, rather than a slot number guessed from the screenshot, wins.
	for (auto *candidate : SpaceStructure::Iterate()) {
		// A service-held craft needs only an otherwise valid Core destination to
		// prove edit refusal; that checkpoint predates the Storage blueprint.
		if (scenario == "service" && candidate->state.owner == _local_company && candidate->state.kind == SpaceBuildKind::Core) { site = candidate; break; }
		if (candidate->state.owner == _local_company && candidate->state.kind == SpaceBuildKind::Storage && candidate->state.phase == SpaceStructurePhase::Blueprint) { site = candidate; break; }
	}
	REQUIRE(builder != nullptr);
	REQUIRE(site != nullptr);
	if (scenario == "service") { CheckConstructionServiceHold(*builder, *site); return; }
	if (scenario == "removed") { CheckConstructionTargetReuse(*builder, *site); return; }
	if (scenario == "delivery") { CheckConstructionDelivery(*builder, *site); return; }
	if (scenario == "interruptions") { CheckConstructionInterruptions(*builder, *site); return; }
	if (scenario == "presentation") { CheckConstructionPresentation(*builder, *site); return; }
	REQUIRE(HasOrbitalConstructionInputs(*site) == supplied);
	ReportConstructionBoundary("loaded", *builder, *site);
	const auto original_work = static_cast<const OrbitalConstructionProgress &>(site->job);
	const std::array initial_inputs{site->job.inputs[0].TotalCount(), site->job.inputs[1].TotalCount()};
	const auto fleet = ComputeSpaceVehicleDigest(), structures = ComputeSpaceStructureDigest();
	const auto &before = builder->space_orders;
	const auto quote = CmdEditSpaceOrder({}, builder->index, builder->generation, SpaceOrderEdit::Insert,
		static_cast<uint8_t>(before.entries.size()), SpaceOrderAction::Construct, SpaceOrderTargetKind::Blueprint, site->index.base(), site->generation);
	REQUIRE(quote.Succeeded());
	CHECK(ComputeSpaceVehicleDigest() == fleet);
	CHECK(ComputeSpaceStructureDigest() == structures);
	fmt::print("CONSTRUCTION_COMMAND estimate_success=true query_unchanged=true\n");
	ShowSpaceConstructionWindow(GetSpaceStructureReference(*site));
	Window *window = FindWindowById(WindowClass::SpaceConstruction, 0);
	REQUIRE(window != nullptr);
	REQUIRE_FALSE(window->IsWidgetDisabled(WID_SC_QUEUE));
	const auto old_size = builder->space_orders.entries.size();
	window->OnClick({}, WID_SC_QUEUE, 1);
	REQUIRE(builder->space_orders.entries.size() == old_size + 1);
	CHECK(builder->space_orders.entries.back().target == SpaceOrderTarget{SpaceOrderTargetKind::Blueprint, site->index.base(), site->generation});
	CHECK(builder->space_orders.cursor == 0);
	ReportConstructionBoundary("after-append-click", *builder, *site);
	// Follow the retained tour's explicit Clear then Queue sequence. Queue edits
	// restart and start the native hull; painting must never perform this change.
	window->OnClick({}, WID_SC_CLEAR, 1);
	REQUIRE(builder->space_orders.entries.empty());
	window->OnClick({}, WID_SC_QUEUE, 1);
	REQUIRE(builder->space_orders.entries.size() == 1);
	REQUIRE(builder->space_orders.entries.front().action == SpaceOrderAction::Construct);
	CHECK_FALSE(builder->vehstatus.Test(VehState::Stopped));
	ReportConstructionBoundary("after-clear-and-queue", *builder, *site);
	AdvanceConstructionTrace(148, *builder, *site);
	REQUIRE(GetSpaceStructureBerth(*builder, *site).has_value());
	CHECK(site->job.inputs[0].TotalCount() == initial_inputs[0]);
	CHECK(site->job.inputs[1].TotalCount() == initial_inputs[1]);
	CHECK(site->job.consumed == std::array<uint16_t, 2>{0, 0});
	if (supplied) {
		CHECK(site->job.work_ticks > original_work.work_ticks);
		CHECK(builder->space_orders.blocked == SpaceOrderBlock::None);
		CheckConstructionReadiness(*builder, OrbitalConstructionBlock::Working);
	} else {
		CHECK(static_cast<const OrbitalConstructionProgress &>(site->job) == original_work);
		CHECK(builder->space_orders.blocked == SpaceOrderBlock::MaterialsMissing);
		CheckConstructionReadiness(*builder, OrbitalConstructionBlock::MaterialsMissing);
	}
	CHECK(ValidateSpaceVehicles());
	fmt::print("CONSTRUCTION_RESULT supplied={} ordinary_ticks=148 work_before={} work_after={} blocked={}\n", supplied, original_work.work_ticks, site->job.work_ticks, to_underlying(builder->space_orders.blocked));
	if (scenario == "queue") CheckConstructionQueueEdits(*builder, *site, *window);
}
