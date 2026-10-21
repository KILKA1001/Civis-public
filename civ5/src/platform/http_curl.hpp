#pragma once
#include "civ5/platform/http_download.hpp"
#include <optional>

namespace civ5::platform {
// A packaged 64-bit curl/OpenSSL executable may be selected explicitly when
// the host Schannel store cannot establish HTTPS. The normal WinHTTP path is
// used unless this transport is configured.
std::optional<std::filesystem::path> configured_https_curl();
HttpDownload download_https_curl(const HttpRequest &request,
    const std::filesystem::path &destination,std::uint64_t maximum_bytes,
    DownloadProgress progress,const std::filesystem::path &executable);
} // namespace civ5::platform
