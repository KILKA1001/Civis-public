#include "civ5/runtime/bnw_exploration_dirty_checkpoint.hpp"
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
template<class Fn> void rejects(Fn &&run) {
    bool rejected=false;
    try {run();} catch(const std::exception &) {rejected=true;}
    check(rejected,"Invalid BNW exploration checkpoint was accepted");
}
std::int64_t &dirty(saves::SavedPlayerObservation &player) {
    for(auto &block:player.strategy.blocks)
        if(block.name && std::string_view(block.name)=="economic_ai")
            for(auto &field:block.fields)
                if(field.name && std::string_view(field.name)=="exploration_plots_dirty")
                    return field.value;
    throw std::runtime_error("BNW EconomicAI discovery flag missing");
}
bool alive(const saves::SavedPlayerObservation &player) {
    for(const auto &field:player.core.fields)
        if(field.name && std::string_view(field.name)=="m_bAlive" &&
           field.values.size()==1) return field.values[0]==1;
    throw std::runtime_error("BNW player alive field missing");
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
          "Selected BNW save has no full player roster");
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,std::string(64,'a')};
    // Prefer an unmodified source flag. A synthetic 0-to-1 fixture remains
    // available if every living player in this particular save is already dirty.
    auto source=std::move(imported.snapshot);
    std::size_t recipient=64;
    for(std::size_t i=0;i<64;++i)
        if(alive(source.all_players->players[i]) &&
           dirty(source.all_players->players[i])==0) {recipient=i;break;}
    if(recipient==64) {
        for(std::size_t i=0;i<64;++i)
            if(alive(source.all_players->players[i])) {recipient=i;break;}
        check(recipient<64,"Selected BNW save has no living player");
        dirty(source.all_players->players[recipient])=0;
    }
    saves::CivisSaveSection section;
    {
        auto world=source;
        dirty(world.all_players->players[recipient])=1;
        section=runtime::make_bnw_exploration_dirty_checkpoint(
            world,source,identity);
        auto unrelated=world;
        bool altered=false;
        for(auto &block:unrelated.all_players->players[recipient].strategy.blocks)
            if(block.name && std::string_view(block.name)=="economic_ai") {
                for(auto &field:block.fields)
                    if(field.name && std::string_view(field.name)!=
                       "exploration_plots_dirty") {
                        ++field.value;altered=true;break;
                    }
                if(altered) break;
            }
        check(altered,"Selected BNW EconomicAI has no second scalar for guard test");
        rejects([&]{(void)runtime::make_bnw_exploration_dirty_checkpoint(
            unrelated,source,identity);});
    }
    check(section.id==runtime::bnw_exploration_dirty_checkpoint_section_id &&
          section.bytes.size()==8+4+64*3+4+64*12,
          "BNW exploration section ID or wire count changed");
    const auto output=std::filesystem::current_path()/(
        "bnw-exploration-checkpoint-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+
        ".CivisSave");
    {
        auto document=saves::make_civis_import_document(imported.source,original.sha256);
        document.sections.push_back(section);
        saves::write_civis_save(output,document);
    }
    struct Cleanup {std::filesystem::path path;~Cleanup(){
        std::error_code error;std::filesystem::remove(path,error);
    }} cleanup{output};
    {
        const auto imported_container=saves::read_loaded_save(output);
        check(imported_container.live_bnw_exploration_dirty &&
              *imported_container.live_bnw_exploration_dirty==section.bytes &&
              !imported_container.live_bnw_player_strategy,
              "Common Civis importer lost BNW exploration section");
        auto conflicting=imported_container;
        conflicting.live_bnw_player_strategy=section.bytes;
        rejects([&]{(void)saves::make_civis_import_document(
            conflicting,original.sha256);});
        const auto reopened=saves::read_civis_save(output);
        const saves::CivisSaveSection *read_section=nullptr;
        for(const auto &item:reopened.sections)
            if(item.id==section.id) read_section=&item;
        check(read_section && read_section->bytes==section.bytes,
              "Atomic CivisSave write/read lost BNW exploration section");
        auto restored=source;
        runtime::restore_bnw_exploration_dirty_checkpoint(
            *read_section,restored,source,identity);
        for(std::size_t i=0;i<64;++i)
            check(dirty(restored.all_players->players[i])==
                  (i==recipient?1:dirty(source.all_players->players[i])),
                  "BNW exploration round-trip changed a player flag");
    }
    check(runtime::identify_file(path).sha256==original.sha256,
          "BNW exploration checkpoint changed original Civ5Save");
    auto bad=section;
    bad.bytes[8]=2;
    auto destination=source;
    rejects([&]{runtime::restore_bnw_exploration_dirty_checkpoint(
        bad,destination,source,identity);});
    check(dirty(destination.all_players->players[recipient])==0,
          "Version rejection partially changed player state");
    bad=section;bad.bytes[8+4+64*3+4+12*63]=0;
    bad.bytes[8+4+64*3+4+12*63+1]=0;
    rejects([&]{runtime::restore_bnw_exploration_dirty_checkpoint(
        bad,destination,source,identity);});
    check(dirty(destination.all_players->players[recipient])==0,
          "Late player-ID rejection partially changed player state");
    auto wrong=identity;wrong.rules_sha256[0]='b';
    rejects([&]{runtime::restore_bnw_exploration_dirty_checkpoint(
        section,destination,source,wrong);});
    std::cout << "PASS BNW EconomicAI dirty checkpoint recipient " << recipient << '\n';
}
}

int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save");
        selected(argv[1]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW exploration checkpoint: " << error.what() << '\n';
        return 1;
    }
}
