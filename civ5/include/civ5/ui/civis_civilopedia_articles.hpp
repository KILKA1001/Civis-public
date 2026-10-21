#pragma once

#include "civ5/ui/civis_civilopedia_geometry.hpp"
#include "civ5/ui/world_view.hpp"
#include <memory>
#include <optional>

namespace civ5::ui {

struct CivisHomeArticleSource {
    bool verified = false;
    std::string lua_sha256, xml_sha256, instance_manager_sha256;
    std::string blocker;
};

// Source-only verification of the first selected Civilopedia category:
// HomePage PopulateList/DisplayList, InstanceManager and ListItemInstance.
CivisHomeArticleSource inspect_bnw_home_article_source(
    const SelectedBnwCivilopediaLayoutSources &sources);

struct CivisHomeArticleItem {
    std::int32_t entry_id = 0;
    std::string text_key, instance_name, parent_control;
    CivisControlGeometry button;
    struct Label {
        std::string id, parent_id;
        std::int32_t offset_x = 0, offset_y = 0;
        std::int32_t truncate_width = 0;
    } label;
    std::optional<WorldViewRect> measured_button_bounds;
};

struct CivisHomeArticleRowsMeasurement {
    CivilopediaLayoutOwnerKey owner;
    WorldViewRect left_panel, list_stack;
    std::vector<WorldViewRect> button_bounds;
};

struct CivisHomeArticleResult {
    bool available = false;
    CivisHomeArticleItem item;
    std::string blocker;
};

// Owns 16 real Civis Button/Label model instances for CategoryHomePage=1.
// Absolute rectangles are published only from owner measurements. Original
// BuildInstanceForControl, callbacks, SortChildren, CalculateSize and
// CalculateInternalSize remain unavailable in the live stock Lua context.
class CivisHomeArticleSubtree {
  public:
    static std::unique_ptr<CivisHomeArticleSubtree> open(
        const SelectedBnwCivilopediaLayoutSources &sources,
        const CivisCivilopediaControlTree &controls,
        const CivilopediaLayoutOwnerKey &current_owner,
        std::int32_t selected_category_id,
        std::int32_t viewport_width, std::int32_t viewport_height,
        std::string &blocker);

    CivisHomeArticleResult item(const CivilopediaLayoutOwnerKey &current_owner,
                                 std::int32_t entry_id) const;
    CivisControlGeometryResult list_stack(
        const CivilopediaLayoutOwnerKey &current_owner) const;
    std::optional<CivisHomeArticleRowsMeasurement> measured_rows(
        const CivilopediaLayoutOwnerKey &current_owner) const;
    bool attach_measured_rows(const CivilopediaLayoutOwnerKey &current_owner,
                              const CivisHomeArticleRowsMeasurement &measurement,
                              std::string &blocker);
    std::size_t item_count() const { return items_.size(); }
    bool measured_rows_available() const { return measured_rows_available_; }
    bool native_callbacks_available() const { return false; }

  private:
    CivisHomeArticleSubtree(CivilopediaLayoutOwnerKey owner,
                            std::int32_t viewport_width,
                            std::int32_t viewport_height,
                            std::int32_t left_panel_width,
                            std::int32_t left_panel_height,
                            std::vector<CivisHomeArticleItem> items);
    CivilopediaLayoutOwnerKey owner_;
    std::int32_t viewport_width_, viewport_height_;
    std::int32_t left_panel_width_, left_panel_height_;
    CivisControlGeometry list_stack_{"ListOfArticles", "LeftScrollPanel", 64, 64, 7, 0};
    std::vector<CivisHomeArticleItem> items_;
    bool measured_rows_available_ = false;
    std::optional<CivisHomeArticleRowsMeasurement> measured_rows_;
};

} // namespace civ5::ui
