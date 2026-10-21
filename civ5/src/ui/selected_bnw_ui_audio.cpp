#include "civ5/ui/selected_bnw_ui_audio.hpp"
#include "civ5/ui/bnw_new_era_sound_archive.hpp"
#include <stdexcept>
#include <string_view>

namespace civ5::ui {
namespace fs = std::filesystem;

std::unique_ptr<LiveUiAudioBridge> make_selected_bnw_ui_audio_bridge(
    const fs::path &game_root, const runtime::SavedContentProfile &profile,
    const content::Catalog &catalog, std::unique_ptr<UiPcmSink> sink) {
    if (!profile.can_activate || !profile.enabled_mods.empty() ||
        profile.enabled_dlc.empty())
        throw std::invalid_argument("Selected BNW audio requires a complete DLC-only profile");
    if (!sink) throw std::invalid_argument("Selected BNW audio sink is missing");
    const auto root = fs::canonical(game_root);
    if (!fs::is_regular_file(root / "CivilizationV.exe") ||
        !fs::is_directory(root / "Assets/Sounds"))
        throw std::invalid_argument("Selected BNW audio needs local executable and Sounds assets");
    auto base = std::make_shared<assets::LooseResourceIndex>(root);
    base->scan({"Assets/Sounds", {}, false});
    assets::ResourceMountTable resources(assets::resource_lookup(base),
        assets::resource_lookup_all(base), assets::resource_keys(base));
    std::vector<UiSelectedDlcAudio> selected;
    bool expansion2 = false;
    std::uint64_t mount_id = 1;
    for (const auto index : profile.enabled_dlc) {
        const auto &package = catalog.packages.at(index);
        if (package.kind != content::PackageKind::dlc)
            throw std::invalid_argument("Selected BNW audio package is not DLC");
        if (content::canonical_guid(package.id) ==
            "6da07636-4123-4018-b643-6575b4ec336b") expansion2 = true;
        const auto package_root = fs::canonical(package.path.parent_path());
        const auto relative = package_root.lexically_relative(root);
        if (relative.empty() || relative.is_absolute() ||
            *relative.begin() == fs::path(".."))
            throw std::invalid_argument("Selected BNW audio package is outside local game root");
        auto layer = std::make_shared<assets::LooseResourceIndex>(root);
        const auto sounds = relative / "Sounds";
        if (fs::is_directory(root / sounds))
            layer->scan({sounds, {}, false});
        resources.mount({mount_id++, 0, 0, assets::resource_lookup(layer), true, true,
            assets::resource_lookup_all(layer), assets::resource_keys(layer)});
        selected.push_back({index, {true, true, true}});
    }
    if (!expansion2)
        throw std::invalid_argument("Selected BNW audio profile omits Expansion2");
    if (fs::is_regular_file(root / "Resource/Common/SoundData.fpk")) {
        const auto sound = mount_bnw_new_era_sound_archive(
            resources, root, mount_id++);
        if (sound.status != BnwNewEraSoundMountStatus::mounted)
            throw std::invalid_argument("Selected BNW SoundData archive: " +
                                        sound.reason);
    }
    return std::make_unique<LiveUiAudioBridge>(std::move(resources), catalog,
        selected, std::move(sink));
}

} // namespace civ5::ui
