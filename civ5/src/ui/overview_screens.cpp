#include "civ5/ui/overview_screens.hpp"
#include "civ5/ui/economic_overview_fields.hpp"
#include "civ5/ui/city_screens.hpp"
#include "civ5/runtime/science.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>
#include <charconv>
#include <iterator>
#include <limits>
#include <set>
#include <stdexcept>

namespace civ5::ui {
namespace {
std::string decimal(std::int64_t value) { return std::to_string(value); }

std::string gold_from_hundredths(std::int64_t value) {
    // The treasury snapshot stores hundredths of a gold piece. Preserve that
    // precision while presenting the native Treasury label.
    const auto whole = value / 100;
    const auto fraction = value % 100;
    const auto absolute_fraction = fraction < 0 ? -fraction : fraction;
    return (value < 0 && whole == 0 ? "-" : "") + decimal(whole) + "." +
           (absolute_fraction < 10 ? "0" : "") + decimal(absolute_fraction);
}

std::string hundredths(std::int64_t value) {
    const auto magnitude = value < 0 ? std::uint64_t{0} - static_cast<std::uint64_t>(value)
                                     : static_cast<std::uint64_t>(value);
    const auto fraction = magnitude % 100;
    return std::string(value < 0 ? "-" : "") + std::to_string(magnitude / 100) +
           "." + (fraction < 10 ? "0" : "") + std::to_string(fraction);
}

void collect(const content::XmlNode &node, std::vector<OverviewControl> &out) {
    const auto id = node.attribute("ID");
    const auto string = node.attribute("String");
    const auto texture = node.attribute("Texture");
    const auto style = node.attribute("Style");
    if (!id.empty() || !string.empty() || !texture.empty() || !style.empty() ||
        ((node.name == "Box" || node.name == "Stack") &&
         !node.attribute("Size").empty()))
        out.push_back({id, node.name, string, texture, style,
                       node.attribute("Anchor"), node.attribute("Offset"), node.attribute("Size")});
    for (const auto &child : node.children) collect(child, out);
}

std::optional<std::pair<std::int32_t, std::int32_t>> pair_of(std::string_view value) {
    const auto comma = value.find_first_of(",.");
    if (comma == std::string_view::npos) return std::nullopt;
    auto parse = [](std::string_view text) -> std::optional<std::int32_t> {
        std::int32_t result = 0;
        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), result);
        if (error != std::errc{} || end != text.data() + text.size()) return std::nullopt;
        return result;
    };
    const auto first = parse(value.substr(0, comma));
    const auto second = parse(value.substr(comma + 1));
    if (!first || !second) return std::nullopt;
    return std::pair{*first, *second};
}

std::string selected_path(const assets::AssetHandle &handle) {
    const auto &source = handle.candidate().provenance;
    if (source.archive_relative_path)
        return *source.archive_relative_path + ":" + source.source_relative_path;
    return source.source_relative_path;
}

void unavailable(OverviewFrameState &frame, std::string_view id, std::string reason) {
    frame.pages[std::string(id)].unavailable_reason = std::move(reason);
}

std::string display_text(std::string_view key, const OverviewLocalizer &localize) {
    if (localize && key.starts_with("TXT_KEY_")) {
        if (auto text = localize(key)) return *text;
    }
    return std::string(key);
}

std::string display_row_label(const OverviewRow &row, const OverviewLocalizer &localize,
                              OverviewLanguage language) {
    if (row.label == "Followers in owned cities")
        return language == OverviewLanguage::russian ? "Последователи в своих городах"
                                                     : "Followers in owned cities";
    if (row.label == "Unassigned trade unit") {
        const auto name = localize ? localize(row.label_key) : std::nullopt;
        return (language == OverviewLanguage::russian ? "Свободный торговый юнит: "
                                                      : "Unassigned trade unit: ") +
               name.value_or(row.label_key);
    }
    if (!row.label_key.empty() && localize) {
        if (auto translated = localize(row.label_key)) return *translated;
    }
    return row.label;
}

std::string unavailable_text(std::string_view reason, OverviewLanguage language) {
    if (language == OverviewLanguage::english) return "Unavailable: " + std::string(reason);
    if (reason == "Happiness components are unavailable")
        return "Недоступно: составляющие счастья не получены";
    if (reason == "Live BNW overview data is not connected")
        return "Недоступно: данные обзора BNW не подключены";
    if (reason == "No active route details supplied by live state")
        return "Недоступно: сведения о действующих маршрутах не получены";
    if (reason == "Trade city names, route yields, and destination choices are unavailable")
        return "Недоступно: названия городов, доходы маршрутов и выбор направлений";
    if (reason == "Trade route yields and destination choices are unavailable")
        return "Недоступно: доходы маршрутов и выбор направлений";
    if (reason == "Trade unit types and idle status are unavailable")
        return "Недоступно: типы торговых юнитов и их свободное состояние";
    if (reason == "Overview snapshot for active player is unavailable")
        return "Недоступно: нет снимка обзора для текущего игрока";
    if (reason == "Live page data is unavailable")
        return "Недоступно: данные этой страницы не получены";
    if (reason == "Supply capacity, components, remaining/deficit, unit status, movement and strength are unavailable")
        return "Недоступно: предел и составляющие снабжения, остаток или нехватка, состояние, движение и сила юнитов";
    if (reason == "Faith, beliefs and world religion totals are unavailable")
        return "Недоступно: вера, верования и мировые итоги религий";
    if (reason == "Demographic rivals and unowned metrics are unavailable")
        return "Недоступно: показатели соперников и неподключённые данные демографии";
    return "Недоступно: " + std::string(reason);
}

