#pragma once
#include <cerrno>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

// Private OS observation backend. No persistent cursor, namespace verdict,
// watcher quiet bit, caller permission or prior pass metadata is retained.
namespace civ5::ui::native_fresh_find_detail {
inline bool unsupported_fetch(DWORD error)noexcept{
    return error==ERROR_INVALID_PARAMETER||error==ERROR_NOT_SUPPORTED||error==ERROR_INVALID_FUNCTION;
}
inline HANDLE first(const wchar_t* search,WIN32_FIND_DATAW* data){
    const DWORD saved_error=GetLastError();const int saved_errno=errno;
    // Standard preserves every field, including short-name data. Same wildcard
    // and name-match query; LARGE_FETCH changes only the query buffer strategy.
    const HANDLE handle=FindFirstFileExW(search,FindExInfoStandard,data,
        FindExSearchNameMatch,nullptr,FIND_FIRST_EX_LARGE_FETCH);
    if(handle!=INVALID_HANDLE_VALUE)return handle;
    const DWORD error=GetLastError();
    if(!unsupported_fetch(error)){SetLastError(error);return INVALID_HANDLE_VALUE;}
    // An unsupported implementation falls back to a NEW actual legacy query;
    // errors/absence are never converted to a successful old cached cursor.
    SetLastError(saved_error);errno=saved_errno;
    return FindFirstFileW(search,data);
}
inline HANDLE first(const char* search,WIN32_FIND_DATAA* data){
    const DWORD saved_error=GetLastError();const int saved_errno=errno;
    const HANDLE handle=FindFirstFileExA(search,FindExInfoStandard,data,
        FindExSearchNameMatch,nullptr,FIND_FIRST_EX_LARGE_FETCH);
    if(handle!=INVALID_HANDLE_VALUE)return handle;
    const DWORD error=GetLastError();
    if(!unsupported_fetch(error)){SetLastError(error);return INVALID_HANDLE_VALUE;}
    SetLastError(saved_error);errno=saved_errno;
    return FindFirstFileA(search,data);
}
}
