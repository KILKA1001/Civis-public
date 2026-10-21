#pragma once
#include "civ5/runtime/bnw_research_queue_checkpoint.hpp"
#include <filesystem>
#include <memory>
#include <optional>
namespace civ5::ui { class SelectedBnwReadOnlyPreview; }
namespace civ5::runtime {
class BnwResearchChoiceCommit;
// Partial research persistence only: no turn, Lua journal or world readiness.
// Factory admits a genuine selected Host and checks the measured input's exact
// retained section identity. It clones its comparison baseline ONCE, not per export.
class BnwResearchQueueSaveOwner final {
 public:
  ~BnwResearchQueueSaveOwner();
  BnwResearchQueueSaveOwner(const BnwResearchQueueSaveOwner&)=delete;
  BnwResearchQueueSaveOwner& operator=(const BnwResearchQueueSaveOwner&)=delete;
  static std::shared_ptr<BnwResearchQueueSaveOwner> from_selected_preview(
      const ui::SelectedBnwReadOnlyPreview&,const std::filesystem::path& actual_input);
  // A changed UI revision requires the genuine SAME-state complete dirty tail.
  // Passing a receipt does not authenticate any unrelated world effect.
  saves::CivisSaveDocument prepare(
      std::shared_ptr<const BnwResearchChoiceCommit> completed_choice={}) const;
  void write_new(const std::filesystem::path&,
      std::shared_ptr<const BnwResearchChoiceCommit> completed_choice={}) const;
 private:
  struct Impl;std::shared_ptr<Impl> impl_;
  explicit BnwResearchQueueSaveOwner(std::shared_ptr<Impl>);
};
namespace detail {
// Existing ID33 v1 wire, emitted directly from queue/flag owners. No world copy.
// nullopt is the genuine unchanged-original queue case, NOT an empty ID33.
std::optional<BnwResearchQueueCheckpointBundle> prepare_bnw_research_queue_bundle(
 const LiveSessionState&,const LiveSessionState&,const saves::SavedSessionSnapshot&,
 const content::TechnologyRules&,const content::InfoTypeCatalog&,
 const SessionCheckpointIdentity&);
}
}
