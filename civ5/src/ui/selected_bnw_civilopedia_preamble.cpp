#include "civ5/ui/selected_bnw_civilopedia_preamble.hpp"
#include "civ5/ui/selected_bnw_icon_support_step.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/lua_gameplay_services.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/assets/asset_source.hpp"
#include <array>
#include <fstream>
#include <regex>
#include <set>
#include <stdexcept>
#include <vector>

namespace civ5::ui {
namespace {
namespace fs = std::filesystem;

constexpr std::string_view native_start =
    "local g_ListItemManager = InstanceManager:new( \"ListItemInstance\", "
    "\"ListItemButton\", Controls.ListOfArticles );";
constexpr std::array<std::string_view, 5> context_events = {
    "Events.SearchForPediaEntry.Add( SearchForPediaEntry );",
    "Events.GoToPediaHomePage.Add( GoToPediaHomePage );",
    "Events.GameplaySetActivePlayer.Add(OnClose);",
    "Events.MultiplayerGameLobbyInvite.Add( OnMultiplayerGameInvite );",
    "Events.MultiplayerGameServerInvite.Add( OnMultiplayerGameInvite );",
};

bool has_id(const content::XmlNode &node, std::string_view id) {
    if (node.attribute("ID") == id) return true;
    for (const auto &child : node.children)
        if (has_id(child, id)) return true;
    return false;
}

std::size_t count_id(const content::XmlNode &node, std::string_view id) {
    std::size_t count = node.attribute("ID") == id ? 1 : 0;
    for (const auto &child : node.children) count += count_id(child, id);
    return count;
}

const content::XmlNode *find_instance(const content::XmlNode &node,
                                      std::string_view name) {
    if (node.name == "Instance" && node.attribute("Name") == name) return &node;
    for (const auto &child : node.children)
        if (const auto *found = find_instance(child, name)) return found;
    return nullptr;
}

std::size_t count_instance(const content::XmlNode &node, std::string_view name) {
    std::size_t count = node.name == "Instance" &&
        node.attribute("Name") == name ? 1 : 0;
    for (const auto &child : node.children)
        count += count_instance(child, name);
    return count;
}

bool direct_child(const content::XmlNode &node, std::string_view kind,
                  std::string_view id) {
    for (const auto &child : node.children)
        if (child.name == kind && child.attribute("ID") == id) return true;
    return false;
}

const content::XmlNode *direct_child_with_id(const content::XmlNode &node,
                                             std::string_view id) {
    for (const auto &child : node.children)
        if (child.attribute("ID") == id) return &child;
    return nullptr;
}

const content::XmlNode *find_id_with_path(const content::XmlNode &node,
                                          std::string_view id,
                                          std::vector<std::string> &path) {
    auto segment = node.name;
    if (const auto name = node.attribute("ID"); !name.empty())
        segment += "#" + name;
    path.push_back(std::move(segment));
    if (node.attribute("ID") == id) return &node;
    for (const auto &child : node.children)
        if (const auto *found = find_id_with_path(child, id, path)) return found;
    path.pop_back();
    return nullptr;
}

const content::XmlNode *find_article_stack(const content::XmlNode &node) {
    if (node.name == "ScrollPanel" &&
        node.attribute("ID") == "LeftScrollPanel") {
        for (const auto &child : node.children)
            if (child.name == "Stack" &&
                child.attribute("ID") == "ListOfArticles") return &child;
    }
    for (const auto &child : node.children)
        if (const auto *found = find_article_stack(child)) return found;
    return nullptr;
}

const content::XmlNode *find_prereq_grid(const content::XmlNode &node) {
    if (node.name == "Grid" && node.attribute("ID") == "PrereqTechFrame") {
        for (const auto &child : node.children)
            if (child.name == "Grid" &&
                child.attribute("ID") == "PrereqTechInnerFrame") return &child;
    }
    for (const auto &child : node.children)
        if (const auto *found = find_prereq_grid(child)) return found;
    return nullptr;
}

std::size_t source_line(std::string_view source, std::size_t position) {
    std::size_t line = 1;
    for (std::size_t index = 0; index < position; ++index)
        if (source[index] == '\n') ++line;
    return line;
}

std::string read_verified(const fs::path &path, std::string &sha) {
    const auto identity = runtime::identify_file(path);
    if (identity.bytes > 64ULL * 1024 * 1024)
        throw std::runtime_error("Selected Civilopedia source exceeds 64 MiB");
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Selected Civilopedia source cannot be opened");
    std::string source((std::istreambuf_iterator<char>(input)),
                        std::istreambuf_iterator<char>());
    if (!input.eof() && input.fail())
        throw std::runtime_error("Selected Civilopedia source cannot be read");
    const auto bytes = std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t *>(source.data()), source.size());
    if (runtime::identify_bytes(bytes).sha256 != identity.sha256)
        throw std::runtime_error("Selected Civilopedia source changed during read");
    sha = identity.sha256;
    return source;
}

} // namespace

