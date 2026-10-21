#include "civ5/runtime/bnw_player_read_tail_transients.hpp"
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
    check(blocked,"Invalid BNW player Read-tail state accepted");
}
std::int64_t &core(saves::SavedPlayerObservation &player,
                   std::string_view wanted) {
    for(auto &field:player.core.fields)
        if(field.name && wanted==field.name && field.values.size()==1)
            return field.values.front();
    throw std::runtime_error("Selected BNW core scalar unavailable");
}
std::int64_t core(const saves::SavedPlayerObservation &player,
                  std::string_view wanted) {
    for(const auto &field:player.core.fields)
        if(field.name && wanted==field.name && field.values.size()==1)
            return field.values.front();
    throw std::runtime_error("Selected BNW core scalar unavailable");
}
void selected(const std::filesystem::path &path,
              const std::filesystem::path &game_root) {
    const auto original=runtime::identify_file(path).sha256;
    auto bootstrap=ui::bootstrap_selected_bnw_preview(path,game_root,0);
    check(!bootstrap.preview.blocker && bootstrap.preview.host,
          "Selected Gandhi BNW preview unavailable");
    const auto &host=*bootstrap.preview.host;
    const auto &state=host.session().state();
    const auto &source=host.session().source();
    const auto loaded=runtime::rebuild_bnw_player_read_tail_transients(
        state,source,host.identity());
    check(loaded.current && state.player_read_tail.current &&
          state.player_read_tail.requests.size()==loaded.requests.size() &&
          state.player_read_tail.active_player_ids==loaded.active_player_ids &&
          loaded.requests.size()==22,
          "BNW CvDiplomacyRequests::Init major roster changed");
    for(std::int64_t i=0;i<64;++i) {
        check(loaded.requests.contains(i)==(i<22),
              "BNW request object branch differed from SDK MAX_MAJOR_CIVS");
        if(i<22) {
            const auto &request=loaded.requests.at(i);
            check(request.player==i && request.next_ai_player==-1 &&
                  !request.request_active && request.active_from_player==-1 &&
                  request.request_count==0,
                  "BNW DiplomacyRequests::Init/Uninit transient values changed");
        }
    }
    std::vector<std::int64_t> expected;
    for(std::int64_t i=0;i<64;++i) {
        const auto &player=state.world.all_players->players[static_cast<std::size_t>(i)];
        if(core(player,"m_bAlive") && core(player,"m_bTurnActive"))
            expected.push_back(i);
    }
    check(loaded.active_player_ids==expected &&
          !host.session().readiness().can_continue(),
          "BNW CvGame active-turn query changed or turn was enabled");
    {
        auto bad=state;
        bad.world.all_players->context.strategy.major=23;
        rejects([&] {(void)runtime::rebuild_bnw_player_read_tail_transients(
            bad,source,host.identity());});
        bad.world.all_players->context.strategy.major=22;
        core(bad.world.all_players->players[0],"m_eID")=1;
        rejects([&] {(void)runtime::rebuild_bnw_player_read_tail_transients(
            bad,source,host.identity());});
        core(bad.world.all_players->players[0],"m_eID")=0;
        bad.setup.players.at(0).alive=!bad.setup.players.at(0).alive;
        rejects([&] {(void)runtime::rebuild_bnw_player_read_tail_transients(
            bad,source,host.identity());});
    }
    check(runtime::identify_file(path).sha256==original,
          "BNW Read-tail fixture changed original .Civ5Save");
    std::cout << "PASS BNW player Read tail: 22 request owners, "
              << loaded.active_player_ids.size() << " active turns\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi BNW save and installed game root");
        selected(argv[1],argv[2]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW player Read tail: " << error.what() << '\n';
        return 1;
    }
}
