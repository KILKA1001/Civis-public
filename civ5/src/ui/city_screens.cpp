#include "civ5/ui/city_screens.hpp"
#include <algorithm>
#include <charconv>
#include <stdexcept>

namespace civ5::ui {
namespace {

const saves::CityCoreField *field(const saves::CityCoreObservation &city,
                                  std::string_view name) {
    const auto it = std::find_if(city.fields.begin(), city.fields.end(), [&](const auto &value) {
        return value.name && name == value.name;
    });
    return it == city.fields.end() ? nullptr : &*it;
}

std::optional<std::int64_t> scalar(const saves::CityCoreObservation &city,
                                   std::string_view name) {
    const auto *value = field(city, name);
    return value && value->values.size() == 1 ? std::optional(value->values[0]) : std::nullopt;
}

std::optional<std::string> city_name(const saves::CityCoreObservation &city) {
    const auto *value = field(city, "m_strName");
    if (!value || value->encoding != saves::CityCoreEncoding::string32 || value->bytes.empty())
        return std::nullopt;
    auto name = std::string(value->bytes.begin(), value->bytes.end());
    if (!name.empty() && name.back() == '\0') name.pop_back();
    return name.empty() ? std::nullopt : std::optional(std::move(name));
}

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

const content::XmlNode *parent_of_id(const content::XmlNode &node,
                                     std::string_view id) {
    for (const auto &child : node.children) {
        if (child.attribute("ID") == id) return &node;
        if (const auto *parent = parent_of_id(child, id)) return parent;
    }
    return nullptr;
}

std::int32_t number(std::string_view text) {
    std::int32_t value = 0;
    const auto *begin = text.data();
    const auto end = text.find_first_of(",.");
    if (begin && std::from_chars(begin, begin + (end == text.npos ? text.size() : end), value).ec == std::errc{})
        return value;
    return 0;
}

WorldViewRect authored_rect(const content::XmlNode &node, std::int32_t width,
                            std::int32_t height) {
    const auto size = node.attribute("Size");
    const auto split = size.find_first_of(",.");
    const auto w = number(size), h = split == size.npos ? 0 : number(std::string_view(size).substr(split + 1));
    const auto offset = node.attribute("Offset");
    const auto offset_split = offset.find_first_of(",.");
    const auto ox = number(offset);
    const auto oy = offset_split == offset.npos ? 0 : number(std::string_view(offset).substr(offset_split + 1));
    const auto anchor = node.attribute("Anchor");
    std::int32_t x = ox, y = oy;
    if (anchor.size() >= 3) {
        if (anchor[0] == 'R') x = width - w - ox;
        else if (anchor[0] == 'C') x = (width - w) / 2 + ox;
        if (anchor[2] == 'B') y = height - h - oy;
        else if (anchor[2] == 'C') y = (height - h) / 2 + oy;
    }
    return {x, y, w, h};
}

void paint(CityScreenPlan &plan, WorldViewPaintKind kind, WorldViewRect rect,
           std::string source, std::string value = {}) {
    plan.paints.push_back({kind, rect, std::move(source), {}, std::move(value),
                           "TwCenMT20", 0xffffffff});
}

void hit(CityScreenPlan &plan, WorldViewRect rect, UiCommand command, bool enabled,
         std::string reason = {}) {
    plan.hits.push_back({rect, std::move(command), enabled, std::move(reason)});
}

} // namespace

CityScreenFrame capture_city_screen_frame(const runtime::CityYieldContext &live,
    const saves::SavedSessionSnapshot &source, std::int64_t player_id,
    std::optional<std::int64_t> selected_city_id,
    const std::function<std::optional<std::string>(std::string_view)> &localize) {
    CityScreenFrame frame;
    frame.player_id = player_id;
    frame.selected_city_id = selected_city_id;
    const auto player = live.players.find(player_id);
    if (player == live.players.end()) return frame;
    const saves::SavedPlayerObservation *saved_player = nullptr;
    if (source.all_players) {
        const auto found = std::find_if(source.all_players->players.begin(),
            source.all_players->players.end(), [&](const auto &entry) {
                return entry.saved_player_id == player_id;
            });
        if (found != source.all_players->players.end()) saved_player = &*found;
    }
    for (const auto &[id, city] : player->second.cities) {
        if (id != city.id || city.owner != player_id)
            throw std::runtime_error("Live city owner or index mismatch");
        CityScreenCity item;
        item.owner = city.owner; item.id = city.id; item.x = city.x; item.y = city.y;
        item.population = city.population; item.puppet = city.puppet;
        item.resistance = city.resistance; item.razing = city.razing;
        if (city.production_orders) {
            item.queue_resolved = true;
            for (const auto &order : *city.production_orders)
                item.queue.push_back({order.type, order.item, std::nullopt});
            if (!city.production_orders->empty() && city.production_progress &&
                city.production_progress->source_profile == content::TechnologyProfile::bnw &&
                city.production_progress->fully_resolved) {
                const auto &order = city.production_orders->front();
                const runtime::YieldValues *values = nullptr;
                if (order.type == 0) values = &city.production_progress->units;
                else if (order.type == 1) values = &city.production_progress->buildings;
                else if (order.type == 2) values = &city.production_progress->projects;
                if (values && order.item >= 0) {
                    const auto found = values->find(order.item);
                    item.current_production_progress_times100 =
                        found == values->end() ? 0 : found->second;
                }
            }
        }
        if (saved_player && city.source_core) {
            const auto saved = std::find_if(saved_player->cities.cities.begin(),
                saved_player->cities.cities.end(), [&](const auto &entry) {
                    return entry.core.start_offset == city.source_core->first &&
                           entry.core.end_offset == city.source_core->second &&
                           scalar(entry.core, "m_iID") == city.id &&
                           scalar(entry.core, "m_eOwner") == player_id &&
                           scalar(entry.core, "m_iX") == city.x &&
                           scalar(entry.core, "m_iY") == city.y;
                });
            if (saved != saved_player->cities.cities.end()) {
                item.name = city_name(saved->core);
                if (item.name && item.name->starts_with("TXT_KEY_"))
                    item.name = localize ? localize(*item.name) : std::nullopt;
            }
        }
        frame.cities.push_back(std::move(item));
    }
    return frame;
}

void resolve_city_queue_names(CityScreenFrame &frame,
    const content::RulesDatabase &database, const content::InfoLocalizer &localize) {
    if (!localize) return;
    static constexpr std::string_view tables[] = {
        "Units", "Buildings", "Projects", "Specialists", "Processes"};
    for (auto &city : frame.cities) {
        if (city.owner != frame.player_id || !city.queue_resolved) continue;
        for (auto &order : city.queue) {
            order.name.reset();
            if (order.type < 0 || order.type >= 5 || order.item < 0)
                continue;
            const auto table = tables[static_cast<std::size_t>(order.type)];
            std::optional<std::string> label;
            std::uint64_t matches = 0;
            const content::DatabaseStatement query{
                "SELECT * FROM " + std::string(table) + " WHERE ID = ? LIMIT 2",
                {order.item}};
            try {
                const auto report = database.query_rows(query, [&](const content::RulesRowView &row) {
                    ++matches;
                    if (row.integer("ID") != order.item) return;
                    const auto info = content::read_info_metadata(row, localize);
                    const auto &text = info.text.value ? info.text : info.description;
                    if (text.value && !text.value->empty() && *text.value != text.key &&
                        !text.value->starts_with("TXT_KEY_")) label = *text.value;
                }, 2);
                if (report.all_succeeded() && matches == 1) order.name = std::move(label);
            } catch (const std::exception &) {
                // An unavailable rules row or text lookup must not invent a
                // queue label or hide the rest of the live city frame.
            }
        }
    }
}

const CityScreenCity *selected_city(const UiFrameSnapshot &frame) {
    const auto *state = frame.find<CityScreenFrame>();
    if (!state || !frame.active_player_id() || state->player_id != *frame.active_player_id() ||
        !state->selected_city_id) return nullptr;
    const auto it = std::find_if(state->cities.begin(), state->cities.end(),
        [&](const auto &city) {
            return city.id == *state->selected_city_id &&
                   city.owner == state->player_id;
        });
    return it == state->cities.end() ? nullptr : &*it;
}

bool CityScreenSource::contains(std::string_view id) const {
    return std::find(control_ids.begin(), control_ids.end(), id) != control_ids.end();
}

CityScreenSource read_city_screen_source(const assets::ResourceMountTable &resources,
                                         std::string_view context_id) {
    std::string key;
    if (context_id == "CityView") key = "Assets/UI/InGame/CityView/CityView.xml";
    else if (context_id == "ProductionPopup") key = "Assets/UI/InGame/Popups/ProductionPopup.xml";
    else throw std::invalid_argument("Context does not belong to U6");
    const auto resolved = resources.resolve(assets::AssetKey(key));
    const auto *asset = std::get_if<assets::AssetHandle>(&resolved.asset);
    if (!asset) throw std::runtime_error("BNW city XML is missing or ambiguous: " + key);
    const auto bytes = asset->read_bytes(64ULL * 1024 * 1024);
    auto xml = content::read_xml_metadata(std::string(bytes.begin(), bytes.end()));
    if (xml.name != "Context") throw std::runtime_error("City XML root is not Context");
    CityScreenSource source{std::string(context_id),
        asset->candidate().provenance.source_relative_path, std::move(xml), {}};
    collect_ids(source.document, source.control_ids);
    const auto required = context_id == "CityView" ? "CityPopulationLabel" : "StackOStacks";
    if (!source.contains(required)) throw std::runtime_error("Selected city XML misses required control");
    return source;
}

std::size_t production_popup_visible_rows(const CityScreenSource &source,
                                          std::int32_t viewport_height) {
    const auto layout = layout_production_popup(source, 1280, viewport_height);
    return layout ? static_cast<std::size_t>(layout->scroll_panel.height /
                                             layout->choice_button.height) : 0;
}

std::optional<CityViewLayout> layout_city_view(
    const CityScreenSource &source, std::int32_t width, std::int32_t height) {
    if (source.context_id != "CityView" ||
        source.selected_source_path.find("Expansion2") == std::string::npos ||
        width <= 0 || height <= 0) return std::nullopt;
    const auto *title = find_id(source.document, "TitlePanel");
    const auto *info = find_id(source.document, "CityInfo");
    const auto *panel = find_id(source.document, "ProdQueueBackground");
    const auto *purchase = find_id(source.document, "PurchaseButton");
    const auto *production = find_id(source.document, "ProductionButton");
    const auto *production_label = find_id(source.document, "ProductionButtonLabel");
    const auto *stack = parent_of_id(source.document, "ProductionButton");
    const auto *close = find_id(source.document, "ReturnToMapButton");
    if (!title || !info || !panel || !purchase || !production ||
        !production_label || !stack || !close ||
        !find_id(source.document, "CityNameTitleBarLabel") ||
        title->attribute("Anchor") != "C,T" ||
        title->attribute("Offset") != "0,36" ||
        title->attribute("Size") != "512.128" ||
        title->attribute("Texture").empty() ||
        info->attribute("Anchor") != "L,T" ||
        info->attribute("Offset") != "-36,-10" ||
        info->attribute("Size") != "339,230" ||
        panel->attribute("Anchor") != "L,B" ||
        panel->attribute("Offset") != "-36,-8" ||
        panel->attribute("Size") != "500,300" ||
        stack->name != "Stack" || stack->attribute("Anchor") != "L,T" ||
        stack->attribute("Offset") != "280,-36" ||
        stack->attribute("Padding") != "4" ||
        purchase->attribute("Size") != "160,27" ||
        production->attribute("Size") != "220,36" ||
        production_label->attribute("String") != "TXT_KEY_CITYVIEW_QUEUE_PROD" ||
        close->attribute("Anchor") != "C,B" ||
        close->attribute("Offset") != "0,84" ||
        close->attribute("Size") != "280,36") return std::nullopt;
    // The selected Lua starts with productionQueueOpen=false and changes the
    // panel height to 280 when it refreshes the city. The same bottom anchor
    // retains its eight-pixel offset. The stack places PurchaseButton first.
    auto production_panel = authored_rect(*panel, width, height);
    production_panel.height = 280;
    production_panel.y += 20;
    const auto purchase_button = WorldViewRect{
        production_panel.x + 280, production_panel.y - 36, 160, 27};
    const auto production_button = WorldViewRect{
        purchase_button.x, purchase_button.y + 27 + 4, 220, 36};
    const auto return_button = authored_rect(*close, width, height);
    if (production_button.x < 0 || production_button.y < 0 ||
        production_button.x + production_button.width > width ||
        production_button.y + production_button.height > height ||
        return_button.x < 0 || return_button.y < 0 ||
        return_button.x + return_button.width > width ||
        return_button.y + return_button.height > height) return std::nullopt;
    return CityViewLayout{authored_rect(*title, width, height),
        authored_rect(*info, width, height), production_panel,
        purchase_button, production_button, return_button,
        title->attribute("Texture")};
}

std::optional<ProductionPopupLayout> layout_production_popup(
    const CityScreenSource &source, std::int32_t width, std::int32_t height) {
    if (source.context_id != "ProductionPopup" || source.selected_source_path.empty() ||
        width <= 0 || height <= 0) return std::nullopt;
    // These values belong to the active Expansion2 ProductionPopup.xml and
    // ProductionPopup.lua pair. Refuse a different XML instead of transferring
    // Lua's screen-height offsets to an unrelated selected profile.
    if (source.selected_source_path.find("Expansion2") == std::string::npos ||
        !source.contains("StackOStacks")) return std::nullopt;
    const auto *scroll = find_id(source.document, "ScrollPanel");
    const auto *backdrop = find_id(source.document, "Backdrop");
    const auto *up = find_id(source.document, "UpButton");
    const auto *down = find_id(source.document, "DownButton");
    if (!scroll || !backdrop || !up || !down ||
        scroll->attribute("Size") != "340,400" ||
        backdrop->attribute("Size") != "380,300" ||
        up->attribute("Style") != "ScrollBarUp" ||
        down->attribute("Style") != "ScrollBarDown" ||
        up->attribute("AnchorSide") != "O.I" ||
        down->attribute("AnchorSide") != "O.I" ||
        height <= 414) return std::nullopt;
    const auto pair = [](std::string_view text)
        -> std::optional<std::pair<std::int32_t, std::int32_t>> {
        const auto comma = text.find(',');
        if (comma == text.npos) return std::nullopt;
        std::int32_t x = 0, y = 0;
        const auto parse = [](std::string_view part, std::int32_t &out) {
            const auto result = std::from_chars(part.data(), part.data() + part.size(), out);
            return result.ec == std::errc{} && result.ptr == part.data() + part.size();
        };
        return parse(text.substr(0, comma), x) && parse(text.substr(comma + 1), y)
            ? std::optional(std::pair{x, y}) : std::nullopt;
    };
    std::vector<const content::XmlNode *> path;
    const auto find_scroll = [&](const auto &self, const content::XmlNode &node) -> bool {
        path.push_back(&node);
        if (node.attribute("ID") == "ScrollPanel") return true;
        for (const auto &child : node.children)
            if (self(self, child)) return true;
        path.pop_back();
        return false;
    };
    if (!find_scroll(find_scroll, source.document)) return std::nullopt;
    WorldViewRect parent{0, 0, width, height};
    for (std::size_t i = 1; i < path.size(); ++i) {
        const auto *node = path[i];
        auto size = pair(node->attribute("Size"));
        if (!size || size->first <= 0 || size->second <= 0) return std::nullopt;
        if (node == backdrop) size->second = height - 300;
        if (node == scroll) size->second = height - 396;
        const auto offset_text = node->attribute("Offset");
        const auto offset = offset_text.empty()
            ? std::optional(std::pair<std::int32_t, std::int32_t>{0, 0})
            : pair(offset_text);
        if (!offset) return std::nullopt;
        const auto anchor = node->attribute("Anchor");
        if (anchor != "L,T" && anchor != "R,T") return std::nullopt;
        const std::int64_t x = anchor == "R,T"
            ? static_cast<std::int64_t>(parent.x) + parent.width - size->first - offset->first
            : static_cast<std::int64_t>(parent.x) + offset->first;
        const std::int64_t y = static_cast<std::int64_t>(parent.y) + offset->second;
        if (x < 0 || y < 0 || x + size->first > width || y + size->second > height)
            return std::nullopt;
        parent = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(y),
                  size->first, size->second};
    }
    const auto instance = std::find_if(source.document.children.begin(),
        source.document.children.end(), [](const auto &node) {
            return node.name == "Instance" && node.attribute("Name") == "ProdButton";
        });
    if (instance == source.document.children.end()) return std::nullopt;
    const auto button = std::find_if(instance->children.begin(), instance->children.end(),
        [](const auto &node) {
            return node.name == "Button" && node.attribute("ID") == "Button";
        });
    if (button == instance->children.end()) return std::nullopt;
    const auto button_size = pair(button->attribute("Size"));
    if (!button_size || button_size->first <= 0 || button_size->second <= 0 ||
        button_size->first > parent.width || button_size->second > parent.height)
        return std::nullopt;
    // ScrollBarUp/Down are 18x18 in selected Assets/UI/Styles.xml. Their
    // O.I left anchor puts their right edge at the panel's left edge; Lua
    // overrides the offsets to (15,0) and (15,screenY-414).
    const WorldViewRect up_button{parent.x - 18 + 15, parent.y, 18, 18};
    const WorldViewRect down_button{up_button.x, parent.y + height - 414, 18, 18};
    if (up_button.x < 0 || down_button.y + down_button.height > height)
        return std::nullopt;
    return ProductionPopupLayout{parent,
        {parent.x, parent.y, button_size->first, button_size->second},
        up_button, down_button};
}

