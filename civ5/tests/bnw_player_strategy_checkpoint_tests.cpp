#include "civ5/runtime/bnw_player_strategy_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
template<class F> void rejects(F &&f) {
    bool rejected=false;
    try {f();} catch(const std::exception &) {rejected=true;}
    check(rejected,"Invalid full BNW strategy checkpoint was accepted");
}
saves::PlayerStrategyBlock &block(saves::SavedPlayerObservation &player,
                                  std::string_view wanted) {
    for(auto &part:player.strategy.blocks)
        if(part.name && std::string_view(part.name)==wanted) return part;
    throw std::runtime_error("Selected BNW strategy block missing");
}
std::int64_t &field(saves::PlayerStrategyBlock &part,std::string_view wanted) {
    for(auto &item:part.fields)
        if(item.name && std::string_view(item.name)==wanted) return item.value;
    throw std::runtime_error("Selected BNW strategy scalar missing");
}
std::vector<std::int64_t> &array(saves::PlayerStrategyBlock &part,
                                 std::string_view wanted) {
    for(auto &item:part.arrays)
        if(item.name && std::string_view(item.name)==wanted) return item.values;
    throw std::runtime_error("Selected BNW strategy array missing");
}
void selected(const std::filesystem::path &path) {
    const auto original=runtime::identify_file(path);
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto imported=saves::read_saved_session(path,context);
    check(imported.snapshot.all_players &&
          imported.snapshot.all_players->players.size()==64,
          "Selected Gandhi save has no complete player strategies");
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,std::string(64,'a')};
    auto source=std::move(imported.snapshot);
    saves::CivisSaveSection section;
    {
        auto live=source;
        auto &player=live.all_players->players.at(0);
        auto &dirty=field(block(player,"economic_ai"),"exploration_plots_dirty");
        dirty=1-dirty;
        auto &specialization=field(block(player,"city_specialization_ai"),
                                   "specializations_dirty");
        specialization=1-specialization;
        ++field(block(player,"grand_strategy_ai"),"turns_since_active_set");
        auto &flavors=array(block(player,"wonder_production_ai"),"latest_flavors");
        check(!flavors.empty(),"Selected BNW wonder flavor array is empty");
        ++flavors.front();
        section=runtime::make_bnw_player_strategy_checkpoint(live,source,identity);
        check(section.id==runtime::bnw_player_strategy_checkpoint_section_id &&
              section.bytes.at(8)==1,
              "BNW strategy section identity/version changed");
        auto wrong=live;
        wrong.all_players->players[0].strategy.blocks.pop_back();
        rejects([&]{(void)runtime::make_bnw_player_strategy_checkpoint(
            wrong,source,identity);});
        wrong=live;
        array(block(wrong.all_players->players[0],"wonder_production_ai"),
              "latest_flavors").push_back(0);
        rejects([&]{(void)runtime::make_bnw_player_strategy_checkpoint(
            wrong,source,identity);});
        wrong=live;
        wrong.all_players->players[0].saved_player_id=42;
        rejects([&]{(void)runtime::make_bnw_player_strategy_checkpoint(
            wrong,source,identity);});
    }
    const auto output=std::filesystem::current_path()/(
        "bnw-strategy-checkpoint-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+
        ".CivisSave");
    {
        auto document=saves::make_civis_import_document(imported.source,original.sha256);
        document.sections.push_back(section);
        saves::write_civis_save(output,document);
    }
    struct Cleanup {std::filesystem::path path;~Cleanup(){
        std::error_code ignored;std::filesystem::remove(path,ignored);
    }} cleanup{output};
    {
        const auto imported_container=saves::read_loaded_save(output);
        check(imported_container.live_bnw_player_strategy &&
              *imported_container.live_bnw_player_strategy==section.bytes &&
              !imported_container.live_bnw_exploration_dirty,
              "Common Civis importer lost BNW strategy section");
        auto conflicting=imported_container;
        conflicting.live_bnw_exploration_dirty=section.bytes;
        rejects([&]{(void)saves::make_civis_import_document(
            conflicting,original.sha256);});
        const auto reopened=saves::read_civis_save(output);
        const saves::CivisSaveSection *read_section=nullptr;
        for(const auto &item:reopened.sections)
            if(item.id==section.id) read_section=&item;
        check(read_section && read_section->bytes==section.bytes,
              "Atomic CivisSave write/read lost full BNW strategy section");
        auto restored=source;
        runtime::restore_bnw_player_strategy_checkpoint(
            *read_section,restored,source,identity);
        auto &player=restored.all_players->players[0];
        auto &old=source.all_players->players[0];
        check(field(block(player,"economic_ai"),"exploration_plots_dirty")==
                  1-field(block(old,"economic_ai"),"exploration_plots_dirty") &&
              field(block(player,"city_specialization_ai"),"specializations_dirty")==
                  1-field(block(old,"city_specialization_ai"),"specializations_dirty") &&
              field(block(player,"grand_strategy_ai"),"turns_since_active_set")==
                  1+field(block(old,"grand_strategy_ai"),"turns_since_active_set") &&
              array(block(player,"wonder_production_ai"),"latest_flavors").front()==
                  1+array(block(old,"wonder_production_ai"),"latest_flavors").front(),
              "BNW six-block strategy values did not round-trip");
    }
    check(runtime::identify_file(path).sha256==original.sha256,
          "BNW strategy checkpoint changed source Civ5Save");
    auto destination=source;
    auto corrupt=section;
    corrupt.bytes.back()=0x7f;
    for(std::size_t i=1;i<=7;++i) corrupt.bytes[corrupt.bytes.size()-1-i]=0xff;
    rejects([&]{runtime::restore_bnw_player_strategy_checkpoint(
        corrupt,destination,source,identity);});
    check(field(block(destination.all_players->players[0],"economic_ai"),
                "exploration_plots_dirty")==
          field(block(source.all_players->players[0],"economic_ai"),
                "exploration_plots_dirty"),
          "Late strategy value rejection partially changed destination");
    corrupt=section;corrupt.bytes[8]=2;
    rejects([&]{runtime::restore_bnw_player_strategy_checkpoint(
        corrupt,destination,source,identity);});
    auto foreign=identity;foreign.rules_sha256[0]='b';
    rejects([&]{runtime::restore_bnw_player_strategy_checkpoint(
        section,destination,source,foreign);});
    std::cout << "PASS BNW 64-player six-block strategy checkpoint\n";
}
}

int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save");
        selected(argv[1]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW strategy checkpoint: " << error.what() << '\n';
        return 1;
    }
}
