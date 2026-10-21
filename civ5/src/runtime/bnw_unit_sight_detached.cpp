#include "civ5/runtime/bnw_unit_sight_detached.hpp"
#include "civ5/runtime/bnw_live_plot_visibility.hpp"
#include <algorithm>
#include <climits>
#include <map>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
saves::PlotArrayObservation &array(
    std::vector<saves::PlotArrayObservation> &arrays,
    std::string_view wanted,std::size_t width) {
    saves::PlotArrayObservation *found=nullptr;
    for(auto &item:arrays)
        if(name(item.name)==wanted) {
            require(!found,"Duplicate detached BNW sight array");found=&item;
        }
    require(found && found->columns==1 && found->values.size()==width,
            "Detached BNW sight array shape changed");
    return *found;
}
std::int64_t field(const std::vector<saves::NamedSaveInteger> &fields,
                   std::string_view wanted) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &item:fields)
        if(name(item.name)==wanted) {
            require(!found,"Duplicate detached BNW sight field");found=&item;
        }
    require(found,"Detached BNW sight field is absent");return found->value;
}
saves::MapRegionRecord &area(saves::SavedSessionSnapshot &world,
                             std::int64_t id) {
    require(static_cast<bool>(world.map_regions),
            "Detached BNW sight area state is absent");
    saves::MapRegionContainer *areas=nullptr;
    for(auto &container:world.map_regions->containers)
        if(name(container.name)=="areas") {
            require(!areas,"Duplicate detached BNW area container");
            areas=&container;
        }
    require(areas,"Detached BNW area container is absent");
    saves::MapRegionRecord *found=nullptr;
    for(auto &record:areas->records)
        if(field(record.fields,"id")==id) {
            require(!found,"Duplicate detached BNW area ID");found=&record;
        }
    require(found,"Detached BNW area ID is absent");return *found;
}
void counter(LiveSessionState &copy,const BnwSightPlotDelta &delta,
             std::int64_t team,bool old_phase) {
    const auto next=old_phase?delta.after_old:delta.after_new;
    const auto invisible_next=old_phase?delta.invisible_after_old:
                                         delta.invisible_after_new;
    const auto expected=old_phase?delta.before:delta.after_old;
    const auto invisible_expected=old_phase?delta.invisible_before:
                                             delta.invisible_after_old;
    require(next>=0 && next<=INT16_MAX && invisible_next>=0 &&
            invisible_next<=INT16_MAX,
            "Detached BNW sight target exceeds SDK short counters");
    auto &plot=copy.world.map_plots->plots.at(delta.plot);
    auto &visibility=array(plot.arrays,"visibility",80).values;
    auto &invisible=array(plot.arrays,"invisible_visibility",80).values;
    auto &cache=copy.plot_visibility;
    const auto slot=static_cast<std::size_t>(delta.plot*cache.wire_slots+team);
    require(cache.current && cache.wire_slots==80 && cache.team_slots==64 &&
            slot<cache.counts.size() && slot<cache.source_raw.size() &&
            cache.counts[slot]==expected &&
            visibility[static_cast<std::size_t>(team)]==cache.source_raw[slot] &&
            invisible[static_cast<std::size_t>(team)]==invisible_expected,
            "Detached BNW sight source/cache counters diverged");
    // CvPlot::read has already clamped negative source visibility in cache.
    // From this point the detached live value becomes the authoritative raw
    // value for its own copy; the immutable session source is never written.
    visibility[static_cast<std::size_t>(team)]=next;
    cache.source_raw[slot]=next;
    cache.counts[slot]=next;
    invisible[static_cast<std::size_t>(team)]=invisible_next;
}
void reveal(LiveSessionState &copy,const BnwSightRevealEdge &edge,
            std::int64_t team,std::map<std::int64_t,std::int64_t> &area_deltas) {
    auto &plot=copy.world.map_plots->plots.at(edge.plot);
    auto &cell=copy.world.map_projection->cells.at(edge.plot);
    require(plot.index==edge.plot && cell.index==edge.plot &&
            cell.revealed_teams,
            "Detached BNW sight plot reveal views diverged");
    auto &words=array(plot.arrays,"revealed_words",4).values;
    const auto word=static_cast<std::size_t>(team/32);
    require(words[word]>=0 && words[word]<=UINT32_MAX,
            "Detached BNW sight reveal word exceeds u32");
    const auto bit=std::uint32_t{1}<<(team%32);
    const auto current=(static_cast<std::uint32_t>(words[word])&bit)!=0;
    require(current==cell.revealed_teams->contains(team) &&
            current!=edge.first_reveal,
            "Detached BNW sight reveal precondition changed");
    if(!edge.first_reveal) return;
    words[word]=static_cast<std::uint32_t>(words[word])|bit;
    cell.revealed_teams->insert(team);
    if(edge.area>=0) {
        auto &record=area(copy.world,edge.area);
        auto &counts=array(record.arrays,"revealed_tiles",80).values;
        const auto slot=static_cast<std::size_t>(team);
        const auto already=area_deltas[edge.area];
        require(edge.area_revealed_before>=0 &&
                counts[slot]==edge.area_revealed_before+already &&
                counts[slot]<INT32_MAX,
                "Detached BNW area reveal counter changed or overflowed");
        ++counts[slot];++area_deltas[edge.area];
    } else require(edge.area==-1,
                   "Detached BNW first reveal has invalid area ID");
}
}

BnwDetachedSightResult apply_bnw_unit_sight_detached(
    const LiveSessionState &state,const BnwSetXYRequest &request,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity) {
    BnwDetachedSightResult result;
    try {
        result.plan=plan_bnw_unit_sight_reveal(
            state,request,database,types,defines,identity);
        require(result.plan.calculated,
                "Detached BNW sight needs a complete read-only reveal plan");
        const auto team=result.plan.team;
        require(team>=0 && team<64,
                "Detached BNW sight team exceeds SDK range");
        LiveSessionState copy=state;
        // Expansion2 CvUnit.cpp:13129 removes sight from the old center;
        // CvUnit.cpp:13188 adds sight from the new center. Grouping by plot
        // does not alter independent counter results or area totals.
        for(const auto &delta:result.plan.counters.plots)
            if(delta.old_calls) counter(copy,delta,team,true);
        for(const auto &delta:result.plan.counters.plots)
            if(delta.new_calls) counter(copy,delta,team,false);
        std::map<std::int64_t,std::int64_t> area_deltas;
        for(const auto &edge:result.plan.newly_visible)
            reveal(copy,edge,team,area_deltas);
        (void)bnw_live_plot_visibility_cache(copy);
        result.detached=std::move(copy);
        result.calculated=true;
        result.blockers=result.plan.blockers;
        result.reason="Partial detached SDK sight state; mandatory reveal/meet/AI/Lua/UI and persistence remain";
    } catch(const std::exception &error) {
        result.calculated=false;
        result.detached.reset();
        result.blockers={BnwSightBlocker::unsupported_candidate};
        result.reason=error.what();
    }
    return result;
}
} // namespace civ5::runtime
