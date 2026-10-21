#include "civ5/ui/screen_compositor.hpp"
#include "civ5/ui/bnw_train_renderer_refresh_receipt.hpp"
#include "civ5/ui/ideology_layout.hpp"
#include "civ5/ui/economic_overview_fields.hpp"
#include "civ5/ui/ui_owner_fences.hpp"
#include "civ5/runtime/bnw_research_choice_commit.hpp"
#include <algorithm>
#include <exception>
#include <charconv>
#include <limits>
#include <map>
#include <stdexcept>
#include <tuple>
#include <atomic>
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include "civ5/ui/bnw_notification_graphics_publication.hpp"
#include "civ5/runtime/bnw_owned_gameplay_event_pump_loan.hpp"
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"

namespace civ5::ui {
namespace {

void append_hits(ComposedUiFrame &result, std::string_view context,
                 const std::vector<WorldViewHit> &hits) {
    for (const auto &hit : hits)
        result.hits.push_back({hit.bounds, std::string(context), hit.command,
                               hit.enabled, hit.unavailable_reason});
}

bool valid_bounds(const WorldViewRect &rect, std::int32_t width,
                  std::int32_t height) {
    return rect.x >= 0 && rect.y >= 0 && rect.width > 0 && rect.height > 0 &&
           static_cast<std::int64_t>(rect.x) + rect.width <= width &&
           static_cast<std::int64_t>(rect.y) + rect.height <= height;
}

bool point_inside(const WorldViewRect &rect, std::int32_t x, std::int32_t y) {
    return rect.width > 0 && rect.height > 0 && x >= rect.x && y >= rect.y &&
           static_cast<std::int64_t>(x) < static_cast<std::int64_t>(rect.x) + rect.width &&
           static_cast<std::int64_t>(y) < static_cast<std::int64_t>(rect.y) + rect.height;
}

std::optional<WorldViewRect> clipped(const WorldViewRect &row,
                                     const WorldViewRect &panel) {
    const auto left = std::max(row.x, panel.x);
    const auto top = std::max(row.y, panel.y);
    const auto right = std::min(static_cast<std::int64_t>(row.x) + row.width,
                                static_cast<std::int64_t>(panel.x) + panel.width);
    const auto bottom = std::min(static_cast<std::int64_t>(row.y) + row.height,
                                 static_cast<std::int64_t>(panel.y) + panel.height);
    if (right <= left || bottom <= top) return std::nullopt;
    return WorldViewRect{left, top, static_cast<std::int32_t>(right - left),
                         static_cast<std::int32_t>(bottom - top)};
}

void append_production_hits(ComposedUiFrame &result, const CityScreenSource &source,
    const CityScreenPlan &plan, const UiFrameSnapshot &frame,
    UiGameplayBackend *backend, const MeasuredProductionPopup *measured,
    std::int32_t width, std::int32_t height) {
    const auto *choices = frame.find<ProductionPopupChoices>();
    const auto *city = selected_city(frame);
    const bool matched = measured && choices && city &&
        frame.active_player_id() && frame.turn() &&
        choices->player_id == *frame.active_player_id() &&
        choices->city_id == city->id && choices->turn == *frame.turn() &&
        measured->selected_source_path == source.selected_source_path &&
        choices->selected_source_path == source.selected_source_path &&
        choices->backend_owner == backend &&
        choices->visible_rows == production_popup_visible_rows(source, height) &&
        choices->visible_rows > 0 &&
        choices->first_visible <= choices->entries.size();
    std::optional<WorldViewRect> panel;
    if (matched && source.contains("ScrollPanel") && measured->scroll_panel &&
        valid_bounds(*measured->scroll_panel, width, height)) {
        panel = *measured->scroll_panel;
        result.production_scroll_panel = panel;
    }
    using Key = std::tuple<std::int64_t, std::int64_t, std::int64_t>;
    std::map<Key, std::vector<WorldViewRect>> rows;
    if (panel)
        for (const auto &row : measured->rows)
            rows[{row.choice.city_id, row.choice.order_type,
                  row.choice.item_id}].push_back(row.bounds);
    std::size_t unmeasured = 0;
    for (const auto &hit : plan.hits) {
        const auto *gameplay = std::get_if<UiGameplayCommand>(&hit.command);
        const auto *choice = gameplay ? gameplay->as<CityChooseProduction>() : nullptr;
        if (!choice) {
            result.hits.push_back({hit.bounds, source.context_id, hit.command,
                                   hit.enabled, hit.unavailable_reason});
            continue;
        }
        const auto found = rows.find({choice->city_id, choice->order_type,
                                      choice->item_id});
        if (found == rows.end() || found->second.size() != 1 ||
            !valid_bounds(found->second.front(), width, height)) {
            ++unmeasured;
            continue;
        }
        const auto visible = clipped(found->second.front(), *panel);
        if (!visible) { ++unmeasured; continue; }
        result.hits.push_back({*visible, source.context_id, hit.command,
                               hit.enabled, hit.unavailable_reason});
    }
    if (unmeasured)
        result.unresolved.push_back(
            "Visible production rows require unique presenter measurements");
    if (!panel) {
        result.unresolved.push_back(
            "Selected ProductionPopup ScrollPanel has no matching measurement");
        return;
    }
    const auto max_first = choices->entries.size() > choices->visible_rows
        ? choices->entries.size() - choices->visible_rows : 0;
    const auto append_button = [&](std::string_view control_id,
        const std::optional<WorldViewRect> &bounds,
        ProductionPopupScrollDirection direction, bool enabled,
        const char *reason) {
        if (!source.contains(control_id) || !bounds ||
            !valid_bounds(*bounds, width, height)) return;
        result.hits.push_back({*bounds, source.context_id,
            city_scroll_production_popup(choices->city_id, direction),
            enabled, enabled ? std::string{} : std::string(reason)});
    };
    append_button("UpButton", measured->up_button,
        ProductionPopupScrollDirection::up, choices->first_visible > 0,
        "Production popup is already at the first choice");
    append_button("DownButton", measured->down_button,
        ProductionPopupScrollDirection::down,
        choices->first_visible < max_first,
        "Production popup is already at the last choice");
}

void append_paints(ComposedUiFrame &result, const std::vector<WorldViewPaint> &paints,
                   const std::vector<std::string> &unresolved) {
    result.paints.insert(result.paints.end(), paints.begin(), paints.end());
    result.unresolved.insert(result.unresolved.end(), unresolved.begin(), unresolved.end());
}

void unavailable(ComposedUiFrame &result, std::string reason,
                 std::int32_t width, std::int32_t height) {
    result.unresolved.push_back(reason);
    const WorldViewRect panel{std::max(0, (width - 700) / 2),
                              std::max(0, (height - 180) / 2),
                              std::min(width, 700), std::min(height, 180)};
    result.paints.push_back({WorldViewPaintKind::rectangle, panel, {}, {}, {},
                             "Grid9DetailFive140", 0xffffffff});
    result.paints.push_back({WorldViewPaintKind::text,
                             {panel.x + 24, panel.y + 48,
                              std::max(0, panel.width - 48), 80}, {}, {},
                             "N/A: " + std::move(reason), "TwCenMT20", 0xffffffff});
}

std::optional<ProgressionScreen> progression_id(std::string_view id) {
    for (auto candidate : {ProgressionScreen::technologies, ProgressionScreen::policies,
                           ProgressionScreen::ideology,
                           ProgressionScreen::technology_popup})
        if (id == progression_context_id(candidate)) return candidate;
    return std::nullopt;
}

const content::XmlNode *progression_control(const content::XmlNode &node,
                                            std::string_view id) {
    if (node.attribute("ID") == id) return &node;
    for (const auto &child : node.children)
        if (const auto *found = progression_control(child, id)) return found;
    return nullptr;
}

std::optional<std::pair<std::int32_t, std::int32_t>> numeric_pair(
    const content::XmlNode &node, std::string_view attribute) {
    const auto text = node.attribute(std::string(attribute));
    const auto comma = text.find(',');
    if (comma == std::string::npos) return std::nullopt;
    try {
        std::size_t first_end = 0, second_end = 0;
        const auto first = std::stoll(text.substr(0, comma), &first_end);
        const auto tail = text.substr(comma + 1);
        const auto second = std::stoll(tail, &second_end);
        if (text.substr(first_end, comma - first_end).find_first_not_of(" \t") !=
                std::string::npos ||
            tail.substr(second_end).find_first_not_of(" \t") != std::string::npos ||
            first < -10000 || first > 10000 || second < -10000 || second > 10000)
            return std::nullopt;
        return std::pair{static_cast<std::int32_t>(first),
                         static_cast<std::int32_t>(second)};
    } catch (const std::exception &) { return std::nullopt; }
}

const content::XmlNode *direct_child(const content::XmlNode &node,
                                     std::string_view name, std::string_view id = {}) {
    const content::XmlNode *found = nullptr;
    for (const auto &child : node.children)
        if (child.name == name && (id.empty() || child.attribute("ID") == id)) {
            if (found) return nullptr;
            found = &child;
        }
    return found;
}

struct PolicyPanelLayout {
    WorldViewRect bounds, viewport;
    std::int32_t content_height = 0;
};
struct PolicyScreenLayout {
    WorldViewRect root, pane, close;
    std::map<std::string, PolicyPanelLayout, std::less<>> panels;
};

std::optional<PolicyScreenLayout> build_progression_screen_plan(
    const ProgressionScreenSource &source, std::int32_t width,
    std::int32_t height, std::string &reason) {
    // Only the selected BNW SocialPolicyPopup hierarchy is understood here.
    // Its two stacks and each branch ScrollPanel must retain authored sizes,
    // anchors and offsets; a changed XML fails closed instead of placing hits.
    const content::XmlNode *root = nullptr;
    for (const auto &child : source.document.children)
        if (child.name == "Box" && numeric_pair(child, "Size") ==
                std::optional<std::pair<std::int32_t, std::int32_t>>{{1024, 768}} &&
            child.attribute("Anchor") == "C,C") {
            if (root) { reason = "Ambiguous policy root Box"; return std::nullopt; }
            root = &child;
        }
    if (!root || numeric_pair(*root, "Offset") !=
            std::optional<std::pair<std::int32_t, std::int32_t>>{{0, 24}}) {
        reason = "Selected policy root Box size or anchor is unresolved";
        return std::nullopt;
    }
    const auto *grid = direct_child(*root, "Grid");
    const auto *pane = grid ? direct_child(*grid, "Box", "SocialPolicyPane") : nullptr;
    const auto *close = grid ? direct_child(*grid, "GridButton", "CloseButton") : nullptr;
    if (!grid || !pane || !close || grid->attribute("Anchor") != "C,T" ||
        pane->attribute("Anchor") != "L,T" || close->attribute("Anchor") != "L,B" ||
        numeric_pair(*grid, "Size") !=
            std::optional<std::pair<std::int32_t, std::int32_t>>{{1005, 783}} ||
        numeric_pair(*grid, "Offset") !=
            std::optional<std::pair<std::int32_t, std::int32_t>>{{0, -5}}) {
        reason = "Selected policy Grid, SocialPolicyPane or CloseButton is unresolved";
        return std::nullopt;
    }
    const auto pane_size = numeric_pair(*pane, "Size");
    const auto pane_offset = numeric_pair(*pane, "Offset");
    const auto close_size = numeric_pair(*close, "Size");
    const auto close_offset = numeric_pair(*close, "Offset");
    if (pane_size != std::optional<std::pair<std::int32_t, std::int32_t>>{{970, 668}} ||
        !pane_offset || !close_size || !close_offset) {
        reason = "Selected policy pane or close geometry is unresolved";
        return std::nullopt;
    }
    const WorldViewRect root_rect{(width - 1024) / 2, (height - 768) / 2 + 24,
                                  1024, 768};
    const WorldViewRect grid_rect{root_rect.x + (1024 - 1005) / 2,
                                  root_rect.y - 5, 1005, 783};
    PolicyScreenLayout layout;
    layout.root = root_rect;
    layout.pane = {grid_rect.x + pane_offset->first,
                   grid_rect.y + pane_offset->second, 970, 668};
    layout.close = {grid_rect.x + close_offset->first,
        grid_rect.y + grid_rect.height - close_offset->second - close_size->second,
        close_size->first, close_size->second};
    if (!valid_bounds(layout.root, width, height) ||
        !valid_bounds(layout.pane, width, height) ||
        !valid_bounds(layout.close, width, height)) {
        reason = "Selected policy XML does not fit the viewport";
        return std::nullopt;
    }
    const auto *vertical = direct_child(*pane, "Stack");
    if (!vertical || vertical->attribute("StackGrowth") != "Bottom" ||
        numeric_pair(*vertical, "Offset") !=
            std::optional<std::pair<std::int32_t, std::int32_t>>{{0, 25}}) {
        reason = "Selected SocialPolicyPane vertical Stack is unresolved";
        return std::nullopt;
    }
    auto row_y = layout.pane.y + 25;
    std::size_t rows = 0;
    for (const auto &child : vertical->children) {
        if (child.name == "Image") {
            if (numeric_pair(child, "Size") !=
                std::optional<std::pair<std::int32_t, std::int32_t>>{{970, 5}}) {
                reason = "Selected policy row trim size is unresolved";
                return std::nullopt;
            }
            row_y += 5;
            continue;
        }
        if (child.name == "Box" && child.attribute("Anchor") == "L,B" &&
            numeric_pair(child, "Size") ==
                std::optional<std::pair<std::int32_t, std::int32_t>>{{970, 46}}) {
            row_y += 46; // Authored culture information footer; no action hit.
            continue;
        }
        if (child.name != "Stack" || child.attribute("StackGrowth") != "Right") {
            reason = "Selected policy row Stack layout is unresolved";
            return std::nullopt;
        }
        ++rows;
        auto column_x = layout.pane.x;
        std::int32_t row_height = 0;
        for (const auto &column : child.children) {
            const auto size = numeric_pair(column, "Size");
            if (!size || size->first <= 0 || size->second <= 0 ||
                (column.name != "Box" && column.name != "Image")) {
                reason = "Selected policy branch column size is unresolved";
                return std::nullopt;
            }
            row_height = std::max(row_height, size->second);
            if (column.name == "Box") {
                const content::XmlNode *scroll = nullptr;
                auto visit = [&](auto &&self, const content::XmlNode &node) -> void {
                    if (node.name == "ScrollPanel" && !node.attribute("ID").empty()) {
                        if (scroll) { scroll = nullptr; throw std::runtime_error(
                            "Ambiguous policy branch ScrollPanel"); }
                        scroll = &node;
                    }
                    for (const auto &nested : node.children) self(self, nested);
                };
                try { visit(visit, column); }
                catch (const std::exception &) {
                    reason = "Ambiguous policy branch ScrollPanel";
                    return std::nullopt;
                }
                if (scroll) {
                    const auto scroll_size = numeric_pair(*scroll, "Size");
                    const auto scroll_offset = numeric_pair(*scroll, "Offset");
                    if (*size != std::pair<std::int32_t, std::int32_t>{190, 290} ||
                        scroll_size !=
                            std::optional<std::pair<std::int32_t, std::int32_t>>{{190, 500}} ||
                        scroll_offset !=
                            std::optional<std::pair<std::int32_t, std::int32_t>>{{0, 86}} ||
                        scroll->attribute("Anchor") != "C,T") {
                        reason = "Selected branch ScrollPanel geometry is unresolved";
                        return std::nullopt;
                    }
                    const WorldViewRect box{column_x, row_y, 190, 290};
                    const WorldViewRect scroll_rect{column_x, row_y + 86, 190, 500};
                    const auto visible = clipped(scroll_rect, box);
                    const auto clipped_pane = visible ? clipped(*visible, layout.pane) : std::nullopt;
                    if (!clipped_pane ||
                        !layout.panels.emplace(scroll->attribute("ID"),
                            PolicyPanelLayout{box, *clipped_pane, 500}).second) {
                        reason = "Selected policy branch viewport is missing or duplicated";
                        return std::nullopt;
                    }
                }
            }
            column_x += size->first;
        }
        if (row_height != 290 || column_x != layout.pane.x + layout.pane.width) {
            reason = "Selected policy branch row dimensions are unresolved";
            return std::nullopt;
        }
        row_y += row_height;
    }
    if (rows != 2 || layout.panels.empty()) {
        reason = "Selected policy branch rows or ScrollPanels are unavailable";
        return std::nullopt;
    }
    return layout;
}

void append_policy_screen(ComposedUiFrame &result, const ProgressionRenderPlan &plan,
    const ProgressionScreenSource &source, const LiveUiFrame &frame,
    UiGameplayBackend *backend, std::int32_t width, std::int32_t height) {
    if (frame.policy_scroll && (!frame.snapshot.active_player_id() ||
        !frame.snapshot.turn() ||
        frame.policy_scroll->player_id != *frame.snapshot.active_player_id() ||
        frame.policy_scroll->turn != *frame.snapshot.turn() ||
        frame.policy_scroll->viewport_width != width ||
        frame.policy_scroll->viewport_height != height ||
        frame.policy_scroll->selected_source_path != source.selected_source_path)) {
        unavailable(result, "Selected policy frame or scroll source is unavailable",
                    width, height);
        return;
    }
    std::string blocker;
    const auto layout = build_progression_screen_plan(source, width, height, blocker);
    if (!layout) {
        unavailable(result, std::move(blocker), width, height);
        return;
    }
    result.paints.push_back({WorldViewPaintKind::rectangle, layout->root, {}, {}, {},
                             "Grid9DetailFive140"});
    result.paints.push_back({WorldViewPaintKind::text,
        {layout->root.x + 20, layout->root.y + 12, layout->root.width - 40, 32},
        {}, {}, "Social Policies", "MenuTitleCaption"});
    for (const auto &[id, panel] : layout->panels) {
        result.paints.push_back({WorldViewPaintKind::rectangle, panel.bounds, {}, {}, {},
                                 "Grid9FrameButtons"});
        result.paints.push_back({WorldViewPaintKind::text,
            {panel.bounds.x + 4, panel.bounds.y + 4, panel.bounds.width - 8, 28},
            {}, {}, id, "TwCenMT18"});
        std::int32_t offset = 0;
        if (frame.policy_scroll) {
            const auto requested = frame.policy_scroll->offsets.find(id);
            if (requested != frame.policy_scroll->offsets.end())
                offset = requested->second;
        }
        const auto max_offset = std::max(0, panel.content_height - panel.viewport.height);
        if (offset < 0 || offset > max_offset) {
            result.unresolved.push_back("Selected policy scroll offset is outside " + id);
            continue;
        }
        if (frame.policy_scroll)
            result.policy_scroll_regions.push_back({panel.viewport, id,
                source.selected_source_path, offset, max_offset});
    }
    for (const auto &item : plan.items) {
        if (item.kind != ProgressionItemKind::policy || !item.rect) {
            result.unresolved.push_back("PolicyIcon placement is unavailable for policy " +
                                        std::to_string(item.id));
            continue;
        }
        const auto panel = layout->panels.find(item.parent_control_id);
        if (panel == layout->panels.end()) {
            result.unresolved.push_back("Selected policy parent control is unavailable: " +
                                        item.parent_control_id);
            continue;
        }
        std::int32_t scroll_offset = 0;
        if (frame.policy_scroll) {
            const auto region = std::find_if(result.policy_scroll_regions.begin(),
                result.policy_scroll_regions.end(), [&](const auto &value) {
                    return value.panel_id == item.parent_control_id;
                });
            if (region == result.policy_scroll_regions.end()) continue;
            scroll_offset = region->offset;
        }
        if (item.rect->x < 0 || item.rect->y < 0 || item.rect->width <= 0 ||
            item.rect->height <= 0 ||
            item.rect->x + item.rect->width > panel->second.viewport.width ||
            item.rect->y + item.rect->height > panel->second.content_height) {
            result.unresolved.push_back("PolicyIcon exceeds authored ScrollPanel: " +
                                        std::to_string(item.id));
            continue;
        }
        const WorldViewRect icon{panel->second.viewport.x +
                static_cast<std::int32_t>(item.rect->x),
            panel->second.viewport.y + static_cast<std::int32_t>(item.rect->y) -
                scroll_offset,
            static_cast<std::int32_t>(item.rect->width),
            static_cast<std::int32_t>(item.rect->height)};
        const auto visible = clipped(icon, panel->second.viewport);
        if (!visible) continue;
        result.paints.push_back({WorldViewPaintKind::rectangle, *visible, {}, {}, {},
                                 "Grid9FrameButtons"});
        const auto command = progression_choose_policy(item.id);
        const auto *gameplay = std::get_if<UiGameplayCommand>(&command);
        const auto availability = frame.policy_scroll && backend && gameplay && !item.owned
            ? backend->availability(*gameplay, frame.snapshot)
            : UiCommandResult{UiCommandStatus::unavailable,
                item.owned ? "Already owned" : frame.policy_scroll
                    ? "Progression backend is unavailable"
                    : "Selected policy action owner is unavailable"};
        result.hits.push_back({*visible, "SocialPolicyPopup", command,
            availability.status == UiCommandStatus::ready, availability.reason});
    }
    result.paints.push_back({WorldViewPaintKind::rectangle, layout->close, {}, {}, {},
                             "SmallButton"});
    result.paints.push_back({WorldViewPaintKind::text, layout->close, {}, {},
                             "Close", "TwCenMT18"});
    result.hits.push_back({layout->close, "SocialPolicyPopup",
        UiCloseScreen{"SocialPolicyPopup"}, true, {}});
    result.unresolved.insert(result.unresolved.end(), plan.unresolved.begin(),
                             plan.unresolved.end());
    result.unresolved.push_back(
        "SocialPolicyPane portrait atlas, branch masks and confirmation controls are unresolved");
    result.unresolved.push_back(
        "SocialPolicyPane InfoStack culture labels and native scrollbar art are unresolved");
}

std::optional<std::pair<std::int32_t, std::int32_t>> city_list_pair(std::string_view text) {
    const auto split = text.find_first_of(",.");
    if (split == text.npos) return std::nullopt;
    std::int32_t x = 0, y = 0;
    const auto read = [](std::string_view part, std::int32_t &out) {
        const auto parsed = std::from_chars(part.data(), part.data() + part.size(), out);
        return parsed.ec == std::errc{} && parsed.ptr == part.data() + part.size();
    };
    return read(text.substr(0, split), x) && read(text.substr(split + 1), y)
        ? std::optional(std::pair{x, y}) : std::nullopt;
}

void append_city_list(ComposedUiFrame &result, const content::XmlNode &source,
    const UiFrameSnapshot &frame, const OverviewLocalizer &localize,
    std::int32_t width, std::int32_t height) {
    const auto *cities = frame.find<CityScreenFrame>();
    if (!cities || !frame.active_player_id() ||
        cities->player_id != *frame.active_player_id()) {
        unavailable(result, "Owned live city list is unavailable", width, height);
        return;
    }
    const auto grid = std::find_if(source.children.begin(), source.children.end(),
        [](const content::XmlNode &node) { return node.name == "Grid"; });
    const auto *scroll = progression_control(source, "ScrollPanel");
    const auto *row = progression_control(source, "Root");
    const auto *row_button = progression_control(source, "Button");
    const auto *population = progression_control(source, "Population");
    const auto *name = progression_control(source, "CityName");
    const auto *close = progression_control(source, "CloseButton");
    if (grid == source.children.end() || !scroll || !row || !row_button || !population ||
        !name || !close) {
        unavailable(result, "Selected CityList XML controls are incomplete", width, height);
        return;
    }
    const auto panel_size = city_list_pair(grid->attribute("Size"));
    const auto panel_offset = city_list_pair(grid->attribute("Offset"));
    const auto scroll_size = city_list_pair(scroll->attribute("Size"));
    const auto scroll_offset = city_list_pair(scroll->attribute("Offset"));
    const auto row_size = city_list_pair(row->attribute("Size"));
    const auto button_size = city_list_pair(row_button->attribute("Size"));
    if (!panel_size || !panel_offset || !scroll_size || !scroll_offset ||
        !row_size || !button_size || panel_size->first <= 0 || panel_size->second <= 0 ||
        scroll_size->second <= 0 || row_size->second <= 0 || button_size->first <= 0) {
        unavailable(result, "Selected CityList XML layout is unresolved", width, height);
        return;
    }
    const WorldViewRect panel{std::max(0, panel_offset->first), panel_offset->second,
        panel_size->first, panel_size->second};
    const WorldViewRect rows{panel_offset->first + scroll_offset->first,
        panel_offset->second + scroll_offset->second,
        scroll_size->first, scroll_size->second};
    result.paints.push_back({WorldViewPaintKind::rectangle, panel, {}, {}, {},
                             grid->attribute("Style")});
    result.hits.push_back({panel, "CityList", UiCloseScreen{"CityList"}, false,
        "CityList panel has no action at this position"});
    const auto max_rows = static_cast<std::size_t>(rows.height / row_size->second);
    std::size_t visible = 0;
    for (const auto &city : cities->cities) {
        if (city.owner != cities->player_id || city.id < 0 ||
            !city.name || city.name->empty() || city.population <= 0) continue;
        if (visible == max_rows) {
            result.unresolved.push_back("Additional live cities require CityList scrolling");
            break;
        }
        const WorldViewRect item{rows.x, rows.y +
            static_cast<std::int32_t>(visible) * row_size->second,
            std::min(button_size->first, rows.width), row_size->second};
        result.paints.push_back({WorldViewPaintKind::rectangle, item, {}, {}, {}, "BoxButton"});
        result.paints.push_back({WorldViewPaintKind::text,
            {item.x + 70, item.y, 45, item.height}, {}, {},
            std::to_string(city.population), "TwCenMT22"});
        result.paints.push_back({WorldViewPaintKind::text,
            {item.x + 120, item.y, std::max(0, item.width - 120), item.height}, {}, {},
            *city.name, "TwCenMT18"});
        result.hits.push_back({item, "CityList", UiGameplayCommand::make("CityList",
            WorldViewAction{WorldViewActionKind::open_city, city.id}), true, {}});
        ++visible;
    }
    if (!visible)
        result.unresolved.push_back("No owned live city has a resolved name and population");
    const auto close_text = localize ? localize(close->attribute("String")) : std::nullopt;
    const WorldViewRect close_bounds{panel.x + 120,
        panel.y + panel.height - 44, std::min(150, panel.width - 120), 28};
    if (close_bounds.width > 0 && close_bounds.y >= 0 &&
        close_bounds.x + close_bounds.width <= width &&
        close_bounds.y + close_bounds.height <= height) {
        if (close_text && !close_text->empty())
            result.paints.push_back({WorldViewPaintKind::text, close_bounds,
                {}, {}, *close_text, "TwCenMT16"});
        result.hits.push_back({close_bounds, "CityList",
            UiCloseScreen{"CityList"}, true, {}});
    }
    result.unresolved.push_back("CityList health, strength, production and native scrolling are unavailable");
}

void append_progression(ComposedUiFrame &result, const ProgressionRenderPlan &plan,
                        std::string_view context, const UiFrameSnapshot &frame,
                        UiGameplayBackend *backend, std::int32_t width,
                        std::int32_t height) {
    const WorldViewRect panel{std::max(0, (width - 1080) / 2),
                              std::max(0, (height - 760) / 2),
                              std::min(width, 1080), std::min(height, 760)};
    result.paints.push_back({WorldViewPaintKind::rectangle, panel, {}, {}, {},
                             "Grid9DetailFive140"});
    result.paints.push_back({WorldViewPaintKind::text,
                             {panel.x + 24, panel.y + 16, panel.width - 48, 36}, {}, {},
                             std::string(context), "MenuTitleCaption"});
    std::int32_t x = panel.x + 28, y = panel.y + 70;
    const auto row_width = std::min(430, std::max(0, panel.width - 56));
    for (const auto &item : plan.items) {
        if (!row_width || y + 42 > panel.y + panel.height - 56) {
            result.unresolved.push_back("Additional progression items require authored scrolling");
            break;
        }
        const WorldViewRect row{x, y, row_width, 38};
        result.paints.push_back({WorldViewPaintKind::rectangle, row, {}, {}, {}, "BaseButton"});
        result.paints.push_back({WorldViewPaintKind::text,
                                 {row.x + 8, row.y + 5, row.width - 16, 28}, {}, {},
                                 item.text.empty() ? "N/A" : item.text, "TwCenMT20"});
        UiCommand command = item.kind == ProgressionItemKind::technology
            ? plan.screen == ProgressionScreen::technology_popup
                ? progression_choose_popup_technology(item.id)
                : progression_choose_technology(item.id)
            : item.kind == ProgressionItemKind::policy
                ? progression_choose_policy(item.id) : progression_choose_ideology(item.id);
        const auto *gameplay = std::get_if<UiGameplayCommand>(&command);
        const auto availability = backend && gameplay && !item.owned
            ? backend->availability(*gameplay, frame)
            : UiCommandResult{UiCommandStatus::unavailable,
                              item.owned ? "Already owned" : "Progression backend is unavailable"};
        result.hits.push_back({row, std::string(context), std::move(command),
                               availability.status == UiCommandStatus::ready,
                               availability.reason});
        y += 44;
        if (y + 42 > panel.y + panel.height - 56 &&
            x + 460 + row_width <= panel.x + panel.width - 28) {
            x += 460;
            y = panel.y + 70;
        }
    }
    result.unresolved.insert(result.unresolved.end(), plan.unresolved.begin(),
                             plan.unresolved.end());
    result.unresolved.push_back("Progression list uses a temporary non-native layout");
}

void append_technology_tree_close(ComposedUiFrame &result,
    const ProgressionScreenSource &source, const ProgressionRenderPlan &plan,
    const LiveUiFrame &live, const OverviewLocalizer &localize,
    std::int32_t width, std::int32_t height) {
    // Selected TechTree.xml places CloseButton inside the centered 705px
    // TechTreeScrollPanel. TechTree.lua binds it to DequeuePopup.
    const auto *scroll = direct_child(source.document, "ScrollPanel", "TechTreeScrollPanel");
    const auto *close = scroll ? direct_child(*scroll, "GridButton", "CloseButton") : nullptr;
    if (source.selected_source_path.empty() ||
        plan.selected_source_path != source.selected_source_path ||
        !scroll || !close || scroll->attribute("Anchor") != "L,C" ||
        scroll->attribute("Size") != "Full,705" ||
        close->attribute("Anchor") != "L,B" ||
        close->attribute("AnchorSide") != "i,o" ||
        close->attribute("ConsumeMouse") != "1" ||
        close->attribute("Style") != "BaseButton" ||
        close->attribute("String") != "TXT_KEY_CLOSE" ||
        numeric_pair(*close, "Size") !=
            std::optional<std::pair<std::int32_t, std::int32_t>>{{100, 32}} ||
        numeric_pair(*close, "Offset") !=
            std::optional<std::pair<std::int32_t, std::int32_t>>{{7, -13}} ||
        height < 705) {
        result.unresolved.push_back("Selected TechTree CloseButton layout is unresolved");
        return;
    }
    const WorldViewRect button{7, (height - 705) / 2 + 705 - 13 - 32, 100, 32};
    if (!valid_bounds(button, width, height)) {
        result.unresolved.push_back("TechTree CloseButton is outside the viewport");
        return;
    }
    result.paints.push_back({WorldViewPaintKind::rectangle, button,
                             {}, {}, {}, "BaseButton"});
    const auto label = localize ? localize(close->attribute("String")) : std::nullopt;
    if (label && !label->empty())
        result.paints.push_back({WorldViewPaintKind::text,
            {button.x + 4, button.y, button.width - 8, button.height},
            {}, {}, *label, "TwCenMT16"});
    else result.unresolved.push_back("Selected TechTree close label is unavailable");
    const auto *event = live.snapshot.find<ProgressionTechnologyTreeEvent>();
    result.hits.push_back({button, "TechTree",
        progression_tree_close_command(event ? event->token : 0),
        live.technology_tree_close_ready,
        live.technology_tree_close_ready ? std::string{}
            : "BNW technology tree popup owner is unavailable"});
}

void append_technology_popup(ComposedUiFrame &result,
    const ProgressionScreenSource &source, const ProgressionRenderPlan &plan,
    const LiveUiFrame &live, UiGameplayBackend *backend,
    const OverviewLocalizer &localize,
    std::int32_t width, std::int32_t height) {
    const auto &frame = live.snapshot;
    const auto *popup = frame.find<ProgressionTechPopupState>();
    if (!popup || !frame.active_player_id() || !popup->token ||
        popup->player_id != *frame.active_player_id() ||
        source.selected_source_path.empty() ||
        plan.selected_source_path != source.selected_source_path) {
        unavailable(result, "Active BNW technology choice owner is unavailable", width, height);
        return;
    }
    const auto *background = direct_child(source.document, "Box", "TechBackground");
    const auto *open_tree = background
        ? direct_child(*background, "GridButton", "OpenTTButton") : nullptr;
    const auto *tree_label = open_tree ? direct_child(*open_tree, "Label") : nullptr;
    const auto *scroll = background ? direct_child(*background, "ScrollPanel", "ScrollPanel") : nullptr;
    const auto *stack = scroll ? direct_child(*scroll, "Stack", "ButtonStack") : nullptr;
    const auto *instance = direct_child(source.document, "Instance");
    const auto *container = instance ? direct_child(*instance, "Container", "TechButtonContainer") : nullptr;
    const auto *button = container ? direct_child(*container, "Button", "TechButton") : nullptr;
    if (!background || !open_tree || !tree_label || !scroll || !stack ||
        !container || !button ||
        background->attribute("Anchor") != "L,T" ||
        background->attribute("Size") != "340,Full" ||
        background->attribute("ConsumeMouse") != "1" ||
        open_tree->attribute("Anchor") != "L,T" ||
        numeric_pair(*open_tree, "Offset") !=
            std::optional<std::pair<std::int32_t, std::int32_t>>{{8, 260}} ||
        numeric_pair(*open_tree, "Size") !=
            std::optional<std::pair<std::int32_t, std::int32_t>>{{310, 36}} ||
        tree_label->attribute("Anchor") != "C,C" ||
        tree_label->attribute("String") != "TXT_KEY_OPEN_TECH_TREE" ||
        scroll->attribute("Anchor") != "L,T" ||
        scroll->attribute("Vertical") != "1" ||
        numeric_pair(*scroll, "Offset") !=
            std::optional<std::pair<std::int32_t, std::int32_t>>{{10, 298}} ||
        numeric_pair(*scroll, "Size") !=
            std::optional<std::pair<std::int32_t, std::int32_t>>{{290, 0}} ||
        stack->attribute("StackGrowth") != "Bottom" ||
        stack->attribute("Anchor") != "L,T" ||
        stack->attribute("Padding") != "6" ||
        numeric_pair(*stack, "Offset") !=
            std::optional<std::pair<std::int32_t, std::int32_t>>{{28, 12}} ||
        container->attribute("Anchor") != "L,T" ||
        numeric_pair(*container, "Size") !=
            std::optional<std::pair<std::int32_t, std::int32_t>>{{270, 70}} ||
        button->attribute("Anchor") != "L,T" ||
        numeric_pair(*button, "Offset") !=
            std::optional<std::pair<std::int32_t, std::int32_t>>{{0, 0}} ||
        numeric_pair(*button, "Size") !=
            std::optional<std::pair<std::int32_t, std::int32_t>>{{270, 60}} ||
        width < 340 || height <= 300) {
        unavailable(result, "Selected TechPopup XML layout is unresolved", width, height);
        return;
    }
    result.paints.push_back({WorldViewPaintKind::rectangle,
        {0, 0, 340, height}, {}, {}, {}, "Grid9DetailFive140"});
    result.hits.push_back({{0, 0, 340, height}, "TechPopup",
        UiCloseScreen{"TechPopup"}, false,
        "TechPopup background consumes input without an action"});
    const auto title = localize ? localize("TXT_KEY_TECH_POPUP_HEADER") : std::nullopt;
    if (title && !title->empty())
        result.paints.push_back({WorldViewPaintKind::text,
            {220, 160, 250, 80}, {}, {}, *title, "TwCenMT18"});
    else result.unresolved.push_back("Selected BNW technology chooser title is unavailable");
    const WorldViewRect tree_button{8, 260, 310, 36};
    result.paints.push_back({WorldViewPaintKind::rectangle, tree_button,
        {}, {}, {}, "Grid9DarkBlue8Button"});
    const auto label_key = tree_label->attribute("String");
    const auto label = localize ? localize(label_key) : std::nullopt;
    if (label && !label->empty())
        result.paints.push_back({WorldViewPaintKind::text,
            {tree_button.x + 8, tree_button.y + 3,
             tree_button.width - 16, tree_button.height - 6},
            {}, {}, *label, "TwCenMT22"});
    else result.unresolved.push_back("Selected BNW OpenTTButton text is unavailable");
    result.hits.push_back({tree_button, "TechPopup",
        progression_popup_open_tree_command(popup->token),
        live.technology_tree_transition_ready,
        live.technology_tree_transition_ready ? std::string{}
            : "BNW CHOOSETECH to TECH_TREE owner transition is unavailable"});
    const auto *view = live.tech_popup_scroll ? &*live.tech_popup_scroll : nullptr;
    const bool matched = view && frame.turn() &&
        view->player_id == popup->player_id && view->turn == *frame.turn() &&
        view->frame == frame.frame() &&
        view->event_token == popup->token &&
        view->selected_source_path == source.selected_source_path &&
        view->viewport_width == width && view->viewport_height == height &&
        view->choice_count == plan.items.size() &&
        view->choice_count <= 4096 &&
        (view->scroll_panel_height == height - 300 ||
         view->scroll_panel_height == height - 244) &&
        view->scroll_panel_height > 0 &&
        view->vertical_offset_pixels >= 0 &&
        view->vertical_offset_pixels <= 1000000;
    if (!matched) {
        result.unresolved.push_back("Native TechPopup ScrollPanel offset is unavailable");
        result.unresolved.insert(result.unresolved.end(), plan.unresolved.begin(),
                                 plan.unresolved.end());
        return;
    }
    const auto viewport = clipped({10, 298, 290, view->scroll_panel_height},
                                  {0, 0, width, height});
    if (!viewport) {
        result.unresolved.push_back("Native TechPopup ScrollPanel viewport is unavailable");
        return;
    }
    result.tech_popup_scroll_region = TechPopupScrollRegion{
        *viewport, source.selected_source_path, popup->player_id,
        *frame.turn(), width, height, frame.frame(), popup->token,
        view->scroll_panel_height, view->vertical_offset_pixels,
        plan.items.size()};
    for (std::size_t index = 0; index < plan.items.size(); ++index) {
        const auto &item = plan.items[index];
        if (item.kind != ProgressionItemKind::technology ||
            item.parent_control_id != "ButtonStack" || !item.rect ||
            item.rect->x != 0 || item.rect->width != 270 ||
            item.rect->height != 60 || item.rect->y < 0 ||
            item.rect->y != static_cast<std::int64_t>(index) * 76 ||
            item.rect->y > 1000000) {
            result.unresolved.push_back("TechPopup choice geometry is unresolved");
            continue;
        }
        const auto row_y = 310 + item.rect->y - view->vertical_offset_pixels;
        if (row_y + 60 <= viewport->y) continue;
        if (row_y >= static_cast<std::int64_t>(viewport->y) + viewport->height) break;
        const WorldViewRect row{38, static_cast<std::int32_t>(row_y), 270, 60};
        const auto visible = clipped(row, *viewport);
        if (!visible) continue;
        auto command = progression_choose_popup_technology(item.id);
        const auto *gameplay = std::get_if<UiGameplayCommand>(&command);
        const auto availability = backend && gameplay
            ? backend->availability(*gameplay, frame)
            : UiCommandResult{UiCommandStatus::unavailable,
                              "BNW research choice backend is unavailable"};
        result.paints.push_back({WorldViewPaintKind::rectangle, row,
            {}, {}, {}, item.selected ? "Grid9DarkBlue8Button"
                : availability.status == UiCommandStatus::ready
                    ? "Grid9Green8" : "GridBlack8", 0xffffffff, *viewport});
        const WorldViewRect text{row.x + 64, row.y + 4, 200, 32};
        if (clipped(text, *viewport) && !item.text.empty())
            result.paints.push_back({WorldViewPaintKind::text, text,
                {}, {}, item.text, "TwCenMT16", 0xffffffff, *viewport});
        result.hits.push_back({*visible, "TechPopup", std::move(command),
            availability.status == UiCommandStatus::ready, availability.reason});
    }
    result.unresolved.insert(result.unresolved.end(), plan.unresolved.begin(),
                             plan.unresolved.end());
    result.unresolved.push_back(
        "TechPopup portrait, unlock icons, turns, meter and native scrollbar art are unresolved");
}

} // namespace

UiScreenCompositor::UiScreenCompositor(const assets::ResourceMountTable &resources,
                                       const UiContextRegistry &registry,
                                       UiGameplayBackend *backend,
                                       OverviewLocalizer localize,
                                       OverviewLanguage overview_language)
    : progression_{read_progression_screen_source(resources, ProgressionScreen::technologies),
                   read_progression_screen_source(resources, ProgressionScreen::policies),
                   read_progression_screen_source(resources, ProgressionScreen::ideology),
                   read_progression_screen_source(resources,
                                                  ProgressionScreen::technology_popup)},
      city_{read_city_screen_source(resources, "CityView"),
            read_city_screen_source(resources, "ProductionPopup")},
      overview_(read_overview_sources(resources, registry)),
      popups_(PopupCatalog::from_resources(resources, registry)),
      system_(read_system_sources(resources, registry)), backend_(backend),
      gui_localizer_(std::make_shared<const OverviewLocalizer>(std::move(localize))),
      localize_(*gui_localizer_?OverviewLocalizer{[held=gui_localizer_](std::string_view key) {
          return (*held)(key);
      }}:OverviewLocalizer{}), overview_language_(overview_language) {
    if (const auto *context = registry.find("CityList")) {
        const auto selected = resources.resolve(assets::AssetKey(context->source_key + ".xml"));
        const auto *asset = std::get_if<assets::AssetHandle>(&selected.asset);
        if (!asset) throw std::runtime_error("Active CityList XML is missing or ambiguous");
        const auto bytes = asset->read_bytes(64ULL * 1024 * 1024);
        city_list_xml_ = content::read_xml_metadata(std::string(bytes.begin(), bytes.end()));
        if (city_list_xml_->name != "Context")
            throw std::runtime_error("Active CityList XML root is not Context");
    }
}

UiScreenCompositor::UiScreenCompositor(GuiRetainedStorageTag,const UiScreenCompositor& actual)
    : progression_(actual.progression_),city_(actual.city_),city_list_xml_(actual.city_list_xml_),
      overview_(actual.overview_),popups_(actual.popups_),system_(actual.system_),
      backend_(actual.backend_),gui_localizer_(actual.gui_localizer_),
      localize_(actual.localize_),overview_language_(actual.overview_language_) {
    // localize_ is now only the constructor-owned forwarding closure: its
    // copy contains a shared storage pin, no application target-copy callback.
}
ComposedUiFrame UiScreenCompositor::compose(const LiveUiFrame &frame,
                                             const UiScreenController &screens,
                                             std::int32_t width,
                                             std::int32_t height,
                                             const MeasuredProductionPopup *production,
                                             const EconomicBoundFields *economic_fields) const {
    if (width <= 0 || height <= 0)
        throw std::invalid_argument("UI compositor viewport must be positive");
    ComposedUiFrame result;
    result.frame_owner = frame.owner;
    result.frame_sequence = frame.sequence;
    result.snapshot_frame = frame.snapshot.frame();
    const auto world_paint_offset=result.paints.size();
    if (frame.world_view) {
        // Validate producer indices before mapping them into this composed stream.
        // Source/VM/frame authority is checked by the owned renderer separately.
        std::size_t previous_end = 0;
        for (std::size_t i = 0; i < frame.world_view->owned_paint_spans.size(); ++i) {
            const auto &span = frame.world_view->owned_paint_spans[i];
            if (span.context_id.empty() || span.begin > span.end ||
                span.end > frame.world_view->paints.size() ||
                span.begin < previous_end)
                throw std::runtime_error("WorldView producer paint span is invalid");
            for (std::size_t j = 0; j < i; ++j)
                if (frame.world_view->owned_paint_spans[j].context_id == span.context_id)
                    throw std::runtime_error("WorldView producer context span is repeated");
            previous_end = span.end;
        }
        append_paints(result, frame.world_view->paints, {});
        for (const auto &span : frame.world_view->owned_paint_spans)
            result.worldview_owned_paint_spans.push_back(
                {span.context_id, world_paint_offset + span.begin,
                 world_paint_offset + span.end});
    }
    const auto focused = screens.focused_screen();
    if (!focused) {
        if (frame.world_view && ui_owners_current(frame.snapshot))
            for (const auto &request : frame.world_view->readonly_tooltips) {
                if (!request.source_paint_index || *request.source_paint_index>=frame.world_view->paints.size() ||
                    frame.world_view->paints[*request.source_paint_index].kind!=WorldViewPaintKind::text) continue;
                auto mapped=request;
                mapped.source_paint_index=world_paint_offset+*request.source_paint_index;
                result.readonly_tooltips.push_back(std::move(mapped));
            }
        return result;
    }
    result.focused_context = focused;
    const auto screen_paint_start = result.paints.size();
    const auto *descriptor = screens.registry().find(*focused);
    if (!descriptor || !screens.visible(*focused)) {
        unavailable(result, "Focused BNW context is unknown or hidden", width, height);
        return result;
    }
    if (*focused == "CityList") {
        if (city_list_xml_)
            append_city_list(result, *city_list_xml_, frame.snapshot, localize_, width, height);
        else unavailable(result, "Active CityList XML is unavailable", width, height);
        return result;
    }
    if (const auto progression = progression_id(*focused)) {
        const auto index = static_cast<std::size_t>(*progression);
        if (*progression == ProgressionScreen::ideology) {
            const auto layout = build_ideology_layout(progression_[index],
                frame.snapshot, width, height,
                frame.ideology_view ? &*frame.ideology_view : nullptr,
                localize_);
            append_paints(result, layout.paints, layout.unresolved);
            result.hits.insert(result.hits.end(), layout.hits.begin(), layout.hits.end());
            result.ideology_scroll_region = layout.scroll_region;
            result.ideology_tooltip_keys = layout.tooltip_keys;
            result.ideology_tenet_headers = layout.tenet_headers;
            return result;
        }
        const auto planned = frame.progression && frame.progression->screen == *progression &&
                             frame.progression->selected_source_path ==
                                 progression_[index].selected_source_path
            ? *frame.progression
            : plan_progression_screen(progression_[index], *progression, frame.snapshot);
        if (*progression == ProgressionScreen::policies)
            append_policy_screen(result, planned, progression_[index], frame,
                                 backend_, width, height);
        else if (*progression == ProgressionScreen::technology_popup)
            append_technology_popup(result, progression_[index], planned,
                                    frame, backend_, localize_, width, height);
        else {
            append_progression(result, planned, *focused,
                               frame.snapshot, backend_, width, height);
            append_technology_tree_close(result, progression_[index], planned,
                                         frame, localize_, width, height);
        }
        return result;
    }
    if (descriptor->owner == UiContextOwner::city) {
        const auto source = std::find_if(city_.begin(), city_.end(), [&](const auto &entry) {
            return entry.context_id == *focused;
        });
        if (source != city_.end()) {
            const auto planned = plan_city_screen(*source, frame.snapshot, width, height,
                                                  backend_, localize_);
            append_paints(result, planned.paints, planned.unresolved);
            if (*focused == "ProductionPopup")
                append_production_hits(result, *source, planned, frame.snapshot,
                    backend_, production, width, height);
            else append_hits(result, *focused, planned.hits);
        } else unavailable(result, "City screen source is unavailable", width, height);
    } else if (descriptor->owner == UiContextOwner::overview) {
        const auto source = std::find_if(overview_.begin(), overview_.end(), [&](const auto &entry) {
            return entry.context_id == *focused;
        });
        if (source != overview_.end()) {
            const auto planned = plan_overview(*source, frame.snapshot, width, height,
                                               localize_, overview_language_,
                                               *focused == "EconomicOverview" ?
                                                   economic_fields : nullptr);
            for (const auto &paint : planned.paints)
                result.paints.push_back({paint.kind == OverviewPaintKind::text
                                             ? WorldViewPaintKind::text
                                         : paint.kind == OverviewPaintKind::texture
                                             ? WorldViewPaintKind::texture
                                             : WorldViewPaintKind::rectangle,
                                         {paint.bounds.x, paint.bounds.y,
                                          paint.bounds.width, paint.bounds.height},
                                         paint.kind == OverviewPaintKind::texture ? paint.value : std::string{},
                                         {}, paint.kind == OverviewPaintKind::text ? paint.value : std::string{},
                                         paint.style});
            result.unresolved.insert(result.unresolved.end(), planned.unresolved.begin(),
                                     planned.unresolved.end());
            // A live EconomicOverview owner consumes these controls through
            // its measured popup lifecycle, including Processed on close.
            if (*focused != "EconomicOverview")
                for (const auto &hit : planned.hits)
                    result.hits.push_back({{hit.bounds.x, hit.bounds.y,
                                            hit.bounds.width, hit.bounds.height},
                                           *focused, hit.command, true, {}});
        } else unavailable(result, "Overview source is unavailable", width, height);
    } else if (descriptor->owner == UiContextOwner::popup) {
        const auto *source = popups_.find(*focused);
        if (source) {
            const auto planned = plan_popup_screen(*source, frame.snapshot, width, height,
                                                   backend_, localize_);
            append_paints(result, planned.paints, planned.unresolved);
            append_hits(result, *focused, planned.hits);
            result.popup_scroll_panel = planned.scroll_bounds;
            result.popup_edit_box = planned.edit_box;
        } else unavailable(result, "Popup source is unavailable", width, height);
    } else if (descriptor->owner == UiContextOwner::system) {
        const auto source = std::find_if(system_.begin(), system_.end(), [&](const auto &entry) {
            return entry.context_id == *focused;
        });
        if (source != system_.end()) {
            const auto planned = plan_system_screen(*source, frame.snapshot, width, height,
                                                    localize_, backend_);
            append_paints(result, planned.paints, planned.unresolved);
            append_hits(result, *focused, planned.hits);
            result.civilopedia_search_box = planned.search_edit_box;
            result.civilopedia_article_scroll_panel = planned.article_scroll_panel;
        } else unavailable(result, "System screen source is unavailable", width, height);
    } else {
        unavailable(result, "Focused context has no 2D screen plan", width, height);
    }
    if (result.paints.size() == screen_paint_start)
        unavailable(result, "Live data for this screen is unavailable", width, height);
    return result;
}

std::optional<ProductionPopupControlRects> measure_civis_production_popup(
    const LiveUiFrame &frame, const CityScreenSource &source,
    std::int32_t width, std::int32_t height) {
    const auto layout = layout_production_popup(source, width, height);
    const auto *choices = frame.snapshot.find<ProductionPopupChoices>();
    if (!layout || !choices || choices->selected_source_path != source.selected_source_path ||
        choices->visible_rows != production_popup_visible_rows(source, height) ||
        choices->first_visible > choices->entries.size() ||
        choices->visible_rows >
            static_cast<std::size_t>(layout->scroll_panel.height /
                                     layout->choice_button.height))
        return std::nullopt;
    ProductionPopupControlRects measured;
    measured.frame_owner = frame.owner;
    measured.frame_sequence = frame.sequence;
    measured.snapshot_frame = frame.snapshot.frame();
    measured.selected_source_path = source.selected_source_path;
    measured.scroll_panel = layout->scroll_panel;
    measured.up_button = layout->up_button;
    measured.down_button = layout->down_button;
    const auto visible = std::min(choices->visible_rows,
        choices->entries.size() - choices->first_visible);
    for (std::size_t slot = 0; slot < visible; ++slot)
        measured.visible_choice_buttons.push_back(WorldViewRect{
            layout->choice_button.x,
            layout->choice_button.y +
                static_cast<std::int32_t>(slot) * layout->choice_button.height,
            layout->choice_button.width, layout->choice_button.height});
    return measured;
}

PresentedUiFrame present_live_ui_frame(LiveUiHost &host,
    const UiScreenCompositor &compositor, std::int32_t width,
    std::int32_t height, const ProductionPopupControlMeasurer &measure) {
    PresentedUiFrame presented{host.begin_frame(width, height), {}};
    std::optional<MeasuredProductionPopup> production;
    if (host.screens().focused_screen() ==
            std::optional<std::string>{"ProductionPopup"} && measure) {
        const auto &source = compositor.production_popup_source();
        const auto *choices = presented.live.snapshot.find<ProductionPopupChoices>();
        if (choices && !source.selected_source_path.empty() &&
            choices->selected_source_path == source.selected_source_path &&
            choices->visible_rows == production_popup_visible_rows(source, height) &&
            choices->visible_rows &&
            choices->first_visible <= choices->entries.size()) {
            const auto controls = measure(presented.live, source);
            const auto visible = std::min(choices->visible_rows,
                choices->entries.size() - choices->first_visible);
            if (controls && controls->frame_owner == presented.live.owner &&
                controls->frame_sequence == presented.live.sequence &&
                controls->snapshot_frame == presented.live.snapshot.frame() &&
                controls->selected_source_path == source.selected_source_path &&
                controls->visible_choice_buttons.size() == visible) {
                production.emplace();
                production->selected_source_path = controls->selected_source_path;
                production->scroll_panel = controls->scroll_panel;
                production->up_button = controls->up_button;
                production->down_button = controls->down_button;
                for (std::size_t index = 0; index < visible; ++index) {
                    if (!controls->visible_choice_buttons[index]) continue;
                    const auto &choice = choices->entries[choices->first_visible + index];
                    production->rows.push_back({
                        {choices->city_id, choice.order_type, choice.item_id},
                        *controls->visible_choice_buttons[index]});
                }
            }
        }
    }
    presented.composed = compositor.compose(presented.live, host.screens(), width,
        height, production ? &*production : nullptr);
    return presented;
}

LiveUiInputResult click_composed_ui(LiveUiHost &host, const LiveUiFrame &frame,
    const ComposedUiFrame &composed, std::int32_t x, std::int32_t y) {
    if (composed.frame_owner != frame.owner ||
        composed.frame_sequence != frame.sequence ||
        composed.snapshot_frame != frame.snapshot.frame() ||
        composed.focused_context != host.screens().focused_screen())
        return {true, {UiCommandStatus::invalid, "Composed UI frame is stale"}};
    return host.click(frame, x, y,
        std::span<const LiveUiHit>(composed.hits));
}

LiveUiInputResult click_composed_ui(LiveUiHost &host, const LiveUiFrame &frame,
    const ComposedUiFrame &composed, std::int32_t press_x, std::int32_t press_y,
    std::int32_t release_x, std::int32_t release_y) {
    if (composed.frame_owner != frame.owner ||
        composed.frame_sequence != frame.sequence ||
        composed.snapshot_frame != frame.snapshot.frame() ||
        composed.focused_context != host.screens().focused_screen())
        return {true, {UiCommandStatus::invalid, "Composed UI frame is stale"}};
    const auto measured_at = [&](std::int32_t x, std::int32_t y)
        -> const LiveUiHit * {
        for (auto it = composed.hits.rbegin(); it != composed.hits.rend(); ++it) {
            const auto &bounds = it->bounds;
            if (bounds.width > 0 && bounds.height > 0 && x >= bounds.x && y >= bounds.y &&
                std::int64_t(x) < std::int64_t(bounds.x) + bounds.width &&
                std::int64_t(y) < std::int64_t(bounds.y) + bounds.height)
                return &*it;
        }
        return nullptr;
    };
    const auto *pressed = measured_at(press_x, press_y);
    const auto *released = measured_at(release_x, release_y);
    bool same_target = pressed && pressed == released;
    if (!pressed && !released && frame.world_view) {
        const auto *world_press = frame.world_view->hit(press_x, press_y);
        const auto *world_release = frame.world_view->hit(release_x, release_y);
        same_target = world_press && world_press == world_release;
    }
    if (same_target)
        return host.click(frame, release_x, release_y,
            std::span<const LiveUiHit>(composed.hits));
    const auto focused = composed.focused_context
        ? host.screens().registry().find(*composed.focused_context) : nullptr;
    if (focused && focused->modal)
        return {true, {UiCommandStatus::invalid,
                       "A modal UI screen owns input focus"}};
    return {};
}

LiveUiInputResult wheel_composed_ui(LiveUiHost &host, const LiveUiFrame &frame,
    const ComposedUiFrame &composed, std::int32_t x, std::int32_t y,
    std::int32_t wheel_delta) {
    if (composed.frame_owner != frame.owner ||
        composed.frame_sequence != frame.sequence ||
        composed.snapshot_frame != frame.snapshot.frame() ||
        composed.focused_context != host.screens().focused_screen())
        return {true, {UiCommandStatus::invalid, "Composed UI frame is stale"}};
    if (composed.focused_context == "SocialPolicyPopup") {
        for (const auto &region : composed.policy_scroll_regions)
            if (point_inside(region.bounds, x, y))
                return host.wheel_policy(frame, wheel_delta, region);
        return {true, {UiCommandStatus::invalid,
                       "Focused policy screen owns wheel input"}};
    }
    if (composed.focused_context == "TechPopup") {
        if (composed.tech_popup_scroll_region)
            return host.wheel_tech_popup(frame, x, y, wheel_delta,
                                         *composed.tech_popup_scroll_region);
        return {true, {UiCommandStatus::unavailable,
                       "Technology chooser scroll owner is unavailable"}};
    }
    if (composed.focused_context == "ChooseIdeology") {
        if (composed.ideology_scroll_region)
            return host.wheel_ideology(frame, x, y, wheel_delta,
                                       *composed.ideology_scroll_region);
        return {true, {UiCommandStatus::unavailable,
                       "Focused ideology overlay has no confirmed wheel movement"}};
    }
    if (composed.popup_scroll_panel)
        return host.wheel_popup(frame, x, y, wheel_delta, *composed.popup_scroll_panel);
    if (composed.civilopedia_article_scroll_panel)
        return host.wheel_civilopedia_article(frame, x, y, wheel_delta,
            *composed.civilopedia_article_scroll_panel);
    if (composed.focused_context) {
        const auto *focused = host.screens().registry().find(*composed.focused_context);
        if (focused && focused->modal)
            return {true, {UiCommandStatus::unavailable,
                           "Focused modal screen has no scrollable choice region"}};
    }
    if (!composed.production_scroll_panel)
        return composed.focused_context == "ProductionPopup"
            ? LiveUiInputResult{true, {UiCommandStatus::unavailable,
                                       "Production popup scroll geometry is unavailable"}}
            : LiveUiInputResult{};
    return host.wheel_production_popup(frame, x, y, wheel_delta,
                                       *composed.production_scroll_panel);
}

namespace {
bool current_search_frame(const LiveUiHost &host, const LiveUiFrame &frame,
                          const ComposedUiFrame &composed) {
    return host.is_current_frame(frame) && composed.frame_owner == frame.owner &&
           composed.frame_sequence == frame.sequence &&
           composed.snapshot_frame == frame.snapshot.frame() &&
           composed.focused_context == std::optional<std::string>{"CivilopediaScreen"} &&
           host.screens().focused_screen() == composed.focused_context &&
           composed.civilopedia_search_box.has_value() &&
           frame.snapshot.find<SystemScreenFrame>();
}

bool inside(const WorldViewRect &r, std::int32_t x, std::int32_t y) {
    return r.width > 0 && r.height > 0 && x >= r.x && y >= r.y &&
           std::int64_t(x) < std::int64_t(r.x) + r.width &&
           std::int64_t(y) < std::int64_t(r.y) + r.height;
}

void append_utf8(std::string &result, char32_t value) {
    if (value <= 0x7f) result.push_back(static_cast<char>(value));
    else if (value <= 0x7ff) {
        result.push_back(static_cast<char>(0xc0 | (value >> 6)));
        result.push_back(static_cast<char>(0x80 | (value & 0x3f)));
    } else if (value <= 0xffff) {
        result.push_back(static_cast<char>(0xe0 | (value >> 12)));
        result.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3f)));
        result.push_back(static_cast<char>(0x80 | (value & 0x3f)));
    } else {
        result.push_back(static_cast<char>(0xf0 | (value >> 18)));
        result.push_back(static_cast<char>(0x80 | ((value >> 12) & 0x3f)));
        result.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3f)));
        result.push_back(static_cast<char>(0x80 | (value & 0x3f)));
    }
}

