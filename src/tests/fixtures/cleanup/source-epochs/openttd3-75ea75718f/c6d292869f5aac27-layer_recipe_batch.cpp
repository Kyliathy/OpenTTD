/*
 * This file is part of OpenTTD.
 * OpenTTD is free software under the GNU General Public License, version 2.
 * See <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file layer_recipe_batch.cpp Shared all-input transaction boundaries and actual utility participant admission. @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md, Groups 7/13; r2mlvs12-solar-farms-mirrors-and-eti.md Group 8 */

#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../multilayer_industry.h"
#include "excavation_fixture.h"
#include "utility_power_fixture.h"
#include "../core/backup_type.hpp"

#include "../safeguards.h"

/** Test-owned facility stock implements the actual public view/commit boundary; it publishes no runtime IDs or grid. */
class RecipeBatchTestFacility : public LayerRecipeInventoryView, public LayerRecipeCommitParticipant {
public:
	LayerRecipeInventorySnapshot stock; ///< This test facility's actual input/output/work owner.
	bool refuse_validation = false; ///< Failure injection before any write.
	bool refuse_commit = false; ///< Failure injection at the complete commit boundary.
	uint commits = 0; ///< Actual whole commits, distinct from pure quotes.

	/** @param recipe Selected valid definition whose roles initialize bounded stock for this test owner. */
	explicit RecipeBatchTestFacility(const MultiLayerRecipe &recipe)
	{
		stock.recipe = recipe.id;
		stock.layer = MapLayer::Surface;
		stock.revision = 1;
		stock.active = true;
		stock.work_ticks = static_cast<uint64_t>(recipe.work_economy_ticks) * recipe.max_batches;
		// Deliver each required role into this test owner's real stock; no copied gameplay inventory supplies a batch.
		for (const auto &input : recipe.inputs) stock.inputs.push_back({input.label, input.quantity * recipe.max_batches, 1024});
		// Every product has its own bounded slot, so a full secondary slot must prevent all input debits.
		for (const auto &output : recipe.outputs) stock.outputs.push_back({output.label, 0, 1024});
	}

	/** @return Pure snapshot of this facility's actual stocks; no quote advances work or a revision. */
	LayerRecipeInventorySnapshot Capture() const override { return this->stock; }
	/** @return This exact actual view, rather than a proxy for another facility. */
	const LayerRecipeInventoryView &Inventory() const override { return *this; }

	/** @param proposal Complete checked stock/work proposal. @return Whether every actual slot still accepts its delta. */
	bool Validate(const LayerRecipeBatchProposal &proposal) const override
	{
		// Failure injection and a changed owner revision refuse the whole transaction before any physical stock write.
		if (this->refuse_validation || proposal.before.revision != stock.revision || proposal.work_ticks > stock.work_ticks) return false;
		// Recheck every delivered input against this actual owner, independently of the copied quote.
		for (const auto &input : proposal.inputs) {
			const auto slot = std::ranges::find(stock.inputs, input.label, &MultiLayerRecipeStock::label);
			if (slot == stock.inputs.end()) return false;
			if (slot->amount < input.amount) return false;
		}
		// Every output must fit before the non-failing commit starts; partial output writes cannot consume any input.
		for (const auto &output : proposal.outputs) {
			const auto slot = std::ranges::find(stock.outputs, output.label, &MultiLayerRecipeStock::label);
			if (slot == stock.outputs.end()) return false;
			if (output.amount > slot->capacity - slot->amount) return false;
		}
		return true;
	}

	/** @param proposal Fully revalidated complete transaction. @return Whole commit or unchanged injected refusal. */
	bool Commit(const LayerRecipeBatchProposal &proposal) noexcept override
	{
		if (this->refuse_commit) return false;
		if (!this->Validate(proposal)) return false;
		// Every lookup and arithmetic operation has been admitted before these non-allocating writes begin.
		for (const auto &input : proposal.inputs) std::ranges::find(stock.inputs, input.label, &MultiLayerRecipeStock::label)->amount -= input.amount;
		for (const auto &output : proposal.outputs) std::ranges::find(stock.outputs, output.label, &MultiLayerRecipeStock::label)->amount += output.amount;
		stock.work_ticks -= proposal.work_ticks;
		++stock.revision;
		++this->commits;
		return true;
	}
};

/** @return The content owner's unchanged bootstrap Copper/Coal refinery definition. */
static const MultiLayerRecipe &BatchTestRecipe() { return GetMultiLayerManufacturingRecipes().front().recipe; }

