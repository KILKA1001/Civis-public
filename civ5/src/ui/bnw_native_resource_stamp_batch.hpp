#pragma once
#include "bnw_native_resource_stamp.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <exception>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

// Private metadata supplier only. Caller owns an immutable journal and all
// genuine Source/provider PRE/POST checks around this lexical call. Workers
// receive owning path operands, not Source/RootRead/HeaderGuard/currentness
// authority. All workers join before observations return to the creator.
namespace civ5::ui::native_resource_stamp_detail {
struct StampErrorState final {
    int number=errno;
#if defined(_WIN32)
    DWORD native=GetLastError();
#endif
    void restore()const noexcept {
#if defined(_WIN32)
        SetLastError(native);
#endif
        errno=number;
    }
};
class FreshStampObservation final {
    std::optional<Stamp> value_;
    std::exception_ptr failure_;
    StampErrorState error_;
public:
    explicit FreshStampObservation(Stamp value):value_(std::move(value)){}
    explicit FreshStampObservation(std::exception_ptr failure):failure_(std::move(failure)){}
    const Stamp& observation_or_throw()const {
        error_.restore();
        if(failure_)std::rethrow_exception(failure_);
        if(!value_)throw std::logic_error("Fresh resource stat observation absent");
        return *value_;
    }
};
inline std::vector<FreshStampObservation> serial_fresh_stamp_batch(std::span<const Stamp> pins) {
    std::vector<FreshStampObservation> out;out.reserve(pins.size());
    for(const auto& pin:pins) {
        try {out.emplace_back(stamp(pin.path));}
        catch(...) {out.emplace_back(std::current_exception());}
    }
    return out;
}
#if defined(_WIN32) && defined(_LIBCPP_VERSION) && _LIBCPP_VERSION == 230100 && _LIBCPP_HAS_INT128
namespace batch_private {
struct Row final {
    std::filesystem::path path; // Owned before workers start.
    std::optional<FreshStampObservation> observation;
    bool requires_serial=false;
};
// Forward every existing native operation exactly once. The marker is not
// an access/currentness verdict; it only selects serial reobservation where
// legacy fallback APIs can own ambient error state across successive paths.
struct TrackingBackend final {
    NativeBackend native;
    bool used_handle=false;
    bool by_name(const std::filesystem::path& path,FILE_STAT_INFORMATION& out){return native.by_name(path,out);}
    HANDLE open(const std::filesystem::path& path){used_handle=true;return native.open(path);}
    bool basic(HANDLE file,FILE_BASIC_INFO& out){return native.basic(file,out);}
    bool tag(HANDLE file,FILE_ATTRIBUTE_TAG_INFO& out){return native.tag(file,out);}
    bool standard(HANDLE file,FILE_STANDARD_INFO& out){return native.standard(file,out);}
    bool identity(HANDLE file,BY_HANDLE_FILE_INFORMATION& out){return native.identity(file,out);}
    DWORD error()const{return native.error();}
    bool close(HANDLE file)noexcept{return native.close(file);}
};
inline void merge_cell(native_stamp_diagnostic::Cell& dst,
        const native_stamp_diagnostic::Cell& src,bool same_frequency)noexcept {
    const auto add=[&](std::uint64_t& target,std::uint64_t value) {
        const auto max=std::numeric_limits<std::uint64_t>::max();
        if(value>max-target) {
            target=max;
            if(&target!=&dst.overflows&&dst.overflows!=max)++dst.overflows;
        }else target+=value;
    };
    add(dst.overflows,src.overflows);
    add(dst.calls,src.calls);add(dst.success,src.success);add(dst.failure,src.failure);
    add(dst.unreported,src.unreported);add(dst.abandoned,src.abandoned);
    add(dst.start_failures,src.start_failures);add(dst.end_failures,src.end_failures);
    add(dst.unavailable,src.unavailable);add(dst.regressions,src.regressions);
    if(same_frequency) {add(dst.samples,src.samples);add(dst.ticks,src.ticks);}
    else add(dst.unavailable,src.samples); // No mixed-frequency tick arithmetic.
}
inline void merge_collectors(native_stamp_diagnostic::Collector& owner,
        const std::array<native_stamp_diagnostic::Collector,8>& workers)noexcept {
    const auto max=std::numeric_limits<std::uint64_t>::max();
    for(const auto& worker:workers) {
        const bool same=worker.frequency==owner.frequency;
        const auto add_frequency=[&](std::uint64_t value) {
            if(value>max-owner.frequency_failures) {
                owner.frequency_failures=max;
                if(owner.cells.front().overflows!=max)++owner.cells.front().overflows;
            }else owner.frequency_failures+=value;
        };
        add_frequency(worker.frequency_failures);
        if(worker.frequency>0&&!same)add_frequency(1);
        for(std::size_t n=0;n<owner.cells.size();++n)
            merge_cell(owner.cells[n],worker.cells[n],same);
    }
}
// Each work item calls the ORIGINAL stamp helper. Its first path resolution
// and second independent path resolution stay successive within this path;
// the existing ordinary/by-name/error/reparse/handle fallback logic is intact.
inline bool run_paths(std::vector<Row>& rows,
        std::array<native_stamp_diagnostic::Collector,8>& reports,
        bool profiled,const StampErrorState& initial_error) {
    std::atomic<std::size_t> next{0};
    const auto count=std::min<std::size_t>(8,rows.size());
    std::vector<std::jthread> workers;workers.reserve(count);
    try {
        for(std::size_t n=0;n<count;++n)workers.emplace_back([&,lane=n]() noexcept {
            // Independent thread-local Collector; parent is never installed
            // here. No counter/timer state is shared between workers.
            std::optional<native_stamp_diagnostic::ScopedCollector> diagnostic;
            initial_error.restore();
            if(profiled)diagnostic.emplace(reports[lane]);
            for(;;) {
                const auto index=next.fetch_add(1,std::memory_order_relaxed);
                if(index>=rows.size())return;
                auto& row=rows[index];initial_error.restore();
                TrackingBackend backend;
                native_stamp_diagnostic::NativeClock clock;
                native_stamp_diagnostic::NativeError errors;
                try {
                    row.observation.emplace(observe_fresh_stamp(row.path,backend,
                        native_stamp_diagnostic::active,clock,errors));
                    row.requires_serial=backend.used_handle;
                }catch(...) {
                    row.requires_serial=true;
                    row.observation.emplace(std::current_exception());
                }
            }
        });
    }catch(const std::system_error&) {
        for(auto& worker:workers)worker.join();
        return false; // Scheduling grants nothing; owner reobserves serially.
    }
    for(auto& worker:workers)worker.join();
    return true;
}
}
#endif
inline std::vector<FreshStampObservation> fresh_two_target_stamp_batch(std::span<const Stamp> pins) {
#if defined(_WIN32) && defined(_LIBCPP_VERSION) && _LIBCPP_VERSION == 230100 && _LIBCPP_HAS_INT128
    if(pins.size()<64||!by_name_function())return serial_fresh_stamp_batch(pins);
    const StampErrorState initial_error;
    auto*const owner_collector=native_stamp_diagnostic::active;
    std::vector<batch_private::Row> rows;rows.reserve(pins.size());
    for(const auto& pin:pins)rows.push_back(batch_private::Row{pin.path});
    std::array<native_stamp_diagnostic::Collector,8> reports{};
    const bool returned=batch_private::run_paths(rows,reports,owner_collector!=nullptr,initial_error);
    // Join completed FIRST. Only the creator touches parent telemetry.
    if(owner_collector)batch_private::merge_collectors(*owner_collector,reports);
    initial_error.restore();
    if(!returned||std::any_of(rows.begin(),rows.end(),[](const auto& row){return row.requires_serial;}))
        return serial_fresh_stamp_batch(pins);
    std::vector<FreshStampObservation> out;out.reserve(rows.size());
    for(auto& row:rows) {
        if(!row.observation)throw std::logic_error("Joined resource stat worker omitted actual observation");
        out.push_back(std::move(*row.observation));
    }
    initial_error.restore();
    return out; // Actual values/errors only, not a currentness/completion receipt.
#else
    return serial_fresh_stamp_batch(pins);
#endif
}
}