SelectedBnwCivilopediaPreamble inspect_civilopedia_preamble_source(
    std::string_view source, const content::XmlNode &layout) {
    SelectedBnwCivilopediaPreamble result;
    const auto first = source.find("include( \"InstanceManager\" );");
    const auto second = source.find("include( \"IconSupport\" );");
    const auto boundary = source.find(native_start);
    if (first == std::string_view::npos || second == std::string_view::npos ||
        boundary == std::string_view::npos || first >= second || second >= boundary ||
        source.find(native_start, boundary + 1) != std::string_view::npos) {
        result.blocker = "Selected Civilopedia include/native Controls order changed";
        return result;
    }
    if (source.substr(second + std::string_view("include( \"IconSupport\" );").size(),
                      boundary - second - std::string_view("include( \"IconSupport\" );").size())
            .find("Controls.") != std::string_view::npos) {
        result.blocker = "Selected Civilopedia has an earlier native Controls dependency";
        return result;
    }
    if (!has_id(layout, "ListOfArticles")) {
        result.blocker = "Selected Civilopedia XML lacks first native ListOfArticles control";
        return result;
    }
    std::size_t previous = boundary;
    for (const auto event : context_events) {
        const auto position = source.find(event, previous);
        if (position == std::string_view::npos ||
            source.find(event, position + 1) != std::string_view::npos) {
            result.blocker = "Selected Civilopedia authored event registration changed";
            return result;
        }
        previous = position + event.size();
    }
    result.first_native_control = "ListOfArticles";
    result.first_native_line = source_line(source, boundary);
    result.authored_later_event_registrations = context_events.size();
    result.status = SelectedBnwCivilopediaPreambleStatus::source_verified;
    result.blocker = "Native Controls.ListOfArticles/InstanceManager lifecycle is unavailable; "
                     "Civilopedia event registrations and research TestAll remain unverified";
    return result;
}

