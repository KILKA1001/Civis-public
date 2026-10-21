#include "civ5/ui/new_era_popup_audio.hpp"
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
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
                 "Assets/Sounds/XML/NotificationSounds.xml"})
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
        require(proof.verified && proof.popup_audio_ordinal > 0 &&
            !proof.in_game_sha256.empty() && !proof.lua_sha256.empty() &&
            !proof.xml_sha256.empty() && !proof.popup_audio_sha256.empty() &&
            catalog.find(UiAudioKind::popup, "BUTTONPOPUP_NEW_ERA") &&
            catalog.find(UiAudioKind::popup,
                "BUTTONPOPUP_NEW_ERA")->script_id == "AS2D_INTERFACE_NEW_ERA",
            "selected New Era popup and authored audio row are verified");

        PopupEvent event;
        event.context_id = "NewEraPopup";
        event.event_id = 91;
        PopupFrame popup{7, {event}, 12, 5};
        const auto frame = UiFrameSnapshot(20, 7, 12)
            .with(std::make_shared<const PopupFrame>(popup));
        NewEraPopupShownReceipt receipt{7, 91, 12, 5, 20, 700};
        NewEraPopupAudioPlanner planner(proof);
        require(!planner.shown(UiFrameSnapshot(20, 7, 12), receipt).cue,
            "era state without actual owner popup event emits nothing");
        const auto first = planner.shown(frame, receipt);
        require(first.cue && first.cue->id == "BUTTONPOPUP_NEW_ERA" &&
            first.cue->sequence == 700 && !first.duplicate,
            "one PopupShown receipt yields one typed cue");
        const auto repeated = planner.shown(frame, receipt);
        require(!repeated.cue && repeated.duplicate,
            "repeat render or receipt cannot replay sound");
        auto stale = receipt;
        stale.frame = 21;
        require(!planner.shown(frame, stale).cue,
            "stale frame receipt cannot emit sound");
        auto foreign = receipt;
        foreign.player_id = 8;
        require(!planner.shown(frame, foreign).cue,
            "foreign player receipt cannot emit sound");
        foreign = receipt;
        foreign.owner_sequence = 6;
        require(!planner.shown(frame, foreign).cue,
            "foreign popup owner receipt cannot emit sound");
        auto delegated = event;
        delegated.audio_cue = UiAuthoredAudioCue{701, "BUTTONPOPUP_NEW_ERA"};
        const auto delegated_frame = UiFrameSnapshot(21, 7, 12)
            .with(std::make_shared<const PopupFrame>(
                PopupFrame{7, {delegated}, 12, 6}));
        const NewEraPopupShownReceipt second{7, 91, 12, 6, 21, 701};
        require(!planner.shown(delegated_frame, second).cue,
            "host-owned popup cue blocks duplicate planner delivery");
        UiAudioQueue queue(catalog);
        require(queue.emit(UiAudioKind::popup, first.cue->id) ==
                    UiAudioEmitStatus::queued &&
            queue.drain().front().script_id == "AS2D_INTERFACE_NEW_ERA",
            "typed cue resolves to selected authored AS2D script; no playback claimed");
        std::cout << "selected BNW New Era popup audio plan: PASS\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