const WorldViewHit *CityScreenPlan::hit(std::int32_t x, std::int32_t y) const {
    for (auto it = hits.rbegin(); it != hits.rend(); ++it) {
        const auto &r = it->bounds;
        if (x >= r.x && y >= r.y && x < r.x + r.width && y < r.y + r.height)
            return &*it;
    }
    return nullptr;
}

UiCommand city_open_command() { return UiOpenScreen{"CityView"}; }
UiCommand city_production_popup_command() { return UiOpenScreen{"ProductionPopup"}; }
UiCommand city_choose_production(std::int64_t city_id, std::int64_t order_type,
                                 std::int64_t item_id) {
    return UiGameplayCommand::make("ProductionPopup", CityChooseProduction{city_id, order_type, item_id});
}
UiCommand city_scroll_production_popup(std::int64_t city_id,
                                      ProductionPopupScrollDirection direction) {
    return UiGameplayCommand::make("ProductionPopup",
        ProductionPopupScroll{city_id, direction});
}
UiCommand city_queue_command(CityQueueAction action) {
    return UiGameplayCommand::make("CityView", action);
}

CityScreenPlan plan_city_screen(const CityScreenSource &source,
    const UiFrameSnapshot &frame, std::int32_t width, std::int32_t height,
    const UiGameplayBackend *backend,
    const std::function<std::optional<std::string>(std::string_view)> &localize) {
    CityScreenPlan plan{source.context_id, source.selected_source_path, {}, {}, {}};
    if (width <= 0 || height <= 0) throw std::invalid_argument("Invalid city viewport");
    const auto *city = selected_city(frame);
    if (!city) {
        plan.unresolved.push_back("No selected live city owned by the active player");
        return plan;
    }
    const auto rect = [&](std::string_view id) {
        const auto *node = find_id(source.document, id);
        if (!node) throw std::runtime_error("Selected city XML misses control " + std::string(id));
        return authored_rect(*node, width, height);
    };
    if (source.context_id == "CityView") {
        const auto layout = layout_city_view(source, width, height);
        if (!layout) {
            plan.unresolved.push_back("Selected BNW CityView XML/Lua layout is unavailable");
            return plan;
        }
        const auto info = layout->city_info;
        paint(plan, WorldViewPaintKind::texture, layout->title_panel,
              layout->title_texture);
        paint(plan, WorldViewPaintKind::text,
              {layout->title_panel.x + 96, layout->title_panel.y + 54,
               layout->title_panel.width - 192, 32}, {},
              city->name.value_or("—"));
        paint(plan, WorldViewPaintKind::rectangle, info, "Grid9DetailFive140");
        paint(plan, WorldViewPaintKind::text, {info.x + 150, info.y + 75, 90, 30}, {},
              std::to_string(city->population));
        const auto production = layout->production_panel;
        paint(plan, WorldViewPaintKind::rectangle, production, "Grid9DetailFive140");
        std::string current = "—";
        if (city->queue_resolved && !city->queue.empty())
            current = city->queue.front().name.value_or("—");
        if (!city->queue_resolved) plan.unresolved.push_back("Live production queue is unavailable");
        else if (!city->queue.empty() && !city->queue.front().name)
            plan.unresolved.push_back("Current production item name is not resolved");
        paint(plan, WorldViewPaintKind::text,
              {production.x + 155, production.y + 15, 220, 35}, {}, current);
        const auto change = layout->production_button;
        paint(plan, WorldViewPaintKind::rectangle, change, "BaseButton");
        if (city->queue_resolved && localize) {
            const std::string_view key = city->queue.empty()
                ? "TXT_KEY_CITYVIEW_CHOOSE_PROD"
                : "TXT_KEY_CITYVIEW_CHANGE_PROD";
            const auto label = localize(key);
            if (label && !label->empty() && *label != key &&
                !label->starts_with("TXT_KEY_"))
                paint(plan, WorldViewPaintKind::text, change, {}, *label);
            else plan.unresolved.push_back("Active city production button text is unavailable");
        } else if (!city->queue_resolved)
            plan.unresolved.push_back("Current production button state is unavailable");
        hit(plan, change, city_production_popup_command(), true);
        const auto close = layout->return_button;
        paint(plan, WorldViewPaintKind::rectangle, close, "BaseButton");
        hit(plan, close, UiCloseScreen{"CityView"}, true);
        if (city->queue.size() > 1)
            plan.unresolved.push_back("Queue row layout depends on active Lua and remains unavailable");
    } else if (source.context_id == "ProductionPopup") {
        const auto layout = layout_production_popup(source, width, height);
        auto backdrop = rect("Backdrop");
        if (layout) backdrop.height = height - 300; // ProductionPopup.lua SetSize
        paint(plan, WorldViewPaintKind::rectangle, backdrop, "Grid9DetailSix140");
        const auto title = rect("CityName");
        paint(plan, WorldViewPaintKind::text,
              {backdrop.x + 20, backdrop.y + 12, backdrop.width - 40, 34}, {},
              city->name.value_or("—"));
        (void)title;
        const auto stack = rect("StackOStacks");
        if (!layout) plan.unresolved.push_back(
            "Selected production XML/Lua has no supported viewport layout");
        const auto *choices = frame.find<ProductionPopupChoices>();
        if (!choices || !frame.active_player_id() || !frame.turn() ||
            choices->player_id != *frame.active_player_id() ||
            choices->city_id != city->id || choices->turn != *frame.turn()) {
            plan.unresolved.push_back("Live BNW production choices are unavailable or stale");
        } else if (!layout || choices->selected_source_path != source.selected_source_path ||
                   choices->visible_rows != production_popup_visible_rows(source, height) ||
                   !choices->visible_rows) {
            plan.unresolved.push_back("Selected BNW production scroll layout is unavailable");
        } else {
            plan.unresolved.insert(plan.unresolved.end(),
                choices->unresolved.begin(), choices->unresolved.end());
            const auto end = std::min(choices->entries.size(),
                choices->first_visible + choices->visible_rows);
            for (std::size_t index = choices->first_visible; index < end; ++index) {
                const auto &choice = choices->entries[index];
                const auto slot = index - choices->first_visible;
                const WorldViewRect row{layout->choice_button.x,
                    layout->choice_button.y +
                        static_cast<std::int32_t>(slot) * layout->choice_button.height,
                    layout->choice_button.width, layout->choice_button.height};
                const bool backend_matches = backend && backend == choices->backend_owner;
                const bool enabled = backend_matches &&
                    choice.availability.status == UiCommandStatus::ready;
                paint(plan, WorldViewPaintKind::rectangle, row, "BaseButton");
                const auto label = choice.localized_name.value_or(
                    choice.authored_type ? *choice.authored_type + " [unlocalized]"
                                         : "Unresolved production Info");
                paint(plan, WorldViewPaintKind::text, row, {},
                      enabled ? label : label + " [unavailable]");
                hit(plan, row,
                    city_choose_production(city->id, choice.order_type, choice.item_id),
                    enabled, backend_matches ? choice.availability.reason
                        : "Production backend does not match selected choices");
            }
        }
        (void)stack;
        plan.unresolved.push_back(
            "Native BNW Lua category eligibility, sorting and collapse state are unavailable");
        plan.unresolved.push_back("Production purchase eligibility is unavailable");
        const auto close = rect("CloseButton");
        paint(plan, WorldViewPaintKind::rectangle, close, "BaseButton");
        hit(plan, close, UiCloseScreen{"ProductionPopup"}, true);
    } else {
        throw std::invalid_argument("Context does not belong to U6");
    }
    return plan;
}

