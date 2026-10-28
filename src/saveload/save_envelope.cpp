/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_envelope.cpp Bounded original-byte inspection without native chunk dispatch.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 * @see docs/zz-reach2/upgrades/2026-10/m2o23-1-come-space-b-save-lineage.md
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 11; CAPA columns and the MLLN tuple observed without a version guess.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 12; the SSOR book shape from a bounded prefix and the streaming SSM1 inspector.
 */
#include "../stdafx.h"
#include "save_envelope.h"
#include "saveload_filter.h"
#include "../core/sha256.hpp"
#include "../surface_supply_merged_wire.h"
#include <set>

namespace {

/** Fill ordinary short reads and enforce a raw-byte ceiling before any codec can request more. */
class InspectionRawReader final : public LoadFilter {
	uint64_t remaining; ///< Independent limit for this pass, including the original envelope.
	Sha256 *digest; ///< Optional exact bytes observed during this pass, independent of codec consumption.
public:
	/** @param raw Original rewindable reader. @param limit Maximum original bytes in this pass. @param digest Optional complete-pass original byte identity. */
	InspectionRawReader(std::shared_ptr<LoadFilter> raw, uint64_t limit, Sha256 *digest = nullptr) : LoadFilter(std::move(raw)), remaining(limit), digest(digest) {}
	/** @param buf Caller-owned destination. @param len Capacity. @return Complete request or actual bytes at EOF. */
	size_t Read(uint8_t *buf, size_t len) override
	{
		size_t used = 0;
		// Native block codecs expect complete requested headers; accumulate short
		// raw reads without mistaking packet fragmentation for file truncation.
		while (used < len) {
			const auto count = this->chain->Read(buf + used, len - used);
			if (count > len - used) throw SaveFramingError("Invalid raw reader byte count");
			if (count > this->remaining) throw SaveFramingError("Original save byte limit exceeded");
			if (this->digest != nullptr) this->digest->Update(std::span{buf + used, count});
			this->remaining -= count;
			used += count;
			if (count == 0) break;
		}
		return used;
	}
};

/** Fixed-size decoded buffer with explicit row/chunk fences; encoded lengths never allocate memory. */
class InspectionBytes {
	std::shared_ptr<LoadFilter> decoder; ///< Private fresh decoder, never the global loader.
	std::array<uint8_t, 65536> buffer{}; ///< One bounded native-compatible block, also large enough for LZO.
	size_t cursor = 0, available = 0; ///< Actual buffered bytes.
	uint64_t maximum, decoded = 0; ///< Independent expansion ceiling and actual produced count.
public:
	uint64_t position = 0; ///< Actual bytes consumed, independent of any claimed extent.
	/** @param decoder Private native codec. @param maximum Maximum decoded stream length. */
	InspectionBytes(std::shared_ptr<LoadFilter> decoder, uint64_t maximum) : decoder(std::move(decoder)), maximum(maximum) {}
	/** @return Whether a decoded byte remains; complete codec trailers are verified even at the chunk terminator. */
	bool HasByte()
	{
		if (this->cursor != this->available) return true;
		this->available = this->decoder->Read(this->buffer.data(), this->buffer.size());
		// Both a broken reader count and excessive expansion refuse before any
		// buffer offset or byte arithmetic can be used as save evidence.
		if (this->available > this->buffer.size() || this->available > this->maximum - this->decoded) throw SaveFramingError("Decoded save byte limit exceeded");
		this->decoded += this->available;
		this->cursor = 0;
		return this->available != 0;
	}
	/** @param end Exclusive current owner boundary. @return Original decoded byte inside that owner. */
	uint8_t Byte(uint64_t end = UINT64_MAX)
	{
		// A truncated owner cannot borrow the next row, chunk or a fabricated EOF byte.
		if (this->position >= end || !this->HasByte()) throw SaveFramingError("Truncated save owner");
		++this->position;
		return this->buffer[this->cursor++];
	}
	/** @param width Fixed integer width, at most eight. @param end Exclusive owner boundary. @return Network-order scalar. */
	uint64_t Integer(uint8_t width, uint64_t end = UINT64_MAX)
	{
		uint64_t result = 0;
		for (uint8_t byte = 0; byte < width; ++byte) result = (result << 8) | this->Byte(end);
		return result;
	}
	/** @param end Exclusive owner boundary. @return Native gamma integer, with every byte constrained to that owner. */
	uint32_t Gamma(uint64_t end = UINT64_MAX)
	{
		/** @return Next original byte inside this explicit record boundary. */
		auto read = [this, end]() { return this->Byte(end); };
		return ReadSaveGamma(read);
	}
	/** @param count Claimed payload bytes. @param end Parent boundary. @return Checked exclusive child boundary. */
	uint64_t End(uint64_t count, uint64_t end = UINT64_MAX) const
	{
		// Subtraction checks both parent ownership and overflow before adding a
		// hostile claimed extent; no current map or native pool supplies a fallback.
		if (this->position > end || count > end - this->position || count > this->maximum - this->position) throw SaveFramingError("Invalid save extent");
		return this->position + count;
	}
	/** @param end Already admitted exclusive extent. Discard payload without a payload-sized allocation. */
	void SkipTo(uint64_t end)
	{
		// Advance in actual decoded blocks so even a maximum RIFF cannot force
		// per-byte dispatch, and every compressed block/trailer remains checked.
		while (this->position < end) {
			if (!this->HasByte()) throw SaveFramingError("Truncated skipped payload");
			const size_t count = static_cast<size_t>(std::min<uint64_t>(end - this->position, this->available - this->cursor));
			this->position += count; this->cursor += count;
		}
	}
};

/** Detached table schema; only bounded field names/types and child schemas survive header inspection. */
struct InspectionColumn {
	std::string name; ///< Original field name, at most 255 bytes.
	uint8_t type; ///< Original native type plus the native field-length bit.
	std::vector<InspectionColumn> children; ///< Bounded nested struct schema, never gameplay records.
};
using Columns = std::vector<InspectionColumn>;

/** Decode one bounded native table schema, including child headers after the parent terminator.
 * @param bytes Private decoded reader.
 * @param end Exact header row boundary.
 * @param limits Explicit ceilings checked before name/vector allocation.
 * @param fields Aggregate field count shared by this whole nested schema.
 * @param depth Current nesting depth, starting at one.
 * @return Original bounded schema; duplicate keys and unsupported types refuse.
 */
Columns ReadColumns(InspectionBytes &bytes, uint64_t end, const SaveInspectionLimits &limits, uint32_t &fields, uint16_t depth = 1)
{
	if (depth > limits.depth) throw SaveFramingError("Save table nesting limit exceeded");
	Columns columns;
	std::set<std::string> names;
	// Each nonzero descriptor consumes a bounded field/name. Children follow
	// the parent terminator in original field order, exactly as SlTableHeader.
	while (true) {
		const auto type = bytes.Byte(end);
		const auto framing = DecodeSaveTableField(type);
		if (framing.terminal) break;
		const uint8_t primitive = framing.primitive;
		// Unknown high bits/types cannot choose an arbitrary field width; String
		// and Struct require their actual native length-bearing descriptor.
		if (type > 31 || primitive == 0 || primitive > 11 || (primitive >= 10 && !framing.has_length)) throw SaveFramingError("Invalid save field type");
		if (fields == limits.fields) throw SaveFramingError("Save table field limit exceeded");
		++fields;
		const auto length = bytes.Gamma(end);
		if (length == 0 || length > 255 || length > end - bytes.position) throw SaveFramingError("Invalid save field name length"); // Empty/oversized names cannot alias a required paid-history column.
		std::string name;
		name.reserve(length);
		for (uint32_t index = 0; index < length; ++index) name.push_back(static_cast<char>(bytes.Byte(end)));
		if (!names.insert(name).second) throw SaveFramingError("Repeated save table field");
		columns.push_back({std::move(name), type, {}});
	}
	// Every declared struct owns exactly one child header; neither row bytes
	// nor another chunk may be consumed to fill an omitted nested schema.
	for (auto &column : columns) {
		if (DecodeSaveTableField(column.type).primitive == 11) column.children = ReadColumns(bytes, end, limits, fields, depth + 1);
	}
	return columns;
}

/** Native table field codes used by the observed CAPA column contracts (the file's own descriptor bytes). */
constexpr uint8_t FIELD_I8 = 1; ///< Signed byte; the historical B writer stored its Bool shipped flag this way.
constexpr uint8_t FIELD_U8 = 2; ///< Unsigned byte; the merged shipped flag.
constexpr uint8_t FIELD_U32 = 6; ///< Unsigned 32-bit; the legacy B receipt link and both typed asteroid keys.
constexpr uint8_t FIELD_U64 = 8; ///< Unsigned 64-bit; the merged canonical receipt link.

/** Find one field by its exact original name.
 * @param columns Actual bounded schema; names are already unique.
 * @param name Exact source field name.
 * @return The field, or nullptr when the file does not declare it.
 */
const InspectionColumn *FindColumn(const Columns &columns, std::string_view name)
{
	/** @param column Original field. @return Whether it has the requested name. */
	const auto found = std::ranges::find_if(columns, [name](const auto &column) { return column.name == name; });
	return found == columns.end() ? nullptr : &*found;
}

/** Observe the CAPA supply pair from the descriptor alone, so an empty CAPA and chunk order both give the same answer.
 * @param columns Original bounded CAPA schema.
 * @return The declared encoding; a half pair or mixed widths is Malformed.
 */
SupplyColumnEncoding ObserveSupplyColumns(const Columns &columns)
{
	const auto order = FindColumn(columns, "company_supply_order");
	const auto shipped = FindColumn(columns, "company_supply_shipped");
	if (order == nullptr && shipped == nullptr) return SupplyColumnEncoding::Absent;
	// The receipt link and the shipped flag are one paid-stock contract. A half
	// pair would let a packet claim a receipt without shipment state, or the reverse.
	if (order == nullptr || shipped == nullptr) return SupplyColumnEncoding::Malformed;
	// Each writer used one exact width pair. A mixed pair belongs to no writer, so
	// it can neither narrow a merged U64 receipt nor widen a legacy B one.
	if (order->type == FIELD_U32 && shipped->type == FIELD_I8) return SupplyColumnEncoding::LegacyU32Bool;
	if (order->type == FIELD_U64 && shipped->type == FIELD_U8) return SupplyColumnEncoding::CanonicalU64U8;
	return SupplyColumnEncoding::Malformed;
}

/** Observe the typed asteroid origin key from the descriptor alone.
 * @param columns Original bounded CAPA schema.
 * @return Absent, Valid for exactly two U32 keys, or Malformed for any other shape.
 */
SaveEvidenceState ObserveAsteroidOriginColumns(const Columns &columns)
{
	const auto body = FindColumn(columns, "asteroid_body");
	const auto mineral = FindColumn(columns, "asteroid_mineral");
	if (body == nullptr && mineral == nullptr) return SaveEvidenceState::Absent;
	// A typed origin is one (body, mineral) key. Both halves must be present at
	// full U32 width: a lone or narrowed half would truncate a body ID or lose
	// the immutable mineral label that the packet's custody check relies on.
	if (body != nullptr && mineral != nullptr && body->type == FIELD_U32 && mineral->type == FIELD_U32) return SaveEvidenceState::Valid;
	return SaveEvidenceState::Malformed;
}

/** @param columns Actual schema. @param name Exact source field name. @param type Exact native descriptor. @return Matching mandatory field. */
bool HasColumn(const Columns &columns, std::string_view name, uint8_t type)
{
	/** @param column Original field. @return Whether it has the requested exact name and native type. */
	return std::ranges::any_of(columns, [name, type](const auto &column) { return column.name == name && column.type == type; });
}

/** Check a lineage-selected schema against its exact historical field contract.
 * @param columns Original bounded schema.
 * @param required Required original name/type pairs; ordering remains the file's own.
 */
void RequireColumns(const Columns &columns, std::initializer_list<std::pair<std::string_view, uint8_t>> required)
{
	// Even an empty table must carry every original required field with its
	// exact type; otherwise absence could manufacture default paid authority.
	for (const auto &[name, type] : required) {
		if (!HasColumn(columns, name, type)) throw SaveFramingError("Missing or changed lineage table field");
	}
}

/** @param primitive Native scalar type code. @return Original fixed byte width, or zero for non-scalars. */
uint8_t ScalarWidth(uint8_t primitive)
{
	static constexpr uint8_t widths[]{0, 1, 1, 2, 2, 4, 4, 8, 8, 2};
	return primitive < std::size(widths) ? widths[primitive] : 0;
}

/** Read the exact scalar/array framing of selected flat tables without allocating records.
 * @param bytes Private reader at the row's first field.
 * @param columns Exact original schema.
 * @param end Exclusive row boundary.
 * @param metadata Collect only MLMD's classifier-required scalars when non-null.
 */
void ReadSelectedRow(InspectionBytes &bytes, const Columns &columns, uint64_t end, SaveWireFacts *metadata = nullptr)
{
	// Selected signatures are flat. Unknown extension fields are skipped only
	// by their explicit fixed scalar/array lengths, never via a native handler.
	for (const auto &column : columns) {
		const auto framing = DecodeSaveTableField(column.type);
		const auto width = ScalarWidth(framing.primitive);
		if (width == 0) throw SaveFramingError("Unexpected nested lineage field");
		if (framing.has_length) {
			const uint64_t count = bytes.Gamma(end);
			if (count > (end - bytes.position) / width) throw SaveFramingError("Invalid lineage array length");
			// The mandatory metadata digest is exactly sixteen original bytes;
			// a changed length cannot be replaced with the current package hash.
			if (metadata != nullptr && column.name == "required_content.md5sum" && count != 16) throw SaveFramingError("Invalid metadata digest extent");
			bytes.SkipTo(bytes.End(count * width, end));
			continue;
		}
		const auto value = bytes.Integer(width, end);
		if (metadata == nullptr) continue;
		if (column.name == "feature_flags") metadata->feature_flags = static_cast<uint32_t>(value);
		if (column.name == "schema_version" && value != 1) throw SaveFramingError("Unsupported metadata schema"); // Only original schema1 has this profile/feature meaning.
		if (column.name == "policy_flags" && value > 3) throw SaveFramingError("Unknown metadata policy"); // Unknown policy cannot suppress original feature admission.
		if (column.name == "profile_id" && value > 1) throw SaveFramingError("Unknown metadata profile"); // A future profile cannot inherit Classic or Space defaults.
	}
	if (bytes.position != end) throw SaveFramingError("Invalid lineage row length");
}

/** Map only the fixed classifier-selected families, independently of the gameplay handler registry.
 * @param id Original four-byte chunk name.
 * @return Selected fact slot, or Count for unneeded gameplay payload.
 */
SaveLineageFamily Family(std::string_view id)
{
	using F = SaveLineageFamily;
	static constexpr std::pair<std::string_view, F> names[]{
		{"SSOH", F::SupplyHeader}, {"SSOR", F::SupplyOrders}, {"GPGH", F::GeneratorHeader}, {"GPGD", F::GeneratorRows},
		{"UCVH", F::ConversionHeader}, {"UCVD", F::ConversionRows}, {"TGSH", F::TubeSourceHeader}, {"TGSD", F::TubeSourceRows},
		{"CDIM", F::CargoDimensions}, {"SPRC", F::OrbitalRecipes}, {"MLLN", F::Marker},
	};
	for (const auto &[name, family] : names) { if (name == id) return family; }
	return F::Count;
}

/** Enforce historical selected flat-table descriptors before any rows, including empty tables.
 * CAPA facts come from the descriptor, never from `original_version == 441`, so they do not depend on chunk order.
 * @param id Original selected family name.
 * @param columns Actual original field names/types.
 * @param facts Same-file version and collected CAPA extension evidence.
 */
void CheckSelectedColumns(std::string_view id, const Columns &columns, SaveWireFacts &facts)
{
	if (id == "MLMD") {
		RequireColumns(columns, {{"schema_version", 4}, {"profile_id", 2}, {"profile_version", 4}, {"policy_flags", 2}, {"required_content.grfid", 6}, {"required_content.md5sum", 18}, {"content_version", 6}, {"feature_flags", 6}, {"geology_seed", 8}, {"orbital_seed", 8}});
		if (columns.size() != 10) throw SaveFramingError("Changed metadata field count");
	} else if (id == "CAPA") {
		if (facts.original_version >= 424) RequireColumns(columns, {{"source", 4}, {"source_xy", 6}, {"loaded_at_xy", 4}, {"count", 4}, {"periods_in_transit", 4}, {"feeder_share", 7}, {"source_type", 2}, {"source_id", 4}, {"travelled.x", 3}, {"travelled.y", 3}});
		// Presence with the wrong type is corruption, not historical absence. Both
		// observations are recorded even with zero packets; the classifier refuses Malformed.
		facts.cargo_supply_encoding = ObserveSupplyColumns(columns);
		facts.asteroid_origin_columns = ObserveAsteroidOriginColumns(columns);
		// Legacy view for the existing classifier rules: only B's U32/Bool pair is
		// "Complete". The merged U64/U8 pair never counts as B evidence or narrows its IDs.
		switch (facts.cargo_supply_encoding) {
			case SupplyColumnEncoding::LegacyU32Bool: facts.supply_columns = SaveSupplyColumns::Complete; break;
			case SupplyColumnEncoding::Malformed: facts.supply_columns = SaveSupplyColumns::Malformed; break;
			case SupplyColumnEncoding::Absent: case SupplyColumnEncoding::CanonicalU64U8: facts.supply_columns = SaveSupplyColumns::Absent; break;
		}
		// The canonical pair is the merged descriptor proof, and it travels with the
		// merged packet columns; a file that declares it must carry those too.
		if (facts.cargo_supply_encoding == SupplyColumnEncoding::CanonicalU64U8) RequireColumns(columns, {{"count", 4}, {"source_type", 2}, {"source_id", 4}, {"feeder_share", 7}});
		facts.merged_cargo_valid = facts.cargo_supply_encoding == SupplyColumnEncoding::CanonicalU64U8;
	} else if (id == "SSOR") {
		RequireColumns(columns, {{"id", 6}, {"owner", 2}, {"station", 4}, {"label", 6}, {"economy_day", 5}, {"calendar_date", 5}, {"quantity", 4}, {"unit_price", 7}, {"refunded", 4}, {"dispatched", 4}, {"closed", 2}});
	} else if (id == "GPGD") {
		RequireColumns(columns, {{"industry", 4}, {"site", 6}, {"owner", 2}, {"model", 2}, {"work_ticks", 4}, {"materials_committed", 2}, {"has_work_tick", 2}, {"last_work_tick", 8}});
		if (facts.original_version >= 429) RequireColumns(columns, {{"heat_consumer", 6}, {"electrical_producer", 6}, {"service_remainder", 2}, {"has_operating_tick", 2}, {"last_operating_tick", 8}, {"active_service_ticks", 8}});
	} else if (id == "UCVD") {
		RequireColumns(columns, {{"kind", 2}, {"output", 6}, {"input", 6}, {"efficiency", 2}, {"numerator", 2}, {"capacity_w", 8}, {"origin", 6}, {"heat_j", 8}});
	} else if (id == "TGSD") {
		RequireColumns(columns, {{"id", 6}, {"generator", 4}, {"producer", 6}, {"supply", 6}, {"owner", 2}, {"port", 6}, {"inlet", 6}, {"depot", 4}, {"station", 4}, {"active", 2}});
	}
}

/** Observe one RIFF SSOR payload without choosing its writer.
 * The fixed A440 shape comes from a bounded prefix and the extent; the SSM1 book is
 * streamed through its own inspector, replaying that prefix first. Nothing payload-sized
 * is allocated and no receipt is materialized.
 * @param bytes Private reader at the payload start.
 * @param end Checked exclusive payload boundary.
 * @param facts Same-file observations; receives the shape and, for a validated tagged book, its frontier.
 * @param evidence SSOR family slot; receives the record count of the reading that fits.
 */
void ObserveSupplyBook(InspectionBytes &bytes, uint64_t end, SaveWireFacts &facts, SaveChunkEvidence &evidence)
{
	const uint64_t length = end - bytes.position;
	std::array<uint8_t, MERGED_SURFACE_SUPPLY_HEADER_SIZE> prefix{}; // Header-sized, never payload-sized.
	const size_t prefix_size = static_cast<size_t>(std::min<uint64_t>(length, prefix.size()));
	for (size_t index = 0; index < prefix_size; ++index) prefix[index] = bytes.Byte(end);
	// Fixed A440 shape: twelve header bytes, a little-endian record count, and an
	// extent of exactly 14 + 53 * n. A count above the native book limit fits nothing.
	std::optional<uint32_t> fixed_records;
	if (prefix_size >= 14) {
		const uint32_t count = prefix[12] | (uint32_t{prefix[13]} << 8);
		if (count <= 16384 && length == 14 + 53 * uint64_t{count}) fixed_records = count;
	}
	const bool signature = prefix_size >= 4 && ((uint32_t{prefix[0]} << 24) | (uint32_t{prefix[1]} << 16) | (uint32_t{prefix[2]} << 8) | prefix[3]) == MERGED_SURFACE_SUPPLY_MAGIC;
	std::optional<MergedSurfaceSupplyWireSummary> tagged;
	if (signature) {
		size_t cursor = 0;
		/** @param width Network-order scalar width requested by the inspector. @return The next bits, served from the prefix first, then from this payload. */
		const auto read = [&](size_t width) {
			uint64_t value = 0;
			for (size_t byte = 0; byte < width; ++byte) value = (value << 8) | (cursor < prefix_size ? prefix[cursor++] : bytes.Byte(end));
			return value;
		};
		tagged = InspectMergedSurfaceSupplyOrders(static_cast<size_t>(length), read);
	}
	// A recognized SSM1 signature never falls through to the fixed reading: if its
	// book is invalid the family is malformed. When both readings validate on the
	// same bytes, the overlap is kept as Ambiguous for the classifier to refuse.
	if (tagged && fixed_records) {
		facts.supply_book_shape = SupplyBookShape::Ambiguous;
	} else if (tagged) {
		facts.supply_book_shape = SupplyBookShape::Tagged;
	} else if (signature || !fixed_records) {
		facts.supply_book_shape = SupplyBookShape::Malformed;
	} else {
		facts.supply_book_shape = SupplyBookShape::Fixed;
	}
	if (tagged) {
		evidence.schema = MERGED_SURFACE_SUPPLY_SCHEMA; evidence.records = tagged->records;
		facts.merged_supply_next_id = tagged->next_id; facts.merged_supply_valid = true;
	} else if (fixed_records) {
		evidence.records = *fixed_records;
	}
}

/** Collect selected fixed RIFF scalars; unneeded owner data is discarded through the bounded reader.
 * @param id Original chunk name.
 * @param bytes Private reader.
 * @param end Checked exclusive payload boundary.
 * @param facts Same-file observations only.
 * @param evidence Selected family slot, or null for common/unneeded transport.
 */
void ReadRiff(std::string_view id, InspectionBytes &bytes, uint64_t end, SaveWireFacts &facts, SaveChunkEvidence *evidence)
{
	const uint64_t length = end - bytes.position;
	/** @param expected Exact historical extent. */
	auto exact = [length](uint64_t expected) { if (length != expected) throw SaveFramingError("Invalid lineage RIFF extent"); };
	if (id == "SDEF") { exact(10); facts.space_content_version = static_cast<uint16_t>(bytes.Integer(2, end)); }
	if (id == "SPST") {
		const uint64_t header = facts.original_version >= 422 ? 42 : 10;
		if (length < header) throw SaveFramingError("Truncated structure retirement header");
		bytes.Integer(8, end);
		const auto count = bytes.Integer(2, end);
		// Empty common structures have exactly their retirement header; extra
		// hidden bytes cannot fabricate variable-layout lineage evidence.
		if (count == 0 && length != header) throw SaveFramingError("Trailing empty structure payload");
	}
	if (evidence != nullptr) {
		using F = SaveLineageFamily;
		switch (Family(id)) {
			case F::SupplyHeader: {
				exact(12); const auto schema = bytes.Integer(4, end), count = bytes.Integer(4, end), next = bytes.Integer(4, end);
				if (schema != 1 || count > 65535 || next != count + 1) throw SaveFramingError("Invalid supply header bounds"); // Dense retained history must fit its original never-issued frontier.
				evidence->schema = 1; evidence->records = static_cast<uint32_t>(count); break;
			}
			case F::GeneratorHeader: {
				exact(8); const auto schema = bytes.Integer(4, end), count = bytes.Integer(4, end);
				if (schema != (facts.original_version >= 429 ? 2 : 1) || count > 256) throw SaveFramingError("Invalid generator header bounds"); // Original machinery revision and retained site ceiling precede row admission.
				evidence->schema = static_cast<uint16_t>(schema); evidence->records = static_cast<uint32_t>(count); break;
			}
			case F::ConversionHeader: {
				exact(32); const auto schema = bytes.Integer(8, end), sources = bytes.Integer(8, end), origins = bytes.Integer(8, end), epoch = bytes.Integer(8, end);
				if (schema != 1 || sources > 4096 || origins > 16384 || epoch == 0 || epoch == UINT64_MAX) throw SaveFramingError("Invalid conversion header bounds"); // Both row roles and the common nonterminal accounting epoch must remain original.
				evidence->schema = 1; evidence->records = static_cast<uint32_t>(sources + origins); break;
			}
			case F::TubeSourceHeader: {
				exact(20); const auto schema = bytes.Integer(4, end), next = bytes.Integer(4, end), sequence = bytes.Integer(8, end), count = bytes.Integer(4, end);
				if (schema != 1 || count > 256 || next != count || sequence < count || sequence == UINT64_MAX || (count == 0 && sequence != 0)) throw SaveFramingError("Invalid tube source header bounds"); // Original dense source incarnations and once-only sequence must agree before any binding can be interpreted.
				evidence->schema = 1; evidence->records = static_cast<uint32_t>(count); break;
			}
			case F::CargoDimensions: {
				evidence->records = bytes.Byte(end);
				exact(1 + 6 * evidence->records); break;
			}
			case F::OrbitalRecipes: if (length < 11) throw SaveFramingError("Truncated recipe retirement header"); break;
			case F::SupplyOrders: ObserveSupplyBook(bytes, end, facts, *evidence); break; // Shape only; no version or marker picks the decoder.
			case F::Marker: {
				exact(8); // The tuple is recorded only after exact eight-byte framing.
				facts.marker_magic = static_cast<uint32_t>(bytes.Integer(4, end)); facts.marker_schema = static_cast<uint16_t>(bytes.Integer(2, end)); facts.marker_writer_version = static_cast<uint16_t>(bytes.Integer(2, end));
				const bool first_allocation = facts.marker_writer_version == MERGED_SAVE_LINEAGE_VERSION && facts.marker_schema == MERGED_SAVE_LINEAGE_SCHEMA;
				const bool current_allocation = facts.marker_writer_version == CURRENT_MERGED_SAVE_VERSION && facts.marker_schema == CURRENT_MERGED_SAVE_LINEAGE_SCHEMA;
				// A marker names exactly one merged allocation (441/schema 1 or 446/schema 2),
				// and its writer is the outer envelope's own version. Any other tuple is a
				// contradiction: the family is Malformed, which the classifier refuses before
				// any legacy rule could reinterpret the file. The envelope version is read
				// before any chunk, so MLLN before or after CAPA and SSOR gives the same answer.
				if (facts.marker_magic != MERGED_SAVE_LINEAGE_MAGIC || !(first_allocation || current_allocation) || facts.marker_writer_version != facts.original_version) evidence->state = SaveEvidenceState::Malformed;
				break;
			}
			default: throw SaveFramingError("Lineage table encoded as RIFF");
		}
	}
	bytes.SkipTo(end);
}

/** Scan chunks without any gameplay callback; common transport means actual validated same-file framing.
 * @param bytes Private decoded reader.
 * @param facts Original envelope identity and destination observations.
 * @param limits Work/allocation ceilings.
 */
void ReadChunks(InspectionBytes &bytes, SaveWireFacts &facts, const SaveInspectionLimits &limits)
{
	std::set<std::string> ids;
	uint32_t total_rows = 0;
	// One original zero identifier ends the stream. Every other chunk has one
	// unique owner and fully bounded RIFF or native array/table framing.
	while (true) {
		std::string id;
		for (uint8_t index = 0; index < 4; ++index) id.push_back(static_cast<char>(bytes.Byte()));
		if (id == std::string(4, '\0')) break;
		if (ids.size() == limits.chunks) throw SaveFramingError("Save chunk count limit exceeded");
		if (!ids.insert(id).second) throw SaveFramingError("Repeated save chunk identifier");
		// Shared owners are source-era evidence too: a forged early family must
		// not bypass its real introduction by inheriting current default meanings.
		if ((id == "MLMD" && facts.original_version < 371) || (id == "SDEF" && facts.original_version < 410) || (id == "SPST" && facts.original_version < 415)) throw SaveFramingError("Unexpected source-era common family");
		const auto mode = bytes.Byte(), kind = static_cast<uint8_t>(mode & 15);
		const auto family = Family(id);
		SaveChunkEvidence *evidence = family == SaveLineageFamily::Count ? nullptr : &facts.families[static_cast<uint8_t>(family)];
		// Before native TABLE introduction, CAPA carries its original sparse-array
		// layout without named columns. It is framed/skipped rather than assigned
		// modern source fields that the historical file never serialized.
		const bool selected = evidence != nullptr || id == "MLMD" || (id == "CAPA" && facts.original_version >= 295);
		if (evidence != nullptr) { evidence->state = SaveEvidenceState::Valid; evidence->kind = kind == 0 ? SaveEvidenceKind::Riff : SaveEvidenceKind::Table; }
		if (kind == 0) {
			if (id == "MLMD" || id == "CAPA") throw SaveFramingError("Common table encoded as RIFF"); // Shared metadata/packet type evidence cannot be invented from opaque payload.
			/** @return Next original RIFF length byte. */
			auto read = [&bytes]() { return bytes.Byte(); };
			const auto length = ReadSaveRiffLength(mode, read);
			if (evidence != nullptr) evidence->length = length;
			ReadRiff(id, bytes, bytes.End(length), facts, evidence);
			continue;
		}
		// Only RIFF owns the upper length nibble; selected lineage signatures
		// require actual TABLE mode, and fixed common definitions require RIFF.
		if (kind > 4 || mode != kind || (selected && kind != 3) || id == "SDEF" || id == "SPST") throw SaveFramingError("Invalid save chunk kind");
		Columns columns;
		if (SaveChunkHasTableHeader(mode)) {
			const auto header = DecodeSaveArrayRow(bytes.Gamma());
			if (header.terminal || header.payload_bytes > limits.header_bytes) throw SaveFramingError("Invalid save table header extent"); // Header storage is bounded before schema allocation and cannot be replaced by an array terminator.
			const auto end = bytes.End(header.payload_bytes);
			uint32_t fields = 0;
			columns = ReadColumns(bytes, end, limits, fields);
			if (bytes.position != end) throw SaveFramingError("Trailing save table header bytes");
			if (selected) CheckSelectedColumns(id, columns, facts);
		}
		uint32_t records = 0;
		// Count every encoded row, including empty contiguous holes. Sparse row
		// indices live inside their own row extent and cannot consume a next row.
		while (true) {
			const auto row = DecodeSaveArrayRow(bytes.Gamma());
			if (row.terminal) break;
			if (total_rows == limits.rows) throw SaveFramingError("Save row count limit exceeded");
			++total_rows;
			const auto end = bytes.End(row.payload_bytes);
			if (kind == 2 || kind == 4) bytes.Gamma(end); // Both native sparse encodings carry their explicit original index within the row.
			// Native contiguous CAPA tables preserve empty ordinal holes between
			// real packet IDs. They carry no record and must not fabricate scalars;
			// the selected dense machinery/metadata tables still require full rows.
			if (selected && !(id == "CAPA" && row.payload_bytes == 0)) ReadSelectedRow(bytes, columns, end, id == "MLMD" ? &facts : nullptr);
			bytes.SkipTo(end);
			++records;
		}
		if (id == "MLMD" && records != 1) throw SaveFramingError("Metadata must have exactly one original row"); // Empty/extra policy rows cannot choose a fallback profile or hide flags.
		if (evidence != nullptr) evidence->records = records;
	}
	if (bytes.HasByte()) throw SaveFramingError("Trailing decoded save bytes");
	// The four source-era common owners must actually exist with their validated
	// kinds/header fields; absent older transport never consults the live world.
	facts.common_transport_valid = (facts.original_version < 371 || ids.contains("MLMD")) &&
		(facts.original_version < 410 || ids.contains("SDEF")) &&
		(facts.original_version < 415 || ids.contains("SPST")) &&
		(facts.original_version < 68 || ids.contains("CAPA"));
	facts.framing_complete = true;
}

} // namespace

