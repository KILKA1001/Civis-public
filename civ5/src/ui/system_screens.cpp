#include "civ5/ui/system_screens.hpp"
#include <algorithm>
#include <charconv>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>

namespace civ5::ui {
namespace {

bool button(std::string_view element) {
    return element == "Button" || element == "GridButton" ||
           element == "TextButton";
}

void collect(const content::XmlNode &node, std::vector<SystemControl> &controls,
             bool in_instance = false) {
    if (node.name == "Instance") in_instance = true;
    if (!in_instance && (button(node.name) || node.name == "ScrollPanel" ||
                         node.name == "UpButton" || node.name == "DownButton" ||
                         (node.name == "EditBox" &&
                          node.attribute("ID") == "SearchEditBox"))) {
        const auto id = node.attribute("ID");
        if (!id.empty()) {
            auto text = node.attribute("String");
            auto texture = node.attribute("Texture");
            if (id == "SearchButton" && texture.empty())
                for (const auto &child : node.children)
                    if (child.name == "Image" && !child.attribute("Texture").empty()) {
                        texture = child.attribute("Texture");
                        break;
                    }
            if (text.empty()) {
                for (const auto &child : node.children)
                    if (child.name == "Label" && !child.attribute("String").empty()) {
                        text = child.attribute("String");
                        break;
                    }
            }
            controls.push_back({id, node.name, text, texture,
                                node.attribute("Style")});
        }
    }
    for (const auto &child : node.children) collect(child, controls, in_instance);
}

std::optional<std::pair<std::int32_t, std::int32_t>> dimensions(std::string_view value) {
    const auto split = value.find_first_of(",.");
    if (split == value.npos) return std::nullopt;
    auto parse = [](std::string_view text) -> std::optional<std::int32_t> {
        std::int32_t result = 0;
        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), result);
        if (error != std::errc{} || end != text.data() + text.size() || result <= 0)
            return std::nullopt;
        return result;
    };
    const auto width = parse(value.substr(0, split));
    const auto height = parse(value.substr(split + 1));
    if (!width || !height) return std::nullopt;
    return std::pair{*width, *height};
}

void panel_from_xml(const content::XmlNode &xml, SystemScreenSource &source) {
    for (const auto &child : xml.children) {
        if (child.name == "Instance") continue;
        if (child.name != "Grid") continue;
        if (const auto size = dimensions(child.attribute("Size"))) {
            source.authored_width = size->first;
            source.authored_height = size->second;
            if (!child.attribute("Style").empty())
                source.panel_style = child.attribute("Style");
            return;
        }
    }
}

std::string selected_path(const assets::AssetHandle &handle) {
    const auto &source = handle.candidate().provenance;
    if (source.archive_relative_path)
        return *source.archive_relative_path + ":" + source.source_relative_path;
    return source.source_relative_path;
}

std::string display(std::string_view key, const SystemLocalizer &localize) {
    if (localize && key.starts_with("TXT_KEY_"))
        if (const auto text = localize(key)) return *text;
    return std::string(key);
}

UiCommand operation(std::string_view context_id, SystemOperationKind kind,
                    std::string target = {}) {
    return UiGameplayCommand::make(std::string(context_id),
                                   SystemOperation{kind, std::move(target)});
}

bool is_system_owner(const UiContextDescriptor &context) {
    return context.owner == UiContextOwner::system;
}

std::string pedia_text(std::string text) {
    const std::string marker = "[NEWLINE]";
    std::size_t at = 0;
    while ((at = text.find(marker, at)) != std::string::npos) {
        text.replace(at, marker.size(), "\n");
        ++at;
    }
    return text;
}

std::optional<std::string> row_text(const content::XmlNode &row, const char *name) {
    const auto *child = row.child(name);
    if (!child || child->text.empty() || !child->children.empty()) return std::nullopt;
    return child->text;
}

std::optional<std::string> expand_pedia_embedded_keys(
    std::string text, const SystemLocalizer &localize,
    std::set<std::string> &visiting, int depth = 0) {
    if (depth > 8) return std::nullopt;
    std::size_t at = 0;
    while ((at = text.find("{TXT_KEY_", at)) != std::string::npos) {
        const auto end = text.find('}', at);
        if (end == std::string::npos) return std::nullopt;
        const auto key = text.substr(at + 1, end - at - 1);
        if (!visiting.insert(key).second) return std::nullopt;
        const auto value = localize(key);
        const auto expanded = value && !value->empty() && *value != key ?
            expand_pedia_embedded_keys(*value, localize, visiting, depth + 1) :
            std::nullopt;
        visiting.erase(key);
        if (!expanded) return std::nullopt;
        text.replace(at, end - at + 1, *expanded);
        at += expanded->size();
    }
    return text;
}

std::optional<std::string> pedia_league_project_summary(
    const content::XmlNode &project,
    const std::map<std::string, std::pair<std::string, std::string>> &rewards,
    const SystemLocalizer &localize) {
    constexpr std::string_view format_key = "TXT_KEY_PEDIA_LEAGUE_PROJECT_REWARD";
    const auto format = localize(format_key);
    if (!format || format->empty() || *format == format_key) return std::nullopt;
    const auto substitute = [](std::string &text, std::string_view token,
                               std::string_view value) {
        const auto at = text.find(token);
        if (at == std::string::npos ||
            text.find(token, at + token.size()) != std::string::npos) return false;
        text.replace(at, token.size(), value);
        return true;
    };
    std::string summary;
    for (const auto &[field, icon] : {
             std::pair{"RewardTier3", "[ICON_TROPHY_GOLD]"},
             std::pair{"RewardTier2", "[ICON_TROPHY_SILVER]"},
             std::pair{"RewardTier1", "[ICON_TROPHY_BRONZE]"}}) {
        const auto reward_id = row_text(project, field);
        const auto reward = reward_id ? rewards.find(*reward_id) : rewards.end();
        if (reward == rewards.end()) return std::nullopt;
        const auto description = localize(reward->second.first);
        const auto help = localize(reward->second.second);
        if (!description || description->empty() ||
            *description == reward->second.first ||
            !help || help->empty() || *help == reward->second.second)
            return std::nullopt;
        auto line = *format;
        if (!substitute(line, "{1_TrophyIcon}", icon) ||
            !substitute(line, "{2_RewardDescription}", *description) ||
            !substitute(line, "{3_RewardHelp}", *help)) return std::nullopt;
        std::set<std::string> visiting;
        const auto expanded = expand_pedia_embedded_keys(
            std::move(line), localize, visiting);
        if (!expanded) return std::nullopt;
        if (!summary.empty()) summary += "\n\n";
        summary += pedia_text(*expanded);
    }
    return summary.empty() ? std::nullopt : std::optional{std::move(summary)};
}

struct PediaBuildingClass {
    std::int32_t max_global = -1, max_player = -1, max_team = -1;
    std::string selected_source_path;
};

std::int32_t pedia_xml_integer(const content::XmlNode &row, const char *name,
                                std::int32_t fallback) {
    const auto value = row_text(row, name);
    if (!value) return fallback;
    std::int32_t parsed = 0;
    const auto [end, error] = std::from_chars(value->data(),
        value->data() + value->size(), parsed);
    if (error != std::errc{} || end != value->data() + value->size())
        throw std::runtime_error(std::string("Invalid active BuildingClasses integer: ") + name);
    return parsed;
}

