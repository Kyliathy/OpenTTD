/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file orbital_recipe_inventory.cpp Actual module packet/work adapter boundaries in declared isolated consumed-frame fixtures.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10B
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10C native runtime/record boundary fixtures.
 * Declared unit frames verify packet/shared-power/runtime boundaries; actual paid processor procurement remains Group11.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../orbital_recipe.h"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../hub_cargo.h"
#include "../core/backup_type.hpp"
#include "../fios.h"
#include "../saveload/space_cargo_preview.h"
#include "../transport_tube.h"
#include "../timer/timer_game_tick.h"
#include "excavation_fixture.h"
#include <filesystem>
#include "layer_save_fixture.h"
#include "../safeguards.h"

/** Declare an isolated fully consumed test frame; no gameplay procurement is inferred.
 * @param kind Original canonical construction definition.
 * @param index Native restore fixture identity and next positive incarnation.
 * @param x Nonoverlapping geometry adjacent to the separate Core fixture.
 * @return Inert consumed frame with no packet stock or functioning panel.
 */
static SpaceStructureRecord RecipeFrame(SpaceBuildKind kind, uint16_t index, uint16_t x)
{
	const auto &definition = *GetSpaceConstructionDefinition(kind); SpaceStructureRecord row;
	row.ref = {SpaceStructureID{index}, uint64_t{index} + 1}; row.state.owner = _current_company;
	row.state.kind = kind; row.state.x = x; row.state.y = 4; row.state.capital_paid = definition.capital;
	row.state.phase = SpaceStructurePhase::FrameComplete; row.work_ticks = definition.work_ticks;
	row.consumed = {static_cast<uint16_t>(definition.inputs[0].quantity), static_cast<uint16_t>(definition.inputs[1].quantity)};
	return row;
}

