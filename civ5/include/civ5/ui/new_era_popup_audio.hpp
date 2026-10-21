#pragma once

#include "civ5/ui/popup_screens.hpp"
#include "civ5/ui/ui_audio_events.hpp"
#include <set>
#include <tuple>

namespace civ5::ui {

struct NewEraAudioSourceProof {
    bool verified = false;
    std::string in_game_sha256, lua_sha256, xml_sha256;
    std::string popup_audio_sha256;
    std::string lua_selected_name, xml_selected_name;
    std::string popup_audio_selected_name;
    std::optional<std::uint64_t> popup_audio_mount_id;
    std::size_t popup_audio_ordinal = 0;
    std::string blocker;
};

// Proves the selected Expansion2 context registration, NewEraPopup event path
// and native ButtonPopupSounds row. No event is inferred from those files.
NewEraAudioSourceProof inspect_new_era_audio_source(
    const assets::ResourceMountTable &resources, const UiAudioCatalog &catalog);

// The live popup owner issues this only after PopupShown in ShowHideHandler. A
// visible frame, selected XML or era counter cannot stand in for this receipt.
struct NewEraPopupShownReceipt {
    std::int64_t player_id = -1, event_id = -1;
    std::int32_t turn = -1;
    std::uint64_t owner_sequence = 0, frame = 0, cue_sequence = 0;
};

struct NewEraAudioPlanResult {
    std::optional<UiAuthoredAudioCue> cue;
    bool duplicate = false;
    std::string blocker;
};

// Produces one owner-scoped authored cue for the existing client audio bridge.
// If PopupEvent.audio_cue is already set, the host owns delivery and this
// planner refuses a second route. It does not perform sound or create a popup.
class NewEraPopupAudioPlanner {
  public:
    explicit NewEraPopupAudioPlanner(NewEraAudioSourceProof source)
        : source_(std::move(source)) {}
    NewEraAudioPlanResult shown(const UiFrameSnapshot &frame,
        const NewEraPopupShownReceipt &receipt);

  private:
    NewEraAudioSourceProof source_;
    std::set<std::tuple<std::int64_t, std::int32_t, std::int64_t,
                        std::uint64_t, std::uint64_t>> delivered_;
};

} // namespace civ5::ui
