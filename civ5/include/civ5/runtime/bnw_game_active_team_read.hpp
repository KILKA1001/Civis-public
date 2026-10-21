#pragma once
#include <cstdint>
namespace civ5::content {class RulesDatabase;}
namespace civ5::runtime {
struct LiveSessionState;
namespace detail {
// Pure source read; caller must already own the genuine current Game lease.
// Detached DTO evaluation creates no gameplay or UI selection authority.
bool bnw_player_human_read(const LiveSessionState&,std::int32_t,const content::RulesDatabase&);
std::int32_t bnw_game_active_player_read(const LiveSessionState&);
std::int32_t bnw_game_active_team_read(const LiveSessionState&);
std::int32_t bnw_player_team_read(const LiveSessionState&,std::int32_t player);
}
}
