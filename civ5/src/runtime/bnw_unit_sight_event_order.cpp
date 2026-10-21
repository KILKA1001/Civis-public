#include "civ5/runtime/bnw_unit_sight_event_order.hpp"
#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
}

BnwSightEventOrderResult trace_bnw_unit_sight_event_order(
    const LiveSessionState &state,const BnwSetXYRequest &request,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity) {
    BnwSightEventOrderResult result;
    try {
        result.exploration=apply_bnw_unit_sight_exploration_detached(
            state,request,database,types,defines,identity);
        if(!result.exploration.calculated ||
           !result.exploration.sight.detached)
            throw std::runtime_error(result.exploration.reason.empty()
                ? "BNW ordered sight events need detached exploration state"
                : result.exploration.reason);
        const auto &plan=result.exploration.sight.plan;
        const auto &counters=plan.counters;
        std::map<std::uint64_t,const BnwSightPlotDelta *> deltas;
        std::map<std::uint64_t,const BnwSightRevealEdge *> edges;
        std::size_t expected_calls=0;
        for(const auto &delta:counters.plots) {
            require(deltas.emplace(delta.plot,&delta).second &&
                    delta.new_calls<=1,
                    "BNW sight counter plan duplicated a plot");
            expected_calls+=delta.new_calls;
        }
        for(const auto &edge:plan.newly_visible)
            require(edges.emplace(edge.plot,&edge).second,
                    "BNW reveal plan duplicated a visibility edge");
        require(counters.new_call_order.size()==expected_calls,
                "BNW sight ring order omitted a new-center call");
        std::set<std::uint64_t> seen;
        std::int64_t last_ring=-1;
        auto recipients=result.exploration.dirty_players;
        std::sort(recipients.begin(),recipients.end());
        require(std::adjacent_find(recipients.begin(),recipients.end())==
                    recipients.end(),
                "BNW discovery recipient was repeated");
        for(const auto &call:counters.new_call_order) {
            require(call.ring>=last_ring && call.ring>=0 &&
                    call.ring<=counters.visibility_range+1 &&
                    seen.insert(call.plot).second,
                    "BNW sight new-center ring order is invalid");
            last_ring=call.ring;
            const auto found=deltas.find(call.plot);
            require(found!=deltas.end() && found->second->new_calls==1,
                    "BNW sight ring call has no counter delta");
            const auto &delta=*found->second;
            if(!(delta.after_old==0 && delta.after_new>0)) continue;
            const auto present=edges.find(call.plot);
            require(present!=edges.end(),
                    "BNW sight visible edge is absent from reveal plan");
            const auto &edge=*present->second;
            // SDK CvPlot.cpp:7694 enters setRevealed. Natural-wonder Lua,
            // rewards and notifications are inside that call; the previous
            // stage refuses those plots before an event trace is returned.
            result.events.push_back({BnwSightOrderedEffect::set_revealed,
                call.plot,-1,-1,0,edge.first_reveal});
            // CvPlot.cpp:7707 exploration tracking is after setRevealed and
            // only for first discovery. SDK loops players by ascending ID.
            if(edge.first_reveal)
                for(const auto player:recipients)
                    result.events.push_back({
                        BnwSightOrderedEffect::exploration_player_dirty,
                        call.plot,player,-1,0,true});
            // CvPlot.cpp:7723 refreshes six neighbors in DirectionTypes order.
            for(const auto adjacent:edge.adjacent_owner_refresh)
                result.events.push_back({
                    BnwSightOrderedEffect::adjacent_owner_refresh,
                    call.plot,-1,-1,adjacent,edge.first_reveal});
            // CvPlot.cpp:7734-7757 visits the plot roster before its city.
            // A stale unit handle, human suppression, and CvTeam::meet's
            // entire first-contact cascade remain unresolved blockers.
            for(const auto unit:edge.meet_unit_candidates) {
                result.events.push_back({
                    BnwSightOrderedEffect::unit_meet_candidate,
                    call.plot,unit.owner,unit.id,0,edge.first_reveal});
                ++result.unit_meet_candidates;
            }
            if(edge.meet_city_candidate) {
                result.events.push_back({
                    BnwSightOrderedEffect::city_meet_candidate,
                    call.plot,edge.meet_city_candidate->owner,
                    edge.meet_city_candidate->id,0,edge.first_reveal});
                ++result.city_meet_candidates;
            }
        }
        require(seen.size()==expected_calls,
                "BNW sight ordered calls are incomplete");
        result.calculated=true;
        result.reason="BNW sight order traced; meet, Lua and UI effects remain unexecuted";
    } catch(const std::exception &error) {
        result.calculated=false;
        result.exploration.sight.detached.reset();
        result.events.clear();
        result.reason=error.what();
    }
    return result;
}
} // namespace civ5::runtime
