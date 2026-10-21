#pragma once
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <system_error>
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winioctl.h>
#include "bnw_native_resource_stamp_diagnostic.hpp"
#endif

// Private metadata observation only. This is no filesystem freeze, cached
// currentness receipt, resource owner or issuer. Every caller still observes
// each retained path on every existing validation and preserves its bookends.
namespace civ5::ui::native_resource_stamp_detail {
struct Stamp {
    std::filesystem::path path;
    std::uintmax_t size;
    std::filesystem::file_time_type written;
    bool operator==(const Stamp&) const = default;
};
inline Stamp legacy_stamp(const std::filesystem::path& path) {
    return {path,std::filesystem::file_size(path),std::filesystem::last_write_time(path)};
}

// Installed libc++230100 uses nanosecond/int128 file_clock with Unix epoch.
// Other library backends retain their own exact filesystem implementation.
#if defined(_WIN32) && defined(_LIBCPP_VERSION) && _LIBCPP_VERSION == 230100 && _LIBCPP_HAS_INT128
inline std::filesystem::file_time_type filetime_clock(std::uint64_t ticks) {
    using Clock=std::chrono::file_clock;
    using Rep=std::filesystem::file_time_type::duration::rep;
    static_assert(std::filesystem::file_time_type::duration::period::num==1 &&
                  std::filesystem::file_time_type::duration::period::den==1000000000);
    static_assert(sizeof(Rep)>sizeof(std::uint64_t));
    // FILETIME counts 100ns since1601; the full unsigned payload fits Rep.
    // from_sys is public and retains the actual file_clock epoch mapping.
    const auto nanos=(static_cast<Rep>(ticks)-static_cast<Rep>(116444736000000000ULL))*100;
    return Clock::from_sys(std::chrono::sys_time<std::filesystem::file_time_type::duration>{
        std::filesystem::file_time_type::duration{nanos}});
}
// The current SDK has the native FILE_STAT_INFORMATION layout, but exposes
// the API/enum only for NTDDI_WIN11_ZN. Resolve it at runtime without changing
// the application's minimum Windows target or adding a mandatory import.
enum class ByNameClass : int { stat = 0 }; // SDK FileStatByNameInfo == 0.
using ByNameFunction = BOOL (WINAPI*)(PCWSTR,ByNameClass,PVOID,ULONG);
static_assert(sizeof(FILE_STAT_INFORMATION)==72);
static_assert(offsetof(FILE_STAT_INFORMATION,LastWriteTime)==24);
static_assert(offsetof(FILE_STAT_INFORMATION,EndOfFile)==48);
static_assert(offsetof(FILE_STAT_INFORMATION,FileAttributes)==56);
static_assert(offsetof(FILE_STAT_INFORMATION,EffectiveAccess)==68);
inline ByNameFunction by_name_function() noexcept {
    // Only an OS function address is retained. No file metadata, verdict,
    // handle, permission, path or currentness receipt survives a pass.
    static const auto function=[]() noexcept -> ByNameFunction {
        const auto saved_error=GetLastError();const int saved_errno=errno;
        const auto module=GetModuleHandleW(L"kernel32.dll");
        const auto found=module?GetProcAddress(module,"GetFileInformationByName"):nullptr;
        SetLastError(saved_error);errno=saved_errno;
        return reinterpret_cast<ByNameFunction>(found);
    }();
    return function;
}
struct NativeBackend {
    bool by_name(const std::filesystem::path& path,FILE_STAT_INFORMATION& out) {
        const auto function=by_name_function();
        if(!function)return false;
        return function(path.c_str(),ByNameClass::stat,&out,sizeof(out))!=0;
    }
    HANDLE open(const std::filesystem::path& path) {
        return CreateFileW(path.c_str(),FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,
            OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr);
    }
    bool basic(HANDLE file,FILE_BASIC_INFO& out){return GetFileInformationByHandleEx(file,FileBasicInfo,&out,sizeof(out))!=0;}
    bool tag(HANDLE file,FILE_ATTRIBUTE_TAG_INFO& out){return GetFileInformationByHandleEx(file,FileAttributeTagInfo,&out,sizeof(out))!=0;}
    bool standard(HANDLE file,FILE_STANDARD_INFO& out){return GetFileInformationByHandleEx(file,FileStandardInfo,&out,sizeof(out))!=0;}
    bool identity(HANDLE file,BY_HANDLE_FILE_INFORMATION& out){return GetFileInformationByHandle(file,&out)!=0;}
    DWORD error()const{return GetLastError();}
    bool close(HANDLE file)noexcept{return CloseHandle(file)!=0;}
};
[[noreturn]] inline void backend_error(const std::filesystem::path& path,DWORD code,const char* operation) {
    // Keep the label of the independent failing filesystem operation.
    throw std::filesystem::filesystem_error(operation,path,
        std::error_code(static_cast<int>(code),std::system_category()));
}
// The test backend scripts only OS responses/order/errors. Production always
// instantiates NativeBackend; this helper conveys no owner/currentness proof.
struct StatObservation {
    std::uintmax_t size;
    std::uint64_t written;
    bool symbolic,directory;
};
template<class Backend> StatObservation stat_pass(const std::filesystem::path& path,Backend& backend,const char* operation) {
    const HANDLE file=backend.open(path);
    if(file==INVALID_HANDLE_VALUE)backend_error(path,backend.error(),operation);
    struct Close {
        Backend& backend;HANDLE file;
        ~Close(){backend.close(file);}
    } close{backend,file};
    FILE_BASIC_INFO basic{};
    if(!backend.basic(file,basic))backend_error(path,backend.error(),operation);
    bool symbolic=false;
    if(basic.FileAttributes&FILE_ATTRIBUTE_REPARSE_POINT) {
        FILE_ATTRIBUTE_TAG_INFO tag{};
        if(!backend.tag(file,tag))backend_error(path,backend.error(),operation);
        symbolic=tag.ReparseTag==IO_REPARSE_TAG_SYMLINK;
    }
    FILE_STANDARD_INFO standard{};
    if(!backend.standard(file,standard))backend_error(path,backend.error(),operation);
    BY_HANDLE_FILE_INFORMATION identity{};
    if(!backend.identity(file,identity))backend_error(path,backend.error(),operation);
    return {static_cast<std::uintmax_t>(standard.EndOfFile.QuadPart),
        static_cast<std::uint64_t>(basic.LastWriteTime.QuadPart),symbolic,
        (basic.FileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0};
}
template<class Backend> Stamp two_target_stamp(const std::filesystem::path& path,Backend& backend) {
    // Preserve BOTH stat passes and their separate open/close lifetimes. Use
    // the exact original path; libc++ posix_stat passes p.c_str() to Win32.
    const auto size_pass=stat_pass(path,backend,"in file_size");
    if(size_pass.symbolic || size_pass.directory)
        throw std::filesystem::filesystem_error("in file_size",path,
            std::make_error_code(!size_pass.symbolic && size_pass.directory?
                std::errc::is_a_directory:std::errc::not_supported));
    // last_write_time queries all fields, but does not require a regular
    // target. A rebind to a directory between passes retains old behavior.
    const auto time_pass=stat_pass(path,backend,"in last_write_time");
    return {path,size_pass.size,filetime_clock(time_pass.written)};
}
template<class Backend,class Clock,class Errors> Stamp observe_stamp(
    const std::filesystem::path& path,Backend& backend,
    native_stamp_diagnostic::Collector* collector,Clock& clock,Errors& errors) {
    // Null is the default: no timer/error-state operation or adapter is used.
    if(!collector)return two_target_stamp(path,backend);
    native_stamp_diagnostic::ProfiledBackend<Backend,Clock,Errors> observed(backend,*collector,clock,errors);
    return two_target_stamp(path,observed);
}
// TWO independent path queries remain: size observes the first pass and time
// observes the second. Rebinds between passes still resolve the original path.
// A fast observation is not an authority to bypass any package/lease checks.
template<class Backend> StatObservation fresh_stat_pass(
    const std::filesystem::path& path,Backend& backend,const char* operation,
    native_stamp_diagnostic::Pass pass) {
    if constexpr(requires { backend.select_pass(pass); })backend.select_pass(pass);
    const auto saved_error=GetLastError();const int saved_errno=errno;
    FILE_STAT_INFORMATION observed{};
    const auto& native=path.native();
    // Ambiguous DOS devices, UNC/extended namespaces, relative names and ADS
    // retain the original handle path. This chooses implementation, not domain.
    const bool dos_path=native.size()>2 &&
        ((native[0]>=L'A'&&native[0]<=L'Z') || (native[0]>=L'a'&&native[0]<=L'z')) &&
        native[1]==L':' && (native[2]==L'\\'||native[2]==L'/') &&
        native.find(L':',2)==std::wstring::npos;
    const bool queried=dos_path && backend.by_name(path,observed);
    // GetFileInformationByName can report link metadata. Preserve the complete
    // old handle path for ALL reparse points, directories, unusual attributes,
    // unproven access, failure and absent API. No unsupported-domain rejection.
    constexpr DWORD ordinary=FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_HIDDEN|
        FILE_ATTRIBUTE_SYSTEM|FILE_ATTRIBUTE_ARCHIVE|FILE_ATTRIBUTE_NORMAL|
        FILE_ATTRIBUTE_TEMPORARY|FILE_ATTRIBUTE_SPARSE_FILE|FILE_ATTRIBUTE_COMPRESSED|
        FILE_ATTRIBUTE_OFFLINE|FILE_ATTRIBUTE_NOT_CONTENT_INDEXED|FILE_ATTRIBUTE_ENCRYPTED;
    const bool ordinary_file=queried && observed.FileAttributes!=0 &&
        (observed.FileAttributes&~ordinary)==0 && observed.ReparseTag==0 &&
        (observed.EffectiveAccess&FILE_READ_ATTRIBUTES)!=0 &&
        observed.NumberOfLinks!=0 && observed.EndOfFile.QuadPart>=0;
    // Failed/ambiguous optional queries must not alter the original fallback's
    // input error state. The fallback preserves its original open/query labels.
    SetLastError(saved_error);errno=saved_errno;
    if(!ordinary_file)return stat_pass(path,backend,operation);
    return {static_cast<std::uintmax_t>(observed.EndOfFile.QuadPart),
        static_cast<std::uint64_t>(observed.LastWriteTime.QuadPart),false,false};
}
template<class Backend> Stamp fresh_two_target_stamp(
    const std::filesystem::path& path,Backend& backend) {
    const auto size_pass=fresh_stat_pass(path,backend,"in file_size",native_stamp_diagnostic::Pass::size);
    if(size_pass.symbolic || size_pass.directory)
        throw std::filesystem::filesystem_error("in file_size",path,
            std::make_error_code(!size_pass.symbolic && size_pass.directory?
                std::errc::is_a_directory:std::errc::not_supported));
    const auto time_pass=fresh_stat_pass(path,backend,"in last_write_time",native_stamp_diagnostic::Pass::time);
    return {path,size_pass.size,filetime_clock(time_pass.written)};
}
template<class Backend,class Clock,class Errors> Stamp observe_fresh_stamp(
    const std::filesystem::path& path,Backend& backend,
    native_stamp_diagnostic::Collector* collector,Clock& clock,Errors& errors) {
    if(!collector)return fresh_two_target_stamp(path,backend);
    native_stamp_diagnostic::ProfiledBackend<Backend,Clock,Errors> observed(backend,*collector,clock,errors);
    return fresh_two_target_stamp(path,observed);
}
#endif
inline Stamp stamp(const std::filesystem::path& path) {
#if defined(_WIN32) && defined(_LIBCPP_VERSION) && _LIBCPP_VERSION == 230100 && _LIBCPP_HAS_INT128
    NativeBackend backend;native_stamp_diagnostic::NativeClock clock;native_stamp_diagnostic::NativeError errors;
    return observe_fresh_stamp(path,backend,native_stamp_diagnostic::active,clock,errors);
#else
    return legacy_stamp(path);
#endif
}
}
