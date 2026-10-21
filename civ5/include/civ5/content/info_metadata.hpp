#pragma once
#include "civ5/content/info_collections.hpp"
#include <functional>

namespace civ5::content {
using InfoLocalizer = std::function<std::string(std::string_view)>;
struct InfoText {
    std::string key;
    // Absent until a host localizer is supplied; never masquerade a key as text.
    std::optional<std::string> value;
};
struct InfoMetadata {
    std::int64_t authored_id = -1;
    std::string type, civilopedia, strategy, help, disabled_help;
    InfoText text, description;
};
// Fresh CvBaseInfo cache, including Text fallback to a non-NULL Description.
InfoMetadata read_info_metadata(const RulesRowView &row, const InfoLocalizer &localizer = {});
} // namespace civ5::content
