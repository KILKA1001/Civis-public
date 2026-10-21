#pragma once

#include "civ5/ui/new_era_audio_asset.hpp"

namespace civ5::ui {

enum class NewEraDeliveryStatus {
    unavailable, duplicate, asset_blocked, bridge_blocked,
    sink_submitted, sink_rejected, uncertain
};

struct NewEraDeliveryResult {
    NewEraDeliveryStatus status = NewEraDeliveryStatus::unavailable;
    std::optional<UiAuthoredAudioCue> cue;
    std::optional<NewEraAssetPreflight> preflight;
    std::optional<LiveUiAudioResult> bridge_result;
    std::string blocker;
};

// Must be called only by the live NewEra PopupShown owner. It consumes the
// receipt once; blocked or partial effects are never replayed automatically.
// `sink_submitted` means PCM was accepted by the sink, not audible playback.
class NewEraAudioDelivery {
  public:
    NewEraAudioDelivery(NewEraAudioSourceProof source,
        LiveUiAudioBridge &bridge)
        : source_(source), planner_(std::move(source)), bridge_(&bridge) {}

    NewEraDeliveryResult shown(const UiFrameSnapshot &frame,
        const NewEraPopupShownReceipt &receipt);

  private:
    NewEraAudioSourceProof source_;
    NewEraPopupAudioPlanner planner_;
    LiveUiAudioBridge *bridge_;
};

} // namespace civ5::ui
