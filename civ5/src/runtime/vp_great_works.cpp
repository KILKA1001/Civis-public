#include "civ5/runtime/vp_great_works.hpp"
#include "civ5/runtime/vp_religion_beliefs_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include "civ5/saves/vp_city_production_profile.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
constexpr auto layout=saves::GameStateLayout::vp542_corpus;
std::int64_t id64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("Great-work ID exceeds int64");return static_cast<std::int64_t>(n);}
std::int64_t add(std::int64_t a,std::int64_t b){return wide::add(a,b);}
std::int64_t mul(std::int64_t a,std::int64_t b){return wide::multiply_divide(a,b,1);}
std::int64_t value(const YieldValues &v,std::int64_t id,std::int64_t fallback=0){const auto i=v.find(id);return i==v.end()?fallback:i->second;}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){const T *found=nullptr;
    for(const auto &x:v)if(x.name&&std::string_view(x.name)==name){if(found)throw std::runtime_error("Duplicate great-work field");found=&x;}
    if(!found)throw std::runtime_error("Missing great-work field: "+std::string(name));return *found;}
std::int64_t scalar(const auto &v,const char *name){const auto &a=named(v,name).values;if(a.size()!=1)throw std::runtime_error("Invalid great-work scalar");return a[0];}
std::int64_t define(const VpGreatWorkContext &c,const char *name){const auto *d=c.religion.water.defines.find(name);
    if(!d||!d->lookup.error.empty())throw std::runtime_error("Unresolved great-work define: "+std::string(name));return std::get<std::int64_t>(d->lookup.value);}
