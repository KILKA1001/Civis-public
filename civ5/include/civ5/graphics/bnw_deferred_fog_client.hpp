#pragma once
#include "civ5/runtime/bnw_deferred_fog.hpp"
#include "civ5/materials/fog_of_war.hpp"
#include "civ5/scene/saved_fog_of_war.hpp"
namespace civ5::graphics {
struct BnwRendererSceneReceipt;
// Call immediately after the production camera's real apply_saved_map_fog +
// tick(0), with its resulting counts and the reconciled scene receipt. This
// validates the existing fow->state, not a substitute graphics owner.
runtime::BnwFogClientReceipt capture_bnw_fog_client_baseline(
    const runtime::LiveSessionState &live,
    const runtime::SessionCheckpointIdentity &identity,
    const BnwRendererSceneReceipt &scene_receipt,
    const scene::SavedFogOfWarCounts &initial_feed,
    const materials::FogOfWarState &client,std::string client_id,bool debug_mode);
// Applies each actual GameplayFOWChanged event to the same native FOW state.
// Validate-before-write and a private copy make rejection leave it unchanged.
runtime::BnwFogClientReceipt consume_bnw_deferred_fog_batch(
    const runtime::BnwDeferredFogBatch &batch,
    const runtime::BnwFogClientReceipt &previous,materials::FogOfWarState &client);
// Atomically consumes the pending journal and updates both receipt owner and
// native FOW state. On failure the journal and actual client remain retryable.
runtime::BnwFogClientReceipt publish_bnw_deferred_fog(
    runtime::BnwDeferredFogOwner &owner,materials::FogOfWarState &client);
} // namespace civ5::graphics