std::map<std::string, PediaBuildingClass> read_pedia_building_classes(
    const assets::ResourceMountTable &resources) {
    std::map<std::string, PediaBuildingClass> result;
    // Preserve the same base -> Expansion -> Expansion2 selection order as
    // the article loader. The selected supplemental files add BNW classes.
    for (const auto *key : {
            "Assets/Gameplay/XML/Buildings/CIV5BuildingClasses.xml",
            "Assets/DLC/Expansion/Gameplay/XML/Buildings/CIV5BuildingClasses.xml",
            "Assets/DLC/Expansion/Gameplay/XML/Buildings/CIV5BuildingClasses_Expansion.xml",
            "Assets/DLC/Expansion2/Gameplay/XML/Buildings/CIV5BuildingClasses.xml",
            "Assets/DLC/Expansion2/Gameplay/XML/Buildings/CIV5BuildingClasses_Expansion2.xml",
            "Assets/DLC/Expansion2/Gameplay/XML/Buildings/CIV5BuildingClasses_Inherited_Expansion2.xml"}) {
        const auto mounted = resources.resolve(assets::AssetKey(key));
        const auto *handle = std::get_if<assets::AssetHandle>(&mounted.asset);
        if (!handle) {
            if (!std::holds_alternative<std::monostate>(mounted.asset))
                throw std::runtime_error("Ambiguous active BuildingClasses XML: " +
                                         std::string(key));
            continue;
        }
        const auto bytes = handle->read_bytes(64ULL * 1024 * 1024);
        const auto xml = content::read_xml_metadata(std::string(bytes.begin(), bytes.end()));
        if (xml.name != "GameData")
            throw std::runtime_error("Invalid active BuildingClasses XML: " +
                                     std::string(key));
        const auto *rows = xml.child("BuildingClasses");
        if (!rows) continue;
        const auto path = selected_path(*handle);
        for (const auto &row : rows->children) {
            if (row.name != "Row" && row.name != "Replace" &&
                row.name != "InsertOrAbort") continue;
            const auto type = row_text(row, "Type");
            if (!type || (row.name == "InsertOrAbort" && result.contains(*type)))
                continue;
            result[*type] = {
                pedia_xml_integer(row, "MaxGlobalInstances", -1),
                pedia_xml_integer(row, "MaxPlayerInstances", -1),
                pedia_xml_integer(row, "MaxTeamInstances", -1), path};
        }
    }
    return result;
}

constexpr std::size_t pedia_page_size = 12;

std::size_t utf8_next(std::string_view value, std::size_t at) {
    if (at >= value.size()) return value.size();
    ++at;
    while (at < value.size() &&
           (static_cast<unsigned char>(value[at]) & 0xc0) == 0x80) ++at;
    return at;
}

std::vector<std::string> pedia_article_lines(std::string_view body,
                                             std::size_t columns) {
    std::vector<std::string> lines;
    if (!columns) return lines;
    std::size_t paragraph = 0;
    while (paragraph <= body.size()) {
        const auto next_paragraph = body.find('\n', paragraph);
        const auto end = next_paragraph == std::string_view::npos ?
            body.size() : next_paragraph;
        if (paragraph == end) lines.emplace_back();
        while (paragraph < end) {
            auto at = paragraph;
            std::size_t count = 0, last_space = std::string_view::npos;
            while (at < end && count < columns) {
                if (body[at] == ' ') last_space = at;
                at = utf8_next(body, at);
                ++count;
            }
            if (at < end && last_space != std::string_view::npos &&
                last_space > paragraph) at = last_space;
            lines.emplace_back(body.substr(paragraph, at - paragraph));
            paragraph = at;
            while (paragraph < end && body[paragraph] == ' ') ++paragraph;
        }
        if (next_paragraph == std::string_view::npos) break;
        paragraph = next_paragraph + 1;
    }
    return lines;
}

const SystemPediaEntry *selected_pedia_article(const SystemScreenFrame &state) {
    if (!state.selected_pedia_id) return nullptr;
    const auto found = std::find_if(state.pedia_entries.begin(), state.pedia_entries.end(),
        [&](const auto &entry) { return entry.id == *state.selected_pedia_id; });
    return found == state.pedia_entries.end() ? nullptr : &*found;
}

std::size_t pedia_article_max_scroll(const SystemScreenFrame &state) {
    const auto *article = selected_pedia_article(state);
    if (!article || !state.pedia_article_layout.wrap_columns ||
        !state.pedia_article_layout.visible_lines) return 0;
    const auto count = pedia_article_lines(article->description,
        state.pedia_article_layout.wrap_columns).size();
    return count > state.pedia_article_layout.visible_lines ?
        count - state.pedia_article_layout.visible_lines : 0;
}

std::string fold_pedia_utf8(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        const auto byte = static_cast<unsigned char>(text[i]);
        if (byte >= 'A' && byte <= 'Z') result.push_back(static_cast<char>(byte + 32));
        else if (byte == 0xd0 && i + 1 < text.size()) {
            const auto next = static_cast<unsigned char>(text[i + 1]);
            if (next == 0x81) { result += "\xd1\x91"; ++i; }
            else if (next >= 0x90 && next <= 0x9f) {
                result.push_back(static_cast<char>(0xd0));
                result.push_back(static_cast<char>(next + 0x20)); ++i;
            } else if (next >= 0xa0 && next <= 0xaf) {
                result.push_back(static_cast<char>(0xd1));
                result.push_back(static_cast<char>(next - 0x20)); ++i;
            } else result.push_back(text[i]);
        } else result.push_back(text[i]);
    }
    return result;
}

std::string trim_pedia_search(std::string_view input) {
    const auto first = input.find_first_not_of(" \t\r\n");
    if (first == input.npos) return {};
    const auto last = input.find_last_not_of(" \t\r\n");
    return std::string(input.substr(first, last - first + 1));
}

bool valid_pedia_search(std::string_view input) {
    std::size_t characters = 0;
    for (const auto ch : input)
        if ((static_cast<unsigned char>(ch) & 0xc0) != 0x80) ++characters;
    return characters <= 50;
}

// Only tabs whose category can be distinguished in the current selected
// article catalog are routable. Beliefs still need selected GameInfo proof.
bool pedia_tab_matches(std::uint8_t tab, std::string_view category) {
    switch (tab) {
    case 3: return category == "Technologies";
    case 4: return category == "Units";
    case 6: return category == "Buildings";
    case 7: return category == "Wonders" || category == "Projects";
    case 8: return category == "Policies";
    case 12: return category == "Terrains" || category == "Features";
    case 13: return category == "Resources";
    case 14: return category == "Improvements";
    case 15: return category == "Religions";
    case 16: return category == "Resolutions" || category == "LeagueProjects";
    default: return false;
    }
}

std::uint8_t pedia_tab_for(std::string_view category) {
    for (const std::uint8_t tab : {3, 4, 6, 7, 8, 12, 13, 14, 15, 16})
        if (pedia_tab_matches(tab, category)) return tab;
    return 0;
}

bool pedia_tab_available(const SystemScreenFrame &state, std::uint8_t tab) {
    return std::any_of(state.pedia_entries.begin(), state.pedia_entries.end(),
        [tab](const auto &entry) { return pedia_tab_matches(tab, entry.category); });
}

std::vector<std::size_t> pedia_matches(const SystemScreenFrame &state) {
    std::vector<std::size_t> result;
    const auto query = fold_pedia_utf8(trim_pedia_search(state.pedia_search));
    for (std::size_t i = 0; i < state.pedia_entries.size(); ++i)
        if ((!state.pedia_category_tab ||
             pedia_tab_matches(state.pedia_category_tab, state.pedia_entries[i].category)) &&
            (query.empty() || fold_pedia_utf8(state.pedia_entries[i].title).find(query) !=
                                 std::string::npos))
            result.push_back(i);
    return result;
}

bool has_pedia_article(const SystemScreenFrame &state, std::string_view id) {
    return std::any_of(state.pedia_entries.begin(), state.pedia_entries.end(),
        [id](const auto &entry) { return entry.id == id; });
}

} // namespace

UiCommand civilopedia_search_command(std::string_view query) {
    return UiGameplayCommand::make("CivilopediaScreen",
        SystemPediaAction{SystemPediaActionKind::search, std::string(query)});
}

SystemPediaArticleLayout civilopedia_article_layout(std::int32_t width,
                                                     std::int32_t height) {
    if (width <= 0 || height <= 80) return {};
    const auto panel_width = std::min(1024, width);
    const auto panel_height = height - 80;
    const auto list_height = panel_height - 84;
    const auto article_width = panel_width - 274 - 66;
    // Reserve a row below the article for the authored PrereqTech button.
    const auto body_height = list_height - 102;
    if (article_width <= 32 || body_height <= 0) return {};
    // TwCenMT18 uses the 20-pixel GlyphGen path. A conservative codepoint
    // width keeps EN/RU glyphs in the body bounds until exact font metrics
    // become part of the layout contract.
    return {static_cast<std::size_t>(std::max(1, (article_width - 64) / 20)),
            static_cast<std::size_t>(std::max(1, body_height / 20))};
}

std::size_t civilopedia_article_max_scroll(const SystemScreenFrame &state) {
    return pedia_article_max_scroll(state);
}

