#pragma once
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

namespace civ5::platform {
struct HttpRequest {
    std::string url;
    std::string form_body; // Empty means GET, otherwise a form-encoded POST.
};
struct HttpDownload {
    std::uint64_t bytes = 0;
    std::string final_url;
};
using DownloadProgress = std::function<bool(std::uint64_t)>;
// HTTPS via WinHTTP or a packaged x64 curl with certificate verification.
// Destination is created exclusively; a failure leaves a partial file outside
// the installed cache.
HttpDownload download_https(const HttpRequest &request, const std::filesystem::path &destination,
                            std::uint64_t maximum_bytes, DownloadProgress progress = {});
std::string form_encode(const std::string &text);
} // namespace civ5::platform
