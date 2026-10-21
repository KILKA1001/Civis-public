#include "civ5/runtime/bnw_research_choice.hpp"
#include "civ5/runtime/research_selection_checkpoint.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/saves/loaded_save.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value,const char *message) {
    if (!value) throw std::runtime_error(message);
}
template<class F> void rejects(F action) {
    bool caught=false;
    try {action();} catch (const std::exception &) {caught=true;}
    check(caught,"Expected research-selection checkpoint rejection");
}
civ5::runtime::LiveSession make_session() {
    using namespace civ5;
    saves::SavedSession source;
    source.snapshot.game_state.layout=saves::GameStateLayout::expansion2_version1;
    runtime::LiveSessionState state{
        source.snapshot,runtime::ResearchContext{}, {},runtime::PlayerResearchContext{},
        runtime::CityYieldContext{},runtime::BnwUnhappinessContext{},
        runtime::MaintenanceContext{},runtime::TreasuryContext{},
        runtime::ScienceContext{},runtime::ResearchTurnState{},
        {runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{5,0,0}),
         runtime::SimulationRandom(runtime::RandomAlgorithm::bnw,{7,0,0})}};
    state.setup.players[5].team=7;
    state.player_research.players[5]={};
    state.research_turn.players[5].founded_first_city=true;
    state.research_selection.players[5].civilization="CIV_TEST";
    state.research_selection.players[5].advanced_start_points=11;
    state.research_selection.disabled_technologies["CIV_TEST"]={};
    state.technologies.emplace_back(content::TechnologyProfile::bnw,7,3);
    state.maintenance.team_eras[7]=0;
    return runtime::LiveSession::from_restored_bnw(std::move(source),std::move(state));
}
} // namespace