/** Real profile/module/packet/utility owners; initial consumed frames are explicitly isolated unit inputs. */
class OrbitalRecipeRig {
	ExcavationFixture world{true}; ///< Match bundled ordinal positions in isolated label-based unit fixtures; ordinary file tests initialize actual cargo properties through the real loader.
	LayerSaveContent content{world.ActivatedContent()}; ///< Actual required content admission for module commands.
	AutoRestoreBackup<TimerGameCalendar::Year> year{TimerGameCalendar::year, TimerGameCalendar::Year{1995}};
	bool first_opportunity = true; ///< The reset ledger already owns its first unallocated interval; later opportunities must rotate it.
public:
	std::unique_ptr<OrbitalRecipeState> state; ///< One actual incarnation's metadata; native attachment follows in10C.
	/** @param kind Canonical processor activated by actual module commands. @param choice Published alternative, or the module default. @param twin Declare a second actual consumed-frame fixture sharing this original Core's finite panel. */
	explicit OrbitalRecipeRig(SpaceBuildKind kind = SpaceBuildKind::Steelworks, MultiLayerRecipeID choice = {}, bool twin = false)
	{
		ResetUtilityPowerState(); ResetSpaceStructures();
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1995}, 0, 1);
		std::vector rows{RecipeFrame(SpaceBuildKind::Core, 0, 4), RecipeFrame(kind, 1, 7)};
		if (twin) rows.push_back(RecipeFrame(kind, 2, 1));
		REQUIRE(InstallLoadedSpaceStructures(rows, twin ? 4 : 3));
		auto &core = Core(); auto &site = Site();
		REQUIRE(CmdActivateOrbitalCore({DoCommandFlag::Execute}, core.index, core.generation).Succeeded());
		// Actual shared inventory ticks complete commissioning; no fabricated countdown or utility balance accelerates activation.
		for (uint tick = 0; tick < GetMultiLayerSpaceBudget().core_activation_ticks; ++tick) TickOrbitalConstructionInventory();
		REQUIRE(CmdActivateOrbitalModule({DoCommandFlag::Execute}, site.index, site.generation).Succeeded());
		for (uint tick = 0; tick < GetMultiLayerSpaceBudget().core_activation_ticks; ++tick) TickOrbitalConstructionInventory();
		REQUIRE(IsOrbitalModuleOperational(site)); const auto *definition = GetOrbitalRecipeDefinition(kind, choice); REQUIRE(definition);
		if (twin) {
			auto &second = *SpaceStructure::Get(SpaceStructureID{2});
			REQUIRE(CmdActivateOrbitalModule({DoCommandFlag::Execute}, second.index, second.generation).Succeeded());
			// The second declared native frame completes its own actual activation; it cannot borrow the first processor's countdown or create another panel.
			for (uint tick = 0; tick < GetMultiLayerSpaceBudget().core_activation_ticks; ++tick) TickOrbitalConstructionInventory();
			REQUIRE(IsOrbitalModuleOperational(second));
		}
		if (definition->recipe.power_j_per_batch != 0) {
			const auto consumer = RegisterUtilityPowerConsumer(_current_company, MapLayer::Space, UtilityConsumerEffect::AssemblyWork, 750, std::array{core.module->producer}); REQUIRE(consumer);
			state = CreateOrbitalRecipeState(GetSpaceStructureReference(site), choice, GetSpaceStructureReference(core), *consumer);
		} else {
			state = CreateOrbitalRecipeState(GetSpaceStructureReference(site), choice);
		}
		REQUIRE(state); REQUIRE(ValidateSpaceStructures());
	}
	/** Destroy isolated physical owners before restoring the caller's actual content and company. */
	~OrbitalRecipeRig() { _space_structure_pool.CleanPool(); ResetSpaceStructures(); ResetUtilityPowerState(); }
	/** @return Actual public-pool fixture Core. */
	SpaceStructure &Core() const { return *SpaceStructure::Get(SpaceStructureID{0}); }
	/** @return Actual public-pool fixture processor. */
	SpaceStructure &Site() const { return *SpaceStructure::Get(SpaceStructureID{1}); }
	/** @return Original immutable selected content definition. */
	const MultiLayerRecipe &Recipe() const { return GetOrbitalRecipeDefinition(Site().state.kind, state->choice)->recipe; }
	/** Add declared unit-fixture inputs/products as real native packets, preserving original physical ownership.
	 * @param label Actual configured physical cargo meaning.
	 * @param quantity Explicit isolated whole-unit amount, never a paid-world procurement assertion.
	 */
	void Add(CargoLabel label, uint16_t quantity)
	{
		const auto cargo = GetCargoTypeByLabel(label); REQUIRE(IsValidCargoType(cargo));
		REQUIRE(CargoPacket::CanAllocateItem());
		auto *packet = CargoPacket::Create(StationID::Invalid(), quantity, Source{Source::Invalid, SourceType::Industry}); REQUIRE(packet);
		Site().module->stock[cargo].Append(packet, StationID::Invalid()); ++state->revision;
	}
	/** @param label Actual configured compartment meaning. @return Whole native packet quantity held by this same module. */
	uint Count(CargoLabel label) const { return Site().module->stock[GetCargoTypeByLabel(label)].TotalCount(); }
	/** Publish and allocate the next real Core opportunity without spending its source energy. */
	void Grant()
	{
		if (!first_opportunity) {
			AutoRestoreBackup actor(_current_company, OWNER_DEITY); REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
		}
		PublishOrbitalUtilitySources(GetUtilityPowerInterval(), 1);
		AutoRestoreBackup actor(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), std::array{UtilityPowerRequest{state->consumer, 750}}) == UtilityPowerError::None);
		first_opportunity = false;
	}
	/** @param inventory Same actual packet/paid-work participant to earn one full canonical batch. */
	void Earn(OrbitalRecipeInventory &inventory)
	{
		// The published 74-tick batch must consume 74 separate real utility opportunities while all physical inputs remain held.
		for (uint tick = 0; tick < Recipe().work_economy_ticks; ++tick) {
			Grant(); const auto proposal = PrepareLayerRecipeWork(Recipe(), inventory, inventory, 750);
			REQUIRE(proposal.error == LayerRecipeCommitError::None);
			REQUIRE(CommitLayerRecipeWork(Recipe(), inventory, inventory, proposal) == LayerRecipeCommitError::None);
		}
	}
};

/** Existing canonical family records define alternatives and retained buffers without a second numerical recipe catalogue. */
TEST_CASE("Orbital processor choices retain published bounded cargo families", "[orbital-recipe-inventory]")
{
	CHECK(GetDefaultOrbitalRecipe(SpaceBuildKind::Steelworks) == MultiLayerRecipeID{"OSTL"});
	CHECK(GetDefaultOrbitalRecipe(SpaceBuildKind::Goodsworks) == MultiLayerRecipeID{"OGDS"});
	CHECK(GetDefaultOrbitalRecipe(SpaceBuildKind::Refinery) == MultiLayerRecipeID{"OREF"});
	CHECK(GetDefaultOrbitalRecipe(SpaceBuildKind::IceProcessor) == MultiLayerRecipeID{"PICE"});
	CHECK(GetOrbitalRecipeDefinition(SpaceBuildKind::Steelworks, MultiLayerRecipeID{"OGDS"}) == nullptr);
	CHECK(GetOrbitalRecipeDefinition(SpaceBuildKind::Core) == nullptr);
	CHECK(GetOrbitalRecipeDefinition(SpaceBuildKind::Refinery, MultiLayerRecipeID{"PICE"}) != nullptr);
	CHECK(GetOrbitalRecipeDefinition(SpaceBuildKind::IceProcessor, MultiLayerRecipeID{"OREF"}) != nullptr);
	CHECK(GetOrbitalRecipeCargoCapacity(SpaceBuildKind::Steelworks, CargoLabel{"NORE"}) == 1024);
	CHECK(GetOrbitalRecipeCargoCapacity(SpaceBuildKind::Steelworks, CargoLabel{"STEL"}) == 1024);
	CHECK(GetOrbitalRecipeCargoCapacity(SpaceBuildKind::Steelworks, CargoLabel{"GOOD"}) == 0);
	CHECK(GetOrbitalRecipeCargoCapacity(SpaceBuildKind::Refinery, CargoLabel{"WICE"}) == 1024);
}

