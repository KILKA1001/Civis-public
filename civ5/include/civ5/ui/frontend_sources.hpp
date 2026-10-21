#pragma once
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/content/manifest.hpp"

namespace civ5::ui {
// These are authored source observations, not evaluated Lua state or an
// entitlement/renderer/execution capability. The XML document remains intact.
struct FrontendControlSource {
    std::string id, element, text_key, style, texture;
    bool interactive = false;
    bool static_hidden = false, static_disabled = false;
    std::string static_state_reason;
    std::vector<std::string> ancestor_ids;
};
struct FrontendContextSource {
    std::string id, source_key;
    bool hidden = false, defer_load = false;
};
struct FrontendScreenSource {
    std::string source_key, selected_source_path;
    std::optional<std::uint64_t> mount_id;
    content::XmlNode document;
    std::vector<FrontendControlSource> controls;
    std::vector<FrontendContextSource> contexts;
    std::string lua_source_key, lua_selected_source_path, lua_text;
    const FrontendControlSource *find_control(std::string_view id) const;
};
struct FrontendSourceBudget {
    std::uint64_t maximum_screens = 128;
    std::uint64_t maximum_resource_bytes = 8ULL * 1024 * 1024;
    std::uint64_t maximum_total_bytes = 32ULL * 1024 * 1024;
    std::uint64_t maximum_nodes_per_screen = 100000;
    std::uint64_t maximum_depth = 64;
};
class FrontendSourceCatalog {
  public:
    // Only the caller's active VFS selects payloads. No installation probing,
    // fallback to another candidate, Lua execution or game-world initialization.
    // Baseline rejects selected EUI/VPUI overrides, reporting them explicitly.
    static FrontendSourceCatalog from_resources(const assets::ResourceMountTable &resources,
        FrontendSourceBudget budget = {}, std::string root = "FrontEnd.xml");
    const std::vector<FrontendScreenSource> &screens() const { return screens_; }
    const std::vector<std::string> &diagnostics() const { return diagnostics_; }
    // Case-insensitive VFS basename; extensionless context names are supported.
    const FrontendScreenSource *find(std::string_view source_key) const;
  private:
    std::vector<FrontendScreenSource> screens_;
    std::vector<std::string> diagnostics_;
};
} // namespace civ5::ui
