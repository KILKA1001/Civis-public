#include "civ5/ui/selected_bnw_civilopedia_layout.hpp"
#include "civ5/ui/selected_bnw_civilopedia_preamble.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <set>
#include <span>
#include <stdexcept>

namespace civ5::ui {
namespace {
namespace fs = std::filesystem;

std::string read_verified(const fs::path &path, std::string &sha) {
    const auto identity = runtime::identify_file(path);
    if (identity.bytes > 64ULL * 1024 * 1024)
        throw std::runtime_error("Selected Civilopedia layout source exceeds 64 MiB");
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Selected Civilopedia layout source cannot be opened");
    const std::string source((std::istreambuf_iterator<char>(input)),
                              std::istreambuf_iterator<char>());
    if (!input.eof() && input.fail())
        throw std::runtime_error("Selected Civilopedia layout source cannot be read");
    const auto bytes = std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t *>(source.data()), source.size());
    if (runtime::identify_bytes(bytes).sha256 != identity.sha256)
        throw std::runtime_error("Selected Civilopedia layout source changed during read");
    sha = identity.sha256;
    return source;
}

void find_style(const content::XmlNode &node, std::size_t &count,
                const content::XmlNode *&found) {
    if (node.name == "VertSlider") { ++count; found = &node; }
    for (const auto &child : node.children) find_style(child, count, found);
}

std::string trim(std::string line) {
    const auto begin = line.find_first_not_of(" \t\r");
    if (begin == std::string::npos) return {};
    const auto end = line.find_last_not_of(" \t\r");
    return line.substr(begin, end - begin + 1);
}

bool exact_layout_source(std::string_view source) {
    constexpr std::array<std::string_view, 18> lines = {
        "local _, screenSizeY = UIManager:GetScreenSizeVal(); -- Controls.BackDrop:GetSize();",
        "local scrollBarWidth = Controls.ScrollBar:GetSize().x;",
        "local scrollPanelSize = Controls.ScrollPanel:GetSize();",
        "local scrollBarSize = { x = scrollBarWidth; y = screenSizeY - 172; };",
        "Controls.ScrollBar:SetSize( scrollBarSize );",
        "Controls.LeftScrollBar:SetSize( scrollBarSize );",
        "Controls.ScrollBar:SetOffsetVal( 0, 18 );",
        "Controls.LeftScrollBar:SetOffsetVal( 0, 18 );",
        "Controls.DownButton:SetOffsetVal( 0, screenSizeY - 126 - 18 );",
        "Controls.LeftDownButton:SetOffsetVal( 0, screenSizeY - 126 - 18 );",
        "Controls.LeftUpButton:SetOffsetVal( 0, 0 );",
        "scrollPanelSize.y = screenSizeY - 126;",
        "Controls.ScrollPanel:SetSize( scrollPanelSize );",
        "local leftScrollPanelSize = Controls.LeftScrollPanel:GetSize();",
        "leftScrollPanelSize.y = screenSizeY - 126;",
        "Controls.LeftScrollPanel:SetSize( leftScrollPanelSize );",
        "Controls.ScrollPanel:CalculateInternalSize();",
        "Controls.LeftScrollPanel:CalculateInternalSize();",
    };
    const auto start = source.find(lines.front());
    if (start == std::string_view::npos ||
        source.find(lines.front(), start + 1) != std::string_view::npos)
        return false;
    auto position = start;
    for (const auto expected : lines) {
        const auto end = source.find('\n', position);
        if (end == std::string_view::npos ||
            trim(std::string(source.substr(position, end - position))) != expected)
            return false;
        position = end + 1;
    }
    return true;
}

bool same_owner(const CivilopediaLayoutOwnerKey &a,
                const CivilopediaLayoutOwnerKey &b) {
    return a.frame_number == b.frame_number &&
           a.owner_token == b.owner_token &&
           a.player_id == b.player_id && a.game_turn == b.game_turn &&
           a.payload_sha256 == b.payload_sha256 &&
           a.rules_sha256 == b.rules_sha256 &&
           a.styles_sha256 == b.styles_sha256 &&
           a.civilopedia_sha256 == b.civilopedia_sha256 &&
           a.xml_sha256 == b.xml_sha256;
}

bool same_instruction(const CivilopediaLayoutInstruction &a,
                      const CivilopediaLayoutInstruction &b) {
    return a.operation == b.operation && a.control_id == b.control_id &&
           a.x == b.x && a.y == b.y && a.authored_line == b.authored_line;
}

void find_owned_control(const content::XmlNode &node,
                        const content::XmlNode *parent, std::string_view id,
                        const content::XmlNode *&found,
                        const content::XmlNode *&found_parent,
                        std::size_t &count) {
    if (node.attribute("ID") == id) {
        found = &node;
        found_parent = parent;
        ++count;
    }
    for (const auto &child : node.children)
        find_owned_control(child, &node, id, found, found_parent, count);
}

} // namespace

