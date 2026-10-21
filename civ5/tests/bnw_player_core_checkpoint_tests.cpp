#include "civ5/runtime/bnw_player_core_checkpoint.hpp"
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
    check(rejected,"Invalid BNW player core checkpoint was accepted");
}
saves::PlayerCoreField &field(saves::SavedPlayerObservation &player,
                              std::string_view wanted) {
    for(auto &part:player.core.fields)
        if(part.name && std::string_view(part.name)==wanted) return part;
    throw std::runtime_error("Selected BNW player core field missing");
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
          "Selected Gandhi save has no full player roster");
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,std::string(64,'a')};
    auto source=std::move(imported.snapshot);
    saves::CivisSaveSection section;
    std::int64_t land_after=0,proximity_after=0;
    {
        auto world=source;
        auto &player=world.all_players->players[0];
        auto &land=field(player,"m_iTotalLandScored").values.at(0);
        ++land;land_after=land;
        auto &proximity=field(player,"m_aiProximityToPlayer").values.at(1);
        proximity=proximity==3?2:3;proximity_after=proximity;
        section=runtime::make_bnw_player_core_checkpoint(world,source,identity);
        check(section.id==runtime::bnw_player_core_checkpoint_section_id &&
              section.bytes.at(8)==1,
              "BNW player core section identity/version changed");
        auto wrong=world;
        field(wrong.all_players->players[0],"m_aiProximityToPlayer").values.push_back(0);
        rejects([&]{(void)runtime::make_bnw_player_core_checkpoint(
            wrong,source,identity);});
        wrong=world;
        wrong.all_players->players[0].saved_player_id=42;
        rejects([&]{(void)runtime::make_bnw_player_core_checkpoint(
            wrong,source,identity);});
        wrong=world;
        field(wrong.all_players->players[0],"m_eID").values[0]=42;
        rejects([&]{(void)runtime::make_bnw_player_core_checkpoint(
            wrong,source,identity);});
    }
    const auto output=std::filesystem::current_path()/(
        "bnw-player-core-checkpoint-"+
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
        const auto reopened=saves::read_civis_save(output);
        const saves::CivisSaveSection *read_section=nullptr;
        for(const auto &item:reopened.sections)
            if(item.id==section.id) read_section=&item;
        check(read_section && read_section->bytes==section.bytes,
              "Atomic CivisSave write/read lost BNW player core section");
        auto restored=source;
        runtime::restore_bnw_player_core_checkpoint(
            *read_section,restored,source,identity);
        auto &player=restored.all_players->players[0];
        check(field(player,"m_iTotalLandScored").values.at(0)==land_after &&
              field(player,"m_aiProximityToPlayer").values.at(1)==proximity_after,
              "BNW player core values did not round-trip");
    }
    check(runtime::identify_file(path).sha256==original.sha256,
          "BNW player core checkpoint changed source Civ5Save");
    auto destination=source;
    auto bad=section;
    bad.bytes.pop_back();
    rejects([&]{runtime::restore_bnw_player_core_checkpoint(
        bad,destination,source,identity);});
    check(field(destination.all_players->players[0],"m_iTotalLandScored").values.at(0)==
          field(source.all_players->players[0],"m_iTotalLandScored").values.at(0),
          "Truncated section partially changed player core");
    bad=section;bad.bytes[8]=2;
    rejects([&]{runtime::restore_bnw_player_core_checkpoint(
        bad,destination,source,identity);});
    auto foreign=identity;foreign.rules_sha256[0]='b';
    rejects([&]{runtime::restore_bnw_player_core_checkpoint(
        section,destination,source,foreign);});
    std::cout << "PASS BNW full 64-player core checkpoint\n";
}
}

int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save");
        selected(argv[1]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW player core checkpoint: " << error.what() << '\n';
        return 1;
    }
}
