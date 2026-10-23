/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file native_cargo_custody_fixture.h Declared actual native owner components for read-only custody tests.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md physical owner boundaries.
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-h-cargo-custody.md exact inventory.
 * These are real pool/container objects with explicit test stock, not paid gameplay, operational fleets or saved-world acceptance.
 */
#ifndef TESTS_NATIVE_CARGO_CUSTODY_FIXTURE_H
#define TESTS_NATIVE_CARGO_CUSTODY_FIXTURE_H
#include "surface_supply_fixture.h"
#include "../cargo_custody.h"
#include "../train.h"
#include "../space_vehicle.h"
#include "../space_manifest.h"
#include "../space_pod.h"
#include "../space_structure.h"
#include "../orbital_station.h"
#include "../launch_center.h"
#include "../vehicle_func.h"
#include <map>

/** One independently declared expected physical reference, keyed by its actual allocated packet ID. */
struct NativeCargoCustodyExpectation {
	CargoLabel label; ///< Explicit expected physical meaning, not obtained from the observer being tested.
	StationID queue; ///< Real station only for goods; every job/vehicle/compartment is Invalid.
	uint16_t quantity; ///< Declared units in this unique native fragment.
};

/** Real native containers for every current owner family, without an alternate physical inventory. */
class NativeCargoCustodyRig : public SurfaceSupplyAdmissionRig {
	std::vector<StationCargoList *> cleanup_lists; ///< Non-owning addresses of fixture-populated lists, used only for safe component teardown.
public:
	Train *train = nullptr; ///< Actual ordinary vehicle freight owner.
	SpaceVehicle *craft = nullptr; ///< Actual Space hull, separately owning native freight and manifest supplies.
	SpacePod *pod = nullptr; ///< Actual passive shell, owning its sole descent job.
	SpaceStructure *blueprint = nullptr, *dock = nullptr, *factory = nullptr; ///< Separate real construction, completed service/storage and manufacture owners.
	std::map<CargoPacketID, NativeCargoCustodyExpectation> expected; ///< Independent test declarations, never installed stock or receipt authority.

	/** Construct actual native component owners in an isolated fixture world; callers initialize MockEnvironment/language first. */
	NativeCargoCustodyRig()
	{
		REQUIRE(Vehicle::GetNumItems() == 0); REQUIRE(SpacePod::GetNumItems() == 0); REQUIRE(SpaceStructure::GetNumItems() == 0);
		REQUIRE(Vehicle::CanAllocateItem(2)); REQUIRE(SpacePod::CanAllocateItem()); REQUIRE(SpaceStructure::CanAllocateItem(3));
		this->train = Train::Create(); this->train->owner = _current_company; this->train->cargo_type = GetCargoTypeByLabel(CargoLabel{"RFMT"});
		this->craft = SpaceVehicle::Create(); this->craft->owner = _current_company; this->craft->cargo_type = GetCargoTypeByLabel(CargoLabel{"GOOD"});
		this->craft->manifest = std::make_unique<SpaceManifest>();
		this->pod = SpacePod::Create(); this->pod->generation = 1; this->pod->state.owner = _current_company;
		this->pod->descent = std::make_unique<SpaceDescentJob>(); this->pod->descent->cargo = GetCargoTypeByLabel(CargoLabel{"STEL"});
		this->blueprint = SpaceStructure::Create(); this->blueprint->generation = 1; this->blueprint->state.kind = SpaceBuildKind::Core;
		this->dock = SpaceStructure::Create(); this->dock->generation = 2; this->dock->state.kind = SpaceBuildKind::Dock;
		this->dock->state.phase = SpaceStructurePhase::FrameComplete; this->dock->module = std::make_unique<OrbitalModule>();
		this->factory = SpaceStructure::Create(); this->factory->generation = 3; this->factory->state.kind = SpaceBuildKind::Assembly;
		this->factory->state.phase = SpaceStructurePhase::FrameComplete; this->factory->module = std::make_unique<OrbitalModule>();
		this->factory->module->manufacture = std::make_unique<OrbitalManufactureJob>(); this->factory->module->manufacture->model = 21;
		auto *source = Station::Get(this->station); source->launch_hub = std::make_unique<LaunchCollectionHub>();
		auto job = std::make_unique<SurfaceHandlingJob>(); job->cargo = GetCargoTypeByLabel(CargoLabel{"RFMT"});
		source->launch_hub->handling.push_back(std::move(job));
	}

	/** Retire only this component's declared packets through their actual containers before native owner pool teardown. */
	~NativeCargoCustodyRig()
	{
		// The pointers are cleanup witnesses, not another stock owner. Empty each
		// populated actual list before Space pool teardown drops its references.
		for (auto *list : this->cleanup_lists) list->Truncate();
		this->train->cargo.Truncate(); this->craft->cargo.Truncate();
		Station::Get(this->station)->launch_hub.reset();
		_vehicle_pool.CleanPool(); ResetVehicleHash();
		_space_pod_pool.CleanPool(); ResetSpacePods();
		_space_structure_pool.CleanPool(); ResetSpaceStructures();
	}