CivilopediaNativeControlTree inspect_bnw_civilopedia_native_control_tree(
    const content::XmlNode &layout) {
    CivilopediaNativeControlTree result;
    if (layout.name != "Context") {
        result.blocker = "Selected Civilopedia native control root is not Context";
        return result;
    }
    struct Required {
        std::string_view id, kind, parent_id, style;
    };
    constexpr std::array<Required, 8> required = {{
        {"ScrollPanel", "ScrollPanel", "", ""},
        {"ScrollBar", "ScrollBar", "ScrollPanel", "VertSlider"},
        {"UpButton", "UpButton", "ScrollPanel", "ScrollBarUp"},
        {"DownButton", "DownButton", "ScrollPanel", "ScrollBarDown"},
        {"LeftScrollPanel", "ScrollPanel", "", ""},
        {"LeftScrollBar", "ScrollBar", "LeftScrollPanel", "VertSlider"},
        {"LeftUpButton", "UpButton", "LeftScrollPanel", "ScrollBarUp"},
        {"LeftDownButton", "DownButton", "LeftScrollPanel", "ScrollBarDown"},
    }};
    std::array<const content::XmlNode *, required.size()> nodes{};
    std::array<const content::XmlNode *, required.size()> parents{};
    for (std::size_t i = 0; i < required.size(); ++i) {
        std::size_t count = 0;
        find_owned_control(layout, nullptr, required[i].id,
                           nodes[i], parents[i], count);
        if (count != 1 || !nodes[i] || !parents[i] ||
            nodes[i]->name != required[i].kind ||
            nodes[i]->attribute("Style") != required[i].style ||
            parents[i]->attribute("ID") != required[i].parent_id) {
            result.blocker = "Selected Civilopedia native control ID, kind, style or parent changed: " +
                             std::string(required[i].id);
            return result;
        }
        result.controls.push_back({std::string(required[i].id),
                                   std::string(required[i].kind),
                                   std::string(required[i].parent_id),
                                   std::string(required[i].style)});
    }
    if (parents[0] != parents[4] || parents[0]->name != "Box" ||
        parents[1] != nodes[0] || parents[2] != nodes[0] ||
        parents[3] != nodes[0] || parents[5] != nodes[4] ||
        parents[6] != nodes[4] || parents[7] != nodes[4] ||
        nodes[0]->attribute("Size") != "1002,980" ||
        nodes[0]->attribute("Vertical") != "1" ||
        nodes[0]->attribute("AutoScrollBar") != "1" ||
        nodes[4]->attribute("Size") != "294,980" ||
        nodes[4]->attribute("Vertical") != "1" ||
        nodes[1]->attribute("Length") != "800" ||
        nodes[5]->attribute("Length") != "930" ||
        !nodes[1]->attribute("Size").empty() ||
        !nodes[5]->attribute("Size").empty()) {
        result.blocker = "Selected Civilopedia ScrollPanel/scrollbar hierarchy changed";
        result.controls.clear();
        return result;
    }
    result.verified = true;
    result.blocker = "XML identities are verified; native control instances are unavailable";
    return result;
}

