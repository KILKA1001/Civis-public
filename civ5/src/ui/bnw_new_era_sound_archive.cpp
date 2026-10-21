#include "civ5/ui/bnw_new_era_sound_archive.hpp"
#include <memory>
#include <variant>

namespace civ5::ui {

BnwNewEraSoundMountResult mount_bnw_new_era_sound_archive(
    assets::ResourceMountTable &resources,
    const std::filesystem::path &game_root, std::uint64_t mount_id) {
    for (const auto &mount : resources.mounts())
        if (mount.id == mount_id)
            return {BnwNewEraSoundMountStatus::mount_id_in_use,
                    "Selected resource mount ID is already registered"};

    constexpr auto relative = "Resource/Common/SoundData.fpk";
    const auto opened = assets::FpkSource::open(game_root / relative, relative);
    const auto *source = std::get_if<std::shared_ptr<assets::FpkSource>>(&opened);
    if (!source)
        return {BnwNewEraSoundMountStatus::archive_unavailable,
                std::get<assets::FpkDiagnostic>(opened).reason};

    const assets::AssetKey key("NotificationNewEra.wav");
    const auto candidates = (*source)->find_all(key);
    if (candidates.size() != 1)
        return {BnwNewEraSoundMountStatus::missing_or_duplicate_payload,
                "Installed SoundData must contain exactly one NotificationNewEra.wav"};

    auto next = resources;
    next.mount({mount_id,0,0,assets::resource_lookup(*source),true,true,
                assets::resource_lookup_all(*source),assets::resource_keys(*source)});
    const auto selected = next.resolve(key);
    const auto *asset = std::get_if<assets::AssetHandle>(&selected.asset);
    if (!asset || selected.mount_id != mount_id ||
        asset->candidate().original_name != candidates.front().candidate().original_name ||
        asset->candidate().payload_size != candidates.front().candidate().payload_size)
        return {BnwNewEraSoundMountStatus::selected_payload_mismatch,
                "Selected VFS did not resolve the installed New Era sound"};
    resources = std::move(next);
    return {BnwNewEraSoundMountStatus::mounted,{}};
}

} // namespace civ5::ui
