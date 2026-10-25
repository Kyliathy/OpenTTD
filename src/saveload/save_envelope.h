/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file save_envelope.h Detached byte preflight and fresh decoder boundary.
 * @see docs/zz-reach2/architecture/savegame/archi-savegame.md
 */
#ifndef SAVE_ENVELOPE_H
#define SAVE_ENVELOPE_H

#include "save_lineage.h"
#include "save_framing.h"
#include <array>
#include <memory>

struct LoadFilter;
class FileHandle;

/** Explicit resource ceilings; scanning never allocates a payload-sized buffer or gameplay pool. */
struct SaveInspectionLimits {
	uint64_t raw_bytes = uint64_t{8} << 30; ///< Bound original compressed identity work.
	uint64_t decoded_bytes = uint64_t{8} << 30; ///< Bound expansion independently of raw size.
	uint32_t chunks = 4096; ///< Bound distinct chunk identifiers and dispatch work.
	uint32_t rows = 16777216; ///< Bound aggregate array records across the entire stream.
	uint32_t header_bytes = 1048576; ///< Bound one schema before storing names/columns.
	uint32_t fields = 65535; ///< Bound aggregate fields across all nested tables.
	uint16_t depth = 32; ///< Bound recursive schema nesting before descent.
};

/** Inspect the exact raw stream using detached errors and the original native codec.
 * @param raw Original rewindable file or already-buffered network reader, at its own start.
 * @param limits Explicit bounded work/allocation policy; defaults admit native map sizes.
 * @param allow_headerless_lzo Native load entry may explicitly retain its ancient pre-envelope LZO fallback; ordinary modern inspection refuses unknown tags.
 * @return Same-file facts with the digest of all original compressed bytes. Raw input is rewound on success and failure.
 * @throws SaveFramingError Invalid/truncated framing, unsupported codec or resource ceiling.
 * No command, gameplay chunk handler, global ReadBuffer or world initialization is reachable from the scanner.
 */
SaveWireFacts InspectSaveWireFacts(const std::shared_ptr<LoadFilter> &raw, const SaveInspectionLimits &limits = {}, bool allow_headerless_lzo = false);

/** Reconstruct native codec state without consulting or mutating global save state.
 * @param raw Original reader positioned just after the eight-byte modern envelope.
 * @param tag Original four-byte compression identifier.
 * @param version Original unchanged numeric version, including LZO's ancient endianness distinction.
 * @return Fresh decoder whose failures throw detached SaveFramingError exclusively.
 */
std::shared_ptr<LoadFilter> CreateSaveInspectionDecoder(std::shared_ptr<LoadFilter> raw, const std::array<uint8_t, 4> &tag, uint16_t version);

/** Expose the same file reader used by native loading for byte inspection/replay.
 * @param file Owned open file, retaining its actual initial offset as the reset point.
 * @return Original native file-backed reader; ownership transfers without copying file bytes.
 */
std::shared_ptr<LoadFilter> CreateSaveFileReader(FileHandle &&file);

#endif /* SAVE_ENVELOPE_H */