/** Complete physical outputs are allocated before all-input settlement, with one actual paid-work debit and no transport payment. */
TEST_CASE("Actual orbital packets settle one shared electrically paid batch", "[orbital-recipe-inventory]")
{
	OrbitalRecipeRig rig; rig.Add(CargoLabel{"NORE"}, 4); OrbitalRecipeInventory inventory(*rig.state);
	const auto money = Company::Get(_current_company)->money; rig.Earn(inventory);
	CHECK(rig.Count(CargoLabel{"NORE"}) == 4); CHECK(rig.Count(CargoLabel{"STEL"}) == 0);
	CHECK(rig.state->electrical->Capture().paid_j == 55500);
	const auto source_paid = GetUtilityPowerProducer(rig.Core().module->producer)->consumed_lifetime_j; CHECK(source_paid == 55500);
	const auto proposal = PrepareLayerRecipeBatch(rig.Recipe(), inventory); REQUIRE(proposal.plan.batches == 1);
	const auto packet_count = CargoPacket::GetNumItems(); const auto digest = ComputeSpaceStructureDigest();
	REQUIRE(inventory.PrepareOutputs(proposal, false)); CHECK(CargoPacket::GetNumItems() == packet_count); CHECK(ComputeSpaceStructureDigest() == digest);
	REQUIRE(inventory.PrepareOutputs(proposal, true)); CHECK(CargoPacket::GetNumItems() == packet_count + 1);
	CHECK(rig.Count(CargoLabel{"NORE"}) == 4); CHECK(rig.Count(CargoLabel{"STEL"}) == 0);
	CHECK(CommitLayerRecipeBatch(rig.Recipe(), inventory, inventory, proposal).error == LayerRecipeCommitError::None);
	CHECK(rig.Count(CargoLabel{"NORE"}) == 0); CHECK(rig.Count(CargoLabel{"STEL"}) == 2);
	CHECK(rig.state->consumed.at(CargoLabel{"NORE"}) == 4); CHECK(rig.state->produced.at(CargoLabel{"STEL"}) == 2);
	CHECK(rig.state->batches.at(MultiLayerRecipeID{"OSTL"}) == 1); CHECK(rig.state->electrical->Capture().paid_j == 0);
	CHECK(GetUtilityPowerProducer(rig.Core().module->producer)->consumed_lifetime_j == source_paid);
	CHECK(Company::Get(_current_company)->money == money); CHECK(ValidateSpaceStructures());
	CHECK(CommitLayerRecipeBatch(rig.Recipe(), inventory, inventory, proposal).error != LayerRecipeCommitError::None);
}

/** An additional whole-output refusal exercises allocation rollback without issuing fake pool identities or exhausting millions of packets.
 * @param packets Proposed complete output-reference count; refusal is independent of real pool capacity.
 * @return False, preserving every physical input and already paid work receipt.
 */
static bool RefuseRecipeAllocation([[maybe_unused]] size_t packets) { return false; }

/** Failed output allocation retains paid progress and real input packets for an ordinary retry with no second source debit. */
TEST_CASE("Orbital allocation refusal retains inputs and paid work for retry", "[orbital-recipe-inventory]")
{
	OrbitalRecipeRig rig; rig.Add(CargoLabel{"NORE"}, 4);
	OrbitalRecipeInventory blocked(*rig.state, RefuseRecipeAllocation); rig.Earn(blocked);
	const auto proposal = PrepareLayerRecipeBatch(rig.Recipe(), blocked); REQUIRE(proposal.plan.batches == 1);
	const auto digest = ComputeSpaceStructureDigest(), utility = ComputeUtilityPowerDigest(); const auto packets = CargoPacket::GetNumItems();
	CHECK_FALSE(blocked.PrepareOutputs(proposal, false)); CHECK_FALSE(blocked.PrepareOutputs(proposal, true));
	CHECK(ComputeSpaceStructureDigest() == digest); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(CargoPacket::GetNumItems() == packets);
	CHECK(rig.state->electrical->Capture().paid_j == 55500); CHECK(rig.state->batches.at(rig.state->choice) == 0);
	OrbitalRecipeInventory retry(*rig.state); const auto fresh = PrepareLayerRecipeBatch(rig.Recipe(), retry);
	REQUIRE(retry.PrepareOutputs(fresh, true)); CHECK(CommitLayerRecipeBatch(rig.Recipe(), retry, retry, fresh).error == LayerRecipeCommitError::None);
	CHECK(rig.Count(CargoLabel{"STEL"}) == 2); CHECK(ComputeUtilityPowerDigest() == utility);
}