void validate(const VpGreatWorkContext &c,const CityYieldState &city){const auto &w=c.religion.water;
    if(c.cities.profile!=vp||c.city_rules.profile!=vp||w.players.profile!=vp||&w.cities!=&c.cities||!w.options||!w.options->all_succeeded()||
       w.defines.profile()!=content::GlobalDefineProfile::vp542||&c.cities.players.at(city.owner).cities.at(city.id)!=&city||!city.building_counts_resolved)
        throw CityYieldUnavailable("Great works require resolved shared VP city state");
    (void)vp_great_work_city(c.cities,city.owner,city.id);(void)vp_great_work_player(c.cities,city.owner);
}
std::int64_t num_buildings(const VpGreatWorkContext &c,const CityYieldState &city,std::int64_t id){
    const auto real=value(city.real_buildings,id),free=value(city.free_buildings,id);
    return define(c,"CITY_MAX_NUM_BUILDINGS")<=1?std::max(real,free):add(real,free);
}
std::int64_t theme_value(const VpGreatWorkRules::Building &b,std::int64_t index){
    // Native GetThemingBonusInfo returns null when the first bonus is zero.
    // An unused allocated entry has a zero bonus. Owned rules have no fixed bound.
    if(index<0||b.themes.empty()||!b.themes.front())throw CityYieldUnavailable("Invalid active building theme");
    return static_cast<std::uint64_t>(index)<b.themes.size()?b.themes[static_cast<std::size_t>(index)]:0;
}
}
VpGreatWorkRules load_vp_great_work_rules(const content::RulesDatabase &db,const content::InfoTypeCatalog &types){
    VpGreatWorkRules r;
    const auto load=[&](const char *table,const auto &consume){const auto q=db.prefetch_info(table,consume);if(!q.all_succeeded())throw std::runtime_error("Great-work Info: "+q.error);return q.slots;};
    const auto type_id=[&](const auto &text){if(text)if(const auto *t=types.find(*text))return t->id;return std::int64_t{-1};};
    const auto id=[&](const char *name){const auto *t=types.find(name);return t?t->id:std::int64_t{-1};};
    load("Yields",[&](auto n,auto,const auto &){r.yields.insert(id64(n));return true;});
    r.class_slots=load("BuildingClasses",[](auto,auto,const auto &){return true;});
    r.art=id("GREAT_WORK_ART");r.artifact=id("GREAT_WORK_ARTIFACT");
    const char *slot_names[]={"GREAT_WORK_SLOT_ART_ARTIFACT","GREAT_WORK_SLOT_LITERATURE","GREAT_WORK_SLOT_MUSIC","GREAT_WORK_SLOT_FILM","GREAT_WORK_SLOT_RELIC"};
    for(std::size_t i=0;i<r.slots.size();++i)r.slots[i]=id(slot_names[i]);
    const auto yields=[&](const char *table,const char *key,content::DatabaseValue type,YieldValues &v){
        const auto q=db.query_rows({std::string("select Yields.ID, Yield from ")+table+" inner join Yields on Yields.Type = YieldType where "+key+" = ?",{type}},[&](const auto &row){
            const auto y=row.integer_at(0),n=row.integer_at(1);if(y<0)throw std::runtime_error("Negative great-work yield ID");if(n)v[y]=n;else v.erase(y);});
        if(!q.all_succeeded())throw std::runtime_error("Great-work yield relation: "+q.error);
    };
    r.building_slots=load("Buildings",[&](auto n,auto,const auto &row){auto &b=r.buildings[id64(n)];b.building_class=type_id(row.text("BuildingClass"));b.slot=type_id(row.text("GreatWorkSlotType"));
        b.yield=type_id(row.text("GreatWorkYieldType"));b.slots=std::max<std::int64_t>(0,row.integer("GreatWorkCount"));
        const auto type=row.text("Type");const content::DatabaseValue param=type?content::DatabaseValue(std::string(*type)):content::DatabaseValue{};
        yields("Building_ThemingYieldBonus","BuildingType",param,b.theme_yields);
        const auto q=db.query_rows({"select Bonus, Description, SameEra, UniqueEras, ConsecutiveEras, MustBeArt, MustBeArtifact, MustBeEqualArtArtifact, RequiresOwner, RequiresAnyButOwner, RequiresSamePlayer, RequiresUniquePlayers, AIPriority from Building_ThemingBonuses where BuildingType = ?",{param}},[&](const auto &t){b.themes.push_back(t.integer_at(0));});
        if(!q.all_succeeded())throw std::runtime_error("Great-work theme relation: "+q.error);return true;});
    load("Beliefs",[&](auto n,auto,const auto &row){auto &b=r.beliefs[id64(n)];b.minimum_population=row.integer("MinPopulation");const auto type=row.text("Type");
        yields("Belief_GreatWorkYieldChanges","BeliefType",type?content::DatabaseValue(std::string(*type)):content::DatabaseValue{},b.yields);return true;});return r;
}
void restore_vp_great_works(const saves::SavedSessionSnapshot &s,const VpGreatWorkRules &r,CityYieldContext &world){
    if(world.profile!=vp||!s.all_players||s.all_players->layout!=layout||s.religion_culture.layout!=layout)throw std::runtime_error("Great-work import requires VP world");
    std::map<std::int64_t,saves::GreatWorkObservation> works;
    for(std::size_t i=0;i<s.religion_culture.great_works.size();++i){const auto &w=s.religion_culture.great_works[i];if(!w.class_type)throw std::runtime_error("Unresolved saved work class");works.emplace(id64(i),w);}
    const auto vector=[&](const auto &fields,const char *name){const auto &v=named(fields,name).values;YieldValues result;
        if(v.size()!=world.yield_slots)throw std::runtime_error("Great-work yield wire span mismatch");for(std::size_t i=0;i<v.size();++i)if(v[i])result[id64(i)]=v[i];return result;};
    std::set<std::int64_t> players;auto &extension=ensure_vp_great_work_profile(world);
    for(const auto &sp:s.all_players->players){const auto owner=sp.saved_player_id;auto &p=world.players.at(owner);
        if(!players.insert(owner).second||sp.core.layout!=layout||sp.traits.layout!=layout||sp.strategy.layout!=layout||sp.cities.layout!=layout||scalar(sp.core.fields,"m_eID")!=owner)
            throw std::runtime_error("Great-work player binding mismatch");
        auto &pg=extension.players[owner];pg.tourism_modifier=scalar(sp.core.fields,"m_iGreatWorksTourismModifierGlobal");pg.capital_theming_modifier=scalar(sp.traits.fields,"m_iCapitalThemingBonusModifier");
        // Pinned VP enum POLICYMOD_THEMING_BONUS: native vector byte offset 0xc4.
        // This is a wire identity, not a host capacity for policies or modifiers.
        const auto &policy=named(named(sp.strategy.blocks,"policies").arrays,"policy_modifiers");
        if(policy.columns!=1||policy.values.size()<=49)throw std::runtime_error("Missing saved theming policy modifier");pg.theming_policy_modifier=policy.values[49];
        pg.per_work=vector(sp.core.fields,"m_aiGreatWorkYieldChange");pg.trait_per_work=vector(sp.traits.fields,"m_iGreatWorkYieldChanges");
        const char *core_names[]={"m_aiArtYieldBonus","m_aiArtifactYieldBonus","m_aiLitYieldBonus","m_aiMusicYieldBonus","m_aiFilmYieldBonus","m_aiRelicYieldBonus"};
        const char *trait_names[]={"m_iArtYieldChanges","m_iArtifactYieldChanges","m_iLitYieldChanges","m_iMusicYieldChanges"};
        for(std::size_t i=0;i<6;++i)pg.type_bonus[i]=vector(sp.core.fields,core_names[i]);
        for(std::size_t i=0;i<4;++i)pg.trait_type_bonus[i]=vector(sp.traits.fields,trait_names[i]);
        std::set<std::int64_t> cities;
        for(const auto &sc:sp.cities.cities){const auto id=scalar(sc.core.fields,"m_iID");auto &city=p.cities.at(id);
            if(!cities.insert(id).second||sc.core.layout!=layout||sc.production.layout!=layout||city.owner!=owner||city.id!=id||scalar(sc.core.fields,"m_eOwner")!=owner||
               city.source_core!=std::optional{std::pair{sc.core.start_offset,sc.core.end_offset}}||!city.building_counts_resolved||saves::vp_city_production_building_slots(sc.production.context)!=r.building_slots)
                throw std::runtime_error("Great-work city binding mismatch");
            auto &cg=extension.cities[{owner,id}];cg.per_work=vector(sc.core.fields,"m_aiGreatWorkYieldChange");cg.per_local_theme=vector(sc.core.fields,"m_aiYieldChangesPerLocalTheme");
            const auto &b=named(sc.production.blocks,"buildings");cg.tourism_modifier=named(b.fields,"m_iGreatWorksTourismModifier").value;
            const auto &themes=named(b.arrays,"m_paiThemingBonusIndex");if(themes.columns!=1||themes.values.size()!=r.building_slots)throw std::runtime_error("Great-work building wire span mismatch");
            for(std::size_t i=0;i<themes.values.size();++i)if(themes.values[i]!=-1)cg.theming_indices[id64(i)]=themes.values[i];
            const auto &slots=named(b.arrays,"m_aBuildingGreatWork");if(slots.columns!=3||slots.values.size()%3)throw std::runtime_error("Invalid great-work slots");
            for(std::size_t i=0;i<slots.values.size();i+=3)cg.works.push_back({slots.values[i],slots.values[i+1],slots.values[i+2]});
            // CvCityBuildings::Read enumerates every source building index, including holes.
            std::set<std::int64_t> existing;for(const auto *counts:{&city.real_buildings,&city.free_buildings})for(const auto &[bid,n]:*counts){
                if(bid<0||static_cast<std::uint64_t>(bid)>=r.building_slots)throw std::runtime_error("Saved building outside source span");if(n>0)existing.insert(bid);}
            cg.existing_buildings.assign(existing.begin(),existing.end());
        }if(cities.size()!=p.cities.size())throw std::runtime_error("Incomplete great-work cities");
    }if(players.size()!=world.players.size())throw std::runtime_error("Incomplete great-work players");world.great_works=std::move(works);
}
std::int64_t vp_building_from_class(const VpGreatWorkContext &c,CityYieldState &city,std::int64_t cls,bool fallback){
    validate(c,city);auto &s=vp_great_work_city(c.cities,city.owner,city.id);
    if(c.religion.water.options->enabled("BUILDINGS_THOROUGH_PREREQUISITES")){
        if(cls==-1)throw CityYieldUnavailable("NO_BUILDINGCLASS violates the source lookup precondition");
        if(!s.building_by_class){YieldValues cache;
            for(auto id:s.existing_buildings){const auto i=c.rules.buildings.find(id);if(i==c.rules.buildings.end())continue;const auto bc=i->second.building_class;
                if(bc>=0&&static_cast<std::uint64_t>(bc)<c.rules.class_slots&&num_buildings(c,city,id)>0)cache.emplace(bc,id);}
            s.building_by_class=std::move(cache);
        }const auto id=value(*s.building_by_class,cls,-1);if(!fallback||id!=-1)return id;
    }
    const auto civ=c.cities.players.at(city.owner).civilization;const auto i=c.city_rules.civilization_buildings.find(civ);
    if(i==c.city_rules.civilization_buildings.end())throw CityYieldUnavailable("Unresolved great-work civilization buildings");return value(i->second,cls,-1);
}
std::int64_t vp_city_building_count(const VpGreatWorkContext &c,const CityYieldState &city,std::int64_t building){
    validate(c,city);return num_buildings(c,city,building);
}
bool vp_city_has_building_class(const VpGreatWorkContext &c,const CityYieldState &city,std::int64_t cls){
    validate(c,city);if(cls==-1)throw CityYieldUnavailable("NO_BUILDINGCLASS violates the source presence precondition");
    if(c.religion.water.options->enabled("BUILDINGS_THOROUGH_PREREQUISITES")){
        // Presence scans the live list, independently of the lazy class cache.
        for(auto id:vp_great_work_city(c.cities,city.owner,city.id).existing_buildings){const auto i=c.rules.buildings.find(id);
            if(i!=c.rules.buildings.end()&&i->second.building_class==cls&&num_buildings(c,city,id)>0)return true;}
        return false;
    }
    const auto civ=c.cities.players.at(city.owner).civilization;const auto i=c.city_rules.civilization_buildings.find(civ);
    if(i==c.city_rules.civilization_buildings.end())throw CityYieldUnavailable("Unresolved civilization buildings for class presence");
    const auto id=value(i->second,cls,-1);return id!=-1&&num_buildings(c,city,id)>0;
}
std::int64_t vp_great_works_in_building(const VpGreatWorkContext &c,const CityYieldState &city,std::int64_t cls){
    std::int64_t n=0;for(const auto &w:vp_great_work_city(c.cities,city.owner,city.id).works)if(w.building_class==cls)n=add(n,1);return n;
}
std::int64_t vp_great_works_in_slot(const VpGreatWorkContext &c,CityYieldState &city,std::int64_t slot,bool artifact,bool art){
    validate(c,city);if(slot==-1)return 0;std::int64_t n=0;
    for(const auto &w:vp_great_work_city(c.cities,city.owner,city.id).works){const auto id=vp_building_from_class(c,city,w.building_class);const auto b=c.rules.buildings.find(id);if(b==c.rules.buildings.end())continue;
        bool match=b->second.slot==slot;if(artifact||art){if(!c.cities.great_works)throw CityYieldUnavailable("Unresolved global great works");const auto &work=c.cities.great_works->at(w.work);
            if(!work.class_type)throw CityYieldUnavailable("Unresolved great-work class");match=*work.class_type==(artifact?c.rules.artifact:c.rules.art);}
        if(match)n=add(n,1);
    }return n;
}
std::int64_t vp_city_great_works(const VpGreatWorkContext &c,CityYieldState &city,bool ignore_yield){
    validate(c,city);std::int64_t n=0;
    for(const auto &w:vp_great_work_city(c.cities,city.owner,city.id).works){const auto id=vp_building_from_class(c,city,w.building_class);const auto b=c.rules.buildings.find(id);
        if(b!=c.rules.buildings.end()&&(ignore_yield||b->second.yield!=-1))n=add(n,1);}
    return n;
}
std::int64_t vp_themed_buildings(const VpGreatWorkContext &c,const CityYieldState &city){
    validate(c,city);const auto &state=vp_great_work_city(c.cities,city.owner,city.id);std::int64_t n=0;for(auto id:state.existing_buildings)if(c.rules.buildings.contains(id)&&value(state.theming_indices,id,-1)!=-1)n=add(n,1);return n;
}
std::int64_t vp_theming_multiplier(const VpGreatWorkContext &c,const CityYieldState &city){
    validate(c,city);const auto &p=c.cities.players.at(city.owner);const auto &s=vp_great_work_player(c.cities,city.owner);
    return mul(add(100,s.theming_policy_modifier),add(100,city.id==p.capital?s.capital_theming_modifier:0));
}
std::int64_t vp_theming_bonus(const VpGreatWorkContext &c,CityYieldState &city,std::int64_t cls){
    const auto id=vp_building_from_class(c,city,cls);if(id==-1||num_buildings(c,city,id)<=0)return 0;
    const auto index=value(vp_great_work_city(c.cities,city.owner,city.id).theming_indices,id,-1);if(index==-1)return 0;
    const auto &b=c.rules.buildings.at(vp_building_from_class(c,city,cls,true));const auto bonus=wide::multiply_divide(theme_value(b,index),vp_theming_multiplier(c,city),10000);
    const auto &p=c.cities.players.at(city.owner);const auto &setup=c.religion.water.players;
    if(city.id==p.capital&&vp_great_work_player(c.cities,city.owner).capital_theming_modifier&&c.religion.water.options->enabled("ENABLE_ACHIEVEMENTS")&&
       setup.is_human(setup.players.at(city.owner))&&setup.game_type!=1&&setup.game_type!=2&&setup.game_type!=3&&bonus>=16)c.services.unlock_capital_theming_achievement(city.owner);
    return bonus;
}
std::int64_t vp_current_theming_bonuses(const VpGreatWorkContext &c,const CityYieldState &city,std::int64_t yield){
    validate(c,city);const auto &state=vp_great_work_city(c.cities,city.owner,city.id);std::int64_t n=0;for(auto id:state.existing_buildings){const auto &b=c.rules.buildings.at(id);
        if(b.yield!=yield&&yield!=c.rules.tourism)continue;const auto index=value(state.theming_indices,id,-1);if(index==-1)continue;
        n=add(n,wide::multiply_divide(theme_value(b,index),vp_theming_multiplier(c,city),10000));
    }return n;
}
VpGreatWorkYield vp_great_work_yield(const VpGreatWorkContext &c,CityYieldState &city,std::int64_t yield){
    validate(c,city);if(!c.rules.yields.contains(yield))throw std::out_of_range("Invalid great-work yield");VpGreatWorkYield q;
    if(yield>c.rules.culture_local&&!c.religion.water.options->enabled("BALANCE_CORE_JFD")){q.suppressed=true;return q;}
    const auto &s=vp_great_work_city(c.cities,city.owner,city.id);const auto &pg=vp_great_work_player(c.cities,city.owner);
    for(auto id:s.existing_buildings){const auto it=c.rules.buildings.find(id);if(it==c.rules.buildings.end()||it->second.slots<=0)continue;const auto &b=it->second;
        const auto n=vp_great_works_in_building(c,city,b.building_class);q.works=add(q.works,n);
        if(vp_theming_bonus(c,city,b.building_class)>0)q.extra_themes=add(q.extra_themes,value(b.theme_yields,yield));
        if(yield==b.yield)q.matching_works=add(q.matching_works,n);
    }
    q.extra_themes=add(q.extra_themes,mul(value(s.per_local_theme,yield),vp_themed_buildings(c,city)));
    if(q.works<=0)return q;
    q.per_work=add(add(value(pg.per_work,yield),value(pg.trait_per_work,yield)),value(s.per_work,yield));
    if(yield==c.rules.tourism)q.per_work=add(q.per_work,define(c,"BASE_TOURISM_PER_GREAT_WORK"));
    if(!vp_city_majority_religion(city))throw CityYieldUnavailable("Unresolved great-work majority religion");
    VpPlotReligionSelection selected;
    if(*vp_city_majority_religion(city)>=c.cities.pantheon){selected.majority=vp_find_religion(c.cities,*vp_city_majority_religion(city),city.owner);
        if(selected.majority){const auto &r=*selected.majority;const auto &profile=vp_religion_beliefs(r);
            for(auto bid:r.beliefs){if(bid==-1)continue;const auto &b=c.rules.beliefs.at(bid);const auto n=city.population>=b.minimum_population?value(b.yields,yield):0;
                if(n&&vp_plot_belief_valid(c.religion,bid,profile.belief_religion,city.owner,&city))q.beliefs=add(q.beliefs,n);}
            selected.secondary=vp_plot_religion_selection(c.cities,c.religion.rules,city).secondary;
            if(selected.secondary!=-1)q.secondary=value(c.rules.beliefs.at(selected.secondary).yields,yield);
        }
    }
    const auto pantheon=vp_permanent_pantheon(c.religion.water,city,selected);if(pantheon.religion)q.pantheon=value(c.rules.beliefs.at(pantheon.belief).yields,yield);
    q.per_work=add(q.per_work,add(add(q.beliefs,q.secondary),q.pantheon));
    q.base_times100=add(mul(mul(q.matching_works,define(c,"BASE_CULTURE_PER_GREAT_WORK")),100),mul(mul(q.works,q.per_work),100));
    if(yield==c.rules.tourism)q.tourism_times100=wide::multiply_divide(add(s.tourism_modifier,pg.tourism_modifier),q.base_times100,100);
    q.themes=vp_current_theming_bonuses(c,city,yield);
    const auto &slots=c.rules.slots;
    const std::array<std::int64_t,6> counts={vp_great_works_in_slot(c,city,slots[0],false,true),vp_great_works_in_slot(c,city,slots[0],true),
        vp_great_works_in_slot(c,city,slots[1]),vp_great_works_in_slot(c,city,slots[2]),vp_great_works_in_slot(c,city,slots[3]),vp_great_works_in_slot(c,city,slots[4])};
    for(std::size_t i=0;i<counts.size();++i)if(counts[i]>0){if(i<4)q.type_bonus=add(q.type_bonus,mul(value(pg.trait_type_bonus[i],yield),counts[i]));q.type_bonus=add(q.type_bonus,mul(value(pg.type_bonus[i],yield),counts[i]));}
    q.total_times100=add(add(add(q.base_times100,q.tourism_times100),mul(q.themes,100)),mul(add(q.type_bonus,q.extra_themes),100));return q;
}
std::int64_t vp_cached_great_work_yield(const VpGreatWorkContext &c,CityYieldState &city,std::int64_t yield){
    validate(c,city);if(!c.rules.yields.contains(yield))throw std::out_of_range("Invalid great-work yield");auto &cache=vp_great_work_city(c.cities,city.owner,city.id).yield_cache;
    const auto i=cache.find(yield);if(i!=cache.end()&&i->second!=-1)return i->second;
    const auto n=vp_great_work_yield(c,city,yield).total_times100;cache[yield]=n;return n;
}
void invalidate_vp_great_work_yields(CityYieldContext &world,const CityYieldState &city){vp_great_work_city(world,city.owner,city.id).yield_cache.clear();}
void invalidate_vp_building_class_cache(CityYieldContext &world,const CityYieldState &city){vp_great_work_city(world,city.owner,city.id).building_by_class.reset();}
} // namespace civ5::runtime
