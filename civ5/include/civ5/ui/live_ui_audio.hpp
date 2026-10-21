#pragma once

#include "civ5/ui/ui_audio_assets.hpp"
#include "civ5/ui/ui_winmm_audio.hpp"
#include <memory>
#include <optional>

namespace civ5::ui {
class BnwAudioCompletedResourceOwner;class BnwAudioSourceCatalog;

enum class LiveUiAudioDisposition { processed, duplicate_or_stale, invalid_transition };

struct LiveUiAudioResult {
    LiveUiAudioDisposition disposition = LiveUiAudioDisposition::invalid_transition;
    UiAudioEmitStatus emission = UiAudioEmitStatus::unknown;
    std::optional<UiAudioPlaybackResult> playback;
};

// The GameCore/Lua event owner supplies an increasing event sequence and an
// authored BUTTONPOPUP_*/NOTIFICATION_* ID for each real transition. A host
// may forward those typed cues; context navigation or render alone has no cue.
// Resources are copied into an immutable mount snapshot and outlive the
// catalog/resolver; the sink is owned and destroyed before those resources.
class LiveUiAudioBridge {
  public:
    explicit LiveUiAudioBridge(assets::ResourceMountTable resources,
        std::unique_ptr<UiPcmSink> sink = std::make_unique<WinMmPcmSink>());
    // The VFS must contain exactly the locally activated base/DLC resources.
    // selected is the caller's DLC activation order and category selection;
    // catalog is needed only during construction, while the copied VFS lives
    // for the full bridge lifetime. The UI event catalog uses this same VFS.
    LiveUiAudioBridge(assets::ResourceMountTable resources,
        const content::Catalog &catalog, std::span<const UiSelectedDlcAudio> selected,
        std::unique_ptr<UiPcmSink> sink = std::make_unique<WinMmPcmSink>());
    // Strict startup entry. Raw mount constructors remain diagnostic components.
    static std::unique_ptr<LiveUiAudioBridge> from_completed_resources(
        std::shared_ptr<const BnwAudioCompletedResourceOwner>,
        std::shared_ptr<const BnwAudioSourceCatalog>);
    LiveUiAudioBridge(const LiveUiAudioBridge &) = delete;
    LiveUiAudioBridge &operator=(const LiveUiAudioBridge &) = delete;

    LiveUiAudioResult transition(std::uint64_t event_sequence,
                                  UiAudioKind kind, std::string_view authored_id);
    LiveUiAudioResult script_transition(std::uint64_t event_sequence,
        std::string_view source_context, std::string_view authored_script_id);

    // WinMM errors can occur after enqueue. A queued submission is not proof
    // that a speaker played sound; poll this from outside the render pass.
    std::optional<WinMmAudioSnapshot> device_status() const;
    std::uint64_t last_transition_sequence() const { return last_sequence_; }
    const UiAudioCatalog &audio_catalog() const { return catalog_; }
    UiAudioAssetResolution inspect_script(std::string_view script_id) const {
        return resolver_.resolve(script_id);
    }

  private:
    LiveUiAudioResult finish(UiAudioEmitStatus emission);
    bool accept_sequence(std::uint64_t sequence);

    std::shared_ptr<const assets::ResourceMountTable> resources_;
    UiAudioCatalog catalog_;
    UiAudioAssetResolver resolver_;
    UiAudioQueue queue_;
    std::unique_ptr<UiPcmSink> sink_;
    std::uint64_t last_sequence_ = 0;
};

} // namespace civ5::ui
