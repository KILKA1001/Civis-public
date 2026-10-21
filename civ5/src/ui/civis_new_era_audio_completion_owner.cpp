#include "civ5/ui/civis_new_era_audio_completion_owner.hpp"
#include "civ5/ui/ui_owner_fences.hpp"
#include <limits>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace civ5::ui {
namespace {
bool valid_sha(const std::string &s) {
    if (s.size() != 64) return false;
    for (const char c : s)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}
CivisNewEraCompletionResult result(CivisNewEraCompletionStatus s, std::string why = {}) {
    return {s, std::move(why)};
}
bool idle(const WinMmAudioSnapshot &s) { return !s.active && s.queued == 0; }
}
struct CivisAudioSessionRetirementToken::Impl {
    explicit Impl(CivisAudioSessionIdentity id) : identity(std::move(id)) {}
    const CivisAudioSessionIdentity identity;
    const std::thread::id thread = std::this_thread::get_id();
    mutable std::mutex mutex;
    bool claimed = false, quarantine = false;
    std::uint64_t last_consumed_cue = 0;
};
CivisAudioSessionRetirementToken::CivisAudioSessionRetirementToken(CivisAudioSessionIdentity id) {
    if (!id.generation || !valid_sha(id.payload_sha256) || !valid_sha(id.rules_sha256))
        throw std::invalid_argument("audio session requires pinned payload/rules and nonzero generation");
    impl_ = std::make_unique<Impl>(std::move(id));
}
CivisAudioSessionRetirementToken::~CivisAudioSessionRetirementToken() = default;
bool CivisAudioSessionRetirementToken::quarantined() const {
    std::lock_guard lock(impl_->mutex);
    return impl_->quarantine;
}
struct CivisNewEraAudioCompletionOwner::Impl {
    std::shared_ptr<CivisAudioSessionRetirementToken> token;
    CivisNewEraAudioContextIdentity identity;
    Current current;
    Observer observer;
    std::unique_ptr<LiveUiAudioBridge> bridge;
    std::unique_ptr<NewEraAudioDelivery> delivery;
    std::thread::id thread = std::this_thread::get_id();
    WinMmAudioSnapshot baseline;
    std::optional<NewEraPopupShownReceipt> pending;
    bool closed = false, retired = false, busy = false, claim = false;
    std::string blocker;
    bool same_thread() const { return thread == std::this_thread::get_id(); }
    void shutdown() {
        delivery.reset();
        bridge.reset();
    }
    void release_claim(bool uncertain) {
        if (!claim) return;
        std::lock_guard lock(token->impl_->mutex);
        token->impl_->quarantine |= uncertain;
        token->impl_->claimed = false;
        claim = false;
    }
    CivisNewEraCompletionResult retire(std::string why, bool uncertain = false) {
        retired = true;
        blocker = std::move(why);
        release_claim(uncertain || pending.has_value());
        // Destroy delivery before its borrowed bridge; shutdown cancels/joins.
        // A mounted-resource observer can reenter during delivery. Keep the
        // in-flight borrowed bridge alive until its owning operation unwinds.
        if (!busy) shutdown();
        pending.reset();
        return result(CivisNewEraCompletionStatus::retired, blocker);
    }
    bool is_current() noexcept {
        try { return current && current(identity) && !closed && !retired; }
        catch (...) { return false; }
    }
    CivisNewEraCompletionResult entry() {
        if (!same_thread()) return result(CivisNewEraCompletionStatus::wrong_thread, "audio owner thread mismatch");
        if (closed) return result(CivisNewEraCompletionStatus::closed);
        if (retired) return result(CivisNewEraCompletionStatus::retired, blocker);
        if (busy) return retire("reentrant audio owner operation");
        return result(CivisNewEraCompletionStatus::idle);
    }
};
CivisNewEraAudioCompletionOwner::CivisNewEraAudioCompletionOwner(std::unique_ptr<Impl> p)
    : impl_(std::move(p)) {}
