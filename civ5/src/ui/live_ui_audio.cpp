#include "civ5/ui/live_ui_audio.hpp"
#include "civ5/ui/bnw_audio_source_catalog.hpp"
#include <stdexcept>
#include <utility>

namespace civ5::ui {

LiveUiAudioBridge::LiveUiAudioBridge(assets::ResourceMountTable resources,
                                     std::unique_ptr<UiPcmSink> sink)
    : LiveUiAudioBridge(std::move(resources), content::Catalog{}, {}, std::move(sink)) {}

LiveUiAudioBridge::LiveUiAudioBridge(assets::ResourceMountTable resources,
    const content::Catalog &catalog, std::span<const UiSelectedDlcAudio> selected,
    std::unique_ptr<UiPcmSink> sink)
    : resources_(std::make_shared<const assets::ResourceMountTable>(std::move(resources))),
      catalog_(UiAudioCatalog::from_resources(*resources_)),
      resolver_(UiAudioAssetResolver::from_selected_dlc(*resources_, catalog, selected)),
      queue_(catalog_), sink_(std::move(sink)) {
    if (!sink_) throw std::invalid_argument("Live UI audio sink is missing");
}

std::unique_ptr<LiveUiAudioBridge> LiveUiAudioBridge::from_completed_resources(
    std::shared_ptr<const BnwAudioCompletedResourceOwner> owner,
    std::shared_ptr<const BnwAudioSourceCatalog> catalog) {
    if(!owner||!catalog)throw std::invalid_argument("Completed live audio bridge source owners missing");
    owner=owner->retain_factory_owner(owner);catalog=catalog->retain_factory_owner(catalog);
    catalog->require_same_completed_resources(*owner);
    // UiAudioAssetResolver retains a raw table pointer. A scoped table cannot
    // back that resolver; neither a copy nor a shared_ptr alias grants a loan.
    throw std::runtime_error("Completed live audio bridge requires scoped native script/stream resolver and actual initialized device/MASTER owner");
}
bool LiveUiAudioBridge::accept_sequence(std::uint64_t sequence) {
    if (!sequence || sequence <= last_sequence_) return false;
    last_sequence_ = sequence;
    return true;
}

LiveUiAudioResult LiveUiAudioBridge::finish(UiAudioEmitStatus emission) {
    LiveUiAudioResult result{LiveUiAudioDisposition::processed, emission, {}};
    if (emission != UiAudioEmitStatus::queued) return result;
    auto playback = submit_ui_audio_events(queue_, resolver_, *sink_);
    if (playback.size() != 1)
        throw std::logic_error("One UI transition produced an unexpected audio batch");
    result.playback = std::move(playback.front());
    return result;
}

LiveUiAudioResult LiveUiAudioBridge::transition(std::uint64_t event_sequence,
                                                 UiAudioKind kind,
                                                 std::string_view authored_id) {
    if (!event_sequence || authored_id.empty() || kind == UiAudioKind::explicit_script)
        return {};
    if (!accept_sequence(event_sequence))
        return {LiveUiAudioDisposition::duplicate_or_stale,
                UiAudioEmitStatus::unknown, {}};
    return finish(queue_.emit(kind, authored_id));
}

LiveUiAudioResult LiveUiAudioBridge::script_transition(
    std::uint64_t event_sequence, std::string_view source_context,
    std::string_view authored_script_id) {
    if (!event_sequence || source_context.empty() || authored_script_id.empty())
        return {};
    if (!accept_sequence(event_sequence))
        return {LiveUiAudioDisposition::duplicate_or_stale,
                UiAudioEmitStatus::unknown, {}};
    return finish(queue_.emit_script(source_context, authored_script_id));
}

std::optional<WinMmAudioSnapshot> LiveUiAudioBridge::device_status() const {
    if (const auto *device = dynamic_cast<const WinMmPcmSink *>(sink_.get()))
        return device->snapshot();
    return std::nullopt;
}

} // namespace civ5::ui
