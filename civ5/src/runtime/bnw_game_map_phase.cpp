#include "civ5/runtime/bnw_game_map_phase.hpp"
#include "civ5/runtime/bnw_map_occupied_preflight.hpp"
#include <climits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>

namespace civ5::runtime {
namespace {
LiveActionResult reject(const char *reason) {
    return {LiveActionOutcome::rejected,reason};
}
LiveActionResult resolve_live_improvements(
    LiveSessionState &draft,const BnwPlotDomainRules &rules) {
    std::map<std::uint32_t,std::int64_t> by_hash;
    for(const auto &[id,rule]:rules.improvements) {
        if(id<0 || id>INT32_MAX || rule.hash==0 ||
           !by_hash.emplace(rule.hash,id).second)
            return reject("BNW active improvement ID/hash is ambiguous");
    }
    auto &world=draft.world;
    for(std::size_t index=0;index<world.map_plots->plots.size();++index) {
        const auto &plot=world.map_plots->plots[index];
        auto &cell=world.map_projection->cells[index];
        if(!plot.version || *plot.version<1 || *plot.version>7)
            return {LiveActionOutcome::rejected,
                "BNW plot " + std::to_string(index) +
                ": improvement save version is unsupported"};
        const bool hashed=*plot.version>=5;
        const auto wanted=hashed?"improvement_type_hash":"improvement_type";
        const saves::NamedSaveInteger *raw=nullptr;
        for(const auto &entry:plot.fields)
            if(entry.name && std::string_view(entry.name)==wanted) {
                if(raw) return {LiveActionOutcome::rejected,
                    "BNW plot " + std::to_string(index) +
                    ": improvement source field is duplicated"};
                raw=&entry;
            }
        if(!raw) return {LiveActionOutcome::rejected,
            "BNW plot " + std::to_string(index) +
            ": improvement source field is unavailable"};
        if(hashed) {
            if(raw->value<0 || raw->value>UINT32_MAX ||
               !cell.improvement_type_hash ||
               *cell.improvement_type_hash!=static_cast<std::uint64_t>(raw->value))
                return {LiveActionOutcome::rejected,
                    "BNW plot " + std::to_string(index) +
                    ": raw/projected improvement hash disagrees"};
            if(raw->value==0) {
                if(cell.improvement_type && *cell.improvement_type!=-1)
                    return {LiveActionOutcome::rejected,
                        "BNW plot " + std::to_string(index) +
                        ": no-improvement projection disagrees"};
                continue;
            }
            const auto found=by_hash.find(static_cast<std::uint32_t>(raw->value));
            if(found==by_hash.end() ||
               (cell.improvement_type && *cell.improvement_type!=found->second))
                return {LiveActionOutcome::rejected,
                    "BNW plot " + std::to_string(index) +
                    ": improvement hash is absent from active rules"};
            cell.improvement_type=found->second;
        } else {
            if(raw->value<-1 || raw->value>INT32_MAX ||
               !cell.improvement_type || *cell.improvement_type!=raw->value ||
               (raw->value!=-1 && !rules.improvements.contains(raw->value)))
                return {LiveActionOutcome::rejected,
                    "BNW plot " + std::to_string(index) +
                    ": legacy improvement ID is absent from active rules"};
        }
    }
    return {LiveActionOutcome::committed,{}};
}
LiveActionResult run_unitless_map_turn(
    LiveSessionState &state,const BnwMapTurnRules &rules) {
    const auto &world=state.world;
    if(world.game_state.layout!=saves::GameStateLayout::expansion2_version1 ||
       world.map_header.layout!=saves::GameStateLayout::expansion2_version1 ||
       !world.map_plots ||
       world.map_plots->layout!=saves::GameStateLayout::expansion2_version1 ||
       !world.map_projection ||
       world.map_plots->plots.size()!=world.map_header.grid.size() ||
       world.map_projection->cells.size()!=world.map_header.grid.size())
        return reject("BNW game map phase requires a complete owned map");
    for(std::size_t index=0;index<world.map_plots->plots.size();++index) {
        const auto &plot=world.map_plots->plots[index];
        const auto &cell=world.map_projection->cells[index];
        if(plot.index!=index || cell.index!=index || !cell.units)
            return reject("BNW game map phase plot identity or unit roster is unresolved");
        if(!cell.units->empty())
            return reject("BNW game map phase requires unit validity GameCore services");
        const saves::PlotArrayObservation *raw_units=nullptr;
        for(const auto &array:plot.arrays)
            if(array.name && std::string_view(array.name)=="units") {
                if(raw_units)
                    return reject("BNW game map phase has duplicate raw unit rosters");
                raw_units=&array;
            }
        if(!raw_units || raw_units->columns!=2 || !raw_units->values.empty())
            return reject("BNW game map phase raw plot units are unresolved");
        const saves::NamedSaveInteger *anchor=nullptr;
        for(const auto &field:plot.fields)
            if(field.name && std::string_view(field.name)=="world_anchor") {
                if(anchor)
                    return reject("BNW game map phase has duplicate world anchors");
                anchor=&field;
            }
        if(!anchor || anchor->value!=-1)
            return reject("BNW game map phase requires world anchor removal service");
    }
    // CvPlot::verifyUnitValidPlot has two empty loops for an empty roster.
    // CvPlot::SetWorldAnchor(NO_WORLD_ANCHOR) sends no engine event when the
    // previous anchor is already NO_WORLD_ANCHOR. The existing map-turn core
    // now owns all serializable duration, land-score, and anchor-data writes.
    BnwMapTurnServices services;
    services.verify_units=[limit=rules.plot_unit_limit](
        LiveSessionState &draft,std::uint64_t index) {
        return verify_bnw_plot_units(draft,index,limit,{});
    };
    // Work on a detached draft even when this function is called outside the
    // turn scheduler. A later malformed plot cannot leak earlier increments.
    auto draft=state;
    auto result=run_bnw_map_turn_phase(
        draft,rules.ownership_score_duration_threshold,services);
    if(result.outcome==LiveActionOutcome::committed)
        state=std::move(draft);
    return result;
}
} // namespace

BnwTurnHandler make_bnw_game_map_phase_handler(
    const content::GlobalDefineCache &defines) {
    const auto rules=resolve_bnw_map_turn_rules(defines);
    return {BnwTurnPhase::game_map,
            [rules](LiveSessionState &state,std::int64_t player) {
                if(player!=-1)
                    return reject("BNW game map requires the game phase");
                return run_unitless_map_turn(state,rules);
            }};
}

BnwTurnHandler make_bnw_game_map_phase_handler(
    const content::GlobalDefineCache &defines,
    BnwPlotDomainRules improvement_rules,
    BnwPlotUnitReadServices unit_reads,
    BnwWorldAnchorEmitter anchor_emitter) {
    const auto rules=resolve_bnw_map_turn_rules(defines);
    auto pending=std::make_shared<std::optional<BnwWorldAnchorJournal>>();
    auto run=[rules,improvement_rules=std::move(improvement_rules),
              unit_reads=std::move(unit_reads),
              emitter=std::move(anchor_emitter),pending](
              LiveSessionState &state,std::int64_t player)->LiveActionResult {
        if(player!=-1) return reject("BNW game map requires the game phase");
        // Never discard a committed but unpublished engine event. A caller
        // whose later game phase rolled back must create a new handler.
        if(*pending && ((*pending)->uncertain() ||
           (*pending)->delivered()<(*pending)->pending().size()))
            return reject("BNW map phase has an unpublished world anchor journal");
        const auto roster=preflight_bnw_map_occupied(state.world);
        if(!roster.shape_valid)
            return {LiveActionOutcome::rejected,roster.reason};
        if(!roster.anchors.empty() && !emitter)
            return reject("BNW GameplayWorldAnchor removal emitter unavailable");
        std::set<std::uint64_t> occupied;
        for(const auto &unit:roster.units) occupied.insert(unit.plot);
        auto draft=state;
        const auto resolved=resolve_live_improvements(draft,improvement_rules);
        if(resolved.outcome!=LiveActionOutcome::committed) return resolved;
        BnwWorldAnchorJournal staged(emitter);
        BnwMapTurnServices services;
        services.verify_units=[occupied=std::move(occupied),
                               unit_reads,rules](
                               LiveSessionState &world,std::uint64_t index) {
            if(occupied.contains(index))
                return run_bnw_verified_no_effect_plot_units(
                    world,index,rules.plot_unit_limit,unit_reads);
            const auto &cell=world.world.map_projection->cells.at(index);
            if(!cell.units || !cell.units->empty())
                return LiveActionResult{LiveActionOutcome::rejected,
                    "BNW empty plot unit roster changed during map turn"};
            const auto &plot=world.world.map_plots->plots.at(index);
            const saves::PlotArrayObservation *raw=nullptr;
            for(const auto &entry:plot.arrays)
                if(entry.name && std::string_view(entry.name)=="units") {
                    if(raw) return LiveActionResult{LiveActionOutcome::rejected,
                        "BNW empty plot has duplicate raw unit rosters"};
                    raw=&entry;
                }
            if(!raw || raw->columns!=2 || !raw->values.empty())
                return LiveActionResult{LiveActionOutcome::rejected,
                    "BNW empty plot raw unit roster changed during map turn"};
            return LiveActionResult{LiveActionOutcome::committed,{}};
        };
        services.stage_anchor_removal=[&staged](
            LiveSessionState &world,std::uint64_t index,std::int64_t old) {
            const auto plan=plan_bnw_world_anchor_clear(world,index);
            if(!plan.valid || !plan.removal || plan.removal->old_anchor!=old)
                return LiveActionResult{LiveActionOutcome::rejected,
                    plan.reason.empty()?"BNW old world anchor changed":plan.reason};
            return stage_bnw_world_anchor_clear(world,index,staged);
        };
        auto result=run_bnw_map_turn_phase(
            draft,rules.ownership_score_duration_threshold,services);
        if(result.outcome!=LiveActionOutcome::committed) return result;
        pending->emplace(std::move(staged));
        state=std::move(draft);
        return result;
    };
    auto after=[pending](LiveSession &session)->LiveActionResult {
        if(!*pending)
            return reject("BNW map phase has no committed anchor journal");
        const auto published=(*pending)->publish(session.state());
        if(published.status==BnwWorldAnchorPublicationStatus::complete)
            return {LiveActionOutcome::committed,{}};
        const auto prefix=published.status==BnwWorldAnchorPublicationStatus::uncertain?
            "BNW world anchor publication uncertain: ":
            "BNW world anchor publication rejected: ";
        return {LiveActionOutcome::rejected,
                std::string(prefix)+published.reason};
    };
    return {BnwTurnPhase::game_map,std::move(run),std::move(after)};
}
} // namespace civ5::runtime
