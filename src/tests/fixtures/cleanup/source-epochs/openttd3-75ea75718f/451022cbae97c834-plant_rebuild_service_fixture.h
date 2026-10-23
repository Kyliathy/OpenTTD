/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file plant_rebuild_service_fixture.h Actual reconstructed generator routing and native service observations.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs18-geothermal-energy-and-industry.md Groups1CCD/1CD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 * These helpers borrow ordinary-loaded machinery; no floor, material, source, time or account is supplied.
 */
#ifndef TESTS_PLANT_REBUILD_SERVICE_FIXTURE_H
#define TESTS_PLANT_REBUILD_SERVICE_FIXTURE_H
#include "plant_rebuild_delivery_fixture.h"
#include "../geothermal_electrical.h"
#include "../geothermal_service_cmd.h"
#include "../geothermal_utility_cmd.h"
#include "../town.h"

/** @param world Actual reconstructed machinery and original retained town route. @param machine Actual VentTap1 or Magma2 role. @return Explicit actual paid risers/terminal/town for the synchronized recovery command, with no ledger identities provided by the caller. */
inline GeothermalTownServiceRequest PlantRebuildTownRequest(const PlantRebuildCargoWorld &world, uint machine)
{
	GeothermalTownServiceRequest request; request.generator = world.machines[machine];
	// The retained native town association is unique. Its original terminal
	// and paid sending machinery survive source demolition and reconstruction.
	for (const auto &consumer : CaptureUtilityPowerState().consumers) {
		if (consumer.town == TownID::Invalid()) continue;
		REQUIRE(request.town == TownID::Invalid()); request.town = consumer.town; request.terminal = consumer.town_terminal;
	}
	for (const auto &riser : CaptureGeothermalRisers()) {
		if (GetTileLayer(riser.site.portal) == MapLayer::DeepUnderground) request.first_riser = riser.industry;
		if (GetTileLayer(riser.site.portal) == MapLayer::Underground) request.second_riser = riser.industry;
	}
	REQUIRE(request.town != TownID::Invalid()); REQUIRE(Station::IsValidID(request.terminal));
	REQUIRE(Industry::IsValidID(request.first_riser)); REQUIRE(Industry::IsValidID(request.second_riser)); return request;
}

/** @param world Actual reconstructed machinery. @param machine Native model role1/2. @return Its original town consumer after a pure quote and native atomic recovery, preserving old spent history and acquiring fresh source rights only. */
inline UtilityConsumerID ConnectPlantRebuildTown(const PlantRebuildCargoWorld &world, uint machine)
{
	const auto request = PlantRebuildTownRequest(world, machine); const auto before = CaptureUtilityPowerState();
	const auto digest = ComputeUtilityPowerDigest(), generators = ComputeGeothermalGeneratorDigest(); const auto random = _random;
	const auto cash = Company::Get(_current_company)->money;
	const auto proposed = ConnectGeothermalTownService(request, false); INFO("Rebuilt town connection error=" << to_underlying(proposed.error)); REQUIRE(proposed.error == UtilityPowerError::None);
	REQUIRE(Command<Commands::ConnectGeothermalTownService>::Do({}, request.generator, request.first_riser, request.second_riser, request.terminal, request.town).Succeeded());
	CHECK(ComputeUtilityPowerDigest() == digest); CHECK(ComputeGeothermalGeneratorDigest() == generators); CHECK(Company::Get(_current_company)->money == cash); CHECK(std::ranges::equal(random.state, _random.state));
	REQUIRE(Command<Commands::ConnectGeothermalTownService>::Unsafe<CommandCallback>(INVALID_STRING_ID, nullptr, false, false,
		GetGeothermalGeneratorPort(request.generator), std::make_tuple(request.generator, request.first_riser, request.second_riser, request.terminal, request.town)).Succeeded());
	const auto after = CaptureUtilityPowerState(); CHECK(after.next_consumer == before.next_consumer + 1); CHECK(after.next_link == before.next_link + 1);
	CHECK(after.next_producer == before.next_producer); CHECK(after.next_share == before.next_share); CHECK(after.operation_sequence == before.operation_sequence + 1);
	CHECK(proposed.source.base() >= before.next_consumer); CHECK(GetUtilityPowerConsumer(proposed.destination)->town_supply == proposed.source);
	CHECK(Company::Get(_current_company)->money == cash); CHECK(ComputeGeothermalGeneratorDigest() == generators); CHECK(std::ranges::equal(random.state, _random.state));
	const auto connected = ComputeUtilityPowerDigest(); CHECK(Command<Commands::ConnectGeothermalTownService>::Do({}, request.generator, request.first_riser, request.second_riser, request.terminal, request.town).Failed()); CHECK(ComputeUtilityPowerDigest() == connected);
	fmt::print("PLANT_SERVICE_CONNECTION model={} source={} relay={} town={} first={} second={}\n", machine, proposed.source.base(), proposed.relay.base(), proposed.destination.base(), proposed.links[0].base(), proposed.links[1].base()); return proposed.destination;
}

