#pragma once

#include "civ5/ui/civis_civilopedia_articles.hpp"

namespace civ5::ui {

struct CivisHomeMeasuredButtonEnvelope {
    bool available = false;
    WorldViewRect screen_bounds;
    std::int32_t height_below_stack_top = 0;
    bool native_internal_size_available = false;
    std::string blocker;
};

struct CivisHomeReadOnlyHit {
    bool inside_panel = false;
    bool matched_button = false;
    std::int32_t entry_id = 0;
    std::string text_key;
    bool native_callback_available = false;
    std::string blocker;
};

// Envelope of owner-measured HomePage buttons only. It excludes other
// ScrollPanel children, native Stack anchoring and scrollbar range.
CivisHomeMeasuredButtonEnvelope measure_civis_home_button_envelope(
    const CivisHomeArticleSubtree &subtree,
    const CivilopediaLayoutOwnerKey &current_owner);

// Read-only target lookup in measured, clipped screen coordinates. This does
// not dispatch SelectArticle or enable a stock UI callback.
CivisHomeReadOnlyHit hit_test_civis_home_article(
    const CivisHomeArticleSubtree &subtree,
    const CivilopediaLayoutOwnerKey &current_owner,
    std::int32_t screen_x, std::int32_t screen_y);

} // namespace civ5::ui