std::size_t utf8_characters(std::string_view text) {
    return std::count_if(text.begin(), text.end(), [](char ch) {
        return (static_cast<unsigned char>(ch) & 0xc0) != 0x80;
    });
}
} // namespace

bool CivilopediaSearchEditor::select(LiveUiHost &host, const LiveUiFrame &frame,
    const ComposedUiFrame &composed, std::int32_t press_x, std::int32_t press_y,
    std::int32_t release_x, std::int32_t release_y) {
    if (!current_search_frame(host, frame, composed)) {
        blur();
        return false;
    }
    const auto &box = *composed.civilopedia_search_box;
    focused_ = inside(box, press_x, press_y) &&
               inside(box, release_x, release_y);
    high_surrogate_ = 0;
    return focused_;
}

namespace {
bool current_popup_edit_frame(const LiveUiHost &host, const LiveUiFrame &frame,
                              const ComposedUiFrame &composed) {
    const auto *view = frame.snapshot.find<PopupTextView>();
    return view && host.is_current_frame(frame) &&
           composed.frame_owner == frame.owner &&
           composed.frame_sequence == frame.sequence &&
           composed.snapshot_frame == frame.snapshot.frame() &&
           composed.focused_context == std::optional<std::string>{view->context_id} &&
           host.screens().focused_screen() == composed.focused_context &&
           composed.popup_edit_box.has_value() &&
           frame.snapshot.active_player_id() &&
           view->player_id == *frame.snapshot.active_player_id();
}
}

