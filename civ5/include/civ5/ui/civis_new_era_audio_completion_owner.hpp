#pragma once
#include "civ5/ui/new_era_audio_delivery.hpp"
#include <functional>
#include <memory>
#include <string>

namespace civ5::ui {
struct CivisAudioSessionIdentity {
    std::string payload_sha256, rules_sha256;
    std::uint64_t generation = 0;
    bool operator==(const CivisAudioSessionIdentity &) const = default;
};
struct CivisNewEraAudioContextIdentity {
    CivisAudioSessionIdentity session;
    std::uint64_t context_generation = 0, popup_owner_sequence = 0;
    std::int64_t player_id = -1;
    std::int32_t turn = -1;
};
// Retain ONE token for the whole selected-host lifetime, across context owners
// and LiveSession state replacement/restore. session.generation identifies
// that lifetime, NOT LiveSession::state_generation(). No replacement token
// may bypass quarantine: WinMM has no public cancellation receipt.
class CivisAudioSessionRetirementToken {
  public:
    explicit CivisAudioSessionRetirementToken(CivisAudioSessionIdentity);
    ~CivisAudioSessionRetirementToken();
    bool quarantined() const;
    CivisAudioSessionRetirementToken(const CivisAudioSessionRetirementToken &) = delete;
    CivisAudioSessionRetirementToken &operator=(const CivisAudioSessionRetirementToken &) = delete;
  private:
    friend class CivisNewEraAudioCompletionOwner;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
enum class CivisNewEraCompletionStatus {
    idle, submitted, pending, completed, duplicate, busy, rejected,
    retired, closed, wrong_thread
};
struct CivisNewEraCompletionResult {
    CivisNewEraCompletionStatus status = CivisNewEraCompletionStatus::rejected;
    std::string blocker;
};
struct CivisNewEraCompletionNotice {
    CivisNewEraAudioContextIdentity owner;
    NewEraPopupShownReceipt shown;
    WinMmAudioSnapshot released_device;
};
// Civis completion correlation, not original Wwise/native callback parity.
// Creates an exclusive bridge + WinMM sink internally. Only PopupShown receipts
// may submit PCM; polling cannot manufacture a popup or cue. Device completion
// means successful release after WHDR_DONE, never proof of audible playback.
class CivisNewEraAudioCompletionOwner final
    : public std::enable_shared_from_this<CivisNewEraAudioCompletionOwner> {
  public:
    // Must attest the SAME selected host, pinned source/VFS selection and
    // source-issued popup occurrence, player/turn, native state generation
    // and UI publication. Keep its actual owner alive or weakly fail closed.
    // The token does not substitute for those checks. A copied mount table
    // does not freeze external file bytes. Do not capture replaceable borrowed
    // frame/receipt references; shown() pins its inputs before callbacks.
    using Current = std::function<bool(const CivisNewEraAudioContextIdentity &)>;
    using Observer = std::function<void(const CivisNewEraCompletionNotice &)>;
    static std::shared_ptr<CivisNewEraAudioCompletionOwner> open(
        std::shared_ptr<CivisAudioSessionRetirementToken>,
        CivisNewEraAudioContextIdentity, assets::ResourceMountTable,
        Current, Observer, WinMmPcmSink::DeviceFactory = make_winmm_wave_out_device);
    static std::shared_ptr<CivisNewEraAudioCompletionOwner> open_selected(
        std::shared_ptr<CivisAudioSessionRetirementToken>,
        CivisNewEraAudioContextIdentity, assets::ResourceMountTable,
        const content::Catalog &, std::span<const UiSelectedDlcAudio>,
        Current, Observer, WinMmPcmSink::DeviceFactory = make_winmm_wave_out_device);
    ~CivisNewEraAudioCompletionOwner();
    CivisNewEraCompletionResult shown(const UiFrameSnapshot &, const NewEraPopupShownReceipt &);
    CivisNewEraCompletionResult poll();
    CivisNewEraCompletionResult close();
    CivisNewEraAudioCompletionOwner(const CivisNewEraAudioCompletionOwner &) = delete;
    CivisNewEraAudioCompletionOwner &operator=(const CivisNewEraAudioCompletionOwner &) = delete;
  private:
    struct Impl;
    explicit CivisNewEraAudioCompletionOwner(std::unique_ptr<Impl>);
    static std::shared_ptr<CivisNewEraAudioCompletionOwner> make(
        std::shared_ptr<CivisAudioSessionRetirementToken>,
        CivisNewEraAudioContextIdentity, assets::ResourceMountTable,
        const content::Catalog *, std::span<const UiSelectedDlcAudio>,
        Current, Observer, WinMmPcmSink::DeviceFactory);
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::ui
