#include "civ5/ui/new_era_audio_delivery.hpp"

namespace civ5::ui {

NewEraDeliveryResult NewEraAudioDelivery::shown(
    const UiFrameSnapshot &frame, const NewEraPopupShownReceipt &receipt) {
    NewEraDeliveryResult result;
    const auto planned = planner_.shown(frame, receipt);
    if (!planned.cue) {
        result.status = planned.duplicate ? NewEraDeliveryStatus::duplicate
                                          : NewEraDeliveryStatus::unavailable;
        result.blocker = planned.blocker;
        return result;
    }
    result.cue = planned.cue;
    result.preflight = inspect_new_era_audio_asset(source_, *planned.cue,
        *bridge_);
    if (result.preflight->disposition != NewEraAssetDisposition::pcm_ready) {
        result.status = NewEraDeliveryStatus::asset_blocked;
        result.blocker = result.preflight->blocker;
        return result;
    }
    try {
        result.bridge_result = bridge_->transition(planned.cue->sequence,
            UiAudioKind::popup, planned.cue->id);
        const auto &bridge = *result.bridge_result;
        if (bridge.disposition != LiveUiAudioDisposition::processed ||
            bridge.emission != UiAudioEmitStatus::queued || !bridge.playback) {
            result.status = NewEraDeliveryStatus::bridge_blocked;
            result.blocker = "NewEra cue was not queued by the selected audio bridge";
        } else if (bridge.playback->status == UiPcmSubmitStatus::submitted) {
            result.status = NewEraDeliveryStatus::sink_submitted;
        } else if (bridge.playback->status == UiPcmSubmitStatus::sink_rejected) {
            result.status = NewEraDeliveryStatus::sink_rejected;
            result.blocker = "NewEra PCM sink rejected the owned buffer";
        } else {
            result.status = NewEraDeliveryStatus::bridge_blocked;
            result.blocker = bridge.playback->error.empty()
                ? "NewEra asset or PCM submission changed after preflight"
                : bridge.playback->error;
        }
    } catch (const std::exception &error) {
        result.status = NewEraDeliveryStatus::uncertain;
        result.blocker = error.what();
    } catch (...) {
        result.status = NewEraDeliveryStatus::uncertain;
        result.blocker = "NewEra audio bridge failed after transition began";
    }
    return result;
}

} // namespace civ5::ui
