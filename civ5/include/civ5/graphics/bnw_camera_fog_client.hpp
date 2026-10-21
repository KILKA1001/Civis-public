#pragma once
#include "civ5/graphics/bnw_rendered_fog_frame.hpp"
#include <type_traits>

namespace civ5::graphics {
// One client borrows the enclosing execution owner. Its original saved feed
// establishes a new native boundary; committed game drafts retain this same
// journal value and may deliver only explicitly recorded source fog events.
class BnwCameraFogClient {
  public:
    BnwCameraFogClient(const runtime::LiveSession &session,
        runtime::BnwGameActivationOwner &activation,
        runtime::SessionCheckpointIdentity identity,BnwRendererSceneReceipt scene,
        const scene::SavedFogOfWarCounts &initial_feed,materials::FogOfWarState &native,
        GlFogOfWarMap &map,std::string client_id)
        :session_(session),state_(&session.state()),activation_(activation),
         identity_(std::move(identity)),scene_(std::move(scene)),native_(native),map_(map) {
        require(activation.identity.payload_sha256==identity_.payload_sha256 &&
            activation.identity.rules_sha256==identity_.rules_sha256,
            "Camera FOW execution owner belongs to another source or rules");
        require(!activation.deferred_fog,"Camera FOW cannot replace a retained execution journal");
        const auto baseline=capture_bnw_fog_client_baseline(session.state(),identity_,scene_,
            initial_feed,native_,std::move(client_id),false);
        auto journal=runtime::make_bnw_deferred_fog_owner(baseline);
        client_id_=baseline.snapshot().binding.client_id;
        activation.deferred_fog=std::move(journal);
        journal_=&*activation.deferred_fog;
    }
    BnwCameraFogClient(const BnwCameraFogClient &)=delete;
    BnwCameraFogClient &operator=(const BnwCameraFogClient &)=delete;
    bool valid_handoff() const noexcept {
        if(handoff_lost_) return false;
        const auto fail=[&] {handoff_lost_=true;return false;};
        if(&session_.state()!=state_ ||
            activation_.identity.payload_sha256!=identity_.payload_sha256 ||
            activation_.identity.rules_sha256!=identity_.rules_sha256 ||
            !activation_.deferred_fog || &*activation_.deferred_fog!=journal_) return fail();
        const auto &journal=*activation_.deferred_fog;
        if(!journal.queue_known || !journal.published || journal.published->sequence()<sequence_) return fail();
        const auto &binding=journal.published->snapshot().binding;
        if(binding.identity.payload_sha256!=identity_.payload_sha256 ||
            binding.identity.rules_sha256!=identity_.rules_sha256 || binding.client_id!=client_id_ ||
            binding.map_guid!=scene_.map_guid || binding.active_player<0 || binding.active_team<0 ||
            static_cast<std::uint64_t>(binding.active_player)!=scene_.active_player ||
            static_cast<std::uint64_t>(binding.active_team)!=scene_.active_team) return fail();
        sequence_=journal.published->sequence();
        return true;
    }
    BnwNativeFogFrameBinding binding() const {
        require(valid_handoff(),"Camera FOW execution journal was reset, replaced or rebound");
        return {journal_,&native_,&map_,identity_,&activation_,&handoff_lost_};
    }
    runtime::BnwDeferredFogOwner &journal() {
        require(valid_handoff(),"Camera FOW execution journal was reset, replaced or rebound");
        return *activation_.deferred_fog;
    }
    const runtime::BnwGameActivationOwner &activation() const noexcept { return activation_; }
    // Validate and apply on private copies. Neither pending nor newly queued
    // source work is lost when the second stage or actual native client rejects.
    // Static world rebuild is deliberately a separate renderer obligation.
    bool consume_committed() {
        require(valid_handoff(),"Camera FOW execution journal was reset, replaced or rebound");
        const auto &live=session_.state();
        require(matches_bnw_renderer_scene_receipt(scene_,live.world,live.setup.game_turn,scene_.active_player),
            "Camera FOW committed world requires rebuilding the immutable renderer scene");
        auto &actual=*activation_.deferred_fog;
        if(!actual.pending && actual.queued_plots.empty()) {
            require(matches_native(actual,native_,live),"Camera FOW changed without committed source journal events");
            return false;
        }
        auto draft=actual;auto native_draft=native_;
        if(draft.pending) publish_bnw_deferred_fog(draft,native_draft);
        if(!draft.queued_plots.empty()) {
            runtime::stage_bnw_deferred_fog_flush(live,draft);
            publish_bnw_deferred_fog(draft,native_draft);
        }
        native_draft.tick(0.f);
        require(matches_native(draft,native_draft,live),"Camera FOW journal does not describe current committed world");
        static_assert(std::is_nothrow_move_assignable_v<runtime::BnwDeferredFogOwner>);
        static_assert(std::is_nothrow_move_assignable_v<materials::FogOfWarState>);
        actual=std::move(draft);native_=std::move(native_draft);
        sequence_=actual.published->sequence();
        return true;
    }
  private:
    static void require(bool value,const char *reason) {if(!value)throw std::runtime_error(reason);}
    bool matches_native(const runtime::BnwDeferredFogOwner &journal,
        const materials::FogOfWarState &native,const runtime::LiveSessionState &live) const {
        try {
            if(!journal.queue_known || !journal.published || journal.pending ||
                !journal.queued_plots.empty() || native.dirty()) return false;
            const auto &applied=journal.published->snapshot();
            const auto fresh=runtime::capture_bnw_fog_snapshot(live,identity_,client_id_);
            if(!runtime::same_bnw_fog_client(applied.binding,fresh.binding) ||
                applied.binding.revision!=fresh.binding.revision || applied.modes!=fresh.modes ||
                applied.binding.game_turn!=fresh.binding.game_turn ||
                journal.published->grid_offset()!=native.grid_offset() ||
                fresh.binding.grid.width!=static_cast<std::uint64_t>(native.width()) ||
                fresh.binding.grid.height!=static_cast<std::uint64_t>(native.height())) return false;
            for(std::uint64_t plot=0;plot<fresh.binding.grid.size();++plot) {
                const auto xy=fresh.binding.grid.coordinates(plot);
                if(native.visibility().at(xy[0]*fresh.binding.grid.height+xy[1])!=2-fresh.modes.at(plot))return false;
            }
            return true;
        } catch(const std::exception &) {return false;}
    }
    const runtime::LiveSession &session_;
    const runtime::LiveSessionState *state_;
    runtime::BnwGameActivationOwner &activation_;
    runtime::SessionCheckpointIdentity identity_;
    BnwRendererSceneReceipt scene_;
    materials::FogOfWarState &native_;
    GlFogOfWarMap &map_;
    runtime::BnwDeferredFogOwner *journal_=nullptr;
    std::string client_id_;
    mutable std::uint64_t sequence_=0;
    mutable bool handoff_lost_=false;
};
} // namespace civ5::graphics
