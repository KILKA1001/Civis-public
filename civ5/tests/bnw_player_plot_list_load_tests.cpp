#include "civ5/runtime/bnw_player_plot_list_load.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool condition,const char *reason) {
    if(!condition) throw std::runtime_error(reason);
}
template<class F> void rejects(F &&operation) {
    bool blocked=false;
    try {operation();} catch(const std::exception &) {blocked=true;}
    check(blocked,"Invalid BNW post-load player plot list accepted");
}
saves::PlayerTailNode &plot_node(saves::SavedPlayerObservation &player) {
    for(auto &entry:player.tail.fields)
        if(entry.name && std::string_view(entry.name)=="m_aiPlots") return entry;
    throw std::runtime_error("Selected BNW player plot-list unavailable");
}
const saves::PlayerTailNode &plot_node(const saves::SavedPlayerObservation &player) {
    for(const auto &entry:player.tail.fields)
        if(entry.name && std::string_view(entry.name)=="m_aiPlots") return entry;
    throw std::runtime_error("Selected BNW player plot-list unavailable");
}
void selected(const std::filesystem::path &path,
              const std::filesystem::path &game_root) {
    const auto source_hash=runtime::identify_file(path).sha256;
    auto bootstrap=ui::bootstrap_selected_bnw_preview(path,game_root,0);
    check(!bootstrap.preview.blocker && bootstrap.preview.host,
          "Selected Gandhi BNW preview unavailable");
    const auto &host=*bootstrap.preview.host;
    const auto &source=host.session().source();
    const auto &state=host.session().state();
    const auto lists=runtime::rebuild_bnw_player_plot_lists(
        state,source,host.identity());
    check(lists.current && state.player_plot_lists.current &&
          state.player_plot_lists.players.size()==lists.players.size() &&
          state.player_plot_lists.map_plot_count==lists.map_plot_count &&
          lists.players.size()==64 &&
          lists.map_plot_count==state.world.map_header.grid.size() &&
          lists.map_plot_count>0,
          "BNW plot-list load did not reconstruct complete owner roster");
    bool saw_short_prefix=false;
    std::size_t saved_entries=0;
    for(std::size_t i=0;i<64;++i) {
        const auto &raw=plot_node(source.snapshot.all_players->players[i]);
        const auto &owned=lists.players.at(i);
        check(raw.wire_count && *raw.wire_count>=0 &&
              owned.source_wire_count==raw.values.size() &&
              owned.plots.size()==lists.map_plot_count,
              "BNW plot-list player dimension or wire prefix changed");
        saved_entries+=raw.values.size();
        saw_short_prefix|=raw.values.size()<owned.plots.size();
        for(std::size_t j=0;j<owned.plots.size();++j)
            check(owned.plots[j]==(j<raw.values.size()?raw.values[j]:-1),
                  "BNW CvPlayer::Read did not preserve prefix/-1 padding");
    }
    check(saw_short_prefix && !host.session().readiness().can_continue(),
          "Selected BNW lacks tested -1 suffix or incorrectly enables turn");
    {
        auto bad=state;
        auto &node=plot_node(bad.world.all_players->players[0]);
        node.wire_count=static_cast<std::int64_t>(lists.map_plot_count)+1;
        rejects([&] {(void)runtime::rebuild_bnw_player_plot_lists(
            bad,source,host.identity());});
        node.wire_count=plot_node(state.world.all_players->players[0]).wire_count;
        if(!node.values.empty()) {
            node.values[0]=static_cast<std::int64_t>(lists.map_plot_count);
            rejects([&] {(void)runtime::rebuild_bnw_player_plot_lists(
                bad,source,host.identity());});
            node.values[0]=plot_node(state.world.all_players->players[0]).values[0];
        }
        bad.world.map_header.grid.width++;
        rejects([&] {(void)runtime::rebuild_bnw_player_plot_lists(
            bad,source,host.identity());});
        bad.world.map_header.grid.width=state.world.map_header.grid.width;
        bad.world.all_players->players[0].saved_player_id=1;
        rejects([&] {(void)runtime::rebuild_bnw_player_plot_lists(
            bad,source,host.identity());});
    }
    check(runtime::identify_file(path).sha256==source_hash,
          "BNW plot-list load fixture changed original .Civ5Save");
    std::cout << "PASS BNW player plot-list Read hook: " << saved_entries
              << " saved values, " << lists.map_plot_count << " map slots\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi BNW save and installed game root");
        selected(argv[1],argv[2]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW player plot-list Read hook: " << error.what() << '\n';
        return 1;
    }
}