UiCommandResult apply_civilopedia_action(SystemScreenFrame &state,
    const SystemPediaAction &action) {
    if (state.pedia_entries.empty())
        return {UiCommandStatus::unavailable, "Civilopedia article data is unavailable"};
    switch (action.kind) {
    case SystemPediaActionKind::follow_link: {
        const auto *article = selected_pedia_article(state);
        if (!article || std::find(article->prerequisite_tech_ids.begin(),
                article->prerequisite_tech_ids.end(), action.article_id) ==
                article->prerequisite_tech_ids.end())
            return {UiCommandStatus::unavailable, "Civilopedia link is unavailable"};
        [[fallthrough]];
    }
    case SystemPediaActionKind::select_article:
        if (!has_pedia_article(state, action.article_id))
            return {UiCommandStatus::unavailable, "Civilopedia article is unavailable"};
        if (state.selected_pedia_id != action.article_id) {
            if (state.pedia_history_index + 1 < state.pedia_history.size())
                state.pedia_history.resize(state.pedia_history_index + 1);
            state.pedia_history.push_back(action.article_id);
            state.pedia_history_index = state.pedia_history.size() - 1;
            state.selected_pedia_id = action.article_id;
            state.pedia_article_first_line = 0;
        }
        if (const auto *selected = selected_pedia_article(state);
            selected && state.pedia_category_tab &&
            !pedia_tab_matches(state.pedia_category_tab, selected->category)) {
            state.pedia_category_tab = pedia_tab_for(selected->category);
            state.pedia_page = 0;
        }
        return {UiCommandStatus::performed, {}};
    case SystemPediaActionKind::previous_page:
        if (!state.pedia_page)
            return {UiCommandStatus::unavailable, "No previous Civilopedia page"};
        --state.pedia_page;
        return {UiCommandStatus::performed, {}};
    case SystemPediaActionKind::next_page:
        if (const auto matches = pedia_matches(state);
            matches.empty() ||
            state.pedia_page >= (matches.size() - 1) / pedia_page_size)
            return {UiCommandStatus::unavailable, "No next Civilopedia page"};
        ++state.pedia_page;
        return {UiCommandStatus::performed, {}};
    case SystemPediaActionKind::search:
        if (!valid_pedia_search(action.article_id))
            return {UiCommandStatus::invalid, "Civilopedia search exceeds authored 50 characters"};
        state.pedia_search = trim_pedia_search(action.article_id);
        state.pedia_category_tab = 0;
        state.pedia_page = 0;
        return {UiCommandStatus::performed, {}};
    case SystemPediaActionKind::submit_search: {
        const auto query = fold_pedia_utf8(trim_pedia_search(state.pedia_search));
        if (query.empty())
            return {UiCommandStatus::unavailable, "Civilopedia search query is empty"};
        const SystemPediaEntry *exact = nullptr, *partial = nullptr;
        std::size_t exact_count = 0, partial_count = 0;
        for (const auto &entry : state.pedia_entries) {
            const auto title = fold_pedia_utf8(entry.title);
            if (title == query) { exact = &entry; ++exact_count; }
            if (title.find(query) != std::string::npos) {
                partial = &entry;
                ++partial_count;
            }
        }
        const auto *target = exact_count == 1 ? exact :
            exact_count == 0 && partial_count == 1 ? partial : nullptr;
        if (!target)
            return {UiCommandStatus::unavailable,
                    exact_count || partial_count ?
                        "Civilopedia search has multiple authored matches" :
                        "Civilopedia search has no authored match"};
        const auto id = target->id;
        const auto tab = pedia_tab_for(target->category);
        const auto result = apply_civilopedia_action(state,
            {SystemPediaActionKind::select_article, id});
        if (result.status == UiCommandStatus::performed &&
            state.pedia_category_session_owned && tab) {
            state.pedia_category_tab = tab;
            state.pedia_page = 0;
        }
        return result;
    }
    case SystemPediaActionKind::history_back:
    case SystemPediaActionKind::history_forward: {
        const bool back = action.kind == SystemPediaActionKind::history_back;
        if (state.pedia_history.empty() ||
            state.pedia_history_index >= state.pedia_history.size() ||
            (back && state.pedia_history_index == 0) ||
            (!back && state.pedia_history_index + 1 >= state.pedia_history.size()))
            return {UiCommandStatus::unavailable, "No adjacent Civilopedia history"};
        const auto target = back ? state.pedia_history_index - 1 :
                                   state.pedia_history_index + 1;
        if (!has_pedia_article(state, state.pedia_history[target]))
            return {UiCommandStatus::unavailable, "Civilopedia history article is unavailable"};
        state.pedia_history_index = target;
        state.selected_pedia_id = state.pedia_history[target];
        if (const auto *selected = selected_pedia_article(state))
            state.pedia_category_tab = pedia_tab_for(selected->category);
        state.pedia_page = 0;
        state.pedia_article_first_line = 0;
        return {UiCommandStatus::performed, {}};
    }
    case SystemPediaActionKind::select_category: {
        std::uint8_t tab = 0;
        for (std::uint8_t candidate = 1; candidate <= 16; ++candidate)
            if (action.article_id == std::to_string(candidate)) tab = candidate;
        if (!tab || !pedia_tab_available(state, tab))
            return {UiCommandStatus::unavailable,
                    "Selected Civilopedia category has no supported article data"};
        if (state.pedia_category_tab == tab && state.pedia_search.empty())
            return {UiCommandStatus::unavailable, "Civilopedia category is already selected"};
        state.pedia_category_tab = tab;
        state.pedia_search.clear();
        state.pedia_page = 0;
        return {UiCommandStatus::performed, {}};
    }
    case SystemPediaActionKind::article_scroll_up:
    case SystemPediaActionKind::article_scroll_down: {
        if (!selected_pedia_article(state))
            return {UiCommandStatus::unavailable, "No selected Civilopedia article"};
        if (!state.pedia_article_layout.wrap_columns ||
            !state.pedia_article_layout.visible_lines)
            return {UiCommandStatus::unavailable, "Civilopedia scroll layout is unavailable"};
        const auto maximum = pedia_article_max_scroll(state);
        if (state.pedia_article_first_line > maximum)
            return {UiCommandStatus::invalid, "Civilopedia scroll position is stale"};
        if (action.kind == SystemPediaActionKind::article_scroll_up) {
            if (!state.pedia_article_first_line)
                return {UiCommandStatus::unavailable, "Already at the start of the article"};
            --state.pedia_article_first_line;
        } else {
            if (state.pedia_article_first_line >= maximum)
                return {UiCommandStatus::unavailable, "Already at the end of the article"};
            ++state.pedia_article_first_line;
        }
        return {UiCommandStatus::performed, {}};
    }
    }
    return {UiCommandStatus::invalid, "Unknown Civilopedia action"};
}

bool SystemScreenSource::has(std::string_view id) const {
    return std::any_of(controls.begin(), controls.end(),
                       [id](const auto &control) { return control.id == id; });
}

std::vector<SystemScreenSource> read_system_sources(
    const assets::ResourceMountTable &resources, const UiContextRegistry &registry) {
    std::vector<SystemScreenSource> result;
    for (const auto &context : registry.contexts()) {
        if (!is_system_owner(context)) continue;
        const auto key = context.source_key + ".xml";
        const auto mounted = resources.resolve(assets::AssetKey(key));
        SystemScreenSource source;
        source.context_id = context.id;
        source.source_key = key;
        const auto *handle = std::get_if<assets::AssetHandle>(&mounted.asset);
        if (!handle) {
            if (!std::holds_alternative<std::monostate>(mounted.asset))
                throw std::runtime_error("Ambiguous active BNW system XML: " + key);
            // Some authored LuaContexts point at absent files. Preserve the
            // registry entry and make the focused screen explicitly unavailable.
            source.source_available = false;
            result.push_back(std::move(source));
            continue;
        }
        const auto bytes = handle->read_bytes(64ULL * 1024 * 1024);
        const auto xml = content::read_xml_metadata(std::string(bytes.begin(), bytes.end()));
        if (xml.name != "Context")
            throw std::runtime_error("Invalid active BNW system XML: " + key);
        source.selected_source_path = selected_path(*handle);
        panel_from_xml(xml, source);
        collect(xml, source.controls);
        result.push_back(std::move(source));
    }
    return result;
}