	/** Declare one original packet without granting production or purchase authority.
	 * @param label Explicit expected canonical material.
	 * @param queue Real goods queue or Invalid for all other owners.
	 * @param quantity Declared actual units.
	 * @return Newly allocated component packet, which the caller immediately transfers to one native list.
	 */
	CargoPacket *Packet(CargoLabel label, StationID queue, uint16_t quantity = 1)
	{
		REQUIRE(CargoPacket::CanAllocateItem());
		auto *packet = CargoPacket::Create(this->station, quantity, Source{Source::Invalid, SourceType::Industry});
		REQUIRE(this->expected.emplace(packet->index, NativeCargoCustodyExpectation{label, queue, quantity}).second);
		return packet;
	}
	/** Remember an actual destination before a tested transfer populates it.
	 * @param list Real native container; its original owner still owns every fragment.
	 * This is only a teardown witness, never production inventory or validation evidence.
	 */
	void Track(StationCargoList &list)
	{
		if (std::ranges::find(this->cleanup_lists, &list) == this->cleanup_lists.end()) this->cleanup_lists.push_back(&list);
	}
	/** Place explicitly restored original component stock in one real owner.
	 * @param list Actual destination whose teardown must precede pool cleanup.
	 * @param packet Original declared fragment, transferred to this sole owner.
	 */
	void Own(StationCargoList &list, CargoPacket *packet)
	{
		this->Track(list); list.Append(packet, StationID::Invalid());
	}

	/** Put declared units in one actual station-style owner and remember only its teardown address.
	 * @param list Actual queue, compartment or job container.
	 * @param label Independently specified expected meaning.
	 * @param queue Real goods-queue identity only; defaults to non-station ownership.
	 * @param quantity Explicit physical unit count.
	 * @return Actual packet identity before any compatible native merge; fixture calls use distinct empty slots.
	 */
	CargoPacketID Add(StationCargoList &list, CargoLabel label, StationID queue = StationID::Invalid(), uint16_t quantity = 1)
	{
		auto *packet = this->Packet(label, queue, quantity); const auto id = packet->index;
		this->Own(list, packet); return id;
	}

	/** Populate every real owner family with independent known labels, including all four heavy manufacture slots. */
	void Populate()
	{
		auto *source = Station::Get(this->station);
		this->Add(source->goods[GetCargoTypeByLabel(CargoLabel{"STEL"})].GetOrCreateData().cargo, CargoLabel{"STEL"}, this->station);
		this->train->cargo.Append(this->Packet(CargoLabel{"RFMT"}, StationID::Invalid()));
		this->craft->cargo.Append(this->Packet(CargoLabel{"GOOD"}, StationID::Invalid()));
		this->Add(source->launch_hub->handling[0]->packets, CargoLabel{"RFMT"});
		const std::array supplies{CargoLabel{"PROP"}, CargoLabel{"RFMT"}, CargoLabel{"ELEC"}};
		// Explicit compartment expectations are independent of the observer's
		// GetSpaceSupplyCargo lookup and include the non-refundable fuel role.
		for (size_t slot = 0; slot < supplies.size(); ++slot) this->Add(this->craft->manifest->supplies[slot], supplies[slot]);
		this->Add(this->pod->descent->packets, CargoLabel{"STEL"});
		this->Add(this->blueprint->job.inputs[0], CargoLabel{"STEL"}); this->Add(this->blueprint->job.inputs[1], CargoLabel{"GOOD"});
		this->Add(this->dock->module->stock[GetCargoTypeByLabel(CargoLabel{"RFMT"})], CargoLabel{"RFMT"});
		// Both real berths retain separate RFMT/ELEC material; visiting only the
		// first job or general module stock cannot satisfy these expectations.
		for (auto *job : this->dock->module->GetServiceJobs()) {
			this->Add(job->inputs[0], CargoLabel{"RFMT"}); this->Add(job->inputs[1], CargoLabel{"ELEC"});
		}
		const std::array assembly{CargoLabel{"STEL"}, CargoLabel{"GOOD"}, CargoLabel{"RBPT"}, CargoLabel{"ELEC"}};
		// Heavy component slots are the A-only extension absent from the donor
		// walk; declare all four independent labels rather than echoing its recipe lookup.
		for (size_t slot = 0; slot < assembly.size(); ++slot) this->Add(this->factory->module->manufacture->inputs[slot], assembly[slot]);
	}
};

#endif /* TESTS_NATIVE_CARGO_CUSTODY_FIXTURE_H */