SelectedBnwArticleListControlContract inspect_civilopedia_article_list_control(
    std::string_view source, const content::XmlNode &layout,
    std::string_view instance_source) {
    SelectedBnwArticleListControlContract result;
    const auto preamble = inspect_civilopedia_preamble_source(source, layout);
    if (preamble.status != SelectedBnwCivilopediaPreambleStatus::source_verified) {
        result.blocker = preamble.blocker;
        return result;
    }
    constexpr std::string_view heading =
        "local g_ListHeadingManager = InstanceManager:new( \"ListHeadingInstance\", "
        "\"ListHeadingButton\", Controls.ListOfArticles );";
    constexpr std::string_view next =
        "local g_PrereqTechManager = InstanceManager:new( \"PrereqTechInstance\", "
        "\"PrereqTechButton\", Controls.PrereqTechInnerFrame );";
    const auto first = source.find(native_start);
    const auto heading_line = source.find('\n', first);
    if (heading_line == std::string_view::npos ||
        source.substr(heading_line + 1, heading.size()) != heading) {
        result.blocker = "Selected Civilopedia second article-list manager changed";
        return result;
    }
    const auto next_line = source.find('\n', heading_line + 1);
    if (next_line == std::string_view::npos ||
        source.substr(next_line + 1, next.size()) != next) {
        result.blocker = "Selected Civilopedia next native Controls boundary changed";
        return result;
    }
    const auto *item = find_instance(layout, "ListItemInstance");
    const auto *heading_instance = find_instance(layout, "ListHeadingInstance");
    const auto *stack = find_article_stack(layout);
    if (count_id(layout, "ListOfArticles") != 1 || !stack ||
        stack->attribute("Size") != "64,64" ||
        stack->attribute("Offset") != "7,0" ||
        stack->attribute("StackGrowth") != "Bottom" ||
        !item || count_instance(layout, "ListItemInstance") != 1 ||
        count_id(*item, "ListItemButton") != 1 ||
        !direct_child(*item, "Button", "ListItemButton") ||
        !heading_instance || count_instance(layout, "ListHeadingInstance") != 1 ||
        count_id(*heading_instance, "ListHeadingButton") != 1 ||
        !direct_child(*heading_instance, "Button", "ListHeadingButton")) {
        result.blocker = "Selected Civilopedia XML article-list control or instances changed";
        return result;
    }
    const auto ctor = instance_source.find(
        "new = function(self, instanceName, rootControlName, ParentControl)");
    const auto ctor_end = instance_source.find("end,", ctor);
    const auto generational = instance_source.find("GenerationalInstanceManager =");
    if (ctor == std::string_view::npos || ctor_end == std::string_view::npos ||
        generational == std::string_view::npos || ctor >= generational ||
        ctor_end >= generational) {
        result.blocker = "Selected InstanceManager constructor is ambiguous";
        return result;
    }
    const auto body = instance_source.substr(ctor, ctor_end - ctor);
    if (body.find("o.m_ParentControl    = ParentControl;") == std::string_view::npos ||
        body.find("o.m_InstanceName     = instanceName;") == std::string_view::npos ||
        body.find("o.m_RootControlName  = rootControlName;") == std::string_view::npos ||
        body.find("return o;") == std::string_view::npos ||
        body.find("ContextPtr") != std::string_view::npos ||
        body.find("RegisterCallback") != std::string_view::npos ||
        body.find("BuildInstance") != std::string_view::npos) {
        result.blocker = "Selected InstanceManager constructor has unverified native effects";
        return result;
    }
    result.control_id = "ListOfArticles";
    result.control_kind = "Stack";
    result.parent_id = "LeftScrollPanel";
    result.item_instance = "ListItemInstance";
    result.item_root = "ListItemButton";
    result.heading_instance = "ListHeadingInstance";
    result.heading_root = "ListHeadingButton";
    result.next_native_line = source_line(source, next_line + 1);
    result.verified = true;
    result.blocker = "Next native Controls.PrereqTechInnerFrame requires an owned control provider";
    return result;
}

