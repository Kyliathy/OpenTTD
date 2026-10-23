/* This file is part of OpenTTD. Licensed under the GNU General Public License, version 2. */

/**
 * @file surface_supply_fixture.h Shared actual paid station/packet/recipient fixture; setup arrivals are explicitly not driven acceptance.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs6-mines-and-resource-chains.md Groups13CB-13CD.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md.
 */
#ifndef TESTS_SURFACE_SUPPLY_FIXTURE_H
#define TESTS_SURFACE_SUPPLY_FIXTURE_H
#include "processor_fixture.h"
#include "../surface_supply.h"
#include "../train.h"
#include "../economy_base.h"
#include "../timer/timer_game_economy.h"
#include "../table/strings.h"

/** Real command-funded electronics recipient and native source stations; boundary arrivals are explicit fixture inputs. */
class SurfaceSupplyRig : public ProcessorRig {
	AutoRestoreBackup<uint64_t> inflation{_economy.inflation_prices, uint64_t{1} << 16}; ///< Ordinary uninflated baseline; tariff tests change the actual economy multiplier.
	AutoRestoreBackup<bool> infinite{_settings_game.difficulty.infinite_money, false}; ///< Admission checks real available cash.
	AutoRestoreBackup<TimerGameEconomy::Date> day{TimerGameEconomy::date}; ///< Restore company-cap day probes.
public:
	Station *origin = nullptr; ///< Actual Surface pickup; its native queue owns all purchased stock.
	Station *second = nullptr; ///< Another actual Surface pickup sharing the same company's cap.
	Station *destination = nullptr; ///< Actual electronics catchment receiving Refined Metals.
	Train *carrier = nullptr; ///< Pool-owned carrier for packet/payment boundary checks; driven command-built delivery is13CD.

	/** @param serialized_native_slots Use distributed native cargo slots for actual ordinary-save fixtures; boundary tests retain isolated slots. */
	explicit SurfaceSupplyRig(bool serialized_native_slots = false) : ProcessorRig(MultiLayerRecipeID{"ELEC"}, 30, 30, serialized_native_slots)
	{
		ResetSurfaceSupplyOrders();
		this->origin = this->StationAt(TileXYL(20, 20, MapLayer::Surface));
		this->second = this->StationAt(TileXYL(22, 20, MapLayer::Surface));
		this->destination = this->StationAt(TileXYL(29, 30, MapLayer::Surface));
		(*Town::Iterate().begin())->townnametype = SPECSTR_TOWNNAME_START;
		// Ordinary station removal rebuilds signs just as afterload does. Use
		// actual default station/town names rather than fixture-only invalid string IDs.
		for (Station *station : Station::Iterate()) station->string_id = STR_SV_STNAME;
	}

	/** Retire native carrier/payment before stations and clear isolated purchase history before the company disappears. */
	~SurfaceSupplyRig()
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu}; // Suppress only standalone money text; actual payment still settles normally.
		_vehicle_pool.CleanPool();
		ResetSurfaceSupplyOrders();
	}

	/**
	 * Read the actual source queue for a canonical meaning.
	 * @param label Existing cargo label, resolved through the real active registry.
	 * @return Native physical queue, not a purchase-local inventory.
	 */
	StationCargoList &Queue(CargoLabel label)
	{
		return this->origin->goods[GetCargoTypeByLabel(label)].GetOrCreateData().cargo;
	}

	/**
	 * Load purchased stock through native packet movement into a real carrier and stage final industry delivery.
	 * @param label Actual purchased cargo meaning.
	 * @param quantity Whole units already present in the native source queue.
	 */
	void Load(CargoLabel label, uint16_t quantity)
	{
		REQUIRE(Train::CanAllocateItem());
		this->carrier = Train::Create();
		this->carrier->owner = _current_company;
		this->carrier->cargo_type = GetCargoTypeByLabel(label);
		this->carrier->last_station_visited = this->destination->index;
		REQUIRE(CargoPayment::CanAllocateItem());
		this->carrier->cargo_payment = CargoPayment::Create(this->carrier);
		const std::array<StationID, 0> next{};
		REQUIRE(this->Queue(label).Load(quantity, &this->carrier->cargo, next, this->origin->xy) == quantity);
	}

	/** Settle actual native bank/vehicle profit now, suppressing only fixture graphical money effects. */
	void SettlePayment()
	{
		AutoRestoreBackup mode{_game_mode, GameMode::Menu};
		delete this->carrier->cargo_payment;
		this->carrier->cargo_payment = nullptr;
	}
};

#endif /* TESTS_SURFACE_SUPPLY_FIXTURE_H */
