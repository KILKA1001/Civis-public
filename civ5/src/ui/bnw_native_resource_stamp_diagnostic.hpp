#pragma once
// Private opt-in telemetry only. No metadata verdict or currentness authority.
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <filesystem>
#include <type_traits>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
namespace civ5::ui::native_stamp_diagnostic {
enum class Pass : unsigned { size, time, count };
enum class Step : unsigned { open, basic, tag, standard, identity, close, by_name, count };
enum class Outcome { success, failure, unreported, abandoned };
struct Cell {
    std::uint64_t calls=0,success=0,failure=0,unreported=0,abandoned=0;
    std::uint64_t samples=0,ticks=0,start_failures=0,end_failures=0;
    std::uint64_t unavailable=0,regressions=0,overflows=0;
};
struct Collector {
    std::array<Cell,static_cast<unsigned>(Pass::count)*static_cast<unsigned>(Step::count)> cells{};
    std::int64_t frequency=0;
    std::uint64_t frequency_failures=0;
    Cell& cell(Pass pass,Step step)noexcept {
        return cells[static_cast<unsigned>(pass)*static_cast<unsigned>(Step::count)+static_cast<unsigned>(step)];
    }
};
inline thread_local Collector* active=nullptr;
struct NativeError {
    DWORD get()const noexcept{return GetLastError();}
    void set(DWORD value)const noexcept{SetLastError(value);}
};
struct NativeClock {
    bool tick(std::int64_t& value)const noexcept {
        LARGE_INTEGER observed{};const bool ok=QueryPerformanceCounter(&observed)!=0;
        value=observed.QuadPart;return ok;
    }
};
class ScopedCollector final {
    Collector* previous_;
public:
    explicit ScopedCollector(Collector& collector)noexcept:previous_(active) {
        const auto error=GetLastError();const int saved_errno=errno;
        LARGE_INTEGER frequency{};
        if(QueryPerformanceFrequency(&frequency)&&frequency.QuadPart>0)
            collector.frequency=frequency.QuadPart;
        else {collector.frequency=0;++collector.frequency_failures;}
        SetLastError(error);errno=saved_errno;
        active=&collector;
    }
    ~ScopedCollector()noexcept{active=previous_;}
    ScopedCollector(const ScopedCollector&)=delete;
    ScopedCollector& operator=(const ScopedCollector&)=delete;
};
template<class Clock,class Errors> class Measurement final {
    Cell& cell_;Collector& collector_;Clock& clock_;Errors& errors_;
    std::int64_t begin_=0;bool began_=false,finished_=false;
public:
    Measurement(Collector& collector,Pass pass,Step step,Clock& clock,Errors& errors)noexcept
        :cell_(collector.cell(pass,step)),collector_(collector),clock_(clock),errors_(errors) {
        const auto error=errors_.get();const int saved_errno=errno;
        if(collector_.frequency<=0)++cell_.unavailable;
        else {began_=clock_.tick(begin_);if(!began_)++cell_.start_failures;}
        errors_.set(error);errno=saved_errno;
    }
    void finish(Outcome outcome)noexcept {
        if(finished_)return;
        // Preserve the error state produced by the ORIGINAL API, including
        // unsuccessful open/query/close. Never restore its pre-call state here.
        const auto error=errors_.get();const int saved_errno=errno;
        ++cell_.calls;
        if(outcome==Outcome::success)++cell_.success;
        else if(outcome==Outcome::failure)++cell_.failure;
        else if(outcome==Outcome::unreported)++cell_.unreported;
        else ++cell_.abandoned;
        if(began_) {
            std::int64_t end=0;
            if(!clock_.tick(end))++cell_.end_failures;
            else if(begin_<0||end<begin_)++cell_.regressions;
            else {
                const auto elapsed=static_cast<std::uint64_t>(end-begin_);
                if(elapsed>std::numeric_limits<std::uint64_t>::max()-cell_.ticks)++cell_.overflows;
                else {cell_.ticks+=elapsed;++cell_.samples;}
            }
        }
        finished_=true;errors_.set(error);errno=saved_errno;
    }
    ~Measurement()noexcept{if(!finished_)finish(Outcome::abandoned);}
    Measurement(const Measurement&)=delete;Measurement& operator=(const Measurement&)=delete;
};
// Production instantiates ONLY NativeBackend/NativeClock/NativeError. This
// adapter forwards each original method exactly once and holds no OS handle.
// The fixture supplies scripted backend/timer responses to the SAME adapter.
template<class Backend,class Clock,class Errors> class ProfiledBackend final {
    Backend& backend_;Collector& collector_;Clock& clock_;Errors& errors_;
    unsigned opens_=0;Pass pass_=Pass::size;bool explicit_pass_=false;
public:
    ProfiledBackend(Backend& backend,Collector& collector,Clock& clock,Errors& errors)noexcept
        :backend_(backend),collector_(collector),clock_(clock),errors_(errors){}
    // Explicit pass selection is used only by the new two-query production
    // observer; the existing scripted/original observer keeps open-order labels.
    void select_pass(Pass pass)noexcept{pass_=pass;explicit_pass_=true;}
    bool by_name(const std::filesystem::path& path,FILE_STAT_INFORMATION& out) {
        Measurement<Clock,Errors> measured(collector_,pass_,Step::by_name,clock_,errors_);
        const bool result=backend_.by_name(path,out);
        measured.finish(result?Outcome::success:Outcome::failure);return result;
    }
    HANDLE open(const std::filesystem::path& path) {
        if(!explicit_pass_)pass_=opens_++==0?Pass::size:Pass::time;
        Measurement<Clock,Errors> measured(collector_,pass_,Step::open,clock_,errors_);
        const auto result=backend_.open(path);
        measured.finish(result==INVALID_HANDLE_VALUE?Outcome::failure:Outcome::success);return result;
    }
    bool basic(HANDLE file,FILE_BASIC_INFO& out) {
        Measurement<Clock,Errors> measured(collector_,pass_,Step::basic,clock_,errors_);
        const bool result=backend_.basic(file,out);measured.finish(result?Outcome::success:Outcome::failure);return result;
    }
    bool tag(HANDLE file,FILE_ATTRIBUTE_TAG_INFO& out) {
        Measurement<Clock,Errors> measured(collector_,pass_,Step::tag,clock_,errors_);
        const bool result=backend_.tag(file,out);measured.finish(result?Outcome::success:Outcome::failure);return result;
    }
    bool standard(HANDLE file,FILE_STANDARD_INFO& out) {
        Measurement<Clock,Errors> measured(collector_,pass_,Step::standard,clock_,errors_);
        const bool result=backend_.standard(file,out);measured.finish(result?Outcome::success:Outcome::failure);return result;
    }
    bool identity(HANDLE file,BY_HANDLE_FILE_INFORMATION& out) {
        Measurement<Clock,Errors> measured(collector_,pass_,Step::identity,clock_,errors_);
        const bool result=backend_.identity(file,out);measured.finish(result?Outcome::success:Outcome::failure);return result;
    }
    DWORD error()const{return backend_.error();}
    void close(HANDLE file)noexcept(noexcept(backend_.close(file))) {
        Measurement<Clock,Errors> measured(collector_,pass_,Step::close,clock_,errors_);
        if constexpr(std::is_void_v<decltype(backend_.close(file))>) {
            backend_.close(file);measured.finish(Outcome::unreported);
        } else {
            const bool result=backend_.close(file);measured.finish(result?Outcome::success:Outcome::failure);
        }
    }
};
inline void report()noexcept {
    const auto* collector=active;if(!collector)return;
    const auto error=GetLastError();const int saved_errno=errno;
    // Output is called only by the test's existing coarse progress reporter,
    // outside the stat API chain. No callbacks, per-path data or authority.
    static constexpr const char* passes[] = {"file_size","last_write_time"};
    static constexpr const char* steps[] = {"open","basic","tag","standard","identity","close","by_name"};
    for(unsigned p=0;p<static_cast<unsigned>(Pass::count);++p)
        for(unsigned s=0;s<static_cast<unsigned>(Step::count);++s) {
            const auto& c=collector->cells[p*static_cast<unsigned>(Step::count)+s];if(!c.calls)continue;
            std::fprintf(stderr,"Stamp SUBSTEP DIAGNOSTIC pass=%s step=%s frequency=%lld frequency_failures=%llu calls=%llu success=%llu failure=%llu unreported=%llu abandoned=%llu samples=%llu ticks=%llu start_failures=%llu end_failures=%llu unavailable=%llu regressions=%llu overflows=%llu\n",
                passes[p],steps[s],static_cast<long long>(collector->frequency),
                static_cast<unsigned long long>(collector->frequency_failures),
                static_cast<unsigned long long>(c.calls),static_cast<unsigned long long>(c.success),
                static_cast<unsigned long long>(c.failure),static_cast<unsigned long long>(c.unreported),
                static_cast<unsigned long long>(c.abandoned),static_cast<unsigned long long>(c.samples),
                static_cast<unsigned long long>(c.ticks),static_cast<unsigned long long>(c.start_failures),
                static_cast<unsigned long long>(c.end_failures),static_cast<unsigned long long>(c.unavailable),
                static_cast<unsigned long long>(c.regressions),static_cast<unsigned long long>(c.overflows));
        }
    std::fflush(stderr);SetLastError(error);errno=saved_errno;
}
}