SelectedBnwPrereqTechControlContract inspect_civilopedia_prereq_tech_control(
    std::string_view source, const content::XmlNode &layout,
    std::string_view instance_source) {
    SelectedBnwPrereqTechControlContract result;
    const auto article = inspect_civilopedia_article_list_control(
        source, layout, instance_source);
    if (!article.verified) {
        result.blocker = article.blocker;
        return result;
    }
    constexpr std::string_view prereq =
        "local g_PrereqTechManager = InstanceManager:new( \"PrereqTechInstance\", "
        "\"PrereqTechButton\", Controls.PrereqTechInnerFrame );";
    constexpr std::string_view next =
        "local g_GreatWorksManager = InstanceManager:new( \"GreatWorksInstance\", "
        "\"GreatWorksButton\", Controls.GreatWorksInnerFrame );";
    const auto third = source.find(prereq);
    const auto third_end = source.find('\n', third);
    if (third == std::string_view::npos || third_end == std::string_view::npos ||
        source.substr(third_end + 1, next.size()) != next ||
        source.find(prereq, third + 1) != std::string_view::npos) {
        result.blocker = "Selected Civilopedia prerequisite-tech/next control order changed";
        return result;
    }
    const auto *grid = find_prereq_grid(layout);
    const auto *instance = find_instance(layout, "PrereqTechInstance");
    if (count_id(layout, "PrereqTechFrame") != 1 ||
        count_id(layout, "PrereqTechInnerFrame") != 1 || !grid ||
        grid->attribute("Size") != "208,140" ||
        grid->attribute("Offset") != "0,0" ||
        grid->attribute("Style") != "Grid9Frame" ||
        !instance || count_instance(layout, "PrereqTechInstance") != 1 ||
        count_id(*instance, "PrereqTechButton") != 1 ||
        !direct_child(*instance, "Button", "PrereqTechButton")) {
        result.blocker = "Selected Civilopedia XML prerequisite-tech Grid/Instance changed";
        return result;
    }
    result.control_id = "PrereqTechInnerFrame";
    result.control_kind = "Grid";
    result.parent_id = "PrereqTechFrame";
    result.instance_name = "PrereqTechInstance";
    result.instance_root = "PrereqTechButton";
    result.next_native_control = "GreatWorksInnerFrame";
    result.next_native_line = source_line(source, third_end + 1);
    result.verified = true;
    result.blocker = "Next native Controls.GreatWorksInnerFrame requires an owned control provider";
    return result;
}

SelectedBnwManagerBlockContract inspect_civilopedia_manager_block(
    std::string_view source, const content::XmlNode &layout,
    std::string_view instance_source) {
    SelectedBnwManagerBlockContract result;
    const auto prereq = inspect_civilopedia_prereq_tech_control(
        source, layout, instance_source);
    if (!prereq.verified) {
        result.blocker = prereq.blocker;
        return result;
    }
    static const std::regex manager_pattern(
        R"regex(^local (g_[A-Za-z0-9_]+) = InstanceManager:new\( "([A-Za-z0-9_]+)", "([A-Za-z0-9_]+)", Controls\.([A-Za-z0-9_]+) \);$)regex");
    const auto first = source.find(native_start);
    auto position = first;
    std::set<std::string> seen_managers;
    while (position < source.size()) {
        const auto end = source.find('\n', position);
        if (end == std::string_view::npos) break;
        auto line = std::string(source.substr(position, end - position));
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::smatch match;
        if (!std::regex_match(line, match, manager_pattern)) break;
        SelectedBnwManagerBinding binding;
        binding.lua_name = match[1].str();
        binding.instance_name = match[2].str();
        binding.instance_root = match[3].str();
        binding.parent_control = match[4].str();
        binding.source_line = source_line(source, position);
        if (!seen_managers.insert(binding.lua_name).second ||
            count_instance(layout, binding.instance_name) != 1 ||
            count_id(layout, binding.parent_control) != 1) {
            result.blocker = "Selected Civilopedia manager/instance/control identity is ambiguous";
            return result;
        }
        const auto *instance = find_instance(layout, binding.instance_name);
        if (!instance || count_id(*instance, binding.instance_root) != 1 ||
            !direct_child_with_id(*instance, binding.instance_root)) {
            result.blocker = "Selected Civilopedia manager instance root is not an XML direct child";
            return result;
        }
        std::vector<std::string> path;
        const auto *control = find_id_with_path(layout, binding.parent_control, path);
        if (!control || path.empty()) {
            result.blocker = "Selected Civilopedia manager parent control is missing";
            return result;
        }
        binding.parent_kind = control->name;
        for (const auto &segment : path) {
            if (!binding.xml_owner_path.empty()) binding.xml_owner_path += '/';
            binding.xml_owner_path += segment;
        }
        result.bindings.push_back(std::move(binding));
        position = end + 1;
    }
    if (result.bindings.size() != 33 ||
        result.bindings.front().lua_name != "g_ListItemManager" ||
        result.bindings.back().lua_name != "g_ImprovementsManager") {
        result.blocker = "Selected Civilopedia contiguous manager block changed";
        return result;
    }
    constexpr std::string_view next =
        "local _, screenSizeY = UIManager:GetScreenSizeVal(); -- Controls.BackDrop:GetSize();";
    while (position < source.size()) {
        const auto end = source.find('\n', position);
        if (end == std::string_view::npos) break;
        auto line = std::string(source.substr(position, end - position));
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto active = line.find_first_not_of(" \t");
        if (active == std::string::npos || line.compare(active, 2, "--") == 0) {
            position = end + 1;
            continue;
        }
        if (line != next) {
            result.blocker = "Selected Civilopedia first native UIManager call changed";
            return result;
        }
        result.next_native_line = source_line(source, position);
        result.verified = true;
        result.blocker = "UIManager.GetScreenSizeVal and viewport-dependent Controls layout require an owned UI provider";
        return result;
    }
    result.blocker = "Selected Civilopedia first native UIManager call is missing";
    return result;
}

