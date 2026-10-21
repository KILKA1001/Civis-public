#pragma once
#include "gl_bnw_warrior_retained.hpp"
#include "civ5/graphics/bnw_unit_member_transform_rules.hpp"
#include "bnw_warrior_owned_terrain.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include <limits>
#include <map>
#include <utility>

namespace civ5::validation { struct PreviewPresenter; }
namespace civ5::graphics {
class BnwInitialCenterUnitRendererLoan;
class BnwInitialCenterUnitVisibilityProof;
class GlBnwWarriorOwnedScene;
// Genuine camera initialization storage only. Complete Core roster admission
// and visibility proof are a separate sealed Loan operation.
class BnwInitialCenterUnitRendererStorage final {
    friend struct ::civ5::validation::PreviewPresenter;
    friend class BnwInitialCenterUnitRendererLoan;
    friend class BnwInitialCenterUnitVisibilityProof;
    friend class GlBnwWarriorOwnedScene;
    const std::thread::id creator_=std::this_thread::get_id();
    std::weak_ptr<BnwInitialCenterUnitRendererStorage> self_;
    std::size_t borrowed_pins_=0;
    const std::weak_ptr<ui::BnwClientSceneFrame> scene_;
    const std::weak_ptr<const ui::SelectedBnwPreviewHost> host_;
    const runtime::LiveSessionState* const state_;
    const std::uint64_t generation_,initial_present_count_;
    GlBnwWarriorOwnedScene* const imported_;
    const std::weak_ptr<const void> imported_lifetime_,paint_lifetime_;
    const BnwImportedUnitVisualRecord* const imported_occurrence_;
    // Minted exclusively from the actual retained drawn renderer at the
    // successful presentation factory; hidden Core nodes are NOT actor slots.
    const std::map<std::pair<std::int64_t,std::int64_t>,const GlBnwWarriorOwnedScene*> actual_receivers_;
    void require_missing_center_receiver(std::int32_t owner,std::int32_t full_id) const;
    BnwInitialCenterUnitRendererStorage(const std::shared_ptr<ui::BnwClientSceneFrame>&,
        GlBnwWarriorOwnedScene&,const std::weak_ptr<const void>&,
        const std::weak_ptr<const void>&,std::uint64_t);
    static std::shared_ptr<BnwInitialCenterUnitRendererStorage> after_actual_present(
        const std::shared_ptr<ui::BnwClientSceneFrame>&,GlBnwWarriorOwnedScene&,
        const std::weak_ptr<const void>&,const std::weak_ptr<const void>&,std::uint64_t);
    void identity() const;
public:
    BnwInitialCenterUnitRendererStorage(const BnwInitialCenterUnitRendererStorage&)=delete;
    BnwInitialCenterUnitRendererStorage& operator=(const BnwInitialCenterUnitRendererStorage&)=delete;
};
// Retained Civis owner for the restricted selected Warrior path. The caller
// supplies its actual same-revision generated MaterialWorld and a reconciled
// renderer scene receipt. This owner never manufactures a native placement
// pin; it draws generated placement and retains all original parity blockers.
class GlBnwWarriorOwnedScene {
  public:
    GlBnwWarriorOwnedScene(std::shared_ptr<GlContext> context,
        const std::filesystem::path &asset_root,const std::filesystem::path &sdk_cache,
        BnwRendererSceneReceipt receipt,const runtime::LiveSessionState &live,
        const BnwWarriorOwnedTerrain &generated,
        BnwImportedUnitVisualOwner units,BnwUnitMemberTransformRules rules,
        std::int64_t player,std::int64_t id):
        receipt_(std::move(receipt)),terrain_(generated.world()),generation_(generated.receipt()),units_(std::move(units)),
        rules_(rules),player_(player),id_(id),backend_(std::move(context),asset_root,sdk_cache) {
        if(!terrain_ || !receipt_.projection_reconciled ||
           !matches_bnw_renderer_scene_receipt(receipt_,live.world,live.setup.game_turn,player_) ||
           !units_.matches(live.world))throw std::invalid_argument("Owned Warrior scene revision missing/differs");
        const auto &source=terrain_->terrain().source();
        if(generation_.verified_masks!=960 || generation_.source_payload_sha256!=receipt_.restored_payload_sha256 ||
           generation_.final_height_pool_sha256!=runtime::identify_bytes(terrain_->terrain().owner().height.pool_bytes()).sha256 ||
           generation_.expanded_grid_sha256!=runtime::identify_bytes(source.initial_cells).sha256)
            throw std::invalid_argument("Owned Warrior retained terrain generation receipt differs");
        verify_bnw_warrior_owned_terrain_cells(live.world,source.initial_cells);
        (void)transforms(live);
    }
    void select_sdk_sample(int index) {backend_.select_sdk_sample(index);}
    std::size_t buffer_uploads() const noexcept {return backend_.buffer_uploads();}
    const std::string &metadata_sha256() const noexcept {return backend_.metadata_sha256();}
    std::array<terrain::Matrix4,12> transforms(const runtime::LiveSessionState &live) const {
        if(!matches_bnw_renderer_scene_receipt(receipt_,live.world,live.setup.game_turn,player_))
            throw std::invalid_argument("Owned Warrior live scene changed; owner refresh required");
        const auto &world=terrain_->terrain();
        verify_bnw_warrior_owned_terrain_cells(live.world,world.source().initial_cells);
        auto native_rules=rules_;native_rules.scale=backend_.native_model_scale(rules_.scale);
        return bnw_warrior_owned_scene_transforms_with_rules(units_,live.world,player_,id_,
            world.source().layout,world.owner(),receipt_.wrap_x,bnw_unit_initial_minimum_height(),native_rules);
    }
    bool center_unit_visibility_for_draw(const runtime::LiveSessionState& live) const;
    GlBnwWarriorDrawResult draw(const runtime::LiveSessionState &live,const GlBnwWarriorFrame &frame) {
        if(!center_unit_visibility_for_draw(live)) {
            GlBnwWarriorDrawResult excluded;
            excluded.blocker="Actual owned center-unit visibility excluded this imported unit";
            return excluded;
        }
        return backend_.draw_diagnostic(transforms(live),frame);
    }
  private:
    friend class BnwInitialCenterUnitRendererStorage;
    friend class BnwInitialCenterUnitRendererLoan;
    friend class BnwInitialCenterUnitVisibilityProof;
    friend struct ::civ5::validation::PreviewPresenter;
    static bool same_center_control(const auto& a,const auto& b) {
        return !a.owner_before(b)&&!b.owner_before(a);
    }
    const std::thread::id center_creator_=std::this_thread::get_id();
    std::weak_ptr<BnwInitialCenterUnitRendererStorage> center_storage_;
    mutable bool center_visible_=true; // Preserve existing draw until an actual issued store.
    mutable std::uint64_t center_visibility_serial_=0;
    void store_center_unit_visibility(const BnwInitialCenterUnitRendererStorage&,bool) const;
    BnwRendererSceneReceipt receipt_;
    std::shared_ptr<const materials::MaterialWorld> terrain_;
    BnwWarriorOwnedTerrainGenerationReceipt generation_;
    BnwImportedUnitVisualOwner units_;
    BnwUnitMemberTransformRules rules_;
    std::int64_t player_,id_;
    GlBnwWarriorRetainedBackend backend_;
    // Last member retires BEFORE backend destruction. No observer owns this
    // lifetime strongly, so an old renderer pointer can never revive itself.
    const std::shared_ptr<const void> center_lifetime_=std::make_shared<const std::uint8_t>(0);
};
inline BnwInitialCenterUnitRendererStorage::BnwInitialCenterUnitRendererStorage(
    const std::shared_ptr<ui::BnwClientSceneFrame>& scene,GlBnwWarriorOwnedScene& imported,
    const std::weak_ptr<const void>& imported_lifetime,const std::weak_ptr<const void>& paint,
    std::uint64_t presented)
    :scene_(scene),host_(scene->selected_host_),state_(&scene->session_.state()),
     generation_(scene->session_.state_generation()),initial_present_count_(presented),
     imported_(&imported),imported_lifetime_(imported_lifetime),paint_lifetime_(paint),
     imported_occurrence_(&imported.units_.find(imported.player_,imported.id_)),
     actual_receivers_{{{imported.player_,imported.id_},&imported}} {}
inline void BnwInitialCenterUnitRendererStorage::identity() const {
    if(creator_!=std::this_thread::get_id()||paint_lifetime_.expired()||imported_lifetime_.expired())
        throw std::runtime_error("Center renderer creator/paint/imported lifetime retired");
    const auto canonical=self_.lock();
    const auto scene=scene_.lock();
    if(!canonical||canonical.get()!=this||!GlBnwWarriorOwnedScene::same_center_control(canonical,self_)||
       canonical.use_count()<=static_cast<long>(borrowed_pins_+1)||!scene||!GlBnwWarriorOwnedScene::same_center_control(scene,scene_)||
       !GlBnwWarriorOwnedScene::same_center_control(scene,scene->gui_self_)||
       scene->gui_thread_!=creator_||scene->gui_publication_pins_.load()+1>=static_cast<std::size_t>(scene.use_count()))
        throw std::runtime_error("Center renderer canonical Scene/storage facade retired");
    const auto associated=scene->center_unit_renderer_.lock();
    if(!associated||associated.get()!=this||!GlBnwWarriorOwnedScene::same_center_control(associated,self_)||
       !scene->selected_host_||!GlBnwWarriorOwnedScene::same_center_control(host_,scene->selected_host_)||
       &scene->session_.state()!=state_||scene->scene_state_!=state_||
       scene->session_.state_generation()!=generation_||scene->scene_state_generation_!=generation_||
       initial_present_count_==0)
        throw std::runtime_error("Center renderer actual initialization/source registration changed");
    // Current-find before inspecting the actual unique renderer. The registry
    // mirrors the sole currently owned drawn actor; no saved roster/hidden node
    // can manufacture a Created/model receiver.
    const auto receiver=actual_receivers_.find({imported_occurrence_->owner,imported_occurrence_->id});
    if(actual_receivers_.size()!=1||receiver==actual_receivers_.end()||receiver->second!=imported_)
        throw std::runtime_error("Center renderer actual full-owner/full-ID receiver registry changed");
    // Lifetime is checked before this borrowed unique renderer is dereferenced.
    if(imported_->center_creator_!=creator_||
       !GlBnwWarriorOwnedScene::same_center_control(imported_lifetime_,imported_->center_lifetime_)||
       !GlBnwWarriorOwnedScene::same_center_control(imported_->center_storage_,self_)||
       &imported_->units_.find(imported_->player_,imported_->id_)!=imported_occurrence_)
        throw std::runtime_error("Center renderer SAME imported occurrence/storage changed");
}
inline void BnwInitialCenterUnitRendererStorage::require_missing_center_receiver(
    std::int32_t owner,std::int32_t full_id) const {
    identity();
    if(actual_receivers_.find({owner,full_id})!=actual_receivers_.end())
        throw std::runtime_error("Center pending identity already has an actual registered renderer receiver");
    identity(); // SAME actual registry/control/Scene/initialized lifetime POST.
}
inline std::shared_ptr<BnwInitialCenterUnitRendererStorage>
BnwInitialCenterUnitRendererStorage::after_actual_present(
    const std::shared_ptr<ui::BnwClientSceneFrame>& scene,GlBnwWarriorOwnedScene& imported,
    const std::weak_ptr<const void>& imported_lifetime,const std::weak_ptr<const void>& paint,
    std::uint64_t presented) {
    const auto creator=std::this_thread::get_id();
    if(!scene||scene->gui_thread_!=creator||paint.expired()||imported_lifetime.expired()||presented==0)
        throw std::runtime_error("Center renderer requires actual completed camera presentation");
    if(!scene->selected_host_||!GlBnwWarriorOwnedScene::same_center_control(scene,scene->gui_self_)||
       !scene->center_unit_renderer_.expired())
        throw std::runtime_error("Center renderer canonical selected Scene missing/already initialized");
    // The caller's real scene.lock is borrowed through this factory. Count it
    // before ANY mutable source callback so it cannot hide facade retirement.
    struct FactoryScenePin {
        std::atomic<std::size_t>& pins;
        explicit FactoryScenePin(std::atomic<std::size_t>& actual):pins(actual) {++pins;}
        ~FactoryScenePin() noexcept {--pins;}
    } factory_pin(scene->gui_publication_pins_);
    const auto factory_scene_current=[&] {
        if(scene->gui_thread_!=creator||!GlBnwWarriorOwnedScene::same_center_control(scene,scene->gui_self_)||
           static_cast<std::size_t>(scene.use_count())<=scene->gui_publication_pins_.load()||
           paint.expired()||imported_lifetime.expired())
            throw std::runtime_error("Center renderer actual Scene facade retired before source callback");
    };
    factory_scene_current();
    const auto current_frame=scene->frame_;
    scene->verify_owned();factory_scene_current();
    if(paint.expired()||imported_lifetime.expired())throw std::runtime_error("Center renderer retired in source callback");
    if(imported.center_creator_!=creator||!GlBnwWarriorOwnedScene::same_center_control(imported_lifetime,imported.center_lifetime_))
        throw std::runtime_error("Center renderer imported control differs");
    scene->require_publication_finality(current_frame);
    if(!scene->current())throw std::runtime_error("Center renderer actual painted Scene is stale");
    factory_scene_current();
    if(paint.expired()||imported_lifetime.expired())throw std::runtime_error("Center renderer retired in final initialization callback");
    scene->require_publication_finality(current_frame);
    (void)imported.transforms(scene->session_.state());
    auto result=std::shared_ptr<BnwInitialCenterUnitRendererStorage>(new BnwInitialCenterUnitRendererStorage(
        scene,imported,imported_lifetime,paint,presented));
    result->self_=result;
    // No callback after these real source registrations.
    imported.center_storage_=result;scene->center_unit_renderer_=result;
    result->identity();return result;
}
inline bool GlBnwWarriorOwnedScene::center_unit_visibility_for_draw(const runtime::LiveSessionState& live) const {
    if(center_creator_!=std::this_thread::get_id())throw std::runtime_error("Center visibility draw foreign thread");
    const std::weak_ptr<BnwInitialCenterUnitRendererStorage> empty;
    if(center_storage_.owner_before(empty)||empty.owner_before(center_storage_)) {
        const auto storage=center_storage_.lock();
        if(!storage)throw std::runtime_error("Associated center renderer storage retired");
        storage->identity();
        if(storage->state_!=&live||storage->imported_!=this)throw std::runtime_error("Center visibility draw belongs to another actual state/renderer");
    }
    (void)transforms(live); // Existing receipt/terrain/roster checks also apply to excluded draws.
    return center_visible_;
}
inline void GlBnwWarriorOwnedScene::store_center_unit_visibility(
    const BnwInitialCenterUnitRendererStorage& storage,bool visible) const {
    storage.identity();
    if(storage.imported_!=this||!same_center_control(center_storage_,storage.self_)||
       center_visibility_serial_==std::numeric_limits<std::uint64_t>::max())
        throw std::runtime_error("Center visibility actual receiver/serial mismatch");
    ++center_visibility_serial_;center_visible_=visible;
    storage.identity();
}
} // namespace civ5::graphics