/** Both actual secondary inputs settle together into the content-defined physical Goods form; tampered whole-output quotes consume nothing. */
TEST_CASE("Orbital Goodsworks commits both native inputs and physical Goods exactly once", "[orbital-recipe-inventory]")
{
	OrbitalRecipeRig rig(SpaceBuildKind::Goodsworks); rig.Add(CargoLabel{"STEL"}, 2); rig.Add(CargoLabel{"RARE"}, 1);
	OrbitalRecipeInventory inventory(*rig.state); rig.Earn(inventory);
	const auto proposal = PrepareLayerRecipeBatch(rig.Recipe(), inventory); REQUIRE(proposal.plan.batches == 1);
	REQUIRE(inventory.PrepareOutputs(proposal, true)); const auto digest = ComputeSpaceStructureDigest();
	auto tampered = proposal; ++tampered.outputs.front().amount;
	CHECK(CommitLayerRecipeBatch(rig.Recipe(), inventory, inventory, tampered).error == LayerRecipeCommitError::InvalidProposal);
	CHECK(ComputeSpaceStructureDigest() == digest); CHECK(rig.state->electrical->Capture().paid_j == 55500);
	CHECK(CommitLayerRecipeBatch(rig.Recipe(), inventory, inventory, proposal).error == LayerRecipeCommitError::None);
	CHECK(rig.Count(CargoLabel{"STEL"}) == 0); CHECK(rig.Count(CargoLabel{"RARE"}) == 0); CHECK(rig.Count(CargoLabel{"GOOD"}) == 4);
	CHECK(rig.state->consumed.at(CargoLabel{"STEL"}) == 2); CHECK(rig.state->consumed.at(CargoLabel{"RARE"}) == 1);
	CHECK(rig.state->produced.at(CargoLabel{"GOOD"}) == 4); CHECK(rig.state->batches.at(MultiLayerRecipeID{"OGDS"}) == 1);
	CHECK(ValidateSpaceStructures());
}

/** Secondary material, whole-unit room and native packet-reference room each gate work before source energy can be consumed. */
TEST_CASE("Orbital Goodsworks refuses incomplete inputs and congested products unchanged", "[orbital-recipe-inventory]")
{
	OrbitalRecipeRig rig(SpaceBuildKind::Goodsworks); rig.Add(CargoLabel{"STEL"}, 2);
	const auto fault = GENERATE(0, 1, 2);
	if (fault != 0) rig.Add(CargoLabel{"RARE"}, 1);
	if (fault == 1) rig.Add(CargoLabel{"GOOD"}, 1024);
	if (fault == 2) {
		// Distinct real ages prevent native Append merging; every bucket/reference remains owned by the same original compartment.
		for (uint i = 0; i < MAX_ORBITAL_RECIPE_PACKET_REFERENCES; ++i) {
			rig.Add(CargoLabel{"GOOD"}, 1); HubCargoTransfer::Age(rig.Site().module->stock[GetCargoTypeByLabel(CargoLabel{"GOOD"})]);
		}
	}
	OrbitalRecipeInventory inventory(*rig.state); rig.Grant(); const auto digest = ComputeSpaceStructureDigest(), utility = ComputeUtilityPowerDigest();
	const auto quote = PrepareLayerRecipeWork(rig.Recipe(), inventory, inventory, 750);
	CHECK(quote.error != LayerRecipeCommitError::None); CHECK(CommitLayerRecipeWork(rig.Recipe(), inventory, inventory, quote) != LayerRecipeCommitError::None);
	CHECK(ComputeSpaceStructureDigest() == digest); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(rig.Count(CargoLabel{"STEL"}) == 2);
	CHECK(rig.state->electrical->Capture().paid_j == 0); CHECK(ValidateSpaceStructures());
}

/** Published unpowered alternatives use explicit unit baseline work and actual module stock; no electrical connection is invented. */
TEST_CASE("Orbital unpowered alternatives settle native packets through the same planner", "[orbital-recipe-inventory]")
{
	const bool ice = GENERATE(false, true);
	OrbitalRecipeRig rig(ice ? SpaceBuildKind::IceProcessor : SpaceBuildKind::Refinery);
	const CargoLabel input{ice ? "WICE" : "NORE"}, output{ice ? "PROP" : "RFMT"};
	rig.Add(input, ice ? 2 : 4); rig.state->unpowered_work_ticks = rig.Recipe().work_economy_ticks;
	OrbitalRecipeInventory inventory(*rig.state); const auto utility = ComputeUtilityPowerDigest();
	const auto proposal = PrepareLayerRecipeBatch(rig.Recipe(), inventory); REQUIRE(proposal.plan.batches == 1);
	REQUIRE(inventory.PrepareOutputs(proposal, true)); CHECK(CommitLayerRecipeBatch(rig.Recipe(), inventory, inventory, proposal).error == LayerRecipeCommitError::None);
	CHECK(rig.Count(input) == 0); CHECK(rig.Count(output) == (ice ? 1 : 2)); CHECK(rig.state->unpowered_work_ticks == 0);
	CHECK_FALSE(rig.state->electrical); CHECK(ComputeUtilityPowerDigest() == utility); CHECK(ValidateSpaceStructures());
}

