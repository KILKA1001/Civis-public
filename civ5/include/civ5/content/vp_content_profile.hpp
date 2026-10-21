#pragma once
#include "civ5/content/vp_profile.hpp"
#include "civ5/runtime/save_preflight.hpp"

namespace civ5::content {
// Only the verified EUI installer component may account for CP/VP manifest
// files that it deliberately omits. The common content resolver receives
// exact declarations, without knowing any VP-specific policy.
std::vector<runtime::ManifestFileOmission> vp_manifest_file_omissions(
    const VpProfile &installer, const Catalog &catalog,
    const runtime::SavedContentProfile &selected);
} // namespace civ5::content
