#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/map_state.hpp"
#include <optional>
#include <string>
#include <vector>

namespace civ5::graphics { struct BnwFogClientReceiptAccess; }
namespace civ5::runtime {
struct LiveSessionState;
struct BnwFogBinding {
    SessionCheckpointIdentity identity;
    std::array<std::uint8_t,16> map_guid{};
    saves::MapGrid grid;
    std::int64_t active_player=-1,active_team=-1,game_turn=-1;
    std::string client_id,revision;
};
struct BnwFogSnapshot {
    BnwFogBinding binding;
    // Logical y*width+x order. SDK FOGOFWARMODE: 0 unexplored, 1 novis, 2 off.
    std::vector<std::uint8_t> modes;
    std::uint64_t visible=0,explored=0,unexplored=0,visible_unrevealed=0;
};
struct BnwDeferredFogUpdate {
    std::uint64_t plot=0,x=0,y=0;
    std::uint8_t mode=0;
};
struct BnwDeferredFogBatch {
    BnwFogSnapshot snapshot;
    std::uint64_t sequence=0;
    std::string before_revision,digest;
    std::vector<BnwDeferredFogUpdate> updates;
};
// Constructed only by the real FogOfWarState baseline/consumer adapters.
// This acknowledges native CPU fog state, not GL pixels or full scene parity.
class BnwFogClientReceipt {
  public:
    const BnwFogSnapshot &snapshot() const noexcept {return snapshot_;}
    std::uint64_t sequence() const noexcept {return sequence_;}
    std::uint64_t events_applied() const noexcept {return events_applied_;}
    const std::string &batch_digest() const noexcept {return batch_digest_;}
    std::array<std::int32_t,3> grid_offset() const noexcept {return grid_offset_;}
  private:
    friend struct civ5::graphics::BnwFogClientReceiptAccess;
    BnwFogClientReceipt(BnwFogSnapshot snapshot,std::uint64_t sequence,
                        std::uint64_t events,std::string digest,std::array<std::int32_t,3> offset)
        :snapshot_(std::move(snapshot)),sequence_(sequence),events_applied_(events),
         batch_digest_(std::move(digest)),grid_offset_(offset) {}
    BnwFogSnapshot snapshot_;
    std::uint64_t sequence_=0,events_applied_=0;
    std::string batch_digest_;
    std::array<std::int32_t,3> grid_offset_{};
};
struct BnwDeferredFogOwner {
    // Unknown imported queues must never be treated as empty. The baseline
    // adapter establishes a new tracked client boundary; all subsequent
    // source updateFog(true) calls must append through queue_bnw_deferred_fog.
    bool queue_known=false;
    std::optional<BnwFogClientReceipt> published;
    std::vector<std::uint64_t> queued_plots;
    std::optional<BnwDeferredFogBatch> pending;
};

BnwFogSnapshot capture_bnw_fog_snapshot(
    const LiveSessionState &state,const SessionCheckpointIdentity &identity,
    std::string client_id);
std::string bnw_deferred_fog_batch_digest(const BnwDeferredFogBatch &batch);
void validate_bnw_deferred_fog_batch(const BnwDeferredFogBatch &batch);
BnwDeferredFogOwner make_bnw_deferred_fog_owner(const BnwFogClientReceipt &baseline);
void queue_bnw_deferred_fog(BnwDeferredFogOwner &owner,std::uint64_t plot);
// CvMap::updateDeferredFog: current modes are read in queued source order,
// then the native queue clears into a pending publication journal. Failure
// leaves owner unchanged. Called on the enclosing activation's private owner.
void stage_bnw_deferred_fog_flush(const LiveSessionState &state,
                                 BnwDeferredFogOwner &owner);
// A receipt must prove every queued GameplayFOWChanged event reached the
// bound native fog owner. Failure leaves the pending journal retryable.
void accept_bnw_deferred_fog_receipt(BnwDeferredFogOwner &owner,
                                   const BnwFogClientReceipt &receipt);
bool same_bnw_fog_client(const BnwFogBinding &a,const BnwFogBinding &b);
} // namespace civ5::runtime
