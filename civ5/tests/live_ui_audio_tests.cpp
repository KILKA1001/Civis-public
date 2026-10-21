#include "civ5/ui/live_ui_audio.hpp"
#include "civ5/ui/live_ui_host.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

namespace {
using namespace civ5;
namespace fs = std::filesystem;

void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void write(const fs::path &root, std::string_view name, std::string_view text) {
    const auto path = root / fs::path(name);
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out.write(text.data(), text.size());
    if (!out) throw std::runtime_error("Could not write UI audio fixture");
}

struct Sink final : ui::UiPcmSink {
    int calls = 0;
    bool accept = true;
    std::vector<std::uint8_t> last;
    ui::UiPcmFormat format;
    bool submit(const ui::UiPcmBuffer &buffer) override {
        ++calls;
        last = buffer.interleaved_samples;
        format = buffer.format;
        return accept;
    }
};

struct FailedDevice final : ui::IWaveOutDevice {
    bool start(ui::UiPcmBuffer, std::string &error) override {
        error = "Simulated waveOutOpen failure";
        return false;
    }
    bool wait_for_progress(std::chrono::milliseconds, std::string &) override {
        return false;
    }
    bool done() const override { return false; }
    bool release(bool, std::string &) override { return true; }
};

void host_transitions(assets::ResourceMountTable mounts) {
    auto sink = std::make_unique<Sink>();
    auto *observed = sink.get();
    ui::LiveUiAudioBridge bridge(std::move(mounts), std::move(sink));
    const auto registry = ui::UiContextRegistry::from_documents(
        content::read_xml_metadata(R"(<Context>
          <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
          <LuaContext FileName="Assets/UI/InGame/Popups/GenericPopup" ID="GenericPopup" Hidden="True"/>
        </Context>)"), content::read_xml_metadata("<Context/>"));
    ui::PopupEvent popup;
    popup.context_id = "GenericPopup";
    popup.event_id = 42;
    popup.audio_cue = ui::UiAuthoredAudioCue{1, "BUTTONPOPUP_TEST"};
    ui::WorldViewState world;
    world.player_id = 0;
    std::vector<ui::LiveUiAudioResult> outcomes;
    ui::LiveUiSources sources;
    sources.audio_bridge = &bridge;
    sources.audio_observer = [&](ui::UiAudioKind, const ui::UiAuthoredAudioCue &,
                                 const ui::LiveUiAudioResult &result) {
        outcomes.push_back(result);
    };
    std::uint64_t frame_number = 0;
    ui::LiveUiHost host(registry, [&] {
        return ui::UiFrameSnapshot(++frame_number, 0, 12)
            .with<ui::PopupFrame>(std::make_shared<const ui::PopupFrame>(
                ui::PopupFrame{0, {popup}}))
            .with<ui::WorldViewState>(std::make_shared<const ui::WorldViewState>(world));
    }, nullptr, sources);

    auto frame = host.begin_frame(1280, 960);
    require(observed->calls == 0, "Frame construction does not invent popup sound");
    require(host.dispatch(ui::UiOpenScreen{"GenericPopup"}, frame).status ==
                ui::UiCommandStatus::navigation &&
                observed->calls == 1 && outcomes.back().playback &&
                outcomes.back().playback->status == ui::UiPcmSubmitStatus::submitted,
            "Focused typed popup open submits its authored PCM once");
    require(host.dispatch(ui::UiOpenScreen{"GenericPopup"}, frame).status ==
                ui::UiCommandStatus::invalid && observed->calls == 1,
            "Stale popup command does not replay sound");
    frame = host.begin_frame(1280, 960);
    require(observed->calls == 1 && outcomes.size() == 1,
            "Repeated popup render does not emit a transition");
    require(host.dispatch(ui::UiOpenScreen{"GenericPopup"}, frame).status ==
                ui::UiCommandStatus::navigation && observed->calls == 1,
            "Opening already focused popup does not emit a transition");
    frame = host.begin_frame(1280, 960);
    require(host.dispatch(ui::UiCloseScreen{"GenericPopup"}, frame).status ==
                ui::UiCommandStatus::navigation, "Popup closes before reopening");
    frame = host.begin_frame(1280, 960);
    require(host.dispatch(ui::UiOpenScreen{"GenericPopup"}, frame).status ==
                ui::UiCommandStatus::navigation && observed->calls == 1 &&
                outcomes.back().disposition == ui::LiveUiAudioDisposition::duplicate_or_stale,
            "Reopened same owner event is deduplicated by bridge sequence");

    world.notifications = {{9, "New event", {}, false,
                            ui::UiAuthoredAudioCue{2, "NOTIFICATION_PCM"}}};
    frame = host.begin_frame(1280, 960);
    require(observed->calls == 2 && outcomes.back().playback &&
                outcomes.back().playback->status == ui::UiPcmSubmitStatus::submitted,
            "Owner-sequenced notification emits one authored PCM");
    frame = host.begin_frame(1280, 960);
    require(observed->calls == 2 &&
                outcomes.back().disposition == ui::LiveUiAudioDisposition::duplicate_or_stale,
            "Repeated notification frame is deduplicated");
    world.notifications = {{10, "Missing asset", {}, false,
                            ui::UiAuthoredAudioCue{3, "NOTIFICATION_TEST"}}};
    frame = host.begin_frame(1280, 960);
    require(observed->calls == 2 && outcomes.back().playback &&
                outcomes.back().playback->status ==
                    ui::UiPcmSubmitStatus::resolution_blocked,
            "Missing local notification asset has observable blocked status");
    observed->accept = false;
    world.notifications = {{11, "Device failure", {}, false,
                            ui::UiAuthoredAudioCue{4, "NOTIFICATION_PCM"}}};
    frame = host.begin_frame(1280, 960);
    require(observed->calls == 3 && outcomes.back().playback &&
                outcomes.back().playback->status == ui::UiPcmSubmitStatus::sink_rejected,
            "Audio sink refusal is observable without fake playback success");
    world.player_id = 1;
    world.notifications = {{12, "Another player", {}, false,
                            ui::UiAuthoredAudioCue{5, "NOTIFICATION_PCM"}}};
    frame = host.begin_frame(1280, 960);
    require(observed->calls == 3 && bridge.last_transition_sequence() == 4,
            "Another player's notification does not reach audio bridge");
}

} // namespace

int main(int argc, char **argv) {
    try {
        const auto root = fs::temp_directory_path() / "civis-live-ui-audio-test";
        write(root, "Assets/Sounds/XML/ButtonPopupSounds.xml",
              "<ButtonPopupSounds>"
              "<ButtonPopupSound><ID>BUTTONPOPUP_TEST</ID><AudioScript>AS2D_TEST_PCM</AudioScript></ButtonPopupSound>"
              "<ButtonPopupSound><ID>BUTTONPOPUP_SILENT</ID><AudioScript>AS2D_PLACEHOLDER_BLANK</AudioScript></ButtonPopupSound>"
              "</ButtonPopupSounds>");
        write(root, "Assets/Sounds/XML/NotificationSounds.xml",
              "<NotificationSounds><NotificationSound><ID>NOTIFICATION_TEST</ID>"
              "<AudioScript>AS2D_TEST_MISSING</AudioScript></NotificationSound>"
              "<NotificationSound><ID>NOTIFICATION_PCM</ID>"
              "<AudioScript>AS2D_TEST_PCM</AudioScript></NotificationSound>"
              "</NotificationSounds>");
        write(root, "Assets/Sounds/XML/Audio2DScripts.xml",
              "<Script2DFile><Script2DSounds>"
              "<Script2DSound><ScriptID>AS2D_TEST_PCM</ScriptID><SoundID>SND_TEST_PCM</SoundID></Script2DSound>"
              "<Script2DSound><ScriptID>AS2D_TEST_MISSING</ScriptID><SoundID>SND_TEST_MISSING</SoundID></Script2DSound>"
              "</Script2DSounds></Script2DFile>");
        write(root, "Assets/Sounds/XML/AudioDefines.xml",
              "<AudioDefinesFile><SoundDatas>"
              "<SoundData><SoundID>SND_TEST_PCM</SoundID><Filename>TestPcm</Filename></SoundData>"
              "<SoundData><SoundID>SND_TEST_MISSING</SoundID><Filename>Absent</Filename></SoundData>"
              "</SoundDatas></AudioDefinesFile>");
        constexpr char wave_bytes[] =
            "RIFF\x28\0\0\0WAVEfmt \x10\0\0\0\x01\0\x01\0"
            "\x40\x1f\0\0\x40\x1f\0\0\x01\0\x08\0"
            "data\x04\0\0\0\x00\x40\x80\xff";
        const std::string wave(wave_bytes, sizeof(wave_bytes) - 1);
        write(root, "Assets/Sounds/TestPcm.wav", wave);
        auto index = std::make_shared<assets::LooseResourceIndex>(root);
        for (const auto name : {
                 "Assets/Sounds/XML/ButtonPopupSounds.xml",
                 "Assets/Sounds/XML/NotificationSounds.xml",
                 "Assets/Sounds/XML/Audio2DScripts.xml",
                 "Assets/Sounds/XML/AudioDefines.xml",
                 "Assets/Sounds/TestPcm.wav"}) index->add_file(name);
        assets::ResourceMountTable mounts(assets::resource_lookup(index));
        auto host_mounts = mounts;
        auto async_mounts = mounts;
        auto sink = std::make_unique<Sink>();
        auto *observed = sink.get();
        ui::LiveUiAudioBridge bridge(std::move(mounts), std::move(sink));

        require(observed->calls == 0 && !bridge.device_status(),
                "Bridge construction does not synthesize an audio transition");
        const auto first = bridge.transition(1, ui::UiAudioKind::popup,
                                             "BUTTONPOPUP_TEST");
        require(first.disposition == ui::LiveUiAudioDisposition::processed &&
                    first.emission == ui::UiAudioEmitStatus::queued &&
                    first.playback &&
                    first.playback->status == ui::UiPcmSubmitStatus::submitted &&
                    first.playback->event.sequence == 1 &&
                    first.playback->event.script_id == "AS2D_TEST_PCM" &&
                    observed->calls == 1 &&
                    observed->last == std::vector<std::uint8_t>({0,64,128,255}),
                "Authored popup ID yields one authored PCM submission");
        require(bridge.transition(1, ui::UiAudioKind::popup, "BUTTONPOPUP_TEST")
                    .disposition == ui::LiveUiAudioDisposition::duplicate_or_stale &&
                    observed->calls == 1,
                "Repeated frame's same transition sequence cannot replay sound");
        require(bridge.transition(2, ui::UiAudioKind::popup, "UNAUTHORED_POPUP")
                    .emission == ui::UiAudioEmitStatus::unknown && observed->calls == 1,
                "Unknown popup ID cannot invent a script");
        require(bridge.transition(3, ui::UiAudioKind::popup, "BUTTONPOPUP_SILENT")
                    .emission == ui::UiAudioEmitStatus::silent && observed->calls == 1,
                "Authored blank cue remains silent");
        const auto blocked = bridge.transition(4, ui::UiAudioKind::notification,
                                                "NOTIFICATION_TEST");
        require(blocked.playback &&
                    blocked.playback->status == ui::UiPcmSubmitStatus::resolution_blocked &&
                    observed->calls == 1,
                "Missing notification asset is observable without fake success");
        observed->accept = false;
        const auto rejected = bridge.script_transition(5, "LuaContext", "AS2D_TEST_PCM");
        require(rejected.playback &&
                    rejected.playback->status == ui::UiPcmSubmitStatus::sink_rejected &&
                    observed->calls == 2,
                "A device refusal is reported for an authored direct script");
        require(bridge.transition(4, ui::UiAudioKind::notification, "NOTIFICATION_TEST")
                    .disposition == ui::LiveUiAudioDisposition::duplicate_or_stale &&
                    bridge.last_transition_sequence() == 5 && observed->calls == 2,
                "Stale event owner sequence cannot requeue playback");
        auto winmm = std::make_unique<ui::WinMmPcmSink>(1,
            [] { return std::make_unique<FailedDevice>(); });
        ui::LiveUiAudioBridge asynchronous(std::move(async_mounts), std::move(winmm));
        const auto queued = asynchronous.transition(1, ui::UiAudioKind::popup,
                                                    "BUTTONPOPUP_TEST");
        require(queued.playback &&
                    queued.playback->status == ui::UiPcmSubmitStatus::submitted,
                "WinMM bridge reports enqueue separately from device completion");
        for (int i = 0; i < 200; ++i) {
            const auto status = asynchronous.device_status();
            if (status && status->failed == 1) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        const auto device = asynchronous.device_status();
        require(device && device->accepted == 1 && device->failed == 1 &&
                    device->completed == 0 &&
                    device->last_error == "Simulated waveOutOpen failure",
                "Async device failure remains observable after accepted enqueue");
        host_transitions(std::move(host_mounts));

        write(root, "Assets/DLC/Selected/Sounds/XML/ButtonPopupSounds.xml",
              "<ButtonPopupSounds><ButtonPopupSound><ID>BUTTONPOPUP_TEST</ID>"
              "<AudioScript>AS2D_TEST_MISSING</AudioScript>"
              "</ButtonPopupSound></ButtonPopupSounds>");
        auto selected_index = std::make_shared<assets::LooseResourceIndex>(root);
        selected_index->add_file("Assets/DLC/Selected/Sounds/XML/ButtonPopupSounds.xml");
        assets::ResourceMountTable selected_resources(assets::resource_lookup(index));
        selected_resources.mount({7, 0, 0, assets::resource_lookup(selected_index),
                                  true, true, assets::resource_lookup_all(selected_index),
                                  assets::resource_keys(selected_index)});
        content::Catalog selected_catalog;
        content::PackageManifest selected_package{};
        selected_package.kind = content::PackageKind::dlc;
        selected_package.document = content::read_xml_metadata(
            "<Civ5Package><Gameplay/></Civ5Package>");
        selected_catalog.packages.push_back(std::move(selected_package));
        const ui::UiSelectedDlcAudio selected_dlc{0, {true,true,false}};
        auto selected_sink = std::make_unique<Sink>();
        auto *selected_observed = selected_sink.get();
        ui::LiveUiAudioBridge selected_bridge(std::move(selected_resources),
            selected_catalog, std::span(&selected_dlc, 1), std::move(selected_sink));
        const auto *selected_popup = selected_bridge.audio_catalog().find(
            ui::UiAudioKind::popup, "BUTTONPOPUP_TEST");
        const auto selected_popup_result = selected_bridge.transition(1,
            ui::UiAudioKind::popup, "BUTTONPOPUP_TEST");
        require(selected_popup && selected_popup->script_id == "AS2D_TEST_MISSING" &&
                    selected_bridge.audio_catalog().popup_source()->mount_id == 7 &&
                    selected_popup_result.playback &&
                    selected_popup_result.playback->status ==
                        ui::UiPcmSubmitStatus::resolution_blocked &&
                    selected_observed->calls == 0,
                "Selected VFS popup table and script resolver share one mounted profile");

        if (argc == 2) {
            const fs::path game_root(argv[1]);
            auto base_audio = std::make_shared<assets::LooseResourceIndex>(game_root);
            for (const auto name : {
                     "Assets/Sounds/XML/ButtonPopupSounds.xml",
                     "Assets/Sounds/XML/NotificationSounds.xml",
                     "Assets/Sounds/XML/Audio2DScripts.xml",
                     "Assets/Sounds/XML/AudioDefines.xml"}) base_audio->add_file(name);
            assets::ResourceMountTable base_profile(assets::resource_lookup(base_audio),
                assets::resource_lookup_all(base_audio), assets::resource_keys(base_audio));
            auto base_sink = std::make_unique<Sink>();
            auto *base_observed = base_sink.get();
            ui::LiveUiAudioBridge base_bridge(base_profile, std::move(base_sink));
            constexpr auto caravan_id = "AS2D_INTERFACE_CARAVAN_TRADE_ROUTE";
            require(base_bridge.inspect_script(caravan_id).status ==
                        ui::UiAudioAssetStatus::missing_script &&
                        base_bridge.script_transition(1, "Lua", caravan_id).playback->status ==
                            ui::UiPcmSubmitStatus::resolution_blocked &&
                        base_observed->calls == 0,
                    "Base-only audio profile cannot resolve BNW Caravan cue");

            auto bnw_audio = std::make_shared<assets::LooseResourceIndex>(game_root);
            bnw_audio->scan({"Assets/DLC/Expansion2/Sounds/XML", {}, false});
            bnw_audio->scan({"Assets/DLC/Expansion2/Sounds/Interface", {}, false});
            base_profile.mount({200, 0, 0, assets::resource_lookup(bnw_audio),
                                true, true, assets::resource_lookup_all(bnw_audio),
                                assets::resource_keys(bnw_audio)});
            content::Catalog bnw_catalog;
            bnw_catalog.packages.push_back(content::read_manifest(
                game_root / "Assets/DLC/Expansion2/Expansion2.Civ5Pkg"));
            const ui::UiSelectedDlcAudio bnw{0, {true,true,false}};
            auto bnw_sink = std::make_unique<Sink>();
            auto *bnw_observed = bnw_sink.get();
            ui::LiveUiAudioBridge bnw_bridge(std::move(base_profile), bnw_catalog,
                std::span(&bnw, 1), std::move(bnw_sink));
            const auto caravan = bnw_bridge.inspect_script(caravan_id);
            const auto submitted = bnw_bridge.script_transition(1, "Lua", caravan_id);
            require(caravan.status == ui::UiAudioAssetStatus::pcm_ready &&
                        caravan.script_source && caravan.script_source->provenance &&
                        caravan.script_source->provenance->source_relative_path ==
                            "Assets/DLC/Expansion2/Sounds/XML/Audio2DScriptsExpansion2.xml" &&
                        submitted.playback &&
                        submitted.playback->status == ui::UiPcmSubmitStatus::submitted &&
                        bnw_observed->calls == 1 &&
                        bnw_observed->format.sample_rate == 22050 &&
                        bnw_observed->format.bits_per_sample == 16 &&
                        !bnw_observed->last.empty(),
                    "Selected BNW profile resolves and submits real Caravan PCM");
            const auto main_menu = bnw_bridge.inspect_script("AS2D_MUSIC_MAIN_MENU");
            const auto blocked_menu = bnw_bridge.script_transition(2, "Menu",
                "AS2D_MUSIC_MAIN_MENU");
            require(main_menu.status == ui::UiAudioAssetStatus::ambiguous_script &&
                        main_menu.conflicting_sources.size() >= 2 &&
                        blocked_menu.playback &&
                        blocked_menu.playback->status ==
                            ui::UiPcmSubmitStatus::resolution_blocked &&
                        bnw_observed->calls == 1,
                    "Base/BNW main menu override is blocked pending native precedence");

            const auto *notification = bnw_bridge.audio_catalog().find(
                ui::UiAudioKind::notification, "NOTIFICATION_GENERIC");
            require(notification && notification->script_id ==
                        "AS2D_EVENT_NOTIFICATION_NEUTRAL",
                    "Selected retail notification ID retains its authored AS2D mapping");
            ui::WorldViewState live_world;
            live_world.player_id = 0;
            live_world.notifications = {{77, "Retail notification", {}, false,
                ui::UiAuthoredAudioCue{3, "NOTIFICATION_GENERIC"}}};
            std::vector<ui::LiveUiAudioResult> live_results;
            ui::LiveUiSources live_sources;
            live_sources.audio_bridge = &bnw_bridge;
            live_sources.audio_observer = [&](ui::UiAudioKind kind,
                const ui::UiAuthoredAudioCue &cue, const ui::LiveUiAudioResult &result) {
                require(kind == ui::UiAudioKind::notification &&
                            cue.id == "NOTIFICATION_GENERIC",
                        "Retail host forwards the typed notification cue");
                live_results.push_back(result);
            };
            auto live_registry = ui::UiContextRegistry::from_documents(
                content::read_xml_metadata(R"(<Context>
                  <LuaContext FileName="Assets/UI/InGame/WorldView/WorldView" ID="WorldView"/>
                </Context>)"), content::read_xml_metadata("<Context/>"));
            ui::LiveUiHost live_host(std::move(live_registry), [&] {
                return ui::UiFrameSnapshot(1, 0, 12).with<ui::WorldViewState>(
                    std::make_shared<const ui::WorldViewState>(live_world));
            }, nullptr, live_sources);
            (void)live_host.begin_frame(1280, 960);
            require(live_results.size() == 1 && live_results.front().playback &&
                        live_results.front().emission == ui::UiAudioEmitStatus::queued &&
                        live_results.front().playback->status ==
                            ui::UiPcmSubmitStatus::resolution_blocked &&
                        bnw_observed->calls == 1,
                    "Selected retail cue reaches resolver and reports missing local asset");
            (void)live_host.begin_frame(1280, 960);
            require(live_results.back().disposition ==
                        ui::LiveUiAudioDisposition::duplicate_or_stale &&
                        bnw_observed->calls == 1,
                    "Selected retail notification does not replay on render");
        }
        std::error_code ignored;
        fs::remove_all(root, ignored);
        std::cout << "Live UI audio bridge tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