bool PopupNameEditor::select(LiveUiHost &host, const LiveUiFrame &frame,
    const ComposedUiFrame &composed, std::int32_t press_x, std::int32_t press_y,
    std::int32_t release_x, std::int32_t release_y) {
    if (!current_popup_edit_frame(host, frame, composed)) {
        blur();
        return false;
    }
    const auto &box = *composed.popup_edit_box;
    focused_ = inside(box, press_x, press_y) &&
               inside(box, release_x, release_y);
    high_surrogate_ = 0;
    if (focused_) {
        const auto *view = frame.snapshot.find<PopupTextView>();
        context_id_ = view->context_id;
        player_id_ = view->player_id;
        event_id_ = view->event_id;
        pending_submit_ = false;
    } else blur();
    return focused_;
}

UiCommandResult PopupNameEditor::input(LiveUiHost &host,
    const LiveUiFrame &frame, const ComposedUiFrame &composed,
    std::span<const char16_t> characters) {
    const auto *view = frame.snapshot.find<PopupTextView>();
    if (!focused_ || !current_popup_edit_frame(host, frame, composed) ||
        !view || view->context_id != context_id_ ||
        view->player_id != player_id_ || view->event_id != event_id_) {
        blur();
        return {UiCommandStatus::invalid, "Popup EditBox has no current focus"};
    }
    std::string text = frame.snapshot.find<PopupTextView>()->text;
    bool changed = false, submit = false;
    for (const auto code : characters) {
        if (code == u'\r' || code == u'\n') {
            high_surrogate_ = 0;
            submit = true;
            continue;
        }
        if (code == u'\b') {
            high_surrogate_ = 0;
            if (!text.empty()) {
                while (!text.empty()) {
                    const bool lead =
                        (static_cast<unsigned char>(text.back()) & 0xc0) != 0x80;
                    text.pop_back();
                    if (lead) break;
                }
                changed = true;
            }
            continue;
        }
        char32_t scalar = 0;
        if (code >= 0xd800 && code <= 0xdbff) {
            high_surrogate_ = code;
            continue;
        }
        if (code >= 0xdc00 && code <= 0xdfff) {
            if (!high_surrogate_) continue;
            scalar = 0x10000 + ((char32_t(high_surrogate_) - 0xd800) << 10) +
                     (char32_t(code) - 0xdc00);
            high_surrogate_ = 0;
        } else {
            high_surrogate_ = 0;
            scalar = code;
        }
        const auto max_chars = view->context_id == "ChooseReligion" ? 16 : 15;
        if (scalar < 0x20 || scalar == 0x7f ||
            utf8_characters(text) >= static_cast<std::size_t>(max_chars))
            continue;
        append_utf8(text, scalar);
        changed = true;
    }
    if (changed) {
        const auto result = host.edit_popup_text(text, frame);
        pending_submit_ = (submit || pending_submit_) &&
                          result.status == UiCommandStatus::navigation;
        return result;
    }
    if (submit || pending_submit_) {
        pending_submit_ = false;
        const auto *view = frame.snapshot.find<PopupTextView>();
        if (view && view->context_id == "ChooseReligion") {
            const auto *selected =
                frame.snapshot.find<ReligionSelectionView>();
            if (!selected || !selected->editing_name ||
                selected->event_id != view->event_id)
                return {UiCommandStatus::invalid,
                    "Religion name owner changed before submission"};
            return host.dispatch(UiGameplayCommand::make("ChooseReligion",
                ReligionUiAction{ReligionUiActionKind::accept_name,
                    selected->owner_sequence, selected->event_id,
                    selected->religion_id}), frame);
        }
        const auto *events = frame.snapshot.find<PopupFrame>();
        if (!events || events->player_id != view->player_id)
            return {UiCommandStatus::invalid, "Popup event changed before name submission"};
        const auto event = std::find_if(events->events.begin(), events->events.end(),
            [&](const PopupEvent &candidate) {
                return candidate.context_id == view->context_id &&
                       candidate.event_id == view->event_id;
            });
        if (event == events->events.end() || event->choices.size() != 1)
            return {UiCommandStatus::unavailable,
                    "Rename has no unique live accept choice"};
        return host.dispatch(popup_choice_command({view->context_id,
            view->event_id, event->choices.front().id, view->text}), frame);
    }
    return {UiCommandStatus::unavailable, "No popup name text change"};
}

