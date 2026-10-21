#include "civ5/ui/new_era_audio_asset.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

class NoPlaybackSink final : public civ5::ui::UiPcmSink {
  public:
    bool submit(const civ5::ui::UiPcmBuffer &) override {
        ++calls;
        return false;
    }
    std::size_t calls = 0;
};
} // namespace

int main(int argc, char **argv) {
    try {
        using namespace civ5;
        using namespace civ5::ui;
        require(argc == 2, "installed BNW game root required");
        const std::filesystem::path root(argv[1]);
        auto base = std::make_shared<assets::LooseResourceIndex>(root);
        auto bnw = std::make_shared<assets::LooseResourceIndex>(root);
        for (const auto *path : {
                 "Assets/Sounds/XML/ButtonPopupSounds.xml",
                 "Assets/Sounds/XML/NotificationSounds.xml",
                 "Assets/Sounds/XML/Audio2DScripts.xml",
                 "Assets/Sounds/XML/AudioDefines.xml"})
            base->add_file(root / path);
        for (const auto *path : {
                 "Assets/DLC/Expansion2/UI/InGame/InGame.xml",
                 "Assets/DLC/Expansion2/UI/InGame/Popups/NewEraPopup.lua",
                 "Assets/DLC/Expansion2/UI/InGame/Popups/NewEraPopup.xml"})
            bnw->add_file(root / path);
        assets::ResourceMountTable resources(assets::resource_lookup(base), {},
            assets::resource_keys(base));
        resources.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {},
            assets::resource_keys(bnw)});
        const auto catalog = UiAudioCatalog::from_resources(resources);
        const auto proof = inspect_new_era_audio_source(resources, catalog);
        require(proof.verified, "selected NewEra popup audio source verified");
        auto sink = std::make_unique<NoPlaybackSink>();
        auto *observed = sink.get();
        LiveUiAudioBridge bridge(std::move(resources), std::move(sink));
        PopupEvent event;
        event.context_id = "NewEraPopup";
        event.event_id = 91;
        const auto frame = UiFrameSnapshot(20, 7, 12)
            .with(std::make_shared<const PopupFrame>(
                PopupFrame{7, {event}, 12, 5}));
        NewEraPopupAudioPlanner planner(proof);
        const auto issued = planner.shown(frame,
            NewEraPopupShownReceipt{7, 91, 12, 5, 20, 700});
        require(issued.cue.has_value(),
            "owner PopupShown produced a typed cue");
        const auto preflight = inspect_new_era_audio_asset(
            proof, *issued.cue, bridge);
        require(preflight.resolution &&
            preflight.resolution->script_id == "AS2D_INTERFACE_NEW_ERA" &&
            preflight.resolution->sound_id == "SND_INTERFACE_NEW_ERA" &&
            preflight.resolution->filename == "NotificationNewEra" &&
            preflight.resolution->script_source &&
            preflight.resolution->definition_source &&
            preflight.resolution->status == UiAudioAssetStatus::missing_asset &&
            preflight.disposition == NewEraAssetDisposition::asset_blocked &&
            observed->calls == 0 && bridge.last_transition_sequence() == 0,
            "retail XML chain resolves, but unmounted sound payload blocks without playback");
        auto mismatched = proof;
        ++mismatched.popup_audio_ordinal;
        require(inspect_new_era_audio_asset(mismatched, *issued.cue, bridge)
                    .disposition == NewEraAssetDisposition::selected_catalog_mismatch,
            "wrong selected popup table ordinal blocks cue");
        auto foreign = *issued.cue;
        foreign.id = "BUTTONPOPUP_TECH_AWARD";
        require(inspect_new_era_audio_asset(proof, foreign, bridge)
                    .disposition == NewEraAssetDisposition::invalid_cue,
            "foreign popup ID cannot bind to NewEra resource");
        require(observed->calls == 0 && bridge.last_transition_sequence() == 0,
            "all inspections leave sink and transition state untouched");
        std::cout << "selected BNW NewEra cue resource preflight: PASS\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
