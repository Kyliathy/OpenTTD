# First-party content distribution/runtime language pairing: docs/zz-reach2/upgrades/2026-10/r2mlvs1-gameplay-contracts-and-content.md and r2mlvs6-mines-and-resource-chains.md.
# Focused actual-engine Space art validation: docs/zz-reach2/upgrades/2026-10/r2mlvs10-asteroids-and-mining-probes.md Group14BG.
# Original orbital selectors/decoded append: docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group13K.
# Native rocket source/phase selectors: docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group62J.
# Isolated genuine productive recovery admission: docs/zz-reach2/upgrades/2026-10/r2mlvs11-orbital-stations-and-manufacturing.md Group15CSGZU.
# Current underground processor source qualification: docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group41B.
# Selected underground complete native loading: docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Group79E.
# Current selected-ground typed/consumer boundary: docs/zz-reach2/upgrades/2026-10/r2mlvs14-remaining-assets-and-runtime-integration.md Groups79FB-H.
# Focused tube terminal/cargo producers: docs/zz-reach2/upgrades/2026-10/r2mlvs17-underground-transport-tubes.md Group9.
cmake_minimum_required(VERSION 3.17)

if(NOT BINARY_NAME)
    set(BINARY_NAME openttd)
endif()

project(${BINARY_NAME}
    VERSION 16.0
    LANGUAGES CXX
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Please run \"cmake ..\" from the build directory. You may need to delete \"${CMAKE_SOURCE_DIR}/CMakeCache.txt\" first.")
endif()

# Debug mode by default.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if (EMSCRIPTEN)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/os/emscripten/cmake")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)

# Use GNUInstallDirs to allow customisation
# but set our own default data and bin dir
if(NOT CMAKE_INSTALL_DATADIR)
    set(CMAKE_INSTALL_DATADIR "share/games")
endif()
if(NOT CMAKE_INSTALL_BINDIR)
    set(CMAKE_INSTALL_BINDIR "games")
endif()
include(GNUInstallDirs)

include(Options)
set_options()
set_directory_options()

include(Static)
set_static_if_needed()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

# An empty target for the tools
add_custom_target(tools)

include(CompileFlags)
compile_flags()

if(APPLE OR UNIX)
    add_definitions(-DUNIX)
endif()

if(UNIX)
    find_package(Doxygen)
endif()

list(APPEND GENERATED_SOURCE_FILES "${CMAKE_BINARY_DIR}/generated/rev.cpp")
if(WIN32)
    list(APPEND GENERATED_SOURCE_FILES "${CMAKE_BINARY_DIR}/generated/ottdres.rc")
endif()

# Generate a target to determine version, which is execute every 'make' run
add_custom_target(find_version
        ${CMAKE_COMMAND}
                -DFIND_VERSION_BINARY_DIR=${CMAKE_BINARY_DIR}/generated
                -DCPACK_BINARY_DIR=${CMAKE_BINARY_DIR}
                -DREV_MAJOR=${PROJECT_VERSION_MAJOR}
                -DREV_MINOR=${PROJECT_VERSION_MINOR}
                -DWINDOWS=${WIN32}
                -DDOXYGEN_WARN_FORMAT_LINE=${DOXYGEN_WARN_FORMAT_LINE}
                -DDOXYGEN_WARN_FILE=${DOXYGEN_WARN_FILE}
                -DDOXYGEN_GS_WARN_FILE=${DOXYGEN_GS_WARN_FILE}
                -DDOXYGEN_AI_WARN_FILE=${DOXYGEN_AI_WARN_FILE}
                -P "${CMAKE_SOURCE_DIR}/cmake/scripts/FindVersion.cmake"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        BYPRODUCTS ${GENERATED_SOURCE_FILES}
)

# Documentation
if(DOXYGEN_EXECUTABLE)
    add_custom_target(docs)
    add_custom_target(docs_source
        ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating documentation for source"
    )
    add_dependencies(docs_source
        find_version
    )
    add_dependencies(docs
        docs_source
    )
endif()

include(AddCustomXXXTimestamp)

