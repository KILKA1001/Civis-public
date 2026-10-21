#include "civ5/runtime/bnw_first_contact_approach_after_load.hpp"
#include "civ5/runtime/bnw_personality_flavors.hpp"
#include <climits>
#include <initializer_list>
#include <set>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <variant>

namespace civ5::runtime {
namespace {
const saves::PlayerDiplomacyField *field(
    const saves::PlayerDiplomacyObservation &observation,
    std::string_view name) {
    const saves::PlayerDiplomacyField *found=nullptr;
    for(const auto &candidate:observation.fields) {
        if(candidate.name && candidate.name==name) {
            if(found) return nullptr;
            found=&candidate;
        }
    }
    return found;
}
const saves::PlayerStrategyBlock *grand(
    const saves::PlayerStrategyObservation &observation) {
    const saves::PlayerStrategyBlock *found=nullptr;
    for(const auto &block:observation.blocks) {
        if(block.name && std::string_view(block.name)=="grand_strategy_ai") {
            if(found) return nullptr;
            found=&block;
        }
    }
    return found;
}
std::optional<std::int64_t> active_strategy(
    const saves::PlayerStrategyObservation &observation) {
    const auto *block=grand(observation);
    if(!block) return std::nullopt;
    std::optional<std::int64_t> value;
    for(const auto &entry:block->fields) {
        if(entry.name && std::string_view(entry.name)=="active_strategy") {
            if(value) return std::nullopt;
            value=entry.value;
        }
    }
    return value;
}
std::optional<std::int64_t> rule(
    const content::GlobalDefineCache &defines,std::string_view name) {
    const auto *entry=defines.find(name);
    if(!entry || entry->lookup.sqlite_code!=0 ||
       !std::holds_alternative<std::int64_t>(entry->lookup.value))
        return std::nullopt;
    const auto value=std::get<std::int64_t>(entry->lookup.value);
    if(value<INT32_MIN || value>INT32_MAX) return std::nullopt;
    return value;
}
std::int64_t checked_row(const saves::PlayerDiplomacyObservation &saved,
    const saves::PlayerDiplomacyObservation &owned,std::string_view name,
    saves::DiplomacyEncoding encoding,std::size_t rows,std::size_t index) {
    const auto *a=field(saved,name),*b=field(owned,name);
    if(!a || !b || a->encoding!=encoding || b->encoding!=encoding ||
       a->rows!=1 || b->rows!=1 || a->columns!=rows || b->columns!=rows ||
       a->values.size()!=rows || b->values!=a->values || index>=rows)
        throw std::runtime_error(std::string("BNW diplomacy row unavailable: ")+std::string(name));
    const auto value=a->values[index];
    if(encoding==saves::DiplomacyEncoding::boolean && (value<0 || value>1))
        throw std::runtime_error(std::string("BNW diplomacy boolean invalid: ")+std::string(name));
    return value;
}
std::int64_t checked_team_flag(const saves::SavedSession &source,
    const LiveSessionState &state,std::int64_t team,std::string_view name) {
    if(!source.snapshot.teams || !state.world.teams || team<0 || team>=63 ||
       source.snapshot.teams->teams.size()!=64 ||
       state.world.teams->teams.size()!=64)
        throw std::runtime_error("BNW team promise roster unavailable");
    const auto &saved=source.snapshot.teams->teams[static_cast<std::size_t>(team)];
    const auto &owned=state.world.teams->teams[static_cast<std::size_t>(team)];
    if(saved.index!=static_cast<std::size_t>(team) ||
       owned.index!=static_cast<std::size_t>(team))
        throw std::runtime_error("BNW team promise ID inconsistent");
    std::optional<std::int64_t> a,b;
    for(const auto &entry:saved.fields) if(entry.name && name==entry.name) {
        if(a) throw std::runtime_error("BNW duplicate source team promise flag");
        a=entry.value;
    }
    for(const auto &entry:owned.fields) if(entry.name && name==entry.name) {
        if(b) throw std::runtime_error("BNW duplicate owned team promise flag");
        b=entry.value;
    }
    if(!a || !b || *a!=*b || *a<0 || *a>1)
        throw std::runtime_error(std::string("BNW team promise flag unavailable: ")+std::string(name));
    return *a;
}
using Bias=std::pair<const char *,std::size_t>;
void add_rules(std::array<std::int64_t,7> &weights,
    const content::GlobalDefineCache &defines,
    std::initializer_list<Bias> biases,std::int64_t divisor=1) {
    for(const auto &[name,index]:biases) {
        const auto value=rule(defines,name);
        if(!value || index>=weights.size() || divisor<=0)
            throw std::runtime_error(std::string("BNW approach rule unavailable: ")+name);
        const auto change=*value/divisor; // C++ truncates negative integers toward zero like SDK.
        if(weights[index]+change<INT32_MIN || weights[index]+change>INT32_MAX)
            throw std::runtime_error(std::string("BNW approach native weight overflow: ")+name);
        weights[index]+=change;
    }
}
std::int64_t checked_nuke_count(const saves::SavedPlayerObservation &saved,
    const saves::SavedPlayerObservation &owned) {
    const saves::PlayerCoreField *a=nullptr,*b=nullptr;
    for(const auto &entry:saved.core.fields)
        if(entry.name && std::string_view(entry.name)=="m_iNumNukeUnits") {
            if(a) throw std::runtime_error("BNW duplicate source nuke counter");
            a=&entry;
        }
    for(const auto &entry:owned.core.fields)
        if(entry.name && std::string_view(entry.name)=="m_iNumNukeUnits") {
            if(b) throw std::runtime_error("BNW duplicate owned nuke counter");
            b=&entry;
        }
    if(!a || !b || a->encoding!=saves::PlayerCoreEncoding::i32 ||
       b->encoding!=saves::PlayerCoreEncoding::i32 || a->values.size()!=1 ||
       b->values!=a->values || a->values[0]<0 || a->values[0]>INT32_MAX)
        throw std::runtime_error("BNW CvPlayer::getNumNukeUnits source/owned counter unavailable");
    return a->values[0];
}
std::int64_t checked_personality_flavor(
    const saves::SavedPlayerObservation &saved,
    const saves::SavedPlayerObservation &owned,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types) {
    const auto *nuke=types.find("FLAVOR_USE_NUKE");
    if(!nuke || nuke->table!="Flavors" || nuke->id<0 || nuke->id>1023)
        throw std::runtime_error("BNW FLAVOR_USE_NUKE active type unavailable");
    const auto &personality=checked_bnw_personality_flavors(saved,owned,database);
    if(nuke->id>=static_cast<std::int64_t>(personality.size()))
        throw std::runtime_error("BNW FlavorManager source/owned/rules dimensions differ");
    return personality[static_cast<std::size_t>(nuke->id)];
}
std::int64_t checked_grand_nuke_mod(
    const saves::SavedPlayerObservation &saved,
    const saves::SavedPlayerObservation &owned,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types) {
    const auto a=active_strategy(saved.strategy),b=active_strategy(owned.strategy);
    if(!a || !b || *a!=*b || *a< -1)
        throw std::runtime_error("BNW active GrandStrategy source/owned ID differs");
    if(*a==-1) return 0;
    std::optional<std::string> strategy_type;
    for(const auto &[_,entry]:types.entries())
        if(entry.table=="AIGrandStrategies" && entry.id==*a) {
            if(strategy_type)
                throw std::runtime_error("BNW duplicate active GrandStrategy ID");
            strategy_type=entry.type;
        }
    if(!strategy_type)
        throw std::runtime_error("BNW active GrandStrategy type unavailable");
    std::optional<std::int64_t> mod;
    const auto report=database.query_rows({
        "SELECT Flavor FROM AIGrandStrategy_FlavorMods "
        "WHERE AIGrandStrategyType=? AND FlavorType=?",
        {*strategy_type,std::string("FLAVOR_USE_NUKE")}},
        [&](const auto &row) {
            if(mod) throw std::runtime_error("BNW duplicate GrandStrategy nuke flavor mod");
            mod=row.integer("Flavor");
        },2);
    if(!report.all_succeeded())
        throw std::runtime_error("BNW GrandStrategy nuke flavor mod rule unavailable");
    if(mod && (*mod<INT32_MIN || *mod>INT32_MAX))
        throw std::runtime_error("BNW GrandStrategy nuke flavor mod out of range");
    return mod.value_or(0); // CvDatabaseUtility::SetFlavors defaults to zero.
}
std::int64_t checked_team_array(const saves::SavedSession &source,
    const LiveSessionState &state,std::int64_t team,std::string_view name,
    std::int64_t other) {
    if(!source.snapshot.teams || !state.world.teams || team<0 || team>=63 ||
       other<0 || other>=63 || source.snapshot.teams->teams.size()!=64 ||
       state.world.teams->teams.size()!=64)
        throw std::runtime_error("BNW team relation roster unavailable");
    const auto &a=source.snapshot.teams->teams[static_cast<std::size_t>(team)];
    const auto &b=state.world.teams->teams[static_cast<std::size_t>(team)];
    auto find=[&](const saves::TeamObservation &t) {
        const saves::PlotArrayObservation *value=nullptr;
        for(const auto &entry:t.arrays) if(entry.name && name==entry.name) {
            if(value) throw std::runtime_error("BNW duplicate team relation array");
            value=&entry;
        }
        return value;
    };
    const auto *x=find(a),*y=find(b);
    if(a.index!=static_cast<std::size_t>(team) ||
       b.index!=static_cast<std::size_t>(team) || !x || !y ||
       x->columns!=1 || y->columns!=1 || x->values.size()!=64 ||
       y->values.size()!=64 ||
       y->values[static_cast<std::size_t>(other)]!=
           x->values[static_cast<std::size_t>(other)])
        throw std::runtime_error(std::string("BNW team relation unavailable: ")+std::string(name));
    const auto value=x->values[static_cast<std::size_t>(other)];
    if((name=="has_met" || name=="at_war") && (value<0 || value>1))
        throw std::runtime_error("BNW invalid team relation boolean");
    return value;
}
std::int64_t checked_player_team(const saves::SavedSession &source,
    const LiveSessionState &state,std::int64_t player) {
    if(!source.snapshot.pregame || !state.world.pregame ||
       player<0 || player>=63 || !state.setup.players.contains(player))
        throw std::runtime_error("BNW player team setup unavailable");
    const auto &a=source.snapshot.pregame->archive.integer_arrays;
    const auto &b=state.world.pregame->archive.integer_arrays;
    const auto x=a.find("team_types"),y=b.find("team_types");
    if(x==a.end() || y==b.end() || x->second.size()<64 ||
       y->second!=x->second)
        throw std::runtime_error("BNW source/owned team mapping differs");
    const auto team=x->second[static_cast<std::size_t>(player)];
    if(team<0 || team>=63 || state.setup.players.at(player).team!=team)
        throw std::runtime_error("BNW player team mirror inconsistent");
    return team;
}
bool checked_player_valid(const saves::SavedSession &source,
    const LiveSessionState &state,std::int64_t recipient_team,
    std::int64_t other) {
    if(!source.snapshot.all_players || !state.world.all_players ||
       other<0 || other>=63 || state.cities.profile!=content::TechnologyProfile::bnw ||
       !state.cities.players.contains(other) ||
       !state.setup.players.contains(other))
        throw std::runtime_error("BNW IsPlayerValid roster unavailable");
    const auto &a=source.snapshot.all_players->players.at(
        static_cast<std::size_t>(other));
    const auto &b=state.world.all_players->players.at(
        static_cast<std::size_t>(other));
    if(a.saved_player_id!=other || b.saved_player_id!=other)
        throw std::runtime_error("BNW IsPlayerValid player ID inconsistent");
    const saves::PlayerCoreField *alive_a=nullptr,*alive_b=nullptr;
    for(const auto &entry:a.core.fields) if(entry.name &&
        std::string_view(entry.name)=="m_bAlive") {
        if(alive_a) throw std::runtime_error("BNW duplicate source alive field");
        alive_a=&entry;
    }
    for(const auto &entry:b.core.fields) if(entry.name &&
        std::string_view(entry.name)=="m_bAlive") {
        if(alive_b) throw std::runtime_error("BNW duplicate owned alive field");
        alive_b=&entry;
    }
    if(!alive_a || !alive_b || alive_a->values.size()!=1 ||
       alive_b->values!=alive_a->values || alive_a->values[0]<0 ||
       alive_a->values[0]>1 ||
       state.setup.players.at(other).alive!=(alive_a->values[0]==1))
        throw std::runtime_error("BNW IsPlayerValid alive state inconsistent");
    if(alive_a->values[0]==0) return false;
    if(a.cities.header.wire_city_count<0 ||
       a.cities.cities.size()!=
           static_cast<std::size_t>(a.cities.header.wire_city_count) ||
       b.cities.header.wire_city_count!=a.cities.header.wire_city_count ||
       b.cities.cities.size()!=a.cities.cities.size() ||
       state.cities.players.at(other).cities.size()!=a.cities.cities.size())
        throw std::runtime_error("BNW IsPlayerValid city roster inconsistent");
    if(a.cities.cities.empty()) return false;
    const auto other_team=checked_player_team(source,state,other);
    if(checked_team_array(source,state,recipient_team,"has_met",other_team)==0)
        return false;
    return recipient_team!=other_team;
}
std::int64_t checked_proximity(const saves::SavedPlayerObservation &saved,
    const saves::SavedPlayerObservation &owned,std::int64_t target) {
    const saves::PlayerCoreField *a=nullptr,*b=nullptr;
    for(const auto &entry:saved.core.fields)
        if(entry.name && std::string_view(entry.name)=="m_aiProximityToPlayer") {
            if(a) throw std::runtime_error("BNW duplicate source proximity vector");
            a=&entry;
        }
    for(const auto &entry:owned.core.fields)
        if(entry.name && std::string_view(entry.name)=="m_aiProximityToPlayer") {
            if(b) throw std::runtime_error("BNW duplicate owned proximity vector");
            b=&entry;
        }
    if(!a || !b || target<0 || target>=63 ||
       a->encoding!=saves::PlayerCoreEncoding::vector_i32 ||
       b->encoding!=saves::PlayerCoreEncoding::vector_i32 ||
       a->values.size()!=64 || b->values.size()!=64)
        throw std::runtime_error("BNW proximity vector unavailable");
    for(std::size_t i=0;i<64;++i)
        if(i!=static_cast<std::size_t>(target) && a->values[i]!=b->values[i])
            throw std::runtime_error("BNW unrelated proximity entries changed");
    const auto value=b->values[static_cast<std::size_t>(target)];
    if(value< -1 || value>3)
        throw std::runtime_error("BNW owned proximity enum unavailable");
    return value;
}
std::int64_t checked_coop_matrix(const saves::SavedPlayerObservation &saved,
    const saves::SavedPlayerObservation &owned,std::string_view name,
    saves::DiplomacyEncoding encoding,std::size_t other,std::size_t target) {
    const auto *a=field(saved.diplomacy,name),*b=field(owned.diplomacy,name);
    if(!a || !b || a->encoding!=encoding || b->encoding!=encoding ||
       a->rows!=22 || b->rows!=22 || a->columns!=22 || b->columns!=22 ||
       a->values.size()!=22*22 || b->values!=a->values ||
       other>=22 || target>=22)
        throw std::runtime_error(std::string("BNW coop-war source/owned matrix unavailable: ")+std::string(name));
    const auto value=a->values[other*22+target];
    if(encoding==saves::DiplomacyEncoding::i8 && (value< -1 || value>2))
        throw std::runtime_error("BNW coop-war state enum unavailable");
    if(encoding==saves::DiplomacyEncoding::i16 &&
       (value<INT16_MIN || value>INT16_MAX))
        throw std::runtime_error("BNW coop-war counter encoding unavailable");
    return value;
}
} // namespace

BnwFirstContactApproachAfterLoad preflight_bnw_first_contact_approach_after_load(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity,
    const BnwDiplomacyScratchpadLoad &scratchpad) {
    BnwFirstContactApproachAfterLoad result;
    const auto prior=preflight_bnw_first_contact_approach(
        source,state,request,database,types,defines,identity);
    result.recipient=prior.recipient;
    result.target=prior.target;
    result.target_team=prior.target_team;
    if(!prior.source_verified || !prior.major_branch_verified ||
       prior.gate!=BnwFirstContactApproachGate::scratchpad_unrestored) {
        result.reason=prior.reason;
        return result;
    }
    if(!scratchpad.current() || !scratchpad.matches_load_clock(state) ||
       scratchpad.identity().payload_sha256!=identity.payload_sha256 ||
       scratchpad.identity().rules_sha256!=identity.rules_sha256 ||
       scratchpad.value_at(result.target)!=-1) {
        result.gate=BnwFirstContactApproachAfterLoadGate::scratchpad_foreign_or_stale;
        result.reason="BNW post-Read approach scratchpad is stale or has foreign identity";
        return result;
    }
    result.neutral_default=prior.neutral_default;
    result.weights[6]=prior.neutral_default;
    const auto current=rule(defines,"APPROACH_BIAS_FOR_CURRENT");
    if(defines.profile()!=content::GlobalDefineProfile::bnw || !current) {
        result.gate=BnwFirstContactApproachAfterLoadGate::current_bias_rule_unavailable;
        result.reason="BNW current approach bias is unavailable from active rules";
        return result;
    }
    result.current_bias=*current;
    result.weights[6]+=*current; // Reset -1 -> MAJOR_CIV_APPROACH_NEUTRAL.
    // Since eOldApproach is neutral, SDK branches at 2786, 2790, 2795
    // cannot add deceptive, hostile or current-war biases.
    if(!source.snapshot.all_players || !state.world.all_players ||
       source.snapshot.all_players->players.size()!=64 ||
       state.world.all_players->players.size()!=64 ||
       result.recipient<0 || result.recipient>=63) {
        result.gate=BnwFirstContactApproachAfterLoadGate::war_state_unavailable;
        result.reason="BNW post-Read approach player roster is unavailable";
        return result;
    }
    const auto &saved=source.snapshot.all_players->players.at(
        static_cast<std::size_t>(result.recipient));
    const auto &owned=state.world.all_players->players.at(
        static_cast<std::size_t>(result.recipient));
    const auto *saved_war=field(saved.diplomacy,"m_paeWarState");
    const auto *owned_war=field(owned.diplomacy,"m_paeWarState");
    const auto target=static_cast<std::size_t>(result.target);
    if(!saved_war || !owned_war ||
       saved_war->encoding!=saves::DiplomacyEncoding::i8 ||
       owned_war->encoding!=saves::DiplomacyEncoding::i8 ||
       saved_war->rows!=1 || owned_war->rows!=1 ||
       saved_war->columns!=63 || owned_war->columns!=63 ||
       saved_war->values.size()!=63 || owned_war->values!=saved_war->values ||
       target>=63 || saved_war->values[target]<-1 ||
       saved_war->values[target]>5) {
        result.gate=BnwFirstContactApproachAfterLoadGate::war_state_unavailable;
        result.reason="BNW GetWarState source/owned i8 row is unavailable or changed";
        return result;
    }
    result.war_state=saved_war->values[target];
    if(result.war_state==-1 || result.war_state>1) {
        const auto saved_active=active_strategy(saved.strategy);
        const auto owned_active=active_strategy(owned.strategy);
        const auto *conquest=types.find("AIGRANDSTRATEGY_CONQUEST");
        if(!saved_active || !owned_active ||
           *saved_active!=*owned_active || !conquest ||
           conquest->table!="AIGrandStrategies") {
            result.gate=BnwFirstContactApproachAfterLoadGate::conquest_strategy_unavailable;
            result.reason="BNW conquest GrandStrategy source/owned ID or active rules is unavailable";
            return result;
        }
        result.world_conquest=*saved_active==conquest->id;
        if(result.world_conquest) {
            const auto bias=rule(defines,"APPROACH_WAR_CONQUEST_GRAND_STRATEGY");
            if(!bias) {
                result.gate=BnwFirstContactApproachAfterLoadGate::conquest_strategy_unavailable;
                result.reason="BNW conquest approach bias is unavailable";
                return result;
            }
            result.weights[0]+=*bias;
        }
    }
    const auto *saved_personality=field(
        saved.diplomacy,"m_paiPersonalityMajorCivApproachBiases");
    const auto *owned_personality=field(
        owned.diplomacy,"m_paiPersonalityMajorCivApproachBiases");
    if(!saved_personality || !owned_personality ||
       saved_personality->encoding!=saves::DiplomacyEncoding::i8 ||
       owned_personality->encoding!=saves::DiplomacyEncoding::i8 ||
       saved_personality->rows!=1 || owned_personality->rows!=1 ||
       saved_personality->columns!=7 || owned_personality->columns!=7 ||
       saved_personality->values.size()!=7 ||
       owned_personality->values!=saved_personality->values) {
        result.gate=BnwFirstContactApproachAfterLoadGate::personality_unavailable;
        result.reason="BNW personality approach biases are unavailable or changed";
        return result;
    }
    for(std::size_t i=0;i<7;++i)
        result.weights[i]+=saved_personality->values[i];
    for(const auto weight:result.weights) {
        if(weight<INT32_MIN || weight>INT32_MAX) {
            result.gate=BnwFirstContactApproachAfterLoadGate::personality_unavailable;
            result.reason="BNW approach prefix overflows native 32-bit weights";
            return result;
        }
    }
    result.pre_opinion_weights=result.weights;
    const auto *saved_opinion=field(saved.diplomacy,"m_paeMajorCivOpinion");
    const auto *owned_opinion=field(owned.diplomacy,"m_paeMajorCivOpinion");
    if(!saved_opinion || !owned_opinion ||
       saved_opinion->encoding!=saves::DiplomacyEncoding::i8 ||
       owned_opinion->encoding!=saves::DiplomacyEncoding::i8 ||
       saved_opinion->rows!=1 || owned_opinion->rows!=1 ||
       saved_opinion->columns!=22 || owned_opinion->columns!=22 ||
       saved_opinion->values.size()!=22 ||
       owned_opinion->values!=saved_opinion->values ||
       target>=22 || saved_opinion->values[target]<-1 ||
       saved_opinion->values[target]>6) {
        result.gate=BnwFirstContactApproachAfterLoadGate::opinion_unavailable;
        result.reason="BNW GetMajorCivOpinion source/owned i8 row is unavailable or changed";
        return result;
    }
    result.opinion=saved_opinion->values[target];
    // Expansion2 CvDiplomacyAI.cpp:2829-2874. Rows are opinion enum values
    // UNFORGIVABLE..ALLY, columns are approach enum WAR..NEUTRAL. A nullptr
    // means that this branch does not ask GC for that particular define.
    static constexpr const char *opinion_rules[7][7]={
        {"APPROACH_OPINION_UNFORGIVABLE_WAR",
         "APPROACH_OPINION_UNFORGIVABLE_HOSTILE",
         "APPROACH_OPINION_UNFORGIVABLE_DECEPTIVE",
         "APPROACH_OPINION_UNFORGIVABLE_GUARDED",nullptr,nullptr,nullptr},
        {"APPROACH_OPINION_ENEMY_WAR","APPROACH_OPINION_ENEMY_HOSTILE",
         "APPROACH_OPINION_ENEMY_DECEPTIVE","APPROACH_OPINION_ENEMY_GUARDED",
         nullptr,nullptr,nullptr},
        {"APPROACH_OPINION_COMPETITOR_WAR",
         "APPROACH_OPINION_COMPETITOR_HOSTILE",
         "APPROACH_OPINION_COMPETITOR_DECEPTIVE",
         "APPROACH_OPINION_COMPETITOR_GUARDED",nullptr,nullptr,nullptr},
        {nullptr,nullptr,"APPROACH_OPINION_NEUTRAL_DECEPTIVE",nullptr,
         nullptr,"APPROACH_OPINION_NEUTRAL_FRIENDLY",nullptr},
        {nullptr,"APPROACH_OPINION_FAVORABLE_HOSTILE",
         "APPROACH_OPINION_FAVORABLE_DECEPTIVE",nullptr,nullptr,
         "APPROACH_OPINION_FAVORABLE_FRIENDLY",nullptr},
        {nullptr,"APPROACH_OPINION_FRIEND_HOSTILE",nullptr,nullptr,nullptr,
         "APPROACH_OPINION_FRIEND_FRIENDLY",nullptr},
        {nullptr,nullptr,nullptr,nullptr,nullptr,
         "APPROACH_OPINION_ALLY_FRIENDLY",nullptr},
    };
    if(result.opinion>=0) {
        for(std::size_t i=0;i<7;++i) {
            const auto *name=opinion_rules[result.opinion][i];
            if(!name) continue;
            const auto bias=rule(defines,name);
            if(!bias || result.weights[i]+*bias<INT32_MIN ||
               result.weights[i]+*bias>INT32_MAX) {
                result.gate=BnwFirstContactApproachAfterLoadGate::opinion_rule_unavailable;
                result.reason="BNW active opinion rule is missing or overflows native approach weight";
                return result;
            }
            result.weights[i]+=*bias;
        }
    }
    result.pre_friendship_weights=result.weights;
    const auto *saved_friendship=field(saved.diplomacy,"m_pabDoFAccepted");
    const auto *owned_friendship=field(owned.diplomacy,"m_pabDoFAccepted");
    if(!saved_friendship || !owned_friendship ||
       saved_friendship->encoding!=saves::DiplomacyEncoding::boolean ||
       owned_friendship->encoding!=saves::DiplomacyEncoding::boolean ||
       saved_friendship->rows!=1 || owned_friendship->rows!=1 ||
       saved_friendship->columns!=22 || owned_friendship->columns!=22 ||
       saved_friendship->values.size()!=22 ||
       owned_friendship->values!=saved_friendship->values ||
       saved_friendship->values[target]<0 ||
       saved_friendship->values[target]>1) {
        result.gate=BnwFirstContactApproachAfterLoadGate::friendship_unavailable;
        result.reason="BNW IsDoFAccepted source/owned boolean row is unavailable or changed";
        return result;
    }
    result.friendship_accepted=saved_friendship->values[target]==1;
    if(result.friendship_accepted) {
        // Expansion2 CvDiplomacyAI.cpp:2871-2876: deceptive, friendly,
        // hostile, guarded. The read order matters if a rule is unavailable.
        static constexpr std::pair<const char *,std::size_t> friendship_rules[]={
            {"APPROACH_DECEPTIVE_WORKING_WITH_PLAYER",2},
            {"APPROACH_FRIENDLY_WORKING_WITH_PLAYER",5},
            {"APPROACH_HOSTILE_WORKING_WITH_PLAYER",1},
            {"APPROACH_GUARDED_WORKING_WITH_PLAYER",3},
        };
        for(const auto &[name,index]:friendship_rules) {
            const auto bias=rule(defines,name);
            if(!bias || result.weights[index]+*bias<INT32_MIN ||
               result.weights[index]+*bias>INT32_MAX) {
                result.gate=BnwFirstContactApproachAfterLoadGate::friendship_rule_unavailable;
                result.reason="BNW active friendship rule is missing or overflows native approach weight";
                return result;
            }
            result.weights[index]+=*bias;
        }
    }
    result.pre_denouncement_weights=result.weights;
    const auto *saved_denounced=field(saved.diplomacy,"m_pabDenouncedPlayer");
    const auto *owned_denounced=field(owned.diplomacy,"m_pabDenouncedPlayer");
    if(!saved_denounced || !owned_denounced ||
       saved_denounced->encoding!=saves::DiplomacyEncoding::boolean ||
       owned_denounced->encoding!=saves::DiplomacyEncoding::boolean ||
       saved_denounced->rows!=1 || owned_denounced->rows!=1 ||
       saved_denounced->columns!=22 || owned_denounced->columns!=22 ||
       saved_denounced->values.size()!=22 ||
       owned_denounced->values!=saved_denounced->values ||
       saved_denounced->values[target]<0 ||
       saved_denounced->values[target]>1) {
        result.gate=BnwFirstContactApproachAfterLoadGate::denounced_unavailable;
        result.reason="BNW IsDenouncedPlayer source/owned boolean row is unavailable or changed";
        return result;
    }
    result.denounced_target=saved_denounced->values[target]==1;
    if(result.denounced_target) {
        // Expansion2 CvDiplomacyAI.cpp:2879-2887: WAR, HOSTILE,
        // GUARDED, FRIENDLY, DECEPTIVE in this source order.
        static constexpr std::pair<const char *,std::size_t> denounced_rules[]={
            {"APPROACH_WAR_DENOUNCED",0},
            {"APPROACH_HOSTILE_DENOUNCED",1},
            {"APPROACH_GUARDED_DENOUNCED",3},
            {"APPROACH_FRIENDLY_DENOUNCED",5},
            {"APPROACH_DECEPTIVE_DENOUNCED",2},
        };
        for(const auto &[name,index]:denounced_rules) {
            const auto bias=rule(defines,name);
            if(!bias || result.weights[index]+*bias<INT32_MIN ||
               result.weights[index]+*bias>INT32_MAX) {
                result.gate=BnwFirstContactApproachAfterLoadGate::denounced_rule_unavailable;
                result.reason="BNW active denouncement rule is missing or overflows native approach weight";
                return result;
            }
            result.weights[index]+=*bias;
        }
    }
    result.pre_betrayal_weights=result.weights;
    const auto own_bool=[&](std::string_view name) {
        return checked_row(saved.diplomacy,owned.diplomacy,name,
                           saves::DiplomacyEncoding::boolean,22,target)==1;
    };
    const auto own_i8=[&](std::string_view name) {
        return checked_row(saved.diplomacy,owned.diplomacy,name,
                           saves::DiplomacyEncoding::i8,22,target);
    };
    const auto own_i16=[&](std::string_view name) {
        return checked_row(saved.diplomacy,owned.diplomacy,name,
                           saves::DiplomacyEncoding::i16,22,target);
    };
    try {
        // Expansion2 CvDiplomacyAI.cpp:2890, 19793. This boolean belongs to
        // the recipient's DiplomacyAI, not the target's.
        result.friend_betrayal=own_bool("m_pabFriendDenouncedUs");
        if(result.friend_betrayal)
            add_rules(result.weights,defines,{
                {"APPROACH_WAR_DENOUNCED",0},
                {"APPROACH_HOSTILE_DENOUNCED",1},
                {"APPROACH_GUARDED_DENOUNCED",3},
                {"APPROACH_FRIENDLY_DENOUNCED",5},
                {"APPROACH_DECEPTIVE_DENOUNCED",2}});
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactApproachAfterLoadGate::friend_betrayal_unavailable;
        result.reason=error.what();return result;
    }
    result.pre_reciprocal_weights=result.weights;
    try {
        // SDK :2899 calls the target's IsDenouncedPlayer(eMyPlayer).
        const auto &saved_target=source.snapshot.all_players->players.at(target);
        const auto &owned_target=state.world.all_players->players.at(target);
        if(saved_target.saved_player_id!=result.target ||
           owned_target.saved_player_id!=result.target ||
           saved_target.diplomacy.layout!=saves::GameStateLayout::expansion2_version1 ||
           owned_target.diplomacy.layout!=saves::GameStateLayout::expansion2_version1)
            throw std::runtime_error("BNW reciprocal denouncement player identity differs");
        result.reciprocal_denouncement=checked_row(
            saved_target.diplomacy,owned_target.diplomacy,
            "m_pabDenouncedPlayer",saves::DiplomacyEncoding::boolean,22,
            static_cast<std::size_t>(result.recipient))==1;
        if(result.reciprocal_denouncement)
            add_rules(result.weights,defines,{
                {"APPROACH_WAR_DENOUNCED",0},
                {"APPROACH_HOSTILE_DENOUNCED",1},
                {"APPROACH_GUARDED_DENOUNCED",3},
                {"APPROACH_FRIENDLY_DENOUNCED",5},
                {"APPROACH_DECEPTIVE_DENOUNCED",2}});
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactApproachAfterLoadGate::reciprocal_denouncement_unavailable;
        result.reason=error.what();return result;
    }
    result.pre_protected_minor_weights=result.weights;
    try {
        // SDK :2919-2944. The commented WORKING AGAINST branch has no
        // runtime effect; protected-minor anger is the next real read.
        const auto killed=own_i8("m_paiOtherPlayerNumProtectedMinorsKilled");
        if(killed<0) throw std::runtime_error("BNW negative protected-minor killed count");
        bool angry_killed=false;
        if(killed>0) {
            const auto turns=own_i16("m_paiOtherPlayerTurnsSinceKilledProtectedMinor");
            const auto limit=rule(defines,
                "OPINION_WEIGHT_KILLED_PROTECTED_MINOR_NUM_TURNS_UNTIL_FORGIVEN");
            if(!limit) throw std::runtime_error("BNW killed-minor forgiveness rule unavailable");
            angry_killed=turns>-1 && turns<*limit;
        }
        bool angry_attacked=false;
        if(!angry_killed) {
            const auto attacked=own_i8("m_paiOtherPlayerNumProtectedMinorsAttacked");
            if(attacked<0) throw std::runtime_error("BNW negative protected-minor attacked count");
            if(attacked>0) {
                const auto turns=own_i16("m_paiOtherPlayerTurnsSinceAttackedProtectedMinor");
                const auto limit=rule(defines,
                    "OPINION_WEIGHT_ATTACKED_PROTECTED_MINOR_NUM_TURNS_UNTIL_FORGIVEN");
                if(!limit) throw std::runtime_error("BNW attacked-minor forgiveness rule unavailable");
                angry_attacked=turns>-1 && turns<*limit;
            }
        }
        if(angry_killed || angry_attacked) {
            const auto turns=own_i16("m_paiOtherPlayerTurnsSinceAttackedProtectedMinor");
            bool recent=angry_killed;
            if(!recent) {
                const auto limit=rule(defines,
                    "OPINION_WEIGHT_ATTACKED_PROTECTED_MINOR_RECENTLY_NUM_TURNS");
                if(!limit) throw std::runtime_error("BNW protected-minor recent rule unavailable");
                recent=turns<*limit;
            }
            if(recent)
                add_rules(result.weights,defines,{
                    {"APPROACH_ATTACKED_PROTECTED_MINOR_WAR",0},
                    {"APPROACH_ATTACKED_PROTECTED_MINOR_HOSTILE",1},
                    {"APPROACH_ATTACKED_PROTECTED_MINOR_GUARDED",3}});
            else
                add_rules(result.weights,defines,{
                    {"APPROACH_ATTACKED_PROTECTED_MINOR_PAST_WAR",0},
                    {"APPROACH_ATTACKED_PROTECTED_MINOR_PAST_HOSTILE",1},
                    {"APPROACH_ATTACKED_PROTECTED_MINOR_PAST_GUARDED",3}});
        }
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactApproachAfterLoadGate::protected_minor_unavailable;
        result.reason=error.what();return result;
    }
    result.pre_demand_weights=result.weights;
    try {
        // SDK :2951-2955 IsDemandEverMade returns counter != -1.
        const auto counter=own_i16("m_paiDemandCounter");
        if(counter< -1) throw std::runtime_error("BNW demand counter out of range");
        if(counter!=-1)
            add_rules(result.weights,defines,{
                {"APPROACH_DECEPTIVE_DEMAND",2},
                {"APPROACH_FRIENDLY_DEMAND",5}});
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactApproachAfterLoadGate::demand_unavailable;
        result.reason=error.what();return result;
    }
    try {
        // Expansion2 CvDiplomacyAI.cpp:2959-3076. Each else-if is retained:
        // a personal breach suppresses the world/team and ignored branches.
        const auto team_flag=[&](std::string_view name) {
            return checked_team_flag(source,state,result.target_team,name)==1;
        };
        if(own_bool("m_pabPlayerBrokenMilitaryPromise"))
            add_rules(result.weights,defines,{
                {"APPROACH_WAR_BROKEN_MILITARY_PROMISE",0},
                {"APPROACH_DECEPTIVE_BROKEN_MILITARY_PROMISE",2},
                {"APPROACH_FRIENDLY_BROKEN_MILITARY_PROMISE",5}});
        else if(team_flag("broken_military_promise"))
            add_rules(result.weights,defines,{
                {"APPROACH_WAR_BROKEN_MILITARY_PROMISE_WORLD",0},
                {"APPROACH_DECEPTIVE_BROKEN_MILITARY_PROMISE_WORLD",2},
                {"APPROACH_FRIENDLY_BROKEN_MILITARY_PROMISE_WORLD",5}});
        else if(own_bool("m_pabPlayerIgnoredMilitaryPromise"))
            add_rules(result.weights,defines,{
                {"APPROACH_DECEPTIVE_IGNORED_MILITARY_PROMISE",2},
                {"APPROACH_FRIENDLY_IGNORED_MILITARY_PROMISE",5}});
        if(own_bool("m_pabPlayerBrokenExpansionPromise"))
            add_rules(result.weights,defines,{
                {"APPROACH_WAR_BROKEN_EXPANSION_PROMISE",0},
                {"APPROACH_DECEPTIVE_BROKEN_EXPANSION_PROMISE",2},
                {"APPROACH_FRIENDLY_BROKEN_EXPANSION_PROMISE",5}});
        else if(own_bool("m_pabPlayerIgnoredExpansionPromise"))
            add_rules(result.weights,defines,{
                {"APPROACH_WAR_IGNORED_EXPANSION_PROMISE",0},
                {"APPROACH_DECEPTIVE_IGNORED_EXPANSION_PROMISE",2},
                {"APPROACH_FRIENDLY_IGNORED_EXPANSION_PROMISE",5}});
        if(own_bool("m_pabPlayerBrokenBorderPromise"))
            add_rules(result.weights,defines,{
                {"APPROACH_WAR_BROKEN_BORDER_PROMISE",0},
                {"APPROACH_DECEPTIVE_BROKEN_BORDER_PROMISE",2},
                {"APPROACH_FRIENDLY_BROKEN_BORDER_PROMISE",5}});
        else if(own_bool("m_pabPlayerIgnoredBorderPromise"))
            add_rules(result.weights,defines,{
                {"APPROACH_WAR_IGNORED_BORDER_PROMISE",0},
                {"APPROACH_DECEPTIVE_IGNORED_BORDER_PROMISE",2},
                {"APPROACH_FRIENDLY_IGNORED_BORDER_PROMISE",5}});
        if(own_bool("m_pabPlayerBrokenAttackCityStatePromise"))
            add_rules(result.weights,defines,{
                {"APPROACH_WAR_BROKEN_CITY_STATE_PROMISE",0},
                {"APPROACH_DECEPTIVE_BROKEN_CITY_STATE_PROMISE",2},
                {"APPROACH_FRIENDLY_BROKEN_CITY_STATE_PROMISE",5}});
        else if(team_flag("broken_city_state_promise"))
            add_rules(result.weights,defines,{
                {"APPROACH_WAR_BROKEN_CITY_STATE_PROMISE_WORLD",0},
                {"APPROACH_DECEPTIVE_BROKEN_CITY_STATE_PROMISE_WORLD",2},
                {"APPROACH_FRIENDLY_BROKEN_CITY_STATE_PROMISE_WORLD",5}});
        else if(own_bool("m_pabPlayerIgnoredAttackCityStatePromise"))
            add_rules(result.weights,defines,{
                {"APPROACH_WAR_IGNORED_CITY_STATE_PROMISE",0},
                {"APPROACH_DECEPTIVE_IGNORED_CITY_STATE_PROMISE",2},
                {"APPROACH_FRIENDLY_IGNORED_CITY_STATE_PROMISE",5}});
        if(own_bool("m_pabPlayerBrokenBullyCityStatePromise"))
            add_rules(result.weights,defines,{
                {"APPROACH_WAR_BROKEN_CITY_STATE_PROMISE",0},
                {"APPROACH_DECEPTIVE_BROKEN_CITY_STATE_PROMISE",2},
                {"APPROACH_FRIENDLY_BROKEN_CITY_STATE_PROMISE",5}},2);
        else if(own_bool("m_pabPlayerIgnoredBullyCityStatePromise"))
            add_rules(result.weights,defines,{
                {"APPROACH_WAR_IGNORED_CITY_STATE_PROMISE",0},
                {"APPROACH_DECEPTIVE_IGNORED_CITY_STATE_PROMISE",2},
                {"APPROACH_FRIENDLY_IGNORED_CITY_STATE_PROMISE",5}},2);
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactApproachAfterLoadGate::promise_unavailable;
        result.reason=error.what();return result;
    }
    try {
        // Expansion2 CvDiplomacyAI.cpp:3079-3114, GetMilitaryThreat:6761.
        const auto threat=checked_row(saved.diplomacy,owned.diplomacy,
            "m_paeMilitaryThreat",saves::DiplomacyEncoding::i8,63,target);
        if(threat< -1 || threat>4)
            throw std::runtime_error("BNW military threat enum out of range");
        switch(threat) {
        case 4: add_rules(result.weights,defines,{
            {"APPROACH_DECEPTIVE_MILITARY_THREAT_CRITICAL",2},
            {"APPROACH_GUARDED_MILITARY_THREAT_CRITICAL",3},
            {"APPROACH_AFRAID_MILITARY_THREAT_CRITICAL",4},
            {"APPROACH_FRIENDLY_MILITARY_THREAT_CRITICAL",5}});break;
        case 3: add_rules(result.weights,defines,{
            {"APPROACH_DECEPTIVE_MILITARY_THREAT_SEVERE",2},
            {"APPROACH_GUARDED_MILITARY_THREAT_SEVERE",3},
            {"APPROACH_AFRAID_MILITARY_THREAT_SEVERE",4},
            {"APPROACH_FRIENDLY_MILITARY_THREAT_SEVERE",5}});break;
        case 2: add_rules(result.weights,defines,{
            {"APPROACH_DECEPTIVE_MILITARY_THREAT_MAJOR",2},
            {"APPROACH_GUARDED_MILITARY_THREAT_MAJOR",3},
            {"APPROACH_AFRAID_MILITARY_THREAT_MAJOR",4},
            {"APPROACH_FRIENDLY_MILITARY_THREAT_MAJOR",5}});break;
        case 1: add_rules(result.weights,defines,{
            {"APPROACH_DECEPTIVE_MILITARY_THREAT_MINOR",2},
            {"APPROACH_GUARDED_MILITARY_THREAT_MINOR",3},
            {"APPROACH_AFRAID_MILITARY_THREAT_MINOR",4},
            {"APPROACH_FRIENDLY_MILITARY_THREAT_MINOR",5}});break;
        case 0: add_rules(result.weights,defines,{
            {"APPROACH_HOSTILE_MILITARY_THREAT_NONE",1}});break;
        default: break; // NO_THREAT_VALUE has no switch arm in SDK.
        }
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactApproachAfterLoadGate::military_threat_unavailable;
        result.reason=error.what();return result;
    }
    try {
        // Expansion2 CvPlayer.cpp:21891 stores the counters. Player::Read
        // calls FlavorManager::Read at 22149 after reset/Init, overwriting
        // randomized personality with both saved arrays. The getter in
        // CvGrandStrategyAI.cpp:763 adds the active strategy's flavor mod.
        const auto &saved_target=source.snapshot.all_players->players.at(target);
        const auto &owned_target=state.world.all_players->players.at(target);
        result.own_nuke_units=checked_nuke_count(saved,owned);
        result.target_nuke_units=checked_nuke_count(saved_target,owned_target);
        result.target_nuke_personality=checked_personality_flavor(
            saved_target,owned_target,database,types);
        result.target_nuke_grand_mod=checked_grand_nuke_mod(
            saved_target,owned_target,database,types);
        const auto active=active_strategy(saved_target.strategy);
        if(!active) throw std::runtime_error("BNW target GrandStrategy ID unavailable");
        std::int64_t flavor=result.target_nuke_personality;
        if(*active!=-1) {
            flavor+=result.target_nuke_grand_mod;
            if(flavor<0) flavor=0;
        }
        if(flavor<INT32_MIN || flavor>INT32_MAX)
            throw std::runtime_error("BNW grand-strategy nuke flavor overflows native int");
        const auto own_nuked=checked_row(saved.diplomacy,owned.diplomacy,
            "m_paiNumTimesNuked",saves::DiplomacyEncoding::i16,22,target);
        const auto target_nuked=checked_row(saved_target.diplomacy,owned_target.diplomacy,
            "m_paiNumTimesNuked",saves::DiplomacyEncoding::i16,22,
            static_cast<std::size_t>(result.recipient));
        if(own_nuked<0 || target_nuked<0)
            throw std::runtime_error("BNW nuked-by counts are negative");
        if(!state.setup.players.contains(result.target))
            throw std::runtime_error("BNW nuke target setup is unavailable");
        const bool human=state.setup.is_human(state.setup.players.at(result.target));
        if(human || own_nuked>0 || target_nuked>0)
            result.target_nuke_likelihood=100;
        else {
            const auto base=flavor+1;
            if(base>46340 || base< -46340)
                throw std::runtime_error("BNW nuke likelihood would overflow native int");
            result.target_nuke_likelihood=base*base;
        }
        // Expansion2 CvDiplomacyAI.cpp:3121-3154. These are local weights,
        // never a published diplomatic decision or gameplay side effect.
        auto add_constant=[&](std::size_t index,std::int64_t amount) {
            const auto value=result.weights[index]+amount;
            if(value<INT32_MIN || value>INT32_MAX)
                throw std::runtime_error("BNW nuke weight overflows native int");
            result.weights[index]=value;
        };
        if(result.target_nuke_units==0 && result.own_nuke_units>0) {
            add_constant(0,25);add_constant(1,25);add_constant(2,25);
            add_constant(3,-25);add_constant(4,-50);
        } else if(result.target_nuke_units>0 && result.own_nuke_units==0 &&
                  result.target_nuke_likelihood>50) {
            add_constant(4,50);add_constant(3,50);add_constant(2,-25);
            add_constant(0,-50);
        } else if(result.target_nuke_units>0 && result.own_nuke_units>0 &&
                  result.target_nuke_likelihood>25) {
            add_constant(3,25);add_constant(2,-25);add_constant(0,-25);
            add_constant(6,25);
        }
        result.nuke_inputs_verified=true;
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactApproachAfterLoadGate::nuke_inputs_unavailable;
        result.reason=error.what();return result;
    }
    try {
        // Expansion2 CvDiplomacyAI.cpp:3161-3190. The loop scans all 63
        // civ players, excluding the target and everyone on its team before
        // IsPlayerValid; only valid at-war players change the local weights.
        const auto recipient_team=checked_player_team(source,state,result.recipient);
        for(std::int64_t other=0;other<63;++other) {
            const auto other_team=checked_player_team(source,state,other);
            if(other==result.target || other_team==result.target_team)
                continue;
            if(!checked_player_valid(source,state,recipient_team,other))
                continue;
            if(checked_team_array(source,state,recipient_team,"at_war",other_team)==0)
                continue;
            ++result.other_wars;
            const auto all_wars=checked_row(saved.diplomacy,owned.diplomacy,
                "m_eStateAllWars",saves::DiplomacyEncoding::i32,1,0);
            if(all_wars<0 || all_wars>2)
                throw std::runtime_error("BNW StateAllWars enum unavailable");
            const char *war_rule=all_wars==0
                ? "APPROACH_WAR_AT_WAR_WITH_PLAYER_WARS_NEUTRAL"
                : all_wars==1
                    ? "APPROACH_WAR_AT_WAR_WITH_PLAYER_WARS_WINNING"
                    : "APPROACH_WAR_AT_WAR_WITH_PLAYER_WARS_LOSING";
            add_rules(result.weights,defines,{{war_rule,0},
                {"APPROACH_HOSTILE_AT_WAR_WITH_PLAYER",1},
                {"APPROACH_DECEPTIVE_AT_WAR_WITH_PLAYER",2},
                {"APPROACH_GUARDED_AT_WAR_WITH_PLAYER",3},
                {"APPROACH_FRIENDLY_AT_WAR_WITH_PLAYER",5}});
        }
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactApproachAfterLoadGate::at_war_loop_unavailable;
        result.reason=error.what();return result;
    }
    try {
        // Expansion2 :3197-3243. bLookAtOtherPlayers=true at the caller.
        const auto recipient_team=checked_player_team(source,state,result.recipient);
        if(!source.snapshot.pregame || !state.world.pregame)
            throw std::runtime_error("BNW other-player class roster unavailable");
        const auto &a=source.snapshot.pregame->archive.flag_arrays;
        const auto &b=state.world.pregame->archive.flag_arrays;
        const auto minors=a.find("minor_nation_civs"),owned_minors=b.find("minor_nation_civs");
        if(minors==a.end() || owned_minors==b.end() ||
           minors->second.size()<63 || owned_minors->second!=minors->second)
            throw std::runtime_error("BNW other-player minor flags unavailable");
        for(std::int64_t other=0;other<63;++other) {
            if(other==result.target ||
               !checked_player_valid(source,state,recipient_team,other))
                continue;
            ++result.other_valid_players;
            const bool minor=minors->second[static_cast<std::size_t>(other)];
            const auto goal=checked_row(saved.diplomacy,owned.diplomacy,
                "m_paeWarGoal",saves::DiplomacyEncoding::i8,63,
                static_cast<std::size_t>(other));
            if(goal< -1 || goal>4)
                throw std::runtime_error("BNW other-player war goal enum unavailable");
            std::int64_t approach=-1;
            if(!minor) {
                if(other>=22)
                    throw std::runtime_error("BNW other major exceeds MAX_MAJOR_CIVS");
                approach=checked_row(saved.diplomacy,owned.diplomacy,
                    "m_paeMajorCivApproach",saves::DiplomacyEncoding::i8,22,
                    static_cast<std::size_t>(other));
                if(approach< -1 || approach>6)
                    throw std::runtime_error("BNW other-player approach enum unavailable");
            }
            if(goal==1 || (!minor && approach==0))
                add_rules(result.weights,defines,{
                    {"APPROACH_WAR_PLANNING_WAR_WITH_ANOTHER_PLAYER",0},
                    {"APPROACH_HOSTILE_PLANNING_WAR_WITH_ANOTHER_PLAYER",1},
                    {"APPROACH_DECEPTIVE_PLANNING_WAR_WITH_ANOTHER_PLAYER",2},
                    {"APPROACH_GUARDED_PLANNING_WAR_WITH_ANOTHER_PLAYER",3},
                    {"APPROACH_FRIENDLY_PLANNING_WAR_WITH_ANOTHER_PLAYER",5}});
            if(minor) continue;
            if(approach==1)
                add_rules(result.weights,defines,{
                    {"APPROACH_WAR_HOSTILE_WITH_ANOTHER_PLAYER",0},
                    {"APPROACH_HOSTILE_HOSTILE_WITH_ANOTHER_PLAYER",1},
                    {"APPROACH_DECEPTIVE_HOSTILE_WITH_ANOTHER_PLAYER",2},
                    {"APPROACH_FRIENDLY_HOSTILE_WITH_ANOTHER_PLAYER",5}});
            else if(approach==4)
                add_rules(result.weights,defines,{
                    {"APPROACH_WAR_AFRAID_WITH_ANOTHER_PLAYER",0},
                    {"APPROACH_HOSTILE_AFRAID_WITH_ANOTHER_PLAYER",1},
                    {"APPROACH_DECEPTIVE_AFRAID_WITH_ANOTHER_PLAYER",2},
                    {"APPROACH_FRIENDLY_AFRAID_WITH_ANOTHER_PLAYER",5}});
        }
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactApproachAfterLoadGate::other_players_unavailable;
        result.reason=error.what();return result;
    }
    try {
        // Expansion2 CvTradeClasses.cpp:2472 sums stored origin/destination
        // yields for this exact owner pair. Positive trade value later calls
        // calculateGoldRate, which is a separate live-world boundary.
        const auto *gold=types.find("YIELD_GOLD");
        if(!gold || gold->table!="Yields" || gold->id<0 ||
           gold->id!=state.cities.gold ||
           state.world.trade.routes.size()!=source.snapshot.trade.routes.size() ||
           state.cities.trade_routes.size()!=source.snapshot.trade.routes.size())
            throw std::runtime_error("BNW trade-route/gold yield context unavailable");
        auto same=[](const saves::TradeRouteObservation &a,
                     const saves::TradeRouteObservation &b) {
            return a.id==b.id && a.origin_x==b.origin_x &&
                a.origin_y==b.origin_y && a.destination_x==b.destination_x &&
                a.destination_y==b.destination_y &&
                a.origin_owner==b.origin_owner &&
                a.destination_owner==b.destination_owner &&
                a.origin_yields==b.origin_yields &&
                a.destination_yields==b.destination_yields &&
                a.connection_type==b.connection_type && a.unit_id==b.unit_id;
        };
        std::int64_t total=0;
        for(std::size_t i=0;i<source.snapshot.trade.routes.size();++i) {
            const auto &a=source.snapshot.trade.routes[i];
            if(!same(a,state.world.trade.routes[i]) ||
               !same(a,state.cities.trade_routes[i]))
                throw std::runtime_error("BNW source/owned trade route differs");
            const bool empty=a.origin_x==-1 && a.origin_y==-1 &&
                a.destination_x==-1 && a.destination_y==-1;
            if(empty) continue;
            if(a.origin_yields.size()<=static_cast<std::size_t>(gold->id) ||
               a.destination_yields.size()<=static_cast<std::size_t>(gold->id))
                throw std::runtime_error("BNW trade route gold yield slot absent");
            if(a.origin_owner==result.recipient && a.destination_owner==result.target)
                total+=a.origin_yields[static_cast<std::size_t>(gold->id)];
            if(a.destination_owner==result.recipient && a.origin_owner==result.target)
                total+=a.destination_yields[static_cast<std::size_t>(gold->id)];
            if(total<INT32_MIN || total>INT32_MAX)
                throw std::runtime_error("BNW trade value overflows native int");
        }
        result.trade_value=total/100;
        if(result.trade_value>0)
            throw std::runtime_error("BNW positive trade value requires live calculateGoldRate before approach weights");
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactApproachAfterLoadGate::trade_value_unavailable;
        result.reason=error.what();return result;
    }
    try {
        // Expansion2 CvDiplomacyAI.cpp:3271-3323. All percentages multiply
        // a native int before division, so overflow blocks this diagnostic.
        const auto projection=checked_row(saved.diplomacy,owned.diplomacy,
            "m_paeWarProjection",saves::DiplomacyEncoding::i8,63,target);
        if(projection< -1 || projection>5)
            throw std::runtime_error("BNW war projection enum unavailable");
        auto percent=[&](std::size_t index,const char *name) {
            const auto value=rule(defines,name);
            if(!value) throw std::runtime_error("BNW war projection rule unavailable");
            const auto product=result.weights[index]*(*value);
            if(product<INT32_MIN || product>INT32_MAX)
                throw std::runtime_error("BNW war projection native multiplication overflow");
            result.weights[index]=product/100;
        };
        switch(projection) {
        case 0:
            percent(0,"APPROACH_WAR_PROJECTION_DESTRUCTION_PERCENT");
            percent(2,"APPROACH_WAR_PROJECTION_DESTRUCTION_PERCENT");
            percent(3,"APPROACH_GUARDED_PROJECTION_DESTRUCTION_PERCENT");break;
        case 1:
            percent(0,"APPROACH_WAR_PROJECTION_DEFEAT_PERCENT");
            percent(2,"APPROACH_WAR_PROJECTION_DEFEAT_PERCENT");
            percent(3,"APPROACH_GUARDED_PROJECTION_DEFEAT_PERCENT");break;
        case 2:
            percent(0,"APPROACH_WAR_PROJECTION_STALEMATE_PERCENT");
            percent(3,"APPROACH_GUARDED_PROJECTION_STALEMATE_PERCENT");break;
        case 3:
            percent(0,"APPROACH_WAR_PROJECTION_UNKNOWN_PERCENT");
            percent(3,"APPROACH_GUARDED_PROJECTION_UNKNOWN_PERCENT");break;
        case 4:
            percent(0,"APPROACH_WAR_PROJECTION_GOOD_PERCENT");
            percent(2,"APPROACH_WAR_PROJECTION_GOOD_PERCENT");
            percent(3,"APPROACH_GUARDED_PROJECTION_GOOD_PERCENT");break;
        case 5:
            percent(0,"APPROACH_WAR_PROJECTION_VERY_GOOD_PERCENT");
            percent(2,"APPROACH_WAR_PROJECTION_VERY_GOOD_PERCENT");
            percent(3,"APPROACH_GUARDED_PROJECTION_VERY_GOOD_PERCENT");break;
        default: break;
        }
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactApproachAfterLoadGate::war_projection_unavailable;
        result.reason=error.what();return result;
    }
    try {
        // Expansion2 CvDiplomacyAI.cpp:7544 checks proximity before city
        // count. The earlier first-contact chain proves the only permitted
        // source/owned proximity delta for this selected target.
        const auto proximity=checked_proximity(saved,owned,result.target);
        const auto &raw_cities=source.snapshot.all_players->players.at(target).cities;
        const auto &live_cities=state.cities.players.at(result.target).cities;
        if(raw_cities.header.wire_city_count<0 ||
           raw_cities.cities.size()!=
               static_cast<std::size_t>(raw_cities.header.wire_city_count) ||
           state.world.all_players->players.at(target).cities.cities.size()!=
               raw_cities.cities.size() ||
           live_cities.size()!=raw_cities.cities.size())
            throw std::runtime_error("BNW reckless-expander city roster unavailable");
        if(proximity>=2 && raw_cities.cities.size()>=4)
            throw std::runtime_error("BNW reckless-expander average cities and military strength unavailable");
        // Both early-return branches are exact no-ops for local weights.
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactApproachAfterLoadGate::reckless_expander_unavailable;
        result.reason=error.what();return result;
    }
    try {
        // Expansion2 CvDiplomacyAI.cpp:3336-3355 scans MAX_MAJOR_CIVS.
        const auto &saved_target=source.snapshot.all_players->players.at(target);
        const auto &owned_target=state.world.all_players->players.at(target);
        bool dogpile=false;
        for(std::int64_t other=0;other<22;++other) {
            if(other==result.target || other==result.recipient)
                continue;
            if(!checked_player_valid(source,state,result.target_team,other))
                continue;
            const auto war=checked_row(saved_target.diplomacy,
                owned_target.diplomacy,"m_paeWarState",
                saves::DiplomacyEncoding::i8,63,static_cast<std::size_t>(other));
            if(war< -1 || war>5)
                throw std::runtime_error("BNW dogpile war-state enum unavailable");
            if(war!=-1) dogpile=true;
        }
        if(dogpile) {
            for(const auto [index,percent]:{
                    std::pair<std::size_t,std::int64_t>{0,200},
                    std::pair<std::size_t,std::int64_t>{2,150}}) {
                const auto product=result.weights[index]*percent;
                if(product<INT32_MIN || product>INT32_MAX)
                    throw std::runtime_error("BNW dogpile native multiplication overflow");
                result.weights[index]=product/100;
            }
        }
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactApproachAfterLoadGate::dogpile_unavailable;
        result.reason=error.what();return result;
    }
    try {
        // Expansion2 CvDiplomacyAI.cpp:3361-3388 uses one rule for both
        // WAR and DECEPTIVE, multiplying each native int before division.
        const auto proximity=checked_proximity(saved,owned,result.target);
        const char *name=proximity==3 ? "APPROACH_WAR_PROXIMITY_NEIGHBORS"
            : proximity==2 ? "APPROACH_WAR_PROXIMITY_CLOSE"
            : proximity==1 ? "APPROACH_WAR_PROXIMITY_FAR"
            : proximity==0 ? "APPROACH_WAR_PROXIMITY_DISTANT" : nullptr;
        if(name) {
            const auto percent=rule(defines,name);
            if(!percent) throw std::runtime_error("BNW proximity weight rule unavailable");
            for(const std::size_t index:{0u,2u}) {
                const auto product=result.weights[index]*(*percent);
                if(product<INT32_MIN || product>INT32_MAX)
                    throw std::runtime_error("BNW proximity native multiplication overflow");
                result.weights[index]=product/100;
            }
        }
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactApproachAfterLoadGate::proximity_weight_unavailable;
        result.reason=error.what();return result;
    }
    try {
        // Expansion2 CvDiplomacyAI.cpp:3394-3404.
        const auto recipient_team=checked_player_team(source,state,result.recipient);
        const auto peace=checked_team_array(source,state,recipient_team,
            "turn_made_peace",result.target_team);
        if(peace>state.maintenance.elapsed_turns)
            throw std::runtime_error("BNW peace treaty turn is in the future");
        if(peace> -1) {
            const auto dampener=rule(defines,"TURNS_SINCE_PEACE_WEIGHT_DAMPENER");
            if(!dampener)
                throw std::runtime_error("BNW peace dampener rule unavailable");
            if(state.maintenance.elapsed_turns-peace<*dampener) {
                const auto percent=rule(defines,
                    "APPROACH_WAR_HAS_MADE_PEACE_BEFORE_PERCENT");
                if(!percent)
                    throw std::runtime_error("BNW peace approach rule unavailable");
                const auto product=result.weights[0]*(*percent);
                if(product<INT32_MIN || product>INT32_MAX)
                    throw std::runtime_error("BNW peace native multiplication overflow");
                result.weights[0]=product/100;
            }
        }
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactApproachAfterLoadGate::peace_treaty_unavailable;
        result.reason=error.what();return result;
    }
    try {
        // Expansion2 :3408-3413, CvGame::countMajorCivsAlive scans all 63.
        if(!source.snapshot.pregame || !state.world.pregame)
            throw std::runtime_error("BNW duel class roster unavailable");
        const auto &a=source.snapshot.pregame->archive.flag_arrays;
        const auto &b=state.world.pregame->archive.flag_arrays;
        const auto minors=a.find("minor_nation_civs"),owned_minors=b.find("minor_nation_civs");
        if(minors==a.end() || owned_minors==b.end() ||
           minors->second.size()<63 || owned_minors->second!=minors->second)
            throw std::runtime_error("BNW duel minor flags unavailable");
        std::int64_t alive_majors=0;
        for(std::int64_t player=0;player<63;++player) {
            const auto &raw=source.snapshot.all_players->players.at(
                static_cast<std::size_t>(player));
            const auto &live=state.world.all_players->players.at(
                static_cast<std::size_t>(player));
            const saves::PlayerCoreField *x=nullptr,*y=nullptr;
            for(const auto &f:raw.core.fields)
                if(f.name && std::string_view(f.name)=="m_bAlive") {
                    if(x) throw std::runtime_error("BNW duplicate duel source alive field");
                    x=&f;
                }
            for(const auto &f:live.core.fields)
                if(f.name && std::string_view(f.name)=="m_bAlive") {
                    if(y) throw std::runtime_error("BNW duplicate duel owned alive field");
                    y=&f;
                }
            if(!x || !y || x->values.size()!=1 || y->values!=x->values ||
               x->values[0]<0 || x->values[0]>1 ||
               !state.setup.players.contains(player) ||
               state.setup.players.at(player).alive!=(x->values[0]==1))
                throw std::runtime_error("BNW duel alive roster inconsistent");
            if(x->values[0]==1 && !minors->second[static_cast<std::size_t>(player)])
                ++alive_majors;
        }
        if(alive_majors==2) {
            result.weights[2]=0;
            result.weights[5]=0;
        }
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactApproachAfterLoadGate::duel_unavailable;
        result.reason=error.what();return result;
    }
    try {
        // Expansion2 CvDiplomacyAI.cpp:18570-18629. Global accepted state
        // is the maximum over valid major players; only SOON/ACCEPTED make
        // the code read the minimum nonnegative counter (initially 9999).
        // The saved matrices are row-major [other major][target major].
        if(result.target<0 || result.target>=22)
            throw std::runtime_error("BNW coop-war target is not a major player");
        const auto recipient_team=checked_player_team(source,state,result.recipient);
        std::int64_t best_state=-1;
        for(std::int64_t other=0;other<22;++other) {
            if(!checked_player_valid(source,state,recipient_team,other))
                continue;
            const auto value=checked_coop_matrix(saved,owned,
                "m_ppaacCoopWarAcceptedState",saves::DiplomacyEncoding::i8,
                static_cast<std::size_t>(other),
                static_cast<std::size_t>(result.target));
            if(value>best_state) best_state=value;
        }
        if(best_state==1 || best_state==2) {
            std::int64_t best_counter=9999; // MAX_TURNS_SAFE_ESTIMATE.
            for(std::int64_t other=0;other<22;++other) {
                if(!checked_player_valid(source,state,recipient_team,other))
                    continue;
                const auto value=checked_coop_matrix(saved,owned,
                    "m_ppaaiCoopWarCounter",saves::DiplomacyEncoding::i16,
                    static_cast<std::size_t>(other),
                    static_cast<std::size_t>(result.target));
                if(value>=0 && value<best_counter) best_counter=value;
            }
            const auto turns=rule(defines,"COOP_WAR_LOCKED_TURNS");
            if(!turns) throw std::runtime_error("BNW coop-war locked turns rule unavailable");
            if(best_counter<=*turns)
                add_rules(result.weights,defines,{
                    {"COOP_WAR_LOCKED_TURNS_WAR_WEIGHT",0}});
        }
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactApproachAfterLoadGate::coop_war_unavailable;
        result.reason=error.what();return result;
    }
    result.gate=BnwFirstContactApproachAfterLoadGate::random_factor_unimplemented;
    result.reason="BNW deterministic approach weights checked through coop war; JonRand, canDeclareWar, logging and SetMajorCivApproach remain unavailable";
    return result;
}

} // namespace civ5::runtime
