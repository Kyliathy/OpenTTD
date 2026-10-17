/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file layer_recipe_work.cpp Actual shared utility debits earn recipe-bound progress and settle one complete material batch.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group10
 * Isolated facility stocks and external generation verify the public producer boundary; they claim no paid orbital infrastructure or procurement.
 */
#include "../stdafx.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../layer_recipe_work.h"
#include "../core/backup_type.hpp"
#include "excavation_fixture.h"
#include "utility_power_fixture.h"
#include "../safeguards.h"

/** @return Exact published OSTL definition, retaining its real 74-tick/55,500-J bill and NORE/STEL units. */
static const MultiLayerRecipe &PaidSteelRecipe()
{
	const MultiLayerRecipe *selected = nullptr;
	// Only the published stable identity selects this electrical recipe; a cheaper local definition cannot make the staged-work producer appear usable.
	for (const auto &record : GetMultiLayerManufacturingRecipes()) if (record.recipe.id == MultiLayerRecipeID{"OSTL"}) selected = &record.recipe;
	REQUIRE(selected != nullptr); return *selected;
}

/** Isolated physical facility owns material stocks/revision; its shared noncopyable receipt owns the actual already-paid work. */
class PaidRecipeFacility final : public LayerRecipeInventoryView, public LayerRecipeElectricalWorkParticipant {
public:
	LayerRecipeInventorySnapshot stock; ///< Actual isolated unit-facility quantities, never copied gameplay packets.
	LayerRecipeElectricalWork work; ///< Same single shared paid-work owner throughout preparation and complete settlement.
	UtilityConsumerID consumer; ///< Existing actual ledger consumer; no local energy balance is maintained.
	const LayerRecipeElectricalWork *wrong_view = nullptr; ///< Explicit wrong-owner injection; mutable physical participant still exposes its original receipt.
	bool reject_work = false, fail_work = false, reject_batch = false, fail_batch = false; ///< Complete unchanged-refusal injections.
	bool fake_work = false; ///< A copied scalar duration must not replace the private actual paid receipt.
	uint batches = 0; ///< Observed actual complete material commits.

	/** @param recipe Exact selected immutable electrical recipe. @param consumer Existing native electrical reservation owner. */
	PaidRecipeFacility(const MultiLayerRecipe &recipe, UtilityConsumerID consumer) : work(recipe, consumer), consumer(consumer)
	{
		stock.recipe = recipe.id; stock.layer = MapLayer::Space; stock.revision = 1; stock.active = true;
		// Each original input/output role has one bounded physical unit-fixture slot; a full output must prevent every debit, including electricity for further work.
		for (const auto &input : recipe.inputs) stock.inputs.push_back({input.label, input.quantity, 1024});
		for (const auto &output : recipe.outputs) stock.outputs.push_back({output.label, 0, 1024});
	}
	/** @return Actual material/revision fields and work derived from the single shared receipt, without granting scalar power. */
	LayerRecipeInventorySnapshot Capture() const override
	{
		auto snapshot = stock; snapshot.work_ticks = fake_work ? PaidSteelRecipe().work_economy_ticks : work.WorkTicks();
		const auto allocation = GetUtilityAllocation(consumer); snapshot.power_j = allocation ? allocation->granted_energy_j : 0; return snapshot;
	}
	/** @return Exact actual physical inventory view used by both shared transactions. */
	const LayerRecipeInventoryView &Inventory() const override { return *this; }
	/** @return Original receipt, or explicit wrong-owner read-only injection. */
	const LayerRecipeElectricalWork *ElectricalWork() const override { return wrong_view == nullptr ? &work : wrong_view; }
	/** @return Same original mutable receipt; another view's progress cannot be settled here. */
	LayerRecipeElectricalWork *ElectricalWork() override { return &work; }
	/** @param grant Receives the actual current native reservation identity. @return Whether this facility has a current owned allocation. */
	bool GetElectricalGrant(UtilityGrantID &grant) const override { grant = {consumer, GetUtilityPowerInterval()}; return GetUtilityAllocation(consumer).has_value(); }
	/** @param proposal Original physical work quote. @return Whether the complete actual stock/revision owner still agrees. */
	bool ValidateWork(const LayerRecipeWorkProposal &proposal) const override { return !reject_work && EqualLayerRecipeInventory(proposal.before, Capture()); }
	/** @param proposal Checked actual work action. @return Whole unchanged refusal or physical revision advancement; only shared code updates paid work. */
	bool CommitWork([[maybe_unused]] const LayerRecipeWorkProposal &proposal) noexcept override
	{
		if (fail_work) return false;
		++stock.revision; return true;
	}
	/** @param proposal Original complete material quote. @return Whether all actual input/output slots and physical work still fit. */
	bool Validate(const LayerRecipeBatchProposal &proposal) const override
	{
		// Whole-owner refusal or changed physical state blocks all material writes; this callback never spends another utility grant.
		if (reject_batch || proposal.before.revision != stock.revision || proposal.work_ticks > work.WorkTicks()) return false;
		// Revalidate every physical debit and output bound before the non-allocating complete commit starts.
		for (const auto &input : proposal.inputs) {
			const auto slot = std::ranges::find(stock.inputs, input.label, &MultiLayerRecipeStock::label);
			if (slot == stock.inputs.end() || slot->amount < input.amount) return false;
		}
		for (const auto &output : proposal.outputs) {
			const auto slot = std::ranges::find(stock.outputs, output.label, &MultiLayerRecipeStock::label);
			if (slot == stock.outputs.end() || slot->amount > slot->capacity || output.amount > slot->capacity - slot->amount) return false;
		}
		return true;
	}
	/** @param proposal Fully revalidated material transaction. @return Complete success or unchanged injected refusal, retaining paid work on failure. */
	bool Commit(const LayerRecipeBatchProposal &proposal) noexcept override
	{
		if (fail_batch) return false;
		if (!Validate(proposal)) return false;
		// All slots already fit. These physical writes cannot fail; the same shared transaction debits the original private paid-work receipt afterward.
		for (const auto &input : proposal.inputs) std::ranges::find(stock.inputs, input.label, &MultiLayerRecipeStock::label)->amount -= input.amount;
		for (const auto &output : proposal.outputs) std::ranges::find(stock.outputs, output.label, &MultiLayerRecipeStock::label)->amount += output.amount;
		++stock.revision; ++batches; return true;
	}
};