SelectedBnwCivilopediaLayoutSources inspect_bnw_civilopedia_layout_sources(
    const fs::path &game_root) {
    SelectedBnwCivilopediaLayoutSources result;
    try {
        const auto root = fs::canonical(game_root);
        const auto expansion2 = root / "Assets/DLC/Expansion2/UI/Civilopedia";
        const auto script = read_verified(expansion2 / "CivilopediaScreen.lua",
                                          result.civilopedia_sha256);
        const auto instance = read_verified(root / "Assets/UI/InstanceManager.lua",
                                            result.instance_manager_sha256);
        const auto xml_path = expansion2 / "CivilopediaScreen.xml";
        result.xml_sha256 = runtime::identify_file(xml_path).sha256;
        const auto layout = content::read_xml(xml_path);
        if (runtime::identify_file(xml_path).sha256 != result.xml_sha256)
            throw std::runtime_error("Selected Civilopedia XML changed during read");
        result.control_tree = inspect_bnw_civilopedia_native_control_tree(layout);
        if (!result.control_tree.verified)
            throw std::runtime_error(result.control_tree.blocker);
        const auto managers = inspect_civilopedia_manager_block(script, layout, instance);
        if (!managers.verified || managers.next_native_line != 113 ||
            !exact_layout_source(script))
            throw std::runtime_error("Selected Civilopedia native layout source changed: " +
                                     managers.blocker);
        const auto style_path = root / "Assets/UI/Styles.xml";
        result.styles_path = style_path.generic_string();
        result.styles_sha256 = runtime::identify_file(style_path).sha256;
        const auto styles = content::read_xml(style_path);
        std::size_t count = 0;
        const content::XmlNode *slider = nullptr;
        find_style(styles, count, slider);
        if (styles.name != "StyleSheet" || count != 1 || !slider ||
            slider->attribute("Size") != "18,18" ||
            slider->attribute("Vertical") != "1")
            throw std::runtime_error("Selected base VertSlider style is ambiguous");
        result.authored_vert_slider_width = 18;
        result.verified = true;
        result.blocker = "Styles.xml is a source candidate; native VFS/style resolution "
                         "and CalculateInternalSize remain unverified";
    } catch (const std::exception &error) {
        result.blocker = error.what();
    }
    return result;
}

SelectedBnwCivilopediaVfsStyle resolve_bnw_civilopedia_vert_slider_style(
    const assets::ResourceMountTable &resources,
    const SelectedBnwCivilopediaLayoutSources &sources) {
    SelectedBnwCivilopediaVfsStyle result;
    result.requested_key = "Assets/UI/Styles.xml";
    try {
        if (!sources.verified || !sources.control_tree.verified ||
            sources.styles_path.empty())
            throw std::runtime_error("Selected Civilopedia source/style preflight is incomplete");
        const assets::AssetKey key(result.requested_key);
        const auto mounted = resources.resolve(key);
        const auto *handle = std::get_if<assets::AssetHandle>(&mounted.asset);
        if (!handle)
            throw std::runtime_error("Selected VFS Styles.xml is absent or ambiguous");
        const auto &origin = handle->candidate().provenance;
        const auto expected_root = fs::canonical(
            fs::path(sources.styles_path).parent_path().parent_path().parent_path());
        if (origin.kind != assets::SourceKind::loose ||
            origin.archive_relative_path.has_value() ||
            !fs::equivalent(fs::path(origin.source_id), expected_root) ||
            assets::AssetKey(origin.source_relative_path).normalized_name() !=
                key.normalized_name())
            throw std::runtime_error("Selected VFS Styles.xml does not belong to pinned base UI");
        std::set<std::pair<std::string, std::optional<std::string>>> same_source;
        for (const auto &candidate : resources.resolve_all(key)) {
            const auto *other = std::get_if<assets::AssetHandle>(&candidate.asset);
            if (!other || candidate.mount_id != mounted.mount_id) continue;
            const auto &other_origin = other->candidate().provenance;
            if (other_origin.kind != origin.kind ||
                other_origin.source_id != origin.source_id) continue;
            same_source.emplace(other_origin.source_relative_path,
                                other_origin.archive_relative_path);
        }
        if (same_source.size() != 1)
            throw std::runtime_error("Selected VFS Styles.xml source has ambiguous records");
        const auto bytes = handle->read_bytes(64ULL * 1024 * 1024);
        const auto sha = runtime::identify_bytes(std::span<const std::uint8_t>(bytes)).sha256;
        if (sha != sources.styles_sha256)
            throw std::runtime_error("Selected VFS Styles.xml bytes differ from inspected source");
        const auto xml = content::read_xml_metadata(std::string(bytes.begin(), bytes.end()));
        std::size_t count = 0;
        const content::XmlNode *slider = nullptr;
        find_style(xml, count, slider);
        if (xml.name != "StyleSheet" || count != 1 || !slider ||
            slider->attribute("Size") != "18,18" ||
            slider->attribute("Vertical") != "1")
            throw std::runtime_error("Selected VFS VertSlider style changed");
        result.selected_source_verified = true;
        result.source_id = origin.source_id;
        result.source_relative_path = origin.source_relative_path;
        result.mount_id = mounted.mount_id;
        result.sha256 = sha;
        result.authored_vert_slider_width = sources.authored_vert_slider_width;
        result.blocker = "Selected VertSlider width is source-verified; native "
                         "ScrollBar:GetSize and CalculateInternalSize remain unavailable";
    } catch (const std::exception &error) {
        result.blocker = error.what();
    }
    return result;
}

