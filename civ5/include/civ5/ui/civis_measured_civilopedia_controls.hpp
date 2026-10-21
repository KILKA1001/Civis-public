#pragma once
#include "civ5/ui/civis_civilopedia_geometry.hpp"
#include "civ5/ui/civis_measured_ui_lifecycle.hpp"

namespace civ5::ui {

// Executes the selected first layout block using the existing source-verified
// nine setters and two real Civis measured content calculations. It does not
// change the diagnostic native control tree or claim original native parity.
class CivisMeasuredCivilopediaControlTree {
  public:
    static std::unique_ptr<CivisMeasuredCivilopediaControlTree> open(
        const assets::ResourceMountTable &resources,
        const SelectedBnwCivilopediaLayoutSources &sources,
        const SelectedBnwCivilopediaLayoutPreflight &plan,
        const CivilopediaMeasuredLayout &measured,
        CivisUiExecutionScope scope, std::string &blocker);
    CivisControlSizeResult get_size(const CivilopediaLayoutOwnerKey &current,
                                    std::string_view id) const;
    CivisControlGeometryResult get_geometry(const CivilopediaLayoutOwnerKey &current,
                                            std::string_view id) const;
    CivisGeometryStepResult set_size(const CivilopediaLayoutOwnerKey &current,
        std::string_view id, std::int32_t width, std::int32_t height);
    CivisGeometryStepResult set_offset(const CivilopediaLayoutOwnerKey &current,
        std::string_view id, std::int32_t x, std::int32_t y);
    CivisUiStep publish(const CivilopediaLayoutOwnerKey &current,
                        CivisMeasuredScrollSnapshot snapshot);
    CivisGeometryStepResult calculate_internal_size(
        const CivilopediaLayoutOwnerKey &current, std::string_view id);
    CivisGeometryStepResult apply_ordered_plan(const CivilopediaLayoutOwnerKey &current);
    CivisMeasuredScrollResult scroll_state(const CivilopediaLayoutOwnerKey &current,
                                           std::string_view id) const;
    CivisUiStep set_scroll(const CivilopediaLayoutOwnerKey &current,
                            std::string_view id, std::int32_t x, std::int32_t y);
    const CivilopediaLayoutOwnerKey &owner() const { return prefix_->owner(); }
    std::size_t completed_operations() const;
    const CivisCivilopediaControlTree &geometry_prefix() const { return *prefix_; }
    CivisUiExecutionScope scope() const { return scope_; }
    bool native_parity_verified() const { return false; }
  private:
    CivisMeasuredCivilopediaControlTree(std::unique_ptr<CivisCivilopediaControlTree> prefix,
        std::vector<CivilopediaLayoutInstruction> instructions,
        CivisUiExecutionScope scope);
    std::unique_ptr<CivisCivilopediaControlTree> prefix_;
    std::vector<CivilopediaLayoutInstruction> instructions_;
    CivisUiExecutionScope scope_;
    CivisMeasuredScrollLayout scroll_;
    std::map<std::string, std::uint64_t> revisions_;
    std::size_t internal_completed_ = 0;
};
} // namespace civ5::ui