SelectedBnwCategoryCallbackBoundary inspect_civilopedia_first_callback(
    std::string_view source, const content::XmlNode &layout,
    std::string_view instance_source) {
    SelectedBnwCategoryCallbackBoundary result;
    const auto managers = inspect_civilopedia_manager_block(
        source, layout, instance_source);
    if (!managers.verified) {
        result.blocker = managers.blocker;
        return result;
    }
    constexpr std::string_view registration =
        "Controls[buttonName]:RegisterCallback( Mouse.eLClick, "
        "CivilopediaCategory[i].buttonClicked );";
    const auto callback = source.find(registration);
    const auto category_loop = source.find("for i = 1, numCategories, 1 do");
    if (callback == std::string_view::npos ||
        source.find(registration, callback + 1) != std::string_view::npos ||
        category_loop == std::string_view::npos || category_loop >= callback ||
        source.find("local buttonName = \"CategoryButton\"..tostring(i);",
                    category_loop) > callback ||
        source.find("local numCategories = 16;") == std::string_view::npos ||
        source.find("UIManager:GetScreenSizeVal()") >= callback ||
        source.find("Controls.ScrollBar:GetSize().x") >= callback ||
        source.find("Controls.ScrollPanel:CalculateInternalSize()") >= callback) {
        result.blocker = "Selected Civilopedia first callback/layout source order changed";
        return result;
    }
    for (int index = 1; index <= 16; ++index) {
        const auto id = "CategoryButton" + std::to_string(index);
        if (count_id(layout, id) != 1) {
            result.blocker = "Selected Civilopedia category button identity is ambiguous";
            return result;
        }
        std::vector<std::string> path;
        const auto *button = find_id_with_path(layout, id, path);
        if (!button || button->name != "Button") {
            result.blocker = "Selected Civilopedia category callback target is not a Button";
            return result;
        }
    }
    std::vector<std::string> scrollbar_path;
    const auto *scrollbar = find_id_with_path(layout, "ScrollBar", scrollbar_path);
    if (!scrollbar || count_id(layout, "ScrollBar") != 1 ||
        scrollbar->name != "ScrollBar" ||
        scrollbar->attribute("Style") != "VertSlider" ||
        !scrollbar->attribute("Size").empty() ||
        scrollbar->attribute("Length") != "800") {
        result.blocker = "Selected Civilopedia ScrollBar style-based native width changed";
        return result;
    }
    result.source_verified = true;
    result.first_callback_line = source_line(source, callback);
    result.authored_category_buttons = 16;
    result.required_mouse_event = "Mouse.eLClick";
    result.blocker = "Registration is unavailable before measured UIManager screen size, "
                     "native VertSlider width and Controls.CalculateInternalSize";
    return result;
}

