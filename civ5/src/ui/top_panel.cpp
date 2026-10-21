#include "civ5/ui/top_panel.hpp"
#include "civ5/content/manifest.hpp"
#include <algorithm>
#include <filesystem>
#include <charconv>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace civ5::ui {
namespace {
constexpr std::string_view top_panel_bar_texture = "assets\\UI\\Art\\WorldView\\TopPanelBar.dds";

TopPanelElement element(const content::XmlNode &node) {
    if (node.name.empty())
        throw std::runtime_error("TopPanel element has no name");
    TopPanelElement result{node.name, node.attributes};
    for (std::size_t i = 0; i < result.attributes.size(); ++i)
        for (std::size_t j = 0; j < i; ++j)
            if (result.attributes[i].first == result.attributes[j].first)
                throw std::runtime_error("Duplicate TopPanel attribute: " + result.attributes[i].first);
    return result;
}

bool has_attribute(const content::XmlNode &node, std::string_view key) {
    return std::any_of(node.attributes.begin(), node.attributes.end(),
                       [&](const auto &pair) { return pair.first == key; });
}

template <class Predicate>
const content::XmlNode &exactly_one(const std::vector<content::XmlNode> &children, std::string_view description,
                                    Predicate predicate) {
    const content::XmlNode *found = nullptr;
    std::size_t count = 0;
    for (const auto &child : children) {
        if (!predicate(child))
            continue;
        found = &child;
        ++count;
    }
    if (count != 1)
        throw std::runtime_error("Civ V TopPanel must contain exactly one " + std::string(description) +
                                 ", found " + std::to_string(count));
    return *found;
}

TopPanelStack stack(const content::XmlNode &node) {
    TopPanelStack result;
    result.element = element(node);
    result.children.reserve(node.children.size());
    for (const auto &child : node.children)
        result.children.push_back(element(child));
    return result;
}

std::string upper_ui_locale(std::string value) {
    // Retail TopPanel.lua calls Locale.ToUpper. The active EN/RU catalogs use
    // ASCII and Cyrillic; preserve other UTF-8 sequences byte-for-byte.
    for (std::size_t i = 0; i < value.size(); ++i) {
        const auto first = static_cast<unsigned char>(value[i]);
        if (first >= 'a' && first <= 'z') {
            value[i] = static_cast<char>(first - ('a' - 'A'));
            continue;
        }
        if (i + 1 >= value.size())
            continue;
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

std::int32_t integer(std::string_view value, std::string_view description) {
    std::int32_t result = 0;
    const auto *begin = value.data();
    const auto *end = begin + value.size();
    const auto parsed = std::from_chars(begin, end, result);
    if (parsed.ec != std::errc{} || parsed.ptr != end)
        throw std::runtime_error("Invalid TopPanel " + std::string(description) + ": " + std::string(value));
    return result;
}

std::int32_t nonnegative_integer(std::string_view value, std::string_view description) {
    const auto result = integer(value, description);
    if (result < 0)
        throw std::runtime_error("Negative TopPanel " + std::string(description) + ": " + std::string(value));
    return result;
}

std::pair<std::int32_t, std::int32_t> stack_offset(std::string_view value) {
    const auto comma = value.find(',');
    if (comma == std::string_view::npos || comma == 0 || comma + 1 >= value.size())
        throw std::runtime_error("TopPanel Stack Offset is not a pair: " + std::string(value));
    return {integer(value.substr(0, comma), "Stack Offset"),
            integer(value.substr(comma + 1), "Stack Offset")};
}

std::pair<std::int32_t, std::int32_t> element_pair(std::string_view value, char separator,
                                                   std::string_view description) {
    const auto split = value.find(separator);
    if (split == std::string_view::npos || split == 0 || split + 1 >= value.size())
        throw std::runtime_error("TopPanel " + std::string(description) + " is not a pair: " + std::string(value));
    return {integer(value.substr(0, split), description), integer(value.substr(split + 1), description)};
}

TopPanelGrid9Rect pair_attribute(const content::XmlNode &node, std::string_view key) {
    const auto value = node.attribute(std::string(key));
    const auto comma = value.find(',');
    if (comma == std::string::npos || comma == 0 || comma + 1 >= value.size())
        throw std::runtime_error("TopPanel Grid9 attribute is not a pair: " + std::string(key));
    const auto x = integer(std::string_view(value).substr(0, comma), key);
    const auto y = integer(std::string_view(value).substr(comma + 1), key);
    if (x < 0 || y < 0)
        throw std::runtime_error("TopPanel Grid9 attribute is negative: " + std::string(key));
    return {x, y, 0, 0};
}

TopPanelGrid9Rect size_start(const content::XmlNode &node, std::string_view size_key,
                             std::string_view start_key) {
    const auto size = pair_attribute(node, size_key);
    const auto start = pair_attribute(node, start_key);
    return {start.x, start.y, size.x, size.y};
}

constexpr std::array<std::pair<std::string_view, std::string_view>, 9> grid9_attributes{
    std::pair{"ULSize", "ULTexStart"}, std::pair{"UCSize", "UCTexStart"},
    std::pair{"URSize", "URTexStart"}, std::pair{"LSize", "LTexStart"},
    std::pair{"CSize", "CTexStart"}, std::pair{"RSize", "RTexStart"},
    std::pair{"LLSize", "LLTexStart"}, std::pair{"LCSize", "LCTexStart"},
    std::pair{"LRSize", "LRTexStart"}};

std::string localized(const TopPanelTextLocalizer &localizer, std::string_view key,
                      std::vector<std::string> arguments = {}) {
    if (!localizer.text)
        throw std::runtime_error("TopPanel presentation requires a bound localization callback");
    return localizer.text(key, arguments);
}

std::string color(int red, int green, int blue, std::string value) {
    return "[COLOR:" + std::to_string(red) + ":" + std::to_string(green) + ":" + std::to_string(blue) +
           ":255]" + std::move(value) + "[/COLOR]";
}

std::string science(const TopPanelState &state, const TopPanelPlayerState &player,
                    const TopPanelTextLocalizer &localizer) {
    if (state.no_science)
        return localized(localizer, "TXT_KEY_TOP_PANEL_SCIENCE_OFF");
    std::string value;
    if (player.science_per_turn <= 0)
        value = color(255, 60, 60, localized(localizer, "TXT_KEY_NO_SCIENCE"));
    else if (static_cast<std::int64_t>(player.gold) + player.gold_per_turn < 0)
        value = color(255, 60, 0, "+" + std::to_string(player.science_per_turn));
    else
        value = color(33, 190, 247, "+" + std::to_string(player.science_per_turn));
    return "[ICON_RESEARCH]" + value;
}

std::string happiness(const TopPanelState &state, const TopPanelPlayerState &player,
                      const TopPanelTextLocalizer &localizer) {
    if (state.no_happiness)
        return localized(localizer, "TXT_KEY_TOP_PANEL_HAPPINESS_OFF");
    if (!player.empire_unhappy)
        return "[ICON_HAPPINESS_1]" + color(60, 255, 60, std::to_string(player.excess_happiness));
    if (player.empire_very_unhappy)
        return "[ICON_HAPPINESS_4]" + color(255, 60, 60, std::to_string(-static_cast<std::int64_t>(player.excess_happiness)));
    return "[ICON_HAPPINESS_3]" + color(255, 60, 60, std::to_string(-static_cast<std::int64_t>(player.excess_happiness)));
}

std::string golden_age(const TopPanelState &state, const TopPanelPlayerState &player,
                      const TopPanelTextLocalizer &localizer) {
    if (state.no_happiness)
        return localized(localizer, "TXT_KEY_TOP_PANEL_GOLDEN_AGES_OFF");
    std::string value;
    if (player.golden_age_turns > 0)
        value = upper_ui_locale(localized(localizer,
            state.expansion2_ui && player.golden_age_tourism_modifier > 0
                ? "TXT_KEY_UNIQUE_GOLDEN_AGE_ANNOUNCE" : "TXT_KEY_GOLDEN_AGE_ANNOUNCE")) +
                " (" + std::to_string(player.golden_age_turns) + ")";
    else
        value = std::to_string(player.golden_age_progress) + "/" + std::to_string(player.golden_age_threshold);
    return "[ICON_GOLDEN_AGE]" + color(255, 255, 255, std::move(value));
}

std::string culture(const TopPanelState &state, const TopPanelPlayerState &player,
                    const TopPanelTextLocalizer &localizer) {
    if (state.no_policies)
        return localized(localizer, "TXT_KEY_TOP_PANEL_POLICIES_OFF");
    std::string value = std::to_string(player.culture);
    if (player.next_policy_cost > 0)
        value += "/" + std::to_string(player.next_policy_cost);
    value += " (+" + std::to_string(player.culture_per_turn) + ")";
    return "[ICON_CULTURE]" + color(255, 0, 255, std::move(value));
}

std::string hundredths(int value);

std::string happiness_tooltip(const TopPanelState &state, const TopPanelPlayerState &player,
                              const TopPanelTextLocalizer &localizer) {
    if (state.no_happiness)
        return localized(localizer, "TXT_KEY_TOP_PANEL_HAPPINESS_OFF_TOOLTIP");
    std::string result;
    if (!player.empire_unhappy)
        result = localized(localizer, "TXT_KEY_TP_TOTAL_HAPPINESS",
                           {std::to_string(player.excess_happiness)});
    else {
        const auto icon = player.empire_very_unhappy ? "[ICON_HAPPINESS_4]" : "[ICON_HAPPINESS_3]";
        result = localized(localizer, "TXT_KEY_TP_TOTAL_UNHAPPINESS",
                           {icon, std::to_string(-static_cast<std::int64_t>(player.excess_happiness))});
    }
    if (player.empire_very_unhappy) {
        if (player.empire_super_unhappy)
            result += "[NEWLINE][NEWLINE][COLOR:255:60:60:255]" +
                      localized(localizer, "TXT_KEY_TP_EMPIRE_SUPER_UNHAPPY") + "[/COLOR]";
        result += "[NEWLINE][NEWLINE][COLOR:255:60:60:255]" +
                  localized(localizer, "TXT_KEY_TP_EMPIRE_VERY_UNHAPPY") + "[/COLOR]";
    } else if (player.empire_unhappy) {
        result += "[NEWLINE][NEWLINE][COLOR:255:60:60:255]" +
                  localized(localizer, "TXT_KEY_TP_EMPIRE_UNHAPPY") + "[/COLOR]";
    }

    const auto total_happiness = player.total_happiness != 0
                                     ? player.total_happiness
                                     : player.happiness_from_policies + player.happiness_from_resources +
                                           player.happiness_from_buildings + player.happiness_from_garrisoned_units +
                                           player.happiness_from_minors + player.happiness_from_handicap +
                                           player.happiness_from_trade_routes + player.happiness_from_religion +
                                           player.happiness_from_natural_wonders +
                                           player.extra_happiness_per_city * player.city_count;
    result += "[NEWLINE][NEWLINE][COLOR:150:255:150:255]" +
              localized(localizer, "TXT_KEY_TP_HAPPINESS_SOURCES", {std::to_string(total_happiness)});
    result += "[NEWLINE]  [ICON_BULLET]" +
              localized(localizer, "TXT_KEY_TP_HAPPINESS_FROM_RESOURCES",
                        {std::to_string(player.happiness_from_resources)});

    int resource_count = 0;
    int base_resource_happiness = 0;
    for (const auto &resource : player.strategic_resources) {
        if (resource.total <= 0 || resource.happiness == 0)
            continue;
        ++resource_count;
        base_resource_happiness += resource.happiness;
        result += "[NEWLINE]          +" +
                  localized(localizer, "TXT_KEY_TP_HAPPINESS_EACH_RESOURCE",
                            {std::to_string(resource.happiness), resource.icon_text_key,
                             resource.description_text_key.empty()
                                 ? resource.icon_text_key
                                 : localized(localizer, resource.description_text_key)});
    }
    if (player.happiness_from_resource_variety > 0)
        result += "[NEWLINE]          +" +
                  localized(localizer, "TXT_KEY_TP_HAPPINESS_RESOURCE_VARIETY",
                            {std::to_string(player.happiness_from_resource_variety)});
    if (player.extra_happiness_per_luxury >= 1)
        result += "[NEWLINE]          +" +
                  localized(localizer, "TXT_KEY_TP_HAPPINESS_EXTRA_PER_RESOURCE",
                            {std::to_string(player.extra_happiness_per_luxury), std::to_string(resource_count)});
    const auto other_resource_happiness = player.happiness_from_resources - base_resource_happiness -
                                          player.happiness_from_resource_variety -
                                          player.extra_happiness_per_luxury * resource_count;
    if (other_resource_happiness > 0)
        result += "[NEWLINE]          +" +
                  localized(localizer, "TXT_KEY_TP_HAPPINESS_OTHER_SOURCES",
                            {std::to_string(other_resource_happiness)});
    const auto append_happiness = [&](std::string_view key, int value, bool always = false) {
        if (!always && value == 0)
            return;
        result += "[NEWLINE]  [ICON_BULLET]" + localized(localizer, key, {std::to_string(value)});
    };
    append_happiness("TXT_KEY_TP_HAPPINESS_POLICIES", player.happiness_from_policies,
                     player.happiness_from_policies >= 0);
    append_happiness("TXT_KEY_TP_HAPPINESS_BUILDINGS", player.happiness_from_buildings, true);
    append_happiness("TXT_KEY_TP_HAPPINESS_GARRISONED_UNITS", player.happiness_from_garrisoned_units);
    append_happiness("TXT_KEY_TP_HAPPINESS_CONNECTED_CITIES", player.happiness_from_trade_routes);
    append_happiness("TXT_KEY_TP_HAPPINESS_STATE_RELIGION", player.happiness_from_religion);
    append_happiness("TXT_KEY_TP_HAPPINESS_NATURAL_WONDERS", player.happiness_from_natural_wonders);
    append_happiness("TXT_KEY_TP_HAPPINESS_CITY_COUNT",
                     player.extra_happiness_per_city * player.city_count);
    append_happiness("TXT_KEY_TP_HAPPINESS_CITY_STATE_FRIENDSHIP", player.happiness_from_minors);
    append_happiness("TXT_KEY_TP_HAPPINESS_DIFFICULTY_LEVEL", player.happiness_from_handicap, true);
    result += "[/COLOR]";

    const auto population_unhappiness = player.unhappiness_from_city_population_times100 -
                                         player.unhappiness_from_city_specialists_times100 -
                                         player.unhappiness_from_puppet_city_population_times100;
    result += "[NEWLINE][NEWLINE][COLOR:255:150:150:255]" +
              localized(localizer, "TXT_KEY_TP_UNHAPPINESS_TOTAL",
                        {std::to_string(player.total_unhappiness)}) +
              "[NEWLINE]  [ICON_BULLET]" +
              localized(localizer, "TXT_KEY_TP_UNHAPPINESS_CITY_COUNT",
                        {hundredths(player.unhappiness_from_city_count_times100)});
    if (player.unhappiness_from_captured_city_count_times100 != 0)
        result += "[NEWLINE]  [ICON_BULLET]" +
                  localized(localizer, "TXT_KEY_TP_UNHAPPINESS_CAPTURED_CITY_COUNT",
                            {hundredths(player.unhappiness_from_captured_city_count_times100)});
    result += "[NEWLINE]  [ICON_BULLET]" +
              localized(localizer, "TXT_KEY_TP_UNHAPPINESS_POPULATION", {hundredths(population_unhappiness)});
    if (player.unhappiness_from_puppet_city_population_times100 > 0)
        result += "[NEWLINE]  [ICON_BULLET]" +
                  localized(localizer, "TXT_KEY_TP_UNHAPPINESS_PUPPET_CITIES",
                            {hundredths(player.unhappiness_from_puppet_city_population_times100)});
    if (player.unhappiness_from_city_specialists_times100 > 0)
        result += "[NEWLINE]  [ICON_BULLET]" +
                  localized(localizer, "TXT_KEY_TP_UNHAPPINESS_SPECIALISTS",
                            {hundredths(player.unhappiness_from_city_specialists_times100)});
    if (player.unhappiness_from_occupied_cities_times100 != 0)
        result += "[NEWLINE]  [ICON_BULLET]" +
                  localized(localizer, "TXT_KEY_TP_UNHAPPINESS_OCCUPIED_POPULATION",
                            {hundredths(player.unhappiness_from_occupied_cities_times100)});
    if (player.unhappiness_from_units_times100 != 0)
        result += "[NEWLINE]  [ICON_BULLET]" +
                  localized(localizer, "TXT_KEY_TP_UNHAPPINESS_UNITS",
                            {hundredths(player.unhappiness_from_units_times100)});
    if (player.happiness_from_policies < 0)
        result += "[NEWLINE]  [ICON_BULLET]" +
                  localized(localizer, "TXT_KEY_TP_HAPPINESS_POLICIES",
                            {std::to_string(player.happiness_from_policies)});
    result += "[/COLOR]";
    if (!state.no_basic_help)
        result += "[NEWLINE][NEWLINE]" + localized(localizer, "TXT_KEY_TP_HAPPINESS_EXPLANATION");
    return result;
}

std::string resources(const TopPanelPlayerState &player, const TopPanelTextLocalizer &localizer) {
    std::string result;
    for (const auto &resource : player.strategic_resources) {
        if (!resource.revealed_and_city_tradeable && resource.used <= 0)
            continue;
        const auto icon = resource.icon_text_key.starts_with("[")
                              ? resource.icon_text_key
                              : localized(localizer, resource.icon_text_key);
        const auto value = std::to_string(resource.available) + " " + icon + "   ";
        if (resource.available > 0)
            result += "[COLOR_POSITIVE_TEXT]" + value + "[ENDCOLOR]";
        else if (resource.available < 0)
            result += "[COLOR_WARNING_TEXT]" + value + "[ENDCOLOR]";
        else
            result += value;
    }
    return result;
}

std::string golden_age_tooltip(const TopPanelState &state, const TopPanelPlayerState &player,
                               const TopPanelTextLocalizer &localizer) {
    if (state.no_happiness)
        return localized(localizer, "TXT_KEY_TOP_PANEL_HAPPINESS_OFF_TOOLTIP");
    std::string result;
    if (player.golden_age_turns > 0) {
        result = localized(localizer, "TXT_KEY_TP_GOLDEN_AGE_NOW", {std::to_string(player.golden_age_turns)});
    } else {
        result = localized(localizer, "TXT_KEY_TP_GOLDEN_AGE_PROGRESS",
                           {std::to_string(player.golden_age_progress),
                            std::to_string(player.golden_age_threshold)});
        result += "[NEWLINE]";
        if (player.excess_happiness >= 0)
            result += localized(localizer, "TXT_KEY_TP_GOLDEN_AGE_ADDITION",
                                {std::to_string(player.excess_happiness)});
        else
            result += "[COLOR_WARNING_TEXT]" +
                      localized(localizer, "TXT_KEY_TP_GOLDEN_AGE_LOSS",
                                {std::to_string(-static_cast<std::int64_t>(player.excess_happiness))}) +
                      "[ENDCOLOR]";
    }
    result += "[NEWLINE][NEWLINE]" + localized(localizer, "TXT_KEY_TP_GOLDEN_AGE_EFFECT");
    return result;
}

std::string culture_tooltip(const TopPanelState &state, const TopPanelPlayerState &player,
                            const TopPanelTextLocalizer &localizer) {
    if (state.no_policies)
        return localized(localizer, "TXT_KEY_TOP_PANEL_POLICIES_OFF_TOOLTIP");
    const auto needed = static_cast<std::int64_t>(player.next_policy_cost) - player.culture;
    std::string turns;
    if (needed <= 0)
        turns = "0";
    else if (player.culture_per_turn == 0)
        turns = "?";
    else
        turns = std::to_string((needed + player.culture_per_turn - 1) / player.culture_per_turn);
    std::string result = localized(localizer, "TXT_KEY_NEXT_POLICY_TURN_LABEL", {turns});
    if (!state.no_basic_help) {
        result += "[NEWLINE][NEWLINE]" +
                  localized(localizer, "TXT_KEY_TP_CULTURE_ACCUMULATED", {std::to_string(player.culture)}) +
                  "[NEWLINE]";
        if (player.next_policy_cost > 0)
            result += localized(localizer, "TXT_KEY_TP_CULTURE_NEXT_POLICY",
                                {std::to_string(player.next_policy_cost)});
    }
    bool first = true;
    const auto append_source = [&](std::string_view key, int value) {
        if (value == 0)
            return;
        if (first) {
            result += "[NEWLINE]";
            first = false;
        }
        result += "[NEWLINE]" + localized(localizer, key, {std::to_string(value)});
    };
    append_source("TXT_KEY_TP_CULTURE_FOR_FREE", player.culture_for_free);
    append_source("TXT_KEY_TP_CULTURE_FROM_CITIES", player.culture_from_cities);
    append_source("TXT_KEY_TP_CULTURE_FROM_HAPPINESS", player.culture_from_happiness);
    append_source("TXT_KEY_TP_CULTURE_FROM_MINORS", player.culture_from_minors);
    if (!state.no_basic_help)
        result += "[NEWLINE][NEWLINE]" +
                  localized(localizer, "TXT_KEY_TP_CULTURE_CITY_COST",
                            {std::to_string(player.num_cities_policy_cost_mod)});
    return result;
}

std::string resource_tooltip(const TopPanelPlayerState &player, const TopPanelTextLocalizer &localizer) {
    std::string result;
    bool first = true;
    for (const auto &resource : player.strategic_resources) {
        if (!resource.revealed_and_city_tradeable)
            continue;
        if (first)
            first = false;
        else
            result += "[NEWLINE][NEWLINE]";
        const auto icon = resource.icon_text_key.starts_with("[")
                              ? resource.icon_text_key
                              : localized(localizer, resource.icon_text_key);
        result += std::to_string(resource.available) + " " + icon;
        if (!resource.description_text_key.empty())
            result += " " + localized(localizer, resource.description_text_key);
        if (resource.used != 0 || resource.total != 0)
            result += ": " + localized(localizer, "TXT_KEY_TP_RESOURCE_INFO",
                                        {std::to_string(resource.total), std::to_string(resource.used)});
    }
    return result;
}

std::string hundredths(int value) {
    const auto signed_value = static_cast<std::int64_t>(value);
    const auto negative = signed_value < 0;
    const auto magnitude = negative ? -signed_value : signed_value;
    const auto whole = magnitude / 100;
    const auto fraction = magnitude % 100;
    std::string result = std::to_string(whole);
    if (fraction != 0) {
        result += '.';
        if (fraction < 10)
            result += '0';
        result += std::to_string(fraction);
        while (!result.empty() && result.back() == '0')
            result.pop_back();
    }
    return negative ? '-' + result : result;
}

void append_science_source(std::string &result, bool &first, const TopPanelTextLocalizer &localizer,
                           std::string_view key, int value, bool trailing_newline = false) {
    if (value == 0)
        return;
    if (!first)
        result += "[NEWLINE]";
    first = false;
    result += localized(localizer, key, {hundredths(value)});
    if (trailing_newline)
        result += "[NEWLINE]";
}

std::string science_tooltip(const TopPanelState &state, const TopPanelPlayerState &player,
                            const TopPanelTextLocalizer &localizer) {
    if (state.no_science)
        return localized(localizer, "TXT_KEY_TOP_PANEL_SCIENCE_OFF_TOOLTIP");
    std::string result;
    if (!state.no_basic_help) {
        result = localized(localizer, "TXT_KEY_TP_SCIENCE", {std::to_string(player.science_per_turn)});
        if (player.city_count > 0)
            result += "[NEWLINE][NEWLINE]";
    }
    bool first = true;
    append_science_source(result, first, localizer, "TXT_KEY_TP_SCIENCE_FROM_BUDGET_DEFICIT",
                          player.science_from_budget_deficit_times100, true);
    append_science_source(result, first, localizer, "TXT_KEY_TP_SCIENCE_FROM_CITIES",
                          player.science_from_cities_times100);
    append_science_source(result, first, localizer, "TXT_KEY_TP_SCIENCE_FROM_MINORS",
                          player.science_from_other_players_times100);
    append_science_source(result, first, localizer, "TXT_KEY_TP_SCIENCE_FROM_HAPPINESS",
                          player.science_from_happiness_times100);
    append_science_source(result, first, localizer, "TXT_KEY_TP_SCIENCE_FROM_RESEARCH_AGREEMENTS",
                          player.science_from_research_agreements_times100);
    return result;
}

std::string gold_tooltip(const TopPanelState &state, const TopPanelPlayerState &player,
                         const TopPanelTextLocalizer &localizer) {
    const auto diplomacy = static_cast<std::int64_t>(player.gold_per_turn_from_diplomacy);
    const auto diplomacy_income = std::max<std::int64_t>(diplomacy, 0);
    const auto diplomacy_expenses = std::max<std::int64_t>(-diplomacy, 0);
    const auto city_income = hundredths(player.gold_from_cities_times100);
    // Preserve Lua's division/addition order before math.floor, including
    // fractional city income carrying into the combined total.
    const auto city_connection = player.city_connection_gold_times100 / 100.0;
    const auto total_income = player.gold_from_cities_times100 / 100.0 + diplomacy_income + city_connection;
    const auto total_expenses = std::int64_t{player.unit_cost} + player.unit_supply + player.building_maintenance +
                                player.improvement_maintenance + diplomacy_expenses;
    std::string result;
    if (!state.no_basic_help) {
        result = localized(localizer, "TXT_KEY_TP_AVAILABLE_GOLD", {std::to_string(player.gold)});
        result += "[NEWLINE][NEWLINE]";
    }
    result += "[COLOR:150:255:150:255]+";
    result += localized(localizer, "TXT_KEY_TP_TOTAL_INCOME",
                        {std::to_string(static_cast<std::int64_t>(std::floor(total_income)))});
    result += "[NEWLINE]  [ICON_BULLET]" +
              localized(localizer, "TXT_KEY_TP_CITY_OUTPUT", {city_income});
    result += "[NEWLINE]  [ICON_BULLET]" +
              localized(localizer, "TXT_KEY_TP_GOLD_FROM_TR",
                         {std::to_string(static_cast<std::int64_t>(std::floor(city_connection)))});
    if (diplomacy_income > 0)
        result += "[NEWLINE]  [ICON_BULLET]" +
                  localized(localizer, "TXT_KEY_TP_GOLD_FROM_OTHERS", {std::to_string(diplomacy_income)});
    result += "[/COLOR]";
    result += "[NEWLINE][COLOR:255:150:150:255]";
    result += "[NEWLINE]-" + localized(localizer, "TXT_KEY_TP_TOTAL_EXPENSES", {std::to_string(total_expenses)});
    if (player.unit_cost != 0)
        result += "[NEWLINE]  [ICON_BULLET]" +
                  localized(localizer, "TXT_KEY_TP_UNIT_MAINT", {std::to_string(player.unit_cost)});
    if (player.unit_supply != 0)
        result += "[NEWLINE]  [ICON_BULLET]" +
                  localized(localizer, "TXT_KEY_TP_GOLD_UNIT_SUPPLY", {std::to_string(player.unit_supply)});
    if (player.building_maintenance != 0)
        result += "[NEWLINE]  [ICON_BULLET]" +
                  localized(localizer, "TXT_KEY_TP_GOLD_BUILDING_MAINT", {std::to_string(player.building_maintenance)});
    if (player.improvement_maintenance != 0)
        result += "[NEWLINE]  [ICON_BULLET]" + localized(
                  localizer, "TXT_KEY_TP_GOLD_TILE_MAINT", {std::to_string(player.improvement_maintenance)});
    if (diplomacy_expenses != 0)
        result += "[NEWLINE]  [ICON_BULLET]" +
                  localized(localizer, "TXT_KEY_TP_GOLD_TO_OTHERS", {std::to_string(diplomacy_expenses)});
    result += "[/COLOR]";
    // This is the authored test: income plus treasury, not income minus expenses.
    if (total_income + player.gold < 0)
        result += "[NEWLINE][COLOR:255:60:60:255]" +
                  localized(localizer, "TXT_KEY_TP_LOSING_SCIENCE_FROM_DEFICIT") + "[/COLOR]";
    if (!state.no_basic_help)
        result += "[NEWLINE][NEWLINE]" + localized(localizer, "TXT_KEY_TP_GOLD_EXPLANATION");
    return result;
}
} // namespace

std::string TopPanelElement::attribute(std::string_view key) const {
    for (const auto &[name, value] : attributes)
        if (name == key)
            return value;
    return {};
}

TopPanelLayout TopPanelLayout::from_xml(const content::XmlNode &root) {
    if (root.name != "Context")
        throw std::runtime_error("Civ V TopPanel root must be Context");
    const auto &center_bar = exactly_one(root.children, "centre TopPanelBar", [](const auto &node) {
        return node.name == "Image" && node.attribute("Texture") == top_panel_bar_texture &&
               node.attribute("Anchor") == "C,T";
    });
    const auto &right_bar = exactly_one(root.children, "right TopPanelBar", [](const auto &node) {
        return node.name == "Image" && node.attribute("Texture") == top_panel_bar_texture &&
               node.attribute("Anchor") == "R,T";
    });
    const auto &info = exactly_one(root.children, "TopPanelInfoStack", [](const auto &node) {
        return node.name == "Stack" && node.attribute("ID") == "TopPanelInfoStack";
    });
    const auto &right = exactly_one(root.children, "right TopPanel stack", [](const auto &node) {
        return node.name == "Stack" && node.attribute("Anchor") == "R,T" && !has_attribute(node, "ID");
    });
    const auto &tooltip = exactly_one(root.children, "TooltipTypeTopPanel", [](const auto &node) {
        return node.name == "ToolTipType" && node.attribute("Name") == "TooltipTypeTopPanel";
    });
    const auto &grid = exactly_one(tooltip.children, "TooltipTypeTopPanel Grid", [](const auto &node) {
        return node.name == "Grid" && node.attribute("ID") == "TopPanelMouseover";
    });
    const auto &label = exactly_one(grid.children, "TopPanelMouseover TooltipLabel", [](const auto &node) {
        return node.name == "Label" && node.attribute("ID") == "TooltipLabel";
    });
    return TopPanelLayout{element(root), element(center_bar), stack(info), element(right_bar), stack(right),
                          element(tooltip), element(grid), element(label)};
}

TopPanelLayout read_top_panel(const std::string &path) {
    return TopPanelLayout::from_xml(content::read_xml(std::filesystem::path(path)));
}

namespace {
content::XmlNode read_ui_xml(const assets::ResourceMountTable &resources, std::string_view key) {
    const auto mounted = resources.resolve(assets::AssetKey(std::string(key)));
    const auto *handle = std::get_if<assets::AssetHandle>(&mounted.asset);
    if (!handle)
        throw std::runtime_error("TopPanel VFS XML is missing or ambiguous: " + std::string(key));
    constexpr std::uint64_t maximum = 64ULL * 1024 * 1024;
    const auto bytes = handle->read_bytes(maximum);
    return content::read_xml_metadata(std::string(bytes.begin(), bytes.end()));
}

TopPanelGrid9Style grid9_style_from_document(const content::XmlNode &root, std::string_view style_name) {
    if (style_name.empty())
        throw std::runtime_error("TopPanel Grid9 style name is empty");
    const content::XmlNode *found = nullptr;
    for (const auto &child : root.children) {
        if (child.name != style_name)
            continue;
        if (found)
            throw std::runtime_error("TopPanel Grid9 style is duplicated: " + std::string(style_name));
        found = &child;
    }
    if (!found)
        throw std::runtime_error("TopPanel Grid9 style is missing: " + std::string(style_name));
    return TopPanelGrid9Style::from_xml(*found);
}
} // namespace

TopPanelLayout read_top_panel(const assets::ResourceMountTable &resources) {
    return TopPanelLayout::from_xml(read_ui_xml(resources, "Assets/UI/InGame/TopPanel.xml"));
}

TopPanelStackPlacement plan_top_panel_stack(const TopPanelStack &stack) {
    const auto offset = stack_offset(stack.element.attribute("Offset"));
    const auto padding = nonnegative_integer(stack.element.attribute("Padding"), "Stack Padding");
    const auto growth = stack.element.attribute("StackGrowth");
    if (growth == "Left")
        return {offset.first, offset.second, padding, TopPanelStackGrowth::left};
    if (growth == "Right")
        return {offset.first, offset.second, padding, TopPanelStackGrowth::right};
    throw std::runtime_error("TopPanel Stack has unsupported StackGrowth: " + growth);
}

TopPanelElementSize top_panel_element_size(const TopPanelElement &element) {
    const auto size = element_pair(element.attribute("Size"), '.', "element Size");
    if (size.first < 0 || size.second < 0)
        throw std::runtime_error("TopPanel element Size is negative: " + element.name);
    return {static_cast<std::uint32_t>(size.first), static_cast<std::uint32_t>(size.second)};
}

TopPanelPoint top_panel_element_offset(const TopPanelElement &element) {
    const auto offset = element_pair(element.attribute("Offset"), ',', "element Offset");
    return {offset.first, offset.second};
}

TopPanelPoint top_panel_element_texture_offset(const TopPanelElement &element) {
    const auto offset = element_pair(element.attribute("TextureOffset"), ',', "TextureOffset");
    return {offset.first, offset.second};
}

std::int32_t top_panel_element_integer(const TopPanelElement &element, std::string_view key) {
    const auto value = element.attribute(key);
    if (value.empty())
        throw std::runtime_error("TopPanel element integer is missing: " + std::string(key));
    return integer(value, key);
}

TopPanelGrid9Style TopPanelGrid9Style::from_xml(const content::XmlNode &node) {
    if (node.name.empty() || node.name == "StyleSheet")
        throw std::runtime_error("TopPanel Grid9 style node is invalid");
    TopPanelGrid9Style result;
    result.name = node.name;
    result.texture = node.attribute("Texture");
    if (result.texture.empty())
        throw std::runtime_error("TopPanel Grid9 style has no Texture: " + result.name);
    for (std::size_t i = 0; i < grid9_attributes.size(); ++i) {
        const auto &[size_key, start_key] = grid9_attributes[i];
        result.source[i] = size_start(node, size_key, start_key);
        if (result.source[i].width == 0 || result.source[i].height == 0)
            throw std::runtime_error("TopPanel Grid9 source rectangle is empty: " + result.name);
    }
    if (!node.attribute("StateOffsetIncrement").empty())
        result.state_offset = pair_attribute(node, "StateOffsetIncrement");
    return result;
}

TopPanelGrid9Style read_top_panel_grid9_style(const std::string &path, std::string_view style_name) {
    return grid9_style_from_document(content::read_xml(std::filesystem::path(path)), style_name);
}

TopPanelGrid9Style read_top_panel_grid9_style(const assets::ResourceMountTable &resources,
                                              std::string_view style_name) {
    return grid9_style_from_document(read_ui_xml(resources, "Assets/UI/Styles.xml"), style_name);
}

TopPanelLocalizationCatalog TopPanelLocalizationCatalog::read_directory(
    const std::string &directory, std::string_view language_root) {
    if (directory.empty() || language_root.empty())
        throw std::runtime_error("TopPanel localization directory/language is empty");
    std::vector<std::filesystem::path> files;
    for (const auto &entry : std::filesystem::recursive_directory_iterator(directory))
        if (entry.is_regular_file() && entry.path().extension() == ".xml")
            files.push_back(entry.path());
    std::sort(files.begin(), files.end());
    if (files.empty())
        throw std::runtime_error("TopPanel localization directory has no XML files");
    TopPanelLocalizationCatalog result;
    for (const auto &path : files) {
        const auto document = content::read_xml(path);
        if (document.name != "GameData")
            throw std::runtime_error("TopPanel localization root is not GameData: " + path.string());
        const content::XmlNode *language = nullptr;
        for (const auto &child : document.children)
            if (child.name == language_root) {
                if (language)
                    throw std::runtime_error("TopPanel localization language root is duplicated: " +
                                             std::string(language_root));
                language = &child;
            }
        if (!language)
            throw std::runtime_error("TopPanel localization language root is missing: " + path.string());
        for (const auto &row : language->children) {
            if (row.name != "Row")
                throw std::runtime_error("TopPanel localization has a non-Row child: " + path.string());
            const auto tag = row.attribute("Tag");
            const auto *text = row.child("Text");
            if (tag.empty() || !text || text->children.size() != 0)
                throw std::runtime_error("TopPanel localization row is malformed: " + path.string());
            if (!result.rows.emplace(tag, text->text).second)
                throw std::runtime_error("TopPanel localization tag is duplicated: " + tag);
        }
    }
    if (result.rows.empty())
        throw std::runtime_error("TopPanel localization catalog is empty");
    return result;
}

TopPanelLocalizationCatalog TopPanelLocalizationCatalog::read_resources(
    const assets::ResourceMountTable &resources, const std::vector<std::string> &keys,
    std::string_view language_root) {
    if (language_root.empty())
        throw std::runtime_error("TopPanel localization language is empty");
    if (keys.empty())
        throw std::runtime_error("TopPanel localization VFS key list is empty");
    TopPanelLocalizationCatalog result;
    constexpr std::uint64_t maximum = 64ULL * 1024 * 1024;
    for (const auto &key : keys) {
        const assets::AssetKey requested(key);
        const auto mounted = resources.resolve(requested);
        const auto *selected = std::get_if<assets::AssetHandle>(&mounted.asset);
        std::optional<assets::AssetHandle> exact;
        if (selected && selected->candidate().normalized_name == requested.normalized_name())
            exact = *selected;
        else if (key.find('/') != std::string::npos) {
            // Gameplay text is language-scoped by path. Native UI lookup by
            // basename can select another language with the same XML name.
            // The all-matching branch recovers the requested active source.
            for (const auto &candidate : resources.resolve_all(requested))
                if (const auto *handle = std::get_if<assets::AssetHandle>(&candidate.asset);
                    handle && handle->candidate().normalized_name == requested.normalized_name())
                    exact = *handle;
        } else if (selected) exact = *selected;
        if (!exact)
            throw std::runtime_error("TopPanel localization VFS XML is missing or ambiguous: " + key);
        const auto bytes = exact->read_bytes(maximum);
        const auto document = content::read_xml_metadata(std::string(bytes.begin(), bytes.end()));
        if (document.name != "GameData")
            throw std::runtime_error("TopPanel localization root is not GameData: " + key);
        const content::XmlNode *language = nullptr;
        for (const auto &child : document.children)
            if (child.name == language_root) {
                if (language)
                    throw std::runtime_error("TopPanel localization language root is duplicated: " +
                                             std::string(language_root));
                language = &child;
            }
        if (!language)
            throw std::runtime_error("TopPanel localization language root is missing: " + key);
        for (const auto &row : language->children) {
            if (row.name != "Row")
                throw std::runtime_error("TopPanel localization has a non-Row child: " + key);
            const auto tag = row.attribute("Tag");
            const auto *text = row.child("Text");
            if (tag.empty() || !text || !text->children.empty())
                throw std::runtime_error("TopPanel localization row is malformed: " + key);
            if (!result.rows.emplace(tag, text->text).second)
                throw std::runtime_error("TopPanel localization tag is duplicated: " + tag);
        }
    }
    if (result.rows.empty())
        throw std::runtime_error("TopPanel localization VFS catalog is empty");
    return result;
}

TopPanelLocalizationCatalog TopPanelLocalizationCatalog::read_active_bnw_resources(
    const assets::ResourceMountTable &resources, std::string_view language_root) {
    std::string language;
    if (language_root == "Language_en_US") language = "en_US";
    else if (language_root == "Language_RU_RU") language = "RU_RU";
    else throw std::runtime_error("TopPanel BNW active language is unsupported");
    const std::array<std::string, 3> prefixes = {
        "Assets/Gameplay/XML/NewText/" +
            (language_root == "Language_en_US" ? "EN_US" : language),
        "Assets/DLC/Expansion/Gameplay/XML/Text/" + language,
        "Assets/DLC/Expansion2/Gameplay/XML/Text/" + language};
    TopPanelLocalizationCatalog result;
    for (const auto &prefix : prefixes) {
        const auto keys = resources.keys(prefix);
        if (keys.empty())
            throw std::runtime_error("Active BNW localization layer is missing: " + prefix);
        for (const auto &key : keys) {
            const auto path = std::filesystem::path(key);
            auto extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (extension != ".xml" || key.find("/LeaderDialog/") != std::string::npos ||
                key.find("Scenario") != std::string::npos) continue;
            try {
                const auto layer = read_resources(resources, {key}, language_root);
                for (const auto &[tag, value] : layer.rows) result.rows[tag] = value;
            } catch (const std::runtime_error &error) {
                // A few authored XML documents have a language node but no
                // rows; they contribute nothing to this text layer.
                if (std::string_view(error.what()) != "TopPanel localization VFS catalog is empty") throw;
            }
        }
    }
    if (result.rows.empty()) throw std::runtime_error("Active BNW localization catalog is empty");
    return result;
}

std::string TopPanelLocalizationCatalog::format(std::string_view key,
                                                 const std::vector<std::string> &arguments) const {
    const auto found = rows.find(std::string(key));
    if (found == rows.end())
        throw std::runtime_error("TopPanel localization key is missing: " + std::string(key));
    std::string result;
    const auto &source = found->second;
    std::size_t cursor = 0;
    while (cursor < source.size()) {
        const auto open = source.find('{', cursor);
        if (open == std::string::npos) {
            result.append(source, cursor, std::string::npos);
            break;
        }
        result.append(source, cursor, open - cursor);
        const auto close = source.find('}', open + 1);
        if (close == std::string::npos)
            throw std::runtime_error("TopPanel localization placeholder is unterminated: " + std::string(key));
        std::size_t digit = open + 1;
        while (digit < close && std::isdigit(static_cast<unsigned char>(source[digit])))
            ++digit;
        if (digit == open + 1 || digit == close)
            throw std::runtime_error("TopPanel localization placeholder is malformed: " + std::string(key));
        const auto index = integer(std::string_view(source).substr(open + 1, digit - open - 1), "placeholder");
        if (index <= 0 || static_cast<std::size_t>(index) > arguments.size())
            throw std::runtime_error("TopPanel localization argument is missing: " + std::string(key));
        const auto &argument = arguments[static_cast<std::size_t>(index - 1)];
        if (key == "TXT_KEY_TOP_PANEL_GOLD" && index == 2) {
            const auto value = integer(argument, "gold-per-turn");
            if (value < 0)
                result += "[COLOR:255:60:60:255](-" + std::to_string(-static_cast<std::int64_t>(value)) +")[/COLOR]";
            else
                result += "(+" + argument + ")";
        } else {
            result += argument;
        }
        cursor = close + 1;
    }
    return result;
}

namespace {
TopPanelBarBinding bind_top_panel_bar_asset(const TopPanelLayout &layout,
                                            const assets::AssetLookup &result) {
    if (layout.center_bar.attribute("Texture") != top_panel_bar_texture ||
        layout.right_bar.attribute("Texture") != top_panel_bar_texture)
        throw std::runtime_error("TopPanel bars do not use the authored TopPanelBar texture");
    if (std::holds_alternative<std::monostate>(result))
        throw std::runtime_error("Selected UI source has no TopPanelBar.dds");
    if (std::holds_alternative<assets::AmbiguousAsset>(result))
        throw std::runtime_error("Selected UI source has ambiguous TopPanelBar.dds");
    const auto &handle = std::get<assets::AssetHandle>(result);
    const auto dds = assets::DdsData::parse(handle.read_bytes(16 * 1024 * 1024));
    if (dds.face_count() != 1 || dds.mip_count() == 0 || dds.width() == 0 || dds.height() == 0)
        throw std::runtime_error("TopPanelBar.dds has unsupported dimensions or faces");
    return TopPanelBarBinding{handle.candidate().provenance, handle.candidate().original_name,
                              dds.width(), dds.height(), dds.format(), dds.mip_count()};
}
} // namespace

TopPanelBarBinding bind_top_panel_bar(const TopPanelLayout &layout,
                                      const std::shared_ptr<const assets::AssetSource> &source) {
    if (!source)
        throw std::runtime_error("TopPanel bar binding requires a selected asset source");
    return bind_top_panel_bar_asset(layout, source->find(assets::AssetKey("TopPanelBar.dds")));
}

TopPanelBarBinding bind_top_panel_bar(const TopPanelLayout &layout,
                                      const assets::ResourceMountTable &resources) {
    return bind_top_panel_bar_asset(layout, resources.resolve(assets::AssetKey("TopPanelBar.dds")).asset);
}

TopPanelRenderPlan plan_top_panel(const TopPanelLayout &layout, std::uint32_t viewport_width,
                                  std::uint32_t viewport_height) {
    if (viewport_width == 0 || viewport_height == 0)
        throw std::runtime_error("TopPanel render plan requires a nonempty viewport");
    if (layout.center_bar.attribute("Size") != "Full.32" ||
        layout.right_bar.attribute("Size") != "295.32")
        throw std::runtime_error("TopPanel bars do not use the authored display sizes");
    constexpr std::uint32_t bar_height = 32;
    constexpr std::uint32_t right_width = 295;
    if (right_width > viewport_width)
        throw std::runtime_error("TopPanel right bar exceeds the viewport");
    return {{0, 0, viewport_width, bar_height}, {viewport_width - right_width, 0, right_width, bar_height}};
}

TopPanelGrid9Plan plan_top_panel_grid9(const TopPanelLayout &layout, const TopPanelGrid9Style &style,
                                       TopPanelRect frame) {
    if (frame.width == 0 || frame.height == 0)
        throw std::runtime_error("TopPanel Grid9 frame must be nonempty");
    const auto &grid = layout.tooltip_grid;
    if (grid.attribute("Style") != style.name)
        throw std::runtime_error("TopPanel tooltip style does not match Grid9 style");
    const auto top = style.source[0].height;
    const auto bottom = style.source[6].height;
    const auto left = style.source[0].width;
    const auto right = style.source[2].width;
    if (frame.width < static_cast<std::uint32_t>(left + right) ||
        frame.height < static_cast<std::uint32_t>(top + bottom))
        throw std::runtime_error("TopPanel Grid9 frame is smaller than its corners");
    TopPanelGrid9Plan result;
    result.frame = frame;
    const auto grid_padding = grid.attribute("Padding");
    const auto comma = grid_padding.find(',');
    if (comma == std::string::npos)
        throw std::runtime_error("TopPanel tooltip padding is not a pair");
    const auto pad_x = integer(std::string_view(grid_padding).substr(0, comma), "Padding");
    const auto pad_y = integer(std::string_view(grid_padding).substr(comma + 1), "Padding");
    if (frame.width < static_cast<std::uint32_t>(pad_x * 2) ||
        frame.height < static_cast<std::uint32_t>(pad_y * 2))
        throw std::runtime_error("TopPanel tooltip padding exceeds frame");
    result.content = {frame.x + static_cast<std::uint32_t>(pad_x), frame.y + static_cast<std::uint32_t>(pad_y),
                      frame.width - static_cast<std::uint32_t>(pad_x * 2),
                      frame.height - static_cast<std::uint32_t>(pad_y * 2)};
    const auto center_width = frame.width - static_cast<std::uint32_t>(left + right);
    const auto center_height = frame.height - static_cast<std::uint32_t>(top + bottom);
    result.destination = {
        TopPanelRect{frame.x, frame.y, static_cast<std::uint32_t>(left), static_cast<std::uint32_t>(top)},
        TopPanelRect{frame.x + static_cast<std::uint32_t>(left), frame.y, center_width,
                     static_cast<std::uint32_t>(top)},
        TopPanelRect{frame.x + frame.width - static_cast<std::uint32_t>(right), frame.y,
                     static_cast<std::uint32_t>(right), static_cast<std::uint32_t>(top)},
        TopPanelRect{frame.x, frame.y + static_cast<std::uint32_t>(top), static_cast<std::uint32_t>(left),
                     center_height},
        TopPanelRect{frame.x + static_cast<std::uint32_t>(left), frame.y + static_cast<std::uint32_t>(top),
                     center_width, center_height},
        TopPanelRect{frame.x + frame.width - static_cast<std::uint32_t>(right),
                     frame.y + static_cast<std::uint32_t>(top), static_cast<std::uint32_t>(right), center_height},
        TopPanelRect{frame.x, frame.y + frame.height - static_cast<std::uint32_t>(bottom),
                     static_cast<std::uint32_t>(left), static_cast<std::uint32_t>(bottom)},
        TopPanelRect{frame.x + static_cast<std::uint32_t>(left), frame.y + frame.height -
                                                               static_cast<std::uint32_t>(bottom),
                     center_width, static_cast<std::uint32_t>(bottom)},
        TopPanelRect{frame.x + frame.width - static_cast<std::uint32_t>(right),
                     frame.y + frame.height - static_cast<std::uint32_t>(bottom),
                     static_cast<std::uint32_t>(right), static_cast<std::uint32_t>(bottom)}};
    return result;
}

TopPanelActionEvent top_panel_action_event(TopPanelAction action) {
    switch (action) {
    case TopPanelAction::open_civilopedia:
        return {action, TopPanelActionRoute::search_for_pedia_entry};
    case TopPanelAction::open_game_menu:
        return {action, TopPanelActionRoute::queue_game_menu};
    case TopPanelAction::exit_city_screen:
        return {action, TopPanelActionRoute::serial_event_exit_city_screen};
    case TopPanelAction::open_policy_choice:
        return {action, TopPanelActionRoute::choose_policy_popup};
    case TopPanelAction::open_technology_tree:
        return {action, TopPanelActionRoute::tech_tree_popup};
    }
    throw std::runtime_error("Unknown Civ V TopPanel action");
}

std::optional<TopPanelPresentation> present_top_panel(const TopPanelState &state,
                                                      const TopPanelTextLocalizer &localizer) {
    if (!state.player)
        return std::nullopt;
    if (state.player->city_count < 0)
        throw std::runtime_error("Civ V TopPanel city count cannot be negative");
    const auto &player = *state.player;
    const bool has_cities = player.city_count > 0;
    TopPanelPresentation result;
    result.show_info_stack = has_cities;
    result.civilopedia_action = TopPanelAction::open_civilopedia;
    result.science_action = TopPanelAction::open_technology_tree;
    result.culture_action = TopPanelAction::open_policy_choice;
    if (has_cities) {
        TopPanelButton button;
        if (state.head_selected_city && state.city_screen_up) {
            button.text = upper_ui_locale(localized(localizer, "TXT_KEY_RETURN"));
            button.tooltip = localized(localizer, "TXT_KEY_CITY_SCREEN_EXIT_TOOLTIP");
            button.action = TopPanelAction::exit_city_screen;
        } else {
            button.text = upper_ui_locale(localized(localizer, "TXT_KEY_MENU"));
            button.tooltip = localized(localizer, "TXT_KEY_MENU_TOOLTIP");
            // Retail TopPanel.lua only changes this button to RETURN when the
            // city screen is actually up. Having a selected city in the world
            // view must still open the game menu.
            button.action = TopPanelAction::open_game_menu;
        }
        result.menu_button = std::move(button);
        result.science_text = science(state, player, localizer);
        result.gold_text = localized(localizer, "TXT_KEY_TOP_PANEL_GOLD",
                                     {std::to_string(player.gold), std::to_string(player.gold_per_turn)});
        if (state.expansion2_ui) {
            result.international_trade_routes_text = localized(
                localizer, "TXT_KEY_TOP_PANEL_INTERNATIONAL_TRADE_ROUTES",
                {std::to_string(player.international_trade_routes_used),
                 std::to_string(player.international_trade_routes_available)});
            result.tourism_text = "[ICON_TOURISM] +" + std::to_string(player.tourism);
            result.faith_text = state.no_religion
                ? localized(localizer, "TXT_KEY_TOP_PANEL_RELIGION_OFF")
                : "[ICON_PEACE]" + std::to_string(player.faith) + " (+" +
                      std::to_string(player.faith_per_turn) + ")";
            result.international_trade_routes_tooltip =
                localized(localizer, "TXT_KEY_TOP_PANEL_INTERNATIONAL_TRADE_ROUTES_TT",
                          {std::to_string(player.international_trade_routes_used),
                           std::to_string(player.international_trade_routes_available)});
            if (state.no_religion)
                result.faith_tooltip = localized(localizer, "TXT_KEY_TOP_PANEL_RELIGION_OFF_TOOLTIP");
        }
        result.happiness_text = happiness(state, player, localizer);
        result.golden_age_text = golden_age(state, player, localizer);
        result.culture_text = culture(state, player, localizer);
        result.resource_text = resources(player, localizer);
        result.science_tooltip = science_tooltip(state, player, localizer);
        result.gold_tooltip = gold_tooltip(state, player, localizer);
        result.happiness_tooltip = happiness_tooltip(state, player, localizer);
        result.golden_age_tooltip = golden_age_tooltip(state, player, localizer);
        result.culture_tooltip = culture_tooltip(state, player, localizer);
        result.resource_tooltip = resource_tooltip(player, localizer);
    }
    // This branch is outside GetNumCities() in the retail UpdateData. The
    // parent stack may be hidden, but the child still receives its update.
    result.show_unit_supply = player.unit_production_maintenance_mod != 0;
    if (result.show_unit_supply)
        result.unit_supply_tooltip = localized(
            localizer, "TXT_KEY_UNIT_SUPPLY_REACHED_TOOLTIP",
            {std::to_string(player.units_supplied), std::to_string(player.units_out_of_supply),
             std::to_string(-static_cast<std::int64_t>(player.unit_production_maintenance_mod))});
    result.current_turn_text = localized(localizer, "TXT_KEY_TP_TURN_COUNTER", {std::to_string(state.game_turn)});
    result.current_date_text = state.turn_date;
    if (state.available_metrics) {
        const auto known = [&](TopPanelMetric metric) { return state.available_metrics->contains(metric); };
        const auto unavailable = [](std::optional<std::string> &text, std::optional<std::string> &tooltip) {
            if (!text) return;
            text = "N/A";
            tooltip = "Live game data unavailable";
        };
        if (!state.no_science && !known(TopPanelMetric::science))
            unavailable(result.science_text, result.science_tooltip);
        if (!known(TopPanelMetric::gold)) unavailable(result.gold_text, result.gold_tooltip);
        if (!known(TopPanelMetric::trade))
            unavailable(result.international_trade_routes_text, result.international_trade_routes_tooltip);
        if (!state.no_happiness && !known(TopPanelMetric::happiness))
            unavailable(result.happiness_text, result.happiness_tooltip);
        if (!state.no_happiness && !known(TopPanelMetric::golden_age))
            unavailable(result.golden_age_text, result.golden_age_tooltip);
        if (!state.no_policies && !known(TopPanelMetric::culture))
            unavailable(result.culture_text, result.culture_tooltip);
        if (!known(TopPanelMetric::tourism)) unavailable(result.tourism_text, result.tourism_tooltip);
        if (!known(TopPanelMetric::faith)) unavailable(result.faith_text, result.faith_tooltip);
        if (!known(TopPanelMetric::resources)) unavailable(result.resource_text, result.resource_tooltip);
        if (!known(TopPanelMetric::unit_supply)) {
            result.show_unit_supply = false;
            result.unit_supply_tooltip.reset();
        }
    }
    return result;
}

TopPanelControls::TopPanelControls(const TopPanelLayout &layout, const TopPanelTextLocalizer &localizer) {
    const auto element = [](const TopPanelStack &stack, std::string_view id) -> const TopPanelElement & {
        const auto found = std::find_if(stack.children.begin(), stack.children.end(), [&](const auto &child) {
            return child.attribute("ID") == id;
        });
        if (found == stack.children.end())
            throw std::runtime_error("Missing authored TopPanel control: " + std::string(id));
        return *found;
    };
    const auto text = [&](const TopPanelElement &control, std::string_view attribute) {
        const auto value = control.attribute(attribute);
        return value.starts_with("TXT_KEY_") ? localized(localizer, value) : value;
    };
    const auto visible = [](const TopPanelElement &control) {
        const auto hidden = control.attribute("Hidden");
        return hidden != "1" && hidden != "true";
    };
    presentation_.show_info_stack = visible(layout.info_stack.element);
    presentation_.show_unit_supply = visible(element(layout.info_stack, "UnitSupplyString"));
    presentation_.science_text = text(element(layout.info_stack, "SciencePerTurn"), "String");
    presentation_.gold_text = text(element(layout.info_stack, "GoldPerTurn"), "String");
    const auto find_extra = [&](std::string_view id) {
        return std::find_if(layout.info_stack.children.begin(), layout.info_stack.children.end(),
                            [&](const auto &child) { return child.attribute("ID") == id; });
    };
    const auto trade = find_extra("InternationalTradeRoutes");
    const auto tourism = find_extra("TourismString");
    const auto faith = find_extra("FaithString");
    const auto extras = (trade != layout.info_stack.children.end()) +
                        (tourism != layout.info_stack.children.end()) +
                        (faith != layout.info_stack.children.end());
    if (extras != 0 && extras != 3)
        throw std::runtime_error("Authored BNW TopPanel controls are incomplete");
    if (extras == 3) {
        presentation_.international_trade_routes_text = text(*trade, "String");
        presentation_.tourism_text = text(*tourism, "String");
        presentation_.faith_text = text(*faith, "String");
    }
    presentation_.happiness_text = text(element(layout.info_stack, "HappinessString"), "String");
    presentation_.golden_age_text = text(element(layout.info_stack, "GoldenAgeString"), "String");
    presentation_.culture_text = text(element(layout.info_stack, "CultureString"), "String");
    presentation_.resource_text = text(element(layout.info_stack, "ResourceString"), "String");
    const auto &menu = element(layout.right_stack, "MenuButton");
    presentation_.menu_button = TopPanelButton{text(menu, "String"), text(menu, "ToolTip"),
                                               TopPanelAction::open_game_menu};
    presentation_.current_turn_text = text(element(layout.right_stack, "CurrentTurn"), "String");
    presentation_.current_date_text = text(element(layout.right_stack, "CurrentDate"), "String");
}

const TopPanelPresentation &TopPanelControls::apply(const std::optional<TopPanelPresentation> &update) {
    if (!update)
        return presentation_;
    const auto menu = presentation_.menu_button;
    if (update->show_info_stack)
        presentation_ = *update;
    else {
        presentation_.show_info_stack = false;
        presentation_.current_turn_text = update->current_turn_text;
        presentation_.current_date_text = update->current_date_text;
        presentation_.show_unit_supply = update->show_unit_supply;
        presentation_.unit_supply_tooltip = update->unit_supply_tooltip;
    }
    if (!update->menu_button)
        presentation_.menu_button = menu;
    return presentation_;
}

TopPanelRuntime::TopPanelRuntime(StateProvider state_provider, TopPanelTextLocalizer localizer,
                                 TopPanelActionSink action_sink)
    : state_provider_(std::move(state_provider)), localizer_(std::move(localizer)),
      action_sink_(std::move(action_sink)) {
    if (!state_provider_)
        throw std::invalid_argument("TopPanel runtime requires a state provider");
    if (!localizer_.text)
        throw std::invalid_argument("TopPanel runtime requires a localization provider");
}

TopPanelState TopPanelRuntime::state() const { return state_provider_(); }

std::optional<TopPanelPresentation> TopPanelRuntime::present() const {
    return present_top_panel(state(), localizer_);
}

TopPanelActionEvent TopPanelRuntime::dispatch(TopPanelAction action) const {
    const auto event = top_panel_action_event(action);
    if (action_sink_)
        action_sink_(event);
    return event;
}
} // namespace civ5::ui
