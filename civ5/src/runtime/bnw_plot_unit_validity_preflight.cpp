#include "civ5/runtime/bnw_plot_unit_validity_preflight.hpp"
#include <stdexcept>
#include <string>

namespace civ5::runtime {
namespace {
void require(bool condition,const char *reason) {
    if(!condition) throw std::runtime_error(reason);
}
const char *check_name(BnwPlotUnitCheck check) {
    switch(check) {
    case BnwPlotUnitCheck::saved_identity:return "saved_identity";
    case BnwPlotUnitCheck::delayed_death:return "delayed_death";
    case BnwPlotUnitCheck::at_plot:return "at_plot";
    case BnwPlotUnitCheck::cargo:return "cargo";
    case BnwPlotUnitCheck::in_combat:return "in_combat";
    case BnwPlotUnitCheck::stack_limit:return "friendly_units_of_type";
    case BnwPlotUnitCheck::valid_domain:return "valid_domain";
    case BnwPlotUnitCheck::can_enter_territory:return "can_enter_territory";
    case BnwPlotUnitCheck::same_owner_team:return "same_owner_team";
    case BnwPlotUnitCheck::unit_team:return "unit_team";
    case BnwPlotUnitCheck::plot_team:return "plot_team";
    case BnwPlotUnitCheck::visible_enemy:return "visible_enemy";
    case BnwPlotUnitCheck::invisible:return "invisible";
    }
    return "unknown";
}
} // namespace

BnwPlotUnitValidityPreflight preflight_bnw_verify_unit_valid_plot(
    const LiveSessionState &state,std::uint64_t plot_index,
    std::int64_t plot_unit_limit,const BnwPlotUnitReadServices &services) {
    BnwPlotUnitValidityPreflight result;
    try {
        require(plot_unit_limit>=0,"BNW plot unit limit is invalid");
        const auto source=preflight_bnw_map_occupied(state.world);
        require(source.shape_valid,"BNW plot unit source roster is invalid");
        require(state.world.map_projection &&
                plot_index<state.world.map_projection->cells.size(),
                "BNW plot unit index is outside the map");
        const auto &cell=state.world.map_projection->cells[plot_index];
        require(cell.units && cell.owner,
                "BNW plot unit owner or roster is unresolved");
        result.source_valid=true;
        bool all_live_inputs=true;
        const auto add=[&](saves::SavedPlotUnitReference handle,unsigned pass,
                           BnwPlotUnitCheck check,bool resolved,std::int64_t value,
                           bool live_proof=false) {
            result.steps.push_back({plot_index,handle,pass,check,resolved,
                                    live_proof,value});
        };
        const auto missing=[&](saves::SavedPlotUnitReference handle,
                               BnwPlotUnitCheck check) {
            result.reason="BNW plot " + std::to_string(plot_index) +
                " unit " + std::to_string(handle.owner) + ":" +
                std::to_string(handle.id) + " requires GameCore predicate " +
                check_name(check);
        };
        const auto ask_number=[&](const BnwUnitValidityNumber &query,
                                  saves::SavedPlotUnitReference handle,
                                  unsigned pass,BnwPlotUnitCheck check)
                                  -> std::optional<std::int64_t> {
            const auto answer=query?query(state,plot_index,handle):std::nullopt;
            add(handle,pass,check,answer.has_value(),answer.value_or(0),
                answer.has_value());
            if(!answer) missing(handle,check);
            return answer;
        };
        const auto ask_bool=[&](const BnwUnitValidityProbe &query,
                                saves::SavedPlotUnitReference handle,
                                unsigned pass,BnwPlotUnitCheck check)
                                -> std::optional<bool> {
            const auto answer=query?query(state,plot_index,handle):std::nullopt;
            add(handle,pass,check,answer.has_value(),answer.value_or(false),
                answer.has_value());
            if(!answer) missing(handle,check);
            return answer;
        };
        const auto basic=[&](const BnwUnitValidityProbe &query,
                             saves::SavedPlotUnitReference handle,unsigned pass,
                             BnwPlotUnitCheck check,bool saved)
                             -> std::optional<bool> {
            if(!query) {
                all_live_inputs=false;
                add(handle,pass,check,true,saved);
                return saved;
            }
            const auto answer=query(state,plot_index,handle);
            add(handle,pass,check,answer.has_value(),answer.value_or(false),
                answer.has_value());
            if(!answer) {missing(handle,check);return std::nullopt;}
            require(*answer==saved,
                    "BNW live unit registry disagrees with saved map inputs");
            return answer;
        };
        // CvPlot copies one ordered linked-list snapshot for both passes.
        // The canonical raw/projected roster was checked above.
        std::vector<BnwOccupiedPlotUnit> old;
        for(const auto &unit:source.units)
            if(unit.plot==plot_index) old.push_back(unit);
        require(old.size()==cell.units->size(),
                "BNW plot unit preflight snapshot roster changed");
        for(const auto &unit:old) {
            const auto handle=unit.handle;
            const auto exists=basic(services.exists,handle,1,
                BnwPlotUnitCheck::saved_identity,unit.saved_unit_found);
            if(!exists) return result;
            if(!*exists) continue; // GetPlayerUnit returned NULL.
            const auto death=basic(services.delayed_death,handle,1,
                BnwPlotUnitCheck::delayed_death,unit.delayed_death);
            if(!death) return result;
            if(*death) continue;
            const auto at=basic(services.at_plot,handle,1,
                BnwPlotUnitCheck::at_plot,unit.at_saved_plot);
            if(!at) return result;
            if(!*at) continue;
            const auto cargo=basic(services.cargo,handle,1,
                BnwPlotUnitCheck::cargo,unit.transport_link);
            if(!cargo) return result;
            if(*cargo) continue;
            const auto combat=basic(services.in_combat,handle,1,
                BnwPlotUnitCheck::in_combat,unit.combat_link);
            if(!combat) return result;
            if(*combat) continue;
            // The source predicate counts a subset of units on this plot.
            // At most the roster size can be friendly; that upper bound proves
            // the stack branch false when it does not exceed the selected cap.
            std::optional<std::int64_t> friendly;
            if(old.size()<=static_cast<std::uint64_t>(plot_unit_limit)) {
                friendly=static_cast<std::int64_t>(old.size());
                add(handle,1,BnwPlotUnitCheck::stack_limit,true,*friendly);
            } else friendly=ask_number(services.friendly_units_of_type,handle,
                                      1,BnwPlotUnitCheck::stack_limit);
            if(!friendly) return result;
            require(*friendly>=0 && static_cast<std::uint64_t>(*friendly)<=old.size(),
                    "BNW friendly unit count exceeds plot roster");
            if(*friendly>plot_unit_limit) {
                result.effects.push_back({plot_index,handle,1,
                    BnwPlotUnitEffectCause::stack_limit});
                result.reason="BNW stack-limit jump/kill effects are required";
                return result;
            }
            const auto domain=ask_bool(services.valid_domain,handle,1,
                                       BnwPlotUnitCheck::valid_domain);
            if(!domain) return result;
            if(!*domain) {
                result.effects.push_back({plot_index,handle,1,
                    BnwPlotUnitEffectCause::invalid_domain_or_territory});
                result.reason="BNW invalid-domain jump/kill effects are required";
                return result;
            }
            const auto territory=ask_bool(services.can_enter_territory,handle,1,
                                           BnwPlotUnitCheck::can_enter_territory);
            if(!territory) return result;
            if(!*territory) {
                result.effects.push_back({plot_index,handle,1,
                    BnwPlotUnitEffectCause::invalid_domain_or_territory});
                result.reason="BNW territory jump/kill effects are required";
                return result;
            }
        }
        if(*cell.owner>=0) {
            for(const auto &unit:old) {
                const auto handle=unit.handle;
                const auto exists=basic(services.exists,handle,2,
                    BnwPlotUnitCheck::saved_identity,unit.saved_unit_found);
                if(!exists) return result;
                if(!*exists) continue;
                const auto death=basic(services.delayed_death,handle,2,
                    BnwPlotUnitCheck::delayed_death,unit.delayed_death);
                if(!death) return result;
                if(*death) continue;
                const auto at=basic(services.at_plot,handle,2,
                    BnwPlotUnitCheck::at_plot,unit.at_saved_plot);
                if(!at) return result;
                if(!*at) continue;
                const auto combat=basic(services.in_combat,handle,2,
                    BnwPlotUnitCheck::in_combat,unit.combat_link);
                if(!combat) return result;
                if(*combat) continue;
                // When the same player owns the plot and unit, both teams are
                // that player's live team. Otherwise the team predicates must
                // be supplied explicitly, before visible-enemy/invisibility.
                if(handle.owner==*cell.owner) {
                    add(handle,2,BnwPlotUnitCheck::same_owner_team,true,1);
                    continue;
                }
                const auto unit_team=ask_number(services.unit_team,handle,2,
                                                BnwPlotUnitCheck::unit_team);
                if(!unit_team) return result;
                const auto plot_team=ask_number(services.plot_team,handle,2,
                                                BnwPlotUnitCheck::plot_team);
                if(!plot_team) return result;
                if(*unit_team==*plot_team) continue;
                const auto enemy=ask_bool(services.visible_enemy,handle,2,
                                           BnwPlotUnitCheck::visible_enemy);
                if(!enemy) return result;
                if(!*enemy) continue;
                const auto invisible=ask_bool(services.invisible,handle,2,
                                               BnwPlotUnitCheck::invisible);
                if(!invisible) return result;
                if(!*invisible) {
                    result.effects.push_back({plot_index,handle,2,
                        BnwPlotUnitEffectCause::visible_enemy});
                    result.reason="BNW visible-enemy jump/kill effects are required";
                    return result;
                }
            }
        }
        result.complete_no_effect=all_live_inputs;
        if(!all_live_inputs)
            result.reason="BNW live unit registry predicates remain unverified";
    } catch(const std::exception &error) {
        result.source_valid=false;
        result.reason=error.what();
    }
    return result;
}

LiveActionResult run_bnw_verified_no_effect_plot_units(
    LiveSessionState &state,std::uint64_t plot_index,
    std::int64_t plot_unit_limit,const BnwPlotUnitReadServices &services) {
    const auto plan=preflight_bnw_verify_unit_valid_plot(
        state,plot_index,plot_unit_limit,services);
    if(!plan.source_valid || !plan.complete_no_effect || !plan.effects.empty())
        return {LiveActionOutcome::rejected,
            plan.reason.empty()?"BNW unit validity proof is incomplete":plan.reason};
    return {LiveActionOutcome::committed,{}};
}

} // namespace civ5::runtime
