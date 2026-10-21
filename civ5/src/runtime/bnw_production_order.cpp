#include "civ5/runtime/bnw_production_order.hpp"
#include <exception>
#include <string>

namespace civ5::runtime {
namespace {
using Order=CityYieldState::ProductionOrder;

const std::set<std::int64_t> *known_ids(const ProductionProgressRules &rules,
                                       BnwProductionKind kind) {
    switch (kind) {
    case BnwProductionKind::train: return &rules.units;
    case BnwProductionKind::construct: return &rules.buildings;
    case BnwProductionKind::create: return &rules.projects;
    case BnwProductionKind::prepare: return &rules.specialists;
    case BnwProductionKind::maintain: return &rules.processes;
    }
    return nullptr;
}

std::string_view lua_event(BnwProductionKind kind) {
    switch (kind) {
    case BnwProductionKind::train: return "CityCanTrain";
    case BnwProductionKind::construct: return "CityCanConstruct";
    case BnwProductionKind::create: return "CityCanCreate";
    case BnwProductionKind::prepare: return "CityCanPrepare";
    case BnwProductionKind::maintain: return "CityCanMaintain";
    }
    return {};
}

bool valid_order(const ProductionProgressRules &rules, const Order &order) {
    const auto *ids=known_ids(rules,static_cast<BnwProductionKind>(order.type));
    return ids && ids->contains(order.item);
}
} // namespace

LiveActionResult choose_bnw_city_production(
    LiveSession &session, std::int64_t owner, std::int64_t city,
    const BnwProductionChoice &choice, const ProductionProgressRules &rules,
    const BnwProductionOrderServices &services) {
    if (!services.lua_ready || !services.core_can_order_read_only ||
        !services.lua_test_all_read_only || !services.apply_owned_effect ||
        !services.dirty_after_commit)
        return {LiveActionOutcome::rejected,
                "BNW production requires native, Lua, owned-effect and dirty services"};
    if (!services.lua_ready())
        return {LiveActionOutcome::rejected, "BNW production Lua host is unavailable"};
    if (rules.profile!=content::TechnologyProfile::bnw ||
        !known_ids(rules,choice.kind) ||
        !known_ids(rules,choice.kind)->contains(choice.item))
        return {LiveActionOutcome::rejected, "BNW production Info is unavailable"};
    if (choice.kind==BnwProductionKind::train && choice.secondary==-1 &&
        !services.default_unit_ai_read_only)
        return {LiveActionOutcome::rejected,
                "BNW unit default AI service is unavailable"};

    auto result=apply_live_action(session,[&](LiveSessionState &state) {
        if (state.cities.profile!=content::TechnologyProfile::bnw ||
            !state.setup.players.contains(owner) ||
            !state.cities.players.contains(owner) ||
            !state.cities.players.at(owner).cities.contains(city))
            return LiveActionResult{LiveActionOutcome::rejected,
                                    "BNW production city is unavailable"};
        auto &city_state=state.cities.players.at(owner).cities.at(city);
        if (city_state.owner!=owner || city_state.id!=city ||
            !city_state.production_orders || !city_state.production_progress ||
            city_state.production_progress->source_profile!=
                content::TechnologyProfile::bnw ||
            !city_state.production_progress->fully_resolved)
            return LiveActionResult{LiveActionOutcome::rejected,
                                    "BNW production queue is unresolved"};
        auto &queue=*city_state.production_orders;
        for (const auto &old : queue)
            if (!valid_order(rules,old))
                return LiveActionResult{LiveActionOutcome::rejected,
                                        "Existing BNW production Info is unavailable"};

        // CvCity::clearOrderQueue repeatedly calls popOrder(0): release the
        // old making counter, operation slot and active process before erasing
        // each head, then activate the next head. All effects remain detached.
        while (!queue.empty()) {
            const auto old=queue.front();
            services.apply_owned_effect(state,owner,city,
                BnwProductionOwnedEffect::release_commitment,old);
            services.apply_owned_effect(state,owner,city,
                BnwProductionOwnedEffect::uncommit_operation,old);
            if (old.type==static_cast<std::int64_t>(BnwProductionKind::maintain))
                services.apply_owned_effect(state,owner,city,
                    BnwProductionOwnedEffect::stop_process,old);
            queue.erase(queue.begin());
            if (!queue.empty() &&
                queue.front().type==static_cast<std::int64_t>(BnwProductionKind::maintain))
                services.apply_owned_effect(state,owner,city,
                    BnwProductionOwnedEffect::start_process,queue.front());
        }

        // Native can* precedes CityCan* TestAll in the BNW SDK. The evaluator
        // sees the detached, already-cleared queue, matching pushOrder(bPop).
        if (!services.core_can_order_read_only(state,owner,city,choice))
            return LiveActionResult{LiveActionOutcome::rejected,
                                    "BNW production native eligibility rejected"};
        if (!services.lua_ready())
            throw std::runtime_error("BNW production Lua host became unavailable");
        const auto event=lua_event(choice.kind);
        if (services.lua_test_all_read_only(event,owner,city,choice.item,state)==
            std::optional<bool>{false})
            return LiveActionResult{LiveActionOutcome::rejected,
                                    "BNW production Lua eligibility rejected"};

        auto secondary=choice.secondary;
        if (choice.kind==BnwProductionKind::train && secondary==-1) {
            const auto resolved=services.default_unit_ai_read_only(state,choice.item);
            if (!resolved || *resolved<0)
                return LiveActionResult{LiveActionOutcome::rejected,
                                        "BNW unit default AI is unresolved"};
            secondary=*resolved;
        }
        const Order order{static_cast<std::int64_t>(choice.kind),choice.item,
                          secondary,choice.save,choice.rush,std::nullopt};
        services.apply_owned_effect(state,owner,city,
            BnwProductionOwnedEffect::add_commitment,order);
        queue.push_back(order);
        if (choice.kind==BnwProductionKind::maintain)
            services.apply_owned_effect(state,owner,city,
                BnwProductionOwnedEffect::start_process,order);
        return LiveActionResult{LiveActionOutcome::committed,{}};
    });
    if (result.outcome!=LiveActionOutcome::committed) return result;
    try {
        services.dirty_after_commit(owner,city);
    } catch (const std::exception &error) {
        result.reason=std::string("Production order committed; dirty notification failed: ")+error.what();
    } catch (...) {
        result.reason="Production order committed; dirty notification failed";
    }
    return result;
}

} // namespace civ5::runtime