std::vector<SystemPediaEntry> read_civilopedia_entries(
    const assets::ResourceMountTable &resources, const SystemLocalizer &localize) {
    std::vector<SystemPediaEntry> result;
    if (!localize) return result;
    const auto building_classes = read_pedia_building_classes(resources);
    for (const auto &[folder, file, table, category] : {
             std::tuple{"Technologies", "CIV5Technologies.xml", "Technologies", "Technologies"},
             std::tuple{"Units", "CIV5Units.xml", "Units", "Units"},
             std::tuple{"Buildings", "CIV5Buildings.xml", "Buildings", "Buildings"},
             std::tuple{"GameInfo", "CIV5Projects.xml", "Projects", "Projects"},
             std::tuple{"GameInfo", "CIV5Policies.xml", "Policies", "Policies"},
             std::tuple{"Terrain", "CIV5Improvements.xml", "Improvements", "Improvements"},
             std::tuple{"Religions", "Civ5Religions.xml", "Religions", "Religions"},
             std::tuple{"GameInfo", "CIV5Resolutions.xml", "Resolutions", "Resolutions"},
             std::tuple{"GameInfo", "CIV5Resolutions.xml", "LeagueProjects", "LeagueProjects"},
             std::tuple{"Terrain", "CIV5Resources.xml", "Resources", "Resources"},
             std::tuple{"Terrain", "CIV5Terrains.xml", "Terrains", "Terrains"},
             std::tuple{"Terrain", "CIV5Features.xml", "Features", "Features"}}) {
        std::map<std::string, SystemPediaEntry> layered;
        std::vector<std::string> keys;
        if (std::string_view(category) == "Projects") {
            // For the pinned BNW profile the EXE request CIV5Projects.xml
            // resolves to this Expansion2 file; its activated GameInfo
            // database has six rows and no base Utopia project.
            keys.push_back("Assets/DLC/Expansion2/Gameplay/XML/GameInfo/CIV5Projects.xml");
        } else {
            for (const auto *prefix : {"Assets/Gameplay/XML/",
                                       "Assets/DLC/Expansion/Gameplay/XML/",
                                       "Assets/DLC/Expansion2/Gameplay/XML/"}) {
                keys.push_back(std::string(prefix) + folder + "/" + file);
                if (std::string_view(category) != "Buildings") continue;
                if (std::string_view(prefix) == "Assets/DLC/Expansion/Gameplay/XML/")
                    keys.push_back(std::string(prefix) +
                        "Buildings/CIV5Buildings_Expansion.xml");
                if (std::string_view(prefix) == "Assets/DLC/Expansion2/Gameplay/XML/") {
                    keys.push_back(std::string(prefix) +
                        "Buildings/CIV5Buildings_Expansion2.xml");
                    keys.push_back(std::string(prefix) +
                        "Buildings/CIV5Buildings_Inherited_Expansion2.xml");
                }
            }
        }
        for (const auto &key : keys) {
            const auto mounted = resources.resolve(assets::AssetKey(key));
            const auto *handle = std::get_if<assets::AssetHandle>(&mounted.asset);
            if (!handle) {
                if (!std::holds_alternative<std::monostate>(mounted.asset))
                    throw std::runtime_error("Ambiguous active Civilopedia table: " + key);
                continue;
            }
            const auto bytes = handle->read_bytes(64ULL * 1024 * 1024);
            const auto xml = content::read_xml_metadata(std::string(bytes.begin(), bytes.end()));
            if (xml.name != "GameData")
                throw std::runtime_error("Invalid active Civilopedia table: " + key);
            const auto *rows = xml.child(table);
            if (!rows) continue;
            const auto path = selected_path(*handle);
            std::map<std::string, std::pair<std::string, std::string>> league_rewards;
            if (std::string_view(category) == "LeagueProjects") {
                const auto *reward_rows = xml.child("LeagueProjectRewards");
                if (!reward_rows) continue;
                for (const auto &reward : reward_rows->children) {
                    if (reward.name != "Row") continue;
                    const auto id = row_text(reward, "Type");
                    const auto title = row_text(reward, "Description");
                    const auto help = row_text(reward, "Help");
                    if (!id || !title || !help ||
                        !league_rewards.emplace(*id, std::pair{*title, *help}).second)
                        throw std::runtime_error("Invalid active LeagueProjectRewards row: " + key);
                }
            }
            for (const auto &row : rows->children) {
                if (row.name != "Row" &&
                    (std::string_view(category) != "Buildings" ||
                     (row.name != "Replace" && row.name != "InsertOrAbort")))
                    continue;
                const auto id = row_text(row, "Type");
                const auto shown = row_text(row, "ShowInPedia");
                if (!id) continue;
                // Active BNW CivilopediaScreen.lua PopulateList excludes the
                // pantheon sentinel from GameInfo.Religions on tab 15.
                if (std::string_view(category) == "Religions" &&
                    *id == "RELIGION_PANTHEON") continue;
                if (std::string_view(category) == "Projects" &&
                    (*id == "PROJECT_SS_COCKPIT" ||
                     *id == "PROJECT_SS_STASIS_CHAMBER" ||
                     *id == "PROJECT_SS_ENGINE" ||
                     *id == "PROJECT_SS_BOOSTER"))
                    continue; // projectsToIgnore in active CivilopediaScreen.lua
                if (row.name == "InsertOrAbort" && layered.contains(*id)) continue;
                if (row.name == "Replace") layered.erase(*id);
                if (shown && (*shown == "false" || *shown == "0")) {
                    layered.erase(*id);
                    continue;
                }
                const auto title_key = row_text(row, "Description");
                // World Congress Lua SelectArticle renders Resolution.Help;
                // it has no Civilopedia column in the active BNW table.
                const auto body_key = std::string_view(category) == "LeagueProjects" ?
                    std::optional<std::string>{} : row_text(row,
                        std::string_view(category) == "Resolutions" ? "Help" : "Civilopedia");
                if (!title_key || (std::string_view(category) != "LeagueProjects" &&
                                   !body_key)) continue;
                const auto title = localize(*title_key);
                const auto body = std::string_view(category) == "LeagueProjects" ?
                    pedia_league_project_summary(row, league_rewards, localize) :
                    localize(*body_key);
                if (!title || title->empty() || !body || body->empty() ||
                    *title == *title_key || (body_key && *body == *body_key)) continue;
                // Russian localization may include grammatical variants after '|'.
                // The first form is the authored visible article title.
                const auto title_form = title->substr(0, title->find('|'));
                SystemPediaEntry entry{*id, pedia_text(title_form), pedia_text(*body),
                                       category, path, {}, {}};
                if (std::string_view(category) == "Buildings") {
                    const auto class_id = row_text(row, "BuildingClass");
                    const auto found = class_id ? building_classes.find(*class_id) :
                        building_classes.end();
                    if (found == building_classes.end()) {
                        // Keep the article in the full search catalog, but
                        // never assign an unsupported category tab.
                        entry.category = "UnclassifiedBuildings";
                    } else {
                        const auto &limits = found->second;
                        const auto specialists = pedia_xml_integer(row, "SpecialistCount", 0);
                        if (limits.max_global > 0 ||
                            (limits.max_player == 1 && specialists == 0))
                            entry.category = "Wonders";
                        else if (limits.max_global < 0 && limits.max_team < 0 &&
                                 (limits.max_player != 1 || specialists > 0))
                            entry.category = "Buildings";
                        else entry.category = "UnclassifiedBuildings";
                        entry.category_source_path = limits.selected_source_path;
                    }
                }
                // The active BNW Civilopedia Lua turns these direct fields
                // into clickable PrereqTechInstance buttons.
                if (std::string_view(category) == "Units" ||
                    std::string_view(category) == "Buildings" ||
                    std::string_view(category) == "Projects")
                    if (const auto prerequisite = row_text(row,
                            std::string_view(category) == "Projects" ?
                                "TechPrereq" : "PrereqTech");
                        prerequisite && !prerequisite->empty())
                        entry.prerequisite_tech_ids.push_back(*prerequisite);
                layered[*id] = std::move(entry);
            }
        }
        for (auto &[id, entry] : layered) result.push_back(std::move(entry));
    }
    std::stable_sort(result.begin(), result.end(), [](const auto &a, const auto &b) {
        // Authored tab sections list wonders before projects and resolutions
        // before LeagueProjects, independently of their localized titles.
        const auto section = [](std::string_view category) {
            return category == "Wonders" ? std::string_view{"Wonders/0"} :
                   category == "Projects" ? std::string_view{"Wonders/1"} :
                   category == "Resolutions" ? std::string_view{"WorldCongress/0"} :
                   category == "LeagueProjects" ? std::string_view{"WorldCongress/1"} :
                   category;
        };
        if (a.category != b.category) return section(a.category) < section(b.category);
        return a.title < b.title;
    });
    std::set<std::string> selected_techs;
    for (const auto &entry : result)
        if (entry.category == "Technologies") selected_techs.insert(entry.id);
    for (auto &entry : result)
        std::erase_if(entry.prerequisite_tech_ids,
            [&](const std::string &id) { return !selected_techs.contains(id); });
    return result;
}

