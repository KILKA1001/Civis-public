#pragma once

#include "civ5/ui/economic_overview_interaction.hpp"
#include "civ5/ui/ui_audio_events.hpp"

namespace civ5::ui {

struct EconomicAudioSourceProof {
    bool verified = false;
    bool popup_is_silent = false;
    std::string popup_audio_xml_path, popup_audio_xml_sha256;
    std::string popup_audio_selected_name;
    std::optional<std::uint64_t> popup_audio_mount_id;
    std::string blocker;
};

// The selected ButtonPopupSounds row is authoritative for popup audio. The
// chosen TopPanel/EconomicOverview Lua must not add a separate explicit cue.
EconomicAudioSourceProof inspect_economic_audio_source(
    const assets::ResourceMountTable &resources, const UiAudioCatalog &catalog,
    const EconomicGoldSourceProof &open_source,
    const EconomicCloseSourceProof &close_source);

struct EconomicAudioOpenResult {
    EconomicGoldClickResult popup;
    // Engaged only when the popup was actually queued. Silent means the
    // selected authored row has AS2D_PLACEHOLDER_BLANK and emitted no sound.
    std::optional<UiAudioEmitStatus> audio;
};

// Keeps audio attached to the actual owner transition. Rendering and blocked
// input never enqueue sound. Closing has no authored EconomicOverview cue.
class EconomicOverviewAudioInteraction {
  public:
    EconomicOverviewAudioInteraction(EconomicOverviewInteraction &interaction,
        UiAudioCatalog catalog, EconomicAudioSourceProof source);

    EconomicAudioOpenResult gold_click(const runtime::LiveSession &session,
        const UiFrameSnapshot &frame, const EconomicGoldClick &measurement,
        std::int32_t x, std::int32_t y);
    EconomicCloseResult close(const runtime::LiveSession &session,
        const UiFrameSnapshot &frame, const EconomicCloseMeasurement &measurement,
        EconomicCloseInput input, std::int32_t x, std::int32_t y);
    std::vector<UiAudioEvent> drain_audio() { return queue_.drain(); }
    std::size_t pending_audio() const { return queue_.pending(); }

  private:
    EconomicOverviewInteraction *interaction_;
    UiAudioCatalog catalog_;
    UiAudioQueue queue_;
    EconomicAudioSourceProof source_;
    bool catalog_matches_ = false;
};

} // namespace civ5::ui
