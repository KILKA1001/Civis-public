#include "civ5/runtime/bnw_happiness_policies.hpp"
#include "civ5/runtime/bnw_policy_branch_mapping.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <set>
#include <stdexcept>
#include <type_traits>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool ok,const char *why) {if(!ok)throw std::runtime_error(why);}
std::int32_t integer(std::int64_t n) {
    need(n>=INT32_MIN && n<=INT32_MAX,"BNW policy happiness exceeds native int");
    return static_cast<std::int32_t>(n);
}
std::int32_t add(std::int32_t a,std::int32_t b) {return integer(static_cast<std::int64_t>(a)+b);}
std::int32_t multiply(std::int32_t a,std::int32_t b) {return integer(static_cast<std::int64_t>(a)*b);}
const auto &named(const auto &rows,std::string_view name) {
    const typename std::remove_reference_t<decltype(rows)>::value_type *found=nullptr;
    for(const auto &row:rows)if(row.name && name==row.name) {
        need(!found,"Duplicate BNW policy happiness field");found=&row;
    }
    need(found,"Missing BNW policy happiness field");return *found;
}
std::int32_t scalar(const auto &fields,std::string_view name) {
    const auto &v=named(fields,name).values;
    need(v.size()==1,"Malformed BNW policy happiness scalar");return integer(v[0]);
}
}
BnwPolicyHappinessRules BnwPolicyHappinessRules::load(const content::RulesDatabase &database) {
    BnwPolicyHappinessRules result;
    result.sha256_=identify_bytes(database.gameplay_fingerprint_data()).sha256;
    result.types_=database.cache_info_types();
    need(result.types_.report().all_succeeded(),"BNW policy happiness type cache failed");
    const auto resolve=[&](const std::optional<std::string> &type,std::string_view table) {
        if(!type || type->empty())return std::int64_t{-1};
        const auto *entry=result.types_.find(*type);
        need(entry && entry->table==table,"BNW policy happiness rule reference unresolved");return entry->id;
    };
    std::map<std::int64_t,std::int64_t> free_openers;
    const auto branches=database.prefetch_info("PolicyBranchTypes",[&](auto slot,auto,const auto &row) {
        need(slot<=INT32_MAX,"BNW policy branch slot exceeds source width");
        const auto opener=resolve(row.text("FreePolicy"),"Policies");
        if(opener>=0)free_openers.emplace(static_cast<std::int64_t>(slot),opener);
        return true;
    });
    need(branches.all_succeeded() && branches.slots<=INT32_MAX,"BNW policy branch rules failed");
    result.branch_slots_=branches.slots;
    const auto policies=database.prefetch_info("Policies",[&](auto slot,auto,const auto &row) {
        need(slot<=INT32_MAX,"BNW policy slot exceeds source width");
        auto branch=resolve(row.text("PolicyBranchType"),"PolicyBranchTypes");
        branch=detail::bnw_policy_blocked_branch(static_cast<std::int64_t>(slot),branch,free_openers);
        need(branch==-1 || (branch>=0 && static_cast<std::uint64_t>(branch)<result.branch_slots_),
             "BNW policy blocked-branch binding invalid");
        const Policy policy{integer(row.integer("ExtraHappiness")),integer(row.integer("ExtraHappinessPerCity")),branch};
        need(result.policies_.emplace(static_cast<std::int64_t>(slot),policy).second,"Duplicate native policy slot");
        return true;
    });
    need(policies.all_succeeded() && policies.slots<=INT32_MAX,"BNW policy happiness rules failed");
    result.policy_slots_=policies.slots;return result;
}
std::vector<bool> BnwPolicyHappinessRules::flags(const saves::TeamHashedArray &wire,bool branches) const {
    need(wire.wire_count>=0 && static_cast<std::uint64_t>(wire.wire_count)==wire.rows.size(),
         "BNW policy hashed flag cardinality differs");
    std::vector<bool> result(branches?branch_slots_:policy_slots_,false);
    for(const auto &row:wire.rows) {
        need(row.type_hash<=UINT32_MAX,"BNW policy hash exceeds native width");
        if(!row.type_hash) {need(row.values.empty(),"BNW zero policy hash consumed a value");continue;}
        need(row.values.size()==1 && (row.values[0]==0 || row.values[0]==1),"BNW policy flag is not boolean");
        const auto *type=types_.find_hash(row.type_hash);
        need(type && type->table==(branches?"PolicyBranchTypes":"Policies") && type->id>=0 &&
             static_cast<std::uint64_t>(type->id)<result.size(),"BNW policy hash identity unresolved");
        result.at(static_cast<std::size_t>(type->id))=row.values[0]!=0;
    }
    return result;
}
void restore_bnw_policy_membership(const saves::SavedSessionSnapshot &world,
    ResearchContext &setup,const BnwPolicyHappinessRules &rules) {
    need(setup.profile==content::TechnologyProfile::bnw && setup.players.size()==64 && world.all_players &&
         world.all_players->layout==bnw && world.all_players->players.size()==64,
         "BNW policy import requires complete owned player registries");
    std::map<std::int64_t,std::set<std::int64_t>> memberships;
    for(std::size_t player=0;player<64;++player) {
        const auto &raw=world.all_players->players[player];
        need(raw.index==player && raw.saved_player_id==static_cast<std::int64_t>(player) && raw.core.layout==bnw &&
             scalar(raw.core.fields,"m_eID")==static_cast<std::int64_t>(player) && raw.strategy.layout==bnw &&
             setup.players.contains(static_cast<std::int64_t>(player)) && !setup.players.at(player).policies_resolved,
             "BNW policy import identity differs or current policy owner already resolved");
        const auto &block=named(raw.strategy.blocks,"policies");const auto version=named(block.fields,"version").value;
        need(version==1 || version==2,"BNW policy import saved version unknown");
        const auto flags=rules.flags(named(block.hashes,"has_policy"),false);
        auto &membership=memberships[static_cast<std::int64_t>(player)];
        for(std::size_t slot=0;slot<flags.size();++slot)if(flags[slot]) {
            need(rules.policies().contains(static_cast<std::int64_t>(slot)),"BNW policy import references null native Info entry");
            membership.insert(static_cast<std::int64_t>(slot));
        }
    }
    for(auto &[id,membership]:memberships) {
        auto &player=setup.players.at(id);player.policies=std::move(membership);player.policies_resolved=true;
    }
}
BnwPolicyHappinessQuote quote_bnw_happiness_from_policies(const LiveSessionState &state,
    const BnwPolicyHappinessRules &rules,const SessionCheckpointIdentity &identity,std::int64_t player) {
    need(identity.payload_sha256.size()==64 && identity.rules_sha256==rules.rules_sha256(),"BNW policy happiness selected rules differ");
    need(state.setup.profile==content::TechnologyProfile::bnw && state.cities.profile==content::TechnologyProfile::bnw &&
         state.world.all_players && state.world.all_players->layout==bnw && state.world.all_players->players.size()==64 &&
         state.setup.players.size()==64 && state.cities.players.size()==64 && player>=0 && player<64,
         "BNW policy happiness requires complete owned BNW player/city registries");
    const auto &raw=state.world.all_players->players.at(static_cast<std::size_t>(player));
    need(raw.index==static_cast<std::uint64_t>(player) && raw.saved_player_id==player && raw.core.layout==bnw &&
         scalar(raw.core.fields,"m_eID")==player && raw.strategy.layout==bnw && raw.cities.layout==bnw,
         "BNW policy happiness player identity differs");
    const auto &policies=named(raw.strategy.blocks,"policies");
    const auto version=named(policies.fields,"version").value;
    need(version==1 || version==2,"BNW policy happiness saved policy version unknown");
    const auto owned=rules.flags(named(policies.hashes,"has_policy"),false);
    const auto blocked=rules.flags(named(policies.hashes,"branch_blocked"),true);
    std::set<std::int64_t> membership;
    for(std::size_t slot=0;slot<owned.size();++slot)if(owned[slot])membership.insert(static_cast<std::int64_t>(slot));
    const auto &shared=state.setup.players.at(player);
    need(shared.policies_resolved && shared.policies==membership,
         "BNW policy happiness shared live policy owner differs");
    BnwPolicyHappinessQuote quote;
    for(std::size_t slot=0;slot<owned.size();++slot)if(owned[slot]) {
        quote.owned_policies=add(quote.owned_policies,1);
        need(rules.policies().contains(static_cast<std::int64_t>(slot)),"BNW owned policy has no native Info entry");
        const auto &policy=rules.policies().at(static_cast<std::int64_t>(slot));
        if(policy.blocked_branch>=0 && blocked.at(static_cast<std::size_t>(policy.blocked_branch)))continue;
        quote.active_policies=add(quote.active_policies,1);
        quote.flat=add(quote.flat,policy.extra);
        quote.per_city_modifier=add(quote.per_city_modifier,policy.per_city);
    }
    const auto &cities=state.cities.players.at(player).cities;
    need(raw.cities.cities.size()==cities.size() && raw.cities.header.wire_city_count==static_cast<std::int64_t>(cities.size()),
         "BNW policy happiness city collection count differs");
    quote.cities=integer(static_cast<std::int64_t>(cities.size()));
    quote.from_city_count=multiply(quote.cities,quote.per_city_modifier);
    quote.population_divisor=scalar(raw.core.fields,"m_iHappinessPerXPopulation");
    if(quote.population_divisor>0) {
        std::set<std::int64_t> ids;
        for(const auto &city:raw.cities.cities) {
            const auto id=scalar(city.core.fields,"m_iID");
            need(city.core.layout==bnw && scalar(city.core.fields,"m_eOwner")==player && ids.insert(id).second && cities.contains(id),
                 "BNW policy happiness city identity differs");
            const auto &live=cities.at(id);
            need(live.owner==player && live.id==id && live.source_core==std::optional{std::pair{city.core.start_offset,city.core.end_offset}} &&
                 scalar(city.core.fields,"m_iPopulation")==live.population && scalar(city.core.fields,"m_bPuppet")==live.puppet,
                 "BNW policy happiness current city owner differs from serialized binding");
            if(live.puppet)continue;
            const auto population=integer(live.population);
            need(population>=0,"BNW policy happiness city population negative");
            const auto amount=population/quote.population_divisor;
            quote.population_terms.push_back({id,population,amount});
            quote.from_population=add(quote.from_population,amount);
        }
    }
    quote.total=add(add(quote.flat,quote.from_city_count),quote.from_population);return quote;
}
}
