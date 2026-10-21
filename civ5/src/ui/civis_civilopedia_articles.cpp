#include "civ5/ui/civis_civilopedia_articles.hpp"
#include "civ5/ui/selected_bnw_civilopedia_preamble.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <array>
#include <fstream>
#include <stdexcept>

namespace civ5::ui {
namespace {
namespace fs = std::filesystem;

std::string read_pinned(const fs::path &path, const std::string &sha) {
    const auto before = runtime::identify_file(path);
    if (before.sha256 != sha || before.bytes > 64ULL * 1024 * 1024)
        throw std::runtime_error("Selected Civilopedia article source identity changed");
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Selected Civilopedia article source cannot open");
    const std::string text((std::istreambuf_iterator<char>(input)),
                           std::istreambuf_iterator<char>());
    if (!input.eof() && input.fail())
        throw std::runtime_error("Selected Civilopedia article source cannot read");
    if (runtime::identify_file(path).sha256 != sha)
        throw std::runtime_error("Selected Civilopedia article source changed during read");
    return text;
}

bool ordered(std::string_view source,
             std::initializer_list<std::string_view> tokens) {
    std::size_t cursor = 0;
    for (const auto token : tokens) {
        const auto found = source.find(token, cursor);
        if (found == std::string_view::npos) return false;
        cursor = found + token.size();
    }
    return true;
}

std::string_view section(std::string_view source,
                         std::string_view start,
                         std::string_view next) {
    const auto begin = source.find(start);
    if (begin == std::string_view::npos ||
        source.find(start, begin + 1) != std::string_view::npos) return {};
    const auto end = source.find(next, begin + start.size());
    if (end == std::string_view::npos) return {};
    return source.substr(begin, end - begin);
}

const content::XmlNode *unique_instance(const content::XmlNode &layout,
                                        std::string_view name) {
    const content::XmlNode *found = nullptr;
    for (const auto &child : layout.children)
        if (child.name == "Instance" && child.attribute("Name") == name) {
            if (found) return nullptr;
            found = &child;
        }
    return found;
}

bool same_owner(const CivilopediaLayoutOwnerKey &a,
                const CivilopediaLayoutOwnerKey &b) {
    return a.frame_number == b.frame_number && a.owner_token == b.owner_token &&
           a.player_id == b.player_id && a.game_turn == b.game_turn &&
           a.payload_sha256 == b.payload_sha256 &&
           a.rules_sha256 == b.rules_sha256 &&
           a.styles_sha256 == b.styles_sha256 &&
           a.civilopedia_sha256 == b.civilopedia_sha256 &&
           a.xml_sha256 == b.xml_sha256;
}

bool within(const WorldViewRect &inner, const WorldViewRect &outer) {
    return inner.width > 0 && inner.height > 0 && outer.width > 0 &&
           outer.height > 0 && inner.x >= outer.x && inner.y >= outer.y &&
           std::int64_t(inner.x) + inner.width <= std::int64_t(outer.x) + outer.width &&
           std::int64_t(inner.y) + inner.height <= std::int64_t(outer.y) + outer.height;
}

} // namespace

CivisHomeArticleSource inspect_bnw_home_article_source(
    const SelectedBnwCivilopediaLayoutSources &sources) {
    CivisHomeArticleSource result;
    try {
        if (!sources.verified || !sources.control_tree.verified)
            throw std::runtime_error("Selected Civilopedia source is unverified");
        const auto root = fs::path(sources.styles_path)
            .parent_path().parent_path().parent_path();
        const auto civilopedia = read_pinned(
            root / "Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua",
            sources.civilopedia_sha256);
        const auto instance = read_pinned(root / "Assets/UI/InstanceManager.lua",
                                          sources.instance_manager_sha256);
        const auto xml_path = root /
            "Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml";
        if (runtime::identify_file(xml_path).sha256 != sources.xml_sha256)
            throw std::runtime_error("Selected Civilopedia article XML identity changed");
        const auto layout = content::read_xml(xml_path);
        if (runtime::identify_file(xml_path).sha256 != sources.xml_sha256)
            throw std::runtime_error("Selected Civilopedia article XML changed during read");
        const auto manager = inspect_civilopedia_article_list_control(
            civilopedia, layout, instance);
        if (!manager.verified || manager.parent_id != "LeftScrollPanel" ||
            manager.item_instance != "ListItemInstance" ||
            manager.item_root != "ListItemButton" ||
            civilopedia.find("local CategoryHomePage = 1;") == std::string::npos ||
            civilopedia.find("local numCategories = 16;") == std::string::npos ||
            civilopedia.find("local selectedCategory = CategoryHomePage;") == std::string::npos)
            throw std::runtime_error("Selected HomePage/InstanceManager owner contract changed");
        const auto populate = section(civilopedia,
            "CivilopediaCategory[CategoryHomePage].PopulateList = function()",
            "CivilopediaCategory[CategoryGameConcepts].PopulateList = function()");
        const auto display = section(civilopedia,
            "CivilopediaCategory[CategoryHomePage].DisplayList = function(",
            "CivilopediaCategory[CategoryGameConcepts].DisplayList = function(");
        if (populate.empty() || display.empty() ||
            !ordered(populate, {
                "for i=1, numCategories,1 do",
                "local compoundName = \"TXT_KEY_PEDIA_CATEGORY_\" .. tostring(i) .. \"_LABEL\"",
                "article.entryName = name;", "article.entryID = i;",
                "sortedList[CategoryHomePage][1][tableid] = article;"}) ||
            !ordered(display, {
                "g_ListHeadingManager:ResetInstances();",
                "g_ListItemManager:ResetInstances();",
                "for i, v in ipairs(sortedList[CategoryHomePage][1]) do",
                "local thisListInstance = g_ListItemManager:GetInstance();",
                "thisListInstance.ListItemLabel:SetText( v.entryName );",
                "thisListInstance.ListItemButton:SetVoids( v.entryID, addToList );",
                "thisListInstance.ListItemButton:RegisterCallback( Mouse.eLClick, CivilopediaCategory[CategoryHomePage].SelectArticle );",
                "Controls.ListOfArticles:SortChildren( SortFunction );",
                "ResizeEtc();"}) ||
            instance.find("ContextPtr:BuildInstanceForControl(self.m_InstanceName, controlTable, self.m_ParentControl);") ==
                std::string::npos)
            throw std::runtime_error("Selected HomePage article creation order changed");
        const auto *list = unique_instance(layout, "ListItemInstance");
        const auto *button = list ? list->child("Button") : nullptr;
        const auto *label = button ? button->child("Label") : nullptr;
        if (!button || button->attribute("ID") != "ListItemButton" ||
            button->attribute("Size") != "256,22" ||
            button->attribute("Offset") != "0,0" ||
            !label || label->attribute("ID") != "ListItemLabel" ||
            label->attribute("Offset") != "12,-2" ||
            label->attribute("TruncateWidth") != "244")
            throw std::runtime_error("Selected ListItemInstance button/label geometry changed");
        result.verified = true;
        result.lua_sha256 = sources.civilopedia_sha256;
        result.xml_sha256 = sources.xml_sha256;
        result.instance_manager_sha256 = sources.instance_manager_sha256;
        result.blocker = "HomePage templates verified; native BuildInstanceForControl, "
                         "Stack CalculateSize and ScrollPanel CalculateInternalSize unavailable";
    } catch (const std::exception &error) {
        result.blocker = error.what();
    }
    return result;
}

CivisHomeArticleSubtree::CivisHomeArticleSubtree(
    CivilopediaLayoutOwnerKey owner, std::int32_t viewport_width,
    std::int32_t viewport_height, std::int32_t left_panel_width,
    std::int32_t left_panel_height, std::vector<CivisHomeArticleItem> items)
    : owner_(std::move(owner)), viewport_width_(viewport_width),
      viewport_height_(viewport_height), left_panel_width_(left_panel_width),
      left_panel_height_(left_panel_height), items_(std::move(items)) {}

std::unique_ptr<CivisHomeArticleSubtree> CivisHomeArticleSubtree::open(
    const SelectedBnwCivilopediaLayoutSources &sources,
    const CivisCivilopediaControlTree &controls,
    const CivilopediaLayoutOwnerKey &current_owner,
    std::int32_t selected_category_id,
    std::int32_t viewport_width, std::int32_t viewport_height,
    std::string &blocker) {
    blocker.clear();
    const auto source = inspect_bnw_home_article_source(sources);
    const auto panel = controls.get_size(current_owner, "LeftScrollPanel");
    if (!source.verified || !same_owner(controls.owner(), current_owner) ||
        controls.completed_operations() != 9 || selected_category_id != 1 ||
        !panel.available || panel.width <= 0 || panel.height <= 0 ||
        viewport_width <= 0 || viewport_height <= 0) {
        blocker = source.verified
            ? "HomePage article owner/category or Civis panel layout is unavailable"
            : source.blocker;
        return {};
    }
    std::vector<CivisHomeArticleItem> items;
    items.reserve(16);
    for (std::int32_t id = 1; id <= 16; ++id) {
        items.push_back({id,
            "TXT_KEY_PEDIA_CATEGORY_" + std::to_string(id) + "_LABEL",
            "ListItemInstance#" + std::to_string(id), "ListOfArticles",
            {"ListItemButton", "ListOfArticles", 256, 22, 0, 0},
            {"ListItemLabel", "ListItemButton", 12, -2, 244},
            std::nullopt});
    }
    return std::unique_ptr<CivisHomeArticleSubtree>(new CivisHomeArticleSubtree(
        current_owner, viewport_width, viewport_height, panel.width,
        panel.height, std::move(items)));
}

CivisHomeArticleResult CivisHomeArticleSubtree::item(
    const CivilopediaLayoutOwnerKey &current_owner,
    std::int32_t entry_id) const {
    if (!same_owner(owner_, current_owner))
        return {false, {}, "HomePage article subtree belongs to another frame/owner"};
    if (entry_id < 1 || entry_id > std::int32_t(items_.size()))
        return {false, {}, "HomePage article entry ID is outside selected source"};
    return {true, items_[std::size_t(entry_id - 1)], {}};
}

CivisControlGeometryResult CivisHomeArticleSubtree::list_stack(
    const CivilopediaLayoutOwnerKey &current_owner) const {
    if (!same_owner(owner_, current_owner))
        return {false, {}, "ListOfArticles belongs to another frame/owner"};
    return {true, list_stack_, {}};
}

std::optional<CivisHomeArticleRowsMeasurement>
CivisHomeArticleSubtree::measured_rows(
    const CivilopediaLayoutOwnerKey &current_owner) const {
    if (!same_owner(owner_, current_owner)) return std::nullopt;
    return measured_rows_;
}

bool CivisHomeArticleSubtree::attach_measured_rows(
    const CivilopediaLayoutOwnerKey &current_owner,
    const CivisHomeArticleRowsMeasurement &measurement,
    std::string &blocker) {
    blocker.clear();
    if (!same_owner(owner_, current_owner) ||
        !same_owner(owner_, measurement.owner) ||
        measurement.button_bounds.size() != items_.size()) {
        blocker = "HomePage row measurements have a stale owner or wrong count";
        return false;
    }
    const WorldViewRect viewport{0, 0, viewport_width_, viewport_height_};
    if (!within(measurement.left_panel, viewport) ||
        measurement.left_panel.width != left_panel_width_ ||
        measurement.left_panel.height != left_panel_height_ ||
        !within(measurement.list_stack, measurement.left_panel) ||
        measurement.list_stack.x != measurement.left_panel.x + 7 ||
        measurement.list_stack.y != measurement.left_panel.y) {
        blocker = "HomePage measured panel/Stack differs from owned viewport and XML";
        return false;
    }
    std::int64_t previous_bottom = measurement.left_panel.y;
    for (const auto &button : measurement.button_bounds) {
        if (!within(button, measurement.left_panel) ||
            button.x != measurement.list_stack.x || button.width != 256 ||
            button.height != 22 || button.y < previous_bottom) {
            blocker = "HomePage measured button geometry/order differs from source";
            return false;
        }
        previous_bottom = std::int64_t(button.y) + button.height;
    }
    for (std::size_t i = 0; i < items_.size(); ++i)
        items_[i].measured_button_bounds = measurement.button_bounds[i];
    measured_rows_ = measurement;
    measured_rows_available_ = true;
    return true;
}

} // namespace civ5::ui