static SelectedBnwCivilopediaPreamble run_source_order(
    const fs::path &game_root, const content::Catalog &catalog,
    const runtime::SavedContentProfile &profile,
    const SelectedBnwPreviewHost &selected, bool bind_article_list,
    bool bind_prereq_tech, bool bind_manager_block) {
    SelectedBnwCivilopediaPreamble result;
    try {
        const auto previous = execute_selected_bnw_icon_support_include(
            game_root, catalog, profile, selected);
        if (previous.status != SelectedBnwIconSupportStatus::include_executed)
            throw std::runtime_error("Selected IconSupport step is blocked: " + previous.blocker);
        const auto root = fs::canonical(game_root);
        const auto expansion2 = root / "Assets/DLC/Expansion2/UI/Civilopedia";
        const auto script = read_verified(expansion2 / "CivilopediaScreen.lua",
                                          result.context_source_sha256);
        if (result.context_source_sha256 != previous.context_source_sha256)
            throw std::runtime_error("Selected Civilopedia source changed between includes");
        const auto layout_path = expansion2 / "CivilopediaScreen.xml";
        result.xml_sha256 = runtime::identify_file(layout_path).sha256;
        const auto layout = content::read_xml(layout_path);
        auto shape = inspect_civilopedia_preamble_source(script, layout);
        if (shape.status != SelectedBnwCivilopediaPreambleStatus::source_verified)
            throw std::runtime_error(shape.blocker);
        const auto boundary = script.find(native_start);
        std::string instance_sha, icon_sha, vector_sha;
        const auto instance = read_verified(root / "Assets/UI/InstanceManager.lua", instance_sha);
        const auto icon = read_verified(root / "Assets/UI/IconSupport.lua", icon_sha);
        const auto vector = read_verified(root / "Assets/Gameplay/Lua/FLuaVector.lua", vector_sha);
        if (instance_sha != previous.instance_manager_sha256 ||
            icon_sha != previous.icon_support_sha256 ||
            vector_sha != previous.vector_sha256)
            throw std::runtime_error("Selected Civilopedia include changed between steps");
        SelectedBnwArticleListControlContract article;
        SelectedBnwPrereqTechControlContract prereq;
        SelectedBnwManagerBlockContract managers;
        if (bind_article_list) {
            article = inspect_civilopedia_article_list_control(script, layout, instance);
            if (!article.verified) throw std::runtime_error(article.blocker);
        }
        if (bind_prereq_tech) {
            prereq = inspect_civilopedia_prereq_tech_control(script, layout, instance);
            if (!prereq.verified) throw std::runtime_error(prereq.blocker);
        }
        if (bind_manager_block) {
            managers = inspect_civilopedia_manager_block(script, layout, instance);
            if (!managers.verified) throw std::runtime_error(managers.blocker);
        }

        const auto &session = selected.session();
        result.owner_player_id = selected.active_player_id();
        result.owner_game_turn = session.state().setup.game_turn;
        result.owner_payload_sha256 = selected.identity().payload_sha256;
        result.owner_rules_sha256 = selected.identity().rules_sha256;
        auto detached_setup = session.state().setup;
        auto detached_database = selected.database().clone_gameplay();
        auto types = detached_database->cache_info_types();
        if (!types.report().all_succeeded())
            throw std::runtime_error("Selected Civilopedia GameInfo catalog is incomplete");
        runtime::ScriptSourceProvider sources;
        sources.files = [](std::string_view) {
            return std::vector<std::string>{
                "Assets/UI/InstanceManager.lua", "Assets/UI/IconSupport.lua",
                "Assets/Gameplay/Lua/FLuaVector.lua"};
        };
        sources.read = [instance, icon, vector, &result](std::string_view,
                                                        std::string_view path) -> std::string {
            result.observed_include_order.emplace_back(path);
            if (path == "Assets/UI/InstanceManager.lua") return instance;
            if (path == "Assets/UI/IconSupport.lua") return icon;
            if (path == "Assets/Gameplay/Lua/FLuaVector.lua") return vector;
            throw runtime::ScriptError("Unexpected selected Civilopedia include");
        };
        runtime::ScriptUiEventProvider events;
        events.notifications = {"AfterModsDeactivate", "AfterModsActivate"};
        runtime::LuaScriptHost host({64ULL * 1024 * 1024, 2'000'000},
            std::move(sources), {},
            runtime::lua_gameplay_services(*detached_database, detached_setup,
                                           session.state().technologies, types),
            {}, {}, {}, std::move(events));
        if (bind_manager_block) {
            std::string controls = "Controls={";
            std::set<std::string> seen;
            for (const auto &binding : managers.bindings)
                if (seen.insert(binding.parent_control).second) {
                    controls += binding.parent_control + "={id='" +
                        binding.parent_control + "',kind='" +
                        binding.parent_kind + "'},";
                }
            controls += '}';
            host.execute("CivilopediaScreen", controls,
                         "=source-verified-civilopedia-manager-controls");
        } else if (bind_article_list)
            host.execute("CivilopediaScreen",
                bind_prereq_tech
                    ? "Controls={ListOfArticles={id='ListOfArticles',kind='Stack',"
                      "parent_id='LeftScrollPanel'},"
                      "PrereqTechInnerFrame={id='PrereqTechInnerFrame',kind='Grid',"
                      "parent_id='PrereqTechFrame'}}"
                    : "Controls={ListOfArticles={id='ListOfArticles',kind='Stack',"
                      "parent_id='LeftScrollPanel'}}",
                "=source-verified-civilopedia-controls");
        const auto heading_end = bind_article_list
            ? script.find('\n', script.find('\n', boundary) + 1) : boundary;
        if (bind_article_list && heading_end == std::string::npos)
            throw std::runtime_error("Selected article-list manager lines are incomplete");
        const auto prereq_end = bind_prereq_tech
            ? script.find('\n', heading_end + 1) : heading_end;
        if (bind_prereq_tech && prereq_end == std::string::npos)
            throw std::runtime_error("Selected prerequisite-tech manager line is incomplete");
        const auto manager_end = bind_manager_block
            ? script.find('\n', script.find("local g_ImprovementsManager =", boundary))
            : prereq_end;
        if (bind_manager_block && manager_end == std::string::npos)
            throw std::runtime_error("Selected manager block has no complete final line");
        std::string executable = script.substr(0,
            bind_manager_block ? manager_end + 1 :
            bind_prereq_tech ? prereq_end + 1 :
            bind_article_list ? heading_end + 1 : boundary);
        if (bind_manager_block) {
            for (const auto &binding : managers.bindings)
                executable += "\nassert(" + binding.lua_name +
                    ".m_InstanceName=='" + binding.instance_name + "' and " +
                    binding.lua_name + ".m_RootControlName=='" +
                    binding.instance_root + "' and " + binding.lua_name +
                    ".m_ParentControl==Controls." + binding.parent_control +
                    " and " + binding.lua_name + ".m_iCount==0)";
        } else if (bind_article_list)
            executable +=
                "\nassert(g_ListItemManager.m_InstanceName=='ListItemInstance' and "
                "g_ListItemManager.m_RootControlName=='ListItemButton' and "
                "g_ListItemManager.m_ParentControl==Controls.ListOfArticles and "
                "g_ListHeadingManager.m_InstanceName=='ListHeadingInstance' and "
                "g_ListHeadingManager.m_RootControlName=='ListHeadingButton' and "
                "g_ListHeadingManager.m_ParentControl==Controls.ListOfArticles and "
                "g_ListItemManager.m_iCount==0 and g_ListHeadingManager.m_iCount==0)";
        if (bind_prereq_tech && !bind_manager_block)
            executable +=
                "\nassert(g_PrereqTechManager.m_InstanceName=='PrereqTechInstance' and "
                "g_PrereqTechManager.m_RootControlName=='PrereqTechButton' and "
                "g_PrereqTechManager.m_ParentControl==Controls.PrereqTechInnerFrame and "
                "g_PrereqTechManager.m_iCount==0)";
        host.execute("CivilopediaScreen", executable,
                     "@Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua:prefix");
        if (result.observed_include_order != std::vector<std::string>{
                "Assets/UI/InstanceManager.lua", "Assets/UI/IconSupport.lua",
                "Assets/Gameplay/Lua/FLuaVector.lua"})
            throw std::runtime_error("Selected Civilopedia include execution order changed");
        host.execute("CivilopediaScreen",
            "assert(type(Alphabetically)=='function' and "
            "type(IconLookup)=='function' and #GameInfo.IconTextureAtlases>0 and "
            "Events.AfterModsDeactivate.Count()==1 and "
            "Events.AfterModsActivate.Count()==1)",
            "=Civilopedia-preamble-owner-check");
        result.first_native_control = shape.first_native_control;
        result.first_native_line = shape.first_native_line;
        result.authored_later_event_registrations =
            shape.authored_later_event_registrations;
        result.status = bind_manager_block
            ? SelectedBnwCivilopediaPreambleStatus::manager_block_bound
            : bind_prereq_tech
            ? SelectedBnwCivilopediaPreambleStatus::prereq_tech_bound
            : bind_article_list
                ? SelectedBnwCivilopediaPreambleStatus::article_list_bound
                : SelectedBnwCivilopediaPreambleStatus::preamble_executed;
        result.bound_article_list_managers = bind_article_list ? 2 : 0;
        result.bound_prereq_tech_managers = bind_prereq_tech ? 1 : 0;
        result.bound_manager_count = bind_manager_block ? managers.bindings.size() : 0;
        result.next_native_control = bind_manager_block
            ? "UIManager.GetScreenSizeVal"
            : bind_prereq_tech
            ? prereq.next_native_control
            : bind_article_list ? "PrereqTechInnerFrame" : "ListOfArticles";
        result.blocker = bind_manager_block
            ? managers.blocker : bind_prereq_tech
            ? prereq.blocker : bind_article_list ? article.blocker : shape.blocker;
    } catch (const std::exception &error) {
        result.status = SelectedBnwCivilopediaPreambleStatus::source_blocked;
        result.blocker = error.what();
    }
    return result;
}

