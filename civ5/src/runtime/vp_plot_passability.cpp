#include "civ5/runtime/vp_plot_passability_profile.hpp"
#include <limits>

namespace civ5::runtime {
namespace {
template<class T> const T &named(const std::vector<T> &values,std::string_view name){
    const T *found=nullptr;
    for(const auto &value:values)if(value.name&&value.name==name){
        if(found)throw ScriptError("Duplicate plot field: "+std::string(name));
        found=&value;
    }
    if(!found)throw ScriptError("Missing plot field: "+std::string(name));
    return *found;
}
}

void restore_vp_plot_passability_flags(PlotPassability &state,std::uint64_t plot,
    const saves::PlotObservation &saved,std::uint64_t team_slots){
    if(state.layout!=saves::GameStateLayout::vp542_corpus||plot>=state.plots.size())
        throw ScriptError("VP passability restore requires a valid VP plot");
    const auto flags=named(saved.fields,"packed_flags").value;
    if(flags<0||flags>UINT16_MAX)throw ScriptError("Invalid source plot flags");
    state.plots[plot].impassable=(flags&(1<<14))!=0;
    const auto &teams=named(saved.arrays,"teams");
    if(teams.columns!=10||teams.values.size()%10||teams.values.size()/10!=team_slots)
        throw ScriptError("Malformed plot team flags");
    auto &team_flags=ensure_vp_plot_team_flags(state,plot);
    for(std::size_t j=7;j<teams.values.size();j+=10){
        if(teams.values[j]!=0&&teams.values[j]!=1)throw ScriptError("Invalid team impassability");
        const auto team=j/10;
        if(team>static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
            throw ScriptError("Plot team ID exceeds int64");
        team_flags.emplace(static_cast<std::int64_t>(team),teams.values[j]!=0);
    }
}
} // namespace civ5::runtime
