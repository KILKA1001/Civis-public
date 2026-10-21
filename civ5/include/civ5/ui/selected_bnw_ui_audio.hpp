#pragma once

#include "civ5/runtime/save_preflight.hpp"
#include "civ5/ui/live_ui_audio.hpp"
#include <filesystem>
#include <memory>

namespace civ5::ui {

// Build a selected, local BNW audio VFS from the same DLC identity/order as
// the restored save. The returned bridge owns its mounts and sink. It makes
// no event: only a real owner may supply an authored cue and sequence.
// The installed base SoundData FPK is mounted when present, including the
// stereo IMA New Era WAV. FSB/OGG and duplicate AS2D IDs remain unresolved.
std::unique_ptr<LiveUiAudioBridge> make_selected_bnw_ui_audio_bridge(
    const std::filesystem::path &game_root,
    const runtime::SavedContentProfile &profile,
    const content::Catalog &catalog,
    std::unique_ptr<UiPcmSink> sink = std::make_unique<WinMmPcmSink>());

} // namespace civ5::ui
