#include "civ5/content/info_metadata.hpp"

namespace civ5::content {
InfoMetadata read_info_metadata(const RulesRowView &row, const InfoLocalizer &localizer) {
    InfoMetadata info;
    info.authored_id = row.integer("ID");
    info.type = row.text("Type").value_or("");
    info.civilopedia = row.text("Civilopedia").value_or("");
    info.strategy = row.text("Strategy").value_or("");
    info.help = row.text("Help").value_or("");
    info.disabled_help = row.text("DisabledHelp").value_or("");
    info.text.key = row.text("Text").value_or("");
    if (!info.text.key.empty() && localizer) info.text.value = localizer(info.text.key);
    if (const auto description = row.text("Description")) {
        info.description.key = *description;
        if (localizer) info.description.value = localizer(*description);
        if (info.text.key.empty()) info.text = info.description;
    }
    return info;
}
} // namespace civ5::content