/** Owner/incarnation, history representability and original electrical source each prevent unauthorized or unrecordable paid work. */
TEST_CASE("Orbital work refuses stale owners connections and exhausted history", "[orbital-recipe-inventory]")
{
	OrbitalRecipeRig rig; rig.Add(CargoLabel{"NORE"}, 4); OrbitalRecipeInventory inventory(*rig.state); rig.Grant();
	const auto quote = PrepareLayerRecipeWork(rig.Recipe(), inventory, inventory, 750); REQUIRE(quote.error == LayerRecipeCommitError::None);
	const auto fault = GENERATE(0, 1, 2, 3, 4);
	switch (fault) {
		case 0: ++rig.state->site.generation; break;
		case 1: ++rig.state->source.generation; break;
		case 2: rig.state->consumed.at(CargoLabel{"NORE"}) = UINT64_MAX; break;
		case 3: rig.state->revision = UINT64_MAX - 2; break;
		case 4: rig.state->batches.at(rig.state->choice) = UINT64_MAX; break;
	}
	const auto utility = ComputeUtilityPowerDigest();
	CHECK(CommitLayerRecipeWork(rig.Recipe(), inventory, inventory, quote) != LayerRecipeCommitError::None);
	CHECK(ComputeUtilityPowerDigest() == utility); CHECK(rig.Count(CargoLabel{"NORE"}) == 4); CHECK(rig.state->electrical->Capture().paid_j == 0);
}

/** Copied buffer admission uses only same-file cargo meaning, while native admission checks the actual configured cargo table. */
TEST_CASE("Orbital copied buffers resolve same-file labels independently of the active world", "[orbital-recipe-inventory]")
{
	OrbitalRecipeRig rig; rig.Add(CargoLabel{"NORE"}, 4); const auto cargo = GetCargoTypeByLabel(CargoLabel{"NORE"});
	LoadCheckData data; data.gameplay_profile_id = to_underlying(MultiLayerGameplayProfileID::MultilayerIndustry);
	data.native_cargo_dimensions_present = true; data.native_cargo_dimension_rows = {{cargo, CargoLabel{"NORE"}, CargoSpec::Get(cargo)->weight}};
	data.space_structure_rows = CaptureSpaceStructures(); REQUIRE(ValidateSpaceCargoPreviewDimensions(data)); REQUIRE(ValidateSpaceStructures());
	{
		AutoRestoreBackup meaning(CargoSpec::Get(cargo)->label, CargoLabel{"COAL"});
		CHECK(ValidateSpaceCargoPreviewDimensions(data)); CHECK_FALSE(ValidateSpaceStructures());
		OrbitalRecipeInventory inventory(*rig.state); CHECK_FALSE(inventory.Capture().active);
	}
	data.native_cargo_dimension_rows.front().label = CargoLabel{"GOOD"}; CHECK_FALSE(ValidateSpaceCargoPreviewDimensions(data));
	data.native_cargo_dimension_rows.front().label = CargoLabel{"NORE"}; data.space_structure_rows.back().module->quantities[cargo] = 1025;
	CHECK_FALSE(ValidateSpaceCargoPreviewDimensions(data)); CHECK(ValidateSpaceStructures());
}

/** Advance the actual common phase and native recipe callback with one explicit synchronized clock opportunity per unit step.
 * @param ticks Positive isolated native tick count; no fixture grant or work amount is injected.
 */
static void RunNativeRecipeTicks(uint ticks)
{
	// Each iteration uses the real common publisher/allocator and actual module callback; an explicit unit clock step is not a fabricated electrical or physical-work receipt.
	for (uint i = 0; i < ticks; ++i) { ++TimerGameTick::counter; PrepareTubePowerTick(); TickOrbitalRecipes(); }
}

