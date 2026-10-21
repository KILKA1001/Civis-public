#include "civ5/ui/ui_winmm_audio.hpp"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <limits>
#include <stdexcept>
#include <optional>
#include <utility>

namespace civ5::ui {
namespace {

bool valid(const UiPcmBuffer &buffer) {
    const auto &format = buffer.format;
    if ((format.channels != 1 && format.channels != 2) ||
        (format.bits_per_sample != 8 && format.bits_per_sample != 16) ||
        !format.sample_rate || buffer.interleaved_samples.empty() ||
        buffer.interleaved_samples.size() > std::numeric_limits<DWORD>::max() ||
        format.data_bytes != buffer.interleaved_samples.size()) return false;
    const auto align = static_cast<std::uint32_t>(format.channels) *
                       (format.bits_per_sample / 8);
    return buffer.interleaved_samples.size() % align == 0 &&
           format.sample_rate <= std::numeric_limits<DWORD>::max() / align;
}

std::string winmm_error(const char *action, MMRESULT code) {
    char message[256]{};
    if (waveOutGetErrorTextA(code, message, sizeof(message)) == MMSYSERR_NOERROR)
        return std::string(action) + ": " + message;
    return std::string(action) + " failed (" + std::to_string(code) + ")";
}

class NativeWaveOutDevice final : public IWaveOutDevice {
  public:
    ~NativeWaveOutDevice() override {
        if (device_ || event_) {
            std::string ignored;
            // Normal callers release explicitly. A failed release must retain
            // this object instead of reaching this destructor.
            release(true, ignored);
        }
    }

    bool start(UiPcmBuffer buffer, std::string &error) override {
        if (!valid(buffer)) { error = "Invalid PCM format"; return false; }
        buffer_.emplace(std::move(buffer));
        const auto &pcm = buffer_->format;
        WAVEFORMATEX format{};
        format.wFormatTag = WAVE_FORMAT_PCM;
        format.nChannels = pcm.channels;
        format.nSamplesPerSec = pcm.sample_rate;
        format.wBitsPerSample = pcm.bits_per_sample;
        format.nBlockAlign = static_cast<WORD>(pcm.channels * (pcm.bits_per_sample / 8));
        format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
        event_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!event_) { error = "CreateEventW failed (" + std::to_string(GetLastError()) + ")"; return false; }
        const auto opened = waveOutOpen(&device_, WAVE_MAPPER, &format,
            reinterpret_cast<DWORD_PTR>(event_), 0, CALLBACK_EVENT);
        if (opened != MMSYSERR_NOERROR) {
            device_ = nullptr;
            error = winmm_error("waveOutOpen", opened);
            return false;
        }
        header_.lpData = reinterpret_cast<LPSTR>(buffer_->interleaved_samples.data());
        header_.dwBufferLength = static_cast<DWORD>(buffer_->interleaved_samples.size());
        const auto prepared = waveOutPrepareHeader(device_, &header_, sizeof(header_));
        if (prepared != MMSYSERR_NOERROR) {
            error = winmm_error("waveOutPrepareHeader", prepared);
            return false;
        }
        prepared_ = true;
        const auto written = waveOutWrite(device_, &header_, sizeof(header_));
        if (written != MMSYSERR_NOERROR) {
            error = winmm_error("waveOutWrite", written);
            return false;
        }
        written_ = true;
        return true;
    }

    bool wait_for_progress(std::chrono::milliseconds timeout,
                           std::string &error) override {
        const auto result = WaitForSingleObject(event_, static_cast<DWORD>(timeout.count()));
        if (result == WAIT_OBJECT_0 || result == WAIT_TIMEOUT) return true;
        error = "WinMM event wait failed (" + std::to_string(GetLastError()) + ")";
        return false;
    }

    bool done() const override { return written_ && (header_.dwFlags & WHDR_DONE); }

