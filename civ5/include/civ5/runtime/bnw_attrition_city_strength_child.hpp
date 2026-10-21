#pragma once
#include <array>
#include "civ5/runtime/bnw_attrition_garrison_child.hpp"
namespace civ5::runtime {
class BnwAttritionCityInfoDirtyLoan;
struct BnwAttritionCityStrengthProgress {
 bool issued=false,prepared=false,attempted=false,executed=false,failed=false;
 bool base_assignment_invoked=false,hills_query_returned=false,hill_assignment_invoked=false;
 bool archive_dirty=false,cache_stored=false;
 std::uint64_t previous_source_serial=0,source_serial=0,before_revision=0,after_revision=0,archive_changes=0;
 std::optional<std::int32_t>owner,city,unit,before_strength,base_strength,after_strength,observed_damage,defender;
 std::uint32_t get_garrisoned_unit_returned=0;
 // Return of updateStrengthValue is NOT issued before genuine CityInfo18.
 bool city_info_dirty_returned=false,update_strength_returned=false;
};
class BnwNativeAttritionCityStrengthRead final {
 public:~BnwNativeAttritionCityStrengthRead();
 BnwNativeAttritionCityStrengthRead(const BnwNativeAttritionCityStrengthRead&)=delete;
 BnwNativeAttritionCityStrengthRead&operator=(const BnwNativeAttritionCityStrengthRead&)=delete;
 private:
 friend class BnwNativeAttritionPrefix;
 friend class BnwAttritionCityInfoDirtyLoan;
 friend class BnwNativeImportedGarrisonRead;
 struct Impl;std::shared_ptr<Impl>impl_;
 explicit BnwNativeAttritionCityStrengthRead(std::shared_ptr<Impl>);
 static std::shared_ptr<BnwNativeAttritionCityStrengthRead>prepare_same_live(
  const ui::SelectedBnwPreviewHost&,LiveSessionState&,
  const std::array<std::shared_ptr<const BnwNativeImportedGarrisonRead>,3>&);
 void require_without_callbacks(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,bool)const;
 void execute_without_callbacks(LiveSessionState&)const;
 const void*receiver_without_callbacks()const;
 std::uint64_t after_revision_without_callbacks()const;
 BnwAttritionCityStrengthProgress facts_without_callbacks()const;
};
}
