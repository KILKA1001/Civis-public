#pragma once

#include "civ5/ui/live_ui_audio.hpp"
#include "civ5/ui/new_era_popup_audio.hpp"

namespace civ5::ui {

enum class NewEraAssetDisposition {
    invalid_cue, stale_sequence, selected_catalog_mismatch,
    asset_blocked, pcm_ready
};

struct NewEraAssetPreflight {
    NewEraAssetDisposition disposition = NewEraAssetDisposition::invalid_cue;
    std::optional<UiAudioAssetResolution> resolution;
    std::string blocker;
};

// Pure inspection of the existing selected bridge. Even pcm_ready only means
// the selected WAV can be decoded; no transition, sink or device is invoked.
NewEraAssetPreflight inspect_new_era_audio_asset(
    const NewEraAudioSourceProof &source, const UiAuthoredAudioCue &cue,
    const LiveUiAudioBridge &bridge);

} // namespace civ5::ui
