# Include this directory after the runtime and graphics targets. Frontend code
# owns no modifications to the existing world/compositor/launch implementations.
if(NOT TARGET civis_runtime)
    return()
endif()

target_sources(civis_runtime PRIVATE
    ../src/runtime/startup_loading.cpp
    ../src/ui/frontend_startup.cpp
    ../src/ui/frontend_sources.cpp
    ../src/ui/frontend_menu.cpp
    ../src/ui/frontend_content.cpp
    ../src/ui/frontend_save_discovery.cpp
    ../src/ui/frontend_existing.cpp)

if(WIN32 AND TARGET civ5_gl)
    target_sources(civis_runtime PRIVATE
        ../src/ui/detail/civis_frontend_party_bank_handoff.cpp
        ../src/ui/detail/civis_frontend_party_scene_source.cpp
        ../src/ui/detail/civis_frontend_party_routing_owner.cpp
        ../src/ui/detail/civis_frontend_party_world_source.cpp
        ../src/ui/detail/civis_saved_world_loading_owner.cpp
        ../src/ui/detail/civis_frontend_party_material_storage.cpp
        ../src/ui/detail/civis_basic_water_surface.cpp
        ../src/ui/detail/frontend_party_authored_lighting.cpp
        ../src/ui/detail/frontend_party_authored_lighting_source26.cpp
        ../src/ui/frontend_party_alpha_land_layers.cpp)
    # Runtime saved-world and normal Camera consumers share these implementations.
    target_link_libraries(civis_runtime PRIVATE
        $<LINK_ONLY:civ5_scene_saved_sources>
        $<LINK_ONLY:civ5_bnw_deferred_fog_client>)
    add_executable(civis_frontend ../runtime/frontend_main.cpp
        ../validation/gl_bnw_warrior_retained.cpp)
    target_include_directories(civis_frontend PRIVATE ../validation)
    target_link_libraries(civis_frontend PRIVATE civis_runtime civ5_gl
        civ5_ui_screen_renderer_shared civ5_bnw_fog_map_metadata civ5_bnw_unit_visuals
        civ5_scene_saved_sources civ5_scene_saved_fog civ5_bnw_deferred_fog_client
        comctl32 d3d9 d3dcompiler bcrypt shlwapi shell32 ole32 uuid)
    if(MINGW)
        target_link_options(civis_frontend PRIVATE -static)
    endif()
endif()

if(BUILD_TESTING)
    foreach(module IN ITEMS menu sources content existing process_backend save_discovery startup)
        add_executable(civis_frontend_${module}_tests ../tests/frontend_${module}_tests.cpp)
        target_link_libraries(civis_frontend_${module}_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_frontend_${module}_tests PRIVATE -static)
        endif()
    endforeach()
    add_test(NAME runtime.frontend_startup COMMAND civis_frontend_startup_tests)
    add_test(NAME runtime.frontend_process_backend COMMAND civis_frontend_process_backend_tests)
    add_test(NAME runtime.frontend_save_discovery COMMAND civis_frontend_save_discovery_tests)
    add_test(NAME runtime.frontend_menu COMMAND civis_frontend_menu_tests ${CIV5_ASSETS_ROOT})
    add_test(NAME runtime.frontend_sources COMMAND civis_frontend_sources_tests ${CIV5_ASSETS_ROOT})
    if(IS_DIRECTORY "${CIV5_ASSETS_ROOT}" AND EXISTS "${CIV5_BNW_SELECTED_SAVE}")
        add_test(NAME runtime.frontend_content COMMAND civis_frontend_content_tests
            "${CIV5_ASSETS_ROOT}" "${CIV5_BNW_SELECTED_SAVE}")
    else()
        add_test(NAME runtime.frontend_content COMMAND civis_frontend_content_tests)
    endif()
    add_test(NAME runtime.frontend_existing COMMAND civis_frontend_existing_tests ${CIV5_ASSETS_ROOT})
endif()
