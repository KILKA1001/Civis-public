#pragma once

#include "civ5/assets/resource_mounts.hpp"
#include "civ5/content/manifest.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::ui {

enum class UiAudioKind { popup, notification, explicit_script };

// Supplied by the live event owner, not inferred from an XML context or a
// rendered frame. Sequence numbers must increase across popup/notification
// transitions for one audio bridge. ID names a row in the selected UI table.
struct UiAuthoredAudioCue {
    std::uint64_t sequence = 0;
    std::string id;
};

struct UiAudioMapping {
    std::string id;
    std::string script_id;
    std::size_t ordinal = 0;
};

struct UiAudioSource {
    std::string requested_key;
    std::string selected_name;
    std::optional<std::uint64_t> mount_id;
    std::optional<assets::Provenance> provenance;
};

// These tables map UI event IDs to authored audio script IDs. An AS2D script
// still needs the game's audio definitions, a decoder and a playback backend.
class UiAudioCatalog {
  public:
    static UiAudioCatalog from_documents(const content::XmlNode &popups,
                                          const content::XmlNode &notifications);
    static UiAudioCatalog from_resources(const assets::ResourceMountTable &resources);

    const UiAudioMapping *find(UiAudioKind kind, std::string_view id) const;
    const std::vector<UiAudioMapping> &popups() const { return popups_; }
    const std::vector<UiAudioMapping> &notifications() const { return notifications_; }
    const std::optional<UiAudioSource> &popup_source() const { return popup_source_; }
    const std::optional<UiAudioSource> &notification_source() const {
        return notification_source_;
    }

  private:
    std::vector<UiAudioMapping> popups_, notifications_;
    std::optional<UiAudioSource> popup_source_, notification_source_;
};

struct UiAudioEvent {
    std::uint64_t sequence = 0;
    UiAudioKind kind = UiAudioKind::popup;
    std::string id;
    std::string script_id;
    std::optional<std::size_t> authored_ordinal;
};

enum class UiAudioEmitStatus { queued, silent, unknown };

// Game/UI code emits transitions once when they happen. Building a frame does
// not emit sound, so repeated renders cannot replay a notification by accident.
class UiAudioQueue {
  public:
    explicit UiAudioQueue(const UiAudioCatalog &catalog) : catalog_(&catalog) {}

    UiAudioEmitStatus emit(UiAudioKind kind, std::string_view id);
    // For authored Lua calls to Events.AudioPlay2DSound(script), where no
    // popup/notification table row exists. The caller supplies that script.
    UiAudioEmitStatus emit_script(std::string_view source_context,
                                  std::string_view script_id);
    std::vector<UiAudioEvent> drain();
    std::size_t pending() const { return pending_.size(); }

  private:
    const UiAudioCatalog *catalog_;
    std::vector<UiAudioEvent> pending_;
    std::uint64_t next_sequence_ = 1;
};

} // namespace civ5::ui
