#include "civ5/ui/bnw_city_production_backend.hpp"
#include "civ5/content/info_metadata.hpp"
#include <array>
#include <exception>
#include <map>
#include <utility>

namespace civ5::ui {
namespace {

const std::set<std::int64_t> *known_ids(const runtime::ProductionProgressRules &rules,
                                         runtime::BnwProductionKind kind) {
    switch (kind) {
    case runtime::BnwProductionKind::train: return &rules.units;
    case runtime::BnwProductionKind::construct: return &rules.buildings;
    case runtime::BnwProductionKind::create: return &rules.projects;
    case runtime::BnwProductionKind::prepare: return &rules.specialists;
    case runtime::BnwProductionKind::maintain: return &rules.processes;
    }
    return nullptr;
}

bool complete(const runtime::BnwProductionOrderServices &services,
              const runtime::BnwProductionChoice &choice) {
    return services.lua_ready && services.core_can_order_read_only &&
           services.lua_test_all_read_only && services.apply_owned_effect &&
           services.dirty_after_commit &&
           (choice.kind != runtime::BnwProductionKind::train ||
            services.default_unit_ai_read_only);
}

std::optional<std::string> localized_choice_name(
    const content::RulesDatabase *database, const content::InfoLocalizer &localize,
    std::string_view table, std::int64_t id, std::string_view type) {
    if (!database || !localize) return std::nullopt;
    std::size_t matches = 0;
    std::optional<std::string> name;
    try {
        const auto report = database->query_rows({
            "SELECT * FROM " + std::string(table) + " WHERE ID = ? LIMIT 2", {id}},
            [&](const content::RulesRowView &row) {
                ++matches;
                const auto row_type = row.text("Type");
                if (row.integer("ID") != id || !row_type || *row_type != type)
                    return;
                const auto info = content::read_info_metadata(row, localize);
                const auto &text = info.text.value ? info.text : info.description;
                if (text.value && !text.value->empty() && *text.value != text.key &&
                    !text.value->starts_with("TXT_KEY_")) name = *text.value;
            }, 2);
        if (!report.all_succeeded() || matches != 1) return std::nullopt;
    } catch (const std::exception &) {
        return std::nullopt;
    }
    return name;
}

} // namespace

BnwCityProductionUiBackend::BnwCityProductionUiBackend(
    runtime::LiveSession &session, const runtime::ProductionProgressRules &rules,
    runtime::BnwProductionOrderServices services, BnwProductionPreview preview,
    const content::RulesDatabase *database, content::InfoLocalizer localize)
    : session_(&session), rules_(&rules), services_(std::move(services)),
      preview_(std::move(preview)), database_(database),
      localize_(std::move(localize)) {}

UiCommandResult BnwCityProductionUiBackend::availability(
    const UiGameplayCommand &command, const UiFrameSnapshot &frame) const {
    const auto *requested = command.as<CityChooseProduction>();
    if (!requested || command.source_id() != "ProductionPopup" ||
        requested->city_id < 0 || requested->item_id < 0 ||
        requested->order_type < 0 || requested->order_type > 4)
        return {UiCommandStatus::invalid, "Invalid BNW production command"};
    const auto *city = selected_city(frame);
    if (!city || city->id != requested->city_id || !frame.active_player_id() ||
        !frame.turn())
        return {UiCommandStatus::unavailable, "Selected live city is unavailable"};
    const auto owner = *frame.active_player_id();
    const auto &state = session_->state();
    if (state.setup.game_turn != *frame.turn() ||
        !state.setup.players.contains(owner) ||
        !state.setup.players.at(owner).alive ||
        state.cities.profile != content::TechnologyProfile::bnw ||
        !state.cities.players.contains(owner) ||
        !state.cities.players.at(owner).cities.contains(city->id))
        return {UiCommandStatus::unavailable, "Live BNW city frame is stale"};
    const auto &live_city = state.cities.players.at(owner).cities.at(city->id);
    if (city->owner != owner || live_city.owner != owner || live_city.id != city->id ||
        !city->queue_resolved || !live_city.production_orders ||
        !live_city.production_progress ||
        live_city.production_progress->source_profile != content::TechnologyProfile::bnw ||
        !live_city.production_progress->fully_resolved ||
        live_city.production_orders->size() != city->queue.size())
        return {UiCommandStatus::unavailable, "Live BNW production queue is unresolved or stale"};
    for (std::size_t index = 0; index < city->queue.size(); ++index)
        if (city->queue[index].type != (*live_city.production_orders)[index].type ||
            city->queue[index].item != (*live_city.production_orders)[index].item)
            return {UiCommandStatus::unavailable, "Live BNW production queue is stale"};
    for (const auto &old : *live_city.production_orders) {
        const auto *old_ids = known_ids(*rules_,
            static_cast<runtime::BnwProductionKind>(old.type));
        if (!old_ids || !old_ids->contains(old.item))
            return {UiCommandStatus::unavailable,
                    "Existing BNW production Info is unavailable"};
    }

    const runtime::BnwProductionChoice choice{
        static_cast<runtime::BnwProductionKind>(requested->order_type),
        requested->item_id};
    const auto *ids = known_ids(*rules_, choice.kind);
    if (rules_->profile != content::TechnologyProfile::bnw || !ids ||
        !ids->contains(choice.item))
        return {UiCommandStatus::unavailable, "BNW production Info is unavailable"};
    if (!complete(services_, choice) || !preview_)
        return {UiCommandStatus::unavailable,
                "BNW production eligibility, Lua or owned-effect service is unavailable"};
    try {
        if (!services_.lua_ready())
            return {UiCommandStatus::unavailable, "BNW production Lua host is unavailable"};
        const auto decision = preview_(state, owner, city->id, choice);
        return decision.status == UiCommandStatus::ready
            ? decision
            : UiCommandResult{UiCommandStatus::unavailable,
                decision.reason.empty() ? "BNW production eligibility is unavailable"
                                        : decision.reason};
    } catch (const std::exception &error) {
        return {UiCommandStatus::unavailable, error.what()};
    } catch (...) {
        return {UiCommandStatus::unavailable, "BNW production preview failed"};
    }
}

UiCommandResult BnwCityProductionUiBackend::execute(
    const UiGameplayCommand &command, const UiFrameSnapshot &frame) {
    const auto available = availability(command, frame);
    if (available.status != UiCommandStatus::ready) return available;
    const auto *requested = command.as<CityChooseProduction>();
    const runtime::BnwProductionChoice choice{
        static_cast<runtime::BnwProductionKind>(requested->order_type),
        requested->item_id};
    try {
        const auto result = runtime::choose_bnw_city_production(
            *session_, *frame.active_player_id(), requested->city_id,
            choice, *rules_, services_);
        return {result.outcome == runtime::LiveActionOutcome::committed
                    ? UiCommandStatus::performed : UiCommandStatus::unavailable,
                result.reason};
    } catch (const std::exception &error) {
        return {UiCommandStatus::unavailable, error.what()};
    } catch (...) {
        return {UiCommandStatus::unavailable, "BNW production action failed"};
    }
}

std::optional<ProductionPopupChoices> BnwCityProductionUiBackend::capture_choices(
    const UiFrameSnapshot &frame) const {
    const auto *city = selected_city(frame);
    if (!city || !frame.active_player_id() || !frame.turn()) return std::nullopt;
    ProductionPopupChoices result;
    result.player_id = *frame.active_player_id();
    result.city_id = city->id;
    result.turn = *frame.turn();
    result.backend_owner = this;
    if (rules_->profile != content::TechnologyProfile::bnw) {
        result.unresolved.push_back("Active production rules are not BNW");
        return result;
    }
    struct Category {
        runtime::BnwProductionKind kind;
        const char *table;
        const std::set<std::int64_t> *ids;
    };
    const std::array<Category, 5> categories{{
        {runtime::BnwProductionKind::train, "Units", &rules_->units},
        {runtime::BnwProductionKind::construct, "Buildings", &rules_->buildings},
        {runtime::BnwProductionKind::create, "Projects", &rules_->projects},
        {runtime::BnwProductionKind::prepare, "Specialists", &rules_->specialists},
        {runtime::BnwProductionKind::maintain, "Processes", &rules_->processes},
    }};
    for (const auto &category : categories) {
        const auto identities = rules_->info_identity.find(category.table);
        std::map<std::string, std::size_t> type_counts;
        if (identities != rules_->info_identity.end())
            for (const auto &[id, type] : identities->second)
                if (category.ids->contains(id) && type && !type->empty())
                    ++type_counts[*type];
        for (const auto id : *category.ids) {
            ProductionPopupChoice entry;
            entry.order_type = static_cast<std::int64_t>(category.kind);
            entry.item_id = id;
            if (identities != rules_->info_identity.end()) {
                const auto found = identities->second.find(id);
                if (found != identities->second.end() && found->second &&
                    !found->second->empty())
                    entry.authored_type = *found->second;
            }
            if (!entry.authored_type)
                entry.availability = {UiCommandStatus::unavailable,
                    "BNW production Type is unresolved"};
            else if (type_counts[*entry.authored_type] != 1)
                entry.availability = {UiCommandStatus::unavailable,
                    "BNW production Type is ambiguous"};
            else {
                entry.localized_name = localized_choice_name(database_, localize_,
                    category.table, id, *entry.authored_type);
                if (!entry.localized_name) {
                    entry.availability = {UiCommandStatus::unavailable,
                        "Active BNW production name is untranslated or unavailable"};
                    result.entries.push_back(std::move(entry));
                    continue;
                }
                const auto command = std::get<UiGameplayCommand>(city_choose_production(
                    city->id, entry.order_type, entry.item_id));
                entry.availability = availability(command, frame);
            }
            result.entries.push_back(std::move(entry));
        }
    }
    if (result.entries.empty())
        result.unresolved.push_back("No active BNW production Info is available");
    return result;
}

} // namespace civ5::ui
