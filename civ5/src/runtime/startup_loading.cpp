#include "civ5/runtime/startup_loading.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace civ5::runtime {
std::optional<double> StartupStageSnapshot::fraction() const noexcept {
    if (!total_units || !*total_units) return {};
    return static_cast<double>(completed_units) / static_cast<double>(*total_units);
}
const StartupStageSnapshot *StartupLoadingSnapshot::current() const noexcept {
    return current_stage < stages.size() ? &stages[current_stage] : nullptr;
}
namespace detail {
struct StartupCancellationAtBoundary {};
struct StartupLoadingState {
    const std::thread::id thread = std::this_thread::get_id();
    const std::chrono::steady_clock::time_point origin = std::chrono::steady_clock::now();
    std::vector<StartupLoadingStage> work;
    StartupLoadingSnapshot value;
    StartupLoading::Observer observer;
    std::optional<std::chrono::steady_clock::time_point> stopped;
    std::uint64_t operation = 0;
    bool advancing = false, callback_active = false, notifying = false, alive = true;

    void require_thread() const {
        if (std::this_thread::get_id() != thread)
            throw std::logic_error("Startup loading used on another thread");
    }
    std::chrono::milliseconds elapsed() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            stopped.value_or(std::chrono::steady_clock::now()) - origin);
    }
    StartupLoadingSnapshot snapshot() const {
        auto result = value;
        result.elapsed = elapsed();
        return result;
    }
    void notify(StartupLoadingEvent event) {
        ++value.revision;
        if (!observer) return;
        if (notifying) throw std::logic_error("Startup progress observer reentry");
        notifying = true;
        struct Reset { bool &flag; ~Reset() { flag = false; } } reset{notifying};
        observer(event, snapshot());
    }
    bool terminal() const {
        return value.phase == StartupLoadingPhase::completed ||
            value.phase == StartupLoadingPhase::failed || value.phase == StartupLoadingPhase::cancelled;
    }
    void fail(std::string reason) {
        if (value.phase == StartupLoadingPhase::failed) return;
        stopped = std::chrono::steady_clock::now();
        value.phase = StartupLoadingPhase::failed;
        value.error = std::move(reason);
        if (value.current_stage < value.stages.size()) {
            auto &stage = value.stages[value.current_stage];
            stage.phase = StartupStagePhase::failed;
            stage.finished_after = elapsed();
        }
        // The original cause survives a second exception from the error observer.
        try { notify(StartupLoadingEvent::failed); } catch (...) {}
    }
    void cancel() {
        stopped = std::chrono::steady_clock::now();
        value.phase = StartupLoadingPhase::cancelled;
        if (value.current_stage < value.stages.size()) {
            auto &stage = value.stages[value.current_stage];
            if (stage.phase != StartupStagePhase::completed) {
                stage.phase = StartupStagePhase::cancelled;
                stage.finished_after = elapsed();
            }
        }
        notify(StartupLoadingEvent::cancelled);
    }
    void add(StartupLoadingStage stage) {
        if (value.phase != StartupLoadingPhase::idle || advancing || notifying)
            throw std::logic_error("Startup stages must be registered before loading");
        if (stage.id.empty() || stage.title_key.empty() || !stage.step ||
            std::any_of(work.begin(), work.end(), [&](const auto &entry) { return entry.id == stage.id; }))
            throw std::invalid_argument("Startup stage needs a unique ID, title and real step");
        value.stages.push_back({stage.id, stage.title_key});
        try { work.push_back(std::move(stage)); }
        catch (...) { value.stages.pop_back(); throw; }
    }
};
} // namespace detail

StartupStageProgress::StartupStageProgress(std::shared_ptr<detail::StartupLoadingState> state,
                                         std::uint64_t operation)
    : state_(std::move(state)), operation_(operation) {}
