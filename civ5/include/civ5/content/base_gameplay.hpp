#pragma once
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/runtime/file_identity.hpp"

namespace civ5::content {
struct NativeGameplayRecipe {
    runtime::FileIdentity executable;
    std::string engine_schema;
    std::vector<std::string> requests;
};
// Reads the pinned original PE as data. Extracts arguments of the verified
// straight-line loader, not a cluster of strings or executed original code.
NativeGameplayRecipe read_native_gameplay_recipe(const std::filesystem::path &executable);
struct GameplaySource {
    std::string request;
    assets::MountedResource resource;
    bool selected() const { return std::holds_alternative<assets::AssetHandle>(resource.asset); }
};
// Missing names are normal (vanilla lacks some expansion declarations).
// Only the implemented loose physical-path case of EXE 0x4249C0 is accepted.
// Unsupported backings are errors, not silently treated as missing content.
std::vector<GameplaySource> plan_base_gameplay(std::span<const std::string> requests,
                                               const assets::ResourceMountTable &resources);
} // namespace civ5::content
