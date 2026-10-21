#include "civ5/runtime/bnw_notification_load_transients.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
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
    bool blocked=false;
    try {operation();} catch(const std::exception &) {blocked=true;}
    check(blocked,"Invalid BNW notification post-load state accepted");
}
std::int64_t &field(saves::PlayerAdministrationRecord &record,
                    std::string_view wanted) {
    for(auto &entry:record.fields)
        if(entry.name && wanted==entry.name) return entry.value;
    throw std::runtime_error("BNW notification field unavailable");
}
std::int64_t field(const saves::PlayerAdministrationRecord &record,
                   std::string_view wanted) {
    for(const auto &entry:record.fields)
        if(entry.name && wanted==entry.name) return entry.value;
    throw std::runtime_error("BNW notification field unavailable");
}
void selected(const std::filesystem::path &path,
              const std::filesystem::path &game_root) {
    const auto source_hash=runtime::identify_file(path).sha256;
    auto bootstrap=ui::bootstrap_selected_bnw_preview(path,game_root,0);
    check(!bootstrap.preview.blocker && bootstrap.preview.host,
          "Selected Gandhi BNW preview unavailable");
    auto &host=*bootstrap.preview.host;
    const auto &source=host.session().source();
    const auto &state=host.session().state();
    const auto loaded=runtime::rebuild_bnw_notification_load_transients(
        state,source,host.identity());
    check(state.notification_load_transients.current &&
          state.notification_load_transients.players==loaded.players &&
          state.notification_load_transients.external_update_pending==
              loaded.external_update_pending,
          "Normal BNW LiveSession did not rebuild notification read transients");
    std::size_t rings=0,active=0,first_ring=64;
    for(std::size_t i=0;i<64;++i) {
        const auto &ring=state.world.all_players->players[i].administration.blocks.at(0);
        if(field(ring,"has_notifications")==0) {
            check(!loaded.players.contains(i),
                  "Absent BNW notification object gained transient slots");
            continue;
        }
        if(first_ring==64) first_ring=i;
        ++rings;
        const auto &transient=loaded.players.at(i);
        for(const auto &slot:transient.slots)
            check(slot.needs_broadcast && !slot.wait_extra_turn,
                  "BNW Notification operator>> post-load flags differ from SDK");
        const auto begin=field(ring,"m_iNotificationsBeginIndex");
        const auto end=field(ring,"m_iNotificationsEndIndex");
        std::size_t expected=0;
        for(auto cursor=begin;cursor!=end;cursor=(cursor+1)%100)
            expected+=field(ring.children.at(static_cast<std::size_t>(cursor)),
                            "m_bDismissed")==0;
        check(expected==transient.active_undismissed.size(),
              "BNW post-load ring iteration changed active source order");
        active+=expected;
    }
    check(rings>0 && loaded.players.size()==rings &&
          loaded.external_update_pending==(active>0) &&
          !host.session().readiness().can_continue(),
          "Selected BNW notification load projection or readiness changed");
    {
        auto bad=state;
        auto &ring=bad.world.all_players->players.at(first_ring).administration.blocks.at(0);
        field(ring,"version")=1;
        rejects([&] {(void)runtime::rebuild_bnw_notification_load_transients(
            bad,source,host.identity());});
        field(ring,"version")=2;
        field(ring,"m_iNotificationsBeginIndex")=100;
        rejects([&] {(void)runtime::rebuild_bnw_notification_load_transients(
            bad,source,host.identity());});
        field(ring,"m_iNotificationsBeginIndex")=
            field(state.world.all_players->players[first_ring].administration.blocks[0],
                  "m_iNotificationsBeginIndex");
        bad.world.all_players->players[first_ring].saved_player_id=-1;
        rejects([&] {(void)runtime::rebuild_bnw_notification_load_transients(
            bad,source,host.identity());});
    }
    check(runtime::identify_file(path).sha256==source_hash,
          "BNW notification load fixture modified source .Civ5Save");
    std::cout << "PASS BNW notification post-load transients " << rings
              << " rings, " << active << " active entries\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi BNW save and installed game root");
        selected(argv[1],argv[2]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW notification post-load transients: "
                  << error.what() << '\n';
        return 1;
    }
}