int main() {
    try {
        using namespace civ5;
        using namespace civ5::runtime;
        content::TechnologyRules technologies;
        technologies.collection.slots=3;
        for(std::int64_t id=0;id<3;++id) {
            auto &tech=technologies.entries[id];
            tech.booleans={{"Disable",false},{"Repeat",false}};
            tech.and_prerequisites.default_value=-1;
            tech.or_prerequisites.default_value=-1;
            tech.and_prerequisites.extent=2;
            tech.or_prerequisites.extent=2;
        }
        technologies.entries[2].and_prerequisites.values={{0,0},{1,1}};
        content::RulesDatabase database;
        database.apply_sql("CREATE TABLE Defines(Name,Value);INSERT INTO Defines VALUES"
            "('NUM_AND_TECH_PREREQS',2),('NUM_OR_TECH_PREREQS',2);");
        const auto defines=database.cache_global_defines(content::GlobalDefineProfile::bnw);
        const BnwResearchChoiceRules choice_rules{technologies,defines,7};
        auto game=make_session();
        int dirty=0;
        BnwResearchChoiceServices services;
        services.lua_ready=[] {return true;};
        services.lua_test_all_read_only=[](std::string_view,std::int64_t,
            std::int64_t,const LiveSessionState &) -> std::optional<bool> {
            return std::nullopt;
        };
        services.dirty_after_commit=[&](std::int64_t) {++dirty;};
        const auto selected=push_bnw_research_choice(
            game,5,2,false,choice_rules,services);
        check(selected.outcome==LiveActionOutcome::committed && dirty==3 &&
              game.state().player_research.players.at(5).queue==
                  std::vector<std::int64_t>({0,1,2}) &&
              game.state().research_turn.players.at(5).researching==
                  std::set<std::int64_t>({0,1,2}),
              "Research choice did not update the owned queue and flags");
        const SessionCheckpointIdentity identity{std::string(64,'a'),std::string(64,'b')};
        const auto section=make_research_selection_checkpoint(
            game.state().research_selection,game.state().player_research,
            game.state().research_turn,game.state().setup,technologies,identity);
        check(section.id==static_cast<std::uint32_t>(
                  saves::CivisSaveSectionId::live_research_selection) &&
              section.flags==0 && section.bytes.size()>150,
              "Missing versioned research-selection checkpoint section");

        auto resumed=make_session();
        const auto restore=[&](const saves::CivisSaveSection &candidate,
                               const SessionCheckpointIdentity &key,
                               bool queue_match=false,bool flags_match=false) {
            auto &state=resumed.mutable_state();
            restore_research_selection_checkpoint(candidate,state.research_selection,
                state.player_research,state.research_turn,state.setup,technologies,key,
                queue_match,flags_match);
        };
        rejects([&]{restore(section,identity,true,false);});
        rejects([&]{restore(section,identity,false,true);});
        check(resumed.state().player_research.players.at(5).queue.empty() &&
              resumed.state().research_turn.players.at(5).researching.empty(),
              "Conflicting companion overlays partially changed live state");
        restore(section,identity);
        check(resumed.state().player_research.players.at(5).queue==
                  game.state().player_research.players.at(5).queue &&
              resumed.state().research_turn.players.at(5).researching==
                  game.state().research_turn.players.at(5).researching &&
              resumed.state().research_selection.players.at(5).advanced_start_points==11 &&
              resumed.state().research_selection.players.at(5).civilization=="CIV_TEST",
              "Checkpoint failed to restore the complete logical research choice");
        restore(section,identity,true,true);
        auto wrong=identity;wrong.rules_sha256=std::string(64,'c');
        rejects([&]{restore(section,wrong);});
        wrong=identity;wrong.payload_sha256=std::string(64,'c');
        rejects([&]{restore(section,wrong);});
        auto damaged=section;damaged.bytes.pop_back();
        rejects([&]{restore(damaged,identity);});
        damaged=section;damaged.bytes.push_back(0);
        rejects([&]{restore(damaged,identity);});
        damaged=section;damaged.bytes[8]=2;
        rejects([&]{restore(damaged,identity);});
        check(resumed.state().player_research.players.at(5).queue.size()==3,
              "Damaged checkpoint changed the committed queue");
        auto invalid_state=game.state().research_turn;
        invalid_state.players.at(5).researching.erase(1);
        rejects([&]{make_research_selection_checkpoint(
            game.state().research_selection,game.state().player_research,
            invalid_state,game.state().setup,technologies,identity);});
        auto bad_selection=game.state().research_selection;
        bad_selection.players.at(5).civilization="MISSING_CIV";
        rejects([&]{make_research_selection_checkpoint(
            bad_selection,game.state().player_research,game.state().research_turn,
            game.state().setup,technologies,identity);});
        ResearchSelectionCheckpointBudget small;
        small.entries=5;
        rejects([&]{make_research_selection_checkpoint(
            game.state().research_selection,game.state().player_research,
            game.state().research_turn,game.state().setup,technologies,identity,
            small);});

        saves::CivisSaveDocument document;document.flags=1;
        document.sections={{static_cast<std::uint32_t>(
                saves::CivisSaveSectionId::imported_payload),0,{1,2,3}},
            saves::make_save_metadata_section({std::string(64,'d'),{}}),section};
        const auto path=std::filesystem::temp_directory_path()/
            ("civis-research-selection-"+
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+
             ".CivisSave");
        saves::write_civis_save(path,document);
        const auto loaded=saves::read_loaded_save(path);
        check(loaded.live_research_selection==section.bytes,
              "Physical Civis container lost research-selection section");
        const auto repacked=saves::make_civis_import_document(loaded,std::string(64,'e'));
        check(repacked.sections.size()==document.sections.size() &&
              repacked.sections.back().bytes==section.bytes,
              "Repack dropped research-selection checkpoint");
        rejects([&]{(void)saves::observe_loaded_session(loaded,{});});
        std::filesystem::remove(path);
        std::cout << "PASS BNW research selection checkpoint\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL BNW research selection checkpoint: " << error.what() << '\n';
        return 1;
    }
}