/** Test-owned public inventory participant reads real ledger availability; it owns no electrical amount or source identity. */
class PoweredRecipeBatchTestFacility : public RecipeBatchTestFacility {
	UtilityGrantID grant; ///< Exact already registered consumer/epoch, issued by the real utility owner.
public:
	/** @param recipe Immutable test definition with bounded electrical demand. @param grant Actual ledger-issued reservation. */
	PoweredRecipeBatchTestFacility(const MultiLayerRecipe &recipe, UtilityGrantID grant) : RecipeBatchTestFacility(recipe), grant(grant) {}
	/** @return Actual material/work snapshot plus a read-only projection of this owner's real current electrical balance. */
	LayerRecipeInventorySnapshot Capture() const override
	{
		auto snapshot = RecipeBatchTestFacility::Capture();
		const auto allocation = GetUtilityAllocation(this->grant.consumer);
		snapshot.power_j = allocation ? allocation->granted_energy_j : 0;
		return snapshot;
	}
	/** @param result Receives the actual registered consumer/epoch. @return Whether this inventory has an explicit real reservation. */
	bool GetElectricalGrant(UtilityGrantID &result) const override { result = this->grant; return true; }
};

/** A failed complete recipe cannot consume energy before material/output/work admission; success settles the same original source once. */
TEST_CASE("Shared powered recipes commit actual ledger energy with their complete inventory participant", "[utility-power-recipe]")
{
	ExcavationFixture world;
	UtilityPowerReset ledger;
	const auto producer = RegisterUtilityPowerProducer(_current_company, MapLayer::Surface, UtilityGenerationKind::External);
	REQUIRE(producer.has_value());
	const auto consumer = RegisterUtilityPowerConsumer(_current_company, MapLayer::Surface, UtilityConsumerEffect::ProcessorBoost, 81, std::array{*producer});
	REQUIRE(consumer.has_value());
	REQUIRE(PublishUtilityGeneration(*producer, 1, 3, 1000) == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityPower(1, std::array{UtilityPowerRequest{*consumer, 81}}) == UtilityPowerError::None);
	}
	MultiLayerRecipe recipe = BatchTestRecipe();
	recipe.max_batches = 2; recipe.power_j_per_batch = 10;
	PoweredRecipeBatchTestFacility facility(recipe, {*consumer, 1});
	SECTION("complete inventory validation refuses") { facility.refuse_validation = true; }
	SECTION("complete inventory commit refuses") { facility.refuse_commit = true; }
	SECTION("another layer cannot borrow this reservation") { facility.stock.layer = MapLayer::Underground; }
	SECTION("complete inventory and actual energy settle together") {}
	const auto before = facility.Capture();
	const auto digest = ComputeUtilityPowerDigest();
	const auto proposal = PrepareLayerRecipeBatch(recipe, facility);
	REQUIRE(proposal.plan.batches == 2);
	const auto receipt = CommitLayerRecipeBatch(recipe, facility, facility, proposal);
	// Either owner refusal or a physical layer mismatch blocks the whole
	// material/work/energy transaction; none may be partially settled.
	if (facility.refuse_validation || facility.refuse_commit || facility.stock.layer != MapLayer::Surface) {
		CHECK(receipt.error == LayerRecipeCommitError::Refused);
		CHECK(facility.stock.inputs[0].amount == before.inputs[0].amount);
		CHECK(facility.stock.outputs[0].amount == 0);
		CHECK(facility.stock.work_ticks == before.work_ticks);
		CHECK(ComputeUtilityPowerDigest() == digest);
	} else {
		REQUIRE(receipt.error == LayerRecipeCommitError::None);
		CHECK(receipt.batches == 2);
		CHECK(facility.stock.inputs[0].amount == 0);
		CHECK(facility.stock.outputs[0].amount == proposal.outputs[0].amount);
		CHECK(GetUtilityAllocation(*consumer)->granted_energy_j == 61);
		CHECK(CaptureUtilityPowerState().producers.front().consumed_lifetime_j == 20);
		CHECK(CommitLayerRecipeBatch(recipe, facility, facility, proposal).error == LayerRecipeCommitError::Stale);
		CHECK(GetUtilityAllocation(*consumer)->granted_energy_j == 61);
	}
}

