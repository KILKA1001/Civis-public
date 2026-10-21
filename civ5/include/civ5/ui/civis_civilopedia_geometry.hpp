#pragma once

#include "civ5/ui/selected_bnw_civilopedia_layout.hpp"
#include <array>
#include <memory>

namespace civ5::ui {

struct CivisControlGeometry {
    std::string id, parent_id;
    std::int32_t width = 0, height = 0;
    std::int32_t offset_x = 0, offset_y = 0;
};

struct CivisControlSizeResult {
    bool available = false;
    std::int32_t width = 0, height = 0;
    std::string blocker;
};

struct CivisControlGeometryResult {
    bool available = false;
    CivisControlGeometry geometry;
    std::string blocker;
};

struct CivisGeometryStepResult {
    bool applied = false;
    std::size_t completed_operations = 0;
    std::string blocker;
};

// A frame-owned Civis geometry object. It mutates eight controls from selected
// XML/VFS evidence, but is not the game's native Controls implementation.
// The original ScrollPanel::CalculateInternalSize algorithm is unavailable;
// an ordered plan stops at that call without issuing a success receipt.
class CivisCivilopediaControlTree {
  public:
    static std::unique_ptr<CivisCivilopediaControlTree> open(
        const assets::ResourceMountTable &selected_ui_resources,
        const SelectedBnwCivilopediaLayoutSources &sources,
        const SelectedBnwCivilopediaLayoutPreflight &plan,
        const CivilopediaMeasuredLayout &measured,
        std::string &blocker);

    CivisControlSizeResult get_size(const CivilopediaLayoutOwnerKey &current,
                                    std::string_view control_id) const;
    CivisControlGeometryResult get_geometry(
        const CivilopediaLayoutOwnerKey &current,
        std::string_view control_id) const;
    CivisGeometryStepResult set_size(const CivilopediaLayoutOwnerKey &current,
                                     std::string_view control_id,
                                     std::int32_t width, std::int32_t height);
    CivisGeometryStepResult set_offset(const CivilopediaLayoutOwnerKey &current,
                                       std::string_view control_id,
                                       std::int32_t x, std::int32_t y);
    CivisGeometryStepResult calculate_internal_size(
        const CivilopediaLayoutOwnerKey &current,
        std::string_view control_id);
    CivisGeometryStepResult apply_ordered_plan(
        const CivilopediaLayoutOwnerKey &current);
    const CivilopediaLayoutOwnerKey &owner() const { return owner_; }
    std::size_t completed_operations() const { return next_operation_; }

  private:
    CivisCivilopediaControlTree(
        CivilopediaLayoutOwnerKey owner,
        std::vector<CivilopediaLayoutInstruction> instructions,
        std::array<CivisControlGeometry, 8> controls);
    CivisGeometryStepResult apply_mutation(
        const CivilopediaLayoutOwnerKey &current,
        CivilopediaLayoutOperation operation,
        std::string_view control_id, std::int32_t x, std::int32_t y);
    CivisControlGeometry *find(std::string_view control_id);
    const CivisControlGeometry *find(std::string_view control_id) const;

    CivilopediaLayoutOwnerKey owner_;
    std::vector<CivilopediaLayoutInstruction> instructions_;
    std::array<CivisControlGeometry, 8> controls_;
    std::size_t next_operation_ = 0;
};

} // namespace civ5::ui
