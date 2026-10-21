#include "civ5/runtime/bnw_team_meet_lua_quote.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <set>

namespace civ5::runtime {
namespace {
bool hash_shape(std::string_view value) {
    return value.size()==64 &&
        std::all_of(value.begin(),value.end(),[](char c){
            return (c>='0'&&c<='9')||(c>='a'&&c<='f');
        });
}
bool plan_order(const BnwSightMeetEffectPlan &plan) {
    if(!plan.calculated || plan.contacts.size()!=1 ||
       plan.events.size()<8) return false;
    const auto &contact=plan.contacts.front();
    if(!contact.new_contact || contact.revealing_team<0 ||
       contact.revealing_team>=63 || contact.other_team<0 ||
       contact.other_team>=63 ||
       contact.revealing_team==contact.other_team) return false;
    const auto &events=plan.events;
    if(events.front().kind!=BnwMeetEffectKind::has_met ||
       events.front().team!=contact.revealing_team ||
       events.front().other_team!=contact.other_team ||
       events.back().kind!=BnwMeetEffectKind::lua_team_meet ||
       events.back().team!=contact.revealing_team ||
       events.back().other_team!=contact.other_team ||
       !events.back().external) return false;
    std::size_t second=events.size(),first_engine=events.size(),
                second_engine=events.size();
    for(std::size_t i=1;i+1<events.size();++i) {
        const auto &event=events[i];
        if(event.kind==BnwMeetEffectKind::lua_team_meet) return false;
        if(event.kind==BnwMeetEffectKind::has_met) {
            if(second!=events.size()) return false;
            second=i;
        }
        if(event.kind==BnwMeetEffectKind::gameplay_met_team) {
            if(first_engine==events.size()) first_engine=i;
            else if(second_engine==events.size()) second_engine=i;
            else return false;
        }
    }
    return first_engine<second && second<second_engine &&
        second_engine<events.size()-1 &&
        events[second].team==contact.other_team &&
        events[second].other_team==contact.revealing_team &&
        events[first_engine].team==contact.revealing_team &&
        events[second_engine].team==contact.other_team;
}
} // namespace

BnwTeamMeetLuaQuote quote_bnw_team_meet_lua_fixture(
    const BnwSightMeetEffectPlan &plan,
    std::span<const BnwDetachedGameplayLuaChunk> ordered_chunks) {
    BnwTeamMeetLuaQuote quote;
    if(!plan.calculated) {
        quote.reason=plan.reason.empty()
            ? "BNW TeamMeet needs an ordered effect plan":plan.reason;
        return quote;
    }
    if(plan.contacts.empty() && plan.events.empty()) {
        quote.gate=BnwTeamMeetLuaGate::no_new_contact;
        quote.reason="BNW TeamMeet is not called when the sight edge has no new contact";
        return quote;
    }
    if(!plan_order(plan)) {
        quote.reason="BNW TeamMeet source-order contact/engine event boundary changed";
        return quote;
    }
    if(ordered_chunks.empty() || ordered_chunks.size()>1024) {
        quote.gate=BnwTeamMeetLuaGate::source_plan_unavailable;
        quote.reason="BNW TeamMeet needs bounded ordered gameplay Lua sources";
        return quote;
    }
    std::set<std::string> names;
    for(const auto &chunk:ordered_chunks) {
        if(chunk.context!="gameplay" || chunk.name.empty() ||
           chunk.source.empty() || !hash_shape(chunk.sha256) ||
           !names.insert(chunk.name).second) {
            quote.gate=BnwTeamMeetLuaGate::source_plan_unavailable;
            quote.reason="BNW TeamMeet gameplay source context, name or digest invalid";
            return quote;
        }
        const auto bytes=std::span(
            reinterpret_cast<const std::uint8_t *>(chunk.source.data()),
            chunk.source.size());
        if(identify_bytes(bytes).sha256!=chunk.sha256) {
            quote.gate=BnwTeamMeetLuaGate::source_plan_unavailable;
            quote.reason="BNW TeamMeet gameplay source SHA-256 changed";
            return quote;
        }
    }
    const auto &contact=plan.contacts.front();
    // SDK CvTeam.cpp:1654 pushes eTeam before GetID.
    quote.first_argument=contact.other_team;
    quote.second_argument=contact.revealing_team;
    try {
        LuaScriptHost host({64ULL*1024*1024,2'000'000});
        for(const auto &chunk:ordered_chunks)
            host.execute(chunk.context,chunk.source,chunk.name);
        host.complete_loading();
        const std::array<ScriptValue,2> args{
            quote.first_argument,quote.second_argument};
        const auto result=host.call_hook("TeamMeet",args);
        if(!result.errors.empty())
            throw ScriptError("BNW TeamMeet listener failed: "+
                              result.errors.front());
        quote.handled=result.value.has_value();
        quote.gate=BnwTeamMeetLuaGate::fixture_quoted_stock_context_unverified;
        quote.reason="BNW TeamMeet quoted in disposable gameplay Lua; prior AI effects and selected source order remain unexecuted";
    } catch(const std::exception &error) {
        quote.gate=BnwTeamMeetLuaGate::callback_failed;
        quote.reason=error.what();
    }
    return quote;
}
} // namespace civ5::runtime
