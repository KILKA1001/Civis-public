#include "civ5/platform/http_download.hpp"
#include "http_curl.hpp"
#include "civ5/content/mod_archive.hpp"
#include "civ5/runtime/binary_reader.hpp"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#include <array>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

namespace civ5::platform {
namespace {
void require(bool ok, const char *operation) {
    if (!ok)
        throw std::runtime_error(std::string(operation) + " failed (Win32 " + std::to_string(GetLastError()) +
                                 ")");
}
std::wstring wide(const std::string &text) {
    if (!runtime::valid_utf8(text) || text.find('\0') != std::string::npos || text.size() > INT_MAX)
        throw std::runtime_error("Invalid HTTP UTF-8 field");
    const auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                           static_cast<int>(text.size()), nullptr, 0);
    require(count != 0, "Convert URL");
    std::wstring result(static_cast<std::size_t>(count), '\0');
    require(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
                                result.data(), count) == count,
            "Convert URL");
    return result;
}
std::string utf8(const std::wstring &text) {
    const auto count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
                                           static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    require(count != 0, "Convert final URL");
    std::string result(static_cast<std::size_t>(count), '\0');
    require(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
                                result.data(), count, nullptr, nullptr) == count,
            "Convert final URL");
    return result;
}
struct InternetClose {
    void operator()(void *handle) const {
        if (handle)
            WinHttpCloseHandle(handle);
    }
};
using Internet = std::unique_ptr<void, InternetClose>;
} // namespace
std::string form_encode(const std::string &text) {
    constexpr char hex[] = "0123456789ABCDEF";
    std::string result;
    for (const unsigned char c : text) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' ||
            c == '_' || c == '.' || c == '~')
            result += static_cast<char>(c);
        else {
            result += '%';
            result += hex[c >> 4];
            result += hex[c & 15];
        }
    }
    return result;
}
HttpDownload download_https(const HttpRequest &specification, const std::filesystem::path &destination,
                            std::uint64_t maximum_bytes, DownloadProgress progress) {
    const auto url = wide(specification.url);
    if (url.find_first_of(L"\r\n\t#") != std::wstring::npos || url.size() > 32768)
        throw std::runtime_error("Invalid HTTPS URL");
    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwSchemeLength = parts.dwHostNameLength = parts.dwUrlPathLength = parts.dwExtraInfoLength =
        parts.dwUserNameLength = parts.dwPasswordLength = static_cast<DWORD>(-1);
    require(WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0, &parts), "Parse URL");
    if (parts.nScheme != INTERNET_SCHEME_HTTPS || parts.dwHostNameLength == 0 || parts.dwUserNameLength ||
        parts.dwPasswordLength)
        throw std::runtime_error("Download requires HTTPS without URL credentials");
    if (specification.form_body.size() > 1024 * 1024)
        throw std::runtime_error("HTTP form exceeds metadata budget");
    if (const auto curl = configured_https_curl())
        return download_https_curl(specification, destination, maximum_bytes,
                                   std::move(progress), *curl);
    const std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
    std::wstring path(parts.lpszUrlPath, parts.dwUrlPathLength);
    if (path.empty())
        path = L"/";
    if (parts.dwExtraInfoLength)
        path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    Internet session(WinHttpOpen(L"Civis/0.1 (Civ5 content acquisition)", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                 WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    require(session != nullptr, "Open HTTP session");
    require(WinHttpSetTimeouts(session.get(), 15000, 30000, 30000, 30000), "Set HTTP timeouts");
    Internet connection(WinHttpConnect(session.get(), host.c_str(), parts.nPort, 0));
    require(connection != nullptr, "Connect HTTPS");
    const bool post = !specification.form_body.empty();
    Internet request(WinHttpOpenRequest(connection.get(), post ? L"POST" : L"GET", path.c_str(), nullptr,
                                        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                        WINHTTP_FLAG_SECURE));
    require(request != nullptr, "Open HTTPS request");
    DWORD redirect = WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
    require(WinHttpSetOption(request.get(), WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect)),
            "Set redirect policy");
    const auto *headers =
        post ? L"Content-Type: application/x-www-form-urlencoded\r\nAccept-Encoding: identity\r\n"
             : L"Accept-Encoding: identity\r\n";
    require(WinHttpSendRequest(request.get(), headers, static_cast<DWORD>(-1),
                               post ? const_cast<char *>(specification.form_body.data())
                                    : WINHTTP_NO_REQUEST_DATA,
                               static_cast<DWORD>(specification.form_body.size()),
                               static_cast<DWORD>(specification.form_body.size()), 0),
            "Send HTTPS request");
    require(WinHttpReceiveResponse(request.get(), nullptr), "Receive HTTPS response");
    DWORD status = 0, size = sizeof(status);
    require(WinHttpQueryHeaders(request.get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX),
            "Read HTTP status");
    if (status != 200)
        throw std::runtime_error("HTTP status " + std::to_string(status));
    DWORD url_bytes = 0;
    WinHttpQueryOption(request.get(), WINHTTP_OPTION_URL, nullptr, &url_bytes);
    require(GetLastError() == ERROR_INSUFFICIENT_BUFFER && url_bytes <= 128 * 1024, "Query final URL size");
    std::vector<wchar_t> final_url((url_bytes + sizeof(wchar_t) - 1) / sizeof(wchar_t));
    require(WinHttpQueryOption(request.get(), WINHTTP_OPTION_URL, final_url.data(), &url_bytes),
            "Read final URL");
    HttpDownload result{0, utf8(final_url.data())};
    content::require_plain_directory(destination.parent_path());
    const auto file = CreateFileW(destination.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                  FILE_ATTRIBUTE_NORMAL, nullptr);
    require(file != INVALID_HANDLE_VALUE, "Create download file");
    try {
        std::array<std::uint8_t, 65536> buffer;
        for (;;) {
            DWORD received = 0;
            require(
                WinHttpReadData(request.get(), buffer.data(), static_cast<DWORD>(buffer.size()), &received),
                "Read HTTPS body");
            if (received == 0)
                break;
            if (received > maximum_bytes - result.bytes)
                throw std::runtime_error("Download exceeds configured byte budget");
            DWORD written = 0;
            require(WriteFile(file, buffer.data(), received, &written, nullptr) && written == received,
                    "Write download");
            result.bytes += received;
            if (progress && !progress(result.bytes))
                throw std::runtime_error("Download cancelled");
        }
        require(FlushFileBuffers(file), "Flush download");
    } catch (...) {
        CloseHandle(file);
        throw;
    }
    CloseHandle(file);
    return result;
}
} // namespace civ5::platform
