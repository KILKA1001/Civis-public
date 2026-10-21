#pragma once

#include "civ5/ui/ui_audio_assets.hpp"
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

namespace civ5::ui {

// A device owns a submitted buffer until release() has detached it from the OS.
// release(false) is called only after a completion signal; release(true) cancels.
// False from release means ownership must be retained to avoid freeing DMA data.
class IWaveOutDevice {
  public:
    virtual ~IWaveOutDevice() = default;
    virtual bool start(UiPcmBuffer buffer, std::string &error) = 0;
    virtual bool wait_for_progress(std::chrono::milliseconds timeout,
                                   std::string &error) = 0;
    virtual bool done() const = 0;
    virtual bool release(bool cancel, std::string &error) = 0;
};

std::unique_ptr<IWaveOutDevice> make_winmm_wave_out_device();

enum class WinMmEnqueueStatus { enqueued, invalid_format, queue_full, stopped };

struct WinMmAudioSnapshot {
    std::size_t queued = 0;
    bool active = false;
    std::uint64_t accepted = 0, completed = 0, failed = 0, canceled = 0;
    std::string last_error;
};

// submit() only queues a copy. The worker does every waveOut call and retains
// the PCM bytes through device completion. No device wait occurs in submit().
class WinMmPcmSink final : public UiPcmSink {
  public:
    using DeviceFactory = std::function<std::unique_ptr<IWaveOutDevice>()>;
    explicit WinMmPcmSink(std::size_t maximum_queued = 32,
                          DeviceFactory factory = make_winmm_wave_out_device);
    ~WinMmPcmSink() override;
    WinMmPcmSink(const WinMmPcmSink &) = delete;
    WinMmPcmSink &operator=(const WinMmPcmSink &) = delete;

    WinMmEnqueueStatus enqueue(const UiPcmBuffer &buffer);
    bool submit(const UiPcmBuffer &buffer) override {
        return enqueue(buffer) == WinMmEnqueueStatus::enqueued;
    }
    WinMmAudioSnapshot snapshot() const;
    //Serialize on the owning client thread, never on the worker. Stops queued
    //PCM, performs actual cancellation/release and joins. Repeat is idempotent.
    //last_error/failed preserve OS-release failure; completion is not invented.
    WinMmAudioSnapshot close();

  private:
    void run(std::stop_token stop);

    const std::size_t maximum_queued_;
    DeviceFactory factory_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<UiPcmBuffer> queue_;
    WinMmAudioSnapshot counters_;
    bool stopping_ = false;
    std::jthread worker_;
};

} // namespace civ5::ui
