#pragma once
#include "civ5/runtime/city_yields.hpp"
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::saves {struct SavedSession;}
namespace civ5::runtime {
struct LiveSessionState;
class BnwCityFoodGrowthPending;class BnwCityEntryPending;
class BnwCitySpecialistCounts;class BnwPlayerHappinessPrefixOwner;
class BnwCityReligionYieldUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
namespace detail {
// Supplied arithmetic only. No child completion, source stage or writer grant.
std::int32_t bnw_city_religion_centi_math(std::span<const CityYieldReligion>,std::int32_t);
std::int32_t bnw_city_religion_sum_math(std::span<const std::int32_t>);
}
// No public issuer or advancement. This child is owned by the actual Entry
// operation AFTER its genuine 37-position plot-yield child returns.
class BnwCityReligionYieldOwner final {
 public:
  BnwCityReligionYieldOwner(const BnwCityReligionYieldOwner&)=delete;
  BnwCityReligionYieldOwner& operator=(const BnwCityReligionYieldOwner&)=delete;
  ~BnwCityReligionYieldOwner();
 private:
  friend class BnwCityFoodGrowthPending;friend class BnwCityEntryPending;friend class BnwPlayerHappinessPrefixOwner;
  struct Impl;std::unique_ptr<Impl> impl_;
  explicit BnwCityReligionYieldOwner(std::unique_ptr<Impl>)noexcept;
  struct Cursor {std::uint8_t phase=0,yield=0;std::int32_t building_class=0,followers=0,secondary=-1;};
  struct Step {
   const Impl* owner=nullptr;std::uint64_t serial=0;Cursor after;
   std::string_view field;std::int8_t vector_yield=-1;
   std::int32_t before=0,after_value=0;
   bool writes=false,base_city_screen=false;
  };
  static std::int32_t capture_majority_for_entry(const BnwCityEntryPending&,
   const std::shared_ptr<const ui::SelectedBnwPreviewHost>&,
   const std::shared_ptr<const LiveSessionState>& actual_parent_allocation,
   const std::shared_ptr<const saves::SavedSession>& actual_parent_source);
  static std::unique_ptr<BnwCityReligionYieldOwner> after_city_yield_child(
   const BnwCityEntryPending&,std::shared_ptr<const ui::SelectedBnwPreviewHost>,
   const std::shared_ptr<const LiveSessionState>& actual_parent_allocation,
   const std::shared_ptr<const saves::SavedSession>& actual_parent_source,
   std::int32_t retained_entry_majority);
  static std::int32_t capture_majority_for_population(const BnwCityFoodGrowthPending&,
   const std::shared_ptr<const ui::SelectedBnwPreviewHost>&,const std::shared_ptr<const LiveSessionState>&,
   const std::shared_ptr<const saves::SavedSession>&);
  static std::unique_ptr<BnwCityReligionYieldOwner> after_population_city_yield_child(
   const BnwCityFoodGrowthPending&,std::shared_ptr<const ui::SelectedBnwPreviewHost>,
   const std::shared_ptr<const LiveSessionState>&,const std::shared_ptr<const saves::SavedSession>&,std::int32_t);
  void require_current(const BnwCityFoodGrowthPending&,const std::shared_ptr<const ui::SelectedBnwPreviewHost>&)const;
  Step prepare_next(const BnwCityFoodGrowthPending&,const std::shared_ptr<const ui::SelectedBnwPreviewHost>&,const BnwCitySpecialistCounts*)const;
  void validate_before_publication(const Step&,const BnwCityFoodGrowthPending&,const std::shared_ptr<const ui::SelectedBnwPreviewHost>&)const;
  template<class Parent>void require_current_bound(const Parent&,const std::shared_ptr<const ui::SelectedBnwPreviewHost>&)const;
  template<class Parent>Step prepare_next_bound(const Parent&,const std::shared_ptr<const ui::SelectedBnwPreviewHost>&,const BnwCitySpecialistCounts*)const;
  template<class Parent>void validate_before_publication_bound(const Step&,const Parent&,const std::shared_ptr<const ui::SelectedBnwPreviewHost>&)const;
  void require_current(const BnwCityEntryPending&,
   const std::shared_ptr<const ui::SelectedBnwPreviewHost>&)const;
  void require_player_happiness_tail(const BnwCityFoodGrowthPending&,
   const std::shared_ptr<const ui::SelectedBnwPreviewHost>&)const;
  bool needs_specialist_total()const noexcept;
  bool player_happiness_tail_reached()const noexcept;
  void require_player_happiness_tail(const BnwCityEntryPending&,
   const std::shared_ptr<const ui::SelectedBnwPreviewHost>&)const;
  Step prepare_next(const BnwCityEntryPending&,
   const std::shared_ptr<const ui::SelectedBnwPreviewHost>&,
   const BnwCitySpecialistCounts* reached_specialists)const;
  void validate_before_publication(const Step&,const BnwCityEntryPending&,
   const std::shared_ptr<const ui::SelectedBnwPreviewHost>&)const;
  // Only after the existing parent's actual no-throw raw+mirror publication.
  void commit_published(const Step&)noexcept;
  void retire()noexcept;
};
}
