/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file manufacturing_fixture.h Isolated original native factory, packet and common-grant contracts; no gameplay procurement/construction claim.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Groups7/8/8B
 */
#ifndef TESTS_MANUFACTURING_FIXTURE_H
#define TESTS_MANUFACTURING_FIXTURE_H
#include "../stdafx.h"
#include "mock_environment.h"
#include "../3rdparty/catch2/catch.hpp"
#include "../orbital_manufacturing.h"
#include "../space_structure.h"
#include "../space_structure_cmd.h"
#include "../command_func.h"
#include "../space_approach.h"
#include "../space_vehicle.h"
#include "../vehicle_func.h"
#include "../hub_cargo.h"
#include "../company_base.h"
#include "../core/backup_type.hpp"
#include "../core/random_func.hpp"
#include "../transport_tube.h"
#include "../timer/timer_game_tick.h"
#include "excavation_fixture.h"
#include "../safeguards.h"

/** Native company/profile, explicit isolated consumed frames and actual packet objects; never a gameplay predecessor or payment proof. */
class ManufacturingReservationRig {
	ExcavationFixture world;
	AutoRestoreBackup<TimerGameCalendar::Year> year{TimerGameCalendar::year, TimerGameCalendar::Year{1995}};
	AutoRestoreBackup<uint64_t> tick{TimerGameTick::counter, uint64_t{0}};
	/** @param kind Exact isolated frame recipe. @param id Fixture pool identity. @param x Layer-local left edge. @param y Layer-local top edge.
	 * @return Detached fully consumed frame for command tests, with no useful facility or physical cargo.
	 */
	static SpaceStructureRecord Frame(SpaceBuildKind kind, uint16_t id, uint16_t x, uint16_t y)
	{
		const auto &recipe = *GetSpaceConstructionDefinition(kind); SpaceStructureRecord row;
		row.ref = {SpaceStructureID{id}, uint64_t{id} + 1}; row.state.owner = _current_company; row.state.kind = kind;
		row.state.x = x; row.state.y = y; row.state.capital_paid = recipe.capital; row.state.phase = SpaceStructurePhase::FrameComplete;
		row.work_ticks = recipe.work_ticks;
		row.consumed = {static_cast<uint16_t>(recipe.inputs[0].quantity), static_cast<uint16_t>(recipe.inputs[1].quantity)};
		return row;
	}
public:
	/** @return Exact isolated package descriptors for native Engine boundary fixtures; this is not disk content or gameplay procurement evidence. */
	const GRFConfigList &ActivatedContent() const { return this->world.ActivatedContent(); }
	SpaceStructure *core = nullptr, *assembly = nullptr, *storage = nullptr; ///< Sole isolated native owners, all physically adjacent through their paid Core.
	/** Install/activate actual role owners and register the original explicit Assembly connection; create no hull, material or energy. */
	ManufacturingReservationRig()
	{
		ResetTubePowerVehicles(); ResetUtilityPowerState(); ResetSpaceStructures();
		TimerGameCalendar::date = TimerGameCalendar::ConvertYMDToDate(TimerGameCalendar::Year{1995}, 0, 1);
		const std::array rows{Frame(SpaceBuildKind::Core, 0, 4, 4), Frame(SpaceBuildKind::Assembly, 1, 7, 6), Frame(SpaceBuildKind::Storage, 2, 5, 7)};
		REQUIRE(InstallLoadedSpaceStructures(rows, 4));
		core = SpaceStructure::Get(SpaceStructureID{0}); assembly = SpaceStructure::Get(SpaceStructureID{1}); storage = SpaceStructure::Get(SpaceStructureID{2});
		// Each isolated frame follows its real public activation countdown; this setup supplies role contracts without claiming material-paid gameplay construction.
		for (auto *site : {core, assembly, storage}) {
			REQUIRE(CmdActivateOrbitalModule(DoCommandFlag::Execute, site->index, site->generation).Succeeded());
			for (uint done = 0; done < 74; ++done) TickOrbitalConstructionInventory();
			REQUIRE(IsOrbitalModuleOperational(*site));
		}
		REQUIRE(CmdConnectOrbitalServices(DoCommandFlag::Execute, assembly->index, assembly->generation, core->index, core->generation).Succeeded());
	}
	/** Dispose only this isolated contract fixture's physical packets, then retire its owners before restoring caller profile/company/clocks. */
	~ManufacturingReservationRig()
	{
		// This isolated world owns its whole pool; native teardown clears generated hulls/manifests before packet-containing factories and the company disappear.
		_vehicle_pool.CleanPool();
		// Pool cleanup deliberately skips per-hull hash unlinking; discard its viewport/tile pointers before another isolated world can write through a retired hull.
		ResetVehicleHash();
		// Every explicit unit packet is removed through its sole container; clearing a pool must not leave stale physical references for the next focused case.
		for (auto *site : SpaceStructure::Iterate()) if (site->module) {
			for (auto &stock : site->module->stock) if (stock.TotalCount() != 0) HubCargoTransfer::Consume(stock, stock.TotalCount());
			if (site->module->manufacture) for (auto &stock : site->module->manufacture->inputs) if (stock.TotalCount() != 0) HubCargoTransfer::Consume(stock, stock.TotalCount());
		}
		_space_structure_pool.CleanPool(); ResetSpaceStructures(); ResetTubePowerVehicles(); ResetUtilityPowerState();
	}
	/** @param site Isolated stock owner. @param label Declared physical cargo label. @param quantity Explicit unit quantity; no runtime production/save claim. */
	void Add(SpaceStructure &site, CargoLabel label, uint16_t quantity)
	{
		REQUIRE(CargoPacket::CanAllocateItem());
		const auto cargo = GetCargoTypeByLabel(label); REQUIRE(IsValidCargoType(cargo));
		site.module->stock[cargo].Append(CargoPacket::Create(quantity, uint16_t{7}, StationID::Invalid(), TileXYL(2, 2, MapLayer::Surface), Money{quantity * 10}), StationID::Invalid());
	}
	/** @param model Canonical recipe whose entire input set is represented. @param extra Unreserved units retained in each source label after reservation. */
	void Supply(uint16_t model, uint16_t extra = 0)
	{
		// Every declared common/heavy input is represented by an actual packet; omitting a component must be observable at the same admission boundary.
		for (const auto &input : GetSpaceVehicleConstruction(model)->inputs) this->Add(*core, input.label, static_cast<uint16_t>(input.quantity + extra));
	}
	/** @param model Immutable requested recipe. @param flags Estimate or isolated direct execution; native command payment is verified separately. @return Actual public request admission. */
	CommandCost Queue(uint16_t model, DoCommandFlags flags = {})
	{
		return CmdQueueOrbitalManufacture(flags, assembly->index, assembly->generation, core->index, core->generation, model);
	}
	/** Gather/publish/allocate through the real common driver, keeping reservation separate for grant-loss boundary checks. */
	void Allocate()
	{
		AutoRestoreBackup pause(_pause_mode, PauseModes{}); ++TimerGameTick::counter;
		PrepareTubePowerTick();
	}
	/** Offer one normal allocation/reservation opportunity through actual public owners, without a private account or synthetic grant. */
	void Opportunity()
	{
		AutoRestoreBackup pause(_pause_mode, PauseModes{}); this->Allocate(); TickOrbitalManufacturingReservations();
	}
};

