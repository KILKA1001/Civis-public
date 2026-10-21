#pragma once
#include "civ5/runtime/bnw_happiness_update_prefix.hpp"
#include <memory>
#include <stdexcept>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
class BnwCityFoodGrowthPending;class BnwCityEntryPending;class BnwCityReligionYieldOwner;
class BnwPlayerHappinessPrefixUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
// No independent issuer. Only the actual registered Religion/Entry call can
// retain and publish this native prefix; supplied stage math has no authority.
class BnwPlayerHappinessPrefixOwner final {
 public:
 ~BnwPlayerHappinessPrefixOwner();
 BnwPlayerHappinessPrefixOwner(const BnwPlayerHappinessPrefixOwner&)=delete;
 BnwPlayerHappinessPrefixOwner& operator=(const BnwPlayerHappinessPrefixOwner&)=delete;
 private:
 friend class BnwCityFoodGrowthPending;friend class BnwCityEntryPending;
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwPlayerHappinessPrefixOwner(std::unique_ptr<Impl>)noexcept;
 struct Step {
  const Impl* owner=nullptr;std::uint8_t stage=0;
  std::int32_t before=0,after=0,operand=0;
  std::string next_raw_seal;
 };
 static std::unique_ptr<BnwPlayerHappinessPrefixOwner> from_religion_tail(
  const BnwCityEntryPending&,const BnwCityReligionYieldOwner&,
  std::shared_ptr<const ui::SelectedBnwPreviewHost>,
  const std::shared_ptr<const LiveSessionState>&,
  const std::shared_ptr<const saves::SavedSession>&);
 void require_current(const BnwCityEntryPending&,
  const std::shared_ptr<const ui::SelectedBnwPreviewHost>&)const;
 Step prepare_next(const BnwCityEntryPending&,
  const std::shared_ptr<const ui::SelectedBnwPreviewHost>&)const;
 void validate_before_publication(const Step&,const BnwCityEntryPending&,
  const std::shared_ptr<const ui::SelectedBnwPreviewHost>&)const;
 static std::unique_ptr<BnwPlayerHappinessPrefixOwner> from_population_religion_tail(
  const BnwCityFoodGrowthPending&,const BnwCityReligionYieldOwner&,
  std::shared_ptr<const ui::SelectedBnwPreviewHost>,
  const std::shared_ptr<const LiveSessionState>&,
  const std::shared_ptr<const saves::SavedSession>&);
 void require_current(const BnwCityFoodGrowthPending&,
  const std::shared_ptr<const ui::SelectedBnwPreviewHost>&)const;
 Step prepare_next(const BnwCityFoodGrowthPending&,
  const std::shared_ptr<const ui::SelectedBnwPreviewHost>&)const;
 void validate_before_publication(const Step&,const BnwCityFoodGrowthPending&,
  const std::shared_ptr<const ui::SelectedBnwPreviewHost>&)const;
 template<class Parent>static std::unique_ptr<BnwPlayerHappinessPrefixOwner> from_religion_tail_bound(
  const Parent&,const BnwCityReligionYieldOwner&,std::shared_ptr<const ui::SelectedBnwPreviewHost>,
  const std::shared_ptr<const LiveSessionState>&,const std::shared_ptr<const saves::SavedSession>&);
 template<class Parent>void require_current_bound(const Parent&,const std::shared_ptr<const ui::SelectedBnwPreviewHost>&)const;
 template<class Parent>Step prepare_next_bound(const Parent&,const std::shared_ptr<const ui::SelectedBnwPreviewHost>&)const;
 template<class Parent>void validate_before_publication_bound(const Step&,const Parent&,const std::shared_ptr<const ui::SelectedBnwPreviewHost>&)const;
 void commit_published(Step&&,std::uint64_t revision)noexcept;
 void retire()noexcept;
};
}
