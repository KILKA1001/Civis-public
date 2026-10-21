#include "civ5/runtime/maintenance.hpp"
#include "civ5/runtime/vp_maintenance_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>
#include <cmath>

namespace civ5::runtime {
namespace {
using content::TechnologyProfile;
std::int64_t i64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("Maintenance size exceeds int64");return static_cast<std::int64_t>(n);}
std::int64_t val(const YieldValues &v,std::int64_t id){const auto i=v.find(id);return i==v.end()?0:i->second;}
template<class T> const T &named(const std::vector<T> &v,std::string_view name) {
    const T *p=nullptr;for(const auto &f:v)if(f.name && std::string_view(f.name)==name){if(p)throw std::runtime_error("Duplicate maintenance field: "+std::string(name));p=&f;}
    if(!p)throw std::runtime_error("Missing maintenance field: "+std::string(name));return *p;
}
template<class T> std::int64_t scalar(const std::vector<T> &v,std::string_view name) {
    const auto &a=named(v,name).values;if(a.size()!=1)throw std::runtime_error("Invalid maintenance scalar: "+std::string(name));return a.front();
}
YieldValues sparse(const std::vector<std::int64_t> &v){YieldValues r;for(std::size_t i=0;i<v.size();++i)if(v[i])r[i64(i)]=v[i];return r;}
const content::GlobalDefineValue &def(const content::GlobalDefineCache &d,std::string_view name) {
    const auto *p=d.find(name);if(!p || !p->lookup.error.empty())throw std::runtime_error("Missing maintenance define: "+std::string(name));return p->lookup.value;
}
std::int64_t integer(const content::GlobalDefineCache &d,std::string_view name){return std::get<std::int64_t>(def(d,name));}
double real(const content::GlobalDefineCache &d,std::string_view name){return std::get<double>(def(d,name));}
std::int64_t mul(std::int64_t a,std::int64_t b){return wide::multiply_divide(a,b,1);}
std::int64_t percent(std::int64_t a,std::int64_t b){return wide::multiply_divide(a,b,100);}
std::int64_t mod(std::int64_t a,std::int64_t b){return percent(a,wide::add(100,b));}
std::int64_t truncate(double n){if(!std::isfinite(n)||n < -0x1p63||n>=0x1p63)throw std::overflow_error("Maintenance real exceeds int64");return static_cast<std::int64_t>(n);}
bool opt(const content::CustomModOptions *o,std::string_view name){return o && o->enabled(name);}
bool human(const MaintenanceWorld &w,const content::CustomModOptions *o,std::int64_t id){return w.setup.is_human(w.setup.players.at(id))&&!opt(o,"HUMAN_USES_AI_HANDICAP");}
bool human_teammate(const MaintenanceWorld &w,std::int64_t id) {
    const auto team=w.setup.players.at(id).team;
    for(const auto &[other,p]:w.setup.players)if(w.setup.is_human(p)&&p.alive&&p.team==team)return true;return false;
}
const MaintenanceHandicap &handicap(const MaintenanceRules &r,std::int64_t id) {
    const auto i=r.handicaps.find(id);if(i!=r.handicaps.end())return i->second;
    if(r.profile==TechnologyProfile::bnw)throw MaintenanceUnavailable("Missing BNW maintenance handicap");
    static const MaintenanceHandicap empty{};return empty; // VP empty CvHandicapInfo fallback
}
const MaintenanceHandicap &player_handicap(const MaintenanceRules &r,const MaintenanceWorld &w,
    const content::GlobalDefineCache &d,const content::CustomModOptions *o,std::int64_t id) {
    const auto &p=w.setup.players.at(id);
    return handicap(r,w.setup.is_human(p)&&opt(o,"HUMAN_USES_AI_HANDICAP")?integer(d,"AI_HANDICAP"):p.handicap);
}
void validate(const MaintenanceContext &s,const MaintenanceWorld &w,const MaintenanceRules &r,
    const content::GlobalDefineCache &d,const content::CustomModOptions *o) {
    const bool vp=r.profile==TechnologyProfile::vp542;
    if(s.profile!=r.profile||w.setup.profile!=r.profile||w.diplomacy.profile!=r.profile||w.cities.profile!=r.profile||
       d.profile()!=(vp?content::GlobalDefineProfile::vp542:content::GlobalDefineProfile::bnw)||
       (vp&&(!o||!o->all_succeeded()))||(!vp&&o))throw std::runtime_error("Maintenance profile/rules mismatch");
}
bool major(const MaintenanceWorld &w,std::int64_t id){return w.diplomacy.team_is_major(w.setup,w.setup.players.at(id).team);}
bool vassal(const MaintenanceWorld &w,std::int64_t id){return w.diplomacy.teams.at(w.setup.players.at(id).team).master!=-1;}
std::int64_t free_units(const MaintenancePlayer &p,const MaintenanceWorld &w,const MaintenanceRules &r,
    const content::GlobalDefineCache &d,const content::CustomModOptions *o,std::int64_t id,std::int64_t domain=-1,bool military=false) {
    const bool vp=r.profile==TechnologyProfile::vp542;
    std::int64_t n=0;
    // VP counts handicap free units inside this getter AND again in its caller.
    if(vp&&major(w,id))n=wide::add(player_handicap(r,w,d,o,id).free_units,human(w,o,id)?0:handicap(r,w.setup.game_handicap).ai_free_units);
    for(const auto &[unit,u]:p.units) {
        if(domain!=-1&&u.domain!=domain)continue;
        if(military&&(vp?u.civilian:!u.military))continue;
        if(u.no_maintenance)n=wide::add(n,1);
        else if(p.free_garrison) {
            if(!u.garrison)throw MaintenanceUnavailable("BNW free garrison needs the live plot best-defender query");
            if(*u.garrison)n=wide::add(n,1);
        }
        if(vp&&opt(o,"BALANCE_CORE_JFD")&&vp_maintenance_profile(p).contract_units.contains(unit))n=wide::add(n,1); // intentionally additive
    }
    return n;
}
std::int64_t count_domain(const MaintenancePlayer &p,std::int64_t domain){std::int64_t n=0;for(const auto &[id,u]:p.units)if(u.domain==domain&&u.military)n=wide::add(n,1);return n;}
std::int64_t unit_supply(const MaintenanceContext &s,const MaintenanceWorld &w,const MaintenanceRules &r,
    const content::GlobalDefineCache &d,std::int64_t id) {
    const auto &p=s.players.at(id);const auto &b=w.setup.players.at(id);
    auto n=std::max<std::int64_t>(0,wide::subtract(p.outside_units,integer(d,"INITIAL_FREE_OUTSIDE_UNITS")));
    n=percent(n,integer(d,"INITIAL_OUTSIDE_UNIT_GOLD_PERCENT"));n=percent(n,handicap(r,b.handicap).unit);
    if(!w.setup.is_human(b)&&!human_teammate(w,id)&&!p.barbarian)
        n=percent(n,std::max<std::int64_t>(0,wide::add(100,mul(handicap(r,w.setup.game_handicap).ai_per_era,s.team_eras.at(b.team)))));
    if(!s.estimate_end_turn||!integer(d,"UNIT_MAINTENANCE_GAME_EXPONENT_DIVISOR"))throw MaintenanceUnavailable("Zero maintenance progress divisor");
    const double progress=static_cast<float>(s.elapsed_turns)/static_cast<float>(s.estimate_end_turn);
    const double multiplier=1.0+progress*integer(d,"UNIT_MAINTENANCE_GAME_MULTIPLIER");
    const double exponent=1.0+progress/integer(d,"UNIT_MAINTENANCE_GAME_EXPONENT_DIVISOR");
    return mod(truncate(std::pow(multiplier*static_cast<double>(n),exponent)),p.supply_modifier);
}
}
MaintenanceRules load_maintenance_rules(const content::RulesDatabase &db,TechnologyProfile profile,const content::InfoTypeCatalog &types) {
    MaintenanceRules r;r.profile=profile;const bool vp=profile==TechnologyProfile::vp542;
    if(!types.report().all_succeeded())throw std::runtime_error("Incomplete maintenance type catalog");
    const auto info=[&](const char *table,const auto &f){const auto q=db.prefetch_info(table,f);if(!q.all_succeeded())throw std::runtime_error("Maintenance Info: "+q.error);};
    info("HandicapInfos",[&](auto slot,auto,const auto &row){
        MaintenanceHandicap h;h.free_units=row.integer("GoldFreeUnits");
        if(vp)h.free_units=wide::add(h.free_units,row.integer("MaintenanceFreeUnits"));
        h.ai_free_units=row.integer("AIMaintenanceFreeUnits");h.unit=row.integer("UnitCostPercent");h.ai_unit=row.integer("AIUnitCostPercent");
        h.building=row.integer("BuildingCostPercent");h.ai_building=row.integer("AIBuildingCostPercent");
        h.improvement=row.integer(vp?"ImprovementCostPercent":"RouteCostPercent");
        if(vp&&row.integer("RouteCostPercent"))h.improvement=percent(h.improvement,row.integer("RouteCostPercent"));
        h.ai_improvement=row.integer("AIImprovementCostPercent");h.inflation=row.integer("InflationPercent");
        h.ai_inflation=row.integer("AIInflationPercent");h.ai_per_era=row.integer("AIPerEraModifier");r.handicaps[i64(slot)]=h;return true;
    });
    info("Eras",[&](auto slot,auto,const auto &row){r.era_building_percent[i64(slot)]=row.integer("BuildingMaintenancePercent");return true;});
    const auto resolve=[&](const auto &name){const auto *v=types.find(name.value_or(""));return v?v->id:-1;};
    info("Units",[&](auto slot,auto,const auto &row){r.units[i64(slot)]={resolve(row.text("Domain")),resolve(row.text("CombatClass")),
        row.integer("Range"),row.integer("NukeDamageLevel"),row.integer("NoMaintenance")!=0};return true;});
    info("UnitCombatInfos",[&](auto slot,auto,const auto &){r.combat_classes.insert(i64(slot));return true;});
    if(vp){auto &profile=ensure_vp_maintenance_rules_profile(r);
        info("Contracts",[&](auto slot,auto,const auto &){profile.contracts.insert(i64(slot));return true;});}
    return r;
}
MaintenanceContext restore_maintenance(const saves::SavedSessionSnapshot &snap,const MaintenanceWorld &w,
    const MaintenanceRules &r,const content::InfoTypeCatalog &types) {
    const bool vp=r.profile==TechnologyProfile::vp542;
    const auto layout=vp?saves::GameStateLayout::vp542_corpus:saves::GameStateLayout::expansion2_version1;
    if(w.setup.profile!=r.profile||w.diplomacy.profile!=r.profile||w.cities.profile!=r.profile||
       !snap.all_players||!snap.teams||!snap.map_plots||snap.game_state.layout!=layout||snap.all_players->layout!=layout||
       snap.teams->layout!=layout||snap.map_plots->layout!=layout||!types.report().all_succeeded()||
       (vp!=std::holds_alternative<saves::VpPrefixObservation>(snap.game_state.prefix)))
        throw std::runtime_error("Maintenance needs matching complete saved state");
    MaintenanceContext s;s.profile=r.profile;
    std::visit([&](const auto &prefix){for(std::size_t i=0;i<prefix.scalar_names.size();++i) {
        const std::string_view name=prefix.scalar_names[i];
        if(name=="elapsed_game_turns")s.elapsed_turns=prefix.scalars[i];
        if(name=="estimate_end_turn")s.estimate_end_turn=prefix.scalars[i];
        if(name=="default_estimate_end_turn")s.default_end_turn=prefix.scalars[i];
    }},snap.game_state.prefix);
    for(const auto &t:snap.teams->teams) {
        const auto id=named(t.fields,"id").value;
        if(!w.diplomacy.teams.contains(id)||!s.team_eras.emplace(id,named(t.fields,"era").value).second)
            throw std::runtime_error("Maintenance team identity mismatch");
    }
    if(s.team_eras.size()!=w.diplomacy.teams.size())throw std::runtime_error("Incomplete maintenance team roster");
    if(vp){if(snap.final_systems.layout!=layout||!snap.final_systems.vp)throw std::runtime_error("Missing VP maintenance contracts");
        ensure_vp_maintenance_context_profile(s).contracts=snap.final_systems.vp->active_contracts;}
    const auto plot_at=[&](std::int64_t x,std::int64_t y)->const saves::PlotObservation* {
        if(x<0||y<0||static_cast<std::uint64_t>(x)>=snap.map_header.grid.width||static_cast<std::uint64_t>(y)>=snap.map_header.grid.height)return nullptr;
        return &snap.map_plots->plots.at(snap.map_header.grid.index(x,y));
    };
    for(const auto &saved:snap.all_players->players) {
        const auto id=saved.saved_player_id;
        if(saved.core.layout!=layout||saved.traits.layout!=layout||saved.administration.layout!=layout||saved.units.layout!=layout||
           saved.cities.layout!=layout||(vp&&saved.tail.layout!=layout)||!w.setup.players.contains(id)||!w.cities.players.contains(id)||!w.diplomacy.players.contains(id))
            throw std::runtime_error("Maintenance player/profile mismatch");
        MaintenancePlayer p;
        // BARBARIAN_PLAYER in both selected source wire contracts. This is an
        // imported role, not an ID bound on the subsequently owned world.
        p.barbarian=id==63;
        const auto core=[&](const char *n){return scalar(saved.core.fields,n);};
        p.base_free=core("m_iBaseFreeUnits");p.gold_per_unit_times100=core("m_iGoldPerUnit");p.extra_unit_cost=core("m_iExtraUnitCost");
        p.outside_units=core("m_iNumOutsideUnits");p.unit_modifier=core("m_iUnitGoldMaintenanceMod");p.supply_modifier=core("m_iUnitSupplyMod");
        p.building_modifier=core("m_iBuildingGoldMaintenanceMod");p.improvement_modifier=core(vp?"m_iImprovementGoldMaintenanceMod":"m_iRouteGoldMaintenanceMod");
        p.free_garrison=core("m_iGarrisonFreeMaintenanceCount")>0;
        p.land_modifier=scalar(saved.traits.fields,"m_iLandUnitMaintenanceModifier");p.naval_modifier=scalar(saved.traits.fields,"m_iNavalUnitMaintenanceModifier");
        if(vp||(saved.traits.version&&*saved.traits.version>=10))p.combat_modifiers=sparse(named(saved.traits.fields,"m_paiMaintenanceModifierUnitCombat").values);
        const auto &treasury=named(saved.administration.blocks,"treasury");
        const auto tr=[&](const char *n){return named(treasury.fields,n).value;};
        p.base_building=tr("m_iBaseBuildingGoldMaintenance");p.base_improvement=tr("m_iBaseImprovementGoldMaintenance");
        p.cached_unit_expense=tr("m_iExpensePerTurnUnitMaintenance");p.diplomacy_gold=tr("m_iGoldPerTurnFromDiplomacy");
        if(vp) {
            auto &profile=ensure_vp_maintenance_profile(p,s.profile);
            profile.vassal_modifier=core("m_iVassalGoldMaintenanceMod");profile.vassal_taxes_times100=tr("m_iExpensePerTurnFromVassalTax");
            profile.outgoing_espionage_gold=named(saved.tail.fields,"m_aiOutgoingEspionageYields").values.at(2);
            for(auto [type,n]:sparse(named(saved.core.fields,"m_abActiveContract").values))if(n)profile.active_contracts.insert(type);
        } else p.cached_supply_expense=tr("m_iExpensePerTurnUnitSupply");
        std::set<std::int64_t> city_ids;
        for(const auto &city:saved.cities.cities) {
            const auto cid=scalar(city.core.fields,"m_iID");
            if(city.core.layout!=layout||scalar(city.core.fields,"m_eOwner")!=id||!city_ids.insert(cid).second||!w.cities.players.at(id).cities.contains(cid))throw std::runtime_error("Maintenance city identity mismatch");
            if(vp)ensure_vp_maintenance_profile(p,s.profile).extra_building_percent[cid]=scalar(city.core.fields,"m_iExtraBuildingMaintenance");
        }
        if(city_ids.size()!=w.cities.players.at(id).cities.size())throw std::runtime_error("Incomplete maintenance city roster");
        for(const auto &saved_unit:saved.units.units) {
            if(saved_unit.core.layout!=layout||saved_unit.tail.layout!=layout)throw std::runtime_error("Maintenance unit profile mismatch");
            const auto &fields=saved_unit.core.fields;const auto uvalue=[&](const char *n){return scalar(fields,n);};
            const auto uid=uvalue("m_iID");if(uvalue("m_eOwner")!=id)throw std::runtime_error("Maintenance unit owner mismatch");
            MaintenanceUnit u;
            u.source_core={{saved_unit.core.start_offset,saved_unit.core.end_offset}};
            if(vp)u.type=uvalue("m_eUnitType");
            else {const auto hash=uvalue("idUnitType");const auto *type=hash>0?types.find_hash(static_cast<std::uint64_t>(hash)):nullptr;
                if(!type)throw MaintenanceUnavailable("Unresolved BNW unit type hash");u.type=type->id;}
            const auto it=r.units.find(u.type);if(it==r.units.end())throw MaintenanceUnavailable("Unresolved maintenance unit Info");
            const auto &info=it->second;u.domain=info.domain;u.combat=vp?uvalue("m_eCombatType"):info.combat;
            u.military=uvalue("m_iBaseCombat")>0;
            u.civilian=!u.military;
            if(vp) {
                u.civilian=u.civilian&&!(wide::add(info.range,uvalue("m_iExtraRange"))>0&&uvalue("m_iBaseRangedCombat")>0)&&info.nuke<=0;
                u.no_maintenance=uvalue("m_bIsNoMaintenance")!=0;u.garrison=uvalue("m_iGarrisonCityID")!=-1;
                if(uvalue("m_eUnitContract")!=-1)ensure_vp_maintenance_profile(p,s.profile).contract_units.insert(uid);
            } else {
                u.no_maintenance=info.no_maintenance;
                const auto *plot=plot_at(uvalue("m_iX"),uvalue("m_iY"));
                if(u.domain==4&&plot)u.domain=named(plot->fields,"plot_type").value==3?0:2;
                const auto layer=scalar(named(saved_unit.tail.blocks,"state").children,"m_iMapLayer");
                if(layer!=0||!plot||named(plot->fields,"plot_city_id").value==-1||named(plot->fields,"owner").value!=id)u.garrison=false;
                // An own city on the main layer needs CvPlot::getBestDefender.
            }
            if(!p.units.emplace(uid,std::move(u)).second)throw std::runtime_error("Duplicate maintenance unit ID");
        }
        if(!s.players.emplace(id,std::move(p)).second)throw std::runtime_error("Duplicate maintenance player ID");
    }
    if(s.players.size()!=w.setup.players.size()||s.players.size()!=w.cities.players.size()||s.players.size()!=w.diplomacy.players.size())throw std::runtime_error("Incomplete maintenance player roster");
    return s;
}
UnitMaintenanceQuote unit_maintenance(const MaintenanceContext &s,const MaintenanceWorld &w,const MaintenanceRules &r,
    const content::GlobalDefineCache &d,const content::CustomModOptions *o,std::int64_t id,std::int64_t override_units) {
    validate(s,w,r,d,o);const bool vp=r.profile==TechnologyProfile::vp542;
    UnitMaintenanceQuote q;const auto &p=s.players.at(id);const auto &b=w.setup.players.at(id);
    if(w.cities.players.at(id).cities.empty())return q;
    if(!vp&&override_units!=-1)throw std::runtime_error("BNW has no unit-maintenance count override");
    const auto &h=player_handicap(r,w,d,o,id);const auto &gh=handicap(r,w.setup.game_handicap);
    q.paid_units=override_units;
    if(override_units==-1) {
        q.free_units=wide::add(wide::add(h.free_units,free_units(p,w,r,d,o,id)),p.base_free);
        if(vp&&!human(w,o,id))q.free_units=wide::add(q.free_units,gh.ai_free_units);
        q.paid_units=std::max<std::int64_t>(0,wide::subtract(i64(p.units.size()),q.free_units));
    }
    q.base_times100=mul(q.paid_units,p.gold_per_unit_times100);
    // Semantic domain constants from the source enum, not collection limits.
    for(const auto &[domain,modifier]:std::vector<std::pair<std::int64_t,std::int64_t>>{{2,p.land_modifier},{0,p.naval_modifier}})if(modifier) {
        auto paid=wide::subtract(count_domain(p,domain),free_units(p,w,r,d,o,id,domain,true));
        if(vp)paid=std::max<std::int64_t>(0,paid);
        const auto cost=mul(paid,p.gold_per_unit_times100);
        const auto delta=vp?wide::subtract(mod(cost,modifier),cost):percent(cost,modifier);
        q.base_times100=wide::add(q.base_times100,delta);
    }
    for(auto combat:r.combat_classes)if(const auto modifier=val(p.combat_modifiers,combat)) {
        std::int64_t n=0;for(const auto &[unit,u]:p.units)if(u.combat==combat)n=wide::add(n,1);
        const auto cost=mul(n,p.gold_per_unit_times100);
        q.base_times100=wide::add(q.base_times100,wide::subtract(mod(cost,modifier),cost));
    }
    if(!s.default_end_turn||!integer(d,"UNIT_MAINTENANCE_GAME_EXPONENT_DIVISOR"))throw MaintenanceUnavailable("Zero maintenance progress divisor");
    if(vp) {
        auto progress=wide::multiply_divide(s.elapsed_turns,100,s.default_end_turn);
        if(opt(o,"BALANCE_VP")) {
            const TeamTechnologies *tech=nullptr;for(const auto &t:w.technologies)if(t.team()==b.team){if(tech)throw std::runtime_error("Duplicate maintenance team");tech=&t;}
            if(!tech||tech->profile()!=s.profile||!tech->slots())throw MaintenanceUnavailable("Unresolved maintenance team technologies");
            progress=std::max<std::int64_t>(1,wide::multiply_divide(tech->known_count(),100,i64(tech->slots())));
        }
        float multiplier=static_cast<float>(progress)*static_cast<float>(integer(d,"UNIT_MAINTENANCE_GAME_MULTIPLIER"));
        multiplier/=100.f;multiplier+=1.f;q.multiplier=multiplier;
        float exponent=static_cast<float>(progress)/static_cast<float>(integer(d,"UNIT_MAINTENANCE_GAME_EXPONENT_DIVISOR"));
        exponent/=100.f;exponent+=1.f;q.exponent=exponent;
    } else {
        const double progress=static_cast<double>(s.elapsed_turns)/static_cast<double>(s.default_end_turn);
        q.multiplier=1.0+progress*integer(d,"UNIT_MAINTENANCE_GAME_MULTIPLIER");
        q.exponent=1.0+progress/integer(d,"UNIT_MAINTENANCE_GAME_EXPONENT_DIVISOR");
        q.extra_times100=mul(p.extra_unit_cost,100);
    }
    const double temp=q.multiplier*static_cast<double>(wide::add(q.base_times100,q.extra_times100))/100.0;
    double cost=std::pow(temp,q.exponent);
    if(vp) {
        if(major(w,id)) {double difference=cost-temp;difference*=h.inflation;difference/=100;
            if(!human(w,o,id)){difference*=gh.ai_inflation;difference/=100;}cost=temp+difference;}
        cost+=p.extra_unit_cost;
    }
    if(p.unit_modifier){cost*=wide::add(100,p.unit_modifier);cost/=100;}
    if(vp) {
        if(major(w,id)){cost*=h.unit;cost/=100;if(!human(w,o,id)){cost*=gh.ai_unit;cost/=100;}}
        if(vassal(w,id)){cost*=wide::subtract(100,integer(d,"VASSALAGE_VASSAL_UNIT_MAINT_COST_PERCENT"));cost/=100;}
    } else if(w.setup.is_human(b)){cost*=h.unit;cost/=100;}
    else if(!human_teammate(w,id)){cost*=gh.ai_unit;cost/=100;}
    q.total=std::max<std::int64_t>(0,truncate(cost));return q;
}
MaintenanceQuote maintenance_costs(MaintenanceContext &s,const MaintenanceWorld &w,const MaintenanceRules &r,
    const content::GlobalDefineCache &d,const content::CustomModOptions *o,std::int64_t id) {
    validate(s,w,r,d,o);const bool vp=r.profile==TechnologyProfile::vp542;
    auto &p=s.players.at(id);const auto &b=w.setup.players.at(id);const auto &h=player_handicap(r,w,d,o,id);const auto &gh=handicap(r,w.setup.game_handicap);
    MaintenanceQuote q;q.units=unit_maintenance(s,w,r,d,o,id);p.cached_unit_expense=q.units.total;
    if(!vp){q.supply=unit_supply(s,w,r,d,id);p.cached_supply_expense=q.supply;}
    q.buildings=p.base_building;
    if(vp&&opt(o,"BALANCE_RESOURCE_SHORTAGE_BUILDING_REFUND")) {
        const auto &profile=vp_maintenance_profile(p);
        std::int64_t extra=0;for(const auto &[city,c]:w.cities.players.at(id).cities)if(val(profile.extra_building_percent,city)>0)extra=wide::add(extra,val(profile.extra_building_percent,city));
        q.buildings=mod(q.buildings,extra);
    }
    q.buildings=mod(q.buildings,p.building_modifier);
    if(vp) {if(major(w,id)){q.buildings=percent(q.buildings,h.building);if(!human(w,o,id))q.buildings=percent(q.buildings,gh.ai_building);}}
    else if(w.setup.is_human(b))q.buildings=percent(q.buildings,h.building);
    else if(!human_teammate(w,id))q.buildings=percent(q.buildings,gh.ai_building);
    const auto era=r.era_building_percent.find(w.setup.start_era);
    if(era==r.era_building_percent.end()&&!vp)throw MaintenanceUnavailable("Missing BNW start-era Info");
    q.buildings=percent(q.buildings,era==r.era_building_percent.end()?0:era->second);
    q.improvements=mod(p.base_improvement,p.improvement_modifier);
    if(!vp||major(w,id)){q.improvements=percent(q.improvements,h.improvement);if(vp&&!human(w,o,id))q.improvements=percent(q.improvements,gh.ai_improvement);}
    if(vp) {
        auto &vp_quote=ensure_vp_maintenance_quote(q);
        const auto &profile=vp_maintenance_profile(p);
        const auto &team=w.diplomacy.teams.at(b.team);
        if(team.alive_count!=0) {
            std::int64_t expense=0;
            for(auto v:w.diplomacy.major_players)if(w.diplomacy.teams.at(w.setup.players.at(v).team).master==b.team) {
                for(const auto &[cid,c]:w.cities.players.at(v).cities)expense=wide::add(expense,std::max<std::int64_t>(0,truncate(std::pow(static_cast<double>(c.population),real(d,"VASSALAGE_VASSAL_CITY_POP_EXPONENT")))));
                expense=wide::add(expense,std::max<std::int64_t>(0,percent(s.players.at(v).cached_unit_expense,integer(d,"VASSALAGE_VASSAL_UNIT_MAINT_COST_PERCENT"))));
            }
            vp_quote.vassals=expense/team.alive_count;
            auto leader=team.members.empty()?-1:team.members.front();
            for(auto member:team.members)if(w.setup.players.at(member).alive){leader=member;break;}
            if(leader==id)vp_quote.vassals=wide::add(vp_quote.vassals,expense%team.alive_count);
            vp_quote.vassals=mod(vp_quote.vassals,profile.vassal_modifier);
        }
        vp_quote.taxes=profile.vassal_taxes_times100/100;
        if(opt(o,"BALANCE_CORE_JFD"))for(auto type:vp_maintenance_rules_profile(r).contracts)if(profile.active_contracts.contains(type)) {
            for(const auto &c:vp_maintenance_context_profile(s).contracts)if(c.type==type){vp_quote.contracts=wide::add(vp_quote.contracts,c.maintenance);break;}
        }
        vp_quote.espionage=profile.outgoing_espionage_gold;vp_quote.diplomacy=std::max<std::int64_t>(0,wide::subtract(0,p.diplomacy_gold));
    }
    q.total=q.units.total;for(auto v:{q.supply,q.buildings,q.improvements})q.total=wide::add(q.total,v);
    if(vp) {
        const auto &vp_quote=vp_maintenance_quote(q);
        for(auto v:{vp_quote.vassals,vp_quote.taxes,vp_quote.contracts,vp_quote.espionage,vp_quote.diplomacy})
            q.total=wide::add(q.total,v);
    }
    return q;
}
} // namespace civ5::runtime
