#pragma once
#include "civ5/content/rules_database.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <stdexcept>
#include <string_view>
namespace civ5::runtime {
class LiveSession;
struct LiveSessionState;
class BnwCityCurrentFoodDifference;
class BnwCitySpecialistCounts;
class BnwCityCurrentFoodConsumption;
class BnwCityCurrentFoodRateUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
class BnwCityFoodRateRules final {
 public:
  // Retain the actual DB facade as well as its private immutable Impl guard.
  // An alias is valid only with its actual owning host's strong control block.
  static BnwCityFoodRateRules load(std::shared_ptr<const content::RulesDatabase>,std::string_view rules_sha);
 private:
  friend class BnwCityCurrentFoodRate;
  struct Impl;std::shared_ptr<const Impl> impl_;
  explicit BnwCityFoodRateRules(std::shared_ptr<const Impl>)noexcept;
  void require_current()const;
};
class BnwCityCurrentFoodRate final {
 public:
  // Revalidate source/rules/generation and every reached input, then return
  // getYieldRateTimes100(FOOD,false). No freshness or growth capability follows.
  std::int32_t value_times100()const;
 private:
  friend class BnwCityCurrentFoodDifference;
  friend class BnwCitySpecialistCounts;
  friend class BnwCityCurrentFoodConsumption;
  struct Impl;std::shared_ptr<Impl> impl_;
  explicit BnwCityCurrentFoodRate(std::shared_ptr<Impl>)noexcept;
  void bind_initial();
  void bind_metadata();
  // Actual source/city identity only; no computed FOOD or happiness input.
  void bind_source_metadata();
  void require_metadata_current()const;
  // Private genuine specialist issuer. Numeric FOOD admission stays separate.
  static BnwCityCurrentFoodRate bind_specialist_source_only(
   std::shared_ptr<const LiveSession>,std::shared_ptr<const BnwCityFoodRateRules>,std::int64_t,std::int64_t);
  void capture_initial();
  static BnwCityCurrentFoodRate bind_source_only(
   std::shared_ptr<const LiveSession>,std::shared_ptr<const BnwCityFoodRateRules>,std::int64_t,std::int64_t);
  const LiveSessionState& current_source_state()const;
  std::shared_ptr<const content::RulesDatabase> current_source_database_owner()const;
  std::string current_rules_sha()const;
  const content::InfoTypeCatalog& current_source_info_types()const;
  const content::GlobalDefineCache& current_source_defines()const;
  std::int32_t canonical_owner()const;
  std::int32_t canonical_city()const;
  std::int32_t canonical_core_scalar(std::string_view)const;
  void require_current()const;
  friend BnwCityCurrentFoodRate inspect_bnw_city_current_food_rate(
   std::shared_ptr<const LiveSession>,std::shared_ptr<const BnwCityFoodRateRules>,std::int64_t,std::int64_t);
};
BnwCityCurrentFoodRate inspect_bnw_city_current_food_rate(
 std::shared_ptr<const LiveSession>,std::shared_ptr<const BnwCityFoodRateRules>,std::int64_t owner,std::int64_t city);
}
