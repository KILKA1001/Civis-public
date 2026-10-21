#pragma once
static constexpr char lua_pregame_bootstrap[]=R"lua(
local active_player,slot_status,civilization,set_leader_type,player_color,set_player_color,native_preserve = ...
return function(env)
    if native_preserve and native_preserve(env) then return end
    env.PreGame={GetSlotStatus=slot_status,GetCivilization=civilization,SetLeaderType=set_leader_type,
        GetPlayerColor=player_color,SetPlayerColor=set_player_color}
    env.SlotStatus={SS_OPEN=0,SS_COMPUTER=1,SS_CLOSED=2,SS_TAKEN=3,SS_OBSERVER=4,SS_MAX_SLOT_STATUS=5}
    env.Game=env.Game or {}
    env.Game.GetActivePlayer=active_player
end
)lua";
