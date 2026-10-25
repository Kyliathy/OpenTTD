/* This file is part of OpenTTD, licensed under the GNU GPL version 2. */
/** @file ground_render_camera_fixture.h Actual camera/visibility independence for installed Ground selectors.
 * @see docs/zz-reach2/architecture/map-layers/archi-map-layers.md
 * @see docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md
 */
#ifndef TESTS_GROUND_RENDER_CAMERA_FIXTURE_H
#define TESTS_GROUND_RENDER_CAMERA_FIXTURE_H
#include "../gfx_type.h"
#include <functional>
#include <vector>
/** Prepare real window language before a partial component fixture installs its deliberately narrow cargo definitions. */
void PrepareGroundRenderCameraLanguage();
/** Compare actual owner selections through main/extra camera and native visibility changes.
 * @param query Pure consumer query returning its complete sprite identity vector at a fixed phase.
 * Real stock/accounts/RNG/clock are observed. Raster emission and earned transport remain separate.
 */
void CheckGroundRenderCameraPurity(const std::function<std::vector<SpriteID>()> &query);
#endif /* TESTS_GROUND_RENDER_CAMERA_FIXTURE_H */
