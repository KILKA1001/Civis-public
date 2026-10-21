#pragma once
#include "civ5/ui/bnw_client_frame.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace civ5::ui {
enum class BnwTechTreeEffect {
    queued, show_semaphore_incremented, popup_shown,
    dequeued, hide_semaphore_decremented, popup_processed, open_flag_cleared
};
struct BnwTechTreeEffectObservation {
    BnwTechTreeEffect effect;
    ProgressionTechnologyTreeEvent occurrence;
    std::int32_t semaphore = 0;
    // Original science request Data2=-1; normal request has no Data1.
    std::int32_t data2 = -1, processed_result = 0;
    bool native_vm_executed = false, refresh_display_executed = false;
};

// Source-bound CIVIS local queue/show/hide/Processed ownership. This does not
// execute the original Lua VM or RefreshDisplay and is not a native receipt.
// Borrowed session, SAME shared screen controller and resource table must
// outlive this UI-thread owner. The admitted client owner is also borrowed
// through the close/post-ack callbacks. A state replacement requires a NEW owner.
// Each token is a process-unique Civis occurrence, never an original field.
class BnwTechTreeLifecycle {
  public:
    using Observer = std::function<void(const BnwTechTreeEffectObservation &)>;
    BnwTechTreeLifecycle(const runtime::LiveSession &, std::int64_t active_player,
                        UiScreenController &, const assets::ResourceMountTable &,
                        Observer = {});
    ~BnwTechTreeLifecycle();
    BnwTechTreeLifecycle(const BnwTechTreeLifecycle &) = delete;
    BnwTechTreeLifecycle &operator=(const BnwTechTreeLifecycle &) = delete;

    // Install INSIDE immutable client/UI publication, including absent events.
    // Existing foreign events/evidence are rejected rather than overwritten.
    UiFrameSnapshot augment_snapshot(const runtime::LiveSession &, const UiFrameSnapshot &) const;
    bool matches_snapshot(const runtime::LiveSession &, const UiFrameSnapshot &) const noexcept;
    std::optional<ProgressionTechnologyTreeEvent> active_event(const runtime::LiveSession &) const noexcept;

    // Call ONLY inside the shared frame/scene dispatch after its BEFORE gates.
    // Raw TopPanelRuntime telemetry alone is not admitted. Successful/uncertain
    // effects intentionally retire the frame; outer dispatch must invalidate it.
    UiCommandResult admit_science_action(const runtime::LiveSession &,
        const BnwClientFrameOwner &, const std::shared_ptr<const BnwClientFrame> &,
        const LiveUiHost &, const LiveUiFrame &, const TopPanelActionEvent &);
    // Validates occurrence/session/source, not the intentionally retired host
    // sequence. Actually dequeues SAME controller; host must not double-pop.
    UiCommandResult process(const ProgressionTechnologyTreeEvent &);
    // Host post-ack capability: actual completed local effects + same hidden
    // controller/parent, never event absence alone or an external-native claim.
    bool local_dequeue_observed(const ProgressionTechnologyTreeEvent &) const noexcept;

    const ProgressionScreenSource &source() const;
    std::vector<BnwTechTreeEffectObservation> journal() const;
    std::int32_t semaphore() const;
    bool uncertain() const;
  private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};
} // namespace civ5::ui
