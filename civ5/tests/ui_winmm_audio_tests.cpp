#include "civ5/ui/ui_winmm_audio.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>

namespace {
using namespace civ5;
using namespace std::chrono_literals;

void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

struct FakeState {
    std::mutex mutex;
    std::condition_variable condition;
    int starts = 0, normal_releases = 0, canceled_releases = 0;
    bool allow_completion = false, fail_start = false, throw_start = false;
    std::vector<std::uint8_t> observed_at_release;
};

class FakeDevice final : public ui::IWaveOutDevice {
  public:
    explicit FakeDevice(std::shared_ptr<FakeState> state) : state_(std::move(state)) {}
    bool start(ui::UiPcmBuffer buffer, std::string &error) override {
        buffer_.emplace(std::move(buffer));
        std::lock_guard lock(state_->mutex);
        ++state_->starts;
        state_->condition.notify_all();
        if (state_->throw_start) throw std::runtime_error("Fake start threw");
        if (state_->fail_start) { error = "Fake open failed"; return false; }
        return true;
    }
    bool wait_for_progress(std::chrono::milliseconds timeout,
                           std::string &) override {
        std::unique_lock lock(state_->mutex);
        state_->condition.wait_for(lock, timeout,
            [&] { return state_->allow_completion; });
        return true;
    }
    bool done() const override {
        std::lock_guard lock(state_->mutex);
        return state_->allow_completion;
    }
    bool release(bool cancel, std::string &) override {
        std::lock_guard lock(state_->mutex);
        if (buffer_) state_->observed_at_release = buffer_->interleaved_samples;
        if (cancel) ++state_->canceled_releases;
        else ++state_->normal_releases;
        buffer_.reset();
        state_->condition.notify_all();
        return true;
    }
  private:
    std::shared_ptr<FakeState> state_;
    std::optional<ui::UiPcmBuffer> buffer_;
};

template<class Predicate>
void await_state(const std::shared_ptr<FakeState> &state, Predicate predicate) {
    std::unique_lock lock(state->mutex);
    require(state->condition.wait_for(lock, 2s, predicate),
            "Timed out waiting for fake audio device");
}

ui::UiPcmBuffer pcm() {
    return {ui::UiPcmFormat{1, 8, 8000, 44, 4}, {0, 64, 128, 255},
            assets::Provenance{assets::SourceKind::loose, "test", "Test.wav", {}}};
}

} // namespace

int main() {
    try {
        auto state = std::make_shared<FakeState>();
        {
            ui::WinMmPcmSink sink(1, [state] { return std::make_unique<FakeDevice>(state); });
            auto first = pcm();
            require(sink.enqueue(first) == ui::WinMmEnqueueStatus::enqueued,
                    "First PCM buffer queues");
            first.interleaved_samples.assign(4, 9);
            await_state(state, [&] { return state->starts == 1; });
            require(sink.enqueue(pcm()) == ui::WinMmEnqueueStatus::enqueued,
                    "Second PCM buffer occupies the one-slot pending queue");
            require(sink.enqueue(pcm()) == ui::WinMmEnqueueStatus::queue_full,
                    "Bounded pending queue reports overflow");
            auto invalid = pcm();
            invalid.format.bits_per_sample = 24;
            require(sink.enqueue(invalid) == ui::WinMmEnqueueStatus::invalid_format,
                    "Unsupported sample format is rejected before device open");
            {
                std::lock_guard lock(state->mutex);
                state->allow_completion = true;
            }
            state->condition.notify_all();
            await_state(state, [&] { return state->normal_releases == 2; });
            const auto snapshot = sink.snapshot();
            require(snapshot.accepted == 2 && snapshot.completed == 2 &&
                    snapshot.failed == 0 && snapshot.queued == 0 &&
                    state->observed_at_release == pcm().interleaved_samples,
                    "Copied PCM remains intact until both device completions");
        }

        state = std::make_shared<FakeState>();
        state->fail_start = true;
        {
            ui::WinMmPcmSink sink(1, [state] { return std::make_unique<FakeDevice>(state); });
            require(sink.enqueue(pcm()) == ui::WinMmEnqueueStatus::enqueued,
                    "Async device error follows accepted queue entry");
            await_state(state, [&] { return state->canceled_releases == 1; });
            for (int i = 0; i < 100 && sink.snapshot().failed == 0; ++i)
                std::this_thread::sleep_for(1ms);
            require(sink.snapshot().failed == 1 &&
                    sink.snapshot().last_error == "Fake open failed",
                    "Device open error is visible in async status");
        }

        state = std::make_shared<FakeState>();
        state->throw_start = true;
        {
            ui::WinMmPcmSink sink(1, [state] { return std::make_unique<FakeDevice>(state); });
            require(sink.enqueue(pcm()) == ui::WinMmEnqueueStatus::enqueued,
                    "Queued PCM survives a device exception");
            await_state(state, [&] { return state->canceled_releases == 1; });
            for (int i = 0; i < 100 && sink.snapshot().failed == 0; ++i)
                std::this_thread::sleep_for(1ms);
            require(sink.snapshot().failed == 1 &&
                    sink.snapshot().last_error == "Fake start threw" &&
                    state->observed_at_release == pcm().interleaved_samples,
                    "Exception cleanup retains PCM until device release");
        }

        state = std::make_shared<FakeState>();
        {
            ui::WinMmPcmSink sink(1, [state] { return std::make_unique<FakeDevice>(state); });
            require(sink.enqueue(pcm()) == ui::WinMmEnqueueStatus::enqueued,
                    "Pending audio queues before shutdown");
            await_state(state, [&] { return state->starts == 1; });
        }
        require(state->canceled_releases == 1 &&
                state->observed_at_release == pcm().interleaved_samples,
                "Shutdown cancels device while retaining PCM until release");

        std::cout << "WinMM PCM backend tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
