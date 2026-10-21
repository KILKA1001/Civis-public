#pragma once
#include <cstdint>
#include <memory>
#include <optional>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
class BnwNativeAttritionPrefix;class BnwNativeAttritionCityStrengthRead;struct LiveSessionState;
enum class BnwAttritionGarrisonBoundary {query_owner,city_strength_owner,info_bar_dirty_owner};
struct BnwAttritionGarrisonFacts {
 std::int32_t layer=-1,observed_damage=0;bool plot_returned=false,city_returned=false,garrisoned=false;
 std::optional<std::uint64_t>plot;
 std::optional<std::int32_t>city_owner,city_id,plot_owner,best_full_id;
};
struct BnwAttritionGarrisonProgress {
 bool issued=false,prepared=false,executed=false,failed=false;
 std::uint32_t native_calls_entered=0,native_calls_returned=0;
 std::uint64_t previous_source_serial=0,source_serial=0,revision=0;
 std::optional<std::int32_t>unit;
 BnwAttritionGarrisonFacts first;
 BnwAttritionGarrisonBoundary boundary=BnwAttritionGarrisonBoundary::query_owner;
};
// Closed read supplier using the existing actual imported best-defender
// implementation. No public constructor/factory/continuation authority.
class BnwNativeImportedGarrisonRead final {
 public:~BnwNativeImportedGarrisonRead();
 BnwNativeImportedGarrisonRead(const BnwNativeImportedGarrisonRead&)=delete;
 BnwNativeImportedGarrisonRead&operator=(const BnwNativeImportedGarrisonRead&)=delete;
 private:friend class BnwNativeAttritionPrefix;
 friend class BnwNativeAttritionCityStrengthRead;
 struct Impl;std::shared_ptr<Impl>impl_;
 explicit BnwNativeImportedGarrisonRead(std::shared_ptr<Impl>);
 static std::shared_ptr<const BnwNativeImportedGarrisonRead>read_same_live(
  const ui::SelectedBnwPreviewHost&,const LiveSessionState&,std::int32_t,std::int32_t);
 void require_without_callbacks(const ui::SelectedBnwPreviewHost&,const LiveSessionState&)const;
 void require_after_city_strength_without_callbacks(const ui::SelectedBnwPreviewHost&,
  const LiveSessionState&,const BnwNativeAttritionCityStrengthRead&)const;
 BnwAttritionGarrisonFacts facts_without_callbacks()const;
 const void*city_without_callbacks()const;
};
}
