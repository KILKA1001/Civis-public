#include "civ5/ui/ui_audio_events.hpp"
#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace civ5::ui {
namespace {

constexpr std::string_view popup_key = "Assets/Sounds/XML/ButtonPopupSounds.xml";
constexpr std::string_view notification_key = "Assets/Sounds/XML/NotificationSounds.xml";
constexpr std::string_view blank_script = "AS2D_PLACEHOLDER_BLANK";

std::string required_text(const content::XmlNode &row, const char *key) {
    const auto *node = row.child(key);
    if (!node || node->text.empty())
        throw std::runtime_error(std::string("UI audio row has no ") + key);
    return node->text;
}

std::vector<UiAudioMapping> parse_table(const content::XmlNode &document,
                                         std::string_view root_name,
                                         std::string_view row_name) {
    if (document.name != root_name)
        throw std::runtime_error("UI audio table has wrong XML root: " + document.name);
    std::vector<UiAudioMapping> result;
    std::unordered_set<std::string> ids;
    for (const auto &row : document.children) {
        if (row.name != row_name) continue;
        UiAudioMapping mapping{required_text(row, "ID"),
                                required_text(row, "AudioScript"), result.size()};
        if (!ids.insert(mapping.id).second)
            throw std::runtime_error("Duplicate UI audio ID: " + mapping.id);
        result.push_back(std::move(mapping));
    }
    if (result.empty()) throw std::runtime_error("UI audio table has no rows");
    return result;
}

struct SelectedDocument {
    content::XmlNode xml;
    UiAudioSource source;
};

SelectedDocument selected_document(const assets::ResourceMountTable &resources,
                                    std::string_view key) {
    auto selected = resources.resolve(assets::AssetKey(std::string(key)));
    const auto *handle = std::get_if<assets::AssetHandle>(&selected.asset);
    if (!handle)
        throw std::runtime_error("UI audio XML is missing or ambiguous: " + std::string(key));
    const auto bytes = handle->read_bytes(64ULL * 1024 * 1024);
    return {content::read_xml_metadata(std::string(bytes.begin(), bytes.end())),
            {std::string(key), handle->candidate().original_name, selected.mount_id,
             handle->candidate().provenance}};
}

} // namespace

UiAudioCatalog UiAudioCatalog::from_documents(const content::XmlNode &popups,
                                                const content::XmlNode &notifications) {
    UiAudioCatalog result;
    result.popups_ = parse_table(popups, "ButtonPopupSounds", "ButtonPopupSound");
    result.notifications_ = parse_table(notifications, "NotificationSounds", "NotificationSound");
    return result;
}

UiAudioCatalog UiAudioCatalog::from_resources(const assets::ResourceMountTable &resources) {
    const auto popups = selected_document(resources, popup_key);
    const auto notifications = selected_document(resources, notification_key);
    auto result = from_documents(popups.xml, notifications.xml);
    result.popup_source_ = popups.source;
    result.notification_source_ = notifications.source;
    return result;
}

const UiAudioMapping *UiAudioCatalog::find(UiAudioKind kind, std::string_view id) const {
    if (kind == UiAudioKind::explicit_script) return nullptr;
    const auto &rows = kind == UiAudioKind::popup ? popups_ : notifications_;
    const auto found = std::find_if(rows.begin(), rows.end(),
                                    [id](const auto &row) { return row.id == id; });
    return found == rows.end() ? nullptr : &*found;
}

UiAudioEmitStatus UiAudioQueue::emit(UiAudioKind kind, std::string_view id) {
    const auto *mapping = catalog_->find(kind, id);
    if (!mapping) return UiAudioEmitStatus::unknown;
    if (mapping->script_id == blank_script) return UiAudioEmitStatus::silent;
    pending_.push_back({next_sequence_++, kind, mapping->id,
                        mapping->script_id, mapping->ordinal});
    return UiAudioEmitStatus::queued;
}

UiAudioEmitStatus UiAudioQueue::emit_script(std::string_view source_context,
                                            std::string_view script_id) {
    if (source_context.empty() || script_id.empty()) return UiAudioEmitStatus::unknown;
    if (script_id == blank_script) return UiAudioEmitStatus::silent;
    pending_.push_back({next_sequence_++, UiAudioKind::explicit_script,
                        std::string(source_context), std::string(script_id), std::nullopt});
    return UiAudioEmitStatus::queued;
}

std::vector<UiAudioEvent> UiAudioQueue::drain() {
    std::vector<UiAudioEvent> events;
    events.swap(pending_);
    return events;
}

} // namespace civ5::ui
