#include "civ5/runtime/bnw_happiness_resources.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool ok,const std::string &why) {if(!ok)throw std::runtime_error(why);}
std::int32_t integer(std::int64_t n) {
    need(n>=INT32_MIN && n<=INT32_MAX,"BNW resource happiness exceeds native int");
    return static_cast<std::int32_t>(n);
}
std::int32_t add(std::int32_t a,std::int32_t b) {return integer(static_cast<std::int64_t>(a)+b);}
std::int32_t percent(std::int32_t a,std::int32_t b) {
    return integer(static_cast<std::int64_t>(a)*b)/100;
}
template<class T> const T &named(const std::vector<T> &rows,std::string_view name) {
    const T *found=nullptr;
    for(const auto &row:rows)if(row.name && std::string_view(row.name)==name) {
        need(!found,"Duplicate BNW resource happiness field: "+std::string(name));found=&row;
    }
    need(found,"Missing BNW resource happiness field: "+std::string(name));return *found;
}
template<class T> std::int32_t scalar(const std::vector<T> &rows,std::string_view name) {
    const auto &row=named(rows,name);need(row.values.size()==1,"BNW resource scalar is malformed");
    return integer(row.values.front());
}
std::vector<std::int32_t> resource_array(const saves::PlayerCoreObservation &core,
    std::string_view name,const BnwResourceHappinessRules &rules) {
    const auto &field=named(core.fields,name);
    need(field.encoding==saves::PlayerCoreEncoding::hashed_i32 && field.wire_count &&
         *field.wire_count==field.hashes.size() && field.row_counts.empty() && field.bytes.empty(),
         "BNW resource hash-array shape is malformed");
    std::vector<std::int32_t> result(static_cast<std::size_t>(rules.resource_slots()),0);
    std::size_t value=0;
    for(const auto hash:field.hashes) {
        need(hash<=UINT32_MAX,"BNW resource hash exceeds native width");if(!hash)continue;
        need(value<field.values.size(),"BNW resource hash has no value");
        const auto *type=rules.resolve(hash);
        need(type && type->table=="Resources" && type->id>=0 &&
             static_cast<std::uint64_t>(type->id)<rules.resource_slots() && rules.resources().contains(type->id),
             "BNW resource hash is unresolved in selected rules");
        result[static_cast<std::size_t>(type->id)]=integer(field.values[value++]);
    }
    need(value==field.values.size(),"BNW resource hash-array has extra values");return result;
}
bool banned(const saves::GameLeaguesObservation &leagues,std::int64_t player,std::int64_t resource) {
    for(const auto &league:leagues.leagues) {
        if(std::none_of(league.members.begin(),league.members.end(),
            [player](const auto &member){return member.player==player;}))continue;
        for(const auto &resolution:league.active_resolutions)
            if(named(resolution.effects.flags,"no_resource_happiness").value &&
               resolution.proposer_decision.vote.choice==resource)return true;
    }
    return false;
}
std::int32_t define(const content::GlobalDefineCache &defines,std::string_view name) {
    const auto *entry=defines.find(name);
    need(entry && entry->kind==content::GlobalDefineKind::integer && entry->lookup.found &&
         entry->lookup.error.empty() && std::holds_alternative<std::int64_t>(entry->lookup.value),
         "BNW resource happiness define unavailable: "+std::string(name));
    return integer(std::get<std::int64_t>(entry->lookup.value));
}
}
BnwResourceHappinessRules BnwResourceHappinessRules::load(const content::RulesDatabase &database) {
    BnwResourceHappinessRules result;
    result.sha256_=identify_bytes(database.gameplay_fingerprint_data()).sha256;
    result.types_=database.cache_info_types();need(result.types_.report().all_succeeded(),"BNW resource InfoType cache failed");
    const auto report=database.prefetch_info("Resources",[&](auto slot,auto,const auto &row) {
        need(slot<=INT32_MAX,"BNW resource slot exceeds native int");
        Resource resource{integer(row.integer("Happiness")),integer(row.integer("ResourceUsage"))};
        need(resource.usage>=0 && resource.usage<=2 &&
             result.resources_.emplace(static_cast<std::int64_t>(slot),resource).second,
             "BNW resource usage/slot is malformed");return true;
    });
    need(report.all_succeeded() && report.slots<=INT32_MAX,"BNW resource rules failed");result.slots_=report.slots;
    const auto defines=database.cache_global_defines(content::GlobalDefineProfile::bnw);
    result.minor_quantity_=define(defines,"MINOR_POLICY_RESOURCE_MULTIPLIER");
    result.minor_happiness_=define(defines,"MINOR_POLICY_RESOURCE_HAPPINESS_MULTIPLIER");
    result.variety_=define(defines,"HAPPINESS_PER_EXTRA_LUXURY");return result;
}
BnwResourceHappinessQuote quote_bnw_happiness_from_resources(const LiveSessionState &state,
    const BnwResourceHappinessRules &rules,const SessionCheckpointIdentity &identity,std::int64_t player) {
    need(identity.payload_sha256.size()==64 && identity.rules_sha256==rules.rules_sha256(),
         "BNW resource happiness selected identity differs");
    need(state.setup.profile==content::TechnologyProfile::bnw && state.world.all_players &&
         state.world.all_players->layout==bnw && state.world.all_players->players.size()==64 &&
         state.world.leagues.layout==bnw && player>=0 && player<64,
         "BNW resource happiness live profile/roster is incomplete");
    const auto &raw=state.world.all_players->players.at(static_cast<std::size_t>(player));
    need(raw.saved_player_id==player && raw.core.layout==bnw && raw.traits.layout==bnw &&
         scalar(raw.core.fields,"m_eID")==player,"BNW resource happiness player identity differs");
    const auto used=resource_array(raw.core,"m_paiNumResourceUsed",rules);
    const auto total=resource_array(raw.core,"m_paiNumResourceTotal",rules);
    const auto exported=resource_array(raw.core,"m_paiResourceExport",rules);
    const auto imported=resource_array(raw.core,"m_paiResourceImport",rules);
    const auto minors=resource_array(raw.core,"m_paiResourceFromMinors",rules);
    const auto siphoned=resource_array(raw.core,"m_paiResourcesSiphoned",rules);
    const auto retention=scalar(raw.traits.fields,"m_iLuxuryHappinessRetention");
    const auto extra=scalar(raw.core.fields,"m_iExtraHappinessPerLuxury");
    const bool minor_bonus=scalar(raw.core.fields,"m_iMinorResourceBonusCount")>0;
    BnwResourceHappinessQuote quote;
    for(const auto &[slot,resource]:rules.resources()) {
        if(resource.usage!=2)continue;
        const auto i=static_cast<std::size_t>(slot);BnwLuxuryHappinessTerm term;term.resource=slot;
        term.banned=banned(state.world.leagues,player,slot);
        term.from_minors=minor_bonus?percent(minors[i],rules.minor_quantity_multiplier()):minors[i];
        auto available=add(total[i],imported[i]);available=add(available,term.from_minors);
        available=add(available,siphoned[i]);available=add(available,integer(-static_cast<std::int64_t>(exported[i])));
        term.available=add(available,integer(-static_cast<std::int64_t>(used[i])));
        const auto happiness=term.banned?0:resource.happiness;
        if(term.available>0)term.base=happiness;
        else if(retention>0 && exported[i]>0) {term.retained_export=true;term.base=percent(happiness,retention);}
        term.after_minor_bonus=term.base;
        if(term.base) {
            if(minor_bonus && term.from_minors>0)term.after_minor_bonus=percent(term.base,rules.minor_happiness_multiplier());
            term.extra=extra;term.total=add(term.after_minor_bonus,extra);
        }
        if(term.base>0)quote.positive_resource_types=add(quote.positive_resource_types,1);
        quote.resource_happiness=add(quote.resource_happiness,term.total);quote.luxuries.push_back(term);
    }
    if(quote.positive_resource_types>1)
        quote.variety=integer(static_cast<std::int64_t>(quote.positive_resource_types-1)*rules.variety_bonus());
    quote.total=add(quote.resource_happiness,quote.variety);return quote;
}
} // namespace civ5::runtime
