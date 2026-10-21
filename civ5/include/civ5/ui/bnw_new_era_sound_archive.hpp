#pragma once

#include "civ5/assets/resource_mounts.hpp"
#include <filesystem>
#include <string>

namespace civ5::ui {

enum class BnwNewEraSoundMountStatus {
    mounted, archive_unavailable, missing_or_duplicate_payload,
    mount_id_in_use, selected_payload_mismatch
};

struct BnwNewEraSoundMountResult {
    BnwNewEraSoundMountStatus status = BnwNewEraSoundMountStatus::archive_unavailable;
    std::string reason;
};

// Mount the installed base SoundData archive into the selected VFS before
// constructing LiveUiAudioBridge. The bridge owns a copy of that VFS.
BnwNewEraSoundMountResult mount_bnw_new_era_sound_archive(
    assets::ResourceMountTable &resources,
    const std::filesystem::path &game_root, std::uint64_t mount_id);

} // namespace civ5::ui
