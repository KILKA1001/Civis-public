#include "civ5/runtime/pregame_roster.hpp"
#include <limits>

namespace civ5::runtime {
PregameRoster restore_pregame_roster(const saves::PregameObservation &saved,const content::RulesDatabase &rules){
    if(saved.slots_version!=3||saved.archive_version!=6||
       (saved.layout!=saves::GameStateLayout::expansion2_version1&&saved.layout!=saves::GameStateLayout::vp542_corpus))
        throw std::runtime_error("Unverified pregame roster contract");
    PregameRoster result;result.active_player=saved.archive.integers.at("active_player");
    const auto &statuses=saved.archive.integer_arrays.at("slot_status");
    const auto &minors=saved.archive.flag_arrays.at("minor_nation_civs");
    const auto &minor_keys=saved.archive.string_arrays.at("minor_civ_types");
    const auto &keys=saved.slots.string_arrays.at("civilization_keys");
    const auto leaders=saved.slots.string_arrays.find("leader_keys");
    const auto colors=saved.archive.string_arrays.find("player_colors");
    const auto &indices=saved.slots.integer_arrays.at("civilizations");
    if(statuses.size()>static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())||
       (keys.empty()?indices.size():keys.size())!=statuses.size()||minors.size()!=statuses.size()||
       minor_keys.size()!=statuses.size()||
       (leaders!=saved.slots.string_arrays.end()&&!leaders->second.empty()&&leaders->second.size()!=statuses.size()))
        throw std::runtime_error("Incomplete saved pregame roster");
    if(colors!=saved.archive.string_arrays.end()&&!colors->second.empty()&&colors->second.size()!=statuses.size())
        throw std::runtime_error("Incomplete saved player colors");
    for(std::size_t i=0;i<statuses.size();++i){
        std::int64_t civilization=-1;
        std::int64_t leader=-1;
        std::int64_t minor_type=-1;
        std::string leader_key;
        std::int64_t player_color=-1;
        std::string player_color_key;
        if(keys.empty())civilization=indices[i]; // native legacy fallback
        else if(!keys[i].empty()){
            const auto key=keys[i].substr(0,keys[i].find('\0'));
            const auto query=rules.query_rows({"SELECT ID FROM Civilizations WHERE Type = ? LIMIT 1",{key}},
                [&](const auto &row){civilization=row.integer_at(0);},1);
            if(!query.all_succeeded())throw std::runtime_error("Pregame civilization lookup failed: "+query.error);
        }
        if(leaders!=saved.slots.string_arrays.end()&&!leaders->second.empty()&&!leaders->second[i].empty()){
            const auto &wire=leaders->second[i];
            const auto key=wire.substr(0,wire.find('\0'));
            if(!key.empty()){
                const auto query=rules.query_rows({"SELECT ID FROM Leaders WHERE Type = ? LIMIT 1",{key}},
                    [&](const auto &row){leader=row.integer_at(0);},1);
                if(!query.all_succeeded())throw std::runtime_error("Pregame leader lookup failed: "+query.error);
                if(leader!=-1)leader_key=key;
            }
        }
        if(minors[i]&&!minor_keys[i].empty()){
            const auto &wire=minor_keys[i];
            const auto key=wire.substr(0,wire.find('\0'));
            if(!key.empty()){
                const auto query=rules.query_rows({"SELECT ID FROM MinorCivilizations WHERE Type = ? LIMIT 1",{key}},
                    [&](const auto &row){minor_type=row.integer_at(0);},1);
                if(!query.all_succeeded())throw std::runtime_error("Pregame minor civilization lookup failed: "+query.error);
            }
        }
        if(colors!=saved.archive.string_arrays.end()&&!colors->second.empty()&&!colors->second[i].empty()){
            const auto &wire=colors->second[i];
            const auto key=wire.substr(0,wire.find('\0'));
            if(!key.empty()){
                const auto query=rules.query_rows({"SELECT ID FROM PlayerColors WHERE Type = ? LIMIT 1",{key}},
                    [&](const auto &row){player_color=row.integer_at(0);},1);
                if(!query.all_succeeded())throw std::runtime_error("Pregame player color lookup failed: "+query.error);
                if(player_color!=-1)player_color_key=key;
            }
        }
        result.slots.emplace(static_cast<std::int64_t>(i),PregameSlot{statuses[i],civilization,leader,minors[i],minor_type,
            std::move(leader_key),player_color,std::move(player_color_key)});
    }
    return result;
}
ScriptPregameProvider lua_pregame_services(const PregameRoster &roster){
    return {
        [&roster]{return roster.active_player;},
        [&roster](std::int64_t id){const auto it=roster.slots.find(id);return id<0||it==roster.slots.end()?2:it->second.status;},
        [&roster](std::int64_t id){const auto it=roster.slots.find(id);return id<0||it==roster.slots.end()?-1:it->second.civilization;},
        {},
        [&roster](std::int64_t id){const auto it=roster.slots.find(id);return id<0||it==roster.slots.end()?-1:it->second.player_color;},
        {}
    };
}
ScriptPregameProvider lua_pregame_services(PregameRoster &roster,const content::RulesDatabase &rules){
    auto provider=lua_pregame_services(static_cast<const PregameRoster &>(roster));
    provider.set_leader_type=[&roster,&rules](std::int64_t player,std::int64_t leader){
        const auto it=roster.slots.find(player);
        if(it==roster.slots.end())return; // Native ignores players outside its slot domain.
        std::optional<std::string> key;
        if(leader!=-1){
            const auto report=rules.query_rows({"SELECT Type FROM Leaders WHERE ID=? LIMIT 1",{leader}},
                [&](const content::RulesRowView &row){key=row.text("Type");},1);
            if(!report.all_succeeded())throw ScriptError("Pregame leader lookup failed: "+report.error);
        }
        it->second.leader=key?leader:-1;
        it->second.leader_key=key.value_or("");
    };
    provider.set_player_color=[&roster,&rules](std::int64_t player,std::int64_t color){
        const auto it=roster.slots.find(player);
        if(it==roster.slots.end())return;
        std::optional<std::string> key;
        if(color!=-1){
            const auto report=rules.query_rows({"SELECT Type FROM PlayerColors WHERE ID=? LIMIT 1",{color}},
                [&](const content::RulesRowView &row){key=row.text("Type");},1);
            if(!report.all_succeeded())throw ScriptError("Pregame player color lookup failed: "+report.error);
        }
        it->second.player_color=key?color:-1;
        it->second.player_color_key=key.value_or("");
    };
    return provider;
}
} // namespace civ5::runtime
