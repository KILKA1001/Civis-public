#include "civ5/runtime/vp_unit_supply.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
constexpr auto layout=saves::GameStateLayout::vp542_corpus;
std::int64_t id64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("Supply ID/count exceeds int64");return static_cast<std::int64_t>(n);}
std::int64_t add(std::int64_t a,std::int64_t b){return wide::add(a,b);}
std::int64_t mul(std::int64_t a,std::int64_t b){return wide::multiply_divide(a,b,1);}
std::int64_t percent(std::int64_t a,std::int64_t b){return wide::multiply_divide(a,b,100);}
std::int64_t value(const auto &map,std::int64_t id){const auto i=map.find(id);return i==map.end()?0:i->second;}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){const T *found=nullptr;
    for(const auto &x:v)if(x.name&&std::string_view(x.name)==name){if(found)throw std::runtime_error("Duplicate supply field");found=&x;}
    if(!found)throw std::runtime_error("Missing supply field: "+std::string(name));return *found;}
std::int64_t scalar(const auto &v,const char *name){const auto &a=named(v,name).values;if(a.size()!=1)throw std::runtime_error("Invalid supply scalar");return a[0];}
std::int64_t define(const VpUnitSupplyContext &c,const char *name){const auto *v=c.defines.find(name);
    if(!v||!v->lookup.error.empty())throw std::runtime_error("Unresolved supply define: "+std::string(name));return std::get<std::int64_t>(v->lookup.value);}
void validate(const VpUnitSupplyContext &c){
    if(c.units.profile!=vp||c.players.profile!=vp||c.cities.profile!=vp||c.diplomacy.profile!=vp||
       c.defines.profile()!=content::GlobalDefineProfile::vp542||!c.options.all_succeeded())
        throw std::runtime_error("Supply requires resolved shared VP world");
    (void)vp_world_city_supply_modifier(c.units);
}
const auto &state(const VpUnitSupplyContext &c,std::int64_t id){return vp_unit_supply_player(c.units,id);}
bool major(const VpUnitSupplyContext &c,std::int64_t id){return c.diplomacy.team_is_major(c.players,c.players.players.at(id).team);}
bool minor(const VpUnitSupplyContext &c,std::int64_t id){return c.diplomacy.team_is_minor(c.players,c.players.players.at(id).team);}
bool human(const VpUnitSupplyContext &c,std::int64_t id){return c.players.is_human(c.players.players.at(id))&&!c.options.enabled("HUMAN_USES_AI_HANDICAP");}
const VpUnitSupplyRules::Handicap &handicap(const VpUnitSupplyContext &c,std::int64_t id){
    const auto i=c.rules.handicaps.find(id);static const VpUnitSupplyRules::Handicap empty{};return i==c.rules.handicaps.end()?empty:i->second;}
const VpUnitSupplyRules::Bonus &player_handicap(const VpUnitSupplyContext &c,std::int64_t id){const auto &p=c.players.players.at(id);
    return handicap(c,c.players.is_human(p)&&c.options.enabled("HUMAN_USES_AI_HANDICAP")?define(c,"AI_HANDICAP"):p.handicap).player;}
std::int64_t tech_progress(const VpUnitSupplyContext &c,std::int64_t player){
    const auto team=c.players.players.at(player).team;const TeamTechnologies *found=nullptr;
    for(const auto &t:c.technologies)if(t.team()==team){if(found)throw std::runtime_error("Duplicate supply technology team");found=&t;}
    if(!found||found->profile()!=vp||!found->slots())throw std::runtime_error("Unresolved supply technology domain");
    return std::clamp<std::int64_t>(wide::multiply_divide(found->known_count(),100,id64(found->slots())),0,100);
}
std::int64_t tech_reduce(const VpUnitSupplyContext &c,std::int64_t player,std::int64_t supply,const char *name){
    const auto reduction=percent(tech_progress(c,player),define(c,name));return wide::multiply_divide(supply,100,std::max<std::int64_t>(1,add(100,reduction)));}