std::int64_t demographic_approval(std::int64_t excess_happiness) {
    // Demographics.lua: clamp(60 + GetExcessHappiness() * 3, 0, 100).
    // Compare before multiplying so even a corrupt saved scalar cannot overflow.
    if (excess_happiness <= -20) return 0;
    if (excess_happiness >= 14) return 100;
    return 60 + excess_happiness * 3;
}

bool budget_deficit_matches(const runtime::TreasuryPlayer &player,
                            const runtime::TreasuryNetQuote &gold,
                            const runtime::ScienceQuote &science) {
    if (science.gold_rate_times100 != gold.net_times100 ||
        science.budget_deficit > 0) return false;
    try {
        return science.budget_deficit == std::min<std::int64_t>(0,
            runtime::wide::add(player.balance_times100, gold.net_times100));
    } catch (const std::overflow_error &) {
        return false;
    }
}

bool active_trade_route(const saves::TradeRouteObservation &route) {
    // CvGameTrade keeps empty slots with all four endpoints at -1. A partial
    // or negative endpoint is not an established route we can display.
    return route.origin_x >= 0 && route.origin_y >= 0 &&
           route.destination_x >= 0 && route.destination_y >= 0 &&
           route.origin_owner >= 0 && route.destination_owner >= 0;
}

bool empty_trade_route_slot(const saves::TradeRouteObservation &route) {
    return route.origin_x == -1 && route.origin_y == -1 &&
           route.destination_x == -1 && route.destination_y == -1;
}

std::optional<std::string> trade_unit_description_key(const content::RulesDatabase &database,
                                                       std::int64_t unit_type) {
    if (unit_type < 0)
        throw std::runtime_error("Trade unit type is unresolved in live roster");
    std::optional<std::string> key;
    bool trade = false;
    std::uint64_t matches = 0;
    const auto report = database.query_rows(
        {"SELECT Trade, Description FROM Units WHERE ID = ? LIMIT 2", {unit_type}},
        [&](const content::RulesRowView &row) {
            ++matches;
            trade = row.integer("Trade") == 1;
            if (trade) key = row.text("Description");
        }, 2);
    if (!report.all_succeeded() || matches != 1)
        throw std::runtime_error("Trade unit type is absent from active BNW rules");
    if (trade && (!key || !key->starts_with("TXT_KEY_")))
        throw std::runtime_error("Trade unit has no active BNW description key");
    return key;
}

std::optional<std::string> route_city_name(const runtime::CityYieldContext &cities,
    const saves::SavedSessionSnapshot *saved, std::int64_t owner, std::int64_t x,
    std::int64_t y,
    const std::function<std::optional<std::string>(std::string_view)> &localize_city) {
    if (!saved || !saved->all_players) return std::nullopt;
    const auto player = cities.players.find(owner);
    if (player == cities.players.end()) return std::nullopt;
    // The route wire has coordinates and owner, but no BNW city ID. Require
    // one live city at that endpoint before borrowing its saved name.
    std::optional<std::int64_t> city_id;
    for (const auto &[id, city] : player->second.cities) {
        if (id != city.id || city.owner != owner || city.x != x || city.y != y)
            continue;
        if (city_id) return std::nullopt;
        city_id = id;
    }
    if (!city_id) return std::nullopt;
    const auto frame = capture_city_screen_frame(cities, *saved, owner, city_id,
                                                  localize_city);
    const auto city = std::find_if(frame.cities.begin(), frame.cities.end(),
        [&](const CityScreenCity &entry) {
            return entry.id == *city_id && entry.owner == owner &&
                   entry.x == x && entry.y == y;
        });
    if (city == frame.cities.end() || !city->name || city->name->empty() ||
        city->name->starts_with("TXT_KEY_")) return std::nullopt;
    return city->name;
}

} // namespace