/** Isolated native factory/common-grant participant using exact declared unit content, without a disk/procurement acceptance claim. */
class ManufacturingMaterializationRig : public ManufacturingReservationRig {
	GRFConfigList previous; ///< Caller package descriptors, restored after the isolated hulls disappear.
public:
	/** Expose only the parent fixture's exact selected package descriptors to its real native Engine owner; grant no hull, work or recipe. */
	ManufacturingMaterializationRig()
	{
		// Real native hull placement updates sprite bounds even in a headless unit world; mock dimensions support that owner path without claiming delivered artwork.
		MockEnvironment::Instance();
		this->previous.swap(_grfconfig);
		// Catalogue identity is declared unit input, matching this isolated world's public profile boundary rather than reading user-wide installed content.
		for (const auto &descriptor : this->ActivatedContent()) {
			auto copy = std::make_unique<GRFConfig>(); copy->ident = descriptor->ident; copy->version = descriptor->version; copy->status = descriptor->status;
			_grfconfig.push_back(std::move(copy));
		}
	}
	/** Remove this isolated world's hulls before restoring caller descriptors; the parent then disposes its remaining sole packet owners. */
	~ManufacturingMaterializationRig()
	{
		_vehicle_pool.CleanPool();
		// Match native world initialization after bulk deletion: no subsequent hull may link its viewport through a freed product from the previous fixture.
		ResetVehicleHash();
		_grfconfig.clear(); this->previous.swap(_grfconfig);
	}
	/** Offer one actual normal common allocation, reservation and paid-work opportunity; no grant, progress or completion is synthesized. */
	void Tick()
	{
		AutoRestoreBackup pause(_pause_mode, PauseModes{}); this->Allocate(); TickOrbitalManufacturingReservations(); TickOrbitalManufacturingWork();
	}
	/** @param model Canonical recipe to queue and reserve. @return Its actual original contained job, with zero paid work. */
	OrbitalManufactureJob &Start(uint16_t model)
	{
		this->Supply(model); REQUIRE(this->Queue(model, DoCommandFlag::Execute).Succeeded()); this->Tick();
		REQUIRE(this->assembly->module->manufacture->phase == OrbitalManufacturePhase::Reserved);
		REQUIRE(this->assembly->module->manufacture->work_ticks == 0); return *this->assembly->module->manufacture;
	}
	/** @param ticks Exact count of real public opportunities; every step still goes through the original common power allocation/consume participant. */
	void Work(uint ticks) { for (uint tick = 0; tick < ticks; ++tick) this->Tick(); }
};

#endif
