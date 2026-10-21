#pragma once
#include "civ5/runtime/bnw_happiness_receipt_types.hpp"
#include "civ5/runtime/bnw_happiness_buildings.hpp"
#include "civ5/runtime/bnw_happiness_resources.hpp"
#include "civ5/runtime/bnw_religion_happiness_terms.hpp"
namespace civ5::runtime {
class BnwHappinessPrefixRules {
 public:
    static BnwHappinessPrefixRules load(const content::RulesDatabase &);
    const std::string &rules_sha256() const noexcept {return buildings_.rules_sha256();}
    const BnwBuildingHappinessRules &buildings() const noexcept {return buildings_;}
    const BnwResourceHappinessRules &resources() const noexcept {return resources_;}
    std::int32_t default_happiness(std::int64_t handicap) const;
 private:
    BnwBuildingHappinessRules buildings_;
    BnwResourceHappinessRules resources_;
    std::map<std::int64_t,std::int32_t> handicaps_;
};
class BnwHappinessPrefixInput {
 public:
    const std::string &epoch() const noexcept {return epoch_;}
 private:
    friend struct BnwHappinessPrefixAccess;
    BnwHappinessPrefixInput(SessionCheckpointIdentity identity,std::int64_t player,std::string epoch)
        :identity_(std::move(identity)),player_(player),epoch_(std::move(epoch)) {}
    SessionCheckpointIdentity identity_;std::int64_t player_=-1;std::string epoch_;
};
class BnwHappinessPrefixDraft {
 public:
    const LiveSessionState &state() const noexcept {return state_;}
    const auto &stores() const noexcept {return stores_;}
    std::int64_t player() const noexcept {return player_;}
    const SessionCheckpointIdentity &identity() const noexcept {return identity_;}
    const std::string &input_epoch() const noexcept {return input_epoch_;}
    std::int32_t initial_happiness() const noexcept {return initial_happiness_;}
 private:
    friend struct BnwHappinessPrefixAccess;
    BnwHappinessPrefixDraft(const LiveSessionState &state,SessionCheckpointIdentity identity,std::int64_t player)
        :state_(state),identity_(std::move(identity)),player_(player) {}
    LiveSessionState state_;SessionCheckpointIdentity identity_;std::int64_t player_=-1;
    std::string epoch_,input_epoch_;std::int32_t initial_happiness_=0;std::vector<BnwHappinessNativeStore> stores_;
};
struct BnwHappinessPrefixExecution {
    std::unique_ptr<BnwHappinessPrefixDraft> prefix;
    std::string frontier;
};
// Quote the actual current source/native input epoch. Execution refuses stale
// cache/roster/rules inputs, and writes the six source-ordered m_iHappiness
// stores only into a private copy. This is not full happiness or publication.
BnwHappinessPrefixInput attest_bnw_happiness_prefix_input(const LiveSessionState &,
    const BnwHappinessPrefixRules &,const SessionCheckpointIdentity &,std::int64_t player);
BnwHappinessPrefixExecution execute_bnw_happiness_native_prefix(const LiveSessionState &,
    const BnwHappinessPrefixInput &,const BnwHappinessPrefixRules &);
namespace detail {
// Supplied native arithmetic only; no stage receipt or publication permission.
// Called by the old detached prefix and the private genuine reached-stage owner.
BnwHappinessNativeStore bnw_happiness_prefix_stage_math(const LiveSessionState&,
 const BnwHappinessPrefixRules&,const SessionCheckpointIdentity&,std::int64_t,
 BnwHappinessNativeStore::Stage);
BnwHappinessNativeStore bnw_happiness_prefix_reached_stage_math(const LiveSessionState&,
 const BnwHappinessPrefixRules&,const SessionCheckpointIdentity&,std::int64_t,
 BnwHappinessNativeStore::Stage);
}
struct BnwHappinessReligionContinuation {
    BnwReligionHappinessTermsQuote religion;
    std::optional<BnwHappinessNativeStore> store;
    std::string frontier;
};
// Fork AT THE INTERMEDIATE CACHE, dispatch mandatory founder Lua and any second
// accumulator in the SAME epoch, then add religion to its current native cache.
// Retain prefix while the returned epoch lives (source_world references it).
// Any failure discards that VM/world; original and prefix remain unchanged.
// No publish: natural wonders/minors/leagues/connections/UI are still required.
BnwHappinessReligionContinuation continue_bnw_happiness_prefix_religion(
    const BnwHappinessPrefixDraft &,const BnwHappinessPrefixRules &,
    const SavedContentProfile &,BnwGameplayLuaEpochOwner &,const BnwReligionHappinessRules &);
}