/** Publish one actual 30-kW/27-ms opportunity and allocate at most its real bounded request.
 * @param producer Original real unit-fixture source.
 * @param consumer Original real unit-facility consumer.
 * @param interval Exact next native accounting opportunity, with unused old grants expired by the actual owner.
 */
static void GrantRecipeWork(UtilityProducerID producer, UtilityConsumerID consumer, uint64_t interval)
{
	if (interval != GetUtilityPowerInterval()) {
		AutoRestoreBackup system_actor(_current_company, OWNER_DEITY); REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None); REQUIRE(GetUtilityPowerInterval() == interval);
	}
	REQUIRE(PublishUtilityGeneration(producer, interval, 30000, 1) == UtilityPowerError::None);
	AutoRestoreBackup system_actor(_current_company, OWNER_DEITY); REQUIRE(AllocateUtilityPower(interval, std::array{UtilityPowerRequest{consumer, 750}}) == UtilityPowerError::None);
}

/** Complete one canonical electrical batch through 74 actual ledger opportunities, retaining whole inputs until material settlement.
 * @param recipe Original unchanged OSTL definition.
 * @param facility Actual isolated material/work owner.
 * @param producer Original real electrical source.
 */
static void EarnPaidRecipeBatch(const MultiLayerRecipe &recipe, PaidRecipeFacility &facility, UtilityProducerID producer)
{
	// Each opportunity pays only one actual 750-J work tick. Repeated API calls cannot compress a 74-tick recipe into one grant or spend the same opportunity twice.
	for (uint64_t interval = 1; interval <= recipe.work_economy_ticks; ++interval) {
		GrantRecipeWork(producer, facility.consumer, interval); const auto quote = PrepareLayerRecipeWork(recipe, facility, facility, 750);
		REQUIRE(quote.error == LayerRecipeCommitError::None); REQUIRE(CommitLayerRecipeWork(recipe, facility, facility, quote) == LayerRecipeCommitError::None);
	}
}

