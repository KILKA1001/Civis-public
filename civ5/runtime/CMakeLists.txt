# Runtime integration lives in this subdirectory to keep concurrent graphics
# work independent. Both targets use the same civ5_assets library.
# Existing Windows builds already pin clang++; use the C driver from that
# same toolchain when no C compiler/toolchain override was supplied.
if(NOT DEFINED CMAKE_C_COMPILER AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    string(REPLACE "clang++" "clang" _civis_c_driver "${CMAKE_CXX_COMPILER}")
    if(EXISTS "${_civis_c_driver}")
        set(CMAKE_C_COMPILER "${_civis_c_driver}")
    endif()
endif()

enable_language(C)
add_library(civis_lua51 STATIC
    ../third_party/lua51/lapi.c ../third_party/lua51/lcode.c
    ../third_party/lua51/ldebug.c ../third_party/lua51/ldo.c
    ../third_party/lua51/ldump.c ../third_party/lua51/lfunc.c
    ../third_party/lua51/lgc.c ../third_party/lua51/llex.c
    ../third_party/lua51/lmem.c ../third_party/lua51/lobject.c
    ../third_party/lua51/lopcodes.c ../third_party/lua51/lparser.c
    ../third_party/lua51/lstate.c ../third_party/lua51/lstring.c
    ../third_party/lua51/ltable.c ../third_party/lua51/ltm.c
    ../third_party/lua51/lundump.c ../third_party/lua51/lvm.c
    ../third_party/lua51/lzio.c ../third_party/lua51/lauxlib.c
    ../third_party/lua51/lbaselib.c ../third_party/lua51/ltablib.c
    ../third_party/lua51/lstrlib.c ../third_party/lua51/lmathlib.c
    ../third_party/lua51/ldblib.c ../third_party/lua51/loslib.c)
target_include_directories(civis_lua51 PUBLIC ../third_party/lua51)
# Windows long is 32-bit even in an x64 process. Lua integer formatting must
# use long long so wide IDs and counters do not pass through a 32-bit cast.
target_compile_definitions(civis_lua51 PUBLIC LUA_USELONGLONG)
add_library(civis_lzma STATIC
    ../third_party/lzma/7zArcIn.c ../third_party/lzma/7zBuf.c ../third_party/lzma/7zBuf2.c
    ../third_party/lzma/7zDec.c ../third_party/lzma/7zDeflate.c
    ../third_party/lzma/7zCrc.c ../third_party/lzma/7zCrcOpt.c
    ../third_party/lzma/7zFile.c ../third_party/lzma/7zStream.c
    ../third_party/lzma/Bcj2.c ../third_party/lzma/Bra.c ../third_party/lzma/Bra86.c
    ../third_party/lzma/BraIA64.c ../third_party/lzma/CpuArch.c ../third_party/lzma/Delta.c
    ../third_party/lzma/Lzma2Dec.c ../third_party/lzma/LzmaDec.c
    ../third_party/lzma/Ppmd7.c ../third_party/lzma/Ppmd7Dec.c)
target_include_directories(civis_lzma PUBLIC ../third_party/lzma)
target_include_directories(civis_lzma PRIVATE ../third_party/zlib)
target_compile_definitions(civis_lzma PRIVATE Z7_PPMD_SUPPORT Z7_EXTRACT_ONLY CIVIS_LZMA_PORTABLE_BYTE_ACCESS)
add_library(civis_sqlite STATIC ../third_party/sqlite/sqlite3.c)
target_include_directories(civis_sqlite PUBLIC ../third_party/sqlite)
target_compile_definitions(civis_sqlite PRIVATE SQLITE_OMIT_LOAD_EXTENSION=1 SQLITE_THREADSAFE=1)
add_library(civis_zlib STATIC
    ../third_party/zlib/adler32.c ../third_party/zlib/crc32.c
    ../third_party/zlib/inflate.c ../third_party/zlib/inffast.c
    ../third_party/zlib/inftrees.c ../third_party/zlib/zutil.c)
target_include_directories(civis_zlib PUBLIC ../third_party/zlib)
target_link_libraries(civis_lzma PRIVATE civis_zlib)
add_library(civis_runtime STATIC
    ../src/runtime/bnw_city_food_base_yield.cpp
    ../src/runtime/bnw_city_current_food_consumption.cpp
    ../src/runtime/bnw_city_food_production_conversion.cpp
    ../src/runtime/bnw_retained_turn_cursor.cpp
    ../src/runtime/bnw_city_food_turn.cpp
    ../src/runtime/bnw_city_food_production_order.cpp
    ../src/runtime/bnw_game_turn_timer_owner.cpp
    ../src/runtime/bnw_plot_terrain_checkpoint_pair.cpp
    ../src/runtime/bnw_plot_yield_refresh.cpp
    ../src/runtime/bnw_city_food_consumption.cpp
    ../src/runtime/bnw_city_food_scalars.cpp
    ../src/runtime/bnw_grand_logging_continuation.cpp
    ../src/ui/native_label_color_pack.cpp
    ../src/ui/bnw_native_label_field_storage.cpp
    ../src/ui/bnw_label_process_module_owner.cpp
    ../src/graphics/bnw_font_texture_observation.cpp
    ../src/ui/bnw_native_root_context_fields.cpp
    ../src/runtime/bnw_plot_build_progress_diagnostic.cpp
    ../src/runtime/lua_script_host.cpp
    ../src/runtime/civis_lua_table_cardinality.cpp
    ../src/runtime/lua_treasury_services.cpp
    ../src/runtime/pregame_roster.cpp
    ../src/runtime/saved_value_fallback.cpp
    ../src/content/lua_assets.cpp
    ../src/runtime/binary_reader.cpp
    ../src/runtime/simulation_random.cpp
    ../src/saves/save_header.cpp
    ../src/saves/pregame.cpp
    ../src/saves/chunked_stream.cpp
    ../src/saves/save_payload.cpp
    ../src/saves/loaded_save.cpp
    ../src/saves/session_snapshot.cpp
    ../src/saves/civis_save.cpp
    ../src/saves/civis_metadata.cpp
    ../src/saves/vp_prefix.cpp
    ../src/saves/expansion2_prefix.cpp
    ../src/saves/game_state.cpp
    ../src/saves/game_progression.cpp
    ../src/saves/game_history.cpp
    ../src/saves/game_religion_culture.cpp
    ../src/saves/game_leagues.cpp
    ../src/saves/game_trade.cpp
    ../src/saves/game_tail.cpp
    ../src/saves/map_state.cpp
    ../src/saves/map_regions.cpp
    ../src/saves/team_state.cpp
    ../src/saves/player_core.cpp
    ../src/saves/player_strategy.cpp
    ../src/saves/player_diplomacy.cpp
    ../src/saves/player_knowledge.cpp
    ../src/saves/player_planning.cpp
    ../src/saves/player_traits.cpp
    ../src/saves/player_espionage.cpp
    ../src/saves/player_trade_culture.cpp
    ../src/saves/player_administration.cpp
    ../src/saves/city_core.cpp
    ../src/saves/city_production.cpp
    ../src/saves/city_management.cpp
    ../src/saves/city_tail.cpp
    ../src/saves/cities.cpp
    ../src/saves/unit_core.cpp
    ../src/saves/unit_tail.cpp
    ../src/saves/units.cpp
    ../src/saves/army_operations.cpp
    ../src/saves/player_tail.cpp
    ../src/saves/all_players.cpp
    ../src/saves/supporting_classes.cpp
    ../src/saves/inflate_stream.cpp)
target_link_libraries(civis_runtime PUBLIC civ5_assets)
target_link_libraries(civis_runtime PRIVATE civis_zlib)
target_link_libraries(civis_runtime PRIVATE civis_sqlite)
target_link_libraries(civis_runtime PRIVATE civis_lzma)
target_link_libraries(civis_runtime PRIVATE civis_lua51)
target_compile_features(civis_runtime PUBLIC cxx_std_20)
set_target_properties(civis_runtime PROPERTIES CXX_EXTENSIONS OFF)
if(WIN32)
    # SQLite's recursive compound SELECT compiler needs more than Windows'
    # default 1 MiB stack for valid mod queries in Debug/UBSan builds. Reserve
    # 8 MiB on runtime consumers; pages are committed on demand. This leaves
    # SQL semantics and SQLite's supported query depth unchanged.
    if(MINGW)
        target_link_options(civis_runtime INTERFACE "LINKER:--stack,8388608")
    elseif(MSVC)
        target_link_options(civis_runtime INTERFACE "LINKER:/STACK:8388608")
    endif()
    target_sources(civ5_assets PRIVATE ../src/assets/resource_mounts.cpp)
    # One lock supplies bootstrap provenance and immutable runtime binary pins.
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../config/inno_decoder.lock.json" _inno_lock)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/../config/inno_decoder.lock.json")
    string(JSON _inno_count LENGTH "${_inno_lock}" binaries)
    math(EXPR _inno_last "${_inno_count} - 1")
    set(_inno_header "struct DecoderPin { const char *name; const char *sha256; };\nstatic constexpr DecoderPin decoder_pins[] = {\n")
    foreach(_index RANGE 0 ${_inno_last})
        string(JSON _name GET "${_inno_lock}" binaries ${_index} name)
        string(JSON _hash GET "${_inno_lock}" binaries ${_index} sha256)
        string(APPEND _inno_header "{\"${_name}\", \"${_hash}\"},\n")
    endforeach()
    string(APPEND _inno_header "};\n")
    file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/inno_decoder_pins.hpp"
        CONTENT "${_inno_header}" @ONLY NEWLINE_STYLE CRLF)
    target_include_directories(civis_runtime PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    # Keep VP GameCore calculations in a separate compilation module. It is
    # embedded in the current static runtime until the host has a module ABI;
    # common save/content services remain independent of this source list.
    add_library(civis_vp_gamecore OBJECT
        ../src/runtime/plot_movement.cpp
        ../src/runtime/vp_plot_passability.cpp
        ../src/runtime/vp_player_research.cpp
        ../src/runtime/vp_research_cost.cpp
        ../src/runtime/vp_research_binding.cpp
        ../src/runtime/vp_research_selection.cpp
        ../src/runtime/vp_research_ai_weights.cpp
        ../src/runtime/vp_research_ai_candidates.cpp
        ../src/runtime/vp_research_ai_choice.cpp
        ../src/runtime/vp_research_checkpoint.cpp
        ../src/runtime/vp_simulation_random.cpp
        ../src/runtime/vp_team_technologies.cpp
        ../src/runtime/vp_science_checkpoint.cpp
        ../src/runtime/vp_vassal_tax_turn.cpp
        ../src/runtime/vp_culture_history_turn.cpp
        ../src/runtime/vp_culture_income.cpp
        ../src/runtime/vp_policy_cost.cpp
        ../src/runtime/vp_policy_phase.cpp
        ../src/runtime/vp_espionage_turn.cpp
        ../src/runtime/vp_attack_operations.cpp
        ../src/runtime/vp_army_construction.cpp
        ../src/runtime/vp_city_construction_intrigue.cpp
        ../src/runtime/vp_diplomacy_intrigue.cpp
        ../src/runtime/vp_travelling_turn.cpp
        ../src/runtime/vp_city_espionage.cpp
        ../src/runtime/vp_espionage_checkpoint.cpp
        ../src/runtime/vp_network_points.cpp
        ../src/runtime/vp_network_point_binding.cpp
        ../src/runtime/vp_gathering_intel_turn.cpp
        ../src/runtime/vp_espionage_player_turn.cpp
        ../src/runtime/vp_schmooze_turn.cpp
        ../src/runtime/vp_anarchy_turn.cpp
        ../src/runtime/vp_city_yield_cache.cpp
        ../src/runtime/vp_nature_yield.cpp
        ../src/runtime/vp_plot_religion.cpp
        ../src/runtime/vp_player_plot_yield.cpp
        ../src/runtime/vp_plot_yield.cpp
        ../src/runtime/vp_city_food.cpp
        ../src/runtime/vp_city_growth.cpp
        ../src/runtime/vp_unit_supply.cpp
        ../src/runtime/vp_great_works.cpp
        ../src/runtime/vp_city_building_yields.cpp
        ../src/runtime/vp_city_religion_minor_yields.cpp
        ../src/runtime/vp_city_trait_yields.cpp
        ../src/runtime/vp_city_happiness.cpp
        ../src/runtime/vp_city_base_yields.cpp
        ../src/runtime/vp_corporations.cpp
        ../src/runtime/vp_city_production.cpp
        ../src/runtime/vp_building_production.cpp
        ../src/runtime/vp_unit_traits.cpp
        ../src/runtime/vp_player_unit_production.cpp
        ../src/runtime/vp_city_unit_production.cpp)
    target_link_libraries(civis_vp_gamecore PRIVATE civ5_assets civis_sqlite civis_lua51)
    target_compile_features(civis_vp_gamecore PRIVATE cxx_std_20)
    set_target_properties(civis_vp_gamecore PROPERTIES CXX_EXTENSIONS OFF)
    target_sources(civis_runtime PRIVATE
        $<TARGET_OBJECTS:civis_vp_gamecore>
        ../src/content/manifest.cpp
        ../src/content/game_data_xml.cpp
        ../src/content/loose_resource_plan.cpp
        ../src/content/base_gameplay.cpp
        ../src/content/rules_bootstrap.cpp
        ../src/content/saved_rules.cpp
        ../src/content/rules_database.cpp
        ../src/content/info_types.cpp
        ../src/content/info_metadata.cpp
        ../src/content/technology_rules.cpp
        ../src/content/custom_mod_options.cpp
        ../src/content/research_rules.cpp
        ../src/runtime/team_technologies.cpp
        ../src/runtime/lua_gameplay_services.cpp
        ../src/runtime/map_geometry.cpp
        ../src/runtime/lua_map_services.cpp
        ../src/runtime/research.cpp
        ../src/runtime/research_context.cpp
        ../src/runtime/player_research.cpp
        ../src/runtime/research_progress.cpp
        ../src/runtime/research_checkpoint.cpp
        ../src/runtime/treasury_checkpoint.cpp
        ../src/runtime/research_turn_checkpoint.cpp
        ../src/runtime/research_selection_checkpoint.cpp
        ../src/runtime/simulation_random_checkpoint.cpp
        ../src/runtime/research_turn.cpp
        ../src/runtime/player_economy_research_stage.cpp
        ../src/runtime/live_session.cpp
        ../src/runtime/bnw_game_read_hooks.cpp
        ../src/runtime/live_action_transaction.cpp
        ../src/runtime/bnw_turn_scheduler.cpp
        ../src/runtime/bnw_map_turn.cpp
        ../src/runtime/bnw_game_clock.cpp
        ../src/runtime/bnw_player_culture_bomb_turn.cpp
        ../src/runtime/bnw_player_conversion_timer_turn.cpp
        ../src/runtime/bnw_player_settled_city_turn.cpp
        ../src/runtime/bnw_player_conscript_turn.cpp
        ../src/runtime/bnw_player_scalar_prefix.cpp
        ../src/runtime/bnw_player_pre_turn.cpp
        ../src/runtime/bnw_player_trade_turn.cpp
        ../src/runtime/bnw_player_trade_strategy_prefix.cpp
        ../src/runtime/bnw_player_cramped_turn.cpp
        ../src/runtime/bnw_player_uprising_turn.cpp
        ../src/runtime/bnw_player_city_revolt_turn.cpp
        ../src/runtime/bnw_player_maya_turn.cpp
        ../src/runtime/bnw_game_clock_checkpoint.cpp
        ../src/runtime/bnw_map_plot_checkpoint.cpp
        ../src/runtime/bnw_unit_checkpoint.cpp
        ../src/runtime/bnw_unit_roster_checkpoint.cpp
        ../src/runtime/bnw_unit_class_death.cpp
        ../src/runtime/bnw_unit_support_death.cpp
        ../src/runtime/bnw_unit_death_xp_quote.cpp
        ../src/runtime/bnw_unit_capture_death_quote.cpp
        ../src/runtime/bnw_unit_kill_frontier.cpp
        ../src/runtime/bnw_unit_kill_entry.cpp
        ../src/runtime/bnw_unit_can_save_buffer.cpp
        ../src/runtime/bnw_unit_can_save_lua_adapter.cpp
        ../src/runtime/bnw_unit_set_xy.cpp
        ../src/runtime/bnw_unit_sight_preflight.cpp
        ../src/runtime/bnw_unit_sight_plan.cpp
        ../src/runtime/bnw_unit_sight_reveal_plan.cpp
        ../src/runtime/bnw_unit_sight_detached.cpp
        ../src/runtime/bnw_unit_sight_reveal_details.cpp
        ../src/runtime/bnw_unit_sight_exploration.cpp
        ../src/runtime/bnw_unit_sight_event_order.cpp
        ../src/runtime/bnw_sight_meet_detached.cpp
        ../src/runtime/bnw_sight_meet_effect_plan.cpp
        ../src/runtime/bnw_team_meet_lua_quote.cpp
        ../src/runtime/bnw_meet_proximity_stage.cpp
        ../src/runtime/bnw_first_contact_rng_stage.cpp
        ../src/runtime/bnw_first_contact_war_decision.cpp
        ../src/runtime/bnw_first_contact_approach_tail.cpp
        ../src/runtime/bnw_game_activation_preflight.cpp
        ../src/runtime/bnw_game_activation_execution.cpp
        ../src/runtime/bnw_player_do_turn_prefix.cpp
        ../src/runtime/bnw_player_do_turn_revolts.cpp
        ../src/runtime/bnw_player_do_turn_trade.cpp
        ../src/runtime/bnw_player_do_turn_military.cpp
        ../src/runtime/bnw_player_grand_military_average.cpp
        ../src/runtime/bnw_grand_world_averages.cpp
        ../src/runtime/bnw_personality_flavors.cpp
        ../src/runtime/bnw_grand_first_priority.cpp
        ../src/runtime/bnw_total_jons_culture.cpp
        ../src/runtime/bnw_grand_culture_priority.cpp
        ../src/runtime/bnw_grand_first_roll.cpp
        ../src/runtime/bnw_grand_united_nations_priority.cpp
        ../src/runtime/bnw_grand_united_nations_category.cpp
        ../src/runtime/bnw_grand_second_roll.cpp
        ../src/runtime/bnw_grand_spaceship_priority.cpp
        ../src/runtime/bnw_grand_spaceship_category.cpp
        ../src/runtime/bnw_grand_third_roll.cpp
        ../src/runtime/bnw_grand_conquest_category.cpp
        ../src/runtime/bnw_grand_fourth_roll.cpp
        ../src/runtime/bnw_grand_pressure.cpp
        ../src/runtime/bnw_grand_selection.cpp
        ../src/ui/bnw_native_original_stat_service.cpp
        ../src/ui/bnw_native_archive_construction.cpp
        ../src/ui/bnw_native_cold_filesystem_owner.cpp
        ../src/ui/bnw_native_archive_registry_source.cpp
        ../src/ui/bnw_native_archive_v6_entry_admission.cpp
        ../src/ui/bnw_native_archive_basename_cache.cpp
        ../src/ui/bnw_native_primary32_loop_journal.cpp
        ../src/ui/bnw_native_primary32_record_snapshot.cpp
        ../src/ui/bnw_native_default_namespace_policy.cpp
        ../src/ui/bnw_native_database_validation_settings.cpp
        ../src/ui/bnw_native_config_dictionary.cpp
        ../src/content/bnw_globals_cache_source_owner.cpp
        ../src/content/bnw_info_type_cache_source.cpp
        ../src/runtime/bnw_grand_guess_empty.cpp
        ../src/runtime/bnw_deferred_fog.cpp
        ../src/runtime/bnw_activation_connections.cpp
        ../src/runtime/bnw_founder_benefits_lua.cpp
        ../src/runtime/bnw_founder_religion_native.cpp
        ../src/runtime/bnw_religion_happiness_terms.cpp
        ../src/runtime/bnw_happiness_resources.cpp
        ../src/runtime/bnw_happiness_natural_wonders.cpp
        ../src/runtime/bnw_happiness_policies.cpp
        ../src/runtime/bnw_happiness_buildings.cpp
        ../src/runtime/bnw_happiness_minors.cpp
        ../src/runtime/bnw_happiness_update_prefix.cpp
        ../src/runtime/bnw_happiness_update_suffix.cpp
        ../src/runtime/bnw_activation_happiness.cpp
        ../src/runtime/bnw_city_strategy_yields.cpp
        ../src/runtime/bnw_city_registry.cpp
        ../src/runtime/bnw_unit_cycle.cpp
        ../src/runtime/bnw_normal_plot_yield.cpp
        ../src/runtime/bnw_activation_builder_tasking.cpp
        ../src/runtime/bnw_selected_stock_lua_bootstrap.cpp
        ../src/runtime/bnw_stock_lua_client_extent.cpp
        ../src/ui/bnw_slider_graphic_cache.cpp
        ../src/ui/bnw_native_slider_setter.cpp
        ../src/ui/bnw_civilopedia_native_initial_graph.cpp
        ../src/ui/bnw_native_ui_color_owner.cpp
        ../src/ui/bnw_native_early_input_lease.cpp
        ../src/ui/bnw_native_package_discovery_owner.cpp
        ../src/ui/bnw_native_user_directory_owner.cpp
        ../src/ui/bnw_native_resource_boot_settings_owner.cpp
        ../src/ui/bnw_native_package_pre_skin_owner.cpp
        ../src/ui/bnw_native_package_archive_time_owner.cpp
        ../src/graphics/bnw_base_unit_member_info_compilation.cpp
        ../src/graphics/bnw_base_unit_art_compilation.cpp
        ../src/graphics/bnw_unit_fx_source_declarations.cpp
        ../src/graphics/bnw_unit_model_resource_request_owner.cpp
        ../src/ui/bnw_selected_native_color_projection.cpp
        ../src/ui/bnw_empty_article_stack_owner.cpp
        ../src/ui/bnw_civilopedia_right_tree_owner.cpp
        ../src/ui/bnw_civilopedia_label_metrics.cpp
        ../src/ui/bnw_client_text_policy.cpp
        ../src/ui/bnw_text_locale_adapter.cpp
        ../src/ui/bnw_system_font_cpu_owner.cpp
        ../src/ui/bnw_system_font_cache_owner.cpp
        ../src/ui/bnw_civilopedia_grid_graphic_cache.cpp
        ../src/runtime/bnw_pathfinder_cache_controls.cpp
        ../src/runtime/bnw_end_turn_request.cpp
        ../src/runtime/bnw_update_moves_prefix.cpp
        ../src/runtime/bnw_update_moves_local_sp.cpp
        ../src/runtime/bnw_busy_state.cpp
  ../src/runtime/bnw_ui_transient_checkpoint.cpp
        ../src/runtime/bnw_ready_units.cpp
        ../src/runtime/bnw_unit_order_owners.cpp
        ../src/runtime/bnw_unit_finish_moves.cpp
        ../src/runtime/bnw_unit_reset.cpp
        ../src/runtime/bnw_player_turn_deactivation.cpp
        ../src/runtime/bnw_notification_cleanup.cpp
        ../src/runtime/bnw_diplomacy_turn_end.cpp
        ../src/runtime/bnw_unit_movement_query.cpp
        ../src/runtime/bnw_sight_tech_share.cpp
        ../src/runtime/bnw_sight_diplomacy_preflight.cpp
        ../src/runtime/bnw_sight_research_clear.cpp
        ../src/runtime/bnw_sight_first_contact.cpp
        ../src/runtime/bnw_first_contact_military_preflight.cpp
        ../src/runtime/bnw_first_contact_economic_preflight.cpp
        ../src/runtime/bnw_first_contact_posture_preflight.cpp
        ../src/runtime/bnw_first_contact_expansion_preflight.cpp
        ../src/runtime/bnw_first_contact_target_preflight.cpp
        ../src/runtime/bnw_first_contact_relationship_preflight.cpp
        ../src/runtime/bnw_first_contact_approach_preflight.cpp
        ../src/runtime/bnw_first_contact_approach_after_load.cpp
        ../src/runtime/bnw_first_contact_approach_random_quote.cpp
        ../src/runtime/bnw_first_contact_approach_war_gate.cpp
        ../src/runtime/bnw_can_declare_war_lua.cpp
        ../src/runtime/bnw_detached_gameplay_lua.cpp
        ../src/runtime/bnw_diplomacy_scratchpad_load.cpp
        ../src/runtime/bnw_notification_load_transients.cpp
        ../src/runtime/bnw_player_plot_list_load.cpp
        ../src/runtime/bnw_player_read_tail_transients.cpp
        ../src/runtime/bnw_game_plot_extras_load.cpp
        ../src/runtime/bnw_belief_availability.cpp
        ../src/runtime/bnw_player_proximity_checkpoint.cpp
        ../src/runtime/bnw_team_meet_checkpoint.cpp
        ../src/runtime/bnw_exploration_dirty_checkpoint.cpp
        ../src/runtime/bnw_player_strategy_checkpoint.cpp
        ../src/runtime/bnw_player_core_checkpoint.cpp
        ../src/runtime/bnw_live_player_core_export.cpp
        ../src/runtime/bnw_live_player_state_export.cpp
        ../src/runtime/bnw_city_core_checkpoint.cpp
        ../src/runtime/bnw_city_production_checkpoint.cpp
        ../src/runtime/bnw_city_management_checkpoint.cpp
        ../src/runtime/bnw_city_tail_checkpoint.cpp
        ../src/runtime/bnw_treasury_balance_checkpoint.cpp
        ../src/runtime/bnw_team_progress_checkpoint.cpp
        ../src/runtime/bnw_culture_bomb_timer_checkpoint.cpp
        ../src/runtime/bnw_research_queue_checkpoint.cpp
        ../src/runtime/bnw_maintenance_expense_checkpoint.cpp
        ../src/runtime/bnw_player_knowledge_checkpoint.cpp
        ../src/runtime/bnw_player_knowledge_live_bundle.cpp
        ../src/runtime/bnw_player_administration_live_bundle.cpp
        ../src/runtime/bnw_world_religion_progress.cpp
        ../src/runtime/bnw_barbarians_begin.cpp
        ../src/runtime/bnw_game_supporting_checkpoint.cpp
        ../src/runtime/bnw_game_score_checkpoint.cpp
        ../src/runtime/bnw_game_score.cpp
        ../src/runtime/bnw_game_deals.cpp
        ../src/runtime/bnw_game_teams.cpp
        ../src/runtime/bnw_game_deals_checkpoint.cpp
        ../src/runtime/bnw_game_map_phase.cpp
        ../src/runtime/bnw_team_turn_checkpoint.cpp
        ../src/runtime/bnw_team_turn_pair.cpp
        ../src/runtime/bnw_barbarian_tech_grant.cpp
        ../src/runtime/bnw_barbarian_team_has_tech.cpp
        ../src/runtime/bnw_map_occupied_preflight.cpp
        ../src/runtime/bnw_team_tech_post.cpp
        ../src/runtime/bnw_world_anchor_clear.cpp
        ../src/runtime/bnw_plot_unit_validity_preflight.cpp
        ../src/runtime/bnw_team_tech_route_build.cpp
        ../src/runtime/bnw_world_anchor_graphics_adapter.cpp
        ../src/runtime/bnw_plot_domain_binding.cpp
        ../src/runtime/bnw_team_tech_domain_yields.cpp
        ../src/runtime/bnw_unit_territory_probe.cpp
        ../src/runtime/bnw_team_tech_player_map.cpp
        ../src/runtime/bnw_plot_unit_live_services.cpp
        ../src/runtime/bnw_team_tech_barbarian_traits.cpp
        ../src/runtime/bnw_team_tech_post_traits.cpp
        ../src/runtime/bnw_team_tech_lua_gate.cpp
        ../src/runtime/bnw_team_tech_after_lua_plan.cpp
        ../src/runtime/bnw_team_tech_best_route.cpp
        ../src/runtime/bnw_team_tech_player_research.cpp
        ../src/runtime/bnw_team_tech_share_culture.cpp
        ../src/runtime/bnw_team_tech_suffix_plan.cpp
        ../src/runtime/bnw_team_tech_suffix_callbacks.cpp
        ../src/runtime/bnw_team_tech_reward_ui.cpp
        ../src/runtime/bnw_team_tech_suffix_receipt_checkpoint.cpp
        ../src/runtime/bnw_selected_gameplay_lua_owner.cpp
        ../src/runtime/bnw_unit_visibility_probe.cpp
        ../src/runtime/bnw_unit_jump_detached.cpp
        ../src/runtime/bnw_unit_jump_checkpoint.cpp
        ../src/runtime/bnw_unit_sight_checkpoint.cpp
        ../src/runtime/bnw_unit_jump_composite.cpp
        ../src/runtime/bnw_unit_reveal_details_checkpoint.cpp
        ../src/runtime/bnw_unowned_plot_yield.cpp
        ../src/runtime/bnw_player_administration_checkpoint.cpp
        ../src/runtime/bnw_fresh_archive_values.cpp
        ../src/runtime/bnw_fresh_archive_full.cpp
        ../src/runtime/bnw_fresh_policies_uninit.cpp
        ../src/runtime/bnw_fresh_economic_uninit.cpp
        ../src/runtime/bnw_fresh_military_uninit.cpp
        ../src/runtime/bnw_fresh_city_specialization_uninit.cpp
        ../src/runtime/bnw_fresh_wonder_uninit.cpp
        ../src/runtime/bnw_fresh_grand_strategy_uninit.cpp
        ../src/runtime/bnw_fresh_diplomacy_uninit.cpp
        ../src/runtime/bnw_fresh_religions_uninit.cpp
        ../src/runtime/bnw_fresh_religion_ai_uninit.cpp
        ../src/runtime/bnw_fresh_espionage_uninit.cpp
        ../src/runtime/bnw_fresh_espionage_ai_uninit.cpp
        ../src/runtime/bnw_fresh_trade_uninit.cpp
        ../src/runtime/bnw_fresh_trade_ai_uninit.cpp
        ../src/runtime/bnw_fresh_league_ai_uninit.cpp
        ../src/runtime/bnw_fresh_player_techs_uninit.cpp
        ../src/runtime/bnw_fresh_flavor_manager_uninit.cpp
        ../src/runtime/bnw_fresh_tactical_ai_uninit.cpp
        ../src/runtime/bnw_fresh_mid_ai_uninit.cpp
        ../src/runtime/bnw_fresh_post_connections_uninit.cpp
        ../src/runtime/bnw_fresh_player_tail_containers.cpp
        ../src/runtime/bnw_fresh_player_entities_uninit.cpp
        ../src/runtime/bnw_fresh_player_operations_uninit.cpp
        ../src/runtime/bnw_fresh_player_post_operations.cpp
        ../src/runtime/bnw_fresh_player_post_delta_scalars.cpp
        ../src/runtime/bnw_fresh_player_uninit_prefix.cpp
        ../src/runtime/bnw_player_temporary_bonus_turn.cpp
        ../src/runtime/bnw_live_city_export.cpp
        ../src/runtime/bnw_live_player_city_export.cpp
        ../src/runtime/bnw_live_map_player_city_export.cpp
        ../src/runtime/bnw_live_rng_export.cpp
        ../src/runtime/bnw_area_unit_checkpoint.cpp
        ../src/runtime/bnw_nearby_cities.cpp
        ../src/runtime/bnw_city_flavor_recipients.cpp
        ../src/runtime/bnw_player_flavor_recipients.cpp
        ../src/runtime/bnw_live_plot_impassability.cpp
        ../src/runtime/bnw_adjacent_land.cpp
        ../src/runtime/bnw_live_plot_visibility.cpp
        ../src/runtime/bnw_player_gold.cpp
        ../src/runtime/bnw_deficit_selection.cpp
        ../src/runtime/bnw_deficit_rules.cpp
        ../src/runtime/bnw_deficit_scrap.cpp
        ../src/runtime/bnw_deficit_scrap_rules.cpp
        ../src/runtime/bnw_deficit_live_binding.cpp
        ../src/runtime/bnw_military_defense.cpp
        ../src/runtime/bnw_military_base_units.cpp
        ../src/runtime/bnw_military_settler_demand.cpp
        ../src/runtime/bnw_delayed_death.cpp
        ../src/runtime/bnw_unit_kill_lua.cpp
        ../src/runtime/bnw_player_research.cpp
        ../src/runtime/bnw_research_choice.cpp
        ../src/runtime/bnw_production_order.cpp
        ../src/runtime/bnw_fresh_game_content.cpp
        ../src/runtime/bnw_map_script.cpp
        ../src/runtime/bnw_fresh_yield.cpp
        ../src/runtime/bnw_fractal.cpp
        ../src/runtime/bnw_plot_type_transition.cpp
        ../src/runtime/bnw_setup_plot_grid.cpp
        ../src/runtime/bnw_city_work.cpp
        ../src/runtime/bnw_fresh_terrain.cpp
        ../src/runtime/bnw_fresh_plot_setter.cpp
        ../src/runtime/bnw_fresh_feature_setter.cpp
        ../src/runtime/bnw_fresh_feature_events.cpp
        ../src/runtime/bnw_fresh_feature_effects.cpp
        ../src/runtime/bnw_fresh_start_coast.cpp
        ../src/runtime/bnw_fresh_start_resources.cpp
        ../src/runtime/bnw_fresh_start_luxuries.cpp
        ../src/runtime/bnw_fresh_start_players.cpp
        ../src/runtime/bnw_fresh_pregame_roster.cpp
        ../src/runtime/bnw_fresh_player_colors.cpp
        ../src/runtime/bnw_fresh_player_init_prefix.cpp
        ../src/runtime/bnw_fresh_alive_prefix.cpp
        ../src/runtime/bnw_fresh_diplo_votes.cpp
        ../src/runtime/bnw_fresh_league_state.cpp
        ../src/runtime/bnw_fresh_team_init_prefix.cpp
        ../src/runtime/bnw_fresh_team_info_init.cpp
        ../src/runtime/bnw_fresh_player_reset_prefix.cpp
        ../src/runtime/bnw_fresh_player_info_reset.cpp
        ../src/runtime/bnw_fresh_economic_ai_init.cpp
        ../src/runtime/bnw_fresh_military_ai_init.cpp
        ../src/runtime/bnw_fresh_city_specialization_init.cpp
        ../src/runtime/bnw_fresh_game_builder.cpp
        ../src/runtime/bnw_fresh_first_player_state.cpp
        ../src/runtime/bnw_fresh_builder_player_diagnostic.cpp
        ../src/runtime/bnw_fresh_archive_vector.cpp
        ../src/runtime/bnw_fresh_slot_info.cpp
        ../src/runtime/bnw_fresh_player_objects.cpp
        ../src/runtime/bnw_fresh_player_uninit_plan.cpp
        ../src/runtime/research_selection.cpp
        ../src/runtime/research_ai_weights.cpp
        ../src/runtime/research_ai_candidates.cpp
        ../src/runtime/research_ai_choice.cpp
        ../src/runtime/city_yields.cpp
        ../src/runtime/production_progress.cpp
        ../src/runtime/production_checkpoint.cpp
        ../src/runtime/bnw_unhappiness.cpp
        ../src/runtime/maintenance.cpp
        ../src/runtime/route_economy.cpp
        ../src/runtime/plot_water.cpp
        ../src/runtime/yield_fraction.cpp
        ../src/runtime/improvement_yields.cpp
        ../src/runtime/improvement_yield_calculation.cpp
        ../src/runtime/bnw_nature_yield.cpp
        ../src/runtime/resource_visibility.cpp
        ../src/runtime/treasury.cpp
        ../src/runtime/treasury_turn.cpp
        ../src/runtime/science.cpp
        ../src/content/global_defines.cpp
        ../src/content/mod_activation.cpp
        ../src/content/mod_script_registry.cpp
        ../src/content/save_dimensions.cpp
        ../src/content/saved_culture.cpp
        ../src/content/saved_terrain.cpp
        ../src/content/mod_archive.cpp
        ../src/content/mod_zip_archive.cpp
        ../src/content/mod_acquisition.cpp
        ../src/ui/top_panel.cpp
        ../src/ui/top_panel_live.cpp
        ../src/ui/bnw_top_panel_preview_bridge.cpp
        ../src/ui/bnw_feature_art_tag_read_owner.cpp
        ../src/ui/bnw_resource_art_tag_read_owner.cpp
        ../src/ui/bnw_whole_raw_label_input.cpp
        ../src/ui/bnw_unit_panel_preview_bridge.cpp
        ../src/ui/world_view.cpp
        ../src/ui/top_panel_tooltip.cpp
        ../src/ui/minimap_panel.cpp
        ../src/ui/bnw_minimap_fog_preparation.cpp
        ../src/ui/bnw_tech_tree_lifecycle.cpp
        ../src/ui/minimap_ui.cpp
        ../src/ui/ui_runtime.cpp
        ../src/ui/live_ui_host.cpp
        ../src/ui/live_session_frame.cpp
        ../src/ui/bnw_client_frame.cpp
        ../src/ui/client_window_input.cpp
        ../src/ui/win32_client_window.cpp
        ../src/ui/civis_measured_ui_lifecycle.cpp
        ../src/ui/civis_measured_civilopedia_controls.cpp
        ../src/ui/civis_measured_civilopedia_lua.cpp
        ../src/ui/civis_civilopedia_category_callbacks.cpp
        ../src/ui/selected_civilopedia_locale.cpp
        ../src/ui/civis_selected_civilopedia_text_data_owner.cpp
        ../src/ui/civis_locale_reference_text.cpp
        ../src/ui/bnw_info_default_form.cpp
        ../src/ui/bnw_unit_panel_secondary_stats.cpp
        ../src/ui/bnw_unit_panel_stat_rows.cpp
        ../src/ui/native_extended_font_style.cpp
        ../src/ui/civis_selected_game_option_owner.cpp
        ../src/ui/civis_civilopedia_units_query_owner.cpp
        ../src/ui/civis_locale_text_owner.cpp
        ../src/ui/bnw_city_banner_appearance.cpp
        ../src/ui/civis_civilopedia_input_owner.cpp
        ../src/ui/civis_civilopedia_tooltip_owner.cpp
        ../src/ui/ui_texture_asset.cpp
        ../src/ui/glyphgen_style.cpp
        ../src/ui/selected_bnw_preview_host.cpp
        ../src/ui/selected_bnw_research_preview.cpp
        ../src/ui/selected_bnw_research_lua_inventory.cpp
        ../src/ui/selected_bnw_stock_ui_lua_step.cpp
        ../src/ui/selected_bnw_icon_support_step.cpp
        ../src/ui/selected_bnw_civilopedia_preamble.cpp
        ../src/ui/selected_bnw_civilopedia_layout.cpp
        ../src/ui/bnw_civilopedia_scroll_subtree.cpp
        ../src/ui/bnw_civilopedia_scrollbar_owner.cpp
        ../src/ui/bnw_civilopedia_initial_content.cpp
        ../src/ui/civis_civilopedia_geometry.cpp
        ../src/ui/civis_civilopedia_articles.cpp
        ../src/ui/civis_civilopedia_measured_hit.cpp
        ../src/ui/bnw_live_preview_launch.cpp
        ../src/ui/bnw_city_production_backend.cpp
        ../src/ui/saved_session_frame.cpp
        ../src/ui/screen_compositor.cpp
        ../src/ui/ui_audio_events.cpp
        ../src/ui/ui_audio_assets.cpp
        ../src/ui/ui_winmm_audio.cpp
        ../src/ui/live_ui_audio.cpp
        ../src/ui/selected_bnw_ui_audio.cpp
        ../src/ui/progression_screens.cpp
        ../src/ui/ideology_layout.cpp
        ../src/ui/ideology_frame_owner.cpp
        ../src/ui/city_screens.cpp
        ../src/ui/overview_screens.cpp
        ../src/ui/economic_overview_tabs.cpp
        ../src/ui/economic_overview_close.cpp
        ../src/ui/economic_overview_close_service.cpp
        ../src/ui/economic_overview_lifecycle.cpp
        ../src/ui/economic_overview_open_bridge.cpp
        ../src/ui/economic_overview_interaction.cpp
        ../src/ui/economic_overview_audio.cpp
        ../src/ui/economic_overview_active_tabs.cpp
        ../src/ui/economic_overview_fields.cpp
        ../src/ui/economic_overview_popup_feed.cpp
        ../src/ui/economic_overview_live_input.cpp
        ../src/ui/economic_overview_gold_input.cpp
        ../src/ui/economic_overview_scene.cpp
        ../src/ui/trade_route_overview_input.cpp
        ../src/ui/trade_route_available_rows.cpp
        ../src/ui/trade_route_instance_plan.cpp
        ../src/ui/new_era_popup_audio.cpp
        ../src/ui/new_era_audio_asset.cpp
        ../src/ui/new_era_audio_delivery.cpp
        ../src/ui/civis_new_era_audio_completion_owner.cpp
        ../src/ui/bnw_new_era_sound_archive.cpp
        ../src/ui/popup_screens.cpp
        ../src/ui/religion_belief_preview.cpp
        ../src/ui/bnw_notification_popup_owner.cpp
        ../src/ui/bnw_selected_notification_backend.cpp
        ../src/ui/bnw_add_popup_queue.cpp
        ../src/ui/system_screens.cpp
        ../src/ui/glyphgen.cpp
        ../src/ui/unit_name_font_selection.cpp
        ../src/content/inno_archive.cpp
        ../src/content/vp_profile.cpp
        ../src/ui/top_panel_saved_state.cpp
        ../src/platform/steam_installation.cpp
        ../src/platform/http_download.cpp
        ../src/platform/http_curl.cpp
        ../src/runtime/file_identity.cpp
        ../src/runtime/save_preflight.cpp)
    # Settings call the existing material implementation. Its terrain compile
    # options belong to that library, rather than every GameCore translation unit.
    target_link_libraries(civis_runtime PRIVATE $<LINK_ONLY:civ5_materials> $<LINK_ONLY:civ5_terrain> d3d9 xmllite shlwapi shell32 ole32 uuid advapi32 bcrypt winhttp winmm user32 gdi32)
    if(MSVC)
        set_property(SOURCE ../src/ui/bnw_native_resource_boot_settings_owner.cpp
            APPEND PROPERTY COMPILE_OPTIONS /fp:strict)
    else()
        set_property(SOURCE ../src/ui/bnw_native_resource_boot_settings_owner.cpp
            APPEND PROPERTY COMPILE_OPTIONS -fno-fast-math -ffp-contract=off)
    endif()
    add_executable(civis_inspect inspect_main.cpp)
    target_link_libraries(civis_inspect PRIVATE civis_runtime)
    add_executable(civis_rules_replay rules_replay_main.cpp)
    target_link_libraries(civis_rules_replay PRIVATE civis_runtime)
    add_executable(civis_saved_rules saved_rules_main.cpp)
    target_link_libraries(civis_saved_rules PRIVATE civis_runtime civ5_terrain)
    add_executable(civis_ice_region_builder ice_region_builder_main.cpp)
    target_link_libraries(civis_ice_region_builder PRIVATE civis_runtime civ5_terrain bcrypt)
    add_executable(civis_payload payload_main.cpp)
    target_link_libraries(civis_payload PRIVATE civis_runtime)
    add_executable(civis_save_convert convert_save_main.cpp)
    target_link_libraries(civis_save_convert PRIVATE civis_runtime)
    add_executable(civis_session_snapshot session_snapshot_main.cpp)
    target_link_libraries(civis_session_snapshot PRIVATE civis_runtime)
    add_executable(civis_package_resources package_resources_main.cpp)
    target_link_libraries(civis_package_resources PRIVATE civis_runtime)
    add_executable(civis_acquire acquire_main.cpp)
    target_link_libraries(civis_acquire PRIVATE civis_runtime)
    add_executable(civis_vp_profile vp_profile_main.cpp)
    target_link_libraries(civis_vp_profile PRIVATE civis_runtime)
    add_executable(civis_lua_smoke lua_smoke_main.cpp)
    target_link_libraries(civis_lua_smoke PRIVATE civis_runtime)
    configure_file(../config/mod_sources.xml mod_sources.xml COPYONLY)
    foreach(target civis_acquire civis_saved_rules)
        add_custom_command(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_BINARY_DIR}/mod_sources.xml $<TARGET_FILE_DIR:${target}>/mod_sources.xml)
    endforeach()
    if(MINGW)
        target_link_options(civis_inspect PRIVATE -municode -static)
        target_link_options(civis_rules_replay PRIVATE -municode -static)
        target_link_options(civis_saved_rules PRIVATE -municode -static)
        target_link_options(civis_payload PRIVATE -municode -static)
        target_link_options(civis_save_convert PRIVATE -municode -static)
        target_link_options(civis_session_snapshot PRIVATE -municode -static)
        target_link_options(civis_package_resources PRIVATE -municode -static)
        target_link_options(civis_acquire PRIVATE -municode -static)
        target_link_options(civis_vp_profile PRIVATE -municode -static)
        target_link_options(civis_lua_smoke PRIVATE -municode -static)
    endif()