std::optional<UiCommand> system_control_command(std::string_view id,
    std::string_view control, const SystemScreenFrame *state) {
    const auto close = [&] { return UiCommand{UiCloseScreen{std::string(id)}}; };
    const auto open = [](std::string_view target) {
        return UiCommand{UiOpenScreen{std::string(target)}};
    };
    if (id == "GameMenu") {
        if (control == "ReturnButton") return close();
        if (control == "SaveGameButton") return open("SaveMenu");
        if (control == "LoadGameButton")
            return operation(id, SystemOperationKind::open_load_menu);
        if (control == "OptionsButton") return open("OptionsMenu_InGame");
        if (control == "DetailsButton")
            return operation(id, SystemOperationKind::view_game_details);
        if (control == "ModsButton")
            return operation(id, SystemOperationKind::view_active_mods);
        if (control == "MainMenuButton" || control == "ExitGameButton")
            return open("ExitConfirm");
        if (control == "RestartGameButton") return operation(id, SystemOperationKind::restart);
        if (control == "QuickSaveButton") return operation(id, SystemOperationKind::quick_save);
        if (control == "RetireButton") return operation(id, SystemOperationKind::retire);
    } else if (id == "SaveMenu" || id == "SaveMapMenu") {
        if (control == "BackButton" || control == "No") return close();
        if (control == "SaveMapButton" && id == "SaveMenu") return open("SaveMapMenu");
        if (control == "SaveButton")
            return operation(id, id == "SaveMenu" ? SystemOperationKind::save :
                             SystemOperationKind::save_map);
        if (control == "Delete") return operation(id, SystemOperationKind::delete_save,
            state && state->selected_save_id ? *state->selected_save_id : "");
        if (control == "Yes") return operation(id, SystemOperationKind::delete_save,
            state && state->selected_save_id ? *state->selected_save_id : "");
    } else if (id == "LoadMenu") {
        if (control == "BackButton" || control == "No") return close();
        if (control == "StartButton") return operation(id, SystemOperationKind::start_loaded_game,
            state && state->selected_save_id ? *state->selected_save_id : "");
        if (control == "Delete" || control == "Yes")
            return operation(id, SystemOperationKind::delete_save,
                state && state->selected_save_id ? *state->selected_save_id : "");
    } else if (id == "OptionsMenu_InGame") {
        if (control == "CancelButton") return close();
        if (control == "AcceptButton" || control == "ApplyResButton")
            return operation(id, SystemOperationKind::apply_options);
        if (control == "DefaultButton") return operation(id, SystemOperationKind::reset_options);
    } else if (id == "CivilopediaScreen") {
        if (control == "CloseButton") return close();
        for (std::uint8_t tab = 1; tab <= 16; ++tab)
            if (control == "CategoryButton" + std::to_string(tab))
                return UiGameplayCommand::make(std::string(id),
                    SystemPediaAction{SystemPediaActionKind::select_category,
                                      std::to_string(tab)});
        if (control == "SearchButton")
            return UiGameplayCommand::make(std::string(id),
                SystemPediaAction{SystemPediaActionKind::submit_search, {}});
        if (control.starts_with("PediaEntry:"))
            return UiGameplayCommand::make(std::string(id),
                SystemPediaAction{SystemPediaActionKind::select_article,
                                  std::string(control.substr(11))});
        if (control.starts_with("PediaLink:"))
            return UiGameplayCommand::make(std::string(id),
                SystemPediaAction{SystemPediaActionKind::follow_link,
                                  std::string(control.substr(10))});
        if (control.starts_with("PediaSearch:"))
            return civilopedia_search_command(control.substr(12));
        if (control == "PediaPrevious")
            return UiGameplayCommand::make(std::string(id),
                SystemPediaAction{SystemPediaActionKind::previous_page, {}});
        if (control == "PediaNext")
            return UiGameplayCommand::make(std::string(id),
                SystemPediaAction{SystemPediaActionKind::next_page, {}});
        if (control == "BackButton")
            return UiGameplayCommand::make(std::string(id),
                SystemPediaAction{SystemPediaActionKind::history_back, {}});
        if (control == "ForwardButton")
            return UiGameplayCommand::make(std::string(id),
                SystemPediaAction{SystemPediaActionKind::history_forward, {}});
        if (control == "PediaArticleUp" || control == "PediaArticleDown")
            return UiGameplayCommand::make(std::string(id),
                SystemPediaAction{control == "PediaArticleUp" ?
                    SystemPediaActionKind::article_scroll_up :
                    SystemPediaActionKind::article_scroll_down, {}});
    } else if (id == "EndGameMenu") {
        if (control == "DemographicsButton") return open("EndGameDemographics");
        if (control == "RankingButton") return open("Ranking");
        if (control == "ReplayButton") return open("EndGameReplay");
        if (control == "BeyondButton")
            return operation(id, SystemOperationKind::open_beyond_earth_store);
        if (control == "MainMenuButton") return open("ExitConfirm");
        if (control == "BackButton")
            return operation(id, SystemOperationKind::continue_after_end);
    } else if (id == "ExitConfirm") {
        if (control == "No") return close();
        if (control == "Yes") {
            if (state && state->pending_exit)
                return operation(id, *state->pending_exit);
            return operation(id, SystemOperationKind::exit_game);
        }
    } else if (id == "PlayerChange") {
        if (control == "ChangePasswordButton") return open("ChangePassword");
        if (control == "SaveButton") return open("SaveMenu");
        if (control == "MainMenuButton") return open("ExitConfirm");
        if (control == "ContinueButton")
            return operation(id, SystemOperationKind::switch_player);
    } else if (id == "ChangePassword") {
        if (control == "CancelButton") return close();
        if (control == "OKButton") return operation(id, SystemOperationKind::change_password);
    }
    return std::nullopt;
}

const WorldViewHit *SystemScreenPlan::hit(std::int32_t x, std::int32_t y) const {
    for (auto it = hits.rbegin(); it != hits.rend(); ++it) {
        const auto &r = it->bounds;
        if (x >= r.x && y >= r.y && x < r.x + r.width && y < r.y + r.height)
            return &*it;
    }
    return nullptr;
}

