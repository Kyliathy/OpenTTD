/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file processor_freight_fixture.h Explicit isolated cargo dimensions for native transport and replacement contracts.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12
 * The inherited fixture supplies declared frames/registry slots; real native loaders separately prove actual configured dimensions.
 */
#ifndef TESTS_PROCESSOR_FREIGHT_FIXTURE_H
#define TESTS_PROCESSOR_FREIGHT_FIXTURE_H
#include "manufacturing_fixture.h"

/** Isolated native factory participants with explicitly declared physical cargo weights, restored by the parent registry owner. */
class ProcessorFreightRig : public ManufacturingMaterializationRig {
public:
	/** Install positive unit dimensions before producing a hull; the parent's isolated CargoSpec snapshot restores every value during teardown. */
	ProcessorFreightRig()
	{
		// Label-only manufacture fixtures need no mass until freight participates. These declared dimensions make physical holds and retained service assets meaningful without claiming disk content or gameplay procurement.
		for (const auto label : {CargoLabel{"STEL"}, CargoLabel{"GOOD"}, CargoLabel{"NORE"}, CargoLabel{"RARE"}, CargoLabel{"WICE"}, CargoLabel{"RFMT"}, CargoLabel{"PROP"}, CargoLabel{"ELEC"}, CargoLabel{"RBPT"}, CargoLabel{"PASS"}}) {
			const auto cargo = GetCargoTypeByLabel(label); REQUIRE(IsValidCargoType(cargo)); CargoSpec::Get(cargo)->weight = label == CargoLabel{"GOOD"} ? 8 : 16;
		}
	}
};
#endif
