#include "civ5/runtime/treasury.hpp"
#include "civ5/runtime/vp_maintenance_profile.hpp"
#include "civ5/runtime/vp_treasury_income_profile.hpp"
#include "civ5/runtime/vp_treasury_minor_profile.hpp"
#include "civ5/runtime/vp_treasury_religion_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
using content::TechnologyProfile;
std::int64_t i64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("Treasury size exceeds int64");return static_cast<std::int64_t>(n);}
std::int64_t value(const YieldValues &a,std::int64_t id){const auto i=a.find(id);return i==a.end()?0:i->second;}
std::int64_t mul(std::int64_t a,std::int64_t b){return wide::multiply_divide(a,b,1);}
std::int64_t mod(std::int64_t a,std::int64_t b){return wide::multiply_divide(a,wide::add(100,b),100);}
template<class T> const T &named(const std::vector<T> &v,std::string_view name){
    const T *p=nullptr;for(const auto &f:v)if(f.name&&std::string_view(f.name)==name){if(p)throw std::runtime_error("Duplicate treasury field: "+std::string(name));p=&f;}
    if(!p)throw std::runtime_error("Missing treasury field: "+std::string(name));return *p;
}
template<class T> std::int64_t scalar(const std::vector<T> &v,std::string_view name){const auto &a=named(v,name).values;if(a.size()!=1)throw std::runtime_error("Invalid treasury scalar");return a.front();}
YieldValues sparse(const std::vector<std::int64_t> &v){YieldValues a;for(std::size_t i=0;i<v.size();++i)if(v[i])a[i64(i)]=v[i];return a;}
std::int64_t integer(const content::GlobalDefineCache &d,std::string_view name){const auto *v=d.find(name);if(!v||!v->lookup.error.empty())throw TreasuryUnavailable("Unresolved treasury define: "+std::string(name));return std::get<std::int64_t>(v->lookup.value);}
bool opt(const content::CustomModOptions *o,std::string_view name){return o&&o->enabled(name);}
void validate(const TreasuryContext &s,const MaintenanceWorld &w,const MaintenanceContext &m,
    const TreasuryRules &r,const content::GlobalDefineCache &d,const content::CustomModOptions *o){
    const bool vp=s.profile==TechnologyProfile::vp542;
    if(s.profile!=r.profile||s.profile!=w.setup.profile||s.profile!=w.diplomacy.profile||s.profile!=w.cities.profile||s.profile!=m.profile||
       d.profile()!=(vp?content::GlobalDefineProfile::vp542:content::GlobalDefineProfile::bnw)||
       (vp&&(!o||!o->all_succeeded()))||(!vp&&o))throw std::runtime_error("Treasury profile/rules mismatch");
}
const TreasuryReligion *religion(const TreasuryContext &s,std::int64_t id,std::int64_t founder=-1){
    if(id==-1)return nullptr;for(const auto &r:s.religions)if(r.religion==id&&(id!=0||r.founder==founder))return &r;return nullptr;
}
std::int64_t majority(const CityYieldState &c){
    std::int64_t total=0,most=-1,pressure=0,id=-1;
    for(const auto &r:c.religions){total=wide::add(total,r.followers);if(r.followers>most||(r.followers==most&&r.pressure>pressure)){most=r.followers;pressure=r.pressure;id=r.religion;}}
    return mul(most,2)>=total?id:-1;
}
std::int64_t owned_religion(const TreasuryContext &s,const content::CustomModOptions &o,std::int64_t id){
    const auto &p=s.players.at(id);
    if(o.enabled("BALANCE_VP")){const auto &state=vp_treasury_religion_profile(p);
        return state.owns_state_religion&&state.state_religion!=0?state.state_religion:-1;}
    for(const auto &r:s.religions)if(r.founder==id&&!r.pantheon&&r.holy_city&&r.holy_city->first==id)return r.religion;
    return -1;
}
// IsBeliefValid's bHolyCityOnly=false branch, as called by GetCSYieldBonus.
bool valid_cs_belief(const TreasuryContext &s,const MaintenanceWorld &w,const content::CustomModOptions &o,
    const TreasuryReligion &container,const TreasuryBelief &b,std::int64_t belief,std::int64_t player){
    auto rid=container.belief_religion;const auto *r=religion(s,rid);
    bool eligible=false;
    if(r)eligible=r->pantheon&&!r->holy_plot_owner?r->founder==player:r->holy_plot_owner==player;
    const auto city=container.holy_city;
    const auto city_majority=city?vp_treasury_minor_profile(s.players.at(city->first)).city_majorities.at(city->second):-1;
    if(rid!=-1&&(b.founder||b.enhancer)&&(!eligible||(city&&city->first!=player)))return false;
    if(b.follower&&city&&(city_majority!=rid||city->first!=player))return false;
    if(b.reformation&&((city&&city_majority!=rid)||(!eligible&&vp_treasury_religion_profile(s.players.at(player)).state_religion!=rid)))return false;
    if(b.pantheon){
        if(rid==-1)rid=0;
        if(city){
            bool in_pantheon=false;
            if(city_majority!=rid){
                const TreasuryReligion *pantheon=nullptr;
                for(const auto &p:s.religions)if(p.founder==player&&p.pantheon){pantheon=&p;break;}
                if(pantheon){const auto *parent=religion(s,rid,player);in_pantheon=parent&&pantheon->belief_lookup.contains(belief)&&parent->belief_lookup.contains(belief);}
                if(!in_pantheon)return false;
            }
            if(city->first!=player)return false;
        }
    }
    return !o.enabled("BALANCE_UNIQUE_BELIEFS_ONLY_FOR_CIV")||b.civilization==-1||b.civilization==w.cities.players.at(player).civilization;
}
std::int64_t city_state_religion_modifier(const TreasuryContext &s,const MaintenanceWorld &w,
    const TreasuryRules &rules,const content::CustomModOptions &o,std::int64_t player){
    const auto *r=religion(s,vp_treasury_religion_profile(s.players.at(player)).state_religion);if(!r)return 0;
    std::int64_t n=0;
    for(auto belief:r->beliefs){const auto i=rules.beliefs.find(belief);if(i==rules.beliefs.end())throw TreasuryUnavailable("Unresolved city-state belief Info");
        if(i->second.city_state_modifier&&valid_cs_belief(s,w,o,*r,i->second,belief,player))n=wide::add(n,i->second.city_state_modifier);}
    return n;
}
std::int64_t era_bonus(const std::map<std::int64_t,YieldValues> &rules,std::int64_t yield,std::int64_t era){
    const auto y=rules.find(yield);if(y==rules.end()||era<0)return 0;
    std::int64_t result=0;
    // Skip absent zero entries instead of looping down a potentially wide ID.
    for(auto i=y->second.upper_bound(era);i!=y->second.begin();){--i;if(i->first<0)break;result=wide::add(result,i->second);if(result>0)break;}
    return result;
}
std::int64_t bnw_religion_gold(const TreasuryContext &s,const MaintenanceWorld &w,const TreasuryRules &rules,
    std::int64_t id,const TreasuryServices &services){
    std::optional<std::int64_t> overridden;if(services.founder_benefits_religion)overridden=services.founder_benefits_religion(id);
    std::int64_t rid=-1;
    if(overridden)rid=*overridden;
    else {
        for(const auto &r:s.religions)if(r.founder==id&&!r.pantheon)rid=r.religion; // last created religion
        const auto *r=religion(s,rid);if(!r||r->founder!=id||r->pantheon||r->holy_plot_owner!=id)rid=-1;
    }
    const auto *r=religion(s,rid);if(!r)return 0;
    std::int64_t city_gold=0,followers_per_gold=0,cities=0,followers=0;
    for(auto id_:r->belief_lookup){const auto i=rules.beliefs.find(id_);if(i==rules.beliefs.end())throw TreasuryUnavailable("Unresolved BNW treasury belief Info");
        city_gold=wide::add(city_gold,i->second.gold_per_city);followers_per_gold=wide::add(followers_per_gold,i->second.followers_per_gold);}
    for(const auto &[player,p]:w.cities.players)if(w.setup.players.at(player).alive)for(const auto &[cid,c]:p.cities){
        if(majority(c)==rid)cities=wide::add(cities,1);
        if(followers_per_gold>0)for(const auto &status:c.religions)if(status.religion==rid){followers=wide::add(followers,status.followers);break;}
    }
    return wide::add(mul(cities,city_gold),followers_per_gold>0?followers/followers_per_gold:0);
}
}
TreasuryRules load_treasury_rules(const content::RulesDatabase &db,TechnologyProfile profile,const content::InfoTypeCatalog &types){
    if(!types.report().all_succeeded())throw std::runtime_error("Incomplete treasury type catalog");
    TreasuryRules r;r.profile=profile;const bool vp=profile==TechnologyProfile::vp542;
    const auto info=[&](const char *table,const auto &f){const auto q=db.prefetch_info(table,f);if(!q.all_succeeded())throw std::runtime_error("Treasury Info: "+q.error);};
    info("Beliefs",[&](auto slot,auto,const auto &row){auto &b=r.beliefs[i64(slot)];
        b.gold_per_city=row.integer("GoldPerFollowingCity");b.followers_per_gold=row.integer("GoldPerXFollowers");
        if(vp){b.city_state_modifier=row.integer("CSYieldBonusFromSharedReligion");
            b.founder=row.integer("Founder")!=0;b.enhancer=row.integer("Enhancer")!=0;b.follower=row.integer("Follower")!=0;
            b.reformation=row.integer("Reformation")!=0;b.pantheon=row.integer("Pantheon")!=0;
            const auto *type=types.find(row.text("CivilizationType").value_or(""));b.civilization=type?type->id:-1;}
        return true;});
    if(vp){
        info("Resources",[&](auto slot,auto,const auto &row){if(row.integer("ResourceUsage")==2)r.luxury_resources.insert(i64(slot));return true;});
        info("MinorCivilizations",[&](auto slot,auto,const auto &row){auto &m=r.minors[i64(slot)];
            m.type=row.text("Type").value_or("");
            for(bool ally:{false,true}){
                const auto table=ally?"MinorCivilizationTraits_AllyYieldBonuses":"MinorCivilizationTraits_FriendYieldBonuses";
                const auto q=db.query_rows({std::string("SELECT Eras.ID, Yields.ID, Yield FROM ")+table+
                    " INNER JOIN Eras ON Eras.Type = EraType INNER JOIN Yields ON Yields.Type = YieldType WHERE MinorCivTraitType = ?",
                    {row.text("MinorCivTrait").value_or("")}},[&](const auto &v){
                        auto &n=(ally?m.allies:m.friends)[v.integer_at(1)][v.integer_at(0)];n=wide::add(n,v.integer_at(2));});
                if(!q.all_succeeded())throw std::runtime_error("Minor yield rules: "+q.error);
            }return true;});
    }
    return r;
}
TreasuryContext restore_treasury(const saves::SavedSessionSnapshot &snap,const MaintenanceWorld &w,
    const TreasuryRules &rules,const content::InfoTypeCatalog &types){
    const bool vp=rules.profile==TechnologyProfile::vp542;
    const auto layout=vp?saves::GameStateLayout::vp542_corpus:saves::GameStateLayout::expansion2_version1;
    if(!snap.all_players||!snap.pregame||!snap.map_plots||snap.all_players->layout!=layout||snap.pregame->layout!=layout||
       snap.map_plots->layout!=layout||snap.trade.layout!=layout||snap.religion_culture.layout!=layout||
       w.setup.profile!=rules.profile||w.cities.profile!=rules.profile||w.diplomacy.profile!=rules.profile||!types.report().all_succeeded())
        throw std::runtime_error("Treasury needs matching complete session and rules");
    TreasuryContext s;s.profile=rules.profile;s.trade_routes=snap.trade.routes;
    for(const auto &saved:snap.all_players->players){
        const auto id=saved.saved_player_id;
        if(!w.setup.players.contains(id)||!w.cities.players.contains(id)||saved.core.layout!=layout||saved.traits.layout!=layout||saved.administration.layout!=layout||
           saved.planning.layout!=layout||(vp&&(saved.tail.layout!=layout||saved.knowledge.layout!=layout)))throw std::runtime_error("Treasury player/profile mismatch");
        TreasuryPlayer p;p.source_core={{saved.core.start_offset,saved.core.end_offset}};
        p.source_administration={{saved.administration.start_offset,saved.administration.end_offset}};
        const auto &tr=named(saved.administration.blocks,"treasury");
        p.balance_times100=named(tr.fields,"m_iGold").value;p.connections_times100=named(tr.fields,"m_iCityConnectionGoldTimes100").value;
        p.connection_modifier=named(tr.fields,"m_iCityConnectionTradeRouteGoldModifier").value;p.connection_change=named(tr.fields,"m_iCityConnectionTradeRouteGoldChange").value;
        p.lifetime_gross_gold=named(tr.fields,"m_iLifetimeGrossGoldIncome").value;
        p.balance_history_times100=named(tr.arrays,"m_GoldBalanceForTurnTimes100").values;
        p.income_history_times100=named(tr.arrays,"m_GoldChangeForTurnTimes100").values;
        p.anarchy=scalar(saved.core.fields,"m_iAnarchyNumTurns")>0;
        p.ally=named(named(saved.planning.blocks,"minor_civ_ai").fields,"m_eAlly").value;
        if(vp||(saved.traits.version&&*saved.traits.version>=7))p.gold_per_partner=named(saved.traits.fields,"m_iYieldChangePerTradePartner").values.at(2);
        if(vp){
            auto &income=ensure_vp_treasury_income_profile(p);
            income.internal_trade=named(tr.fields,"m_iInternalTradeGoldBonus").value;
            income.incoming_espionage=named(saved.tail.fields,"m_aiIncomingEspionageYields").values.at(2);
            income.jfd_minors=named(saved.core.fields,"m_aiYieldFromMinors").values.at(2);
            income.annexed_minors_times100=named(saved.core.fields,"m_piYieldPerTurnFromAnnexedMinorsTimes100").values.at(2);
            income.gold_per_export=named(saved.traits.fields,"m_iYieldFromExport").values.at(2);
            income.gold_per_import=named(saved.traits.fields,"m_iYieldFromImport").values.at(2);
            auto &minor_profile=ensure_vp_treasury_minor_profile(p);
            minor_profile.trait_cs_modifier=scalar(saved.traits.fields,"m_iCityStateBonusModifier");minor_profile.cs_modifier=scalar(saved.core.fields,"m_iCSYieldBonusModifier");
            income.exported=sparse(named(saved.core.fields,"m_paiResourceExport").values);
            income.imported=sparse(named(saved.core.fields,"m_paiResourceImportFromMajor").values);
            income.from_minors=sparse(named(saved.core.fields,"m_paiResourceFromMinors").values);
            income.from_alliances=sparse(named(saved.core.fields,"m_paiResourceFromCSAlliances").values);
            income.siphoned=sparse(named(saved.core.fields,"m_paiResourcesSiphoned").values);
            const auto &rel=named(saved.knowledge.blocks,"player_religions");auto &religion=ensure_vp_treasury_religion_profile(p);
            religion.state_religion=named(rel.fields,"state_religion").value;religion.owns_state_religion=named(rel.fields,"owns_state_religion").value!=0;
            const auto &minor=named(saved.planning.blocks,"minor_civ_ai");
            minor_profile.permanent_ally=named(minor.fields,"m_ePermanentAlly").value;minor_profile.no_ally=named(minor.fields,"m_bNoAlly").value!=0;
            for(auto [who,n]:sparse(named(minor.arrays,"m_abFriends").values))if(n)minor_profile.friends.insert(who);
            const auto &minor_names=snap.pregame->archive.string_arrays.at("minor_civ_types");
            if(id<0||static_cast<std::uint64_t>(id)>=minor_names.size())throw std::runtime_error("Treasury minor type wire row missing");
            const auto *type=types.find(minor_names.at(static_cast<std::size_t>(id)));minor_profile.minor_type=type?type->id:-1;
            // Selected source loop uses [MAX_MAJOR_CIVS, MAX_CIV_PLAYERS).
            if(id>=i64(saved.planning.dimensions.major)&&id<i64(saved.planning.dimensions.civ_teams))s.minor_players.insert(id);
            for(const auto &city:saved.cities.cities){
                const auto cid=scalar(city.core.fields,"m_iID");if(city.tail.layout!=layout||!w.cities.players.at(id).cities.contains(cid)||scalar(city.core.fields,"m_eOwner")!=id)throw std::runtime_error("Treasury city identity mismatch");
                if(!minor_profile.city_majorities.emplace(cid,scalar(named(city.tail.blocks,"religions").children,"m_eMajorityCityReligion")).second)throw std::runtime_error("Duplicate treasury city");
            }
            if(minor_profile.city_majorities.size()!=w.cities.players.at(id).cities.size())throw std::runtime_error("Incomplete treasury city roster");
        }
        if(!s.players.emplace(id,std::move(p)).second)throw std::runtime_error("Duplicate treasury player");
    }
    if(s.players.size()!=w.setup.players.size()||s.players.size()!=w.cities.players.size())throw std::runtime_error("Incomplete treasury roster");
    for(const auto &saved:snap.religion_culture.religions){
        TreasuryReligion r;r.religion=saved.religion;r.founder=saved.founder;r.pantheon=saved.pantheon;r.belief_religion=saved.beliefs.religion.value_or(saved.religion);
        if(vp){r.beliefs=saved.beliefs.belief_ids;for(auto [id,n]:sparse(saved.beliefs.belief_lookup))if(n==1)r.belief_lookup.insert(id);}
        else {for(auto hash:saved.beliefs.belief_hashes){const auto *type=hash?types.find_hash(hash):nullptr;if(type&&rules.beliefs.contains(type->id))r.belief_lookup.insert(type->id);}r.beliefs.assign(r.belief_lookup.begin(),r.belief_lookup.end());}
        const auto x=saved.holy_city_x,y=saved.holy_city_y;
        if(x>=0&&y>=0&&static_cast<std::uint64_t>(x)<snap.map_header.grid.width&&static_cast<std::uint64_t>(y)<snap.map_header.grid.height){
            const auto &plot=snap.map_plots->plots.at(snap.map_header.grid.index(x,y));r.holy_plot_owner=named(plot.fields,"owner").value;
            // VP getPlotCity gates its owning-city reference by m_bIsCity;
            // BNW has a separate plot-city reference. Working/owning a normal
            // tile does not make it a holy city.
            if(!vp||named(plot.fields,"is_city").value){
                const auto owner=named(plot.fields,vp?"owning_city_owner":"plot_city_owner").value;
                const auto cid=named(plot.fields,vp?"owning_city_id":"plot_city_id").value;
                if(owner!=-1&&cid!=-1&&w.cities.players.contains(owner)&&w.cities.players.at(owner).cities.contains(cid))r.holy_city={{owner,cid}};
            }
        }
        s.religions.push_back(std::move(r));
    }
    return s;
}
MinorYieldQuote minor_yield_times100(const TreasuryContext &s,const MaintenanceWorld &w,const MaintenanceContext &m,
    const TreasuryRules &rules,const content::GlobalDefineCache &d,const content::CustomModOptions &o,
    std::int64_t minor,std::int64_t id,std::int64_t yield){
    validate(s,w,m,rules,d,&o);if(s.profile!=TechnologyProfile::vp542)throw std::runtime_error("VP minor-yield query requires VP");
    if(yield<0||static_cast<std::uint64_t>(yield)>=w.cities.yield_slots)throw std::out_of_range("Minor yield ID");
    MinorYieldQuote q;if(!w.diplomacy.major_players.contains(id))return q;
    const auto &p=vp_treasury_minor_profile(s.players.at(id)),&cs=vp_treasury_minor_profile(s.players.at(minor));
    const bool friends=cs.friends.contains(id),allies=s.players.at(minor).ally==id||(cs.permanent_ally==id&&!cs.no_ally);
    if(friends||allies){const auto info=rules.minors.find(cs.minor_type);if(info==rules.minors.end())throw TreasuryUnavailable("Unresolved MinorCivilizations Info");
        const auto era=m.team_eras.at(w.setup.players.at(id).team);
        if(friends)q.friends=era_bonus(info->second.friends,yield,era);if(allies)q.allies=era_bonus(info->second.allies,yield,era);}
    q.total_times100=wide::add(q.friends,q.allies);if(!q.total_times100)return q;
    q.modifier=wide::add(p.trait_cs_modifier,p.cs_modifier);
    const auto capital=w.cities.players.at(minor).capital;
    if(w.cities.players.at(minor).cities.contains(capital)){
        const auto owned=owned_religion(s,o,id);
        if(owned!=-1&&cs.city_majorities.at(capital)==owned)q.modifier=wide::add(q.modifier,city_state_religion_modifier(s,w,rules,o,id));
    }
    if(q.modifier>0)q.total_times100=mod(q.total_times100,q.modifier);
    if(o.enabled("BALANCE_CITY_STATE_SCALE")){q.scale=wide::add(100,mul(std::max<std::int64_t>(0,wide::subtract(i64(w.cities.players.at(minor).cities.size()),1)),integer(d,"CITY_STATE_SCALE_PER_CITY_MOD")));q.total_times100=wide::multiply_divide(q.total_times100,q.scale,100);}
    return q;
}
TreasuryIncomeQuote gross_gold_times100(const TreasuryContext &s,const MaintenanceWorld &w,const MaintenanceContext &m,
    const TreasuryRules &rules,const CityYieldRules &city_rules,const content::GlobalDefineCache &d,
    const content::CustomModOptions *o,std::int64_t id,const TreasuryServices &services){
    validate(s,w,m,rules,d,o);if(city_rules.profile!=s.profile)throw std::runtime_error("Treasury city-rule profile mismatch");
    const bool vp=s.profile==TechnologyProfile::vp542;const auto &p=s.players.at(id);const auto &mp=m.players.at(id);TreasuryIncomeQuote q;
    for(const auto &[cid,c]:w.cities.players.at(id).cities)q.cities=wide::add(q.cities,city_yield_times100(w.cities,city_rules,d,id,cid,w.cities.gold).total_times100);
    q.diplomacy=mul(vp?std::max<std::int64_t>(0,mp.diplomacy_gold):mp.diplomacy_gold,100);q.connections=p.connections_times100;
    if(!vp)q.religion=mul(bnw_religion_gold(s,w,rules,id,services),100); // VP getter explicitly returns zero.
    std::set<std::int64_t> partners;
    for(const auto &r:s.trade_routes){
        if(r.origin_x==-1&&r.origin_y==-1&&r.destination_x==-1&&r.destination_y==-1)continue;
        if(r.origin_owner==r.destination_owner)continue;
        if(r.origin_owner==id)partners.insert(r.destination_owner);else if(r.destination_owner==id)partners.insert(r.origin_owner);
    }
    const auto era=vp?std::max<std::int64_t>(1,m.team_eras.at(w.setup.players.at(id).team)):1;
    auto traits=mul(mul(vp&&opt(o,"BALANCE_VP")?era:1,p.gold_per_partner),i64(partners.size()));
    if(vp){const auto &income=vp_treasury_income_profile(p);
        std::int64_t exported=0,imported=0;for(auto r:rules.luxury_resources){if(value(income.exported,r)>0)exported=wide::add(exported,1);
            if(value(income.imported,r)>0||value(income.from_minors,r)>0||value(income.from_alliances,r)>0||value(income.siphoned,r)>0)imported=wide::add(imported,1);}
        if(income.gold_per_export>0)traits=wide::add(traits,mul(mul(era,exported),income.gold_per_export));
        if(income.gold_per_import>0)traits=wide::add(traits,mul(mul(era,imported),income.gold_per_import));
    }
    q.traits=mul(traits,100);
    if(vp){
        auto &vp_quote=ensure_vp_treasury_income_quote(q);
        const auto &income=vp_treasury_income_profile(p);
        vp_quote.internal_trade=mul(income.internal_trade,100);const auto tid=w.setup.players.at(id).team;const auto &team=w.diplomacy.teams.at(tid);
        if(team.alive_count!=0){std::int64_t taxes=0;for(auto v:w.diplomacy.major_players)if(w.diplomacy.teams.at(w.setup.players.at(v).team).master==tid)taxes=wide::add(taxes,vp_maintenance_profile(m.players.at(v)).vassal_taxes_times100);
            vp_quote.vassal_taxes=taxes/team.alive_count;auto leader=team.members.empty()?-1:team.members.front();
            for(auto member:team.members)if(w.setup.players.at(member).alive){leader=member;break;}
            if(leader==id)vp_quote.vassal_taxes=wide::add(vp_quote.vassal_taxes,taxes%team.alive_count);
        }
        for(auto minor:s.minor_players)if(w.setup.players.at(minor).alive)vp_quote.minors=wide::add(vp_quote.minors,minor_yield_times100(s,w,m,rules,d,*o,minor,id,w.cities.gold).total_times100);
        if(opt(o,"BALANCE_CORE_JFD"))vp_quote.jfd_minors=mul(income.jfd_minors,100);
        vp_quote.annexed_minors=income.annexed_minors_times100;vp_quote.espionage=mul(income.incoming_espionage,100);
    }
    for(auto n:{q.cities,q.diplomacy,q.connections,q.religion,q.traits})q.total_times100=wide::add(q.total_times100,n);
    if(vp) {
        const auto &vp_quote=vp_treasury_income_quote(q);
        for(auto n:{vp_quote.internal_trade,vp_quote.vassal_taxes,vp_quote.minors,vp_quote.jfd_minors,vp_quote.annexed_minors,vp_quote.espionage})
            q.total_times100=wide::add(q.total_times100,n);
    }
    return q;
}
TreasuryNetQuote net_gold_times100(const TreasuryContext &s,const MaintenanceWorld &w,MaintenanceContext &m,
    const TreasuryRules &rules,const CityYieldRules &city_rules,const MaintenanceRules &maintenance_rules,
    const content::GlobalDefineCache &d,const content::CustomModOptions *o,std::int64_t id,const TreasuryServices &services){
    validate(s,w,m,rules,d,o);TreasuryNetQuote q;
    if(s.profile==TechnologyProfile::vp542&&s.players.at(id).anarchy){q.anarchy=true;return q;}
    q.income=gross_gold_times100(s,w,m,rules,city_rules,d,o,id,services);
    q.expenses=maintenance_costs(m,w,maintenance_rules,d,o,id);
    q.net_times100=wide::subtract(q.income.total_times100,mul(q.expenses->total,100));return q;
}
} // namespace civ5::runtime