UiCommandResult CivilopediaSearchEditor::input(LiveUiHost &host,
    const LiveUiFrame &frame, const ComposedUiFrame &composed,
    std::span<const char16_t> characters) {
    if (!focused_ || !current_search_frame(host, frame, composed)) {
        blur();
        return {UiCommandStatus::invalid, "Civilopedia search input has no current focus"};
    }
    std::string query = frame.snapshot.find<SystemScreenFrame>()->pedia_search;
    bool submit = false, changed = false;
    for (const auto code : characters) {
        if (code == u'\r' || code == u'\n') {
            submit = true;
            high_surrogate_ = 0;
            continue;
        }
        if (code == u'\b') {
            high_surrogate_ = 0;
            if (!query.empty()) {
                while (!query.empty()) {
                    const bool lead =
                        (static_cast<unsigned char>(query.back()) & 0xc0) != 0x80;
                    query.pop_back();
                    if (lead) break;
                }
                changed = true;
            }
            continue;
        }
        char32_t scalar = 0;
        if (code >= 0xd800 && code <= 0xdbff) {
            high_surrogate_ = code;
            continue;
        }
        if (code >= 0xdc00 && code <= 0xdfff) {
            if (!high_surrogate_) continue;
            scalar = 0x10000 + ((char32_t(high_surrogate_) - 0xd800) << 10) +
                     (char32_t(code) - 0xdc00);
            high_surrogate_ = 0;
        } else {
            high_surrogate_ = 0;
            scalar = code;
        }
        if (scalar < 0x20 || scalar == 0x7f || utf8_characters(query) >= 50)
            continue;
        append_utf8(query, scalar);
        changed = true;
    }
    if (!changed && !submit)
        return {UiCommandStatus::unavailable, "No Civilopedia search text change"};
    return host.search_civilopedia(query, frame);
}

} // namespace civ5::ui