/** Split grants earn exact physical work; only complete original material settlement consumes the paid receipt once. */
TEST_CASE("Shared staged work pays each real opportunity and settles one original complete batch", "[layer-recipe-work]")
{
	ExcavationFixture world; UtilityPowerReset ledger; const auto &recipe = PaidSteelRecipe();
	const auto producer = RegisterUtilityPowerProducer(_current_company, MapLayer::Space, UtilityGenerationKind::External); REQUIRE(producer);
	const auto consumer = RegisterUtilityPowerConsumer(_current_company, MapLayer::Space, UtilityConsumerEffect::AssemblyWork, 750, std::array{*producer}); REQUIRE(consumer);
	PaidRecipeFacility facility(recipe, *consumer); GrantRecipeWork(*producer, *consumer, 1);
	const auto first = PrepareLayerRecipeWork(recipe, facility, facility, 300); REQUIRE(first.energy_j == 300); REQUIRE(CommitLayerRecipeWork(recipe, facility, facility, first) == LayerRecipeCommitError::None);
	CHECK(facility.work.WorkTicks() == 0); CHECK(facility.work.Capture().paid_j == 300); CHECK(facility.stock.inputs[0].amount == 4); CHECK(CommitLayerRecipeWork(recipe, facility, facility, first) == LayerRecipeCommitError::Stale);
	const auto rest = PrepareLayerRecipeWork(recipe, facility, facility, 450); REQUIRE(rest.energy_j == 450); REQUIRE(CommitLayerRecipeWork(recipe, facility, facility, rest) == LayerRecipeCommitError::None);
	CHECK(facility.work.WorkTicks() == 1); CHECK(facility.work.Capture().paid_j == 750); CHECK(PrepareLayerRecipeWork(recipe, facility, facility, 750).error == LayerRecipeCommitError::Refused);
	// Remaining actual opportunities preserve the original complete input bill until all74 paid work ticks are present.
	for (uint64_t interval = 2; interval <= 74; ++interval) {
		GrantRecipeWork(*producer, *consumer, interval); const auto work = PrepareLayerRecipeWork(recipe, facility, facility, 750);
		REQUIRE(CommitLayerRecipeWork(recipe, facility, facility, work) == LayerRecipeCommitError::None); CHECK(facility.stock.inputs[0].amount == 4); CHECK(facility.stock.outputs[0].amount == 0);
	}
	REQUIRE(facility.work.WorkTicks() == 74); CHECK(GetUtilityPowerProducer(*producer)->consumed_lifetime_j == 55500);
	const auto batch = PrepareLayerRecipeBatch(recipe, facility); REQUIRE(batch.plan.batches == 1); CHECK(batch.power_j == 0); CHECK(batch.paid_power_j == 55500);
	const auto power = ComputeUtilityPowerDigest(); REQUIRE(CommitLayerRecipeBatch(recipe, facility, facility, batch).error == LayerRecipeCommitError::None);
	CHECK(ComputeUtilityPowerDigest() == power); CHECK(facility.stock.inputs[0].amount == 0); CHECK(facility.stock.outputs[0].amount == 2); CHECK(facility.work.WorkTicks() == 0); CHECK(facility.batches == 1);
	CHECK(CommitLayerRecipeBatch(recipe, facility, facility, batch).error != LayerRecipeCommitError::None); CHECK(PrepareLayerRecipeWork(recipe, facility, facility, 750).error == LayerRecipeCommitError::Refused);
}

/** Invalid physical bounds, scalar work, wrong receipt or either failing owner preserve both material and actual electricity. */
TEST_CASE("Shared staged work refuses whole physical and copied scalar faults without paying", "[layer-recipe-work]")
{
	ExcavationFixture world; UtilityPowerReset ledger; const auto &recipe = PaidSteelRecipe();
	const auto producer = RegisterUtilityPowerProducer(_current_company, MapLayer::Space, UtilityGenerationKind::External); REQUIRE(producer);
	const auto consumer = RegisterUtilityPowerConsumer(_current_company, MapLayer::Space, UtilityConsumerEffect::AssemblyWork, 750, std::array{*producer}); REQUIRE(consumer);
	PaidRecipeFacility facility(recipe, *consumer), other(recipe, *consumer); GrantRecipeWork(*producer, *consumer, 1);
	SECTION("missing delivered input") { facility.stock.inputs[0].amount = 0; }
	SECTION("full actual output") { facility.stock.outputs[0].amount = facility.stock.outputs[0].capacity; }
	SECTION("inactive actual facility") { facility.stock.active = false; }
	SECTION("another physical receipt") { facility.wrong_view = &other.work; }
	SECTION("copied unpaid whole work") { facility.fake_work = true; }
	SECTION("another saved alternative") { facility.stock.recipe = MultiLayerRecipeID{"OGDS"}; }
	SECTION("physical validation refuses") { facility.reject_work = true; }
	SECTION("physical commit refuses") { facility.fail_work = true; }
	const auto before = facility.Capture(); const auto receipt = facility.work.Capture(); const auto power = ComputeUtilityPowerDigest(); const auto quote = PrepareLayerRecipeWork(recipe, facility, facility, 750);
	if (quote.error == LayerRecipeCommitError::None) CHECK(CommitLayerRecipeWork(recipe, facility, facility, quote) != LayerRecipeCommitError::None);
	CHECK(PrepareLayerRecipeBatch(recipe, facility).plan.batches == 0);
	CHECK(EqualLayerRecipeInventory(before, facility.Capture())); CHECK(facility.work.Capture() == receipt); CHECK(ComputeUtilityPowerDigest() == power); CHECK(facility.batches == 0);
}