if(OPTION_TOOLS_ONLY)
    if(HOST_BINARY_DIR)
        unset(HOST_BINARY_DIR CACHE)
    endif()
    add_subdirectory(${CMAKE_SOURCE_DIR}/src)
    return()
endif()

if(APPLE)
	# Avoid searching for headers in Frameworks, and libraries in LIBDIR.
	set(CMAKE_FIND_FRAMEWORK LAST)
endif()

# Prefer -pthread over -lpthread, which is often the better option of the two.
set(CMAKE_THREAD_PREFER_PTHREAD YES)
# Make sure we have Threads available.
find_package(Threads REQUIRED)

find_package(ZLIB)
find_package(LibLZMA)
find_package(LZO)
find_package(PNG)

if(WIN32 OR EMSCRIPTEN)
    # Windows uses WinHttp for HTTP requests.
    # Emscripten uses Javascript for HTTP requests.
else()
    # All other targets use libcurl.
    find_package(CURL)
endif()

# Breakpad doesn't support emscripten.
if(NOT EMSCRIPTEN)
    find_package(unofficial-breakpad NO_MODULE)
endif()

if(NOT OPTION_DEDICATED)
    if(WIN32)
        find_package(Xaudio2)
    else()
        find_package(Allegro)
        if(NOT APPLE)
            find_package(Freetype)
            find_package(SDL2)
            find_package(Fluidsynth)
            if(Freetype_FOUND)
                find_package(Fontconfig)
            endif()
            find_package(Harfbuzz)
            find_package(ICU OPTIONAL_COMPONENTS i18n uc)
        endif()
    endif()
    find_package(OpusFile)
    find_package(Soxr)
endif()
if(APPLE)
    enable_language(OBJCXX)

    find_package(Iconv)

    find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)
    find_library(AUDIOUNIT_LIBRARY AudioUnit)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(QUARTZCORE_LIBRARY QuartzCore)
endif()

if(NOT EMSCRIPTEN AND NOT OPTION_DEDICATED)
    find_package(OpenGL COMPONENTS OpenGL)
endif()

if(MSVC)
    find_package(Editbin REQUIRED)
endif()

find_package(SSE)

find_package(Grfcodec)

include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_FOUND)

show_options()

if(UNIX AND NOT APPLE AND NOT OPTION_DEDICATED)
    if(NOT SDL2_FOUND AND NOT ALLEGRO_FOUND)
        message(FATAL_ERROR "SDL2 or Allegro is required for this platform")
    endif()
    if(HARFBUZZ_FOUND AND NOT ICU_I18N_FOUND)
        message(WARNING "HarfBuzz depends on ICU i18n to function; HarfBuzz will be disabled")
    endif()
    if(NOT HARFBUZZ_FOUND)
        message(WARNING "Without HarfBuzz and ICU i18n the game will not be able to render right-to-left languages correctly")
    endif()
endif()
if(APPLE)
    if(NOT AUDIOTOOLBOX_LIBRARY)
        message(FATAL_ERROR "AudioToolbox is required for this platform")
    endif()
    if(NOT AUDIOUNIT_LIBRARY)
        message(FATAL_ERROR "AudioUnit is required for this platform")
    endif()
    if(NOT COCOA_LIBRARY)
        message(FATAL_ERROR "Cocoa is required for this platform")
    endif()
    if(NOT QUARTZCORE_LIBRARY)
        message(FATAL_ERROR "QuartzCore is required for this platform")
    endif()
endif()

if(OPTION_PACKAGE_DEPENDENCIES)
    if(NOT UNIX)
        message(FATAL_ERROR "Can only package dependencies on Linux")
    endif()
    if(OPTION_INSTALL_FHS)
        message(FATAL_ERROR "Cannot install in FHS folders when we are packaging dependencies")
    endif()
    if(${CMAKE_VERSION} VERSION_LESS "3.16.0")
        message(FATAL_ERROR "OPTION_PACKAGE_DEPENDENCIES can only work with CMake 3.16+; you are using ${CMAKE_VERSION}")
    endif()

    # If we are packaging dependencies, we do two things:
    # 1) set the RPATH to include $ORIGIN/lib; $ORIGIN (that literal string)
    #    is a Linux indicator for "path where application is". In CMake, we
    #    have to do this before add_executable() is executed.
    # 2) copy the libraries that we compile against to the "lib" folder.
    #    This is done in InstallAndPackage.cmake.
    set(CMAKE_INSTALL_RPATH "\$ORIGIN/lib")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
