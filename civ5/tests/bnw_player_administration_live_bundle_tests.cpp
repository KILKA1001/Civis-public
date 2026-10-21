#include "civ5/runtime/bnw_player_administration_live_bundle.hpp"
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
void check(bool ok,const char *why) {
    if(!ok) throw std::runtime_error(why);
}
template<class F> void rejects(F &&f) {
    bool failed=false;
    try {f();} catch(const std::exception &) {failed=true;}
    check(failed,"Unsafe BNW ID36 live bundle accepted");
}
std::string_view name(const char *text) {
    return text?std::string_view(text):std::string_view();
}
saves::PlayerAdministrationRecord &block(
    saves::SavedPlayerObservation &player,std::string_view wanted) {
    for(auto &entry:player.administration.blocks)
        if(name(entry.name)==wanted) return entry;
    throw std::runtime_error("Selected BNW administration block unavailable");
}
const saves::PlayerAdministrationRecord &block(
    const saves::SavedPlayerObservation &player,std::string_view wanted) {
    for(const auto &entry:player.administration.blocks)
        if(name(entry.name)==wanted) return entry;
    throw std::runtime_error("Selected BNW administration block unavailable");
}
std::int64_t &field(saves::PlayerAdministrationRecord &record,
                    std::string_view wanted) {
    for(auto &entry:record.fields)
        if(name(entry.name)==wanted) return entry.value;
    throw std::runtime_error("Selected BNW administration field unavailable");
}
std::int64_t field(const saves::PlayerAdministrationRecord &record,
                   std::string_view wanted) {
    for(const auto &entry:record.fields)
        if(name(entry.name)==wanted) return entry.value;
    throw std::runtime_error("Selected BNW administration field unavailable");
}
const saves::CivisSaveSection &section(
    const saves::CivisSaveDocument &document,std::uint32_t id) {
    for(const auto &entry:document.sections) if(entry.id==id) return entry;
    throw std::runtime_error("BNW ID36 document section unavailable");
}
saves::SavedSessionDecodeContext full_context() {
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    context.permit_live_bnw_player_administration_overlay=true;
    return context;
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
    const auto &source=host.session().source();
    auto changed=source.snapshot;
    std::size_t changed_player=64;
    std::string_view flag;
    for(std::size_t i=0;i<64 && changed_player==64;++i) {
        auto &pre=block(changed.all_players->players[i],"pre_city");
        for(const auto candidate:{"m_bEverPoppedGoody","m_bEverTrainedBuilder"})
            if(field(pre,candidate)==0) {
                field(pre,candidate)=1;
                changed_player=i;flag=candidate;break;
            }
    }
    check(changed_player<64,"Selected BNW has no unset goody/builder history flag");
    // The ring cursor is a real CvNotifications field, even if the selected
    // turn has no active notification. A live UI owner must be made afresh.
    std::size_t changed_ring=64;
    for(std::size_t i=0;i<64 && changed_ring==64;++i) {
        if(!host.session().state().setup.players.contains(i) ||
           !host.session().state().setup.players.at(i).alive) continue;
        auto &ring=block(changed.all_players->players[i],"notifications");
        if(field(ring,"has_notifications")!=1 ||
           field(ring,"version")!=2 ||
           field(ring,"m_ePlayer")!=static_cast<std::int64_t>(i)) continue;
        auto &next=field(ring,"m_iCurrentLookupIndex");
        if(next>=0 && next<INT32_MAX) {++next;changed_ring=i;}
    }
    check(changed_ring<64,"Selected BNW has no version-2 live notification ring");
    const auto raw=runtime::make_bnw_player_administration_checkpoint(
        changed,source.snapshot,host.identity());
    {
        auto bundle=runtime::restore_bnw_player_administration_notification_bundle(
            raw,host.session().state(),source,host.database(),host.identity());
        check(bundle.changed_notification_players==1 && bundle.changed_player_flags==1 &&
              field(block(bundle.state.world.all_players->players[changed_player],
                          "pre_city"),flag)==1 &&
              field(block(bundle.state.world.all_players->players[changed_ring],
                          "notifications"),"m_iCurrentLookupIndex")==
                  field(block(changed.all_players->players[changed_ring],
                              "notifications"),"m_iCurrentLookupIndex") &&
              field(block(source.snapshot.all_players->players[changed_player],
                          "pre_city"),flag)==0,
              "BNW ID36 detached bundle lost notification/flag or changed source");
    }
    {
        auto bad=changed;
        ++field(block(bad.all_players->players[0],"treasury"),"m_iGold");
        const auto unsafe=runtime::make_bnw_player_administration_checkpoint(
            bad,source.snapshot,host.identity());
        rejects([&] {(void)runtime::restore_bnw_player_administration_notification_bundle(
            unsafe,host.session().state(),source,host.database(),host.identity());});
        bad=changed;
        auto &queue=block(bad.all_players->players[0],"pre_city").arrays[3].values;
        if(!queue.empty()) ++queue.front();
        else ++field(block(bad.all_players->players[0],"pre_city"),
                       "m_eEndTurnBlockingType");
        const auto unsafe_queue=runtime::make_bnw_player_administration_checkpoint(
            bad,source.snapshot,host.identity());
        rejects([&] {(void)runtime::restore_bnw_player_administration_notification_bundle(
            unsafe_queue,host.session().state(),source,host.database(),host.identity());});
        bad=changed;
        field(block(bad.all_players->players[changed_ring],"notifications"),
              "m_iNotificationsBeginIndex")=100;
        const auto unsafe_ring=runtime::make_bnw_player_administration_checkpoint(
            bad,source.snapshot,host.identity());
        rejects([&] {(void)runtime::restore_bnw_player_administration_notification_bundle(
            unsafe_ring,host.session().state(),source,host.database(),host.identity());});
        auto stale=host.session().state();
        ++stale.treasury.players.at(0).balance_times100;
        rejects([&] {(void)runtime::restore_bnw_player_administration_notification_bundle(
            raw,stale,source,host.database(),host.identity());});
        check(field(block(host.session().state().world.all_players->players[changed_player],
                          "pre_city"),flag)==0,
              "Rejected ID36 bundle mutated baseline LiveSession");
    }
    const auto suffix=std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    Cleanup cleanup{
        std::filesystem::current_path()/("bnw-id36-bundle-"+suffix+"-1.CivisSave"),
        std::filesystem::current_path()/("bnw-id36-bundle-"+suffix+"-2.CivisSave")};
    auto document=saves::make_civis_import_document(source.source,original.sha256);
    document.sections.push_back(raw);
    saves::write_civis_save(cleanup.first,document);
    auto normal=ui::bootstrap_selected_bnw_preview(cleanup.first,game_root,0);
    check(!normal.preview.blocker && normal.preview.host &&
          field(block(normal.preview.host->session().state().world.all_players->players[changed_player],
                      "pre_city"),flag)==1 &&
          field(block(normal.preview.host->session().state().world.all_players->players[changed_ring],
                      "notifications"),"m_iCurrentLookupIndex")==
              field(block(changed.all_players->players[changed_ring],
                          "notifications"),"m_iCurrentLookupIndex") &&
          !normal.preview.host->session().readiness().can_continue(),
          "Normal BNW LiveSession did not restore supported ID36 changes");
    {
        auto imported=saves::read_saved_session(cleanup.first,full_context());
        check(imported.source.live_bnw_player_administration &&
              *imported.source.live_bnw_player_administration==raw.bytes,
              "Physical BNW ID36 import dropped live administration section");
        {
            auto first=runtime::restore_bnw_player_administration_notification_bundle(
                section(saves::read_civis_save(cleanup.first),raw.id),
                host.session().state(),imported,host.database(),host.identity());
            check(first.changed_notification_players==1 && first.changed_player_flags==1,
                  "Physical BNW ID36 import did not restore detached live state");
        }
        auto reexport=saves::make_civis_import_document(imported.source,original.sha256);
        check(section(reexport,raw.id).bytes==raw.bytes,
              "BNW ID36 physical re-export lost source-bound section");
        saves::write_civis_save(cleanup.second,reexport);
    }
    {
        auto again=saves::read_saved_session(cleanup.second,full_context());
        auto second=runtime::restore_bnw_player_administration_notification_bundle(
            section(saves::read_civis_save(cleanup.second),raw.id),
            host.session().state(),again,host.database(),host.identity());
        check(second.changed_notification_players==1 && second.changed_player_flags==1 &&
              runtime::identify_file(path).sha256==original.sha256 &&
              !host.session().readiness().can_continue(),
              "BNW ID36 re-import diverged, altered original or claimed a full turn");
    }
    std::cout << "PASS BNW ID36 notification/flag detached live bundle\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi BNW save and installed game root");
        selected(argv[1],argv[2]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW ID36 live bundle: " << error.what() << '\n';
        return 1;
    }
}
