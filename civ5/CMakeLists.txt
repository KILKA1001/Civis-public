cmake_minimum_required(VERSION 3.24)
project(civ5 LANGUAGES CXX)
set(CIV5_BNW_REFERENCE_EXE "" CACHE FILEPATH "Optional locally supplied reference executable; not distributed")
set(CIV5_BNW_SELECTED_SAVE "${CMAKE_CURRENT_SOURCE_DIR}/../build/selected-bnw-gandhi.Civ5Save"
    CACHE FILEPATH "Selected BNW save for actual-source graphics/UI tests")
set(CIV5_BNW_AI_GROWTH_SAVE "${CMAKE_CURRENT_SOURCE_DIR}/../build/selected-bnw-ai-growth-turn17.Civ5Save"
    CACHE FILEPATH "Real unmodded BNW save containing AI cities for numeric growth checks")
set(CIV5_ASSETS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../civ5-assets"
    CACHE PATH "Immutable extracted Civ5 sources for asset tests")
set(CIV5_BNW_TEST_UI_LANGUAGE "" CACHE STRING
    "Explicit installed Language Type for actual BNW UI tests (for example ru_RU)")

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Civ V engine requires a 64-bit target; 32-bit builds are unsupported.")
endif()

# Production targets have no JVM, Python, research or original-game dependency.
add_library(civ5_terrain STATIC
    src/terrain/height_addressing.cpp
    src/terrain/native_math.cpp
    src/terrain/world_layout.cpp
    src/terrain/terrain_source_grid.cpp
    src/terrain/terrain_source_rivers.cpp
    src/terrain/terrain_auxiliary.cpp
    src/terrain/terrain_auxiliary_owners.cpp
    src/terrain/terrain_marsh.cpp
    src/terrain/terrain_decal_updates.cpp
    src/terrain/terrain_decal_assets.cpp
    src/terrain/terrain_decal_geometry.cpp
    src/terrain/terrain_road_grid.cpp
    src/terrain/scene_command_queue.cpp
    src/terrain/coast_road_network.cpp
    src/terrain/scene_mask.cpp
    src/terrain/scene_mask_polygon.cpp
    src/terrain/scene_direction.cpp
    src/terrain/scene_polygon_decals.cpp
    src/terrain/scene_polygon_chunks.cpp
    src/terrain/scene_polygon_owner.cpp
    src/terrain/scene_polygon_plot_store.cpp
    src/terrain/plot_graphics_state.cpp
    src/terrain/view_bounds.cpp
    src/terrain/camera_frame.cpp
    src/terrain/overlay.cpp
    src/terrain/camera_controller.cpp
    src/terrain/land_membership.cpp
    src/terrain/land_styles.cpp
    src/terrain/height_owner.cpp
    src/terrain/worker_field.cpp
    src/terrain/terrain_pieces.cpp
    src/terrain/terrain_placement.cpp
    src/terrain/terrain_occurrences.cpp
    src/terrain/terrain_world.cpp
    src/terrain/worker_gates.cpp
    src/terrain/coast_curves.cpp
    src/terrain/coast_decal_paths.cpp
    src/terrain/coast_strip.cpp
    src/terrain/coast_assets.cpp
    src/terrain/coast_map.cpp
    src/terrain/coast_perturbation.cpp
    src/terrain/worker_geometry.cpp
    src/terrain/coast_raster.cpp
    src/terrain/river_raster.cpp
    src/terrain/river_network.cpp
    src/terrain/river_geometry.cpp
    src/terrain/river_triangles.cpp
    src/terrain/river_surface.cpp
    src/terrain/land_grid.cpp)