SelectedBnwCivilopediaLayoutPreflight plan_bnw_civilopedia_native_layout(
    const SelectedBnwCivilopediaLayoutSources &sources,
    const runtime::SessionCheckpointIdentity &identity,
    std::int64_t active_player_id, std::int64_t current_turn,
    std::uint64_t expected_frame_number,
    const CivilopediaMeasuredLayout &measured) {
    SelectedBnwCivilopediaLayoutPreflight result;
    if (!sources.verified || expected_frame_number == 0 ||
        measured.frame_number != expected_frame_number ||
        measured.owner_token.empty() ||
        measured.player_id != active_player_id ||
        measured.game_turn != current_turn ||
        measured.payload_sha256 != identity.payload_sha256 ||
        measured.rules_sha256 != identity.rules_sha256 ||
        measured.resolved_styles_sha256 != sources.styles_sha256 ||
        measured.viewport_width <= 0 || measured.viewport_height <= 172 ||
        measured.scrollbar_width <= 0 ||
        measured.scrollbar_width > measured.viewport_width ||
        measured.scroll_panel_width <= 0 ||
        measured.scroll_panel_height <= 0 ||
        measured.left_panel_width <= 0 ||
        measured.left_panel_height <= 0) {
        result.blocker = "Civilopedia owner frame, style provenance or measured geometry is stale/incomplete";
        return result;
    }
    const auto height = measured.viewport_height;
    const auto add = [&result](CivilopediaLayoutOperation operation,
                               const char *control, std::int32_t x,
                               std::int32_t y, std::size_t line) {
        result.instructions.push_back({operation, control, x, y, line});
    };
    add(CivilopediaLayoutOperation::set_size, "ScrollBar",
        measured.scrollbar_width, height - 172, 117);
    add(CivilopediaLayoutOperation::set_size, "LeftScrollBar",
        measured.scrollbar_width, height - 172, 118);
    add(CivilopediaLayoutOperation::set_offset, "ScrollBar", 0, 18, 119);
    add(CivilopediaLayoutOperation::set_offset, "LeftScrollBar", 0, 18, 120);
    add(CivilopediaLayoutOperation::set_offset, "DownButton", 0, height - 144, 121);
    add(CivilopediaLayoutOperation::set_offset, "LeftDownButton", 0, height - 144, 122);
    add(CivilopediaLayoutOperation::set_offset, "LeftUpButton", 0, 0, 123);
    add(CivilopediaLayoutOperation::set_size, "ScrollPanel",
        measured.scroll_panel_width, height - 126, 125);
    add(CivilopediaLayoutOperation::set_size, "LeftScrollPanel",
        measured.left_panel_width, height - 126, 128);
    add(CivilopediaLayoutOperation::calculate_internal_size, "ScrollPanel", 0, 0, 129);
    add(CivilopediaLayoutOperation::calculate_internal_size, "LeftScrollPanel", 0, 0, 130);
    result.owner = {measured.frame_number, measured.owner_token, measured.player_id,
                    measured.game_turn, measured.payload_sha256,
                    measured.rules_sha256, measured.resolved_styles_sha256,
                    sources.civilopedia_sha256, sources.xml_sha256};
    result.source_and_owner_verified = true;
    result.blocker = "Native VFS/style resolution and CalculateInternalSize effects are not "
                     "executed; Civilopedia callbacks and context remain unavailable";
    return result;
}