CivisNewEraAudioCompletionOwner::~CivisNewEraAudioCompletionOwner() {
    impl_->release_claim(impl_->pending.has_value());
    impl_->delivery.reset();
    impl_->bridge.reset();
}
std::shared_ptr<CivisNewEraAudioCompletionOwner> CivisNewEraAudioCompletionOwner::open(
    std::shared_ptr<CivisAudioSessionRetirementToken> token,
    CivisNewEraAudioContextIdentity id, assets::ResourceMountTable resources,
    Current current, Observer observer, WinMmPcmSink::DeviceFactory factory) {
    return make(std::move(token), std::move(id), std::move(resources), nullptr, {},
        std::move(current), std::move(observer), std::move(factory));
}
std::shared_ptr<CivisNewEraAudioCompletionOwner> CivisNewEraAudioCompletionOwner::open_selected(
    std::shared_ptr<CivisAudioSessionRetirementToken> token,
    CivisNewEraAudioContextIdentity id, assets::ResourceMountTable resources,
    const content::Catalog &catalog, std::span<const UiSelectedDlcAudio> selected,
    Current current, Observer observer, WinMmPcmSink::DeviceFactory factory) {
    return make(std::move(token), std::move(id), std::move(resources), &catalog, selected,
        std::move(current), std::move(observer), std::move(factory));
}
std::shared_ptr<CivisNewEraAudioCompletionOwner> CivisNewEraAudioCompletionOwner::make(
    std::shared_ptr<CivisAudioSessionRetirementToken> token,
    CivisNewEraAudioContextIdentity id, assets::ResourceMountTable resources,
    const content::Catalog *catalog, std::span<const UiSelectedDlcAudio> selected,
    Current current, Observer observer, WinMmPcmSink::DeviceFactory factory) {
    if (!token || !current || !observer || !factory || !id.context_generation ||
        !id.popup_owner_sequence || id.player_id < 0 || id.turn < 0)
        throw std::invalid_argument("audio owner requires session, current context and observer");
    auto p = std::make_unique<Impl>();
    p->token = token;
    p->identity = std::move(id);
    p->current = std::move(current);
    p->observer = std::move(observer);
    {
        std::lock_guard lock(token->impl_->mutex);
        if (token->impl_->thread != std::this_thread::get_id() ||
            token->impl_->identity != p->identity.session || token->impl_->claimed || token->impl_->quarantine)
            throw std::runtime_error("audio session foreign, already claimed or quarantined");
        token->impl_->claimed = true;
        p->claim = true;
    }
    try {
        auto sink = std::make_unique<WinMmPcmSink>(1, std::move(factory));
        if (catalog) p->bridge = std::make_unique<LiveUiAudioBridge>(resources, *catalog, selected, std::move(sink));
        else p->bridge = std::make_unique<LiveUiAudioBridge>(resources, std::move(sink));
        const auto proof = inspect_new_era_audio_source(resources, p->bridge->audio_catalog());
        if (!proof.verified) throw std::runtime_error(proof.blocker);
        p->delivery = std::make_unique<NewEraAudioDelivery>(proof, *p->bridge);
        const auto status = p->bridge->device_status();
        if (!status || !idle(*status) || status->accepted || status->completed || status->failed || status->canceled)
            throw std::runtime_error("exclusive audio backend must start empty");
        p->baseline = *status;
    } catch (...) {
        p->release_claim(false); // Construction emits no cue and submits no PCM.
        throw;
    }
    return std::shared_ptr<CivisNewEraAudioCompletionOwner>(new CivisNewEraAudioCompletionOwner(std::move(p)));
}
CivisNewEraCompletionResult CivisNewEraAudioCompletionOwner::shown(
    const UiFrameSnapshot &borrowed_frame, const NewEraPopupShownReceipt &borrowed_receipt) {
    const auto keep_alive = shared_from_this();
    auto &p = *impl_;
    const auto entry = p.entry();
    if (entry.status != CivisNewEraCompletionStatus::idle) return entry;
    p.busy = true;
    struct Exit { Impl &p; ~Exit() { p.busy = false; if (p.closed || p.retired) p.shutdown(); } } exit{p};
    // Pin the original publication and occurrence BEFORE any current/source
    // callback can replace the caller optional or reallocate its event vector.
    const auto frame = borrowed_frame;
    const auto receipt = borrowed_receipt;
    if (!p.is_current()) return p.retire("audio context is no longer current");
    if (!frame.find<UiOwnerFences>()) return result(CivisNewEraCompletionStatus::rejected, "PopupShown requires explicit current frame fences");
    if (!ui_owners_current(frame) || p.closed || p.retired || !p.is_current())
        return p.retire("PopupShown frame/context changed during guard");
    if (receipt.player_id != p.identity.player_id || receipt.turn != p.identity.turn ||
        receipt.owner_sequence != p.identity.popup_owner_sequence)
        return result(CivisNewEraCompletionStatus::rejected, "PopupShown receipt belongs to a different owner");
    if (p.pending) {
        const auto &old = *p.pending;
        return result(old.cue_sequence == receipt.cue_sequence && old.event_id == receipt.event_id &&
            old.frame == receipt.frame ? CivisNewEraCompletionStatus::duplicate : CivisNewEraCompletionStatus::busy);
    }
    {
        std::lock_guard lock(p.token->impl_->mutex);
        if (receipt.cue_sequence && receipt.cue_sequence <= p.token->impl_->last_consumed_cue)
            return result(CivisNewEraCompletionStatus::duplicate, "session cue already consumed");
    }
    const auto before = p.bridge->device_status();
    if (!before || !idle(*before) || before->accepted != p.baseline.accepted ||
        before->completed != p.baseline.completed || before->failed != p.baseline.failed ||
        before->canceled != p.baseline.canceled || before->accepted == std::numeric_limits<std::uint64_t>::max())
        return p.retire("exclusive audio baseline changed", true);
    // Retain the receipt before any possible external backend effect.
    p.pending = receipt;
    NewEraDeliveryResult delivery;
    try { delivery = p.delivery->shown(frame, receipt); }
    catch (...) { return p.retire("audio delivery threw after receipt retention", true); }
    if (p.closed || p.retired) return p.retire("audio owner retired during in-flight delivery", true);
    if (delivery.cue) {
        std::lock_guard lock(p.token->impl_->mutex);
        p.token->impl_->last_consumed_cue = delivery.cue->sequence;
    }
    if (delivery.status != NewEraDeliveryStatus::sink_submitted) {
        const auto after = p.bridge->device_status();
        if (!after || after->accepted != before->accepted ||
            after->completed != before->completed || after->failed != before->failed ||
            after->canceled != before->canceled || !idle(*after) ||
            delivery.status == NewEraDeliveryStatus::uncertain)
            return p.retire("audio submission outcome uncertain", true);
        p.pending.reset(); // Unchanged private counters prove no backend effect.
        if (!ui_owners_current(frame) || !p.is_current())
            return p.retire("PopupShown frame/context changed during rejected delivery");
        if (delivery.status == NewEraDeliveryStatus::duplicate)
            return result(CivisNewEraCompletionStatus::duplicate);
        return result(CivisNewEraCompletionStatus::rejected, delivery.blocker);
    }
    // Delivery can resolve mounted files and reenter actual owner fences.
    // Retain and recheck the ORIGINAL publication, never a replacement frame.
    if (!ui_owners_current(frame) || !p.is_current())
        return p.retire("PopupShown frame/context changed after submission");
    return result(CivisNewEraCompletionStatus::submitted);
}
CivisNewEraCompletionResult CivisNewEraAudioCompletionOwner::poll() {
    const auto keep_alive = shared_from_this();
    auto &p = *impl_;
    const auto entry = p.entry();
    if (entry.status != CivisNewEraCompletionStatus::idle) return entry;
    p.busy = true;
    struct Exit { Impl &p; ~Exit() { p.busy = false; if (p.closed || p.retired) p.shutdown(); } } exit{p};
    if (!p.is_current()) return p.retire("audio completion context is no longer current");
    if (!p.pending) return result(CivisNewEraCompletionStatus::idle);
    const auto status = p.bridge->device_status();
    if (!status || status->accepted != p.baseline.accepted + 1 ||
        status->failed != p.baseline.failed || status->canceled != p.baseline.canceled ||
        status->completed < p.baseline.completed || status->completed > p.baseline.completed + 1)
        return p.retire("audio completion counters uncertain; session quarantined", true);
    if (status->completed == p.baseline.completed) {
        if (status->queued + static_cast<std::size_t>(status->active) != 1)
            return p.retire("audio cue vanished without release receipt", true);
        return result(CivisNewEraCompletionStatus::pending);
    }
    if (!idle(*status)) return p.retire("completion backend still owns another cue", true);
    const CivisNewEraCompletionNotice notice{p.identity, *p.pending, *status};
    p.pending.reset(); // Consume before observer; never replay a throwing callback.
    p.baseline = *status;
    if (!p.is_current()) return p.retire("audio context changed before completion observer");
    try { p.observer(notice); }
    catch (...) { return p.retire("audio completion observer failed"); }
    if (p.closed) return result(CivisNewEraCompletionStatus::closed);
    if (p.retired || !p.is_current()) return p.retire("audio context changed during completion observer");
    return result(CivisNewEraCompletionStatus::completed);
}
CivisNewEraCompletionResult CivisNewEraAudioCompletionOwner::close() {
    auto &p = *impl_;
    if (!p.same_thread()) return result(CivisNewEraCompletionStatus::wrong_thread, "audio close thread mismatch");
    if (p.closed) return result(CivisNewEraCompletionStatus::closed);
    p.closed = true;
    p.release_claim(p.pending.has_value());
    if (!p.busy) p.shutdown();
    p.pending.reset();
    return result(CivisNewEraCompletionStatus::closed);
}
} // namespace civ5::ui