OverviewFrameState capture_overview_frame(const runtime::ResearchContext &game,
    const runtime::CityYieldContext &cities, const runtime::TreasuryContext &treasury,
    std::int64_t player_id, const runtime::MaintenanceContext *maintenance,
    const content::RulesDatabase *database, const content::InfoLocalizer &localize_info,
    const saves::SavedSessionSnapshot *saved,
    const std::function<std::optional<std::string>(std::string_view)> &localize_city,
    const runtime::TreasuryNetQuote *gold_quote,
    const runtime::ScienceQuote *science_quote) {
    OverviewFrameState frame;
    frame.player_id = player_id;
    const auto player = game.players.find(player_id);
    if (game.profile != content::TechnologyProfile::bnw ||
        cities.profile != content::TechnologyProfile::bnw ||
        treasury.profile != content::TechnologyProfile::bnw ||
        player == game.players.end() || !player->second.alive) return frame;
    const auto city_player = cities.players.find(player_id);
    const auto treasury_player = treasury.players.find(player_id);
    if (city_player != cities.players.end()) {
        auto &economy = frame.pages["EconomicOverview"];
        economy.rows.push_back({"Cities", decimal(city_player->second.cities.size()),
                                "TXT_KEY_EO_CITIES"});
        economy.rows.push_back({"Population", decimal(city_player->second.total_population),
                                "TXT_KEY_DEMOGRAPHICS_POPULATION"});
        if (treasury_player != treasury.players.end())
            economy.rows.push_back({"Treasury", gold_from_hundredths(
                treasury_player->second.balance_times100), "TXT_KEY_EO_TOTAL_GOLD"});
        auto &general = frame.pages["EconomicGeneralInfo"];
        std::optional<CityScreenFrame> named_cities;
        if (saved && saved->all_players) {
            try {
                named_cities = capture_city_screen_frame(cities, *saved, player_id,
                                                        std::nullopt, localize_city);
            } catch (const std::exception &) {
                // A malformed or stale city source may not replace live city
                // data. Leave every name unresolved for this frame.
            }
        }
        for (const auto &[id, city] : city_player->second.cities) {
            if (city.owner != player_id || city.id != id || city.population < 0) continue;
            OverviewCityRow row{id, city.population, std::nullopt};
            if (named_cities) {
                const auto named = std::find_if(named_cities->cities.begin(),
                    named_cities->cities.end(), [&](const CityScreenCity &candidate) {
                        return candidate.owner == player_id && candidate.id == id &&
                               candidate.x == city.x && candidate.y == city.y;
                    });
                if (named != named_cities->cities.end() && named->name &&
                    !named->name->empty() && !named->name->starts_with("TXT_KEY_"))
                    row.name = named->name;
            }
            general.cities.push_back(std::move(row));
        }
        if (treasury_player != treasury.players.end()) {
            general.rows.push_back({"TotalGoldValue",
                                    hundredths(treasury_player->second.balance_times100)});
            // GetCityConnectionGoldTimes100 is restored directly from the
            // owned treasury player, independent of a completed net quote.
            general.rows.push_back({"TradeIncomeValue",
                                    hundredths(treasury_player->second.connections_times100)});
        }
        if (treasury_player != treasury.players.end() && gold_quote && gold_quote->expenses) {
            const auto &income = gold_quote->income;
            const auto &expense = *gold_quote->expenses;
            general.science_lost_visible = gold_quote->net_times100 < 0;
            general.rows.push_back({"NetGoldValue",
                                    hundredths(gold_quote->net_times100)});
            general.rows.push_back({"GrossGoldValue", hundredths(income.total_times100)});
            general.rows.push_back({"TotalExpenseValue", decimal(expense.total)});
            general.rows.push_back({"CityIncomeValue", hundredths(income.cities)});
            general.rows.push_back({"DiploIncomeValue",
                                    hundredths(std::max<std::int64_t>(0, income.diplomacy))});
            general.rows.push_back({"ReligionIncomeValue",
                                    hundredths(std::max<std::int64_t>(0, income.religion))});
            general.rows.push_back({"UnitExpenseValue", decimal(expense.units.total)});
            general.rows.push_back({"BuildingExpenseValue", decimal(expense.buildings)});
            general.rows.push_back({"TileExpenseValue", decimal(expense.improvements)});
            general.rows.push_back({"DiploExpenseValue", income.diplomacy < 0
                ? hundredths(income.diplomacy).substr(1) : "0.00"});
            // EconomicGeneralInfo.lua shows this row only at negative GPT.
            // The same owner's complete science quote carries the native
            // GetScienceFromBudgetDeficitTimes100 component. Require its gold
            // rate to match this frame's net quote before using it.
            if (gold_quote->net_times100 < 0 && science_quote &&
                budget_deficit_matches(treasury_player->second, *gold_quote,
                                       *science_quote))
                general.rows.push_back({"ScienceLostValue",
                                        hundredths(science_quote->budget_deficit)});
        }
        // The serialized total is citizens, whereas the native Demographics
        // population column calls GetRealPopulation(). Do not label it as the
        // latter. Approval is the one native column derivable from owned state.
        auto &demographics = frame.pages["Demographics"];
        demographics.unavailable_reason =
            "Demographic rivals and unowned metrics are unavailable";
        if (city_player->second.excess_happiness)
            demographics.rows.push_back({"Approval",
                decimal(demographic_approval(*city_player->second.excess_happiness)) + "%",
                "TXT_KEY_DEMOGRAPHICS_APPROVAL"});
        frame.pages["EndGameDemographics"] = frame.pages["Demographics"];
        if (city_player->second.excess_happiness)
            frame.pages["HappinessInfo"].rows.push_back(
                {"Overall happiness", decimal(*city_player->second.excess_happiness),
                 "TXT_KEY_CO_VICTORY_EXCESS_HAPPINESS_HEADER"});
    }
    if (player->second.policies_resolved)
        frame.pages["CultureOverview"].rows.push_back(
            {"Adopted policies", decimal(player->second.policies.size()),
             "TXT_KEY_POLICIES_ADOPTED"});
    auto &trade_page = frame.pages["TradeRouteOverview"];
    std::set<std::int64_t> assigned_trade_units;
    bool has_outgoing_route = false;
    bool all_route_city_names_resolved = true;
    std::vector<std::string> named_outgoing_routes;
    bool all_incoming_city_names_resolved = true;
    std::vector<std::string> named_incoming_routes;
    for (const auto &route : treasury.trade_routes) {
        // A vacated CvGameTrade slot can retain its former owner IDs. Four
        // absent endpoints still mean empty, not a partially decoded route.
        if (empty_trade_route_slot(route)) continue;
        const bool outgoing = route.origin_owner == player_id;
        const bool incoming = route.destination_owner == player_id && !outgoing;
        if (!outgoing && !incoming) continue;
        if (!active_trade_route(route)) {
            if (outgoing) all_route_city_names_resolved = false;
            if (incoming) all_incoming_city_names_resolved = false;
            continue;
        }
        // The retail opening tab is YourTR. The incoming city-pair quote is
        // only for TopPanel; incoming tab yields and possible destinations
        // have distinct native queries and remain unavailable here.
        const auto origin = route_city_name(cities, saved, route.origin_owner,
            route.origin_x, route.origin_y, localize_city);
        const auto destination = route_city_name(cities, saved, route.destination_owner,
            route.destination_x, route.destination_y, localize_city);
        if (incoming) {
            all_incoming_city_names_resolved &= origin.has_value() && destination.has_value();
            if (origin && destination)
                named_incoming_routes.push_back(*origin + " → " + *destination);
            continue;
        }
        has_outgoing_route = true;
        assigned_trade_units.insert(route.unit_id);
        all_route_city_names_resolved &= origin.has_value() && destination.has_value();
        if (origin && destination)
            named_outgoing_routes.push_back(*origin + " → " + *destination);
        const auto origin_text = origin.value_or(decimal(route.origin_x) + "," +
            decimal(route.origin_y));
        const auto destination_text = destination.value_or(decimal(route.destination_x) +
            "," + decimal(route.destination_y));
        trade_page.rows.push_back({"Trade route", origin_text + " → " +
            destination_text, "TXT_KEY_TRO_TAB_EXISTING_TR"});
    }
    if (all_route_city_names_resolved)
        frame.named_outgoing_trade_routes = std::move(named_outgoing_routes);
    if (all_incoming_city_names_resolved)
        frame.named_incoming_trade_routes = std::move(named_incoming_routes);
    bool idle_units_resolved = maintenance && database &&
        maintenance->profile == content::TechnologyProfile::bnw;
    if (idle_units_resolved) {
        const auto player_units = maintenance->players.find(player_id);
        idle_units_resolved = player_units != maintenance->players.end();
        if (idle_units_resolved) {
            std::vector<OverviewRow> idle_rows;
            for (const auto &[unit_id, unit] : player_units->second.units) {
                std::optional<std::string> description;
                try { description = trade_unit_description_key(*database, unit.type); }
                catch (const std::exception &) { idle_units_resolved = false; break; }
                if (!description || assigned_trade_units.contains(unit_id)) continue;
                idle_rows.push_back({"Unassigned trade unit", "#" + decimal(unit_id),
                                     *description});
            }
            if (idle_units_resolved)
                trade_page.rows.insert(trade_page.rows.end(), idle_rows.begin(), idle_rows.end());
        }
    }
    trade_page.unavailable_reason = idle_units_resolved
        ? has_outgoing_route && all_route_city_names_resolved
            ? "Trade route yields and destination choices are unavailable"
            : "Trade city names, route yields, and destination choices are unavailable"
        : "Trade unit types and idle status are unavailable";
    auto &religion_page = frame.pages["ReligionOverview"];
    religion_page.unavailable_reason =
        "Faith, beliefs and world religion totals are unavailable";
    for (const auto &religion : treasury.religions) {
        if (religion.founder != player_id) continue;
        religion_page.rows.push_back(
            {"Religion", "#" + decimal(religion.religion), "TXT_KEY_RO_WR_RELIGION"});
        // This is an active-player subtotal, not the native World Religions
        // global city count. Reject incomplete city identities or invalid
        // follower lists rather than presenting a partial subtotal as fact.
        if (city_player == cities.players.end()) continue;
        std::int64_t followers = 0;
        bool complete = true;
        for (const auto &[id, city] : city_player->second.cities) {
            if (id != city.id || city.owner != player_id || city.population < 0) {
                complete = false;
                break;
            }
            bool seen = false;
            for (const auto &status : city.religions) {
                if (status.religion != religion.religion) continue;
                if (seen || status.followers < 0 || status.followers > city.population ||
                    followers > std::numeric_limits<std::int64_t>::max() - status.followers) {
                    complete = false;
                    break;
                }
                seen = true;
                followers += status.followers;
            }
            if (!complete) break;
        }
        if (complete)
            religion_page.rows.push_back({"Followers in owned cities", decimal(followers)});
    }
    for (std::size_t i = 0; i < cities.leagues.size(); ++i) {
        const auto &members = cities.leagues[i].members;
        if (std::find(members.begin(), members.end(), player_id) == members.end()) continue;
        frame.pages["LeagueOverviewPopup"].rows.push_back(
            {"World Congress", "#" + decimal(static_cast<std::int64_t>(i)) +
             " · " + decimal(members.size()) + " members", "TXT_KEY_LEAGUE_OVERVIEW"});
    }
    if (maintenance && maintenance->profile == content::TechnologyProfile::bnw) {
        const auto army = maintenance->players.find(player_id);
        if (army != maintenance->players.end()) {
            auto &page = frame.pages["MilitaryOverview"];
            // Unit IDs and military classification are owned and validated by
            // MaintenanceContext. The screen's richer live status is not.
            // MilitaryOverview.lua reads GetNumUnitsSupplied() and its
            // handicap/city/population components, then chooses remaining or
            // deficit through GetNumUnitsOutOfSupply(). BNW maintenance's
            // cached supply expense and modifier are different quantities.
            page.unavailable_reason =
                "Supply capacity, components, remaining/deficit, unit status, movement and strength are unavailable";
            for (const auto &[id, unit] : army->second.units) {
                std::string value = "#" + decimal(id) + " · type #" + decimal(unit.type);
                if (database && localize_info && unit.type >= 0) {
                    std::optional<std::string> name;
                    std::uint64_t matches = 0;
                    try {
                        const auto report = database->query_rows(
                            {"SELECT Description FROM Units WHERE ID = ? LIMIT 2", {unit.type}},
                            [&](const content::RulesRowView &row) {
                                ++matches;
                                const auto key = row.text("Description");
                                if (key) name = localize_info(*key);
                            }, 2);
                        if (report.all_succeeded() && matches == 1 && name &&
                            !name->empty() && !name->starts_with("TXT_KEY_"))
                            value = "#" + decimal(id) + " · " + *name;
                    } catch (const std::exception &) {
                        // A missing rules/text row cannot become a fabricated name.
                    }
                }
                page.rows.push_back({{}, std::move(value), unit.military
                    ? "TXT_KEY_MILITARY_UNIT" : "TXT_KEY_CIVILIAN_UNIT"});
            }
            // MilitaryOverview.lua sets SupplyUseValue from GetNumUnits().
            // The complete owner-scoped live roster supplies precisely that
            // count; no supply cap or deficit can be inferred from it.
            page.rows.push_back({"Supply use", decimal(army->second.units.size()),
                                 "TXT_KEY_SUPPLY_USE"});
        }
    }
    // The owned runtime has no contact graph, espionage
    // reports, victory counters, or end-game replay timeline at this boundary.
    // These pages remain explicit rather than implying a complete overview.
    for (const auto *id : {"DiploOverview", "DealsPanel", "RelationsPanel", "GlobalPanel",
                           "VictoryProgress", "WhosWinningPopup",
                           "EspionageOverview", "Ranking", "EndGameReplay"})
        unavailable(frame, id, "Live BNW overview data is not connected");
    if (!frame.pages.contains("MilitaryOverview"))
        unavailable(frame, "MilitaryOverview", "Live BNW overview data is not connected");
    return frame;
}