/** Normal common allocation, pause/resume and real output commit retain one bounded native processing owner. */
TEST_CASE("Native orbital processor earns real shared work through pause and resume", "[orbital-recipe-runtime]")
{
	OrbitalRecipeRig rig; rig.Add(CargoLabel{"NORE"}, 4); ResetTubePowerState();
	AutoRestoreBackup pause(_pause_mode, PauseModes{}); AutoRestoreBackup tick(TimerGameTick::counter, uint64_t{0});
	auto &site = rig.Site(); auto &core = rig.Core(); auto &state = *site.module->processing;
	REQUIRE(CmdConnectOrbitalServices({DoCommandFlag::Execute}, site.index, site.generation, core.index, core.generation).Succeeded());
	REQUIRE(CmdControlOrbitalRecipe({DoCommandFlag::Execute}, site.index, site.generation, 0, static_cast<uint8_t>(OrbitalRecipeControl::Start)).Succeeded());
	CHECK(state.enabled); RunNativeRecipeTicks(17); CHECK(state.electrical->Capture().paid_j == 12750); CHECK(rig.Count(CargoLabel{"NORE"}) == 4);
	REQUIRE(ValidateOrbitalRecipes());
	REQUIRE(CmdControlOrbitalRecipe({DoCommandFlag::Execute}, site.index, site.generation, 0, static_cast<uint8_t>(OrbitalRecipeControl::Pause)).Succeeded());
	RunNativeRecipeTicks(3); CHECK(state.electrical->Capture().paid_j == 12750); CHECK(state.paid_lifetime_j == 12750);
	REQUIRE(CmdControlOrbitalRecipe({DoCommandFlag::Execute}, site.index, site.generation, 0, static_cast<uint8_t>(OrbitalRecipeControl::Resume)).Succeeded());
	RunNativeRecipeTicks(57); CHECK(rig.Count(CargoLabel{"NORE"}) == 0); CHECK(rig.Count(CargoLabel{"STEL"}) == 2);
	CHECK(state.batches.at(MultiLayerRecipeID{"OSTL"}) == 1); CHECK(state.residue_mass_sixteenths.at(MultiLayerRecipeID{"OSTL"}) == 32);
	CHECK(state.paid_lifetime_j == 55500); CHECK(state.electrical->Capture().paid_j == 0); REQUIRE(ValidateOrbitalRecipes());
	const auto rows = CaptureOrbitalRecipes(); TickOrbitalRecipes(); CHECK(CaptureOrbitalRecipes() == rows);
	RunNativeRecipeTicks(5); CHECK(state.paid_lifetime_j == 55500); CHECK(GetOrbitalRecipeWait(site) == OrbitalRecipeWait::Input);
	ResetTubePowerState();
}

/** Cancellation and validated private restore preserve spent energy and the same opportunity cap, while copied faults refuse independently. */
TEST_CASE("Native orbital cancellation restore and copied records retain exact paid history", "[orbital-recipe-runtime]")
{
	OrbitalRecipeRig rig; rig.Add(CargoLabel{"NORE"}, 4); ResetTubePowerState();
	AutoRestoreBackup pause(_pause_mode, PauseModes{}); AutoRestoreBackup tick(TimerGameTick::counter, uint64_t{0});
	auto &site = rig.Site(); auto &core = rig.Core(); auto &state = *site.module->processing;
	REQUIRE(CmdConnectOrbitalServices({DoCommandFlag::Execute}, site.index, site.generation, core.index, core.generation).Succeeded());
	REQUIRE(CmdControlOrbitalRecipe({DoCommandFlag::Execute}, site.index, site.generation, 0, static_cast<uint8_t>(OrbitalRecipeControl::Start)).Succeeded());
	RunNativeRecipeTicks(1); REQUIRE(state.electrical->Capture().paid_j == 750); const auto original = CaptureOrbitalRecipes();
	CHECK(CmdControlOrbitalRecipe({DoCommandFlag::Execute, DoCommandFlag::QueryCost}, site.index, site.generation, 0, static_cast<uint8_t>(OrbitalRecipeControl::Cancel)).Succeeded());
	CHECK(CaptureOrbitalRecipes() == original);
	REQUIRE(CmdControlOrbitalRecipe({DoCommandFlag::Execute}, site.index, site.generation, 0, static_cast<uint8_t>(OrbitalRecipeControl::Cancel)).Succeeded());
	CHECK(state.discarded_paid_j == 750); CHECK(state.paid_lifetime_j == 750); CHECK(state.electrical->Capture().paid_j == 0);
	const auto saved = state.electrical->Capture(); REQUIRE(ValidateOrbitalRecipes());
	// Replace this sole existing private owner only after complete native joins pass; the detached record is never allowed to add another active owner or earn work independently.
	state.electrical.reset(); state.electrical = LayerRecipeElectricalWork::Restore(rig.Recipe(), saved, MapLayer::Space); REQUIRE(state.electrical);
	CHECK(state.electrical->Capture() == saved);
	REQUIRE(CmdControlOrbitalRecipe({DoCommandFlag::Execute}, site.index, site.generation, 0, static_cast<uint8_t>(OrbitalRecipeControl::Start)).Succeeded());
	TickOrbitalRecipes(); CHECK(state.paid_lifetime_j == 750); CHECK(state.electrical->Capture().paid_j == 0);
	RunNativeRecipeTicks(1); CHECK(state.paid_lifetime_j == 1500); CHECK(state.electrical->Capture().paid_j == 750); REQUIRE(ValidateOrbitalRecipes());
	auto rows = CaptureOrbitalRecipes(); const auto sites = CaptureSpaceStructures(); const auto utility = CaptureUtilityPowerState();
	REQUIRE(ValidateOrbitalRecipeRecords(rows, sites, utility, TimerGameTick::counter, GetSpaceStructureRetirementHistory()));
	++rows.front().consumed.at(CargoLabel{"NORE"}); CHECK_FALSE(ValidateOrbitalRecipeRecords(rows, sites, utility, TimerGameTick::counter, GetSpaceStructureRetirementHistory()));
	rows = CaptureOrbitalRecipes(); ++rows.front().source.generation; CHECK_FALSE(ValidateOrbitalRecipeRecords(rows, sites, utility, TimerGameTick::counter, GetSpaceStructureRetirementHistory()));
	rows = CaptureOrbitalRecipes(); ++rows.front().electrical->interval_paid_j; CHECK_FALSE(ValidateOrbitalRecipeRecords(rows, sites, utility, TimerGameTick::counter, GetSpaceStructureRetirementHistory()));
	CHECK(rig.Count(CargoLabel{"NORE"}) == 4); CHECK(state.discarded_paid_j == 750); ResetTubePowerState();
}

