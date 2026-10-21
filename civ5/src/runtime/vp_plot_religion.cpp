#include "civ5/runtime/vp_plot_religion.hpp"
#include "civ5/runtime/vp_religion_beliefs_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
constexpr auto layout=saves::GameStateLayout::vp542_corpus;
std::int64_t i64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("Religion index exceeds int64");return static_cast<std::int64_t>(n);}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){const T *found=nullptr;
    for(const auto &x:v)if(x.name&&std::string_view(x.name)==name){if(found)throw std::runtime_error("Duplicate plot religion field");found=&x;}
    if(!found)throw std::runtime_error("Missing plot religion field: "+std::string(name));return *found;}
std::int64_t scalar(const auto &v,const char *name){const auto &a=named(v,name).values;if(a.size()!=1)throw std::runtime_error("Invalid plot religion scalar");return a[0];}
std::int64_t required(const std::optional<std::int64_t> &v){if(!v)throw std::runtime_error("Unresolved plot religion state");return *v;}
std::int64_t value(const YieldValues &v,std::int64_t id){const auto it=v.find(id);return it==v.end()?0:it->second;}
std::int64_t value(const VpPlotReligionRules::Matrix &v,std::int64_t id,std::int64_t y){const auto it=v.find(id);return it==v.end()?0:value(it->second,y);}
void assign(YieldValues &v,std::int64_t id,std::int64_t n){if(id<0)throw std::runtime_error("Negative religion relation ID");if(n)v[id]=n;else v.erase(id);}
const auto &religion_state(const CityYieldReligionBeliefs &r){return vp_religion_beliefs(r);}
void live_city(const CityYieldContext &s,const CityYieldState *city){if(city&&&s.players.at(city->owner).cities.at(city->id)!=city)throw std::runtime_error("Plot religion requires live city");}
void validate(const VpPlotReligionContext &c,const CityYieldState *city){
    if(c.water.rules.profile!=vp||c.water.cities.profile!=vp||c.water.state.layout!=layout||!c.water.options||!c.water.options->all_succeeded())throw std::runtime_error("Plot religion profile mismatch");
    live_city(c.water.cities,city);
}
bool has_belief(const CityYieldReligionBeliefs &r,std::int64_t id){return id==-1?r.beliefs.empty():value(religion_state(r).lookup,id)==1;}
bool shared_pantheon(const CityYieldContext &s,std::int64_t belief,std::int64_t religion,std::int64_t player){
    const CityYieldReligionBeliefs *pantheon=nullptr;
    for(const auto &r:s.religions)if(r.founder==player&&religion_state(r).pantheon){pantheon=&r;break;}
    if(!pantheon||pantheon->religion==-1||religion==-1)return false;
    const auto *a=vp_find_religion(s,pantheon->religion,player),*b=vp_find_religion(s,religion,player);
    if(!a||!b)throw std::runtime_error("Unresolved VP pantheon association");return has_belief(*a,belief)&&has_belief(*b,belief);
}
template<class F> std::int64_t majority_sum(const VpPlotReligionContext &c,const CityYieldReligionBeliefs &r,std::int64_t player,const CityYieldState *city,F get){
    std::int64_t sum=0;for(const auto id:r.beliefs){const auto n=get(c.rules.beliefs.at(id));
        if(n&&vp_plot_belief_valid(c,id,religion_state(r).belief_religion,player,city))sum=wide::add(sum,n);}return sum;
}
struct Requirements {bool improvement=false,no_improvement=false,resource=false;};
Requirements requirements(const VpPlotReligionContext &c,const CityYieldReligionBeliefs &r,std::int64_t player){
    Requirements result;for(const auto id:r.beliefs){const auto &b=c.rules.beliefs.at(id);
        if((b.improvement||b.no_improvement||b.resource)&&vp_plot_belief_valid(c,id,religion_state(r).belief_religion,player)){
            result.improvement|=b.improvement;result.no_improvement|=b.no_improvement;result.resource|=b.resource;}}
    return result;
}
bool connects(const VpPlotReligionRules &r,std::int64_t improvement,std::int64_t resource){
    if(resource<0||static_cast<std::uint64_t>(resource)>=r.resource_slots)return false;
    const auto &i=r.improvements.at(improvement);return i.connects_all||i.trades.contains(resource);
}
std::int64_t terrain_gate(const VpPlotReligionRules &r,Requirements req,std::int64_t n,std::int64_t improvement,std::int64_t resource,bool secondary){
    if(!req.improvement&&!req.resource&&!req.no_improvement)return n;
    bool accept=false;
    if(req.improvement&&req.resource)accept=improvement!=-1&&resource!=-1&&connects(r,improvement,resource);
    else if(req.improvement)accept=improvement!=-1&&(!secondary||connects(r,improvement,resource));
    else if(req.resource)accept=resource!=-1;
    else if(req.no_improvement)accept=improvement==-1;
    // Original caps rejected zero at a negative n too. Its no-feature branches
    // are unreachable under this outer condition/else-if ordering.
    return std::min(accept?n:0,n);
}
}
VpPlotReligionRules load_vp_plot_religion_rules(const content::RulesDatabase &db,const content::InfoTypeCatalog &types){
    VpPlotReligionRules r;
    const auto load=[&](const char *table,const auto &consume){const auto q=db.prefetch_info(table,consume);if(!q.all_succeeded())throw std::runtime_error("Plot religion Info: "+q.error);return q.slots;};
    const auto type_id=[&](const auto &text){if(text)if(const auto *t=types.find(*text))return t->id;return std::int64_t{-1};};
    load("Yields",[&](auto slot,auto,const auto &){r.yields.insert(i64(slot));return true;});
    r.resource_slots=load("Resources",[](auto,auto,const auto &){return true;});
    load("Features",[&](auto slot,auto,const auto &row){if(row.boolean("NaturalWonder")||row.boolean("PseudoNaturalWonder"))r.natural_wonders.insert(i64(slot));return true;});
    r.belief_slots=load("Beliefs",[&](auto slot,auto,const auto &row){auto &b=r.beliefs[i64(slot)];
        b.pantheon=row.boolean("Pantheon");b.founder=row.boolean("Founder");b.follower=row.boolean("Follower");b.enhancer=row.boolean("Enhancer");b.reformation=row.boolean("Reformation");
        b.improvement=row.boolean("RequiresImprovement");b.no_improvement=row.boolean("RequiresNoImprovement");b.resource=row.boolean("RequiresResource");b.no_feature=row.boolean("RequiresNoFeature");b.civilization=type_id(row.text("CivilizationType"));
        const auto type=row.text("Type");const content::DatabaseValue param=type?content::DatabaseValue(std::string(*type)):content::DatabaseValue{};
        const auto matrix=[&](const char *table,const char *infos,const char *column,VpPlotReligionRules::Matrix &m){
            const auto q=db.query_rows({std::string("select ")+infos+".ID, Yields.ID, Yield from "+table+" inner join "+infos+" on "+infos+".Type = "+column+" inner join Yields on Yields.Type = YieldType where BeliefType = ?",{param}},[&](const auto &a){
                const auto id=a.integer_at(0);if(id<0)throw std::runtime_error("Negative plot religion Info ID");assign(m[id],a.integer_at(1),a.integer_at(2));});
            if(!q.all_succeeded())throw std::runtime_error("Plot religion relation: "+q.error);};
        matrix("Belief_TerrainYieldChanges","Terrains","TerrainType",b.terrain);matrix("Belief_PlotYieldChanges","Plots","PlotType",b.plot);
        matrix("Belief_FeatureYieldChanges","Features","FeatureType",b.feature);matrix("Belief_UnimprovedFeatureYieldChanges","Features","FeatureType",b.unimproved_feature);
        matrix("Belief_ResourceYieldChanges","Resources","ResourceType",b.resources);matrix("Belief_ImprovementYieldChanges","Improvements","ImprovementType",b.improvements);
        const auto vector=[&](const char *table,YieldValues &v){const auto q=db.query_rows({std::string("select Yields.ID, Yield from ")+table+" inner join Yields on Yields.Type = YieldType where BeliefType = ?",{param}},[&](const auto &a){assign(v,a.integer_at(0),a.integer_at(1));});
            if(!q.all_succeeded())throw std::runtime_error("Plot religion vector: "+q.error);};
        vector("Belief_LakePlotYield",b.lake);vector("Belief_YieldChangeNaturalWonder",b.wonder);return true;});
    load("Improvements",[&](auto slot,auto,const auto &row){auto &i=r.improvements[i64(slot)];i.connects_all=row.boolean("ConnectsAllResources");i.great_person=row.boolean("CreatedByGreatPerson");
        i.cityside=row.boolean("Cityside");i.ignore_ownership=row.boolean("IgnoreOwnership");i.created_resource=type_id(row.text("ImprovementResource"));
        const auto type=row.text("Type");const content::DatabaseValue param=type?content::DatabaseValue(std::string(*type)):content::DatabaseValue{};
        const auto q=db.query_rows({"select Resources.ID, ResourceTrade, ResourceMakesValid from Improvement_ResourceTypes inner join Resources on Resources.Type = ResourceType where ImprovementType = ?",{param}},[&](const auto &a){
            const auto id=a.integer_at(0);if(id<0)throw std::runtime_error("Negative improvement religion resource");
            if(a.integer_at(1))i.trades.insert(id);else i.trades.erase(id);if(a.integer_at(2))i.valid_resources.insert(id);else i.valid_resources.erase(id);});
        if(!q.all_succeeded())throw std::runtime_error("Plot religion improvement relation: "+q.error);return true;});
    return r;
}
void restore_vp_plot_religion(const saves::SavedSessionSnapshot &s,const VpPlotReligionRules &r,CityYieldContext &cities){
    if(cities.profile!=vp||!s.all_players||s.all_players->layout!=layout||s.religion_culture.layout!=layout)throw std::runtime_error("VP plot religion import profile mismatch");
    std::vector<CityYieldReligionBeliefs> religions;
    for(const auto &source:s.religion_culture.religions){CityYieldReligionBeliefs a;a.religion=source.religion;a.founder=source.founder;a.beliefs=source.beliefs.belief_ids;
        auto &profile=ensure_vp_religion_beliefs(a,cities.profile);
        profile.belief_religion=required(source.beliefs.religion);profile.holy_x=source.holy_city_x;profile.holy_y=source.holy_city_y;profile.pantheon=source.pantheon;
        if(source.beliefs.belief_lookup.size()!=r.belief_slots)throw std::runtime_error("VP belief lookup differs from active Info span");
        for(std::size_t i=0;i<source.beliefs.belief_lookup.size();++i)if(const auto n=source.beliefs.belief_lookup[i])profile.lookup[i64(i)]=n;
        for(const auto id:a.beliefs)if(!r.beliefs.contains(id))throw std::runtime_error("Unknown saved VP belief");religions.push_back(std::move(a));}
    std::set<std::int64_t> seen;
    for(const auto &source:s.all_players->players){const auto id=source.saved_player_id;
        if(!seen.insert(id).second||source.core.layout!=layout||source.knowledge.layout!=layout||source.cities.layout!=layout)throw std::runtime_error("VP plot religion player profile mismatch");
        auto &p=cities.players.at(id);p.second_pantheon_count=scalar(source.core.fields,"m_iSecondReligionPantheonCount");
        vp_player_state_religion(p)=named(named(source.knowledge.blocks,"player_religions").fields,"state_religion").value;
        std::set<std::int64_t> ids;
        for(const auto &sc:source.cities.cities){const auto cid=scalar(sc.core.fields,"m_iID");auto &city=p.cities.at(cid);
            if(!ids.insert(cid).second||sc.core.layout!=layout||sc.tail.layout!=layout||city.id!=cid||city.owner!=id||scalar(sc.core.fields,"m_eOwner")!=id||city.source_core!=std::optional{std::pair{sc.core.start_offset,sc.core.end_offset}})
                throw std::runtime_error("VP plot religion city binding mismatch");
            const auto &rel=named(sc.tail.blocks,"religions");vp_city_majority_religion(city)=scalar(rel.children,"m_eMajorityCityReligion");city.religions.clear();
            for(const auto &v:named(rel.children,"m_ReligionStatus").children)city.religions.push_back({scalar(v.children,"m_eReligion"),scalar(v.children,"m_iFollowers"),scalar(v.children,"m_iPressure")});}
        if(ids.size()!=p.cities.size())throw std::runtime_error("Incomplete plot religion cities");}
    if(seen.size()!=cities.players.size())throw std::runtime_error("Incomplete plot religion players");cities.religions=std::move(religions);
}
const CityYieldReligionBeliefs *vp_find_religion(const CityYieldContext &s,std::int64_t id,std::int64_t player){
    if(s.profile!=vp)throw std::runtime_error("VP religion lookup profile mismatch");if(id==-1)return nullptr;
    for(const auto &r:s.religions)if(r.religion==id&&(id!=s.pantheon||r.founder==player))return &r;return nullptr;
}
VpPlotReligionSelection vp_plot_religion_selection(const CityYieldContext &s,const VpPlotReligionRules &rules,const CityYieldState &city){
    live_city(s,&city);VpPlotReligionSelection result;result.majority=vp_find_religion(s,required(vp_city_majority_religion(city)),city.owner);
    if(required(s.players.at(city.owner).second_pantheon_count)>0&&city.religions.size()>1&&city.religions[1].pressure>0){
        if(const auto *r=vp_find_religion(s,city.religions[1].religion,city.owner))for(const auto id:r->beliefs){const auto b=rules.beliefs.find(id);
            if(b!=rules.beliefs.end()&&b->second.pantheon){result.secondary=id;break;}}}
    return result;
}
bool vp_pantheon_belief_in_religion(const CityYieldContext &s,std::int64_t belief,std::int64_t religion,std::int64_t player){
    if(s.profile!=vp)throw std::runtime_error("VP pantheon profile mismatch");return shared_pantheon(s,belief,religion,player);
}
std::int64_t vp_created_pantheon(const CityYieldContext &world,std::int64_t player){
    if(world.profile!=vp)throw std::runtime_error("Created pantheon requires VP world");
    for(const auto &r:world.religions)if(r.founder==player&&religion_state(r).pantheon)return r.religion;return -1;
}
std::int64_t vp_created_religion(const PlotWaterContext &w,std::int64_t player,bool include_pantheon){
    if(w.cities.profile!=vp||w.state.layout!=layout)throw std::runtime_error("Created religion requires VP world");
    const auto &world=w.cities;
    const auto city_by_ref=[](const auto &cities,const auto &ref)->const CityYieldState*{
        const auto p=cities.players.find(ref.owner);if(p==cities.players.end())return nullptr;
        const auto c=p->second.cities.find(ref.id);return c==p->second.cities.end()?nullptr:&c->second;};
    std::int64_t created=-1;MapGeometry geometry{w.map.grid,w.map.wrap_x,w.map.wrap_y,w.state.layout};
    for(const auto &r:world.religions)if(r.founder==player){
        const auto &profile=religion_state(r);
        if(!include_pantheon&&profile.pantheon)continue;
        const auto plot=geometry.plot(profile.holy_x,profile.holy_y);const CityYieldState *holy=nullptr;
        if(plot){const auto &p=w.map.cells.at(*plot);if(p.is_city){if(!p.plot_city)throw std::runtime_error("Unresolved holy plot city");holy=city_by_ref(w.cities,*p.plot_city);}}
        if(holy&&holy->owner==player){created=r.religion;break;}if(profile.pantheon)created=r.religion;
    }
    return created;
}
VpPermanentPantheonSelection vp_permanent_pantheon(const PlotWaterContext &w,const CityYieldState &city,VpPlotReligionSelection selected){
    if(w.cities.profile!=vp||w.state.layout!=layout||!w.options||!w.options->all_succeeded())throw std::runtime_error("Permanent pantheon requires VP world");
    live_city(w.cities,&city);if(!w.options->enabled("BALANCE_PERMANENT_PANTHEONS"))return {};
    const auto &world=w.cities;
    // HasCreatedPantheon delegates to the same live created-religion query.
    const auto created=vp_created_religion(w,city.owner,true);
    if(created==-1)return {};
    const auto *pantheon=vp_find_religion(world,world.pantheon,city.owner);if(!pantheon)return {};
    std::int64_t belief=-1;const auto state=required(vp_player_state_religion(world.players.at(city.owner)));
    // Source order matters: a state religion can precede the personal pantheon.
    for(const auto &r:world.religions){const auto &profile=religion_state(r);
        if((r.founder==city.owner&&profile.pantheon)||(!profile.pantheon&&r.religion==state)){belief=r.beliefs.empty()?-1:r.beliefs.front();break;}}
    if(belief==-1||belief==selected.secondary)return {};
    if(selected.majority&&vp_pantheon_belief_in_religion(world,belief,required(vp_city_majority_religion(city)),city.owner))return {};
    return {pantheon,belief};
}
bool vp_plot_belief_valid(const VpPlotReligionContext &c,std::int64_t belief,std::int64_t religion,std::int64_t player,const CityYieldState *city){
    validate(c,city);if(player==-1)return true;const auto &s=c.water.cities;const auto &p=s.players.at(player);const auto &b=c.rules.beliefs.at(belief);bool eligible=false;
    if(const auto *r=vp_find_religion(s,religion)){const auto &a=religion_state(*r);const auto &map=c.water.map;MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,layout};const auto holy=geometry.plot(a.holy_x,a.holy_y);
        if(a.pantheon&&!holy)eligible=r->founder==player;else if(holy)eligible=required(map.cells.at(*holy).owner)==player;}
    if(religion!=-1&&(b.founder||b.enhancer)&&(!eligible||(city&&city->owner!=player)))return false;
    if(b.follower&&city&&(required(vp_city_majority_religion(*city))!=religion||city->owner!=player))return false;
    if(b.reformation&&((city&&required(vp_city_majority_religion(*city))!=religion)||(!eligible&&required(vp_player_state_religion(p))!=religion)))return false;
    if(b.pantheon){if(religion==-1)religion=s.pantheon;
        if(city&&((required(vp_city_majority_religion(*city))!=religion&&!shared_pantheon(s,belief,religion,player))||city->owner!=player))return false;}
    return !c.water.options->enabled("BALANCE_UNIQUE_BELIEFS_ONLY_FOR_CIV")||b.civilization==-1||b.civilization==p.civilization;
}
std::int64_t VpReligionNatureQuote::total()const{std::int64_t n=0;for(auto v:{terrain,plot,lake,unimproved_feature,feature,wonder,resource})n=wide::add(n,v);return n;}
VpReligionNatureQuote vp_religion_nature_yield(const VpPlotReligionContext &c,std::int64_t plot,std::int64_t yield,std::int64_t player,std::int64_t improvement,std::int64_t feature,std::int64_t resource,const CityYieldState *city,VpPlotReligionSelection selection){
    validate(c,city);VpReligionNatureQuote q;if(!selection.majority||player==-1)return q;
    if(plot<0||!c.rules.yields.contains(yield))throw std::out_of_range("Invalid VP religion plot/yield");const auto &p=c.water.map.cells.at(plot);const auto &r=*selection.majority;
    const auto *secondary=selection.secondary==-1?nullptr:&c.rules.beliefs.at(selection.secondary);
    const auto sum=[&](const auto &get){auto n=majority_sum(c,r,player,city,get);return secondary?wide::add(n,get(*secondary)):n;};
    const auto terrain=required(p.terrain_type);
    q.terrain=terrain_gate(c.rules,requirements(c,r,player),majority_sum(c,r,player,city,[&](const auto &b){return value(b.terrain,terrain,yield);}),improvement,resource,false);
    if(secondary)q.terrain=wide::add(q.terrain,terrain_gate(c.rules,{secondary->improvement,secondary->no_improvement,secondary->resource},value(secondary->terrain,terrain,yield),improvement,resource,true));
    q.plot=sum([&](const auto &b){return value(b.plot,required(p.plot_type),yield);});if(plot_is_lake(c.water,plot))q.lake=sum([&](const auto &b){return value(b.lake,yield);});
    if(feature!=-1){if(improvement==-1)q.unimproved_feature=sum([&](const auto &b){return value(b.unimproved_feature,feature,yield);});
        // All three source Requires(No)Improvement branches add the same feature value.
        q.feature=sum([&](const auto &b){return value(b.feature,feature,yield);});
        if(c.rules.natural_wonders.contains(required(p.feature_type)))q.wonder=sum([&](const auto &b){return value(b.wonder,yield);});}
    if(resource!=-1)q.resource=sum([&](const auto &b){return value(b.resources,resource,yield);});return q;
}
std::int64_t vp_religion_improvement_yield(const VpPlotReligionContext &c,std::int64_t yield,std::int64_t player,std::int64_t improvement,std::int64_t resource,const CityYieldState *city,VpPlotReligionSelection selection){
    validate(c,city);if(!city||!selection.majority||player==-1||improvement==-1)return 0;const auto ii=c.rules.improvements.find(improvement);if(ii==c.rules.improvements.end())return 0;
    if(!c.rules.yields.contains(yield))throw std::out_of_range("Invalid VP religion yield");const auto &i=ii->second;const auto &r=*selection.majority;
    const auto connected=[&]{return resource!=-1&&(i.valid_resources.contains(resource)||i.created_resource==resource);};
    const bool require=requirements(c,r,city->owner).resource&&!i.great_person&&!i.cityside&&!i.ignore_ownership;
    std::int64_t n=0;if(!require||connected())n=majority_sum(c,r,city->owner,city,[&](const auto &b){return value(b.improvements,improvement,yield);});
    if(selection.secondary!=-1){const auto &b=c.rules.beliefs.at(selection.secondary);
        if(!b.resource||i.great_person||i.cityside||connected())n=wide::add(n,value(b.improvements,improvement,yield));}
    return n;
}
} // namespace civ5::runtime
