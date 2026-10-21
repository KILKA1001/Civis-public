#pragma once
#include "civ5/runtime/bnw_total_jons_culture.hpp"
namespace civ5::runtime {
class BnwGameActivationHappinessPending;
enum class BnwGrandCulturePriorityStatus { own_tourism,foreign_scan,foreign_total,foreign_tourism,weights,influence,complete,discarded };
struct BnwGrandCultureForeign {std::int64_t player=-1;std::optional<std::int32_t> culture,tourism;std::uint64_t founder_calls=0;};
struct BnwGrandCulturePriorityProgress {
 std::int32_t base=0,initial=0,own_culture=0;
 std::optional<std::int32_t> own_tourism,category,before_random;
 std::size_t slots_visited=0;
 std::int32_t alive=0,ahead_culture=0,behind_culture=0,ahead_tourism=0,behind_tourism=0;
 std::int32_t culture_weight=0,tourism_weight=0,influential_weight=0;
 std::vector<BnwGrandCultureForeign> foreigners;
 std::optional<BnwCultureInfluentialCount> influential;
};
struct BnwGrandCulturePriorityAdvance {bool applied=false,dispatched=false,nonrepeatable=false;std::string blocker;};
class BnwGrandCulturePriorityPending {
 public:
 BnwGrandCulturePriorityPending(BnwGrandCulturePriorityPending&&) noexcept=default;
 BnwGrandCulturePriorityPending(const BnwGrandCulturePriorityPending&)=delete;
 auto status()const noexcept{return status_;}
 const auto &progress()const noexcept{return progress_;}
 const BnwTeamTechLuaEpoch *epoch()const noexcept{return nested_?nested_->epoch():epoch_.get();}
 const BnwTotalJonsCulturePending *foreign_total()const noexcept{return nested_.get();}
 auto player()const noexcept{return player_;}
 const auto &identity()const noexcept{return identity_;}
 const auto &profile()const noexcept{return profile_;}
 const auto &after_revision()const noexcept{return checkpoint_;}
 std::unique_ptr<BnwTeamTechLuaEpoch> take_completed_epoch();
 private:
 friend std::unique_ptr<BnwGrandCulturePriorityPending> stage_bnw_grand_culture_priority(const BnwGameActivationHappinessPending&,std::unique_ptr<BnwTeamTechLuaEpoch>&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
 friend BnwGrandCulturePriorityAdvance continue_bnw_grand_culture_priority(BnwGrandCulturePriorityPending&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
 friend const BnwGrandCulturePriorityProgress &checked_bnw_grand_culture_priority(const BnwGrandCulturePriorityPending&,const BnwTeamTechLuaEpoch&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
 BnwGrandCulturePriorityPending()=default;
 std::unique_ptr<BnwTeamTechLuaEpoch> epoch_;
 std::unique_ptr<BnwTotalJonsCulturePending> nested_;
 SavedContentProfile profile_;SessionCheckpointIdentity identity_;std::int64_t player_=-1,next_slot_=0;
 BnwGrandCulturePriorityProgress progress_;std::string checkpoint_;
 const LiveSessionState *world_address_=nullptr;const LuaScriptHost *host_address_=nullptr;
 BnwGrandCulturePriorityStatus status_=BnwGrandCulturePriorityStatus::own_tourism;
};
// Genuine private enclosing source prefix + completed own culture only.
// Moves the SAME epoch after checks. No caller-authored quote or completion bool.
std::unique_ptr<BnwGrandCulturePriorityPending> stage_bnw_grand_culture_priority(
 const BnwGameActivationHappinessPending&,std::unique_ptr<BnwTeamTechLuaEpoch>&,
 const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
// Completed source locals survive later read-only refusals. A rejected nested
// started callback, or inability to retain its new checkpoint, discards the VM.
BnwGrandCulturePriorityAdvance continue_bnw_grand_culture_priority(
 BnwGrandCulturePriorityPending&,const BnwGrandWorldAverageRules&,
 const BnwHappinessSuffixRules&,const content::RulesDatabase&);
const BnwGrandCulturePriorityProgress &checked_bnw_grand_culture_priority(
 const BnwGrandCulturePriorityPending&,const BnwTeamTechLuaEpoch&,
 const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
}