CivilopediaNativeLayoutReceiptValidation validate_bnw_civilopedia_layout_receipt(
    const SelectedBnwCivilopediaLayoutPreflight &plan,
    const CivilopediaLayoutOwnerKey &current_owner,
    const CivilopediaNativeLayoutReceipt &receipt) {
    CivilopediaNativeLayoutReceiptValidation result;
    if (!plan.source_and_owner_verified || plan.owner.frame_number == 0 ||
        plan.owner.owner_token.empty() ||
        !same_owner(plan.owner, current_owner) ||
        !same_owner(plan.owner, receipt.owner)) {
        result.blocker = "Civilopedia native-layout receipt has a stale or different owner";
        return result;
    }
    if (receipt.operations.size() != plan.instructions.size()) {
        result.blocker = "Civilopedia native-layout receipt has a different operation count";
        return result;
    }
    std::size_t internal = 0;
    for (std::size_t i = 0; i < plan.instructions.size(); ++i) {
        const auto &expected = plan.instructions[i];
        const auto &observed = receipt.operations[i];
        if (!observed.completed || !same_instruction(expected, observed.instruction)) {
            result.blocker = "Civilopedia native-layout receipt changed the authored operation order";
            return result;
        }
        if (expected.operation == CivilopediaLayoutOperation::calculate_internal_size) {
            if (internal >= receipt.internal_size_results.size() ||
                !receipt.internal_size_results[internal].completed ||
                receipt.internal_size_results[internal].control_id != expected.control_id) {
                result.blocker = "Civilopedia CalculateInternalSize result is absent or mismatched";
                return result;
            }
            ++internal;
        }
    }
    if (internal != receipt.internal_size_results.size()) {
        result.blocker = "Civilopedia CalculateInternalSize result count differs from source";
        return result;
    }
    result.receipt_matches_plan = true;
    result.blocker = "Owner receipt structure matches; native Controls adapter and live "
                     "Civilopedia context remain unavailable";
    return result;
}

SelectedBnwCivilopediaLayoutPreflight preflight_selected_bnw_civilopedia_layout(
    const fs::path &game_root,
    const assets::ResourceMountTable &selected_ui_resources,
    const content::Catalog &catalog,
    const runtime::SavedContentProfile &profile,
    const SelectedBnwPreviewHost &selected,
    std::uint64_t expected_frame_number,
    const CivilopediaMeasuredLayout &measured) {
    SelectedBnwCivilopediaLayoutPreflight blocked;
    try {
        if (!profile.can_activate || !profile.all_requirements_located ||
            !profile.enabled_mods.empty())
            throw std::runtime_error("Selected BNW layout profile is incomplete or includes mods");
        constexpr std::string_view expansion2_id =
            "6da07636-4123-4018-b643-6575b4ec336b";
        const auto root = fs::canonical(game_root);
        std::size_t expansion2_count = 0;
        for (const auto index : profile.enabled_dlc) {
            if (index >= catalog.packages.size())
                throw std::runtime_error("Selected BNW DLC index is outside catalog");
            const auto &package = catalog.packages[index];
            if (package.kind != content::PackageKind::dlc)
                throw std::runtime_error("Selected BNW package is not DLC");
            const auto package_root = fs::canonical(package.path.parent_path());
            if (content::canonical_guid(package.id) == expansion2_id) {
                ++expansion2_count;
                if (!fs::equivalent(package_root, root / "Assets/DLC/Expansion2"))
                    throw std::runtime_error("Selected Expansion2 UI source differs from game root");
            }
            const auto ui = package_root / "UI";
            if (!fs::exists(ui)) continue;
            for (const auto &entry : fs::recursive_directory_iterator(ui))
                if (entry.is_regular_file() &&
                    entry.path().filename() == fs::path("Styles.xml"))
                    throw std::runtime_error(
                        "Selected DLC overrides Styles.xml; native VFS order unresolved");
        }
        if (expansion2_count != 1)
            throw std::runtime_error("Selected BNW Expansion2 source is missing/ambiguous");
        const auto sources = inspect_bnw_civilopedia_layout_sources(game_root);
        const auto style = resolve_bnw_civilopedia_vert_slider_style(
            selected_ui_resources, sources);
        if (!style.selected_source_verified)
            throw std::runtime_error("Selected BNW Styles VFS source is blocked: " +
                                     style.blocker);
        return plan_bnw_civilopedia_native_layout(
            sources, selected.identity(),
            selected.active_player_id(), selected.session().state().setup.game_turn,
            expected_frame_number, measured);
    } catch (const std::exception &error) {
        blocked.blocker = error.what();
        return blocked;
    }
}

} // namespace civ5::ui