SelectedBnwCivilopediaPreamble run_selected_bnw_stock_ui_context_diagnostic(
    const fs::path &game_root, const content::Catalog &catalog,
    const runtime::SavedContentProfile &profile,
    const SelectedBnwPreviewHost &selected) {
    return run_source_order(game_root, catalog, profile, selected,
                            false, false, false);
}

SelectedBnwCivilopediaPreamble execute_selected_bnw_article_list_binding(
    const fs::path &game_root, const content::Catalog &catalog,
    const runtime::SavedContentProfile &profile,
    const SelectedBnwPreviewHost &selected) {
    return run_source_order(game_root, catalog, profile, selected,
                            true, false, false);
}

SelectedBnwCivilopediaPreamble execute_selected_bnw_prereq_tech_binding(
    const fs::path &game_root, const content::Catalog &catalog,
    const runtime::SavedContentProfile &profile,
    const SelectedBnwPreviewHost &selected) {
    return run_source_order(game_root, catalog, profile, selected,
                            true, true, false);
}

SelectedBnwCivilopediaPreamble execute_selected_bnw_manager_block_binding(
    const fs::path &game_root, const content::Catalog &catalog,
    const runtime::SavedContentProfile &profile,
    const SelectedBnwPreviewHost &selected) {
    return run_source_order(game_root, catalog, profile, selected,
                            true, true, true);
}

SelectedBnwCivilopediaPreamble execute_selected_bnw_civilopedia_preamble(
    const fs::path &game_root, const content::Catalog &catalog,
    const runtime::SavedContentProfile &profile,
    const SelectedBnwPreviewHost &selected) {
    return run_selected_bnw_stock_ui_context_diagnostic(
        game_root, catalog, profile, selected);
}

} // namespace civ5::ui
