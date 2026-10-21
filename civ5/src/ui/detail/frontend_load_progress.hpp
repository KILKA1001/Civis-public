#pragma once
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <string_view>
#include <utility>

namespace civ5::ui::detail {
// Presentation/diagnostics only. Neither an elapsed time nor an emitted stage
// grants readiness. The actual load owners still perform all source checks.
class FrontendLoadProgress {
    using Clock = std::chrono::steady_clock;
    std::function<void(std::string_view)> observer_;
    Clock::time_point started_ = Clock::now();
    std::ofstream journal_;
    std::string stage_;
    bool polling_ = false;
public:
    explicit FrontendLoadProgress(std::function<void(std::string_view)> observer)
        : observer_(std::move(observer)) {}
    void open(const std::filesystem::path& directory) {
        journal_.open(directory / "loading-stages.log", std::ios::out);
        started_ = Clock::now();
    }
    void stage(std::string_view value) {
        stage_.assign(value);
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - started_).count();
        std::fprintf(stderr, "[CIVIS_LOAD] stage=%s elapsed_ms=%lld\n",
            stage_.c_str(), static_cast<long long>(elapsed));
        std::fflush(stderr);
        if (journal_) {
            journal_ << elapsed << '\t' << stage_ << '\n';
            journal_.flush();
        }
        poll();
    }
    void poll() {
        if (polling_ && observer_) observer_(stage_);
    }
    void enable_polling() { polling_ = true; }
    void disable_polling() { polling_ = false; }
    void failed() noexcept {
        polling_ = false;
        try { stage("failed"); } catch (...) {}
    }
};
}