/** Incomplete material admission consumes no electricity; cancelling a quote releases only its unused real reservation. */
TEST_CASE("Powered recipe pauses and cancelled quotes preserve their sole material and energy owners", "[utility-power-recipe]")
{
	ExcavationFixture world;
	UtilityPowerReset ledger;
	const auto producer = RegisterUtilityPowerProducer(_current_company, MapLayer::Surface, UtilityGenerationKind::External);
	REQUIRE(producer.has_value());
	const auto consumer = RegisterUtilityPowerConsumer(_current_company, MapLayer::Surface, UtilityConsumerEffect::ProcessorBoost, 81, std::array{*producer});
	REQUIRE(consumer.has_value());
	REQUIRE(PublishUtilityGeneration(*producer, 1, 3, 1000) == UtilityPowerError::None);
	{
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(AllocateUtilityPower(1, std::array{UtilityPowerRequest{*consumer, 81}}) == UtilityPowerError::None);
	}
	MultiLayerRecipe recipe = BatchTestRecipe();
	recipe.max_batches = 2;
	recipe.power_j_per_batch = 10;
	PoweredRecipeBatchTestFacility facility(recipe, {*consumer, 1});
	SECTION("missing transported secondary input") { facility.stock.inputs.back().amount = 0; }
	SECTION("full product slot") { facility.stock.outputs.front().amount = facility.stock.outputs.front().capacity; }
	SECTION("cancel an admitted quote") {}
	const auto before = facility.Capture();
	const auto digest = ComputeUtilityPowerDigest();
	const auto quote = PrepareLayerRecipeBatch(recipe, facility);
	// A material pause owns no electrical debit, even though this exact facility
	// has a positive real allocation. Quotes themselves reserve no new power.
	if (quote.plan.batches == 0) {
		const auto receipt = CommitLayerRecipeBatch(recipe, facility, facility, quote);
		CHECK(receipt.error == LayerRecipeCommitError::None);
		CHECK(receipt.batches == 0);
		CHECK(ComputeUtilityPowerDigest() == digest);
	}
	CHECK(facility.commits == 0);
	CHECK(facility.stock.work_ticks == before.work_ticks);
	CHECK(GetUtilityAllocation(*consumer)->granted_energy_j == 81);
	// The job owns cancellation: discard its transient quote and explicitly
	// expire unused ledger energy. Release never consumes stock or source fuel.
	const auto release = PrepareUtilityMutation(UtilityMutationKind::ReleaseUnused, {*consumer, 1}, 81);
	REQUIRE(CommitUtilityMutation(release).error == UtilityPowerError::None);
	CHECK(GetUtilityAllocation(*consumer)->granted_energy_j == 0);
	CHECK(CaptureUtilityPowerState().producers.front().consumed_lifetime_j == 0);
	CHECK(CommitLayerRecipeBatch(recipe, facility, facility, quote).error == LayerRecipeCommitError::Stale);
	CHECK(CommitUtilityMutation(release).error == UtilityPowerError::StaleMutation);
	CHECK(facility.stock.inputs.front().amount == before.inputs.front().amount);
	CHECK(facility.stock.inputs.back().amount == before.inputs.back().amount);
	CHECK(facility.stock.outputs.front().amount == before.outputs.front().amount);
	CHECK(facility.commits == 0);
}

/** Missing secondary input and full secondary output retain every delivered input and every work tick. */
TEST_CASE("Shared recipe batches require all inputs and every output slot", "[layer-recipe-batch]")
{
	ExcavationFixture world; // The planner needs an allocated layer and the registered cargoes, also when this case runs alone.
	const std::array<MultiLayerRecipeCargo, 2> outputs{{{CargoLabel{"RFMT"}, 2}, {CargoLabel{"CHEM"}, 1}}};
	MultiLayerRecipe recipe = BatchTestRecipe();
	recipe.outputs = outputs; // Test-only valid multi-output shape; no content registry or runtime definition is installed.
	RecipeBatchTestFacility facility(recipe);
	SECTION("Missing Coal preserves Copper") { facility.stock.inputs[1].amount = 0; }
	SECTION("Full secondary output preserves both inputs") { facility.stock.outputs[1].amount = facility.stock.outputs[1].capacity; }
	SECTION("Inactive facility preserves delivered buffers") { facility.stock.active = false; }
	const auto before = facility.Capture();
	const auto proposal = PrepareLayerRecipeBatch(recipe, facility);
	CHECK(proposal.plan.batches == 0);
	CHECK(CommitLayerRecipeBatch(recipe, facility, facility, proposal).batches == 0);
	CHECK(facility.stock.inputs[0].amount == before.inputs[0].amount);
	CHECK(facility.stock.inputs[1].amount == before.inputs[1].amount);
	CHECK(facility.stock.outputs[0].amount == before.outputs[0].amount);
	CHECK(facility.stock.outputs[1].amount == before.outputs[1].amount);
	CHECK(facility.stock.work_ticks == before.work_ticks);
	CHECK(facility.stock.revision == before.revision);
	CHECK(facility.commits == 0);
}

