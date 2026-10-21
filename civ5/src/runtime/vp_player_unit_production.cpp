#include "civ5/runtime/vp_player_unit_production.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <stdexcept>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
constexpr auto layout=saves::GameStateLayout::vp542_corpus;
std::int64_t id64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("Unit production ID exceeds int64");return static_cast<std::int64_t>(n);}
std::int64_t value(const YieldValues &a,std::int64_t id){const auto i=a.find(id);return i==a.end()?0:i->second;}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){const T *found=nullptr;
    for(const auto &x:v)if(x.name&&std::string_view(x.name)==name){if(found)throw std::runtime_error("Duplicate unit production field");found=&x;}
    if(!found)throw std::runtime_error("Missing unit production field: "+std::string(name));return *found;}
std::int64_t scalar(const auto &v,const char *name){const auto &a=named(v,name).values;if(a.size()!=1)throw std::runtime_error("Invalid unit production scalar");return a[0];}
const VpUnitProductionProfileState::Player &validate(const VpPlayerUnitProductionContext &c,std::int64_t owner){
    const auto &s=c.production.supply;const auto &t=c.traits;
    const auto *profile=find_vp_unit_production_profile(s.cities);
    if(s.cities.profile!=vp||s.players.profile!=vp||s.diplomacy.profile!=vp||&t.players!=&s.players||&t.diplomacy!=&s.diplomacy||
       &t.religion.religion.water.cities!=&s.cities||!s.cities.players.contains(owner)||!profile||!profile->players.contains(owner))
        throw CityYieldUnavailable("Unit production requires resolved shared VP state");
    return profile->players.at(owner);
}
std::int64_t active_wars(const VpPlayerUnitProductionContext &c,std::int64_t owner){
    const auto &base=c.production.supply.players;const auto &dip=c.production.supply.diplomacy;
    const auto &me=base.players.at(owner);const auto &wars=dip.teams.at(me.team).at_war;
    std::int64_t count=0;for(auto other:dip.major_players){const auto &p=base.players.at(other);
        if(p.alive&&wars.contains(p.team))count=wide::add(count,1);}
    return count;
}
}
VpPlayerUnitProductionRules load_vp_player_unit_production_rules(const content::RulesDatabase &db,const content::InfoTypeCatalog &types){
    VpPlayerUnitProductionRules r;const auto type=[&](const auto &v,const char *field){const auto s=v.text(field);if(s)if(const auto *x=types.find(*s))return x->id;return std::int64_t{-1};};
    const auto traits=db.prefetch_info("Traits",[&](auto,auto,const auto &){return true;});
    const auto combats=db.prefetch_info("UnitCombatInfos",[&](auto,auto,const auto &){return true;});
    if(!traits.all_succeeded()||!combats.all_succeeded())throw CityYieldUnavailable("Unresolved unit production trait/combat Info");
    r.trait_slots=traits.slots;r.combat_slots=combats.slots;
    const auto fill=[&](std::vector<std::int64_t> &target,const char *table,const char *key,const std::string &name){
        target.assign(r.trait_slots,0);const auto sql=std::string("select Traits.ID, Trait from ")+table+" inner join Traits on TraitType = Traits.Type where "+key+" = ?";
        const auto q=db.query_rows({sql,{content::DatabaseValue(name)}},[&](const auto &row){const auto id=row.integer_at(0);
            if(id<0||static_cast<std::uint64_t>(id)>=r.trait_slots)throw CityYieldUnavailable("Invalid unit production trait ID");target[id]=row.integer_at(1);});
        if(!q.all_succeeded())throw CityYieldUnavailable("Unresolved unit production trait relation: "+q.error);
    };
    const auto specials=db.prefetch_info("SpecialUnits",[&](auto id,auto,const auto &row){
        const auto name=row.text("Type");if(!name)throw CityYieldUnavailable("Missing special unit type");
        fill(r.special_trait_modifiers[id64(id)],"SpecialUnit_ProductionTraits","SpecialUnitType",std::string(*name));return true;});
    const auto units=db.prefetch_info("Units",[&](auto id,auto,const auto &row){
        const auto name=row.text("Type");if(!name)throw CityYieldUnavailable("Missing unit type");auto &u=r.units[id64(id)];
        u.military=row.boolean("MilitaryProduction");u.found=row.boolean("Found");u.combat=type(row,"CombatClass");u.special=type(row,"Special");
        fill(u.trait_modifiers,"Unit_ProductionTraits","UnitType",std::string(*name));return true;});
    if(!specials.all_succeeded()||!units.all_succeeded())throw CityYieldUnavailable("Unresolved unit production Info");return r;
}
void restore_vp_player_unit_production(const saves::SavedSessionSnapshot &s,const VpPlayerUnitProductionRules &r,CityYieldContext &world){
    if(world.profile!=vp||!s.all_players||s.all_players->layout!=layout)throw CityYieldUnavailable("Unit production import requires VP state");
    auto &profile=ensure_vp_unit_production_profile(world);profile.players.clear();
    std::set<std::int64_t> seen;for(const auto &sp:s.all_players->players){const auto owner=sp.saved_player_id;
        if(!world.players.contains(owner))throw std::runtime_error("Missing unit production player");
        if(!seen.insert(owner).second||sp.core.layout!=layout||scalar(sp.core.fields,"m_eID")!=owner)throw std::runtime_error("Unit production player binding mismatch");
        VpUnitProductionProfileState::Player a;a.military=scalar(sp.core.fields,"m_iMilitaryProductionModifier");
        a.military_per_major_war=scalar(sp.core.fields,"m_iMilitaryProductionModPerMajorWar");a.settler=scalar(sp.core.fields,"m_iSettlerProductionModifier");
        const auto &v=named(sp.core.fields,"m_paiUnitCombatProductionModifiers").values;
        if(v.size()!=r.combat_slots)throw std::runtime_error("Unit combat production wire span mismatch");
        for(std::size_t i=0;i<v.size();++i)if(v[i])a.combat[id64(i)]=v[i];profile.players[owner]=std::move(a);
    }if(seen.size()!=world.players.size())throw std::runtime_error("Incomplete unit production players");
}
VpPlayerUnitProductionQuote vp_player_unit_production(const VpPlayerUnitProductionContext &c,std::int64_t owner,std::int64_t id){
    VpPlayerUnitProductionQuote q;q.general=vp_player_general_production(c.production,owner);
    const auto i=c.rules.units.find(id);if(i==c.rules.units.end()){q.total=q.general.total;return q;}
    const auto &p=validate(c,owner);const auto &u=i->second;
    if(u.military){q.military=p.military;q.major_wars=active_wars(c,owner);q.war_military=wide::multiply_divide(p.military_per_major_war,q.major_wars,1);}
    if(u.found)q.settler=p.settler;if(u.combat!=-1)q.combat=value(p.combat,u.combat);
    const auto &potential=vp_potential_unit_traits(c.traits,owner);
    for(std::size_t position=0;position<potential.size();++position)if(vp_unit_trait_active(c.traits,owner,potential[position])){
        if(position>=u.trait_modifiers.size())throw CityYieldUnavailable("Unit production trait position exceeds source slots");
        q.unit_traits=wide::add(q.unit_traits,u.trait_modifiers[position]);
        if(u.special!=-1){const auto found=c.rules.special_trait_modifiers.find(u.special);
            if(found==c.rules.special_trait_modifiers.end())continue;const auto &special=found->second;
            if(position>=special.size())throw CityYieldUnavailable("Special unit trait position exceeds source slots");
            q.special_traits=wide::add(q.special_traits,special[position]);}}
    for(auto n:{q.general.total,q.military,q.war_military,q.settler,q.combat,q.unit_traits,q.special_traits})q.total=wide::add(q.total,n);return q;
}
} // namespace civ5::runtime