std::vector<OverviewSource> read_overview_sources(const assets::ResourceMountTable &resources,
    const UiContextRegistry &registry) {
    std::vector<OverviewSource> result;
    for (const auto &context : registry.contexts()) {
        if (context.owner != UiContextOwner::overview) continue;
        const auto key = context.source_key + ".xml";
        const auto mounted = resources.resolve(assets::AssetKey(key));
        const auto *handle = std::get_if<assets::AssetHandle>(&mounted.asset);
        if (!handle) throw std::runtime_error("Missing or ambiguous BNW overview XML: " + key);
        const auto bytes = handle->read_bytes(64ULL * 1024 * 1024);
        const auto xml = content::read_xml_metadata(std::string(bytes.begin(), bytes.end()));
        if (xml.name != "Context") throw std::runtime_error("Invalid BNW overview XML: " + key);
        OverviewSource source{context.id, key, selected_path(*handle), {}};
        collect(xml, source.controls);
        result.push_back(std::move(source));
    }
    // EconomicOverview.xml embeds EconomicGeneralInfo as a LuaContext. Retain
    // both selected sources when either context is focused: the parent owns the
    // tabs/close control, while the child owns the financial layout.
    auto parent = std::find_if(result.begin(), result.end(), [](const auto &source) {
        return source.context_id == "EconomicOverview";
    });
    auto child = std::find_if(result.begin(), result.end(), [](const auto &source) {
        return source.context_id == "EconomicGeneralInfo";
    });
    auto happiness = std::find_if(result.begin(), result.end(), [](const auto &source) {
        return source.context_id == "HappinessInfo";
    });
    if (parent != result.end() && child != result.end()) {
        const bool embedded = std::any_of(parent->controls.begin(), parent->controls.end(),
            [](const auto &control) {
                return control.element == "LuaContext" &&
                       control.id == "EconomicGeneralInfo";
            });
        if (embedded) {
            const auto parent_controls = parent->controls;
            const auto child_controls = child->controls;
            parent->controls.insert(parent->controls.end(), child_controls.begin(),
                                    child_controls.end());
            child->controls.insert(child->controls.end(), parent_controls.begin(),
                                   parent_controls.end());
            if (happiness != result.end() &&
                std::any_of(parent_controls.begin(), parent_controls.end(),
                    [](const auto &control) {
                        return control.element == "LuaContext" &&
                               control.id == "HappinessInfo";
                    }))
                happiness->controls.insert(happiness->controls.end(),
                    parent_controls.begin(), parent_controls.end());
        }
    }
    return result;
}