endif()
if(BUILD_TESTING AND WIN32)
    add_executable(civis_resource_tests ../tests/resource_mounts_tests.cpp)
    target_link_libraries(civis_resource_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_resource_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.resource_mounts COMMAND civis_resource_tests)
    add_executable(civis_runtime_tests ../tests/runtime_tests.cpp)
    target_link_libraries(civis_runtime_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_runtime_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.save_content COMMAND civis_runtime_tests)
    add_executable(civis_live_session_tests ../tests/live_session_tests.cpp)
    target_link_libraries(civis_live_session_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_live_session_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.live_session COMMAND civis_live_session_tests)
    add_executable(civis_live_action_transaction_tests ../tests/live_action_transaction_tests.cpp)
    target_link_libraries(civis_live_action_transaction_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_live_action_transaction_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.live_action_transaction COMMAND civis_live_action_transaction_tests)
    add_executable(civis_bnw_turn_scheduler_tests ../tests/bnw_turn_scheduler_tests.cpp)
    target_link_libraries(civis_bnw_turn_scheduler_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_turn_scheduler_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_turn_scheduler COMMAND civis_bnw_turn_scheduler_tests)
    add_executable(civis_bnw_map_turn_tests ../tests/bnw_map_turn_tests.cpp)
    target_link_libraries(civis_bnw_map_turn_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_map_turn_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_map_turn COMMAND civis_bnw_map_turn_tests)
    add_executable(civis_bnw_game_clock_tests ../tests/bnw_game_clock_tests.cpp)
    target_link_libraries(civis_bnw_game_clock_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_game_clock_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_game_clock COMMAND civis_bnw_game_clock_tests)
    add_executable(civis_bnw_player_culture_bomb_turn_tests
        ../tests/bnw_player_culture_bomb_turn_tests.cpp)
    target_link_libraries(civis_bnw_player_culture_bomb_turn_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_culture_bomb_turn_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_player_culture_bomb_turn
        COMMAND civis_bnw_player_culture_bomb_turn_tests)
    add_executable(civis_bnw_player_conversion_timer_turn_tests
        ../tests/bnw_player_conversion_timer_turn_tests.cpp)
    target_link_libraries(civis_bnw_player_conversion_timer_turn_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_conversion_timer_turn_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_player_conversion_timer_turn
        COMMAND civis_bnw_player_conversion_timer_turn_tests)
    add_executable(civis_bnw_player_settled_city_turn_tests
        ../tests/bnw_player_settled_city_turn_tests.cpp)
    target_link_libraries(civis_bnw_player_settled_city_turn_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_settled_city_turn_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_player_settled_city_turn
        COMMAND civis_bnw_player_settled_city_turn_tests)
    add_executable(civis_bnw_player_conscript_turn_tests
        ../tests/bnw_player_conscript_turn_tests.cpp)
    target_link_libraries(civis_bnw_player_conscript_turn_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_conscript_turn_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_player_conscript_turn
        COMMAND civis_bnw_player_conscript_turn_tests)
    add_executable(civis_bnw_player_scalar_prefix_tests
        ../tests/bnw_player_scalar_prefix_tests.cpp)
    target_link_libraries(civis_bnw_player_scalar_prefix_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_scalar_prefix_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_player_scalar_prefix
        COMMAND civis_bnw_player_scalar_prefix_tests)
    add_executable(civis_bnw_player_cramped_turn_tests
        ../tests/bnw_player_cramped_turn_tests.cpp)
    target_link_libraries(civis_bnw_player_cramped_turn_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_cramped_turn_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_player_cramped_turn
        COMMAND civis_bnw_player_cramped_turn_tests)
    add_executable(civis_bnw_player_trade_turn_tests
        ../tests/bnw_player_trade_turn_tests.cpp)
    target_link_libraries(civis_bnw_player_trade_turn_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_trade_turn_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_player_trade_turn
        COMMAND civis_bnw_player_trade_turn_tests)
    add_executable(civis_bnw_player_uprising_turn_tests
        ../tests/bnw_player_uprising_turn_tests.cpp)
    target_link_libraries(civis_bnw_player_uprising_turn_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_uprising_turn_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_player_uprising_turn
        COMMAND civis_bnw_player_uprising_turn_tests)
    add_executable(civis_bnw_player_city_revolt_turn_tests
        ../tests/bnw_player_city_revolt_turn_tests.cpp)
    target_link_libraries(civis_bnw_player_city_revolt_turn_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_city_revolt_turn_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_player_city_revolt_turn
        COMMAND civis_bnw_player_city_revolt_turn_tests)
    add_executable(civis_bnw_player_maya_turn_tests
        ../tests/bnw_player_maya_turn_tests.cpp)
    target_link_libraries(civis_bnw_player_maya_turn_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_maya_turn_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_player_maya_turn
        COMMAND civis_bnw_player_maya_turn_tests)
    add_executable(civis_bnw_treasury_balance_checkpoint_tests
        ../tests/bnw_treasury_balance_checkpoint_tests.cpp)
    target_link_libraries(civis_bnw_treasury_balance_checkpoint_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_treasury_balance_checkpoint_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}")
        add_test(NAME runtime.bnw_treasury_balance_checkpoint
            COMMAND civis_bnw_treasury_balance_checkpoint_tests
                ${CIV5_BNW_SELECTED_SAVE})
    endif()
    add_executable(civis_bnw_treasury_live_import_tests
        ../tests/bnw_treasury_live_import_tests.cpp)
    target_link_libraries(civis_bnw_treasury_live_import_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_treasury_live_import_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND
       EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_treasury_live_import
            COMMAND civis_bnw_treasury_live_import_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_bnw_team_progress_checkpoint_tests
        ../tests/bnw_team_progress_checkpoint_tests.cpp)
    target_link_libraries(civis_bnw_team_progress_checkpoint_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_progress_checkpoint_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND
       EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_team_progress_checkpoint
            COMMAND civis_bnw_team_progress_checkpoint_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_bnw_team_progress_live_import_tests
        ../tests/bnw_team_progress_live_import_tests.cpp)
    target_link_libraries(civis_bnw_team_progress_live_import_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_progress_live_import_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND
       EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_team_progress_live_import
            COMMAND civis_bnw_team_progress_live_import_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_bnw_fresh_archive_values_tests
        ../tests/bnw_fresh_archive_values_tests.cpp)
    target_link_libraries(civis_bnw_fresh_archive_values_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_archive_values_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_archive_values
        COMMAND civis_bnw_fresh_archive_values_tests)
    add_executable(civis_bnw_fresh_archive_full_tests
        ../tests/bnw_fresh_archive_full_tests.cpp)
    target_link_libraries(civis_bnw_fresh_archive_full_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_archive_full_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_archive_full
        COMMAND civis_bnw_fresh_archive_full_tests)
    add_executable(civis_bnw_fresh_policies_uninit_tests
        ../tests/bnw_fresh_policies_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_policies_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_policies_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_policies_uninit
        COMMAND civis_bnw_fresh_policies_uninit_tests)
    add_executable(civis_bnw_fresh_economic_uninit_tests
        ../tests/bnw_fresh_economic_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_economic_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_economic_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_economic_uninit
        COMMAND civis_bnw_fresh_economic_uninit_tests)
    add_executable(civis_bnw_fresh_military_uninit_tests
        ../tests/bnw_fresh_military_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_military_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_military_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_military_uninit
        COMMAND civis_bnw_fresh_military_uninit_tests)
    add_executable(civis_bnw_fresh_city_specialization_uninit_tests
        ../tests/bnw_fresh_city_specialization_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_city_specialization_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_city_specialization_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_city_specialization_uninit
        COMMAND civis_bnw_fresh_city_specialization_uninit_tests)
    add_executable(civis_bnw_fresh_wonder_uninit_tests
        ../tests/bnw_fresh_wonder_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_wonder_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_wonder_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_wonder_uninit
        COMMAND civis_bnw_fresh_wonder_uninit_tests)
    add_executable(civis_bnw_fresh_grand_strategy_uninit_tests
        ../tests/bnw_fresh_grand_strategy_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_grand_strategy_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_grand_strategy_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_grand_strategy_uninit
        COMMAND civis_bnw_fresh_grand_strategy_uninit_tests)
    add_executable(civis_bnw_fresh_diplomacy_uninit_tests
        ../tests/bnw_fresh_diplomacy_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_diplomacy_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_diplomacy_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_diplomacy_uninit
        COMMAND civis_bnw_fresh_diplomacy_uninit_tests)
    add_executable(civis_bnw_fresh_religions_uninit_tests
        ../tests/bnw_fresh_religions_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_religions_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_religions_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_religions_uninit
        COMMAND civis_bnw_fresh_religions_uninit_tests)
    add_executable(civis_bnw_fresh_religion_ai_uninit_tests
        ../tests/bnw_fresh_religion_ai_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_religion_ai_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_religion_ai_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_religion_ai_uninit
        COMMAND civis_bnw_fresh_religion_ai_uninit_tests)
    add_executable(civis_bnw_fresh_espionage_uninit_tests
        ../tests/bnw_fresh_espionage_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_espionage_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_espionage_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_espionage_uninit
        COMMAND civis_bnw_fresh_espionage_uninit_tests)
    add_executable(civis_bnw_fresh_espionage_ai_uninit_tests
        ../tests/bnw_fresh_espionage_ai_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_espionage_ai_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_espionage_ai_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_espionage_ai_uninit
        COMMAND civis_bnw_fresh_espionage_ai_uninit_tests)
    add_executable(civis_bnw_fresh_trade_uninit_tests
        ../tests/bnw_fresh_trade_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_trade_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_trade_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_trade_uninit
        COMMAND civis_bnw_fresh_trade_uninit_tests)
    add_executable(civis_bnw_fresh_trade_ai_uninit_tests
        ../tests/bnw_fresh_trade_ai_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_trade_ai_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_trade_ai_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_trade_ai_uninit
        COMMAND civis_bnw_fresh_trade_ai_uninit_tests)
    add_executable(civis_bnw_fresh_league_ai_uninit_tests
        ../tests/bnw_fresh_league_ai_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_league_ai_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_league_ai_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_league_ai_uninit
        COMMAND civis_bnw_fresh_league_ai_uninit_tests)
    add_executable(civis_bnw_fresh_player_techs_uninit_tests
        ../tests/bnw_fresh_player_techs_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_player_techs_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_player_techs_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_player_techs_uninit
        COMMAND civis_bnw_fresh_player_techs_uninit_tests)
    add_executable(civis_bnw_fresh_flavor_manager_uninit_tests
        ../tests/bnw_fresh_flavor_manager_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_flavor_manager_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_flavor_manager_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_flavor_manager_uninit
        COMMAND civis_bnw_fresh_flavor_manager_uninit_tests)
    add_executable(civis_bnw_fresh_tactical_ai_uninit_tests
        ../tests/bnw_fresh_tactical_ai_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_tactical_ai_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_tactical_ai_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_tactical_ai_uninit
        COMMAND civis_bnw_fresh_tactical_ai_uninit_tests)
    add_executable(civis_bnw_fresh_mid_ai_uninit_tests
        ../tests/bnw_fresh_mid_ai_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_mid_ai_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_mid_ai_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_mid_ai_uninit
        COMMAND civis_bnw_fresh_mid_ai_uninit_tests)
    add_executable(civis_bnw_fresh_post_connections_uninit_tests
        ../tests/bnw_fresh_post_connections_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_post_connections_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_post_connections_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_post_connections_uninit
        COMMAND civis_bnw_fresh_post_connections_uninit_tests)
    add_executable(civis_bnw_fresh_player_tail_containers_tests
        ../tests/bnw_fresh_player_tail_containers_tests.cpp)
    target_link_libraries(civis_bnw_fresh_player_tail_containers_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_player_tail_containers_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_player_tail_containers
        COMMAND civis_bnw_fresh_player_tail_containers_tests)
    add_executable(civis_bnw_fresh_player_entities_uninit_tests
        ../tests/bnw_fresh_player_entities_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_player_entities_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_player_entities_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_player_entities_uninit
        COMMAND civis_bnw_fresh_player_entities_uninit_tests)
    add_executable(civis_bnw_fresh_player_operations_uninit_tests
        ../tests/bnw_fresh_player_operations_uninit_tests.cpp)
    target_link_libraries(civis_bnw_fresh_player_operations_uninit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_player_operations_uninit_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_player_operations_uninit
        COMMAND civis_bnw_fresh_player_operations_uninit_tests)
    add_executable(civis_bnw_fresh_player_post_operations_tests
        ../tests/bnw_fresh_player_post_operations_tests.cpp)
    target_link_libraries(civis_bnw_fresh_player_post_operations_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_player_post_operations_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_player_post_operations
        COMMAND civis_bnw_fresh_player_post_operations_tests)
    add_executable(civis_bnw_fresh_player_post_delta_scalars_tests
        ../tests/bnw_fresh_player_post_delta_scalars_tests.cpp)
    target_link_libraries(civis_bnw_fresh_player_post_delta_scalars_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_player_post_delta_scalars_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_player_post_delta_scalars
        COMMAND civis_bnw_fresh_player_post_delta_scalars_tests)
    add_executable(civis_bnw_world_religion_progress_tests
        ../tests/bnw_world_religion_progress_tests.cpp)
    target_link_libraries(civis_bnw_world_religion_progress_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_world_religion_progress_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_world_religion_progress
        COMMAND civis_bnw_world_religion_progress_tests)
    add_executable(civis_bnw_barbarians_begin_tests
        ../tests/bnw_barbarians_begin_tests.cpp)
    target_link_libraries(civis_bnw_barbarians_begin_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_barbarians_begin_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_barbarians_begin
        COMMAND civis_bnw_barbarians_begin_tests)
    add_executable(civis_bnw_game_supporting_checkpoint_tests
        ../tests/bnw_game_supporting_checkpoint_tests.cpp)
    target_link_libraries(civis_bnw_game_supporting_checkpoint_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_game_supporting_checkpoint_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_game_supporting_checkpoint
        COMMAND civis_bnw_game_supporting_checkpoint_tests)
    add_executable(civis_bnw_game_deals_tests ../tests/bnw_game_deals_tests.cpp)
    target_link_libraries(civis_bnw_game_deals_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_game_deals_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_game_deals COMMAND civis_bnw_game_deals_tests)
    add_executable(civis_bnw_game_teams_tests ../tests/bnw_game_teams_tests.cpp)
    target_link_libraries(civis_bnw_game_teams_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_game_teams_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_game_teams COMMAND civis_bnw_game_teams_tests)
    add_executable(civis_bnw_game_deals_checkpoint_tests
        ../tests/bnw_game_deals_checkpoint_tests.cpp)
    target_link_libraries(civis_bnw_game_deals_checkpoint_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_game_deals_checkpoint_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_game_deals_checkpoint
        COMMAND civis_bnw_game_deals_checkpoint_tests)
    add_executable(civis_bnw_game_map_phase_tests
        ../tests/bnw_game_map_phase_tests.cpp)
    target_link_libraries(civis_bnw_game_map_phase_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_game_map_phase_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_game_map_phase
        COMMAND civis_bnw_game_map_phase_tests)
    add_executable(civis_bnw_team_turn_checkpoint_tests
        ../tests/bnw_team_turn_checkpoint_tests.cpp)
    target_link_libraries(civis_bnw_team_turn_checkpoint_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_turn_checkpoint_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_team_turn_checkpoint
        COMMAND civis_bnw_team_turn_checkpoint_tests)
    add_executable(civis_bnw_team_turn_pair_tests
        ../tests/bnw_team_turn_pair_tests.cpp)
    target_link_libraries(civis_bnw_team_turn_pair_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_turn_pair_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_team_turn_pair
        COMMAND civis_bnw_team_turn_pair_tests)
    add_executable(civis_bnw_team_awards_checkpoint_tests
        ../tests/bnw_team_awards_checkpoint_tests.cpp)
    target_link_libraries(civis_bnw_team_awards_checkpoint_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_awards_checkpoint_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_team_awards_checkpoint
        COMMAND civis_bnw_team_awards_checkpoint_tests)
    if(CIV5_BNW_INSTALL_ROOT AND EXISTS "${CIV5_BNW_SELECTED_SAVE}")
        foreach(_team_checkpoint_case team_turn_checkpoint team_turn_pair team_awards_checkpoint)
            add_test(NAME runtime.bnw_${_team_checkpoint_case}_selected
                COMMAND civis_bnw_${_team_checkpoint_case}_tests
                    "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
            set_tests_properties(runtime.bnw_${_team_checkpoint_case}_selected
                PROPERTIES TIMEOUT 1800)
        endforeach()
    endif()
    add_executable(civis_bnw_barbarian_tech_grant_tests
        ../tests/bnw_barbarian_tech_grant_tests.cpp)
    target_link_libraries(civis_bnw_barbarian_tech_grant_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_barbarian_tech_grant_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_barbarian_tech_grant
        COMMAND civis_bnw_barbarian_tech_grant_tests)
    add_executable(civis_bnw_barbarian_team_has_tech_tests
        ../tests/bnw_barbarian_team_has_tech_tests.cpp)
    target_link_libraries(civis_bnw_barbarian_team_has_tech_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_barbarian_team_has_tech_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_barbarian_team_has_tech
        COMMAND civis_bnw_barbarian_team_has_tech_tests)
    add_executable(civis_bnw_map_occupied_preflight_tests
        ../tests/bnw_map_occupied_preflight_tests.cpp)
    target_link_libraries(civis_bnw_map_occupied_preflight_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_map_occupied_preflight_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_team_tech_post_tests
        ../tests/bnw_team_tech_post_tests.cpp)
    target_link_libraries(civis_bnw_team_tech_post_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_tech_post_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_world_anchor_clear_tests
        ../tests/bnw_world_anchor_clear_tests.cpp)
    target_link_libraries(civis_bnw_world_anchor_clear_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_world_anchor_clear_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_world_anchor_clear
        COMMAND civis_bnw_world_anchor_clear_tests)
    add_executable(civis_bnw_plot_unit_validity_preflight_tests
        ../tests/bnw_plot_unit_validity_preflight_tests.cpp)
    target_link_libraries(civis_bnw_plot_unit_validity_preflight_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_plot_unit_validity_preflight_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_team_tech_route_build_tests
        ../tests/bnw_team_tech_route_build_tests.cpp)
    target_link_libraries(civis_bnw_team_tech_route_build_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_tech_route_build_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_world_anchor_graphics_adapter_tests
        ../tests/bnw_world_anchor_graphics_adapter_tests.cpp)
    target_link_libraries(civis_bnw_world_anchor_graphics_adapter_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_world_anchor_graphics_adapter_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_plot_domain_binding_tests
        ../tests/bnw_plot_domain_binding_tests.cpp)
    target_link_libraries(civis_bnw_plot_domain_binding_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_plot_domain_binding_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_team_tech_domain_yields_tests
        ../tests/bnw_team_tech_domain_yields_tests.cpp)
    target_link_libraries(civis_bnw_team_tech_domain_yields_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_tech_domain_yields_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_unit_territory_probe_tests
        ../tests/bnw_unit_territory_probe_tests.cpp)
    target_link_libraries(civis_bnw_unit_territory_probe_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_unit_territory_probe_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_team_tech_player_map_tests
        ../tests/bnw_team_tech_player_map_tests.cpp)
    target_link_libraries(civis_bnw_team_tech_player_map_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_tech_player_map_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_plot_unit_live_services_tests
        ../tests/bnw_plot_unit_live_services_tests.cpp)
    target_link_libraries(civis_bnw_plot_unit_live_services_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_plot_unit_live_services_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_team_tech_barbarian_traits_tests
        ../tests/bnw_team_tech_barbarian_traits_tests.cpp)
    target_link_libraries(civis_bnw_team_tech_barbarian_traits_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_tech_barbarian_traits_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_team_tech_lua_gate_tests
        ../tests/bnw_team_tech_lua_gate_tests.cpp)
    target_link_libraries(civis_bnw_team_tech_lua_gate_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_tech_lua_gate_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_team_tech_post_traits_tests
        ../tests/bnw_team_tech_post_traits_tests.cpp)
    target_link_libraries(civis_bnw_team_tech_post_traits_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_tech_post_traits_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_team_tech_after_lua_plan_tests
        ../tests/bnw_team_tech_after_lua_plan_tests.cpp)
    target_link_libraries(civis_bnw_team_tech_after_lua_plan_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_tech_after_lua_plan_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_team_tech_best_route_tests
        ../tests/bnw_team_tech_best_route_tests.cpp)
    target_link_libraries(civis_bnw_team_tech_best_route_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_tech_best_route_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_team_tech_player_research_tests
        ../tests/bnw_team_tech_player_research_tests.cpp)
    target_link_libraries(civis_bnw_team_tech_player_research_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_tech_player_research_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_team_tech_share_culture_tests
        ../tests/bnw_team_tech_share_culture_tests.cpp)
    target_link_libraries(civis_bnw_team_tech_share_culture_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_tech_share_culture_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_team_tech_suffix_plan_tests
        ../tests/bnw_team_tech_suffix_plan_tests.cpp)
    target_link_libraries(civis_bnw_team_tech_suffix_plan_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_tech_suffix_plan_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_team_tech_suffix_callbacks_tests
        ../tests/bnw_team_tech_suffix_callbacks_tests.cpp)
    target_link_libraries(civis_bnw_team_tech_suffix_callbacks_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_tech_suffix_callbacks_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_team_tech_reward_ui_tests
        ../tests/bnw_team_tech_reward_ui_tests.cpp)
    target_link_libraries(civis_bnw_team_tech_reward_ui_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_tech_reward_ui_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_team_tech_suffix_receipt_checkpoint_tests
        ../tests/bnw_team_tech_suffix_receipt_checkpoint_tests.cpp)
    target_link_libraries(civis_bnw_team_tech_suffix_receipt_checkpoint_tests
        PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_team_tech_suffix_receipt_checkpoint_tests
            PRIVATE -static)
    endif()
    add_executable(civis_bnw_selected_gameplay_lua_owner_tests
        ../tests/bnw_selected_gameplay_lua_owner_tests.cpp)
    target_link_libraries(civis_bnw_selected_gameplay_lua_owner_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_selected_gameplay_lua_owner_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_unit_visibility_probe_tests
        ../tests/bnw_unit_visibility_probe_tests.cpp)
    target_link_libraries(civis_bnw_unit_visibility_probe_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_unit_visibility_probe_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_unit_jump_detached_tests
        ../tests/bnw_unit_jump_detached_tests.cpp)
    target_link_libraries(civis_bnw_unit_jump_detached_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_unit_jump_detached_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_unit_sight_checkpoint_tests
        ../tests/bnw_unit_sight_checkpoint_tests.cpp)
    target_link_libraries(civis_bnw_unit_sight_checkpoint_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_unit_sight_checkpoint_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_unit_jump_composite_tests
        ../tests/bnw_unit_jump_composite_tests.cpp)
    target_link_libraries(civis_bnw_unit_jump_composite_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_unit_jump_composite_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_unit_reveal_details_checkpoint_tests
        ../tests/bnw_unit_reveal_details_checkpoint_tests.cpp)
    target_link_libraries(civis_bnw_unit_reveal_details_checkpoint_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_unit_reveal_details_checkpoint_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_partial_unit_overlay_load_guard_tests
        ../tests/bnw_partial_unit_overlay_load_guard_tests.cpp)
    target_link_libraries(civis_bnw_partial_unit_overlay_load_guard_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_partial_unit_overlay_load_guard_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_diplomacy_scratchpad_civis_tests
        ../tests/bnw_diplomacy_scratchpad_civis_tests.cpp)
    target_link_libraries(civis_bnw_diplomacy_scratchpad_civis_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_diplomacy_scratchpad_civis_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND
       EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        foreach(test_target IN ITEMS
                civis_bnw_team_tech_lua_gate_tests
                civis_bnw_team_tech_post_traits_tests
                civis_bnw_team_tech_after_lua_plan_tests
                civis_bnw_team_tech_best_route_tests
                civis_bnw_team_tech_player_research_tests
                civis_bnw_team_tech_share_culture_tests
                civis_bnw_team_tech_suffix_plan_tests
                civis_bnw_team_tech_suffix_callbacks_tests
                civis_bnw_team_tech_reward_ui_tests
                civis_bnw_team_tech_suffix_receipt_checkpoint_tests
                civis_bnw_selected_gameplay_lua_owner_tests
                civis_bnw_diplomacy_scratchpad_civis_tests)
            add_test(NAME runtime.${test_target} COMMAND ${test_target}
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
        endforeach()
        add_test(NAME runtime.civis_bnw_unit_jump_detached_tests
            COMMAND civis_bnw_unit_jump_detached_tests
                ${CIV5_BNW_SELECTED_SAVE})
        add_test(NAME runtime.civis_bnw_unit_sight_checkpoint_tests
            COMMAND civis_bnw_unit_sight_checkpoint_tests
                ${CIV5_BNW_SELECTED_SAVE})
        add_test(NAME runtime.civis_bnw_unit_jump_composite_tests
            COMMAND civis_bnw_unit_jump_composite_tests
                ${CIV5_BNW_SELECTED_SAVE})
        add_test(NAME runtime.civis_bnw_unit_reveal_details_checkpoint_tests
            COMMAND civis_bnw_unit_reveal_details_checkpoint_tests
                ${CIV5_BNW_SELECTED_SAVE})
        add_test(NAME runtime.civis_bnw_partial_unit_overlay_load_guard_tests
            COMMAND civis_bnw_partial_unit_overlay_load_guard_tests
                ${CIV5_BNW_SELECTED_SAVE})
    endif()
    add_executable(civis_bnw_game_score_checkpoint_tests
        ../tests/bnw_game_score_checkpoint_tests.cpp)
    target_link_libraries(civis_bnw_game_score_checkpoint_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_game_score_checkpoint_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_game_score_checkpoint
        COMMAND civis_bnw_game_score_checkpoint_tests)
    add_executable(civis_bnw_game_score_tests
        ../tests/bnw_game_score_tests.cpp)
    target_link_libraries(civis_bnw_game_score_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_game_score_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_game_score COMMAND civis_bnw_game_score_tests)
    add_executable(civis_bnw_unowned_plot_yield_tests
        ../tests/bnw_unowned_plot_yield_tests.cpp)
    target_link_libraries(civis_bnw_unowned_plot_yield_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_unowned_plot_yield_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND
       EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_unowned_plot_yield
            COMMAND civis_bnw_unowned_plot_yield_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_bnw_fresh_player_uninit_prefix_tests
        ../tests/bnw_fresh_player_uninit_prefix_tests.cpp)
    target_link_libraries(civis_bnw_fresh_player_uninit_prefix_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_player_uninit_prefix_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_player_uninit_prefix
        COMMAND civis_bnw_fresh_player_uninit_prefix_tests)
    add_executable(civis_bnw_player_temporary_bonus_turn_tests
        ../tests/bnw_player_temporary_bonus_turn_tests.cpp)
    target_link_libraries(civis_bnw_player_temporary_bonus_turn_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_temporary_bonus_turn_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_player_temporary_bonus_turn
        COMMAND civis_bnw_player_temporary_bonus_turn_tests)
    add_executable(civis_bnw_culture_bomb_timer_checkpoint_tests
        ../tests/bnw_culture_bomb_timer_checkpoint_tests.cpp)
    target_link_libraries(civis_bnw_culture_bomb_timer_checkpoint_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_culture_bomb_timer_checkpoint_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}")
        add_test(NAME runtime.bnw_culture_bomb_timer_checkpoint
            COMMAND civis_bnw_culture_bomb_timer_checkpoint_tests
                ${CIV5_BNW_SELECTED_SAVE})
    endif()
    add_executable(civis_bnw_culture_bomb_timer_live_import_tests
        ../tests/bnw_culture_bomb_timer_live_import_tests.cpp)
    target_link_libraries(civis_bnw_culture_bomb_timer_live_import_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_culture_bomb_timer_live_import_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND
       EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_culture_bomb_timer_live_import
            COMMAND civis_bnw_culture_bomb_timer_live_import_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_bnw_research_queue_checkpoint_tests
        ../tests/bnw_research_queue_checkpoint_tests.cpp)
    target_link_libraries(civis_bnw_research_queue_checkpoint_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_research_queue_checkpoint_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND
       EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_research_queue_checkpoint
            COMMAND civis_bnw_research_queue_checkpoint_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_bnw_maintenance_expense_checkpoint_tests
        ../tests/bnw_maintenance_expense_checkpoint_tests.cpp)
    target_link_libraries(civis_bnw_maintenance_expense_checkpoint_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_maintenance_expense_checkpoint_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND
       EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_maintenance_expense_checkpoint
            COMMAND civis_bnw_maintenance_expense_checkpoint_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_bnw_player_knowledge_checkpoint_tests
        ../tests/bnw_player_knowledge_checkpoint_tests.cpp)
    target_link_libraries(civis_bnw_player_knowledge_checkpoint_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_knowledge_checkpoint_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND
       EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_player_knowledge_checkpoint
            COMMAND civis_bnw_player_knowledge_checkpoint_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_bnw_player_knowledge_live_bundle_tests
        ../tests/bnw_player_knowledge_live_bundle_tests.cpp)
    target_link_libraries(civis_bnw_player_knowledge_live_bundle_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_knowledge_live_bundle_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND
       EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_player_knowledge_live_bundle
            COMMAND civis_bnw_player_knowledge_live_bundle_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_bnw_player_administration_live_bundle_tests
        ../tests/bnw_player_administration_live_bundle_tests.cpp)
    target_link_libraries(civis_bnw_player_administration_live_bundle_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_administration_live_bundle_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_notification_load_transients_tests
        ../tests/bnw_notification_load_transients_tests.cpp)
    target_link_libraries(civis_bnw_notification_load_transients_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_notification_load_transients_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_player_plot_list_load_tests
        ../tests/bnw_player_plot_list_load_tests.cpp)
    target_link_libraries(civis_bnw_player_plot_list_load_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_plot_list_load_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_player_read_tail_transients_tests
        ../tests/bnw_player_read_tail_transients_tests.cpp)
    target_link_libraries(civis_bnw_player_read_tail_transients_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_read_tail_transients_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_game_plot_extras_load_tests
        ../tests/bnw_game_plot_extras_load_tests.cpp)
    target_link_libraries(civis_bnw_game_plot_extras_load_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_game_plot_extras_load_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND
       EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_game_plot_extras_load
            COMMAND civis_bnw_game_plot_extras_load_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND
       EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_player_read_tail_transients
            COMMAND civis_bnw_player_read_tail_transients_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND
       EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_player_plot_list_load
            COMMAND civis_bnw_player_plot_list_load_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND
       EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_notification_load_transients
            COMMAND civis_bnw_notification_load_transients_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND
       EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_player_administration_live_bundle
            COMMAND civis_bnw_player_administration_live_bundle_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_bnw_player_administration_checkpoint_tests
        ../tests/bnw_player_administration_checkpoint_tests.cpp)
    target_link_libraries(civis_bnw_player_administration_checkpoint_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_administration_checkpoint_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND
       EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_player_administration_checkpoint
            COMMAND civis_bnw_player_administration_checkpoint_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_bnw_composed_checkpoint_loader_tests
        ../tests/bnw_composed_checkpoint_loader_tests.cpp)
    target_link_libraries(civis_bnw_composed_checkpoint_loader_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_composed_checkpoint_loader_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND
       EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_composed_checkpoint_loader
            COMMAND civis_bnw_composed_checkpoint_loader_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_bnw_game_read_hooks_tests ../tests/bnw_game_read_hooks_tests.cpp)
    target_link_libraries(civis_bnw_game_read_hooks_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_game_read_hooks_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_game_read_hooks COMMAND civis_bnw_game_read_hooks_tests)
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}")
        add_executable(civis_bnw_map_plot_checkpoint_tests ../tests/bnw_map_plot_checkpoint_tests.cpp)
        target_link_libraries(civis_bnw_map_plot_checkpoint_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_map_plot_checkpoint_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_map_plot_checkpoint COMMAND civis_bnw_map_plot_checkpoint_tests
            ${CIV5_BNW_SELECTED_SAVE}
            ${CIV5_ASSETS_ROOT})
        add_executable(civis_bnw_unit_checkpoint_tests ../tests/bnw_unit_checkpoint_tests.cpp)
        target_link_libraries(civis_bnw_unit_checkpoint_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_unit_checkpoint_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_unit_checkpoint COMMAND civis_bnw_unit_checkpoint_tests
            ${CIV5_BNW_SELECTED_SAVE})
        add_executable(civis_bnw_exploration_dirty_checkpoint_tests ../tests/bnw_exploration_dirty_checkpoint_tests.cpp)
        target_link_libraries(civis_bnw_exploration_dirty_checkpoint_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_exploration_dirty_checkpoint_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_exploration_dirty_checkpoint COMMAND civis_bnw_exploration_dirty_checkpoint_tests
            ${CIV5_BNW_SELECTED_SAVE})
        add_executable(civis_bnw_player_strategy_checkpoint_tests ../tests/bnw_player_strategy_checkpoint_tests.cpp)
        target_link_libraries(civis_bnw_player_strategy_checkpoint_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_player_strategy_checkpoint_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_player_strategy_checkpoint COMMAND civis_bnw_player_strategy_checkpoint_tests
            ${CIV5_BNW_SELECTED_SAVE})
        add_executable(civis_bnw_player_core_checkpoint_tests ../tests/bnw_player_core_checkpoint_tests.cpp)
        target_link_libraries(civis_bnw_player_core_checkpoint_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_player_core_checkpoint_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_player_core_checkpoint COMMAND civis_bnw_player_core_checkpoint_tests
            ${CIV5_BNW_SELECTED_SAVE})
        add_executable(civis_bnw_live_player_core_export_tests
            ../tests/bnw_live_player_core_export_tests.cpp)
        target_link_libraries(civis_bnw_live_player_core_export_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_live_player_core_export_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_live_player_core_export
            COMMAND civis_bnw_live_player_core_export_tests
            ${CIV5_BNW_SELECTED_SAVE})
        add_executable(civis_bnw_live_player_state_export_tests
            ../tests/bnw_live_player_state_export_tests.cpp)
        target_link_libraries(civis_bnw_live_player_state_export_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_live_player_state_export_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_live_player_state_export
            COMMAND civis_bnw_live_player_state_export_tests
            ${CIV5_BNW_SELECTED_SAVE})
        add_executable(civis_bnw_live_city_export_tests
            ../tests/bnw_live_city_export_tests.cpp)
        target_link_libraries(civis_bnw_live_city_export_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_live_city_export_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_live_city_export
            COMMAND civis_bnw_live_city_export_tests
            ${CIV5_BNW_SELECTED_SAVE})
        add_executable(civis_bnw_live_player_city_export_tests
            ../tests/bnw_live_player_city_export_tests.cpp)
        target_link_libraries(civis_bnw_live_player_city_export_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_live_player_city_export_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_live_player_city_export
            COMMAND civis_bnw_live_player_city_export_tests
            ${CIV5_BNW_SELECTED_SAVE})
        add_executable(civis_bnw_live_map_player_city_export_tests
            ../tests/bnw_live_map_player_city_export_tests.cpp)
        target_link_libraries(civis_bnw_live_map_player_city_export_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_live_map_player_city_export_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_live_rng_export_tests
            ../tests/bnw_live_rng_export_tests.cpp)
        target_link_libraries(civis_bnw_live_rng_export_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_live_rng_export_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_city_core_checkpoint_tests ../tests/bnw_city_core_checkpoint_tests.cpp)
        target_link_libraries(civis_bnw_city_core_checkpoint_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_city_core_checkpoint_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_city_core_checkpoint COMMAND civis_bnw_city_core_checkpoint_tests
            ${CIV5_BNW_SELECTED_SAVE})
        add_executable(civis_bnw_city_production_checkpoint_tests ../tests/bnw_city_production_checkpoint_tests.cpp)
        target_link_libraries(civis_bnw_city_production_checkpoint_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_city_production_checkpoint_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_city_production_checkpoint COMMAND civis_bnw_city_production_checkpoint_tests
            ${CIV5_BNW_SELECTED_SAVE})
        add_executable(civis_bnw_city_management_checkpoint_tests ../tests/bnw_city_management_checkpoint_tests.cpp)
        target_link_libraries(civis_bnw_city_management_checkpoint_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_city_management_checkpoint_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_city_management_checkpoint COMMAND civis_bnw_city_management_checkpoint_tests
            ${CIV5_BNW_SELECTED_SAVE})
        add_executable(civis_bnw_city_tail_checkpoint_tests ../tests/bnw_city_tail_checkpoint_tests.cpp)
        target_link_libraries(civis_bnw_city_tail_checkpoint_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_city_tail_checkpoint_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_city_tail_checkpoint COMMAND civis_bnw_city_tail_checkpoint_tests
            ${CIV5_BNW_SELECTED_SAVE})
        add_executable(civis_bnw_unit_set_xy_tests ../tests/bnw_unit_set_xy_tests.cpp)
        target_link_libraries(civis_bnw_unit_set_xy_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_unit_set_xy_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_unit_set_xy COMMAND civis_bnw_unit_set_xy_tests
            ${CIV5_BNW_SELECTED_SAVE})
        add_executable(civis_bnw_unit_sight_preflight_tests ../tests/bnw_unit_sight_preflight_tests.cpp)
        target_link_libraries(civis_bnw_unit_sight_preflight_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_unit_sight_preflight_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_unit_sight_preflight COMMAND civis_bnw_unit_sight_preflight_tests
            ${CIV5_BNW_SELECTED_SAVE})
        add_executable(civis_bnw_unit_sight_plan_tests ../tests/bnw_unit_sight_plan_tests.cpp)
        target_link_libraries(civis_bnw_unit_sight_plan_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_unit_sight_plan_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_unit_sight_reveal_plan_tests ../tests/bnw_unit_sight_reveal_plan_tests.cpp)
        target_link_libraries(civis_bnw_unit_sight_reveal_plan_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_unit_sight_reveal_plan_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_unit_sight_detached_tests ../tests/bnw_unit_sight_detached_tests.cpp)
        target_link_libraries(civis_bnw_unit_sight_detached_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_unit_sight_detached_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_unit_sight_reveal_details_tests ../tests/bnw_unit_sight_reveal_details_tests.cpp)
        target_link_libraries(civis_bnw_unit_sight_reveal_details_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_unit_sight_reveal_details_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_unit_sight_exploration_tests ../tests/bnw_unit_sight_exploration_tests.cpp)
        target_link_libraries(civis_bnw_unit_sight_exploration_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_unit_sight_exploration_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_unit_sight_event_order_tests ../tests/bnw_unit_sight_event_order_tests.cpp)
        target_link_libraries(civis_bnw_unit_sight_event_order_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_unit_sight_event_order_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_sight_meet_detached_tests ../tests/bnw_sight_meet_detached_tests.cpp)
        target_link_libraries(civis_bnw_sight_meet_detached_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_sight_meet_detached_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_game_activation_preflight_tests ../tests/bnw_game_activation_preflight_tests.cpp)
        target_link_libraries(civis_bnw_game_activation_preflight_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_game_activation_preflight_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_first_contact_approach_tail_tests ../tests/bnw_first_contact_approach_tail_tests.cpp)
        target_link_libraries(civis_bnw_first_contact_approach_tail_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_first_contact_approach_tail_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_first_contact_approach_tail COMMAND civis_bnw_first_contact_approach_tail_tests)
        add_executable(civis_bnw_game_activation_execution_tests ../tests/bnw_game_activation_execution_tests.cpp)
        add_executable(civis_bnw_activation_connections_tests ../tests/bnw_activation_connections_tests.cpp)
        add_executable(civis_bnw_founder_benefits_lua_tests ../tests/bnw_founder_benefits_lua_tests.cpp)
        add_executable(civis_bnw_founder_religion_native_tests ../tests/bnw_founder_religion_native_tests.cpp)
        add_executable(civis_bnw_religion_happiness_terms_tests ../tests/bnw_religion_happiness_terms_tests.cpp)
        target_link_libraries(civis_bnw_religion_happiness_terms_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_religion_happiness_terms_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_religion_happiness_terms COMMAND civis_bnw_religion_happiness_terms_tests)
        target_link_libraries(civis_bnw_founder_religion_native_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_founder_religion_native_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_founder_religion_native COMMAND civis_bnw_founder_religion_native_tests)
        add_executable(civis_bnw_end_turn_request_tests ../tests/bnw_end_turn_request_tests.cpp)
        add_executable(civis_bnw_update_moves_prefix_tests ../tests/bnw_update_moves_prefix_tests.cpp)
        add_executable(civis_bnw_update_moves_local_sp_tests ../tests/bnw_update_moves_local_sp_tests.cpp)
        target_link_libraries(civis_bnw_update_moves_local_sp_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_update_moves_local_sp_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_human_auto_moves_tests ../tests/bnw_human_auto_moves_tests.cpp)
        target_link_libraries(civis_bnw_human_auto_moves_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_human_auto_moves_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_selected_stock_lua_bootstrap_tests ../tests/bnw_selected_stock_lua_bootstrap_tests.cpp)
        add_executable(civis_bnw_slider_graphic_cache_tests ../tests/bnw_slider_graphic_cache_tests.cpp)
        add_executable(civis_bnw_native_slider_setter_tests ../tests/bnw_native_slider_setter_tests.cpp)
        add_executable(civis_bnw_civilopedia_native_initial_graph_tests ../tests/bnw_civilopedia_native_initial_graph_tests.cpp)
        add_executable(civis_bnw_native_root_context_fields_tests ../tests/bnw_native_root_context_fields_tests.cpp)
        target_link_libraries(civis_bnw_native_root_context_fields_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_native_root_context_fields_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_native_label_field_storage_tests ../tests/bnw_native_label_field_storage_tests.cpp)
        target_link_libraries(civis_bnw_native_label_field_storage_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_native_label_field_storage_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_native_ui_color_owner_tests ../tests/bnw_native_ui_color_owner_tests.cpp)
        add_executable(civis_bnw_native_package_discovery_owner_tests ../tests/bnw_native_package_discovery_owner_tests.cpp)
        add_executable(civis_bnw_native_first_archive_metadata_tests ../tests/bnw_native_first_archive_metadata_tests.cpp)
        target_link_libraries(civis_bnw_native_first_archive_metadata_tests PRIVATE civis_runtime)
        add_executable(civis_bnw_native_installation_parent_prefix_tests
            ../tests/bnw_native_installation_parent_prefix_tests.cpp)
        target_link_libraries(civis_bnw_native_installation_parent_prefix_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_native_first_archive_metadata_tests PRIVATE -static -Wl,--strip-debug)
            target_link_options(civis_bnw_native_installation_parent_prefix_tests PRIVATE -static -Wl,--strip-debug)
        endif()
        add_executable(civis_bnw_native_package_archive_time_owner_tests
            ../tests/bnw_native_package_archive_time_owner_tests.cpp)
        add_executable(civis_bnw_native_original_stat_service_tests
            ../tests/bnw_native_original_stat_service_tests.cpp)
        target_include_directories(civis_bnw_native_original_stat_service_tests PRIVATE ../src/ui)
        target_link_libraries(civis_bnw_native_original_stat_service_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_native_original_stat_service_tests PRIVATE -static)
        endif()
        foreach(_source_test IN ITEMS resource_bucket_order original_stat_path package_category_stage archive_construction_source database_validation_settings_source)
            add_executable(civis_bnw_native_${_source_test}_tests
                ../tests/bnw_native_${_source_test}_tests.cpp)
            target_include_directories(civis_bnw_native_${_source_test}_tests PRIVATE ../include ../src/ui)
            target_compile_features(civis_bnw_native_${_source_test}_tests PRIVATE cxx_std_20)
            if(MINGW)
                target_link_options(civis_bnw_native_${_source_test}_tests PRIVATE -static)
            endif()
        endforeach()
        target_sources(civis_bnw_native_package_category_stage_tests PRIVATE
            ../src/ui/bnw_native_package_category_stage.cpp)
        add_test(NAME runtime.bnw_native_database_validation_settings_source
            COMMAND civis_bnw_native_database_validation_settings_source_tests)
        add_executable(civis_bnw_globals_cache_source_owner_tests
            ../tests/bnw_globals_cache_source_owner_tests.cpp)
        target_link_libraries(civis_bnw_globals_cache_source_owner_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_globals_cache_source_owner_tests PRIVATE -static)
        endif()
        set(CIV5_BNW_GLOBALS_CACHE_SOURCE_FIXTURE "" CACHE FILEPATH
            "Locally generated original BNW Globals cache source fixture")
        if(EXISTS "${CIV5_BNW_GLOBALS_CACHE_SOURCE_FIXTURE}")
            add_test(NAME runtime.bnw_globals_cache_source_owner
                COMMAND civis_bnw_globals_cache_source_owner_tests
                "${CIV5_BNW_GLOBALS_CACHE_SOURCE_FIXTURE}"
                "${CIV5_BNW_INSTALL_ROOT}/Assets/DLC/Expansion2/CvGameCore_Expansion2.dll"
                "${CIV5_ASSETS_ROOT}/Assets/DLC/Expansion2/CvGameCore_Expansion2.dll")
        endif()
        add_test(NAME runtime.bnw_native_original_stat_path
            COMMAND civis_bnw_native_original_stat_path_tests)
        foreach(_fixture_test IN ITEMS resource_bucket_order package_category_stage archive_construction_source)
            set(CIV5_BNW_NATIVE_${_fixture_test}_FIXTURE "" CACHE FILEPATH
                "Locally generated original ${_fixture_test} source fixture")
            if(EXISTS "${CIV5_BNW_NATIVE_${_fixture_test}_FIXTURE}")
                add_test(NAME runtime.bnw_native_${_fixture_test}
                    COMMAND civis_bnw_native_${_fixture_test}_tests
                    "${CIV5_BNW_NATIVE_${_fixture_test}_FIXTURE}")
            endif()
        endforeach()
        target_link_libraries(civis_bnw_native_package_archive_time_owner_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_native_package_archive_time_owner_tests PRIVATE -static)
        endif()
        foreach(_early_test IN ITEMS input_lease package_input art_bootstrap)
            add_executable(civis_bnw_native_early_${_early_test}_tests
                ../tests/bnw_native_early_${_early_test}_tests.cpp)
            target_link_libraries(civis_bnw_native_early_${_early_test}_tests PRIVATE civis_runtime)
            if(MINGW)
                target_link_options(civis_bnw_native_early_${_early_test}_tests PRIVATE -static)
            endif()
        endforeach()
        target_link_libraries(civis_bnw_native_package_discovery_owner_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_native_package_discovery_owner_tests PRIVATE -static)
        endif()
        target_link_libraries(civis_bnw_native_ui_color_owner_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_native_ui_color_owner_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_grand_world_averages_tests ../tests/bnw_grand_world_averages_tests.cpp)
        add_executable(civis_bnw_grand_first_priority_tests ../tests/bnw_grand_first_priority_tests.cpp)
        add_executable(civis_bnw_total_jons_culture_tests ../tests/bnw_total_jons_culture_tests.cpp)
        add_executable(civis_bnw_grand_culture_priority_tests ../tests/bnw_grand_culture_priority_tests.cpp)
        add_executable(civis_bnw_grand_first_roll_tests ../tests/bnw_grand_first_roll_tests.cpp)
        target_sources(civis_bnw_grand_first_roll_tests PRIVATE ../tests/bnw_grand_selection_integration_probe.cpp)
        add_executable(civis_bnw_grand_united_nations_priority_tests ../tests/bnw_grand_united_nations_priority_tests.cpp)
        add_executable(civis_bnw_grand_spaceship_priority_tests ../tests/bnw_grand_spaceship_priority_tests.cpp)
        target_link_libraries(civis_bnw_grand_spaceship_priority_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_grand_spaceship_priority_tests PRIVATE -static)
        endif()
        target_link_libraries(civis_bnw_grand_united_nations_priority_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_grand_united_nations_priority_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_native_package_pre_skin_source_tests ../tests/bnw_native_package_pre_skin_source_tests.cpp)
        target_link_libraries(civis_bnw_native_package_pre_skin_source_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_native_package_pre_skin_source_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_native_package_pre_skin_source COMMAND civis_bnw_native_package_pre_skin_source_tests "${CIV5_ASSETS_ROOT}")
        target_link_libraries(civis_bnw_grand_first_roll_tests PRIVATE civis_runtime)
        if(WIN32)
            target_link_libraries(civis_bnw_grand_first_roll_tests PRIVATE civ5_bnw_deferred_fog_client)
        endif()
        if(MINGW)
            target_link_options(civis_bnw_grand_first_roll_tests PRIVATE -static)
        endif()
        target_link_libraries(civis_bnw_grand_culture_priority_tests PRIVATE civis_runtime)
        if(WIN32)
            target_link_libraries(civis_bnw_grand_culture_priority_tests PRIVATE civ5_bnw_deferred_fog_client)
        endif()
        if(MINGW)
            target_link_options(civis_bnw_grand_culture_priority_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_player_tourism_influence_tests ../tests/bnw_player_tourism_influence_tests.cpp)
        target_link_libraries(civis_bnw_player_tourism_influence_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_player_tourism_influence_tests PRIVATE -static)
        endif()
        target_link_libraries(civis_bnw_total_jons_culture_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_total_jons_culture_tests PRIVATE -static)
        endif()
        target_link_libraries(civis_bnw_grand_first_priority_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_grand_first_priority_tests PRIVATE -static)
        endif()
        target_link_libraries(civis_bnw_grand_world_averages_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_grand_world_averages_tests PRIVATE -static)
        endif()
        foreach(_grand_test IN ITEMS guess_empty guess_execution)
            add_executable(civis_bnw_grand_${_grand_test}_tests ../tests/bnw_grand_${_grand_test}_tests.cpp)
            target_link_libraries(civis_bnw_grand_${_grand_test}_tests PRIVATE civis_runtime)
            if(MINGW)
                target_link_options(civis_bnw_grand_${_grand_test}_tests PRIVATE -static)
            endif()
        endforeach()
        if(WIN32)
            # The retained execution fixture consumes the existing CPU fog
            # receipt owner; this target is declared by the parent directory.
            target_link_libraries(civis_bnw_grand_guess_execution_tests
                PRIVATE civ5_bnw_deferred_fog_client)
        endif()
        target_link_libraries(civis_bnw_civilopedia_native_initial_graph_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_civilopedia_native_initial_graph_tests PRIVATE -static)
        endif()
        target_link_libraries(civis_bnw_native_slider_setter_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_native_slider_setter_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_empty_article_stack_owner_tests ../tests/bnw_empty_article_stack_owner_tests.cpp)
        add_executable(civis_bnw_civilopedia_right_tree_owner_tests ../tests/bnw_civilopedia_right_tree_owner_tests.cpp)
        add_executable(civis_bnw_client_text_policy_tests ../tests/bnw_client_text_policy_tests.cpp)
        add_executable(civis_bnw_city_strategy_yields_tests ../tests/bnw_city_strategy_yields_tests.cpp)
        add_executable(civis_bnw_activation_builder_tasking_tests ../tests/bnw_activation_builder_tasking_tests.cpp)
        foreach(_civis_new_owner civis_bnw_client_text_policy_tests civis_bnw_city_strategy_yields_tests civis_bnw_activation_builder_tasking_tests)
            target_link_libraries(${_civis_new_owner} PRIVATE civis_runtime)
            if(MINGW)
                target_link_options(${_civis_new_owner} PRIVATE -static)
            endif()
        endforeach()
        add_executable(civis_bnw_civilopedia_right_tree_epoch_tests ../tests/bnw_civilopedia_right_tree_epoch_tests.cpp)
        target_link_libraries(civis_bnw_civilopedia_right_tree_owner_tests PRIVATE civis_runtime)
        target_link_libraries(civis_bnw_civilopedia_right_tree_epoch_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_civilopedia_right_tree_owner_tests PRIVATE -static)
            target_link_options(civis_bnw_civilopedia_right_tree_epoch_tests PRIVATE -static)
        endif()
        if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND
           EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe" AND
           IS_DIRECTORY "${CIV5_ASSETS_ROOT}")
            add_test(NAME runtime.bnw_civilopedia_right_tree_epoch
                COMMAND civis_bnw_civilopedia_right_tree_epoch_tests
                    "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}"
                    "${CIV5_ASSETS_ROOT}")
            set_tests_properties(runtime.bnw_civilopedia_right_tree_epoch
                PROPERTIES TIMEOUT 1200)
        endif()
        target_link_libraries(civis_bnw_empty_article_stack_owner_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_empty_article_stack_owner_tests PRIVATE -static)
        endif()
        if(EXISTS "${CIV5_ASSETS_ROOT}/Assets/UI/Styles.xml")
            add_test(NAME runtime.bnw_empty_article_stack_owner COMMAND civis_bnw_empty_article_stack_owner_tests
                "${CIV5_ASSETS_ROOT}")
        endif()
        target_link_libraries(civis_bnw_slider_graphic_cache_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_slider_graphic_cache_tests PRIVATE -static)
        endif()
        if(EXISTS "${CIV5_ASSETS_ROOT}/Assets/UI/Styles.xml")
            add_test(NAME runtime.bnw_slider_graphic_cache COMMAND civis_bnw_slider_graphic_cache_tests
                "${CIV5_ASSETS_ROOT}")
        endif()
        target_link_libraries(civis_bnw_selected_stock_lua_bootstrap_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_selected_stock_lua_bootstrap_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_happiness_resources_tests ../tests/bnw_happiness_resources_tests.cpp)
        target_link_libraries(civis_bnw_happiness_resources_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_happiness_resources_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_happiness_natural_wonders_tests ../tests/bnw_happiness_natural_wonders_tests.cpp)
        target_link_libraries(civis_bnw_happiness_natural_wonders_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_happiness_natural_wonders_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_unit_movement_query_tests ../tests/bnw_unit_movement_query_tests.cpp)
        target_link_libraries(civis_bnw_unit_movement_query_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_unit_movement_query_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_happiness_policies_tests ../tests/bnw_happiness_policies_tests.cpp)
        add_executable(civis_bnw_happiness_buildings_tests ../tests/bnw_happiness_buildings_tests.cpp)
        add_executable(civis_bnw_happiness_minors_tests ../tests/bnw_happiness_minors_tests.cpp)
        target_link_libraries(civis_bnw_happiness_minors_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_happiness_minors_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_happiness_update_prefix_tests ../tests/bnw_happiness_update_prefix_tests.cpp)
        add_executable(civis_bnw_happiness_update_suffix_tests ../tests/bnw_happiness_update_suffix_tests.cpp)
        add_executable(civis_bnw_activation_happiness_tests ../tests/bnw_activation_happiness_tests.cpp)
        target_link_libraries(civis_bnw_activation_happiness_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_activation_happiness_tests PRIVATE -static)
        endif()
        # The Windows scene target is declared by the parent after this directory.
        if(WIN32)
            add_executable(civis_bnw_game_activation_happiness_tests ../tests/bnw_game_activation_happiness_tests.cpp)
            target_link_libraries(civis_bnw_game_activation_happiness_tests
                PRIVATE civis_runtime civ5_bnw_deferred_fog_client)
            if(MINGW)
                target_link_options(civis_bnw_game_activation_happiness_tests PRIVATE -static)
            endif()
        endif()
        target_link_libraries(civis_bnw_happiness_update_suffix_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_happiness_update_suffix_tests PRIVATE -static)
        endif()
        target_link_libraries(civis_bnw_happiness_update_prefix_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_happiness_update_prefix_tests PRIVATE -static)
        endif()
        target_link_libraries(civis_bnw_happiness_buildings_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_happiness_buildings_tests PRIVATE -static)
        endif()
        target_link_libraries(civis_bnw_happiness_policies_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_happiness_policies_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_unit_order_owners_tests ../tests/bnw_unit_order_owners_tests.cpp)
        target_link_libraries(civis_bnw_unit_order_owners_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_unit_order_owners_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_unit_finish_moves_tests ../tests/bnw_unit_finish_moves_tests.cpp)
        target_link_libraries(civis_bnw_unit_finish_moves_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_unit_finish_moves_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_unit_reset_tests ../tests/bnw_unit_reset_tests.cpp)
        target_link_libraries(civis_bnw_unit_reset_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_unit_reset_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_player_turn_deactivation_tests ../tests/bnw_player_turn_deactivation_tests.cpp)
        add_executable(civis_bnw_notification_cleanup_tests ../tests/bnw_notification_cleanup_tests.cpp)
        add_executable(civis_bnw_player_turn_suffix_tests ../tests/bnw_player_turn_suffix_tests.cpp)
        target_link_libraries(civis_bnw_player_turn_suffix_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_player_turn_suffix_tests PRIVATE -static)
        endif()
        target_link_libraries(civis_bnw_notification_cleanup_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_notification_cleanup_tests PRIVATE -static)
        endif()
        target_link_libraries(civis_bnw_player_turn_deactivation_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_player_turn_deactivation_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_busy_state_tests ../tests/bnw_busy_state_tests.cpp)
        add_executable(civis_bnw_ready_units_tests ../tests/bnw_ready_units_tests.cpp)
        target_link_libraries(civis_bnw_ready_units_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_ready_units_tests PRIVATE -static)
        endif()
        target_link_libraries(civis_bnw_busy_state_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_busy_state_tests PRIVATE -static)
        endif()
        target_link_libraries(civis_bnw_update_moves_prefix_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_update_moves_prefix_tests PRIVATE -static)
        endif()
        target_link_libraries(civis_bnw_end_turn_request_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_end_turn_request_tests PRIVATE -static)
        endif()
        target_link_libraries(civis_bnw_founder_benefits_lua_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_founder_benefits_lua_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_founder_benefits_lua COMMAND civis_bnw_founder_benefits_lua_tests)
        target_link_libraries(civis_bnw_activation_connections_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_activation_connections_tests PRIVATE -static)
        endif()
        target_link_libraries(civis_bnw_game_activation_execution_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_game_activation_execution_tests PRIVATE -static)
        endif()
        add_executable(civis_ui_ideology_layout_tests ../tests/ui_ideology_layout_tests.cpp)
        target_link_libraries(civis_ui_ideology_layout_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_ui_ideology_layout_tests PRIVATE -static)
        endif()
        add_executable(civis_ui_ideology_frame_owner_tests ../tests/ui_ideology_frame_owner_tests.cpp)
        target_link_libraries(civis_ui_ideology_frame_owner_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_ui_ideology_frame_owner_tests PRIVATE -static)
        endif()
        add_executable(civis_ui_ideology_compositor_tests ../tests/ui_ideology_compositor_tests.cpp)
        target_link_libraries(civis_ui_ideology_compositor_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_ui_ideology_compositor_tests PRIVATE -static)
        endif()
        add_executable(civis_bnw_area_unit_checkpoint_tests ../tests/bnw_area_unit_checkpoint_tests.cpp)
        target_link_libraries(civis_bnw_area_unit_checkpoint_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_area_unit_checkpoint_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.bnw_area_unit_checkpoint COMMAND civis_bnw_area_unit_checkpoint_tests
            ${CIV5_BNW_SELECTED_SAVE})
    endif()
    add_executable(civis_bnw_nearby_cities_tests ../tests/bnw_nearby_cities_tests.cpp)
    target_link_libraries(civis_bnw_nearby_cities_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_nearby_cities_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_nearby_cities COMMAND civis_bnw_nearby_cities_tests)
    add_executable(civis_bnw_city_flavor_recipients_tests ../tests/bnw_city_flavor_recipients_tests.cpp)
    target_link_libraries(civis_bnw_city_flavor_recipients_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_city_flavor_recipients_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}")
        add_test(NAME runtime.bnw_city_flavor_recipients COMMAND civis_bnw_city_flavor_recipients_tests
            ${CIV5_BNW_SELECTED_SAVE})
    endif()
    add_executable(civis_bnw_player_flavor_recipients_tests ../tests/bnw_player_flavor_recipients_tests.cpp)
    target_link_libraries(civis_bnw_player_flavor_recipients_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_flavor_recipients_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}")
        add_test(NAME runtime.bnw_player_flavor_recipients COMMAND civis_bnw_player_flavor_recipients_tests
            ${CIV5_BNW_SELECTED_SAVE})
    endif()
    add_executable(civis_bnw_live_plot_impassability_tests ../tests/bnw_live_plot_impassability_tests.cpp)
    target_link_libraries(civis_bnw_live_plot_impassability_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_live_plot_impassability_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}")
        add_test(NAME runtime.bnw_live_plot_impassability COMMAND civis_bnw_live_plot_impassability_tests
            ${CIV5_BNW_SELECTED_SAVE}
            ${CIV5_ASSETS_ROOT})
    endif()
    add_executable(civis_bnw_adjacent_land_tests ../tests/bnw_adjacent_land_tests.cpp)
    target_link_libraries(civis_bnw_adjacent_land_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_adjacent_land_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}")
        add_test(NAME runtime.bnw_adjacent_land COMMAND civis_bnw_adjacent_land_tests
            ${CIV5_BNW_SELECTED_SAVE})
    endif()
    add_executable(civis_bnw_live_plot_visibility_tests ../tests/bnw_live_plot_visibility_tests.cpp)
    target_link_libraries(civis_bnw_live_plot_visibility_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_live_plot_visibility_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}")
        add_test(NAME runtime.bnw_live_plot_visibility COMMAND civis_bnw_live_plot_visibility_tests
            ${CIV5_BNW_SELECTED_SAVE})
    endif()
    add_executable(civis_bnw_player_gold_tests ../tests/bnw_player_gold_tests.cpp)
    target_link_libraries(civis_bnw_player_gold_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_gold_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_player_gold COMMAND civis_bnw_player_gold_tests)
    add_executable(civis_bnw_deficit_selection_tests ../tests/bnw_deficit_selection_tests.cpp)
    target_link_libraries(civis_bnw_deficit_selection_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_deficit_selection_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_deficit_selection COMMAND civis_bnw_deficit_selection_tests)
    add_executable(civis_bnw_deficit_rules_tests ../tests/bnw_deficit_rules_tests.cpp)
    target_link_libraries(civis_bnw_deficit_rules_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_deficit_rules_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_deficit_rules COMMAND civis_bnw_deficit_rules_tests)
    add_executable(civis_bnw_deficit_scrap_tests ../tests/bnw_deficit_scrap_tests.cpp)
    target_link_libraries(civis_bnw_deficit_scrap_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_deficit_scrap_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_deficit_scrap COMMAND civis_bnw_deficit_scrap_tests)
    add_executable(civis_bnw_deficit_scrap_rules_tests ../tests/bnw_deficit_scrap_rules_tests.cpp)
    target_link_libraries(civis_bnw_deficit_scrap_rules_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_deficit_scrap_rules_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_deficit_scrap_rules COMMAND civis_bnw_deficit_scrap_rules_tests)
    add_executable(civis_bnw_deficit_live_binding_tests ../tests/bnw_deficit_live_binding_tests.cpp)
    target_link_libraries(civis_bnw_deficit_live_binding_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_deficit_live_binding_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_deficit_live_binding COMMAND civis_bnw_deficit_live_binding_tests)
    add_executable(civis_bnw_military_defense_tests ../tests/bnw_military_defense_tests.cpp)
    target_link_libraries(civis_bnw_military_defense_tests PRIVATE civis_runtime)
    if (MINGW)
        target_link_options(civis_bnw_military_defense_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_military_defense COMMAND civis_bnw_military_defense_tests)
    add_executable(civis_bnw_military_base_units_tests ../tests/bnw_military_base_units_tests.cpp)
    target_link_libraries(civis_bnw_military_base_units_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_military_base_units_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_military_base_units COMMAND civis_bnw_military_base_units_tests)
    add_executable(civis_bnw_military_settler_demand_tests ../tests/bnw_military_settler_demand_tests.cpp)
    target_link_libraries(civis_bnw_military_settler_demand_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_military_settler_demand_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_military_settler_demand COMMAND civis_bnw_military_settler_demand_tests)
    add_executable(civis_bnw_delayed_death_tests ../tests/bnw_delayed_death_tests.cpp)
    target_link_libraries(civis_bnw_delayed_death_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_delayed_death_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_delayed_death COMMAND civis_bnw_delayed_death_tests)
    add_executable(civis_bnw_unit_kill_lua_tests ../tests/bnw_unit_kill_lua_tests.cpp)
    target_link_libraries(civis_bnw_unit_kill_lua_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_unit_kill_lua_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_unit_kill_lua COMMAND civis_bnw_unit_kill_lua_tests)
    add_executable(civis_bnw_player_research_tests ../tests/bnw_player_research_tests.cpp)
    target_link_libraries(civis_bnw_player_research_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_player_research_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_player_research COMMAND civis_bnw_player_research_tests)
    add_executable(civis_bnw_research_choice_tests ../tests/bnw_research_choice_tests.cpp)
    target_link_libraries(civis_bnw_research_choice_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_research_choice_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_research_choice COMMAND civis_bnw_research_choice_tests)
    add_executable(civis_bnw_production_order_tests ../tests/bnw_production_order_tests.cpp)
    target_link_libraries(civis_bnw_production_order_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_production_order_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_production_order COMMAND civis_bnw_production_order_tests)
    add_executable(civis_bnw_fresh_game_content_tests ../tests/bnw_fresh_game_content_tests.cpp)
    target_link_libraries(civis_bnw_fresh_game_content_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_game_content_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_game_content COMMAND civis_bnw_fresh_game_content_tests)
    add_executable(civis_bnw_map_script_tests ../tests/bnw_map_script_tests.cpp)
    target_link_libraries(civis_bnw_map_script_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_map_script_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_map_script COMMAND civis_bnw_map_script_tests)
    add_executable(civis_saved_terrain_tests ../tests/saved_terrain_projection_tests.cpp)
    target_link_libraries(civis_saved_terrain_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_saved_terrain_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.saved_terrain COMMAND civis_saved_terrain_tests)
    add_executable(civis_rules_tests ../tests/rules_database_tests.cpp)
    target_link_libraries(civis_rules_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_rules_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.rules_database COMMAND civis_rules_tests)
    add_executable(civis_research_ai_turn_tests ../tests/research_ai_turn_tests.cpp)
    target_link_libraries(civis_research_ai_turn_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_research_ai_turn_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.research_ai_turn COMMAND civis_research_ai_turn_tests)
    add_executable(civis_production_checkpoint_tests ../tests/production_checkpoint_tests.cpp)
    target_link_libraries(civis_production_checkpoint_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_production_checkpoint_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.production_checkpoint COMMAND civis_production_checkpoint_tests)
    add_executable(civis_research_checkpoint_tests ../tests/research_checkpoint_tests.cpp)
    target_link_libraries(civis_research_checkpoint_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_research_checkpoint_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.research_checkpoint COMMAND civis_research_checkpoint_tests)
    add_executable(civis_treasury_checkpoint_tests ../tests/treasury_checkpoint_tests.cpp)
    target_link_libraries(civis_treasury_checkpoint_tests PRIVATE civis_runtime)
    if(WIN32)
        target_link_options(civis_treasury_checkpoint_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.treasury_checkpoint COMMAND civis_treasury_checkpoint_tests)
    add_executable(civis_research_turn_checkpoint_tests ../tests/research_turn_checkpoint_tests.cpp)
    target_link_libraries(civis_research_turn_checkpoint_tests PRIVATE civis_runtime)
    if(WIN32)
        target_link_options(civis_research_turn_checkpoint_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.research_turn_checkpoint COMMAND civis_research_turn_checkpoint_tests)
    add_executable(civis_research_selection_checkpoint_tests ../tests/research_selection_checkpoint_tests.cpp)
    target_link_libraries(civis_research_selection_checkpoint_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_research_selection_checkpoint_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.research_selection_checkpoint COMMAND civis_research_selection_checkpoint_tests)
    add_executable(civis_vp_science_checkpoint_tests ../tests/vp_science_checkpoint_tests.cpp)
    target_link_libraries(civis_vp_science_checkpoint_tests PRIVATE civis_runtime)
    if(WIN32)
        target_link_options(civis_vp_science_checkpoint_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.vp_science_checkpoint COMMAND civis_vp_science_checkpoint_tests)
    add_executable(civis_acquisition_tests ../tests/mod_acquisition_tests.cpp)
    target_link_libraries(civis_acquisition_tests PRIVATE civis_runtime)
    target_compile_definitions(civis_acquisition_tests PRIVATE CIVIS_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../tests/data")
    if(MINGW)
        target_link_options(civis_acquisition_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.mod_acquisition COMMAND civis_acquisition_tests)
    if(WIN32)
        add_executable(civis_fake_https_curl ../tests/fake_https_curl.cpp)
        add_executable(civis_http_transport_tests ../tests/http_transport_tests.cpp)
        target_link_libraries(civis_http_transport_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_fake_https_curl PRIVATE -static)
            target_link_options(civis_http_transport_tests PRIVATE -static)
        endif()
        add_test(NAME runtime.http_transport COMMAND civis_http_transport_tests $<TARGET_FILE:civis_fake_https_curl>)
    endif()
    add_executable(civis_profile_tests ../tests/vp_profile_tests.cpp)
    target_link_libraries(civis_profile_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_profile_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.content_profile COMMAND civis_profile_tests)
    add_executable(civis_ui_top_panel_tests ../tests/ui_top_panel_tests.cpp)
    target_link_libraries(civis_ui_top_panel_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_top_panel_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_top_panel COMMAND civis_ui_top_panel_tests)
    add_executable(civis_ui_top_panel_bnw_tests ../tests/ui_top_panel_bnw_tests.cpp)
    target_link_libraries(civis_ui_top_panel_bnw_tests PRIVATE civis_runtime)
    if (MINGW)
        target_link_options(civis_ui_top_panel_bnw_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_top_panel_bnw COMMAND civis_ui_top_panel_bnw_tests)
    add_executable(civis_ui_minimap_tests ../tests/ui_minimap_tests.cpp)
    target_link_libraries(civis_ui_minimap_tests PRIVATE civis_runtime civ5_terrain)
    if(MINGW)
        target_link_options(civis_ui_minimap_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_minimap COMMAND civis_ui_minimap_tests)
    add_executable(civis_ui_minimap_interaction_tests ../tests/ui_minimap_interaction_tests.cpp)
    target_link_libraries(civis_ui_minimap_interaction_tests PRIVATE civis_runtime civ5_terrain)
    if(MINGW)
        target_link_options(civis_ui_minimap_interaction_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_minimap_interaction COMMAND civis_ui_minimap_interaction_tests)
    add_executable(civis_ui_runtime_tests ../tests/ui_runtime_tests.cpp)
    target_link_libraries(civis_ui_runtime_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_runtime_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_runtime COMMAND civis_ui_runtime_tests)
    add_executable(civis_ui_screen_compositor_tests ../tests/ui_screen_compositor_tests.cpp)
    target_link_libraries(civis_ui_screen_compositor_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_screen_compositor_tests PRIVATE -static)
    endif()
    if(CIV5_BNW_INSTALL_ROOT)
        add_test(NAME runtime.ui_screen_compositor COMMAND civis_ui_screen_compositor_tests
            "${CIV5_BNW_INSTALL_ROOT}")
    else()
        add_test(NAME runtime.ui_screen_compositor COMMAND civis_ui_screen_compositor_tests
            "${CIV5_ASSETS_ROOT}")
    endif()
    add_executable(civis_ui_live_host_tests ../tests/ui_live_host_tests.cpp)
    target_link_libraries(civis_ui_live_host_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_live_host_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_live_host COMMAND civis_ui_live_host_tests)
    foreach(_graphics_test IN ITEMS bnw_client_frame client_window_input
            civis_measured_ui_lifecycle civis_measured_civilopedia_controls
            civis_measured_civilopedia_lua civis_civilopedia_category_callbacks
            bnw_client_scene_frame bnw_client_capture bnw_ui_source_observation
            selected_civilopedia_locale civis_civilopedia_input_owner ui_texture_asset
            ui_quad_clip civis_civilopedia_tooltip_owner glyphgen_style
            bnw_top_panel_preview_bridge bnw_unit_panel_preview_bridge
            civis_locale_text_owner bnw_city_banner_appearance
            bnw_unit_panel_secondary_stats bnw_unit_panel_stat_rows
            civis_lua_table_cardinality civis_lua_table_count_capability)
        if(_graphics_test MATCHES "^civis_")
            set(_graphics_test_target ${_graphics_test}_tests)
        else()
            set(_graphics_test_target civis_${_graphics_test}_tests)
        endif()
        add_executable(${_graphics_test_target} ../tests/${_graphics_test}_tests.cpp)
        target_link_libraries(${_graphics_test_target} PRIVATE civis_runtime)
        if(_graphics_test STREQUAL "civis_lua_table_cardinality")
            target_link_libraries(${_graphics_test_target} PRIVATE civis_lua51)
        endif()
        if(_graphics_test MATCHES "^bnw_client_(scene_frame|capture)$" AND WIN32)
            target_link_libraries(${_graphics_test_target} PRIVATE civ5_gl civ5_bnw_deferred_fog_client civ5_scene_saved_fog)
        endif()
        if(MINGW)
            target_link_options(${_graphics_test_target} PRIVATE -static)
        endif()
        if(_graphics_test MATCHES "^civis_measured_civilopedia_" OR
           _graphics_test STREQUAL "civis_civilopedia_category_callbacks" OR
           _graphics_test STREQUAL "selected_civilopedia_locale" OR
           _graphics_test STREQUAL "civis_civilopedia_input_owner" OR
           _graphics_test STREQUAL "civis_locale_text_owner" OR
           _graphics_test STREQUAL "bnw_city_banner_appearance" OR
           _graphics_test STREQUAL "civis_lua_table_cardinality" OR
           _graphics_test STREQUAL "civis_lua_table_count_capability")
            if(CIV5_BNW_INSTALL_ROOT)
                add_test(NAME runtime.${_graphics_test}
                    COMMAND ${_graphics_test_target} "${CIV5_BNW_INSTALL_ROOT}")
            endif()
        elseif(_graphics_test STREQUAL "civis_civilopedia_tooltip_owner" OR
               _graphics_test STREQUAL "bnw_top_panel_preview_bridge" OR
               _graphics_test STREQUAL "bnw_unit_panel_preview_bridge" OR
               _graphics_test STREQUAL "bnw_unit_panel_secondary_stats" OR
               _graphics_test STREQUAL "bnw_unit_panel_stat_rows")
            if(CIV5_BNW_INSTALL_ROOT AND EXISTS "${CIV5_BNW_SELECTED_SAVE}")
                add_test(NAME runtime.${_graphics_test} COMMAND ${_graphics_test_target}
                    "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
            endif()
        elseif(_graphics_test STREQUAL "glyphgen_style")
            if(CIV5_BNW_INSTALL_ROOT)
                add_test(NAME runtime.${_graphics_test} COMMAND ${_graphics_test_target}
                    "${CIV5_BNW_INSTALL_ROOT}")
            else()
                add_test(NAME runtime.${_graphics_test} COMMAND ${_graphics_test_target})
            endif()
        elseif(_graphics_test STREQUAL "ui_texture_asset")
            if(CIV5_BNW_INSTALL_ROOT)
                add_test(NAME runtime.${_graphics_test} COMMAND ${_graphics_test_target}
                    "${CMAKE_CURRENT_BINARY_DIR}/ui-texture-fixtures" "${CIV5_BNW_INSTALL_ROOT}")
            else()
                add_test(NAME runtime.${_graphics_test} COMMAND ${_graphics_test_target}
                    "${CMAKE_CURRENT_BINARY_DIR}/ui-texture-fixtures")
            endif()
        else()
            add_test(NAME runtime.${_graphics_test} COMMAND ${_graphics_test_target})
        endif()
    endforeach()
    add_executable(civis_ui_live_session_frame_tests ../tests/ui_live_session_frame_tests.cpp)
    target_link_libraries(civis_ui_live_session_frame_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_live_session_frame_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_live_session_frame COMMAND civis_ui_live_session_frame_tests)
    add_executable(civis_ui_live_visibility_tests ../tests/ui_live_visibility_tests.cpp)
    target_link_libraries(civis_ui_live_visibility_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_live_visibility_tests PRIVATE -static)
    endif()
    add_executable(civis_selected_bnw_preview_host_tests ../tests/selected_bnw_preview_host_tests.cpp)
    target_link_libraries(civis_selected_bnw_preview_host_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_selected_bnw_preview_host_tests PRIVATE -static)
    endif()
    add_executable(civis_selected_bnw_civis_preview_tests
        ../tests/selected_bnw_civis_preview_tests.cpp)
    target_link_libraries(civis_selected_bnw_civis_preview_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_selected_bnw_civis_preview_tests PRIVATE -static)
    endif()
    add_executable(civis_selected_bnw_research_preview_tests ../tests/selected_bnw_research_preview_tests.cpp)
    target_link_libraries(civis_selected_bnw_research_preview_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_selected_bnw_research_preview_tests PRIVATE -static)
    endif()
    add_executable(civis_selected_bnw_research_lua_inventory_tests
        ../tests/selected_bnw_research_lua_inventory_tests.cpp)
    target_link_libraries(civis_selected_bnw_research_lua_inventory_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_selected_bnw_research_lua_inventory_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.selected_bnw_research_lua_inventory
        COMMAND civis_selected_bnw_research_lua_inventory_tests)
    add_executable(civis_selected_bnw_stock_ui_lua_step_tests
        ../tests/selected_bnw_stock_ui_lua_step_tests.cpp)
    target_link_libraries(civis_selected_bnw_stock_ui_lua_step_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_selected_bnw_stock_ui_lua_step_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.selected_bnw_stock_ui_lua_step
        COMMAND civis_selected_bnw_stock_ui_lua_step_tests)
    add_executable(civis_selected_bnw_icon_support_step_tests
        ../tests/selected_bnw_icon_support_step_tests.cpp)
    target_link_libraries(civis_selected_bnw_icon_support_step_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_selected_bnw_icon_support_step_tests PRIVATE -static)
    endif()
    add_executable(civis_selected_bnw_civilopedia_preamble_tests
        ../tests/selected_bnw_civilopedia_preamble_tests.cpp)
    target_link_libraries(civis_selected_bnw_civilopedia_preamble_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_selected_bnw_civilopedia_preamble_tests PRIVATE -static)
    endif()
    add_executable(civis_selected_bnw_civilopedia_layout_tests
        ../tests/selected_bnw_civilopedia_layout_tests.cpp)
    target_link_libraries(civis_selected_bnw_civilopedia_layout_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_selected_bnw_civilopedia_layout_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_civilopedia_scroll_subtree_tests
        ../tests/bnw_civilopedia_scroll_subtree_tests.cpp)
    target_link_libraries(civis_bnw_civilopedia_scroll_subtree_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_civilopedia_scroll_subtree_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_civilopedia_scrollbar_owner_tests
        ../tests/bnw_civilopedia_scrollbar_owner_tests.cpp)
    add_executable(civis_bnw_civilopedia_initial_content_tests
        ../tests/bnw_civilopedia_initial_content_tests.cpp)
    target_link_libraries(civis_bnw_civilopedia_initial_content_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_civilopedia_initial_content_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_civilopedia_initial_content
        COMMAND civis_bnw_civilopedia_initial_content_tests ${CIV5_ASSETS_ROOT})
    target_link_libraries(civis_bnw_civilopedia_scrollbar_owner_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_civilopedia_scrollbar_owner_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_civilopedia_scrollbar_owner
        COMMAND civis_bnw_civilopedia_scrollbar_owner_tests)
    add_executable(civis_civilopedia_geometry_tests
        ../tests/civis_civilopedia_geometry_tests.cpp)
    target_link_libraries(civis_civilopedia_geometry_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_civilopedia_geometry_tests PRIVATE -static)
    endif()
    add_executable(civis_civilopedia_articles_tests
        ../tests/civis_civilopedia_articles_tests.cpp)
    target_link_libraries(civis_civilopedia_articles_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_civilopedia_articles_tests PRIVATE -static)
    endif()
    add_executable(civis_civilopedia_measured_hit_tests
        ../tests/civis_civilopedia_measured_hit_tests.cpp)
    target_link_libraries(civis_civilopedia_measured_hit_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_civilopedia_measured_hit_tests PRIVATE -static)
    endif()
    add_executable(civis_selected_bnw_ui_audio_tests ../tests/selected_bnw_ui_audio_tests.cpp)
    target_link_libraries(civis_selected_bnw_ui_audio_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_selected_bnw_ui_audio_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.selected_bnw_ui_audio COMMAND civis_selected_bnw_ui_audio_tests)
    add_executable(civis_bnw_policy_selected_probe_tests ../tests/bnw_policy_selected_probe_tests.cpp)
    target_link_libraries(civis_bnw_policy_selected_probe_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_policy_selected_probe_tests PRIVATE -static)
    endif()
    set(_civ5_default_bnw_install_root "")
    if(WIN32)
        set(_civ5_default_bnw_install_root
            "C:/Program Files (x86)/Steam/steamapps/common/Sid Meier's Civilization V")
    endif()
    set(CIV5_BNW_INSTALL_ROOT "${_civ5_default_bnw_install_root}" CACHE PATH
        "Civilization V installation for the selected BNW save bootstrap test")
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}")
        add_test(NAME runtime.selected_bnw_preview_host COMMAND civis_selected_bnw_preview_host_tests
            ${CIV5_BNW_SELECTED_SAVE}
            ${CIV5_ASSETS_ROOT}
            ${CIV5_ASSETS_ROOT})
        if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe" AND
           IS_DIRECTORY "${CIV5_BNW_INSTALL_ROOT}/Assets/DLC")
            add_test(NAME runtime.bnw_live_map_player_city_export
                COMMAND civis_bnw_live_map_player_city_export_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_live_rng_export
                COMMAND civis_bnw_live_rng_export_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_stock_inlandsea COMMAND civis_bnw_map_script_tests
                --stock-game-root "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_area_live_restore COMMAND civis_bnw_area_unit_checkpoint_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_unit_sight_plan COMMAND civis_bnw_unit_sight_plan_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_unit_sight_reveal_plan COMMAND civis_bnw_unit_sight_reveal_plan_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_unit_sight_detached COMMAND civis_bnw_unit_sight_detached_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_unit_sight_reveal_details COMMAND civis_bnw_unit_sight_reveal_details_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_unit_sight_exploration COMMAND civis_bnw_unit_sight_exploration_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_unit_sight_event_order COMMAND civis_bnw_unit_sight_event_order_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_sight_meet_detached COMMAND civis_bnw_sight_meet_detached_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_game_activation_preflight COMMAND civis_bnw_game_activation_preflight_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_game_activation_execution COMMAND civis_bnw_game_activation_execution_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_activation_connections COMMAND civis_bnw_activation_connections_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_end_turn_request COMMAND civis_bnw_end_turn_request_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_update_moves_prefix COMMAND civis_bnw_update_moves_prefix_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_update_moves_local_sp COMMAND civis_bnw_update_moves_local_sp_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_human_auto_moves COMMAND civis_bnw_human_auto_moves_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_selected_stock_lua_bootstrap COMMAND civis_bnw_selected_stock_lua_bootstrap_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}"
                ${CIV5_ASSETS_ROOT})
            add_test(NAME runtime.bnw_happiness_resources COMMAND civis_bnw_happiness_resources_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_happiness_natural_wonders COMMAND civis_bnw_happiness_natural_wonders_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_unit_movement_query COMMAND civis_bnw_unit_movement_query_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_happiness_policies COMMAND civis_bnw_happiness_policies_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_happiness_buildings COMMAND civis_bnw_happiness_buildings_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_happiness_minors COMMAND civis_bnw_happiness_minors_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_activation_happiness COMMAND civis_bnw_activation_happiness_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_city_strategy_yields COMMAND civis_bnw_city_strategy_yields_tests
                ${CIV5_BNW_SELECTED_SAVE} "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_activation_builder_tasking COMMAND civis_bnw_activation_builder_tasking_tests
                ${CIV5_BNW_SELECTED_SAVE} "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_client_text_policy COMMAND civis_bnw_client_text_policy_tests
                ${CIV5_BNW_SELECTED_SAVE} "${CIV5_BNW_INSTALL_ROOT}" ${CIV5_ASSETS_ROOT})
            add_test(NAME runtime.bnw_native_slider_setter COMMAND civis_bnw_native_slider_setter_tests
                ${CIV5_BNW_SELECTED_SAVE} "${CIV5_BNW_INSTALL_ROOT}" ${CIV5_ASSETS_ROOT})
            add_test(NAME runtime.bnw_civilopedia_native_initial_graph COMMAND civis_bnw_civilopedia_native_initial_graph_tests
                ${CIV5_BNW_SELECTED_SAVE} "${CIV5_BNW_INSTALL_ROOT}" ${CIV5_ASSETS_ROOT})
            add_test(NAME runtime.bnw_native_ui_color_owner COMMAND civis_bnw_native_ui_color_owner_tests
                ${CIV5_BNW_SELECTED_SAVE} "${CIV5_BNW_INSTALL_ROOT}" ${CIV5_ASSETS_ROOT})
            add_test(NAME runtime.bnw_native_package_discovery_owner COMMAND civis_bnw_native_package_discovery_owner_tests
                ${CIV5_BNW_SELECTED_SAVE} "${CIV5_BNW_INSTALL_ROOT}" ${CIV5_ASSETS_ROOT})
            add_test(NAME runtime.bnw_native_first_archive_metadata COMMAND civis_bnw_native_first_archive_metadata_tests
                ${CIV5_BNW_SELECTED_SAVE} "${CIV5_BNW_INSTALL_ROOT}" ${CIV5_ASSETS_ROOT})
            set_tests_properties(runtime.bnw_native_first_archive_metadata PROPERTIES TIMEOUT 600)
            if(CIV5_BNW_SDK_ROOT)
                add_test(NAME runtime.bnw_grand_world_averages COMMAND civis_bnw_grand_world_averages_tests
                    ${CIV5_BNW_SELECTED_SAVE} "${CIV5_BNW_INSTALL_ROOT}"
                    "${CIV5_BNW_SDK_ROOT}/CvGameCoreSource")
            else()
                add_test(NAME runtime.bnw_grand_world_averages COMMAND civis_bnw_grand_world_averages_tests
                    ${CIV5_BNW_SELECTED_SAVE} "${CIV5_BNW_INSTALL_ROOT}")
            endif()
            foreach(_grand_test IN ITEMS guess_empty guess_execution first_priority united_nations_priority)
                set(_grand_args ${CIV5_BNW_SELECTED_SAVE} "${CIV5_BNW_INSTALL_ROOT}")
                if(CIV5_BNW_SDK_ROOT)
                    list(APPEND _grand_args "${CIV5_BNW_SDK_ROOT}/CvGameCoreSource")
                endif()
                add_test(NAME runtime.bnw_grand_${_grand_test}
                    COMMAND civis_bnw_grand_${_grand_test}_tests ${_grand_args})
            endforeach()
            set(_culture_args ${CIV5_BNW_SELECTED_SAVE} "${CIV5_BNW_INSTALL_ROOT}")
            if(CIV5_BNW_SDK_ROOT)
                list(APPEND _culture_args "${CIV5_BNW_SDK_ROOT}/CvGameCoreSource")
            endif()
            add_test(NAME runtime.bnw_total_jons_culture
                COMMAND civis_bnw_total_jons_culture_tests ${_culture_args})
            add_test(NAME runtime.bnw_player_tourism_influence
                COMMAND civis_bnw_player_tourism_influence_tests ${_culture_args})
            add_test(NAME runtime.bnw_grand_culture_priority
                COMMAND civis_bnw_grand_culture_priority_tests ${_culture_args})
            set_tests_properties(runtime.bnw_grand_culture_priority PROPERTIES TIMEOUT 1800)
            add_test(NAME runtime.bnw_grand_first_roll
                COMMAND civis_bnw_grand_first_roll_tests ${_culture_args})
            set_tests_properties(runtime.bnw_grand_first_roll PROPERTIES TIMEOUT 2400)
            add_test(NAME runtime.bnw_grand_spaceship_priority
                COMMAND civis_bnw_grand_spaceship_priority_tests ${_culture_args})
            if(TARGET civis_bnw_game_activation_happiness_tests)
                add_test(NAME runtime.bnw_game_activation_happiness COMMAND civis_bnw_game_activation_happiness_tests
                    ${CIV5_BNW_SELECTED_SAVE}
                    "${CIV5_BNW_INSTALL_ROOT}")
            endif()
            add_test(NAME runtime.bnw_happiness_update_prefix COMMAND civis_bnw_happiness_update_prefix_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_unit_order_owners COMMAND civis_bnw_unit_order_owners_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_unit_finish_moves COMMAND civis_bnw_unit_finish_moves_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_unit_reset COMMAND civis_bnw_unit_reset_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_player_turn_deactivation COMMAND civis_bnw_player_turn_deactivation_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_notification_cleanup COMMAND civis_bnw_notification_cleanup_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_player_turn_suffix COMMAND civis_bnw_player_turn_suffix_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_busy_state COMMAND civis_bnw_busy_state_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_ready_units COMMAND civis_bnw_ready_units_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_founder_religion_selected COMMAND civis_bnw_founder_religion_native_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_religion_happiness_selected COMMAND civis_bnw_religion_happiness_terms_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.ui_ideology_layout COMMAND civis_ui_ideology_layout_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.ui_ideology_frame_owner COMMAND civis_ui_ideology_frame_owner_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.ui_ideology_compositor COMMAND civis_ui_ideology_compositor_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.ui_live_visibility COMMAND civis_ui_live_visibility_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}"
                ${CIV5_ASSETS_ROOT})
            add_test(NAME runtime.selected_bnw_preview_bootstrap COMMAND civis_selected_bnw_preview_host_tests
                ${CIV5_BNW_SELECTED_SAVE}
                ${CIV5_ASSETS_ROOT}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.selected_bnw_civis_preview
                COMMAND civis_selected_bnw_civis_preview_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_unit_roster_checkpoint
                COMMAND civis_bnw_unit_roster_checkpoint_tests
                ${CIV5_BNW_SELECTED_SAVE})
            add_test(NAME runtime.selected_bnw_research_preview COMMAND civis_selected_bnw_research_preview_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.selected_bnw_research_lua_inventory_retail
                COMMAND civis_selected_bnw_research_lua_inventory_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.selected_bnw_stock_ui_lua_step_retail
                COMMAND civis_selected_bnw_stock_ui_lua_step_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.selected_bnw_icon_support_step_source
                COMMAND civis_selected_bnw_icon_support_step_tests
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.selected_bnw_icon_support_step_retail
                COMMAND civis_selected_bnw_icon_support_step_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.selected_bnw_civilopedia_preamble_source
                COMMAND civis_selected_bnw_civilopedia_preamble_tests
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.selected_bnw_civilopedia_layout_source
                COMMAND civis_selected_bnw_civilopedia_layout_tests
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_civilopedia_scroll_subtree_source
                COMMAND civis_bnw_civilopedia_scroll_subtree_tests
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.civis_civilopedia_geometry_source
                COMMAND civis_civilopedia_geometry_tests
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.civis_civilopedia_articles_source
                COMMAND civis_civilopedia_articles_tests
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.civis_civilopedia_measured_hit_source
                COMMAND civis_civilopedia_measured_hit_tests
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.selected_bnw_civilopedia_preamble_retail
                COMMAND civis_selected_bnw_civilopedia_preamble_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.bnw_policy_selected_probe COMMAND civis_bnw_policy_selected_probe_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
            add_test(NAME runtime.selected_bnw_ui_audio_retail COMMAND civis_selected_bnw_ui_audio_tests
                ${CIV5_BNW_SELECTED_SAVE}
                "${CIV5_BNW_INSTALL_ROOT}")
        endif()
    endif()
    add_executable(civis_bnw_live_scene_binding_tests
        ../tests/bnw_live_scene_binding_tests.cpp)
    target_link_libraries(civis_bnw_live_scene_binding_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_live_scene_binding_tests PRIVATE -static)
    endif()
    if(WIN32)
        add_executable(civis_bnw_live_preview_early_launch_tests ../tests/bnw_live_preview_early_launch_tests.cpp)
        target_link_libraries(civis_bnw_live_preview_early_launch_tests PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(civis_bnw_live_preview_early_launch_tests PRIVATE -static)
        endif()
        if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
            add_test(NAME runtime.bnw_live_preview_early_launch COMMAND civis_bnw_live_preview_early_launch_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}" "${CIV5_ASSETS_ROOT}")
        endif()
    endif()
    add_executable(civis_bnw_live_preview_launch_tests ../tests/bnw_live_preview_launch_tests.cpp)
    target_link_libraries(civis_bnw_live_preview_launch_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_live_preview_launch_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND
       EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_live_scene_binding COMMAND civis_bnw_live_scene_binding_tests
            ${CIV5_BNW_SELECTED_SAVE}
            "${CIV5_BNW_INSTALL_ROOT}")
        add_test(NAME runtime.bnw_live_preview_launch COMMAND civis_bnw_live_preview_launch_tests
            ${CIV5_BNW_SELECTED_SAVE}
            "${CIV5_BNW_INSTALL_ROOT}"
            "${CIV5_ASSETS_ROOT}")
    endif()
    add_executable(civis_ui_audio_events_tests ../tests/ui_audio_events_tests.cpp)
    target_link_libraries(civis_ui_audio_events_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_audio_events_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_audio_events COMMAND civis_ui_audio_events_tests)
    add_executable(civis_ui_audio_assets_tests ../tests/ui_audio_assets_tests.cpp)
    target_link_libraries(civis_ui_audio_assets_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_audio_assets_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_audio_assets COMMAND civis_ui_audio_assets_tests)
    add_executable(civis_ui_selected_audio_tests ../tests/ui_selected_audio_tests.cpp)
    target_link_libraries(civis_ui_selected_audio_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_selected_audio_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_selected_audio COMMAND civis_ui_selected_audio_tests)
    add_executable(civis_ui_winmm_audio_tests ../tests/ui_winmm_audio_tests.cpp)
    target_link_libraries(civis_ui_winmm_audio_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_winmm_audio_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_winmm_audio COMMAND civis_ui_winmm_audio_tests)
    add_executable(civis_live_ui_audio_tests ../tests/live_ui_audio_tests.cpp)
    target_link_libraries(civis_live_ui_audio_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_live_ui_audio_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.live_ui_audio COMMAND civis_live_ui_audio_tests)
    add_executable(civis_system_screen_tests ../tests/system_screen_tests.cpp)
    target_link_libraries(civis_system_screen_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_system_screen_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.system_screens COMMAND civis_system_screen_tests
        ${CIV5_ASSETS_ROOT})
    add_executable(civis_ui_world_view_tests ../tests/ui_world_view_tests.cpp)
    target_link_libraries(civis_ui_world_view_tests PRIVATE civis_runtime civ5_terrain)
    if(MINGW)
        target_link_options(civis_ui_world_view_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_world_view COMMAND civis_ui_world_view_tests)
    add_executable(civis_ui_unit_name_font_selection_tests ../tests/ui_unit_name_font_selection_tests.cpp)
    target_link_libraries(civis_ui_unit_name_font_selection_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_unit_name_font_selection_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_unit_name_font_selection
        COMMAND civis_ui_unit_name_font_selection_tests "${CIV5_ASSETS_ROOT}")
    add_executable(civis_ui_bnw_city_production_backend_tests ../tests/ui_bnw_city_production_backend_tests.cpp)
    target_link_libraries(civis_ui_bnw_city_production_backend_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_bnw_city_production_backend_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_bnw_city_production_backend COMMAND civis_ui_bnw_city_production_backend_tests)
    add_executable(civis_ui_bnw_production_choices_tests ../tests/ui_bnw_production_choices_tests.cpp)
    target_link_libraries(civis_ui_bnw_production_choices_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_bnw_production_choices_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_bnw_production_choices COMMAND civis_ui_bnw_production_choices_tests)
    add_executable(civis_ui_progression_tests ../tests/ui_progression_tests.cpp)
    target_link_libraries(civis_ui_progression_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_progression_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_progression COMMAND civis_ui_progression_tests)
    add_executable(civis_ui_city_tests ../tests/ui_city_tests.cpp)
    target_link_libraries(civis_ui_city_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_city_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_city COMMAND civis_ui_city_tests)
    add_executable(civis_ui_notification_log_popup_tests ../tests/ui_notification_log_popup_tests.cpp)
    target_link_libraries(civis_ui_notification_log_popup_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_notification_log_popup_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_notification_log_popup COMMAND civis_ui_notification_log_popup_tests
        ${CIV5_ASSETS_ROOT})
    add_executable(civis_ui_overview_tests ../tests/ui_overview_tests.cpp)
    target_link_libraries(civis_ui_overview_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_overview_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_overview COMMAND civis_ui_overview_tests
        ${CIV5_ASSETS_ROOT})
    add_executable(civis_ui_economic_tabs_tests ../tests/ui_economic_tabs_tests.cpp)
    target_link_libraries(civis_ui_economic_tabs_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_economic_tabs_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.ui_economic_tabs_source
            COMMAND civis_ui_economic_tabs_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_ui_economic_close_tests ../tests/ui_economic_close_tests.cpp)
    target_link_libraries(civis_ui_economic_close_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_economic_close_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.ui_economic_close_source
            COMMAND civis_ui_economic_close_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_ui_economic_lifecycle_tests
        ../tests/ui_economic_lifecycle_tests.cpp)
    target_link_libraries(civis_ui_economic_lifecycle_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_economic_lifecycle_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.ui_economic_lifecycle_source
            COMMAND civis_ui_economic_lifecycle_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_ui_economic_open_bridge_tests
        ../tests/ui_economic_open_bridge_tests.cpp)
    target_link_libraries(civis_ui_economic_open_bridge_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_economic_open_bridge_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.ui_economic_open_bridge_source
            COMMAND civis_ui_economic_open_bridge_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_ui_economic_interaction_tests
        ../tests/ui_economic_interaction_tests.cpp)
    target_link_libraries(civis_ui_economic_interaction_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_economic_interaction_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.ui_economic_interaction_source
            COMMAND civis_ui_economic_interaction_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_ui_economic_audio_tests
        ../tests/ui_economic_audio_tests.cpp)
    target_link_libraries(civis_ui_economic_audio_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_economic_audio_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.ui_economic_audio_source
            COMMAND civis_ui_economic_audio_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_ui_economic_active_tabs_tests
        ../tests/ui_economic_active_tabs_tests.cpp)
    target_link_libraries(civis_ui_economic_active_tabs_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_economic_active_tabs_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.ui_economic_active_tabs_source
            COMMAND civis_ui_economic_active_tabs_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_ui_economic_fields_tests
        ../tests/ui_economic_fields_tests.cpp)
    target_link_libraries(civis_ui_economic_fields_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_economic_fields_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.ui_economic_fields_source
            COMMAND civis_ui_economic_fields_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_ui_economic_popup_feed_tests
        ../tests/ui_economic_popup_feed_tests.cpp)
    target_link_libraries(civis_ui_economic_popup_feed_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_economic_popup_feed_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.ui_economic_popup_feed_source
            COMMAND civis_ui_economic_popup_feed_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_ui_economic_live_input_tests
        ../tests/ui_economic_live_input_tests.cpp)
    target_link_libraries(civis_ui_economic_live_input_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_economic_live_input_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.ui_economic_live_input_source
            COMMAND civis_ui_economic_live_input_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_ui_economic_gold_input_tests
        ../tests/ui_economic_gold_input_tests.cpp)
    target_link_libraries(civis_ui_economic_gold_input_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_economic_gold_input_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.ui_economic_gold_input_source
            COMMAND civis_ui_economic_gold_input_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_ui_economic_scene_tests
        ../tests/ui_economic_scene_tests.cpp)
    target_link_libraries(civis_ui_economic_scene_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_economic_scene_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.ui_economic_scene_source
            COMMAND civis_ui_economic_scene_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_ui_trade_route_overview_input_tests
        ../tests/ui_trade_route_overview_input_tests.cpp)
    target_link_libraries(civis_ui_trade_route_overview_input_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_trade_route_overview_input_tests PRIVATE -static)
    endif()
    add_executable(civis_ui_trade_route_tab_geometry_tests
        ../validation/trade_route_tab_geometry_tests.cpp)
    target_link_libraries(civis_ui_trade_route_tab_geometry_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_trade_route_tab_geometry_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_trade_route_tab_geometry
        COMMAND civis_ui_trade_route_tab_geometry_tests
            ${CIV5_ASSETS_ROOT})
    add_executable(civis_ui_trade_route_available_rows_tests
        ../tests/ui_trade_route_available_rows_tests.cpp)
    target_link_libraries(civis_ui_trade_route_available_rows_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_trade_route_available_rows_tests PRIVATE -static)
    endif()
    add_executable(civis_ui_trade_route_incoming_rows_tests
        ../tests/ui_trade_route_incoming_rows_tests.cpp)
    target_link_libraries(civis_ui_trade_route_incoming_rows_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_trade_route_incoming_rows_tests PRIVATE -static)
    endif()
    add_executable(civis_ui_trade_route_instance_plan_tests
        ../tests/ui_trade_route_instance_plan_tests.cpp)
    target_link_libraries(civis_ui_trade_route_instance_plan_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_trade_route_instance_plan_tests PRIVATE -static)
    endif()
    add_executable(civis_ui_new_era_popup_audio_tests
        ../tests/ui_new_era_popup_audio_tests.cpp)
    target_link_libraries(civis_ui_new_era_popup_audio_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_new_era_popup_audio_tests PRIVATE -static)
    endif()
    add_executable(civis_ui_new_era_audio_asset_tests
        ../tests/ui_new_era_audio_asset_tests.cpp)
    target_link_libraries(civis_ui_new_era_audio_asset_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_new_era_audio_asset_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.ui_new_era_audio_asset_source
            COMMAND civis_ui_new_era_audio_asset_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_new_era_audio_completion_owner_tests
        ../tests/civis_new_era_audio_completion_owner_tests.cpp)
    target_link_libraries(civis_new_era_audio_completion_owner_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_new_era_audio_completion_owner_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.new_era_audio_completion_owner_source
            COMMAND civis_new_era_audio_completion_owner_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_ui_new_era_audio_delivery_tests
        ../tests/ui_new_era_audio_delivery_tests.cpp)
    target_link_libraries(civis_ui_new_era_audio_delivery_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_new_era_audio_delivery_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.ui_new_era_audio_delivery_source
            COMMAND civis_ui_new_era_audio_delivery_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.ui_new_era_popup_audio_source
            COMMAND civis_ui_new_era_popup_audio_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.ui_trade_route_instance_plan_source
            COMMAND civis_ui_trade_route_instance_plan_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.ui_trade_route_incoming_rows_source
            COMMAND civis_ui_trade_route_incoming_rows_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.ui_trade_route_available_rows_source
            COMMAND civis_ui_trade_route_available_rows_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.ui_trade_route_overview_input_source
            COMMAND civis_ui_trade_route_overview_input_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
    add_executable(civis_ui_popup_tests ../tests/ui_popup_tests.cpp)
    target_link_libraries(civis_ui_popup_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_popup_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_popup COMMAND civis_ui_popup_tests)
    add_executable(civis_ui_religion_belief_preview_tests ../tests/ui_religion_belief_preview_tests.cpp)
    target_link_libraries(civis_ui_religion_belief_preview_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_religion_belief_preview_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_religion_belief_preview COMMAND civis_ui_religion_belief_preview_tests)
    add_executable(civis_bnw_belief_availability_tests ../tests/bnw_belief_availability_tests.cpp)
    target_link_libraries(civis_bnw_belief_availability_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_belief_availability_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_belief_availability COMMAND civis_bnw_belief_availability_tests)
    add_executable(civis_bnw_first_contact_approach_random_quote_tests
        ../tests/bnw_first_contact_approach_random_quote_tests.cpp)
    target_link_libraries(civis_bnw_first_contact_approach_random_quote_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_first_contact_approach_random_quote_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_first_contact_approach_random_quote
        COMMAND civis_bnw_first_contact_approach_random_quote_tests)
    add_executable(civis_bnw_notification_popup_owner_tests
        ../tests/bnw_notification_popup_owner_tests.cpp)
    target_link_libraries(civis_bnw_notification_popup_owner_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_notification_popup_owner_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_notification_popup_owner
        COMMAND civis_bnw_notification_popup_owner_tests)
    add_executable(civis_bnw_can_declare_war_lua_tests
        ../tests/bnw_can_declare_war_lua_tests.cpp)
    target_link_libraries(civis_bnw_can_declare_war_lua_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_can_declare_war_lua_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_can_declare_war_lua
        COMMAND civis_bnw_can_declare_war_lua_tests)
    add_executable(civis_bnw_fresh_feature_events_tests
        ../tests/bnw_fresh_feature_events_tests.cpp)
    target_link_libraries(civis_bnw_fresh_feature_events_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_feature_events_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_feature_events
        COMMAND civis_bnw_fresh_feature_events_tests)
    add_executable(civis_bnw_selected_notification_backend_tests
        ../tests/bnw_selected_notification_backend_tests.cpp)
    target_link_libraries(civis_bnw_selected_notification_backend_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_selected_notification_backend_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_selected_notification_backend
        COMMAND civis_bnw_selected_notification_backend_tests)
    add_executable(civis_bnw_detached_gameplay_lua_tests
        ../tests/bnw_detached_gameplay_lua_tests.cpp)
    target_link_libraries(civis_bnw_detached_gameplay_lua_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_detached_gameplay_lua_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_detached_gameplay_lua
        COMMAND civis_bnw_detached_gameplay_lua_tests)
    add_executable(civis_bnw_fresh_feature_effects_tests
        ../tests/bnw_fresh_feature_effects_tests.cpp)
    target_link_libraries(civis_bnw_fresh_feature_effects_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_feature_effects_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_feature_effects
        COMMAND civis_bnw_fresh_feature_effects_tests)
    add_executable(civis_bnw_fresh_start_coast_tests
        ../tests/bnw_fresh_start_coast_tests.cpp)
    target_link_libraries(civis_bnw_fresh_start_coast_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_start_coast_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_start_coast
        COMMAND civis_bnw_fresh_start_coast_tests)
    add_executable(civis_bnw_fresh_start_resources_tests
        ../tests/bnw_fresh_start_resources_tests.cpp)
    target_link_libraries(civis_bnw_fresh_start_resources_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_start_resources_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_start_resources
        COMMAND civis_bnw_fresh_start_resources_tests)
    add_executable(civis_bnw_fresh_start_luxuries_tests
        ../tests/bnw_fresh_start_luxuries_tests.cpp)
    target_link_libraries(civis_bnw_fresh_start_luxuries_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_start_luxuries_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_start_luxuries
        COMMAND civis_bnw_fresh_start_luxuries_tests)
    add_executable(civis_bnw_fresh_start_players_tests
        ../tests/bnw_fresh_start_players_tests.cpp)
    target_link_libraries(civis_bnw_fresh_start_players_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_start_players_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_start_players
        COMMAND civis_bnw_fresh_start_players_tests)
    add_executable(civis_bnw_fresh_pregame_roster_tests
        ../tests/bnw_fresh_pregame_roster_tests.cpp)
    target_link_libraries(civis_bnw_fresh_pregame_roster_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_pregame_roster_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_pregame_roster
        COMMAND civis_bnw_fresh_pregame_roster_tests)
    add_executable(civis_bnw_fresh_player_colors_tests
        ../tests/bnw_fresh_player_colors_tests.cpp)
    target_link_libraries(civis_bnw_fresh_player_colors_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_player_colors_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_player_colors
        COMMAND civis_bnw_fresh_player_colors_tests)
    add_executable(civis_bnw_fresh_player_init_prefix_tests
        ../tests/bnw_fresh_player_init_prefix_tests.cpp)
    target_link_libraries(civis_bnw_fresh_player_init_prefix_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_player_init_prefix_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_player_init_prefix
        COMMAND civis_bnw_fresh_player_init_prefix_tests)
    add_executable(civis_bnw_fresh_alive_prefix_tests
        ../tests/bnw_fresh_alive_prefix_tests.cpp)
    target_link_libraries(civis_bnw_fresh_alive_prefix_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_alive_prefix_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_alive_prefix
        COMMAND civis_bnw_fresh_alive_prefix_tests)
    add_executable(civis_bnw_fresh_diplo_votes_tests
        ../tests/bnw_fresh_diplo_votes_tests.cpp)
    target_link_libraries(civis_bnw_fresh_diplo_votes_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_diplo_votes_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_diplo_votes
        COMMAND civis_bnw_fresh_diplo_votes_tests)
    add_executable(civis_bnw_fresh_league_state_tests
        ../tests/bnw_fresh_league_state_tests.cpp)
    target_link_libraries(civis_bnw_fresh_league_state_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_league_state_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_league_state
        COMMAND civis_bnw_fresh_league_state_tests)
    add_executable(civis_bnw_fresh_team_init_prefix_tests
        ../tests/bnw_fresh_team_init_prefix_tests.cpp)
    target_link_libraries(civis_bnw_fresh_team_init_prefix_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_team_init_prefix_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_team_init_prefix
        COMMAND civis_bnw_fresh_team_init_prefix_tests)
    add_executable(civis_bnw_fresh_team_info_init_tests
        ../tests/bnw_fresh_team_info_init_tests.cpp)
    target_link_libraries(civis_bnw_fresh_team_info_init_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_team_info_init_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_team_info_init
        COMMAND civis_bnw_fresh_team_info_init_tests)
    add_executable(civis_bnw_fresh_player_reset_prefix_tests
        ../tests/bnw_fresh_player_reset_prefix_tests.cpp)
    target_link_libraries(civis_bnw_fresh_player_reset_prefix_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_player_reset_prefix_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_player_reset_prefix
        COMMAND civis_bnw_fresh_player_reset_prefix_tests)
    add_executable(civis_bnw_fresh_player_info_reset_tests
        ../tests/bnw_fresh_player_info_reset_tests.cpp)
    target_link_libraries(civis_bnw_fresh_player_info_reset_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_player_info_reset_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_player_info_reset
        COMMAND civis_bnw_fresh_player_info_reset_tests)
    add_executable(civis_bnw_fresh_economic_ai_init_tests
        ../tests/bnw_fresh_economic_ai_init_tests.cpp)
    target_link_libraries(civis_bnw_fresh_economic_ai_init_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_economic_ai_init_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_economic_ai_init
        COMMAND civis_bnw_fresh_economic_ai_init_tests)
    add_executable(civis_bnw_fresh_military_ai_init_tests
        ../tests/bnw_fresh_military_ai_init_tests.cpp)
    target_link_libraries(civis_bnw_fresh_military_ai_init_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_military_ai_init_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_military_ai_init
        COMMAND civis_bnw_fresh_military_ai_init_tests)
    add_executable(civis_bnw_fresh_city_specialization_init_tests
        ../tests/bnw_fresh_city_specialization_init_tests.cpp)
    target_link_libraries(civis_bnw_fresh_city_specialization_init_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_city_specialization_init_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_city_specialization_init
        COMMAND civis_bnw_fresh_city_specialization_init_tests)
    add_executable(civis_bnw_fresh_game_builder_tests
        ../tests/bnw_fresh_game_builder_tests.cpp)
    target_link_libraries(civis_bnw_fresh_game_builder_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_game_builder_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_game_builder
        COMMAND civis_bnw_fresh_game_builder_tests)
    add_executable(civis_bnw_fresh_archive_vector_tests
        ../tests/bnw_fresh_archive_vector_tests.cpp)
    target_link_libraries(civis_bnw_fresh_archive_vector_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_archive_vector_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_archive_vector
        COMMAND civis_bnw_fresh_archive_vector_tests)
    add_executable(civis_bnw_fresh_slot_info_tests
        ../tests/bnw_fresh_slot_info_tests.cpp)
    target_link_libraries(civis_bnw_fresh_slot_info_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_slot_info_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_slot_info
        COMMAND civis_bnw_fresh_slot_info_tests)
    add_executable(civis_bnw_fresh_player_objects_tests
        ../tests/bnw_fresh_player_objects_tests.cpp)
    target_link_libraries(civis_bnw_fresh_player_objects_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_player_objects_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_player_objects
        COMMAND civis_bnw_fresh_player_objects_tests)
    add_executable(civis_bnw_fresh_player_uninit_plan_tests
        ../tests/bnw_fresh_player_uninit_plan_tests.cpp)
    target_link_libraries(civis_bnw_fresh_player_uninit_plan_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_fresh_player_uninit_plan_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_fresh_player_uninit_plan
        COMMAND civis_bnw_fresh_player_uninit_plan_tests)
    add_executable(civis_bnw_unit_roster_checkpoint_tests
        ../tests/bnw_unit_roster_checkpoint_tests.cpp)
    target_link_libraries(civis_bnw_unit_roster_checkpoint_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_unit_roster_checkpoint_tests PRIVATE -static)
    endif()
    add_executable(civis_bnw_unit_class_death_tests
        ../tests/bnw_unit_class_death_tests.cpp)
    target_link_libraries(civis_bnw_unit_class_death_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_unit_class_death_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_unit_class_death
        COMMAND civis_bnw_unit_class_death_tests)
    add_executable(civis_bnw_unit_support_death_tests
        ../tests/bnw_unit_support_death_tests.cpp)
    target_link_libraries(civis_bnw_unit_support_death_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_unit_support_death_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_unit_support_death
        COMMAND civis_bnw_unit_support_death_tests)
    add_executable(civis_bnw_unit_death_xp_quote_tests
        ../tests/bnw_unit_death_xp_quote_tests.cpp)
    target_link_libraries(civis_bnw_unit_death_xp_quote_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_unit_death_xp_quote_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_unit_death_xp_quote
        COMMAND civis_bnw_unit_death_xp_quote_tests)
    add_executable(civis_bnw_unit_capture_death_quote_tests
        ../tests/bnw_unit_capture_death_quote_tests.cpp)
    target_link_libraries(civis_bnw_unit_capture_death_quote_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_unit_capture_death_quote_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_unit_capture_death_quote
        COMMAND civis_bnw_unit_capture_death_quote_tests)
    add_executable(civis_bnw_unit_kill_frontier_tests
        ../tests/bnw_unit_kill_frontier_tests.cpp)
    target_link_libraries(civis_bnw_unit_kill_frontier_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_unit_kill_frontier_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_unit_kill_frontier
        COMMAND civis_bnw_unit_kill_frontier_tests)
    add_executable(civis_bnw_add_popup_queue_tests
        ../tests/bnw_add_popup_queue_tests.cpp)
    target_link_libraries(civis_bnw_add_popup_queue_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_add_popup_queue_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_add_popup_queue
        COMMAND civis_bnw_add_popup_queue_tests)
    add_executable(civis_ui_glyphgen_tests ../tests/ui_glyphgen_tests.cpp)
    target_link_libraries(civis_ui_glyphgen_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_ui_glyphgen_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.ui_glyphgen COMMAND civis_ui_glyphgen_tests)
endif()
if(BUILD_TESTING)
    foreach(_graphics_assert_target IN ITEMS civis_bnw_live_preview_launch_tests
            civis_ui_world_view_tests civis_ui_screen_compositor_tests
            civis_bnw_selected_gameplay_lua_owner_tests)
        if(TARGET ${_graphics_assert_target})
            if(MSVC)
                target_compile_options(${_graphics_assert_target} PRIVATE /UNDEBUG)
            else()
                target_compile_options(${_graphics_assert_target} PRIVATE -UNDEBUG)
            endif()
        endif()
    endforeach()
endif()
if(MSVC)
    target_compile_options(civis_runtime PRIVATE /W4 /utf-8)
else()
    target_compile_options(civis_runtime PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Additional selected graphics owners; installed sources are explicit inputs.
if(BUILD_TESTING AND WIN32)
    add_executable(civis_bnw_native_archive_v6_entry_admission_tests
        ../tests/bnw_native_archive_v6_entry_admission_tests.cpp)
    target_include_directories(civis_bnw_native_archive_v6_entry_admission_tests PRIVATE ../include ../src/ui)
    target_compile_features(civis_bnw_native_archive_v6_entry_admission_tests PRIVATE cxx_std_20)
    if(MINGW)
        target_compile_options(civis_bnw_native_archive_v6_entry_admission_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
        target_link_options(civis_bnw_native_archive_v6_entry_admission_tests PRIVATE -static -Wl,--strip-debug)
    endif()
    add_test(NAME runtime.bnw_native_archive_v6_entry_admission COMMAND civis_bnw_native_archive_v6_entry_admission_tests)
    add_executable(civis_bnw_native_config_dictionary_tests
        ../tests/bnw_native_config_dictionary_tests.cpp
        ../src/ui/bnw_native_config_dictionary.cpp)
    target_compile_features(civis_bnw_native_config_dictionary_tests PRIVATE cxx_std_20)
    if(MINGW)
        target_compile_options(civis_bnw_native_config_dictionary_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
        target_link_options(civis_bnw_native_config_dictionary_tests PRIVATE -static -Wl,--strip-debug)
    endif()
    add_test(NAME runtime.bnw_native_config_dictionary COMMAND civis_bnw_native_config_dictionary_tests)
    add_executable(civis_bnw_native_callback_owner_guard_tests
        ../tests/bnw_native_callback_owner_guard_tests.cpp)
    target_compile_features(civis_bnw_native_callback_owner_guard_tests PRIVATE cxx_std_20)
    if(MINGW)
        target_compile_options(civis_bnw_native_callback_owner_guard_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
        target_link_options(civis_bnw_native_callback_owner_guard_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_native_callback_owner_guard
        COMMAND civis_bnw_native_callback_owner_guard_tests)
    add_executable(civis_bnw_info_type_cache_source_tests
        ../tests/bnw_info_type_cache_source_tests.cpp)
    target_compile_features(civis_bnw_info_type_cache_source_tests PRIVATE cxx_std_20)
    target_link_libraries(civis_bnw_info_type_cache_source_tests PRIVATE civis_runtime)
    if(MINGW)
        target_compile_options(civis_bnw_info_type_cache_source_tests PRIVATE -UNDEBUG)
        target_link_options(civis_bnw_info_type_cache_source_tests PRIVATE -static -Wl,--strip-debug)
    endif()
    add_test(NAME runtime.bnw_info_type_cache_source
        COMMAND civis_bnw_info_type_cache_source_tests)
endif()

if(BUILD_TESTING)
    set(CIV5_BNW_SDK_ROOT "" CACHE PATH "Installed Civ5 SDK source root")
    foreach(_new_graphics_test IN ITEMS bnw_unit_panel_stat_rows_feed
            anchored_text_measurement measured_readonly_text native_extended_font_style
            civis_selected_game_option_owner civis_selected_game_option_lua
            civis_civilopedia_units_query_owner civis_civilopedia_units_query_lua
            civis_civilopedia_buildings_query_owner civis_selected_buildings_description
            civis_bnw_selected_units_query_parent civis_bnw_selected_textdata_parent
            civis_bnw_selected_buildings_parent
            civis_selected_civilopedia_text_data_owner bnw_minimap_fog_callback_pin
            bnw_minimap_fog_preparation win32_viewport_transaction bnw_tech_tree_lifecycle)
        if(_new_graphics_test STREQUAL "win32_viewport_transaction" AND NOT WIN32)
            continue()
        endif()
        if(_new_graphics_test MATCHES "^civis_")
            set(_new_graphics_target ${_new_graphics_test}_tests)
        else()
            set(_new_graphics_target civis_${_new_graphics_test}_tests)
        endif()
        add_executable(${_new_graphics_target} ../tests/${_new_graphics_test}_tests.cpp)
        if(_new_graphics_test MATCHES "^(bnw_minimap_fog_preparation|win32_viewport_transaction|bnw_tech_tree_lifecycle)$")
            if(MSVC)
                target_compile_options(${_new_graphics_target} PRIVATE /UNDEBUG)
            else()
                target_compile_options(${_new_graphics_target} PRIVATE -UNDEBUG)
            endif()
        endif()
        target_link_libraries(${_new_graphics_target} PRIVATE civis_runtime)
        if(MINGW)
            target_link_options(${_new_graphics_target} PRIVATE -static)
        endif()
        if(_new_graphics_test STREQUAL "bnw_unit_panel_stat_rows_feed")
            if(CIV5_BNW_INSTALL_ROOT AND EXISTS "${CIV5_BNW_SELECTED_SAVE}")
                add_test(NAME runtime.${_new_graphics_test} COMMAND ${_new_graphics_target}
                    "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
            endif()
        elseif(_new_graphics_test STREQUAL "civis_selected_game_option_owner")
            if(CIV5_BNW_SDK_ROOT)
                add_test(NAME runtime.${_new_graphics_test} COMMAND ${_new_graphics_target}
                    "${CIV5_BNW_SDK_ROOT}")
            endif()
        elseif(_new_graphics_test STREQUAL "civis_bnw_selected_units_query_parent" OR
               _new_graphics_test STREQUAL "civis_bnw_selected_textdata_parent" OR
               _new_graphics_test STREQUAL "civis_bnw_selected_buildings_parent")
            if(CIV5_BNW_INSTALL_ROOT AND EXISTS "${CIV5_BNW_SELECTED_SAVE}")
                add_test(NAME runtime.${_new_graphics_test} COMMAND ${_new_graphics_target}
                    "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
                set_tests_properties(runtime.${_new_graphics_test} PROPERTIES TIMEOUT 1200)
            endif()
        elseif(_new_graphics_test STREQUAL "civis_selected_game_option_lua" OR
               _new_graphics_test STREQUAL "civis_civilopedia_units_query_owner" OR
               _new_graphics_test STREQUAL "civis_civilopedia_units_query_lua" OR
               _new_graphics_test STREQUAL "civis_civilopedia_buildings_query_owner" OR
               _new_graphics_test STREQUAL "bnw_tech_tree_lifecycle")
            if(CIV5_BNW_INSTALL_ROOT)
                add_test(NAME runtime.${_new_graphics_test} COMMAND ${_new_graphics_target}
                    "${CIV5_BNW_INSTALL_ROOT}")
            endif()
        else()
            add_test(NAME runtime.${_new_graphics_test} COMMAND ${_new_graphics_target})
        endif()
    endforeach()
endif()


# Immutable native feature Info reads only; visibility and placement are separate.
if(BUILD_TESTING)
    add_executable(civis_bnw_feature_art_tag_read_owner_tests
        ../tests/bnw_feature_art_tag_read_owner_tests.cpp)
    target_link_libraries(civis_bnw_feature_art_tag_read_owner_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_feature_art_tag_read_owner_tests PRIVATE cxx_std_20)
    if(MINGW)
        target_link_options(civis_bnw_feature_art_tag_read_owner_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_feature_art_tag_read_owner
        COMMAND civis_bnw_feature_art_tag_read_owner_tests)
endif()

# Strict whole raw UTF8 input replay, independent original reference/SHA/report required.
if(BUILD_TESTING AND WIN32)
    add_executable(civis_bnw_whole_raw_label_input_cross_tests
        ../validation/whole_raw_cross_tests.cpp)
    target_include_directories(civis_bnw_whole_raw_label_input_cross_tests PRIVATE ../validation)
    target_compile_features(civis_bnw_whole_raw_label_input_cross_tests PRIVATE cxx_std_20)
    target_link_libraries(civis_bnw_whole_raw_label_input_cross_tests PRIVATE civis_runtime bcrypt)
    if(MINGW)
        target_link_options(civis_bnw_whole_raw_label_input_cross_tests PRIVATE -municode -static)
    endif()
endif()

if(BUILD_TESTING AND TARGET civis_bnw_native_label_field_storage_tests)
    add_test(NAME runtime.bnw_native_label_field_storage COMMAND civis_bnw_native_label_field_storage_tests)
endif()

# SAME retained font glyph-index CPU owner, with actual selected source/HWND.
if(BUILD_TESTING AND WIN32)
    add_executable(civis_bnw_system_font_glyph_index_owner_tests
        ../tests/bnw_system_font_glyph_index_owner_tests.cpp)
    target_compile_features(civis_bnw_system_font_glyph_index_owner_tests PRIVATE cxx_std_20)
    target_link_libraries(civis_bnw_system_font_glyph_index_owner_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_system_font_glyph_index_owner_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe" AND IS_DIRECTORY "${CIV5_ASSETS_ROOT}")
        add_test(NAME runtime.bnw_system_font_glyph_index_owner
            COMMAND civis_bnw_system_font_glyph_index_owner_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}" "${CIV5_ASSETS_ROOT}")
        set_tests_properties(runtime.bnw_system_font_glyph_index_owner PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Current ResourceInfo read facts; native debug/city layout owners remain pending.
if(BUILD_TESTING)
    add_executable(civis_bnw_resource_art_tag_read_owner_tests
        ../tests/bnw_resource_art_tag_read_owner_tests.cpp)
    target_compile_features(civis_bnw_resource_art_tag_read_owner_tests PRIVATE cxx_std_20)
    target_link_libraries(civis_bnw_resource_art_tag_read_owner_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_bnw_resource_art_tag_read_owner_tests PRIVATE -static)
    endif()
    add_test(NAME runtime.bnw_resource_art_tag_read_owner
        COMMAND civis_bnw_resource_art_tag_read_owner_tests)
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_resource_art_tag_read_owner_selected
            COMMAND civis_bnw_resource_art_tag_read_owner_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_resource_art_tag_read_owner_selected PROPERTIES TIMEOUT 1200)
    endif()
endif()

if(BUILD_TESTING AND TARGET civis_bnw_native_root_context_fields_tests)
    add_test(NAME runtime.bnw_native_root_context_fields COMMAND civis_bnw_native_root_context_fields_tests)
endif()


if(BUILD_TESTING AND TARGET civis_runtime)
    add_executable(civis_bnw_native_archive_basename_cache_tests
        ../tests/bnw_native_archive_basename_cache_tests.cpp)
    target_include_directories(civis_bnw_native_archive_basename_cache_tests PRIVATE ../include ../src/ui)
    target_compile_features(civis_bnw_native_archive_basename_cache_tests PRIVATE cxx_std_20)
    if(MINGW)
        target_compile_options(civis_bnw_native_archive_basename_cache_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
        target_link_options(civis_bnw_native_archive_basename_cache_tests PRIVATE -static -Wl,--strip-debug)
    endif()
    add_test(NAME runtime.bnw_native_archive_basename_cache COMMAND civis_bnw_native_archive_basename_cache_tests)
endif()

if(BUILD_TESTING AND WIN32 AND TARGET civis_runtime)
    add_executable(civis_bnw_native_primary32_loop_journal_tests ../tests/bnw_native_primary32_loop_journal_tests.cpp)
    target_include_directories(civis_bnw_native_primary32_loop_journal_tests PRIVATE ../include ../src/ui)
    target_compile_features(civis_bnw_native_primary32_loop_journal_tests PRIVATE cxx_std_20)
    add_executable(civis_bnw_native_primary32_archives_tests ../tests/bnw_native_primary32_archives_tests.cpp)
    target_link_libraries(civis_bnw_native_primary32_archives_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_native_primary32_archives_tests PRIVATE cxx_std_20)
    if(MINGW)
        foreach(_bnw_primary32_test civis_bnw_native_primary32_loop_journal_tests civis_bnw_native_primary32_archives_tests)
            target_compile_options(${_bnw_primary32_test} PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
            target_link_options(${_bnw_primary32_test} PRIVATE -static -Wl,--strip-debug)
        endforeach()
    endif()
    add_test(NAME runtime.bnw_native_primary32_loop_journal COMMAND civis_bnw_native_primary32_loop_journal_tests)
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe" AND IS_DIRECTORY "${CIV5_ASSETS_ROOT}")
        add_test(NAME runtime.bnw_native_primary32_archives COMMAND civis_bnw_native_primary32_archives_tests
            "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}" "${CIV5_ASSETS_ROOT}")
        set_tests_properties(runtime.bnw_native_primary32_archives PROPERTIES TIMEOUT 1800)
    endif()
endif()

if(BUILD_TESTING AND WIN32 AND TARGET civis_runtime)
    add_executable(civis_bnw_native_default_namespace_policy_tests ../tests/bnw_native_default_namespace_policy_tests.cpp)
    target_link_libraries(civis_bnw_native_default_namespace_policy_tests PRIVATE civis_runtime)
    target_include_directories(civis_bnw_native_default_namespace_policy_tests PRIVATE ../src/ui)
    target_compile_features(civis_bnw_native_default_namespace_policy_tests PRIVATE cxx_std_20)
    if(MINGW)
        target_compile_options(civis_bnw_native_default_namespace_policy_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
        target_link_options(civis_bnw_native_default_namespace_policy_tests PRIVATE -static -Wl,--strip-debug)
    endif()
    add_test(NAME runtime.bnw_native_default_namespace_policy COMMAND civis_bnw_native_default_namespace_policy_tests)
endif()

if(BUILD_TESTING AND TARGET civis_runtime)
    add_executable(civis_bnw_city_food_scalars_tests ../tests/bnw_city_food_scalars_tests.cpp)
    target_link_libraries(civis_bnw_city_food_scalars_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_food_scalars_tests PRIVATE cxx_std_20)
    add_executable(civis_bnw_grand_logging_continuation_tests ../tests/bnw_grand_logging_continuation_tests.cpp)
    target_link_libraries(civis_bnw_grand_logging_continuation_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_grand_logging_continuation_tests PRIVATE cxx_std_20)
    if(MINGW)
        foreach(_bnw_scalar_logger_test civis_bnw_city_food_scalars_tests civis_bnw_grand_logging_continuation_tests)
            target_compile_options(${_bnw_scalar_logger_test} PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
            target_link_options(${_bnw_scalar_logger_test} PRIVATE -static -Wl,--strip-debug)
        endforeach()
    endif()
    add_test(NAME runtime.bnw_grand_logging_continuation COMMAND civis_bnw_grand_logging_continuation_tests)
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}")
        add_test(NAME runtime.bnw_city_food_scalars COMMAND civis_bnw_city_food_scalars_tests "${CIV5_BNW_SELECTED_SAVE}")
    endif()
endif()

if(BUILD_TESTING AND TARGET civis_runtime)
    add_executable(civis_bnw_city_food_consumption_tests ../tests/bnw_city_food_consumption_tests.cpp)
    target_link_libraries(civis_bnw_city_food_consumption_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_food_consumption_tests PRIVATE cxx_std_20)
    if(MINGW)
        target_compile_options(civis_bnw_city_food_consumption_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
        target_link_options(civis_bnw_city_food_consumption_tests PRIVATE -static -Wl,--strip-debug)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}")
        add_test(NAME runtime.bnw_city_food_consumption COMMAND civis_bnw_city_food_consumption_tests "${CIV5_BNW_SELECTED_SAVE}")
    endif()
endif()

if(BUILD_TESTING AND TARGET civis_runtime)
    add_executable(civis_glyphgen_style_admission_tests
        ../tests/glyphgen_style_admission_tests.cpp)
    target_link_libraries(civis_glyphgen_style_admission_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_glyphgen_style_admission_tests PRIVATE -static)
        target_compile_options(civis_glyphgen_style_admission_tests PRIVATE -UNDEBUG)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.glyphgen_style_admission
            COMMAND civis_glyphgen_style_admission_tests "${CIV5_BNW_INSTALL_ROOT}")
    endif()
endif()

# Bounded original-file CRC model; cannot issue a live camera/lens capability.
if(BUILD_TESTING AND TARGET civis_runtime)
    add_executable(civis_file_only_fov_input_tests ../tests/file_only_fov_input_tests.cpp)
    target_compile_features(civis_file_only_fov_input_tests PRIVATE cxx_std_20)
    target_link_libraries(civis_file_only_fov_input_tests PRIVATE civis_runtime)
    if(MINGW)
        target_link_options(civis_file_only_fov_input_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.file_only_fov_input
            COMMAND civis_file_only_fov_input_tests "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
    endif()
endif()

# Own current WorldView font descriptor issuer; native Label remains gated.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/ui/bnw_worldview_title_font_issuer.cpp)
    if(WIN32 AND BUILD_TESTING)
        add_executable(civis_bnw_worldview_font_source_scope_tests ../tests/bnw_worldview_font_source_scope_tests.cpp)
        add_executable(civis_bnw_worldview_title_font_issuer_tests ../tests/bnw_worldview_title_font_issuer_tests.cpp)
        foreach(_target civis_bnw_worldview_font_source_scope_tests civis_bnw_worldview_title_font_issuer_tests)
            target_link_libraries(${_target} PRIVATE civis_runtime)
            target_compile_features(${_target} PRIVATE cxx_std_20)
            if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
                target_compile_options(${_target} PRIVATE -UNDEBUG)
            endif()
            if(MINGW)
                target_link_options(${_target} PRIVATE -static)
            endif()
        endforeach()
        add_test(NAME runtime.bnw_worldview_font_source_scope COMMAND civis_bnw_worldview_font_source_scope_tests "${CMAKE_CURRENT_BINARY_DIR}/worldview-font-source-fixtures")
        if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe" AND EXISTS "${CIV5_ASSETS_ROOT}")
            add_test(NAME runtime.bnw_worldview_title_font_issuer COMMAND civis_bnw_worldview_title_font_issuer_tests ${CIV5_BNW_SELECTED_SAVE} "${CIV5_BNW_INSTALL_ROOT}" ${CIV5_ASSETS_ROOT})
            set_tests_properties(runtime.bnw_worldview_title_font_issuer PROPERTIES TIMEOUT 600)
        endif()
    endif()
endif()

if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_plot_yield_refresh_tests ../tests/bnw_plot_yield_refresh_tests.cpp)
    add_executable(civis_bnw_native_primary32_record_snapshot_tests ../tests/bnw_native_primary32_record_snapshot_tests.cpp)
    add_executable(civis_bnw_native_primary32_record_snapshot_actual_tests ../tests/bnw_native_primary32_record_snapshot_actual_tests.cpp)
    foreach(_target civis_bnw_plot_yield_refresh_tests civis_bnw_native_primary32_record_snapshot_tests civis_bnw_native_primary32_record_snapshot_actual_tests)
        target_link_libraries(${_target} PRIVATE civis_runtime)
        target_include_directories(${_target} PRIVATE ../src/ui)
        target_compile_features(${_target} PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${_target} PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
        endif()
        if(MINGW)
            target_link_options(${_target} PRIVATE -static -Wl,--strip-debug)
        endif()
    endforeach()
    add_test(NAME runtime.bnw_native_primary32_record_snapshot COMMAND civis_bnw_native_primary32_record_snapshot_tests)
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_plot_yield_refresh COMMAND civis_bnw_plot_yield_refresh_tests "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        if(EXISTS "${CIV5_ASSETS_ROOT}")
            add_test(NAME runtime.bnw_native_primary32_record_snapshot_actual COMMAND civis_bnw_native_primary32_record_snapshot_actual_tests "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}" "${CIV5_ASSETS_ROOT}")
            set_tests_properties(runtime.bnw_native_primary32_record_snapshot_actual PROPERTIES TIMEOUT 3600)
        endif()
    endif()
endif()

# CPU selected-host identity/lifetime gate; no GPU/native Label readiness claim.
if(TARGET civis_runtime AND BUILD_TESTING AND WIN32)
    add_executable(civis_bnw_client_scene_selected_host_tests
        ../tests/bnw_client_scene_selected_host_tests.cpp)
    target_link_libraries(civis_bnw_client_scene_selected_host_tests PRIVATE civis_runtime civ5_gl)
    target_compile_features(civis_bnw_client_scene_selected_host_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_client_scene_selected_host_tests PRIVATE -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_client_scene_selected_host_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe" AND IS_DIRECTORY "${CIV5_ASSETS_ROOT}")
        add_test(NAME runtime.bnw_client_scene_selected_host
            COMMAND civis_bnw_client_scene_selected_host_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}" "${CIV5_ASSETS_ROOT}")
        set_tests_properties(runtime.bnw_client_scene_selected_host PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Source-based standalone BNW city and timer components; no full-turn readiness.
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    foreach(_component IN ITEMS bnw_city_food_turn bnw_city_food_production_order bnw_game_turn_timer_owner bnw_plot_terrain_checkpoint_pair bnw_city_food_base_yield bnw_city_current_food_consumption bnw_retained_turn_cursor)
        add_executable(civis_${_component}_tests ../tests/${_component}_tests.cpp)
        target_link_libraries(civis_${_component}_tests PRIVATE civis_runtime)
        target_compile_features(civis_${_component}_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(civis_${_component}_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
        endif()
        if(MINGW)
            target_link_options(civis_${_component}_tests PRIVATE -static -Wl,--strip-debug)
            if(_component MATCHES "^(bnw_city_food_base_yield|bnw_city_current_food_consumption|bnw_retained_turn_cursor)$")
                target_link_options(civis_${_component}_tests PRIVATE -Wl,--stack,8388608)
            endif()
        endif()
        if(EXISTS "${CIV5_BNW_SELECTED_SAVE}")
            if(_component STREQUAL "bnw_game_turn_timer_owner")
                add_test(NAME runtime.${_component} COMMAND civis_${_component}_tests "${CIV5_BNW_SELECTED_SAVE}")
            elseif(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
                add_test(NAME runtime.${_component} COMMAND civis_${_component}_tests
                    "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
                set_tests_properties(runtime.${_component} PROPERTIES TIMEOUT 1200)
            endif()
        endif()
    endforeach()
endif()

# Actual persistent stock owner join changes SourceOwner layout; all concrete
# allocating consumers must be rebuilt against the same current runtime.
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_selected_stock_lua_owner_join_tests
        ../tests/bnw_selected_stock_lua_owner_join_tests.cpp)
    target_link_libraries(civis_bnw_selected_stock_lua_owner_join_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_selected_stock_lua_owner_join_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_selected_stock_lua_owner_join_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_selected_stock_lua_owner_join_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe" AND IS_DIRECTORY "${CIV5_ASSETS_ROOT}")
        add_test(NAME runtime.bnw_selected_stock_lua_owner_join
            COMMAND civis_bnw_selected_stock_lua_owner_join_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}" "${CIV5_ASSETS_ROOT}")
        set_tests_properties(runtime.bnw_selected_stock_lua_owner_join PROPERTIES TIMEOUT 1200)
        # Execute the five genuine HWND/WGL lifetime modes independently.
        # Serial execution prevents concurrent physical-window fixtures.
        foreach(bnw_renderer_join_mode IN ITEMS
                renderer_good renderer_alias renderer_drop
                renderer_resize renderer_foreign_release)
            add_test(NAME runtime.bnw_selected_stock_lua_owner_join.${bnw_renderer_join_mode}
                COMMAND civis_bnw_selected_stock_lua_owner_join_tests
                    "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}"
                    "${CIV5_ASSETS_ROOT}" "${bnw_renderer_join_mode}")
            set_tests_properties(
                runtime.bnw_selected_stock_lua_owner_join.${bnw_renderer_join_mode}
                PROPERTIES TIMEOUT 1200 RUN_SERIAL TRUE)
        endforeach()
    endif()
endif()

# Pure food conversion and source-bound BNW observations. These gates do not
# grant completed cache refresh, unit selection or full-turn continuation.
if(BUILD_TESTING AND TARGET civis_runtime)
    add_executable(civis_bnw_city_food_production_conversion_tests
        ../tests/bnw_city_food_production_conversion_tests.cpp)
    target_link_libraries(civis_bnw_city_food_production_conversion_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_food_production_conversion_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_food_production_conversion_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_food_production_conversion_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    add_test(NAME runtime.bnw_city_food_production_conversion
        COMMAND civis_bnw_city_food_production_conversion_tests)
endif()

if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    foreach(_bnw_observation IN ITEMS bnw_city_registry bnw_city_registry_context_join bnw_unit_cycle bnw_normal_plot_yield bnw_plot_normal_yield_refresh)
        add_executable(civis_${_bnw_observation}_tests ../tests/${_bnw_observation}_tests.cpp)
        target_link_libraries(civis_${_bnw_observation}_tests PRIVATE civis_runtime)
        target_compile_features(civis_${_bnw_observation}_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(civis_${_bnw_observation}_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
        endif()
        if(MINGW)
            target_link_options(civis_${_bnw_observation}_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
        endif()
        if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
            add_test(NAME runtime.${_bnw_observation}
                COMMAND civis_${_bnw_observation}_tests
                    "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
            set_tests_properties(runtime.${_bnw_observation} PROPERTIES TIMEOUT 1200)
        endif()
    endforeach()
endif()
# Windows stock UI requires the tested v6 activation context in every EXE.
# Deliver the RC object through static-library link interfaces.
if(WIN32 AND TARGET civis_runtime)
    # Honor the existing toolchain/cache RC compiler (current cache: windres).
    # No RC language/compiler changes occur on non-Windows builds.
    enable_language(RC)
    set(_civis_bnw_cc6_manifest
        "${CMAKE_CURRENT_SOURCE_DIR}/../resources/bnw_common_controls_v6.manifest")
    file(TO_CMAKE_PATH "${_civis_bnw_cc6_manifest}" CIVIS_BNW_CC6_MANIFEST_FILE)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/../resources/bnw_common_controls_v6.rc.in"
        "${CMAKE_CURRENT_BINARY_DIR}/bnw_common_controls_v6.rc" @ONLY)
    set_source_files_properties(
        "${CMAKE_CURRENT_BINARY_DIR}/bnw_common_controls_v6.rc"
        PROPERTIES OBJECT_DEPENDS "${_civis_bnw_cc6_manifest}")
    add_library(civis_bnw_common_controls_v6_resource OBJECT
        "${CMAKE_CURRENT_BINARY_DIR}/bnw_common_controls_v6.rc")
    set_target_properties(civis_bnw_common_controls_v6_resource
        PROPERTIES LINKER_LANGUAGE CXX)
    add_library(civis_bnw_common_controls_v6_delivery INTERFACE)
    # Link usage, not just INTERFACE_SOURCES: this propagates the actual object
    # to final EXEs even through PRIVATE static-library/LINK_ONLY chains.
    # The resource is not merely archived where no symbol would extract it.
    target_link_libraries(civis_bnw_common_controls_v6_delivery INTERFACE
        civis_bnw_common_controls_v6_resource
        $<TARGET_OBJECTS:civis_bnw_common_controls_v6_resource>)
    target_link_libraries(civis_runtime PUBLIC
        comctl32 civis_bnw_common_controls_v6_delivery)
    target_sources(civis_runtime PRIVATE
        ../src/ui/bnw_civilopedia_native_runtime_owner.cpp)
endif()

# Genuine selected host/native VM join, standalone declaration for easy rebase.
if(BUILD_TESTING AND WIN32 AND TARGET civis_runtime)
    add_executable(civis_bnw_civilopedia_native_runtime_join_tests
        ../tests/bnw_civilopedia_native_runtime_join_tests.cpp)
    target_link_libraries(civis_bnw_civilopedia_native_runtime_join_tests
        PRIVATE civis_runtime)
    target_compile_features(civis_bnw_civilopedia_native_runtime_join_tests
        PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_civilopedia_native_runtime_join_tests
            PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_civilopedia_native_runtime_join_tests
            PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe"
       AND IS_DIRECTORY "${CIV5_ASSETS_ROOT}")
        add_test(NAME runtime.bnw_civilopedia_native_runtime_join
            COMMAND civis_bnw_civilopedia_native_runtime_join_tests
                "${CIV5_BNW_SELECTED_SAVE}"
                "${CIV5_BNW_INSTALL_ROOT}" "${CIV5_ASSETS_ROOT}")
        set_tests_properties(runtime.bnw_civilopedia_native_runtime_join
            PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Wave48: integrated source calculations and same-VM Lua read owner.
# Private SOURCE proposal. Root owns adoption after matching Wave41 gates.
# Actual live session layout changes require fresh coherent consumers.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE
        ../src/runtime/bnw_displayed_plot_yield.cpp
        ../src/runtime/bnw_area_registry.cpp
        ../src/runtime/bnw_city_current_food_rate.cpp
        ../src/runtime/bnw_city_current_food_difference.cpp
        ../src/runtime/bnw_city_specialist_counts.cpp)
endif()

if(BUILD_TESTING AND WIN32 AND TARGET civis_runtime)
    foreach(_bnw_numeric IN ITEMS
            bnw_area_registry
            bnw_city_current_food_rate_math
            bnw_native_unhappiness_math
            bnw_city_food_difference_math
            bnw_city_specialist_counts_math
            bnw_city_current_food_rate
            bnw_displayed_plot_yield
            bnw_normal_private_debug_owner
            bnw_city_current_food_difference
            bnw_city_specialist_counts_genuine)
        add_executable(civis_${_bnw_numeric}_tests
            ../tests/${_bnw_numeric}_tests.cpp)
        target_link_libraries(civis_${_bnw_numeric}_tests PRIVATE civis_runtime)
        target_compile_features(civis_${_bnw_numeric}_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(civis_${_bnw_numeric}_tests PRIVATE
                -Wall -Wextra -Wconversion -Werror -UNDEBUG)
        endif()
        if(MINGW)
            target_link_options(civis_${_bnw_numeric}_tests PRIVATE
                -static -Wl,--strip-debug -Wl,--stack,8388608)
        endif()
        if(_bnw_numeric MATCHES "^(bnw_city_current_food_rate|bnw_displayed_plot_yield|bnw_normal_private_debug_owner|bnw_city_current_food_difference|bnw_city_specialist_counts_genuine)$")
            if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
               AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
                add_test(NAME runtime.${_bnw_numeric}
                    COMMAND civis_${_bnw_numeric}_tests
                        "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
                set_tests_properties(runtime.${_bnw_numeric} PROPERTIES TIMEOUT 1200)
            endif()
        else()
            add_test(NAME runtime.${_bnw_numeric}
                COMMAND civis_${_bnw_numeric}_tests)
        endif()
    endforeach()
    # Existing executable: new implementation consumes the same reconciliation.
    if(TARGET civis_bnw_happiness_update_suffix_tests
       AND NOT TEST runtime.bnw_happiness_update_suffix
       AND EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_happiness_update_suffix
            COMMAND civis_bnw_happiness_update_suffix_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_happiness_update_suffix PROPERTIES TIMEOUT 1200)
    endif()
endif()

# SOURCE ONLY proposal: Root integrates after the coherent future overlay.
# The existing lua_script_host/map bootstrap/stock owner sources stay registered.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE
        ../src/runtime/bnw_lua_calculated_plot_yield.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    foreach(_bnw_lua_read IN ITEMS bnw_lua_calculated_plot_yield bnw_lua_active_team_revealed)
        add_executable(civis_${_bnw_lua_read}_tests
            ../tests/${_bnw_lua_read}_tests.cpp)
        target_link_libraries(civis_${_bnw_lua_read}_tests PRIVATE civis_runtime)
        target_compile_features(civis_${_bnw_lua_read}_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(civis_${_bnw_lua_read}_tests PRIVATE
                -Wall -Wextra -Wconversion -Werror -UNDEBUG)
        endif()
        if(MINGW)
            target_link_options(civis_${_bnw_lua_read}_tests PRIVATE
                -static -Wl,--strip-debug -Wl,--stack,8388608)
        endif()
        if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
            add_test(NAME runtime.${_bnw_lua_read}
                COMMAND civis_${_bnw_lua_read}_tests
                    "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
            set_tests_properties(runtime.${_bnw_lua_read} PROPERTIES TIMEOUT 1200)
        endif()
    endforeach()
endif()

# Wave50: retained city-entry component and pure growth-threshold arithmetic.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE
        ../src/runtime/bnw_city_entry.cpp
        ../src/runtime/bnw_city_growth_threshold_math.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    foreach(_bnw_city_leaf IN ITEMS bnw_city_entry bnw_city_growth_threshold_math)
        add_executable(civis_${_bnw_city_leaf}_tests
            ../tests/${_bnw_city_leaf}_tests.cpp)
        target_link_libraries(civis_${_bnw_city_leaf}_tests PRIVATE civis_runtime)
        target_compile_features(civis_${_bnw_city_leaf}_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(civis_${_bnw_city_leaf}_tests PRIVATE
                -Wall -Wextra -Wconversion -Werror -UNDEBUG)
        endif()
        if(MINGW)
            target_link_options(civis_${_bnw_city_leaf}_tests PRIVATE
                -static -Wl,--strip-debug -Wl,--stack,8388608)
        endif()
        if(_bnw_city_leaf STREQUAL "bnw_city_entry")
            if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
               AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
                add_test(NAME runtime.${_bnw_city_leaf}
                    COMMAND civis_${_bnw_city_leaf}_tests
                        "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
                set_tests_properties(runtime.${_bnw_city_leaf} PROPERTIES TIMEOUT 1200)
            endif()
        else()
            add_test(NAME runtime.${_bnw_city_leaf}
                COMMAND civis_${_bnw_city_leaf}_tests)
        endif()
    endforeach()
endif()

# Wave53: genuine current threshold query and pure growth/starvation suffix.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE
        ../src/runtime/bnw_city_current_growth_threshold.cpp
        ../src/runtime/bnw_city_growth_food_suffix_math.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    foreach(_bnw_growth_leaf IN ITEMS
            bnw_city_current_growth_threshold bnw_city_growth_food_suffix_math)
        add_executable(civis_${_bnw_growth_leaf}_tests
            ../tests/${_bnw_growth_leaf}_tests.cpp)
        target_link_libraries(civis_${_bnw_growth_leaf}_tests PRIVATE civis_runtime)
        target_compile_features(civis_${_bnw_growth_leaf}_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(civis_${_bnw_growth_leaf}_tests PRIVATE
                -Wall -Wextra -Wconversion -Werror -UNDEBUG)
        endif()
        if(MINGW)
            target_link_options(civis_${_bnw_growth_leaf}_tests PRIVATE
                -static -Wl,--strip-debug -Wl,--stack,8388608)
        endif()
        if(_bnw_growth_leaf STREQUAL "bnw_city_current_growth_threshold")
            if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
               AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
                add_test(NAME runtime.${_bnw_growth_leaf}
                    COMMAND civis_${_bnw_growth_leaf}_tests
                        "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
                set_tests_properties(runtime.${_bnw_growth_leaf} PROPERTIES TIMEOUT 1200)
            endif()
        else()
            add_test(NAME runtime.${_bnw_growth_leaf}
                COMMAND civis_${_bnw_growth_leaf}_tests)
        endif()
    endforeach()
endif()

# Wave67: require genuine AI cities separately from the turn0 human fixture.
if(BUILD_TESTING AND WIN32 AND TARGET civis_bnw_city_current_growth_threshold_tests
   AND EXISTS "${CIV5_BNW_AI_GROWTH_SAVE}"
   AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
    add_test(NAME runtime.bnw_city_current_growth_threshold_ai
        COMMAND civis_bnw_city_current_growth_threshold_tests --require-ai
            "${CIV5_BNW_AI_GROWTH_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
    set_tests_properties(runtime.bnw_city_current_growth_threshold_ai PROPERTIES TIMEOUT 1200)
endif()

# Wave55: genuine current city strategy start/end eligibility; no dispatch grant.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_city_current_strategy_eligibility.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_current_strategy_eligibility_tests
        ../tests/bnw_city_current_strategy_eligibility_tests.cpp)
    target_link_libraries(civis_bnw_city_current_strategy_eligibility_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_current_strategy_eligibility_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_current_strategy_eligibility_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_current_strategy_eligibility_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_current_strategy_eligibility
            COMMAND civis_bnw_city_current_strategy_eligibility_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_current_strategy_eligibility PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Wave56: five current native city strategy predicates; no dispatch grant.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_city_current_simple_strategy.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_current_simple_strategy_tests
        ../tests/bnw_city_current_simple_strategy_tests.cpp)
    target_link_libraries(civis_bnw_city_current_simple_strategy_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_current_simple_strategy_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_current_simple_strategy_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_current_simple_strategy_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_current_simple_strategy
            COMMAND civis_bnw_city_current_simple_strategy_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_current_simple_strategy PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Wave57: owned strategy checkpoint capture and private detached restore; no dispatch grant.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_city_strategy_checkpoint_owner.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_strategy_checkpoint_owner_tests
        ../tests/bnw_city_strategy_checkpoint_owner_tests.cpp)
    target_link_libraries(civis_bnw_city_strategy_checkpoint_owner_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_strategy_checkpoint_owner_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_strategy_checkpoint_owner_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_strategy_checkpoint_owner_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_strategy_checkpoint_owner
            COMMAND civis_bnw_city_strategy_checkpoint_owner_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_strategy_checkpoint_owner PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Wave58: five current geographic city strategy predicates; no dispatch grant.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_city_current_geographic_strategy.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_current_geographic_strategy_tests
        ../tests/bnw_city_current_geographic_strategy_tests.cpp)
    target_link_libraries(civis_bnw_city_current_geographic_strategy_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_current_geographic_strategy_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_current_geographic_strategy_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_current_geographic_strategy_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_current_geographic_strategy
            COMMAND civis_bnw_city_current_geographic_strategy_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_current_geographic_strategy PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Wave59: current Coast/Landlocked city strategy predicates; no dispatch grant.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_city_current_coastal_strategy.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_current_coastal_strategy_tests
        ../tests/bnw_city_current_coastal_strategy_tests.cpp)
    target_link_libraries(civis_bnw_city_current_coastal_strategy_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_current_coastal_strategy_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_current_coastal_strategy_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_current_coastal_strategy_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_current_coastal_strategy
            COMMAND civis_bnw_city_current_coastal_strategy_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_current_coastal_strategy PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Wave60: nine current city building and founder strategy predicates; no dispatch grant.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_city_current_building_strategy.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_current_building_strategy_tests
        ../tests/bnw_city_current_building_strategy_tests.cpp)
    target_link_libraries(civis_bnw_city_current_building_strategy_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_current_building_strategy_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_current_building_strategy_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_current_building_strategy_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_current_building_strategy
            COMMAND civis_bnw_city_current_building_strategy_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_current_building_strategy PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Wave61: full obsolete-end city strategy detached transition; no dispatch grant.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_city_strategy_obsolete_end.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_strategy_obsolete_end_tests
        ../tests/bnw_city_strategy_obsolete_end_tests.cpp)
    target_link_libraries(civis_bnw_city_strategy_obsolete_end_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_strategy_obsolete_end_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_strategy_obsolete_end_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_strategy_obsolete_end_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_strategy_obsolete_end
            COMMAND civis_bnw_city_strategy_obsolete_end_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_strategy_obsolete_end PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Wave63: Ordered registered city strategy prefix; incomplete mandatory frontiers stay blocked.
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_strategy_loop_tests ../tests/bnw_city_strategy_loop_tests.cpp)
    target_link_libraries(civis_bnw_city_strategy_loop_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_strategy_loop_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_strategy_loop_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_strategy_loop_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_strategy_loop COMMAND civis_bnw_city_strategy_loop_tests
            "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_strategy_loop PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Wave63: Current trade strategy queries; positive path/connection frontier stays blocked.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_city_current_trade_strategy.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_current_trade_strategy_tests ../tests/bnw_city_current_trade_strategy_tests.cpp)
    target_link_libraries(civis_bnw_city_current_trade_strategy_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_current_trade_strategy_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_current_trade_strategy_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_current_trade_strategy_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_current_trade_strategy COMMAND civis_bnw_city_current_trade_strategy_tests
            "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_current_trade_strategy PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Wave63: authoritative strategy/all-four production state and private end publication.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_city_strategy_runtime.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_strategy_runtime_tests ../tests/bnw_city_strategy_runtime_tests.cpp)
    target_link_libraries(civis_bnw_city_strategy_runtime_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_strategy_runtime_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_strategy_runtime_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_strategy_runtime_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_strategy_runtime COMMAND civis_bnw_city_strategy_runtime_tests
            "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_strategy_runtime PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Wave63: GP/Airlift/espionage source predicates and explicit remaining support frontiers.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_city_current_support_strategy.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_current_support_strategy_tests ../tests/bnw_city_current_support_strategy_tests.cpp)
    target_link_libraries(civis_bnw_city_current_support_strategy_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_current_support_strategy_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_current_support_strategy_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_current_support_strategy_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_current_support_strategy COMMAND civis_bnw_city_current_support_strategy_tests
            "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_current_support_strategy PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Wave64: genuine citizens blockade and naval predicates, explicit remaining frontier.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_city_current_naval_strategy.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_current_naval_strategy_tests ../tests/bnw_city_current_naval_strategy_tests.cpp)
    target_link_libraries(civis_bnw_city_current_naval_strategy_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_current_naval_strategy_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_current_naval_strategy_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_current_naval_strategy_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_current_naval_strategy COMMAND civis_bnw_city_current_naval_strategy_tests
            "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_current_naval_strategy PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Wave65: genuine current SCIENCE process0 read; no credit/cache freshness grant.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_city_current_science_rate.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_current_science_rate_tests ../tests/bnw_city_current_science_rate_tests.cpp)
    target_link_libraries(civis_bnw_city_current_science_rate_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_current_science_rate_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_current_science_rate_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_current_science_rate_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_current_science_rate COMMAND civis_bnw_city_current_science_rate_tests
            "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_current_science_rate PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Wave67: genuine current PRODUCTION and SCIENCE process child.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_city_current_production_rate.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_current_production_rate_tests
        ../tests/bnw_city_current_production_rate_tests.cpp)
    target_link_libraries(civis_bnw_city_current_production_rate_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_current_production_rate_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_current_production_rate_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_current_production_rate_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_current_production_rate
            COMMAND civis_bnw_city_current_production_rate_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_current_production_rate PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Wave74: SAME retained city-entry Citizens verify prefix; real frontiers remain.
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_citizens_verify_tests
        ../tests/bnw_city_citizens_verify_tests.cpp)
    target_link_libraries(civis_bnw_city_citizens_verify_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_citizens_verify_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_citizens_verify_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_citizens_verify_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_citizens_verify
            COMMAND civis_bnw_city_citizens_verify_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_citizens_verify PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Proposal ONLY, incremental current9fed composition under Root ownership.
# Frozen reviewed predecessor: Land65V2 new4+additive Naval hpp/cpp; Capital65V3 new4.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE
        ../src/runtime/bnw_city_current_land_improvement_strategy.cpp
        ../src/runtime/bnw_city_current_capital_strategy.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    foreach(_bnw_city_dispatch_leaf IN ITEMS bnw_city_current_land_improvement_strategy bnw_city_current_capital_strategy)
        add_executable(civis_${_bnw_city_dispatch_leaf}_tests ../tests/${_bnw_city_dispatch_leaf}_tests.cpp)
        target_link_libraries(civis_${_bnw_city_dispatch_leaf}_tests PRIVATE civis_runtime)
        target_compile_features(civis_${_bnw_city_dispatch_leaf}_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(civis_${_bnw_city_dispatch_leaf}_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
        endif()
        if(MINGW)
            target_link_options(civis_${_bnw_city_dispatch_leaf}_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
        endif()
        if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
            add_test(NAME runtime.${_bnw_city_dispatch_leaf} COMMAND civis_${_bnw_city_dispatch_leaf}_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
            set_tests_properties(runtime.${_bnw_city_dispatch_leaf} PROPERTIES TIMEOUT 1200)
        endif()
    endforeach()
endif()

# SOURCE ONLY append proposal. Root owns integration/CMake.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE
        ../src/runtime/bnw_game_active_team_read.cpp
        ../src/ui/bnw_client_selection_control.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_client_selection_dirty_tests
        ../tests/bnw_client_selection_dirty_tests.cpp)
    target_link_libraries(civis_bnw_client_selection_dirty_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_client_selection_dirty_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_client_selection_dirty_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_client_selection_dirty_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_client_selection_dirty
            COMMAND civis_bnw_client_selection_dirty_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_client_selection_dirty PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Cold saved-roster diagnostic; no gameplay acceptance or load hooks.
# Standalone observational tool, outside gameplay acceptance CTest cohort.
if(TARGET civis_runtime)
    add_executable(civis_bnw_save_roster_diagnostic
        bnw_save_roster_diagnostic.cpp)
    target_link_libraries(civis_bnw_save_roster_diagnostic PRIVATE civis_runtime)
    target_compile_features(civis_bnw_save_roster_diagnostic PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(civis_bnw_save_roster_diagnostic PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_save_roster_diagnostic PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
endif()

# SOURCE ONLY incremental proposal after selection/dirty V2. Root owns CMake.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_citizen_plot_value_math.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime)
    add_executable(civis_bnw_citizen_plot_value_math_tests
        ../tests/bnw_citizen_plot_value_math_tests.cpp)
    target_link_libraries(civis_bnw_citizen_plot_value_math_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_citizen_plot_value_math_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_citizen_plot_value_math_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_citizen_plot_value_math_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    add_test(NAME runtime.bnw_citizen_plot_value_math
        COMMAND civis_bnw_citizen_plot_value_math_tests)
endif()
# Existing Entry/SpecialistCounts/Consumption/Growth/Citizens targets are not
# registered twice. Their source objects and inline/allocating consumers rebuild.

# SOURCE ONLY incremental proposal after Wave76 specialist caller. Root owns CMake.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_citizen_specialist_value_math.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime)
    add_executable(civis_bnw_citizen_specialist_value_math_tests
        ../tests/bnw_citizen_specialist_value_math_tests.cpp)
    target_link_libraries(civis_bnw_citizen_specialist_value_math_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_citizen_specialist_value_math_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_citizen_specialist_value_math_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_citizen_specialist_value_math_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    add_test(NAME runtime.bnw_citizen_specialist_value_math
        COMMAND civis_bnw_citizen_specialist_value_math_tests)
endif()
# Existing Entry/SpecialistCounts/Consumption/Growth/Citizens targets are not
# registered twice. Their source objects and inline/allocating consumers rebuild.

# Wave80: actual retained plot UI/gameplay notification tail.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/ui/bnw_plot_yield_refresh_ui_adapter.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_plot_yield_refresh_ui_adapter_tests
        ../tests/bnw_plot_yield_refresh_ui_adapter_tests.cpp)
    target_link_libraries(civis_bnw_plot_yield_refresh_ui_adapter_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_plot_yield_refresh_ui_adapter_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_plot_yield_refresh_ui_adapter_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_plot_yield_refresh_ui_adapter_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_plot_yield_refresh_ui_adapter
            COMMAND civis_bnw_plot_yield_refresh_ui_adapter_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_plot_yield_refresh_ui_adapter PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Wave84 composition: genuine native37 retained city child (SOURCE proposal).
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_city_yield_refresh.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_yield_refresh_tests ../tests/bnw_city_yield_refresh_tests.cpp)
    target_link_libraries(civis_bnw_city_yield_refresh_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_yield_refresh_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(civis_bnw_city_yield_refresh_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_yield_refresh_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_yield_refresh COMMAND civis_bnw_city_yield_refresh_tests
            "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_yield_refresh PROPERTIES TIMEOUT 1200)
    endif()
endif()
# SOURCE proposal only: owned City UpdateReligion prefix and supplied pure math.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_city_religion_yield_owner.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_religion_yield_owner_tests
        ../tests/bnw_city_religion_yield_owner_tests.cpp)
    target_link_libraries(civis_bnw_city_religion_yield_owner_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_religion_yield_owner_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(civis_bnw_city_religion_yield_owner_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_religion_yield_owner_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    add_test(NAME runtime.bnw_city_religion_yield_owner
        COMMAND civis_bnw_city_religion_yield_owner_tests)
endif()

# SOURCE-only NEW metadata owner. Root owns adoption and coherent registration.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_creation_config.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_creation_config_tests ../tests/bnw_creation_config_tests.cpp)
    target_link_libraries(civis_bnw_creation_config_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_creation_config_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_creation_config_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_creation_config_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_creation_config COMMAND civis_bnw_creation_config_tests "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_creation_config PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Wave87 SOURCE proposal only: current private Player happiness prefix.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_player_happiness_prefix_owner.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_player_happiness_prefix_owner_tests ../tests/bnw_player_happiness_prefix_owner_tests.cpp)
    target_link_libraries(civis_bnw_player_happiness_prefix_owner_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_player_happiness_prefix_owner_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_player_happiness_prefix_owner_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_player_happiness_prefix_owner_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_player_happiness_prefix_owner COMMAND civis_bnw_player_happiness_prefix_owner_tests
            "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_player_happiness_prefix_owner PROPERTIES TIMEOUT 1200)
    endif()
endif()
# Existing Entry object contains the native focus/reallocation prefix; no duplicate source.
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_citizens_focus_reallocation_tests ../tests/bnw_citizens_focus_reallocation_tests.cpp)
    target_link_libraries(civis_bnw_citizens_focus_reallocation_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_citizens_focus_reallocation_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_citizens_focus_reallocation_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_citizens_focus_reallocation_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    add_test(NAME runtime.bnw_citizens_focus_reallocation COMMAND civis_bnw_citizens_focus_reallocation_tests)
endif()


# Authored per-context control graph; actual installed XML/InstanceManager fixture.
if(TARGET civis_runtime AND BUILD_TESTING)
    add_executable(civis_script_ui_control_graph_tests ../tests/script_ui_control_graph_tests.cpp)
    target_link_libraries(civis_script_ui_control_graph_tests PRIVATE civis_runtime)
    target_compile_features(civis_script_ui_control_graph_tests PRIVATE cxx_std_20)
    if(MINGW)
        target_link_options(civis_script_ui_control_graph_tests PRIVATE -static)
    endif()
    if(IS_DIRECTORY "${CIV5_ASSETS_ROOT}")
        add_test(NAME runtime.script_ui_control_graph
            COMMAND civis_script_ui_control_graph_tests "${CIV5_ASSETS_ROOT}")
    endif()
endif()

# SOURCE-only selected map script initialization query; no fresh world grant.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_selected_map_init.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_selected_map_init_tests ../tests/bnw_selected_map_init_tests.cpp)
    target_link_libraries(civis_bnw_selected_map_init_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_selected_map_init_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_selected_map_init_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_selected_map_init_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_selected_map_init COMMAND civis_bnw_selected_map_init_tests "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_selected_map_init PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Root SOURCE proposal: retained production/project/CityScreen; stock banner return still mandatory.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE
        ../src/runtime/bnw_city_production_turn.cpp
        ../src/runtime/bnw_city_current_project_production_difference.cpp
        ../src/ui/bnw_city_production_ui_adapter.cpp
)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_production_turn_tests ../tests/bnw_city_production_turn_tests.cpp)
    target_link_libraries(civis_bnw_city_production_turn_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_production_turn_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_production_turn_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_production_turn_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_production_turn COMMAND civis_bnw_city_production_turn_tests
            "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_production_turn PROPERTIES TIMEOUT 1200)
    endif()
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_current_project_production_difference_tests ../tests/bnw_city_current_project_production_difference_tests.cpp)
    target_link_libraries(civis_bnw_city_current_project_production_difference_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_current_project_production_difference_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_current_project_production_difference_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_current_project_production_difference_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_current_project_production_difference COMMAND civis_bnw_city_current_project_production_difference_tests
            "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_current_project_production_difference PROPERTIES TIMEOUT 1200)
    endif()
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_production_ui_adapter_tests ../tests/bnw_city_production_ui_adapter_tests.cpp)
    target_link_libraries(civis_bnw_city_production_ui_adapter_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_production_ui_adapter_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_production_ui_adapter_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_production_ui_adapter_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_production_ui_adapter COMMAND civis_bnw_city_production_ui_adapter_tests
            "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_production_ui_adapter PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Explicit selected input supplied by the host; no machine-specific runtime default.
set(CIV5_BNW_CONFIG_INI "" CACHE FILEPATH "Selected Civilization V config.ini for BNW creation acceptance")

# Root SOURCE proposal: private fresh Game constructor/reset prefix only; Settler Init remains mandatory.
if(TARGET civis_runtime AND WIN32)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_fresh_game.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND TARGET civis_sqlite AND WIN32)
    add_executable(civis_bnw_fresh_game_tests ../tests/bnw_fresh_game_tests.cpp)
    target_link_libraries(civis_bnw_fresh_game_tests PRIVATE civis_runtime civis_sqlite ole32)
    target_include_directories(civis_bnw_fresh_game_tests PRIVATE ../third_party/sqlite)
    target_compile_features(civis_bnw_fresh_game_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_fresh_game_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_fresh_game_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe" AND EXISTS "${CIV5_BNW_CONFIG_INI}")
        add_test(NAME runtime.bnw_fresh_game COMMAND civis_bnw_fresh_game_tests
            "${CIV5_BNW_INSTALL_ROOT}" "${CIV5_BNW_CONFIG_INI}")
        set_tests_properties(runtime.bnw_fresh_game PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Root future SOURCE proposal: locale, Pregame, tactical and advisor storage.
# Registrations do not authorize continuation or bypass actual Game initialization.
if(TARGET civis_runtime AND WIN32)
    target_sources(civis_runtime PRIVATE
        ../src/runtime/bnw_selected_locale_owner.cpp
        ../src/runtime/bnw_fresh_pregame_speed.cpp
        ../src/runtime/bnw_fresh_tactical_map.cpp
        ../src/runtime/bnw_fresh_advisors.cpp
        ../src/runtime/bnw_fresh_city_manager.cpp
    )
endif()

if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_selected_locale_owner_tests ../tests/bnw_selected_locale_owner_tests.cpp)
    target_link_libraries(civis_bnw_selected_locale_owner_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_selected_locale_owner_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_selected_locale_owner_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_selected_locale_owner_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe" AND EXISTS "${CIV5_BNW_CONFIG_INI}")
        add_test(NAME runtime.bnw_selected_locale_owner COMMAND civis_bnw_selected_locale_owner_tests "${CIV5_BNW_INSTALL_ROOT}" "${CIV5_BNW_CONFIG_INI}" "${CMAKE_BINARY_DIR}/bnw_selected_locale_owner-fixtures")
        set_tests_properties(runtime.bnw_selected_locale_owner PROPERTIES TIMEOUT 1200)
    endif()
endif()

if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_fresh_pregame_speed_tests ../tests/bnw_fresh_pregame_speed_tests.cpp)
    target_link_libraries(civis_bnw_fresh_pregame_speed_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_fresh_pregame_speed_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_fresh_pregame_speed_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_fresh_pregame_speed_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe" AND EXISTS "${CIV5_BNW_CONFIG_INI}")
        add_test(NAME runtime.bnw_fresh_pregame_speed COMMAND civis_bnw_fresh_pregame_speed_tests "${CIV5_BNW_INSTALL_ROOT}" "${CIV5_BNW_CONFIG_INI}" "${CMAKE_BINARY_DIR}/bnw_fresh_pregame_speed-fixtures")
        set_tests_properties(runtime.bnw_fresh_pregame_speed PROPERTIES TIMEOUT 1200)
    endif()
endif()

if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_fresh_tactical_map_tests ../tests/bnw_fresh_tactical_map_tests.cpp)
    target_link_libraries(civis_bnw_fresh_tactical_map_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_fresh_tactical_map_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_fresh_tactical_map_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_fresh_tactical_map_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_fresh_tactical_map COMMAND civis_bnw_fresh_tactical_map_tests "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_fresh_tactical_map PROPERTIES TIMEOUT 1200)
    endif()
endif()

if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_fresh_advisors_tests ../tests/bnw_fresh_advisors_tests.cpp)
    target_link_libraries(civis_bnw_fresh_advisors_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_fresh_advisors_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_fresh_advisors_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_fresh_advisors_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(TRUE)
        add_test(NAME runtime.bnw_fresh_advisors COMMAND civis_bnw_fresh_advisors_tests)
        set_tests_properties(runtime.bnw_fresh_advisors PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Root CityManager registration; no owned-city/global realm readiness implied.
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_fresh_city_manager_tests ../tests/bnw_fresh_city_manager_tests.cpp)
    target_link_libraries(civis_bnw_fresh_city_manager_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_fresh_city_manager_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_fresh_city_manager_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_fresh_city_manager_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    add_test(NAME runtime.bnw_fresh_city_manager COMMAND civis_bnw_fresh_city_manager_tests)
    set_tests_properties(runtime.bnw_fresh_city_manager PROPERTIES TIMEOUT 1200)
endif()

# Root-owned descriptor only; this file is never configured by the packet.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_unit_move_pending.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_unit_move_pending_tests ../tests/bnw_unit_move_pending_tests.cpp)
    target_link_libraries(civis_bnw_unit_move_pending_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_unit_move_pending_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_unit_move_pending_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_unit_move_pending_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_unit_move_pending COMMAND civis_bnw_unit_move_pending_tests "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_unit_move_pending PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Root-only ordinary movement-cost registration; query is not a full move grant.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_plot_movement_cost.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_plot_movement_cost_tests ../tests/bnw_plot_movement_cost_tests.cpp)
    target_link_libraries(civis_bnw_plot_movement_cost_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_plot_movement_cost_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_plot_movement_cost_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_plot_movement_cost_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_plot_movement_cost COMMAND civis_bnw_plot_movement_cost_tests "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_plot_movement_cost PROPERTIES TIMEOUT 1200)
    endif()
endif()

# OWN graphics linkage/context target retained by per-file runtime join.
if(WIN32)
    target_link_libraries(civis_runtime PRIVATE $<LINK_ONLY:civ5_bnw_fog_map_metadata>)
endif()
if(TARGET civis_bnw_native_archive_v6_entry_admission_tests)
    target_link_libraries(civis_bnw_native_archive_v6_entry_admission_tests PRIVATE civis_runtime)
endif()
# Add only after applying all pinned V4 + context overlays together.
# Existing launch/host/frame/font files remain existing target sources.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE
        ../src/ui/bnw_owned_camera_ui_source_input.cpp
        ../src/ui/bnw_plot_yield_refresh_owned_ui_context.cpp)
endif()
if(TARGET civis_runtime AND BUILD_TESTING AND WIN32)
    add_executable(civis_bnw_owned_camera_ui_context_actual_tests
        ../tests/bnw_owned_camera_ui_context_actual_tests.cpp)
    target_link_libraries(civis_bnw_owned_camera_ui_context_actual_tests PRIVATE civis_runtime civ5_gl)
    target_compile_features(civis_bnw_owned_camera_ui_context_actual_tests PRIVATE cxx_std_20)
    if(MINGW)
        target_link_options(civis_bnw_owned_camera_ui_context_actual_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe" AND IS_DIRECTORY "${CIV5_ASSETS_ROOT}")
        add_test(NAME runtime.bnw_owned_camera_ui_context_actual
            COMMAND civis_bnw_owned_camera_ui_context_actual_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}" "${CIV5_ASSETS_ROOT}")
        set_tests_properties(runtime.bnw_owned_camera_ui_context_actual PROPERTIES TIMEOUT 1800)
    endif()
endif()




# Root proposal: actual city food growth and detached game-deals prefixes.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE
        ../src/runtime/bnw_city_food_growth_prefix.cpp
        ../src/runtime/bnw_game_deals_turn_prefix.cpp)
    if(MSVC)
        set_property(SOURCE ../src/runtime/bnw_city_food_growth_prefix.cpp ../src/runtime/bnw_game_deals_turn_prefix.cpp APPEND PROPERTY COMPILE_OPTIONS /fp:strict)
    else()
        set_property(SOURCE ../src/runtime/bnw_city_food_growth_prefix.cpp ../src/runtime/bnw_game_deals_turn_prefix.cpp APPEND PROPERTY COMPILE_OPTIONS -fno-fast-math -ffp-contract=off)
    endif()
endif()

if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_food_growth_prefix_tests ../tests/bnw_city_food_growth_prefix_tests.cpp)
    target_link_libraries(civis_bnw_city_food_growth_prefix_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_food_growth_prefix_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_food_growth_prefix_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_food_growth_prefix_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_food_growth_prefix COMMAND civis_bnw_city_food_growth_prefix_tests "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_food_growth_prefix PROPERTIES TIMEOUT 1200)
    endif()
endif()

if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_game_deals_turn_prefix_tests ../tests/bnw_game_deals_turn_prefix_tests.cpp)
    target_link_libraries(civis_bnw_game_deals_turn_prefix_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_game_deals_turn_prefix_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_game_deals_turn_prefix_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_game_deals_turn_prefix_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_game_deals_turn_prefix COMMAND civis_bnw_game_deals_turn_prefix_tests "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_game_deals_turn_prefix PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Root proposal: actual food-owned population / single-plot continuation.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_city_population.cpp)
    if(MSVC)
        set_property(SOURCE ../src/runtime/bnw_city_population.cpp APPEND PROPERTY COMPILE_OPTIONS /fp:strict)
    else()
        set_property(SOURCE ../src/runtime/bnw_city_population.cpp APPEND PROPERTY COMPILE_OPTIONS -fno-fast-math -ffp-contract=off)
    endif()
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_city_population_tests ../tests/bnw_city_population_tests.cpp)
    target_link_libraries(civis_bnw_city_population_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_city_population_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_city_population_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_city_population_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_city_population COMMAND civis_bnw_city_population_tests "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_city_population PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Wave171: actual same-session SavedStorage ID6 provider/snapshot test.
# Root-only prospective registration; no admission or full-world export grant.
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_selected_save_data_tests
        ../tests/bnw_selected_save_data_tests.cpp)
    target_link_libraries(civis_bnw_selected_save_data_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_selected_save_data_tests PRIVATE cxx_std_20)
    target_compile_options(civis_bnw_selected_save_data_tests PRIVATE
        -Wall -Wextra -Wpedantic -Werror)
    if(MINGW)
        target_link_options(civis_bnw_selected_save_data_tests PRIVATE
            -static -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}"
       AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe"
       AND IS_DIRECTORY "${CIV5_ASSETS_ROOT}")
        add_test(NAME runtime.bnw_selected_save_data
            COMMAND civis_bnw_selected_save_data_tests
                "${CIV5_BNW_SELECTED_SAVE}"
                "${CIV5_BNW_INSTALL_ROOT}" "${CIV5_ASSETS_ROOT}")
        set_tests_properties(runtime.bnw_selected_save_data PROPERTIES TIMEOUT 1200)
    endif()
endif()

# Retain both genuine producer TUs from the reviewed startup composition.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE
        ../src/ui/bnw_native_owned_skin_consumer.cpp
        ../src/runtime/bnw_selected_language_input_owner.cpp)
endif()
# BNW minor-city strength: genuine selected-preview scratch effect.
# Register the component independently of the full technology-grant frontier.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_minor_city_strength.cpp)
    if(MSVC)
        set_property(SOURCE ../src/runtime/bnw_minor_city_strength.cpp APPEND PROPERTY COMPILE_OPTIONS /fp:strict)
    else()
        set_property(SOURCE ../src/runtime/bnw_minor_city_strength.cpp APPEND PROPERTY COMPILE_OPTIONS -fno-fast-math -ffp-contract=off)
    endif()
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_minor_city_strength_tests ../tests/bnw_minor_city_strength_tests.cpp)
    target_link_libraries(civis_bnw_minor_city_strength_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_minor_city_strength_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_minor_city_strength_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
    elseif(MSVC)
        target_compile_options(civis_bnw_minor_city_strength_tests PRIVATE /fp:strict /UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_minor_city_strength_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_minor_city_strength COMMAND civis_bnw_minor_city_strength_tests "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_minor_city_strength PROPERTIES TIMEOUT 1200 RUN_SERIAL TRUE)
    endif()
endif()

# Root NEXT: genuine BNW commands, partial persistence and state comparison.
# No complete-turn, full-save or original-BNW parity grant.
if(TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE
        ../src/runtime/bnw_research_choice_commit.cpp
        ../src/ui/bnw_alpha_progression_backend.cpp
        ../src/runtime/bnw_research_queue_save_owner.cpp
        ../src/runtime/bnw_major_player_traits_owner.cpp
        ../src/runtime/normalised_bnw_state.cpp
        ../src/ui/selected_city_production_services.cpp
    )
    if(MSVC)
        set_property(SOURCE ../src/runtime/bnw_research_choice_commit.cpp ../src/ui/bnw_alpha_progression_backend.cpp ../src/runtime/bnw_research_queue_save_owner.cpp ../src/runtime/bnw_major_player_traits_owner.cpp ../src/runtime/normalised_bnw_state.cpp ../src/ui/selected_city_production_services.cpp APPEND PROPERTY COMPILE_OPTIONS /fp:strict)
    else()
        set_property(SOURCE ../src/runtime/bnw_research_choice_commit.cpp ../src/ui/bnw_alpha_progression_backend.cpp ../src/runtime/bnw_research_queue_save_owner.cpp ../src/runtime/bnw_major_player_traits_owner.cpp ../src/runtime/normalised_bnw_state.cpp ../src/ui/selected_city_production_services.cpp APPEND PROPERTY COMPILE_OPTIONS -fno-fast-math -ffp-contract=off)
    endif()
endif()

if(TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_state_dump normalised_bnw_state_main.cpp)
    target_link_libraries(civis_bnw_state_dump PRIVATE civis_runtime)
    target_compile_features(civis_bnw_state_dump PRIVATE cxx_std_20)
endif()

if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_research_choice_commit_tests ../tests/bnw_research_choice_commit_tests.cpp)
    target_link_libraries(civis_bnw_research_choice_commit_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_research_choice_commit_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_research_choice_commit_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
    elseif(MSVC)
        target_compile_options(civis_bnw_research_choice_commit_tests PRIVATE /fp:strict /UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_research_choice_commit_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    # Standalone diagnostic returns UNAVAILABLE; it is not a passed CTest gate.
    # The actual renderer fixture must invoke the exported positive checks.
    add_executable(civis_bnw_alpha_progression_backend_tests ../tests/bnw_alpha_progression_backend_tests.cpp)
    target_link_libraries(civis_bnw_alpha_progression_backend_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_alpha_progression_backend_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_alpha_progression_backend_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
    elseif(MSVC)
        target_compile_options(civis_bnw_alpha_progression_backend_tests PRIVATE /fp:strict /UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_alpha_progression_backend_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    # Standalone diagnostic returns UNAVAILABLE; it is not a passed CTest gate.
    # The actual renderer fixture must invoke the exported positive checks.
    add_executable(civis_bnw_research_queue_save_owner_tests ../tests/bnw_research_queue_save_owner_tests.cpp)
    target_link_libraries(civis_bnw_research_queue_save_owner_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_research_queue_save_owner_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_research_queue_save_owner_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
    elseif(MSVC)
        target_compile_options(civis_bnw_research_queue_save_owner_tests PRIVATE /fp:strict /UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_research_queue_save_owner_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.research_queue_save_owner COMMAND civis_bnw_research_queue_save_owner_tests "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.research_queue_save_owner PROPERTIES TIMEOUT 1200 RUN_SERIAL TRUE)
    endif()
    add_executable(civis_bnw_major_player_traits_owner_tests ../tests/bnw_major_player_traits_owner_tests.cpp)
    target_link_libraries(civis_bnw_major_player_traits_owner_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_major_player_traits_owner_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_major_player_traits_owner_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
    elseif(MSVC)
        target_compile_options(civis_bnw_major_player_traits_owner_tests PRIVATE /fp:strict /UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_major_player_traits_owner_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.major_player_traits_owner COMMAND civis_bnw_major_player_traits_owner_tests "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.major_player_traits_owner PROPERTIES TIMEOUT 1200 RUN_SERIAL TRUE)
    endif()
    add_executable(civis_normalised_bnw_state_tests ../tests/normalised_bnw_state_tests.cpp)
    target_link_libraries(civis_normalised_bnw_state_tests PRIVATE civis_runtime)
    target_compile_features(civis_normalised_bnw_state_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_normalised_bnw_state_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
    elseif(MSVC)
        target_compile_options(civis_normalised_bnw_state_tests PRIVATE /fp:strict /UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_normalised_bnw_state_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.normalised_bnw_state COMMAND civis_normalised_bnw_state_tests "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.normalised_bnw_state PROPERTIES TIMEOUT 1200 RUN_SERIAL TRUE)
    endif()
endif()

# BNW unit constructor/ID prefix: genuine selected-save native trajectory.
if(TARGET civis_runtime AND WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_native_unit_allocator_core.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_executable(civis_bnw_native_unit_allocator_core_tests ../tests/bnw_native_unit_allocator_core_tests.cpp)
    target_link_libraries(civis_bnw_native_unit_allocator_core_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_native_unit_allocator_core_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_native_unit_allocator_core_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
    elseif(MSVC)
        target_compile_options(civis_bnw_native_unit_allocator_core_tests PRIVATE /fp:strict /UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_native_unit_allocator_core_tests PRIVATE -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.bnw_native_unit_allocator_core COMMAND civis_bnw_native_unit_allocator_core_tests "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.bnw_native_unit_allocator_core PROPERTIES TIMEOUT 1200 RUN_SERIAL TRUE)
        add_test(NAME runtime.major_team_has_tech_prefix COMMAND civis_bnw_barbarian_team_has_tech_tests "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.major_team_has_tech_prefix PROPERTIES TIMEOUT 1200 RUN_SERIAL TRUE)
    endif()
endif()

# Root: pending-unit and technology-consumer coherent integration.
if(WIN32)
    target_sources(civis_runtime PRIVATE
        ../src/runtime/bnw_selected_user_options_owner.cpp
        ../src/runtime/bnw_major_tech_transport_consumer.cpp
        ../src/ui/bnw_tech_award_context_owner.cpp
        ../src/ui/bnw_tech_award_plain_label_layout.cpp
        ../src/ui/bnw_tech_award_context_layout.cpp
        ../src/graphics/bnw_selected_pending_unit_visual_loan.cpp
        ../src/graphics/bnw_pending_unit_visual_owner.cpp
        ../src/graphics/bnw_train_presented_frame_proof.cpp
    )
endif()

# Persist the explicitly selected standalone services profile; this is not a
# full-world save or native turn acceptance test.
if(BUILD_TESTING AND TARGET civis_runtime)
    add_executable(civis_standalone_services_tests ../tests/standalone_services_tests.cpp)
    target_link_libraries(civis_standalone_services_tests PRIVATE civis_runtime)
    target_compile_features(civis_standalone_services_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_standalone_services_tests PRIVATE -Wall -Wextra -Wconversion -Werror -UNDEBUG)
    endif()
    if(MINGW)
        target_link_options(civis_standalone_services_tests PRIVATE -static)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe")
        add_test(NAME runtime.standalone_services_checkpoint COMMAND civis_standalone_services_tests
            "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}")
        set_tests_properties(runtime.standalone_services_checkpoint PROPERTIES RUN_SERIAL TRUE TIMEOUT 600)
    endif()
endif()

# Root finite logic integration: actual owners, not readiness substitutes.
if(TARGET civis_runtime AND WIN32)
    target_sources(civis_runtime PRIVATE
        ../src/runtime/bnw_live_game_context.cpp
        ../src/runtime/bnw_net_message_debug_config_owner.cpp
        ../src/runtime/bnw_fresh_civilization_info_collection.cpp
        ../src/ui/bnw_native_installation_input_owner.cpp
        ../src/ui/bnw_audio_live_knob_owner.cpp
        ../src/ui/bnw_audio_native_filename_probes.cpp
        ../src/ui/bnw_audio_native_settings_owner.cpp
        ../src/ui/bnw_audio_source_catalog.cpp
        ../src/ui/bnw_audio_user_settings_input.cpp
        ../src/ui/bnw_new_era_native_xml_owner.cpp)
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    foreach(_bnw_finite_owner bnw_fresh_civilization_info_collection bnw_native_installation_input_owner bnw_fresh_game_user_settings_actual)
        add_executable(civis_${_bnw_finite_owner}_tests ../tests/${_bnw_finite_owner}_tests.cpp)
        target_link_libraries(civis_${_bnw_finite_owner}_tests PRIVATE civis_runtime)
        target_compile_features(civis_${_bnw_finite_owner}_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(civis_${_bnw_finite_owner}_tests PRIVATE
                -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
        endif()
        if(MINGW)
            target_link_options(civis_${_bnw_finite_owner}_tests PRIVATE
                -static -Wl,--strip-debug -Wl,--stack,8388608)
        endif()
    endforeach()
    if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe" AND IS_DIRECTORY "${CIV5_ASSETS_ROOT}")
        add_test(NAME ui.bnw_native_installation_input_owner
            COMMAND civis_bnw_native_installation_input_owner_tests
                "${CIV5_BNW_INSTALL_ROOT}" "${CIV5_ASSETS_ROOT}")
        set_tests_properties(ui.bnw_native_installation_input_owner PROPERTIES RUN_SERIAL TRUE TIMEOUT 1200)
        if(TARGET civis_bnw_native_installation_parent_prefix_tests)
            add_test(NAME ui.bnw_native_installation_parent_prefix
                COMMAND civis_bnw_native_installation_parent_prefix_tests
                    "${CIV5_BNW_INSTALL_ROOT}" "${CIV5_ASSETS_ROOT}")
            set_tests_properties(ui.bnw_native_installation_parent_prefix PROPERTIES RUN_SERIAL TRUE TIMEOUT 1200)
        endif()
        if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_CONFIG_INI}")
            add_test(NAME runtime.bnw_fresh_civilization_info_collection
                COMMAND civis_bnw_fresh_civilization_info_collection_tests
                    "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}"
                    "${CIV5_ASSETS_ROOT}" "${CIV5_BNW_CONFIG_INI}")
            set_tests_properties(runtime.bnw_fresh_civilization_info_collection PROPERTIES RUN_SERIAL TRUE TIMEOUT 1200)
            add_test(NAME runtime.bnw_fresh_game_user_settings_actual
                COMMAND civis_bnw_fresh_game_user_settings_actual_tests
                    "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}"
                    "${CIV5_ASSETS_ROOT}" "${CIV5_BNW_CONFIG_INI}")
            set_tests_properties(runtime.bnw_fresh_game_user_settings_actual PROPERTIES RUN_SERIAL TRUE TIMEOUT 1200)
        endif()
    endif()
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_net_debug_config_actual_boot_tests
        ../tests/runtime_bnw_net_debug_config_actual_boot_tests.cpp)
    target_link_libraries(civis_bnw_net_debug_config_actual_boot_tests PRIVATE civis_runtime)
    target_compile_features(civis_bnw_net_debug_config_actual_boot_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_net_debug_config_actual_boot_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_net_debug_config_actual_boot_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe" AND IS_DIRECTORY "${CIV5_ASSETS_ROOT}" AND EXISTS "${CIV5_BNW_CONFIG_INI}")
        add_test(NAME runtime.bnw_net_debug_config_actual_boot
            COMMAND civis_bnw_net_debug_config_actual_boot_tests
                "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}" "${CIV5_ASSETS_ROOT}"
                --civis-settings "${CIV5_BNW_CONFIG_INI}")
        set_tests_properties(runtime.bnw_net_debug_config_actual_boot PROPERTIES RUN_SERIAL TRUE TIMEOUT 1200)
    endif()
endif()
if(BUILD_TESTING AND TARGET civis_runtime AND WIN32)
    add_executable(civis_bnw_audio_source_actual_tests
        ../tests/bnw_audio_source_actual_tests.cpp
        ../tests/bnw_audio_native_rng_numeric_tests.cpp)
    target_link_libraries(civis_bnw_audio_source_actual_tests PRIVATE civis_runtime civ5_gl)
    target_compile_features(civis_bnw_audio_source_actual_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(civis_bnw_audio_source_actual_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
    endif()
    if(MINGW)
        target_link_options(civis_bnw_audio_source_actual_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe" AND IS_DIRECTORY "${CIV5_ASSETS_ROOT}")
        add_test(NAME runtime.bnw_audio_source_actual COMMAND civis_bnw_audio_source_actual_tests
            "${CIV5_BNW_SELECTED_SAVE}" "${CIV5_BNW_INSTALL_ROOT}" "${CIV5_ASSETS_ROOT}")
        set_tests_properties(runtime.bnw_audio_source_actual PROPERTIES RUN_SERIAL TRUE TIMEOUT 3600)
    endif()
endif()

# Root: actual Game/setup prefix. These tests do not grant full new-game readiness.
if(WIN32 AND TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE
        ../src/runtime/bnw_new_game_setup_rules.cpp
        ../src/runtime/bnw_new_game_player_setup.cpp)
endif()
if(BUILD_TESTING AND WIN32 AND TARGET civis_runtime)
    foreach(_bnw_setup_stage setup_rules setup player_setup first_player)
        set(_bnw_setup_test "bnw_new_game_${_bnw_setup_stage}_actual")
        add_executable(civis_${_bnw_setup_test}_tests ../tests/${_bnw_setup_test}_tests.cpp)
        target_link_libraries(civis_${_bnw_setup_test}_tests PRIVATE civis_runtime)
        target_compile_features(civis_${_bnw_setup_test}_tests PRIVATE cxx_std_20)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(civis_${_bnw_setup_test}_tests PRIVATE
                -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
        endif()
        if(MINGW)
            target_link_options(civis_${_bnw_setup_test}_tests PRIVATE
                -static -Wl,--strip-debug -Wl,--stack,8388608)
        endif()
        if(EXISTS "${CIV5_BNW_INSTALL_ROOT}/Assets/DLC/Expansion2/Expansion2.Civ5Pkg")
            add_test(NAME runtime.${_bnw_setup_test}
                COMMAND civis_${_bnw_setup_test}_tests "${CIV5_BNW_INSTALL_ROOT}")
            set_tests_properties(runtime.${_bnw_setup_test} PROPERTIES
                TIMEOUT 1200 RUN_SERIAL TRUE)
        endif()
    endforeach()
endif()

# Root: complete selected Civis-bank native/UI/audio alpha source cohort.
if(WIN32 AND TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE
        ../src/assets/civis_resource_bank_owner.cpp
        ../src/runtime/bnw_civis_owned_stock_source_bundle.cpp
        ../src/runtime/bnw_selected_unitflag_context_owner.cpp
        ../src/runtime/bnw_selected_worldview_context_owner.cpp
        ../src/runtime/bnw_worldview_native_context_issuer.cpp
        ../src/runtime/bnw_worldview_script_ui_adapter.cpp
        ../src/runtime/bnw_worldview_source_operation_loan.cpp
        ../src/ui/bnw_worldview_control_effects.cpp
        ../src/ui/detail/bnw_normal_nonfont_button_producer.cpp
        ../src/ui/detail/bnw_normal_nonfont_native_lifetime.cpp
        ../src/ui/civis_bank_audio_source_owner.cpp
        ../src/ui/civis_input_policy_owner.cpp
        ../src/ui/civis_resource_bank_font_color_snapshot.cpp
        ../src/ui/civis_resource_bank_ui_snapshot.cpp
)
endif()
if(BUILD_TESTING AND WIN32 AND TARGET civis_runtime AND TARGET civ5_gl)
    add_executable(civis_bank_audio_actual_tests ../tests/civis_bank_audio_actual_tests.cpp)
    target_link_libraries(civis_bank_audio_actual_tests PRIVATE civis_runtime civ5_gl)
    target_compile_features(civis_bank_audio_actual_tests PRIVATE cxx_std_20)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(civis_bank_audio_actual_tests PRIVATE
            -Wall -Wextra -Wconversion -Werror -UNDEBUG -fno-fast-math -ffp-contract=off)
    endif()
    if(MINGW)
        target_link_options(civis_bank_audio_actual_tests PRIVATE
            -static -Wl,--strip-debug -Wl,--stack,8388608)
    endif()
    if(EXISTS "${CIV5_BNW_SELECTED_SAVE}" AND EXISTS "${CIV5_BNW_INSTALL_ROOT}/CivilizationV.exe" AND IS_DIRECTORY "${CIV5_ASSETS_ROOT}")
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


# Root proposal: imported BNW unit/treasury continuation source cohort.
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8 AND TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE
        ../src/runtime/bnw_live_player_gold_pending.cpp
        ../src/runtime/bnw_treasury_same_vm_income_loan.cpp
        ../src/runtime/bnw_two_layer_unit_pathfinder.cpp
        ../src/runtime/bnw_unit_mission_continuation_checkpoint.cpp)
endif()

# Root proposal: actual imported Pregame ReadArchive -> native22 Sync cache.
target_sources(civis_runtime PRIVATE ../src/runtime/bnw_imported_pregame_read_cache.cpp)

# Private Source15: retained early Civis localization before imported Pregame Read.
target_sources(civis_runtime PRIVATE ../src/runtime/bnw_imported_pregame_locale_owner.cpp)

# Private proposal: actual SP Pregame/client startup before imported Read.
target_sources(civis_runtime PRIVATE ../src/runtime/bnw_sp_client_pregame_startup.cpp)

# Root: canonical BNW SP client update under actual Win32 source/time loan.
if(WIN32 AND TARGET civis_runtime)
    target_sources(civis_runtime PRIVATE ../src/runtime/bnw_sp_client_update_loan.cpp)
endif()
