#include "civ5/runtime/vp_player_plot_yield.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include "civ5/runtime/vp_unit_production_profile.hpp"
#include "civ5/runtime/vp_player_accomplishments_profile.hpp"
#include "civ5/runtime/vp_religion_beliefs_profile.hpp"
#include "civ5/runtime/vp_improvement_yield_profile.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
constexpr auto layout=saves::GameStateLayout::vp542_corpus;
using Matrix=ImprovementYieldMatrix;
using V=VpPlayerPlotVector;
using M=VpPlayerPlotMatrix;
std::int64_t i64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("Player plot ID exceeds int64");return static_cast<std::int64_t>(n);}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){const T *found=nullptr;
    for(const auto &x:v)if(x.name&&std::string_view(x.name)==name){if(found)throw std::runtime_error("Duplicate player plot field");found=&x;}
    if(!found)throw std::runtime_error("Missing player plot field: "+std::string(name));return *found;}
std::int64_t scalar(const auto &v,const char *name){const auto &a=named(v,name).values;if(a.size()!=1)throw std::runtime_error("Invalid player plot scalar");return a[0];}
std::int64_t required(const std::optional<std::int64_t> &n){if(!n)throw std::runtime_error("Unresolved player plot input");return *n;}
std::int64_t value(const YieldValues &v,std::int64_t id){const auto it=v.find(id);return it==v.end()?0:it->second;}
std::int64_t value(const Matrix &m,std::int64_t key,std::int64_t y){const auto it=m.find(key);return it==m.end()?0:value(it->second,y);}
void assign(YieldValues &v,std::int64_t id,std::int64_t n){if(id<0)throw std::runtime_error("Negative player plot relation ID");if(n)v[id]=n;else v.erase(id);}
std::int64_t city_value(const YieldModifierRows &rows,std::int64_t key,std::int64_t y){
    const auto it=rows.find(y);if(it==rows.end())return 0;
    for(const auto &[id,n]:it->second)if(id==key)return n;return 0;
}
bool connection(const ImprovementYieldState &s,std::int64_t player,std::int64_t plot){
    if(player==-1)return false;const auto &v=vp_improvement_yield_player(s,player).connections;
    std::size_t first=0,count=v.size();while(count){const auto step=count/2,mid=first+step;
        if(v[mid]<plot){first=mid+1;count-=step+1;}else count=step;}
    return first<v.size()&&!(plot<v[first]);
}
std::int64_t define(const PlotWaterContext &w,const char *name){const auto *d=w.defines.find(name);
    if(!d||!d->lookup.error.empty()||!std::holds_alternative<std::int64_t>(d->lookup.value))throw std::runtime_error("Unresolved player plot define");return std::get<std::int64_t>(d->lookup.value);}
}
VpPlayerPlotRules load_vp_player_plot_rules(const content::RulesDatabase &db,const content::InfoTypeCatalog &types){
    VpPlayerPlotRules r;
    const auto load=[&](const char *table,const auto &consume){const auto q=db.prefetch_info(table,consume);if(!q.all_succeeded())throw std::runtime_error("Player plot Info: "+q.error);return q.slots;};
    const auto span=[&](const char *table){return load(table,[](auto,auto,const auto &){return true;});};
    const auto ref=[&](const auto &text){if(text)if(const auto *t=types.find(*text))return t->id;return std::int64_t{-1};};
    const auto param=[](const auto &row){const auto t=row.text("Type");return t?content::DatabaseValue(std::string(*t)):content::DatabaseValue{};};
    const auto query=[&](const std::string &sql,const auto &p,const auto &consume){const auto q=db.query_rows({sql,{p}},consume);if(!q.all_succeeded())throw std::runtime_error("Player plot relation: "+q.error);};
    r.yield_slots=load("Yields",[&](auto id,auto,const auto &row){r.yields[i64(id)]={row.integer("GoldenAgeYieldThreshold"),row.integer("GoldenAgeYield")};return true;});
    r.plot_slots=span("Plots");r.terrain_slots=span("Terrains");r.feature_slots=span("Features");r.accomplishment_slots=span("Accomplishments");
    r.resource_slots=load("Resources",[&](auto id,auto,const auto &row){auto &a=r.resources[i64(id)];a.usage=row.integer("ResourceUsage");
        query("select Yields.ID, Yield from Resource_YieldChangeFromMonopoly inner join Yields on YieldType = Yields.Type where ResourceType = ?",param(row),[&](const auto &v){assign(a.monopoly,v.integer_at(0),v.integer_at(1));});return true;});
    r.improvement_slots=load("Improvements",[&](auto id,auto,const auto &row){auto &a=r.accomplishments[i64(id)];
        query("select Accomplishments.ID, Yields.ID, Yield from Improvement_AccomplishmentYieldChanges inner join Yields on YieldType = Yields.Type inner join Accomplishments on AccomplishmentType = Accomplishments.Type where ImprovementType = ?",param(row),[&](const auto &v){const auto key=v.integer_at(0);if(key<0)throw std::runtime_error("Negative accomplishment ID");assign(a[key],v.integer_at(1),v.integer_at(2));});return true;});
    std::map<std::int64_t,std::int64_t> free_policy_branches;
    r.branch_slots=load("PolicyBranchTypes",[&](auto id,auto,const auto &row){const auto policy=ref(row.text("FreePolicy"));if(policy!=-1)free_policy_branches.emplace(policy,i64(id));return true;});
    r.policy_slots=load("Policies",[&](auto id,auto,const auto &row){auto &a=r.policies[i64(id)];a.branch=ref(row.text("PolicyBranchType"));
        if(a.branch==-1)if(const auto b=free_policy_branches.find(i64(id));b!=free_policy_branches.end())a.branch=b->second;
        query("select Improvements.ID, CultureChange from Policy_ImprovementCultureChanges inner join Improvements on ImprovementType = Improvements.Type where PolicyType = ?",param(row),[&](const auto &v){assign(a.improvement_culture,v.integer_at(0),v.integer_at(1));});return true;});
    load("Beliefs",[&](auto id,auto,const auto &row){auto &a=r.belief_wonder_modifier[i64(id)];
        query("select Yields.ID, Yield from Belief_YieldModifierNaturalWonder inner join Yields on YieldType = Yields.Type where BeliefType = ?",param(row),[&](const auto &v){assign(a,v.integer_at(0),v.integer_at(1));});return true;});
    return r;
}
VpPlayerPlotState restore_vp_player_plot_yields(const saves::SavedSessionSnapshot &s,const VpPlayerPlotRules &r,CityYieldContext &cities){
    if(!s.all_players||!s.teams||s.all_players->layout!=layout||s.teams->layout!=layout||s.leagues.layout!=layout||cities.profile!=vp||!r.yield_slots||r.yield_slots!=cities.yield_slots)
        throw std::runtime_error("Player plot import profile/width mismatch");
    const auto sparse=[&](const auto &v,std::uint64_t width){if(v.size()!=width)throw std::runtime_error("Player plot vector differs from active span");
        YieldValues a;for(std::size_t i=0;i<v.size();++i)if(v[i])a[i64(i)]=v[i];return a;};
    const auto matrix=[&](const auto &v,std::uint64_t count){if(v.size()%r.yield_slots||v.size()/r.yield_slots!=count)throw std::runtime_error("Player plot matrix differs from active span");
        Matrix a;for(std::size_t i=0;i<v.size();++i)if(v[i])a[i64(i/r.yield_slots)][i64(i%r.yield_slots)]=v[i];return a;};
    VpPlayerPlotState result;
    for(const auto &source:s.teams->teams){const auto id=named(source.fields,"id").value;if(result.teams.contains(id))throw std::runtime_error("Duplicate player plot team");auto &t=result.teams[id];
        const auto &a=named(source.arrays,"terrain_yields"),&b=named(source.arrays,"feature_yields");
        if(a.columns!=r.yield_slots||b.columns!=r.yield_slots)throw std::runtime_error("Player plot team matrix width mismatch");t[0]=matrix(a.values,r.terrain_slots);t[1]=matrix(b.values,r.feature_slots);}
    for(const auto &source:s.all_players->players){const auto id=source.saved_player_id;
        if(!cities.players.contains(id)||result.players.contains(id)||source.core.layout!=layout||source.traits.layout!=layout||source.tail.layout!=layout||source.strategy.layout!=layout||source.cities.layout!=layout)
            throw std::runtime_error("Player plot saved player mismatch");
        auto &p=result.players[id];
        const char *core_vectors[]={"m_aiExtraYieldThreshold","m_aiSeaPlotYield","m_aiLakePlotYield","m_viCityConnectionPlotYield"};
        const char *trait_vectors[]={"m_iMountainRangeYield","m_iYieldChangeNaturalWonder","m_iSeaPlotYieldChanges","m_iLuxuryYieldChanges","m_iYieldChangeStrategicResources"};
        for(std::size_t i=0;i<4;++i)p.yields[i]=sparse(named(source.core.fields,core_vectors[i]).values,r.yield_slots);
        p.yields[4]=sparse(named(source.tail.fields,"m_piYieldChangesNaturalWonder").values,r.yield_slots);
        for(std::size_t i=0;i<5;++i)p.yields[i+5]=sparse(named(source.traits.fields,trait_vectors[i]).values,r.yield_slots);
        const char *tail_matrices[]={"m_ppiPlotYieldChange","m_ppiTerrainYieldChange","m_ppiFeatureYieldChange","m_ppiResourceYieldChange","m_ppiUnimprovedFeatureYieldChange","m_ppiImprovementYieldChange"};
        const std::uint64_t spans[]={r.plot_slots,r.terrain_slots,r.feature_slots,r.resource_slots,r.feature_slots,r.improvement_slots};
        for(std::size_t i=0;i<6;++i)p.matrices[i]=matrix(named(source.tail.fields,tail_matrices[i]).values,spans[i]);
        const char *trait_matrices[]={"m_ppiPlotYieldChange","m_ppiTerrainYieldChange","m_ppiFeatureYieldChange","m_ppiResourceYieldChange","m_ppaaiUnimprovedFeatureYieldChange"};
        for(std::size_t i=0;i<5;++i){const auto &v=named(source.traits.fields,trait_matrices[i]);if(v.columns!=r.yield_slots)throw std::runtime_error("Player plot trait matrix width mismatch");p.matrices[i+6]=matrix(v.values,spans[i]);}
        const char *nested[]={"m_miResourceYieldChangesFromGoldenAge","m_miResourceYieldChangesFromGoldenAgeCap"};
        for(std::size_t i=0;i<2;++i)for(const auto &row:named(source.traits.fields,nested[i]).rows){YieldValues v;
            if(row.values.size()%2)throw std::runtime_error("Invalid nested player plot map");
            for(std::size_t j=0;j<row.values.size();j+=2)v.emplace(row.values[j],row.values[j+1]);p.matrices[11+i].emplace(row.key,std::move(v));}
        ensure_vp_player_accomplishments_profile(cities).players[id]=
            sparse(named(source.core.fields,"m_aiAccomplishments").values,r.accomplishment_slots);
        p.golden_ages=scalar(source.core.fields,"m_iNumGoldenAges");p.monopoly_flat=scalar(source.core.fields,"m_iMonopolyModFlat");
        p.wonder_modifier=scalar(source.traits.fields,"m_iNaturalWonderYieldModifier");p.wltkd_gp_modifier=scalar(source.traits.fields,"m_iWLTKDGPImprovementModifier");
        ensure_vp_city_trait_yield_profile(cities).eras[id]={scalar(source.traits.fields,"m_iFractionalEraScaler"),scalar(source.traits.fields,"m_bIsEraScaling")!=0,scalar(source.traits.fields,"m_bIsOddEraScaler")!=0};
        p.trade_route_only=scalar(source.traits.fields,"m_bTradeRouteOnly")!=0;
        const auto &policies=named(source.strategy.blocks,"policies");const auto &modifiers=named(policies.arrays,"policy_modifiers").values;
        // Selected source layout; the host stores the resolved value, not a fixed policy array.
        if(modifiers.size()!=60)throw std::runtime_error("Unverified VP policy modifier wire layout");p.policy_culture=modifiers.at(11);
        const auto &blocked=named(policies.arrays,"branch_blocked");if(blocked.columns!=1||blocked.values.size()!=r.branch_slots)throw std::runtime_error("Policy branch span mismatch");
        for(std::size_t i=0;i<blocked.values.size();++i)if(blocked.values[i])p.blocked_branches.insert(i64(i));
        std::set<std::int64_t> seen;
        for(const auto &sc:source.cities.cities){const auto cid=scalar(sc.core.fields,"m_iID");auto &city=cities.players.at(id).cities.at(cid);
            if(!seen.insert(cid).second||sc.core.layout!=layout||sc.tail.layout!=layout||city.id!=cid||city.owner!=id||scalar(sc.core.fields,"m_eOwner")!=id||city.source_core!=std::optional{std::pair{sc.core.start_offset,sc.core.end_offset}})
                throw std::runtime_error("Player plot city binding mismatch");
            VpCityPlotBonusState a;
            const char *vectors[]={"m_aiSeaPlotYield","m_aiRiverPlotYield","m_aiLakePlotYield","m_aiSeaResourceYield","m_viCityConnectionPlotYield","m_aiLuxuryExtraYield"};
            YieldValues *destinations[]={&a.sea,&a.river,&a.lake,&a.sea_resource,&a.connection,&a.luxury};
            for(std::size_t i=0;i<6;++i)*destinations[i]=sparse(named(sc.core.fields,vectors[i]).values,r.yield_slots);
            const auto &tail=named(sc.tail.blocks,"remainder");
            const auto rows=[&](const char *name,const auto &fields,const auto &outputs){const auto &v=named(tail.children,name).children;
                if(v.size()!=r.yield_slots)throw std::runtime_error("City plot yield records differ from active span");
                for(std::size_t y=0;y<v.size();++y)for(std::size_t k=0;k<fields.size();++k){const auto &a=named(v[y].children,fields[k]).values;
                    if(a.size()%2)throw std::runtime_error("Invalid city plot modifier pairs");auto &out=(*outputs[k])[i64(y)];for(std::size_t j=0;j<a.size();j+=2)out.emplace_back(a[j],a[j+1]);}};
            rows("m_yieldChanges",std::array{"forTerrain","forFeature","forImprovement","forResource","forPlot"},std::array{&a.terrain,&a.feature,&a.improvement,&a.resource,&a.plot});
            rows("m_eventYields",std::array{"forTerrain","forFeature","forImprovement","forResource"},std::array{&a.event_terrain,&a.event_feature,&a.event_improvement,&a.event_resource});ensure_vp_city_plot_profile_city(cities,id,cid).bonuses=std::move(a);
        }
        if(seen.size()!=cities.players.at(id).cities.size())throw std::runtime_error("Incomplete player plot cities");
    }
    if(result.players.size()!=cities.players.size())throw std::runtime_error("Incomplete player plot roster");
    cities.leagues.clear();auto &unit_profile=ensure_vp_unit_production_profile(cities);
    unit_profile.league_space_ship_production.clear();
    for(const auto &source:s.leagues.leagues){CityYieldContext::League a;for(const auto &m:source.members)a.members.push_back(m.player);
        auto &spaceship=unit_profile.league_space_ship_production.emplace_back();
        for(const auto &resolution:source.active_resolutions){a.culture_per_natural_wonder.push_back(named(resolution.effects.integers,"culture_per_natural_wonder").value);a.culture_per_wonder.push_back(named(resolution.effects.integers,"culture_per_wonder").value);
            spaceship.push_back(named(resolution.effects.integers,"limit_spaceship_production").value);}cities.leagues.push_back(std::move(a));}
    return result;
}
std::int64_t vp_trait_era_modifier(const VpCityTraitYieldProfileState::TraitEraState &p,std::int64_t era){
    std::int64_t n=100;if(!p.era_scaling)return n;
    if(p.odd_era&&era>1)n=wide::add(n,wide::multiply_divide(100,era/2,1));
    if(p.fractional_era>0)n=wide::add(n,wide::multiply_divide(100,era,p.fractional_era));
    if(n==100&&era>1)n=wide::add(n,wide::multiply_divide(100,wide::subtract(era,1),1));return n;
}
std::int64_t vp_player_plot_yield(VpPlayerPlotContext &c,std::int64_t plot,std::int64_t yield,std::int64_t current,std::int64_t player,
    std::int64_t improvement,std::int64_t feature,std::int64_t resource,const CityYieldState *city,VpPlotReligionSelection religion,
    const CityYieldReligionBeliefs *pantheon,bool display,std::optional<std::int64_t> forced){
    auto &ic=c.improvements;auto &w=ic.water;
    if(ic.rules.profile!=vp||ic.state.profile!=vp||w.cities.profile!=vp||w.players.profile!=vp||w.rules.profile!=vp||w.state.layout!=layout||!w.options||!w.options->all_succeeded()||
       &c.movement.map!=&w.map||&c.movement.players!=&w.players||&c.movement.passability!=&w.passability||&ic.resources.map!=&w.map||&ic.resources.players!=&w.players)
        throw std::runtime_error("Player plot yield requires one shared VP world");
    if(city&&&w.cities.players.at(city->owner).cities.at(city->id)!=city)throw std::runtime_error("Player plot yield requires live city");
    if(player==-1)return 0;if(plot<0||w.map.cells.size()!=w.map.grid.size())throw std::out_of_range("Invalid player plot map/index");const auto &cell=w.map.cells.at(static_cast<std::size_t>(plot));const auto &p=c.state.players.at(player);
    const auto xy=w.map.grid.coordinates(static_cast<std::uint64_t>(plot));if(cell.index!=static_cast<std::uint64_t>(plot)||cell.x!=xy[0]||cell.y!=xy[1])throw std::runtime_error("Player plot identity mismatch");
    const auto &live=w.players.players.at(player);const auto team=live.team;const auto &t=c.state.teams.at(team);const auto &yr=c.rules.yields.at(yield);
    const auto terrain=required(cell.terrain_type),type=required(cell.plot_type);const auto era=ic.team_eras.at(team);
    const auto v=[&](V kind){return value(p.yields[static_cast<std::size_t>(kind)],yield);};
    const auto m=[&](M kind,std::int64_t key){return value(p.matrices[static_cast<std::size_t>(kind)],key,yield);};
    std::int64_t n=0;const auto add=[&](std::int64_t amount){n=wide::add(n,amount);};
    const auto f=c.nature_rules.features.find(required(cell.feature_type));const bool wonder=f!=c.nature_rules.features.end()&&(f->second.natural_wonder||f->second.pseudo_natural_wonder);
    add(m(M::trait_plot,type));add(m(M::plot,type));
    const bool mountain=type==0||terrain==c.mountain_terrain;
    if(mountain&&!wonder)add(wide::multiply_divide(v(V::trait_mountain),std::max<std::int64_t>(1,era),1));
    else if(wonder){add(v(V::trait_wonder));add(v(V::wonder));
        if(const auto fi=c.nature_rules.features.find(feature);feature!=-1&&fi!=c.nature_rules.features.end()){
            auto modifier=p.wonder_modifier;VpPlotReligionContext rel{c.religion_rules,w};
            const auto belief_modifier=[&](const CityYieldReligionBeliefs &r){const auto &profile=vp_religion_beliefs(r);std::int64_t a=0;
                for(const auto id:r.beliefs){const auto b=value(c.rules.belief_wonder_modifier,id,yield);if(b&&vp_plot_belief_valid(rel,id,profile.belief_religion,player,city))a=wide::add(a,b);}return a;};
            if(religion.majority){modifier=wide::add(modifier,belief_modifier(*religion.majority));if(religion.secondary!=-1)modifier=wide::add(modifier,value(c.rules.belief_wonder_modifier,religion.secondary,yield));}
            // The source adds a permanent pantheon modifier only in the mountain branch.
            if(mountain&&w.options->enabled("BALANCE_PERMANENT_PANTHEONS")&&pantheon)modifier=wide::add(modifier,belief_modifier(*pantheon));
            if(modifier)add(wide::multiply_divide(value(fi->second.yields[0],yield),modifier,100));
        }
    }
    if(improvement!=-1){add(value(ic.state.players.at(player).own,improvement,yield));const auto &it=ic.state.teams.at(team);add(value(it[0],improvement,yield));
        add(value(it[plot_is_fresh_water(w,plot)?2:1],improvement,yield));
        const auto &accomplishments=vp_player_accomplishments(w.cities,player);
        for(const auto &[id,count]:accomplishments)if(count)add(wide::multiply_divide(value(c.rules.accomplishments.at(improvement),id,yield),count,1));}
    bool connected=connection(ic.state,player,plot);if(forced&&*forced!=ic.automatic_route)connected=*forced!=-1;
    auto terrain_bonus=m(M::trait_terrain,terrain);if(type==1)terrain_bonus=wide::add(terrain_bonus,m(M::trait_terrain,c.hill_terrain));
    const auto &era_state=vp_city_trait_era(w.cities,player);
    const auto improvement_bonus=improvement==-1?0:value(ic.state.players.at(player).traits,improvement,yield);const auto scale=vp_trait_era_modifier(era_state,era);
    MapGeometry geometry{w.map.grid,w.map.wrap_x,w.map.wrap_y,layout};
    const auto neighbors=[&](const auto &fn){for(int d=0;d<6;++d)if(const auto id=geometry.direction(i64(cell.x),i64(cell.y),d))fn(w.map.cells.at(static_cast<std::size_t>(*id)));};
    if(p.trade_route_only){if(required(cell.owner)==player){const bool on_route=required(cell.trade_route_units)>0;bool adjacent=false;
            neighbors([&](const auto &q){if(required(q.trade_route_units)>0)adjacent=true;});
            if(terrain_bonus>0&&(connected||on_route))add(wide::multiply_divide(scale,terrain_bonus,100));
            if(improvement_bonus>0&&(connected||on_route||adjacent))add(wide::multiply_divide(scale,improvement_bonus,100));}}
    else{add(wide::multiply_divide(scale,terrain_bonus,100));add(wide::multiply_divide(scale,improvement_bonus,100));}
    add(m(M::terrain,terrain));if(v(V::threshold)>0&&n>=v(V::threshold))add(define(w,"EXTRA_YIELD"));
    // IsNaturalWonder() defaults to including pseudo wonders in this VP profile.
    auto bonus=feature==-1?value(t[0],terrain,yield):value(t[1],feature,yield);if(wonder&&p.wonder_modifier>0)bonus=wide::multiply_divide(bonus,wide::add(100,p.wonder_modifier),100);add(bonus);
    if(city){const auto &bonus=vp_city_plot_profile_city(w.cities,city->owner,city->id).bonuses;if(!bonus)throw std::runtime_error("Unresolved city plot bonuses");const auto &a=*bonus;
        const auto cv=[&](const auto &v){return value(v,yield);};const auto cm=[&](const auto &rows,std::int64_t key){return city_value(rows,key,yield);};
        const auto revealed=[&]{if(!display)return true;const auto id=geometry.plot(city->x,city->y);if(!id)throw std::runtime_error("Invalid live city plot");const auto &q=w.map.cells.at(static_cast<std::size_t>(*id));
            if(!q.revealed_teams)throw std::runtime_error("Unresolved city revelation");return q.revealed_teams->contains(c.active_team);};
        if(plot_is_valid_move(c.movement,plot,player)){
            if(type==3){const bool lake=plot_is_lake(w,plot);if(lake)add(v(V::lake));else{add(v(V::sea));add(v(V::trait_sea));}
                if(revealed())add(lake?std::max<std::int64_t>(0,cv(a.lake)):cv(a.sea));if(resource!=-1&&revealed())add(cv(a.sea_resource));}
            else{if(!cell.rivers)throw std::runtime_error("Unresolved player plot river");if(cell.rivers->is_river&&revealed())add(cv(a.river));}
            if(terrain!=-1)add(cm(a.terrain,terrain));
        }
        // This query defaults to the actual plot owner and ignores the forced connection.
        if(connection(ic.state,required(cell.owner),plot)){add(v(V::connection));add(cv(a.connection));}
        if(improvement!=-1){if(c.religion_rules.improvements.at(improvement).great_person&&required(city->we_love_king_turns)>0&&p.wltkd_gp_modifier>0)n=wide::multiply_divide(n,wide::add(100,p.wltkd_gp_modifier),100);
            add(cm(a.improvement,improvement));add(m(M::extra_improvement,improvement));}
        if(feature!=-1)add(cm(a.feature,feature));add(cm(a.plot,type));
        if(const auto ri=c.rules.resources.find(resource);resource!=-1&&ri!=c.rules.resources.end()){
            if(resource_revealed_by_team(ic.resources,team,resource)){add(cm(a.resource,resource));if(ri->second.usage==2){add(cv(a.luxury));add(v(V::trait_luxury));}
                if(ri->second.usage==1)add(v(V::trait_strategic));}
            add(cm(a.event_resource,resource));add(m(M::resource,resource));add(m(M::trait_resource,resource));
            const auto ga=wide::multiply_divide(m(M::trait_resource_ga,resource),p.golden_ages,1);if(ga>0)add(std::min(ga,m(M::trait_resource_ga_cap,resource)));
            if(w.options->enabled("BALANCE_RESOURCE_MONOPOLIES")&&c.nature_state.players.at(player).has_global_monopoly.contains(resource)){
                const auto monopoly=value(ri->second.monopoly,yield);if(monopoly>0)add(wide::add(monopoly,p.monopoly_flat));}
        }
        if(feature!=-1)add(cm(a.event_feature,feature));
        if(terrain!=-1){add(cm(a.event_terrain,terrain));neighbors([&](const auto &q){const auto id=required(q.improvement_type);
            if(id!=-1&&required(q.owner)==player)if(const auto info=ic.rules.improvements.find(id);info!=ic.rules.improvements.end()){
                const auto bonus=value(info->second.adjacent_terrains,terrain,yield);if(bonus>0)add(bonus);}});}
        if(improvement!=-1)add(cm(a.event_improvement,improvement));
    }
    if(feature!=-1){if(improvement==-1&&required(cell.improvement_type)==-1){add(m(M::trait_unimproved,feature));add(m(M::unimproved,feature));}
        const auto fi=c.nature_rules.features.find(feature);
        if(fi!=c.nature_rules.features.end()&&(fi->second.natural_wonder||fi->second.pseudo_natural_wonder)&&
           (yield==c.culture||(w.options->enabled("BALANCE_VP")&&c.league_balance_yields.contains(yield))))
            for(const auto &l:w.cities.leagues)if(std::find(l.members.begin(),l.members.end(),player)!=l.members.end())for(const auto b:l.culture_per_natural_wonder)add(b);
        add(m(M::feature,feature));add(m(M::trait_feature,feature));
    }
    if(improvement!=-1&&yield==c.culture){add(p.policy_culture);if(!live.policies_resolved)throw std::runtime_error("Unresolved live policies");
        for(const auto &[id,policy]:c.rules.policies)if(live.policies.contains(id)&&(policy.branch==-1||!p.blocked_branches.contains(policy.branch)))add(value(policy.improvement_culture,improvement));}
    if(w.cities.players.at(player).golden_age&&wide::add(n,current)>=yr.golden_threshold)add(yr.golden);return n;
}
} // namespace civ5::runtime