namespace civ5::ui {
struct BnwOwnedCameraGuiScreen::Impl {
    std::atomic<bool> retired{false},failed{false};
    std::weak_ptr<const std::uint8_t> compositor_lifetime;
    const Loan* active=nullptr;
    const UnitFlagLifecycleLoan* unitflag_event_active=nullptr;
    const ResearchChoiceLoan::Action* research_active=nullptr;
    struct ResearchSource {
        std::atomic<bool> failed{false};
        const ResearchChoiceLoan::Action* active=nullptr;
        std::weak_ptr<const std::uint8_t> renderer_lifetime;
        std::shared_ptr<UiScreenCompositor> plans;
        std::shared_ptr<const std::function<void(const std::shared_ptr<const BnwOwnedCameraGuiScreen>&,
                                                const LiveUiFrame&,const ComposedUiFrame&)>> publish;
    };
    mutable std::shared_ptr<ResearchSource> research;
    // Immutable identity survives retirement; weak controls give no authority.
    std::weak_ptr<BnwClientSceneFrame> origin_scene;
    std::weak_ptr<LiveUiHost> origin_host;
    std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease> origin_window;
    std::weak_ptr<const std::uint8_t> origin_renderer;
    BnwClientFrameIdentity origin_frame;
    bool scene_pin_present=false;
    void clear_scene_pin()noexcept {
        if(scene_pin_present){scene_pin_present=false;--scene->gui_publication_pins_;}
    }
    ~Impl(){clear_scene_pin();}
    std::shared_ptr<BnwClientSceneFrame> scene;
    std::shared_ptr<LiveUiHost> host;
    std::shared_ptr<const BnwClientFrame> frame;
    std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window;
    LiveUiFrame ui;
    ComposedUiFrame layout;
    std::vector<std::pair<const UiContextDescriptor*,bool>> registrations;
    Impl(std::shared_ptr<BnwClientSceneFrame>s,std::shared_ptr<LiveUiHost>h,
         std::shared_ptr<const BnwClientFrame>f,
         std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>w,
         LiveUiFrame u,ComposedUiFrame l):scene(std::move(s)),host(std::move(h)),
         frame(std::move(f)),window(std::move(w)),ui(std::move(u)),layout(std::move(l)){
        origin_scene=scene;origin_host=host;origin_window=window;origin_frame=frame->stamp.identity;
        ++scene->gui_publication_pins_;scene_pin_present=true;
    }
    void release_retired_storage()noexcept {
        if(retired.load()&&!active&&!research_active&&!unitflag_event_active){
            research.reset();window.reset();frame.reset();host.reset();clear_scene_pin();scene.reset();
        }
    }
};
class BnwOwnedCameraGuiScreen::Pin {
    const BnwOwnedCameraGuiScreen& owner_;
 public:
    std::shared_ptr<const BnwOwnedCameraGuiScreen> storage;
    explicit Pin(const BnwOwnedCameraGuiScreen& owner):owner_(owner) {
        if(owner.thread_!=std::this_thread::get_id())throw std::runtime_error("GUI publication foreign thread");
        storage=owner.self_.lock();
        if(!storage||storage.get()!=&owner||storage.owner_before(owner.self_)||owner.self_.owner_before(storage))
            throw std::runtime_error("GUI publication factory owner absent");
        ++owner.pins_;
    }
    ~Pin(){--owner_.pins_;}
};
BnwOwnedCameraGuiScreen::BnwOwnedCameraGuiScreen(std::shared_ptr<Impl>p):impl_(std::move(p)){}
BnwOwnedCameraGuiScreen::~BnwOwnedCameraGuiScreen(){retire();}
void BnwOwnedCameraGuiScreen::require_origin_lifetime_without_frame_callbacks()const {
    // Creator before any mutable source/registration access. No old frame or
    // UI revision validation is performed after genuine native stores.
    if(thread_!=std::this_thread::get_id())throw std::runtime_error("GUI origin foreign creator");
    Pin pin(*this);const auto p=impl_;
    const auto same=[](const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);};
    if(p->compositor_lifetime.expired()||p->retired.load()||p->failed.load()||
       (p->research&&(p->research->failed.load()||p->research->renderer_lifetime.expired()))||
       self_.use_count()<=static_cast<long>(pins_)||!p->scene||!p->host||!p->frame||!p->window)
        throw std::runtime_error("GUI origin producer/retained storage retired");
    if(p->origin_scene.expired()||!same(p->origin_scene,p->scene)||
       p->origin_host.expired()||!same(p->origin_host,p->host)||
       p->origin_window.expired()||!same(p->origin_window,p->window)||
       p->scene->gui_thread_!=thread_||!same(p->scene->gui_self_,p->scene)||
       p->scene->gui_self_.use_count()<=static_cast<long>(p->scene->gui_publication_pins_.load())||
       !p->scene->selected_host_||p->scene->scene_state_!=&p->scene->session_.state()||
       p->scene->scene_state_generation_!=p->scene->session_.state_generation())
        throw std::runtime_error("GUI origin canonical Scene/UI/Window/source replaced");
    p->scene->owner_.require_same_gui_ui_host(p->host); // Existing pure actual registration/control.
    p->window->require_origin_lifetime_without_native_callbacks();
}
struct BnwOwnedCameraGuiScreen::UnitFlagLifecycleLoan::State {
    const std::thread::id creator=std::this_thread::get_id();
    const int unwind=std::uncaught_exceptions();
    const BnwOwnedCameraGuiScreen* const owner;
    const std::shared_ptr<Impl> storage;
    const runtime::BnwStockLuaAttemptLease lease;
    const runtime::BnwStockLuaAttemptLease::ScopeIdentity identity;
    const UnitFlagLifecycleLoan* const loan;
    std::atomic<bool> alive{true};
    mutable bool checking=false;
    State(const BnwOwnedCameraGuiScreen& actual,const runtime::BnwStockLuaAttemptLease& source,
          const UnitFlagLifecycleLoan* token)
      :owner(&actual),storage(actual.impl_),lease(source),
       identity(source.pin_active_scope_without_callbacks()),loan(token){}
    void first()const {
        // Independent saved storage before any borrowed facade/Scene reads.
        if(creator!=std::this_thread::get_id()){
            storage->failed.store(true);lease.invalidate();
            throw std::runtime_error("UnitFlag GUI lifecycle foreign creator");
        }
        if(!alive.load()||storage->retired.load()||storage->failed.load())
            throw std::runtime_error("UnitFlag GUI lifecycle producer retired or poisoned");
        lease.require_scope_identity_without_callbacks(identity);
    }
    void pure()const {
        first();
        if(storage->unitflag_event_active!=loan||storage->active||storage->research_active){
            storage->failed.store(true);lease.invalidate();
            throw std::runtime_error("UnitFlag GUI lifecycle active token substituted or reentered");
        }
        owner->require_origin_lifetime_without_frame_callbacks();first();
    }
    void full()const {
        pure();
        if(checking){fail();throw std::runtime_error("UnitFlag GUI lifecycle full checker reentered");}
        checking=true;struct End{bool& active;~End(){active=false;}}end{checking};
        try{lease.require_operation(identity.serial);first();pure();}catch(...){
            const auto original=std::current_exception();
            try{first();}catch(...){fail();}
            fail();std::rethrow_exception(original);
        }
    }
    void fail()const noexcept {storage->failed.store(true);lease.invalidate();}
};
BnwOwnedCameraGuiScreen::UnitFlagLifecycleLoan::UnitFlagLifecycleLoan(
    const BnwOwnedCameraGuiScreen& owner,const runtime::BnwStockLuaAttemptLease& source)
  :state_(std::make_shared<State>(owner,source,this)){
    const auto s=state_;
    try{
        s->first();
        if(s->storage->unitflag_event_active||s->storage->active||s->storage->research_active){
            s->fail();throw std::runtime_error("UnitFlag GUI lifecycle borrower reentered");
        }
        s->storage->unitflag_event_active=this;
        s->full();
    }catch(...){
        s->alive.store(false);s->fail();
        // Failed nested admission must not clear the enclosing token.
        if(s->creator==std::this_thread::get_id()){
            if(s->storage->unitflag_event_active==this)s->storage->unitflag_event_active=nullptr;
            s->storage->release_retired_storage();
        }
        throw;
    }
}
BnwOwnedCameraGuiScreen::UnitFlagLifecycleLoan::~UnitFlagLifecycleLoan(){
    const auto s=state_;s->alive.store(false);
    if(s->creator!=std::this_thread::get_id()){s->fail();std::terminate();}
    if(std::uncaught_exceptions()>s->unwind||s->storage->unitflag_event_active!=this)s->fail();
    s->storage->unitflag_event_active=nullptr;
    s->storage->release_retired_storage();
}
std::shared_ptr<BnwOwnedCameraGuiScreen::UnitFlagLifecycleLoan::State>
BnwOwnedCameraGuiScreen::UnitFlagLifecycleLoan::retain_storage_without_callbacks()const {
    const auto s=state_;require_origin_from_storage_without_callbacks(s);return s;
}
void BnwOwnedCameraGuiScreen::UnitFlagLifecycleLoan::require_origin_from_storage_without_callbacks(
    const std::shared_ptr<State>& s){
    if(!s)throw std::runtime_error("UnitFlag GUI lifecycle saved storage absent");
    try{s->pure();}catch(...){s->fail();throw;}
}
void BnwOwnedCameraGuiScreen::UnitFlagLifecycleLoan::require_current_from_storage_owned(
    const std::shared_ptr<State>& s){
    if(!s)throw std::runtime_error("UnitFlag GUI lifecycle saved storage absent");
    try{s->full();}catch(...){s->fail();throw;}
}
void BnwOwnedCameraGuiScreen::UnitFlagLifecycleLoan::require_origin_without_callbacks()const {
    const auto s=state_;require_origin_from_storage_without_callbacks(s);
}
void BnwOwnedCameraGuiScreen::UnitFlagLifecycleLoan::require_current_owned()const {
    const auto s=state_;require_current_from_storage_owned(s);
}
void BnwOwnedCameraGuiScreen::UnitFlagLifecycleLoan::require_source_origin_without_callbacks(
    const std::weak_ptr<const SelectedBnwPreviewHost>& expected_host,
    const std::weak_ptr<const runtime::LiveSessionState>& expected_world,std::uint64_t generation)const {
    const auto s=state_;
    require_source_origin_from_storage_without_callbacks(s,expected_host,expected_world,generation);
}
void BnwOwnedCameraGuiScreen::UnitFlagLifecycleLoan::require_source_origin_from_storage_without_callbacks(
    const std::shared_ptr<State>& s,
    const std::weak_ptr<const SelectedBnwPreviewHost>& expected_host,
    const std::weak_ptr<const runtime::LiveSessionState>& expected_world,std::uint64_t generation){
    if(!s)throw std::runtime_error("UnitFlag GUI lifecycle saved storage absent");
    try{
        s->pure();
        {
            const auto host=expected_host.lock();const auto world=expected_world.lock();
            const auto& p=*s->storage;
            if(!host||!world||!p.scene||p.scene->selected_host_.get()!=host.get()||
               p.scene->selected_host_.owner_before(host)||host.owner_before(p.scene->selected_host_)||
               p.scene->scene_state_!=world.get()||p.scene->scene_state_generation_!=generation)
                throw std::runtime_error("UnitFlag lifecycle SAME Scene Host/world/generation differs");
            const auto actual_world=host->session().read_state_lease();
            if(!actual_world||actual_world.get()!=world.get()||actual_world.owner_before(world)||
               world.owner_before(actual_world)||host->session().state_generation()!=generation)
                throw std::runtime_error("UnitFlag lifecycle actual Host world/control replaced");
        }
        // Every temporary checker pin is released before the final origin.
        s->pure();
    }catch(...){s->fail();throw;}
}
void BnwOwnedCameraGuiScreen::with_actual_unitflag_lifecycle_owned(
    const runtime::BnwStockLuaAttemptLease& source,
    const std::function<void(const UnitFlagLifecycleLoan&)>& borrowed)const {
    if(thread_!=std::this_thread::get_id()){
        impl_->failed.store(true);source.invalidate();
        throw std::runtime_error("UnitFlag lifecycle GUI foreign creator");
    }
    Pin pin(*this);const auto p=impl_;
    require_origin_lifetime_without_frame_callbacks();
    UnitFlagLifecycleLoan loan(*this,source);const auto s=loan.state_;
    std::optional<std::function<void(const UnitFlagLifecycleLoan&)>> target;
    try{
        loan.require_current_owned();
        target.emplace(borrowed);s->first();loan.require_current_owned();
        if(!*target)throw std::runtime_error("UnitFlag lifecycle callback target absent");
        (*target)(loan);s->first();loan.require_current_owned();
        target.reset();s->first();loan.require_current_owned();
    }catch(...){
        const auto original=std::current_exception();
        // Target DTOR remains inside the active scope; caught reentry or
        // retirement cannot erase already committed health writes/failure.
        target.reset();
        try{s->first();s->pure();}catch(...){s->fail();}
        s->fail();std::rethrow_exception(original);
    }
}
void BnwOwnedCameraGuiScreen::check()const {
    if(thread_!=std::this_thread::get_id())throw std::runtime_error("GUI publication foreign thread");
    const auto p=impl_;
    if(p->unitflag_event_active){p->failed.store(true);throw std::runtime_error("GUI lifecycle event cannot lend a render/frame publication");}
    const auto pure=[&]{
        if(p->compositor_lifetime.expired()||p->retired.load()||p->failed.load()||
           (p->research&&p->research->failed.load())||self_.use_count()<=static_cast<long>(pins_)||
           !p->scene||!p->host||!p->frame||!p->window)
            throw std::runtime_error("GUI publication retired or outside owner absent");
        const auto& scene=p->scene;
        if(scene->gui_thread_!=thread_||scene->gui_self_.expired()||scene->gui_self_.use_count()<=static_cast<long>(scene->gui_publication_pins_.load())||
           scene.owner_before(scene->gui_self_)||scene->gui_self_.owner_before(scene)||
           scene->frame_.get()!=p->frame.get()||scene->frame_.owner_before(p->frame)||p->frame.owner_before(scene->frame_))
            throw std::runtime_error("GUI publication actual Scene/frame owner changed");
        scene->owner_.require_same_gui_ui_host(p->host);
        const auto* stamp=p->ui.snapshot.find<BnwClientFrameStamp>();
        if(!stamp||stamp->identity!=p->frame->stamp.identity||stamp->selection!=p->frame->stamp.selection||
           stamp->viewport!=p->frame->stamp.viewport||
           stamp!=p->frame->ui.find<BnwClientFrameStamp>()||
           p->layout.frame_owner!=p->ui.owner||p->layout.frame_sequence!=p->ui.sequence||
           p->layout.snapshot_frame!=p->ui.snapshot.frame()||
           p->layout.focused_context!=p->host->screens().focused_screen())
            throw std::runtime_error("GUI publication layout/snapshot identity changed");
        const auto& registry=p->host->screens().registry().contexts();
        if(registry.size()!=p->registrations.size())throw std::runtime_error("GUI registry replaced");
        for(std::size_t n=0;n<registry.size();++n)
            if(&registry[n]!=p->registrations[n].first||p->host->screens().visible(registry[n].id)!=p->registrations[n].second)
                throw std::runtime_error("GUI registration/visibility changed");
    };
    pure();p->window->require_current();pure();
    if(!p->scene->owner_.is_current(p->frame))throw std::runtime_error("GUI client frame stale");pure();
    if(!p->scene->ready_to_draw())throw std::runtime_error("GUI scene revision differs");pure();
    if(!p->host->is_current_frame(p->ui))throw std::runtime_error("GUI live UI frame stale");pure();
    p->window->require_current();pure();
}
void BnwOwnedCameraGuiScreen::require_current()const {Pin pin(*this);check();}
std::shared_ptr<const BnwOwnedCameraGuiScreen>BnwOwnedCameraGuiScreen::retain_owner()const {
    Pin pin(*this);check();return pin.storage;
}
void BnwOwnedCameraGuiScreen::require_same_selected_host(const SelectedBnwPreviewHost& host)const {
    Pin pin(*this);check();
    if(impl_->scene->selected_host_.get()!=&host)throw std::runtime_error("GUI selected Host differs");check();
}
void BnwOwnedCameraGuiScreen::require_same_window(const graphics::BnwOwnedCameraRendererWindowLease& window)const {
    Pin pin(*this);check();if(impl_->window.get()!=&window)throw std::runtime_error("GUI physical Window differs");check();
}
void BnwOwnedCameraGuiScreen::require_worldview_source_origin_without_callbacks(
 const Loan& loan,const graphics::BnwOwnedCameraRendererWindowLease& window,
 const std::weak_ptr<const SelectedBnwPreviewHost>& expected_host,
 const std::weak_ptr<const runtime::LiveSessionState>& expected_world,std::uint64_t generation)const {
 if(thread_!=std::this_thread::get_id()){
  impl_->failed.store(true);throw std::runtime_error("WorldView GUI origin foreign creator");
 }
 require_origin_lifetime_without_frame_callbacks();
 const auto p=impl_;const auto host=expected_host.lock();const auto world=expected_world.lock();
 if(&loan.owner_!=this||p->active!=&loan||p->retired.load()||p->failed.load()||
    !p->scene||!p->window||p->window.get()!=&window||
    p->origin_window.owner_before(p->window)||p->window.owner_before(p->origin_window)||
    !host||!world||p->scene->selected_host_.get()!=host.get()||
    p->scene->selected_host_.owner_before(host)||host.owner_before(p->scene->selected_host_)||
    p->scene->scene_state_!=world.get()||p->scene->scene_state_generation_!=generation){
  p->failed.store(true);throw std::runtime_error("WorldView GUI has another actual Source Host/world/generation");
 }
 const auto actual_world=host->session().read_state_lease();
 if(!actual_world||actual_world.get()!=world.get()||actual_world.owner_before(world)||world.owner_before(actual_world)||
    host->session().state_generation()!=generation){
  p->failed.store(true);throw std::runtime_error("WorldView actual Host session world allocation changed");
 }
 window.require_origin_lifetime_without_native_callbacks();
 require_origin_lifetime_without_frame_callbacks();
}
void BnwOwnedCameraGuiScreen::with_actual_worldview_context_loans(const std::function<void(const Loan&,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan&)>& target)const {
 // Both actual existing lenders retain their own callback target storage and
 // check COPY/invoke/destruction. This wrapper mints no new GUI/window proof.
 Pin pin(*this);const auto p=impl_;check();
 if(!p->window)throw std::runtime_error("WorldView actual GUI has no physical Window");
 const auto* actual_window=p->window.get();
 actual_window->with_actual_window_loan([&](const auto& window){
  require_origin_lifetime_without_frame_callbacks();check();
  with_actual_screen_loan([&](const auto& gui){
   require_origin_lifetime_without_frame_callbacks();window.require_current();gui.require_current();
   std::optional<std::function<void(const Loan&,const graphics::BnwOwnedCameraRendererWindowLease::Loan&)>> owned;
   try{
    owned.emplace(target);require_origin_lifetime_without_frame_callbacks();gui.require_current();window.require_current();
    if(!*owned)throw std::invalid_argument("WorldView native context target absent");
    (*owned)(gui,window);require_origin_lifetime_without_frame_callbacks();gui.require_current();window.require_current();
    owned.reset();require_origin_lifetime_without_frame_callbacks();gui.require_current();window.require_current();
   }catch(...){p->failed.store(true);owned.reset();throw;}
  });
  require_origin_lifetime_without_frame_callbacks();check();window.require_current();
 });
 require_origin_lifetime_without_frame_callbacks();check();
}
void BnwOwnedCameraGuiScreen::with_worldview_frame_storage_owned(const Loan& loan,
 const std::function<void(const BnwClientSceneFrame&,const BnwClientFrame&,
  const LiveUiFrame&,const ComposedUiFrame&)>& borrowed)const {
 Pin pin(*this);const auto p=impl_;
 const auto current=[&]{
  require_origin_lifetime_without_frame_callbacks();
  if(&loan.owner_!=this||p->active!=&loan)throw std::runtime_error("WorldView frame storage outside actual GUI loan");
  loan.require_current();require_origin_lifetime_without_frame_callbacks();
  if(!p->scene||!p->frame||!p->host)throw std::runtime_error("WorldView current GUI frame storage retired");
 };
 std::optional<std::function<void(const BnwClientSceneFrame&,const BnwClientFrame&,
  const LiveUiFrame&,const ComposedUiFrame&)>> target;
 try{
  current();target.emplace(borrowed);current();
  if(!*target)throw std::invalid_argument("WorldView frame storage target absent");
  (*target)(*p->scene,*p->frame,p->ui,p->layout);current();target.reset();current();
 }catch(...){p->failed.store(true);target.reset();throw;}
}
void BnwOwnedCameraGuiScreen::require_notification_storage_origin_without_callbacks(
 const Loan& loan,const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,
 const BnwClientSceneFrame& scene,const BnwClientFrame& frame,const LiveUiFrame& ui,
 const ComposedUiFrame& layout,const std::weak_ptr<const BnwOwnedCameraGuiScreen>& expected_gui,
 const std::weak_ptr<const BnwClientSceneFrame>& expected_scene,
 const std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease>& expected_window,
 const std::weak_ptr<const SelectedBnwPreviewHost>& expected_model_host,
 const std::weak_ptr<const runtime::LiveSessionState>& expected_model_world,
 std::uint64_t expected_model_generation)const {
 if(thread_!=std::this_thread::get_id())throw std::runtime_error("Notification GUI origin foreign creator");
 Pin pin(*this);const auto p=impl_;
 try {
  require_origin_lifetime_without_frame_callbacks();
  const auto same=[](const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);};
  if(&loan.owner_!=this||p->active!=&loan||!p->scene||!p->frame||!p->host||!p->window||
     &window.owner_!=p->window.get()||p->scene.get()!=&scene||p->frame.get()!=&frame||
     &p->ui!=&ui||&p->layout!=&layout||
     scene.frame_.get()!=&frame||!same(scene.frame_,p->frame)||
     scene.gui_screen_.expired()||!same(scene.gui_screen_,self_)||
     ui.owner!=p->host.get()||ui.sequence!=p->host->sequence_||
     ui.owner!=layout.frame_owner||ui.sequence!=layout.frame_sequence||
     ui.snapshot.frame()!=layout.snapshot_frame||
     p->origin_frame!=frame.stamp.identity)
   throw std::runtime_error("Notification GUI active loan or exact frame/layout storage changed");
  const auto* stamp=ui.snapshot.find<BnwClientFrameStamp>();
  if(!stamp||stamp!=frame.ui.find<BnwClientFrameStamp>()||
     stamp->identity!=frame.stamp.identity||stamp->selection!=frame.stamp.selection||
     stamp->viewport!=frame.stamp.viewport)
   throw std::runtime_error("Notification GUI actual source stamp storage changed");
  {
   // Pure-only temporary identity locks; drop all before the final origin.
   const auto g=expected_gui.lock();const auto sc=expected_scene.lock();const auto w=expected_window.lock();
   if(!g||!sc||!w||g.get()!=this||sc.get()!=&scene||w.get()!=p->window.get()||
      !same(g,self_)||!same(sc,p->scene)||!same(w,p->window)||
      !same(expected_scene,p->origin_scene)||!same(expected_window,p->origin_window))
    throw std::runtime_error("Notification GUI canonical weak controls substituted");
   const auto host=expected_model_host.lock();const auto world=expected_model_world.lock();
   if(!host||!world||!scene.selected_host_||host.get()!=scene.selected_host_.get()||
      !same(host,scene.selected_host_)||world.get()!=scene.scene_state_||
      expected_model_generation!=scene.scene_state_generation_||
      &scene.session_!=&host->session())
    throw std::runtime_error("Notification GUI and actual Model Host/world/generation differ");
   const auto actual_world=host->session().read_state_lease();
   if(!actual_world||actual_world.get()!=world.get()||!same(actual_world,world)||
      host->session().state_generation()!=expected_model_generation||
      &scene.session_.state()!=world.get())
    throw std::runtime_error("Notification GUI and Model canonical session world changed");
  }
  // Existing counted GUI/Scene/window inventory, no providers/native queries.
  require_origin_lifetime_without_frame_callbacks();
 }catch(...){p->failed.store(true);throw;}
}
void BnwOwnedCameraGuiScreen::with_actual_notification_publication_owned(
 const runtime::BnwOwnedGameplayEventPumpLoan& pump,
 const runtime::BnwStockLuaAttemptLease& source,
 const std::function<void(BnwNotificationPublicationLender&)>& borrowed)const {
 if(thread_!=std::this_thread::get_id())throw std::runtime_error("Notification GUI factory foreign creator");
 Pin pin(*this);const auto p=impl_;
 // Failure-only retired storage: no active scope is kept past lender retirement.
 std::shared_ptr<BnwNotificationPublicationLender::Storage> captured;
 try {
  require_origin_lifetime_without_frame_callbacks();check();require_origin_lifetime_without_frame_callbacks();
  const auto serial=source.operation_serial();require_origin_lifetime_without_frame_callbacks();
  const auto current=[&]{
   require_origin_lifetime_without_frame_callbacks();
   pump.require_for_source(source);require_origin_lifetime_without_frame_callbacks();
   source.require_operation(serial);require_origin_lifetime_without_frame_callbacks();
  };
  current();
  // Only internal reference-capture lambdas are copied by the nested lenders.
  // The actual caller target is not copied before the counted model exists.
  with_actual_worldview_context_loans([&](const Loan& gui,const auto& window){
   with_worldview_frame_storage_owned(gui,[&](const auto& scene,const auto& frame,
                                             const auto& ui,const auto& layout){
    std::unique_ptr<BnwNotificationOwnerModelLoan> model;
    try {
     current();
     require_origin_lifetime_without_frame_callbacks();
     const std::weak_ptr<const SelectedBnwPreviewHost> scene_host=scene.selected_host_;
     std::weak_ptr<const runtime::LiveSessionState> scene_world;
     {const auto world=scene.session_.read_state_lease();scene_world=world;}
     const auto scene_generation=scene.scene_state_generation_;
     require_notification_storage_origin_without_callbacks(gui,window,scene,frame,ui,layout,
         self_,p->origin_scene,p->origin_window,scene_host,scene_world,scene_generation);
     // Borrow the actual registered UI child; no extra uncounted owner pin.
     const auto* owner=p->host->notification_publication_owner_.get();
     if(!owner)throw std::runtime_error("Notification actual canonical UI child model absent");
     model=BnwNotificationPublicationLender::capture_model_owned(*owner,ui,pump,source);
     require_notification_storage_origin_without_callbacks(gui,window,scene,frame,ui,layout,
         self_,p->origin_scene,p->origin_window,scene_host,scene_world,scene_generation);
     // Source4 lvalue transfer: allocation failure leaves model for this catch.
     BnwNotificationPublicationLender lender(model,*this,gui,window,scene,frame,ui,layout,
         pump,source,self_,p->origin_scene,p->origin_window);
     const auto saved=lender.storage_;captured=saved;
     try {
      lender.invoke_owned(borrowed);
      current();
      require_notification_storage_origin_without_callbacks(gui,window,scene,frame,ui,layout,
          self_,p->origin_scene,p->origin_window,scene_host,scene_world,scene_generation);
     }catch(...){
      BnwNotificationPublicationLender::quarantine_from_storage_without_callbacks(saved);throw;
     }
     // lender retires before frame storage, GUI, window and source scopes.
    }catch(...){
     BnwNotificationPublicationLender::quarantine_captured_model_without_callbacks(model);
     p->failed.store(true);throw;
    }
   });
  });
  current();
 }catch(...){
  BnwNotificationPublicationLender::quarantine_from_storage_without_callbacks(captured);
  p->failed.store(true);throw;
 }
}
void BnwOwnedCameraGuiScreen::retire()const noexcept {
    impl_->retired.store(true);
    if(thread_==std::this_thread::get_id())impl_->release_retired_storage();
}
void BnwOwnedCameraGuiScreen::with_actual_screen_loan(const std::function<void(const Loan&)>& borrowed)const {
    Pin pin(*this);const auto p=impl_;
    if(p->active){p->failed.store(true);throw std::runtime_error("GUI screen loan reentry");}
    Loan loan(*this);p->active=&loan;
    struct End {Impl& p;~End(){p.active=nullptr;p.release_retired_storage();}} end{*p};
    std::optional<std::function<void(const Loan&)>> target;
    try {
        // Callback-free ownership/retirement check before reading borrowed target.
        if(p->compositor_lifetime.expired()||p->retired.load()||p->failed.load()||
           self_.use_count()<=static_cast<long>(pins_))throw std::runtime_error("GUI loan source retired before target copy");
        target.emplace(borrowed);check();
        if(!*target)throw std::invalid_argument("GUI screen target absent");
        (*target)(loan);check();target.reset();check();
    }catch(...){p->failed.store(true);target.reset();throw;}
}
void BnwOwnedCameraGuiScreen::Loan::require_current()const {
    if(owner_.thread_!=std::this_thread::get_id()||owner_.impl_->active!=this)
        throw std::runtime_error("GUI screen loan is not the actual active loan");owner_.check();
}
std::int32_t BnwOwnedCameraGuiScreen::Loan::width()const {require_current();return owner_.impl_->frame->stamp.viewport.framebuffer_width;}
std::int32_t BnwOwnedCameraGuiScreen::Loan::height()const {require_current();return owner_.impl_->frame->stamp.viewport.framebuffer_height;}
std::uint64_t BnwOwnedCameraGuiScreen::Loan::frame_revision()const {require_current();return owner_.impl_->frame->stamp.identity.frame;}
std::shared_ptr<const BnwOwnedCameraGuiScreen>UiScreenCompositor::compose_owned_current_screen(
    const std::shared_ptr<BnwClientSceneFrame>& borrowed_scene,
    const std::shared_ptr<LiveUiHost>& borrowed_host,
    const std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease>& borrowed_window,
    const LiveUiFrame& borrowed_ui,const MeasuredProductionPopup* production,
    const EconomicBoundFields* economic_fields)const {
    if(gui_producer_.thread!=std::this_thread::get_id())throw std::runtime_error("Actual compositor foreign GUI factory");
    // Snapshot all borrowed storage before the first source/provider callback.
    auto scene=borrowed_scene;const auto host=borrowed_host;const auto incoming_window=borrowed_window;
    const auto ui=borrowed_ui;
    const auto producer=std::weak_ptr<const std::uint8_t>(gui_producer_.life);
    const auto owned_production=production?std::optional<MeasuredProductionPopup>{*production}:std::nullopt;
    const auto owned_economic=economic_fields?std::optional<EconomicBoundFields>{*economic_fields}:std::nullopt;
    if(!gui_producer_.life||!scene||scene->gui_thread_!=std::this_thread::get_id()||!host||!incoming_window||
       scene->gui_self_.expired()||scene.owner_before(scene->gui_self_)||scene->gui_self_.owner_before(scene)||
       !scene->selected_host_)throw std::runtime_error("GUI factory needs actual selected Scene factory owner");
    scene->owner_.require_same_gui_ui_host(host);
    std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window;
    incoming_window->with_actual_window_loan([&](const auto& loan){window=loan.retain_owner();});
    if(window.get()!=incoming_window.get()||window.owner_before(incoming_window)||incoming_window.owner_before(window))
        throw std::runtime_error("GUI incoming Window control differs from canonical producer");
    if(producer.expired())throw std::runtime_error("Actual compositor producer retired during Window callback");
    const auto frame=scene->frame_;
    if(!frame||frame->stamp.viewport.framebuffer_width<=0||frame->stamp.viewport.framebuffer_height<=0||
       !scene->owner_.is_current(frame)||!scene->ready_to_draw()||!host->is_current_frame(ui))
        throw std::runtime_error("GUI factory actual selected frame is stale");
    if(producer.expired())throw std::runtime_error("Actual compositor producer retired during frame callback");
    // Retain genuine constructor-owned plans and callback storage BEFORE
    // arbitrary application calls. No kernel may borrow the destructible
    // original compositor after its localizer returns.
    const auto require_factory_identity=[&] {
        if(producer.expired()||scene->gui_thread_!=std::this_thread::get_id()||
           scene.use_count()<=static_cast<long>(scene->gui_publication_pins_.load()+1)||scene->gui_self_.expired()||
           scene.owner_before(scene->gui_self_)||scene->gui_self_.owner_before(scene)||
           scene->frame_.get()!=frame.get()||scene->frame_.owner_before(frame)||frame.owner_before(scene->frame_))
            throw std::runtime_error("GUI factory original producer/outside Scene retired");
        scene->owner_.require_same_gui_ui_host(host);
    };
    require_factory_identity();
    auto storage=std::shared_ptr<UiScreenCompositor>(new UiScreenCompositor(GuiRetainedStorageTag{},*this));
    require_factory_identity();
    auto target=storage->gui_localizer_;
    storage->localize_=*target?OverviewLocalizer{[&require_factory_identity,target](std::string_view key) {
        require_factory_identity();
        auto result=(*target)(key); // Actual retained application callback.
        require_factory_identity(); // Before old kernel reuses source/backend.
        return result;
    }}:OverviewLocalizer{};
    require_factory_identity();
    const auto layout=storage->compose(ui,host->screens(),frame->stamp.viewport.framebuffer_width,
        frame->stamp.viewport.framebuffer_height,owned_production?&*owned_production:nullptr,owned_economic?&*owned_economic:nullptr);
    require_factory_identity();
    storage.reset();target.reset(); // Target/storage destruction precedes publication.
    require_factory_identity();
    auto state=std::make_shared<BnwOwnedCameraGuiScreen::Impl>(scene,host,frame,window,ui,layout);
    state->compositor_lifetime=producer;
    for(const auto& context:host->screens().registry().contexts())
        state->registrations.emplace_back(&context,host->screens().visible(context.id));
    auto result=std::shared_ptr<BnwOwnedCameraGuiScreen>(new BnwOwnedCameraGuiScreen(std::move(state)));
    result->self_=result;
    // The state holds genuine storage. Remove this one FACTORY temporary before
    // the outside-owner check; it cannot stand in for the renderer Scene owner.
    scene.reset();result->require_current();result->impl_->scene->gui_screen_=result;return result;
}
}