UiCommandResult CityGameplayBackend::availability(const UiGameplayCommand &command,
                                                  const UiFrameSnapshot &frame) const {
    const auto *city = selected_city(frame);
    if (!city) return {UiCommandStatus::unavailable, "No selected live city"};
    if (const auto *choice = command.as<CityChooseProduction>()) {
        if (command.source_id() != "ProductionPopup" || choice->city_id != city->id ||
            choice->order_type < 0 || choice->item_id < 0)
            return {UiCommandStatus::invalid, "Invalid city production choice"};
        return {UiCommandStatus::unavailable, "BNW production mutation backend is not connected"};
    }
    if (const auto *queue = command.as<CityQueueAction>()) {
        if (command.source_id() != "CityView" || queue->city_id != city->id ||
            queue->position >= city->queue.size())
            return {UiCommandStatus::invalid, "Invalid production queue action"};
        return {UiCommandStatus::unavailable, "BNW queue mutation backend is not connected"};
    }
    return {UiCommandStatus::invalid, "City screen cannot handle this action"};
}

UiCommandResult CityGameplayBackend::execute(const UiGameplayCommand &command,
                                             const UiFrameSnapshot &frame) {
    const auto result = availability(command, frame);
    return result.status == UiCommandStatus::ready
        ? UiCommandResult{UiCommandStatus::unavailable, "BNW city mutation is not implemented"} : result;
}

} // namespace civ5::ui