endif()

include(CTest)
include(SourceList)

# Needed by everything that uses Squirrel
include_directories(${CMAKE_SOURCE_DIR}/src/3rdparty/squirrel/include)

include(MSVCFilters)

add_library(openttd_lib OBJECT ${GENERATED_SOURCE_FILES})
add_executable(openttd WIN32)
add_executable(openttd_test)
# Focused native order/persistence tests link the real engine once; shared-header changes must not require unrelated scenario compilation. Owner: r2mlvs6-mines-and-resource-chains.md Group13CCC.
add_executable(openttd_surface_supply_test EXCLUDE_FROM_ALL
    src/tests/test_main.cpp
    src/tests/test_artifact_paths.cpp
    src/tests/mock_spritecache.cpp
    src/tests/multilayer_bootstrap.cpp
    src/tests/surface_supply.cpp
    src/tests/surface_supply_custody.cpp
    src/tests/surface_supply_order.cpp
    # Exact native/legacy receipt provenance: archi-map-layers.md; m2o23-1-come-space.md Group10.
    src/tests/surface_supply_receipts.cpp
    src/tests/surface_supply_conversion.cpp
    src/tests/surface_supply_packet_evidence.cpp
    src/tests/surface_supply_departure.cpp
    src/tests/surface_supply_refund_stock.cpp
    src/tests/cargo_custody.cpp
    src/tests/surface_supply_custody_policy.cpp
    src/tests/surface_supply_adoption.cpp
    src/tests/surface_supply_space_custody.cpp
    src/tests/surface_supply_window.cpp
    src/tests/surface_supply_refund.cpp
    src/tests/surface_supply_legacy_lifecycle.cpp
    src/tests/cargo_partial_transfer.cpp
    src/tests/surface_supply_cross_owner.cpp
    # Unified exact tube admission: archi-map-layers.md; m2o23-1-come-space.md Group18.
    src/tests/transport_tube_interchange.cpp
    src/tests/transport_tube_interchange_delivery.cpp
    src/tests/surface_supply_order_owner.cpp
    src/tests/surface_supply_order_wire.cpp
    src/tests/surface_supply_order_native.cpp
    # Actual conventional three-plane bought-material transport: r2mlvs6 Group13CD.
    src/tests/surface_supply_delivery.cpp
    # Authentic ordinary Surface starting-world contracts: r2mlvs11 Group15BB.
    src/tests/surface_entry_native.cpp
    # Native bank cash/debt for genuine factory funding: r2mlvs11 Group15BC.
    src/tests/surface_entry_credit.cpp
    # Paid original factories and read-only readiness: r2mlvs11 Group15BC.
    src/tests/surface_entry_factories.cpp
    # Genuine Surface freight infrastructure: r2mlvs11 Group15BD.
    src/tests/surface_entry_roads.cpp
    # Actual conventional empty purchases/orders: r2mlvs11 Group15BD.
    src/tests/surface_entry_fleet.cpp
    # Exact native game-tick replay of actual products: r2mlvs11 Group15BD.
    src/tests/surface_entry_production.cpp
    # Pure genuine Salt/portal/spoil characterization: r2mlvs11 Group15BE.
    src/tests/surface_entry_salt.cpp
    # Actual joint Surface spoil/rail/portal access: r2mlvs11 Group15BE.
    src/tests/surface_entry_access.cpp
    # Genuine whole Salt mine site and retained cutter: r2mlvs11 Group15BF.
    src/tests/surface_entry_mine.cpp
    # Genuine Salt mine capital and matching bank debt: r2mlvs11 Group15BG.
    src/tests/surface_entry_salt_mine.cpp
    # Pure genuine chemical production/transport/capital terms: r2mlvs11 Group15BH.
    src/tests/surface_entry_chemical.cpp
    # Genuine Propellant capital and original freight income: r2mlvs11 Group15BI.
    src/tests/surface_entry_fuel.cpp
    src/tests/surface_entry_income.cpp
    # Genuine second Chemical works and physical Propellant: r2mlvs11 Group15BID.
    src/tests/surface_entry_propellant.cpp
    # Actual paid launch-site/material/capital query: r2mlvs11 Group15BIE.
    src/tests/surface_entry_launch.cpp
    # Original continuing freight replacement: r2mlvs11 Group15BIEE.
    src/tests/surface_entry_renewal.cpp
    src/tests/surface_entry_goods.cpp
    src/tests/surface_entry_resupply.cpp
    src/tests/surface_entry_fuel_refill.cpp
    src/tests/surface_entry_gateway.cpp
    src/tests/surface_entry_bus_renewal.cpp
    src/tests/surface_entry_road_renewal.cpp
    # Actual regional catchments/paid roads/passenger profits: r2mlvs11 Group15BIEQA.
    src/tests/surface_entry_regional.cpp
    # Actual Salt access producer boundaries: r2mlvs11 Group15BE.
    src/tests/tbm_acquisition.cpp
    src/tests/portal_bootstrap.cpp
    src/tests/tbm_commission.cpp
    src/tests/spoil_receiver_definition.cpp
    src/tests/spoil_receiver_intake.cpp
    src/tests/spoil_receiver_delivery.cpp
    src/tests/spoil_receiver_accounting.cpp
    src/tests/spoil_receiver_leg.cpp
    # Actual mine admission and paid cutter reuse gates: r2mlvs11 Group15BF.
    src/tests/layer_industry_admission.cpp
    src/tests/layer_mine_definition.cpp
    src/tests/layer_mine_production.cpp
    src/tests/layer_mine_dispatch.cpp
    src/tests/tbm_depot_workflow.cpp
    # The reuse gate drives the shared original workflow owner, not a copied rig.
    src/tests/underground_workflow.cpp
    # Variable native Underground station admission: r2mlvs11 Group15BF.3.
    src/tests/conventional_station_art.cpp
)
# The current original financial applications remain immutable; this separate
# admission executable queries the same real engine without relinking them.
add_executable(openttd_surface_capital_test EXCLUDE_FROM_ALL
    src/tests/test_main.cpp
    src/tests/test_artifact_paths.cpp
    src/tests/mock_spritecache.cpp
    src/tests/multilayer_bootstrap.cpp
    src/tests/surface_entry_productive_admission.cpp
)
# Requalify actual command-funded processor geometry/status without relinking
# existing applications or compiling unrelated scenario fixtures. Plan14 Group41B.
add_executable(openttd_processor_contract_test EXCLUDE_FROM_ALL
    src/tests/test_main.cpp
    src/tests/test_artifact_paths.cpp
    src/tests/mock_spritecache.cpp
    src/tests/multilayer_bootstrap.cpp
    src/tests/layer_processor_definition.cpp
    src/tests/layer_processor_presentation.cpp
    src/tests/layer_processor.cpp
)
# Link the same engine objects for the current Space boundaries without recompiling unrelated test scenarios after a shared sprite-range change. Explicit targeting keeps normal build/test entry points intact.
add_executable(openttd_ground_art_test EXCLUDE_FROM_ALL
    src/tests/test_main.cpp
    src/tests/test_artifact_paths.cpp
    src/tests/mock_spritecache.cpp
    src/tests/ground_packed_load.cpp
)
# Keep completed packing observers immutable while current consumer cases
# compile against the same actual engine and source-backed typed pose handoff.
add_executable(openttd_ground_consumer_test EXCLUDE_FROM_ALL
    src/tests/test_main.cpp
    src/tests/test_artifact_paths.cpp
    src/tests/mock_spritecache.cpp
    src/tests/multilayer_bootstrap.cpp
    src/tests/ground_sprite_selection.cpp
    # Current actual mine geometry/status owners qualify selected source binding first: Plan14 Group79FC.
    src/tests/layer_mine_definition.cpp
    src/tests/layer_mine_presentation.cpp
    src/tests/ground_mine_selection.cpp
    # Requalify actual four-processor geometry/status before selected binding: Plan14 Group79FD.
    src/tests/layer_processor_definition.cpp
    src/tests/layer_processor_presentation.cpp
    src/tests/layer_processor.cpp
    src/tests/ground_processor_selection.cpp
    # Qualify current conventional EIDS/capability ownership before rail art: Plan14 Group79FE.
    src/tests/underground_engine.cpp
    src/tests/underground_filter.cpp
    src/tests/ground_train_selection.cpp
    src/tests/multilayer_train_art.cpp
    # Actual conventional road source and phase/custody boundaries: Plan14 Group79FF.
    src/tests/ground_road_selection.cpp
    # Requalify native station/terminal parent geometry before dependent source binding: Plan14 Group79FG.
    src/tests/conventional_station_art.cpp
    src/tests/conventional_terminal_art.cpp
    src/tests/ground_station_selection.cpp
)
# Existing applications remain untouched; this separate target observes the
# same actual engine/GRF cache after the selected underground range append.
# Actual application/viewports remain separate from retained source-selection
# images and original games: Plan14 Group79FHB and its following bounded proofs.
add_executable(openttd_ground_viewport_test EXCLUDE_FROM_ALL
    src/tests/test_main.cpp
    src/tests/test_artifact_paths.cpp
    src/tests/mock_spritecache.cpp
    src/tests/ground_industry_runtime.cpp
    # Actual conventional map/list/purchase and native station viewports: Plan14 Group79FHC.
    src/tests/ground_transport_runtime.cpp
    # Separate station/terminal foreground observer preserves completed transport source epochs: Plan14 Group79FHCC.
    src/tests/ground_station_runtime.cpp
    # Corrected facility packing proves the complete source/prefix through a fresh actual loader: Plan14 Group79FHCCB.
    src/tests/ground_station_offset_load.cpp
)
# Reuse the real native engine while retaining every completed application image.
# Provider checks run before adding operating terminal/interchange consumers.
add_executable(openttd_tube_test EXCLUDE_FROM_ALL
    src/tests/test_main.cpp
    src/tests/tube_test_environment.cpp
    src/tests/test_artifact_paths.cpp
    src/tests/mock_spritecache.cpp
    src/tests/multilayer_bootstrap.cpp
    src/tests/transport_tube_model.cpp
    # Read-only native TBM merge prerequisites: archi-map-layers.md; m2o23-1-come-space.md Group20.
    src/tests/tbm_merge_observation.cpp
    src/tests/tbm_face_stand.cpp
    src/tests/tbm_face_meeting.cpp
    src/tests/tbm_face_deep.cpp
    src/tests/tbm_face_preflight.cpp
    src/tests/tbm_face_transfer.cpp
    src/tests/tbm_face_turn.cpp
    src/tests/tbm_face_steering.cpp
    src/tests/tbm_face_access.cpp
    src/tests/tbm_face_cycle.cpp
    # Native installed phase activation: archi-map-layers.md; m2o23-2-come-space.md Group23.
    src/tests/tbm_face_controller.cpp
    src/tests/portal_signal_boundary.cpp
    src/tests/transport_tube_construction.cpp
    src/tests/transport_tube_readiness.cpp
    src/tests/transport_tube_meter.cpp
    src/tests/transport_tube_interchange_geometry.cpp
    src/tests/transport_tube_interchange.cpp
    src/tests/transport_tube_interchange_delivery.cpp
)
add_executable(openttd_space_art_test EXCLUDE_FROM_ALL
    src/tests/test_main.cpp
    src/tests/test_artifact_paths.cpp
    src/tests/mock_spritecache.cpp
    src/tests/space_sprite.cpp
    # Original orbital source selectors and paid-work owner boundaries: r2mlvs11 Group13K.
    src/tests/launch_fleet_sprite.cpp
    # Physical per-cell facility source/owner bounds: r2mlvs14 Group62JB.
    src/tests/launch_facility_sprite.cpp
    src/tests/launch_facility_render.cpp
    # Actual retained paid-world review, without component poses: r2mlvs14 Group62JD.
    src/tests/launch_fleet_paid_review.cpp
    # Exact actual map/list window dispatch: r2mlvs14 Group62JDB.
    src/tests/launch_craft_window.cpp
    # Passive shell/packet recovery and real UI selection: r2mlvs14 Group62JDC.
    src/tests/launch_pod_review.cpp
    # Current original geometry, phase and custody producers: r2mlvs14 Group62JC.
    src/tests/launch_site.cpp
    src/tests/launch_production.cpp
    src/tests/space_logistics.cpp
    src/tests/launch_flight.cpp
    src/tests/space_descent.cpp
    src/tests/orbital_sprite_craft.cpp
    src/tests/orbital_sprite_module.cpp
    src/tests/orbital_recipe_inventory.cpp
    src/tests/orbital_dock.cpp
    src/tests/manufacturing_materialization.cpp
    src/tests/multilayer_packed_load.cpp
)
set_target_properties(openttd PROPERTIES OUTPUT_NAME "${BINARY_NAME}")
# All other files are added via target_sources()

