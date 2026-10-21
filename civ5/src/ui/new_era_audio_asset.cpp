#include "civ5/ui/new_era_audio_asset.hpp"

namespace civ5::ui {
namespace {
constexpr auto popup_id = "BUTTONPOPUP_NEW_ERA";
constexpr auto script_id = "AS2D_INTERFACE_NEW_ERA";
constexpr auto sound_id = "SND_INTERFACE_NEW_ERA";
constexpr auto filename = "NotificationNewEra";
} // namespace

NewEraAssetPreflight inspect_new_era_audio_asset(
    const NewEraAudioSourceProof &source, const UiAuthoredAudioCue &cue,
    const LiveUiAudioBridge &bridge) {
    NewEraAssetPreflight result;
    if (!source.verified || source.in_game_sha256.empty() ||
        source.lua_sha256.empty() || source.xml_sha256.empty() ||
        source.popup_audio_sha256.empty() ||
        cue.id != popup_id || !cue.sequence) {
        result.blocker = "NewEra owner cue or selected popup source is unverified";
        return result;
    }
    if (cue.sequence <= bridge.last_transition_sequence()) {
        result.disposition = NewEraAssetDisposition::stale_sequence;
        result.blocker = "NewEra cue sequence was already consumed by the bridge";
        return result;
    }
    const auto &catalog = bridge.audio_catalog();
    const auto *mapping = catalog.find(UiAudioKind::popup, popup_id);
    const auto &selected = catalog.popup_source();
    if (!mapping || mapping->script_id != script_id ||
        mapping->ordinal != source.popup_audio_ordinal ||
        !selected ||
        selected->selected_name != source.popup_audio_selected_name ||
        selected->mount_id != source.popup_audio_mount_id) {
        result.disposition = NewEraAssetDisposition::selected_catalog_mismatch;
        result.blocker = "Live bridge popup catalog differs from selected NewEra source";
        return result;
    }
    result.resolution = bridge.inspect_script(script_id);
    if (result.resolution->sound_id != sound_id ||
        result.resolution->filename != filename ||
        !result.resolution->script_source ||
        !result.resolution->definition_source) {
        result.disposition = NewEraAssetDisposition::asset_blocked;
        result.blocker = "Selected NewEra AS2D/SND definition is missing or changed";
        return result;
    }
    if (result.resolution->status != UiAudioAssetStatus::pcm_ready) {
        result.disposition = NewEraAssetDisposition::asset_blocked;
        result.blocker = result.resolution->reason;
        return result;
    }
    result.disposition = NewEraAssetDisposition::pcm_ready;
    return result;
}

} // namespace civ5::ui
