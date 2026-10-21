#include "civ5/runtime/bnw_player_knowledge_live_bundle.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <chrono>
#include <climits>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool condition,const char *why) {
    if(!condition) throw std::runtime_error(why);
}
template<class F> void rejects(F &&operation) {
    bool failed=false;
    try {operation();} catch(const std::exception &) {failed=true;}
    check(failed,"Unsafe BNW ID35 live bundle accepted");
}
std::string_view name(const char *text) {
    return text?std::string_view(text):std::string_view();
}
saves::PlayerKnowledgeBlock &block(saves::SavedPlayerObservation &player,
                                   std::string_view wanted) {
    for(auto &item:player.knowledge.blocks)
        if(name(item.name)==wanted) return item;
    throw std::runtime_error("Selected BNW knowledge block unavailable");
}
const saves::PlayerKnowledgeBlock &block(const saves::SavedPlayerObservation &player,
                                         std::string_view wanted) {
    for(const auto &item:player.knowledge.blocks)
        if(name(item.name)==wanted) return item;
    throw std::runtime_error("Selected BNW knowledge block unavailable");
}
std::int64_t &field(saves::PlayerKnowledgeBlock &owner,
                    std::string_view wanted) {
    for(auto &item:owner.fields)
        if(name(item.name)==wanted) return item.value;
    throw std::runtime_error("Selected BNW knowledge field unavailable");
}
std::int64_t field(const saves::PlayerKnowledgeBlock &owner,
                   std::string_view wanted) {
    for(const auto &item:owner.fields)
        if(name(item.name)==wanted) return item.value;
    throw std::runtime_error("Selected BNW knowledge field unavailable");
}
std::vector<std::int64_t> &array(saves::PlayerKnowledgeBlock &owner,
                                  std::string_view wanted) {
    for(auto &item:owner.arrays)
        if(name(item.name)==wanted) return item.values;
    throw std::runtime_error("Selected BNW knowledge array unavailable");
}
saves::TeamHashedArray &hashed(saves::PlayerKnowledgeBlock &owner,
                                std::string_view wanted) {
    for(auto &item:owner.hashes)
        if(name(item.name)==wanted) return item;
    throw std::runtime_error("Selected BNW knowledge hash array unavailable");
}
saves::SavedSessionDecodeContext full_context() {
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    context.permit_live_bnw_player_knowledge_overlay=true;
    return context;
}
const saves::CivisSaveSection &section(const saves::CivisSaveDocument &document,
                                       std::uint32_t id) {
    for(const auto &item:document.sections) if(item.id==id) return item;
    throw std::runtime_error("Missing BNW ID35 section");
}
struct Cleanup {
    std::filesystem::path first,second;
    ~Cleanup() {
        std::error_code error;
        std::filesystem::remove(first,error);
        std::filesystem::remove(second,error);
    }
};
void selected(const std::filesystem::path &path,
              const std::filesystem::path &game_root) {
    const auto original=runtime::identify_file(path);
    auto bootstrap=ui::bootstrap_selected_bnw_preview(path,game_root,0);
    check(!bootstrap.preview.blocker && bootstrap.preview.host,
          "Selected Gandhi BNW preview unavailable");
    auto &host=*bootstrap.preview.host;
    const auto &source=host.session().source().snapshot;
    auto changed=source;
    auto &prophets=field(block(changed.all_players->players.at(0),
                               "player_religions"),"prophets_spawned");
    check(prophets>=0 && prophets<INT32_MAX-1,
          "Selected BNW prophet counter has no safe next value");
    ++prophets;
    const auto raw=runtime::make_bnw_player_knowledge_checkpoint(
        changed,source,host.identity());
    const auto types=host.database().cache_info_types();
    check(types.report().all_succeeded(),"Selected BNW type catalog unavailable");
    {
        auto altered=changed;
        ++field(block(altered.all_players->players.at(0),
                      "player_religions"),"founding_religion");
        const auto unsafe=runtime::make_bnw_player_knowledge_checkpoint(
            altered,source,host.identity());
        rejects([&] {(void)runtime::restore_bnw_player_knowledge_prophet_bundle(
            unsafe,host.session().state(),host.session().source(),
            host.technologies(),types,host.database(),host.identity());});
        altered=changed;
        auto &flavors=array(block(altered.all_players->players.at(0),
                                  "flavor_manager"),"active_flavors");
        check(!flavors.empty() && flavors.front()<INT32_MAX,
              "Selected BNW active flavor unavailable");
        ++flavors.front();
        const auto unsafe_flavor=runtime::make_bnw_player_knowledge_checkpoint(
            altered,source,host.identity());
        rejects([&] {(void)runtime::restore_bnw_player_knowledge_prophet_bundle(
            unsafe_flavor,host.session().state(),host.session().source(),
            host.technologies(),types,host.database(),host.identity());});
        altered=changed;
        auto &researching=hashed(block(altered.all_players->players.at(0),
                                        "player_techs"),"researching_tech");
        bool changed_flag=false;
        for(auto &row:researching.rows)
            if(row.type_hash!=0 && row.values.size()==1) {
                row.values[0]^=1;changed_flag=true;break;
            }
        check(changed_flag,"Selected BNW researching flags unavailable");
        const auto unsafe_research=runtime::make_bnw_player_knowledge_checkpoint(
            altered,source,host.identity());
        rejects([&] {(void)runtime::restore_bnw_player_knowledge_prophet_bundle(
            unsafe_research,host.session().state(),host.session().source(),
            host.technologies(),types,host.database(),host.identity());});
        auto stale=host.session().state();
        stale.research_turn.players.at(0).cities_founded++;
        rejects([&] {(void)runtime::restore_bnw_player_knowledge_prophet_bundle(
            raw,stale,host.session().source(),host.technologies(),types,
            host.database(),host.identity());});
    }
    const auto suffix=std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    Cleanup cleanup{
        std::filesystem::current_path()/("bnw-prophet-bundle-"+suffix+"-1.CivisSave"),
        std::filesystem::current_path()/("bnw-prophet-bundle-"+suffix+"-2.CivisSave")};
    auto document=saves::make_civis_import_document(
        host.session().source().source,original.sha256);
    document.sections.push_back(raw);
    saves::write_civis_save(cleanup.first,document);
    auto normal=ui::bootstrap_selected_bnw_preview(cleanup.first,game_root,0);
    check(!normal.preview.blocker && normal.preview.host &&
          field(block(normal.preview.host->session().state().world.all_players->players.at(0),
                      "player_religions"),"prophets_spawned")==prophets &&
          !normal.preview.host->session().readiness().can_continue(),
          "Normal BNW LiveSession did not restore supported ID35 prophet change");
    auto imported=saves::read_saved_session(cleanup.first,full_context());
    check(imported.source.live_bnw_player_knowledge &&
          *imported.source.live_bnw_player_knowledge==raw.bytes,
          "Physical BNW ID35 import lost raw state");
    auto bundle=runtime::restore_bnw_player_knowledge_prophet_bundle(
        section(saves::read_civis_save(cleanup.first),raw.id),
        host.session().state(),imported,host.technologies(),types,
        host.database(),host.identity());
    check(bundle.changed_players==1 &&
          field(block(bundle.state.world.all_players->players.at(0),
                      "player_religions"),"prophets_spawned")==prophets &&
          field(block(imported.snapshot.all_players->players.at(0),
                      "player_religions"),"prophets_spawned")==prophets-1,
          "BNW prophet bundle did not preserve source and owned raw state");
    check(!host.session().readiness().can_continue(),
          "BNW prophet bundle incorrectly enabled full turn");
    auto reexport=saves::make_civis_import_document(
        imported.source,original.sha256);
    check(section(reexport,raw.id).bytes==raw.bytes,
          "BNW prophet bundle re-export lost ID35");
    saves::write_civis_save(cleanup.second,reexport);
    const auto again=saves::read_saved_session(cleanup.second,full_context());
    auto restored=runtime::restore_bnw_player_knowledge_prophet_bundle(
        section(saves::read_civis_save(cleanup.second),raw.id),
        host.session().state(),again,host.technologies(),types,
        host.database(),host.identity());
    check(restored.changed_players==1 &&
          field(block(restored.state.world.all_players->players.at(0),
                      "player_religions"),"prophets_spawned")==prophets &&
          runtime::identify_file(path).sha256==original.sha256,
          "BNW prophet bundle second import diverged or modified source");
    std::cout << "PASS BNW ID35 prophet-only detached live bundle\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi BNW save and installed game root");
        selected(argv[1],argv[2]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW ID35 prophet live bundle: " << error.what() << '\n';
        return 1;
    }
}