    bool release(bool cancel, std::string &error) override {
        if (device_) {
            if (cancel && written_ && !done()) {
                const auto reset = waveOutReset(device_);
                if (reset != MMSYSERR_NOERROR) {
                    error = winmm_error("waveOutReset", reset);
                    return false;
                }
            }
            if (prepared_) {
                const auto unprepared = waveOutUnprepareHeader(device_, &header_, sizeof(header_));
                if (unprepared != MMSYSERR_NOERROR) {
                    error = winmm_error("waveOutUnprepareHeader", unprepared);
                    return false;
                }
                prepared_ = false;
            }
            const auto closed = waveOutClose(device_);
            if (closed != MMSYSERR_NOERROR) {
                error = winmm_error("waveOutClose", closed);
                return false;
            }
            device_ = nullptr;
        }
        if (event_) {
            if(!CloseHandle(event_)) {
                error="WinMM CloseHandle failed ("+std::to_string(GetLastError())+")";
                return false; //Retain the genuine failed release owner.
            }
            event_ = nullptr;
        }
        buffer_.reset();
        return true;
    }

  private:
    std::optional<UiPcmBuffer> buffer_;
    HANDLE event_ = nullptr;
    HWAVEOUT device_ = nullptr;
    WAVEHDR header_{};
    bool prepared_ = false, written_ = false;
};

} // namespace

std::unique_ptr<IWaveOutDevice> make_winmm_wave_out_device() {
    return std::make_unique<NativeWaveOutDevice>();
}

WinMmPcmSink::WinMmPcmSink(std::size_t maximum_queued, DeviceFactory factory)
    : maximum_queued_(maximum_queued), factory_(std::move(factory)),
      worker_([this](std::stop_token stop) { run(stop); }) {}

WinMmPcmSink::~WinMmPcmSink() { (void)close(); }
WinMmAudioSnapshot WinMmPcmSink::close() {
    if(worker_.get_id()==std::this_thread::get_id())
        throw std::logic_error("WinMM worker cannot join itself");
    {
        std::lock_guard lock(mutex_);
        stopping_ = true;
    }
    worker_.request_stop();
    condition_.notify_all();
    if(worker_.joinable())worker_.join();
    return snapshot();
}

WinMmEnqueueStatus WinMmPcmSink::enqueue(const UiPcmBuffer &buffer) {
    if (!valid(buffer)) return WinMmEnqueueStatus::invalid_format;
    std::lock_guard lock(mutex_);
    if (stopping_) return WinMmEnqueueStatus::stopped;
    if (queue_.size() >= maximum_queued_) return WinMmEnqueueStatus::queue_full;
    queue_.push_back(buffer);
    ++counters_.accepted;
    condition_.notify_one();
    return WinMmEnqueueStatus::enqueued;
}

WinMmAudioSnapshot WinMmPcmSink::snapshot() const {
    std::lock_guard lock(mutex_);
    auto result = counters_;
    result.queued = queue_.size();
    return result;
}

void WinMmPcmSink::run(std::stop_token stop) {
    for (;;) {
        std::optional<UiPcmBuffer> buffer;
        {
            std::unique_lock lock(mutex_);
            condition_.wait(lock, [&] { return stopping_ || !queue_.empty(); });
            if (stopping_) {
                counters_.canceled += queue_.size();
                queue_.clear();
                return;
            }
            buffer.emplace(std::move(queue_.front()));
            queue_.pop_front();
            counters_.active = true;
        }
        bool completed = false, retained = false;
        std::string error;
        std::unique_ptr<IWaveOutDevice> device;
        try {
            device = factory_();
            if (!device) error = "Wave output device factory returned null";
            else {
                const bool started = device->start(std::move(*buffer), error);
                buffer.reset();
                if (started) {
                    while (!stop.stop_requested()) {
                        if (!device->wait_for_progress(std::chrono::milliseconds(20), error)) break;
                        if (device->done()) { completed = true; break; }
                    }
                }
            }
        } catch (const std::exception &exception) {
            error = exception.what();
        } catch (...) {
            error = "Unknown wave output exception";
        }
        if (device) {
            std::string release_error;
            try {
                if (!device->release(!completed, release_error)) retained = true;
            } catch (...) {
                retained = true;
                release_error = "Wave output cleanup threw an exception";
            }
            if (retained) {
                device.release(); // Keep OS-owned WAVEHDR and PCM alive on cleanup failure.
                if (release_error.empty()) release_error = "Wave output cleanup failed";
            }
            if (!release_error.empty()) error = std::move(release_error);
        }
        {
            std::lock_guard lock(mutex_);
            counters_.active = false;
            if (completed && !retained) ++counters_.completed;
            else if (stop.stop_requested()) ++counters_.canceled;
            else ++counters_.failed;
            if (!error.empty()) counters_.last_error = std::move(error);
        }
    }
}

} // namespace civ5::ui
