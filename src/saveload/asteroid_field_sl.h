/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file asteroid_field_sl.h Explicit bounded ASTR payload adaptation under the original field owner.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Groups22/26; source-qualified ownerless no-target departure.
 * @see docs/zz-reach2/upgrades/2026-10/m2o28-1-merge-oct28.md Group 17; the writer emits its own era, and an older era must lose nothing.
 */
#ifndef SAVELOAD_ASTEROID_FIELD_SL_H
#define SAVELOAD_ASTEROID_FIELD_SL_H
#include "../asteroid.h"

/** Original ASTR source layouts; the native handler maps genuine file versions before decoding. */
enum class AsteroidFieldWireEra : uint8_t { Preview, NativeMining, StarterInputs, Lifecycle, SurveyDeparture, End };
inline constexpr size_t ASTEROID_LIFECYCLE_WIRE_BYTES = 62; ///< Explicit fixed trailer, independent of host enum/optional/Randomizer representation.
/** @param era Genuine source layout. @return Maximum bounded complete ASTR payload extent, or zero for an unknown layout. */
size_t GetAsteroidFieldWireMaximum(AsteroidFieldWireEra era);
/** Encode the field in one writer layout.
 * @param field Original validated current field; knowledge remains with SPKN.
 * @param era Layout of the writer: StarterInputs (merged 441), Lifecycle, or SurveyDeparture (the current, default). Older layouts are never written.
 * @return Canonical ID-ordered ASTR payload, or refusal without changing any owner, RNG or derived index. A layout older than
 *         SurveyDeparture is returned only when decoding it reproduces this exact field, so nothing the layout cannot hold is dropped.
 */
std::optional<std::vector<uint8_t>> EncodeAsteroidField(const AsteroidField &field, AsteroidFieldWireEra era = AsteroidFieldWireEra::SurveyDeparture);
/** @param bytes Complete original ASTR payload, bounded before any body allocation. @param era Genuine source layout, never guessed from payload bytes. @return Fully validated field with current lifecycle/derived lookup, or refusal; old fields migrate only absent schema fields, retaining original bodies/motion/history. */
std::optional<AsteroidField> DecodeAsteroidField(std::span<const uint8_t> bytes, AsteroidFieldWireEra era);
#endif /* SAVELOAD_ASTEROID_FIELD_SL_H */