namespace civ5::ui {
namespace {
template<class A,class B>bool same_research_control(const A&a,const B&b){
    return !a.owner_before(b)&&!b.owner_before(a);
}
}
void UiScreenCompositor::bind_owned_research_renderer_publication(
    const std::shared_ptr<const BnwOwnedCameraGuiScreen>& incoming,
    const std::weak_ptr<const std::uint8_t>& renderer_lifetime,
    const std::function<void(const std::shared_ptr<const BnwOwnedCameraGuiScreen>&,
                             const LiveUiFrame&,const ComposedUiFrame&)>& borrowed)const {
    if(gui_producer_.thread!=std::this_thread::get_id())throw std::logic_error("Research renderer foreign thread");
    if(!incoming)throw std::invalid_argument("Research renderer GUI absent");
    const auto incoming_control=std::weak_ptr<const BnwOwnedCameraGuiScreen>(incoming);
    BnwOwnedCameraGuiScreen::Pin gui_pin(*incoming);const auto& gui=gui_pin.storage;
    const auto life=std::weak_ptr<const std::uint8_t>(gui_producer_.life);
    const auto renderer=renderer_lifetime;
    if(!same_research_control(incoming_control,gui)||renderer.expired()||
       !same_research_control(life,gui->impl_->compositor_lifetime)||gui->impl_->research||
       gui->impl_->retired.load()||gui->impl_->failed.load()||
       gui->self_.use_count()<=static_cast<long>(gui->pins_))
        throw std::logic_error("Research renderer canonical source/storage differs from actual compositor");
    // Callback-free canonical/storage check precedes borrowed target COPY.
    // The retained plan constructor copies only constructor-owned forwarding storage.
    auto plans=std::shared_ptr<UiScreenCompositor>(new UiScreenCompositor(GuiRetainedStorageTag{},*this));
    // Every original-facade field was snapshotted before copying the target.
    using Target=std::function<void(const std::shared_ptr<const BnwOwnedCameraGuiScreen>&,
                                    const LiveUiFrame&,const ComposedUiFrame&)>;
    auto target=std::make_shared<const Target>(borrowed);
    if(life.expired()||renderer.expired())throw std::logic_error("Research source retired during target copy");
    gui->require_current();
    if(!*target)throw std::invalid_argument("Research renderer publication target absent");
    if(!gui->impl_->host->research_handlers_)gui->impl_->host->register_owned_research_handlers();
    gui->require_current();
    auto source=std::make_shared<BnwOwnedCameraGuiScreen::Impl::ResearchSource>();
    source->renderer_lifetime=renderer;source->plans=std::move(plans);source->publish=std::move(target);
    gui->impl_->origin_renderer=renderer;gui->impl_->research=std::move(source);
    gui->require_current();
}
struct BnwOwnedCameraGuiScreen::ResearchChoiceLoan::Action {
    const BnwOwnedCameraGuiScreen& origin;
    const std::shared_ptr<Impl> storage;
    const std::shared_ptr<Impl::ResearchSource> source;
    const ResearchChoiceLoan* actual=nullptr;
    std::unique_ptr<Pin> current_pin;
    const runtime::BnwResearchChoiceCommit* commit=nullptr;
    std::int64_t ready_team=-1;std::uint64_t before_revision=0;
    unsigned dirty_ordinal=0;
    Action(const BnwOwnedCameraGuiScreen& gui,std::shared_ptr<const BnwOwnedCameraGuiScreen> self)
      :origin(gui),storage(gui.impl_),source(storage->research),current_pin(std::make_unique<Pin>(*self)){}
    void pure()const {
        if(origin.thread_!=std::this_thread::get_id()||storage->research_active!=this||
           !actual||!source||source->failed.load()||source->active!=this||source!=storage->research||!source->plans||!source->publish||
           source->renderer_lifetime.expired()||storage->compositor_lifetime.expired()||
           storage->failed.load()||!storage->scene||!storage->host||!storage->window||
           storage->scene->gui_self_.use_count()<=static_cast<long>(storage->scene->gui_publication_pins_.load()))
            throw std::logic_error("Research action source/renderer/outside Scene retired");
        storage->scene->owner_.require_same_gui_ui_host(storage->host);
    }
    void storage_current()const {
        pure();const auto& gui=*current_pin->storage;
        if(gui.impl_->research!=source||gui.impl_->retired.load()||gui.impl_->failed.load()||
           gui.self_.use_count()<=static_cast<long>(gui.pins_))
            throw std::logic_error("Research current GUI canonical storage retired");
    }
    void check()const {
        storage_current();current_pin->storage->require_current();pure();
        if(commit){commit->require_stored_current(*storage->scene->selected_host_);pure();}
    }
    std::shared_ptr<const BnwOwnedCameraGuiScreen> refresh(
        std::optional<runtime::BnwResearchChoiceDirty> dirty=std::nullopt) {
        pure();commit->require_stored_current(*storage->scene->selected_host_);pure();
        const auto scene_frame=current_pin->storage->impl_->frame;
        if(!scene_frame)throw std::logic_error("Research pre-refresh actual frame storage missing");
        storage->window->require_current();pure();
        storage->scene->invalidate();pure();
        const auto captured=storage->scene->begin(scene_frame->stamp.viewport,scene_frame->camera);pure();
        if(!captured||captured->stamp.identity.team!=ready_team||
           captured->stamp.identity.ui_source_revision!=commit->after_revision())
            throw std::logic_error("Research Scene capture is not the actual committed source");
        const auto ui=storage->host->begin_frame(captured->stamp.viewport.framebuffer_width,
                                                captured->stamp.viewport.framebuffer_height);pure();
        storage->host->require_owned_research_handlers(ui,ready_team);pure();
        if(dirty){storage->host->consume_owned_research_dirty(*dirty,ui,*commit);pure();}
        // Own all plans and the canonical localizer target before application calls.
        auto plans=std::shared_ptr<UiScreenCompositor>(new UiScreenCompositor(
            UiScreenCompositor::GuiRetainedStorageTag{},*source->plans));
        const auto localizer=plans->gui_localizer_;
        // The factory invokes this canonical target, then wraps its own source
        // checks around it. Keep the actual Action checks on that invoked target.
        plans->gui_localizer_=std::make_shared<const OverviewLocalizer>(
            *localizer?OverviewLocalizer{[this,localizer](std::string_view key){
                pure();auto text=(*localizer)(key);pure();return text;
            }}:OverviewLocalizer{});
        const auto next=plans->compose_owned_current_screen(storage->scene,storage->host,storage->window,ui);pure();
        plans.reset();pure();
        next->impl_->compositor_lifetime=storage->compositor_lifetime;
        next->impl_->origin_renderer=source->renderer_lifetime;next->impl_->research=source;
        next->require_same_research_choice(*commit);pure();
        auto previous=std::move(current_pin);current_pin=std::make_unique<Pin>(*next);
        // Actual renderer slots are published before the source hands off NEWGui.
        (*source->publish)(next,next->impl_->ui,next->impl_->layout);pure();
        next->require_same_research_choice(*commit);pure();
        previous->storage->retire();return next;
    }
};
void BnwOwnedCameraGuiScreen::ResearchChoiceLoan::require_current()const {
    if(action_.origin.thread_!=std::this_thread::get_id())throw std::logic_error("Research loan foreign thread");
    if(action_.actual!=this)throw std::logic_error("Research loan address differs");action_.check();
}
void BnwOwnedCameraGuiScreen::ResearchChoiceLoan::require_ready_for_team(std::int64_t team)const {
    require_current();
    if(action_.commit||action_.ready_team!=-1)throw std::logic_error("Research preflight already consumed");
    const auto& p=*action_.storage;
    p.host->require_owned_research_handlers(p.ui,team);action_.pure();
    if(p.frame->stamp.identity.team!=team||!p.scene->selected_host_)
        throw std::logic_error("Research handler active team differs");
    action_.ready_team=team;action_.before_revision=p.frame->stamp.identity.ui_source_revision;
    require_current();
}
std::shared_ptr<const BnwOwnedCameraGuiScreen>
BnwOwnedCameraGuiScreen::ResearchChoiceLoan::accept_commit(const runtime::BnwResearchChoiceCommit& commit)const {
    // Stores already happened: old frame is lineage storage, never currentness.
    action_.pure();
    if(action_.actual!=this||action_.commit||action_.ready_team<0)
        throw std::logic_error("Research accept has no actual unconsumed preflight loan");
    commit.require_stored_current(*action_.storage->scene->selected_host_);action_.pure();
    if(commit.team()!=action_.ready_team||commit.before_revision()!=action_.before_revision||
       commit.after_revision()<=commit.before_revision())throw std::logic_error("Research commit lineage differs");
    action_.commit=&commit;const auto result=action_.refresh();require_current();return result;
}
std::shared_ptr<const BnwOwnedCameraGuiScreen>
BnwOwnedCameraGuiScreen::ResearchChoiceLoan::deliver_dirty(runtime::BnwResearchChoiceDirty kind,std::int64_t team)const {
    require_current();
    const std::array<runtime::BnwResearchChoiceDirty,3> order{
        runtime::BnwResearchChoiceDirty::research_buttons,runtime::BnwResearchChoiceDirty::game_data,
        runtime::BnwResearchChoiceDirty::score};
    if(!action_.commit||team!=action_.ready_team||
       order[action_.dirty_ordinal%order.size()]!=kind||action_.dirty_ordinal==std::numeric_limits<unsigned>::max())
        throw std::logic_error("Research dirty order/team differs");
    const auto next=action_.refresh(kind);
    ++action_.dirty_ordinal;require_current();return next;
}
void BnwOwnedCameraGuiScreen::with_actual_research_choice_loan(
    const std::function<void(const ResearchChoiceLoan&)>& borrowed)const {
    Pin pin(*this);const auto p=impl_;
    if(p->active||p->research_active||(p->research&&p->research->active)){
        p->failed.store(true);throw std::logic_error("Research GUI loan reentry");}
    ResearchChoiceLoan::Action action(*this,pin.storage);ResearchChoiceLoan loan(action);
    action.actual=&loan;p->research_active=&action;
    if(action.source)action.source->active=&action;
    struct End{Impl& p;ResearchChoiceLoan::Action& a;~End(){
        if(a.source&&a.source->active==&a)a.source->active=nullptr;
        a.actual=nullptr;p.research_active=nullptr;p.release_retired_storage();
    }}end{*p,action};
    std::optional<std::function<void(const ResearchChoiceLoan&)>> target;
    try{
        action.storage_current();target.emplace(borrowed);action.check();
        if(!*target)throw std::invalid_argument("Research action consumer absent");
        (*target)(loan);action.check();target.reset();action.check();
        if(action.commit&&(!action.dirty_ordinal||action.dirty_ordinal%3||
           action.commit->completed_dirty_calls()!=action.dirty_ordinal))
            throw std::logic_error("Research actual repeated dirty tail did not return normally");
    }catch(...){p->failed.store(true);if(action.source)action.source->failed.store(true);target.reset();throw;}
}
void BnwOwnedCameraGuiScreen::require_same_research_choice(const runtime::BnwResearchChoiceCommit& commit)const {
    Pin pin(*this);check();const auto p=impl_;
    commit.require_stored_current(*p->scene->selected_host_);check();
    const auto* projection=p->ui.snapshot.find<BnwResearchChoiceUiSnapshot>();
    if(!projection||projection->player!=commit.player()||projection->team!=commit.team()||
       projection->revision!=commit.after_revision()||projection->research_queue!=commit.queue())
        throw std::logic_error("GUI does not contain actual committed research snapshot");check();
}
void BnwOwnedCameraGuiScreen::require_same_research_refresh_origin(
    const BnwOwnedCameraGuiScreen& old,const runtime::BnwResearchChoiceCommit& commit)const {
    if(thread_!=std::this_thread::get_id()||old.thread_!=thread_)
        throw std::logic_error("Research refresh origin foreign thread");
    Pin pin(*this);Pin old_pin(old);const auto previous=old.impl_;const auto p=impl_;
    // Immutable factory/control identity remains after old publication retirement.
    if(!same_research_control(p->origin_scene,previous->origin_scene)||
       !same_research_control(p->origin_host,previous->origin_host)||
       !same_research_control(p->origin_window,previous->origin_window)||
       !same_research_control(p->compositor_lifetime,previous->compositor_lifetime)||
       !same_research_control(p->origin_renderer,previous->origin_renderer)||
       p->origin_renderer.expired()||p->origin_frame.frame<=previous->origin_frame.frame||
       p->origin_frame.revision<=previous->origin_frame.revision)
        throw std::logic_error("Research final GUI is not newer SAME renderer/Scene publication");
    require_same_research_choice(commit);check();
}
}

