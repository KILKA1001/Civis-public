#include "civ5/ui/civis_civilopedia_measured_hit.hpp"
#include <algorithm>
#include <limits>

namespace civ5::ui {
namespace {
bool contains(const WorldViewRect &rect, std::int32_t x, std::int32_t y) {
    return rect.width > 0 && rect.height > 0 && x >= rect.x && y >= rect.y &&
           std::int64_t(x) < std::int64_t(rect.x) + rect.width &&
           std::int64_t(y) < std::int64_t(rect.y) + rect.height;
}
}

CivisHomeMeasuredButtonEnvelope measure_civis_home_button_envelope(
    const CivisHomeArticleSubtree &subtree,
    const CivilopediaLayoutOwnerKey &current_owner) {
    CivisHomeMeasuredButtonEnvelope result;
    const auto measured = subtree.measured_rows(current_owner);
    if (!measured || measured->button_bounds.size() != 16 ||
        subtree.item_count() != 16) {
        result.blocker = "Current HomePage owner has no complete measured button geometry";
        return result;
    }
    std::int64_t left = std::numeric_limits<std::int32_t>::max();
    std::int64_t top = std::numeric_limits<std::int32_t>::max();
    std::int64_t right = std::numeric_limits<std::int32_t>::min();
    std::int64_t bottom = std::numeric_limits<std::int32_t>::min();
    for (const auto &row : measured->button_bounds) {
        left = std::min(left, std::int64_t(row.x));
        top = std::min(top, std::int64_t(row.y));
        right = std::max(right, std::int64_t(row.x) + row.width);
        bottom = std::max(bottom, std::int64_t(row.y) + row.height);
    }
    const auto below_stack = bottom - measured->list_stack.y;
    if (right <= left || bottom <= top || below_stack <= 0 ||
        right - left > std::numeric_limits<std::int32_t>::max() ||
        bottom - top > std::numeric_limits<std::int32_t>::max() ||
        below_stack > std::numeric_limits<std::int32_t>::max()) {
        result.blocker = "Measured HomePage button envelope overflows or is empty";
        return result;
    }
    result.available = true;
    result.screen_bounds = {std::int32_t(left), std::int32_t(top),
                            std::int32_t(right - left),
                            std::int32_t(bottom - top)};
    result.height_below_stack_top = std::int32_t(below_stack);
    result.blocker = "Measured buttons do not establish native Stack or ScrollPanel internal size";
    return result;
}

CivisHomeReadOnlyHit hit_test_civis_home_article(
    const CivisHomeArticleSubtree &subtree,
    const CivilopediaLayoutOwnerKey &current_owner,
    std::int32_t screen_x, std::int32_t screen_y) {
    CivisHomeReadOnlyHit result;
    const auto measured = subtree.measured_rows(current_owner);
    if (!measured || measured->button_bounds.size() != 16 ||
        subtree.item_count() != 16) {
        result.blocker = "Current HomePage owner has no measured button hit geometry";
        return result;
    }
    result.inside_panel = contains(measured->left_panel, screen_x, screen_y);
    if (!result.inside_panel) return result;
    for (std::size_t i = 0; i < measured->button_bounds.size(); ++i) {
        if (!contains(measured->button_bounds[i], screen_x, screen_y)) continue;
        const auto item = subtree.item(current_owner, std::int32_t(i + 1));
        if (!item.available || !item.item.measured_button_bounds) {
            result.blocker = "Measured HomePage row lost its current owner";
            return result;
        }
        result.matched_button = true;
        result.entry_id = item.item.entry_id;
        result.text_key = item.item.text_key;
        result.blocker = "SelectArticle callback and native context are unavailable";
        return result;
    }
    return result;
}

} // namespace civ5::ui