/** Published unpowered processing also receives only actual native baseline opportunities, without borrowing its Core's energy. */
TEST_CASE("Native unpowered orbital recipes earn bounded baseline work and preserve explicit choice", "[orbital-recipe-runtime]")
{
	OrbitalRecipeRig rig(SpaceBuildKind::Refinery); rig.Add(CargoLabel{"NORE"}, 4); ResetTubePowerState();
	AutoRestoreBackup pause(_pause_mode, PauseModes{}); AutoRestoreBackup tick(TimerGameTick::counter, uint64_t{0});
	auto &site = rig.Site(); auto &state = *site.module->processing;
	REQUIRE(CmdControlOrbitalRecipe({DoCommandFlag::Execute}, site.index, site.generation, 0, static_cast<uint8_t>(OrbitalRecipeControl::Start)).Succeeded());
	RunNativeRecipeTicks(37); CHECK(state.unpowered_work_ticks == 37); CHECK(rig.Count(CargoLabel{"NORE"}) == 4); REQUIRE(ValidateOrbitalRecipes());
	RunNativeRecipeTicks(37); CHECK(state.unpowered_work_ticks == 0); CHECK(rig.Count(CargoLabel{"NORE"}) == 0); CHECK(rig.Count(CargoLabel{"RFMT"}) == 2);
	CHECK(state.batches.at(MultiLayerRecipeID{"OREF"}) == 1); CHECK(state.paid_lifetime_j == 0); CHECK_FALSE(state.electrical); REQUIRE(ValidateOrbitalRecipes());
	ResetTubePowerState();
}

/** Explicit evacuation and actual native removal transfer completed processing history into the same bounded structure authority. */
TEST_CASE("Native orbital removal retains exact completed recipe and residue history", "[orbital-recipe-runtime]")
{
	OrbitalRecipeRig rig; rig.Add(CargoLabel{"NORE"}, 4); ResetTubePowerState();
	AutoRestoreBackup pause(_pause_mode, PauseModes{}); AutoRestoreBackup tick(TimerGameTick::counter, uint64_t{0});
	auto &site = rig.Site(); auto &core = rig.Core(); const auto ref = GetSpaceStructureReference(site);
	REQUIRE(CmdConnectOrbitalServices({DoCommandFlag::Execute}, site.index, site.generation, core.index, core.generation).Succeeded());
	REQUIRE(CmdControlOrbitalRecipe({DoCommandFlag::Execute}, site.index, site.generation, 0, static_cast<uint8_t>(OrbitalRecipeControl::Start)).Succeeded());
	RunNativeRecipeTicks(74); const auto before = ComputeSpaceStructureDigest();
	CHECK(CmdRemoveOrbitalModule({}, ref.structure, ref.generation).Failed());
	REQUIRE(CmdEvacuateOrbitalModule({DoCommandFlag::Execute, DoCommandFlag::QueryCost}, ref.structure, ref.generation, core.index, core.generation).Succeeded());
	CHECK(ComputeSpaceStructureDigest() == before);
	REQUIRE(CmdEvacuateOrbitalModule({DoCommandFlag::Execute}, ref.structure, ref.generation, core.index, core.generation).Succeeded());
	CHECK(core.module->stock[GetCargoTypeByLabel(CargoLabel{"STEL"})].TotalCount() == 2);
	REQUIRE(CmdRemoveOrbitalModule({DoCommandFlag::Execute}, ref.structure, ref.generation).Succeeded());
	CHECK(ResolveSpaceStructure(ref) == nullptr); CHECK(CaptureOrbitalRecipes().empty()); CHECK(ValidateSpaceStructures());
	const auto &history = GetSpaceStructureRetirementHistory(); CHECK(history.structures_removed == 1); CHECK(history.processing_batches.at(MultiLayerRecipeID{"OSTL"}) == 1);
	CHECK(GetOrbitalRecipeRetirementEnergy(history) == std::optional<uint64_t>{55500});
	CHECK(CmdRemoveOrbitalModule({DoCommandFlag::Execute}, ref.structure, ref.generation).Failed()); CHECK(history.processing_batches.at(MultiLayerRecipeID{"OSTL"}) == 1);
	ResetTubePowerState();
}