namespace civ5::ui {
void BnwOwnedCameraGuiScreen::require_train_presented_origin(const BnwTrainRendererRefreshReceipt& receipt)const {
    if(thread_!=std::this_thread::get_id()||receipt.creator_!=thread_)
        throw std::runtime_error("TRAIN presented origin foreign creator");
    Pin pin(*this);const auto& origin=receipt.origin_;
    if(!origin.opening_gui)throw std::runtime_error("TRAIN opening GUI absent");
    Pin old_pin(*origin.opening_gui);const auto old=origin.opening_gui->impl_;const auto p=impl_;
    // Only immutable original provenance is read from the retired opening leaf.
    if(!p||!old||!p->scene||!p->frame||!p->host||!p->window||
       p->scene.get()!=origin.scene.get()||!same_research_control(p->scene,origin.scene)||
       !same_research_control(p->origin_scene,old->origin_scene)||
       !same_research_control(p->origin_host,old->origin_host)||
       !same_research_control(p->origin_window,old->origin_window)||
       !same_research_control(p->compositor_lifetime,old->compositor_lifetime)||
       !same_research_control(p->origin_renderer,old->origin_renderer)||p->origin_renderer.expired()||
       p->scene->selected_host_.get()!=origin.host.get()||!same_research_control(p->scene->selected_host_,origin.host)||
       p->scene->scene_state_!=origin.state.get()||p->scene->scene_state_generation_!=origin.generation||
       p->frame->stamp.identity.ui_source_revision!=origin.earned_revision||
       p->origin_frame.ui_source_revision!=origin.earned_revision||
       p->origin_frame.owner!=old->origin_frame.owner||
       p->origin_frame.frame<=old->origin_frame.frame||p->origin_frame.revision<=old->origin_frame.revision||
       !same_research_control(p->scene->gui_screen_,self_))
        throw std::runtime_error("TRAIN presented GUI is not actual newer SAME source publication");
    {
        const auto associated=p->scene->gui_screen_.lock();
        if(!associated||associated.get()!=this||!same_research_control(associated,self_))
            throw std::runtime_error("TRAIN presented current GUI association replaced");
    }
    check(); // Full live NEW-frame/UI/Window checks; never old.check/current.
}
}