std::optional<UiCommand> overview_control_command(std::string_view context_id,
    std::string_view control_id) {
    if (control_id == "CloseButton") return UiCommand{UiCloseScreen{std::string(context_id)}};
    if ((context_id == "Demographics" || context_id == "EndGameDemographics") &&
        control_id == "BackButton")
        return UiCommand{UiCloseScreen{std::string(context_id)}};
    if (context_id == "EconomicGeneralInfo" && control_id == "GeneralInfoButton")
        return UiCommand{UiCloseScreen{"EconomicGeneralInfo"}};
    if (context_id == "EconomicGeneralInfo" && control_id == "HappinessButton")
        return UiCommand{UiOpenScreen{"HappinessInfo"}};
    if (context_id == "HappinessInfo" && control_id == "GeneralInfoButton")
        return UiCommand{UiOpenScreen{"EconomicGeneralInfo"}};
    if (context_id == "HappinessInfo" && control_id == "HappinessButton")
        return UiCommand{UiCloseScreen{"HappinessInfo"}};
    if (context_id == "EconomicOverview" && control_id == "GeneralInfoButton")
        return UiCommand{UiOpenScreen{"EconomicGeneralInfo"}};
    if (context_id == "EconomicOverview" && control_id == "HappinessButton")
        return UiCommand{UiOpenScreen{"HappinessInfo"}};
    if (context_id == "DiploOverview" && control_id == "RelationsButton")
        return UiCommand{UiOpenScreen{"RelationsPanel"}};
    if (context_id == "DiploOverview" && control_id == "DealsButton")
        return UiCommand{UiOpenScreen{"DealsPanel"}};
    if (context_id == "DiploOverview" && control_id == "GlobalPoliticsButton")
        return UiCommand{UiOpenScreen{"GlobalPanel"}};
    return std::nullopt;
}

const OverviewHit *OverviewRenderPlan::hit(std::int32_t x, std::int32_t y) const {
    for (auto it = hits.rbegin(); it != hits.rend(); ++it)
        if (x >= it->bounds.x && y >= it->bounds.y &&
            x < it->bounds.x + it->bounds.width && y < it->bounds.y + it->bounds.height)
            return &*it;
    return nullptr;
}

