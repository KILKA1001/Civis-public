#include "civ5/runtime/bnw_happiness_minors.hpp"
#include <stdexcept>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::int32_t integer(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"BNW minor happiness exceeds native int");return static_cast<std::int32_t>(n);}
std::int32_t add(std::int32_t a,std::int32_t b){return integer(static_cast<std::int64_t>(a)+b);}
std::int32_t mul(std::int32_t a,std::int32_t b){return integer(static_cast<std::int64_t>(a)*b);}
const auto &named(const auto &rows,std::string_view name){const typename std::remove_reference_t<decltype(rows)>::value_type *found=nullptr;for(const auto &r:rows)if(r.name&&name==r.name){need(!found,"Duplicate BNW minor happiness field");found=&r;}need(found,"Missing BNW minor happiness field");return *found;}
std::int32_t scalar(const auto &rows,std::string_view name){const auto &v=named(rows,name).values;need(v.size()==1,"Malformed BNW minor happiness scalar");return integer(v[0]);}
std::int32_t define(const content::GlobalDefineCache &d,const std::string &name){const auto *r=d.find(name);need(r&&r->lookup.found&&r->lookup.error.empty()&&std::holds_alternative<std::int64_t>(r->lookup.value),"BNW minor happiness define unknown");return integer(std::get<std::int64_t>(r->lookup.value));}
std::string_view cstring(std::string_view s){return s.substr(0,s.find(char(0)));}
std::vector<std::int32_t> resource_array(const saves::PlayerCoreObservation &core,const char *name,const BnwResourceHappinessRules &rules){
    const auto &f=named(core.fields,name);need(f.encoding==saves::PlayerCoreEncoding::hashed_i32&&f.wire_count&&*f.wire_count==f.hashes.size()&&f.row_counts.empty()&&f.bytes.empty(),"BNW minor resource availability hash span malformed");
    std::vector<std::int32_t> out(rules.resource_slots(),0);std::size_t at=0;for(auto hash:f.hashes){need(hash<=UINT32_MAX,"BNW minor resource hash exceeds native width");if(!hash)continue;need(at<f.values.size(),"BNW minor resource hash payload missing");const auto *type=rules.resolve(hash);need(type&&type->table=="Resources"&&type->id>=0&&static_cast<std::uint64_t>(type->id)<out.size(),"BNW minor resource identity unknown");out.at(type->id)=integer(f.values[at++]);}need(at==f.values.size(),"BNW minor resource hash payload extra");return out;
}
std::int32_t luxuries_available(const saves::PlayerCoreObservation &core,const BnwResourceHappinessRules &rules){
    const auto used=resource_array(core,"m_paiNumResourceUsed",rules),total=resource_array(core,"m_paiNumResourceTotal",rules),exported=resource_array(core,"m_paiResourceExport",rules),imported=resource_array(core,"m_paiResourceImport",rules),minor=resource_array(core,"m_paiResourceFromMinors",rules),siphoned=resource_array(core,"m_paiResourcesSiphoned",rules);
    const auto bonus=scalar(core.fields,"m_iMinorResourceBonusCount")>0;std::int32_t count=0;
    for(const auto &[id,r]:rules.resources()){if(r.usage!=2)continue;const auto from_minor=bonus?mul(minor.at(id),rules.minor_quantity_multiplier())/100:minor.at(id);
        auto available=add(total.at(id),imported.at(id));available=add(available,from_minor);available=add(available,siphoned.at(id));available=add(available,integer(-static_cast<std::int64_t>(exported.at(id))));available=add(available,integer(-static_cast<std::int64_t>(used.at(id))));if(available>0)count=add(count,1);
    }return count;
}
}
BnwMinorHappinessRules BnwMinorHappinessRules::load(const content::RulesDatabase &db){
    BnwMinorHappinessRules r;r.resources_=BnwResourceHappinessRules::load(db);const auto types=db.cache_info_types();need(types.report().all_succeeded(),"BNW minor happiness types unavailable");
    const auto id=[&](const char *name,const char *table){const auto *entry=types.find(name);need(entry&&entry->table==table,"BNW minor era/type reference unknown");return integer(entry->id);};
    r.industrial_=id("ERA_INDUSTRIAL","Eras");r.medieval_=id("ERA_MEDIEVAL","Eras");
    // Serialized Info cache does not contain these fixed native enum rows.
    std::map<std::string,std::int32_t> traits;const auto trait_report=db.prefetch_info("MinorCivTraits",[&](auto slot,auto,const auto &row){const auto type=row.text("Type");need(type&&slot<=INT32_MAX,"BNW minor trait native enum row missing");traits.emplace(*type,integer(slot));return true;});
    need(trait_report.all_succeeded()&&traits.contains("MINOR_TRAIT_MERCANTILE")&&traits.at("MINOR_TRAIT_MERCANTILE")==3,"BNW fixed mercantile source enum binding differs");
    const auto report=db.prefetch_info("MinorCivilizations",[&](auto,auto,const auto &row){const auto type=row.text("Type"),trait=row.text("MinorCivTrait");need(type&&trait,"BNW minor Info type/trait missing");
        need(traits.contains(*trait),"BNW minor trait reference unknown");r.minors_.emplace(*type,traits.at(*trait));return true;});
    need(report.all_succeeded(),"BNW minor happiness Info unavailable");const auto d=db.cache_global_defines(content::GlobalDefineProfile::bnw);
    r.friend_threshold_=define(d,"FRIENDSHIP_THRESHOLD_FRIENDS");r.war_influence_=define(d,"MINOR_FRIENDSHIP_AT_WAR");
    const std::array<std::string,3> eras{"ANCIENT","MEDIEVAL","INDUSTRIAL"};for(std::size_t i=0;i<eras.size();++i){
        r.friend_flat_[i]=define(d,"FRIENDS_HAPPINESS_FLAT_BONUS_AMOUNT_"+eras[i]);r.ally_flat_[i]=define(d,"ALLIES_HAPPINESS_FLAT_BONUS_AMOUNT_"+eras[i]);
        r.friend_luxury_[i]=define(d,"FRIENDS_HAPPINESS_PER_LUXURY_BONUS_AMOUNT_"+eras[i]);r.ally_luxury_[i]=define(d,"ALLIES_HAPPINESS_PER_LUXURY_BONUS_AMOUNT_"+eras[i]);}
    return r;
}
std::int32_t BnwMinorHappinessRules::trait(std::string_view type) const {type=cstring(type);if(type.empty())return -1;need(minors_.contains(type),"BNW current minor civilization type unresolved");return minors_.find(type)->second;}
std::int32_t BnwMinorHappinessRules::era_band(std::int32_t era) const{return era>=industrial_?2:era>=medieval_?1:0;}
BnwMinorHappinessQuote quote_bnw_happiness_from_minors(const LiveSessionState &s,const BnwMinorHappinessRules &rules,const SessionCheckpointIdentity &identity,std::int64_t p){
    need(identity.payload_sha256.size()==64&&identity.rules_sha256==rules.rules_sha256(),"BNW minor happiness selected identity differs");
    need(p>=0&&p<64&&s.setup.profile==content::TechnologyProfile::bnw&&s.setup.players.size()==64&&s.world.all_players&&s.world.all_players->layout==bnw&&
         s.world.all_players->players.size()==64&&s.world.pregame&&s.world.pregame->layout==bnw,"BNW minor happiness complete native player registry required");
    const auto &actor=s.world.all_players->players.at(p);need(actor.saved_player_id==p&&scalar(actor.core.fields,"m_eID")==p,"BNW minor happiness actor identity differs");
    BnwMinorHappinessQuote out;std::optional<std::int32_t> luxury_count;
    for(std::int64_t m=22;m<63;++m){const auto &raw=s.world.all_players->players.at(m);const auto &base=s.setup.players.at(m);
        need(raw.index==static_cast<std::uint64_t>(m)&&raw.saved_player_id==m&&raw.core.layout==bnw&&scalar(raw.core.fields,"m_eID")==m&&
             scalar(raw.core.fields,"m_bAlive")==base.alive,"BNW minor happiness current alive roster differs");if(!base.alive)continue;
        BnwMinorHappinessTerm term;term.minor=m;if(p>=22){out.minors.push_back(term);continue;}
        const auto &types=s.world.pregame->archive.string_arrays.at("minor_civ_types");need(types.size()==64,"BNW current minor type roster incomplete");term.trait=rules.trait(types.at(m));
        if(term.trait!=3){out.minors.push_back(term);continue;}
        need(raw.planning.layout==bnw&&raw.planning.dimensions.major==22,"BNW minor influence source dimensions unknown");const auto &ai=named(raw.planning.blocks,"minor_civ_ai");
        need(named(ai.fields,"version").value==1,"BNW minor influence saved version unknown");const auto ally=integer(named(ai.fields,"m_eAlly").value);
        need(ally>=-1&&ally<22&&s.treasury.players.at(m).ally==ally,"BNW minor current ally cache owner differs");term.allied=ally==p;
        const auto &influence=named(ai.arrays,"m_aiFriendshipWithMajorTimes100");need(influence.columns==1&&influence.values.size()==22,"BNW minor influence native span incomplete");
        term.base_influence_times100=integer(influence.values.at(p));
        const auto team=s.setup.players.at(p).team,minor_team=base.team;
        need(team>=0&&team<64&&minor_team>=0&&minor_team<64&&s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->teams.size()==64&&
             s.world.pregame->archive.integer_arrays.at("team_types").at(p)==team&&s.world.pregame->archive.integer_arrays.at("team_types").at(m)==minor_team,
             "BNW minor happiness team binding differs");const auto &saved_team=s.world.teams->teams.at(team);
        need(saved_team.index==static_cast<std::uint64_t>(team)&&named(saved_team.fields,"id").value==team,"BNW minor happiness actor team identity differs");
        const auto &war=named(saved_team.arrays,"at_war");need(war.columns==1&&war.values.size()==64,"BNW minor war native span incomplete");
        need(war.values.at(minor_team)==0||war.values.at(minor_team)==1,"BNW minor war flag malformed");term.at_war=war.values.at(minor_team)!=0;
        need(s.player_research.teams.at(team).at_war.contains(minor_team)==term.at_war,"BNW minor current war owner differs");
        term.effective_influence=(term.at_war?mul(100,rules.war_influence()):term.base_influence_times100)/100;term.friends=term.effective_influence>=rules.friend_threshold();
        if(term.allied||term.friends){term.era=integer(named(saved_team.fields,"era").value);need(s.maintenance.team_eras.at(team)==term.era,"BNW minor current era owner differs");const auto band=rules.era_band(term.era);
            if(!luxury_count)luxury_count=luxuries_available(actor.core,rules.resources());
            term.luxuries=*luxury_count;
            if(term.allied)term.flat=add(term.flat,rules.flat(true,band));if(term.friends)term.flat=add(term.flat,rules.flat(false,band));
            if(term.allied)term.per_luxury=add(term.per_luxury,mul(term.luxuries,rules.luxury(true,band)));if(term.friends)term.per_luxury=add(term.per_luxury,mul(term.luxuries,rules.luxury(false,band)));
        }
        term.total=add(term.flat,term.per_luxury);out.total=add(out.total,term.total);out.minors.push_back(term);
    }return out;
}
}
