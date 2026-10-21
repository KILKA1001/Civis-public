#pragma once
// PRIVATE DIAGNOSTIC ONLY: observations confer no source/owner/verdict authority.
#include <atomic>
#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstdint>
#include <exception>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace civ5::ui::namespace_phase_diagnostic {
class AmbientError final {
    const int number_=errno;
    const DWORD native_=GetLastError();
public:
    AmbientError()noexcept=default;
    ~AmbientError()noexcept{SetLastError(native_);errno=number_;}
    AmbientError(const AmbientError&)=delete;
    AmbientError& operator=(const AmbientError&)=delete;
};
// This counter identifies log records only; it is never read by source logic.
inline std::atomic<std::uint64_t> next_operation{1};
class Operation final {
    std::uint64_t id_=0;
    const long long ordinal_;
public:
    explicit Operation(long long ordinal)noexcept:ordinal_(ordinal) {
        AmbientError errors;
        id_=next_operation.fetch_add(1,std::memory_order_relaxed);
    }
    ~Operation()noexcept=default;
    Operation(const Operation&)=delete;
    Operation& operator=(const Operation&)=delete;
    std::uint64_t id()const noexcept{return id_;}
    long long ordinal()const noexcept{return ordinal_;}
};
class PhaseSpan final {
    using Clock=std::chrono::steady_clock;
    const Operation* const operation_;
    const char* const phase_;
    Clock::time_point begin_{};
    int exceptions_=0;
    bool active_=false;
public:
    PhaseSpan(const Operation* operation,const char* phase)noexcept
        :operation_(operation),phase_(phase) {
        // A null diagnostic operand does not clock, format, log or issue IDs.
        if(!operation_)return;
        AmbientError errors; // Covers clock/exception sampling AND sink calls.
        try {
            begin_=Clock::now();exceptions_=std::uncaught_exceptions();
            active_=true;
            const auto stamp=std::chrono::duration_cast<std::chrono::milliseconds>(begin_.time_since_epoch()).count();
            std::fprintf(stderr,"[BNW_NAMESPACE_DIAG] BEGIN Installed.receiver.namespace_full_post operation=%llu ordinal=%lld phase=%s steady_ms=%lld outcome=observing\n",
                static_cast<unsigned long long>(operation_->id()),operation_->ordinal(),phase_,static_cast<long long>(stamp));
            std::fflush(stderr); // Best effort: sink failures never become source failures.
        }catch(...){}
    }
    void finish()noexcept {
        if(!active_)return;
        // Capture the CURRENT working error at logging entry, not the phase's
        // starting error. Actual native/stat/hash error provenance stays intact.
        AmbientError errors;
        active_=false;
        try {
            const auto now=Clock::now();
            const auto stamp=std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            const bool monotonic=now>=begin_;
            const auto elapsed=monotonic?
                std::chrono::duration_cast<std::chrono::microseconds>(now-begin_).count():-1;
            const auto* outcome=std::uncaught_exceptions()>exceptions_?"unwind":"normal";
            std::fprintf(stderr,"[BNW_NAMESPACE_DIAG] END Installed.receiver.namespace_full_post operation=%llu ordinal=%lld phase=%s steady_ms=%lld elapsed_us=%lld clock_valid=%d outcome=%s\n",
                static_cast<unsigned long long>(operation_->id()),operation_->ordinal(),phase_,
                static_cast<long long>(stamp),static_cast<long long>(elapsed),monotonic?1:0,outcome);
            std::fflush(stderr);
        }catch(...){}
    }
    ~PhaseSpan()noexcept{finish();}
    PhaseSpan(const PhaseSpan&)=delete;
    PhaseSpan& operator=(const PhaseSpan&)=delete;
};
}
