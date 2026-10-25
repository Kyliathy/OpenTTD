/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_merged_refusal_fixture.h Rebuild malformed441copies from an actual new native successor.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs15-savegame-determinism-and-lifecycle.md
 * Inputs/outputs are invocation-owned test-cache files; retained originals are never edited.
 */
#ifndef TESTS_SAVE_MERGED_REFUSAL_FIXTURE_H
#define TESTS_SAVE_MERGED_REFUSAL_FIXTURE_H
#include "save_supply_wire_fixture.h"
#include "../saveload/saveload_func.h"
#include <fstream>

/** Refuse deliberate marker/version/capability/tag/custody contradictions in both real readers.
 * @param source Actual freshly saved merged successor under test-cache, independently admitted before this call.
 * @param unchanged Assert the caller's original canonical receipts, real packets and wallets after copied refusal or successful native recovery; it creates no state.
 * Every negative is rebuilt beside source with `.bad-N.sav`, and a valid full
 * reload follows each refused native attempt. No rollback of failed full loading is claimed.
 */
inline void CheckMergedNativeRefusals(const std::filesystem::path &source, const std::function<void()> &unchanged)
{
	std::ifstream input(source, std::ios::binary); REQUIRE(input.good());
	const std::vector<uint8_t> raw{std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
	const auto original = EnvelopeReplay(std::make_shared<EnvelopeTestReader>(raw));
	const auto chunks = SupplyWireChunks(original);
	// Each malformed copy changes one actual independently framed fact. The
	// native writer cannot replace missing evidence or repair expected bytes.
	for (unsigned fault = 0; fault < 20; ++fault) {
		INFO(fault); auto bytes = original; uint16_t version = MERGED_SAVE_LINEAGE_VERSION;
		const auto &marker = SupplyWireFind(chunks, "MLLN"), &supply = SupplyWireFind(chunks, "SSOR");
		switch (fault) {
			case 0: bytes.erase(bytes.begin() + marker.begin, bytes.begin() + marker.end); break;
			case 1: {
				const std::vector<uint8_t> duplicate(bytes.begin() + marker.begin, bytes.begin() + marker.end);
				bytes.insert(bytes.end() - 4, duplicate.begin(), duplicate.end()); break;
			}
			case 2: bytes[marker.payload] ^= 1; break;
			case 3: SupplyWireSet(bytes, marker.payload + 4, 2, 2); break;
			case 4: SupplyWireSet(bytes, marker.payload + 6, 440, 2); break;
			case 5: version = 440; break;
			case 6: version = 442; break;
			case 7: case 8: case 9: case 10: case 11: {
				const std::array<std::string_view, 5> ids{"GPGH", "UCVD", "TGSD", "CDIM", "SPRC"};
				const auto &missing = SupplyWireFind(chunks, ids[fault - 7]);
				bytes.erase(bytes.begin() + missing.begin, bytes.begin() + missing.end); break;
			}
			case 12: SupplyWireSet(bytes, supply.payload + 4, 2, 2); break;
			case 13: REQUIRE(supply.end - supply.payload > 20); bytes[supply.payload + 20] = 3; break;
			case 14: {
				const auto &cargo = SupplyWireFind(chunks, "CAPA"); REQUIRE_FALSE(cargo.rows.empty());
				const std::string_view field{"company_supply_order"};
				const auto begin = bytes.begin() + cargo.rows[0].payload, end = bytes.begin() + cargo.rows[0].end;
				const auto name = std::search(begin, end, field.begin(), field.end()); REQUIRE(name != end); REQUIRE(name >= begin + 2);
				REQUIRE(*(name - 2) == 8); *(name - 2) = 6; break;
			}
			case 15: SupplyWireRename(bytes, SupplyWireFind(chunks, "CAPA"), "company_supply_order", "company_supply_ordeX"); break;
			case 16: SupplyWireSet(bytes, supply.payload + 6, 0, 8); break;
			case 17: {
				const auto &metadata = SupplyWireFind(chunks, "MLMD"); REQUIRE(metadata.rows.size() == 2);
				const auto &row = metadata.rows[1]; REQUIRE(row.end - row.payload == 51); REQUIRE(bytes[row.payload + 10] == 16);
				// Schema1stores a16-byte MD5array with its original count byte;
				// the following fixed content/feature scalars remain independently located.
				SupplyWireSet(bytes, row.payload + 31, SupplyWireGet(bytes, row.payload + 31, 4) | (1U << 27), 4); break;
			}
			case 18: {
				const auto &cargo = SupplyWireFind(chunks, "CAPA"); bool changed = false;
				// Empty native ordinal holes are not packets. Mutate only the first
				// real fragment's literal flag, retaining its actual quantity and owner.
				for (size_t index = 1; index < cargo.rows.size(); ++index) {
					const auto &row = cargo.rows[index]; if (row.payload == row.end) continue;
					bytes[row.payload + SupplyWireColumn(bytes, cargo, "company_supply_shipped")] = 2; changed = true; break;
				}
				REQUIRE(changed); break;
			}
			case 19: {
				const auto &cargo = SupplyWireFind(chunks, "CAPA"); bool changed = false;
				const auto column = SupplyWireColumn(bytes, cargo, "company_supply_order");
				// A link with the same low U32bits must still name a different
				// unknown U64receipt; narrowing it would incorrectly adopt paid stock.
				for (size_t index = 1; index < cargo.rows.size(); ++index) {
					const auto &row = cargo.rows[index]; if (row.payload == row.end) continue;
					const auto link = SupplyWireGet(bytes, row.payload + column, 8);
					if (link == 0) continue;
					SupplyWireSet(bytes, row.payload + column, link | (1ULL << 32), 8); changed = true; break;
				}
				REQUIRE(changed); break;
			}
		}
		const auto malformed = EnvelopeStream(std::move(bytes), "OTTN", version);
		const auto path = source.string() + fmt::format(".bad-{}.sav", fault);
		std::ofstream output(path, std::ios::binary); REQUIRE(output.good());
		output.write(reinterpret_cast<const char *>(malformed.data()), malformed.size()); output.close(); REQUIRE(output.good());
		CHECK(SaveOrLoad(path, SaveLoadOperation::Check, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Error);
		unchanged();
		CHECK(SaveOrLoad(path, SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) != SaveLoadResult::Ok);
		REQUIRE(SaveOrLoad(source.string(), SaveLoadOperation::Load, DetailedFileType::GameFile, Subdirectory::None, false) == SaveLoadResult::Ok);
		unchanged();
	}
}
#endif /* TESTS_SAVE_MERGED_REFUSAL_FIXTURE_H */
