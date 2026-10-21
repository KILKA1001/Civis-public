#include "civ5/ui/selected_bnw_ui_audio.hpp"
#include "civ5/ui/live_ui_host.hpp"
#include "civ5/saves/save_header.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
namespace fs = std::filesystem;

void check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
void write(const fs::path &root, std::string_view name, std::string_view bytes) {
    const auto path = root / fs::path(name);
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out.write(bytes.data(), bytes.size());
    if (!out) throw std::runtime_error("Cannot write selected audio fixture");
}
struct Sink final : ui::UiPcmSink {
    std::size_t calls = 0;
    bool accept = true;
    bool submit(const ui::UiPcmBuffer &) override {
        ++calls;
        return accept;
    }
};
}

int main(int argc, char **argv) {
    try {
        check(argc == 1 || argc == 3,
              "Supply either no paths or selected Gandhi save and game root");
        const auto root = fs::temp_directory_path() / "civis-selected-bnw-ui-audio-test";
        write(root, "CivilizationV.exe", "selected fixture");
        write(root, "Assets/Sounds/XML/NotificationSounds.xml",
            "<NotificationSounds><NotificationSound><ID>NOTIFICATION_TEST</ID>"
            "<AudioScript>AS2D_TEST</AudioScript></NotificationSound></NotificationSounds>");
        write(root, "Assets/Sounds/XML/ButtonPopupSounds.xml",
            "<ButtonPopupSounds><ButtonPopupSound><ID>BUTTONPOPUP_TEST</ID>"
            "<AudioScript>AS2D_PLACEHOLDER_BLANK</AudioScript>"
            "</ButtonPopupSound></ButtonPopupSounds>");
        write(root, "Assets/Sounds/XML/Audio2DScripts.xml",
            "<Script2DFile><Script2DSounds><Script2DSound>"
            "<ScriptID>AS2D_TEST</ScriptID><SoundID>SND_TEST</SoundID>"
            "</Script2DSound></Script2DSounds></Script2DFile>");
        write(root, "Assets/Sounds/XML/AudioDefines.xml",
            "<AudioDefinesFile><SoundDatas><SoundData><SoundID>SND_TEST</SoundID>"
            "<Filename>TestPcm</Filename></SoundData></SoundDatas></AudioDefinesFile>");
        constexpr char wave[] =
            "RIFF\x28\0\0\0WAVEfmt \x10\0\0\0\x01\0\x01\0"
            "\x40\x1f\0\0\x40\x1f\0\0\x01\0\x08\0"
            "data\x04\0\0\0\x00\x40\x80\xff";
        write(root, "Assets/Sounds/TestPcm.wav",
            std::string_view(wave, sizeof(wave) - 1));
        write(root, "Assets/DLC/Expansion2/Expansion2.Civ5Pkg",
            "<Civ5Package><Gameplay>"
            "<AudioScripts2D>Audio2DScriptsExpansion2.xml</AudioScripts2D>"
            "<AudioDefines>AudioDefinesExpansion2.xml</AudioDefines>"
            "</Gameplay></Civ5Package>");
        write(root, "Assets/DLC/Expansion2/Sounds/XML/Audio2DScriptsExpansion2.xml",
            "<Script2DFile><Script2DSounds><Script2DSound>"
            "<ScriptID>AS2D_BNW_TEST</ScriptID><SoundID>SND_BNW_TEST</SoundID>"
            "</Script2DSound></Script2DSounds></Script2DFile>");
        write(root, "Assets/DLC/Expansion2/Sounds/XML/AudioDefinesExpansion2.xml",
            "<AudioDefinesFile><SoundDatas><SoundData><SoundID>SND_BNW_TEST</SoundID>"
            "<Filename>BnwPcm</Filename></SoundData></SoundDatas></AudioDefinesFile>");
        write(root, "Assets/DLC/Expansion2/Sounds/BnwPcm.wav",
            std::string_view(wave, sizeof(wave) - 1));
        content::Catalog catalog;
        content::PackageManifest expansion{};
        expansion.kind = content::PackageKind::dlc;
        expansion.id = "6da07636-4123-4018-b643-6575b4ec336b";
        expansion.path = root / "Assets/DLC/Expansion2/Expansion2.Civ5Pkg";
        expansion.document = content::read_xml_metadata(
            "<Civ5Package><Gameplay>"
            "<AudioScripts2D>Audio2DScriptsExpansion2.xml</AudioScripts2D>"
            "<AudioDefines>AudioDefinesExpansion2.xml</AudioDefines>"
            "</Gameplay></Civ5Package>");
        catalog.packages.push_back(std::move(expansion));
        runtime::SavedContentProfile profile;
        profile.can_activate = true;
        profile.enabled_dlc = {0};
        auto incomplete = profile;
        incomplete.enabled_dlc.clear();
        bool rejected_profile = false;
        try {
            (void)ui::make_selected_bnw_ui_audio_bridge(root, incomplete, catalog,
                std::make_unique<Sink>());
        } catch (const std::invalid_argument &) { rejected_profile = true; }
        check(rejected_profile, "Audio factory admitted a profile without selected BNW DLC");
        auto sink = std::make_unique<Sink>();
        auto *observed = sink.get();
        auto bridge = ui::make_selected_bnw_ui_audio_bridge(root, profile, catalog,
            std::move(sink));
        check(bridge->inspect_script("AS2D_BNW_TEST").status ==
                  ui::UiAudioAssetStatus::pcm_ready,
              "Selected BNW manifest and local sound were not mounted together");
        const auto registry = ui::UiContextRegistry::from_documents(
            content::read_xml_metadata(
                "<Context><LuaContext ID='WorldView' FileName='WorldView'/></Context>"),
            content::read_xml_metadata("<Context/>"));
        ui::WorldViewState world;
        world.player_id = 0;
        world.notifications = {{1, "typed owner event", {}, false,
            ui::UiAuthoredAudioCue{1, "NOTIFICATION_TEST"}}};
        std::vector<ui::LiveUiAudioResult> outcomes;
        ui::LiveUiSources sources;
        sources.audio_bridge = bridge.get();
        sources.audio_observer = [&](ui::UiAudioKind,
            const ui::UiAuthoredAudioCue &, const ui::LiveUiAudioResult &result) {
            outcomes.push_back(result);
        };
        std::uint64_t frames = 0;
        ui::LiveUiHost host(registry, [&] {
            return ui::UiFrameSnapshot(++frames, 0, 0).with<ui::WorldViewState>(
                std::make_shared<const ui::WorldViewState>(world));
        }, nullptr, sources);
        (void)host.begin_frame(1280, 960);
        check(observed->calls == 1 && outcomes.size() == 1 &&
              outcomes[0].playback && outcomes[0].playback->status ==
                  ui::UiPcmSubmitStatus::submitted,
              "One owner-supplied notification must submit one local PCM");
        (void)host.begin_frame(1280, 960);
        check(observed->calls == 1 && outcomes.back().disposition ==
                  ui::LiveUiAudioDisposition::duplicate_or_stale,
              "Repeating a UI frame cannot submit the same audio event twice");
        const auto bnw = bridge->script_transition(2,
            "selected BNW test owner", "AS2D_BNW_TEST");
        check(bnw.playback && bnw.playback->status ==
                  ui::UiPcmSubmitStatus::submitted && observed->calls == 2,
              "Selected BNW authored script must use its local PCM payload");
        const auto blocked = bridge->script_transition(3,
            "selected BNW test owner", "AS2D_NOT_AUTHORED");
        check(blocked.playback && blocked.playback->status ==
                  ui::UiPcmSubmitStatus::resolution_blocked && observed->calls == 2,
              "Absent authored script must have an observable blocked result");
        observed->accept = false;
        const auto rejected = bridge->script_transition(4,
            "selected BNW test owner", "AS2D_BNW_TEST");
        check(rejected.playback && rejected.playback->status ==
                  ui::UiPcmSubmitStatus::sink_rejected && observed->calls == 3,
              "Audio device refusal must not report playback success");
        if (argc == 3) {
            const auto header = saves::read_header(argv[1]);
            const auto retail = content::discover_packages({fs::path(argv[2]) / "Assets/DLC"});
            const auto selected = runtime::resolve_saved_content_profile(header, retail);
            auto retail_sink = std::make_unique<Sink>();
            auto *retail_observed = retail_sink.get();
            auto retail_bridge = ui::make_selected_bnw_ui_audio_bridge(
                argv[2], selected, retail, std::move(retail_sink));
            const auto caravan = retail_bridge->inspect_script(
                "AS2D_INTERFACE_CARAVAN_TRADE_ROUTE");
            const auto submitted = retail_bridge->script_transition(1,
                "local selected-content probe", "AS2D_INTERFACE_CARAVAN_TRADE_ROUTE");
            check(caravan.status == ui::UiAudioAssetStatus::pcm_ready &&
                  caravan.script_source && caravan.script_source->provenance &&
                  submitted.playback && submitted.playback->status ==
                      ui::UiPcmSubmitStatus::submitted && retail_observed->calls == 1,
                  "Selected Gandhi audio profile cannot resolve local BNW Caravan PCM");
            const auto new_era = retail_bridge->inspect_script(
                "AS2D_INTERFACE_NEW_ERA");
            const auto era_submission = retail_bridge->script_transition(2,
                "selected BNW New Era owner", "AS2D_INTERFACE_NEW_ERA");
            check(new_era.status == ui::UiAudioAssetStatus::pcm_ready &&
                  new_era.pcm && new_era.pcm->channels == 2 &&
                  era_submission.playback && era_submission.playback->status ==
                      ui::UiPcmSubmitStatus::submitted && retail_observed->calls == 2,
                  "Selected Gandhi audio bridge cannot submit installed New Era PCM");
        }
        std::cout << "selected BNW UI audio PASS\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL selected BNW UI audio: " << error.what() << '\n';
        return 1;
    }
}
