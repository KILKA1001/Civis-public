#include "civ5/runtime/bnw_game_plot_extras_load.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
void check(bool condition,const char *why) {
    if(!condition) throw std::runtime_error(why);
}
template<class F> void rejects(F &&operation) {
    bool blocked=false;
    try {operation();} catch(const std::exception &) {blocked=true;}
    check(blocked,"Unsupported BNW plot-extra state/query accepted");
}
void getter_fixture() {
    runtime::BnwGamePlotExtras state{
        {{{2,3},{1,2,3,4,5,6}},{{2,3},{9,9,9,9,9,9}},
         {{7,8},{0,0,0,0,0,4}}},
        {{{2,3},11},{{2,3},22},{{7,8},-4}}};
    for(std::int64_t yield=0;yield<6;++yield)
        check(state.extra_yield(2,3,yield)==yield+1,
              "BNW duplicate plot-extra yield did not use first source row");
    check(state.extra_yield(7,8,5)==4 &&
          state.extra_yield(8,7,5)==0 &&
          state.extra_cost(2,3)==11 && state.extra_cost(7,8)==-4 &&
          state.extra_cost(8,7)==0,
          "BNW CvGame plot-extra getter order/default differs from SDK");
    rejects([&] {(void)state.extra_yield(2,3,6);});
    state.yields.front().yields.pop_back();
    rejects([&] {(void)state.extra_yield(2,3,0);});
}
void selected(const std::filesystem::path &path,
              const std::filesystem::path &game_root) {
    getter_fixture();
    const auto original=runtime::identify_file(path).sha256;
    auto bootstrap=ui::bootstrap_selected_bnw_preview(path,game_root,0);
    check(!bootstrap.preview.blocker && bootstrap.preview.host,
          "Selected Gandhi BNW preview unavailable");
    const auto &host=*bootstrap.preview.host;
    const auto &source=host.session().source();
    const auto &state=host.session().state();
    const auto extras=runtime::restore_bnw_game_plot_extras(
        state,source,host.identity());
    check(extras.current && state.game_plot_extras.current &&
          state.game_plot_extras.yields.size()==extras.yields.size() &&
          state.game_plot_extras.costs.size()==extras.costs.size() &&
          extras.yields.size()==source.snapshot.history.extra_yields.size() &&
          extras.costs.size()==source.snapshot.history.extra_costs.size(),
          "Selected BNW CvGame::Read plot-extra roster changed");
    for(const auto &entry:extras.yields)
        for(std::size_t yield=0;yield<6;++yield) {
            std::int64_t first=0;
            for(const auto &row:source.snapshot.history.extra_yields)
                if(row.plot.x==entry.plot.x && row.plot.y==entry.plot.y) {
                    first=row.yields[yield];break;
                }
            check(extras.extra_yield(entry.plot.x,entry.plot.y,yield)==first,
                  "Selected BNW plot-extra yield differs from source-order getter");
        }
    for(const auto &entry:extras.costs) {
        std::int64_t first=0;
        for(const auto &row:source.snapshot.history.extra_costs)
            if(row.plot.x==entry.plot.x && row.plot.y==entry.plot.y) {
                first=row.cost;break;
            }
        check(extras.extra_cost(entry.plot.x,entry.plot.y)==first,
              "Selected BNW plot-extra cost differs from source-order getter");
    }
    check(!host.session().readiness().can_continue(),
          "BNW plot-extra getter incorrectly enabled full turn");
    {
        auto bad=state;
        if(bad.world.history.extra_yields.empty())
            bad.world.history.extra_yields.push_back({{2,3},{1,2,3,4,5,6}});
        else ++bad.world.history.extra_yields.front().yields.front();
        rejects([&] {(void)runtime::restore_bnw_game_plot_extras(
            bad,source,host.identity());});
        bad.world.history.extra_yields=state.world.history.extra_yields;
        if(bad.world.history.extra_costs.empty())
            bad.world.history.extra_costs.push_back({{2,3},7});
        else ++bad.world.history.extra_costs.front().cost;
        rejects([&] {(void)runtime::restore_bnw_game_plot_extras(
            bad,source,host.identity());});
        bad.world.history.extra_costs=state.world.history.extra_costs;
        bad.world.history.start_offset++;
        rejects([&] {(void)runtime::restore_bnw_game_plot_extras(
            bad,source,host.identity());});
    }
    check(runtime::identify_file(path).sha256==original,
          "BNW plot-extra fixture changed original .Civ5Save");
    std::cout << "PASS BNW CvGame plot extras: " << extras.yields.size()
              << " yield rows, " << extras.costs.size() << " cost rows\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi BNW save and installed game root");
        selected(argv[1],argv[2]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW CvGame plot-extra Read: " << error.what() << '\n';
        return 1;
    }
}
