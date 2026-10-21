#include "civ5/runtime/bnw_deferred_fog.hpp"
#include "civ5/runtime/bnw_live_plot_visibility.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <type_traits>
namespace civ5::runtime {
namespace {
void require(bool value,const char *why) {if(!value) throw std::runtime_error(why);}
void u64(std::vector<std::uint8_t> &bytes,std::uint64_t value) {
    for(unsigned i=0;i<8;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(i*8)));
}
void text(std::vector<std::uint8_t> &bytes,std::string_view value) {
    u64(bytes,value.size());bytes.insert(bytes.end(),value.begin(),value.end());
}
void append_binding(std::vector<std::uint8_t> &bytes,const BnwFogBinding &b) {
    text(bytes,b.identity.payload_sha256);text(bytes,b.identity.rules_sha256);
    bytes.insert(bytes.end(),b.map_guid.begin(),b.map_guid.end());
    u64(bytes,b.grid.width);u64(bytes,b.grid.height);
    u64(bytes,b.active_player);u64(bytes,b.active_team);u64(bytes,b.game_turn);
    text(bytes,b.client_id);
}
}
bool same_bnw_fog_client(const BnwFogBinding &a,const BnwFogBinding &b) {
    return a.identity.payload_sha256==b.identity.payload_sha256 &&
           a.identity.rules_sha256==b.identity.rules_sha256 && a.map_guid==b.map_guid &&
           a.grid.width==b.grid.width && a.grid.height==b.grid.height &&
           a.active_player==b.active_player && a.active_team==b.active_team && a.client_id==b.client_id;
}
BnwFogSnapshot capture_bnw_fog_snapshot(const LiveSessionState &state,
    const SessionCheckpointIdentity &identity,std::string client_id) {
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    const auto &world=state.world;
    require(identity.payload_sha256.size()==64 && identity.rules_sha256.size()==64 &&
            !client_id.empty() && world.game_state.layout==bnw && world.pregame &&
            world.pregame->layout==bnw && world.map_plots && world.map_projection,
            "BNW fog needs one identified live map/client");
    const auto &cache=bnw_live_plot_visibility_cache(state);
    const auto art=saves::observe_saved_map_team_art(*world.pregame,*world.map_plots);
    const auto active=world.pregame->archive.integers.at("active_player");
    const auto turn=world.pregame->archive.integers.at("game_turn");
    const auto player=state.setup.players.find(active);
    require(active>=0 && active<64 && art.active_player==static_cast<std::uint64_t>(active) &&
            player!=state.setup.players.end() && player->second.team>=0 && player->second.team<64 &&
            art.active_team==static_cast<std::uint64_t>(player->second.team) &&
            turn>=0 && turn==state.setup.game_turn && art.plots.size()==cache.grid.size() &&
            world.map_projection->cells.size()==cache.grid.size(),
            "BNW fog active map/art/player/team or clock differs");
    BnwFogSnapshot result;
    result.binding={identity,world.map_header.guid,cache.grid,active,player->second.team,turn,std::move(client_id),{}};
    result.modes.reserve(art.plots.size());
    std::vector<std::uint8_t> revision;
    append_binding(revision,result.binding);
    for(std::uint64_t i=0;i<cache.grid.size();++i) {
        const auto &saved=art.plots.at(i);
        const auto &cell=world.map_projection->cells.at(i);
        require(saved.plot_index==i && saved.revealed.has_value() && cell.index==i &&
                cell.revealed_teams && cell.revealed_teams->contains(player->second.team)==*saved.revealed,
                "BNW fog live revealed map/art inputs are unresolved or stale");
        const auto visible=cache.counts.at(i*cache.wire_slots+art.active_team);
        require(visible>=0,"BNW fog live visibility is negative");
        const auto mode=!*saved.revealed?std::uint8_t{0}:visible>0?std::uint8_t{2}:std::uint8_t{1};
        result.modes.push_back(mode);
        result.visible+=mode==2;result.explored+=mode==1;result.unexplored+=mode==0;
        result.visible_unrevealed+=visible>0 && !*saved.revealed;
        u64(revision,i);u64(revision,visible);revision.push_back(*saved.revealed);revision.push_back(mode);
    }
    result.binding.revision=identify_bytes(revision).sha256;
    return result;
}
std::string bnw_deferred_fog_batch_digest(const BnwDeferredFogBatch &batch) {
    std::vector<std::uint8_t> bytes;append_binding(bytes,batch.snapshot.binding);
    text(bytes,batch.snapshot.binding.revision);text(bytes,batch.before_revision);u64(bytes,batch.sequence);
    u64(bytes,batch.snapshot.visible);u64(bytes,batch.snapshot.explored);
    u64(bytes,batch.snapshot.unexplored);u64(bytes,batch.snapshot.visible_unrevealed);
    u64(bytes,batch.snapshot.modes.size());bytes.insert(bytes.end(),batch.snapshot.modes.begin(),batch.snapshot.modes.end());
    u64(bytes,batch.updates.size());
    for(const auto &update:batch.updates) {
        u64(bytes,update.plot);u64(bytes,update.x);u64(bytes,update.y);bytes.push_back(update.mode);
    }
    return identify_bytes(bytes).sha256;
}
void validate_bnw_deferred_fog_batch(const BnwDeferredFogBatch &batch) {
    const auto &b=batch.snapshot.binding;
    require(b.identity.payload_sha256.size()==64 && b.identity.rules_sha256.size()==64 &&
            b.revision.size()==64 && !b.client_id.empty() && b.active_player>=0 && b.active_player<64 &&
            b.active_team>=0 && b.active_team<64 && b.game_turn>=0 && b.grid.width>0 && b.grid.height>0 &&
            b.grid.width<=32768 && b.grid.height<=32768 && b.grid.size()<=1048576 &&
            batch.snapshot.modes.size()==b.grid.size() && batch.sequence>0 && batch.before_revision.size()==64,
            "BNW deferred fog batch binding is incomplete");
    std::set<std::uint64_t> seen;
    std::uint64_t visible=0,explored=0,unexplored=0;
    for(const auto mode:batch.snapshot.modes) {
        require(mode<=2,"BNW fog mode is outside SDK range");
        visible+=mode==2;explored+=mode==1;unexplored+=mode==0;
    }
    require(visible==batch.snapshot.visible && explored==batch.snapshot.explored &&
            unexplored==batch.snapshot.unexplored && batch.snapshot.visible_unrevealed<=unexplored,
            "BNW deferred fog batch mode counts differ");
    for(const auto &update:batch.updates) {
        require(update.plot<b.grid.size() && seen.insert(update.plot).second &&
                b.grid.index(update.x,update.y)==update.plot &&
                update.mode==batch.snapshot.modes.at(update.plot),
                "BNW deferred fog batch contains stale or duplicate plot events");
    }
    require(batch.digest==bnw_deferred_fog_batch_digest(batch),"BNW deferred fog batch digest differs");
}
BnwDeferredFogOwner make_bnw_deferred_fog_owner(const BnwFogClientReceipt &baseline) {
    require(baseline.sequence()==0 && baseline.events_applied()==baseline.snapshot().binding.grid.size() &&
            baseline.snapshot().modes.size()==baseline.snapshot().binding.grid.size(),
            "BNW deferred fog needs a complete real client baseline feed receipt");
    BnwDeferredFogOwner result;result.queue_known=true;result.published=baseline;return result;
}
void queue_bnw_deferred_fog(BnwDeferredFogOwner &owner,std::uint64_t plot) {
    require(owner.queue_known && owner.published && plot<owner.published->snapshot().binding.grid.size(),
            "BNW deferred fog queue is unknown or plot is invalid");
    // CvPlot::updateFog(true) suppresses duplicates and preserves append order.
    if(std::find(owner.queued_plots.begin(),owner.queued_plots.end(),plot)==owner.queued_plots.end())
        owner.queued_plots.push_back(plot);
}
void stage_bnw_deferred_fog_flush(const LiveSessionState &state,BnwDeferredFogOwner &owner) {
    require(owner.queue_known && owner.published,"BNW deferred fog queue has no tracked client baseline");
    require(!owner.pending,"BNW deferred fog has an unconsumed publication batch");
    const auto &previous=owner.published->snapshot();
    auto current=capture_bnw_fog_snapshot(state,previous.binding.identity,previous.binding.client_id);
    require(same_bnw_fog_client(current.binding,previous.binding) && current.binding.game_turn>=previous.binding.game_turn,
            "BNW deferred fog map/art/active-client binding changed");
    require(owner.published->sequence()<std::numeric_limits<std::uint64_t>::max(),"BNW fog publication sequence overflow");
    std::set<std::uint64_t> queued;
    for(const auto plot:owner.queued_plots)
        require(plot<current.modes.size() && queued.insert(plot).second,"BNW deferred fog queue is invalid or duplicated");
    require(previous.modes.size()==current.modes.size(),"BNW deferred fog baseline shape changed");
    for(std::uint64_t i=0;i<current.modes.size();++i)
        require(current.modes[i]==previous.modes[i] || queued.contains(i),
                "BNW deferred fog visibility changed without a tracked source updateFog(true)");
    BnwDeferredFogBatch batch{std::move(current),owner.published->sequence()+1,previous.binding.revision,{},{}};
    for(const auto plot:owner.queued_plots) {
        const auto xy=batch.snapshot.binding.grid.coordinates(plot);
        batch.updates.push_back({plot,xy[0],xy[1],batch.snapshot.modes.at(plot)});
    }
    batch.digest=bnw_deferred_fog_batch_digest(batch);validate_bnw_deferred_fog_batch(batch);
    auto replacement=owner;replacement.queued_plots.clear();replacement.pending=std::move(batch);
    static_assert(std::is_nothrow_move_assignable_v<BnwDeferredFogOwner>);
    owner=std::move(replacement);
}
void accept_bnw_deferred_fog_receipt(BnwDeferredFogOwner &owner,const BnwFogClientReceipt &receipt) {
    require(owner.queue_known && owner.pending && owner.published,"BNW deferred fog has no pending receipt obligation");
    const auto &batch=*owner.pending;validate_bnw_deferred_fog_batch(batch);
    require(same_bnw_fog_client(batch.snapshot.binding,receipt.snapshot().binding) &&
            receipt.snapshot().binding.revision==batch.snapshot.binding.revision &&
            receipt.snapshot().binding.game_turn==batch.snapshot.binding.game_turn &&
            receipt.snapshot().modes==batch.snapshot.modes && receipt.sequence()==batch.sequence &&
            receipt.events_applied()==batch.updates.size() && receipt.batch_digest()==batch.digest,
            "BNW deferred fog client receipt does not match committed batch");
    auto replacement=owner;replacement.published=receipt;replacement.pending.reset();owner=std::move(replacement);
}
} // namespace civ5::runtime
