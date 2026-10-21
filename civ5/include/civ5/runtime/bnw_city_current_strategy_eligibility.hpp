#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
class BnwCityEntryPending;
class BnwCityStrategyObsoleteEndOwner;
class BnwCityCurrentStrategyEligibilityUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
enum class BnwCityStrategyTriggerObligation { skipped_for_minor,none,start,end };
struct BnwCityStrategyEligibilityMathInputs {
 // Supplied arithmetic/predicate facts ONLY, not a live strategy/city issuer.
 bool minor=false,no_minor=false,using_strategy=false,permanent=false;
 std::int32_t prerequisite=-1,obsolete=-1,check_turn_count=0,minimum_turns=0;
 std::int32_t game_turn=0,turn_adopted=-1;
 // Missing facts are allowed only when the SDK does not reach HasTech.
 std::map<std::int32_t,bool> has_tech{};
};
BnwCityStrategyTriggerObligation bnw_city_strategy_eligibility_math(
 const BnwCityStrategyEligibilityMathInputs&);
struct BnwCityStrategyEligibilityFact {
 std::int32_t native_id;
 std::string type;
 BnwCityStrategyTriggerObligation obligation;
 // null means IsUsing was NOT read (minor exclusion), never an inferred false.
 std::optional<bool> using_strategy;
};
class BnwCityCurrentStrategyEligibility final {
 public:
  // Recompute ordered current reads. Not an executable strategy plan: earlier
  // real triggers/Lua may alter later facts; retained executor must requery.
  std::vector<BnwCityStrategyEligibilityFact> records()const;
  // Current single native slot only. Numeric facts, never ordered execution.
  BnwCityStrategyEligibilityFact record_at(std::int64_t native_id)const;
  bool obsolete_for_dispatch(std::int64_t native_id)const;
  std::int32_t native_slots()const;
  static constexpr bool strategy_dispatch_executed()noexcept{return false;}
  static constexpr bool city_entry_phase_ready()noexcept{return false;}
 private:
  friend class BnwCityEntryPending;
  friend BnwCityStrategyObsoleteEndOwner bind_bnw_city_strategy_obsolete_end(
   std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
  // Genuine ordered owner avoids the legacy factory's batch current reads.
  static BnwCityCurrentStrategyEligibility prepare_ordered(
   std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
  struct Impl;std::shared_ptr<Impl> impl_;
  explicit BnwCityCurrentStrategyEligibility(std::shared_ptr<Impl>)noexcept;
  void require_current()const;
  friend BnwCityCurrentStrategyEligibility inspect_bnw_city_current_strategy_eligibility(
   std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
};
BnwCityCurrentStrategyEligibility inspect_bnw_city_current_strategy_eligibility(
 std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
}
