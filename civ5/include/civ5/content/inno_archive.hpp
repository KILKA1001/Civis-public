#pragma once
#include "civ5/content/mod_archive.hpp"
#include "civ5/platform/http_download.hpp"

namespace civ5::content {
// A decoded installer contains alternative components, overlays and uninstall
// resources. It is never an active MODS directory or proof of save
// compatibility.
struct InstallerBundle {
    std::filesystem::path root;
    std::string sha256;
    std::uint64_t archive_bytes = 0, files = 0, expanded_bytes = 0;
    bool reused = false;
};
inline constexpr const char *vp_542_installer_sha256 =
    "33251bc5a0c5b1a2b99118c3dc844c994c7b2462b9fb5241efdbddda68085e71";
// Each executable and non-system dependency is pinned and checked as AMD64.
void verify_inno_decoder(const std::filesystem::path &directory);
// Read-only integrity verification of an already published bundle.
InstallerBundle verify_vp_542_bundle(const std::filesystem::path &root, ArchiveBudget budget = {});
std::wstring quote_windows_argument(const std::wstring &argument);
InstallerBundle acquire_vp_542_bundle(const std::filesystem::path &cache,
                                      const std::filesystem::path &decoder,
                                      const std::filesystem::path &local_installer = {},
                                      platform::DownloadProgress progress = {}, ArchiveBudget budget = {});
} // namespace civ5::content
