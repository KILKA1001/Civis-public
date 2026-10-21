#include "civ5/ui/ui_audio_assets.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

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
    if (!out) throw std::runtime_error("Could not write selected audio fixture");
}

content::PackageManifest package(std::string_view xml, std::string id) {
    content::PackageManifest result{};
    result.kind = content::PackageKind::dlc;
    result.id = std::move(id);
    result.document = content::read_xml_metadata(std::string(xml));
    return result;
}

} // namespace

int main(int argc, char **argv) {
    try {
        const auto root = fs::temp_directory_path() / "civis-selected-ui-audio-test";
        const std::string base_scripts =
            "<Script2DFile><Script2DSounds><Script2DSound>"
            "<ScriptID>AS2D_BASE</ScriptID><SoundID>SND_BASE</SoundID>"
            "</Script2DSound></Script2DSounds></Script2DFile>";
        const std::string base_defines =
            "<AudioDefinesFile><SoundDatas><SoundData>"
            "<SoundID>SND_BASE</SoundID><Filename>BaseSound</Filename>"
            "</SoundData></SoundDatas></AudioDefinesFile>";
        const std::string override_scripts =
            "<Script2DFile><Script2DSounds><Script2DSound>"
            "<ScriptID>AS2D_OVERRIDE</ScriptID><SoundID>SND_OVERRIDE</SoundID>"
            "</Script2DSound></Script2DSounds></Script2DFile>";
        const std::string override_defines =
            "<AudioDefinesFile><SoundDatas><SoundData>"
            "<SoundID>SND_OVERRIDE</SoundID><Filename>OverrideSound</Filename>"
            "</SoundData></SoundDatas></AudioDefinesFile>";
        write(root, "Assets/Sounds/XML/Audio2DScripts.xml", base_scripts);
        write(root, "Assets/Sounds/XML/AudioDefines.xml", base_defines);
        write(root, "Assets/DLC/Selected/Sounds/XML/Audio2DScripts.xml", override_scripts);
        write(root, "Assets/DLC/Selected/Sounds/XML/AudioDefines.xml", override_defines);
        const char wave_bytes[] =
            "RIFF\x28\0\0\0WAVEfmt \x10\0\0\0\x01\0\x01\0"
            "\x40\x1f\0\0\x40\x1f\0\0\x01\0\x08\0"
            "data\x04\0\0\0\x00\x40\x80\xff";
        write(root, "Assets/DLC/Selected/Sounds/OverrideSound.wav",
              std::string_view(wave_bytes, sizeof(wave_bytes) - 1));
        auto base = std::make_shared<assets::LooseResourceIndex>(root);
        base->add_file("Assets/Sounds/XML/Audio2DScripts.xml");
        base->add_file("Assets/Sounds/XML/AudioDefines.xml");
        auto override = std::make_shared<assets::LooseResourceIndex>(root);
        override->add_file("Assets/DLC/Selected/Sounds/XML/Audio2DScripts.xml");
        override->add_file("Assets/DLC/Selected/Sounds/XML/AudioDefines.xml");
        override->add_file("Assets/DLC/Selected/Sounds/OverrideSound.wav");
        assets::ResourceMountTable resources(assets::resource_lookup(base),
            assets::resource_lookup_all(base), assets::resource_keys(base));
        resources.mount({7, 0, 0, assets::resource_lookup(override), true, true,
                         assets::resource_lookup_all(override), assets::resource_keys(override)});
        content::Catalog catalog;
        catalog.packages.push_back(package(
            "<Civ5Package><Gameplay><AudioScripts2D>Audio2DScripts.xml</AudioScripts2D>"
            "<AudioDefines>AudioDefines.xml</AudioDefines></Gameplay></Civ5Package>",
            "selected"));
        const ui::UiSelectedDlcAudio selected{0, {true, true, false}};
        const auto chosen = ui::UiAudioAssetResolver::from_selected_dlc(
            resources, catalog, std::span(&selected, 1));
        const auto resolved = chosen.resolve("AS2D_OVERRIDE");
        require(resolved.status == ui::UiAudioAssetStatus::pcm_ready &&
                    resolved.script_source && resolved.definition_source &&
                    resolved.script_source->mount_id == 7 &&
                    resolved.definition_source->mount_id == 7 &&
                    resolved.script_source->provenance &&
                    resolved.script_source->provenance->source_relative_path ==
                        "Assets/DLC/Selected/Sounds/XML/Audio2DScripts.xml" &&
                    chosen.resolve("AS2D_BASE").status ==
                        ui::UiAudioAssetStatus::missing_script,
                "Selected VFS file overrides base XML once with provenance");

        write(root, "Assets/DLC/Conflict/Sounds/XML/ExtraScripts.xml",
              "<Script2DFile><Script2DSounds><Script2DSound>"
              "<ScriptID>AS2D_OVERRIDE</ScriptID><SoundID>SND_DIFFERENT</SoundID>"
              "</Script2DSound><Script2DSound>"
              "<ScriptID>AS2D_SECOND</ScriptID><SoundID>SND_OVERRIDE</SoundID>"
              "</Script2DSound></Script2DSounds></Script2DFile>");
        write(root, "Assets/DLC/Conflict/Sounds/XML/ExtraDefines.xml",
              "<AudioDefinesFile><SoundDatas><SoundData>"
              "<SoundID>SND_OVERRIDE</SoundID><Filename>DifferentSound</Filename>"
              "</SoundData></SoundDatas></AudioDefinesFile>");
        auto conflict = std::make_shared<assets::LooseResourceIndex>(root);
        conflict->add_file("Assets/DLC/Conflict/Sounds/XML/ExtraScripts.xml");
        conflict->add_file("Assets/DLC/Conflict/Sounds/XML/ExtraDefines.xml");
        resources.mount({8, 0, 0, assets::resource_lookup(conflict), true, true,
                         assets::resource_lookup_all(conflict), assets::resource_keys(conflict)});
        catalog.packages.push_back(package(
            "<Civ5Package><Gameplay><AudioScripts2D>ExtraScripts.xml</AudioScripts2D>"
            "<AudioDefines>ExtraDefines.xml</AudioDefines>"
            "</Gameplay></Civ5Package>", "conflict"));
        const std::array selected_both{selected, ui::UiSelectedDlcAudio{1, {true,true,false}}};
        const auto conflicted = ui::UiAudioAssetResolver::from_selected_dlc(
            resources, catalog, selected_both);
        const auto ambiguous = conflicted.resolve("AS2D_OVERRIDE");
        require(ambiguous.status == ui::UiAudioAssetStatus::ambiguous_script &&
                    ambiguous.conflicting_sources.size() == 2 &&
                    ambiguous.conflicting_scripts.size() == 2 &&
                    ambiguous.conflicting_scripts[0].sound_id == "SND_OVERRIDE" &&
                    ambiguous.conflicting_scripts[1].sound_id == "SND_DIFFERENT" &&
                    ambiguous.conflicting_sources[0].mount_id == 7 &&
                    ambiguous.conflicting_sources[1].mount_id == 8,
                "Distinct selected XML files with same ScriptID remain a provenance-rich conflict");
        const auto ambiguous_sound = conflicted.resolve("AS2D_SECOND");
        require(ambiguous_sound.status == ui::UiAudioAssetStatus::ambiguous_sound &&
                    ambiguous_sound.conflicting_sources.size() == 2 &&
                    ambiguous_sound.conflicting_definitions.size() == 2 &&
                    ambiguous_sound.conflicting_definitions[0].filename == "OverrideSound" &&
                    ambiguous_sound.conflicting_definitions[1].filename == "DifferentSound" &&
                    ambiguous_sound.conflicting_sources[0].mount_id == 7 &&
                    ambiguous_sound.conflicting_sources[1].mount_id == 8,
                "Distinct selected definitions with same SoundID remain a conflict");
        write(root, "Assets/DLC/Selected/Other/OverrideSound.wav",
              std::string_view(wave_bytes, sizeof(wave_bytes) - 1));
        override->add_file("Assets/DLC/Selected/Other/OverrideSound.wav");
        require(chosen.resolve("AS2D_OVERRIDE").status ==
                    ui::UiAudioAssetStatus::ambiguous_asset,
                "Two selected payloads with one basename cannot silently pick the last file");

        write(root, "Assets/DLC/DupA/Sounds/XML/DuplicateScripts.xml", override_scripts);
        write(root, "Assets/DLC/DupB/Sounds/XML/DuplicateScripts.xml", override_scripts);
        auto duplicated = std::make_shared<assets::LooseResourceIndex>(root);
        duplicated->add_file("Assets/DLC/DupA/Sounds/XML/DuplicateScripts.xml");
        duplicated->add_file("Assets/DLC/DupB/Sounds/XML/DuplicateScripts.xml");
        resources.mount({9, 0, 0, assets::resource_lookup(duplicated), true, true,
                         assets::resource_lookup_all(duplicated), assets::resource_keys(duplicated)});
        catalog.packages.push_back(package(
            "<Civ5Package><Gameplay><AudioScripts2D>DuplicateScripts.xml</AudioScripts2D>"
            "</Gameplay></Civ5Package>", "ambiguous-file"));
        bool rejected_file = false;
        try {
            const ui::UiSelectedDlcAudio third{2, {true,true,false}};
            (void)ui::UiAudioAssetResolver::from_selected_dlc(
                resources, catalog, std::span(&third, 1));
        } catch (const std::runtime_error &) { rejected_file = true; }
        require(rejected_file, "Ambiguous selected XML basename must block construction");

        if (argc == 2) {
            const fs::path game_root(argv[1]);
            auto real_base = std::make_shared<assets::LooseResourceIndex>(game_root);
            real_base->add_file("Assets/Sounds/XML/Audio2DScripts.xml");
            real_base->add_file("Assets/Sounds/XML/AudioDefines.xml");
            real_base->add_file("Assets/Sounds/Streamed/NotStreamed/Ding.wav");
            auto real_dlc = std::make_shared<assets::LooseResourceIndex>(game_root);
            real_dlc->scan({"Assets/DLC/Expansion2/Sounds/XML", {}, false});
            assets::ResourceMountTable real_resources(assets::resource_lookup(real_base));
            real_resources.mount({200, 0, 0, assets::resource_lookup(real_dlc), true,
                                  true, assets::resource_lookup_all(real_dlc),
                                  assets::resource_keys(real_dlc)});
            content::Catalog real_catalog;
            real_catalog.packages.push_back(content::read_manifest(
                game_root / "Assets/DLC/Expansion2/Expansion2.Civ5Pkg"));
            const ui::UiSelectedDlcAudio bnw{0, {true,true,false}};
            const auto retail = ui::UiAudioAssetResolver::from_selected_dlc(
                real_resources, real_catalog, std::span(&bnw, 1));
            const auto ding = retail.resolve("AS2D_IF_MP_CHAT_DING");
            require(ding.status == ui::UiAudioAssetStatus::pcm_ready &&
                        ding.script_source && ding.script_source->provenance &&
                        ding.script_source->provenance->source_relative_path ==
                            "Assets/Sounds/XML/Audio2DScripts.xml",
                    "Selected BNW DLC leaves the base Ding cue sourced from base XML");
            const auto caravan = retail.resolve("AS2D_INTERFACE_CARAVAN_TRADE_ROUTE");
            require(caravan.script_source && caravan.script_source->provenance &&
                        caravan.script_source->provenance->source_relative_path ==
                            "Assets/DLC/Expansion2/Sounds/XML/Audio2DScriptsExpansion2.xml",
                    "Selected BNW-only script retains its DLC source");
            const auto main_menu = retail.resolve("AS2D_MUSIC_MAIN_MENU");
            require(main_menu.status == ui::UiAudioAssetStatus::ambiguous_script &&
                        main_menu.conflicting_scripts.size() >= 2 &&
                        main_menu.conflicting_scripts[0].sound_id ==
                            "SND_MUSIC_MAIN_MENU" &&
                        main_menu.conflicting_scripts.back().sound_id ==
                            "SND_MUSIC_MAIN_MENU_EXP2" &&
                        main_menu.conflicting_scripts[0].source &&
                        main_menu.conflicting_scripts[0].source->provenance &&
                        main_menu.conflicting_scripts[0].source->provenance->source_relative_path ==
                            "Assets/Sounds/XML/Audio2DScripts.xml" &&
                        main_menu.conflicting_scripts.back().source &&
                        main_menu.conflicting_scripts.back().source->provenance &&
                        main_menu.conflicting_scripts.back().source->provenance->source_relative_path ==
                            "Assets/DLC/Expansion2/Sounds/XML/Audio2DScriptsExpansion2.xml",
                    "Real base/DLC conflicting mappings retain authored order and provenance");
        }
        std::error_code ignored;
        fs::remove_all(root, ignored);
        std::cout << "Selected UI audio tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
