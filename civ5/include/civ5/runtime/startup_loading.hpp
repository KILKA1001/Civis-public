#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace civ5::runtime {

enum class StartupLoadingPhase { idle, loading, completed, failed, cancelled };
enum class StartupStagePhase { queued, started, completed, failed, cancelled };
enum class StartupLoadingEvent { started, progress, completed, failed, cancelled };
enum class StartupStepResult { pending, completed };

struct StartupStageSnapshot {
    std::string id, title_key, detail;
    StartupStagePhase phase = StartupStagePhase::queued;
    std::uint64_t completed_units = 0;
    std::optional<std::uint64_t> total_units;
    std::optional<std::chrono::milliseconds> started_after, finished_after;
    // An unknown or zero denominator has no percentage. Stages are not weights.
    std::optional<double> fraction() const noexcept;
};
struct StartupLoadingSnapshot {
    StartupLoadingPhase phase = StartupLoadingPhase::idle;
    std::vector<StartupStageSnapshot> stages;
    std::size_t current_stage = 0;
    std::uint64_t revision = 0;
    bool cancellation_requested = false;
    std::chrono::milliseconds elapsed{};
    std::string error;
    const StartupStageSnapshot *current() const noexcept;
};

namespace detail { struct StartupLoadingState; }

// A progress handle is valid only inside the current step callback. Copies
// retained by a worker/callback cannot publish into a later stage or run.
class StartupStageProgress {
  public:
    void report(std::uint64_t completed, std::optional<std::uint64_t> total = {},
                std::string detail = {}) const;
    bool cancellation_requested() const;
    // Call only after an operation has returned and unwinding its existing
    // owners is safe. A pending cancellation then exits the current step.
    void checkpoint() const;
  private:
    friend class StartupLoading;
    StartupStageProgress(std::shared_ptr<detail::StartupLoadingState>, std::uint64_t);
    std::weak_ptr<detail::StartupLoadingState> state_;
    std::uint64_t operation_ = 0;
    std::shared_ptr<detail::StartupLoadingState> current() const;
};
struct StartupLoadingStage {
    std::string id, title_key;
    // Return at a safe boundary. pending keeps this stage active for a later
    // UI tick; completed is returned only after the owner's actual work returns.
    std::function<StartupStepResult(const StartupStageProgress &)> step;
};

// UI-thread orchestration and observation only. Callbacks borrow/retain their
// existing owners. This class creates no Bank, database, save, renderer or world.
// completed is not a native/gameplay readiness receipt; that remains the owner's.
class StartupLoading {
  public:
    using Observer = std::function<void(StartupLoadingEvent, const StartupLoadingSnapshot &)>;
    explicit StartupLoading(std::vector<StartupLoadingStage>, Observer = {});
    ~StartupLoading();
    StartupLoading(const StartupLoading &) = delete;
    StartupLoading &operator=(const StartupLoading &) = delete;
    // May be extended before the first advance, without changing navigation.
    void add_stage(StartupLoadingStage);
    // At most one real step per tick. Observers may paint/pump existing input;
    // they may request cancellation, but may not reenter advance/add_stage.
    StartupLoadingPhase advance();
    void request_cancel();
    StartupLoadingSnapshot snapshot() const;
  private:
    std::shared_ptr<detail::StartupLoadingState> state_;
};
} // namespace civ5::runtime
