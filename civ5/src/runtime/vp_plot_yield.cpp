#include "civ5/runtime/vp_plot_yield.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
std::int64_t required(const std::optional<std::int64_t> &v){if(!v)throw std::runtime_error("Unresolved full plot yield input");return *v;}
auto city_by_ref(auto &world,const saves::SavedPlotCityReference &ref)->decltype(&world.players.begin()->second.cities.begin()->second){
    const auto p=world.players.find(ref.owner);if(p==world.players.end())return nullptr;
    const auto c=p->second.cities.find(ref.id);return c==p->second.cities.end()?nullptr:&c->second;
}
void validate(const VpPlotYieldContext &c,const CityYieldState *city){
    const auto &n=c.nature;const auto &p=c.player;const auto &w=p.improvements.water;
    if(&n.water!=&w||&n.movement!=&p.movement||&n.rules!=&p.nature_rules||&n.state!=&p.nature_state||
       &n.cities!=&w.cities||n.active_team!=p.active_team||w.cities.profile!=content::TechnologyProfile::vp542||
       w.state.layout!=saves::GameStateLayout::vp542_corpus||!w.options||!w.options->all_succeeded())
        throw std::runtime_error("Full plot yield requires one shared VP world");
    if(city&&&w.cities.players.at(city->owner).cities.at(city->id)!=city)throw std::runtime_error("Full plot yield requires live city");
}
const saves::SavedMapCellProjection &cell(const VpPlotYieldContext &c,std::int64_t plot){
    const auto &map=c.nature.water.map;if(plot<0||map.cells.size()!=map.grid.size())throw std::out_of_range("Invalid full plot yield map/index");
    const auto &p=map.cells.at(static_cast<std::size_t>(plot));const auto xy=map.grid.coordinates(static_cast<std::uint64_t>(plot));
    if(p.index!=static_cast<std::uint64_t>(plot)||p.x!=xy[0]||p.y!=xy[1])throw std::runtime_error("Full plot yield identity mismatch");return p;
}
}
CityYieldState *vp_plot_effective_city(CityYieldContext &world,const saves::SavedMapCellProjection &p){
    if(!p.yield_city_override||!p.yield_city)throw std::runtime_error("Unresolved effective owning city");
    const auto &r=*p.yield_city_override;return city_by_ref(world,r.owner==-1&&r.id==-1?*p.yield_city:r);
}
const CityYieldReligionBeliefs *vp_plot_permanent_pantheon(const VpPlotYieldContext &c,const CityYieldState &city,VpPlotReligionSelection selected){
    validate(c,&city);return vp_permanent_pantheon(c.nature.water,city,selected).religion;
}

VpPlotYieldQuote vp_plot_yield_fast(VpPlotYieldContext &c,std::int64_t plot,std::int64_t yield,bool display,
    CityYieldState *city,VpPlotReligionSelection religion,const CityYieldReligionBeliefs *pantheon,bool assume_no_improvement){
    validate(c,city);const auto &p=cell(c,plot);auto &w=c.nature.water;auto &ic=c.player.improvements;VpPlotYieldQuote q;q.permanent_pantheon=pantheon!=nullptr;
    if(required(p.terrain_type)==-1){q.early=1;return q;}
    if(w.options->enabled("CORE_NO_YIELD_ICE")&&required(p.feature_type)==c.ice){q.early=2;return q;}
    if(!p.potential_city_work)throw std::runtime_error("Unresolved potential city work");
    if(!*p.potential_city_work){q.early=3;return q;}
    if(display&&ic.resources.debug_mode){if(!p.cached_yields)throw std::runtime_error("Unresolved plot yield cache");q.early=4;q.total=p.cached_yields->at(yield);return q;}
    if(display){const auto *profile=saves::find_vp_plot_projection(p);if(!profile||!profile->revealed_types)throw std::runtime_error("Unresolved revealed plot types");const auto &known=profile->revealed_types->at(c.player.active_team);
        q.player=known.owner;q.improvement=known.improvement;q.route=known.route;if(q.player==-1)q.player=c.active_player;
    }else{q.player=required(p.owner);q.improvement=required(p.improvement_type);if(!p.route)throw std::runtime_error("Unresolved plot route");q.route=p.route->type;
        if(q.player==-1&&city)q.player=city->owner;}
    if(!p.improvement_pillaged||!p.route)throw std::runtime_error("Unresolved plot pillage state");
    if(*p.improvement_pillaged||assume_no_improvement)q.improvement=-1;if(p.route->pillaged)q.route=-1;
    q.feature=required(p.feature_type);q.resource=plot_resource_type(ic.resources,plot,display?c.player.active_team:q.player==-1?-1:w.players.players.at(q.player).team);
    VpPlotReligionContext rel{c.player.religion_rules,w};
    q.nature=vp_nature_yield(c.nature,plot,yield,q.player,q.feature,q.resource,q.improvement,city,display).total;
    q.religion_improvement=vp_religion_improvement_yield(rel,yield,q.player,q.improvement,q.resource,city,religion);
    q.religion_nature=vp_religion_nature_yield(rel,plot,yield,q.player,q.improvement,q.feature,q.resource,city,religion).total();
    q.improvement_yield=vp_improvement_yield(ic,plot,q.improvement,yield,q.player,q.route,q.feature,q.resource,city).total();
    q.total=q.nature;for(const auto n:{q.religion_improvement,q.religion_nature,q.improvement_yield})q.total=wide::add(q.total,n);
    q.player_yield=vp_player_plot_yield(c.player,plot,yield,q.total,q.player,q.improvement,q.feature,q.resource,city,religion,pantheon,display);q.total=wide::add(q.total,q.player_yield);
    if(w.options->enabled("BALANCE_PERMANENT_PANTHEONS")&&pantheon){
        q.pantheon_improvement=vp_religion_improvement_yield(rel,yield,q.player,q.improvement,q.resource,city,{pantheon,-1});
        q.pantheon_nature=vp_religion_nature_yield(rel,plot,yield,q.player,q.improvement,q.feature,q.resource,city,{pantheon,-1}).total();
        q.total=wide::add(q.total,q.pantheon_improvement);q.total=wide::add(q.total,q.pantheon_nature);}
    const auto *profile=saves::find_vp_plot_projection(p);
    if(!profile)throw std::runtime_error("Unresolved plot extra-yield cache");
    for(const auto &[id,n]:profile->extra_yields)if(id==yield){q.extra=n;break;}
    q.total=std::max<std::int64_t>(0,wide::add(q.total,q.extra));return q;
}
VpPlotYieldQuote vp_plot_yield(VpPlotYieldContext &c,std::int64_t plot,std::int64_t yield,bool display,CityYieldState *city,bool assume_no_improvement){
    validate(c,city);const auto &p=cell(c,plot);if(!city)city=vp_plot_effective_city(c.nature.cities,p);
    const auto selection=city?vp_plot_religion_selection(c.nature.cities,c.player.religion_rules,*city):VpPlotReligionSelection{};
    const auto *pantheon=city?vp_plot_permanent_pantheon(c,*city,selection):nullptr;
    // Original wrapper omits the last argument on the permanent-pantheon path.
    return vp_plot_yield_fast(c,plot,yield,display,city,selection,pantheon,pantheon?false:assume_no_improvement);
}
} // namespace civ5::runtime