target_include_directories(civ5_terrain PUBLIC include)
target_compile_features(civ5_terrain PUBLIC cxx_std_20)
set_target_properties(civ5_terrain PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(civ5_terrain PUBLIC /fp:strict PRIVATE /W4)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    # Keep separately rounded float32 operations and signed zero in the ports.
    target_compile_options(civ5_terrain PUBLIC -fno-fast-math -ffp-contract=off PRIVATE -Wall -Wextra -Wpedantic)
else()
    message(FATAL_ERROR "Unvalidated floating-point compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# Raw, read-only resources are separate from terrain math and GPU objects.
add_library(civ5_assets STATIC src/assets/asset_source.cpp src/assets/dds.cpp src/assets/lean.cpp
    src/assets/ui_block_atlas.cpp)
target_link_libraries(civ5_terrain PUBLIC civ5_assets)
target_include_directories(civ5_assets PUBLIC include)
target_compile_features(civ5_assets PUBLIC cxx_std_20)
set_target_properties(civ5_assets PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(civ5_assets PUBLIC /utf-8 PRIVATE /W4)
else()
    target_compile_options(civ5_assets PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(civ5_materials STATIC src/materials/material_sources.cpp src/materials/material_assets.cpp src/materials/material_setup.cpp src/materials/material_world.cpp src/materials/material_cache.cpp src/materials/material_cache_passes.cpp src/materials/material_mesh.cpp src/materials/material_generation.cpp src/materials/material_normals.cpp src/materials/material_shadows.cpp src/materials/lighting_sources.cpp src/materials/lighting_mesh.cpp src/materials/land_material.cpp)
target_link_libraries(civ5_materials PUBLIC civ5_assets civ5_terrain)
target_sources(civ5_materials PRIVATE src/materials/material_cache_raster.cpp)
target_sources(civ5_materials PRIVATE src/materials/material_cache_acquisition.cpp src/materials/material_cache_free_lists.cpp)
target_sources(civ5_materials PRIVATE src/materials/material_cache_projection.cpp)
target_sources(civ5_materials PRIVATE src/materials/material_cache_ordering.cpp)
target_sources(civ5_materials PRIVATE src/materials/material_cache_lifecycle.cpp)
target_sources(civ5_materials PRIVATE src/materials/material_cache_preparation.cpp)
target_sources(civ5_materials PRIVATE src/materials/material_cache_jobs.cpp)
target_sources(civ5_materials PRIVATE src/materials/material_cache_geometry.cpp)

target_sources(civ5_materials PRIVATE src/materials/material_cache_rebuild.cpp)
target_sources(civ5_materials PRIVATE src/materials/material_geometry_pool.cpp)
target_sources(civ5_materials PRIVATE src/materials/terrain_decal_packet.cpp)
target_sources(civ5_materials PRIVATE src/materials/terrain_decal_store.cpp)
target_sources(civ5_materials PRIVATE src/materials/terrain_decal_draw.cpp)
target_sources(civ5_materials PRIVATE src/materials/shared_land_geometry.cpp)
target_sources(civ5_materials PRIVATE src/materials/material_packet_pool.cpp)
target_sources(civ5_materials PRIVATE src/materials/material_cache_scheduler.cpp)
target_sources(civ5_materials PRIVATE src/materials/material_cache_refresh.cpp)
target_sources(civ5_materials PRIVATE src/materials/material_cache_loading.cpp)
target_sources(civ5_materials PRIVATE src/materials/material_cache_startup.cpp)
target_sources(civ5_materials PRIVATE src/materials/material_cache_settings.cpp)
target_sources(civ5_materials PRIVATE src/materials/color_keys.cpp)
target_sources(civ5_materials PRIVATE src/materials/water_surface.cpp)
target_sources(civ5_materials PRIVATE src/materials/river_surface_draw.cpp)
target_sources(civ5_materials PRIVATE src/materials/water_sky.cpp)
target_sources(civ5_materials PRIVATE src/materials/water_animation.cpp)
target_sources(civ5_materials PRIVATE src/materials/water_current.cpp)
target_sources(civ5_materials PRIVATE src/materials/water_reflection.cpp)
target_sources(civ5_materials PRIVATE src/materials/terrain_frame.cpp)
target_sources(civ5_materials PRIVATE src/materials/distance_fog.cpp)
target_sources(civ5_materials PRIVATE src/materials/fog_of_war.cpp)
target_sources(civ5_materials PRIVATE src/materials/fog_of_war_frame.cpp)
set_target_properties(civ5_materials PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(civ5_materials PRIVATE /W4)
else()
    target_compile_options(civ5_materials PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(WIN32)
    # CPU receipt validation reads the actual fog owner's metadata without GL operations.
    add_library(civ5_bnw_fog_map_metadata STATIC
        src/graphics/gl_fog_of_war_map_metadata.cpp)
    target_include_directories(civ5_bnw_fog_map_metadata PUBLIC include)
    target_compile_features(civ5_bnw_fog_map_metadata PUBLIC cxx_std_20)
    set_target_properties(civ5_bnw_fog_map_metadata PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(civ5_bnw_fog_map_metadata PRIVATE /W4)
    else()
        target_compile_options(civ5_bnw_fog_map_metadata PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endif()
add_subdirectory(runtime)
if(WIN32)
    add_library(civ5_bnw_unit_visuals STATIC
        src/graphics/bnw_unit_visual_owner.cpp
        src/graphics/bnw_unit_art_scale_caller.cpp
        src/graphics/bnw_unit_fx_resource_chain.cpp)
    target_link_libraries(civ5_bnw_unit_visuals PUBLIC civis_runtime civ5_terrain)
    target_compile_features(civ5_bnw_unit_visuals PUBLIC cxx_std_20)
    set_target_properties(civ5_bnw_unit_visuals PROPERTIES CXX_EXTENSIONS OFF)
    add_library(civ5_scene_saved_fog STATIC src/scene/saved_fog_of_war.cpp)
    target_link_libraries(civ5_scene_saved_fog PUBLIC civis_runtime civ5_materials)
    target_compile_features(civ5_scene_saved_fog PUBLIC cxx_std_20)
    set_target_properties(civ5_scene_saved_fog PROPERTIES CXX_EXTENSIONS OFF)
    target_link_libraries(civis_session_snapshot PRIVATE civ5_scene_saved_fog)
    add_library(civ5_bnw_deferred_fog_client STATIC src/graphics/bnw_deferred_fog_client.cpp)
    target_link_libraries(civ5_bnw_deferred_fog_client PUBLIC civ5_scene_saved_fog)
    target_compile_features(civ5_bnw_deferred_fog_client PUBLIC cxx_std_20)
    set_target_properties(civ5_bnw_deferred_fog_client PROPERTIES CXX_EXTENSIONS OFF)
    if(BUILD_TESTING)
        add_executable(civ5_scene_saved_fog_tests tests/saved_fog_of_war_tests.cpp)
        target_link_libraries(civ5_scene_saved_fog_tests PRIVATE civ5_scene_saved_fog)
        add_test(NAME terrain.scene_saved_fog COMMAND civ5_scene_saved_fog_tests)
        add_executable(civ5_bnw_deferred_fog_tests tests/bnw_deferred_fog_tests.cpp)
        add_executable(civ5_bnw_pathfinder_cache_controls_tests tests/bnw_pathfinder_cache_controls_tests.cpp)
        target_link_libraries(civ5_bnw_pathfinder_cache_controls_tests PRIVATE civ5_bnw_deferred_fog_client)
        if(MINGW)
            target_link_options(civ5_bnw_pathfinder_cache_controls_tests PRIVATE -static)
        endif()
        target_link_libraries(civ5_bnw_deferred_fog_tests PRIVATE civ5_bnw_deferred_fog_client)
        if(MINGW)
            target_link_options(civ5_bnw_deferred_fog_tests PRIVATE -static)
        endif()
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../build/selected-bnw-gandhi.Civ5Save" AND
           EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
            add_test(NAME graphics.bnw_deferred_fog COMMAND civ5_bnw_deferred_fog_tests
                "${CMAKE_CURRENT_SOURCE_DIR}/../build/selected-bnw-gandhi.Civ5Save"
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_pathfinder_cache_controls COMMAND civ5_bnw_pathfinder_cache_controls_tests
                "${CMAKE_CURRENT_SOURCE_DIR}/../build/selected-bnw-gandhi.Civ5Save"
                "${CIV5_BNW_INSTALL_ROOT}")
        endif()
    endif()
    add_library(civ5_scene_saved_sources STATIC src/scene/saved_scene_sources.cpp)
    target_link_libraries(civ5_scene_saved_sources PUBLIC civ5_scene_saved_fog civ5_terrain)
    target_compile_features(civ5_scene_saved_sources PUBLIC cxx_std_20)
    set_target_properties(civ5_scene_saved_sources PROPERTIES CXX_EXTENSIONS OFF)
    target_link_libraries(civis_saved_rules PRIVATE civ5_scene_saved_sources)
    if(BUILD_TESTING)
        add_executable(civ5_scene_saved_sources_tests tests/saved_scene_sources_tests.cpp)
        target_link_libraries(civ5_scene_saved_sources_tests PRIVATE civ5_scene_saved_sources)
        add_test(NAME terrain.scene_saved_sources COMMAND civ5_scene_saved_sources_tests)
    endif()
    add_library(civ5_scene_landmark_registry STATIC src/content/landmark_art_registry.cpp
        src/content/saved_landmark_art.cpp)
    target_link_libraries(civ5_scene_landmark_registry PUBLIC civis_runtime civ5_terrain)
    target_compile_features(civ5_scene_landmark_registry PUBLIC cxx_std_20)
    set_target_properties(civ5_scene_landmark_registry PROPERTIES CXX_EXTENSIONS OFF)
    add_executable(civ5_saved_landmark_art validation/saved_landmark_art.cpp)
    target_link_libraries(civ5_saved_landmark_art PRIVATE civ5_scene_landmark_registry)
    if(MINGW)
        target_link_options(civ5_saved_landmark_art PRIVATE -municode)
    endif()
    if(MSVC)
        target_compile_options(civ5_scene_landmark_registry PRIVATE /W4)
    else()
        target_compile_options(civ5_scene_landmark_registry PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    if(BUILD_TESTING)
        add_executable(civ5_scene_landmark_registry_tests tests/scene_landmark_registry_tests.cpp)
        target_link_libraries(civ5_scene_landmark_registry_tests PRIVATE civ5_scene_landmark_registry)
        add_test(NAME terrain.scene_landmark_registry COMMAND civ5_scene_landmark_registry_tests)
    endif()
    add_library(civ5_gl STATIC src/graphics/gl_context.cpp src/graphics/gl_texture.cpp src/graphics/gl_material_cache.cpp src/graphics/gl_land_height.cpp src/graphics/gl_material_textures.cpp src/graphics/gl_material_blend.cpp src/graphics/gl_material_normals.cpp src/graphics/gl_material_shadows.cpp src/graphics/gl_lighting_cubes.cpp src/graphics/gl_material_lighting.cpp src/graphics/gl_material_lighting_shaders.cpp src/graphics/gl_land_material.cpp src/graphics/gl_land_material_shaders.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_land_geometry_pool.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_bnw_normal_resource_gpu_image_loan.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/bnw_original_vertex_declaration_gl_schema.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/bnw_original_vertex_declaration_device_result_loan.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_land_patch_resources.cpp)
    target_include_directories(civ5_gl PUBLIC include)
    target_sources(civ5_gl PRIVATE src/graphics/gl_material_cache_raster.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_material_cache_renderer.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_material_decal.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_material_cache_pool.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_material_world.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_color_keys.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_distance_fog.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_fog_of_war.cpp src/graphics/gl_fog_of_war_shaders.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_fog_of_war_map.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_water_surface.cpp src/graphics/gl_water_surface_shaders.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_river_surface.cpp src/graphics/gl_river_surface_shaders.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_river_world.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_water_world.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_water_current.cpp src/graphics/gl_water_current_shaders.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_water_current_map.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_water_reflection_map.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_bnw_world_anchor_scene.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_bnw_natural_wonder_anchor.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/gl_bnw_system_font_atlas.cpp src/graphics/gl_bnw_font_shader.cpp)
    target_sources(civ5_gl PRIVATE
        src/graphics/gl_bnw_nonfont_native_record_projection.cpp
        src/graphics/gl_bnw_nonfont_pipeline.cpp
        src/graphics/gl_bnw_nonfont_native_constant_upload_projection.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/bnw_extended_label_glyph_geometry.cpp)
    target_sources(civ5_gl PRIVATE src/graphics/bnw_extended_label_glyph_cursor.cpp)
    target_link_libraries(civ5_gl PUBLIC civ5_assets civ5_terrain civ5_materials
        civ5_bnw_fog_map_metadata PRIVATE opengl32 gdi32 user32)
    set_target_properties(civ5_gl PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(civ5_gl PRIVATE /W4)
    else()
        target_compile_options(civ5_gl PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    if(BUILD_TESTING)
        add_executable(civ5_gl_bnw_world_anchor_scene_tests
            tests/gl_bnw_world_anchor_scene_tests.cpp)
        target_link_libraries(civ5_gl_bnw_world_anchor_scene_tests
            PRIVATE civ5_gl civis_runtime)
        if(MINGW)
            target_link_options(civ5_gl_bnw_world_anchor_scene_tests PRIVATE -static)
        endif()
        add_executable(civ5_gl_bnw_natural_wonder_anchor_tests
            tests/gl_bnw_natural_wonder_anchor_tests.cpp
            validation/ui_screen_gl.cpp)
        target_include_directories(civ5_gl_bnw_natural_wonder_anchor_tests
            PRIVATE validation)
        target_link_libraries(civ5_gl_bnw_natural_wonder_anchor_tests
            PRIVATE civ5_gl civis_runtime)
        if(MINGW)
            target_link_options(civ5_gl_bnw_natural_wonder_anchor_tests PRIVATE -static)
        endif()
        add_executable(civ5_graphics_gandhi_live_ice_tests
            tests/graphics_gandhi_live_ice_tests.cpp)
        target_link_libraries(civ5_graphics_gandhi_live_ice_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civ5_graphics_gandhi_live_ice_tests PRIVATE -static)
        endif()
        add_test(NAME graphics.gandhi_live_ice
            COMMAND civ5_graphics_gandhi_live_ice_tests)
        add_executable(civ5_graphics_bnw_warrior_binding_tests
            tests/graphics_bnw_warrior_binding_tests.cpp)
        target_link_libraries(civ5_graphics_bnw_warrior_binding_tests
            PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_binding_tests PRIVATE -static)
        endif()
        add_test(NAME graphics.bnw_warrior_binding
            COMMAND civ5_graphics_bnw_warrior_binding_tests)
        add_executable(civ5_graphics_bnw_warrior_geometry_cache_tests
            tests/graphics_bnw_warrior_geometry_cache_tests.cpp)
        target_link_libraries(civ5_graphics_bnw_warrior_geometry_cache_tests
            PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_geometry_cache_tests PRIVATE -static)
        endif()
        add_executable(civ5_graphics_bnw_warrior_skin_tests
            tests/graphics_bnw_warrior_skin_tests.cpp)
        add_executable(civ5_graphics_bnw_native_unit_identity_tests
            tests/graphics_bnw_native_unit_identity_tests.cpp)
        add_executable(civ5_graphics_bnw_warrior_authored_formation_tests
            tests/graphics_bnw_warrior_authored_formation_tests.cpp)
        target_link_libraries(civ5_graphics_bnw_warrior_authored_formation_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_authored_formation_tests PRIVATE -static)
        endif()
        add_test(NAME graphics.bnw_warrior_authored_formation COMMAND civ5_graphics_bnw_warrior_authored_formation_tests)
        add_executable(civ5_graphics_bnw_unit_member_transform_rules_tests
            tests/graphics_bnw_unit_member_transform_rules_tests.cpp)
        add_executable(civ5_graphics_bnw_base_unit_member_info_compilation_tests
            tests/graphics_bnw_base_unit_member_info_compilation_tests.cpp)
        add_executable(civ5_graphics_bnw_base_unit_art_compilation_tests
            tests/graphics_bnw_base_unit_art_compilation_tests.cpp)
        target_link_libraries(civ5_graphics_bnw_base_unit_art_compilation_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civ5_graphics_bnw_base_unit_art_compilation_tests PRIVATE -static)
        endif()
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(civ5_graphics_bnw_base_unit_art_compilation_tests PRIVATE
                -fno-fast-math -ffp-contract=off)
        endif()
        set(CIV5_BNW_UNIT_ART_COMPILATION_FIXTURE "" CACHE FILEPATH
            "Locally generated five-table original unit art SQL binder fixture")
        if(EXISTS "${CIV5_BNW_UNIT_ART_COMPILATION_FIXTURE}")
            add_test(NAME graphics.bnw_base_unit_art_compilation
                COMMAND civ5_graphics_bnw_base_unit_art_compilation_tests
                "${CIV5_ASSETS_ROOT}" "${CIV5_BNW_INSTALL_ROOT}" "${CIV5_BNW_UNIT_ART_COMPILATION_FIXTURE}")
        endif()
        foreach(_model_source IN ITEMS request manager_initializer source_prerequisites)
            if(_model_source STREQUAL "request")
                set(_model_test_source graphics_bnw_unit_model_source_request_tests.cpp)
            elseif(_model_source STREQUAL "manager_initializer")
                set(_model_test_source graphics_bnw_unit_model_manager_initializer_tests.cpp)
            else()
                set(_model_test_source graphics_bnw_unit_model_source_prerequisites_tests.cpp)
            endif()
            add_executable(civ5_graphics_bnw_unit_model_${_model_source}_tests
                tests/${_model_test_source})
            target_compile_features(civ5_graphics_bnw_unit_model_${_model_source}_tests PRIVATE cxx_std_20)
            if(MINGW)
                target_link_options(civ5_graphics_bnw_unit_model_${_model_source}_tests PRIVATE -static)
            endif()
            set(CIV5_BNW_UNIT_MODEL_${_model_source}_FIXTURE "" CACHE FILEPATH
                "Locally generated original model ${_model_source} source fixture")
            if(EXISTS "${CIV5_BNW_UNIT_MODEL_${_model_source}_FIXTURE}")
                add_test(NAME graphics.bnw_unit_model_${_model_source}
                    COMMAND civ5_graphics_bnw_unit_model_${_model_source}_tests
                    "${CIV5_BNW_UNIT_MODEL_${_model_source}_FIXTURE}")
            endif()
        endforeach()
        foreach(_descriptor_test IN ITEMS gsd_v7_descriptor unit_model_kind0_lifetime small_object_allocator_source)
            add_executable(civ5_graphics_bnw_${_descriptor_test}_tests
                tests/graphics_bnw_${_descriptor_test}_tests.cpp)
            target_include_directories(civ5_graphics_bnw_${_descriptor_test}_tests PRIVATE include)
            target_compile_features(civ5_graphics_bnw_${_descriptor_test}_tests PRIVATE cxx_std_20)
            if(MINGW)
                target_link_options(civ5_graphics_bnw_${_descriptor_test}_tests PRIVATE -static)
            endif()
            set(CIV5_BNW_${_descriptor_test}_FIXTURE "" CACHE FILEPATH
                "Locally generated original ${_descriptor_test} source fixture")
        endforeach()
        target_sources(civ5_graphics_bnw_unit_model_kind0_lifetime_tests PRIVATE src/runtime/file_identity.cpp)
        target_link_libraries(civ5_graphics_bnw_unit_model_kind0_lifetime_tests PRIVATE bcrypt)
        target_sources(civ5_graphics_bnw_small_object_allocator_source_tests PRIVATE src/runtime/file_identity.cpp)
        target_link_libraries(civ5_graphics_bnw_small_object_allocator_source_tests PRIVATE bcrypt)
        if(EXISTS "${CIV5_BNW_small_object_allocator_source_FIXTURE}")
            add_test(NAME graphics.bnw_small_object_allocator_source
                COMMAND civ5_graphics_bnw_small_object_allocator_source_tests
                "${CIV5_BNW_small_object_allocator_source_FIXTURE}"
                "${CIV5_BNW_REFERENCE_EXE}"
                "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        endif()
        if(EXISTS "${CIV5_BNW_gsd_v7_descriptor_FIXTURE}")
            add_test(NAME graphics.bnw_gsd_v7_descriptor
                COMMAND civ5_graphics_bnw_gsd_v7_descriptor_tests
                "${CIV5_BNW_gsd_v7_descriptor_FIXTURE}" "${CIV5_ASSETS_ROOT}")
        endif()
        if(EXISTS "${CIV5_BNW_unit_model_kind0_lifetime_FIXTURE}")
            add_test(NAME graphics.bnw_unit_model_kind0_lifetime
                COMMAND civ5_graphics_bnw_unit_model_kind0_lifetime_tests
                "${CIV5_BNW_unit_model_kind0_lifetime_FIXTURE}" "${CIV5_ASSETS_ROOT}" "${CIV5_BNW_INSTALL_ROOT}")
        endif()
        add_executable(civ5_graphics_bnw_unit_art_scale_caller_tests
            tests/graphics_bnw_unit_art_scale_caller_tests.cpp)
        target_link_libraries(civ5_graphics_bnw_unit_art_scale_caller_tests PRIVATE civis_runtime $<LINK_ONLY:civ5_bnw_unit_visuals>)
        add_executable(civ5_graphics_bnw_unit_vis_ini_serializer_tests
            tests/graphics_bnw_unit_vis_ini_serializer_tests.cpp)
        target_include_directories(civ5_graphics_bnw_unit_vis_ini_serializer_tests PRIVATE include)
        target_compile_features(civ5_graphics_bnw_unit_vis_ini_serializer_tests PRIVATE cxx_std_20)
        add_executable(civ5_graphics_bnw_unit_fx_source_tests
            tests/graphics_bnw_unit_fx_source_tests.cpp)
        target_link_libraries(civ5_graphics_bnw_unit_fx_source_tests PRIVATE civis_runtime)
        foreach(_unit_source_test IN ITEMS art_scale_caller vis_ini_serializer fx_source)
            if(MINGW)
                target_link_options(civ5_graphics_bnw_unit_${_unit_source_test}_tests PRIVATE -static)
            endif()
            if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
                target_compile_options(civ5_graphics_bnw_unit_${_unit_source_test}_tests PRIVATE -fno-fast-math -ffp-contract=off)
            endif()
        endforeach()
        set(CIV5_BNW_UNIT_VIS_INI_SERIALIZER_FIXTURE "" CACHE FILEPATH
            "Locally generated bounded original UnitVis INI writer fixture")
        if(EXISTS "${CIV5_BNW_UNIT_VIS_INI_SERIALIZER_FIXTURE}")
            add_test(NAME graphics.bnw_unit_vis_ini_serializer
                COMMAND civ5_graphics_bnw_unit_vis_ini_serializer_tests "${CIV5_BNW_UNIT_VIS_INI_SERIALIZER_FIXTURE}")
        endif()
        set(CIV5_BNW_UNIT_FX_SOURCE_FIXTURE "" CACHE FILEPATH
            "Locally generated bounded original FXSXML file and EC traversal fixture")
        if(EXISTS "${CIV5_BNW_UNIT_FX_SOURCE_FIXTURE}")
            add_test(NAME graphics.bnw_unit_fx_source
                COMMAND civ5_graphics_bnw_unit_fx_source_tests "${CIV5_BNW_UNIT_FX_SOURCE_FIXTURE}")
        endif()
        # The actual desktop caller test is built here. Its process-bound user
        # directory and caps receipt are verified explicitly in the desktop gate.
        target_link_libraries(civ5_graphics_bnw_base_unit_member_info_compilation_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civ5_graphics_bnw_base_unit_member_info_compilation_tests PRIVATE -static)
        endif()
        if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/Assets/Units/Civ5ArtDefines_UnitMembers.xml")
            add_test(NAME graphics.bnw_base_unit_member_info_compilation
                COMMAND civ5_graphics_bnw_base_unit_member_info_compilation_tests
                "${CIV5_ASSETS_ROOT}" "${CIV5_BNW_INSTALL_ROOT}")
        endif()
        add_executable(civ5_graphics_bnw_warrior_gr2_native_basis_tests
            tests/graphics_bnw_warrior_gr2_native_basis_tests.cpp)
        target_include_directories(civ5_graphics_bnw_warrior_gr2_native_basis_tests PRIVATE include)
        add_executable(civ5_graphics_bnw_warrior_gr2_metadata_cache_tests
            tests/graphics_bnw_warrior_gr2_metadata_cache_tests.cpp)
        target_include_directories(civ5_graphics_bnw_warrior_gr2_metadata_cache_tests PRIVATE include validation)
        target_compile_features(civ5_graphics_bnw_warrior_gr2_metadata_cache_tests PRIVATE cxx_std_20)
        target_link_libraries(civ5_graphics_bnw_warrior_gr2_metadata_cache_tests PRIVATE civis_runtime bcrypt)
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_gr2_metadata_cache_tests PRIVATE -static)
        endif()
        target_compile_features(civ5_graphics_bnw_warrior_gr2_native_basis_tests PRIVATE cxx_std_20)
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_gr2_native_basis_tests PRIVATE -static)
        endif()
        target_link_libraries(civ5_graphics_bnw_unit_member_transform_rules_tests PRIVATE civ5_bnw_unit_visuals)
        target_compile_features(civ5_graphics_bnw_unit_member_transform_rules_tests PRIVATE cxx_std_20)
        if(MINGW)
            target_link_options(civ5_graphics_bnw_unit_member_transform_rules_tests PRIVATE -static)
        endif()
        add_test(NAME graphics.bnw_unit_member_transform_rules
            COMMAND civ5_graphics_bnw_unit_member_transform_rules_tests ${CIV5_ASSETS_ROOT})
        add_executable(civ5_graphics_bnw_unit_visual_owner_tests tests/graphics_bnw_unit_visual_owner_tests.cpp)
        target_link_libraries(civ5_graphics_bnw_unit_visual_owner_tests PRIVATE civ5_bnw_unit_visuals)
        if(MINGW)
            target_link_options(civ5_graphics_bnw_unit_visual_owner_tests PRIVATE -static)
        endif()
        add_test(NAME graphics.bnw_unit_visual_owner COMMAND civ5_graphics_bnw_unit_visual_owner_tests
            ${CIV5_ASSETS_ROOT})
        target_include_directories(civ5_graphics_bnw_native_unit_identity_tests PRIVATE include)
        target_compile_features(civ5_graphics_bnw_native_unit_identity_tests PRIVATE cxx_std_20)
        if(MINGW)
            target_link_options(civ5_graphics_bnw_native_unit_identity_tests PRIVATE -static)
        endif()
        add_test(NAME graphics.bnw_native_unit_identity COMMAND civ5_graphics_bnw_native_unit_identity_tests)
        target_link_libraries(civ5_graphics_bnw_warrior_skin_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_skin_tests PRIVATE -static)
        endif()
    endif()
endif()
if(BUILD_TESTING)
    add_executable(civ5_terrain_decal_store_tests tests/terrain_decal_store_tests.cpp)
    target_link_libraries(civ5_terrain_decal_store_tests PRIVATE civ5_materials)
    add_test(NAME materials.terrain_decal_store COMMAND civ5_terrain_decal_store_tests)
    add_executable(civ5_terrain_decal_domain_tests tests/terrain_decal_domain_tests.cpp)
    target_link_libraries(civ5_terrain_decal_domain_tests PRIVATE civ5_materials)
    add_test(NAME materials.terrain_decal_domain COMMAND civ5_terrain_decal_domain_tests)
    add_executable(civ5_terrain_decal_packet_tests tests/terrain_decal_packet_tests.cpp)
    target_link_libraries(civ5_terrain_decal_packet_tests PRIVATE civ5_materials)
    add_test(NAME materials.terrain_decal_packet COMMAND civ5_terrain_decal_packet_tests)
    add_executable(civ5_material_cache_refresh_tests tests/material_cache_refresh_tests.cpp)
    target_link_libraries(civ5_material_cache_refresh_tests PRIVATE civ5_materials)
    add_test(NAME materials.cache_refresh COMMAND civ5_material_cache_refresh_tests)
    add_executable(civ5_material_cache_geometry_oracle validation/material_cache_geometry_oracle.cpp)
    target_link_libraries(civ5_material_cache_geometry_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_cache_geometry_tests tests/material_cache_geometry_tests.cpp)
    target_link_libraries(civ5_material_cache_geometry_tests PRIVATE civ5_materials)
    add_test(NAME materials.cache_geometry COMMAND civ5_material_cache_geometry_tests)
    add_executable(civ5_material_cache_rebuild_oracle validation/material_cache_rebuild_oracle.cpp)
    target_link_libraries(civ5_material_cache_rebuild_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_cache_rebuild_tests tests/material_cache_rebuild_tests.cpp)
    target_link_libraries(civ5_material_cache_rebuild_tests PRIVATE civ5_materials)
    add_test(NAME materials.cache_rebuild COMMAND civ5_material_cache_rebuild_tests)
    add_executable(civ5_material_geometry_pool_oracle validation/material_geometry_pool_oracle.cpp)
    target_link_libraries(civ5_material_geometry_pool_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_geometry_pool_tests tests/material_geometry_pool_tests.cpp)
    target_link_libraries(civ5_material_geometry_pool_tests PRIVATE civ5_materials)
    add_test(NAME materials.geometry_pool COMMAND civ5_material_geometry_pool_tests)
    add_executable(civ5_shared_land_geometry_tests tests/shared_land_geometry_tests.cpp)
    target_link_libraries(civ5_shared_land_geometry_tests PRIVATE civ5_materials)
    add_test(NAME materials.shared_land_geometry COMMAND civ5_shared_land_geometry_tests)
    add_executable(civ5_material_packet_pool_oracle validation/material_packet_pool_oracle.cpp)
    target_link_libraries(civ5_material_packet_pool_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_packet_pool_tests tests/material_packet_pool_tests.cpp)
    target_link_libraries(civ5_material_packet_pool_tests PRIVATE civ5_materials)
    add_test(NAME materials.packet_pool COMMAND civ5_material_packet_pool_tests)
    add_executable(civ5_material_cache_jobs_oracle validation/material_cache_jobs_oracle.cpp)
    target_link_libraries(civ5_material_cache_jobs_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_cache_jobs_tests tests/material_cache_jobs_tests.cpp)
    target_link_libraries(civ5_material_cache_jobs_tests PRIVATE civ5_materials)
    add_test(NAME materials.cache_jobs COMMAND civ5_material_cache_jobs_tests)
    add_executable(civ5_material_cache_preparation_oracle validation/material_cache_preparation_oracle.cpp)
    target_link_libraries(civ5_material_cache_preparation_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_cache_preparation_tests tests/material_cache_preparation_tests.cpp)
    target_link_libraries(civ5_material_cache_preparation_tests PRIVATE civ5_materials)
    add_test(NAME materials.cache_preparation COMMAND civ5_material_cache_preparation_tests)
    add_executable(civ5_material_cache_lifecycle_oracle validation/material_cache_lifecycle_oracle.cpp)
    target_link_libraries(civ5_material_cache_lifecycle_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_cache_scheduler_oracle validation/material_cache_scheduler.cpp)
    target_link_libraries(civ5_material_cache_scheduler_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_cache_lifecycle_tests tests/material_cache_lifecycle_tests.cpp)
    target_link_libraries(civ5_material_cache_lifecycle_tests PRIVATE civ5_materials)
    add_test(NAME materials.cache_lifecycle COMMAND civ5_material_cache_lifecycle_tests)
    add_executable(civ5_material_cache_ordering_oracle validation/material_cache_ordering_oracle.cpp)
    target_link_libraries(civ5_material_cache_ordering_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_cache_ordering_tests tests/material_cache_ordering_tests.cpp)
    target_link_libraries(civ5_material_cache_ordering_tests PRIVATE civ5_materials)
    add_test(NAME materials.cache_ordering COMMAND civ5_material_cache_ordering_tests)
    add_executable(civ5_material_cache_projection_oracle validation/material_cache_projection_oracle.cpp)
    target_link_libraries(civ5_material_cache_projection_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_cache_acquisition_oracle validation/material_cache_acquisition_oracle.cpp)
    target_link_libraries(civ5_material_cache_acquisition_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_cache_pool_tests tests/material_cache_pool_tests.cpp)
    target_link_libraries(civ5_material_cache_pool_tests PRIVATE civ5_materials)
    add_test(NAME materials.cache_pool COMMAND civ5_material_cache_pool_tests)
    if(WIN32)
        add_executable(civ5_gl_material_cache_pool validation/gl_material_cache_pool.cpp)
        target_link_libraries(civ5_gl_material_cache_pool PRIVATE civ5_gl)
    endif()
    add_executable(civ5_river_draw_oracle validation/river_draw_oracle.cpp)
    target_link_libraries(civ5_river_draw_oracle PRIVATE civ5_materials)
    if(MINGW)
        target_link_options(civ5_river_draw_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_overlay_tests tests/overlay_tests.cpp)
    target_link_libraries(civ5_overlay_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.overlay COMMAND civ5_overlay_tests)
    add_executable(civ5_overlay_oracle validation/overlay_oracle.cpp)
    target_link_libraries(civ5_overlay_oracle PRIVATE civ5_terrain)
    if(MINGW)
        target_link_options(civ5_overlay_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_fog_of_war_frame_tests tests/fog_of_war_frame_tests.cpp)
    target_link_libraries(civ5_fog_of_war_frame_tests PRIVATE civ5_materials)
    add_test(NAME materials.fog_of_war_frame COMMAND civ5_fog_of_war_frame_tests)
    add_executable(civ5_fog_of_war_frame_oracle validation/fog_of_war_frame_oracle.cpp)
    target_link_libraries(civ5_fog_of_war_frame_oracle PRIVATE civ5_materials)
    if(MINGW)
        target_link_options(civ5_fog_of_war_frame_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_fog_of_war_tests tests/fog_of_war_tests.cpp)
    target_link_libraries(civ5_fog_of_war_tests PRIVATE civ5_materials)
    add_test(NAME materials.fog_of_war_state COMMAND civ5_fog_of_war_tests)
    add_executable(civ5_fog_of_war_state_oracle validation/fog_of_war_state_oracle.cpp)
    target_link_libraries(civ5_fog_of_war_state_oracle PRIVATE civ5_materials)
    if(MINGW)
        target_link_options(civ5_fog_of_war_state_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_fog_of_war_oracle validation/fog_of_war_oracle.cpp)
    target_link_libraries(civ5_fog_of_war_oracle PRIVATE civ5_materials)
    if(MINGW)
        target_link_options(civ5_fog_of_war_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_distance_fog_tests tests/distance_fog_tests.cpp)
    target_link_libraries(civ5_distance_fog_tests PRIVATE civ5_materials)
    add_test(NAME materials.distance_fog_state COMMAND civ5_distance_fog_tests)
    add_executable(civ5_distance_fog_oracle validation/distance_fog_oracle.cpp)
    target_link_libraries(civ5_distance_fog_oracle PRIVATE civ5_materials)
    if(MINGW)
        target_link_options(civ5_distance_fog_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_terrain_frame_oracle validation/terrain_frame_oracle.cpp)
    target_link_libraries(civ5_terrain_frame_oracle PRIVATE civ5_materials)
    add_executable(civ5_terrain_frame_tests tests/terrain_frame_tests.cpp)
    target_link_libraries(civ5_terrain_frame_tests PRIVATE civ5_materials)
    add_test(NAME materials.terrain_frame_state COMMAND civ5_terrain_frame_tests)
    add_executable(civ5_color_key_tests tests/color_key_tests.cpp)
    target_link_libraries(civ5_color_key_tests PRIVATE civ5_materials)
    add_test(NAME materials.color_key_volume COMMAND civ5_color_key_tests)
    add_executable(civ5_color_key_oracle validation/color_key_oracle.cpp)
    target_link_libraries(civ5_color_key_oracle PRIVATE civ5_materials)
    if(MINGW)
        target_link_options(civ5_color_key_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_camera_frame_oracle validation/camera_frame_oracle.cpp)
    add_executable(civ5_camera_motion_oracle validation/camera_motion_oracle.cpp)
    add_executable(civ5_camera_input_oracle validation/camera_input_oracle.cpp)
    target_link_libraries(civ5_camera_input_oracle PRIVATE civ5_materials)
    target_link_libraries(civ5_camera_motion_oracle PRIVATE civ5_materials)
    target_link_libraries(civ5_camera_frame_oracle PRIVATE civ5_materials)
    add_executable(civ5_view_bounds_oracle validation/view_bounds_oracle.cpp)
    target_link_libraries(civ5_view_bounds_oracle PRIVATE civ5_materials)
    add_executable(civ5_water_reflection_oracle validation/water_reflection_oracle.cpp)
    target_link_libraries(civ5_water_reflection_oracle PRIVATE civ5_materials)
    add_executable(civ5_water_direction_oracle validation/water_direction_oracle.cpp)
    target_link_libraries(civ5_water_direction_oracle PRIVATE civ5_materials)
    add_executable(civ5_water_current_oracle validation/water_current_oracle.cpp)
    target_link_libraries(civ5_water_current_oracle PRIVATE civ5_materials)
    if(MINGW)
        target_link_options(civ5_water_current_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_water_animation_oracle validation/water_animation_oracle.cpp)
    target_link_libraries(civ5_water_animation_oracle PRIVATE civ5_materials)
    add_executable(civ5_water_animation_tests tests/water_animation_tests.cpp)
    target_link_libraries(civ5_water_animation_tests PRIVATE civ5_materials)
    add_test(NAME materials.water_animation_input_guards COMMAND civ5_water_animation_tests)
    add_executable(civ5_water_sky_tests tests/water_sky_tests.cpp)
    target_link_libraries(civ5_water_sky_tests PRIVATE civ5_materials)
    add_test(NAME materials.water_sky COMMAND civ5_water_sky_tests)
    add_executable(civ5_water_sky_oracle validation/water_sky_oracle.cpp)
    target_link_libraries(civ5_water_sky_oracle PRIVATE civ5_materials)
    add_executable(civ5_water_surface_tests tests/water_surface_tests.cpp)
    target_link_libraries(civ5_water_surface_tests PRIVATE civ5_materials)
    add_test(NAME materials.water_surface COMMAND civ5_water_surface_tests)
    add_executable(civ5_water_surface_oracle validation/water_surface_oracle.cpp)
    target_link_libraries(civ5_water_surface_oracle PRIVATE civ5_materials)
    if(MINGW)
        target_link_options(civ5_water_surface_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_lean_tests tests/lean_tests.cpp)
    target_link_libraries(civ5_lean_tests PRIVATE civ5_assets)
    add_test(NAME assets.lean_raw_maps COMMAND civ5_lean_tests)
    add_executable(civ5_material_cache_raster_oracle validation/material_cache_raster_oracle.cpp)
    target_link_libraries(civ5_material_cache_raster_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_cache_passes_tests tests/material_cache_passes_tests.cpp)
    target_link_libraries(civ5_material_cache_passes_tests PRIVATE civ5_materials)
    add_test(NAME materials.cache_passes COMMAND civ5_material_cache_passes_tests)
    add_executable(civ5_material_cache_passes_oracle validation/material_cache_passes_oracle.cpp)
    target_link_libraries(civ5_material_cache_passes_oracle PRIVATE civ5_materials)
    if(MINGW)
        target_link_options(civ5_material_cache_passes_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_material_cache_tests tests/material_cache_tests.cpp)
    target_link_libraries(civ5_material_cache_tests PRIVATE civ5_materials)
    add_test(NAME materials.cache_resources COMMAND civ5_material_cache_tests)
    add_executable(civ5_material_cache_oracle validation/material_cache_oracle.cpp)
    target_link_libraries(civ5_material_cache_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_cache_configuration_oracle validation/material_cache_configuration_oracle.cpp)
    target_link_libraries(civ5_material_cache_configuration_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_load_settings_oracle validation/material_load_settings_oracle.cpp)
    target_link_libraries(civ5_material_load_settings_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_cache_startup_oracle validation/material_cache_startup_oracle.cpp)
    target_link_libraries(civ5_material_cache_startup_oracle PRIVATE civ5_materials)
    if(MINGW)
        target_link_options(civ5_material_cache_oracle PRIVATE -municode)
        target_link_options(civ5_material_cache_startup_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_material_world_oracle validation/material_world_oracle.cpp)
    target_link_libraries(civ5_material_world_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_preparation_world_tests validation/material_preparation_world_tests.cpp)
    target_link_libraries(civ5_material_preparation_world_tests PRIVATE civ5_materials)
    if(MINGW)
        target_link_options(civ5_material_world_oracle PRIVATE -municode)
        target_link_options(civ5_material_preparation_world_tests PRIVATE -municode)
    endif()
    add_executable(civ5_material_setup_oracle validation/material_setup_oracle.cpp)
    target_link_libraries(civ5_material_setup_oracle PRIVATE civ5_materials)
    if(MINGW)
        target_link_options(civ5_material_setup_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_river_generation_oracle validation/river_generation_oracle.cpp)
    add_executable(civ5_river_surface_oracle validation/river_surface_oracle.cpp)
    add_executable(civ5_river_surface_tests tests/river_surface_tests.cpp)
    target_link_libraries(civ5_river_surface_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.river_surface COMMAND civ5_river_surface_tests)
    target_link_libraries(civ5_river_surface_oracle PRIVATE civ5_terrain)
    if(MINGW)
        target_link_options(civ5_river_surface_oracle PRIVATE -municode)
    endif()
    target_link_libraries(civ5_river_generation_oracle PRIVATE civ5_terrain)
    if(MINGW)
        target_link_options(civ5_river_generation_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_terrain_river_generation_oracle validation/terrain_river_generation_oracle.cpp)
    target_link_libraries(civ5_terrain_river_generation_oracle PRIVATE civ5_terrain)
    if(MINGW)
        target_link_options(civ5_terrain_river_generation_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_river_generation_tests tests/river_generation_tests.cpp)
    target_link_libraries(civ5_river_generation_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.river_generation COMMAND civ5_river_generation_tests)
    add_executable(civ5_river_raster_tests tests/river_raster_tests.cpp)
    target_link_libraries(civ5_river_raster_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.river_raster COMMAND civ5_river_raster_tests)
    add_executable(civ5_terrain_river_world_oracle validation/terrain_river_world_oracle.cpp)
    target_link_libraries(civ5_terrain_river_world_oracle PRIVATE civ5_terrain)
    if(MINGW)
        target_link_options(civ5_terrain_river_world_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_river_raster_oracle validation/river_raster_oracle.cpp)
    target_link_libraries(civ5_river_raster_oracle PRIVATE civ5_terrain)
    if(MINGW)
        target_link_options(civ5_river_raster_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_terrain_world_oracle validation/terrain_world_oracle.cpp)
    target_link_libraries(civ5_terrain_world_oracle PRIVATE civ5_terrain)
    if(MINGW)
        target_link_options(civ5_terrain_world_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_terrain_occurrence_oracle validation/terrain_occurrence_oracle.cpp)
    target_link_libraries(civ5_terrain_occurrence_oracle PRIVATE civ5_terrain)
    if(MINGW)
        target_link_options(civ5_terrain_occurrence_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_terrain_source_tests tests/terrain_source_tests.cpp)
    target_link_libraries(civ5_terrain_source_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.source_library_placement COMMAND civ5_terrain_source_tests)
    add_executable(civ5_terrain_source_oracle validation/terrain_source_oracle.cpp)
    target_link_libraries(civ5_terrain_source_oracle PRIVATE civ5_terrain)
    if(MINGW)
        target_link_options(civ5_terrain_source_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_coast_map_tests tests/coast_map_tests.cpp)
    target_link_libraries(civ5_coast_map_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.coast_map_graph_contours_perturbation COMMAND civ5_coast_map_tests)
    add_executable(civ5_coast_map_oracle validation/coast_map_oracle.cpp)
    target_link_libraries(civ5_coast_map_oracle PRIVATE civ5_terrain)
    if(MINGW)
        target_link_options(civ5_coast_map_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_coast_curve_tests tests/coast_curve_tests.cpp)
    target_link_libraries(civ5_coast_curve_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.coast_profiles_character_curves COMMAND civ5_coast_curve_tests)
    add_executable(civ5_worker_geometry_tests tests/worker_geometry_tests.cpp)
    target_link_libraries(civ5_worker_geometry_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.worker_geometry COMMAND civ5_worker_geometry_tests)
    add_executable(civ5_coast_strip_tests tests/coast_strip_tests.cpp)
    target_link_libraries(civ5_coast_strip_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.coast_strip_dispatch COMMAND civ5_coast_strip_tests)
    add_executable(civ5_coast_asset_tests tests/coast_asset_tests.cpp)
    target_link_libraries(civ5_coast_asset_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.coast_source_loading COMMAND civ5_coast_asset_tests)
    add_executable(civ5_coast_asset_oracle validation/coast_asset_oracle.cpp)
    target_link_libraries(civ5_coast_asset_oracle PRIVATE civ5_terrain)
    if(MINGW)
        target_link_options(civ5_coast_asset_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_coast_strip_oracle validation/coast_strip_oracle.cpp)
    target_link_libraries(civ5_coast_strip_oracle PRIVATE civ5_terrain)
    if(MINGW)
        target_link_options(civ5_coast_strip_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_coast_geometry_oracle validation/coast_geometry_oracle.cpp)
    target_link_libraries(civ5_coast_geometry_oracle PRIVATE civ5_terrain)
    if(MINGW)
        target_link_options(civ5_coast_geometry_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_worker_gate_tests tests/worker_gate_tests.cpp)
    target_link_libraries(civ5_worker_gate_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.worker_gate_routing COMMAND civ5_worker_gate_tests)
    add_executable(civ5_worker_gate_oracle validation/worker_gate_oracle.cpp)
    target_link_libraries(civ5_worker_gate_oracle PRIVATE civ5_terrain)
    if(MINGW)
        target_link_options(civ5_worker_gate_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_coast_raster_tests tests/coast_raster_tests.cpp)
    target_link_libraries(civ5_coast_raster_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.coast_raster COMMAND civ5_coast_raster_tests)
    add_executable(civ5_coast_raster_oracle validation/coast_raster_oracle.cpp)
    target_link_libraries(civ5_coast_raster_oracle PRIVATE civ5_terrain)
    if(MINGW)
        target_link_options(civ5_coast_raster_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_worker_field_tests tests/worker_field_tests.cpp)
    target_link_libraries(civ5_worker_field_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.worker_field COMMAND civ5_worker_field_tests)
    add_executable(civ5_worker_field_oracle validation/worker_field_oracle.cpp)
    target_link_libraries(civ5_worker_field_oracle PRIVATE civ5_terrain)
    if(MINGW)
        target_link_options(civ5_worker_field_oracle PRIVATE -municode)
    endif()
    add_executable(civ5_land_material_tests tests/land_material_tests.cpp)
    target_link_libraries(civ5_land_material_tests PRIVATE civ5_materials)
    add_test(NAME materials.land_selection_constants_mesh COMMAND civ5_land_material_tests)
    add_executable(civ5_land_material_oracle validation/land_material_oracle.cpp)
    target_link_libraries(civ5_land_material_oracle PRIVATE civ5_materials)
    add_executable(civ5_lighting_mesh_tests tests/lighting_mesh_tests.cpp)
    target_link_libraries(civ5_lighting_mesh_tests PRIVATE civ5_materials)
    add_test(NAME materials.lighting_mesh COMMAND civ5_lighting_mesh_tests)
    add_executable(civ5_lighting_source_tests tests/lighting_source_tests.cpp)
    target_link_libraries(civ5_lighting_source_tests PRIVATE civ5_materials)
    add_test(NAME materials.lighting_sources_bindings COMMAND civ5_lighting_source_tests)
    add_executable(civ5_lighting_source_oracle validation/lighting_source_oracle.cpp)
    target_link_libraries(civ5_lighting_source_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_shadow_tests tests/material_shadow_tests.cpp)
    target_link_libraries(civ5_material_shadow_tests PRIVATE civ5_materials)
    add_test(NAME materials.macro_shadows COMMAND civ5_material_shadow_tests)
    add_executable(civ5_material_shadow_oracle validation/material_shadow_oracle.cpp)
    target_link_libraries(civ5_material_shadow_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_normal_tests tests/material_normal_tests.cpp)
    target_link_libraries(civ5_material_normal_tests PRIVATE civ5_materials)
    add_test(NAME materials.cache_normals COMMAND civ5_material_normal_tests)
    add_executable(civ5_material_normal_oracle validation/material_normal_oracle.cpp)
    target_link_libraries(civ5_material_normal_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_generation_oracle validation/material_generation_oracle.cpp)
    target_link_libraries(civ5_material_generation_oracle PRIVATE civ5_materials)
    add_executable(civ5_material_generation_tests tests/material_generation_tests.cpp)
    target_link_libraries(civ5_material_generation_tests PRIVATE civ5_materials)
    add_test(NAME materials.geometry_scores_patch COMMAND civ5_material_generation_tests)
    add_executable(civ5_material_tests tests/material_tests.cpp)
    target_link_libraries(civ5_material_tests PRIVATE civ5_materials)
    add_test(NAME materials.sources_constants_resource_zero COMMAND civ5_material_tests)
    add_executable(civ5_material_oracle validation/material_oracle.cpp)
    target_link_libraries(civ5_material_oracle PRIVATE civ5_materials)
    add_executable(civ5_land_grid_tests tests/land_grid_tests.cpp)
    target_link_libraries(civ5_land_grid_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.shared_land_grid COMMAND civ5_land_grid_tests)
    add_executable(civ5_land_grid_oracle validation/land_grid_oracle.cpp)
    target_link_libraries(civ5_land_grid_oracle PRIVATE civ5_terrain)
    add_executable(civ5_asset_tests tests/asset_tests.cpp)
    target_link_libraries(civ5_asset_tests PRIVATE civ5_assets)
    add_test(NAME assets.raw_sources_dds COMMAND civ5_asset_tests "${CMAKE_CURRENT_BINARY_DIR}")
    if(WIN32)
        add_executable(civ5_minimap_dx9_water validation/minimap_dx9_water.cpp)
        target_link_libraries(civ5_minimap_dx9_water PRIVATE civ5_assets d3d9)
        add_executable(civ5_minimap_dx9_surface validation/minimap_dx9_surface.cpp)
        target_link_libraries(civ5_minimap_dx9_surface PRIVATE civ5_assets d3d9)
        add_executable(civ5_minimap_dx9_viewport validation/minimap_dx9_viewport.cpp)
        target_link_libraries(civ5_minimap_dx9_viewport PRIVATE d3d9)
        add_executable(civ5_gl_minimap_viewport validation/gl_minimap_viewport.cpp)
        target_link_libraries(civ5_gl_minimap_viewport PRIVATE civ5_gl)
        add_executable(civ5_gl_minimap_mountain validation/gl_minimap_mountain.cpp)
        target_link_libraries(civ5_gl_minimap_mountain PRIVATE civ5_gl)
        add_executable(civ5_gl_minimap_ice validation/gl_minimap_ice.cpp)
        target_link_libraries(civ5_gl_minimap_ice PRIVATE civ5_gl)
        add_executable(civ5_gl_water_world_probe validation/gl_water_world_probe.cpp)
        target_link_libraries(civ5_gl_water_world_probe PRIVATE civ5_gl d3d9 d3dcompiler)
        if(MINGW)
            target_link_options(civ5_gl_water_world_probe PRIVATE -municode)
        endif()
        add_executable(civ5_gl_water_current validation/gl_water_current.cpp)
        add_executable(civ5_gl_fog_of_war validation/gl_fog_of_war.cpp)
        target_link_libraries(civ5_gl_fog_of_war PRIVATE civ5_gl d3d9)
        add_executable(civ5_fog_of_war_probe validation/fog_of_war_probe.cpp)
        target_link_libraries(civ5_fog_of_war_probe PRIVATE civ5_gl d3d9)
        if(MINGW)
            target_link_options(civ5_gl_fog_of_war PRIVATE -municode)
            target_link_options(civ5_fog_of_war_probe PRIVATE -municode)
        endif()
        target_link_libraries(civ5_gl_water_current PRIVATE civ5_gl d3d9 d3dcompiler)
        add_executable(civ5_gl_water_current_map validation/gl_water_current_map.cpp)
        target_link_libraries(civ5_gl_water_current_map PRIVATE civ5_gl d3d9 d3dcompiler)
        add_executable(civ5_gl_water_reflection_map validation/gl_water_reflection_map.cpp)
        target_link_libraries(civ5_gl_water_reflection_map PRIVATE civ5_gl d3d9 d3dcompiler)
        if(MINGW)
            target_link_options(civ5_gl_water_current PRIVATE -municode)
            target_link_options(civ5_gl_water_current_map PRIVATE -municode)
            target_link_options(civ5_gl_water_reflection_map PRIVATE -municode)
        endif()
        add_executable(civ5_gl_water_surface validation/gl_water_surface.cpp)
        add_executable(civ5_gl_river_surface validation/gl_river_surface.cpp)
        target_link_libraries(civ5_gl_river_surface PRIVATE civ5_gl d3d9 d3dcompiler)
        if(MINGW)
            target_link_options(civ5_gl_river_surface PRIVATE -municode)
        endif()
        target_link_libraries(civ5_gl_water_surface PRIVATE civ5_gl d3d9 d3dcompiler)
        if(MINGW)
            target_link_options(civ5_gl_water_surface PRIVATE -municode)
        endif()
        add_executable(civ5_gl_material_mip_diagnostic validation/gl_material_mip_diagnostic.cpp)
        target_link_libraries(civ5_gl_material_mip_diagnostic PRIVATE civ5_gl d3d9 d3dcompiler)
        add_executable(civ5_gl_material_world validation/gl_material_world.cpp
            validation/ui_screen_gl.cpp validation/gl_bnw_warrior_retained.cpp)
        target_link_libraries(civ5_gl_material_world PRIVATE civ5_gl civ5_scene_saved_fog civ5_bnw_deferred_fog_client civ5_bnw_unit_visuals civis_runtime d3d9 d3dcompiler bcrypt shlwapi shell32)
        add_executable(civ5_gl_bnw_warrior_art_probe
            validation/gl_bnw_warrior_art_probe.cpp)
        target_link_libraries(civ5_gl_bnw_warrior_art_probe
            PRIVATE civ5_gl civis_runtime bcrypt)
        if(MINGW)
            target_link_options(civ5_gl_bnw_warrior_art_probe PRIVATE -static)
        endif()
        add_executable(civ5_gl_bnw_warrior_geometry_probe
            validation/gl_bnw_warrior_geometry_probe.cpp)
        target_link_libraries(civ5_gl_bnw_warrior_geometry_probe
            PRIVATE civ5_gl civis_runtime bcrypt)
        if(MINGW)
            target_link_options(civ5_gl_bnw_warrior_geometry_probe PRIVATE -static)
        endif()
        add_executable(civ5_gl_bnw_warrior_skin_probe
            validation/gl_bnw_warrior_skin_probe.cpp)
        target_link_libraries(civ5_gl_bnw_warrior_skin_probe
            PRIVATE civ5_gl civis_runtime bcrypt)
        if(MINGW)
            target_link_options(civ5_gl_bnw_warrior_skin_probe PRIVATE -static)
        endif()
        add_executable(civ5_gl_bnw_warrior_retained_tests
            tests/gl_bnw_warrior_retained_tests.cpp
            validation/gl_bnw_warrior_retained.cpp)
        add_executable(civ5_gl_bnw_system_font_atlas_tests
            tests/gl_bnw_system_font_atlas_tests.cpp)
        target_link_libraries(civ5_gl_bnw_system_font_atlas_tests PRIVATE civ5_gl civis_runtime)
        target_compile_features(civ5_gl_bnw_system_font_atlas_tests PRIVATE cxx_std_20)
        add_executable(civ5_gl_bnw_font_shader_tests tests/gl_bnw_font_shader_tests.cpp)
        target_link_libraries(civ5_gl_bnw_font_shader_tests PRIVATE civ5_gl civis_runtime)
        target_compile_features(civ5_gl_bnw_font_shader_tests PRIVATE cxx_std_20)
        if(MINGW)
            target_link_options(civ5_gl_bnw_font_shader_tests PRIVATE -static)
        endif()
        # GPU diagnostic: explicit gate only, not added to automatic CPU CTest.
        if(MINGW)
            target_link_options(civ5_gl_bnw_system_font_atlas_tests PRIVATE -static)
        endif()
        add_executable(civ5_gl_bnw_warrior_retained_material_tests
            tests/gl_bnw_warrior_retained_material_tests.cpp
            validation/gl_bnw_warrior_retained.cpp)
        target_include_directories(civ5_gl_bnw_warrior_retained_material_tests PRIVATE validation)
        target_compile_features(civ5_gl_bnw_warrior_retained_material_tests PRIVATE cxx_std_20)
        target_link_libraries(civ5_gl_bnw_warrior_retained_material_tests
            PRIVATE civ5_gl civ5_materials civ5_terrain civis_runtime bcrypt)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(civ5_gl_bnw_warrior_retained_material_tests PRIVATE -ffp-contract=off)
        endif()
        if(MINGW)
            target_link_options(civ5_gl_bnw_warrior_retained_material_tests PRIVATE -static)
        endif()
        add_executable(civ5_bnw_rendered_fog_frame_tests tests/bnw_rendered_fog_frame_tests.cpp)
        target_link_libraries(civ5_bnw_rendered_fog_frame_tests PRIVATE civ5_gl civis_runtime civ5_bnw_deferred_fog_client civ5_scene_saved_fog)
        if(MINGW)
            target_link_options(civ5_bnw_rendered_fog_frame_tests PRIVATE -static)
        endif()
        add_test(NAME graphics.bnw_rendered_fog_frame COMMAND civ5_bnw_rendered_fog_frame_tests)
        add_executable(civ5_bnw_camera_fog_client_tests tests/bnw_camera_fog_client_tests.cpp)
        target_link_libraries(civ5_bnw_camera_fog_client_tests PRIVATE civ5_gl civis_runtime
            civ5_bnw_deferred_fog_client civ5_scene_saved_fog)
        if(MINGW)
            target_link_options(civ5_bnw_camera_fog_client_tests PRIVATE -static)
        endif()
        if(CIV5_BNW_INSTALL_ROOT AND EXISTS "${CIV5_BNW_SELECTED_SAVE}")
            add_test(NAME graphics.bnw_camera_fog_client COMMAND civ5_bnw_camera_fog_client_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        endif()
        add_executable(civ5_gl_bnw_warrior_authored_formation_tests
            tests/gl_bnw_warrior_authored_formation_tests.cpp
            validation/gl_bnw_warrior_retained.cpp)
        target_link_libraries(civ5_gl_bnw_warrior_authored_formation_tests PRIVATE civ5_gl civis_runtime bcrypt)
        if(MINGW)
            target_link_options(civ5_gl_bnw_warrior_authored_formation_tests PRIVATE -static)
        endif()
        add_executable(civ5_gl_bnw_warrior_owned_scene_tests
            tests/gl_bnw_warrior_owned_scene_tests.cpp
            validation/gl_bnw_warrior_retained.cpp)
        add_executable(civ5_graphics_bnw_warrior_material_tests
            tests/graphics_bnw_warrior_material_tests.cpp)
        add_executable(civ5_graphics_bnw_warrior_code_material_tests
            tests/graphics_bnw_warrior_code_material_tests.cpp)
        add_executable(civ5_graphics_bnw_warrior_material_frame_tests
            tests/graphics_bnw_warrior_material_frame_tests.cpp)
        target_include_directories(civ5_graphics_bnw_warrior_material_frame_tests PRIVATE include)
        target_compile_features(civ5_graphics_bnw_warrior_material_frame_tests PRIVATE cxx_std_20)
        add_executable(civ5_graphics_bnw_warrior_native_vertex_weights_tests
            tests/graphics_bnw_warrior_native_vertex_weights_tests.cpp)
        add_executable(civ5_graphics_bnw_warrior_native_vertex_conversion_tests
            tests/graphics_bnw_warrior_native_vertex_conversion_tests.cpp)
        add_executable(civ5_graphics_bnw_warrior_native_vertex_consumer_tests
            tests/graphics_bnw_warrior_native_vertex_consumer_tests.cpp)
        add_executable(civ5_graphics_bnw_warrior_native_clip_time_tests
            tests/graphics_bnw_warrior_native_clip_time_tests.cpp)
        target_include_directories(civ5_graphics_bnw_warrior_native_clip_time_tests PRIVATE include)
        target_compile_features(civ5_graphics_bnw_warrior_native_clip_time_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(civ5_graphics_bnw_warrior_native_clip_time_tests PRIVATE -ffp-contract=off)
        endif()
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_native_clip_time_tests PRIVATE -static)
        endif()
        set(CIVIS_WARRIOR_CLIP_FIXTURE "" CACHE FILEPATH "Optional local native clip-time capture; not distributed")
        if(EXISTS "${CIVIS_WARRIOR_CLIP_FIXTURE}")
            add_test(NAME graphics.bnw_warrior_native_clip_time
                COMMAND civ5_graphics_bnw_warrior_native_clip_time_tests "${CIVIS_WARRIOR_CLIP_FIXTURE}")
        endif()
        add_executable(civ5_graphics_bnw_warrior_idle_curve_tests
            tests/graphics_bnw_warrior_idle_curve_tests.cpp)
        target_include_directories(civ5_graphics_bnw_warrior_idle_curve_tests PRIVATE include)
        target_compile_features(civ5_graphics_bnw_warrior_idle_curve_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(civ5_graphics_bnw_warrior_idle_curve_tests PRIVATE -ffp-contract=off)
        endif()
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_idle_curve_tests PRIVATE -static)
        endif()
        set(CIV5_BNW_WARRIOR_IDLE_CURVE_FIXTURE "" CACHE FILEPATH
            "Locally generated source-leaf fixture for Warrior IdleA curve checks")
        if(EXISTS "${CIV5_BNW_WARRIOR_IDLE_CURVE_FIXTURE}")
            add_test(NAME graphics.bnw_warrior_idle_curve
                COMMAND civ5_graphics_bnw_warrior_idle_curve_tests
                "${CIV5_BNW_WARRIOR_IDLE_CURVE_FIXTURE}")
        endif()
        add_executable(civ5_graphics_bnw_warrior_idle_source_window_tests
            tests/graphics_bnw_warrior_idle_source_window_tests.cpp)
        target_include_directories(civ5_graphics_bnw_warrior_idle_source_window_tests PRIVATE include)
        target_compile_features(civ5_graphics_bnw_warrior_idle_source_window_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(civ5_graphics_bnw_warrior_idle_source_window_tests PRIVATE -ffp-contract=off)
        endif()
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_idle_source_window_tests PRIVATE -static)
        endif()
        set(CIV5_BNW_WARRIOR_IDLE_SOURCE_WINDOW_FIXTURE "" CACHE FILEPATH
            "Locally generated native Warrior IdleA source-window fixture")
        if(EXISTS "${CIV5_BNW_WARRIOR_IDLE_SOURCE_WINDOW_FIXTURE}")
            add_test(NAME graphics.bnw_warrior_idle_source_window
                COMMAND civ5_graphics_bnw_warrior_idle_source_window_tests
                "${CIV5_BNW_WARRIOR_IDLE_SOURCE_WINDOW_FIXTURE}")
        endif()
        add_executable(civ5_graphics_bnw_warrior_idle_window_producer_tests
            tests/graphics_bnw_warrior_idle_window_producer_tests.cpp)
        target_include_directories(civ5_graphics_bnw_warrior_idle_window_producer_tests PRIVATE include)
        target_compile_features(civ5_graphics_bnw_warrior_idle_window_producer_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(civ5_graphics_bnw_warrior_idle_window_producer_tests PRIVATE -ffp-contract=off)
        endif()
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_idle_window_producer_tests PRIVATE -static)
        endif()
        if(EXISTS "${CIV5_BNW_WARRIOR_IDLE_SOURCE_WINDOW_FIXTURE}" AND
           EXISTS "${CIV5_BNW_WARRIOR_IDLE_CURVE_FIXTURE}")
            add_test(NAME graphics.bnw_warrior_idle_window_producer
                COMMAND civ5_graphics_bnw_warrior_idle_window_producer_tests
                "${CIV5_BNW_WARRIOR_IDLE_SOURCE_WINDOW_FIXTURE}"
                "${CIV5_BNW_WARRIOR_IDLE_CURVE_FIXTURE}")
        endif()
        add_executable(civ5_graphics_bnw_warrior_native_bone_matrices_tests
            tests/graphics_bnw_warrior_native_bone_matrices_tests.cpp)
        target_include_directories(civ5_graphics_bnw_warrior_native_bone_matrices_tests PRIVATE include)
        target_compile_features(civ5_graphics_bnw_warrior_native_bone_matrices_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(civ5_graphics_bnw_warrior_native_bone_matrices_tests PRIVATE -ffp-contract=off)
        endif()
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_native_bone_matrices_tests PRIVATE -static)
        endif()
        set(CIV5_BNW_WARRIOR_NATIVE_BONE_MATRICES_FIXTURE "" CACHE FILEPATH
            "Locally generated native Warrior hierarchy and packed-palette fixture")
        if(EXISTS "${CIV5_BNW_WARRIOR_NATIVE_BONE_MATRICES_FIXTURE}" AND
           EXISTS "${CIV5_BNW_WARRIOR_IDLE_CURVE_FIXTURE}")
            add_test(NAME graphics.bnw_warrior_native_bone_matrices
                COMMAND civ5_graphics_bnw_warrior_native_bone_matrices_tests
                "${CIV5_BNW_WARRIOR_NATIVE_BONE_MATRICES_FIXTURE}"
                "${CIV5_BNW_WARRIOR_IDLE_CURVE_FIXTURE}")
        endif()
        add_executable(civ5_graphics_bnw_warrior_native_resource_basis_tests
            tests/graphics_bnw_warrior_native_resource_basis_tests.cpp)
        target_include_directories(civ5_graphics_bnw_warrior_native_resource_basis_tests PRIVATE include)
        target_compile_features(civ5_graphics_bnw_warrior_native_resource_basis_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(civ5_graphics_bnw_warrior_native_resource_basis_tests PRIVATE -ffp-contract=off)
        endif()
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_native_resource_basis_tests PRIVATE -static)
        endif()
        set(CIV5_BNW_WARRIOR_NATIVE_RESOURCE_BASIS_FIXTURE "" CACHE FILEPATH
            "Locally generated native Warrior resource basis and converted-skeleton fixture")
        if(EXISTS "${CIV5_BNW_WARRIOR_NATIVE_RESOURCE_BASIS_FIXTURE}")
            add_test(NAME graphics.bnw_warrior_native_resource_basis
                COMMAND civ5_graphics_bnw_warrior_native_resource_basis_tests
                "${CIV5_BNW_WARRIOR_NATIVE_RESOURCE_BASIS_FIXTURE}")
        endif()
        add_executable(civ5_graphics_bnw_warrior_native_resource_mesh_tests
            tests/graphics_bnw_warrior_native_resource_mesh_tests.cpp)
        target_include_directories(civ5_graphics_bnw_warrior_native_resource_mesh_tests PRIVATE include)
        target_compile_features(civ5_graphics_bnw_warrior_native_resource_mesh_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(civ5_graphics_bnw_warrior_native_resource_mesh_tests PRIVATE -ffp-contract=off)
        endif()
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_native_resource_mesh_tests PRIVATE -static)
        endif()
        set(CIV5_BNW_WARRIOR_NATIVE_RESOURCE_MESH_FIXTURE "" CACHE FILEPATH
            "Locally generated Warrior mesh vectors versus scoped native CPU-emulation fixture")
        if(EXISTS "${CIV5_BNW_WARRIOR_NATIVE_RESOURCE_MESH_FIXTURE}")
            add_test(NAME graphics.bnw_warrior_native_resource_mesh
                COMMAND civ5_graphics_bnw_warrior_native_resource_mesh_tests
                "${CIV5_BNW_WARRIOR_NATIVE_RESOURCE_MESH_FIXTURE}")
        endif()
        target_include_directories(civ5_graphics_bnw_warrior_native_vertex_consumer_tests PRIVATE include)
        add_executable(civ5_graphics_bnw_warrior_native_resource_curves_tests
            tests/graphics_bnw_warrior_native_resource_curves_tests.cpp)
        target_include_directories(civ5_graphics_bnw_warrior_native_resource_curves_tests PRIVATE include)
        target_compile_features(civ5_graphics_bnw_warrior_native_resource_curves_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(civ5_graphics_bnw_warrior_native_resource_curves_tests PRIVATE -ffp-contract=off)
        endif()
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_native_resource_curves_tests PRIVATE -static)
        endif()
        set(CIV5_BNW_WARRIOR_NATIVE_RESOURCE_CURVES_FIXTURE "" CACHE FILEPATH
            "Locally generated compressed Warrior animation curves versus native CPU-emulation fixture")
        if(EXISTS "${CIV5_BNW_WARRIOR_NATIVE_RESOURCE_CURVES_FIXTURE}")
            add_test(NAME graphics.bnw_warrior_native_resource_curves
                COMMAND civ5_graphics_bnw_warrior_native_resource_curves_tests
                "${CIV5_BNW_WARRIOR_NATIVE_RESOURCE_CURVES_FIXTURE}")
        endif()
        target_compile_features(civ5_graphics_bnw_warrior_native_vertex_consumer_tests PRIVATE cxx_std_20)
        add_executable(civ5_graphics_bnw_warrior_native_resource_trackgroup_tests
            tests/graphics_bnw_warrior_native_resource_trackgroup_tests.cpp)
        target_include_directories(civ5_graphics_bnw_warrior_native_resource_trackgroup_tests PRIVATE include)
        target_compile_features(civ5_graphics_bnw_warrior_native_resource_trackgroup_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(civ5_graphics_bnw_warrior_native_resource_trackgroup_tests PRIVATE -ffp-contract=off)
        endif()
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_native_resource_trackgroup_tests PRIVATE -static)
        endif()
        set(CIV5_BNW_WARRIOR_NATIVE_RESOURCE_TRACKGROUP_FIXTURE "" CACHE FILEPATH
            "Locally generated whole native Warrior trackgroup CPU-emulation fixture")
        if(EXISTS "${CIV5_BNW_WARRIOR_NATIVE_RESOURCE_TRACKGROUP_FIXTURE}")
            add_test(NAME graphics.bnw_warrior_native_resource_trackgroup
                COMMAND civ5_graphics_bnw_warrior_native_resource_trackgroup_tests
                "${CIV5_BNW_WARRIOR_NATIVE_RESOURCE_TRACKGROUP_FIXTURE}")
        endif()
        add_executable(civ5_graphics_bnw_warrior_native_vertex_cache_tests
            tests/graphics_bnw_warrior_native_vertex_cache_tests.cpp)
        target_include_directories(civ5_graphics_bnw_warrior_native_vertex_cache_tests PRIVATE validation)
        target_link_libraries(civ5_graphics_bnw_warrior_native_vertex_cache_tests PRIVATE civis_runtime)
        target_compile_features(civ5_graphics_bnw_warrior_native_vertex_cache_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(civ5_graphics_bnw_warrior_native_vertex_consumer_tests PRIVATE -ffp-contract=off)
            target_compile_options(civ5_graphics_bnw_warrior_native_vertex_cache_tests PRIVATE -ffp-contract=off)
        endif()
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_native_vertex_consumer_tests PRIVATE -static)
            target_link_options(civ5_graphics_bnw_warrior_native_vertex_cache_tests PRIVATE -static)
        endif()
        target_include_directories(civ5_graphics_bnw_warrior_native_vertex_conversion_tests PRIVATE include)
        target_compile_features(civ5_graphics_bnw_warrior_native_vertex_conversion_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(civ5_graphics_bnw_warrior_native_vertex_conversion_tests PRIVATE -ffp-contract=off)
        endif()
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_native_vertex_conversion_tests PRIVATE -static)
        endif()
        target_include_directories(civ5_graphics_bnw_warrior_native_vertex_weights_tests PRIVATE include)
        target_compile_features(civ5_graphics_bnw_warrior_native_vertex_weights_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(civ5_graphics_bnw_warrior_native_vertex_weights_tests PRIVATE -ffp-contract=off)
        endif()
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_native_vertex_weights_tests PRIVATE -static)
        endif()
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_material_frame_tests PRIVATE -static)
        endif()
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(civ5_graphics_bnw_warrior_material_frame_tests PRIVATE -ffp-contract=off)
        endif()
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../build/warrior-code-stage/frame-audit/native-alpha.bin")
            add_test(NAME graphics.bnw_warrior_material_frame
                COMMAND civ5_graphics_bnw_warrior_material_frame_tests
                "${CMAKE_CURRENT_SOURCE_DIR}/../build/warrior-code-stage/frame-audit/native-alpha.bin")
        endif()
        target_include_directories(civ5_graphics_bnw_warrior_code_material_tests PRIVATE include)
        target_compile_features(civ5_graphics_bnw_warrior_code_material_tests PRIVATE cxx_std_20)
        add_test(NAME graphics.bnw_warrior_code_material
            COMMAND civ5_graphics_bnw_warrior_code_material_tests)
        add_executable(civ5_gl_bnw_warrior_code_material_tests
            tests/gl_bnw_warrior_code_material_tests.cpp)
        add_executable(civ5_gl_bnw_warrior_sampler_policy_tests
            tests/gl_bnw_warrior_sampler_policy_tests.cpp)
        target_include_directories(civ5_gl_bnw_warrior_sampler_policy_tests PRIVATE include validation)
        target_compile_features(civ5_gl_bnw_warrior_sampler_policy_tests PRIVATE cxx_std_20)
        target_link_libraries(civ5_gl_bnw_warrior_sampler_policy_tests
            PRIVATE civ5_gl civ5_assets civis_runtime bcrypt)
        if(MINGW)
            target_link_options(civ5_gl_bnw_warrior_sampler_policy_tests PRIVATE -static)
        endif()
        target_include_directories(civ5_gl_bnw_warrior_code_material_tests PRIVATE include validation)
        target_compile_features(civ5_gl_bnw_warrior_code_material_tests PRIVATE cxx_std_20)
        target_link_libraries(civ5_gl_bnw_warrior_code_material_tests
            PRIVATE civ5_gl civ5_assets civis_runtime d3d9 d3dcompiler bcrypt)
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_code_material_tests PRIVATE -static)
            target_link_options(civ5_gl_bnw_warrior_code_material_tests PRIVATE -static)
        endif()
        add_executable(civ5_graphics_bnw_warrior_team_color_tests
            tests/graphics_bnw_warrior_team_color_tests.cpp)
        target_link_libraries(civ5_graphics_bnw_warrior_team_color_tests PRIVATE civis_runtime)
        if(EXISTS "${CIV5_ASSETS_ROOT}/Assets/Gameplay/XML/Interface/CIV5Colors.xml")
            add_test(NAME graphics.bnw_warrior_team_color COMMAND civ5_graphics_bnw_warrior_team_color_tests
                "${CIV5_ASSETS_ROOT}")
        endif()
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_team_color_tests PRIVATE -static)
        endif()
        target_include_directories(civ5_graphics_bnw_warrior_material_tests PRIVATE validation)
        target_link_libraries(civ5_graphics_bnw_warrior_material_tests
            PRIVATE civ5_assets civis_runtime bcrypt)
        add_executable(civ5_gl_bnw_warrior_material_tests
            tests/gl_bnw_warrior_material_tests.cpp)
        target_include_directories(civ5_gl_bnw_warrior_material_tests PRIVATE validation)
        target_link_libraries(civ5_gl_bnw_warrior_material_tests
            PRIVATE civ5_gl civ5_assets civis_runtime d3d9 d3dcompiler bcrypt)
        if(MINGW)
            target_link_options(civ5_graphics_bnw_warrior_material_tests PRIVATE -static)
            target_link_options(civ5_gl_bnw_warrior_material_tests PRIVATE -static)
        endif()
        target_include_directories(civ5_gl_bnw_warrior_owned_scene_tests PRIVATE validation)
        target_link_libraries(civ5_gl_bnw_warrior_owned_scene_tests
            PRIVATE civ5_gl civ5_bnw_unit_visuals civ5_materials civis_runtime bcrypt)
        target_compile_features(civ5_gl_bnw_warrior_owned_scene_tests PRIVATE cxx_std_20)
        if(MINGW)
            target_link_options(civ5_gl_bnw_warrior_owned_scene_tests PRIVATE -static)
        endif()
        target_link_libraries(civ5_gl_bnw_warrior_retained_tests
            PRIVATE civ5_gl civis_runtime bcrypt)
        if(MINGW)
            target_link_options(civ5_gl_bnw_warrior_retained_tests PRIVATE -static)
        endif()
        add_executable(civ5_ui_screen_gl_probe validation/ui_screen_gl_probe.cpp
            validation/ui_screen_gl.cpp)
        target_link_libraries(civ5_ui_screen_gl_probe PRIVATE civ5_gl civis_runtime
            d3d9 d3dcompiler bcrypt shlwapi shell32)
        add_executable(civ5_gl_material_world_resources validation/gl_material_world_resources.cpp)
        add_executable(civ5_gl_material_world_decals validation/gl_material_world_decals.cpp)
        target_link_libraries(civ5_gl_material_world_decals PRIVATE civ5_gl civ5_scene_landmark_registry d3d9 d3dcompiler bcrypt)
        add_executable(civ5_gl_scene_context_recovery validation/gl_scene_context_recovery.cpp)
        add_executable(civ5_gl_distance_fog_context_recovery validation/gl_distance_fog_context_recovery.cpp)
        target_link_libraries(civ5_gl_scene_context_recovery PRIVATE civ5_gl d3d9 d3dcompiler bcrypt)
        target_link_libraries(civ5_gl_distance_fog_context_recovery PRIVATE civ5_gl)
        target_link_libraries(civ5_gl_material_world_resources PRIVATE civ5_gl d3d9 d3dcompiler bcrypt)
        add_executable(civ5_gl_material_cache_chain validation/gl_material_cache_chain.cpp)
        target_link_libraries(civ5_gl_material_cache_chain PRIVATE civ5_gl d3d9)
        add_executable(civ5_gl_material_cache_raster validation/gl_material_cache_raster.cpp)
        target_link_libraries(civ5_gl_material_cache_raster PRIVATE civ5_gl d3d9)
        add_executable(civ5_gl_material_cache validation/gl_material_cache.cpp)
        target_link_libraries(civ5_gl_material_cache PRIVATE civ5_gl d3d9 d3dcompiler)
        add_executable(civ5_gl_material_decal validation/gl_material_decal.cpp)
        target_link_libraries(civ5_gl_material_decal PRIVATE civ5_gl d3d9 d3dcompiler)
        add_executable(civ5_gl_terrain_decal validation/gl_terrain_decal.cpp)
        target_link_libraries(civ5_gl_terrain_decal PRIVATE civ5_gl d3d9)
        add_executable(civ5_gl_land_material validation/gl_land_material.cpp)
        target_link_libraries(civ5_gl_land_material PRIVATE civ5_gl d3d9)
        add_executable(civ5_gl_land_geometry_pool validation/gl_land_geometry_pool.cpp)
        target_link_libraries(civ5_gl_land_geometry_pool PRIVATE civ5_gl d3d9)
        add_executable(civ5_gl_shared_land_geometry validation/gl_shared_land_geometry.cpp)
        target_link_libraries(civ5_gl_shared_land_geometry PRIVATE civ5_gl)
        add_executable(civ5_gl_land_patch_resources validation/gl_land_patch_resources.cpp)
        target_link_libraries(civ5_gl_land_patch_resources PRIVATE civ5_gl d3d9)
        add_executable(civ5_gl_material_lighting validation/gl_material_lighting.cpp)
        target_link_libraries(civ5_gl_material_lighting PRIVATE civ5_gl d3d9)
        add_executable(civ5_lighting_cube_audit validation/lighting_cube_audit.cpp)
        target_link_libraries(civ5_lighting_cube_audit PRIVATE civ5_gl)
        add_executable(civ5_gl_material_shadows validation/gl_material_shadows.cpp)
        target_link_libraries(civ5_gl_material_shadows PRIVATE civ5_gl d3d9)
        add_executable(civ5_gl_material_normals validation/gl_material_normals.cpp)
        target_link_libraries(civ5_gl_material_normals PRIVATE civ5_gl d3d9)
        add_executable(civ5_gl_material_blend validation/gl_material_blend.cpp)
        target_link_libraries(civ5_gl_material_blend PRIVATE civ5_gl d3d9)
        add_executable(civ5_material_assets_audit validation/material_assets_audit.cpp)
        target_link_libraries(civ5_material_assets_audit PRIVATE civ5_gl)
        add_executable(civ5_gl_land_height validation/gl_land_height.cpp)
        target_link_libraries(civ5_gl_land_height PRIVATE civ5_gl)
        add_executable(civ5_gl_texture_tests tests/gl_texture_tests.cpp)
        target_link_libraries(civ5_gl_texture_tests PRIVATE civ5_gl)
        add_executable(civ5_gl_asset_audit validation/gl_asset_audit.cpp)
        target_link_libraries(civ5_gl_asset_audit PRIVATE civ5_gl bcrypt)
        add_executable(civ5_texture_sampling validation/texture_sampling.cpp)
        add_executable(civ5_rgba8_mip_sampling validation/rgba8_mip_sampling.cpp)
        target_link_libraries(civ5_rgba8_mip_sampling PRIVATE civ5_gl d3d9 d3dcompiler)
        add_executable(civ5_water_lean_oracle validation/water_lean_oracle.cpp)
        target_link_libraries(civ5_water_lean_oracle PRIVATE civ5_gl d3d9 d3dcompiler)
        if(MINGW)
            target_link_options(civ5_water_lean_oracle PRIVATE -municode)
        endif()
        target_link_libraries(civ5_texture_sampling PRIVATE civ5_gl d3d9 d3dcompiler)
        add_executable(civ5_gl_readback_control validation/gl_readback_control.cpp)
        target_link_libraries(civ5_gl_readback_control PRIVATE civ5_gl)
        add_test(NAME graphics.dds_upload_plan COMMAND civ5_gl_texture_tests)
        add_executable(civ5_dds_native_info validation/dds_native_info.cpp)
        target_link_libraries(civ5_dds_native_info PRIVATE civ5_assets)
        add_executable(civ5_asset_audit validation/asset_audit.cpp)
        target_link_libraries(civ5_asset_audit PRIVATE civ5_assets bcrypt)
    endif()
    add_executable(civ5_terrain_tests tests/height_addressing_tests.cpp)
    target_link_libraries(civ5_terrain_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.height_addressing COMMAND civ5_terrain_tests)
    add_executable(civ5_world_land_tests tests/world_land_tests.cpp)
    target_link_libraries(civ5_world_land_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.world_land_owner COMMAND civ5_world_land_tests)
    add_executable(civ5_terrain_source_grid_tests tests/terrain_source_grid_tests.cpp)
    target_link_libraries(civ5_terrain_source_grid_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.source_grid COMMAND civ5_terrain_source_grid_tests)
    add_executable(civ5_terrain_source_rivers_tests tests/terrain_source_rivers_tests.cpp)
    target_link_libraries(civ5_terrain_source_rivers_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.source_rivers COMMAND civ5_terrain_source_rivers_tests)
    add_executable(civ5_terrain_auxiliary_tests tests/terrain_auxiliary_tests.cpp)
    target_link_libraries(civ5_terrain_auxiliary_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.auxiliary COMMAND civ5_terrain_auxiliary_tests)
    add_executable(civ5_terrain_auxiliary_owners_tests tests/terrain_auxiliary_owners_tests.cpp)
    target_link_libraries(civ5_terrain_auxiliary_owners_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.auxiliary_owners COMMAND civ5_terrain_auxiliary_owners_tests)
    add_executable(civ5_ice_source_weight_probe tests/ice_source_weight_port_probe.cpp)
    target_link_libraries(civ5_ice_source_weight_probe PRIVATE civ5_terrain)
    set(CIVIS_ICE_WEIGHT_FIXTURE "" CACHE FILEPATH "Optional local ice-weight capture; not distributed")
    if(EXISTS "${CIVIS_ICE_WEIGHT_FIXTURE}")
        add_test(NAME terrain.ice_source_weights_control
            COMMAND civ5_ice_source_weight_probe --controlled "${CIVIS_ICE_WEIGHT_FIXTURE}")
    endif()
    add_executable(civ5_terrain_marsh_tests tests/terrain_marsh_tests.cpp)
    target_link_libraries(civ5_terrain_marsh_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.marsh COMMAND civ5_terrain_marsh_tests)
    add_executable(civ5_terrain_decal_updates_tests tests/terrain_decal_updates_tests.cpp)
    target_link_libraries(civ5_terrain_decal_updates_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.decal_updates COMMAND civ5_terrain_decal_updates_tests)
    add_executable(civ5_terrain_decal_assets_tests tests/terrain_decal_assets_tests.cpp)
    target_link_libraries(civ5_terrain_decal_assets_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.decal_assets COMMAND civ5_terrain_decal_assets_tests)
    add_executable(civ5_terrain_decal_geometry_tests tests/terrain_decal_geometry_tests.cpp)
    target_link_libraries(civ5_terrain_decal_geometry_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.decal_geometry COMMAND civ5_terrain_decal_geometry_tests)
    add_executable(civ5_coast_decal_paths_tests tests/coast_decal_paths_tests.cpp)
    target_link_libraries(civ5_coast_decal_paths_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.coast_decal_paths COMMAND civ5_coast_decal_paths_tests)
    add_executable(civ5_terrain_road_grid_tests tests/terrain_road_grid_tests.cpp)
    target_link_libraries(civ5_terrain_road_grid_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.road_grid COMMAND civ5_terrain_road_grid_tests)
    add_executable(civ5_scene_command_queue_tests tests/scene_command_queue_tests.cpp)
    target_link_libraries(civ5_scene_command_queue_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.scene_command_queue COMMAND civ5_scene_command_queue_tests)
    add_executable(civ5_coast_road_network_tests tests/coast_road_network_tests.cpp)
    target_link_libraries(civ5_coast_road_network_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.coast_road_network COMMAND civ5_coast_road_network_tests)
    add_executable(civ5_scene_mask_tests tests/scene_mask_tests.cpp)
    target_link_libraries(civ5_scene_mask_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.scene_mask COMMAND civ5_scene_mask_tests)
    add_executable(civ5_polygon_mask_tests tests/polygon_mask_tests.cpp)
    target_link_libraries(civ5_polygon_mask_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.polygon_mask COMMAND civ5_polygon_mask_tests)
    add_executable(civ5_scene_direction_tests tests/scene_direction_tests.cpp)
    target_link_libraries(civ5_scene_direction_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.scene_direction COMMAND civ5_scene_direction_tests)
    add_executable(civ5_scene_polygon_decals_tests tests/scene_polygon_decals_tests.cpp)
    target_link_libraries(civ5_scene_polygon_decals_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.scene_polygon_decals COMMAND civ5_scene_polygon_decals_tests)
    add_executable(civ5_scene_polygon_chunks_tests tests/scene_polygon_chunks_tests.cpp)
    target_link_libraries(civ5_scene_polygon_chunks_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.scene_polygon_chunks COMMAND civ5_scene_polygon_chunks_tests)
    add_executable(civ5_scene_polygon_owner_tests tests/scene_polygon_owner_tests.cpp)
    target_link_libraries(civ5_scene_polygon_owner_tests PRIVATE civ5_terrain)
    add_executable(civ5_scene_polygon_plot_store_tests tests/scene_polygon_plot_store_tests.cpp)
    target_link_libraries(civ5_scene_polygon_plot_store_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.scene_polygon_plot_store COMMAND civ5_scene_polygon_plot_store_tests)
    add_executable(civ5_plot_graphics_state_tests tests/plot_graphics_state_tests.cpp)
    target_link_libraries(civ5_plot_graphics_state_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.plot_graphics_state COMMAND civ5_plot_graphics_state_tests)
    add_executable(civ5_saved_river_network_tests tests/saved_river_network_tests.cpp)
    target_link_libraries(civ5_saved_river_network_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.saved_river_network COMMAND civ5_saved_river_network_tests)
    add_executable(civ5_saved_map_capacity_tests validation/saved_map_capacity_tests.cpp)
    target_link_libraries(civ5_saved_map_capacity_tests PRIVATE civ5_terrain)
    add_test(NAME terrain.saved_map_capacity COMMAND civ5_saved_map_capacity_tests)
    add_executable(civ5_saved_coast_geometry_tests validation/saved_coast_geometry_tests.cpp)
    target_link_libraries(civ5_saved_coast_geometry_tests PRIVATE civ5_terrain)
    add_executable(civ5_terrain_preparation_tests validation/terrain_preparation_tests.cpp)
    target_link_libraries(civ5_terrain_preparation_tests PRIVATE civ5_terrain)
    add_executable(civ5_terrain_preparation_world_tests validation/terrain_preparation_world_tests.cpp)
    target_link_libraries(civ5_terrain_preparation_world_tests PRIVATE civ5_terrain)
    if(MINGW)
        target_link_options(civ5_saved_coast_geometry_tests PRIVATE -municode)
        target_link_options(civ5_terrain_preparation_tests PRIVATE -municode)
        target_link_options(civ5_terrain_preparation_world_tests PRIVATE -municode)
    endif()
    add_executable(civ5_height_addressing_fixture validation/height_addressing_fixture.cpp)
    target_link_libraries(civ5_height_addressing_fixture PRIVATE civ5_terrain)
    add_executable(civ5_world_land_oracle validation/world_land_oracle.cpp)
    target_link_libraries(civ5_world_land_oracle PRIVATE civ5_terrain)
endif()

if(BUILD_TESTING AND WIN32 AND TARGET civis_runtime)
    add_executable(civ5_extended_label_gdi_tests tests/extended_label_gdi_tests.cpp
        validation/extended_label_gdi.cpp)
    target_link_libraries(civ5_extended_label_gdi_tests PRIVATE civis_runtime gdi32 bcrypt)
    if(MINGW)
        target_link_options(civ5_extended_label_gdi_tests PRIVATE -static)
    endif()
    add_test(NAME graphics.extended_label_gdi COMMAND civ5_extended_label_gdi_tests)
endif()


if(BUILD_TESTING AND WIN32 AND TARGET civis_runtime AND TARGET civ5_gl AND TARGET civ5_scene_saved_fog)
    add_executable(civ5_minimap_fog_gl_consumer_tests tests/minimap_fog_gl_consumer_tests.cpp)
    target_include_directories(civ5_minimap_fog_gl_consumer_tests PRIVATE validation)
    target_compile_features(civ5_minimap_fog_gl_consumer_tests PRIVATE cxx_std_20)
    target_link_libraries(civ5_minimap_fog_gl_consumer_tests PRIVATE
        civ5_gl civis_runtime civ5_bnw_deferred_fog_client civ5_scene_saved_fog)
    if(MINGW)
        target_link_options(civ5_minimap_fog_gl_consumer_tests PRIVATE -static)
    endif()
    if(CIV5_BNW_INSTALL_ROOT)
        add_test(NAME graphics.minimap_fog_gl_consumer COMMAND civ5_minimap_fog_gl_consumer_tests
            "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(graphics.minimap_fog_gl_consumer PROPERTIES RUN_SERIAL TRUE)
    endif()
endif()

# Isolated original FontShader program validation, not native gameplay/Label readiness.
if(BUILD_TESTING AND WIN32 AND TARGET civis_runtime AND TARGET civ5_gl)
    add_executable(civ5_font_shader_original_program_pin_tests
        tests/font_shader_original_program_pin_tests.cpp validation/font_shader_dx9_oracle.cpp)
    target_include_directories(civ5_font_shader_original_program_pin_tests PRIVATE validation)
    target_compile_features(civ5_font_shader_original_program_pin_tests PRIVATE cxx_std_20)
    target_link_libraries(civ5_font_shader_original_program_pin_tests PRIVATE
        civis_runtime d3d9 opengl32 user32)
    add_executable(civ5_font_shader_pixel_runs_tests tests/font_shader_pixel_runs_tests.cpp)
    target_include_directories(civ5_font_shader_pixel_runs_tests PRIVATE validation)
    target_compile_features(civ5_font_shader_pixel_runs_tests PRIVATE cxx_std_20)
    add_test(NAME graphics.font_shader_pixel_runs COMMAND civ5_font_shader_pixel_runs_tests
        "${CMAKE_CURRENT_BINARY_DIR}/font-shader-pixel-run-fixtures")
    if(CIV5_BNW_INSTALL_ROOT)
        add_test(NAME graphics.font_shader_original_program_pin
            COMMAND civ5_font_shader_original_program_pin_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    # Manual GPU/device gate only: actualSave rulesRoot originalGameRoot uniqueOutputDir.
    add_executable(civ5_font_shader_original_oracle_tests
        tests/gl_bnw_font_shader_oracle_tests.cpp validation/font_shader_dx9_oracle.cpp)
    target_include_directories(civ5_font_shader_original_oracle_tests PRIVATE validation)
    target_compile_features(civ5_font_shader_original_oracle_tests PRIVATE cxx_std_20)
    target_link_libraries(civ5_font_shader_original_oracle_tests PRIVATE
        civ5_gl civis_runtime d3d9)
    # Manual source-format9 device gate. No automatic GPU CTest and no live
    # native Label producer claim. SAME actualSave/rules/gameRoot/newOutput args.
    add_executable(civ5_font_shader_packed_oracle_tests
        tests/gl_bnw_font_shader_packed_oracle_tests.cpp validation/font_shader_dx9_oracle.cpp)
    target_include_directories(civ5_font_shader_packed_oracle_tests PRIVATE validation)
    target_compile_features(civ5_font_shader_packed_oracle_tests PRIVATE cxx_std_20)
    target_link_libraries(civ5_font_shader_packed_oracle_tests PRIVATE civ5_gl civis_runtime d3d9)
    if(MINGW)
        target_link_options(civ5_font_shader_packed_oracle_tests PRIVATE -static)
    endif()
    if(MINGW)
        target_link_options(civ5_font_shader_original_program_pin_tests PRIVATE -static)
        target_link_options(civ5_font_shader_pixel_runs_tests PRIVATE -static)
        target_link_options(civ5_font_shader_original_oracle_tests PRIVATE -static)
    endif()
endif()


# Source-bound decoded Build progress transform; no native session publication.
if(BUILD_TESTING AND TARGET civis_runtime AND TARGET civ5_terrain)
    add_executable(civis_bnw_plot_build_progress_diagnostic_tests
        tests/bnw_plot_build_progress_diagnostic_tests.cpp)
    target_link_libraries(civis_bnw_plot_build_progress_diagnostic_tests PRIVATE civis_runtime civ5_terrain)
    target_compile_features(civis_bnw_plot_build_progress_diagnostic_tests PRIVATE cxx_std_20)
    if(MINGW)
        target_link_options(civis_bnw_plot_build_progress_diagnostic_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_plot_build_progress_diagnostic
        COMMAND civis_bnw_plot_build_progress_diagnostic_tests)
endif()

# Explicit immutable preview rules capability: exact canonical identity/refusal gates.
if(BUILD_TESTING AND TARGET civis_runtime)
    add_executable(civis_immutable_rules_fingerprint_tests tests/immutable_rules_fingerprint_tests.cpp)
    target_link_libraries(civis_immutable_rules_fingerprint_tests PRIVATE civis_runtime)
    target_compile_features(civis_immutable_rules_fingerprint_tests PRIVATE cxx_std_20)
    if(MINGW)
        target_link_options(civis_immutable_rules_fingerprint_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.immutable_rules_fingerprint COMMAND civis_immutable_rules_fingerprint_tests)
endif()


# Manual pure CPU geometry gate against an independently original-produced binary.
# Arguments: reference.bin referenceSHA256 NEW report.json. No automatic GPU run.
if(BUILD_TESTING AND WIN32 AND TARGET civ5_gl)
    add_executable(civ5_extended_label_geometry_cross_tests
        validation/extended_label_geometry_cross_tests.cpp
        src/graphics/bnw_extended_label_glyph_geometry.cpp)
    target_include_directories(civ5_extended_label_geometry_cross_tests PRIVATE include validation)
    target_compile_features(civ5_extended_label_geometry_cross_tests PRIVATE cxx_std_20)
    target_link_libraries(civ5_extended_label_geometry_cross_tests PRIVATE bcrypt)
    if(MINGW)
        target_compile_options(civ5_extended_label_geometry_cross_tests PRIVATE -fno-fast-math -ffp-contract=off)
        target_link_options(civ5_extended_label_geometry_cross_tests PRIVATE -municode -static)
    endif()
endif()


# Native hidden-window source/focus publication regression; no GPU or UI callback claim.
if(BUILD_TESTING AND WIN32 AND TARGET civis_runtime)
    add_executable(civis_win32_navigation_publication_tests
        tests/win32_navigation_publication_tests.cpp)
    target_link_libraries(civis_win32_navigation_publication_tests PRIVATE civis_runtime)
    target_compile_features(civis_win32_navigation_publication_tests PRIVATE cxx_std_20)
    if(MINGW)
        target_link_options(civis_win32_navigation_publication_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.win32_navigation_publication
        COMMAND civis_win32_navigation_publication_tests)
    set_tests_properties(runtime.win32_navigation_publication PROPERTIES RUN_SERIAL TRUE)
endif()

# Original57byte cursor replay consumes independently produced reference/SHA/report args.
if(BUILD_TESTING AND WIN32 AND TARGET civ5_gl)
    add_executable(civ5_extended_label_cursor_cross_tests
        validation/extended_label_cursor_cross_tests.cpp
        src/graphics/bnw_extended_label_glyph_cursor.cpp)
    target_include_directories(civ5_extended_label_cursor_cross_tests PRIVATE include validation)
    target_compile_features(civ5_extended_label_cursor_cross_tests PRIVATE cxx_std_20)
    target_link_libraries(civ5_extended_label_cursor_cross_tests PRIVATE bcrypt)
    if(MINGW)
        target_compile_options(civ5_extended_label_cursor_cross_tests PRIVATE -fno-fast-math -ffp-contract=off)
        target_link_options(civ5_extended_label_cursor_cross_tests PRIVATE -municode -static)
    endif()
endif()

# Append bounded own measurement producer to existing graphics library.
if(TARGET civ5_gl)
    target_sources(civ5_gl PRIVATE src/graphics/bnw_worldview_system_title_run.cpp)
endif()
if(TARGET civ5_gl_material_world AND BUILD_TESTING)
    target_sources(civ5_gl_material_world PRIVATE tests/bnw_worldview_system_title_run_live_gates.cpp)
    target_compile_definitions(civ5_gl_material_world PRIVATE CIV5_WORLDVIEW_SYSTEM_TITLE_MEASURE_GATES=1)
endif()
if(TARGET civ5_gl AND TARGET civis_runtime AND BUILD_TESTING AND WIN32)
    add_executable(civ5_bnw_worldview_system_title_measure_rules_tests tests/bnw_worldview_system_title_measure_rules_tests.cpp)
    target_link_libraries(civ5_bnw_worldview_system_title_measure_rules_tests PRIVATE civ5_gl civis_runtime)
    target_compile_features(civ5_bnw_worldview_system_title_measure_rules_tests PRIVATE cxx_std_20)
    if(MINGW)
        target_link_options(civ5_bnw_worldview_system_title_measure_rules_tests PRIVATE -static)
    endif()
    add_test(NAME graphics.bnw_worldview_system_title_measure_rules COMMAND civ5_bnw_worldview_system_title_measure_rules_tests)
endif()

# Standalone private native loose-walk comparison regression; no actual install/playback.
if(BUILD_TESTING AND WIN32 AND TARGET civis_runtime)
    add_executable(civis_bnw_native_loose_streaming_tests
        tests/bnw_native_loose_streaming_tests.cpp)
    target_include_directories(civis_bnw_native_loose_streaming_tests PRIVATE include)
    target_link_libraries(civis_bnw_native_loose_streaming_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_native_loose_streaming_tests PRIVATE cxx_std_20)
    if(MINGW)
        target_link_options(civis_bnw_native_loose_streaming_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_native_loose_streaming COMMAND civis_bnw_native_loose_streaming_tests)
    set_tests_properties(runtime.bnw_native_loose_streaming PROPERTIES RUN_SERIAL TRUE)
endif()

# Initial physical renderer/window lease, separate from native UI acceptance.
if(TARGET civ5_gl)
    target_sources(civ5_gl PRIVATE src/graphics/bnw_owned_camera_renderer_window_lease.cpp)
    target_link_libraries(civ5_gl PRIVATE comctl32)
endif()

# Reciprocal dependency for the real Window TU's retained viewport/binding.
# Peer owns runtime -> LINK_ONLY:civ5_gl and the actual WGL test registrations.
# Both STATIC targets exist here; CMake resolves their normal archive cycle.
if(WIN32 AND TARGET civis_runtime AND TARGET civ5_gl)
    target_link_libraries(civ5_gl PRIVATE civis_runtime)
endif()

# Complete the reviewed static archive cycle after both targets exist.
# Keep one renderer/window implementation in civ5_gl.
if(WIN32 AND TARGET civis_runtime AND TARGET civ5_gl)
    target_link_libraries(civis_runtime PRIVATE $<LINK_ONLY:civ5_gl>)
    foreach(bnw_gl_join_target IN ITEMS
            civis_bnw_selected_stock_lua_owner_join_tests
            civis_bnw_civilopedia_native_runtime_join_tests)
        if(TARGET ${bnw_gl_join_target})
            target_link_libraries(${bnw_gl_join_target} PRIVATE civ5_gl)
        endif()
    endforeach()
endif()

# Opt-in actual selected-client research checkpoint/reload validation fixture.
# This does not register a standalone UNAVAILABLE diagnostic as a passing test.
if(BUILD_TESTING AND WIN32 AND TARGET civ5_gl_material_world)
    target_sources(civ5_gl_material_world PRIVATE tests/bnw_research_choice_restart_fixture.cpp)
    target_include_directories(civ5_gl_material_world PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    target_compile_definitions(civ5_gl_material_world PRIVATE CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE)
endif()

if(BUILD_TESTING AND WIN32 AND TARGET civis_runtime)
    add_executable(civis_bnw_research_checkpoint_restart_verifier
        tests/bnw_research_choice_restart_fixture.cpp
        tests/bnw_research_checkpoint_restart_verifier.cpp)
    target_include_directories(civis_bnw_research_checkpoint_restart_verifier PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    target_link_libraries(civis_bnw_research_checkpoint_restart_verifier PRIVATE civis_runtime)
    target_compile_features(civis_bnw_research_checkpoint_restart_verifier PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_research_checkpoint_restart_verifier PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_research_checkpoint_restart_verifier PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    # Root invokes two fresh processes after the genuine action producer has
    # written and pinned its reference. No synthetic checkpoint CTest gate.
endif()

# Root BNW pending visual runtime geometry linkage.
if(WIN32)
    target_link_libraries(civis_runtime PRIVATE civ5_bnw_unit_visuals)
endif()

# Root: coherent engine UI, policy processing and initial center renderer.
if(WIN32 AND TARGET civis_runtime AND TARGET civ5_gl)
    target_sources(civis_runtime PRIVATE
        src/ui/bnw_engine_ui_turn_owner.cpp
        src/runtime/bnw_policy_adoption.cpp
        src/graphics/bnw_initial_center_unit_renderer_loan.cpp)
endif()

# Tests real selected text and icon resources; no GUI/native parity claim.
if(BUILD_TESTING AND WIN32 AND TARGET civis_runtime)
    add_executable(civis_bnw_tech_award_plain_label_layout_tests
        tests/bnw_tech_award_plain_label_layout_tests.cpp)
    target_link_libraries(civis_bnw_tech_award_plain_label_layout_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_tech_award_plain_label_layout_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_tech_award_plain_label_layout_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_tech_award_plain_label_layout_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/Assets/UI/Fonts/Tw Cent MT/TwCenMT20.ggxml"
            AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/Assets/Gameplay/XML/GameInfo/CIV5IconFontMapping.xml")
        add_test(NAME runtime.bnw_tech_award_plain_label_layout
            COMMAND civis_bnw_tech_award_plain_label_layout_tests
            "${CIV5_BNW_INSTALL_ROOT}/Assets/UI/Fonts/Tw Cent MT/TwCenMT20.ggxml"
            "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_tech_award_plain_label_layout PROPERTIES
            TIMEOUT 60 RUN_SERIAL TRUE)
    endif()
endif()

# Root: real typed gameplay/renderer queues and standalone diagnostic manager.
if(WIN32 AND TARGET civis_runtime AND TARGET civ5_gl)
    target_sources(civis_runtime PRIVATE
        src/graphics/bnw_initial_unit_should_dim_renderer_loan.cpp
        src/graphics/bnw_unit_should_dim_transport.cpp
        src/runtime/bnw_owned_gameplay_event_pump_loan.cpp
        src/ui/bnw_engine_ui_end_turn_queue.cpp
        src/runtime/bnw_net_message_debug_manager_owner.cpp
        src/ui/bnw_audio_sound_data_owner.cpp
        src/ui/bnw_audio_native_pcm_device_owner.cpp
        src/ui/bnw_audio_constructor_rng_owner.cpp
        src/ui/civis_standalone_audio_owner.cpp
        src/ui/civis_client_audio_queue_owner.cpp)
endif()

# Root: standalone menu and genuine content/installation entry points.
add_subdirectory(frontend)


# Root proposal: genuine initial-unit renderer loan and typed transport.
# This block follows civ5_gl creation; runtime/ is configured before that target.
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8 AND TARGET civis_runtime AND TARGET civ5_gl)
    target_sources(civis_runtime PRIVATE
        src/graphics/bnw_initial_unit_setup_graphical_renderer_loan.cpp
        src/graphics/bnw_unit_setup_graphical_transport.cpp)

    # Callable acceptance helpers; no standalone policy test or readiness claim.
    if(BUILD_TESTING)
        add_library(civis_bnw_policy_acceptance_helpers STATIC
            tests/bnw_policy_adoption_fixture.cpp
            tests/bnw_actual_policy_reformation_resource_probe.cpp)
        target_link_libraries(civis_bnw_policy_acceptance_helpers PUBLIC civis_runtime)
        target_include_directories(civis_bnw_policy_acceptance_helpers PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/tests")
        target_compile_features(civis_bnw_policy_acceptance_helpers PUBLIC cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(civis_bnw_policy_acceptance_helpers PRIVATE
                -Wall -Wextra -Wconversion -Werror -UNDEBUG
                -fno-fast-math -ffp-contract=off)
        elseif(MSVC)
            target_compile_options(civis_bnw_policy_acceptance_helpers PRIVATE
                /fp:strict /UNDEBUG)
        endif()
    endif()
endif()


# Genuine existing renderer implementation for runtime consumers.
# Keep its normal renderer compilation separate from Unit test warning flags.
if(WIN32 AND TARGET civ5_gl AND TARGET civis_runtime)
    add_library(civ5_ui_screen_renderer_shared STATIC validation/ui_screen_gl.cpp)
    target_link_libraries(civ5_ui_screen_renderer_shared PUBLIC
        civ5_gl civis_runtime d3d9 d3dcompiler bcrypt shlwapi shell32)
    target_compile_features(civ5_ui_screen_renderer_shared PUBLIC cxx_std_20)
    target_link_libraries(civis_runtime PRIVATE $<LINK_ONLY:civ5_ui_screen_renderer_shared>)
    # These consumers reach the actual UI metadata through the system-font CPU owner.
    foreach(_civis_system_font_link_consumer IN ITEMS
            civis_bnw_system_font_glyph_index_owner_tests
            civ5_gl_bnw_system_font_atlas_tests
            civ5_gl_bnw_font_shader_tests)
        if(TARGET ${_civis_system_font_link_consumer})
            target_link_libraries(${_civis_system_font_link_consumer} PRIVATE
                civ5_ui_screen_renderer_shared)
        endif()
    endforeach()
    if(TARGET civis_bnw_native_unit_allocator_core_tests)
        target_link_libraries(civis_bnw_native_unit_allocator_core_tests PRIVATE
            civ5_ui_screen_renderer_shared)
    endif()
    if(TARGET civis_bnw_tech_tree_lifecycle_tests)
        target_link_libraries(civis_bnw_tech_tree_lifecycle_tests PRIVATE
            civ5_ui_screen_renderer_shared)
    endif()
    if(TARGET civis_bnw_client_scene_selected_host_tests)
        target_link_libraries(civis_bnw_client_scene_selected_host_tests PRIVATE
            civ5_ui_screen_renderer_shared)
    endif()
    if(TARGET civis_script_ui_control_graph_tests)
        target_link_libraries(civis_script_ui_control_graph_tests PRIVATE
            civ5_ui_screen_renderer_shared)
    endif()
    if(TARGET civis_bnw_selected_stock_lua_bootstrap_tests)
        target_link_libraries(civis_bnw_selected_stock_lua_bootstrap_tests PRIVATE
            civ5_ui_screen_renderer_shared)
    endif()
    if(TARGET civis_rules_tests)
        target_link_libraries(civis_rules_tests PRIVATE
            civ5_ui_screen_renderer_shared)
    endif()
endif()

# Actual own-bank audio test follows civ5_gl and the shared UI renderer.
# runtime/ precedes those targets, so its conditional registration is skipped.
if(BUILD_TESTING AND WIN32 AND TARGET civis_runtime AND TARGET civ5_gl
        AND NOT TARGET civis_bank_audio_actual_tests)
    add_executable(civis_bank_audio_actual_tests tests/civis_bank_audio_actual_tests.cpp)
    target_link_libraries(civis_bank_audio_actual_tests PRIVATE
        civis_runtime civ5_gl civ5_ui_screen_renderer_shared)
    target_compile_features(civis_bank_audio_actual_tests PRIVATE cxx_std_20)
    set_target_properties(civis_bank_audio_actual_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/runtime")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(civis_bank_audio_actual_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
    endif()
    if(MINGW)
        target_link_options(civis_bank_audio_actual_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe"
            AND IS_DIRECTORY "${CIV5_ASSETS_ROOT}")
        if(NOT "${CIV5_BNW_TEST_UI_LANGUAGE}" STREQUAL "")
            add_test(NAME ui.civis_bank_audio_actual COMMAND civis_bank_audio_actual_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}" "${CIV5_ASSETS_ROOT}"
                --ui-language "${CIV5_BNW_TEST_UI_LANGUAGE}")
            set_tests_properties(ui.civis_bank_audio_actual PROPERTIES TIMEOUT 1200 RUN_SERIAL TRUE)
        else()
            message(STATUS "ui.civis_bank_audio_actual requires explicit CIV5_BNW_TEST_UI_LANGUAGE; test not registered")
        endif()
    endif()
endif()

# Root: coherent military44, serial45 and prepublication checkpoint cohort.
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8 AND TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE
        src/runtime/bnw_military_request_checkpoint.cpp
        src/runtime/bnw_unit_serial_checkpoint.cpp)
endif()
if(BUILD_TESTING AND WIN32 AND TARGET civis_runtime
        AND TARGET civ5_ui_screen_renderer_shared)
    foreach(_bnw_checkpoint military_request unit_serial)
        set(_bnw_target "civis_bnw_${_bnw_checkpoint}_checkpoint_tests")
        add_executable(${_bnw_target} tests/bnw_${_bnw_checkpoint}_checkpoint_tests.cpp)
        target_link_libraries(${_bnw_target} PRIVATE
            civis_runtime civ5_ui_screen_renderer_shared)
        target_compile_features(${_bnw_target} PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(${_bnw_target} PRIVATE
                -Wall -Wextra -Wconversion -Werror -UNDEBUG
                -fno-fast-math -ffp-contract=off)
        endif()
        set_target_properties(${_bnw_target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/runtime")
        if(MINGW)
            target_link_options(${_bnw_target} PRIVATE
                -static -Wl,--strip-debug -Wl,--stack,8388608)
        endif()
    endforeach()
    add_executable(civis_import_prepublication_tests tests/civis_import_prepublication_tests.cpp)
    target_link_libraries(civis_import_prepublication_tests PRIVATE civis_runtime)
    target_compile_features(civis_import_prepublication_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(civis_import_prepublication_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
    endif()
    set_target_properties(civis_import_prepublication_tests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/runtime")
    if(MINGW)
        target_link_options(civis_import_prepublication_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
endif()

# PRIVATE proposal: append AFTER actual applied Root Save45 registrations.
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8 AND TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE
        src/runtime/bnw_population_totals_checkpoint.cpp
        src/runtime/bnw_food_population_checkpoint_capture.cpp
        src/runtime/bnw_food_population_growth_checkpoint.cpp
        src/runtime/bnw_food_growth_import_admission.cpp)
endif()
if(BUILD_TESTING AND WIN32 AND TARGET civis_runtime AND TARGET civ5_ui_screen_renderer_shared)
    foreach(_bnw_checkpoint population_totals food_population_growth)
        set(_bnw_target "civis_bnw_${_bnw_checkpoint}_checkpoint_tests")
        add_executable(${_bnw_target} tests/bnw_${_bnw_checkpoint}_checkpoint_tests.cpp)
        target_link_libraries(${_bnw_target} PRIVATE civis_runtime civ5_ui_screen_renderer_shared)
        target_compile_features(${_bnw_target} PRIVATE cxx_std_20)
        set_target_properties(${_bnw_target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/runtime")
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(${_bnw_target} PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
        endif()
        if(MINGW)
            target_link_options(${_bnw_target} PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
        endif()
    endforeach()
    add_executable(civis_bnw_food_population_checkpoint_capture_tests tests/bnw_food_population_checkpoint_capture_tests.cpp)
    target_link_libraries(civis_bnw_food_population_checkpoint_capture_tests PRIVATE civis_runtime civ5_ui_screen_renderer_shared)
    target_compile_features(civis_bnw_food_population_checkpoint_capture_tests PRIVATE cxx_std_20)
    set_target_properties(civis_bnw_food_population_checkpoint_capture_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/runtime")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(civis_bnw_food_population_checkpoint_capture_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_food_population_checkpoint_capture_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
endif()

# Root proposal: frozen V6 notification physical receiver/producer sources.
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8 AND TARGET civis_runtime AND TARGET civ5_gl)
    target_sources(civis_runtime PRIVATE
        src/ui/bnw_notification_graphics_publication.cpp
        src/ui/bnw_owned_notification_minimap_receiver.cpp
        src/ui/bnw_notification_authored_art_present_leaf.cpp
        src/ui/bnw_notification_minimap_record_queue.cpp
        src/ui/bnw_owned_notification_plot_fx_receiver.cpp
        src/ui/bnw_notification_terrain_scene_loan.cpp)
endif()

# Root proposal: genuine counted BNW notification model owner and PlayerDirty issuer.
if(WIN32 AND TARGET civis_runtime AND TARGET civ5_gl)
    target_sources(civis_runtime PRIVATE
        src/ui/bnw_owned_notification_publication_owner.cpp
        src/runtime/bnw_player_turn_active_dirty_issuer.cpp)
endif()

# Root: bounded Growth47 gate executables require the existing WorldView renderer definition.
if(BUILD_TESTING AND WIN32 AND TARGET civ5_ui_screen_renderer_shared)
    foreach(_bnw_growth47_renderer_gate
            civis_import_prepublication_tests
            civis_bnw_unit_checkpoint_tests
            civis_bnw_live_player_core_export_tests
            civis_bnw_live_city_export_tests
            civis_runtime_tests
    )
        if(TARGET ${_bnw_growth47_renderer_gate})
            target_link_libraries(${_bnw_growth47_renderer_gate} PRIVATE
                civ5_ui_screen_renderer_shared)
        endif()
    endforeach()
endif()

# Root proposal: reviewed canonical Notification CPU geometry and scroll update loan.
if(WIN32 AND TARGET civis_runtime AND TARGET civ5_gl)
    target_sources(civis_runtime PRIVATE
        src/ui/bnw_scroll_animation_update_loan.cpp
        src/ui/detail/notification_cpu_geometry.cpp)
endif()

# Genuine original ProcessedAutoMoves64 / returned native attrition prerequisites.
target_sources(civis_runtime PRIVATE src/runtime/bnw_processed_auto_moves_import_owner.cpp)
add_executable(civis_bnw_dynamic_turn_mode_tests tests/bnw_dynamic_turn_mode_tests.cpp)
target_link_libraries(civis_bnw_dynamic_turn_mode_tests PRIVATE civis_runtime civ5_ui_screen_renderer_shared)
add_library(civis_bnw_processed_attrition_natural_probes STATIC
  tests/bnw_processed_auto_moves_natural_probe.cpp
  tests/bnw_attrition_mountain_natural_probe.cpp)
target_link_libraries(civis_bnw_processed_attrition_natural_probes PRIVATE civis_runtime civ5_ui_screen_renderer_shared)

# Bounded actual UnitDamage owned enqueue-return; physical model/present remain OPEN.
target_sources(civis_runtime PRIVATE src/graphics/bnw_unit_damage_command_owner.cpp)

# Root: retained canonical UI/parser/health providers for the current partial cut.
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8 AND TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE
        src/runtime/detail/bnw_worldview_canvas_xml_metadata.cpp
        src/ui/detail/notification_layout_baseline.cpp
        src/ui/detail/notification_layout_scroll_receiver.cpp
        src/ui/notification_layout_parent_receiver.cpp
        src/ui/detail/notification_layout_image_state.cpp
        src/ui/detail/notification_layout_runtime_dimensions.cpp
    )
endif()
