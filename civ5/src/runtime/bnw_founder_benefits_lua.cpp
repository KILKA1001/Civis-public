#include "civ5/runtime/bnw_founder_benefits_lua.hpp"
#include <array>
#include <limits>
namespace civ5::runtime {
BnwFounderBenefitsLuaQuote quote_bnw_founder_benefits_lua(
    const LiveSessionState &state,const SavedContentProfile &selected,
    const SessionCheckpointIdentity &identity,BnwGameplayLuaEpochOwner &owner,
    std::int64_t player) {
    BnwFounderBenefitsLuaQuote result;
    try {
        if(state.setup.profile!=content::TechnologyProfile::bnw ||
           !state.setup.players.contains(player) || player<0 || player>INT32_MAX ||
           !selected.can_activate || !selected.all_requirements_located ||
           !selected.enabled_mods.empty() || identity.payload_sha256.size()!=64 ||
           identity.rules_sha256.size()!=64) {
            result.blocker="BNW founder benefits requires a selected stock player/profile identity";
            return result;
        }
        auto fork=owner.fork_at(state);
        if(!fork.epoch) {
            result.blocker="BNW founder benefits has no active selected Lua epoch: "+fork.blocker;
            return result;
        }
        auto &epoch=*fork.epoch;
        if(!epoch.world || !epoch.host || epoch.source_world!=&state ||
           epoch.getter_world!=epoch.world.get() ||
           epoch.identity.payload_sha256!=identity.payload_sha256 ||
           epoch.identity.rules_sha256!=identity.rules_sha256 ||
           epoch.selected_dlc!=selected.enabled_dlc ||
           epoch.selected_mods!=selected.enabled_mods ||
           !epoch.all_selected_contexts_loaded || !epoch.all_listeners_registered ||
           !epoch.all_external_writes_escrowed || !epoch.host->loading_complete() ||
           !epoch.host->source_errors().empty()) {
            result.blocker="BNW founder benefits detached selected Lua epoch is unverified";
            return result;
        }
        const std::array<ScriptValue,1> arguments{player};
        const auto accumulator=epoch.host->accumulate_int(
            "GetFounderBenefitsReligion",arguments,0);
        if(!accumulator.errors.empty()) {
            result.blocker="BNW founder benefits callback failed; detached VM discarded: "+
                accumulator.errors.front();
            return result;
        }
        if(accumulator.value) {
            if(*accumulator.value<std::numeric_limits<std::int32_t>::min() ||
               *accumulator.value>std::numeric_limits<std::int32_t>::max()) {
                result.blocker="BNW founder benefits accumulator exceeds source native int";
                return result;
            }
            result.religion=static_cast<std::int32_t>(*accumulator.value);
            result.status=BnwFounderBenefitsLuaStatus::override_quoted;
        } else result.status=BnwFounderBenefitsLuaStatus::native_fallback_required;
        result.epoch=std::move(fork.epoch);
        result.blocker="BNW founder benefits quoted; native religion effects and atomic VM/world publication remain";
    } catch(const std::exception &error) {
        result.blocker=std::string("BNW founder benefits fork failed; no publication: ")+error.what();
    }
    return result;
}
}
