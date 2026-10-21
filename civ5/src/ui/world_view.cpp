#include "civ5/ui/world_view.hpp"
#include "civ5/ui/city_screens.hpp"
#include "civ5/ui/glyphgen_style.hpp"
#include "civ5/ui/ui_owner_fences.hpp"
#include <algorithm>
#include <charconv>
#include <stdexcept>

namespace civ5::ui {
namespace {

std::int32_t integer(std::string_view text) {
    std::int32_t value = 0;
    const auto end = text.find_first_of(",.");
    const auto segment = text.substr(0, end);
    const auto result = std::from_chars(segment.data(), segment.data() + segment.size(), value);
    return result.ec == std::errc{} ? value : 0;
}

WorldViewPoint pair_value(std::string_view text) {
    const auto delimiter = text.find_first_of(",.");
    if (delimiter == std::string_view::npos) return {integer(text), integer(text)};
    return {integer(text.substr(0, delimiter)), integer(text.substr(delimiter + 1))};
}

bool hidden(std::string_view text) { return text == "1" || text == "true" || text == "True"; }

std::string upper_unit_title(std::string value) {
    // Active EN/RU UnitPanel.lua applies Locale.ToUpper to the resolved name.
    // Match the ASCII/Cyrillic scope of the existing TopPanel uppercase path.
    for (std::size_t i = 0; i < value.size(); ++i) {
        const auto first = static_cast<unsigned char>(value[i]);
        if (first >= 'a' && first <= 'z') {
            value[i] = static_cast<char>(first - ('a' - 'A'));
            continue;
        }
        if (i + 1 >= value.size()) continue;
        const auto second = static_cast<unsigned char>(value[i + 1]);
        if (first == 0xd0 && second >= 0xb0 && second <= 0xbf) {
            value[i + 1] = static_cast<char>(second - 0x20);
            ++i;
        } else if (first == 0xd1 && second >= 0x80 && second <= 0x8f) {
            value[i] = static_cast<char>(0xd0);
            value[i + 1] = static_cast<char>(second + 0x20);
            ++i;
        } else if (first == 0xd1 && second >= 0x90 && second <= 0x9f) {
            value[i] = static_cast<char>(0xd0);
            value[i + 1] = static_cast<char>(second - 0x10);
            ++i;
        }
    }
    return value;
}

std::string plot_help_lines(std::string_view text) {
    constexpr std::string_view newline = "[NEWLINE]";
    std::string result;
    result.reserve(text.size());
    while (!text.empty()) {
        if (text.starts_with(newline)) {
            result.push_back('\n');
            text.remove_prefix(newline.size());
        } else {
            result.push_back(text.front());
            text.remove_prefix(1);
        }
    }
    return result;
}

void add_widgets(std::vector<WorldViewWidget> &widgets, std::string_view context,
                 const content::XmlNode &node, std::string context_font = {},
                 std::string context_style = {}, std::string context_colors = {}) {
    if (node.name=="Context") {
        if (!node.attribute("Font").empty()) context_font=node.attribute("Font");
        if (!node.attribute("FontStyle").empty()) context_style=node.attribute("FontStyle");
        if (!node.attribute("ColorSet").empty()) context_colors=node.attribute("ColorSet");
    }
    const auto id = node.attribute("ID");
    if (!id.empty()) {
        widgets.push_back({std::string(context), id, node.name, node.attribute("Texture"),
                           node.attribute("Style"), node.attribute("String"),
                           node.attribute("Anchor"), node.attribute("AnchorSide"),
                           node.attribute("Size"),
                           pair_value(node.attribute("Offset")),
                           pair_value(node.attribute("TextureOffset")),
                           pair_value(node.attribute("Size")),
                           hidden(node.attribute("Hidden")), node.attribute("Font"),
                           pair_value(node.attribute("Padding")), node.attribute("FontStyle"),
                           node.attribute("Color0"), node.attribute("Color1")});
        if (context=="UnitPanel" && node.name=="TextButton") {
            auto &widget=widgets.back();
            if (widget.font.empty()) widget.font=context_font;
            if (widget.font_style.empty()) widget.font_style=context_style;
            widget.color_set=node.attribute("ColorSet").empty() ? context_colors : node.attribute("ColorSet");
        }
        if (context=="UnitPanel" && node.name=="Label" && id=="UnitName") {
            // This existing presenter projects the selected authored Label;
            // it does not publish native child-context/Label initialization.
            widgets.back().color_set=node.attribute("ColorSet");
        }
        if (context=="UnitPanel" && (node.name=="TextButton" ||
            (node.name=="Label" && id=="UnitName"))) {
            widgets.back().color2=node.attribute("Color2");
            widgets.back().color2_declared=std::any_of(node.attributes.begin(),node.attributes.end(),
                [](const auto &attribute){return attribute.first=="Color2";});
            widgets.back().color3_declared=std::any_of(node.attributes.begin(),node.attributes.end(),
                [](const auto &attribute){return attribute.first=="Color3";});
        }
        if (node.name == "GridButton") {
            for (const auto &child : node.children)
                if (child.name == "GridData") {
                    widgets.back().style = child.attribute("Style");
                    break;
                }
        }
    }
    for (const auto &child : node.children) add_widgets(widgets,context,child,context_font,context_style,context_colors);
}

WorldViewWidget layout_widget(const content::XmlNode &node) {
    WorldViewWidget result;
    result.element=node.name; result.anchor=node.attribute("Anchor");
    result.offset=pair_value(node.attribute("Offset")); result.size=pair_value(node.attribute("Size"));
    result.size_spec=node.attribute("Size"); result.style=node.attribute("Style");
    return result;
}
void collect_unit_stat_layout(std::optional<WorldViewUnitStatLayout> &layout,const content::XmlNode &node) {
    if (node.name=="Grid") for (const auto &box : node.children) if (box.name=="Box")
        for (const auto &stack : box.children) if (stack.name=="Stack" && stack.attribute("StackGrowth")=="Bottom") {
            const auto movement=std::find_if(stack.children.begin(),stack.children.end(),[](const auto &child) {
                return child.attribute("ID")=="UnitMovementBox";
            });
            if (movement==stack.children.end()) continue;
            if (layout || stack.children.empty() || stack.children.front().name!="Box" ||
                node.attribute("Anchor")!="L,B" || box.attribute("Anchor")!="R,T" ||
                stack.attribute("Anchor")!="R,T" || pair_value(stack.attribute("Offset")).x!=0 ||
                pair_value(stack.attribute("Offset")).y!=0 || integer(stack.attribute("Padding"))!=0)
                throw std::runtime_error("Selected UnitPanel stat stack parent layout differs");
            layout=WorldViewUnitStatLayout{layout_widget(node),layout_widget(box),layout_widget(stack.children.front())};
        }
    for (const auto &child : node.children) collect_unit_stat_layout(layout,child);
}
content::XmlNode selected_xml(const assets::ResourceMountTable &resources,
                              const std::string &source_key) {
    const auto selected = resources.resolve(assets::AssetKey(source_key));
    const auto *asset = std::get_if<assets::AssetHandle>(&selected.asset);
    if (!asset) throw std::runtime_error("WorldView XML missing or ambiguous: " + source_key);
    const auto bytes = asset->read_bytes(64ULL * 1024 * 1024);
    return content::read_xml_metadata(std::string(bytes.begin(), bytes.end()));
}

WorldViewRect place(const WorldViewWidget &widget, std::int32_t viewport_width,
                    std::int32_t viewport_height) {
    const auto width = widget.size_spec.starts_with("Full") ? viewport_width : widget.size.x;
    const auto comma = widget.size_spec.find_first_of(",.");
    const auto height = comma != std::string::npos &&
        widget.size_spec.substr(comma + 1).starts_with("Full") ? viewport_height : widget.size.y;
    std::int32_t x = widget.offset.x, y = widget.offset.y;
    if (widget.anchor.size() >= 3) {
        switch (widget.anchor[0]) {
            case 'R': x = viewport_width - width - std::abs(widget.offset.x); break;
            case 'C': x = (viewport_width - width) / 2 + widget.offset.x; break;
            default: break;
        }
        switch (widget.anchor[2]) {
            case 'B': y = viewport_height - height - widget.offset.y; break;
            case 'C': y = (viewport_height - height) / 2 + widget.offset.y; break;
            default: break;
        }
    }
    return {x, y, width, height};
}

WorldViewRect widget_rect(const WorldViewSkin &skin, std::string_view context,
                          std::string_view id, std::int32_t width, std::int32_t height) {
    const auto *widget = skin.find(context, id);
    if (!widget) throw std::runtime_error("WorldView authored widget missing: " + std::string(id));
    return place(*widget, width, height);
}

void paint_panel(WorldViewPlan &plan, WorldViewRect bounds, std::string style,
                 std::string label = {}) {
    if (bounds.width <= 0 || bounds.height <= 0) return;
    plan.paints.push_back({WorldViewPaintKind::rectangle, bounds, {}, {}, {},
                           std::move(style), 0xd0ffffff});
    if (!label.empty())
        plan.paints.push_back({WorldViewPaintKind::text, bounds, {}, {}, std::move(label),
                               "TwCenMT20", 0xffffffff});
}

void paint_icon(WorldViewPlan &plan, const WorldViewIcon &icon, std::string default_texture,
                WorldViewPoint size, std::string style = {}, bool paint_label = true) {
    if (!icon.visible) return;
    const WorldViewRect bounds{icon.screen.x - size.x / 2, icon.screen.y - size.y / 2,
                               size.x, size.y};
    plan.paints.push_back({WorldViewPaintKind::texture, bounds,
                           icon.texture.empty() ? std::move(default_texture) : icon.texture,
                           icon.atlas, {}, std::move(style)});
    if (paint_label && !icon.label.empty()) {
        const WorldViewRect text_bounds{bounds.x - 50, bounds.y + size.y, size.x + 100, 24};
        plan.paints.push_back({WorldViewPaintKind::text, text_bounds, {}, {}, icon.label,
                               "TwCenMT16", 0xffffffff});
    }
}

void add_hit(WorldViewPlan &plan, WorldViewRect bounds, std::string source,
             WorldViewAction action, bool supplied_available,
             const UiFrameSnapshot &frame, const UiGameplayBackend *backend) {
    auto command = UiGameplayCommand::make(std::move(source), action);
    const auto status = supplied_available && backend
        ? backend->availability(command, frame)
        : UiCommandResult{UiCommandStatus::unavailable,
                          supplied_available ? "Gameplay backend is not connected"
                                             : "Action is unavailable in the current game state"};
    const bool ready = status.status == UiCommandStatus::ready;
    plan.hits.push_back({bounds, UiCommand(std::move(command)), ready,
                         ready ? std::string{} : status.reason});
}

void add_navigation_hit(WorldViewPlan &plan, WorldViewRect bounds,
                        const UiContextRegistry &registry, std::string target) {
    const bool known = registry.find(target) != nullptr;
    plan.hits.push_back({bounds, UiCommand(UiOpenScreen{std::move(target)}), known,
                         known ? std::string{} : "Target screen is not active"});
}

} // namespace

const WorldViewHit *WorldViewPlan::hit(std::int32_t x, std::int32_t y) const {
    for (auto it = hits.rbegin(); it != hits.rend(); ++it) {
        const auto &r = it->bounds;
        if (x >= r.x && y >= r.y && x < r.x + r.width && y < r.y + r.height)
            return &*it;
    }
    return nullptr;
}

WorldViewSkin WorldViewSkin::from_resources(const assets::ResourceMountTable &resources,
                                             const UiContextRegistry &registry) {
    WorldViewSkin skin;
    for (const auto &context : registry.contexts()) {
        if (context.owner != UiContextOwner::world_view) continue;
        const auto source_key = context.source_key + ".xml";
        const auto document = selected_xml(resources, source_key);
        if (document.name != "Context")
            throw std::runtime_error("WorldView XML has no Context root: " + source_key);
        skin.source_keys_.push_back(source_key);
        add_widgets(skin.widgets_, context.id, document);
        if (context.id=="UnitPanel") collect_unit_stat_layout(skin.unit_stat_layout_,document);
        if (context.id == "ResourceIconManager")
            for (const auto &instance : document.children) {
                if (instance.name != "Instance" ||
                    instance.attribute("Name") != "ResourceIcon") continue;
                for (const auto &anchor : instance.children)
                    for (const auto &element : anchor.children)
                        if (element.name == "Image" &&
                            element.attribute("ID").empty() &&
                            !element.attribute("Texture").empty()) {
                            skin.resource_icon_background_ = element.attribute("Texture");
                            break;
                        }
            }
    }
    // This LuaContext is nested inside ActionInfoPanel.xml and not declared by
    // WorldView.xml. It is still an active, independently authored overlay.
    const std::string notifications = "Assets/UI/InGame/WorldView/NotificationPanel.xml";
    const auto document = selected_xml(resources, notifications);
    skin.source_keys_.push_back(notifications);
    add_widgets(skin.widgets_, "NotificationPanel", document);
    for (const auto &instance : document.children) {
        if (instance.name != "Instance" || instance.attribute("Name") != "GenericItem")
            continue;
        for (const auto &container : instance.children)
            for (const auto &slide : container.children)
                for (const auto &button : slide.children) {
                    if (button.name != "Button" ||
                        button.attribute("ID") != "GenericButton") continue;
                    for (const auto &element : button.children)
                        if (element.name == "Image" &&
                            !element.attribute("Texture").empty())
                            skin.generic_notification_icon_ = element.attribute("Texture");
                }
    }
    std::optional<content::XmlNode> stat_colors;
    for (auto &widget : skin.widgets_) if (widget.context_id=="UnitPanel" &&
        (widget.element=="TextButton" || (widget.element=="Label" && widget.id=="UnitName"))) {
        try {
            if (!stat_colors) stat_colors=selected_xml(resources,"Assets/UI/ColorAtlas.xml");
            std::optional<content::XmlNode> colors;
            const auto find_set=[&](const auto &self,const content::XmlNode &node)->void {
                if (node.name=="ColorSet" && node.attribute("Name")==widget.color_set) {
                    if (colors) throw std::runtime_error("Ambiguous UnitPanel ColorSet");
                    colors=node;
                }
                for (const auto &child : node.children) self(self,child);
            };
            find_set(find_set,*stat_colors);
            if (!colors) continue;
            if (widget.color3_declared || std::any_of(colors->attributes.begin(),colors->attributes.end(),
                [](const auto &attribute){return attribute.first=="Color3";}))
                throw std::runtime_error("Unsupported fourth color in bounded UnitPanel style");
            const auto color0=widget.color0.empty() ? colors->attribute("Color0") : widget.color0;
            const auto color1=widget.color1.empty() ? colors->attribute("Color1") : widget.color1;
            widget.font_colors[0]=resolve_ui_label_color(resources,color0);
            widget.font_colors[1]=resolve_ui_label_color(resources,color1);
            const auto declared=std::any_of(colors->attributes.begin(),colors->attributes.end(),
                [](const auto &attribute){return attribute.first=="Color2";});
            if (widget.color2_declared || declared)
                widget.font_colors[2]=resolve_ui_label_color(resources,
                    widget.color2_declared ? widget.color2 : colors->attribute("Color2"));
            else
                // Native 540EB0 initializes THREE RGBA-one entries. 601CA0
                // preserves the third entry when this ColorSet omits Color2.
                // A missing set or present unresolved/empty value stays unknown.
                widget.font_colors[2]=0xffffffffu;
        } catch (...) { widget.font_colors={}; }
    }
    for(auto &widget:skin.widgets_) {
        if(widget.context_id!="CityBannerManager"||widget.font_style.empty())continue;
        try {
            widget.font_colors[0]=resolve_ui_label_color(resources,widget.color0);
            widget.font_colors[1]=resolve_ui_label_color(resources,widget.color1);
            if(widget.id=="CityName"&&widget.font_style=="Shadow"&&widget.color1.empty())
                widget.font_colors[1]=city_name_shadow_color(resources);
        }catch(const std::exception &) {
            // Preserve missing source colors as an explicit render dependency.
        }
    }
    return skin;
}

const WorldViewWidget *WorldViewSkin::find(std::string_view context, std::string_view id) const {
    const auto found = std::find_if(widgets_.begin(), widgets_.end(), [&](const auto &widget) {
        return widget.context_id == context && widget.id == id;
    });
    return found == widgets_.end() ? nullptr : &*found;
}

WorldViewPlan plan_world_view(const WorldViewSkin &skin, const UiContextRegistry &registry,
                              const UiFrameSnapshot &frame, std::int32_t viewport_width,
                              std::int32_t viewport_height, const UiGameplayBackend *backend) {
    if (viewport_width <= 0 || viewport_height <= 0)
        throw std::runtime_error("WorldView viewport must be positive");
    WorldViewPlan plan;
    if (!ui_owners_current(frame)) return plan;
    const auto *state = frame.find<WorldViewState>();
    if (!state || !frame.active_player_id() ||
        state->player_id != *frame.active_player_id()) return plan;
    plan.map_visibility = state->map_visibility;

    const auto action_info_paint_begin = plan.paints.size();
    const auto *authored_end_turn_text = skin.find("ActionInfoPanel", "EndTurnText");
    const auto end_turn_label = state->end_turn_label.value_or(
        authored_end_turn_text ? authored_end_turn_text->text : std::string{});
    if (!end_turn_label.empty() && skin.find("ActionInfoPanel", "EndTurnButton")) {
        const auto bounds = widget_rect(skin, "ActionInfoPanel", "EndTurnButton",
                                        viewport_width, viewport_height);
        const auto *widget = skin.find("ActionInfoPanel", "EndTurnButton");
        paint_panel(plan, bounds, widget->style, end_turn_label);
        if (!state->end_turn_available)
            plan.paints.push_back({WorldViewPaintKind::text,
                                   {bounds.x + bounds.width - 52, bounds.y + 4, 48,
                                    bounds.height - 8}, {}, {}, "N/A", "TwCenMT16",
                                   0xc0c0c0ff});
        add_hit(plan, bounds, "ActionInfoPanel", {WorldViewActionKind::end_turn},
                state->end_turn_available, frame, backend);
        if (!state->end_turn_available && !state->end_turn_unavailable_reason.empty())
            plan.hits.back().unavailable_reason = state->end_turn_unavailable_reason;
    }
    plan.owned_paint_spans.push_back(
        {"ActionInfoPanel", action_info_paint_begin, plan.paints.size()});
    if (state->new_turn_text) {
        const auto bounds = widget_rect(skin, "NewTurn", "NewTurnGrid",
                                        viewport_width, viewport_height);
        paint_panel(plan, bounds, "Grid9FrameTurns", *state->new_turn_text);
    }
    if (state->turn_processing_text) {
        const auto bounds = widget_rect(skin, "TurnProcessing", "TurnProcessingGrid",
                                        viewport_width, viewport_height);
        paint_panel(plan, bounds, "Grid9FrameTurns", *state->turn_processing_text);
    }
    if (state->plot_help && !state->plot_help->empty()) {
        const auto *panel = skin.find("PlotHelpText", "TextBox");
        const auto *text_widget = skin.find("PlotHelpText", "Text");
        if (!panel || !text_widget || panel->style.empty())
            throw std::runtime_error("PlotHelpText authored controls are missing");
        // PlotHelpText.lua sets the Label text, autosizes the padded grid and
        // reanchors it. Keep the live owner's BNW line breaks and authored font.
        const auto content = plot_help_lines(*state->plot_help);
        const auto line_count = 1 + std::count(content.begin(), content.end(), '\n');
        const auto pad_x = std::max(0, panel->padding.x);
        const auto pad_y = std::max(0, panel->padding.y);
        const auto panel_height = std::min(viewport_height,
            static_cast<std::int32_t>(line_count) * 20 + pad_y * 2);
        // Exact horizontal DoAutoSize needs the active GlyphGen line metrics at
        // paint time. This bounded planner width remains an approximation.
        constexpr std::int32_t panel_width = 260;
        const WorldViewRect bounds{
            viewport_width - std::abs(panel->offset.x) - panel_width,
            viewport_height - std::abs(panel->offset.y) - panel_height,
            panel_width, panel_height};
        plan.paints.push_back({WorldViewPaintKind::rectangle, bounds, {}, {}, {},
                               panel->style, 0xffffffc8});
        plan.paints.push_back({WorldViewPaintKind::text,
                               {bounds.x + pad_x, bounds.y + pad_y,
                                bounds.width - pad_x * 2, bounds.height - pad_y * 2},
                               {}, {}, content,
                               text_widget->font.empty() ? "TwCenMT16" : text_widget->font,
                               0xffffffff});
    }
    if (state->path_indicators_visible && !state->path_nodes.empty()) {
        const auto *anchor = skin.find("PathHelpManager", "Anchor");
        const auto *label = skin.find("PathHelpManager", "TurnLabel");
        if (!anchor || anchor->element != "WorldAnchor" ||
            !label || label->element != "Label" || label->font.empty())
            throw std::runtime_error("PathHelpManager authored turn indicator is missing");
        // PathHelpManager.lua builds the last node before each turn change and
        // the final node. The UI consumes only current-camera projections.
        bool ordered = true;
        std::int32_t previous_turn = -1;
        for (const auto &node : state->path_nodes) {
            if (node.plot_x < 0 || node.plot_y < 0 || node.turn < 0 ||
                node.turn < previous_turn) {
                ordered = false;
                break;
            }
            previous_turn = node.turn;
        }
        if (ordered)
            for (std::size_t index = 0; index < state->path_nodes.size(); ++index) {
                const auto &node = state->path_nodes[index];
                if (index + 1 < state->path_nodes.size() &&
                    state->path_nodes[index + 1].turn == node.turn) continue;
                if (!node.screen ||
                    node.screen->x < 0 || node.screen->y < 0 ||
                    node.screen->x >= viewport_width ||
                    node.screen->y >= viewport_height) continue;
                plan.paints.push_back({WorldViewPaintKind::text,
                    {node.screen->x - 24, node.screen->y - 12, 48, 24},
                    {}, {}, std::to_string(node.turn), label->font, 0xffffffff});
            }
    }
    const auto unit_panel = [&](const WorldViewUnit &unit, std::string_view context,
                                std::int32_t x, std::int32_t y) {
        if (unit.id < 0 || unit.name.empty()) return;
        const auto *background = skin.find(context, "PrimaryStretchy");
        const auto *name_widget = skin.find(context, "UnitName");
        const auto *name_button = skin.find(context, "UnitNameButton");
        const auto *cycle_left = skin.find(context, "CycleLeft");
        const auto *cycle_right = skin.find(context, "CycleRight");
        const auto *action_button = skin.find(context, "UnitActionButton");
        if (context == "UnitPanel" && (!background || !name_widget || !action_button))
            throw std::runtime_error("Selected UnitPanel authored widgets are missing");
        if (context == "UnitPanel" && unit.portrait) {
            // The active BNW XML draws BottomLeft128x224.dds at the bottom
            // left with a 500-pixel x offset before its grids and portrait.
            // The selected Expansion2 BC008 index expands to 128x224; the
            // pinned G0 frame confirms its visible native-size source pixels.
            plan.paints.push_back({WorldViewPaintKind::texture,
                                   {500, viewport_height - 224, 128, 224},
                                   "indexed:BottomLeft128x224.dds",
                                   {0, 0, 128, 224}, {}, "UnitPanelBackplate"});
        }
        // The unnamed wide Background Grid in active BNW UnitPanel.xml is
        // L,B offset (-37,-39), size 546x208. This is distinct from the
        // dynamic PrimaryStretchy action stack used by the old placeholder.
        const bool selected_panel = context == "UnitPanel";
        const WorldViewRect frame_bounds = selected_panel
            ? WorldViewRect{-37, viewport_height - 208 + 39, 546, 208}
            : WorldViewRect{x, y, 500, 188};
        const bool authored_title = selected_panel && unit.portrait.has_value();
        paint_panel(plan, frame_bounds,
                    selected_panel ? "Grid9DetailFive140"
                                   : (background ? background->style : "Grid9DetailFive140"),
                    authored_title ? std::string{} : unit.name);
        const auto *row_receipt=frame.find<BnwUnitPanelReadRows>();
        const bool sealed_rows=selected_panel && unit.stat_rows && row_receipt &&
            row_receipt->player_id==state->player_id && row_receipt->unit_id==unit.id &&
            row_receipt->turn==frame.turn() && row_receipt->frame_number==frame.frame() &&
            !row_receipt->source_world_revision.empty() && row_receipt->rows==*unit.stat_rows;
        if (selected_panel && unit.stat_boxes) {
            // The active BNW XML nests a 400x128 R,T box 17px inside the
            // wide grid and a bottom-growing stat stack at its right edge.
            // The first, always-sized promotions box is 26px tall. Lua owns
            // row visibility; this frame receives only those booleans.
            const auto checked = [&](std::string_view id, std::int32_t width,
                                     std::int32_t height) -> const WorldViewWidget & {
                const auto *widget = skin.find(context, id);
                if (!widget || widget->element != "Grid" ||
                    widget->style != "Grid9FrameTurns" ||
                    widget->size.x != width || widget->size.y != height)
                    throw std::runtime_error("Selected UnitPanel stat XML differs");
                return *widget;
            };
            const auto &xp = checked("XPFrame", 275, 12);
            const auto &movement = checked("UnitMovementBox", 260, 27);
            const auto &strength = checked("UnitStrengthBox", 260, 27);
            const auto &ranged = checked("UnitRangedAttackBox", 260, 27);
            const auto right = frame_bounds.x + frame_bounds.width - 17;
            auto row_y = frame_bounds.y + 45 + 26;
            const auto paint_row = [&](const WorldViewWidget &widget) {
                plan.paints.push_back({WorldViewPaintKind::rectangle,
                    {right - widget.size.x, row_y, widget.size.x, widget.size.y},
                    {}, {}, {}, widget.style, 0xffffffff});
                row_y += widget.size.y;
            };
            if (unit.stat_boxes->xp) paint_row(xp);
            if (sealed_rows ? unit.stat_rows->movement.shown.value_or(unit.stat_boxes->movement) : unit.stat_boxes->movement) paint_row(movement);
            if (sealed_rows ? unit.stat_rows->strength.shown.value_or(unit.stat_boxes->strength) : unit.stat_boxes->strength) paint_row(strength);
            if (sealed_rows ? unit.stat_rows->ranged.shown.value_or(unit.stat_boxes->ranged) : unit.stat_boxes->ranged) paint_row(ranged);
        }
        if (authored_title) {
            if (!name_button || !cycle_left || !cycle_right ||
                name_button->size.x != 280 || name_button->size.y != 22 ||
                name_widget->font != "TwCenMT24" ||
                cycle_left->texture != "UnitPanelArrows.dds" ||
                cycle_right->texture != cycle_left->texture ||
                cycle_left->size.x != 32 || cycle_left->size.y != 32 ||
                cycle_right->size.x != cycle_left->size.x ||
                cycle_right->size.y != cycle_left->size.y)
                throw std::runtime_error("Selected UnitPanel title/arrow XML differs");
            // Parent boxes in the active XML resolve to UnitNameButton at
            // (212,773,280,22) for the G0 viewport. The label is centered
            // there with a -4 y offset; the GL text path opts into centering.
            plan.paints.push_back({WorldViewPaintKind::text,
                                   {212, viewport_height - 152 - 4, 280, 30},
                                   {}, {}, upper_unit_title(unit.name),
                                   "TwCenMT24:center", 0xffffc8ff});
            if (!name_widget->font_style.empty())
                plan.paints.back().font_style=UiFontStylePaint{
                    name_widget->font,name_widget->font_style,name_widget->font_colors};
            // The original UnitPanel.lua selects 24->20->14 using the SAME
            // title and button widths. Current painter measures its own
            // GlyphGen geometry; this command does not claim native GetSizeVal.
            if (name_widget->font_style=="SoftShadow") {
                const auto *source_context=registry.find(context);
                if (!source_context) throw std::runtime_error("Current UnitPanel source context is missing");
                const auto source_key=source_context->source_key+".xml";
                if (std::find(skin.source_keys().begin(),skin.source_keys().end(),source_key)==skin.source_keys().end())
                    throw std::runtime_error("Current UnitPanel title is not from selected skin source");
                plan.paints.back().unit_name_font_selection=UnitNameOwnFontSelectionRequest{
                    source_key,std::string(context),"UnitName",plan.paints.back().text,
                    name_widget->font_style,state->player_id,unit.id,frame.frame(),frame.turn(),
                    name_button->size.x};
            }
            // Lua makes both cycle buttons visible on unit selection. There
            // is no connected selection backend, so these are artwork only:
            // no hit/command is exposed and no click can report success.
            plan.paints.push_back({WorldViewPaintKind::texture,
                                   {460, viewport_height - 157, 32, 32},
                                   "indexed:" + cycle_left->texture,
                                   {cycle_left->texture_offset.x,
                                    cycle_left->texture_offset.y, 32, 32}, {},
                                   "UnitCycleLeft"});
            plan.paints.push_back({WorldViewPaintKind::texture,
                                   {212, viewport_height - 157, 32, 32},
                                   "indexed:" + cycle_right->texture,
                                   {cycle_right->texture_offset.x,
                                    cycle_right->texture_offset.y, 32, 32}, {},
                                   "UnitCycleRight"});
        }
        if (context == "UnitPanel" && unit.portrait) {
            const auto *authored = skin.find(context, "UnitPortrait");
            const auto &source = *unit.portrait;
            if (!authored || authored->element != "Image" || authored->hidden ||
                source.source_key.empty() || source.crop.x < 0 || source.crop.y < 0 ||
                source.crop.width <= 0 || source.crop.height <= 0)
                throw std::invalid_argument("Selected unit portrait source or authored image is invalid");
            // The active BNW XML places UnitPanel.dds at the bottom left,
            // before UnitPortrait. Its BC004 index expands to a 256-square
            // ring; the pinned G0 frame confirms this native-size slice at
            // (0, viewport_height-256). The XML's wider 550-pixel control
            // needs separate UV/composite evidence before drawing beyond it.
            plan.paints.push_back({WorldViewPaintKind::texture,
                                   {0, viewport_height - 256, 256, 256},
                                   "indexed:UnitPanel.dds", {0, 0, 256, 256}, {},
                                   "UnitPanelRing"});
            plan.paints.push_back({WorldViewPaintKind::texture,
                                   place(*authored, viewport_width, viewport_height),
                                   source.source_key, source.crop, {}, "UnitPortrait"});
        }
        if (sealed_rows && unit.stat_boxes && skin.unit_stat_layout()) {
            const auto &layout=*skin.unit_stat_layout();
            const auto background_bounds=place(layout.background,viewport_width,viewport_height);
            const auto parent_bounds=place(layout.parent_box,background_bounds.width,background_bounds.height);
            const auto right=background_bounds.x+parent_bounds.x+parent_bounds.width;
            auto row_y=background_bounds.y+parent_bounds.y+layout.promotions.size.y;
            if (unit.stat_boxes->xp) {
                const auto *xp=skin.find(context,"XPFrame");
                if (!xp || xp->size.y<=0) throw std::runtime_error("Selected UnitPanel XP layout is absent");
                row_y+=xp->size.y;
            }
            const auto paint_stat=[&](const BnwUnitPanelStatRowRead &row,bool receipt_shown,
                std::string_view grid_id,std::string_view label_id,std::string_view value_id) {
                const auto *grid=skin.find(context,grid_id);
                if (!grid || grid->element!="Grid" || grid->size.x<=0 || grid->size.y<=0)
                    throw std::runtime_error("Selected UnitPanel source row grid is absent");
                if (row.shown==true) {
                    const auto paint_text=[&](std::string_view id,const std::optional<std::string> &text) {
                        if (!text || text->empty()) return;
                        const auto *control=skin.find(context,id);
                        if (!control || control->element!="TextButton" || control->anchor!="R,C")
                            throw std::runtime_error("Selected UnitPanel stat text anchor differs");
                        const auto style=control->font.empty() ? "TextButton:unresolved:right-center" : control->font+":right-center";
                        WorldViewPaint paint{WorldViewPaintKind::text,
                            {right-control->offset.x,row_y+control->offset.y,0,grid->size.y},
                            {},{},*text,style,0xffffffff};
                        if (!control->font.empty() && !control->font_style.empty())
                            paint.font_style=UiFontStylePaint{control->font,control->font_style,control->font_colors};
                        plan.paints.push_back(paint);
                        if (row.tooltip_text && !row.tooltip_text->empty() && row.tooltip_blocker.empty())
                            plan.readonly_tooltips.push_back({std::string(context),std::string(id),paint,*row.tooltip_text,
                                control->font.empty() || control->font_style.empty() || !control->font_colors[0] || !control->font_colors[1] ? "Source TextButton font/effect/color binding is unresolved" : std::string{},plan.paints.size()-1});
                    };
                    paint_text(label_id,row.label);paint_text(value_id,row.value);
                }
                // Unknown typed visibility is resolved for positioning only by
                // the separate, stamped stat-box receipt; never inferred hidden.
                if (row.shown.value_or(receipt_shown)) row_y+=grid->size.y;
            };
            paint_stat(unit.stat_rows->movement,unit.stat_boxes->movement,"UnitMovementBox","UnitStatNameMovement","UnitStatMovement");
            paint_stat(unit.stat_rows->strength,unit.stat_boxes->strength,"UnitStrengthBox","UnitStatNameStrength","UnitStatStrength");
            paint_stat(unit.stat_rows->ranged,unit.stat_boxes->ranged,"UnitRangedAttackBox","UnitStatNameRangedAttack","UnitStatRangedAttack");
        }
        // The selected BNW portrait occupies x=-28..228 in the G0 viewport.
        // Put unknown stats in the panel's right-hand value column rather
        // than painting N/A over the portrait. The values remain unavailable.
        const auto stat_x = selected_panel ? x + 410 : x + 20;
        const auto movement_y = selected_panel ? y + 105 : y + 46;
        const auto strength_y = selected_panel ? y + 135 : y + 75;
        const auto stat_width = selected_panel ? 70 : 220;
        if (!selected_panel || !unit.stat_rows) plan.paints.push_back({WorldViewPaintKind::text,
                               {stat_x, movement_y, stat_width, 28},
                               {}, {}, unit.movement.empty() ? "N/A" : unit.movement,
                               "TwCenMT18"});
        if (!selected_panel || !unit.stat_rows) plan.paints.push_back({WorldViewPaintKind::text,
                               {stat_x, strength_y, stat_width, 28},
                               {}, {}, unit.strength.empty() ? "N/A" : unit.strength,
                               "TwCenMT18"});
        // An empty live action feed has no action to paint or click.
        std::int32_t action_x = x + 18;
        for (const auto &item : unit.actions) {
            const WorldViewRect bounds{action_x, y + 125,
                action_button ? action_button->size.x : 72,
                action_button ? action_button->size.y : 48};
            paint_panel(plan, bounds,
                        action_button ? action_button->style : "BaseButton", item.label);
            add_hit(plan, bounds, std::string(context),
                    {WorldViewActionKind::unit_action, item.id, unit.id},
                    item.available, frame, backend);
            if (!plan.hits.back().enabled)
                plan.paints.push_back({WorldViewPaintKind::text,
                                       {bounds.x + bounds.width - 32, bounds.y + 2, 30, 20},
                                       {}, {}, "N/A", "TwCenMT16", 0xc0c0c0ff});
            action_x += bounds.width + 4;
        }
    };
    if (state->selected_unit)
        unit_panel(*state->selected_unit, "UnitPanel", 8, viewport_height - 190);
    if (state->enemy_unit)
        unit_panel(*state->enemy_unit, "EnemyUnitPanel", viewport_width - 508,
                   viewport_height - 190);

    std::int32_t notification_y = viewport_height / 2 - 36;
    // A type-specific frame needs the live GameCore notification type. Until
    // the owner supplies that mapping, use the authored generic instance,
    // not the unrelated city-state instance.
    const auto *notification_frame = skin.find("NotificationPanel", "GenericButton");
    if ((!notification_frame || notification_frame->texture.empty() ||
         skin.generic_notification_icon().empty()) && !state->notifications.empty())
        throw std::runtime_error("NotificationPanel authored generic art is missing");
    for (const auto &notification : state->notifications) {
        const WorldViewRect bounds{viewport_width - 88, notification_y, 80, 80};
        plan.paints.push_back({WorldViewPaintKind::texture, bounds,
                               notification_frame->texture, {}, {}, {}});
        const auto icon = notification.icon_texture.empty()
            ? skin.generic_notification_icon() : notification.icon_texture;
        if (!icon.empty())
            plan.paints.push_back({WorldViewPaintKind::texture,
                                   notification.icon_texture.empty() ? bounds
                                       : WorldViewRect{bounds.x + 8, bounds.y + 8, 64, 64},
                                   icon, {}, {}, {}});
        if (!notification.text.empty())
            plan.paints.push_back({WorldViewPaintKind::text,
                                   {bounds.x - 260, bounds.y, 250, 48}, {}, {},
                                   notification.text, "TwCenMT16"});
        add_hit(plan, bounds, "ActionInfoPanel",
                {WorldViewActionKind::notification, notification.id}, true, frame, backend);
        notification_y -= 84;
    }

    if (state->city_list_launcher) {
        const auto *pull = skin.find("InfoCorner", "LeftPull");
        const auto *button = skin.find("InfoCorner", "PDButton");
        if (!pull || !button || button->texture.empty())
            throw std::runtime_error("InfoCorner authored city-list launcher is missing");
        const auto bounds = widget_rect(skin, "InfoCorner", "LeftPull",
                                        viewport_width, viewport_height);
        plan.paints.push_back({WorldViewPaintKind::texture, bounds,
                               button->texture, {}, {}, "InfoCorner"});
        add_navigation_hit(plan, bounds, registry, "CityList");
    }
    const auto *info_pull = skin.find("InfoCorner", "LeftPull");
    const auto *info_entry = skin.find("InfoCorner", "Button");
    if (!state->info_corner.empty() &&
        (!info_pull || !info_entry || info_entry->size.x <= 0 ||
         info_entry->size.y <= 0))
        throw std::runtime_error("InfoCorner authored pull-down entry is missing");
    const auto info_base = info_pull
        ? place(*info_pull, viewport_width, viewport_height) : WorldViewRect{};
    std::int32_t info_row = 0;
    for (const auto &item : state->info_corner) {
        const WorldViewRect bounds{
            info_base.x + info_entry->offset.x,
            info_base.y + info_base.height +
                info_row * (info_entry->size.y + 2),
            info_entry->size.x, info_entry->size.y};
        paint_panel(plan, bounds, info_entry->style, item.label);
        if (!item.target_screen.empty()) {
            // Registry membership alone does not prove a presenter exists.
            // CityList is the only implemented InfoCorner target at this
            // boundary; it additionally requires the current owned city frame.
            if (item.target_screen == "CityList" && state->city_list_launcher)
                add_navigation_hit(plan, bounds, registry, item.target_screen);
            else
                plan.hits.push_back({bounds,
                    UiCommand(UiOpenScreen{item.target_screen}), false,
                    "InfoCorner target has no current live screen presenter"});
        } else
            add_hit(plan, bounds, "InfoCorner", {WorldViewActionKind::info_corner, item.id},
                    item.available, frame, backend);
        ++info_row;
    }
    // TeamCityBanner.xml keeps its normal body under CityBannerBackground.
    // CityBannerBackgroundIcon is inside ShowOnMouseOver and must not become
    // the ordinary banner image. The owner supplies the projected anchor and
    // resolved city data; all 2D proportions below come from the active XML.
    const auto *city_background = skin.find("CityBannerManager", "CityBannerBackground");
    const auto *city_button = skin.find("CityBannerManager", "BannerButton");
    const auto *city_name = skin.find("CityBannerManager", "CityName");
    const auto *city_population = skin.find("CityBannerManager", "CityPopulation");
    const auto *city_left = skin.find("CityBannerManager", "CityBannerRightBackground");
    const auto *city_right = skin.find("CityBannerManager", "CityBannerLeftBackground");
    if (!state->city_banners.empty() &&
        (!city_background || city_background->texture.empty() ||
         city_background->size.x <= 0 || city_background->size.y <= 0 ||
         !city_button || city_button->size.x <= 0 || city_button->size.y <= 0 ||
         !city_name || !city_population || !city_left || !city_right ||
         city_left->texture.empty() || city_right->texture.empty()))
        throw std::runtime_error("CityBanner authored TeamCityBanner controls are missing");
    for (const auto &city : state->city_banners) {
        if (!city.visible) continue;
        const auto *appearance=city.appearance&&city.appearance->owner_id&&
            city.appearance->active_player_id==state->player_id&&city.appearance->player_id==state->player_id&&
            city.appearance->city_id==city.id&&city.appearance->frame_number==frame.frame()&&
            frame.turn()&&city.appearance->turn==*frame.turn()?&*city.appearance:nullptr;
        const auto body = WorldViewRect{
            city.screen.x - city_background->size.x / 2,
            city.screen.y - city_background->size.y / 2,
            city_background->size.x, city_background->size.y};
        plan.paints.push_back({WorldViewPaintKind::texture, body,
            city.texture.empty() ? city_background->texture : city.texture,
            city.atlas, {}, "CityBanner"});
        if(appearance)plan.paints.back().tint=appearance->background;
        const auto wing = [&](const WorldViewWidget &widget, bool left) {
            return WorldViewRect{left ? body.x - widget.size.x : body.x + body.width,
                city.screen.y - widget.size.y / 2, widget.size.x, widget.size.y};
        };
        const auto left = wing(*city_left, true);
        const auto right = wing(*city_right, false);
        plan.paints.push_back({WorldViewPaintKind::texture, left,
            city_left->texture, {}, {}, "CityBanner"});
        if(appearance)plan.paints.back().tint=appearance->background;
        plan.paints.push_back({WorldViewPaintKind::texture, right,
            city_right->texture, {}, {}, "CityBanner"});
        if(appearance)plan.paints.back().tint=appearance->background;
        const auto label_style=[](const WorldViewWidget &widget)->std::optional<UiFontStylePaint> {
            if(widget.font.empty()||widget.font_style.empty())return {};
            return UiFontStylePaint{widget.font,widget.font_style,widget.font_colors};
        };
        if (city.population > 0)
            plan.paints.push_back({WorldViewPaintKind::text, left, {}, {},
                std::to_string(city.population), city_population->font.empty()?"TwCenMT20":city_population->font,
                city_population->font_colors[0].value_or(0xffffffff), {}, label_style(*city_population)});
        if (!city.label.empty()) {
            plan.paints.push_back({WorldViewPaintKind::text, body, {}, {},
                city.label, city_name->font.empty()?"TwCenMT18":city_name->font,
                city_name->font_colors[0].value_or(0xffffffff), {}, label_style(*city_name)});
            if(appearance) {
                plan.paints.back().tint=appearance->foreground;
                if(plan.paints.back().font_style)
                    plan.paints.back().font_style->float_colors=std::array<UiFloatColor,2>{appearance->foreground,appearance->shadow};
            }
        }
        const WorldViewRect bounds{
            city.screen.x - city_button->size.x / 2,
            city.screen.y - city_button->size.y / 2,
            city_button->size.x, city_button->size.y};
        const auto *cities = frame.find<CityScreenFrame>();
        const bool confirmed = city.owned_by_active_player && city.id >= 0 &&
            cities && cities->player_id == state->player_id &&
            std::any_of(cities->cities.begin(), cities->cities.end(), [&](const auto &live) {
                return live.owner == state->player_id && live.id == city.id &&
                    live.name && *live.name == city.label;
            });
        const bool enabled = confirmed && registry.find("CityView");
        plan.hits.push_back({bounds,
            UiCommand(UiGameplayCommand::make("CityBannerManager",
                WorldViewAction{WorldViewActionKind::open_city, city.id})),
            enabled, enabled ? std::string{} :
                "Visible city banner has no matching owned live city or CityView"});
    }
    const auto unit_flag_paint_begin = plan.paints.size();
    for (const auto &flag : state->unit_flags) {
        const auto *base = skin.find("UnitFlagManager", "FlagBaseOutline");
        if (!base) throw std::runtime_error("UnitFlag source is missing");
        paint_icon(plan, flag, base->texture, {40, 40}, "UnitFlag");
        if (flag.visible)
            add_hit(plan, {flag.screen.x - 20, flag.screen.y - 20, 40, 40},
                    "UnitFlagManager", {WorldViewActionKind::select_unit, flag.id},
                    true, frame, backend);
    }
    plan.owned_paint_spans.push_back(
        {"UnitFlagManager", unit_flag_paint_begin, plan.paints.size()});
    const auto *resource_source = skin.find("ResourceIconManager", "ResourceIcon");
    const auto *yield_source = skin.find("YieldIconManager", "Image");
    if (!resource_source || !yield_source ||
        (!state->resource_icons.empty() &&
         (resource_source->size.x <= 0 || resource_source->size.y <= 0 ||
          skin.resource_icon_background().empty())) ||
        (!state->yield_icons.empty() &&
         (yield_source->size.x <= 0 || yield_source->size.y <= 0)))
        throw std::runtime_error("WorldView icon source is missing");
    for (const auto &resource : state->resource_icons) {
        if (resource.visible)
            plan.paints.push_back({WorldViewPaintKind::texture,
                {resource.screen.x - resource_source->size.x / 2,
                 resource.screen.y - resource_source->size.y / 2,
                 resource_source->size.x, resource_source->size.y},
                skin.resource_icon_background(), {}, {}, "ResourceIcon"});
        // ResourceIconManager.lua puts the name in the tooltip, not under the icon.
        paint_icon(plan, resource, resource_source->texture, resource_source->size,
                   "ResourceIcon", false);
    }
    // BNW encodes amounts 1..5 in 128x128 atlas cells. Higher default yields
    // need an in-icon atlas overlay from the owner, not text below the image.
    for (const auto &yield : state->yield_icons)
        paint_icon(plan, yield, yield_source->texture, yield_source->size,
                   "YieldIcon", false);
    return plan;
}

} // namespace civ5::ui
