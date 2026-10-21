#pragma once

#include <filesystem>

namespace civ5::platform {
// Win32 file APIs need the extended prefix for deep mod trees. The prefix
// requires an absolute path with Windows separators.
inline std::filesystem::path extended_windows_path(const std::filesystem::path &input) {
    const auto &source = input.native();
    if (source.starts_with(L"\\\\?\\") || source.starts_with(L"\\\\.\\"))
        return input;
    auto absolute = std::filesystem::absolute(input).lexically_normal();
    absolute.make_preferred();
    const auto &native = absolute.native();
    if (native.starts_with(L"\\\\?\\") || native.starts_with(L"\\\\.\\"))
        return absolute;
    if (native.starts_with(L"\\\\"))
        return std::filesystem::path(L"\\\\?\\UNC\\" + native.substr(2));
    return std::filesystem::path(L"\\\\?\\" + native);
}
inline std::filesystem::path unextended_windows_path(const std::filesystem::path &input) {
    const auto &native=input.native();
    if(native.starts_with(L"\\\\?\\UNC\\"))
        return std::filesystem::path(L"\\\\"+native.substr(8));
    if(native.starts_with(L"\\\\?\\"))
        return std::filesystem::path(native.substr(4));
    return input;
}
} // namespace civ5::platform