/** A failed material participant retains all actual paid work without renewing or charging the source again. */
TEST_CASE("Failed complete batch preserves fully paid shared work for a later actual retry", "[layer-recipe-work]")
{
	ExcavationFixture world; UtilityPowerReset ledger; const auto &recipe = PaidSteelRecipe();
	const auto producer = RegisterUtilityPowerProducer(_current_company, MapLayer::Space, UtilityGenerationKind::External); REQUIRE(producer);
	const auto consumer = RegisterUtilityPowerConsumer(_current_company, MapLayer::Space, UtilityConsumerEffect::AssemblyWork, 750, std::array{*producer}); REQUIRE(consumer);
	PaidRecipeFacility facility(recipe, *consumer); EarnPaidRecipeBatch(recipe, facility, *producer); const auto batch = PrepareLayerRecipeBatch(recipe, facility); REQUIRE(batch.plan.batches == 1);
	auto forged = batch; forged.paid_power_j = 0; CHECK(CommitLayerRecipeBatch(recipe, facility, facility, forged).error == LayerRecipeCommitError::InvalidProposal);
	forged = batch; ++forged.electrical_work->consumer; CHECK(CommitLayerRecipeBatch(recipe, facility, facility, forged).error == LayerRecipeCommitError::InvalidProposal);
	SECTION("validation refusal") { facility.reject_batch = true; }
	SECTION("output allocation or physical commit refusal") { facility.fail_batch = true; }
	const auto before = facility.Capture(); const auto receipt = facility.work.Capture(); const auto power = ComputeUtilityPowerDigest();
	CHECK(CommitLayerRecipeBatch(recipe, facility, facility, batch).error == LayerRecipeCommitError::Refused); CHECK(EqualLayerRecipeInventory(before, facility.Capture())); CHECK(facility.work.Capture() == receipt); CHECK(ComputeUtilityPowerDigest() == power);
	facility.reject_batch = false; facility.fail_batch = false; REQUIRE(CommitLayerRecipeBatch(recipe, facility, facility, batch).error == LayerRecipeCommitError::None);
	CHECK(ComputeUtilityPowerDigest() == power); CHECK(GetUtilityPowerProducer(*producer)->consumed_lifetime_j == 55500); CHECK(facility.batches == 1); CHECK(facility.work.Capture().paid_j == 0);
}

/** A large genuine grant cannot compress physical work opportunities, and changed quotes cannot debit a replacement interval. */
TEST_CASE("Staged recipe opportunity caps and original grant identity survive excess power and stale quotes", "[layer-recipe-work]")
{
	ExcavationFixture world; UtilityPowerReset ledger; const auto &recipe = PaidSteelRecipe();
	const auto producer = RegisterUtilityPowerProducer(_current_company, MapLayer::Space, UtilityGenerationKind::External); REQUIRE(producer);
	const auto consumer = RegisterUtilityPowerConsumer(_current_company, MapLayer::Space, UtilityConsumerEffect::AssemblyWork, 55500, std::array{*producer}); REQUIRE(consumer);
	PaidRecipeFacility facility(recipe, *consumer); REQUIRE(PublishUtilityGeneration(*producer, 1, 3000000, 1) == UtilityPowerError::None);
	{ AutoRestoreBackup system_actor(_current_company, OWNER_DEITY); REQUIRE(AllocateUtilityPower(1, std::array{UtilityPowerRequest{*consumer, 55500}}) == UtilityPowerError::None); }
	const auto first = PrepareLayerRecipeWork(recipe, facility, facility, 55500); REQUIRE(first.energy_j == 750); REQUIRE(CommitLayerRecipeWork(recipe, facility, facility, first) == LayerRecipeCommitError::None);
	CHECK(facility.work.WorkTicks() == 1); CHECK(GetUtilityAllocation(*consumer)->granted_energy_j == 54750); CHECK(PrepareLayerRecipeWork(recipe, facility, facility, 55500).limiting == MultiLayerRecipeLimit::WorkTime);
	GrantRecipeWork(*producer, *consumer, 2); const auto next = PrepareLayerRecipeWork(recipe, facility, facility, 750); const auto receipt = facility.work.Capture();
	SECTION("changed actual work quantity") {
		auto changed = next; changed.energy_j = 751; const auto power = ComputeUtilityPowerDigest(); CHECK(CommitLayerRecipeWork(recipe, facility, facility, changed) == LayerRecipeCommitError::Stale); CHECK(ComputeUtilityPowerDigest() == power);
	}
	SECTION("old grant cannot borrow the next real interval") {
		GrantRecipeWork(*producer, *consumer, 3); const auto power = ComputeUtilityPowerDigest(); CHECK(CommitLayerRecipeWork(recipe, facility, facility, next) == LayerRecipeCommitError::Stale); CHECK(ComputeUtilityPowerDigest() == power);
	}
	CHECK(facility.work.Capture() == receipt); CHECK(GetUtilityPowerProducer(*producer)->consumed_lifetime_j == 750); CHECK(facility.stock.inputs[0].amount == 4);
}

