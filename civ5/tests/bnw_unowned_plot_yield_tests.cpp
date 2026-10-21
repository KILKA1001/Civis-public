#include "civ5/runtime/bnw_unowned_plot_yield.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>

namespace {
using namespace civ5;
void check(bool condition,const char *why) {
    if(!condition) throw std::runtime_error(why);
}
template<class F> void rejects(F &&operation) {
    bool blocked=false;
    try {operation();} catch(const std::exception &) {blocked=true;}
    check(blocked,"Unsupported BNW full plot-yield branch accepted");
}
void suffix() {
    using runtime::finish_bnw_unowned_plot_yield;
    auto value=finish_bnw_unowned_plot_yield(3,4);
    check(value.nature==3 && value.extra==4 && value.total==7 &&
          !value.early_zero,
          "BNW plot extra was not inserted after nature yield");
    value=finish_bnw_unowned_plot_yield(3,-9);
    check(value.total==0 && value.extra==-9,
          "BNW plot-yield final zero clamp changed");
    rejects([&] {(void)finish_bnw_unowned_plot_yield(INT32_MAX,1);});
    rejects([&] {(void)finish_bnw_unowned_plot_yield(-1,1);});
}
void selected(const std::filesystem::path &path,
              const std::filesystem::path &game_root) {
    suffix();
    const auto original=runtime::identify_file(path).sha256;
    auto bootstrap=ui::bootstrap_selected_bnw_preview(path,game_root,0);
    check(!bootstrap.preview.blocker && bootstrap.preview.host,
          "Selected Gandhi BNW preview unavailable");
    auto &host=*bootstrap.preview.host;
    const auto &source=host.session().source();
    auto live=host.session().state();
    check(live.world.map_projection.has_value(),
          "Selected BNW map projection unavailable");
    const auto types=host.database().cache_info_types();
    check(types.report().all_succeeded(),
          "Selected BNW Info type catalog unavailable");
    const auto rules=runtime::load_bnw_nature_yield_rules(host.database());
    auto nature_state=runtime::restore_bnw_nature_yields(
        live.world,rules,live.cities);
    const auto passability=runtime::bnw_live_plot_impassability(live);
    const auto water_state=runtime::restore_plot_water(live.world);
    const auto water_rules=runtime::load_plot_water_rules(
        host.database(),content::TechnologyProfile::bnw);
    runtime::PlotWaterContext water{*live.world.map_projection,water_state,
        water_rules,types,passability,live.setup,live.cities,host.defines()};
    // NO_TEAM branch does not query resource visibility; retain a profiled
    // empty owner instead of inventing research/policy state for this query.
    runtime::ResourceVisibilityRules resource_rules;
    runtime::ResourceVisibilityState resource_state;
    runtime::ResourceVisibilityContext resources{*live.world.map_projection,
        resource_state,resource_rules,live.setup,live.player_research,
        live.technologies,types,host.defines(),false};
    runtime::BnwNatureYieldContext nature{rules,nature_state,water,resources};
    const auto extras=runtime::restore_bnw_game_plot_extras(
        live,source,host.identity());
    std::size_t eligible=0,first_eligible=live.world.map_projection->cells.size();
    std::size_t unowned=0,no_improvement=0,no_route=0,workable=0;
    std::size_t hash_zero=0,legacy_minus_one=0,unresolved_improvement=0;
    std::string first_unowned;
    std::string first_error;
    for(std::size_t i=0;i<live.world.map_projection->cells.size();++i) {
        const auto &cell=live.world.map_projection->cells[i];
        if(cell.owner!=-1) continue;
        ++unowned;
        if(first_unowned.empty())
            first_unowned="plot="+std::to_string(i)+" numeric="+
                (cell.improvement_type?std::to_string(*cell.improvement_type):"absent")+
                " hash="+(cell.improvement_type_hash?
                    std::to_string(*cell.improvement_type_hash):"absent");
        const bool legacy=cell.improvement_type &&
            *cell.improvement_type==-1 && !cell.improvement_type_hash;
        const bool hashed=!cell.improvement_type &&
            cell.improvement_type_hash && *cell.improvement_type_hash==0;
        if(legacy) ++legacy_minus_one;
        else if(hashed) ++hash_zero;
        else ++unresolved_improvement;
        if(!legacy && !hashed) continue;
        ++no_improvement;
        if(!cell.route || cell.route->type!=-1) continue;
        ++no_route;
        if(!cell.potential_city_work || !*cell.potential_city_work ||
           !cell.terrain_type || *cell.terrain_type==-1) continue;
        ++workable;
        std::optional<runtime::BnwUnownedPlotYieldQuote> quote,expected;
        try {
            quote=runtime::bnw_unowned_plot_yield(
                nature,live,source,host.identity(),i,0);
            expected=runtime::finish_bnw_unowned_plot_yield(
                runtime::bnw_nature_yield(nature,i,0,-1,false).total,
                extras.extra_yield(cell.x,cell.y,0));
        } catch(const std::exception &error) {
            if(first_error.empty()) first_error=error.what();
            continue;
        }
        check(quote->nature==expected->nature &&
              quote->extra==expected->extra &&
              quote->total==expected->total && !quote->early_zero,
              "Selected BNW simple plot yield differs from SDK suffix");
        if(!eligible) first_eligible=i;
        ++eligible;
        if(eligible==8) break;
    }
    if(!eligible)
        throw std::runtime_error("No selected BNW plot reached the verified branch: unowned="+
            std::to_string(unowned)+" no_improvement="+std::to_string(no_improvement)+
            " no_route="+std::to_string(no_route)+" workable="+
            std::to_string(workable)+" hash_zero="+std::to_string(hash_zero)+
            " legacy_minus_one="+std::to_string(legacy_minus_one)+
            " unresolved="+std::to_string(unresolved_improvement)+
            " first="+first_unowned+" query="+first_error);
    {
        auto &cell=live.world.map_projection->cells[first_eligible];
        const auto old=cell.owner;
        cell.owner=0;
        rejects([&] {(void)runtime::bnw_unowned_plot_yield(
            nature,live,source,host.identity(),first_eligible,0);});
        cell.owner=old;
        rejects([&] {(void)runtime::bnw_unowned_plot_yield(
            nature,live,source,host.identity(),-1,0);});
        check(!runtime::bnw_plot_early_zero_yield(live,first_eligible,0),
              "Normal selected plot cannot take SDK early-zero branch");
        const auto terrain=cell.terrain_type;
        cell.terrain_type=-1;
        const auto no_terrain=runtime::bnw_plot_early_zero_yield(
            live,first_eligible,0);
        check(no_terrain && no_terrain->early_zero &&
              runtime::bnw_unowned_plot_yield(nature,live,source,
                  host.identity(),first_eligible,0).early_zero,
              "NO_TERRAIN must return before owner and improvement checks");
        auto foreign_identity=host.identity();
        foreign_identity.payload_sha256="different-save";
        rejects([&] {(void)runtime::bnw_unowned_plot_yield(
            nature,live,source,foreign_identity,first_eligible,0);});
        cell.terrain_type=terrain;
        const auto workable=cell.potential_city_work;
        cell.potential_city_work=false;
        const auto no_work=runtime::bnw_plot_early_zero_yield(
            live,first_eligible,0);
        check(no_work && no_work->early_zero,
              "Non-workable owned plot must return before owner effects");
        cell.potential_city_work.reset();
        rejects([&] {(void)runtime::bnw_plot_early_zero_yield(
            live,first_eligible,0);});
        cell.potential_city_work=workable;
        rejects([&] {(void)runtime::bnw_plot_early_zero_yield(
            live,first_eligible,6);});
    }
    check(runtime::identify_file(path).sha256==original &&
          !host.session().readiness().can_continue(),
          "BNW plot-yield query altered source or enabled full turn");
    std::cout << "PASS BNW simple unowned plot yield: " << eligible
              << " selected Gandhi cells; hash_zero=" << hash_zero
              << " legacy_minus_one=" << legacy_minus_one
              << " unresolved=" << unresolved_improvement
              << " first=" << first_unowned << '\n';
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi BNW save and installed game root");
        selected(argv[1],argv[2]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW simple unowned plot yield: " << error.what()
                  << '\n';
        return 1;
    }
}