namespace civ5::ui {
std::weak_ptr<BnwClientSceneFrame> BnwOwnedCameraGuiScreen::context_gui_scene_origin_without_callbacks(const Loan& loan)const {
 require_origin_lifetime_without_frame_callbacks();const auto result=impl_->origin_scene;
 require_worldview_scene_origin_without_callbacks(loan,result);
 require_origin_lifetime_without_frame_callbacks();return result;
}
void BnwOwnedCameraGuiScreen::require_worldview_scene_origin_without_callbacks(const Loan& loan,const std::weak_ptr<BnwClientSceneFrame>& scene)const {
 if(thread_!=std::this_thread::get_id())throw std::runtime_error("WorldView projection Scene foreign creator");
 require_origin_lifetime_without_frame_callbacks();const auto p=impl_;
 {
 const auto expected=scene.lock();
 const auto original=p->origin_scene.lock();
 if(!expected||!original||expected.get()!=p->scene.get()||original.get()!=p->scene.get()||
  expected.owner_before(p->scene)||p->scene.owner_before(expected)||original.owner_before(p->scene)||p->scene.owner_before(original))
  throw std::runtime_error("WorldView projection Scene alias/canonical get changed");
 }
 // Both pure checker locks retire before the final borrowed-origin check.
 if(p->active!=&loan||&loan.owner_!=this||!p->scene||scene.expired()||scene.owner_before(p->scene)||p->scene.owner_before(scene)||
  p->origin_scene.owner_before(p->scene)||p->scene.owner_before(p->origin_scene))
  throw std::runtime_error("WorldView projection Scene is not actual canonical GUI member");
 require_origin_lifetime_without_frame_callbacks();
}
}

namespace civ5::ui {
std::shared_ptr<LiveUiHost> BnwOwnedCameraGuiScreen::retain_policy_action_ui()const {
 std::shared_ptr<LiveUiHost> result;
 with_actual_screen_loan([&](const Loan& loan){
  loan.require_current();const auto p=impl_;
  // A real PlotYield Context retains its strict specialized owner checks.
  // Without that Context, the current GUI producer already owns the canonical
  // registered UI; retain that shared member, never adopt a raw host pointer.
  if(p->scene->owned_plot_yield_ui_sources_)
   result=p->scene->pin_owned_host(*p->host);
  else
   result=p->host;
  loan.require_current();
  if(!result||result.get()!=p->host.get()||result.owner_before(p->host)||p->host.owner_before(result))
   throw std::runtime_error("Policy UI is not actual current compositor/Scene host");
  loan.require_current();
 });
 return result;
}
}

namespace civ5::ui {
void BnwOwnedCameraGuiScreen::require_civis_color_source_origin_without_callbacks(
 const std::weak_ptr<const SelectedBnwPreviewHost>& expected_host,
 const assets::CivisResourceBankOwner& bank)const {
 // This helper never enters check()/GUI frame providers or native observer
 // callbacks. Creator and canonical GUI/source lifetime precede weak reads.
 if(thread_!=std::this_thread::get_id())throw std::runtime_error("Own color GUI source foreign creator");
 require_origin_lifetime_without_frame_callbacks();const auto p=impl_;
 {
  const auto expected=expected_host.lock();const auto original_scene=p->origin_scene.lock();
  if(!p->scene||!original_scene||original_scene.get()!=p->scene.get()||
     original_scene.owner_before(p->scene)||p->scene.owner_before(original_scene)||
     !expected||!p->scene->selected_host_||p->scene->selected_host_.get()!=expected.get()||
     p->scene->selected_host_.owner_before(expected_host)||expected_host.owner_before(p->scene->selected_host_))
   throw std::runtime_error("Own color GUI source does not belong to canonical selected Host/Scene");
  // Host6 performs only creator/canonical own-bank metadata and immutable
  // cloned-art/rules checks; no Full boundary or observer enters this scope.
  expected->require_same_civis_resources(bank);
 }
 // Checker Host/Scene pins are gone before the final pure lifetime fence.
 require_origin_lifetime_without_frame_callbacks();
 if(expected_host.expired()||!p->scene||!p->scene->selected_host_||
    p->scene->selected_host_.owner_before(expected_host)||expected_host.owner_before(p->scene->selected_host_))
  throw std::runtime_error("Own color GUI selected Host retired after pure origin check");
}
}
