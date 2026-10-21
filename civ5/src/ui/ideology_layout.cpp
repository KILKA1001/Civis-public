#include "civ5/ui/ideology_layout.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <map>
#include <set>

namespace civ5::ui {
namespace {

const content::XmlNode *child(const content::XmlNode &parent,
                              std::string_view name, std::string_view id = {}) {
    const content::XmlNode *result = nullptr;
    for (const auto &node : parent.children)
        if (node.name == name && (id.empty() || node.attribute("ID") == id)) {
            if (result) return nullptr;
            result = &node;
        }
    return result;
}

const content::XmlNode *instance(const content::XmlNode &parent,
                                 std::string_view name) {
    const content::XmlNode *result = nullptr;
    for (const auto &node : parent.children)
        if (node.name == "Instance" && node.attribute("Name") == name) {
            if (result) return nullptr;
            result = &node;
        }
    return result;
}

std::optional<std::pair<std::int32_t, std::int32_t>> pair_attr(
    const content::XmlNode &node, std::string_view key) {
    const auto value = node.attribute(std::string(key));
    const auto comma = value.find(',');
    if (comma == std::string::npos) return std::nullopt;
    std::int32_t first = 0, second = 0;
    const auto parse = [](std::string_view text, std::int32_t &out) {
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), out);
        return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
    };
    return parse(std::string_view(value).substr(0, comma), first) &&
           parse(std::string_view(value).substr(comma + 1), second)
        ? std::optional(std::pair{first, second}) : std::nullopt;
}

std::optional<WorldViewRect> intersect(WorldViewRect rect, WorldViewRect clip) {
    const auto left = std::max<std::int64_t>(rect.x, clip.x);
    const auto top = std::max<std::int64_t>(rect.y, clip.y);
    const auto right = std::min<std::int64_t>(std::int64_t(rect.x) + rect.width,
                                               std::int64_t(clip.x) + clip.width);
    const auto bottom = std::min<std::int64_t>(std::int64_t(rect.y) + rect.height,
                                                std::int64_t(clip.y) + clip.height);
    if (right <= left || bottom <= top) return std::nullopt;
    return WorldViewRect{static_cast<std::int32_t>(left), static_cast<std::int32_t>(top),
                         static_cast<std::int32_t>(right - left),
                         static_cast<std::int32_t>(bottom - top)};
}

void rectangle(IdeologyLayoutPlan &plan, WorldViewRect rect, std::string style) {
    plan.paints.push_back({WorldViewPaintKind::rectangle, rect, {}, {}, {},
                           std::move(style)});
}

bool overlay_controls_authored(const content::XmlNode &root, bool tenets) {
    const auto *box = child(root, "Box", tenets ? "ViewTenetsPopup" : "ChooseConfirm");
    const auto *grid = box ? child(*box, "Grid") : nullptr;
    const auto size = grid ? pair_attr(*grid, "Size") : std::nullopt;
    if (!size || size->first <= 0 || size->second <= 0 ||
        grid->attribute("Anchor") != "C,C" || grid->attribute("Offset") != "0,0")
        return false;
    if (tenets) {
        const auto *scroll = child(*grid, "ScrollPanel", "ViewTenetsScrollPanel");
        const auto *close = child(*grid, "GridButton", "ViewTenetsCloseButton");
        return scroll && close && pair_attr(*scroll, "Size") &&
            pair_attr(*scroll, "Offset") && pair_attr(*close, "Size") &&
            pair_attr(*close, "Offset") && scroll->attribute("Vertical") == "1" &&
            close->attribute("Anchor") == "C,B";
    }
    const auto *buttons = child(*grid, "Stack", "ButtonStack");
    const auto *yes = buttons ? child(*buttons, "GridButton", "ConfirmYes") : nullptr;
    const auto *no = buttons ? child(*buttons, "GridButton", "ConfirmNo") : nullptr;
    return buttons && yes && no && pair_attr(*buttons, "Offset") &&
        pair_attr(*yes, "Size") && pair_attr(*no, "Size") &&
        buttons->attribute("Anchor") == "C,B" &&
        buttons->attribute("StackGrowth") == "Bottom" &&
        buttons->attribute("Padding") == "24" &&
        yes->attribute("Anchor") == "C,T" && no->attribute("Anchor") == "C,T";
}

} // namespace

