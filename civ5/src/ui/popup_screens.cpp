#include "civ5/ui/popup_screens.hpp"
#include <algorithm>
#include <charconv>
#include <set>
#include <stdexcept>

namespace civ5::ui {
namespace {

void collect_ids(const content::XmlNode &node, std::vector<std::string> &ids) {
    const auto id = node.attribute("ID");
    if (!id.empty()) ids.push_back(id);
    for (const auto &child : node.children) collect_ids(child, ids);
}

const content::XmlNode *find_id(const content::XmlNode &node, std::string_view id) {
    if (node.attribute("ID") == id) return &node;
    for (const auto &child : node.children)
        if (const auto *found = find_id(child, id)) return found;
    return nullptr;
}

std::int32_t number(std::string_view value) {
    std::int32_t result = 0;
    const auto end = value.find_first_of(",.");
    if (std::from_chars(value.data(), value.data() + (end == value.npos ? value.size() : end),
                        result).ec == std::errc{}) return result;
    return 0;
}

std::pair<std::int32_t, std::int32_t> pair(std::string_view value) {
    const auto split = value.find_first_of(",.");
    return {number(value), split == value.npos ? 0 : number(value.substr(split + 1))};
}

WorldViewRect rect(const content::XmlNode &node, std::int32_t width,
                   std::int32_t height) {
    auto [w, h] = pair(node.attribute("Size"));
    if (w <= 0 || h <= 0) return {};
    auto [ox, oy] = pair(node.attribute("Offset"));
    auto anchor = node.attribute("Anchor");
    std::int32_t x = ox, y = oy;
    if (!anchor.empty()) {
        const auto separator = anchor.find_first_of(",.");
        const char ax = anchor[0];
        const char ay = separator == anchor.npos || separator + 1 >= anchor.size()
            ? 'T' : anchor[separator + 1];
        if (ax == 'R' || ax == 'r') x = width - w - ox;
        else if (ax == 'C' || ax == 'c') x = (width - w) / 2 + ox;
        if (ay == 'B' || ay == 'b') y = height - h - oy;
        else if (ay == 'C' || ay == 'c') y = (height - h) / 2 + oy;
    }
    return {x, y, w, h};
}

const content::XmlNode *panel_node(const content::XmlNode &document) {
    for (auto id : {"ButtonStackFrame", "BottomPanel", "MainPanel", "PopupPanel",
                    "MainFrame", "BackgroundPanel", "Backdrop", "BaseFrame", "Panel"}) {
        if (const auto *node = find_id(document, id))
            if (auto [w, h] = pair(node->attribute("Size")); w > 0 && h > 0)
                return node;
    }
    const content::XmlNode *best = nullptr;
    std::int64_t best_area = 0;
    auto visit = [&](auto &&self, const content::XmlNode &node) -> void {
        if (node.name == "Grid" || node.name == "Container" || node.name == "Box") {
            const auto [w, h] = pair(node.attribute("Size"));
            const auto area = std::int64_t(w) * h;
            if (w >= 250 && h >= 100 && w <= 1400 && h <= 1100 && area > best_area) {
                best = &node;
                best_area = area;
            }
        }
        for (const auto &child : node.children) self(self, child);
    };
    visit(visit, document);
    return best;
}

std::optional<std::string> resolved_text(std::string_view value,
    const std::function<std::optional<std::string>(std::string_view)> &localize) {
    if (value.empty()) return std::nullopt;
    if (!value.starts_with("TXT_KEY_")) return std::string(value);
    return localize ? localize(value) : std::nullopt;
}

std::string source_key(const UiContextDescriptor &context) {
    if (context.id == "DeclareWarPopup")
        return "Assets/DLC/Expansion2/UI/InGame/Popups/DeclareWarPopup.xml";
    return context.source_key + ".xml";
}

const PopupEvent *event_for(const UiFrameSnapshot &frame, std::string_view id) {
    const auto *state = frame.find<PopupFrame>();
    if (!state || !frame.active_player_id() || state->player_id != *frame.active_player_id())
        return nullptr;
    const auto it = std::find_if(state->events.begin(), state->events.end(),
        [id](const auto &event) { return event.context_id == id; });
    return it == state->events.end() ? nullptr : &*it;
}

void paint(PopupScreenPlan &plan, WorldViewPaintKind kind, WorldViewRect bounds,
           std::string source, std::string text = {}, std::string style = {}) {
    plan.paints.push_back({kind, bounds, std::move(source), {}, std::move(text),
                           std::move(style), 0xffffffff});
}

bool plan_goody_hut_reward(PopupScreenPlan &plan, const PopupScreenSource &source,
    const PopupEvent &event, const UiFrameSnapshot &frame, WorldViewRect panel,
    const std::function<std::optional<std::string>(std::string_view)> &localize) {
    if (source.context_id != "GoodyHutPopup") return false;
    const auto *title = find_id(source.document, "TitleLabel");
    const auto *body = find_id(source.document, "DescriptionLabel");
    const auto *close = find_id(source.document, "CloseButton");
    if (!title || !body || !close || !event.body || event.body->empty() ||
        !event.choices.empty()) {
        plan.unresolved.push_back("Goody hut reward lacks authored controls or live text");
        return true;
    }
    const auto [close_width, close_height] = pair(close->attribute("Size"));
    if (close_width <= 0 || close_height <= 0 ||
        close_width > panel.width || close_height > panel.height) {
        plan.unresolved.push_back("Selected goody hut CloseButton has no fixed size");
        return true;
    }
    // The selected XML has one centered 400x270 grid and a centered bottom
    // CloseButton. Its title is authored; the reward description is supplied
    // by the current Lua/GameCore popup, never by the XML example string.
    if (auto label = resolved_text(title->attribute("String"), localize))
        paint(plan, WorldViewPaintKind::text,
            {panel.x + 16, panel.y + 60, panel.width - 32, 32}, {}, *label,
            title->attribute("Font"));
    paint(plan, WorldViewPaintKind::text,
        {panel.x + 35, panel.y + 100, panel.width - 70, 160}, {},
        *event.body, body->attribute("Font"));
    const WorldViewRect button{panel.x + (panel.width - close_width) / 2,
        panel.y + panel.height - close_height, close_width, close_height};
    paint(plan, WorldViewPaintKind::rectangle, button,
        close->attribute("Style").empty() ? "BaseButton" : close->attribute("Style"));
    for (const auto &child : close->children)
        if (auto label = resolved_text(child.attribute("String"), localize)) {
            paint(plan, WorldViewPaintKind::text, button, {}, *label,
                child.attribute("Font"));
            break;
        }
    const auto &owner = *event.goody_hut_reward;
    const auto *view = frame.find<GoodyHutCloseView>();
    const bool ready = view && view->player_id == owner.player_id &&
        view->turn == owner.turn && view->owner_sequence == owner.owner_sequence &&
        view->token == owner.token && view->event_id == event.event_id &&
        view->selected_source_path == source.selected_source_path;
    plan.hits.push_back({button, UiGameplayCommand::make(source.context_id,
        GoodyHutClose{owner.owner_sequence, owner.token, event.event_id}),
        ready, ready ? "" : "Native reward popup acknowledgement is unavailable"});
    return true;
}

const content::XmlNode *religion_instance(const content::XmlNode &document) {
    for (const auto &child : document.children)
        if (child.name == "Instance" &&
            child.attribute("Name") == "ReligionButtonInstance") return &child;
    return nullptr;
}

bool plan_religion_strip(PopupScreenPlan &plan, const PopupScreenSource &source,
    const PopupEvent &event, const UiFrameSnapshot &frame, WorldViewRect panel,
    std::int32_t viewport_width, std::int32_t viewport_height,
    const UiGameplayBackend *backend,
    const std::function<std::optional<std::string>(std::string_view)> &localize) {
    if (source.context_id != "ChooseReligion") return false;
    const auto *scroll = find_id(source.document, "ReligionScrollPanel");
    const auto *stack = find_id(source.document, "ReligionStack");
    const auto *instance = religion_instance(source.document);
    const auto *button = instance ? find_id(*instance, "Button") : nullptr;
    const auto *icon = instance ? find_id(*instance, "Icon") : nullptr;
    if (!scroll || !stack || !button || !icon || scroll->attribute("Vertical") != "0") {
        plan.unresolved.push_back("Selected religion XML has no horizontal symbol strip");
        return true;
    }
    auto strip = rect(*scroll, panel.width, panel.height);
    const auto [button_width, button_height] = pair(button->attribute("Size"));
    const auto [icon_width, icon_height] = pair(icon->attribute("Size"));
    const auto padding = number(stack->attribute("Padding"));
    if (strip.width <= 0 || strip.height <= 0 ||
        icon->attribute("Texture").empty() || button_width <= 0 ||
        button_height <= 0 || icon_width != button_width ||
        icon_height != button_height || padding < 0 ||
        button_height > strip.height || button_width + padding <= 0) {
        plan.unresolved.push_back("Selected religion XML has unsupported strip dimensions");
        return true;
    }
    strip.x += panel.x;
    strip.y += panel.y;
    const auto capacity = static_cast<std::size_t>(
        (strip.width + padding) / (button_width + padding));
    if (!capacity) {
        plan.unresolved.push_back("Religion strip has no room for a symbol");
        return true;
    }
    const auto overflow = event.choices.size() > capacity;
    const auto last_first = event.choices.empty() ? 0 : event.choices.size() - 1;
    std::size_t first = 0;
    if (const auto *view = frame.find<PopupScrollView>(); view &&
        frame.active_player_id() && view->player_id == *frame.active_player_id() &&
        view->context_id == source.context_id && view->event_id == event.event_id)
        first = std::min(view->first_visible, last_first);
    if (overflow) {
        plan.scroll_bounds = strip;
        const auto scroll_command = [&](PopupScrollDirection direction) {
            return UiGameplayCommand::make(source.context_id,
                PopupScroll{source.context_id, event.event_id, first, direction});
        };
        // The XML anchors ScrollBarLeft/Right outside the panel's left and
        // right edges. Their style does not declare a measurable width.
        const WorldViewRect left{strip.x - 18, strip.y + strip.height - 18, 18, 18};
        const WorldViewRect right{strip.x + strip.width, left.y, 18, 18};
        paint(plan, WorldViewPaintKind::rectangle, left, "ScrollBarLeft");
        paint(plan, WorldViewPaintKind::rectangle, right, "ScrollBarRight");
        plan.hits.push_back({left, scroll_command(PopupScrollDirection::up), first > 0,
            first ? "" : "Already at the first religion"});
        plan.hits.push_back({right, scroll_command(PopupScrollDirection::down),
            first < last_first,
            first < last_first ? "" : "Already at the last religion"});
    }
    const auto visible = std::min(capacity, event.choices.size() - first);
    const auto content_width = visible ?
        static_cast<std::int32_t>(visible) * button_width +
            static_cast<std::int32_t>(visible - 1) * padding : 0;
    // ChooseReligionPopup.lua centers at <=13 and anchors left above that.
    // The measured XML width produces the same 13-symbol threshold.
    const auto start_x = strip.x + (overflow ? 0 : (strip.width - content_width) / 2);
    const auto start_y = strip.y + (strip.height - button_height) / 2;
    const auto *selected = frame.find<ReligionSelectionView>();
    if (selected && (event.choose_religion->mode !=
            ChooseReligionMode::founding ||
        selected->player_id != event.choose_religion->player_id ||
        selected->turn != event.choose_religion->turn ||
        selected->owner_sequence != event.choose_religion->owner_sequence ||
        selected->event_id != event.event_id ||
        selected->selected_source_path != source.selected_source_path ||
        selected->viewport_width != viewport_width ||
        selected->viewport_height != viewport_height)) selected = nullptr;
    for (std::size_t offset = 0; offset < visible; ++offset) {
        const auto &choice = event.choices[first + offset];
        const WorldViewRect bounds{start_x + static_cast<std::int32_t>(offset) *
            (button_width + padding), start_y, button_width, button_height};
        paint(plan, WorldViewPaintKind::rectangle, bounds, button->attribute("Style"));
        if (choice.icon_atlas == "RELIGION_ATLAS_WHITE" &&
            choice.portrait_index >= 0 && choice.portrait_index < 15) {
            const auto index = choice.portrait_index;
            plan.paints.push_back({WorldViewPaintKind::texture, bounds,
                icon->attribute("Texture"),
                {(index % 5) * icon_width, (index / 5) * icon_height,
                    icon_width, icon_height}, {}, {}, 0xffffffff});
        } else {
            plan.unresolved.push_back("Live religion choice " +
                std::to_string(choice.id) + " has no resolved 64px BNW symbol");
        }
        const auto command = UiGameplayCommand::make(source.context_id,
            ReligionUiAction{ReligionUiActionKind::select_symbol,
                event.choose_religion->owner_sequence, event.event_id, choice.id});
        const bool can_select = event.choose_religion->mode ==
                ChooseReligionMode::founding && choice.available &&
            choice.icon_atlas == "RELIGION_ATLAS_WHITE" &&
            choice.portrait_index >= 0 && choice.portrait_index < 15 &&
            (!selected || selected->religion_id != choice.id);
        (void)backend;
        plan.hits.push_back({bounds, command, can_select,
            can_select ? "" : event.choose_religion->mode ==
                ChooseReligionMode::enhancing
                    ? "BNW enhancement ignores religion symbol clicks"
                    : "This religion symbol is unavailable in the live state"});
    }
    if (selected) {
        const auto chosen = std::find_if(event.choices.begin(), event.choices.end(),
            [&](const PopupChoice &choice) {
                return choice.id == selected->religion_id &&
                    choice.available &&
                    choice.icon_atlas == "RELIGION_ATLAS_WHITE" &&
                    choice.portrait_index >= 0 && choice.portrait_index < 15;
            });
        const auto *detail = find_id(source.document, "ReligionPanel");
        const auto *detail_icon = find_id(source.document, "ReligionIcon");
        const auto *found = find_id(source.document, "FoundReligion");
        if (chosen == event.choices.end() || !detail || !detail_icon || !found ||
            pair(detail->attribute("Size")) != std::pair{953, 479} ||
            pair(detail->attribute("Offset")) != std::pair{19, 132} ||
            pair(detail_icon->attribute("Size")) != std::pair{80, 80} ||
            pair(found->attribute("Size")) != std::pair{180, 45} ||
            pair(found->attribute("Offset")) != std::pair{38, 10}) {
            plan.unresolved.push_back(
                "Selected religion detail controls are unavailable");
        } else {
            const WorldViewRect detail_bounds{panel.x + 19, panel.y + 132,
                                              953, 479};
            paint(plan, WorldViewPaintKind::rectangle, detail_bounds,
                  "ReligionPanel");
            const auto index = chosen->portrait_index;
            plan.paints.push_back({WorldViewPaintKind::texture,
                {detail_bounds.x, detail_bounds.y - 4, 80, 80},
                detail_icon->attribute("Texture"),
                {(index % 5) * 80, (index / 5) * 80, 80, 80},
                {}, {}, 0xffffffff});
            const auto *name_geometry =
                event.choose_religion->name_geometry
                    ? &*event.choose_religion->name_geometry : nullptr;
            if (name_geometry &&
                name_geometry->viewport_width == viewport_width &&
                name_geometry->viewport_height == viewport_height &&
                valid_religion_name_geometry(*name_geometry) &&
                name_geometry->label.x >= detail_bounds.x &&
                name_geometry->label.y >= detail_bounds.y &&
                name_geometry->label.x + name_geometry->label.width <=
                    detail_bounds.x + detail_bounds.width &&
                name_geometry->label.y + name_geometry->label.height <=
                    detail_bounds.y + detail_bounds.height) {
                paint(plan, WorldViewPaintKind::text, name_geometry->label,
                    {}, selected->name, "TwCenMT22");
                plan.unresolved.push_back(
                    "Localized TXT_KEY_CHOOSE_RELIGION_NAME_LABEL formatting is unavailable");
                plan.hits.push_back({name_geometry->label,
                    UiGameplayCommand::make(source.context_id,
                        ReligionUiAction{ReligionUiActionKind::open_name,
                            event.choose_religion->owner_sequence, event.event_id,
                            selected->religion_id}), true, {}});
            } else plan.unresolved.push_back(
                "Religion name label needs owner-measured bounds");
            const WorldViewRect found_bounds{detail_bounds.x + 953 - 180 - 38,
                detail_bounds.y + 479 - 45 - 10, 180, 45};
            paint(plan, WorldViewPaintKind::rectangle, found_bounds,
                  found->attribute("Style"));
            plan.hits.push_back({found_bounds,
                UiGameplayCommand::make(source.context_id,
                    ReligionUiAction{ReligionUiActionKind::found_unavailable,
                        event.choose_religion->owner_sequence, event.event_id,
                        selected->religion_id}), false,
                "Religion name, beliefs and GameCore confirmation are unavailable"});
            plan.unresolved.push_back(
                "Religion name and belief selection need live owner state");
        }
    }
    if (event.title) paint(plan, WorldViewPaintKind::text,
        {panel.x + 24, panel.y + 8, panel.width - 48, 36}, {}, *event.title,
        "TwCenMT24");
    else if (const auto *heading = find_id(source.document, "FoundReligionHeading"))
        if (const auto title = resolved_text(heading->attribute("String"), localize))
            paint(plan, WorldViewPaintKind::text,
                {panel.x + 24, panel.y + 8, panel.width - 48, 36}, {}, *title,
                heading->attribute("Font"));
    if (event.body && !selected) paint(plan, WorldViewPaintKind::text,
        {panel.x + 24, strip.y + strip.height + 12,
            panel.width - 48, panel.height - 200}, {}, *event.body, "TwCenMT20");
    if (event.dismissible) {
        if (const auto *close = find_id(source.document, "CloseButton")) {
            auto close_bounds = rect(*close, panel.width, panel.height);
            close_bounds.x += panel.x;
            close_bounds.y += panel.y;
            paint(plan, WorldViewPaintKind::rectangle, close_bounds,
                close->attribute("Style"));
            if (const auto label = resolved_text(close->attribute("String"), localize))
                paint(plan, WorldViewPaintKind::text, close_bounds, {}, *label,
                    close->attribute("Font"));
            plan.hits.push_back({close_bounds, UiCloseScreen{source.context_id}, true, {}});
        } else plan.unresolved.push_back("Selected religion XML has no CloseButton");
    }
    if (selected && selected->editing_name) {
        // The authored ChangeNamePopup covers the symbol strip. Underlying
        // controls must not receive clicks while this local dialog has focus.
        plan.hits.clear();
        plan.scroll_bounds.reset();
        const auto *geometry = event.choose_religion->name_geometry
            ? &*event.choose_religion->name_geometry : nullptr;
        const auto *popup = find_id(source.document, "ChangeNamePopup");
        const auto *edit = find_id(source.document, "NewName");
        const auto *cancel = find_id(source.document, "ChangeNameCancelButton");
        const auto *default_button = find_id(source.document,
            "ChangeNameDefaultButton");
        const auto *ok = find_id(source.document, "ChangeNameOKButton");
        if (!geometry || geometry->viewport_width != viewport_width ||
            geometry->viewport_height != viewport_height ||
            !valid_religion_name_geometry(*geometry) || !popup || !edit ||
            !cancel || !default_button || !ok ||
            popup->attribute("Size") != "Full.Full" ||
            pair(edit->attribute("Size")) != std::pair{396, 20} ||
            edit->attribute("MaxLength") != "16" ||
            pair(cancel->attribute("Size")) != std::pair{120, 42} ||
            pair(default_button->attribute("Size")) != std::pair{120, 42} ||
            pair(ok->attribute("Size")) != std::pair{120, 42}) {
            plan.unresolved.push_back(
                "Religion name dialog lacks current measured/XML controls");
            return true;
        }
        const auto action = [&](ReligionUiActionKind kind) {
            return UiGameplayCommand::make(source.context_id,
                ReligionUiAction{kind, event.choose_religion->owner_sequence,
                                 event.event_id, selected->religion_id});
        };
        const WorldViewRect full{0, 0, viewport_width, viewport_height};
        paint(plan, WorldViewPaintKind::rectangle, full, "ChangeNamePopup");
        plan.hits.push_back({full, action(ReligionUiActionKind::found_unavailable),
            false, "Religion name dialog owns input focus"});
        paint(plan, WorldViewPaintKind::rectangle, geometry->popup,
              "Grid9DetailFive140");
        plan.edit_box = geometry->edit;
        paint(plan, WorldViewPaintKind::rectangle, geometry->edit, "EditBox");
        paint(plan, WorldViewPaintKind::text, geometry->edit, {}, selected->draft,
              edit->attribute("Font"));
        if (selected->name_error)
            if (const auto *error = find_id(source.document, "ChangeNameError"))
                if (const auto label = resolved_text(
                        error->attribute("String"), localize))
                    paint(plan, WorldViewPaintKind::text,
                        {geometry->popup.x + 20, geometry->popup.y + 84,
                            geometry->popup.width - 40, 30},
                        {}, *label, error->attribute("Font"));
        for (const auto &[node, bounds, kind] : {
            std::tuple{cancel, geometry->cancel,
                ReligionUiActionKind::cancel_name},
            std::tuple{default_button, geometry->default_button,
                ReligionUiActionKind::default_name},
            std::tuple{ok, geometry->ok,
                ReligionUiActionKind::accept_name}}) {
            paint(plan, WorldViewPaintKind::rectangle, bounds,
                  node->attribute("Style"));
            plan.hits.push_back({bounds, action(kind), true, {}});
        }
    }
    return true;
}

bool plan_archaeology_choices(PopupScreenPlan &plan,
    const PopupScreenSource &source, const PopupEvent &event,
    WorldViewRect panel, std::int32_t viewport_width,
    std::int32_t viewport_height) {
    if (source.context_id != "ChooseArchaeology") return false;
    const auto *scroll = find_id(source.document, "ItemScrollPanel");
    const auto *button = find_id(source.document, "Button");
    if (!scroll || !button || scroll->attribute("Vertical") != "1" ||
        number(scroll->attribute("Size")) != 418 ||
        number(button->attribute("Size")) != 418) {
        plan.unresolved.push_back("Selected archaeology XML lacks the authored item scroll");
        return true;
    }
    const auto &owner = *event.choose_archaeology;
    // Expansion2 Lua resizes the 544px scroll by screenHeight - 1020.
    // The XML parent Box is offset 42,322 inside BottomPanel (-42,100).
    const WorldViewRect clip{panel.x + 42, panel.y + 322, 418,
        544 + viewport_height - 1020};
    const auto paint_close = [&] {
        // CloseButton is authored on BottomPanel; its click only dequeues UI.
        const WorldViewRect close{panel.x + 130,
            panel.y + panel.height - 84, 250, 36};
        if (const auto *node = find_id(source.document, "CloseButton");
            node && pair(node->attribute("Size")) == std::pair{250, 36}) {
            paint(plan, WorldViewPaintKind::rectangle, close,
                node->attribute("Style"));
            plan.hits.push_back({close, UiCloseScreen{source.context_id}, true, {}});
        } else plan.unresolved.push_back("Selected archaeology XML has no CloseButton");
    };
    if (clip.height <= 0 || owner.viewport_width != viewport_width ||
        owner.viewport_height != viewport_height ||
        owner.measured_choice_bounds.size() != event.choices.size()) {
        plan.unresolved.push_back(
            "Current native archaeology row bounds are unavailable");
        paint_close();
        return true;
    }
    for (const auto &bounds : owner.measured_choice_bounds)
        if (bounds.width <= 0 || bounds.height <= 0 ||
            bounds.x < clip.x || bounds.y < clip.y ||
            static_cast<std::int64_t>(bounds.x) + bounds.width >
                static_cast<std::int64_t>(clip.x) + clip.width ||
            static_cast<std::int64_t>(bounds.y) + bounds.height >
                static_cast<std::int64_t>(clip.y) + clip.height) {
            plan.unresolved.push_back(
                "Native archaeology row lies outside the authored scroll");
            paint_close();
            return true;
        }
    for (std::size_t i = 0; i < owner.measured_choice_bounds.size(); ++i)
        for (std::size_t j = i + 1; j < owner.measured_choice_bounds.size(); ++j) {
            const auto &a = owner.measured_choice_bounds[i];
            const auto &b = owner.measured_choice_bounds[j];
            if (a.x < b.x + b.width && b.x < a.x + a.width &&
                a.y < b.y + b.height && b.y < a.y + a.height) {
                plan.unresolved.push_back("Native archaeology rows overlap");
                paint_close();
                return true;
            }
        }
    for (std::size_t i = 0; i < event.choices.size(); ++i) {
        const auto &choice = event.choices[i];
        const auto &bounds = owner.measured_choice_bounds[i];
        paint(plan, WorldViewPaintKind::rectangle, bounds, "ItemInstance.Button");
        paint(plan, WorldViewPaintKind::text, bounds, {}, choice.label,
            "TwCenMT20");
        plan.hits.push_back({bounds, popup_choice_command({source.context_id,
            event.event_id, choice.id, std::nullopt}), false,
            "Archaeology confirmation and world effects are unavailable"});
    }
    paint_close();
    return true;
}

bool plan_pantheon_choices(PopupScreenPlan &plan,
    const PopupScreenSource &source, const PopupEvent &event,
    const UiFrameSnapshot &frame, WorldViewRect panel, std::int32_t viewport_width,
    std::int32_t viewport_height) {
    if (source.context_id != "ChoosePantheon") return false;
    const auto *scroll = find_id(source.document, "ItemScrollPanel");
    const auto *button = find_id(source.document, "Button");
    const auto *bottom = find_id(source.document, "BottomPanel");
    if (!scroll || !button || !bottom ||
        pair(bottom->attribute("Size")) != std::pair{510, 963} ||
        pair(bottom->attribute("Offset")) != std::pair{-42, 100} ||
        pair(scroll->attribute("Offset")) != std::pair{0, 5} ||
        scroll->attribute("Vertical") != "1" ||
        pair(scroll->attribute("Size")) != std::pair{418, 822} ||
        pair(button->attribute("Size")) != std::pair{418, 63}) {
        plan.unresolved.push_back("Selected pantheon XML lacks the authored item scroll");
        return true;
    }
    const auto &owner = *event.choose_pantheon;
    // Selected BNW Lua expands ItemScrollPanel by viewportHeight - 1020.
    // XML places it at BottomPanel + Box(42,38) + ScrollPanel(0,5).
    const WorldViewRect clip{panel.x + 42, panel.y + 43, 418,
        822 + viewport_height - 1020};
    const auto paint_close = [&] {
        const auto *node = find_id(source.document, "CloseButton");
        if (!node || pair(node->attribute("Size")) != std::pair{250, 36}) {
            plan.unresolved.push_back("Selected pantheon XML has no CloseButton");
            return;
        }
        const WorldViewRect bounds{panel.x + 130,
            panel.y + panel.height - 84, 250, 36};
        paint(plan, WorldViewPaintKind::rectangle, bounds, node->attribute("Style"));
        plan.hits.push_back({bounds, UiCloseScreen{source.context_id}, true, {}});
    };
    if (event.title)
        paint(plan, WorldViewPaintKind::text,
            {panel.x + 150, 60, 350, 36}, {}, *event.title, "TwCenMT24");
    if (clip.height <= 0 || owner.viewport_width != viewport_width ||
        owner.viewport_height != viewport_height ||
        !valid_pantheon_visible_rows(event)) {
        plan.unresolved.push_back("Current native pantheon row bounds are unavailable");
        paint_close();
        return true;
    }
    for (const auto &[id, bounds] : owner.visible_choice_bounds) {
        const auto found = std::find_if(event.choices.begin(), event.choices.end(),
            [&](const PopupChoice &choice) { return choice.id == id; });
        paint(plan, WorldViewPaintKind::rectangle, bounds, "ItemInstance.Button");
        paint(plan, WorldViewPaintKind::text,
            {bounds.x + 10, bounds.y + 7, bounds.width - 20, 24}, {},
            found->label, "TwCenMT20");
        paint(plan, WorldViewPaintKind::text,
            {bounds.x + 10, bounds.y + 28, bounds.width - 20,
             bounds.height - 30}, {}, found->detail, "TwCenMT16");
        const auto confirmation = owner.confirm_text_by_choice.find(id);
        const bool can_open = found->available &&
            valid_pantheon_confirm_geometry(owner) &&
            confirmation != owner.confirm_text_by_choice.end() &&
            !confirmation->second.empty();
        plan.hits.push_back({bounds, UiGameplayCommand::make(source.context_id,
            PantheonUiAction{PantheonUiActionKind::select_belief,
                owner.owner_sequence, event.event_id, id}), can_open,
            can_open ? "" : "Pantheon confirmation owner geometry or text is unavailable"});
    }
    // No scroll_bounds: native ScrollPanel has no verified wheel increment.
    paint_close();
    const auto *view = frame.find<PantheonConfirmView>();
    if (view && view->player_id == owner.player_id &&
        view->turn == owner.turn &&
        view->owner_sequence == owner.owner_sequence &&
        view->event_id == event.event_id &&
        view->selected_source_path == source.selected_source_path &&
        view->viewport_width == viewport_width &&
        view->viewport_height == viewport_height &&
        valid_pantheon_confirm_geometry(owner)) {
        const auto text = owner.confirm_text_by_choice.find(view->belief_id);
        if (text == owner.confirm_text_by_choice.end() || text->second.empty())
            return true;
        const auto &geometry = *owner.confirm_geometry;
        const WorldViewRect full{0, 0, viewport_width, viewport_height};
        paint(plan, WorldViewPaintKind::rectangle, full, "ChooseConfirm", {},
            "Black.200");
        plan.hits.push_back({full, UiGameplayCommand::make(source.context_id,
            PantheonUiAction{PantheonUiActionKind::confirm_unavailable,
                owner.owner_sequence, event.event_id, view->belief_id}), false,
            "Pantheon GameCore confirmation is unavailable"});
        paint(plan, WorldViewPaintKind::rectangle, geometry.panel,
            "Grid9DetailFive140");
        paint(plan, WorldViewPaintKind::text,
            {geometry.panel.x + 30, geometry.panel.y + 74, 440, 80}, {},
            text->second, "TwCenMT22");
        paint(plan, WorldViewPaintKind::rectangle, geometry.yes, "BaseButton");
        paint(plan, WorldViewPaintKind::rectangle, geometry.no, "BaseButton");
        plan.hits.push_back({geometry.yes, UiGameplayCommand::make(source.context_id,
            PantheonUiAction{PantheonUiActionKind::confirm_unavailable,
                owner.owner_sequence, event.event_id, view->belief_id}), false,
            "Pantheon GameCore confirmation is unavailable"});
        plan.hits.push_back({geometry.no, UiGameplayCommand::make(source.context_id,
            PantheonUiAction{PantheonUiActionKind::cancel_confirmation,
                owner.owner_sequence, event.event_id, view->belief_id}), true, {}});
    }
    return true;
}

} // namespace

bool valid_choose_religion_owner(const UiFrameSnapshot &frame,
    const PopupEvent &event, std::string_view selected_source_path) {
    if (event.context_id != "ChooseReligion" || !event.choose_religion ||
        !frame.active_player_id() || !frame.turn()) return false;
    const auto *queue = frame.find<PopupFrame>();
    const auto &owner = *event.choose_religion;
    if (!queue || queue->player_id != *frame.active_player_id() ||
        queue->turn != *frame.turn() || queue->owner_sequence == 0 ||
        owner.player_id != queue->player_id || owner.turn != queue->turn ||
        owner.owner_sequence != queue->owner_sequence ||
        owner.event_id != event.event_id || owner.event_id < 0 ||
        owner.city_x < 0 || owner.city_y < 0 ||
        (owner.mode != ChooseReligionMode::founding &&
         owner.mode != ChooseReligionMode::enhancing) ||
        owner.selected_source_path.empty() ||
        (!selected_source_path.empty() &&
         owner.selected_source_path != selected_source_path) ||
        event.choices.empty()) return false;
    return std::find_if(queue->events.begin(), queue->events.end(),
        [&](const PopupEvent &queued) { return &queued == &event; }) !=
        queue->events.end();
}

bool valid_religion_name_geometry(
    const ChooseReligionOwnerEvidence::NameGeometry &geometry) {
    if (geometry.viewport_width <= 0 || geometry.viewport_height <= 0 ||
        geometry.popup.width != 500 || geometry.popup.height != 260 ||
        geometry.edit.width != 396 || geometry.edit.height != 20 ||
        geometry.cancel.width != 120 || geometry.cancel.height != 42 ||
        geometry.default_button.width != 120 ||
        geometry.default_button.height != 42 ||
        geometry.ok.width != 120 || geometry.ok.height != 42)
        return false;
    const auto inside = [&](WorldViewRect bounds) {
        return bounds.width > 0 && bounds.height > 0 && bounds.x >= 0 &&
            bounds.y >= 0 &&
            std::int64_t(bounds.x) + bounds.width <= geometry.viewport_width &&
            std::int64_t(bounds.y) + bounds.height <= geometry.viewport_height;
    };
    const auto in_popup = [&](WorldViewRect bounds) {
        return bounds.x >= geometry.popup.x && bounds.y >= geometry.popup.y &&
            std::int64_t(bounds.x) + bounds.width <=
                std::int64_t(geometry.popup.x) + geometry.popup.width &&
            std::int64_t(bounds.y) + bounds.height <=
                std::int64_t(geometry.popup.y) + geometry.popup.height;
    };
    return inside(geometry.label) && inside(geometry.popup) &&
        inside(geometry.edit) && inside(geometry.cancel) &&
        inside(geometry.default_button) && inside(geometry.ok) &&
        in_popup(geometry.edit) && in_popup(geometry.cancel) &&
        in_popup(geometry.default_button) && in_popup(geometry.ok);
}

bool valid_choose_archaeology_owner(const UiFrameSnapshot &frame,
    const PopupEvent &event, std::string_view selected_source_path) {
    if (event.context_id != "ChooseArchaeology" || !event.choose_archaeology ||
        !frame.active_player_id() || !frame.turn()) return false;
    const auto *queue = frame.find<PopupFrame>();
    const auto &owner = *event.choose_archaeology;
    if (!queue || queue->player_id != *frame.active_player_id() ||
        queue->turn != *frame.turn() || queue->owner_sequence == 0 ||
        owner.player_id != queue->player_id || owner.turn != queue->turn ||
        owner.owner_sequence != queue->owner_sequence ||
        owner.event_id != event.event_id || owner.event_id < 0 ||
        owner.unit_index < 0 || owner.dig_plot_index < 0 ||
        owner.selected_source_path.empty() ||
        (!selected_source_path.empty() &&
         owner.selected_source_path != selected_source_path) ||
        event.choices.empty() || event.choices.size() > 3)
        return false;
    bool has_primary = false;
    for (const auto &choice : event.choices) {
        if (owner.written_artifact ? (choice.id != 4 && choice.id != 5)
                                   : (choice.id < 1 || choice.id > 3)) return false;
        has_primary |= choice.id == (owner.written_artifact ? 4 : 1);
    }
    return has_primary && std::find_if(queue->events.begin(), queue->events.end(),
        [&](const PopupEvent &queued) { return &queued == &event; }) !=
        queue->events.end();
}

bool valid_choose_pantheon_owner(const UiFrameSnapshot &frame,
    const PopupEvent &event, std::string_view selected_source_path) {
    if (event.context_id != "ChoosePantheon" || !event.choose_pantheon ||
        !frame.active_player_id() || !frame.turn()) return false;
    const auto *queue = frame.find<PopupFrame>();
    const auto &owner = *event.choose_pantheon;
    if (!queue || queue->player_id != *frame.active_player_id() ||
        queue->turn != *frame.turn() || queue->owner_sequence == 0 ||
        owner.player_id != queue->player_id || owner.turn != queue->turn ||
        owner.owner_sequence != queue->owner_sequence ||
        owner.event_id != event.event_id || owner.event_id < 0 ||
        owner.data2 < 0 ||
        owner.mode != (owner.data2 > 0 ? ChoosePantheonMode::pantheon
                                      : ChoosePantheonMode::reformation) ||
        owner.selected_source_path.empty() ||
        (!selected_source_path.empty() &&
         owner.selected_source_path != selected_source_path) ||
        event.choices.empty()) return false;
    std::set<std::int64_t> ids;
    for (const auto &choice : event.choices)
        if (choice.id < 0 || choice.label.empty() || choice.detail.empty() ||
            !ids.insert(choice.id).second)
            return false;
    return std::find_if(queue->events.begin(), queue->events.end(),
        [&](const PopupEvent &queued) { return &queued == &event; }) !=
        queue->events.end();
}

bool valid_pantheon_confirm_geometry(const ChoosePantheonOwnerEvidence &owner) {
    if (!owner.confirm_geometry || owner.viewport_width < 500 ||
        owner.viewport_height < 310) return false;
    const auto &geometry = *owner.confirm_geometry;
    const auto &panel = geometry.panel;
    if (panel.x != (owner.viewport_width - 500) / 2 ||
        panel.y != (owner.viewport_height - 310) / 2 ||
        panel.width != 500 || panel.height != 310) return false;
    const auto inside = [&](WorldViewRect bounds) {
        return bounds.width == 400 && bounds.height == 42 &&
            bounds.x >= panel.x && bounds.y >= panel.y &&
            static_cast<std::int64_t>(bounds.x) + bounds.width <=
                static_cast<std::int64_t>(panel.x) + panel.width &&
            static_cast<std::int64_t>(bounds.y) + bounds.height <=
                static_cast<std::int64_t>(panel.y) + panel.height;
    };
    return inside(geometry.yes) && inside(geometry.no) &&
        geometry.yes.y + geometry.yes.height < geometry.no.y;
}

bool valid_pantheon_visible_rows(const PopupEvent &event) {
    if (!event.choose_pantheon) return false;
    const auto &owner = *event.choose_pantheon;
    const WorldViewRect clip{0, 143, 418, owner.viewport_height - 198};
    if (owner.viewport_width < 418 || clip.height <= 0 ||
        owner.visible_choice_bounds.empty()) return false;
    std::set<std::int64_t> ids;
    std::vector<WorldViewRect> checked;
    for (const auto &[id, bounds] : owner.visible_choice_bounds) {
        const auto found = std::find_if(event.choices.begin(), event.choices.end(),
            [&](const PopupChoice &choice) { return choice.id == id; });
        if (found == event.choices.end() || !ids.insert(id).second ||
            bounds.width <= 0 || bounds.height < 40 ||
            bounds.x < clip.x || bounds.y < clip.y ||
            static_cast<std::int64_t>(bounds.x) + bounds.width >
                static_cast<std::int64_t>(clip.x) + clip.width ||
            static_cast<std::int64_t>(bounds.y) + bounds.height >
                static_cast<std::int64_t>(clip.y) + clip.height) return false;
        for (const auto &previous : checked)
            if (bounds.x < previous.x + previous.width &&
                previous.x < bounds.x + bounds.width &&
                bounds.y < previous.y + previous.height &&
                previous.y < bounds.y + bounds.height) return false;
        checked.push_back(bounds);
    }
    return true;
}

bool valid_goody_hut_reward_owner(const UiFrameSnapshot &frame,
    const PopupEvent &event, std::string_view selected_source_path) {
    const auto *queue = frame.find<PopupFrame>();
    if (!queue || !frame.active_player_id() || !frame.turn() ||
        event.context_id != "GoodyHutPopup" || !event.goody_hut_reward ||
        !event.body || event.body->empty() || !event.choices.empty() ||
        !event.dismissible) return false;
    const auto &owner = *event.goody_hut_reward;
    return queue->player_id == *frame.active_player_id() &&
        queue->player_id == owner.player_id &&
        queue->turn == *frame.turn() && queue->turn == owner.turn &&
        queue->owner_sequence != 0 &&
        queue->owner_sequence == owner.owner_sequence &&
        owner.token != 0 && owner.event_id == event.event_id &&
        owner.goody_type >= 0 && !owner.selected_source_path.empty() &&
        (selected_source_path.empty() ||
            owner.selected_source_path == selected_source_path);
}

bool PopupScreenSource::contains(std::string_view id) const {
    return std::find(control_ids.begin(), control_ids.end(), id) != control_ids.end();
}

bool valid_popup_name(std::string_view text, std::size_t max_characters) {
    if (text.empty()) return false;
    std::size_t characters = 0;
    bool visible = false;
    for (std::size_t i = 0; i < text.size();) {
        const auto c = static_cast<unsigned char>(text[i]);
        std::size_t length = c < 0x80 ? 1 : c >= 0xc2 && c <= 0xdf ? 2
            : c >= 0xe0 && c <= 0xef ? 3 : c >= 0xf0 && c <= 0xf4 ? 4 : 0;
        if (!length || i + length > text.size() || ++characters > max_characters)
            return false;
        char32_t scalar = c & (length == 1 ? 0x7f : length == 2 ? 0x1f
                             : length == 3 ? 0x0f : 0x07);
        for (std::size_t j = 1; j < length; ++j) {
            const auto continuation = static_cast<unsigned char>(text[i + j]);
            if ((continuation & 0xc0) != 0x80) return false;
            scalar = (scalar << 6) | (continuation & 0x3f);
        }
        if ((length == 2 && scalar < 0x80) ||
            (length == 3 && scalar < 0x800) ||
            (length == 4 && scalar < 0x10000) ||
            scalar > 0x10ffff || (scalar >= 0xd800 && scalar <= 0xdfff) ||
            scalar < 0x20 || (scalar >= 0x7f && scalar <= 0x9f))
            return false;
        if (scalar != ' ') visible = true;
        i += length;
    }
    return visible;
}

PopupCatalog PopupCatalog::from_resources(const assets::ResourceMountTable &resources,
                                          const UiContextRegistry &registry) {
    PopupCatalog result;
    for (const auto &context : registry.contexts()) {
        if (context.owner != UiContextOwner::popup) continue;
        const auto key = source_key(context);
        const auto mounted = resources.resolve(assets::AssetKey(key));
        const auto *asset = std::get_if<assets::AssetHandle>(&mounted.asset);
        if (!asset) {
            result.unresolved_.push_back(context.id + ": missing or ambiguous " + key);
            continue;
        }
        const auto bytes = asset->read_bytes(64ULL * 1024 * 1024);
        auto document = content::read_xml_metadata(std::string(bytes.begin(), bytes.end()));
        if (document.name != "Context") {
            result.unresolved_.push_back(context.id + ": selected XML has no Context root");
            continue;
        }
        PopupScreenSource source{context.id,
            asset->candidate().provenance.source_relative_path, std::move(document), {}};
        collect_ids(source.document, source.control_ids);
        result.sources_.push_back(std::move(source));
    }
    return result;
}

const PopupScreenSource *PopupCatalog::find(std::string_view id) const {
    const auto it = std::find_if(sources_.begin(), sources_.end(),
                                 [id](const auto &source) { return source.context_id == id; });
    return it == sources_.end() ? nullptr : &*it;
}

const WorldViewHit *PopupScreenPlan::hit(std::int32_t x, std::int32_t y) const {
    for (auto it = hits.rbegin(); it != hits.rend(); ++it) {
        const auto &r = it->bounds;
        if (x >= r.x && y >= r.y && x < r.x + r.width && y < r.y + r.height)
            return &*it;
    }
    return nullptr;
}

UiCommand popup_choice_command(const PopupDecision &decision) {
    return UiGameplayCommand::make(decision.context_id, decision);
}

PopupScreenPlan plan_popup_screen(const PopupScreenSource &source,
    const UiFrameSnapshot &frame, std::int32_t viewport_width,
    std::int32_t viewport_height, const UiGameplayBackend *backend,
    const std::function<std::optional<std::string>(std::string_view)> &localize) {
    if (viewport_width <= 0 || viewport_height <= 0)
        throw std::invalid_argument("Invalid popup viewport");
    PopupScreenPlan plan{source.context_id, source.selected_source_path, {}, {}, {}, {}, {}};
    if (source.context_id == "GreatPersonRewardPopup") {
        // GreatPersonRewardPopup.lua needs the current popup Data1 unit type,
        // Data2 city ID, resolved unit portrait and a native Processed event
        // on hide. None is owned by this generic PopupEvent presenter.
        plan.unresolved.push_back(
            "Great person reward needs native popup, city and portrait owners");
        return plan;
    }
    if (source.context_id == "BarbarianCampPopup") {
        // BarbarianCampPopup.lua resolves Data1 gold when its native event
        // arrives and acknowledges the queued popup only when it is hidden.
        // A generic PopupEvent has neither owner evidence nor that callback.
        plan.unresolved.push_back(
            "Barbarian camp reward needs a current native popup and processed owner");
        return plan;
    }
    if (source.context_id == "NaturalWonderPopup") {
        // The native event supplies a plot, finder gold and first-finder flag.
        // Lua resolves the live feature, text and portrait, then processes
        // the queued popup on hide. Generic text proves none of those owners.
        plan.unresolved.push_back(
            "Natural wonder reward needs native plot, portrait and processed owners");
        return plan;
    }
    const auto *event = event_for(frame, source.context_id);
    if (!event) {
        plan.unresolved.push_back("No live popup event for the active player");
        return plan;
    }
    if (source.context_id == "ChooseReligion" &&
        !valid_choose_religion_owner(frame, *event, source.selected_source_path)) {
        plan.unresolved.push_back("No current owner-issued BNW religion event and choices");
        return plan;
    }
    if (source.context_id == "ChooseArchaeology" &&
        !valid_choose_archaeology_owner(frame, *event, source.selected_source_path)) {
        plan.unresolved.push_back(
            "No current owner-issued BNW archaeology event and choices");
        return plan;
    }
    if (source.context_id == "ChoosePantheon" &&
        !valid_choose_pantheon_owner(frame, *event, source.selected_source_path)) {
        plan.unresolved.push_back(
            "No current owner-issued BNW pantheon event and beliefs");
        return plan;
    }
    if (source.context_id == "GoodyHutPopup" &&
        !valid_goody_hut_reward_owner(frame, *event, source.selected_source_path)) {
        plan.unresolved.push_back("No current owner-issued goody hut reward event");
        return plan;
    }
    const auto *node = panel_node(source.document);
    if (!node) {
        plan.unresolved.push_back("Active XML does not expose a fixed popup panel rectangle");
        return plan;
    }
    auto panel = rect(*node, viewport_width, viewport_height);
    if (source.context_id == "ChooseArchaeology" ||
        source.context_id == "ChoosePantheon")
        panel.height += viewport_height - 1020;
    if (panel.width <= 0 || panel.height <= 0) {
        plan.unresolved.push_back("Active popup panel size is dynamic");
        return plan;
    }
    // The XML supplies the texture/style and panel size. Event text and choices
    // are live payloads; no authored placeholder string is shown as game data.
    const auto texture = node->attribute("Texture");
    const auto style = node->attribute("Style");
    paint(plan, texture.empty() ? WorldViewPaintKind::rectangle : WorldViewPaintKind::texture,
          panel, texture.empty() ? style : texture);
    if (plan_goody_hut_reward(plan, source, *event, frame, panel, localize))
        return plan;
    if (plan_religion_strip(plan, source, *event, frame, panel,
            viewport_width, viewport_height, backend, localize))
        return plan;
    if (plan_archaeology_choices(plan, source, *event, panel,
            viewport_width, viewport_height)) return plan;
    if (plan_pantheon_choices(plan, source, *event, frame, panel,
            viewport_width, viewport_height)) return plan;
    const auto margin = 24;
    std::int32_t y = panel.y + margin;
    if (event->title) {
        paint(plan, WorldViewPaintKind::text,
              {panel.x + margin, y, panel.width - margin * 2, 36}, {}, *event->title,
              "TwCenMT24");
        y += 42;
    } else if (const auto *title = find_id(source.document, "PanelTitle")) {
        if (const auto value = resolved_text(title->attribute("String"), localize)) {
            paint(plan, WorldViewPaintKind::text,
                  {panel.x + margin, y, panel.width - margin * 2, 36}, {}, *value,
                  title->attribute("Font"));
            y += 42;
        }
    }
    if (event->body) {
        paint(plan, WorldViewPaintKind::text,
              {panel.x + margin, y, panel.width - margin * 2, 80}, {}, *event->body,
              "TwCenMT20");
        y += 92;
    }
    const bool rename = source.context_id == "SetCityName" ||
                        source.context_id == "SetUnitName";
    const auto *edit = rename ? find_id(source.document,
        source.context_id == "SetCityName" ? "EditCityName" : "EditUnitName") : nullptr;
    std::string entered = event->entered_text.value_or("");
    if (const auto *draft = frame.find<PopupTextView>(); draft &&
        frame.active_player_id() && draft->player_id == *frame.active_player_id() &&
        draft->context_id == source.context_id && draft->event_id == event->event_id)
        entered = draft->text;
    if (rename) {
        if (!edit) plan.unresolved.push_back("Selected rename XML has no authored EditBox");
        else {
            const auto [width, height] = pair(edit->attribute("Size"));
            if (width <= 0 || height <= 0) {
                plan.unresolved.push_back("Selected rename EditBox has no fixed size");
            } else {
                // The active XML centers the 354x184 frame, places the Stack
                // at 20,50, then places the EditBox below its title label.
                const WorldViewRect box{panel.x + (panel.width - width) / 2,
                    panel.y + 78, width, height};
                plan.edit_box = box;
                paint(plan, WorldViewPaintKind::rectangle, box, "EditBox");
                paint(plan, WorldViewPaintKind::text, box, {}, entered,
                      edit->attribute("Font"));
            }
        }
    }
    if (rename) {
        const auto button_rect = [&](const content::XmlNode *button) {
            if (!button) return WorldViewRect{};
            const auto [width, height] = pair(button->attribute("Size"));
            const auto [ox, oy] = pair(button->attribute("Offset"));
            if (width <= 0 || height <= 0) return WorldViewRect{};
            return WorldViewRect{panel.x + (panel.width - width) / 2 + ox,
                panel.y + panel.height - height - oy, width, height};
        };
        const auto button_label = [&](const content::XmlNode *button) {
            if (!button) return std::optional<std::string>{};
            for (const auto &child : button->children)
                if (auto label = resolved_text(child.attribute("String"), localize))
                    return label;
            return std::optional<std::string>{};
        };
        if (const auto *title = find_id(source.document, "Title"); title &&
            !event->title)
            if (auto label = resolved_text(title->attribute("String"), localize))
                paint(plan, WorldViewPaintKind::text,
                    {panel.x, panel.y, panel.width, 34}, {}, *label,
                    title->attribute("Font"));
        const auto *name_label = find_id(source.document,
            source.context_id == "SetCityName" ? "CityLabel" : "UnitLabel");
        if (name_label)
            if (auto label = resolved_text(name_label->attribute("String"), localize))
                paint(plan, WorldViewPaintKind::text,
                    {panel.x + 20, panel.y + 49, panel.width - 40, 26},
                    {}, *label, name_label->attribute("Font"));
        const auto *cancel = find_id(source.document, "CancelButton");
        const auto cancel_box = button_rect(cancel);
        if (cancel_box.width > 0) {
            paint(plan, WorldViewPaintKind::rectangle, cancel_box,
                  cancel->attribute("Style"));
            if (auto label = button_label(cancel))
                paint(plan, WorldViewPaintKind::text, cancel_box, {}, *label,
                      "TwCenMT18");
            plan.hits.push_back({cancel_box, UiCloseScreen{source.context_id}, true, {}});
        } else plan.unresolved.push_back("Selected rename XML has no fixed CancelButton");
        const auto *accept = find_id(source.document, "AcceptButton");
        const auto accept_box = button_rect(accept);
        if (accept_box.width <= 0 || event->choices.size() != 1) {
            plan.unresolved.push_back("Rename requires one live choice and authored AcceptButton");
        } else {
            const auto &choice = event->choices.front();
            const auto command = popup_choice_command(
                {source.context_id, event->event_id, choice.id, entered});
            const auto *gameplay = std::get_if<UiGameplayCommand>(&command);
            const auto availability = !valid_popup_name(entered)
                ? UiCommandResult{UiCommandStatus::unavailable, "Enter a valid name"}
                : choice.available && backend && gameplay
                    ? backend->availability(*gameplay, frame)
                    : UiCommandResult{UiCommandStatus::unavailable,
                        choice.available ? "Popup gameplay backend is not connected"
                                         : "This choice is unavailable in the live state"};
            paint(plan, WorldViewPaintKind::rectangle, accept_box,
                  accept->attribute("Style"));
            if (auto label = button_label(accept))
                paint(plan, WorldViewPaintKind::text, accept_box, {}, *label,
                      "TwCenMT18");
            plan.hits.push_back({accept_box, command,
                availability.status == UiCommandStatus::ready, availability.reason});
        }
        return plan;
    }
    constexpr std::int32_t row_height = 42, row_step = 48, nav_height = 28;
    const auto bottom = panel.y + panel.height - margin - (event->dismissible ? 40 : 0);
    const auto available_height = std::max(0, bottom - y);
    const auto full_rows = static_cast<std::size_t>(available_height / row_step);
    const bool overflow = event->choices.size() > full_rows;
    const auto visible_rows = overflow && available_height > nav_height * 2
        ? static_cast<std::size_t>((available_height - nav_height * 2) / row_step)
        : full_rows;
    if (!visible_rows && !event->choices.empty())
        plan.unresolved.push_back("Popup panel has no room for a choice row");
    // A final short page is intentional: the host and the view share the same
    // upper bound, so wheel input never advances through visually identical pages.
    const auto last_first = event->choices.empty() ? 0 : event->choices.size() - 1;
    std::size_t first = 0;
    if (const auto *scroll = frame.find<PopupScrollView>(); scroll &&
        scroll->context_id == source.context_id &&
        scroll->event_id == event->event_id && frame.active_player_id() &&
        scroll->player_id == *frame.active_player_id())
        first = std::min(scroll->first_visible, last_first);
    const auto row_width = std::min<std::int32_t>(panel.width - margin * 2 -
        (overflow ? 32 : 0), 420);
    if (overflow && visible_rows) {
        plan.scroll_bounds = WorldViewRect{panel.x + margin, y,
            panel.width - margin * 2, available_height};
        const auto scroll_command = [&](PopupScrollDirection direction) {
            return UiGameplayCommand::make(source.context_id,
                PopupScroll{source.context_id, event->event_id, first, direction});
        };
        const WorldViewRect up{panel.x + margin + row_width, y, 28, nav_height};
        const WorldViewRect down{up.x, bottom - nav_height, 28, nav_height};
        paint(plan, WorldViewPaintKind::rectangle, up, "BaseButton");
        paint(plan, WorldViewPaintKind::text, up, {}, "▲", "TwCenMT20");
        paint(plan, WorldViewPaintKind::rectangle, down, "BaseButton");
        paint(plan, WorldViewPaintKind::text, down, {}, "▼", "TwCenMT20");
        plan.hits.push_back({up, scroll_command(PopupScrollDirection::up), first > 0,
                             first ? "" : "Already at the first choice"});
        plan.hits.push_back({down, scroll_command(PopupScrollDirection::down),
                             first < last_first,
                             first < last_first ? "" : "Already at the last choice"});
        y += nav_height;
    }
    for (std::size_t index = first;
         index < event->choices.size() && index - first < visible_rows; ++index) {
        const auto &choice = event->choices[index];
        const WorldViewRect row{panel.x + margin, y, row_width, row_height};
        paint(plan, WorldViewPaintKind::rectangle, row, "BaseButton");
        paint(plan, WorldViewPaintKind::text,
              {row.x + 8, row.y + 4, row.width - 16, row.height - 8}, {}, choice.label,
              "TwCenMT20");
        const auto command = popup_choice_command(
            {source.context_id, event->event_id, choice.id,
             rename ? std::optional<std::string>{entered} : event->entered_text});
        const auto *gameplay = std::get_if<UiGameplayCommand>(&command);
        const auto availability = rename && !valid_popup_name(entered)
            ? UiCommandResult{UiCommandStatus::unavailable, "Enter a valid name"}
            : choice.available && backend && gameplay
            ? backend->availability(*gameplay, frame)
            : UiCommandResult{UiCommandStatus::unavailable,
                choice.available ? "Popup gameplay backend is not connected"
                                 : "This choice is unavailable in the live state"};
        plan.hits.push_back({row, command, availability.status == UiCommandStatus::ready,
                             availability.reason});
        y += row_step;
    }
    if (event->dismissible) {
        const auto *close = find_id(source.document, "CloseButton");
        if (!close) close = find_id(source.document, "CancelButton");
        if (close) {
            // A close route is navigation only; it cannot claim that a
            // religious, diplomatic or reward choice was accepted.
            const WorldViewRect button{panel.x + panel.width - 160,
                panel.y + panel.height - 48, 140, 32};
            paint(plan, WorldViewPaintKind::rectangle, button,
                  close->attribute("Style").empty() ? "BaseButton" : close->attribute("Style"));
            const auto label = resolved_text(close->attribute("String"), localize);
            if (label) paint(plan, WorldViewPaintKind::text, button, {}, *label,
                             "TwCenMT18");
            plan.hits.push_back({button, UiCloseScreen{source.context_id}, true, {}});
        } else {
            plan.unresolved.push_back("Dismissible event has no authored close control");
        }
    }
    return plan;
}

std::optional<PopupScreenPlan> plan_focused_popup(const PopupCatalog &catalog,
    const UiScreenController &controller, const UiFrameSnapshot &frame,
    std::int32_t viewport_width, std::int32_t viewport_height,
    const UiGameplayBackend *backend,
    const std::function<std::optional<std::string>(std::string_view)> &localize) {
    const auto focused = controller.focused_screen();
    if (!focused) return std::nullopt;
    const auto *context = controller.registry().find(*focused);
    if (!context || context->owner != UiContextOwner::popup || !context->modal ||
        !controller.visible(*focused)) return std::nullopt;
    const auto *source = catalog.find(*focused);
    if (!source) return std::nullopt;
    return plan_popup_screen(*source, frame, viewport_width, viewport_height,
                             backend, localize);
}

UiCommandResult PopupGameplayBackend::availability(const UiGameplayCommand &command,
                                                    const UiFrameSnapshot &frame) const {
    const auto *decision = command.as<PopupDecision>();
    if (!decision || decision->context_id != command.source_id())
        return {UiCommandStatus::invalid, "Not a U8 popup decision"};
    const auto *event = event_for(frame, decision->context_id);
    if (!event || event->event_id != decision->event_id)
        return {UiCommandStatus::invalid, "Popup event is stale or belongs to another player"};
    const auto it = std::find_if(event->choices.begin(), event->choices.end(),
        [decision](const auto &choice) { return choice.id == decision->choice_id; });
    if (it == event->choices.end())
        return {UiCommandStatus::invalid, "Popup choice is absent from the live event"};
    if ((decision->context_id == "SetCityName" ||
         decision->context_id == "SetUnitName") &&
        (!decision->entered_text || !valid_popup_name(*decision->entered_text)))
        return {UiCommandStatus::invalid, "Popup name is empty or invalid"};
    if (decision->context_id == "SetCityName" ||
        decision->context_id == "SetUnitName")
        if (const auto *view = frame.find<PopupTextView>(); view &&
            (view->context_id != decision->context_id ||
             view->event_id != decision->event_id ||
             !frame.active_player_id() ||
             view->player_id != *frame.active_player_id() ||
             view->text != *decision->entered_text))
            return {UiCommandStatus::invalid, "Popup name draft is stale"};
    return {UiCommandStatus::unavailable,
            "BNW popup gameplay handler is not connected"};
}

UiCommandResult PopupGameplayBackend::execute(const UiGameplayCommand &command,
                                               const UiFrameSnapshot &frame) {
    return availability(command, frame);
}

} // namespace civ5::ui
