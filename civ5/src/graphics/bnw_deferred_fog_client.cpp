#include "civ5/graphics/bnw_deferred_fog_client.hpp"
#include "civ5/graphics/bnw_scene_restore_receipt.hpp"
#include <limits>
#include <stdexcept>
#include <type_traits>
namespace civ5::graphics {
using runtime::BnwFogClientReceipt;
struct BnwFogClientReceiptAccess {
    static BnwFogClientReceipt make(runtime::BnwFogSnapshot snapshot,
        std::uint64_t sequence,std::uint64_t events,std::string digest,std::array<std::int32_t,3> offset) {
        return BnwFogClientReceipt(std::move(snapshot),sequence,events,std::move(digest),offset);
    }
};
namespace {
void require(bool value,const char *why) {if(!value) throw std::runtime_error(why);}
void native_matches(const runtime::BnwFogSnapshot &snapshot,const materials::FogOfWarState &client) {
    const auto &grid=snapshot.binding.grid;
    require(grid.width==static_cast<std::uint64_t>(client.width()) &&
            grid.height==static_cast<std::uint64_t>(client.height()) &&
            snapshot.modes.size()==grid.size() && client.visibility().size()==grid.size(),
            "BNW deferred fog native client grid differs");
    for(std::uint64_t i=0;i<grid.size();++i) {
        const auto xy=grid.coordinates(i);const auto native=client.visibility().at(xy[0]*grid.height+xy[1]);
        require(snapshot.modes.at(i)<=2 && native==2-snapshot.modes.at(i),
                "BNW deferred fog native visibility does not match active art revision");
    }
}
std::array<std::int32_t,3> cube(const runtime::BnwDeferredFogUpdate &update,
    std::array<std::int32_t,3> offset) {
    const auto x=static_cast<std::int64_t>(offset[0])+static_cast<std::int64_t>(update.x)-
                 static_cast<std::int64_t>(update.y/2);
    const auto y=static_cast<std::int64_t>(offset[1])+static_cast<std::int64_t>(update.y);
    const auto z=-x-y;
    require(x>=-32768 && x<=32767 && y>=-32768 && y<=32767 &&
            z>=std::numeric_limits<std::int32_t>::min() && z<=std::numeric_limits<std::int32_t>::max(),
            "BNW deferred fog event exceeds native cube domain");
    return {static_cast<std::int32_t>(x),static_cast<std::int32_t>(y),static_cast<std::int32_t>(z)};
}
}
BnwFogClientReceipt capture_bnw_fog_client_baseline(
    const runtime::LiveSessionState &live,const runtime::SessionCheckpointIdentity &identity,
    const BnwRendererSceneReceipt &scene_receipt,const scene::SavedFogOfWarCounts &initial_feed,
    const materials::FogOfWarState &client,std::string client_id,bool debug_mode) {
    require(!debug_mode,"BNW deferred fog debug-mode feed is not implemented");
    const auto snapshot=runtime::capture_bnw_fog_snapshot(live,identity,std::move(client_id));
    require(scene_receipt.restored_payload_sha256==identity.payload_sha256 &&
            scene_receipt.projection_reconciled &&
            matches_bnw_renderer_scene_receipt(scene_receipt,live.world,live.setup.game_turn,snapshot.binding.active_player),
            "BNW deferred fog baseline lacks a matching reconciled renderer scene receipt");
    require(initial_feed.visible==snapshot.visible && initial_feed.explored==snapshot.explored &&
            initial_feed.unexplored==snapshot.unexplored && initial_feed.visible_unrevealed==snapshot.visible_unrevealed,
            "BNW deferred fog baseline counts differ from actual initial feed");
    native_matches(snapshot,client);
    // tick(0) after the initial feed must have rebuilt the retained culling
    // grid; an unprocessed initial feed cannot establish a renderable baseline.
    require(!client.dirty(),"BNW deferred fog baseline has not completed its initial tick");
    return BnwFogClientReceiptAccess::make(snapshot,0,snapshot.binding.grid.size(),
                                         snapshot.binding.revision,client.grid_offset());
}
BnwFogClientReceipt consume_bnw_deferred_fog_batch(
    const runtime::BnwDeferredFogBatch &batch,const BnwFogClientReceipt &previous,
    materials::FogOfWarState &client) {
    runtime::validate_bnw_deferred_fog_batch(batch);
    require(runtime::same_bnw_fog_client(batch.snapshot.binding,previous.snapshot().binding) &&
            batch.snapshot.binding.game_turn>=previous.snapshot().binding.game_turn &&
            previous.sequence()<std::numeric_limits<std::uint64_t>::max() &&
            batch.sequence==previous.sequence()+1 && batch.before_revision==previous.snapshot().binding.revision &&
            client.grid_offset()==previous.grid_offset(),
            "BNW deferred fog event feed is stale, reordered or belongs to another client");
    native_matches(previous.snapshot(),client);
    std::vector<std::array<std::int32_t,3>> cubes;cubes.reserve(batch.updates.size());
    for(const auto &update:batch.updates) cubes.push_back(cube(update,client.grid_offset()));
    auto draft=client;
    for(std::size_t i=0;i<batch.updates.size();++i)
        draft.visibility_event(cubes[i],batch.updates[i].mode);
    native_matches(batch.snapshot,draft);
    const auto receipt=BnwFogClientReceiptAccess::make(batch.snapshot,batch.sequence,
        batch.updates.size(),batch.digest,draft.grid_offset());
    static_assert(std::is_nothrow_move_assignable_v<materials::FogOfWarState>);
    client=std::move(draft);return receipt;
}
BnwFogClientReceipt publish_bnw_deferred_fog(runtime::BnwDeferredFogOwner &owner,
                                           materials::FogOfWarState &client) {
    require(owner.queue_known && owner.pending && owner.published,"BNW deferred fog has no committed batch to publish");
    auto owner_draft=owner;auto client_draft=client;
    const auto receipt=consume_bnw_deferred_fog_batch(*owner_draft.pending,*owner_draft.published,client_draft);
    runtime::accept_bnw_deferred_fog_receipt(owner_draft,receipt);
    owner=std::move(owner_draft);client=std::move(client_draft);return receipt;
}
} // namespace civ5::graphics
