/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file space_cargo_preview.h Same-file copied packet ownership across ordinary and Space containers.
 * @see docs/zz-reach2/upgrades/2026-10/r2sff1-space-fixes-features-1.md Group16; candidate-only historical origins and mineral joins.
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group 9CD
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group12; same-file hauler cargo meanings, capacities and whole-packet mass.
 * This validation boundary owns no inventory, packet, vehicle, station, work or utility account.
 */
#ifndef SAVELOAD_SPACE_CARGO_PREVIEW_H
#define SAVELOAD_SPACE_CARGO_PREVIEW_H

struct LoadCheckData;

/** Join original copied references to their same-file physical packet identities without consulting runtime pools.
 * @param data Complete unpublished native/feature metadata from one checked file, including legitimately empty native families.
 * @return Whether every referenced packet exists with positive quantity and belongs to exactly one inspected container. Original family validators retain recipe/quantity/role admission.
 */
bool ValidateSpaceCargoPreviewOwners(const LoadCheckData &data);

/** @param data Complete unpublished same-file body/packet/container metadata. @return Whether referenced asteroid packets' issued origins, production geometry and container labels agree without live-world lookup. ValidateSpaceCargoPreviewOwners separately rejects duplicate/orphan packets. Genuine old files without CDIM retain their original full-load-only cargo-slot check; modern typed origins require explicit candidate labels. */
bool ValidateSpaceAsteroidCargoPreview(const LoadCheckData &data);
/** @param data Complete genuine older candidate after its existing body/cargo semantic checks. @return Whether all legacy packet origins resolve before any copied row changes; only those same-file rows normalize, with an explicit legacy receipt preserving pre-CDIM admission. No active-world packet, company, cargo definition or field is consulted. */
bool NormalizeLegacyAsteroidCargoPreview(LoadCheckData &data);

/** Match immutable descent weights, processor buffers and hauler holdings to this file's configured cargo metadata, without loading NewGRFs.
 * @param data Complete unpublished same-file preview with its version-admitted dimension family.
 * @return Whether configured ordinals are unique/bounded and every known descent, buffer and hauler uses its actual saved cargo meaning/mass. Genuine unknown legacy receipts retain their original native-only weight check.
 */
bool ValidateSpaceCargoPreviewDimensions(const LoadCheckData &data);

/** Validate staged configured dimensions against actual definitions after normal profile/NewGRF reconstruction.
 * @return Whether the current source supplies its exact configured catalogue, or a genuine older source explicitly has none. No gameplay definition is installed from metadata.
 */
bool AcceptLoadedSpaceCargoDimensions();

#endif
