#include "civ5/saves/loaded_save.hpp"
#include "civ5/runtime/bnw_unit_mission_continuation_checkpoint.hpp"
#include "civ5/runtime/bnw_military_request_checkpoint.hpp"
#include "civ5/runtime/bnw_unit_serial_checkpoint.hpp"
#include "civ5/runtime/bnw_population_totals_checkpoint.hpp"
#include "civ5/runtime/bnw_food_population_growth_checkpoint.hpp"
#include "civ5/runtime/binary_reader.hpp"
#include "civ5/saves/game_tail.hpp"
#include <stdexcept>
#include <utility>

namespace civ5::saves {
const char *save_container_kind_name(SaveContainerKind kind) {
    switch (kind) {
    case SaveContainerKind::civ5:
        return "civ5_chunked_zlib";
    case SaveContainerKind::civis:
        return "civis_uncompressed";
    }
    throw std::runtime_error("Invalid save container kind");
}

LoadedSave read_loaded_save(const std::filesystem::path &path, SavePayloadBudget payload_budget,
                            CivisSaveBudget civis_budget, HeaderBudget metadata_budget) {
    if (runtime::ascii_lower(path.extension().string()) == ".civissave") {
        auto imported = read_imported_civis_save(path, civis_budget, metadata_budget);
        LoadedSave result;
        result.container = SaveContainerKind::civis;
        result.header = std::move(imported.metadata.header);
        result.source_sha256 = std::move(imported.metadata.source_sha256);
        result.bytes = std::move(imported.payload);
        result.source_prefix = std::move(imported.source_prefix);
        result.saved_database_override=std::move(imported.saved_database_override);
        result.live_city_production=std::move(imported.live_city_production);
        result.live_team_research=std::move(imported.live_team_research);
        result.live_treasury=std::move(imported.live_treasury);
        result.live_research_turn=std::move(imported.live_research_turn);
        result.live_research_selection=std::move(imported.live_research_selection);
        result.live_simulation_random=std::move(imported.live_simulation_random);
        result.live_bnw_game_clock=std::move(imported.live_bnw_game_clock);
        result.live_bnw_map_plots=std::move(imported.live_bnw_map_plots);
        result.live_bnw_units=std::move(imported.live_bnw_units);
        result.live_bnw_area_units=std::move(imported.live_bnw_area_units);
        result.live_bnw_player_proximity=std::move(imported.live_bnw_player_proximity);
        result.live_bnw_team_meet=std::move(imported.live_bnw_team_meet);
        result.live_bnw_exploration_dirty=std::move(imported.live_bnw_exploration_dirty);
        result.live_bnw_player_strategy=std::move(imported.live_bnw_player_strategy);
        result.live_bnw_player_core=std::move(imported.live_bnw_player_core);
        result.live_bnw_city_core=std::move(imported.live_bnw_city_core);
        result.live_bnw_city_production=std::move(imported.live_bnw_city_production);
        result.live_bnw_city_management=std::move(imported.live_bnw_city_management);
        result.live_bnw_city_tail=std::move(imported.live_bnw_city_tail);
        result.live_bnw_treasury_balance=std::move(imported.live_bnw_treasury_balance);
        result.live_bnw_team_progress=std::move(imported.live_bnw_team_progress);
        result.live_bnw_culture_bomb_timer=std::move(imported.live_bnw_culture_bomb_timer);
        result.live_bnw_research_queue=std::move(imported.live_bnw_research_queue);
        result.live_bnw_maintenance_expense=std::move(imported.live_bnw_maintenance_expense);
        result.live_bnw_player_knowledge=std::move(imported.live_bnw_player_knowledge);
        result.live_bnw_player_administration=std::move(imported.live_bnw_player_administration);
        result.live_bnw_game_supporting=std::move(imported.live_bnw_game_supporting);
        result.live_bnw_game_score_replay=std::move(imported.live_bnw_game_score_replay);
        result.live_bnw_game_deals=std::move(imported.live_bnw_game_deals);
        result.live_bnw_team_turn=std::move(imported.live_bnw_team_turn);
        result.live_bnw_ui_transients=std::move(imported.live_bnw_ui_transients);
        result.profile_sections=std::move(imported.profile_sections);
        result.standalone_services=std::move(imported.standalone_services);
        result.live_bnw_unit_mission_continuation=std::move(imported.live_bnw_unit_mission_continuation);
        result.live_bnw_military_requests=std::move(imported.live_bnw_military_requests);
        result.live_bnw_unit_serial=std::move(imported.live_bnw_unit_serial);
        result.live_bnw_population_totals=std::move(imported.live_bnw_population_totals);
        result.live_bnw_food_growth_journal=std::move(imported.live_bnw_food_growth_journal);
        result.inflate.available_bytes = result.bytes.size();
        // The payload is complete in the Civis section. This flag is scoped to
        // the container kind and must not be interpreted as a zlib checksum.
        result.inflate.stream_end = true;
        return result;
    }
    auto imported = read_save_payload(path, payload_budget);
    LoadedSave result;
    result.container = SaveContainerKind::civ5;
    result.header = std::move(imported.header);
    result.frame = std::move(imported.frame);
    result.inflate = imported.inflate;
    result.bytes = std::move(imported.bytes);
    result.source_prefix = std::move(imported.source_prefix);
    return result;
}

CivisSaveDocument make_civis_import_document(const LoadedSave &source,std::string_view input_sha256,
    std::optional<std::span<const std::uint8_t>> database_override){
    if(source.bytes.empty())throw std::runtime_error("Cannot save an empty imported payload");
    if(source.container==SaveContainerKind::civis&&!source.source_sha256)
        throw std::runtime_error("Imported Civis save has no original source identity");
    if(!database_override&&source.saved_database_override)database_override=*source.saved_database_override;
    if(database_override)validate_saved_database_image(*database_override);
    if(source.source_prefix)validate_source_prefix(*source.source_prefix,source.header);
    if(source.live_bnw_exploration_dirty&&source.live_bnw_player_strategy)
        throw std::runtime_error("BNW exploration dirty and full player strategy sections cannot coexist");
    if(source.live_bnw_player_proximity&&source.live_bnw_player_core)
        throw std::runtime_error("BNW player proximity and full player core sections cannot coexist");
    if(source.live_bnw_research_queue &&
       (!source.live_team_research || !source.live_research_turn ||
        source.live_bnw_team_progress || source.live_research_selection))
        throw std::runtime_error("BNW research queue requires its exclusive ID8 and ID11 companions");
    if(source.live_bnw_player_knowledge && source.live_bnw_research_queue)
        throw std::runtime_error("BNW full player knowledge and research queue sections overlap");
    if(source.live_bnw_player_administration &&
       (source.live_team_research || source.live_treasury ||
        source.live_research_turn || source.live_research_selection ||
        source.live_bnw_treasury_balance || source.live_bnw_research_queue ||
        source.live_bnw_maintenance_expense))
        throw std::runtime_error(
            "BNW full player administration overlaps research, treasury or maintenance sections");
    // Original imports explicitly select the standalone host profile. Legacy
    // Civis containers retain absence rather than silently changing their history.
    auto services=source.standalone_services;
    if(services) require_supported_standalone_services(*services);
    if(source.container==SaveContainerKind::civ5) services=StandaloneServicesConfig{};
    if(services) require_supported_standalone_services(*services);
    CivisSaveDocument document;document.flags=1;
    if(services) document.sections.push_back(make_standalone_services_section(*services));
    document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::imported_payload),0,source.bytes});
    document.sections.push_back(make_save_metadata_section({source.source_sha256.value_or(std::string(input_sha256)),source.header}));
    if(source.source_prefix)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::source_prefix),0,*source.source_prefix});
    if(database_override)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::saved_database),0,
        {database_override->begin(),database_override->end()}});
    if(source.live_city_production)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_city_production),0,
        *source.live_city_production});
    if(source.live_team_research)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_team_research),0,
        *source.live_team_research});
    if(source.live_treasury)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_treasury),0,
        *source.live_treasury});
    if(source.live_research_turn)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_research_turn),0,
        *source.live_research_turn});
    if(source.live_research_selection)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_research_selection),0,
        *source.live_research_selection});
    if(source.live_simulation_random)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_simulation_random),0,
        *source.live_simulation_random});
    if(source.live_bnw_game_clock)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_game_clock),0,
        *source.live_bnw_game_clock});
    if(source.live_bnw_map_plots)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_map_plots),0,
        *source.live_bnw_map_plots});
    if(source.live_bnw_units)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_units),0,
        *source.live_bnw_units});
    if(source.live_bnw_area_units)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_area_units),0,
        *source.live_bnw_area_units});
    if(source.live_bnw_player_proximity)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_player_proximity),0,
        *source.live_bnw_player_proximity});
    if(source.live_bnw_team_meet)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_team_meet),0,
        *source.live_bnw_team_meet});
    if(source.live_bnw_exploration_dirty)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_exploration_dirty),0,
        *source.live_bnw_exploration_dirty});
    if(source.live_bnw_player_strategy)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_player_strategy),0,
        *source.live_bnw_player_strategy});
    if(source.live_bnw_player_core)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_player_core),0,
        *source.live_bnw_player_core});
    if(source.live_bnw_city_core)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_city_core),0,
        *source.live_bnw_city_core});
    if(source.live_bnw_city_production)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_city_production),0,
        *source.live_bnw_city_production});
    if(source.live_bnw_city_management)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_city_management),0,
        *source.live_bnw_city_management});
    if(source.live_bnw_city_tail)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_city_tail),0,
        *source.live_bnw_city_tail});
    if(source.live_bnw_treasury_balance)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_treasury_balance),0,
        *source.live_bnw_treasury_balance});
    if(source.live_bnw_team_progress)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_team_progress),0,
        *source.live_bnw_team_progress});
    if(source.live_bnw_culture_bomb_timer)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_culture_bomb_timer),0,
        *source.live_bnw_culture_bomb_timer});
    if(source.live_bnw_research_queue)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_research_queue),0,
        *source.live_bnw_research_queue});
    if(source.live_bnw_maintenance_expense)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_maintenance_expense),0,
        *source.live_bnw_maintenance_expense});
    if(source.live_bnw_player_knowledge)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_player_knowledge),0,
        *source.live_bnw_player_knowledge});
    if(source.live_bnw_player_administration)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_player_administration),0,
        *source.live_bnw_player_administration});
    if(source.live_bnw_game_supporting)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_game_supporting),0,
        *source.live_bnw_game_supporting});
    if(source.live_bnw_game_score_replay)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_game_score_replay),0,
        *source.live_bnw_game_score_replay});
    if(source.live_bnw_game_deals)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_game_deals),0,
        *source.live_bnw_game_deals});
    if(source.live_bnw_ui_transients)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_ui_transients),0,
        *source.live_bnw_ui_transients});
    if(source.live_bnw_team_turn)document.sections.push_back({static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_team_turn),0,
        *source.live_bnw_team_turn});
    if(source.live_bnw_food_growth_journal){
        CivisSaveSection journal{47,0,*source.live_bnw_food_growth_journal};
        runtime::validate_bnw_food_growth_journal_shape(journal);document.sections.push_back(std::move(journal));
    }
    if(source.live_bnw_population_totals){
        CivisSaveSection totals{46,0,*source.live_bnw_population_totals};
        runtime::validate_bnw_population_totals_checkpoint_shape(totals);
        document.sections.push_back(std::move(totals));
    }
    if(source.live_bnw_unit_serial){
        CivisSaveSection serial{45,0,*source.live_bnw_unit_serial};
        runtime::validate_bnw_unit_serial_checkpoint_shape(serial);
        document.sections.push_back(std::move(serial));
    }
    if(source.live_bnw_military_requests){
        CivisSaveSection military{44,0,*source.live_bnw_military_requests};
        runtime::validate_bnw_military_request_checkpoint_shape(military);
        if(!source.live_bnw_player_strategy)throw std::runtime_error("Military counters require ID23 companion");
        document.sections.push_back(std::move(military));
    }
    if(source.live_bnw_unit_mission_continuation) {
        CivisSaveSection pending{static_cast<std::uint32_t>(CivisSaveSectionId::live_bnw_unit_mission_continuation),0,
            *source.live_bnw_unit_mission_continuation};
        runtime::validate_bnw_unit_mission_continuation_checkpoint_shape(pending);
        document.sections.push_back(std::move(pending));
    }
    for(const auto &[id,bytes]:source.profile_sections)
        document.sections.push_back({id,0,bytes});
    if(source.live_bnw_population_totals)runtime::validate_bnw_population_totals_checkpoint_companions(document);
    if(source.live_bnw_food_growth_journal)runtime::validate_bnw_food_growth_journal_companions(document);
    return document;
}
} // namespace civ5::saves
