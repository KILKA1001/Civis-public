#pragma once
#include "civ5/runtime/bnw_barbarian_team_has_tech.hpp"
namespace civ5::ui {class SelectedBnwReadOnlyPreview;}
namespace civ5::runtime::tests {
// Actual observations only. No callback/phase acknowledgement or grant API.
class BnwMajorStrengthFixtureCapture final {
 public:
  ~BnwMajorStrengthFixtureCapture();
  static std::shared_ptr<const BnwMajorStrengthFixtureCapture> capture_before(
      const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&);
  void verify_after(const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&)const;
 private:
  struct Impl;std::unique_ptr<Impl> impl_;
  explicit BnwMajorStrengthFixtureCapture(std::unique_ptr<Impl>);
};
}
