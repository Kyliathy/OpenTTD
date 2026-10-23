/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file utility_power_local_feed_fixture.h Real paid native plant, tube depot and cable participants for local-feed admission and native-file checks.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs12-solar-farms-mirrors-and-eti.md Group9C.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_UTILITY_POWER_LOCAL_FEED_FIXTURE_H
#define TESTS_UTILITY_POWER_LOCAL_FEED_FIXTURE_H
#include "geothermal_generator_operating_fixture.h"
#include "../utility_power_transfer.h"
#include "../utility_power_lineage.h"
#include "../rail_cmd.h"
#include "../rail_map.h"
#include "../landscape_cmd.h"
#include "../safeguards.h"

/** Actual paid dated plant and two native depot inlets sharing the same purchased cable trunk. */
class NativeLocalFeedRig : public GeothermalGeneratorOperatingRig {
public:
	UtilityProducerID output{UINT32_MAX}; ///< Original exact native conversion source, never a synthetic publication.
	UtilityConsumerID supply{UINT32_MAX}; ///< Sole forwarding reservation linked to that actual output.
	std::array<UtilityConsumerID, 2> recipients; ///< Existing unlinked native TubeWork requests, with no private quantity.
	std::array<UtilityLinkID, 2> feeds; ///< Nonreused local physical attachments, separate from generation.
	std::array<TileIndex, 2> inlets; ///< Actual command-built native tube depots on explicitly prepared floor.

	/** @param model Actual dated native machinery. @param serialized Match shipped native slots for genuine save transport. Build and fund participants before allocating one real synchronized generation interval. */
	explicit NativeLocalFeedRig(GeothermalGeneratorModel model = GeothermalGeneratorModel::MagmaLoop, bool serialized = false) : GeothermalGeneratorOperatingRig(serialized, true, 1000000, model)
	{
		const auto rail = InitialiseTubeRailType(this->ActivatedContent()); REQUIRE(rail != INVALID_RAILTYPE);
		Company::Get(_current_company)->avail_railtypes.Set(rail);
		this->inlets = {TileXYL(32, 24, MapLayer::DeepUnderground), TileXYL(32, 26, MapLayer::DeepUnderground)};
		const std::array cells{this->plant_port, TileXYL(35, 24, MapLayer::DeepUnderground), TileXYL(34, 24, MapLayer::DeepUnderground),
			TileXYL(33, 24, MapLayer::DeepUnderground), this->inlets[0], TileXYL(33, 25, MapLayer::DeepUnderground),
			TileXYL(32, 25, MapLayer::DeepUnderground), this->inlets[1]};
		this->Open(cells);
		// Both real depot purchases precede cable installation, preserving native
		// transport metadata and its independent construction invoices.
		for (const auto inlet : this->inlets) {
			const auto delta = TileIndexDiffCByDiagDir(DiagDirection::SE);
			this->Open(std::array{TileAddWrap(inlet, delta.x, delta.y)});
			const auto built = CmdBuildTrainDepot(DoCommandFlag::Execute, inlet, rail, DiagDirection::SE);
			INFO("native tube depot tile=" << inlet.base() << " refusal=" << built.GetErrorMessage().base());
			REQUIRE(built.Succeeded());
			REQUIRE(GetTubeTerminal(inlet, _current_company).has_value());
		}
		for (const auto cell : cells) {
			if (!GetGeothermalUtilityLink(cell, UtilityEnergyDomain::Electricity).has_value()) this->Build(cell, UtilityEnergyDomain::Electricity); // The original plant output cell was already paid by the native operating fixture.
		}
		this->Operate(); this->output = GetGeothermalGeneratorState(this->generator)->electrical_producer;
		const auto source = RegisterUtilityPowerConsumer(_current_company, MapLayer::DeepUnderground, UtilityConsumerEffect::ProcessorBoost, 10000, std::array{this->output});
		REQUIRE(source.has_value()); this->supply = *source;
		// Each native inlet receives a distinct existing request. Only the shared
		// allocator and complete cable transfer can grant either any electricity.
		for (size_t index = 0; index < this->inlets.size(); ++index) {
			const auto recipient = RegisterUtilityPowerConsumer(_current_company, MapLayer::DeepUnderground, UtilityConsumerEffect::TubeWork, 10000, {});
			REQUIRE(recipient.has_value()); this->recipients[index] = *recipient;
			const auto before = ComputeUtilityPowerDigest();
			const auto quote = RegisterGeothermalLocalFeed(this->generator, this->supply, *recipient, this->inlets[index], false);
			REQUIRE(quote.has_value()); CHECK(ComputeUtilityPowerDigest() == before);
			const auto feed = RegisterGeothermalLocalFeed(this->generator, this->supply, *recipient, this->inlets[index]);
			REQUIRE(feed == quote); this->feeds[index] = *feed;
		}
		++TimerGameTick::counter;
		AutoRestoreBackup system(_current_company, OWNER_DEITY);
		REQUIRE(BeginUtilityPowerInterval() == UtilityPowerError::None);
		PublishGeothermalHeatSources(GetUtilityPowerInterval(), 1); // Match the real common driver's source-before-conversion order; no fixture electricity is published directly.
		RunGeothermalGenerators(GetUtilityPowerInterval(), 1);
		const uint64_t generated = model == GeothermalGeneratorModel::MagmaLoop ? 9450 : 4050;
		REQUIRE(AllocateUtilityPower(GetUtilityPowerInterval(), std::array{UtilityPowerRequest{this->supply, generated},
			UtilityPowerRequest{this->recipients[0], 9450}, UtilityPowerRequest{this->recipients[1], 9450}}) == UtilityPowerError::None);
		REQUIRE(GetUtilityPowerProducer(this->output)->meter.generation_energy_j == generated);
		REQUIRE(GetUtilityPowerConsumer(this->supply)->meter.granted_energy_j == generated);
	}

	/** Remove this fixture's actual depots while their native company and shared utility owners still exist. */
	~NativeLocalFeedRig()
	{
		// Native demolition issues normal topology notifications and retires only
		// the matching inlet; the inherited fixture subsequently removes plants.
		for (const auto inlet : this->inlets) {
			if (!IsRailDepotTile(inlet)) continue;
			CHECK(Command<Commands::LandscapeClear>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false, inlet, std::make_tuple(inlet)).Succeeded());
		}
	}

	/** @param inlet Actual recipient index. @param quantity Exact joule request or offer ceiling. @return Complete installed original-source path without a copied balance. */
	UtilityPathTransferRequest Request(size_t inlet, uint64_t quantity) const
	{
		const auto path = FindGeothermalUtilityPath(_current_company, UtilityEnergyDomain::Electricity, this->plant_port, this->inlets[inlet]);
		REQUIRE(path.has_value());
		UtilityPathTransferRequest request{{this->supply, GetUtilityPowerInterval()}, this->recipients[inlet], this->plant_port, quantity};
		request.legs.push_back({this->feeds[inlet], path->cells}); return request;
	}

	/** @param inlet Actual recipient index. @param quantity Positive exact whole energy. @return Once-only complete shared mutation receipt. */
	UtilityMutationReceipt Transfer(size_t inlet, uint64_t quantity) const
	{
		const auto proposal = PrepareUtilityPathTransfer(this->Request(inlet, quantity)); REQUIRE(proposal.error == UtilityPowerError::None);
		const auto receipt = CommitUtilityPathTransfer(proposal); REQUIRE(receipt.error == UtilityPowerError::None); return receipt;
	}
};

#endif /* TESTS_UTILITY_POWER_LOCAL_FEED_FIXTURE_H */