std::int64_t effective_cities(const VpUnitSupplyContext &c,std::int64_t id){
    const auto &cities=c.cities.players.at(id).cities;const auto &research=c.diplomacy.players.at(id).cities;
    if(cities.size()!=research.size())throw std::runtime_error("Supply city roster mismatch");auto n=id64(cities.size());
    for(const auto &[key,city]:cities){if(city.puppet)--n;if(research.at(key).ignore_happiness)--n;}
    return std::max<std::int64_t>(1,n);
}
}
VpUnitSupplyRules load_vp_unit_supply_rules(const content::RulesDatabase &db,const content::InfoTypeCatalog &types){
    if(!types.report().all_succeeded())throw std::runtime_error("Supply requires resolved types");VpUnitSupplyRules r;
    const auto load=[&](const char *table,const auto &fn){const auto q=db.prefetch_info(table,fn);if(!q.all_succeeded())throw std::runtime_error("Supply Info: "+q.error);};
    load("HandicapInfos",[&](auto id,auto,const auto &row){auto &h=r.handicaps[id64(id)];
        const auto bonus=[&](const char *prefix,VpUnitSupplyRules::Bonus &b){const auto get=[&](const char *suffix){return row.integer(std::string(prefix)+"UnitSupply"+suffix);};
            b={get("Base"),get("PerCity"),get("PopulationPercent"),get("PerEraFlat"),get("PerEraModifier"),get("BonusPercent")};};
        bonus("",h.player);bonus("AI",h.ai);bonus("CityState",h.minor);
        h.player.base=add(h.player.base,row.integer("ProductionFreeUnits"));h.player.city=add(h.player.city,row.integer("ProductionFreeUnitsPerCity"));
        h.player.population=add(h.player.population,row.integer("ProductionFreeUnitsPopulationPercent"));h.ai.percent=add(h.ai.percent,row.integer("AIUnitSupplyPercent"));return true;});
    load("Eras",[&](auto id,auto,const auto &row){r.era_base[id64(id)]=row.integer("UnitSupplyBase");return true;});
    load("MinorCivilizations",[&](auto id,auto,const auto &row){const auto text=row.text("MinorCivTrait");const auto *t=text?types.find(*text):nullptr;r.minor_traits[id64(id)]=t?t->id:-1;r.minor_types[id64(id)]=row.text("Type").value_or("");return true;});return r;
}
void restore_vp_unit_supply(const saves::SavedSessionSnapshot &s,const content::InfoTypeCatalog &types,
    const ResearchContext &setup,PlayerResearchContext &dip,CityYieldContext &cities,MaintenanceContext &units){
    if(setup.profile!=vp||dip.profile!=vp||cities.profile!=vp||units.profile!=vp||!s.pregame||!s.all_players||!s.teams||
       s.pregame->layout!=layout||s.all_players->layout!=layout||s.teams->layout!=layout)throw std::runtime_error("Supply import profile mismatch");
    VpUnitSupplyProfileState supply;
    supply.world_city_supply_modifier=s.pregame->world.values.integers.at("num_cities_unit_supply_mod");
    const auto &minor_names=s.pregame->archive.string_arrays.at("minor_civ_types");std::set<std::int64_t> seen;auto &city_supply=ensure_vp_city_supply_profile(cities);
    for(const auto &sp:s.all_players->players){const auto id=sp.saved_player_id;
        if(!seen.insert(id).second||sp.core.layout!=layout||sp.traits.layout!=layout||sp.planning.layout!=layout||sp.cities.layout!=layout||scalar(sp.core.fields,"m_eID")!=id||!setup.players.contains(id)||!units.players.contains(id))
            throw std::runtime_error("Supply player binding mismatch");
        VpUnitSupplyProfileState::Player a;const auto core=[&](const char *n){return scalar(sp.core.fields,n);};const auto trait=[&](const char *n){return scalar(sp.traits.fields,n);};
        a.military=core("m_iNumMilitaryUnits");a.free=core("m_iSupplyFreeUnits");a.extra_flat=core("m_iExtraSupplyFlat");a.city_flat=core("m_iCitySupplyFlatGlobal");
        a.extra_population=core("m_iExtraSupplyPerPopulation");a.expended=core("m_iUnitSupplyFromExpendedGP");a.puppet=core("m_iPuppetYieldAndSupplyModifierChange");
        a.war_value=core("m_iCachedCurrentWarValue");a.war_modifier=core("m_iWarWearinessModifier");a.trait_war_modifier=trait("m_iWarWearinessModifier");
        a.trait_base=trait("m_iExtraSupply");a.trait_city=trait("m_iExtraSupplyPerCity");a.trait_population=trait("m_iExtraSupplyPerPopulation");a.trait_puppet=trait("m_iPuppetYieldAndSupplyModifierChange");
        const auto &weariness=named(sp.core.fields,"m_aiWarWeariness").values;
        for(auto other:dip.major_players){if(other<0||static_cast<std::uint64_t>(other)>=weariness.size())throw std::runtime_error("Supply war weariness wire span mismatch");
            if(weariness[other])a.war_weariness[other]=weariness[other];}
        if(id<0||static_cast<std::uint64_t>(id)>=minor_names.size())throw std::runtime_error("Supply minor pregame wire index");
        if(const auto *t=types.find(minor_names[id]))a.minor_type=t->id;
        a.personality=named(named(sp.planning.blocks,"minor_civ_ai").fields,"m_ePersonality").value;supply.players[id]=std::move(a);
        std::set<std::int64_t> city_ids;auto &pc=cities.players.at(id);
        for(const auto &sc:sp.cities.cities){const auto cid=scalar(sc.core.fields,"m_iID");auto &city=pc.cities.at(cid);
            if(!city_ids.insert(cid).second||sc.core.layout!=layout||city.id!=cid||city.owner!=id||scalar(sc.core.fields,"m_eOwner")!=id||
               city.source_core!=std::optional{std::pair{sc.core.start_offset,sc.core.end_offset}})throw std::runtime_error("Supply city binding mismatch");
            city_supply.cities[{id,cid}]={scalar(sc.core.fields,"m_iCitySupplyFlat"),scalar(sc.core.fields,"m_iCitySupplyModifier")};}
        if(city_ids.size()!=pc.cities.size())throw std::runtime_error("Incomplete supply cities");
    }
    if(seen.size()!=units.players.size()||seen.size()!=cities.players.size())throw std::runtime_error("Incomplete supply players");seen.clear();
    for(const auto &st:s.teams->teams){const auto id=named(st.fields,"id").value;auto &t=dip.teams.at(id);
        if(!seen.insert(id).second)throw std::runtime_error("Duplicate supply team");ResearchTeamState::WarPeaceState a;
        const auto &locked=named(st.arrays,"turns_locked_at_war").values,&permanent=named(st.arrays,"permanent_war_peace").values;
        for(const auto &[other,unused]:dip.teams){if(other<0||static_cast<std::uint64_t>(other)>=locked.size()||static_cast<std::uint64_t>(other)>=permanent.size())throw std::runtime_error("Supply war/peace wire span mismatch");
            if(locked[other])a.locked_turns[other]=locked[other];if(permanent[other])a.permanent.insert(other);}
        t.war_peace=std::move(a);
    }
    if(seen.size()!=dip.teams.size())throw std::runtime_error("Incomplete supply teams");
    units.extensions.emplace<VpUnitSupplyProfileState>(std::move(supply));
}
bool vp_can_change_war_peace(VpUnitSupplyContext &c,std::int64_t from,std::int64_t to){
    validate(c);if(to==-1||to==from)return false;
    for(const char *name:{"GAMEOPTION_NO_CHANGING_WAR_PEACE","GAMEOPTION_ALWAYS_WAR","GAMEOPTION_ALWAYS_PEACE"})
        if(c.players.game_option_value(c.database,name)!=0)return false;
    const auto &a=c.diplomacy.teams.at(from),&b=c.diplomacy.teams.at(to);if(a.master!=-1)return false;
    if(!a.war_peace||!b.war_peace)throw std::runtime_error("Unresolved war/peace constraints");
    return value(a.war_peace->locked_turns,to)<=0&&value(b.war_peace->locked_turns,from)<=0&&
        !a.war_peace->permanent.contains(to)&&!b.war_peace->permanent.contains(from);
}
std::int64_t vp_highest_war_weariness_percent(VpUnitSupplyContext &c,std::int64_t id){
    validate(c);if(!c.options.enabled("BALANCE_VP")||!major(c,id))return 0;
    const auto &p=state(c,id);const auto team=c.players.players.at(id).team;const auto &t=c.diplomacy.teams.at(team);std::int64_t highest=0;
    for(auto other:c.diplomacy.major_players){const auto theirs=c.players.players.at(other).team;
        if(theirs==team||!t.met.contains(theirs)||!major(c,other))continue;
        const auto weary=value(p.war_weariness,other);if(weary<=highest)continue;
        if(t.at_war.contains(theirs)&&!vp_can_change_war_peace(c,team,theirs))continue;highest=weary;}
    const auto ratio=wide::multiply_divide(highest,100,std::max<std::int64_t>(1,p.war_value));
    const auto modifier=std::min<std::int64_t>(100,add(p.war_modifier,p.trait_war_modifier));
    return std::min<std::int64_t>(100,percent(ratio,wide::subtract(100,modifier)));
}
VpUnitSupplyComponents vp_unit_supply_components(const VpUnitSupplyContext &c,std::int64_t id,bool ignore){
    validate(c);VpUnitSupplyComponents q;if(!c.players.players.at(id).alive)return q;
    const auto &p=state(c,id);const auto &cities=c.cities.players.at(id).cities;const bool is_major=major(c,id),is_minor=minor(c,id);
    const bool special_minor=c.options.enabled("BALANCE_MINOR_UNIT_SUPPLY_HANDICAP"),is_human=human(c,id);
    const auto &h=player_handicap(c,id);const auto &gh=handicap(c,c.players.game_handicap);const auto era=c.players.current_era;
    if(is_major)q.handicap=add(add(p.trait_base,value(c.rules.era_base,c.players.start_era)),add(h.base,mul(h.era_flat,era)));
    if(is_major&&!is_human)q.handicap=add(q.handicap,add(gh.ai.base,mul(gh.ai.era_flat,era)));
    else if(is_minor)q.handicap=add(gh.minor.base,mul(gh.minor.era_flat,era));
    if(!ignore&&(!special_minor||is_major))q.handicap=wide::subtract(q.handicap,percent(c.units.team_eras.at(c.players.players.at(id).team),define(c,"UNIT_SUPPLY_BASE_TECH_REDUCTION_PER_ERA")));
    q.handicap=std::max<std::int64_t>(0,q.handicap);
    std::int64_t city_start=0,pop_start=0;
    if(is_major){q.cities=mul(p.extra_flat,100);city_start=add(add(p.trait_city,p.city_flat),h.city);pop_start=add(add(p.trait_population,p.extra_population),h.population);
        if(!is_human){city_start=add(city_start,gh.ai.city);pop_start=add(pop_start,gh.ai.population);}}
    else if(is_minor){city_start=gh.minor.city;pop_start=add(special_minor?0:p.extra_population,gh.minor.population);
        if(special_minor)q.cities=mul(mul(city_start,id64(cities.size())),100);else city_start=add(city_start,p.city_flat);}
    if(is_major||is_minor)for(const auto &[cid,city]:cities){const auto &s=vp_city_supply(c.cities,id,cid);
        if(is_major||!special_minor){auto mod=std::int64_t{100};if(city.puppet){mod=add(define(c,"PUPPET_YIELD_AND_SUPPLY_MODIFIER_MULTIPLICATIVE"),p.puppet);if(is_major)mod=add(mod,p.trait_puppet);}
            q.cities=add(q.cities,mul(add(city_start,s.flat),mod));}
        auto pop_mod=std::int64_t{100};if(city.puppet){pop_mod=define(c,"PUPPET_YIELD_AND_SUPPLY_MODIFIER_MULTIPLICATIVE");if(is_major)pop_mod=add(add(pop_mod,p.trait_puppet),p.puppet);}
        q.population=add(q.population,percent(mul(city.population,pop_mod),add(pop_start,is_minor&&special_minor?0:s.population_modifier)));}
    q.cities/=100; // Deliberately before reduction; population truncates after it.
    if(!ignore&&(!special_minor||is_major)){q.cities=tech_reduce(c,id,q.cities,"UNIT_SUPPLY_CITIES_TECH_REDUCTION_MULTIPLIER");q.population=tech_reduce(c,id,q.population,"UNIT_SUPPLY_POPULATION_TECH_REDUCTION_MULTIPLIER");}
    q.cities=std::max<std::int64_t>(0,q.cities);q.population=std::max<std::int64_t>(0,q.population/100);
    q.expended=special_minor&&is_minor?0:p.expended;return q;
}
VpUnitSupplyQuote vp_unit_supply(VpUnitSupplyContext &c,std::int64_t id,bool check_war){
    validate(c);VpUnitSupplyQuote q;const auto &u=c.units.players.at(id);if(u.barbarian){q.unlimited=true;return q;}
    if(!c.players.players.at(id).alive)return q;const auto &p=state(c,id);q.units=wide::subtract(p.military,p.free);
    if(p.base_cache&&(!check_war||p.war_cache)){q.cached=true;q.base=*p.base_cache;q.capacity=check_war?*p.war_cache:*p.base_cache;}
    else{
        q.components=vp_unit_supply_components(c,id);auto n=add(add(q.components.handicap,q.components.cities),add(q.components.population,q.components.expended));
        const auto &cities=c.cities.players.at(id).cities;const bool is_major=major(c,id),is_minor=minor(c,id);
        if(!cities.empty()){q.effective_cities=effective_cities(c,id);q.city_reduction=std::max<std::int64_t>(0,mul(q.effective_cities,vp_world_city_supply_modifier(c.units)));}
        n=wide::multiply_divide(n,100,add(100,q.city_reduction));const auto &h=player_handicap(c,id),&ai=handicap(c,c.players.game_handicap).ai,&cs=handicap(c,c.players.game_handicap).minor;
        if(is_major){q.empire_modifier=add(h.percent,mul(h.era_modifier,c.players.current_era));
            if(!human(c,id))q.empire_modifier=add(q.empire_modifier,add(ai.percent,mul(ai.era_modifier,c.players.current_era)));}
        else if(is_minor)q.empire_modifier=add(cs.percent,mul(cs.era_modifier,c.players.current_era));
        if(is_major||is_minor)n=percent(n,add(100,q.empire_modifier));
        if(is_minor){const auto t=c.rules.minor_traits.at(p.minor_type);
            const char *traits[]={"CULTURED","MILITARISTIC","MARITIME","MERCANTILE","RELIGIOUS"};const char *personalities[]={"FRIENDLY","NEUTRAL","HOSTILE","IRRATIONAL"};
            if(t<0||t>=5||p.personality<0||p.personality>=4){q.minor_unready=true;q.over=std::max<std::int64_t>(0,q.units);return q;}
            const auto a="MINOR_CIV_UNIT_SUPPLY_MODIFIER_"+std::string(traits[t]),b="MINOR_CIV_UNIT_SUPPLY_MODIFIER_"+std::string(personalities[p.personality]);
            q.minor_modifier=add(define(c,a.c_str()),define(c,b.c_str()));n=percent(n,add(100,q.minor_modifier));
            if(cities.size()>1){q.extra_city_modifier=mul(id64(cities.size()-1),define(c,"MINOR_CIV_UNIT_SUPPLY_MULTIPLIER_PER_EXTRA_CITY"));n=percent(n,add(100,q.extra_city_modifier));}}
        q.base=std::max<std::int64_t>(0,n);auto war=q.base;
        if(c.options.enabled("BALANCE_VP")&&is_major){q.war_percent=percent(vp_highest_war_weariness_percent(c,id),std::clamp<std::int64_t>(define(c,"UNIT_SUPPLY_WAR_WEARINESS_PERCENT_REDUCTION"),0,100));
            war=percent(war,wide::subtract(100,q.war_percent));}
        auto &cache=vp_unit_supply_player(c.units,id);cache.base_cache=q.base;cache.war_cache=std::max<std::int64_t>(0,war);q.capacity=check_war?*cache.war_cache:*cache.base_cache;
    }
    q.over=std::max<std::int64_t>(0,wide::subtract(q.units,q.capacity));return q;
}
void invalidate_vp_unit_supply(MaintenanceContext &c,std::int64_t id){
    auto &p=vp_unit_supply_player(c,id);p.base_cache.reset();p.war_cache.reset();}
} // namespace civ5::runtime