/** A whole transaction consumes each role and work once; a retry cannot create a second output from the same quote. */
TEST_CASE("Shared recipe commit preserves exact all-input deltas and refuses a retry", "[layer-recipe-batch]")
{
	ExcavationFixture world; // The planner needs an allocated layer and the registered cargoes, also when this case runs alone.
	const auto &recipe = BatchTestRecipe();
	RecipeBatchTestFacility facility(recipe);
	const auto before = facility.Capture();
	const auto proposal = PrepareLayerRecipeBatch(recipe, facility);
	CHECK(proposal.plan.batches == 4);
	CHECK(facility.stock.revision == before.revision);
	const auto receipt = CommitLayerRecipeBatch(recipe, facility, facility, proposal);
	CHECK(receipt.error == LayerRecipeCommitError::None);
	CHECK(receipt.batches == 4);
	CHECK(facility.stock.inputs[0].amount == 0);
	CHECK(facility.stock.inputs[1].amount == 0);
	CHECK(facility.stock.outputs[0].amount == 8);
	CHECK(facility.stock.work_ticks == 0);
	CHECK(facility.commits == 1);
	CHECK(CommitLayerRecipeBatch(recipe, facility, facility, proposal).error == LayerRecipeCommitError::Stale);
	CHECK(facility.stock.outputs[0].amount == 8);
	CHECK(facility.commits == 1);
}

/** Foreign, edited and stale proposals or participant refusals cannot debit the actual owner. */
TEST_CASE("Recipe quotes bind their exact owner state and complete deltas", "[layer-recipe-batch]")
{
	ExcavationFixture world; // The planner needs an allocated layer and the registered cargoes, also when this case runs alone.
	const auto &recipe = BatchTestRecipe();
	RecipeBatchTestFacility facility(recipe), foreign(recipe);
	auto proposal = PrepareLayerRecipeBatch(recipe, facility);
	LayerRecipeCommitError expected = LayerRecipeCommitError::Stale;
	SECTION("Changed secondary input without advancing revision") { --facility.stock.inputs[1].amount; }
	SECTION("Changed work balance") { --facility.stock.work_ticks; }
	SECTION("Changed output capacity") { --facility.stock.outputs[0].capacity; }
	SECTION("Changed selected alternative") { facility.stock.recipe = MultiLayerRecipeID{"BFMT"}; }
	SECTION("Changed owner revision") { ++facility.stock.revision; }
	SECTION("Edited input delta") { ++proposal.inputs[1].amount; expected = LayerRecipeCommitError::InvalidProposal; }
	SECTION("Edited work delta") { ++proposal.work_ticks; expected = LayerRecipeCommitError::InvalidProposal; }
	SECTION("Edited output delta") { ++proposal.outputs[0].amount; expected = LayerRecipeCommitError::InvalidProposal; }
	SECTION("Validation refusal") { facility.refuse_validation = true; expected = LayerRecipeCommitError::Refused; }
	SECTION("Commit refusal") { facility.refuse_commit = true; expected = LayerRecipeCommitError::Refused; }
	SECTION("Foreign participant") {
		CHECK(CommitLayerRecipeBatch(recipe, facility, foreign, proposal).error == LayerRecipeCommitError::WrongOwner);
		CHECK(foreign.commits == 0);
		return;
	}
	const auto before = facility.Capture();
	CHECK(CommitLayerRecipeBatch(recipe, facility, facility, proposal).error == expected);
	CHECK(facility.stock.inputs[0].amount == before.inputs[0].amount);
	CHECK(facility.stock.inputs[1].amount == before.inputs[1].amount);
	CHECK(facility.stock.outputs[0].amount == before.outputs[0].amount);
	CHECK(facility.stock.work_ticks == before.work_ticks);
	CHECK(facility.stock.revision == before.revision);
	CHECK(facility.commits == 0);
}