if(MSVC)
    # The complete network command serialization table exceeds COFF's section limit in Debug.
    set_property(SOURCE src/network/network_command.cpp APPEND PROPERTY COMPILE_OPTIONS "$<$<CONFIG:Debug>:/bigobj>")

    # Add DPI manifest to project; other WIN32 targets get this via ottdres.rc
    target_sources(openttd PRIVATE "${CMAKE_SOURCE_DIR}/os/windows/openttd.manifest")

    # If target -static is used, switch our project to static (/MT) too.
    # If the target ends on -static-md, it will remain dynamic (/MD).
    if(VCPKG_TARGET_TRIPLET MATCHES "-static" AND NOT VCPKG_TARGET_TRIPLET MATCHES "-md")
        set_property(TARGET openttd_lib PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set_property(TARGET openttd PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set_property(TARGET openttd_test PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set_property(TARGET openttd_surface_supply_test PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set_property(TARGET openttd_surface_capital_test PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set_property(TARGET openttd_processor_contract_test PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set_property(TARGET openttd_ground_art_test PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set_property(TARGET openttd_ground_consumer_test PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set_property(TARGET openttd_ground_viewport_test PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set_property(TARGET openttd_tube_test PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set_property(TARGET openttd_space_art_test PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
     endif()
endif()

target_precompile_headers(openttd_lib
    PRIVATE
    src/stdafx.h
    src/core/format.hpp
)
set_source_files_properties(src/3rdparty/fmt/format.cc PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

add_subdirectory(${CMAKE_SOURCE_DIR}/bin)
add_subdirectory(${CMAKE_SOURCE_DIR}/src)
add_subdirectory(${CMAKE_SOURCE_DIR}/media)

add_dependencies(openttd
    find_version)

target_link_libraries(openttd_lib
    openttd::languages
    openttd::settings
    openttd::script_api
    Threads::Threads
)

target_link_libraries(openttd
    openttd_lib
    openttd::media
    openttd::basesets
)

target_link_libraries(openttd_test PRIVATE openttd_lib)
target_link_libraries(openttd_surface_supply_test PRIVATE openttd_lib)
target_link_libraries(openttd_surface_capital_test PRIVATE openttd_lib)
target_link_libraries(openttd_processor_contract_test PRIVATE openttd_lib)

target_link_libraries(openttd_ground_art_test PRIVATE openttd_lib)

target_link_libraries(openttd_ground_consumer_test PRIVATE openttd_lib)
target_link_libraries(openttd_ground_viewport_test PRIVATE openttd_lib)
target_link_libraries(openttd_tube_test PRIVATE openttd_lib)
target_link_libraries(openttd_space_art_test PRIVATE openttd_lib)
if(ANDROID)
    target_link_libraries(openttd_test PRIVATE log)
    target_link_libraries(openttd_surface_supply_test PRIVATE log)
    target_link_libraries(openttd_surface_capital_test PRIVATE log)
    target_link_libraries(openttd_processor_contract_test PRIVATE log)
    target_link_libraries(openttd_ground_art_test PRIVATE log)
    target_link_libraries(openttd_ground_consumer_test PRIVATE log)
    target_link_libraries(openttd_ground_viewport_test PRIVATE log)
    target_link_libraries(openttd_tube_test PRIVATE log)
    target_link_libraries(openttd_space_art_test PRIVATE log)
endif()

include(Catch)
catch_discover_tests(openttd_test)

if(HAIKU)
    target_link_libraries(openttd_lib "be" "network" "midi")
endif()

if(IPO_FOUND)
    set_target_properties(openttd PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE True)
    set_target_properties(openttd PROPERTIES INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL True)
    set_target_properties(openttd PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO True)
endif()
set_target_properties(openttd PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

# Windows multi-configuration builds keep executable snapshots in separate directories.
# Pair their compiled language packs so rebuilding tests cannot strand an older playable binary with newer shared packs.
if(WIN32 AND CMAKE_CONFIGURATION_TYPES)
    add_custom_command(TARGET openttd POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_BINARY_DIR}/lang" "$<TARGET_FILE_DIR:openttd>/lang"
        COMMENT "Pairing language packs with the playable executable"
        VERBATIM
    )
endif()

include(LinkPackage)
link_package(PNG TARGET PNG::PNG ENCOURAGED)
link_package(ZLIB TARGET ZLIB::ZLIB ENCOURAGED)
link_package(LIBLZMA TARGET LibLZMA::LibLZMA ENCOURAGED)
link_package(LZO)

if(NOT WIN32 AND NOT EMSCRIPTEN)
    link_package(CURL ENCOURAGED)
    target_link_libraries(openttd_lib ${CMAKE_DL_LIBS})
endif()

if(NOT EMSCRIPTEN)
    link_package(unofficial-breakpad TARGET unofficial::breakpad::libbreakpad_client)
endif()

if(NOT OPTION_DEDICATED)
    link_package(Fluidsynth)
    link_package(SDL2 TARGET SDL2::SDL2)
    link_package(Allegro)
    link_package(FREETYPE TARGET Freetype::Freetype)
    link_package(Fontconfig TARGET Fontconfig::Fontconfig)
    link_package(Harfbuzz TARGET harfbuzz::harfbuzz)
    link_package(ICU_I18N TARGET ICU::i18n)
    link_package(ICU_UC TARGET ICU::uc)
    link_package(OpusFile TARGET OpusFile::opusfile)
    link_package(Soxr TARGET Soxr::soxr)

    if(SDL2_FOUND AND OPENGL_FOUND AND UNIX)
        # SDL2 dynamically loads OpenGL if needed, so do not link to OpenGL when
        # on Linux. For Windows, we need to link to OpenGL as we also have a win32
        # driver using it.
        add_definitions(-DWITH_OPENGL)
        message(STATUS "OpenGL found -- -DWITH_OPENGL -- (via SDL2)")
    else()
        link_package(OpenGL TARGET OpenGL::GL)
    endif()
endif()

include(3rdparty/llvm/CheckAtomic)

if(APPLE)
    link_package(Iconv TARGET Iconv::Iconv)

    target_link_libraries(openttd_lib
        ${AUDIOTOOLBOX_LIBRARY}
        ${AUDIOUNIT_LIBRARY}
        ${COCOA_LIBRARY}
        ${QUARTZCORE_LIBRARY}
    )

    add_definitions(
        -DWITH_COCOA
    )
endif()

if(EMSCRIPTEN)
    add_library(WASM::WASM INTERFACE IMPORTED)

    # Allow heap-growth, and start with a bigger memory size.
    target_link_libraries(WASM::WASM INTERFACE "-s ALLOW_MEMORY_GROWTH=1")
    target_link_libraries(WASM::WASM INTERFACE "-s INITIAL_MEMORY=33554432")
    target_link_libraries(WASM::WASM INTERFACE "-s DISABLE_EXCEPTION_CATCHING=0")
    target_link_libraries(WASM::WASM INTERFACE "-s WASM_BIGINT")
    add_definitions(-s DISABLE_EXCEPTION_CATCHING=0)

    # Export functions to Javascript.
    target_link_libraries(WASM::WASM INTERFACE "-s EXPORTED_FUNCTIONS='[\"_main\", \"_em_openttd_add_server\"]' -s EXPORTED_RUNTIME_METHODS='[\"cwrap\", \"HEAPU8\"]'")

    # Preload all the files we generate during build.
    # As we do not compile with FreeType / FontConfig, we also have no way to
    # render several languages (like Chinese, ..), so where do you draw the
    # line what languages to include and which not? In the end, especially as
    # the more languages you add the slower downloading becomes, we decided to
    # only ship the English language.
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_BINARY_DIR}/baseset@/baseset")
    # Browser worlds need the same pinned package as native installations before profile activation.
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_BINARY_DIR}/newgrf/multilayer_industry.grf@/newgrf/multilayer_industry.grf")
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_BINARY_DIR}/lang/english.lng@/lang/english.lng")
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_SOURCE_DIR}/bin/ai@/ai")
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_SOURCE_DIR}/bin/game@/game")
    # Documentation files for the in-game text file viewer
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_SOURCE_DIR}/README.md@/README.md")
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_SOURCE_DIR}/CREDITS.md@/CREDITS.md")
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_SOURCE_DIR}/CONTRIBUTING.md@/CONTRIBUTING.md")
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_SOURCE_DIR}/COPYING.md@/COPYING.md")
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_SOURCE_DIR}/known-bugs.md@/known-bugs.md")
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_SOURCE_DIR}/changelog.md@/changelog.md")
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_SOURCE_DIR}/docs/admin_network.md@/docs/admin_network.md")
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_SOURCE_DIR}/docs/debugging_desyncs.md@/docs/debugging_desyncs.md")
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_SOURCE_DIR}/docs/desync.md@/docs/desync.md")
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_SOURCE_DIR}/docs/directory_structure.md@/docs/directory_structure.md")
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_SOURCE_DIR}/docs/eints.md@/docs/eints.md")
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_SOURCE_DIR}/docs/fonts.md@/docs/fonts.md")
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_SOURCE_DIR}/docs/linkgraph.md@/docs/linkgraph.md")
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_SOURCE_DIR}/docs/logging_and_performance_metrics.md@/docs/logging_and_performance_metrics.md")
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_SOURCE_DIR}/docs/multiplayer.md@/docs/multiplayer.md")
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_SOURCE_DIR}/docs/savegame_format.md@/docs/savegame_format.md")
    target_link_libraries(WASM::WASM INTERFACE "--preload-file ${CMAKE_SOURCE_DIR}/docs/symbol_server.md@/docs/symbol_server.md")

    # We use IDBFS for persistent storage.
    target_link_libraries(WASM::WASM INTERFACE "-lidbfs.js")

    # Use custom pre-js and shell.html.
    target_link_libraries(WASM::WASM INTERFACE "--pre-js ${CMAKE_SOURCE_DIR}/os/emscripten/pre.js")
    target_link_libraries(WASM::WASM INTERFACE "--shell-file ${CMAKE_SOURCE_DIR}/os/emscripten/shell.html")

    # Build the .html (which builds the .js, .wasm, and .data too).
    set_target_properties(openttd PROPERTIES SUFFIX ".html")
    target_link_libraries(openttd WASM::WASM)
endif()

if(NOT PERSONAL_DIR STREQUAL "(not set)")
    add_definitions(
        -DWITH_PERSONAL_DIR
        -DPERSONAL_DIR="${PERSONAL_DIR}"
    )
endif()

if(NOT SHARED_DIR STREQUAL "(not set)")
    add_definitions(
        -DWITH_SHARED_DIR
        -DSHARED_DIR="${SHARED_DIR}"
    )
endif()

if(NOT GLOBAL_DIR STREQUAL "(not set)")
    add_definitions(
        -DGLOBAL_DATA_DIR="${GLOBAL_DIR}"
    )
endif()

link_package(SSE)

add_definitions_based_on_options()

if(WIN32)
    add_definitions(
        -DUNICODE
        -D_UNICODE
        -DWITH_UNISCRIBE
        -DPSAPI_VERSION=1
    )

    target_link_libraries(openttd_lib
        ws2_32
        winmm
        imm32
        usp10
        psapi
        winhttp
        bcrypt
    )
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_definitions(-DPOINTER_IS_64BIT)
endif()

enable_testing()

add_subdirectory(regression)

if(APPLE OR WIN32)
    find_package(Pandoc)
endif()

include(InstallAndPackage)