SystemScreenPlan plan_system_screen(const SystemScreenSource &source,
    const UiFrameSnapshot &frame, std::int32_t width, std::int32_t height,
    const SystemLocalizer &localize, const UiGameplayBackend *backend) {
    SystemScreenPlan plan;
    plan.context_id = source.context_id;
    plan.selected_source_path = source.selected_source_path;
    if (!source.source_available) {
        plan.unresolved.push_back("Authored system XML is unavailable: " + source.source_key);
        return plan;
    }
    if (width <= 0 || height <= 0) return plan;
    const bool pedia = source.context_id == "CivilopediaScreen";
    const auto panel_w = std::max(1, std::min(pedia ? 1024 : source.authored_width, width));
    const auto panel_h = std::max(1, std::min(pedia ? height - 80 : source.authored_height,
                                             height));
    const WorldViewRect panel{(width - panel_w) / 2, (height - panel_h) / 2,
                              panel_w, panel_h};
    plan.paints.push_back({WorldViewPaintKind::rectangle, panel,
                           source.panel_style, {}, {}, {}, 0xffffffff});
    const auto *state = frame.find<SystemScreenFrame>();
    const auto label = pedia ? display("TXT_KEY_CIVILOPEDIA", localize) :
                       source.context_id;
    plan.paints.push_back({WorldViewPaintKind::text,
        {panel.x + 20, panel.y + 15, panel.width - 40, 36}, {}, {}, label,
        "MenuTitleCaption", 0xffffffff});
    if (pedia) {
        const auto close = system_control_command(source.context_id, "CloseButton", state);
        if (close && source.has("CloseButton")) {
            const WorldViewRect rect{panel.x + panel.width - 92, panel.y + 14, 68, 30};
            plan.paints.push_back({WorldViewPaintKind::text, rect, {}, {}, "X",
                                   "TwCenMT18", 0xffffffff});
            plan.hits.push_back({rect, *close, true, {}});
        }
        // The active Expansion2 XML declares sixteen 46x50 CategoryButtons.
        // Show every authored tab; only categories backed by distinct selected
        // article rows receive an enabled typed command.
        for (std::uint8_t tab = 1; tab <= 16; ++tab) {
            const auto id = "CategoryButton" + std::to_string(tab);
            const auto control = std::find_if(source.controls.begin(), source.controls.end(),
                [&](const SystemControl &item) { return item.id == id; });
            if (control == source.controls.end()) continue;
            const WorldViewRect rect{panel.x + 38 + (tab - 1) * 48,
                                     panel.y + 53, 46, 50};
            if (!control->texture.empty())
                plan.paints.push_back({WorldViewPaintKind::texture, rect,
                    control->texture,
                    {0, state && state->pedia_category_tab == tab ? 50 : 0, 46, 50},
                    {}, {}, 0xffffffff});
            const auto command = system_control_command(source.context_id, id, state);
            if (!command) continue;
            const SystemGameplayBackend pedia_backend;
            const auto availability = pedia_backend.availability(
                std::get<UiGameplayCommand>(*command), frame);
            plan.hits.push_back({rect, *command,
                availability.status == UiCommandStatus::ready, availability.reason});
        }
        const WorldViewRect list{panel.x + 20, panel.y + 112, 274, panel.height - 136};
        const WorldViewRect article{list.x + list.width + 12, list.y,
                                    panel.width - list.width - 66, list.height};
        plan.paints.push_back({WorldViewPaintKind::rectangle, list,
                               "GridBlackIndent8", {}, {}, {}, 0xffffffff});
        plan.paints.push_back({WorldViewPaintKind::rectangle, article,
                               "GridBlackIndent8", {}, {}, {}, 0xffffffff});
        if (!state || state->pedia_entries.empty()) {
            plan.unresolved.push_back("Civilopedia article data is unavailable");
            plan.paints.push_back({WorldViewPaintKind::text,
                {list.x + 10, list.y + 12, list.width - 20, 40}, {}, {},
                "N/A", "TwCenMT18", 0xffffffff});
            return plan;
        }
        if (source.has("SearchEditBox")) {
            const WorldViewRect search_rect{panel.x + panel.width - 354,
                                            panel.y + 17, 190, 27};
            plan.search_edit_box = search_rect;
            plan.paints.push_back({WorldViewPaintKind::rectangle, search_rect,
                "GridBlackIndent8", {}, {}, {}, 0xffffffff});
            plan.paints.push_back({WorldViewPaintKind::text, search_rect, {}, {},
                state->pedia_search.empty() ? display("TXT_KEY_SEARCH_BUTTON", localize) :
                state->pedia_search, "TwCenMT16", 0xffffffff});
            if (source.has("SearchButton")) {
                const auto button = std::find_if(source.controls.begin(),
                    source.controls.end(), [](const SystemControl &control) {
                        return control.id == "SearchButton";
                    });
                const WorldViewRect button_rect{search_rect.x + search_rect.width + 8,
                                                panel.y + 12, 38, 38};
                plan.paints.push_back({WorldViewPaintKind::rectangle, button_rect,
                    {}, {}, {}, button->style, 0xffffffff});
                if (!button->texture.empty())
                    plan.paints.push_back({WorldViewPaintKind::texture,
                        {button_rect.x + 4, button_rect.y + 4, 30, 30},
                        button->texture, {}, {}, {}, 0xffffffff});
                const auto command = system_control_command(source.context_id,
                    "SearchButton", state);
                const SystemGameplayBackend pedia_backend;
                const auto availability = pedia_backend.availability(
                    std::get<UiGameplayCommand>(*command), frame);
                plan.hits.push_back({button_rect, *command,
                    availability.status == UiCommandStatus::ready,
                    availability.reason});
            }
        }
        for (const auto &[id, kind, x] : {
                std::tuple{"BackButton", SystemPediaActionKind::history_back, panel.x + 22},
                std::tuple{"ForwardButton", SystemPediaActionKind::history_forward, panel.x + 70}}) {
            if (!source.has(id)) continue;
            const WorldViewRect rect{x, panel.y + 18, 40, 27};
            const auto command = system_control_command(source.context_id, id, state);
            if (!command) continue;
            const SystemGameplayBackend pedia_backend;
            const auto availability = pedia_backend.availability(
                std::get<UiGameplayCommand>(*command), frame);
            plan.paints.push_back({WorldViewPaintKind::text, rect, {}, {},
                kind == SystemPediaActionKind::history_back ? "<" : ">",
                "TwCenMT16", 0xffffffff});
            plan.hits.push_back({rect, *command,
                availability.status == UiCommandStatus::ready, availability.reason});
        }
        const auto matches = pedia_matches(*state);
        const auto count = matches.size();
        const auto last_page = count ? (count - 1) / pedia_page_size : 0;
        const auto page = std::min(state->pedia_page, last_page);
        const auto first = page * pedia_page_size;
        const auto end = std::min(first + pedia_page_size, count);
        if (!count)
            plan.paints.push_back({WorldViewPaintKind::text,
                {list.x + 10, list.y + 12, list.width - 20, 32}, {}, {},
                "0", "TwCenMT18", 0xffffffff});
        std::string prior_category;
        std::int32_t y = list.y + 8;
        for (std::size_t index = first; index < end; ++index) {
            const auto &entry = state->pedia_entries[matches[index]];
            if (entry.category != prior_category) {
                prior_category = entry.category;
                const auto category_key = [&]() -> std::string_view {
                    if (entry.category == "Technologies") return "TXT_KEY_PEDIA_TECH_PAGE_LABEL";
                    if (entry.category == "Units") return "TXT_KEY_PEDIA_UNITS_PAGE_LABEL";
                    if (entry.category == "Buildings") return "TXT_KEY_PEDIA_BUILDINGS_PAGE_LABEL";
                    if (entry.category == "Wonders") return "TXT_KEY_PEDIA_WONDERS_PAGE_LABEL";
                    if (entry.category == "Projects") return "TXT_KEY_WONDER_SECTION_3";
                    if (entry.category == "Policies") return "TXT_KEY_PEDIA_POLICIES_PAGE_LABEL";
                    if (entry.category == "Improvements") return "TXT_KEY_PEDIA_IMPROVEMENTS_PAGE_LABEL";
                    if (entry.category == "Religions") return "TXT_KEY_PEDIA_BELIEFS_PAGE_LABEL";
                    if (entry.category == "Resolutions")
                        return "TXT_KEY_PEDIA_WORLD_CONGRESS_CATEGORY_1";
                    if (entry.category == "LeagueProjects")
                        return "TXT_KEY_PEDIA_WORLD_CONGRESS_CATEGORY_2";
                    if (entry.category == "Resources") return "TXT_KEY_PEDIA_RESOURCES_PAGE_LABEL";
                    if (entry.category == "Terrains" || entry.category == "Features")
                        return "TXT_KEY_PEDIA_TERRAIN_PAGE_LABEL";
                    return {};
                }();
                const auto category = category_key.empty() ? entry.category :
                    display(category_key, localize);
                plan.paints.push_back({WorldViewPaintKind::text,
                    {list.x + 10, y, list.width - 20, 22}, {}, {}, category,
                    "TwCenMT18", 0xffffffff});
                y += 24;
            }
            const WorldViewRect rect{list.x + 10, y, list.width - 20, 26};
            plan.paints.push_back({WorldViewPaintKind::text, rect, {}, {},
                entry.title, "TwCenMT16", 0xffffffff});
            const auto command = system_control_command(source.context_id,
                "PediaEntry:" + entry.id, state);
            if (command) plan.hits.push_back({rect, *command, true, {}});
            y += 28;
        }
        if (page > 0) {
            const WorldViewRect rect{list.x + 8, list.y + list.height - 36, 70, 28};
            plan.paints.push_back({WorldViewPaintKind::text, rect, {}, {},
                                   "<", "TwCenMT16", 0xffffffff});
            plan.hits.push_back({rect, *system_control_command(source.context_id,
                "PediaPrevious", state), true, {}});
        }
        if (page < last_page) {
            const WorldViewRect rect{list.x + list.width - 64,
                                     list.y + list.height - 36, 56, 28};
            plan.paints.push_back({WorldViewPaintKind::text, rect, {}, {},
                                   ">", "TwCenMT16", 0xffffffff});
            plan.hits.push_back({rect, *system_control_command(source.context_id,
                "PediaNext", state), true, {}});
        }
        if (state->selected_pedia_id) {
            const auto *found = selected_pedia_article(*state);
            if (!found)
                plan.unresolved.push_back("Selected Civilopedia article is unavailable");
            else {
                plan.paints.push_back({WorldViewPaintKind::text,
                    {article.x + 16, article.y + 12, article.width - 32, 38},
                    {}, {}, found->title, "TwCenMT24", 0xffffffff});
                const WorldViewRect body{article.x + 16, article.y + 58,
                    article.width - 64, article.height - 102};
                const auto layout = civilopedia_article_layout(width, height);
                if (!source.has("ScrollPanel") || !source.has("UpButton") ||
                    !source.has("DownButton") || !layout.wrap_columns ||
                    !layout.visible_lines || state->pedia_article_layout.wrap_columns !=
                    layout.wrap_columns || state->pedia_article_layout.visible_lines !=
                    layout.visible_lines) {
                    plan.unresolved.push_back("Selected Civilopedia scroll layout is unavailable");
                } else {
                    plan.article_scroll_panel = body;
                    const auto lines = pedia_article_lines(found->description,
                        layout.wrap_columns);
                    const auto maximum = lines.size() > layout.visible_lines ?
                        lines.size() - layout.visible_lines : 0;
                    const auto first_line = std::min(state->pedia_article_first_line,
                        maximum);
                    std::string visible_text;
                    bool first_visible_line = true;
                    for (std::size_t index = first_line;
                         index < std::min(lines.size(), first_line + layout.visible_lines);
                         ++index) {
                        if (!first_visible_line) visible_text += '\n';
                        visible_text += lines[index];
                        first_visible_line = false;
                    }
                    plan.paints.push_back({WorldViewPaintKind::text, body,
                        {}, {}, visible_text, "TwCenMT18", 0xffffffff});
                    for (const auto &[control, label, y] : {
                             std::tuple{"PediaArticleUp", "▲", body.y},
                             std::tuple{"PediaArticleDown", "▼",
                                        body.y + body.height - 28}}) {
                        const WorldViewRect rect{article.x + article.width - 40,
                            y, 24, 28};
                        const auto command = system_control_command(source.context_id,
                            control, state);
                        if (!command) continue;
                        const SystemGameplayBackend pedia_backend;
                        const auto available = pedia_backend.availability(
                            std::get<UiGameplayCommand>(*command), frame);
                        plan.paints.push_back({WorldViewPaintKind::text, rect,
                            {}, {}, label, "TwCenMT18", 0xffffffff});
                        plan.hits.push_back({rect, *command,
                            available.status == UiCommandStatus::ready,
                            available.reason});
                    }
                    if (!found->prerequisite_tech_ids.empty()) {
                        const auto &id = found->prerequisite_tech_ids.front();
                        const auto target = std::find_if(state->pedia_entries.begin(),
                            state->pedia_entries.end(), [&](const auto &entry) {
                                return entry.id == id && entry.category == "Technologies";
                            });
                        if (target != state->pedia_entries.end()) {
                            const WorldViewRect rect{article.x + 16,
                                article.y + article.height - 38, article.width - 64, 24};
                            plan.paints.push_back({WorldViewPaintKind::text, rect,
                                {}, {}, display("TXT_KEY_PEDIA_PREREQ_TECH_LABEL", localize) +
                                    " " + target->title, "TwCenMT16", 0xffffffff});
                            const auto command = system_control_command(source.context_id,
                                "PediaLink:" + id, state);
                            if (command) {
                                const SystemGameplayBackend pedia_backend;
                                const auto available = pedia_backend.availability(
                                    std::get<UiGameplayCommand>(*command), frame);
                                plan.hits.push_back({rect, *command,
                                    available.status == UiCommandStatus::ready,
                                    available.reason});
                            }
                        }
                    }
                }
            }
        }
        return plan;
    }
    std::int32_t y = panel.y + 64;
    for (const auto &control : source.controls) {
        // GameMenu.lua OnShowHide hides QuickSave for multiplayer or an
        // activated mod with DisableQuickSave=1. Only that UI owner can
        // publish the evaluated result for this frame.
        if (source.context_id == "GameMenu" &&
            control.id == "QuickSaveButton" &&
            (!state || state->game_menu_quick_save_visible != true)) continue;
        if (source.context_id == "GameMenu" &&
            control.id == "LoadGameButton" &&
            (!state || state->game_menu_load_game_visible != true)) continue;
        // The active GameMenu.lua hides this tab for an empty mod list.
        if (source.context_id == "GameMenu" && control.id == "ModsButton" &&
            (!state || !state->game_menu_active_mod_count ||
             *state->game_menu_active_mod_count == 0)) continue;
        // The selected EndGameMenu.xml authors this control hidden; the
        // active Lua reveals it only after a Technology victory on our team.
        if (source.context_id == "EndGameMenu" && control.id == "BeyondButton" &&
            (!state || state->end_game_technology_victory != true)) continue;
        const auto command = system_control_command(source.context_id, control.id, state);
        if (!command) continue;
        if (y + 36 > panel.y + panel.height - 24) {
            plan.unresolved.push_back("Additional authored controls require scroll layout");
            break;
        }
        const WorldViewRect rect{panel.x + 24, y, panel.width - 48, 34};
        const auto title = display(control.text_key.empty() ? control.id :
                                   control.text_key, localize);
        plan.paints.push_back({WorldViewPaintKind::rectangle, rect,
            control.texture.empty() ? control.style : control.texture, {}, {}, {},
            0xffffffff});
        plan.paints.push_back({WorldViewPaintKind::text, rect, {}, {}, title,
            control.style, 0xffffffff});
        bool enabled = true;
        std::string reason;
        if (const auto *gameplay = std::get_if<UiGameplayCommand>(&*command)) {
            const auto availability = backend ? backend->availability(*gameplay, frame) :
                UiCommandResult{UiCommandStatus::unavailable, "System backend is not connected"};
            enabled = availability.status == UiCommandStatus::ready;
            reason = availability.reason;
        }
        plan.hits.push_back({rect, *command, enabled, reason});
        y += 38;
    }
    if (source.context_id == "SaveMenu" || source.context_id == "LoadMenu") {
        if (!state || state->save_slots.empty())
            plan.unresolved.push_back("Live save-slot listing is unavailable");
        else
            plan.paints.push_back({WorldViewPaintKind::text,
                {panel.x + 24, panel.y + panel.height - 72, panel.width - 48, 30},
                {}, {}, std::to_string(state->save_slots.size()) + " saves",
                "TwCenMT18", 0xffffffff});
    } else if (source.context_id == "EndGameMenu" && state && state->end_game_text) {
        plan.paints.push_back({WorldViewPaintKind::text,
            {panel.x + 24, panel.y + panel.height - 72, panel.width - 48, 44},
            {}, {}, *state->end_game_text, "TwCenMT20", 0xffffffff});
    }
    if (source.context_id == "ExitConfirm" && (!state || !state->pending_exit))
        plan.unresolved.push_back("Pending exit intent is unavailable");
    return plan;
}

