#pragma once

#include "civ5/ui/ui_audio_events.hpp"
#include <cstdint>
#include <array>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::ui {

enum class UiAudioAssetStatus {
    pcm_ready, missing_script, ambiguous_script, missing_sound,
    ambiguous_sound, missing_asset, ambiguous_asset, unsupported_codec,
    malformed_wave
};

struct UiPcmFormat {
    std::uint16_t channels = 0, bits_per_sample = 0;
    std::uint32_t sample_rate = 0;
    std::uint64_t data_offset = 0, data_bytes = 0;
};

struct UiImaAdpcmFormat {
    std::uint64_t data_offset = 0, data_bytes = 0;
    std::uint16_t block_align = 0, samples_per_block = 0;
    std::uint32_t sample_count = 0;
    std::uint16_t channels = 1;
};

struct UiAudioScriptCandidate {
    std::string sound_id;
    std::optional<UiAudioSource> source;
};

struct UiAudioDefinitionCandidate {
    std::string filename;
    std::optional<UiAudioSource> source;
};

struct UiAudioAssetResolution {
    UiAudioAssetStatus status = UiAudioAssetStatus::missing_script;
    std::string script_id, sound_id, filename, reason;
    std::optional<UiAudioSource> script_source, definition_source;
    std::vector<UiAudioSource> conflicting_sources;
    // Ordered evidence for duplicate IDs. Authored base/DLC XML shows that
    // replacement is intended for some IDs, but does not prove the native
    // audio loader's winning-row rule; resolve() therefore blocks them.
    std::vector<UiAudioScriptCandidate> conflicting_scripts;
    std::vector<UiAudioDefinitionCandidate> conflicting_definitions;
    std::optional<assets::AssetHandle> asset;
    std::optional<UiPcmFormat> pcm;
    // Present when the mounted WAVE is decoded to PCM16 by submit_ui_pcm.
    std::optional<UiImaAdpcmFormat> ima_adpcm;
};

struct UiAudioXmlLayer {
    content::XmlNode document;
    std::optional<UiAudioSource> source;
};

struct UiSelectedDlcAudio {
    std::size_t package_index = 0;
    // Common, Gameplay, UISkin activation for this selected package.
    std::array<bool, 3> enabled_categories{};
};

// Resolves selected AS2D definitions through the same active resource mounts
// as the UI. Caller supplies every activated XML layer in activation order;
// duplicate IDs or multiple matching audio files remain explicit blockers.
class UiAudioAssetResolver {
  public:
    static UiAudioAssetResolver from_documents(const assets::ResourceMountTable &resources,
        std::vector<UiAudioXmlLayer> scripts, std::vector<UiAudioXmlLayer> definitions);
    static UiAudioAssetResolver from_resources(const assets::ResourceMountTable &resources,
        const std::vector<std::string> &script_keys = {
            "Assets/Sounds/XML/Audio2DScripts.xml"},
        const std::vector<std::string> &definition_keys = {
            "Assets/Sounds/XML/AudioDefines.xml"});
    // Base masters, then exactly the caller-selected DLCs in activation order.
    // Manifest AudioScripts2D/AudioDefines declarations are resolved through
    // the selected VFS. Repeated declarations of one selected file load once;
    // conflicting script/sound IDs remain ambiguous on resolve().
    static UiAudioAssetResolver from_selected_dlc(
        const assets::ResourceMountTable &resources, const content::Catalog &catalog,
        std::span<const UiSelectedDlcAudio> selected);

    UiAudioAssetResolution resolve(std::string_view script_id) const;

  private:
    explicit UiAudioAssetResolver(const assets::ResourceMountTable &resources)
        : resources_(&resources) {}
    struct Script {
        std::string sound_id;
        std::optional<UiAudioSource> source;
    };
    struct Definition {
        std::string filename;
        std::optional<UiAudioSource> source;
    };
    const assets::ResourceMountTable *resources_;
    std::map<std::string, std::vector<Script>, std::less<>> scripts_;
    std::map<std::string, std::vector<Definition>, std::less<>> definitions_;
};

struct UiPcmBuffer {
    UiPcmFormat format;
    std::vector<std::uint8_t> interleaved_samples;
    assets::Provenance provenance;
};

// A backend seam only. submit() may copy PCM to an actual audio device later;
// a successful call does not claim device playback or codec parity.
class UiPcmSink {
  public:
    virtual ~UiPcmSink() = default;
    virtual bool submit(const UiPcmBuffer &buffer) = 0;
};

enum class UiPcmSubmitStatus {
    submitted, resolution_blocked, size_blocked, sink_rejected, failed_exception
};
UiPcmSubmitStatus submit_ui_pcm(const UiAudioAssetResolver &resolver,
    std::string_view script_id, UiPcmSink &sink, std::uint64_t maximum_bytes = 16ULL * 1024 * 1024);

struct UiAudioPlaybackResult {
    UiAudioEvent event;
    UiPcmSubmitStatus status = UiPcmSubmitStatus::resolution_blocked;
    std::string error;
};

// Drain once per transition batch. The result retains every authored event and
// reports a blocked asset or device refusal without claiming audible playback.
std::vector<UiAudioPlaybackResult> submit_ui_audio_events(
    UiAudioQueue &queue, const UiAudioAssetResolver &resolver, UiPcmSink &sink,
    std::uint64_t maximum_bytes = 16ULL * 1024 * 1024);

} // namespace civ5::ui
