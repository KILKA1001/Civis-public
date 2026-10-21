#include "civ5/ui/frontend_sources.hpp"
#include <algorithm>
#include <array>
#include <deque>
#include <stdexcept>
#include <unordered_set>

namespace civ5::ui {
namespace {
std::string key(std::string_view source) {
    auto result = assets::resource_basename(std::string(source));
    if (!result.ends_with(".xml")) result += ".xml";
    return result;
}
bool true_value(const std::string &value) {
    return value == "1" || value == "true" || value == "True";
}
bool excluded_override(const assets::AssetHandle &handle) {
    const auto &p = handle.candidate().provenance;
    const auto path = "/" + assets::normalize_asset_name(p.source_id + "/" +
        p.source_relative_path + "/" + p.archive_relative_path.value_or("")) + "/";
    return path.find("/ui_bc1/") != std::string::npos ||
        path.find("/vpui/") != std::string::npos || path.find("/eui/") != std::string::npos;
}
bool excluded_context(const std::string &name) {
    static constexpr std::array<std::string_view, 7> excluded{
        "uitestmenu.xml", "gridexamples.xml", "pregamescreen.xml", "ingame.xml",
        "worldview.xml", "ingameroot.xml", "ingamescreen.xml"};
    return std::find(excluded.begin(), excluded.end(), name) != excluded.end();
}
bool interactive(const std::string &element) {
    static constexpr std::array<std::string_view, 11> elements{
        "Button", "GridButton", "CheckBox", "PullDown", "EditBox", "Slider",
        "ScrollBar", "UpButton", "DownButton", "RadioButton", "TextButton"};
    return std::find(elements.begin(), elements.end(), element) != elements.end();
}
std::string label_caption(const content::XmlNode &node, const std::string &attribute) {
    for (const auto &child : node.children) {
        if (child.name == "Instance" || child.name == "Template" ||
            child.name == "ShowOnMouseOver" || interactive(child.name)) continue;
        if (child.name == "Label") {
            const auto text = child.attribute(attribute);
            if (!text.empty()) return text;
        }
        if (auto text = label_caption(child, attribute); !text.empty()) return text;
    }
    return {};
}
std::string caption(const content::XmlNode &node) {
    auto text = node.attribute("String");
    if (!text.empty()) return text;
    if (text = label_caption(node, "String"); !text.empty()) return text;
    if (text = node.attribute("ToolTip"); !text.empty()) return text;
    return label_caption(node, "ToolTip");
}
void extract(const content::XmlNode &node, FrontendScreenSource &screen,
        const FrontendSourceBudget &budget, std::uint64_t &nodes, std::uint64_t depth,
        bool hidden, bool disabled, std::string reason, std::vector<std::string> ancestors,
        std::string parent_label) {
    if (++nodes > budget.maximum_nodes_per_screen || depth > budget.maximum_depth)
        throw std::runtime_error("XML node/depth budget exceeded");
    // Instance children are prototypes populated dynamically by Lua, not
    // permanently present commands. Keep them only in the original document.
    if (node.name == "Instance" || node.name == "Instances" || node.name == "Template") return;
    const auto local_hidden = true_value(node.attribute("Hidden"));
    const auto local_disabled = true_value(node.attribute("Disabled"));
    if (local_hidden || local_disabled) {
        if (!reason.empty()) reason += "; ";
        reason += node.name + "#" + node.attribute("ID") + ":";
        if (local_hidden) reason += " Hidden=" + node.attribute("Hidden");
        if (local_disabled) reason += " Disabled=" + node.attribute("Disabled");
    }
    hidden = hidden || local_hidden;
    disabled = disabled || local_disabled;
    const auto id = node.attribute("ID");
    if (node.name == "LuaContext") {
        const auto file = node.attribute("FileName");
        if (!file.empty()) screen.contexts.push_back({id, key(file), hidden,
            true_value(node.attribute("DeferLoad"))});
    } else if (!id.empty()) {
        auto text = caption(node);
        if (interactive(node.name) && text.empty()) text = parent_label;
        screen.controls.push_back({id, node.name, std::move(text), node.attribute("Style"),
            node.attribute("Texture"), interactive(node.name), hidden, disabled, reason, ancestors});
    }
    if (!id.empty()) ancestors.push_back(id);
    if (node.name == "Label" && !node.attribute("String").empty()) parent_label = node.attribute("String");
    for (const auto &child : node.children)
        extract(child, screen, budget, nodes, depth + 1, hidden, disabled, reason, ancestors, parent_label);
}
} // namespace
const FrontendControlSource *FrontendScreenSource::find_control(std::string_view id) const {
    const auto found = std::find_if(controls.begin(), controls.end(),
        [&](const auto &control) { return control.id == id; });
    return found == controls.end() ? nullptr : &*found;
}
const FrontendScreenSource *FrontendSourceCatalog::find(std::string_view source_key) const {
    const auto requested = key(source_key);
    const auto found = std::find_if(screens_.begin(), screens_.end(),
        [&](const auto &screen) { return screen.source_key == requested; });
    return found == screens_.end() ? nullptr : &*found;
}
FrontendSourceCatalog FrontendSourceCatalog::from_resources(
        const assets::ResourceMountTable &resources, FrontendSourceBudget budget, std::string root) {
    if (!budget.maximum_screens || !budget.maximum_resource_bytes || !budget.maximum_total_bytes ||
        !budget.maximum_nodes_per_screen || !budget.maximum_depth)
        throw std::invalid_argument("Frontend source budgets must be positive");
    FrontendSourceCatalog result;
    std::deque<std::string> pending{key(root)};
    std::unordered_set<std::string> visited;
    std::uint64_t bytes_read = 0;
    auto read = [&](const std::string &name, std::string &path,
                    std::optional<std::uint64_t> &mount_id) -> std::optional<std::string> {
        const auto selected = resources.resolve(assets::AssetKey(name));
        const auto *handle = std::get_if<assets::AssetHandle>(&selected.asset);
        if (!handle) {
            result.diagnostics_.push_back(name + (std::holds_alternative<assets::AmbiguousAsset>(selected.asset)
                ? ": ambiguous selected resource" : ": missing selected resource"));
            return std::nullopt;
        }
        if (excluded_override(*handle)) {
            result.diagnostics_.push_back(name + ": selected EUI/VPUI override excluded from original baseline");
            return std::nullopt;
        }
        const auto length = handle->candidate().payload_size;
        if (length > budget.maximum_resource_bytes || length > budget.maximum_total_bytes - bytes_read)
            throw std::runtime_error("resource byte budget exceeded");
        auto bytes = handle->read_bytes(budget.maximum_resource_bytes);
        bytes_read += length;
        path = handle->candidate().provenance.source_relative_path;
        mount_id = selected.mount_id;
        return std::string(bytes.begin(), bytes.end());
    };
    while (!pending.empty()) {
        auto name = std::move(pending.front());
        pending.pop_front();
        if (visited.contains(name)) continue;
        if (excluded_context(name)) {
            visited.insert(name);
            result.diagnostics_.push_back(name + ": debug or game-world context excluded");
            continue;
        }
        if (visited.size() >= budget.maximum_screens) {
            result.diagnostics_.push_back("Frontend source screen budget exceeded");
            break;
        }
        visited.insert(name);
        try {
            FrontendScreenSource screen;
            screen.source_key = name;
            const auto xml = read(name, screen.selected_source_path, screen.mount_id);
            if (!xml) continue;
            screen.document = content::read_xml_metadata(*xml);
            if (screen.document.name != "Context") throw std::runtime_error("expected Context XML root");
            std::uint64_t nodes = 0;
            extract(screen.document, screen, budget, nodes, 0, false, false, {}, {}, {});
            screen.lua_source_key = name.substr(0, name.size() - 4) + ".lua";
            try {
                std::optional<std::uint64_t> lua_mount;
                if (auto lua = read(screen.lua_source_key, screen.lua_selected_source_path, lua_mount))
                    screen.lua_text = std::move(*lua);
            } catch (const std::exception &error) {
                result.diagnostics_.push_back(screen.lua_source_key + ": " + error.what());
            }
            for (const auto &context : screen.contexts) pending.push_back(context.source_key);
            result.screens_.push_back(std::move(screen));
        } catch (const std::exception &error) {
            result.diagnostics_.push_back(name + ": " + error.what());
        }
    }
    return result;
}
} // namespace civ5::ui
