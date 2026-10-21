#pragma once
#include "civ5/runtime/bnw_city_current_food_rate.hpp"
#include <optional>
#include <vector>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::saves {struct SavedSessionSnapshot;}
namespace civ5::runtime {
class BnwMajorPlayerTraitsOwner;class BnwNativeUnitAllocatorCore;
class BnwCityCurrentFoodConsumption;
class BnwCityCurrentSupportStrategy;
class BnwCityCurrentScienceRate;
class BnwCitySpecialistCountsUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
namespace detail {
// Pure wire-remap/math DTO: null count means null hash; absent mapped ID with
// count means an unknown consumed value. It cannot issue a source owner.
struct BnwSpecialistMathRow {std::optional<std::int32_t> mapped_id,count;};
struct BnwSpecialistMathResult {std::vector<std::int32_t> dense;std::int32_t total=0;};
std::vector<std::int32_t> bnw_specialist_reset_read_dense_math(std::int32_t span,
 const std::vector<BnwSpecialistMathRow>&);
BnwSpecialistMathResult bnw_specialist_reset_read_math(std::int32_t span,
 std::int32_t default_specialist,const std::vector<BnwSpecialistMathRow>&);
}
// Immutable numeric Reset/Read source. Issuance is private to the actual closed
// rate/consumption or SAME genuine current strategy parents; neither a caller
// catalog nor a DTO can mint it.
class BnwCitySpecialistCounts final {
 public:
  void require_current_source()const;
  std::int32_t total_for_city(std::int64_t owner,std::int64_t canonical_city)const;
  std::int32_t count_for_city(std::int64_t owner,std::int64_t canonical_city,std::int32_t specialist)const;
  std::int32_t building_rate_for_city(std::int64_t owner,std::int64_t canonical_city,std::int32_t specialist)const;
  std::vector<std::int64_t> reached_source_seal()const;
  void require_same_world_database(const saves::SavedSessionSnapshot&,
                                  const content::RulesDatabase&)const;
 private:
  friend class BnwCityCurrentSupportStrategy;
  friend class BnwCityEntryPending;friend class BnwCityFoodGrowthPending;
  friend class BnwCityCurrentScienceRate;
class BnwCityCurrentScienceRate;
  friend class BnwCityCurrentFoodRate;
  friend class BnwMajorPlayerTraitsOwner;friend class BnwNativeUnitAllocatorCore;
  friend class BnwCityCurrentFoodConsumption;
  struct Impl;std::shared_ptr<Impl> impl_;
  explicit BnwCitySpecialistCounts(std::shared_ptr<Impl>)noexcept;
  static BnwCitySpecialistCounts from_same_rate(const BnwCityCurrentFoodRate&);
  static BnwCitySpecialistCounts from_same_host(
   std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t canonical_city);
  std::int32_t building_specialists_for_city(std::int64_t owner,std::int64_t city,std::int32_t building)const;
  std::int32_t forced_building_specialists_for_city(std::int64_t owner,std::int64_t city,std::int32_t building)const;
  std::vector<std::int32_t> capture_dense_for_city(std::int64_t owner,std::int64_t city,
   bool total_requested,std::string_view source_array)const;
};
}
