/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file surface_supply_legacy_fixture.h Declared original legacy components using the sole native adoption boundary.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md native stock and money ownership.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-h-cargo-custody.md custody before publication.
 * These inputs prove component admission, not a genuine paid predecessor or original B file decode.
 */
#ifndef TESTS_SURFACE_SUPPLY_LEGACY_FIXTURE_H
#define TESTS_SURFACE_SUPPLY_LEGACY_FIXTURE_H
#include "surface_supply_fixture.h"
#include "../surface_supply_order_runtime.h"
#include "../surface_supply_order_wire.h"

/** Scoped declared history; the caller owns its real station/world and destroys this scope first. */
class LegacySupplyComponent {
	StationID station; ///< Actual command-created source in the caller's component world.
public:
	/** @param station Actual existing native source; no fixture-only installer or company is created. */
	explicit LegacySupplyComponent(StationID station) : station(station)
	{
		REQUIRE(CaptureSurfaceSupplyOrders().records.empty()); ResetSurfaceSupplyOrderLoadStaging();
	}
	/** Retire component metadata before its native physical owners disappear. */
	~LegacySupplyComponent() { ResetSurfaceSupplyOrders(); }
	/** Convert an explicit original B receipt against this component's actual native reference facts.
	 * @param quantity Whole declared original purchase.
	 * @param dispatched Original irreversible first-departure count, never inferred from new movement.
	 * @param label Declared original paid material.
	 * @return Detached canonical book, without publishing history or creating physical stock.
	 */
	SurfaceSupplyOrderSnapshot Candidate(uint16_t quantity = 100, uint16_t dispatched = 0, CargoLabel label = CargoLabel{"RFMT"}) const
	{
		const auto *source = Station::Get(this->station);
		const std::array stations{SurfaceSupplyStationView{source->index, source->owner,
			static_cast<uint16_t>(TileX(source->xy)), static_cast<uint16_t>(TilePlaneY(source->xy)), GetTileLayer(source->xy), true}};
		SurfaceSupplyRecordContext context;
		context.profile = GetCurrentMultiLayerProfileSelection().id; context.width = Map::SizeX(); context.height = Map::SizeY();
		context.companies[_current_company.base()] = true; context.stations = stations;
		const LegacyBSupplyOrderWire row{1, _current_company.base(), this->station.base(), label, TimerGameEconomy::date.base(),
			TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1980}, 0, 1).base(), quantity, 0, dispatched, 25, 0};
		const auto converted = ConvertLegacyBSupply({1, 1, 2, {row}}, context); REQUIRE(converted.has_value()); return *converted;
	}
	/** Supply explicit independent component DATE/MAPS declarations; this is not original-file decoding. */
	void ObserveWorld() const
	{
		REQUIRE(ObserveLoadedSurfaceSupplyEconomyDate(TimerGameEconomy::date));
		REQUIRE(ObserveLoadedSurfaceSupplyMap(Map::SizeX(), Map::SizeY(), static_cast<uint8_t>(Map::LayerCount())));
	}
	/** Adopt only through the real full-load staging and complete physical custody join.
	 * @param candidate Whole detached original history; all actual packet owners must already exist.
	 */
	void Restore(const SurfaceSupplyOrderSnapshot &candidate) const
	{
		REQUIRE(StageLoadedSurfaceSupplyOrders(candidate, TimerGameEconomy::date)); this->ObserveWorld();
		REQUIRE(AcceptLoadedSurfaceSupplyOrders(true));
	}
	/** Declare one original non-paying fragment for immediate placement in its actual owner.
	 * @param quantity Explicit physical units.
	 * @param shipped Original saved departure flag; restoration does not increment history.
	 * @param order Exact original receipt identity; zero declares an unlinked native A control.
	 * @param source Original purpose/payer; B's original payer is unknown and must stay Invalid.
	 * @return Unowned component packet; caller must put it in one real container or explicitly destroy the negative-case orphan.
	 */
	CargoPacket *Packet(uint16_t quantity = 100, bool shipped = false, uint64_t order = 1,
		Source source = Source{Source::Invalid, SourceType::CompanySupply}) const
	{
		REQUIRE(CargoPacket::CanAllocateItem()); auto *packet = CargoPacket::Create(this->station, quantity, source);
		if (order != 0) REQUIRE(packet->RestoreCompanySupplyOrder(order, shipped));
		return packet;
	}
};

/** Capture original allocated scalar facts independently of the runtime adapter for unchanged-input checks.
 * @return Ordered native facts; allocation itself is never treated as physical ownership.
 */
inline std::vector<SurfaceSupplyPacketFacts> LegacyComponentPacketFacts()
{
	std::vector<SurfaceSupplyPacketFacts> result;
	// Compare every allocated fragment, including intentional orphans, so a
	// failed adoption cannot silently repair, relink or dispatch component stock.
	for (const auto *packet : CargoPacket::Iterate()) result.push_back({packet->index, packet->Count(), packet->GetSource(),
		packet->GetFeederShare(), packet->GetCompanySupplyOrder(), packet->HasCompanySupplyShipped()});
	return result;
}
#endif /* TESTS_SURFACE_SUPPLY_LEGACY_FIXTURE_H */
