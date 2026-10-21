#include "civ5/ui/new_era_audio_delivery.hpp"
#include "civ5/ui/bnw_new_era_sound_archive.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

struct Sink final : civ5::ui::UiPcmSink {
    bool accept = true;
    int calls = 0;
    std::vector<std::uint8_t> samples;
    bool submit(const civ5::ui::UiPcmBuffer &buffer) override {
        ++calls;
        samples = buffer.interleaved_samples;
        return accept;
    }
};
} // namespace

int main(int argc, char **argv) {
    try {
        using namespace civ5;
        using namespace civ5::ui;
        namespace fs = std::filesystem;
        require(argc == 2, "installed BNW game root required");
        const fs::path root(argv[1]);
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
        require(proof.verified, "selected BNW NewEra source verified");
        PopupEvent popup_event;
        popup_event.context_id = "NewEraPopup";
        popup_event.event_id = 91;
        const auto frame = UiFrameSnapshot(20, 7, 12)
            .with(std::make_shared<const PopupFrame>(
                PopupFrame{7, {popup_event}, 12, 5}));
        const NewEraPopupShownReceipt receipt{7, 91, 12, 5, 20, 700};

        auto blocked_sink = std::make_unique<Sink>();
        auto *blocked_calls = blocked_sink.get();
        LiveUiAudioBridge blocked_bridge(resources, std::move(blocked_sink));
        NewEraAudioDelivery blocked(proof, blocked_bridge);
        const auto missing = blocked.shown(frame, receipt);
        require(missing.status == NewEraDeliveryStatus::asset_blocked &&
            missing.preflight && missing.preflight->resolution &&
            missing.preflight->resolution->status == UiAudioAssetStatus::missing_asset &&
            blocked_calls->calls == 0 &&
            blocked_bridge.last_transition_sequence() == 0,
            "unmounted retail payload blocks before any bridge/device transition");
        require(blocked.shown(frame, receipt).status ==
                    NewEraDeliveryStatus::duplicate && blocked_calls->calls == 0,
            "blocked owner event is not replayed automatically");

        auto installed = resources;
        const auto mounted_sound = mount_bnw_new_era_sound_archive(
            installed, root, 30);
        require(mounted_sound.status == BnwNewEraSoundMountStatus::mounted,
            "installed New Era sound archive did not mount uniquely");
        require(mount_bnw_new_era_sound_archive(installed, root, 30).status ==
                    BnwNewEraSoundMountStatus::mount_id_in_use,
            "selected sound mount ID was silently reused");
        auto installed_sink = std::make_unique<Sink>();
        auto *installed_calls = installed_sink.get();
        LiveUiAudioBridge installed_bridge(installed, std::move(installed_sink));
        NewEraAudioDelivery installed_delivery(proof, installed_bridge);
        const auto installed_event = installed_delivery.shown(frame, receipt);
        if (installed_event.status != NewEraDeliveryStatus::sink_submitted ||
            !installed_event.preflight || !installed_event.preflight->resolution ||
            !installed_event.preflight->resolution->asset ||
            installed_calls->calls != 1 || installed_calls->samples.empty())
            throw std::runtime_error("installed BNW New Era WAV status=" +
                std::to_string(static_cast<int>(installed_event.status)) +
                " blocker=" + installed_event.blocker + " resolver=" +
                (installed_event.preflight && installed_event.preflight->resolution
                    ? installed_event.preflight->resolution->reason : "absent") +
                " header=" + [&] {
                    if (!installed_event.preflight ||
                        !installed_event.preflight->resolution ||
                        !installed_event.preflight->resolution->asset)
                        return std::string("absent");
                    auto stream = installed_event.preflight->resolution->asset->open_stream();
                    std::array<std::uint8_t,64> bytes{};
                    const auto count = stream.read(bytes);
                    std::string out;
                    constexpr char digits[] = "0123456789abcdef";
                    for (std::size_t i=0; i<count; ++i) {
                        const auto byte=bytes[i];
                        out += digits[byte >> 4];
                        out += digits[byte & 15];
                    }
                    return out;
                }());

        const auto fixture_root = fs::temp_directory_path() /
            "civis-new-era-audio-delivery";
        fs::create_directories(fixture_root);
        const auto wave_path = fixture_root / "NotificationNewEra.wav";
        constexpr char wave_bytes[] =
            "RIFF\x28\0\0\0WAVEfmt \x10\0\0\0\x01\0\x01\0"
            "\x40\x1f\0\0\x40\x1f\0\0\x01\0\x08\0"
            "data\x04\0\0\0\x00\x40\x80\xff";
        {
            std::ofstream out(wave_path, std::ios::binary);
            out.write(wave_bytes, sizeof(wave_bytes) - 1);
            require(static_cast<bool>(out), "synthetic PCM fixture written");
        }
        auto sound = std::make_shared<assets::LooseResourceIndex>(fixture_root);
        sound->add_file(wave_path);
        auto mounted = resources;
        mounted.mount({3, 0, 0, assets::resource_lookup(sound), true, true, {},
            assets::resource_keys(sound)});
        auto sink = std::make_unique<Sink>();
        auto *observed = sink.get();
        LiveUiAudioBridge bridge(mounted, std::move(sink));
        NewEraAudioDelivery delivery(proof, bridge);
        auto foreign = receipt;
        foreign.player_id = 8;
        require(delivery.shown(frame, foreign).status ==
                    NewEraDeliveryStatus::unavailable && observed->calls == 0,
            "foreign owner cannot reach PCM sink");
        const auto performed = delivery.shown(frame, receipt);
        require(performed.status == NewEraDeliveryStatus::sink_submitted &&
            performed.preflight &&
            performed.preflight->disposition == NewEraAssetDisposition::pcm_ready &&
            performed.bridge_result && performed.bridge_result->playback &&
            performed.bridge_result->playback->status == UiPcmSubmitStatus::submitted &&
            observed->calls == 1 && observed->samples.size() == 4 &&
            bridge.last_transition_sequence() == 700,
            "current owner sends exactly one selected PCM to injected sink");
        require(delivery.shown(frame, receipt).status ==
                    NewEraDeliveryStatus::duplicate && observed->calls == 1,
            "repeat PopupShown cannot submit another buffer");

        auto rejecting_sink = std::make_unique<Sink>();
        rejecting_sink->accept = false;
        auto *rejected_calls = rejecting_sink.get();
        LiveUiAudioBridge rejecting_bridge(mounted, std::move(rejecting_sink));
        NewEraAudioDelivery rejecting(proof, rejecting_bridge);
        const auto rejected = rejecting.shown(frame, receipt);
        require(rejected.status == NewEraDeliveryStatus::sink_rejected &&
            rejected_calls->calls == 1 &&
            rejecting.shown(frame, receipt).status ==
                NewEraDeliveryStatus::duplicate &&
            rejected_calls->calls == 1,
            "sink rejection is observable and one-shot");
        std::cout << "selected BNW NewEra owner audio delivery: PASS\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
