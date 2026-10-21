#include "civ5/runtime/bnw_player_plot_list_load.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <climits>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool condition,const char *reason) {
    if(!condition) throw std::runtime_error(reason);
}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
const saves::PlayerTailNode &plot_node(const saves::PlayerTailObservation &tail) {
    const saves::PlayerTailNode *found=nullptr;
    for(const auto &entry:tail.fields)
        if(name(entry.name)=="m_aiPlots") {
            require(!found,"BNW player plot-list node duplicated");
            found=&entry;
        }
    require(found && name(found->encoding)=="signed_i32" &&
            found->wire_count && found->bytes.empty() &&
            found->children.empty(),
            "BNW player plot-list wire node missing or changed");
    return *found;
}
} // namespace

BnwPlayerPlotListLoad rebuild_bnw_player_plot_lists(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity) {
    require(state.setup.profile==content::TechnologyProfile::bnw &&
            state.world.game_state.layout==
                saves::GameStateLayout::expansion2_version1 &&
            identify_bytes(source.source.bytes).sha256==identity.payload_sha256 &&
            state.world.all_players && source.snapshot.all_players &&
            state.world.all_players->players.size()==64 &&
            source.snapshot.all_players->players.size()==64 &&
            state.world.map_header.grid.width==source.snapshot.map_header.grid.width &&
            state.world.map_header.grid.height==source.snapshot.map_header.grid.height &&
            state.world.map_header.grid.size()<=INT32_MAX,
            "BNW player plot-list source, map or roster changed");
    const auto map_count=state.world.map_header.grid.size();
    require(map_count>0 && state.world.map_plots &&
            state.world.map_plots->plots.size()==map_count,
            "BNW player plot-list requires complete decoded map");
    BnwPlayerPlotListLoad result;
    result.map_plot_count=map_count;
    for(std::size_t i=0;i<64;++i) {
        const auto &raw=source.snapshot.all_players->players[i];
        const auto &world=state.world.all_players->players[i];
        require(raw.index==i && world.index==i &&
                raw.saved_player_id==static_cast<std::int64_t>(i) &&
                world.saved_player_id==static_cast<std::int64_t>(i) &&
                raw.tail.layout==saves::GameStateLayout::expansion2_version1 &&
                world.tail.layout==raw.tail.layout &&
                raw.tail.start_offset==world.tail.start_offset &&
                raw.tail.end_offset==world.tail.end_offset &&
                state.setup.players.contains(i),
                "BNW player plot-list owner or wire span changed");
        const auto &raw_node=plot_node(raw.tail);
        const auto &current=plot_node(world.tail);
        require(raw_node.start_offset==current.start_offset &&
                raw_node.end_offset==current.end_offset &&
                raw_node.wire_count==current.wire_count &&
                *current.wire_count>=0 &&
                static_cast<std::uint64_t>(*current.wire_count)<=map_count &&
                current.values.size()==
                    static_cast<std::uint64_t>(*current.wire_count),
                "BNW player plot-list source prefix length changed or exceeds map");
        BnwPlayerPlotList player;
        player.source_wire_count=current.values.size();
        player.plots.assign(static_cast<std::size_t>(map_count),-1);
        for(std::size_t j=0;j<current.values.size();++j) {
            const auto value=current.values[j];
            require(value>=-1 && value<static_cast<std::int64_t>(map_count),
                    "BNW player plot-list contains invalid map index");
            player.plots[j]=static_cast<std::int32_t>(value);
        }
        result.players.emplace(static_cast<std::int64_t>(i),std::move(player));
    }
    result.current=true;
    return result;
}

} // namespace civ5::runtime
