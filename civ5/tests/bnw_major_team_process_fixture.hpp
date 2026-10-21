#pragma once
#include "civ5/runtime/bnw_barbarian_team_has_tech.hpp"
namespace civ5::ui {class SelectedBnwReadOnlyPreview;}
namespace civ5::runtime::tests {
// Called by the real SAME-VM producer, after the actual city loop and before
// the private resource/process continuation. This observer cannot run hooks,
// store flags, advance a cursor or mint a predecessor.
class BnwMajorProcessFixtureCapture final {
 public:
 ~BnwMajorProcessFixtureCapture();
 static std::shared_ptr<const BnwMajorProcessFixtureCapture> capture_before(
     const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&);
 void verify_after(const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&)const;
 private:
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwMajorProcessFixtureCapture(std::unique_ptr<Impl>);
};
}

namespace civ5::runtime::tests {
class BnwMajorTraitsFixtureCapture final {
 public:
 ~BnwMajorTraitsFixtureCapture();
 static std::shared_ptr<const BnwMajorTraitsFixtureCapture> capture_before(
     const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&);
 void verify_after(const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&,
     const BnwMajorPlayerTraitsOwner& actual_owner)const;
 private:
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwMajorTraitsFixtureCapture(std::unique_ptr<Impl>);
};
}

namespace civ5::runtime::tests {
class BnwMajorPolicyFixtureCapture final {
 public:
 ~BnwMajorPolicyFixtureCapture();
 static std::shared_ptr<const BnwMajorPolicyFixtureCapture> capture_before(
     const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&);
 void verify_after(const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&,
     const BnwMajorPlayerTraitsOwner& actual_owner)const;
 private:
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwMajorPolicyFixtureCapture(std::unique_ptr<Impl>);
};
}

namespace civ5::runtime::tests {
// Observes only a genuine producer's real hook+post-hook execution; cannot
// dispatch an event, advance a phase, or manufacture a positive fixture.
class BnwMajorResearchedFixtureCapture final {
 public:
 ~BnwMajorResearchedFixtureCapture();
 static std::shared_ptr<const BnwMajorResearchedFixtureCapture> capture_before(
     const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&);
 void verify_after(const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&)const;
 private:
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwMajorResearchedFixtureCapture(std::unique_ptr<Impl>);
};
}

namespace civ5::runtime::tests {
class BnwMajorPlayerResearchFixtureCapture final {
 public:
 ~BnwMajorPlayerResearchFixtureCapture();
 static std::shared_ptr<const BnwMajorPlayerResearchFixtureCapture> capture_before(
     const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&);
 void verify_after(const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&)const;
 private:
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwMajorPlayerResearchFixtureCapture(std::unique_ptr<Impl>);
};
}

namespace civ5::runtime::tests {
// Observer of the genuine source operation only; cannot create a receipt or
// manufacture an announcement, contact, culture city, or no-listener result.
class BnwMajorAnnouncementFixtureCapture final {
 public:
 ~BnwMajorAnnouncementFixtureCapture();
 static std::shared_ptr<const BnwMajorAnnouncementFixtureCapture> capture_before(
  const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&);
 void verify_after(const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&)const;
 private:
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwMajorAnnouncementFixtureCapture(std::unique_ptr<Impl>);
};
}

namespace civ5::runtime::tests {
class BnwMajorEspionageFixtureCapture final {
 public:
 ~BnwMajorEspionageFixtureCapture();
 static std::shared_ptr<const BnwMajorEspionageFixtureCapture> capture_before(
  const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&);
 void verify_after(const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&)const;
 private:
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwMajorEspionageFixtureCapture(std::unique_ptr<Impl>);
};
}

namespace civ5::runtime::tests {
class BnwMajorHappinessFixtureCapture final {
 public:
 ~BnwMajorHappinessFixtureCapture();
 static std::shared_ptr<const BnwMajorHappinessFixtureCapture> capture_before(
  const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&);
 void verify_after(const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&)const;
 private:
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwMajorHappinessFixtureCapture(std::unique_ptr<Impl>);
};
}

namespace civ5::runtime::tests {
class BnwMajorHappinessSuffixFixtureCapture final {
 public:
 ~BnwMajorHappinessSuffixFixtureCapture();
 static std::shared_ptr<const BnwMajorHappinessSuffixFixtureCapture> capture_before(
  const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&);
 void verify_after(const ui::SelectedBnwReadOnlyPreview&,const BnwMajorTeamHasTechPending&)const;
 private:
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwMajorHappinessSuffixFixtureCapture(std::unique_ptr<Impl>);
};
}