UiCommandResult SystemGameplayBackend::availability(const UiGameplayCommand &command,
                                                     const UiFrameSnapshot &frame) const {
    if (const auto *pedia = command.as<SystemPediaAction>()) {
        if (command.source_id() != "CivilopediaScreen")
            return {UiCommandStatus::invalid, "Civilopedia action has another source"};
        const auto *state = frame.find<SystemScreenFrame>();
        if (!state || state->pedia_entries.empty())
            return {UiCommandStatus::unavailable, "Civilopedia article data is unavailable"};
        if ((pedia->kind == SystemPediaActionKind::select_category ||
             pedia->kind == SystemPediaActionKind::submit_search) &&
            !state->pedia_category_session_owned)
            return {UiCommandStatus::unavailable,
                    "Civilopedia category state has no live session owner"};
        auto proposed = *state;
        const auto result = apply_civilopedia_action(proposed, *pedia);
        return result.status == UiCommandStatus::performed ?
            UiCommandResult{UiCommandStatus::ready, {}} : result;
    }
    const auto *op = command.as<SystemOperation>();
    if (!op) return {UiCommandStatus::invalid, "Not a system operation"};
    if (op->kind == SystemOperationKind::quick_save) {
        if (command.source_id() != "GameMenu")
            return {UiCommandStatus::invalid, "Quick Save action has another source"};
        const auto *state = frame.find<SystemScreenFrame>();
        if (!state || state->game_menu_quick_save_visible != true)
            return {UiCommandStatus::unavailable,
                    "Quick Save visibility has no active GameMenu owner"};
    }
    if (op->kind == SystemOperationKind::open_load_menu) {
        if (command.source_id() != "GameMenu")
            return {UiCommandStatus::invalid, "Load Game action has another source"};
        const auto *state = frame.find<SystemScreenFrame>();
        if (!state || state->game_menu_load_game_visible != true)
            return {UiCommandStatus::unavailable,
                    "Load Game visibility has no active GameMenu owner"};
        return {UiCommandStatus::unavailable,
                "Native LoadMenu popup transition backend is not connected"};
    }
    if (op->kind == SystemOperationKind::view_game_details) {
        if (command.source_id() != "GameMenu")
            return {UiCommandStatus::invalid, "Game details action has another source"};
        return {UiCommandStatus::unavailable,
                "Live game setup, victory and option details are not connected"};
    }
    if (op->kind == SystemOperationKind::view_active_mods) {
        if (command.source_id() != "GameMenu")
            return {UiCommandStatus::invalid, "Active mods action has another source"};
        const auto *state = frame.find<SystemScreenFrame>();
        if (!state || !state->game_menu_active_mod_count ||
            *state->game_menu_active_mod_count == 0)
            return {UiCommandStatus::unavailable, "No owned active mod list"};
        return {UiCommandStatus::unavailable,
                "Live activated mod details panel is not connected"};
    }
    if (op->kind == SystemOperationKind::open_beyond_earth_store) {
        const auto *state = frame.find<SystemScreenFrame>();
        if (command.source_id() != "EndGameMenu" || !state ||
            state->end_game_technology_victory != true)
            return {UiCommandStatus::unavailable,
                    "Active-team Technology victory is not confirmed"};
        return {UiCommandStatus::unavailable,
                "Steam store overlay backend is not connected"};
    }
    if (op->kind == SystemOperationKind::delete_save ||
        op->kind == SystemOperationKind::start_loaded_game) {
        const auto *state = frame.find<SystemScreenFrame>();
        if (op->target.empty() || !state ||
            std::none_of(state->save_slots.begin(), state->save_slots.end(),
                [&](const auto &slot) { return slot.id == op->target; }))
            return {UiCommandStatus::unavailable, "No selected live save slot"};
    }
    if (command.source_id() == "ExitConfirm") {
        const auto *state = frame.find<SystemScreenFrame>();
        if (!state || !state->pending_exit || *state->pending_exit != op->kind)
            return {UiCommandStatus::unavailable, "No matching pending exit confirmation"};
    }
    return {UiCommandStatus::unavailable, "System operation backend is not connected"};
}