/** @param tile Actual declared native port on already paid floor. @param domain Exact separate physical domain. @param remove Explicit isolation of one installed connection. @return Actual native paid capital/removal invoice, with pure estimate and exact once-only company settlement. */
inline Money ChangePlantRebuildPort(TileIndex tile, UtilityEnergyDomain domain, bool remove = false)
{
	const auto cash = Company::Get(_current_company)->money; const auto random = _random; const auto digest = ComputeUtilityPowerDigest();
	const auto prepared = PrepareGeothermalUtilityLink(tile, domain, remove); REQUIRE(prepared.error == GeothermalError::None);
	const auto quote = remove ? Command<Commands::RemoveGeothermalUtilityLink>::Do({}, tile, domain) : Command<Commands::BuildGeothermalUtilityLink>::Do({}, tile, domain);
	REQUIRE(quote.Succeeded()); CHECK(ComputeUtilityPowerDigest() == digest); CHECK(Company::Get(_current_company)->money == cash); CHECK(std::ranges::equal(random.state, _random.state));
	const auto result = remove ? Command<Commands::RemoveGeothermalUtilityLink>::Do(DoCommandFlag::Execute, tile, domain) : Command<Commands::BuildGeothermalUtilityLink>::Do(DoCommandFlag::Execute, tile, domain);
	REQUIRE(result.Succeeded()); CHECK(result.GetCost() == quote.GetCost()); CHECK(Company::Get(_current_company)->money == cash - result.GetCost());
	if (!remove) { const auto installed = GetGeothermalUtilityLink(tile, domain); REQUIRE(installed.has_value()); CHECK(installed->id == prepared.id); }
	fmt::print("PLANT_SERVICE_PORT tile={} domain={} remove={} id={} cost={}\n", tile.base(), to_underlying(domain), remove, prepared.id.base(), result.GetCost().base()); return result.GetCost();
}

/** @param world Actual reconstructed machinery/freight. @param invoices Actual cumulative paid port preparation/isolation, not construction purchases. @return Complete relevant native source/conversion/service/town/edge observation extending all material/account/carrier owners. */
inline std::string CapturePlantRebuildService(const PlantRebuildCargoWorld &world, Money invoices)
{
	std::string result = CapturePlantRebuildDelivery(world, invoices);
	/** @param name Unique native observed scalar. @param value Original field value; signed native fields retain their bits. Append metadata, never gameplay state. */
	const auto add = [&](std::string_view name, uint64_t value) { result += fmt::format("{}={},", name, value); };
	const auto state = CaptureUtilityPowerState(); add("power_next_link", state.next_link);
	// All original and fresh conversion pairs keep immutable efficiency and
	// original thermal attribution; generated output is never another Heat store.
	for (const auto &producer : state.producers) {
		const auto prefix = fmt::format("producer{}_", producer.meter.producer_id.base());
		add(prefix + "input", producer.conversion_input.base()); add(prefix + "efficiency", producer.conversion_efficiency); add(prefix + "conversion_remainder", producer.conversion_remainder);
		for (const auto &origin : producer.conversion_origins) add(prefix + fmt::format("origin{}", origin.producer.base()), origin.heat_j);
	}
	// Keep the unique original town and every old/new forwarding identity.
	// A later source switch cannot erase the preceding useful native service.
	for (const auto &consumer : state.consumers) {
		const auto prefix = fmt::format("consumer{}_", consumer.meter.consumer_id.base()); add(prefix + "active", consumer.active); add(prefix + "effect", to_underlying(consumer.effect));
		add(prefix + "town", consumer.town.base()); add(prefix + "supply", consumer.town_supply.base()); add(prefix + "terminal", consumer.town_terminal.base());
		if (consumer.town == TownID::Invalid()) continue;
		add("town_consumer", consumer.meter.consumer_id.base()); add("town_supply", consumer.town_supply.base()); add("town_id", consumer.town.base()); add("town_terminal", consumer.town_terminal.base());
		add("town_applied", consumer.town_effect_applied); add("town_bonus", GetUtilityTownPowerBonus(consumer.town)); add("town_population", Town::Get(consumer.town)->cache.population);
	}
	// Original retired cells/edges remain tombstones; new installations and
	// source edges have monotonic native IDs and retain current spent flow.
	for (const auto &link : state.links) {
		const auto prefix = fmt::format("link{}_", link.id.base()); add(prefix + "active", link.active); add(prefix + "kind", to_underlying(link.kind)); add(prefix + "flow", link.flow_j);
		add(prefix + "source", link.source.base()); add(prefix + "destination", link.destination.base()); add(prefix + "tile", link.source_endpoint.base());
	}
	return result;
}
#endif /* TESTS_PLANT_REBUILD_SERVICE_FIXTURE_H */
