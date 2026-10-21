#pragma once
#include "civ5/runtime/bnw_happiness_receipt_types.hpp"
#include "civ5/runtime/bnw_happiness_update_prefix.hpp"
#include "civ5/runtime/bnw_happiness_minors.hpp"
#include "civ5/runtime/bnw_happiness_natural_wonders.hpp"
namespace civ5::runtime {
class BnwHappinessSuffixRules {
 public:
    static BnwHappinessSuffixRules load(const content::RulesDatabase &);
    const auto &identity() const noexcept {return minors_.rules_sha256();}
    const auto &minors() const noexcept {return minors_;}
    const auto &wonders() const noexcept {return wonders_;}
    const auto &unhappiness() const noexcept {return unhappiness_;}
    const auto &defines() const noexcept {return defines_;}
    const auto &types() const noexcept {return types_;}
 private:
    BnwMinorHappinessRules minors_;BnwNaturalWonderHappinessRules wonders_;
    BnwUnhappinessRules unhappiness_;content::GlobalDefineCache defines_;content::InfoTypeCatalog types_;
};
struct BnwCityConnectionHappinessQuote {
    std::int32_t rate=0,connected_count=0,total=0;
    std::optional<std::int64_t> capital;
    std::vector<std::int64_t> connected_cities;
};
// Cached native treasury route getter, not path generation. Capital resolves
// via actual BNW FLTA slot/generation rules; route reads use the saved/current
// ordered plot-ID vector and HAS_ANY_ROUTE bit, never cached connection gold.
BnwCityConnectionHappinessQuote quote_bnw_city_connection_happiness(
    const LiveSessionState &,std::int64_t player);
struct BnwHappinessGameDataReceipt {
    std::int64_t player=-1;std::int32_t happiness=0,connections=0,excess=0;
    bool achievement_xp2_45_requested=false;
};
class BnwHappinessGameDataBatch {
 public:
    const auto &receipt() const noexcept {return receipt_;}
 private:
    friend struct BnwHappinessSuffixAccess;
    friend struct BnwHappinessUiPumpAccess;
    friend void apply_bnw_happiness_game_data_batch(const LiveSessionState &,class BnwHappinessGameDataOwner &,const content::RulesDatabase &,const BnwHappinessGameDataBatch &);
    SessionCheckpointIdentity identity_;std::array<std::uint8_t,16> guid_{};
    std::string before_epoch_,after_epoch_;std::uint64_t expected_revision_=0;
    BnwHappinessGameDataReceipt receipt_;
};
// Explicit new Civis client transport/cache, not presumed original saved UI.
// Actual dirty bit and view cache change on receipt; native/UI/Lua fanout is a
// separate mandatory frontier, and this receiver never attests its completion.
class BnwHappinessGameDataOwner {
 public:
    bool dirty() const noexcept {return dirty_;}
    bool pending_fanout() const noexcept {return pending_fanout_;}
    std::uint64_t revision() const noexcept {return revision_;}
    const auto &receipts() const noexcept {return receipts_;}
    std::int32_t observed_happiness() const noexcept {return happiness_;}
 private:
    friend struct BnwHappinessSuffixAccess;
    friend struct BnwHappinessUiPumpAccess;
    friend BnwHappinessGameDataOwner start_bnw_happiness_game_data_client(const LiveSessionState &,const BnwHappinessPrefixRules &,const SessionCheckpointIdentity &,std::int64_t);
    friend void apply_bnw_happiness_game_data_batch(const LiveSessionState &,BnwHappinessGameDataOwner &,const content::RulesDatabase &,const BnwHappinessGameDataBatch &);
    BnwHappinessGameDataOwner(SessionCheckpointIdentity identity,std::array<std::uint8_t,16> guid,std::string input,std::int64_t player,std::int32_t happiness)
        :identity_(std::move(identity)),guid_(guid),input_epoch_(std::move(input)),player_(player),happiness_(happiness){}
    SessionCheckpointIdentity identity_;std::array<std::uint8_t,16> guid_{};std::string input_epoch_;
    std::int64_t player_=-1;std::int32_t happiness_=0;std::uint64_t revision_=0;bool dirty_=false,pending_fanout_=false;
    std::vector<BnwHappinessGameDataReceipt> receipts_;
};
BnwHappinessGameDataOwner start_bnw_happiness_game_data_client(const LiveSessionState &,
    const BnwHappinessPrefixRules &,const SessionCheckpointIdentity &,std::int64_t player);
void apply_bnw_happiness_game_data_batch(const LiveSessionState &,BnwHappinessGameDataOwner &,
    const content::RulesDatabase &,const BnwHappinessGameDataBatch &);
namespace detail {
// Numeric source facts ONLY. This is not a live callback/publication receipt,
// does not authorize stores, and cannot complete a retained city operation.
enum class BnwHappinessPostReligionRead {
    wonders,minors,leagues,connections_cache,connections_add,achievement_condition
};
struct BnwHappinessPostReligionOperand {
    std::int32_t operand=0;
    std::optional<BnwCityConnectionHappinessQuote> connections;
    std::optional<BnwUnhappinessQuote> unhappiness;
    bool achievement_requested=false;
};
// Reads ONLY the requested reached SDK getter on the supplied current world.
// Caller metadata/currentness and authentic SAME Lua epoch remain mandatory
// in a production owner. No stage/result/DTO here can attest that provenance.
BnwHappinessPostReligionOperand bnw_happiness_post_religion_reached_read(
    const LiveSessionState &,const BnwHappinessSuffixRules &,
    const SessionCheckpointIdentity &,std::int64_t player,BnwHappinessPostReligionRead);
}
struct BnwHappinessSuffixExecution {
    BnwHappinessReligionContinuation continuation;
    std::vector<BnwHappinessSuffixStore> stores;
    std::optional<BnwCityConnectionHappinessQuote> connections;
    std::optional<BnwUnhappinessQuote> unhappiness;
    std::optional<BnwHappinessGameDataBatch> batch;
    std::optional<BnwHappinessGameDataOwner> receiver;
    bool native_cache_suffix_complete=false,mandatory_game_data_fanout_complete=false;
    std::string frontier;
};
// Internally invokes mandatory religion continuation once, then uses THAT SAME
// detached Lua/world epoch for every suffix store. Missing/error/stale operands
// discard all suffix+callback effects. At actual dirty receiver frontier a
// private sealed world/VM/receiver remains pending; no publish/full completion.
BnwHappinessSuffixExecution execute_bnw_happiness_remaining_suffix(
    const BnwHappinessPrefixDraft &,const BnwHappinessPrefixRules &,
    const SavedContentProfile &,BnwGameplayLuaEpochOwner &,const BnwReligionHappinessRules &,
    const BnwHappinessSuffixRules &,const BnwHappinessGameDataOwner &,const content::RulesDatabase &);
}
