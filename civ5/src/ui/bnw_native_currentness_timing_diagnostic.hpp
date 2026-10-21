#pragma once
// Private DIAGNOSTIC ONLY. Inclusive durations grant no currentness/ownership.
#include <array>
#include <chrono>
#include <cerrno>
#include <cstdio>
#include <exception>
#include <cstdint>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "bnw_native_resource_stamp_diagnostic.hpp"
namespace civ5::ui::currentness_diagnostic {
enum class Lane : unsigned { package, settings, native_provider, count };
enum class Phase : unsigned { local_guards, lease_before, settings, unreadable,
 loose_walks, package_scan, payload_hashes, resource_checks, resource_stamps,
 lease_after, user_directory, ini_read, ini_parse, adapter_caps, provider_body, count };
inline constexpr const char* lane_names[]={"package.validate","settings.validate_source","test.native_provider"};
inline constexpr const char* phase_names[]={"local_guards","lease_before","settings","unreadable",
 "loose_walks","package_scan","payload_hashes","resource_checks","resource_stamps",
 "lease_after","user_directory","ini_read","ini_parse","adapter_caps","provider_body"};
struct Stats { std::uint64_t operations=0,unwound=0;std::array<std::uint64_t,static_cast<unsigned>(Phase::count)> calls{},us{}; };
inline thread_local std::array<Stats,static_cast<unsigned>(Lane::count)> stats{};
class Timing final {
 using Clock=std::chrono::steady_clock;
 Lane lane_;Phase phase_;Clock::time_point start_{};int exceptions_=0;bool active_=false;
 void record(Clock::time_point now)noexcept{
  auto& s=stats[static_cast<unsigned>(lane_)];const auto i=static_cast<unsigned>(phase_);
  ++s.calls[i];s.us[i]+=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now-start_).count());
 }
public:
 Timing(Lane lane,Phase phase)noexcept:lane_(lane),phase_(phase){
  const auto error=GetLastError();const auto saved_errno=errno;
  try{start_=Clock::now();exceptions_=std::uncaught_exceptions();active_=true;}catch(...){}
  SetLastError(error);errno=saved_errno;
 }
 Timing(const Timing&)=delete;Timing& operator=(const Timing&)=delete;
 void next(Phase phase)noexcept{
  const auto error=GetLastError();const auto saved_errno=errno;
  try{if(active_){const auto now=Clock::now();record(now);start_=now;phase_=phase;}}catch(...){}
  SetLastError(error);errno=saved_errno;
 }
 ~Timing()noexcept{
  const auto error=GetLastError();const auto saved_errno=errno;
  try{if(active_){record(Clock::now());auto& s=stats[static_cast<unsigned>(lane_)];
   ++s.operations;if(std::uncaught_exceptions()>exceptions_)++s.unwound;
   if(s.operations==1||s.operations%64==0){
    std::fprintf(stderr,"Currentness DIAGNOSTIC inclusive timing: lane=%s operations=%llu unwound=%llu",lane_names[static_cast<unsigned>(lane_)],static_cast<unsigned long long>(s.operations),static_cast<unsigned long long>(s.unwound));
    for(unsigned i=0;i<static_cast<unsigned>(Phase::count);++i)if(s.calls[i])
     std::fprintf(stderr," %s_calls=%llu %s_us=%llu",phase_names[i],static_cast<unsigned long long>(s.calls[i]),phase_names[i],static_cast<unsigned long long>(s.us[i]));
    std::fputc('\n',stderr);std::fflush(stderr);
    if(lane_==Lane::package)native_stamp_diagnostic::report();
   }
  }}catch(...){}
  SetLastError(error);errno=saved_errno;
 }
};
}