UiCommandResult SystemGameplayBackend::execute(const UiGameplayCommand &command,
                                                const UiFrameSnapshot &frame) {
    if (command.as<SystemPediaAction>()) {
        const auto available = availability(command, frame);
        return available.status == UiCommandStatus::ready ?
            UiCommandResult{UiCommandStatus::performed, {}} : available;
    }
    return availability(command, frame);
}

UiFrameSnapshot SystemMenuSession::with_pedia_state(const UiFrameSnapshot &frame) const {
    SystemScreenFrame state;
    if (const auto *current = frame.find<SystemScreenFrame>()) state = *current;
    state.selected_pedia_id = selected_pedia_id_;
    state.pedia_page = pedia_page_;
    state.pedia_category_tab = pedia_category_tab_;
    state.pedia_category_session_owned = true;
    state.pedia_search = pedia_search_;
    state.pedia_history = pedia_history_;
    state.pedia_history_index = pedia_history_index_;
    state.pedia_article_first_line = pedia_article_first_line_;
    state.pending_exit = pending_exit_;
    return frame.with<SystemScreenFrame>(
        std::make_shared<const SystemScreenFrame>(std::move(state)));
}

UiCommandResult SystemMenuSession::activate(std::string_view context_id,
    std::string_view control_id, const UiFrameSnapshot &frame,
    UiScreenController &controller, UiGameplayBackend *backend) {
    if (controller.focused_screen() != context_id)
        return {UiCommandStatus::invalid, "System screen does not own focus"};
    if (context_id == "GameMenu" && control_id == "QuickSaveButton") {
        const auto *state = frame.find<SystemScreenFrame>();
        if (!state || state->game_menu_quick_save_visible != true)
            return {UiCommandStatus::unavailable,
                    "Quick Save is hidden or has no active GameMenu owner"};
    }
    if (context_id == "GameMenu" && control_id == "LoadGameButton") {
        const auto *state = frame.find<SystemScreenFrame>();
        if (!state || state->game_menu_load_game_visible != true)
            return {UiCommandStatus::unavailable,
                    "Load Game is hidden or has no active GameMenu owner"};
        return {UiCommandStatus::unavailable,
                "Native LoadMenu popup transition is not connected"};
    }
    const bool asks_exit = control_id == "MainMenuButton" ||
        (context_id == "GameMenu" && control_id == "ExitGameButton");
    if (asks_exit) {
        const auto intent = control_id == "ExitGameButton" ?
            SystemOperationKind::exit_game : SystemOperationKind::exit_to_main;
        const auto result = controller.dispatch(UiOpenScreen{"ExitConfirm"}, frame, backend);
        if (result.status == UiCommandStatus::navigation) pending_exit_ = intent;
        return result;
    }
    if (context_id == "ExitConfirm" && control_id == "No") {
        const auto result = controller.dispatch(UiCloseScreen{"ExitConfirm"}, frame, backend);
        if (result.status == UiCommandStatus::navigation) pending_exit_.reset();
        return result;
    }
    if (context_id == "ExitConfirm" && control_id == "Yes" && !pending_exit_)
        return {UiCommandStatus::unavailable, "No pending exit request"};
    SystemScreenFrame screen_state;
    if (const auto *current = frame.find<SystemScreenFrame>()) screen_state = *current;
    screen_state.pending_exit = pending_exit_;
    screen_state.selected_pedia_id = selected_pedia_id_;
    screen_state.pedia_page = pedia_page_;
    screen_state.pedia_category_tab = pedia_category_tab_;
    screen_state.pedia_category_session_owned = true;
    screen_state.pedia_search = pedia_search_;
    screen_state.pedia_history = pedia_history_;
    screen_state.pedia_history_index = pedia_history_index_;
    screen_state.pedia_article_first_line = pedia_article_first_line_;
    const auto command = system_control_command(context_id, control_id, &screen_state);
    if (!command) return {UiCommandStatus::invalid, "Unrouted system control"};
    const auto routed = frame.with<SystemScreenFrame>(
        std::make_shared<const SystemScreenFrame>(std::move(screen_state)));
    SystemGameplayBackend local_backend;
    const auto *gameplay = std::get_if<UiGameplayCommand>(&*command);
    const auto *pedia = gameplay ? gameplay->as<SystemPediaAction>() : nullptr;
    const auto result = controller.dispatch(*command, routed,
        pedia ? static_cast<UiGameplayBackend *>(&local_backend) : backend);
    if (pedia && result.status == UiCommandStatus::performed) {
        auto updated = *routed.find<SystemScreenFrame>();
        // The command was accepted against the immutable routed frame.
        if (apply_civilopedia_action(updated, *pedia).status == UiCommandStatus::performed) {
            selected_pedia_id_ = std::move(updated.selected_pedia_id);
            pedia_page_ = updated.pedia_page;
            pedia_category_tab_ = updated.pedia_category_tab;
            pedia_search_ = std::move(updated.pedia_search);
            pedia_history_ = std::move(updated.pedia_history);
            pedia_history_index_ = updated.pedia_history_index;
            pedia_article_first_line_ = updated.pedia_article_first_line;
        }
    }
    if (context_id == "ExitConfirm" && control_id == "Yes" &&
        result.status == UiCommandStatus::performed) pending_exit_.reset();
    return result;
}

} // namespace civ5::ui