IdeologyLayoutPlan build_ideology_layout(const ProgressionScreenSource &source,
    const UiFrameSnapshot &frame, std::int32_t width, std::int32_t height,
    const LiveUiFrame::IdeologyView *view,
    const IdeologyHeaderLocalizer &localize) {
    IdeologyLayoutPlan result;
    result.selected_source_path = source.selected_source_path;
    if (width <= 0 || height <= 0 || width > 8192 || height > 8192) {
        result.unresolved.push_back("Invalid ideology viewport");
        return result;
    }
    constexpr std::string_view selected_key =
        "Assets/DLC/Expansion2/UI/InGame/Popups/ChooseIdeologyPopup.xml";
    if (source.requested_key != selected_key ||
        source.selected_source_path.find("Expansion2") == std::string::npos ||
        source.document.name != "Context" ||
        source.document.attribute("ID") != "ChooseIdeologyPopup") {
        result.unresolved.push_back("Selected BNW ideology XML is unavailable");
        return result;
    }
    const auto *panel = child(source.document, "Grid", "BottomPanel");
    const auto *item = instance(source.document, "ItemInstance");
    const auto *civ = instance(source.document, "CivInstance");
    const auto *box = panel ? child(*panel, "Box") : nullptr;
    const auto *scroll = box ? child(*box, "ScrollPanel", "ItemScrollPanel") : nullptr;
    const auto *stack = scroll ? child(*scroll, "Stack", "ItemStack") : nullptr;
    const auto *close = panel ? child(*panel, "GridButton", "CloseButton") : nullptr;
    const auto *button = item ? child(*item, "Button", "Button") : nullptr;
    const auto *free_label = button ? child(*button, "Label", "FreePolicies") : nullptr;
    const auto *adopted = button ? child(*button, "Stack", "AdoptedByStacks") : nullptr;
    const auto *adopted_first = adopted ? child(*adopted, "Stack", "AdoptedByStack1") : nullptr;
    const auto *adopted_second = adopted ? child(*adopted, "Stack", "AdoptedByStack2") : nullptr;
    const auto *civ_icon = civ ? child(*civ, "Image", "CivIconFrame") : nullptr;
    const auto panel_size = panel ? pair_attr(*panel, "Size") : std::nullopt;
    const auto panel_offset = panel ? pair_attr(*panel, "Offset") : std::nullopt;
    const auto box_offset = box ? pair_attr(*box, "Offset") : std::nullopt;
    const auto scroll_size = scroll ? pair_attr(*scroll, "Size") : std::nullopt;
    const auto scroll_offset = scroll ? pair_attr(*scroll, "Offset") : std::nullopt;
    const auto stack_offset = stack ? pair_attr(*stack, "Offset") : std::nullopt;
    const auto button_size = button ? pair_attr(*button, "Size") : std::nullopt;
    const auto free_label_offset = free_label ? pair_attr(*free_label, "Offset")
                                               : std::nullopt;
    std::int32_t free_label_wrap = 0;
    const auto free_wrap_text = free_label ? free_label->attribute("WrapWidth") : "";
    const auto free_wrap_parse = std::from_chars(free_wrap_text.data(),
        free_wrap_text.data() + free_wrap_text.size(), free_label_wrap);
    const auto adopted_offset = adopted ? pair_attr(*adopted, "Offset") : std::nullopt;
    const auto civ_size = civ_icon ? pair_attr(*civ_icon, "Size") : std::nullopt;
    const auto close_size = close ? pair_attr(*close, "Size") : std::nullopt;
    const auto close_offset = close ? pair_attr(*close, "Offset") : std::nullopt;
    if (!panel || !box || !scroll || !stack || !close || !button || !free_label ||
        !free_label_offset || free_label_wrap <= 0 ||
        free_wrap_parse.ec != std::errc{} ||
        free_wrap_parse.ptr != free_wrap_text.data() + free_wrap_text.size() ||
        free_label->attribute("Anchor") != "L,T" ||
        free_label->attribute("Font") != "TwCenMT16" || !adopted ||
        !adopted_first || !adopted_second || !civ_icon ||
        !panel_size || !panel_offset || !box_offset || !scroll_size ||
        !scroll_offset || !stack_offset || !button_size || !adopted_offset || !civ_size ||
        !close_size || !close_offset ||
        panel->attribute("Anchor") != "L,T" ||
        scroll->attribute("Vertical") != "1" ||
        stack->attribute("StackGrowth") != "Bottom" ||
        adopted->attribute("StackGrowth") != "Bottom" ||
        *adopted_offset != std::pair{10, 165} ||
        close->attribute("Anchor") != "C,B" ||
        *civ_size != std::pair{32, 32} ||
        button_size->first <= 0 || scroll_size->first <= 0 ||
        panel_size->first <= 0 || close_size->first <= 0 || close_size->second <= 0) {
        result.unresolved.push_back("Selected BNW ideology controls or anchors are incomplete");
        return result;
    }
    // ChooseIdeologyPopup.lua resizes both controls by screenY - 1020.
    const auto height_offset = height - 1020;
    const auto panel_height = panel_size->second + height_offset;
    const auto scroll_height = scroll_size->second + height_offset;
    if (panel_height <= 0 || scroll_height <= 0 ||
        button_size->first > scroll_size->first) {
        result.unresolved.push_back("Selected BNW ideology panel does not fit the viewport");
        return result;
    }
    const WorldViewRect screen{0, 0, width, height};
    const WorldViewRect raw_panel{panel_offset->first, panel_offset->second,
                                  panel_size->first, panel_height};
    const WorldViewRect raw_scroll{raw_panel.x + box_offset->first + scroll_offset->first,
        raw_panel.y + box_offset->second + scroll_offset->second,
        scroll_size->first, scroll_height};
    const WorldViewRect raw_close{raw_panel.x + (raw_panel.width - close_size->first) / 2 +
                                      close_offset->first,
        raw_panel.y + raw_panel.height - close_offset->second - close_size->second,
        close_size->first, close_size->second};
    result.panel = intersect(raw_panel, screen);
    result.scroll_panel = intersect(raw_scroll, screen);
    result.close_button = intersect(raw_close, screen);
    if (!result.panel || !result.scroll_panel || !result.close_button ||
        result.close_button->width != raw_close.width ||
        result.close_button->height != raw_close.height) {
        result.panel.reset();
        result.scroll_panel.reset();
        result.close_button.reset();
        result.unresolved.push_back("Selected BNW ideology controls do not fit the viewport");
        return result;
    }
    rectangle(result, *result.panel, panel->attribute("Style"));
    rectangle(result, *result.close_button, close->attribute("Style"));
    result.hits.push_back({*result.close_button, "ChooseIdeology",
                           UiCloseScreen{"ChooseIdeology"}, true, {}});

    const auto *event = frame.find<ProgressionIdeologyPopupEvent>();
    const auto *state = progression_for_active_player(frame);
    if (!event || !state || !frame.active_player_id() || !frame.turn() ||
        event->player_id != *frame.active_player_id() || event->token == 0) {
        result.unresolved.push_back("No active BNW ideology choice event");
        return result;
    }
    const auto *evidence = frame.find<IdeologyLayoutEvidence>();
    if (!evidence || evidence->player_id != event->player_id ||
        evidence->event_token != event->token || evidence->turn != *frame.turn() ||
        evidence->frame != frame.frame() ||
        evidence->selected_source_path != source.selected_source_path ||
        evidence->branches.size() != 3) {
        result.unresolved.push_back("Current ideology adopter counts are unavailable");
        return result;
    }
    std::map<std::int64_t, std::uint32_t> counts;
    for (const auto &entry : evidence->branches)
        if (entry.branch_id < 0 || !counts.emplace(entry.branch_id,
                                                   entry.alive_adopters).second) {
            result.unresolved.push_back("Ideology adopter IDs are ambiguous");
            return result;
        }
    constexpr std::array<std::string_view, 3> lua_order{
        "POLICY_BRANCH_AUTOCRACY", "POLICY_BRANCH_FREEDOM", "POLICY_BRANCH_ORDER"};
    std::array<const ProgressionBranch *, 3> branches{};
    for (std::size_t index = 0; index < lua_order.size(); ++index) {
        for (const auto &branch : state->branches)
            if (branch.type == lua_order[index] && branch.ideology) {
                if (branches[index]) {
                    result.unresolved.push_back("Selected ideology branch ID is ambiguous");
                    return result;
                }
                branches[index] = &branch;
            }
        if (!branches[index] || !counts.contains(branches[index]->id)) {
            result.unresolved.push_back("Selected ideology branch or adopter count is missing");
            return result;
        }
    }
    std::map<std::int64_t, std::uint32_t> free_counts;
    bool free_counts_valid = evidence->free_policy_counts &&
        evidence->free_policy_counts->size() == branches.size();
    if (free_counts_valid) {
        for (const auto &entry : *evidence->free_policy_counts)
            if (entry.branch_id < 0 || !free_counts.emplace(entry.branch_id,
                    entry.free_policies).second)
                free_counts_valid = false;
        for (const auto *branch : branches)
            if (!free_counts.contains(branch->id)) free_counts_valid = false;
    }
    constexpr std::string_view free_key =
        "TXT_KEY_CHOOSE_IDEOLOGY_NUM_FREE_POLICIES";
    constexpr std::string_view free_token = "{1_NumFreePolicies}";
    const auto free_template = free_counts_valid && localize
        ? localize(free_key) : std::nullopt;
    const auto token_position = free_template
        ? free_template->find(free_token) : std::string::npos;
    const bool free_template_valid = free_template &&
        token_position != std::string::npos &&
        free_template->find(free_token, token_position + free_token.size()) ==
            std::string::npos;
    if (!free_counts_valid)
        result.unresolved.push_back("Current Game.GetNumFreePolicies ideology values are unavailable");
    else if (!free_template_valid)
        result.unresolved.push_back("Selected ideology free-policy localization is unavailable");
    const bool current_view = view && view->player_id == event->player_id &&
        view->turn == *frame.turn() && view->event_token == event->token &&
        view->selected_source_path == source.selected_source_path &&
        view->viewport_width == width && view->viewport_height == height &&
        view->list_offset == 0 && view->tenet_offset == 0;
    if (view && !current_view)
        result.unresolved.push_back("Ideology overlay identity or native scroll offset is stale");
    const auto *show_tenets = button ? child(*button, "Button", "ShowTenets") : nullptr;
    const auto show_size = show_tenets ? pair_attr(*show_tenets, "Size") : std::nullopt;
    const auto show_offset = show_tenets ? pair_attr(*show_tenets, "Offset") : std::nullopt;
    const bool authored_show = show_tenets && show_size && show_offset &&
        show_tenets->attribute("Anchor") == "R,T" &&
        show_size->first > 0 && show_size->second > 0;
    const bool confirmation_ready = overlay_controls_authored(source.document, false);
    const bool tenets_ready = overlay_controls_authored(source.document, true);
    // The Lua ItemInstance height is 170 + AdoptedByStacks height. CivInstance
    // icons are 32 px; the first 12 form one row, the next 12 a second row.
    std::int32_t y = raw_scroll.y + stack_offset->second;
    const auto x = raw_scroll.x + stack_offset->first;
    for (const auto *branch : branches) {
        const auto count = counts.at(branch->id);
        const auto icon_rows = count == 0 ? 0 : count <= 12 ? 1 : 2;
        const auto row_height = 170 + 32 * icon_rows;
        const WorldViewRect row{x, y, button_size->first, row_height};
        if (const auto visible = intersect(row, *result.scroll_panel)) {
            rectangle(result, *visible, "IdeologyItem");
            if (branch->description && !branch->description->empty())
                result.paints.push_back({WorldViewPaintKind::text,
                    {visible->x + 10, visible->y + 7,
                     std::max(0, visible->width - 20), std::min(28, visible->height)},
                    {}, {}, *branch->description, "TwCenMT24"});
            else result.unresolved.push_back("Selected ideology name is not localized: " +
                                             branch->type);
            if (free_template_valid) {
                auto label = *free_template;
                const auto value = free_counts.at(branch->id);
                label.replace(token_position, free_token.size(), std::to_string(value));
                if (value > 0)
                    label = "[COLOR_POSITIVE_TEXT]" + label + "[ENDCOLOR]";
                const WorldViewRect raw_label{row.x + free_label_offset->first,
                    row.y + free_label_offset->second, free_label_wrap, 28};
                if (const auto clipped = intersect(raw_label, *result.scroll_panel))
                    result.paints.push_back({WorldViewPaintKind::text, *clipped,
                        {}, {}, std::move(label), free_label->attribute("Font")});
            }
            if (current_view && view->overlay == LiveUiFrame::IdeologyOverlay::none &&
                confirmation_ready)
                result.hits.push_back({*visible, "ChooseIdeology",
                    UiGameplayCommand::make("ChooseIdeology", IdeologyUiAction{
                        IdeologyUiActionKind::select_branch, branch->id, event->token}),
                    true, {}});
            else result.hits.push_back({*visible, "ChooseIdeology",
                progression_choose_ideology(branch->id), false,
                "BNW ideology branch effects and notifications are unavailable"});
            if (current_view && view->overlay == LiveUiFrame::IdeologyOverlay::none &&
                authored_show && tenets_ready) {
                const WorldViewRect raw_show{row.x + row.width - show_offset->first -
                    show_size->first, row.y + show_offset->second,
                    show_size->first, show_size->second};
                if (const auto clipped = intersect(raw_show, *result.scroll_panel)) {
                    rectangle(result, *clipped, show_tenets->attribute("Texture"));
                    const auto tooltip_key = show_tenets->attribute("ToolTip");
                    if (!tooltip_key.empty())
                        result.tooltip_keys.push_back({*clipped, tooltip_key});
                    result.hits.push_back({*clipped, "ChooseIdeology",
                        UiGameplayCommand::make("ChooseIdeology", IdeologyUiAction{
                            IdeologyUiActionKind::view_tenets, branch->id, event->token}),
                        true, {}});
                }
            }
        }
        y += row_height;
    }
    if (current_view && view->overlay == LiveUiFrame::IdeologyOverlay::none)
        result.scroll_region = IdeologyScrollRegion{*result.scroll_panel,
            view->player_id, view->turn, width, height, event->token,
            source.selected_source_path, view->overlay, 0,
            std::max(0, y - (raw_scroll.y + raw_scroll.height))};
    if (y > raw_scroll.y + raw_scroll.height)
        result.unresolved.push_back("Native ideology ScrollPanel wheel step is unverified");

    if (current_view && view->overlay != LiveUiFrame::IdeologyOverlay::none) {
        const bool known_branch = std::any_of(branches.begin(), branches.end(),
            [&](const auto *branch) { return branch->id == view->branch_id; });
        result.scroll_region.reset();
        result.hits.push_back({screen, "ChooseIdeology",
            progression_choose_ideology(view->branch_id), false,
            "Ideology overlay owns modal input"});
        if (!known_branch) {
            result.unresolved.push_back("Ideology overlay branch is not in current owner evidence");
            return result;
        }
        const bool tenets = view->overlay == LiveUiFrame::IdeologyOverlay::tenets;
        const auto *overlay = child(source.document, "Box",
            tenets ? "ViewTenetsPopup" : "ChooseConfirm");
        const auto *grid = overlay ? child(*overlay, "Grid") : nullptr;
        const auto grid_size = grid ? pair_attr(*grid, "Size") : std::nullopt;
        if (!overlay || !grid || !grid_size || grid_size->first <= 0 ||
            grid_size->second <= 0 || grid->attribute("Anchor") != "C,C" ||
            grid->attribute("Offset") != "0,0" ||
            (tenets && overlay->attribute("ConsumeMouseButton") != "1") ||
            (!tenets && overlay->attribute("ConsumeMouse") != "1")) {
            result.unresolved.push_back("Selected ideology overlay geometry is incomplete");
            return result;
        }
        const WorldViewRect raw_grid{(width - grid_size->first) / 2,
            (height - grid_size->second) / 2,
            grid_size->first, grid_size->second};
        const auto visible_grid = intersect(raw_grid, screen);
        if (!visible_grid || visible_grid->width != raw_grid.width ||
            visible_grid->height != raw_grid.height) {
            result.unresolved.push_back("Selected ideology overlay does not fit viewport");
            return result;
        }
        rectangle(result, screen, overlay->attribute("Color"));
        rectangle(result, *visible_grid, grid->attribute("Style"));
        if (tenets) {
            const auto *scroll_tenets = child(*grid, "ScrollPanel", "ViewTenetsScrollPanel");
            const auto *close_tenets = child(*grid, "GridButton", "ViewTenetsCloseButton");
            const auto scroll_size_tenets = scroll_tenets
                ? pair_attr(*scroll_tenets, "Size") : std::nullopt;
            const auto scroll_offset_tenets = scroll_tenets
                ? pair_attr(*scroll_tenets, "Offset") : std::nullopt;
            const auto close_size_tenets = close_tenets
                ? pair_attr(*close_tenets, "Size") : std::nullopt;
            const auto close_offset_tenets = close_tenets
                ? pair_attr(*close_tenets, "Offset") : std::nullopt;
            if (!scroll_size_tenets || !scroll_offset_tenets || !close_size_tenets ||
                !close_offset_tenets || scroll_tenets->attribute("Vertical") != "1" ||
                close_tenets->attribute("Anchor") != "C,B") {
                result.unresolved.push_back("Selected tenet controls are incomplete");
                return result;
            }
            const WorldViewRect raw_tenet_scroll{raw_grid.x + scroll_offset_tenets->first,
                raw_grid.y + scroll_offset_tenets->second,
                scroll_size_tenets->first, scroll_size_tenets->second};
            const WorldViewRect raw_tenet_close{raw_grid.x +
                (raw_grid.width - close_size_tenets->first) / 2 + close_offset_tenets->first,
                raw_grid.y + raw_grid.height - close_offset_tenets->second -
                    close_size_tenets->second,
                close_size_tenets->first, close_size_tenets->second};
            const auto scroll_rect = intersect(raw_tenet_scroll, *visible_grid);
            const auto close_rect = intersect(raw_tenet_close, *visible_grid);
            if (!scroll_rect || !close_rect ||
                scroll_rect->width != raw_tenet_scroll.width ||
                scroll_rect->height != raw_tenet_scroll.height ||
                close_rect->width != raw_tenet_close.width ||
                close_rect->height != raw_tenet_close.height) {
                result.unresolved.push_back("Selected tenet controls do not fit overlay");
                return result;
            }
            rectangle(result, *scroll_rect, "ViewTenetsScrollPanel");
            rectangle(result, *close_rect, close_tenets->attribute("Style"));
            result.hits.push_back({*close_rect, "ChooseIdeology",
                UiGameplayCommand::make("ChooseIdeology", IdeologyUiAction{
                    IdeologyUiActionKind::close_overlay, view->branch_id, event->token}),
                true, {}});
            std::int32_t content_height = 0;
            const auto body_paint_start = result.paints.size();
            const auto body_hit_start = result.hits.size();
            const auto header_start = result.tenet_headers.size();
            if (evidence->tenet_help) {
                const auto *template_instance = instance(source.document,
                    "ViewTenetsInstance");
                const auto *template_button = template_instance
                    ? child(*template_instance, "Button", "Button") : nullptr;
                const auto *template_box = template_button
                    ? child(*template_button, "Box", "Box") : nullptr;
                const auto *template_label = template_button
                    ? child(*template_button, "Label", "TenetDescription") : nullptr;
                const auto *content_stack = child(*scroll_tenets, "Stack", "ViewTenetsStack");
                const auto button_size_tenet = template_button
                    ? pair_attr(*template_button, "Size") : std::nullopt;
                const auto box_size_tenet = template_box
                    ? pair_attr(*template_box, "Size") : std::nullopt;
                if (!template_label || !content_stack || !button_size_tenet ||
                    !box_size_tenet || button_size_tenet->first != 542 ||
                    box_size_tenet->first != 542 ||
                    template_label->attribute("Font") != "TwCenMT18" ||
                    template_label->attribute("WrapWidth") != "532" ||
                    content_stack->attribute("StackGrowth") != "Bottom") {
                    result.unresolved.push_back("Selected tenet row controls are incomplete");
                } else {
                    const auto *selected_branch = *std::find_if(branches.begin(), branches.end(),
                        [&](const auto *candidate) { return candidate->id == view->branch_id; });
                    std::int32_t cursor = raw_tenet_scroll.y;
                    bool rows_valid = true;
                    constexpr std::array<std::string_view, 3> headers{
                        "Level1TenetsHeader", "Level2TenetsHeader", "Level3TenetsHeader"};
                    for (std::int32_t level = 1; level <= 3; ++level) {
                        const auto *header = child(*content_stack, "Button", headers[level - 1]);
                        const auto header_size = header ? pair_attr(*header, "Size") : std::nullopt;
                        const auto label_id = std::string(headers[level - 1]) + "Label";
                        const auto *header_label = header
                            ? child(*header, "Label", label_id) : nullptr;
                        const auto stack_id = std::string("Level") + std::to_string(level) +
                            "TenetsStack";
                        const auto *level_stack = child(*content_stack, "Stack", stack_id);
                        if (!header || !header_size || !header_label || !level_stack ||
                            header_size->first != 542 || header_size->second != 27 ||
                            header_label->attribute("Font") != "TwCenMT20" ||
                            header_label->attribute("String") !=
                                std::string("TXT_KEY_POLICYSCREEN_L") +
                                    std::to_string(level) + "_TENET" ||
                            level_stack->attribute("StackGrowth") != "Bottom" ||
                            level_stack->attribute("Padding") != "0") {
                            rows_valid = false;
                            break;
                        }
                        const WorldViewRect header_rect{raw_tenet_scroll.x, cursor,
                            header_size->first, header_size->second};
                        if (const auto visible = intersect(header_rect, *scroll_rect)) {
                            rectangle(result, *visible, header->attribute("Texture"));
                            const auto label_key = header_label->attribute("String");
                            auto localized_title = localize
                                ? localize(label_key) : std::nullopt;
                            if (localized_title && (localized_title->empty() ||
                                localized_title->starts_with("TXT_KEY_")))
                                localized_title.reset();
                            result.tenet_headers.push_back({*visible, level,
                                view->tenet_collapsed[level - 1],
                                label_key,
                                view->tenet_collapsed[level - 1]
                                    ? "[ICON_PLUS]" : "[ICON_MINUS]",
                                std::move(localized_title)});
                            result.hits.push_back({*visible, "ChooseIdeology",
                                UiGameplayCommand::make("ChooseIdeology", IdeologyUiAction{
                                    IdeologyUiActionKind::toggle_tenet_level,
                                    view->branch_id, event->token, level}), true, {}});
                        }
                        cursor += header_size->second;
                        if (view->tenet_collapsed[level - 1]) continue;
                        for (const auto &row : *evidence->tenet_help) {
                            if (row.branch_type != selected_branch->type || row.level != level)
                                continue;
                            const auto match = std::find_if(state->policies.begin(),
                                state->policies.end(), [&](const auto &policy) {
                                    return policy.id == row.policy_id &&
                                        policy.type == row.policy_type &&
                                        policy.branch_type == row.branch_type;
                                });
                            if (match == state->policies.end() || row.help_key.empty() ||
                                row.localized_help.empty() || row.label_width <= 0 ||
                                row.label_width > 532 || row.label_height <= 0 ||
                                row.label_height > 4096) {
                                rows_valid = false;
                                break;
                            }
                            const auto row_height = row.label_height + 30;
                            const WorldViewRect row_rect{raw_tenet_scroll.x, cursor,
                                box_size_tenet->first, row_height};
                            if (const auto visible = intersect(row_rect, *scroll_rect)) {
                                rectangle(result, *visible, template_box->attribute("Color"));
                                const WorldViewRect label_rect{row_rect.x + 5,
                                    row_rect.y + 15, row.label_width, row.label_height};
                                if (const auto label_visible = intersect(label_rect, *scroll_rect))
                                    result.paints.push_back({WorldViewPaintKind::text,
                                        *label_visible, {}, {}, row.localized_help,
                                        template_label->attribute("Font")});
                                result.hits.push_back({*visible, "ChooseIdeology",
                                    progression_choose_ideology(view->branch_id), false,
                                    "BNW tenet Help row is read only"});
                            }
                            cursor += row_height;
                        }
                        if (!rows_valid) break;
                    }
                    if (rows_valid) {
                        content_height = cursor - raw_tenet_scroll.y;
                        if (std::any_of(result.tenet_headers.begin(),
                                result.tenet_headers.end(), [](const auto &header) {
                                    return !header.localized_title;
                                }))
                            result.unresolved.push_back(
                                "Selected tenet header localization is unavailable");
                        result.unresolved.push_back(
                            "UiScreenGlRenderer lacks FontIcons binding for tenet header markup");
                    }
                    else {
                        result.paints.resize(body_paint_start);
                        result.hits.resize(body_hit_start);
                        result.tenet_headers.resize(header_start);
                        result.unresolved.push_back(
                            "Owner tenet Help does not match active BNW controls or policy IDs");
                    }
                }
            } else result.unresolved.push_back(
                "Tenet Help text and row measurements require owner-supplied live data");
            result.scroll_region = IdeologyScrollRegion{*scroll_rect,
                view->player_id, view->turn, width, height, event->token,
                source.selected_source_path, view->overlay, 0,
                std::max(0, content_height - scroll_rect->height)};
        } else {
            const auto *buttons = child(*grid, "Stack", "ButtonStack");
            const auto *yes = buttons ? child(*buttons, "GridButton", "ConfirmYes") : nullptr;
            const auto *no = buttons ? child(*buttons, "GridButton", "ConfirmNo") : nullptr;
            const auto stack_offset_confirm = buttons ? pair_attr(*buttons, "Offset") : std::nullopt;
            const auto yes_size = yes ? pair_attr(*yes, "Size") : std::nullopt;
            const auto no_size = no ? pair_attr(*no, "Size") : std::nullopt;
            if (!buttons || !yes || !no || !stack_offset_confirm || !yes_size ||
                !no_size || buttons->attribute("Anchor") != "C,B" ||
                buttons->attribute("StackGrowth") != "Bottom" ||
                buttons->attribute("Padding") != "24" ||
                yes->attribute("Anchor") != "C,T" || no->attribute("Anchor") != "C,T" ||
                yes_size->first <= 0 || yes_size->second <= 0 ||
                no_size->first <= 0 || no_size->second <= 0) {
                result.unresolved.push_back("Selected ideology confirmation controls are incomplete");
                return result;
            }
            const auto stack_y = raw_grid.y + raw_grid.height -
                stack_offset_confirm->second - yes_size->second - 24 - no_size->second;
            const WorldViewRect raw_yes{raw_grid.x +
                (raw_grid.width - yes_size->first) / 2,
                stack_y, yes_size->first, yes_size->second};
            const WorldViewRect raw_no{raw_grid.x +
                (raw_grid.width - no_size->first) / 2,
                stack_y + yes_size->second + 24,
                no_size->first, no_size->second};
            const auto yes_rect = intersect(raw_yes, *visible_grid);
            const auto no_rect = intersect(raw_no, *visible_grid);
            if (!yes_rect || !no_rect ||
                yes_rect->width != raw_yes.width || yes_rect->height != raw_yes.height ||
                no_rect->width != raw_no.width || no_rect->height != raw_no.height) {
                result.unresolved.push_back("Selected ideology confirmation buttons do not fit overlay");
                return result;
            }
            rectangle(result, *yes_rect, yes->attribute("Style"));
            rectangle(result, *no_rect, no->attribute("Style"));
            result.hits.push_back({*yes_rect, "ChooseIdeology",
                progression_choose_ideology(view->branch_id), false,
                "BNW ideology adoption effects and Lua hooks are unavailable"});
            result.hits.push_back({*no_rect, "ChooseIdeology",
                UiGameplayCommand::make("ChooseIdeology", IdeologyUiAction{
                    IdeologyUiActionKind::close_overlay, view->branch_id, event->token}),
                true, {}});
            result.unresolved.push_back("Confirmation text requires live localization and owner data");
        }
    }
    result.unresolved.push_back(
        "Ideology adopted-by portraits and native scrollbar remain unresolved");
    return result;
}

} // namespace civ5::ui
