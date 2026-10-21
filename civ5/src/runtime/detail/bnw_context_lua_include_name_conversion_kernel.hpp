#pragma once
#include <array>
#include <algorithm>
#include <string>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#endif
namespace civ5::runtime::detail {
// Pure qualified 4652F0/CvPath→6A6780 name conversion; no collection/source
// authority. Input is an owned VALUE, independently safe across new_handler.
// Bookends supplied by a genuine collector must fence every allocation/API.
template<class Bookend>
std::string context_lua_native_name_utf8_owned(std::u16string name,Bookend bookend){
    bookend();
    if(name.size()>259||name.find(u'\0')!=name.npos)
        throw std::runtime_error("Native Lua collector fixed260 path/NUL subset unsupported");
#ifdef _WIN32
    static_assert(sizeof(wchar_t)==sizeof(char16_t));
    std::array<wchar_t,260> path{};
    std::copy(name.begin(),name.end(),path.begin()); //exact fixed path plus true NUL
    const auto required=WideCharToMultiByte(CP_UTF8,0,path.data(),-1,nullptr,0,nullptr,nullptr);
    bookend();
    if(required<=0)throw std::runtime_error("Native Lua collector actual UTF8 sizing failed");
    std::string encoded;encoded.resize(static_cast<std::size_t>(required));bookend();
    const auto written=WideCharToMultiByte(CP_UTF8,0,path.data(),-1,encoded.data(),required,nullptr,nullptr);
    bookend();
    if(written!=required||encoded.back()!='\0')
        throw std::runtime_error("Native Lua collector actual UTF8 write changed/failed");
    encoded.pop_back();bookend();return encoded;
#else
    throw std::runtime_error("Native Lua collector CP_UTF8 conversion requires actual Win32 API");
#endif
}
}