/** Terminal physical stamps refuse more production but still permit explicit no-refund work cancellation and account cleanup. */
TEST_CASE("Native orbital terminal revisions retain safe cancellation and disconnection", "[orbital-recipe-runtime]")
{
	OrbitalRecipeRig rig; rig.Add(CargoLabel{"NORE"}, 4); ResetTubePowerState();
	AutoRestoreBackup pause(_pause_mode, PauseModes{}); AutoRestoreBackup tick(TimerGameTick::counter, uint64_t{0});
	auto &site = rig.Site(); auto &core = rig.Core(); auto &state = *site.module->processing;
	REQUIRE(CmdConnectOrbitalServices({DoCommandFlag::Execute}, site.index, site.generation, core.index, core.generation).Succeeded());
	REQUIRE(CmdControlOrbitalRecipe({DoCommandFlag::Execute}, site.index, site.generation, 0, static_cast<uint8_t>(OrbitalRecipeControl::Start)).Succeeded());
	RunNativeRecipeTicks(1); REQUIRE(state.electrical->Capture().paid_j == 750); state.revision = UINT64_MAX - 1;
	const auto utility = ComputeUtilityPowerDigest();
	REQUIRE(CmdControlOrbitalRecipe({DoCommandFlag::Execute}, site.index, site.generation, 0, static_cast<uint8_t>(OrbitalRecipeControl::Cancel)).Succeeded());
	CHECK(state.revision == UINT64_MAX - 1); CHECK(state.discarded_paid_j == 750); CHECK(ComputeUtilityPowerDigest() == utility);
	const auto after = CaptureOrbitalRecipes();
	CHECK(CmdControlOrbitalRecipe({DoCommandFlag::Execute}, site.index, site.generation, 0, static_cast<uint8_t>(OrbitalRecipeControl::Cancel)).Succeeded());
	CHECK(CaptureOrbitalRecipes() == after);
	REQUIRE(CmdDisconnectOrbitalServices({DoCommandFlag::Execute}, site.index, site.generation).Succeeded());
	CHECK_FALSE(state.electrical); CHECK(state.paid_lifetime_j == 750); CHECK(state.discarded_paid_j == 750); CHECK(ValidateOrbitalRecipes());
	CHECK(rig.Count(CargoLabel{"NORE"}) == 4); ResetTubePowerState();
}

/** Two actual processors share the same finite original Core; exact partial joules survive below-whole-tick progress without compressing the recipe. */
TEST_CASE("Native orbital processors retain fractional work from their actual shared panel", "[orbital-recipe-runtime]")
{
	OrbitalRecipeRig rig(SpaceBuildKind::Steelworks, {}, true); rig.Add(CargoLabel{"NORE"}, 4); ResetTubePowerState();
	AutoRestoreBackup pause(_pause_mode, PauseModes{}); AutoRestoreBackup tick(TimerGameTick::counter, uint64_t{0});
	auto &first = rig.Site(); auto &second = *SpaceStructure::Get(SpaceStructureID{2}); auto &core = rig.Core();
	const auto cargo = GetCargoTypeByLabel(CargoLabel{"NORE"}); REQUIRE(CargoPacket::CanAllocateItem());
	second.module->stock[cargo].Append(CargoPacket::Create(StationID::Invalid(), uint16_t{4}, Source{Source::Invalid, SourceType::Industry}), StationID::Invalid());
	++second.module->processing->revision;
	// Both actual module identities request their own original bounded consumer; native deterministic allocation, not a fixture grant, divides the one real810-J panel opportunity.
	for (auto *site : {&first, &second}) {
		REQUIRE(CmdConnectOrbitalServices({DoCommandFlag::Execute}, site->index, site->generation, core.index, core.generation).Succeeded());
		REQUIRE(CmdControlOrbitalRecipe({DoCommandFlag::Execute}, site->index, site->generation, 0, static_cast<uint8_t>(OrbitalRecipeControl::Start)).Succeeded());
	}
	RunNativeRecipeTicks(13); CHECK(first.module->processing->electrical->Capture().paid_j == 9750);
	CHECK(second.module->processing->electrical->Capture().paid_j == 780); CHECK(second.module->processing->electrical->WorkTicks() == 1);
	CHECK(second.module->processing->electrical->Capture().interval_paid_j == 60); CHECK(GetUtilityPowerProducer(core.module->producer)->consumed_lifetime_j == 10530);
	CHECK(second.module->stock[cargo].TotalCount() == 4); CHECK(ValidateOrbitalRecipes()); ResetTubePowerState();
}