OverviewRenderPlan plan_overview(const OverviewSource &source, const UiFrameSnapshot &frame,
    std::int32_t width, std::int32_t height, const OverviewLocalizer &localize,
    OverviewLanguage language, const EconomicBoundFields *economic_fields) {
    OverviewRenderPlan plan;
    plan.context_id = source.context_id;
    if (width <= 0 || height <= 0) return plan;
    // Static shell and labels come from the selected BNW XML. Native UI layout
    // (anchor sides, stacks, Grid9 expansion) still belongs to the host painter.
    const auto shell = std::find_if(source.controls.begin(), source.controls.end(),
        [](const auto &control) {
            return control.element == "Grid" && control.style == "Grid9DetailFive140" &&
                   control.anchor == "C,C";
        });
    const auto shell_size = shell == source.controls.end() ? std::nullopt : pair_of(shell->size);
    const auto shell_offset = shell == source.controls.end() ? std::nullopt : pair_of(shell->offset);
    const auto panel_width = std::max(1, std::min(width - 32,
        shell_size ? shell_size->first : 1000));
    const auto panel_height = std::max(1, std::min(height - 32,
        shell_size ? shell_size->second : 658));
    const auto left = (width - panel_width) / 2;
    const auto top = (height - panel_height) / 2 +
        (shell_offset ? shell_offset->second : 0);
    plan.paints.push_back({OverviewPaintKind::rectangle,
        {left, top, panel_width, panel_height}, {}, "Grid9DetailFive140"});
    for (const auto &control : source.controls) {
        if (control.element == "Image" &&
            control.texture.find("Top512IconTrim.dds") != std::string::npos) {
            plan.paints.push_back({OverviewPaintKind::texture,
                {left + (panel_width - 256) / 2, top - 14, 256, 64}, control.texture, {}});
            break;
        }
    }
    const auto tab_stack = std::find_if(source.controls.begin(), source.controls.end(),
        [](const auto &control) {
            return control.element == "Stack" && control.anchor == "C,T" &&
                   control.size == "971,32";
        });
    const auto tab_width = tab_stack == source.controls.end()
        ? 971 : pair_of(tab_stack->size)->first;
    std::int32_t tab_x = left + (panel_width - tab_width) / 2;
    for (const auto &control : source.controls) {
        if (control.element == "Label" && control.text_key.starts_with("TXT_KEY_") &&
            (control.id == "Title" || control.style == "MenuTitleCaption")) {
            plan.paints.push_back({OverviewPaintKind::text,
                {left + 24, top + 20, panel_width - 48, 32},
                display_text(control.text_key, localize), control.style});
            break;
        }
    }
    for (const auto &control : source.controls) {
        const auto command = overview_control_command(source.context_id, control.id);
        if (!command) continue;
        OverviewRect rect;
        if (control.id == "BackButton" &&
            (source.context_id == "Demographics" ||
             source.context_id == "EndGameDemographics")) {
            // Expansion2: 960x56 bottom box, BackButton offset (14,0),
            // and the box's bottom offset of 50 from InGameSet.
            rect = {left + (panel_width - 960) / 2 + 14,
                    top + panel_height + 50 - 32, 150, 32};
        }
        else if (control.id == "CloseButton") {
            const auto authored = pair_of(control.size);
            rect = {left + 34, top + panel_height - 86,
                    authored ? authored->first : 150,
                    authored ? authored->second : 32};
        }
        else {
            const auto authored = pair_of(control.size);
            rect = {tab_x, top + 44, authored ? authored->first : 220,
                    authored ? authored->second : 24};
            tab_x += rect.width;
        }
        plan.hits.push_back({rect, *command});
        if (!control.style.empty())
            plan.paints.push_back({OverviewPaintKind::rectangle, rect, {}, control.style});
        plan.paints.push_back({OverviewPaintKind::text, rect,
            display_text(control.id == "CloseButton" || control.id == "BackButton"
                             ? "TXT_KEY_CLOSE" :
                         control.text_key.empty() ? control.id : control.text_key, localize),
            control.style});
    }
    const auto show_unavailable = [&](std::string reason) {
        plan.unresolved.push_back(reason);
        plan.paints.push_back({OverviewPaintKind::text,
            {left + 34, top + 108, panel_width - 68, 30},
            unavailable_text(reason, language), "TwCenMT20"});
    };
    const auto *state = frame.find<OverviewFrameState>();
    if (!state || !frame.active_player_id() || state->player_id != *frame.active_player_id()) {
        show_unavailable("Overview snapshot for active player is unavailable");
        return plan;
    }
    const auto *economic_popup = frame.find<PopupFrame>();
    const bool owned_economy = source.context_id == "EconomicOverview" &&
        economic_fields && economic_fields->available &&
        economic_fields->player_id == *frame.active_player_id() &&
        frame.turn() == economic_fields->turn &&
        frame.frame() == economic_fields->frame && economic_popup &&
        economic_popup->player_id == economic_fields->player_id &&
        economic_popup->turn == economic_fields->turn &&
        economic_popup->owner_sequence == economic_fields->owner_sequence &&
        economic_popup->events.size() == 1 &&
        economic_popup->events.front().context_id == "EconomicOverview";
    if (source.context_id == "EconomicOverview" && economic_fields &&
        !owned_economy) {
        show_unavailable("EconomicOverview field owner is stale or incomplete");
        return plan;
    }
    const auto found = state->pages.find(source.context_id == "EconomicOverview"
        ? owned_economy && economic_fields->tab == EconomicTab::happiness
            ? "HappinessInfo" : "EconomicGeneralInfo"
        : source.context_id);
    if (found == state->pages.end()) {
        show_unavailable("Live page data is unavailable");
        return plan;
    }
    if (source.context_id == "Demographics" ||
        source.context_id == "EndGameDemographics") {
        const auto has_control = [&](std::string_view id) {
            return std::any_of(source.controls.begin(), source.controls.end(),
                [&](const OverviewControl &control) { return control.id == id; });
        };
        const auto has_text = [&](std::string_view key) {
            return std::any_of(source.controls.begin(), source.controls.end(),
                [&](const OverviewControl &control) { return control.text_key == key; });
        };
        constexpr const char *headers[] = {
            "TXT_KEY_DEMOGRAPHICS_TYPE", "TXT_KEY_DEMOGRAPHICS_RANK",
            "TXT_KEY_DEMOGRAPHICS_VALUE", "TXT_KEY_DEMOGRAPHICS_RIVAL_BEST",
            "TXT_KEY_DEMOGRAPHICS_RIVAL_AVERAGE", "TXT_KEY_DEMOGRAPHICS_RIVAL_WORST"};
        if (!has_control("InGameSet") || !has_control("MainStack") ||
            !has_control("BackButton") ||
            !std::all_of(std::begin(headers), std::end(headers), has_text)) {
            show_unavailable("Selected Demographics layout is unavailable");
            return plan;
        }
        // Expansion2 Demographics.xml: centred 956-pixel table, 24-pixel
        // header, 54-pixel Entry instances and columns 206 + 5*150.
        const auto x = (width - 956) / 2;
        const auto header_y = (height - 658) / 2 + 97;
        if (x < 0 || header_y < 0 || header_y + 24 + 8 * 54 > height) {
            show_unavailable("Selected Demographics layout does not fit the frame");
            return plan;
        }
        std::int32_t column_x = x;
        for (std::size_t column = 0; column < std::size(headers); ++column) {
            const auto column_width = column == 0 ? 206 : 150;
            plan.paints.push_back({OverviewPaintKind::text,
                {column_x, header_y, column_width, 24},
                display_text(headers[column], localize), "TwCenMT20"});
            column_x += column_width;
        }
        constexpr const char *metrics[] = {
            "TXT_KEY_DEMOGRAPHICS_POPULATION", "TXT_KEY_DEMOGRAPHICS_FOOD",
            "TXT_KEY_DEMOGRAPHICS_PRODUCTION", "TXT_KEY_DEMOGRAPHICS_GOLD",
            "TXT_KEY_DEMOGRAPHICS_LAND", "TXT_KEY_DEMOGRAPHICS_ARMY",
            "TXT_KEY_DEMOGRAPHICS_APPROVAL", "TXT_KEY_DEMOGRAPHICS_LITERACY"};
        const auto approval = std::find_if(found->second.rows.begin(), found->second.rows.end(),
            [](const OverviewRow &row) {
                return row.label_key == "TXT_KEY_DEMOGRAPHICS_APPROVAL";
            });
        for (std::size_t index = 0; index < std::size(metrics); ++index) {
            const auto y = header_y + 24 + static_cast<std::int32_t>(index) * 54;
            plan.paints.push_back({OverviewPaintKind::text, {x + 24, y, 182, 54},
                display_text(metrics[index], localize), "TwCenMT20"});
            const auto value = index == 6 && approval != found->second.rows.end()
                ? approval->value : "N/A";
            plan.paints.push_back({OverviewPaintKind::text, {x + 206 + 150, y, 150, 54},
                value, "TwCenMT20"});
            for (const auto offset : {206, 506, 656, 806})
                plan.paints.push_back({OverviewPaintKind::text,
                    {x + offset, y, 150, 54}, "N/A", "TwCenMT20"});
        }
        plan.unresolved.push_back(found->second.unavailable_reason);
        return plan;
    }
    if (!found->second.unavailable_reason.empty())
        show_unavailable(found->second.unavailable_reason);
    if (source.context_id == "EconomicOverview" ||
        source.context_id == "EconomicGeneralInfo") {
        if (owned_economy && economic_fields->tab == EconomicTab::happiness) {
            // The active child XML has three scrollable columns. Its native
            // happiness components are not yet frame-bound; do not print XML
            // control IDs as player-facing labels or invent their values.
            show_unavailable("Happiness components are unavailable");
            for (const auto &field : economic_fields->fields)
                if (!field.value)
                    plan.unresolved.push_back(field.control_id + ": " +
                        field.unavailable_reason);
            return plan;
        }
        const auto box = std::find_if(source.controls.begin(), source.controls.end(),
            [](const auto &control) {
                return control.element == "Box" && control.anchor == "C,C" &&
                       control.size == "900,533";
            });
        const auto gold_scroll = std::find_if(source.controls.begin(), source.controls.end(),
            [](const auto &control) { return control.id == "GoldScroll"; });
        const auto main_scroll = std::find_if(source.controls.begin(), source.controls.end(),
            [](const auto &control) { return control.id == "MainScroll"; });
        if (box == source.controls.end() || gold_scroll == source.controls.end() ||
            main_scroll == source.controls.end()) {
            show_unavailable("Selected EconomicGeneralInfo layout is unavailable");
            return plan;
        }
        const auto box_size = pair_of(box->size);
        const auto box_offset = pair_of(box->offset);
        const auto gold_offset = pair_of(gold_scroll->offset);
        const auto gold_size = pair_of(gold_scroll->size);
        const auto main_offset = pair_of(main_scroll->offset);
        const auto main_size = pair_of(main_scroll->size);
        if (!box_size || !box_offset || !gold_offset || !gold_size ||
            !main_offset || !main_size) {
            show_unavailable("Selected economic geometry is invalid");
            return plan;
        }
        const auto bx = (width - box_size->first) / 2 + box_offset->first;
        const auto by = (height - box_size->second) / 2 + box_offset->second;
        const auto gx = bx + gold_offset->first;
        const auto gy = by + gold_offset->second;
        const auto mx = bx + main_offset->first;
        const auto my = by + main_offset->second;
        struct Field { const char *id, *key; bool income_heading, expense_heading; };
        constexpr Field fields[] = {
            {"TotalGoldValue", "TXT_KEY_EO_TOTAL_GOLD", false, false},
            {"NetGoldValue", "TXT_KEY_EO_NET_PER_TURN", false, false},
            {"ScienceLostValue", "TXT_KEY_EO_PENALTY", false, false},
            {"GrossGoldValue", "TXT_KEY_EO_GROSS", false, false},
            {"TotalExpenseValue", "TXT_KEY_EO_TOTAL_EXPENSE", false, false},
            {"CityIncomeValue", "TXT_KEY_EO_INCOME_CITIES", true, false},
            {"DiploIncomeValue", "TXT_KEY_EO_DIPLOMACY", false, false},
            {"ReligionIncomeValue", "TXT_KEY_EO_RELIGION", false, false},
            {"TradeIncomeValue", "TXT_KEY_EO_INCOME_TRADE_DETAILS", false, false},
            {"UnitExpenseValue", "TXT_KEY_EO_UNITS", false, true},
            {"BuildingExpenseValue", "TXT_KEY_EO_BUILDINGS", false, false},
            {"TileExpenseValue", "TXT_KEY_EO_IMPROVEMENTS", false, false},
            {"DiploExpenseValue", "TXT_KEY_EO_DIPLOMACY", false, false},
        };
        auto y = gy;
        for (const auto &field : fields) {
            if (std::string_view(field.id) == "ScienceLostValue" &&
                found->second.science_lost_visible == false) continue;
            if (field.income_heading || field.expense_heading) {
                const auto heading = field.income_heading ? "TXT_KEY_EO_INCOME"
                                                          : "TXT_KEY_EO_EXPENSES";
                plan.paints.push_back({OverviewPaintKind::text,
                    {gx, y, gold_size->first, 45}, display_text(heading, localize),
                    "TwCenMT20"});
                y += 45;
            }
            const auto control = std::find_if(source.controls.begin(), source.controls.end(),
                [&](const auto &candidate) { return candidate.id == field.id; });
            const auto label = std::find_if(source.controls.begin(), source.controls.end(),
                [&](const auto &candidate) { return candidate.text_key == field.key; });
            if (control == source.controls.end() || label == source.controls.end()) {
                plan.unresolved.push_back(std::string("Selected XML lacks ") + field.id);
                continue;
            }
            const auto value = std::find_if(found->second.rows.begin(), found->second.rows.end(),
                [&](const auto &row) { return row.label == field.id; });
            const auto bound = owned_economy ?
                std::find_if(economic_fields->fields.begin(), economic_fields->fields.end(),
                    [&](const auto &candidate) { return candidate.control_id == field.id; })
                : std::vector<EconomicFieldValue>::const_iterator{};
            const bool has_bound = owned_economy && bound != economic_fields->fields.end();
            plan.paints.push_back({OverviewPaintKind::text, {gx + 5, y, 145, 24},
                display_text(field.key, localize), "TwCenMT18"});
            plan.paints.push_back({OverviewPaintKind::text, {gx + 151, y, 54, 24},
                owned_economy ? has_bound ? bound->value.value_or("N/A") : "N/A"
                              : value == found->second.rows.end() ? "N/A" : value->value,
                "TwCenMT18"});
            if (owned_economy && (!has_bound || !bound->value))
                plan.unresolved.push_back(std::string(field.id) + ": " +
                    (has_bound ? bound->unavailable_reason : "field is not source-bound"));
            else if (!owned_economy && value == found->second.rows.end())
                plan.unresolved.push_back(std::string(field.id) + " has no completed live source");
            y += 24;
        }
        plan.paints.push_back({OverviewPaintKind::text, {mx + 48, my, 240, 24},
            display_text("TXT_KEY_PRODPANEL_CITY_NAME", localize), "TwCenMT18"});
        auto row_y = my + 28;
        for (const auto &city : found->second.cities) {
            if (row_y + 34 > my + main_size->second) {
                plan.unresolved.push_back("More cities require the authored MainScroll");
                break;
            }
            plan.paints.push_back({OverviewPaintKind::text, {mx, row_y, 48, 34},
                decimal(city.population), "TwCenMT20"});
            plan.paints.push_back({OverviewPaintKind::text, {mx + 48, row_y, 240, 34},
                city.name.value_or("N/A"), "TwCenMT20"});
            for (const auto column : {288, 336, 384, 432, 480, 528, 576})
                plan.paints.push_back({OverviewPaintKind::text,
                    {mx + column, row_y, 48, 34}, "N/A", "TwCenMT18"});
            row_y += 34;
        }
        if (std::any_of(found->second.cities.begin(), found->second.cities.end(),
                        [](const OverviewCityRow &city) { return !city.name; }))
            plan.unresolved.push_back("Some live city names have no matching saved source");
        plan.unresolved.push_back("City defense, yields and production details are unavailable from this live overview source");
        return plan;
    }
    // The explicit missing-backend line occupies the first row's area.
    std::int32_t y = top + (found->second.unavailable_reason.empty() ? 104 : 144);
    for (const auto &row : found->second.rows) {
        if (y + 28 >= top + panel_height - 64) {
            plan.unresolved.push_back("More rows require the authored scroll container");
            break;
        }
        plan.paints.push_back({OverviewPaintKind::text, {left + 34, y, panel_width / 2 - 48, 26},
            display_row_label(row, localize, language), "TwCenMT20"});
        plan.paints.push_back({OverviewPaintKind::text, {left + panel_width / 2, y,
            panel_width / 2 - 48, 26}, display_text(row.value, localize), "TwCenMT20"});
        y += 30;
    }
    return plan;
}

} // namespace civ5::ui