std::shared_ptr<detail::StartupLoadingState> StartupStageProgress::current() const {
    const auto state = state_.lock();
    if (!state) throw std::logic_error("Startup progress owner was destroyed");
    state->require_thread();
    if (!state->alive || !state->advancing || !state->callback_active || state->notifying ||
        state->operation != operation_ || state->value.phase != StartupLoadingPhase::loading)
        throw std::logic_error("Startup progress callback has already returned");
    return state;
}
void StartupStageProgress::report(std::uint64_t completed, std::optional<std::uint64_t> total,
                                  std::string detail) const {
    const auto state = current();
    auto &stage = state->value.stages.at(state->value.current_stage);
    if (completed < stage.completed_units || (total && completed > *total) ||
        (stage.total_units && total && stage.total_units != total) ||
        (stage.total_units && completed > *stage.total_units))
        throw std::invalid_argument("Startup progress counters went backwards or changed their denominator");
    stage.completed_units = completed;
    if (total) stage.total_units = total;
    stage.detail = std::move(detail);
    state->notify(StartupLoadingEvent::progress);
}
bool StartupStageProgress::cancellation_requested() const {
    return current()->value.cancellation_requested;
}
void StartupStageProgress::checkpoint() const {
    if (current()->value.cancellation_requested) throw detail::StartupCancellationAtBoundary{};
}

StartupLoading::StartupLoading(std::vector<StartupLoadingStage> stages, Observer observer)
    : state_(std::make_shared<detail::StartupLoadingState>()) {
    state_->observer = std::move(observer);
    for (auto &stage : stages) state_->add(std::move(stage));
    if (stages.empty()) throw std::invalid_argument("Startup loading needs at least one actual stage");
}
StartupLoading::~StartupLoading() { state_->alive = false; }
void StartupLoading::add_stage(StartupLoadingStage stage) {
    state_->require_thread(); state_->add(std::move(stage));
}
void StartupLoading::request_cancel() {
    state_->require_thread();
    if (!state_->terminal()) state_->value.cancellation_requested = true;
}
StartupLoadingSnapshot StartupLoading::snapshot() const {
    state_->require_thread(); return state_->snapshot();
}
StartupLoadingPhase StartupLoading::advance() {
    auto &state = *state_;
    state.require_thread();
    if (state.advancing || state.notifying) throw std::logic_error("Startup advance reentry");
    if (state.terminal()) return state.value.phase;
    state.advancing = true;
    struct Reset { detail::StartupLoadingState &state; ~Reset() {
        state.callback_active = false; state.advancing = false;
    } } reset{state};
    try {
        if (state.value.cancellation_requested) { state.cancel(); return state.value.phase; }
        state.value.phase = StartupLoadingPhase::loading;
        auto &stage = state.value.stages.at(state.value.current_stage);
        if (stage.phase == StartupStagePhase::queued) {
            stage.phase = StartupStagePhase::started;
            stage.started_after = state.elapsed();
            state.notify(StartupLoadingEvent::started);
        }
        if (state.value.cancellation_requested) { state.cancel(); return state.value.phase; }
        ++state.operation;
        state.callback_active = true;
        const auto result = state.work.at(state.value.current_stage).step(
            StartupStageProgress(state_, state.operation));
        state.callback_active = false;
        if (result != StartupStepResult::pending && result != StartupStepResult::completed)
            throw std::logic_error("Startup step returned an invalid result");
        if (result == StartupStepResult::completed) {
            // No synthetic units are filled in on return. If the owner supplied
            // a denominator, it must have reported every completed unit itself.
            if (stage.total_units && stage.completed_units != *stage.total_units)
                throw std::runtime_error("Startup stage returned with unfinished reported work");
            stage.phase = StartupStagePhase::completed;
            stage.finished_after = state.elapsed();
            if (!state.value.cancellation_requested && state.value.current_stage + 1 == state.work.size()) {
                state.stopped = std::chrono::steady_clock::now();
                state.value.phase = StartupLoadingPhase::completed;
            }
            state.notify(StartupLoadingEvent::completed);
        }
        // A returned step is the documented safe cancellation boundary.
        if (state.value.cancellation_requested) { state.cancel(); return state.value.phase; }
        if (result == StartupStepResult::completed) {
            if (state.value.current_stage + 1 != state.work.size()) ++state.value.current_stage;
        }
    } catch (const detail::StartupCancellationAtBoundary &) {
        state.callback_active = false;
        try { state.cancel(); } catch (const std::exception &error) { state.fail(error.what()); }
        catch (...) { state.fail("Startup cancellation observer failed"); }
    } catch (const std::exception &error) {
        state.callback_active = false; state.fail(error.what());
    } catch (...) {
        state.callback_active = false; state.fail("Unknown startup loading failure");
    }
    return state.value.phase;
}
} // namespace civ5::runtime