/** Wide work and near-limit narrow stock counters still admit only a complete bounded output. */
TEST_CASE("Recipe planner checks integer limits before complete commit", "[layer-recipe-batch]")
{
	MultiLayerRecipe recipe = BatchTestRecipe();
	recipe.work_economy_ticks = UINT32_MAX;
	RecipeBatchTestFacility facility(recipe);
	facility.stock.work_ticks = UINT64_MAX;
	facility.stock.outputs[0] = {CargoLabel{"RFMT"}, UINT16_MAX - 2, UINT16_MAX};
	const auto proposal = PrepareLayerRecipeBatch(recipe, facility);
	CHECK(proposal.plan.batches == 1);
	CHECK(proposal.work_ticks == UINT32_MAX);
	CHECK(CommitLayerRecipeBatch(recipe, facility, facility, proposal).batches == 1);
	CHECK(facility.stock.outputs[0].amount == UINT16_MAX);
	CHECK(facility.stock.work_ticks == UINT64_MAX - UINT32_MAX);
}

/** A number in a copied snapshot cannot stand in for the absent native grid/vent transaction owner. */
TEST_CASE("Recipe energy planning grants no scalar-only utility mutation", "[layer-recipe-batch]")
{
	Map::Allocate(64, 64, 4); // The pure Deep snapshot still requires an actually allocated layer; test order cannot supply geometry.
	const auto &recipe = GetMultiLayerGeothermalRecipes().front().recipe;
	RecipeBatchTestFacility facility(recipe);
	facility.stock.layer = MapLayer::DeepUnderground;
	facility.stock.heat_j = recipe.heat_j_per_batch * recipe.max_batches;
	facility.stock.power_j = recipe.power_j_per_batch * recipe.max_batches;
	const auto before = facility.Capture();
	const auto proposal = PrepareLayerRecipeBatch(recipe, facility);
	CHECK(proposal.plan.batches == recipe.max_batches);
	CHECK(CommitLayerRecipeBatch(recipe, facility, facility, proposal).error == LayerRecipeCommitError::Refused);
	CHECK(facility.stock.inputs[0].amount == before.inputs[0].amount);
	CHECK(facility.stock.outputs[0].amount == 0);
	CHECK(facility.stock.heat_j == before.heat_j);
	CHECK(facility.stock.power_j == before.power_j);
	CHECK(facility.stock.work_ticks == before.work_ticks);
	CHECK(facility.commits == 0);
}

/** Future dated direct-heat rows may be characterized with supplied scalar quantities; no such number grants custody of a native utility allocation. */
TEST_CASE("Future direct heat recipes retain every input on missing roles full output or absent utility owner", "[layer-recipe-heat-boundary]")
{
	Map::Allocate(64, 64, 4); // Allocate actual Deep geometry independently of earlier mine fixtures, without any geological/utility authority.
	const size_t index = GENERATE(0, 1, 2, 3, 4, 5);
	const auto &record = GetMultiLayerGeothermalRecipes()[index];
	RecipeBatchTestFacility facility(record.recipe);
	facility.stock.layer = record.layer;
	facility.stock.heat_j = record.recipe.heat_j_per_batch * record.recipe.max_batches;
	facility.stock.power_j = record.recipe.power_j_per_batch * record.recipe.max_batches;
	SECTION("all materials fit but the native grant authority is absent") {
		const auto proposal = PrepareLayerRecipeBatch(record.recipe, facility);
		CHECK(proposal.plan.batches == record.recipe.max_batches);
		CHECK(CommitLayerRecipeBatch(record.recipe, facility, facility, proposal).error == LayerRecipeCommitError::Refused);
	}
	SECTION("one transported role is absent") {
		facility.stock.inputs.back().amount = 0;
		const auto proposal = PrepareLayerRecipeBatch(record.recipe, facility);
		CHECK(proposal.plan.batches == 0);
		CHECK(CommitLayerRecipeBatch(record.recipe, facility, facility, proposal).batches == 0);
	}
	SECTION("one physical output slot is full") {
		facility.stock.outputs.front().amount = facility.stock.outputs.front().capacity;
		const auto proposal = PrepareLayerRecipeBatch(record.recipe, facility);
		CHECK(proposal.plan.batches == 0);
		CHECK(CommitLayerRecipeBatch(record.recipe, facility, facility, proposal).batches == 0);
	}
	CHECK(facility.stock.revision == 1);
	CHECK(facility.stock.work_ticks == static_cast<uint64_t>(record.recipe.work_economy_ticks) * record.recipe.max_batches);
	CHECK(facility.stock.heat_j == record.recipe.heat_j_per_batch * record.recipe.max_batches);
	CHECK(facility.stock.power_j == record.recipe.power_j_per_batch * record.recipe.max_batches);
	CHECK(facility.commits == 0);
}