/** @copydoc InspectSaveWireFacts */
SaveWireFacts InspectSaveWireFacts(const std::shared_ptr<LoadFilter> &raw, const SaveInspectionLimits &limits, bool allow_headerless_lzo)
{
	if (raw == nullptr) throw SaveFramingError("Missing original save reader");
	raw->Reset();
	try {
		SaveWireFacts facts;
		Sha256 digest;
		std::array<uint8_t, 65536> block{};
		InspectionRawReader identity(raw, limits.raw_bytes);
		// Hash every original compressed byte, including envelope and trailing
		// codec data, independently of decoder read-ahead or the logical terminator.
		while (const auto count = identity.Read(block.data(), block.size())) digest.Update(std::span{block}.first(count));
		facts.source_digest = digest.Digest();
		raw->Reset();
		Sha256 inspected_digest;
		auto bounded = std::make_shared<InspectionRawReader>(raw, limits.raw_bytes, &inspected_digest);
		std::array<uint8_t, 8> envelope{};
		if (bounded->Read(envelope.data(), envelope.size()) != envelope.size()) throw SaveFramingError("Truncated save envelope");
		facts.original_version = (uint16_t{envelope[4]} << 8) | envelope[5];
		std::array<uint8_t, 4> tag{};
		std::copy_n(envelope.begin(), 4, tag.begin());
		const bool modern = tag == std::array<uint8_t, 4>{'O', 'T', 'T', 'N'} || tag == std::array<uint8_t, 4>{'O', 'T', 'T', 'D'} ||
			tag == std::array<uint8_t, 4>{'O', 'T', 'T', 'Z'} || tag == std::array<uint8_t, 4>{'O', 'T', 'T', 'X'};
		// Only native load entry opts into its historical headerless LZO path.
		// Restart both byte accounting and identity at the actual original start;
		// checksum/length words are compressed data, not a modern version envelope.
		if (!modern && allow_headerless_lzo) {
			raw->Reset(); inspected_digest = Sha256{};
			bounded = std::make_shared<InspectionRawReader>(raw, limits.raw_bytes, &inspected_digest);
			tag = {'O', 'T', 'T', 'D'}; facts.original_version = 0;
		}
		InspectionBytes bytes(CreateSaveInspectionDecoder(bounded, tag, facts.original_version), limits.decoded_bytes);
		ReadChunks(bytes, facts, limits);
		// Include compressed bytes that the codec did not need/read ahead into.
		// A changed raw source between identity and parsing must refuse instead
		// of binding one file's facts to another file's original digest.
		while (bounded->Read(block.data(), block.size()) != 0) {}
		if (inspected_digest.Digest() != facts.source_digest) throw SaveFramingError("Original save changed during inspection");
		raw->Reset();
		return facts;
	} catch (...) {
		raw->Reset();
		throw;
	}
}