/** Source-independent validation rejects malformed progress, while only actual owner/grant transactions may create runtime paid work. */
TEST_CASE("Shared recipe work keeps immutable arithmetic owner and layer bounds explicit", "[layer-recipe-work]")
{
	ExcavationFixture world; UtilityPowerReset ledger; const auto &recipe = PaidSteelRecipe();
	const auto producer = RegisterUtilityPowerProducer(_current_company, MapLayer::Space, UtilityGenerationKind::External); REQUIRE(producer);
	const auto consumer = RegisterUtilityPowerConsumer(_current_company, MapLayer::Space, UtilityConsumerEffect::AssemblyWork, 750, std::array{*producer}); REQUIRE(consumer);
	PaidRecipeFacility facility(recipe, *consumer); GrantRecipeWork(*producer, *consumer, 1);
	const auto original = PrepareLayerRecipeWork(recipe, facility, facility, 750); REQUIRE(CommitLayerRecipeWork(recipe, facility, facility, original) == LayerRecipeCommitError::None);
	GrantRecipeWork(*producer, *consumer, 2); auto record = facility.work.Capture(); REQUIRE(ValidateLayerRecipeElectricalWork(recipe, record));
	SECTION("overpaid pending work") { record.paid_j = 55501; }
	SECTION("changed immutable bill") { ++record.power_per_batch; }
	SECTION("future wrapped sequence") { record.sequence = UINT64_MAX; }
	SECTION("overpaid current opportunity") { record.interval_paid_j = 751; }
	SECTION("another immutable alternative") { record.recipe = MultiLayerRecipeID{"OGDS"}; }
	SECTION("nonintegral staged rate") { auto definition = recipe; ++definition.power_j_per_batch; record.power_per_batch = definition.power_j_per_batch; CHECK_FALSE(ValidateLayerRecipeElectricalWork(definition, record)); return; }
	SECTION("another physical layer cannot use this grant") {
		facility.stock.layer = MapLayer::Underground; const auto power = ComputeUtilityPowerDigest(); CHECK(PrepareLayerRecipeWork(recipe, facility, facility, 750).limiting == MultiLayerRecipeLimit::Power); CHECK(ComputeUtilityPowerDigest() == power); return;
	}
	SECTION("moved receipt leaves no duplicate payable work") {
		auto moved = std::move(facility.work); const auto power = ComputeUtilityPowerDigest(); CHECK(moved.WorkTicks() == 1); CHECK(facility.work.WorkTicks() == 0); CHECK(PrepareLayerRecipeWork(recipe, facility, facility, 750).error != LayerRecipeCommitError::None); CHECK(ComputeUtilityPowerDigest() == power); return;
	}
	SECTION("expired grant cannot pay more work") {
		{ AutoRestoreBackup system_actor(_current_company, OWNER_DEITY); REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None); }
		const auto power = ComputeUtilityPowerDigest(); CHECK(PrepareLayerRecipeWork(recipe, facility, facility, 750).limiting == MultiLayerRecipeLimit::Power); CHECK(facility.work.Capture().paid_j == 750); CHECK(ComputeUtilityPowerDigest() == power); return;
	}
	CHECK_FALSE(ValidateLayerRecipeElectricalWork(recipe, record)); CHECK(facility.work.Capture().paid_j == 750); CHECK(GetUtilityPowerProducer(*producer)->consumed_lifetime_j == 750);
}
