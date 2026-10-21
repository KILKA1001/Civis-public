#include "civ5/graphics/bnw_pending_unit_visual_owner.hpp"
#include "civ5/materials/material_world.hpp"
#include "civ5/graphics/bnw_unit_visual_owner.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include "civ5/runtime/bnw_unit_move_pending.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include <exception>
#include <map>
#include <optional>
#include <limits>
#include <cmath>
#include <stdexcept>
#include <thread>
#include <utility>

namespace civ5::graphics {
namespace {
void need(bool value,const char* why) {
    if(!value) throw std::runtime_error(why);
}
template<class A,class B> bool same_control(const A& a,const B& b) {
    return !a.owner_before(b)&&!b.owner_before(a);
}
}
struct BnwPendingUnitVisualOwner::Impl {
    using Core=runtime::BnwNativeUnitAllocatorCore;
    using Binding=Core::InitialVisibilityBinding;
    struct Slot {
        Binding binding;
        std::shared_ptr<const BnwPendingUnitHiddenSlotProof> proof;
        bool visibility_applied=false;
        bool visible=false;
        std::optional<Binding> position_binding;
        std::shared_ptr<const BnwPendingUnitPositionProof> position_proof;
        std::array<float,3> world_position{};
        std::int32_t map_x=0,map_y=0;
        bool position_applied=false;
        std::uint64_t center_visibility_serial=0;
        explicit Slot(Binding actual):binding(std::move(actual)) {}
    };
    const std::thread::id creator=std::this_thread::get_id();
    std::weak_ptr<BnwPendingUnitVisualOwner> self;
    std::weak_ptr<ui::BnwClientSceneFrame> scene;
    std::weak_ptr<const ui::SelectedBnwPreviewHost> host;
    const runtime::LiveSessionState* state=nullptr;
    std::uint64_t generation=0;
    std::shared_ptr<const materials::MaterialWorld> terrain_world;
    std::weak_ptr<const void> paint_lifetime;
    const void* layout=nullptr;
    const void* height=nullptr;
    bool busy=false,failed=false;
    std::map<const void*,Slot,std::less<const void*>> slots;

    void thread() const {
        need(creator==std::this_thread::get_id(),"Pending visual creator thread changed");
    }
    void scene_identity(const std::shared_ptr<ui::BnwClientSceneFrame>& actual,
                        const BnwPendingUnitVisualOwner* owner,bool registered) const {
        thread();
        need(actual&&same_control(scene,actual)&&same_control(actual->gui_self_,actual),
             "Pending visual Scene control changed");
        need(actual->gui_thread_==creator,"Pending visual Scene creator changed");
        // Exclude this stack's one real Scene pin and all tracked GUI holders.
        // A callback dropping the last external Scene must not resurrect it.
        const auto internal=std::size_t{1}+actual->gui_publication_pins_.load();
        need(static_cast<std::size_t>(actual.use_count())>internal,
             "Pending visual Scene facade retired");
        need(actual->selected_host_&&same_control(host,actual->selected_host_)&&
             &actual->session_.state()==state&&actual->scene_state_==state&&
             actual->scene_state_generation_==generation&&
             actual->session_.state_generation()==generation,
             "Pending visual Host/state generation changed");
        need(!failed,"Pending visual receiver quarantined");
        if(registered) need(actual->pending_unit_visuals_.get()==owner&&
                             same_control(self,actual->pending_unit_visuals_),
                             "Pending visual Scene registration changed");
        else need(!actual->pending_unit_visuals_,"Pending visual Scene already bound");
    }
    void terrain_identity() const {
        thread();
        need(terrain_world&&!paint_lifetime.expired()&&
             layout==&terrain_world->terrain().source().layout&&
             height==&terrain_world->terrain().owner(),
             "Pending visual actual camera terrain source retired");
    }
    struct Guard {
        Impl& p;
        std::shared_ptr<BnwPendingUnitVisualOwner> owner;
        std::shared_ptr<ui::BnwClientSceneFrame> scene;
        const int exceptions=std::uncaught_exceptions();
        explicit Guard(const BnwPendingUnitVisualOwner& supplied):p(*supplied.impl_) {
            p.thread();
            if(p.busy) {p.failed=true;throw std::runtime_error("Pending visual reentry");}
            owner=p.self.lock();
            need(owner&&owner.get()==&supplied&&same_control(p.self,owner)&&owner.use_count()>1,
                 "Pending visual canonical facade retired");
            scene=p.scene.lock();
            p.scene_identity(scene,&supplied,true);
            p.busy=true;
        }
        void identity() const {
            p.thread();
            need(p.busy&&!p.failed&&owner.use_count()>1&&same_control(p.self,owner),
                 "Pending visual active receiver retired");
            p.scene_identity(scene,owner.get(),true);
            if(p.terrain_world) p.terrain_identity();
        }
        void current() const {
            identity();scene->verify_owned();identity();
        }
        ~Guard() {
            if(std::uncaught_exceptions()>exceptions) p.failed=true;
            p.busy=false; // Clear before either stack pin releases its storage.
        }
    };
    static Slot& center_node(const Core& core,const void* node,
                             const std::weak_ptr<const void>& control,const Guard& guard) {
        guard.identity();
        const auto found=guard.p.slots.find(node);
        need(found!=guard.p.slots.end(),"Center visibility actual pending node is not registered");
        auto& slot=found->second;
        need(node&&slot.binding.node==node&&same_control(slot.binding.node_control,control)&&
             !control.expired()&&same_control(slot.binding.core_control,core.factory_self_)&&
             slot.binding.core_control.lock().get()==&core&&slot.proof&&slot.visibility_applied&&
             slot.proof->slot_==&slot&&!slot.visible,
             "Center visibility SAME pending hidden node/control differs");
        binding_identity(slot.binding,guard);guard.identity();return slot;
    }
    static void binding_identity(const Binding& binding,const Guard& guard) {
        guard.identity();
        need(binding.host&&binding.state&&binding.pending&&binding.node&&
             binding.host.get()==guard.scene->selected_host_.get()&&
             same_control(binding.host,guard.scene->selected_host_)&&
             binding.session==&binding.host->session()&&
             binding.state.get()==guard.p.state&&binding.generation==guard.p.generation,
             "Pending visual Core belongs to another actual Scene");
    }
    static std::array<float,3> actual_position(const Binding& binding,const Guard& guard) {
        guard.identity();guard.p.terrain_identity();
        const auto& terrain=guard.p.terrain_world->terrain();
        const auto& source=terrain.source();
        need(source.map_mode==0||source.map_mode==1,
             "Pending position requires actual gameplay terrain wrap mode");
        const auto center=bnw_unit_world_center(source.layout,terrain.owner().parameters,binding.x,binding.y);
        // The fresh renderer geometry owner's genuine constructor floor. This
        // does not assert the unproved Original newborn Teleported linkage.
        const auto z=std::fmax(bnw_unit_initial_minimum_height(),
            bnw_unit_world_height(terrain.owner(),center.x,center.y,source.map_mode==1));
        const std::array<float,3> result{center.x,center.y,z};
        for(const auto value:result)need(std::isfinite(value),"Pending position is not finite");
        guard.identity();return result;
    }
    static void position_identity(const Binding& binding,const BnwPendingUnitHiddenSlotProof& hidden,
                                  const BnwPendingUnitPositionProof& proof,const Guard& guard) {
        binding_identity(binding,guard);guard.p.terrain_identity();
        const auto at=guard.p.slots.find(binding.node);
        need(at!=guard.p.slots.end(),"Pending position SAME slot absent");
        const auto& slot=at->second;
        need(slot.visibility_applied&&!slot.visible&&slot.proof.get()==&hidden&&
             slot.position_applied&&slot.position_binding&&slot.position_proof.get()==&proof&&
             hidden.slot_==&slot&&proof.slot_==&slot&&
             same_control(hidden.owner_,guard.p.self)&&same_control(proof.owner_,guard.p.self)&&
             same_control(hidden.node_,binding.node_control)&&same_control(proof.node_,binding.node_control)&&
             same_control(hidden.core_,binding.core_control)&&same_control(proof.core_,binding.core_control)&&
             same_control(hidden.pending_,binding.pending)&&same_control(proof.pending_,binding.pending)&&
             same_control(proof.hidden_,slot.proof)&&same_control(proof.world_,guard.p.terrain_world)&&
             proof.layout_==guard.p.layout&&proof.height_==guard.p.height,
             "Pending position proof is not this actual hidden slot/terrain/control");
        auto core=binding.core_control.lock();
        need(core&&core.use_count()>1,"Pending position actual Core retired");
        const auto earned_revision=core->initial_happiness_prior_revision_without_callbacks();
        const auto& issued=*slot.position_binding;
        need(issued.host.get()==binding.host.get()&&same_control(issued.host,binding.host)&&
             issued.session==binding.session&&issued.state.get()==binding.state.get()&&
             same_control(issued.state,binding.state)&&issued.pending.get()==binding.pending.get()&&
             same_control(issued.pending,binding.pending)&&issued.node==binding.node&&
             same_control(issued.node_control,binding.node_control)&&same_control(issued.core_control,binding.core_control)&&
             issued.owner==binding.owner&&issued.id==binding.id&&issued.city==binding.city&&
             issued.base_combat==binding.base_combat&&issued.x==binding.x&&issued.y==binding.y&&
             issued.generation==binding.generation&&issued.revision==binding.revision&&
             slot.map_x==binding.x&&slot.map_y==binding.y&&proof.map_x_==slot.map_x&&proof.map_y_==slot.map_y&&
             proof.revision_==earned_revision&&proof.world_position_==slot.world_position,
             "Pending position issued binding/earned revision/stored transform changed");
        need(slot.world_position==actual_position(binding,guard),
             "Pending position transform differs from SAME actual renderer world");
        core->require_initial_visibility_hidden_slot_identity_without_callbacks(binding,hidden);
        guard.identity();
    }

};

BnwPendingUnitHiddenSlotProof::BnwPendingUnitHiddenSlotProof(
    std::weak_ptr<BnwPendingUnitVisualOwner> owner,const void* slot,
    std::weak_ptr<const void> node,std::weak_ptr<const runtime::BnwNativeUnitAllocatorCore> core,
    std::weak_ptr<const runtime::BnwUnitMovePending> pending)
    :owner_(std::move(owner)),slot_(slot),node_(std::move(node)),
     core_(std::move(core)),pending_(std::move(pending)) {}

BnwPendingUnitPositionProof::BnwPendingUnitPositionProof(
    std::weak_ptr<BnwPendingUnitVisualOwner> owner,const void* slot,
    std::weak_ptr<const void> node,std::weak_ptr<const runtime::BnwNativeUnitAllocatorCore> core,
    std::weak_ptr<const runtime::BnwUnitMovePending> pending,
    std::weak_ptr<const BnwPendingUnitHiddenSlotProof> hidden,
    std::weak_ptr<const materials::MaterialWorld> world,const void* layout,const void* height,
    std::array<float,3> position,std::int32_t x,std::int32_t y,std::uint64_t revision)
    :owner_(std::move(owner)),slot_(slot),node_(std::move(node)),core_(std::move(core)),
     pending_(std::move(pending)),hidden_(std::move(hidden)),world_(std::move(world)),
     layout_(layout),height_(height),world_position_(position),map_x_(x),map_y_(y),revision_(revision) {}

BnwPendingUnitVisualOwner::BnwPendingUnitVisualOwner(std::unique_ptr<Impl> impl)
    :impl_(std::move(impl)) {}
BnwPendingUnitVisualOwner::~BnwPendingUnitVisualOwner()=default;

std::shared_ptr<BnwPendingUnitVisualOwner> BnwPendingUnitVisualOwner::attach_to_selected_scene(
    const std::shared_ptr<ui::BnwClientSceneFrame>& supplied) {
    need(supplied!=nullptr,"Pending visual selected Scene absent");
    need(supplied->gui_thread_==std::this_thread::get_id(),"Pending visual creator mismatch");
    auto scene=supplied->gui_self_.lock();
    need(scene&&scene.get()==supplied.get()&&same_control(scene,supplied),
         "Pending visual Scene is not its canonical factory owner");
    need(scene->selected_host_!=nullptr,"Legacy Scene cannot issue pending visual slots");
    auto impl=std::make_unique<Impl>();
    impl->scene=scene;impl->host=scene->selected_host_;
    impl->state=scene->scene_state_;impl->generation=scene->scene_state_generation_;
    impl->scene_identity(scene,nullptr,false);
    scene->verify_owned();impl->scene_identity(scene,nullptr,false);
    auto owner=std::shared_ptr<BnwPendingUnitVisualOwner>(new BnwPendingUnitVisualOwner(std::move(impl)));
    owner->impl_->self=owner;
    // Allocation completed before the last actual mutable-source bookend.
    scene->verify_owned();owner->impl_->scene_identity(scene,nullptr,false);
    scene->pending_unit_visuals_=owner;
    return owner;
}

void BnwPendingUnitVisualOwner::require_current() const {
    Impl::Guard guard(*this);guard.current();
}

void BnwPendingUnitVisualOwner::bind_actual_camera_terrain(
    const std::shared_ptr<const materials::MaterialWorld>& supplied,
    const std::weak_ptr<const void>& paint_lifetime) const {
    // Only the genuine named camera presenter calls this after returned present.
    Impl::Guard guard(*this);
    const auto world=supplied; // Own storage before Scene/provider callbacks.
    need(world&&!paint_lifetime.expired(),"Pending visual camera terrain issuer retired");
    const auto* layout=&world->terrain().source().layout;
    const auto* height=&world->terrain().owner();
    const auto same_origin=[&] {
        guard.identity();
        need(!paint_lifetime.expired(),"Pending visual camera paint scope retired");
        if(guard.p.terrain_world) {
            guard.p.terrain_identity();
            need(guard.p.terrain_world.get()==world.get()&&
                 same_control(guard.p.terrain_world,world)&&
                 same_control(guard.p.paint_lifetime,paint_lifetime)&&
                 guard.p.layout==layout&&guard.p.height==height,
                 "Pending visual camera terrain origin changed");
        }
    };
    same_origin();guard.current();same_origin();
    guard.p.terrain_world=world;guard.p.paint_lifetime=paint_lifetime;
    guard.p.layout=layout;guard.p.height=height;
    guard.p.terrain_identity();guard.identity();
}

std::shared_ptr<BnwPendingUnitVisualOwner> BnwPendingUnitVisualOwner::retain_factory_owner() const {
    auto& p=*impl_;p.thread();
    if(p.busy) {p.failed=true;throw std::runtime_error("Pending visual canonicalization reentry");}
    auto owner=p.self.lock();
    need(owner&&owner.get()==this&&same_control(p.self,owner)&&owner.use_count()>1,
         "Pending visual canonical owner retired");
    auto scene=p.scene.lock();p.scene_identity(scene,this,true);
    // Callback-free canonicalization does not confer a current Core/node.
    return owner;
}

std::shared_ptr<const BnwPendingUnitHiddenSlotProof>
BnwPendingUnitVisualOwner::apply_initial_visibility_false_to_selected_scene(
    const runtime::BnwNativeUnitAllocatorCore& core,const runtime::BnwUnitMovePending& pending,
    const std::shared_ptr<ui::BnwClientSceneFrame>& supplied) {
    // Authenticate incoming control BEFORE any Core/provider callback.
    need(supplied!=nullptr,"Pending visibility requires an actual selected Scene");
    need(supplied->gui_thread_==std::this_thread::get_id(),"Pending visibility creator mismatch");
    need(same_control(supplied->gui_self_,supplied)&&supplied->pending_unit_visuals_,
         "Pending visibility canonical Scene receiver absent");
    // Only a weak receiver witness is copied here. The receiver's tracked Guard
    // provides its real pin; this bridge must not inflate outside-owner counts.
    const auto receiver=supplied->pending_unit_visuals_->impl_->self;
    const auto* actual=supplied->pending_unit_visuals_.get();
    need(same_control(receiver,supplied->pending_unit_visuals_),
         "Pending visibility receiver control differs");
    return actual->apply_initial_visibility_false(core,pending);
}

std::shared_ptr<const BnwPendingUnitHiddenSlotProof>
BnwPendingUnitVisualOwner::apply_initial_visibility_false(
    const runtime::BnwNativeUnitAllocatorCore& core,const runtime::BnwUnitMovePending& pending) const {
    Impl::Guard guard(*this);guard.current();
    auto binding=core.capture_initial_visibility_binding(pending);
    guard.identity();Impl::binding_identity(binding,guard);
    need(binding.pending.get()==&pending&&same_control(binding.core_control,core.factory_self_),
         "Pending visibility supplied Core/Pending identity changed");
    core.require_initial_visibility_binding_current(binding);guard.current();
    need(!guard.p.slots.contains(binding.node),"Pending visibility node already admitted");
    // Own the genuine strong storage anchors and allocate the exact slot before
    // the no-allocation false store. Failed preparation never produces an ACK.
    auto [position,inserted]=guard.p.slots.emplace(binding.node,Impl::Slot(std::move(binding)));
    need(inserted,"Pending visibility slot insertion failed");
    auto& slot=position->second;
    slot.proof=std::shared_ptr<const BnwPendingUnitHiddenSlotProof>(
        new BnwPendingUnitHiddenSlotProof(guard.p.self,&slot,slot.binding.node_control,
                                         slot.binding.core_control,slot.binding.pending));
    core.require_initial_visibility_binding_current(slot.binding);guard.current();
    Impl::binding_identity(slot.binding,guard);
    core.require_initial_visibility_binding_identity_without_callbacks(slot.binding);
    guard.identity();
    slot.visible=false;
    slot.visibility_applied=true;
    // A late failure quarantines the receiver; it does not invent rollback of
    // the actual renderer-side visibility store or advance Core publication.
    core.require_initial_visibility_binding_current(slot.binding);guard.current();
    core.require_initial_visibility_binding_identity_without_callbacks(slot.binding);
    guard.identity();
    return slot.proof;
}

std::shared_ptr<const BnwPendingUnitPositionProof>
BnwPendingUnitVisualOwner::apply_initial_set_position(
    const runtime::BnwNativeUnitAllocatorCore& core,const runtime::BnwUnitMovePending& pending,
    const BnwPendingUnitHiddenSlotProof& hidden) const {
    Impl::Guard guard(*this);guard.identity();guard.p.terrain_identity();
    need(same_control(hidden.owner_,guard.p.self)&&same_control(hidden.core_,core.factory_self_),
         "Pending SetPosition foreign hidden receiver/Core");
    auto binding=core.capture_initial_set_position_binding(pending,hidden);
    guard.identity();Impl::binding_identity(binding,guard);
    const auto at=guard.p.slots.find(binding.node);
    need(at!=guard.p.slots.end(),"Pending SetPosition SAME hidden slot absent");
    auto& slot=at->second;
    need(slot.binding.pending.get()==&pending&&binding.pending.get()==&pending&&
         slot.proof.get()==&hidden&&hidden.slot_==&slot&&slot.visibility_applied&&!slot.visible&&
         !slot.position_applied&&!slot.position_proof&&!slot.position_binding,
         "Pending SetPosition is not the genuine first SAME hidden slot operation");
    // Core27 reads the real pending UnitInfo/counts; no caller flags or defaults.
    const auto domain=core.initial_set_position_domain(binding,hidden);
    const auto hovering=core.initial_set_position_is_hovering(binding,hidden);
    const auto impassable=core.initial_set_position_can_move_impassable(binding,hidden);
    guard.identity();Impl::binding_identity(binding,guard);
    // Teleported 78BB88 payload bit: (CanMoveImpassable && SEA) || Hovering.
    // The 53FE50 selector/floor producer is not admitted by this adapter.
    need(!((impassable&&domain==0)||hovering),
         "Pending SetPosition special native floor branch has no genuine producer");
    need(domain==2,"Pending SetPosition normal adapter requires actual DOMAIN_LAND");
    const auto require_same_normal_operands=[&](const Impl::Binding& actual) {
        guard.identity();Impl::binding_identity(actual,guard);
        core.require_initial_set_position_binding_identity_without_callbacks(actual,hidden);
        need(core.initial_set_position_domain(actual,hidden)==domain&&
             core.initial_set_position_is_hovering(actual,hidden)==hovering&&
             core.initial_set_position_can_move_impassable(actual,hidden)==impassable,
             "Pending SetPosition actual branch operands changed during provider");
        guard.identity();Impl::binding_identity(actual,guard);
    };
    require_same_normal_operands(binding);
    core.require_initial_set_position_binding_current(binding,hidden);
    require_same_normal_operands(binding);guard.current();
    require_same_normal_operands(binding);
    core.require_initial_set_position_binding_identity_without_callbacks(binding,hidden);guard.identity();
    const auto position=Impl::actual_position(binding,guard);
    const auto proof=std::shared_ptr<const BnwPendingUnitPositionProof>(new BnwPendingUnitPositionProof(
        guard.p.self,&slot,binding.node_control,binding.core_control,binding.pending,slot.proof,
        guard.p.terrain_world,guard.p.layout,guard.p.height,position,binding.x,binding.y,
        binding.state->ui_source_revision));
    slot.position_binding.emplace(std::move(binding)); // Real strong storage, before the actual no-allocation store.
    const auto& issued=*slot.position_binding;
    require_same_normal_operands(issued);
    core.require_initial_set_position_binding_current(issued,hidden);
    require_same_normal_operands(issued);guard.current();
    require_same_normal_operands(issued);
    core.require_initial_set_position_binding_identity_without_callbacks(issued,hidden);guard.identity();
    slot.position_proof=proof;
    slot.world_position=position;slot.map_x=issued.x;slot.map_y=issued.y;
    slot.position_applied=true; // SAME renderer-owned slot's real world transform, not plot-coordinate intent.
    require_same_normal_operands(issued);
    core.require_initial_set_position_binding_current(issued,hidden);
    require_same_normal_operands(issued);guard.current();
    require_same_normal_operands(issued);
    core.require_initial_set_position_binding_identity_without_callbacks(issued,hidden);
    Impl::position_identity(issued,hidden,*proof,guard);
    return proof; // A late failure quarantines Guard; no invented rollback/ACK.
}

void BnwPendingUnitVisualOwner::require_initial_set_position_identity_without_callbacks(
    const runtime::BnwNativeUnitAllocatorCore::InitialVisibilityBinding& binding,
    const BnwPendingUnitHiddenSlotProof& hidden,const BnwPendingUnitPositionProof& proof) const {
    Impl::Guard guard(*this);Impl::position_identity(binding,hidden,proof,guard);
}
void BnwPendingUnitVisualOwner::require_initial_set_position_current(
    const runtime::BnwNativeUnitAllocatorCore::InitialVisibilityBinding& binding,
    const BnwPendingUnitHiddenSlotProof& hidden,const BnwPendingUnitPositionProof& proof) const {
    Impl::Guard guard(*this);Impl::position_identity(binding,hidden,proof,guard);
    auto core=binding.core_control.lock();
    need(core&&core.use_count()>1,"Pending SetPosition full source Core retired");
    core->require_initial_set_position_binding_current(binding,hidden);
    Impl::position_identity(binding,hidden,proof,guard);guard.current();
    core->require_initial_set_position_binding_current(binding,hidden);
    need(core.use_count()>1,"Pending SetPosition Core retired during provider");
    Impl::position_identity(binding,hidden,proof,guard);
}

void BnwPendingUnitVisualOwner::draw_pending_slots(
    const std::shared_ptr<const ui::BnwClientFrame>& supplied) const {
    Impl::Guard guard(*this);
    const auto frame=supplied; // Snapshot actual publication before providers.
    guard.current();
    if(guard.p.slots.empty()) return;
    need(frame&&same_control(frame,guard.scene->frame_)&&
         same_control(frame,guard.scene->owner_.current_frame()),
         "Pending visual paint frame control differs from actual owner");
    guard.scene->require_publication_finality(frame);guard.identity();
    need(guard.scene->current(),"Pending visual paint publication is stale");
    guard.identity();guard.scene->require_publication_finality(frame);
    for(const auto& [node,slot]:guard.p.slots) {
        guard.identity();
        auto core=slot.binding.core_control.lock();
        need(core&&core.use_count()>1,"Pending visual Core facade retired");
        Impl::binding_identity(slot.binding,guard);
        need(node==slot.binding.node&&slot.visibility_applied&&slot.proof&&
             slot.proof->slot_==&slot&&same_control(slot.proof->owner_,guard.p.self)&&
             same_control(slot.proof->node_,slot.binding.node_control)&&
             same_control(slot.proof->core_,slot.binding.core_control)&&
             same_control(slot.proof->pending_,slot.binding.pending),
             "Pending visual same-slot visibility was not applied");
        // Core's issuer-bound successor authenticates this exact returned proof.
        // The old city-phase validator is deliberately not a later-phase grant.
        core->require_initial_visibility_hidden_slot_current(slot.binding,*slot.proof);guard.current();
        need(core.use_count()>1,"Pending visual Core retired during source callback");
        core->require_initial_visibility_hidden_slot_identity_without_callbacks(slot.binding,*slot.proof);
        guard.identity();guard.scene->require_publication_finality(frame);
        if(slot.position_applied){
            need(slot.position_binding&&slot.position_proof,"Pending paint position storage missing");
            // AFTER Core's normal return use only its issuer-earned successor,
            // never the first-attempt setter-only validator (!initial_position).
            core->require_initial_set_position_without_callbacks();
            Impl::position_identity(*slot.position_binding,*slot.proof,*slot.position_proof,guard);
            // Actual hidden-slot dispatch consumes this stored transform before
            // excluding the node. No visible model/roster admission is inferred.
            const auto painted_position=slot.world_position;
            need(painted_position==slot.position_proof->world_position_,
                 "Pending paint consumed a foreign position transform");
            core->require_initial_set_position_without_callbacks();guard.identity();
        }else need(!slot.position_binding&&!slot.position_proof,
                   "Pending paint observed an incomplete failed position operation");
        // Actual dispatch gate consumes the stored false value from THIS node.
        // No resource/model consumer for visible newborns has been issued.
        need(!slot.visible,"Visible newborn model admission remains unavailable");
        core->require_initial_visibility_hidden_slot_identity_without_callbacks(slot.binding,*slot.proof);
        guard.identity();
    }
    need(guard.scene->current(),"Pending visual paint sources changed");
    guard.identity();guard.scene->require_publication_finality(frame);
    // The final Scene.current above may invoke a real mutable source. Recheck
    // every earned Core/slot/position relation AFTER that last callback.
    for(const auto& [node,slot]:guard.p.slots) {
        guard.identity();
        auto core=slot.binding.core_control.lock();
        need(core&&core.use_count()>1,"Pending paint Core retired in final Scene callback");
        Impl::binding_identity(slot.binding,guard);
        need(node==slot.binding.node&&slot.visibility_applied&&!slot.visible&&slot.proof&&
             slot.proof->slot_==&slot&&same_control(slot.proof->owner_,guard.p.self)&&
             same_control(slot.proof->node_,slot.binding.node_control)&&
             same_control(slot.proof->core_,slot.binding.core_control)&&
             same_control(slot.proof->pending_,slot.binding.pending),
             "Pending paint SAME hidden slot changed in final Scene callback");
        core->require_initial_visibility_hidden_slot_identity_without_callbacks(slot.binding,*slot.proof);
        if(slot.position_applied) {
            need(slot.position_binding&&slot.position_proof,"Pending final paint position storage missing");
            core->require_initial_set_position_without_callbacks();
            Impl::position_identity(*slot.position_binding,*slot.proof,*slot.position_proof,guard);
        } else need(!slot.position_binding&&!slot.position_proof,
                    "Pending final paint has an incomplete failed position operation");
        guard.identity();
    }
    guard.scene->require_publication_finality(frame);guard.identity();
}
}

namespace civ5::graphics {
std::shared_ptr<const BnwPendingUnitHiddenSlotProof>
BnwPendingUnitVisualOwner::require_action_hidden_slot_proof(
    const runtime::BnwNativeUnitAllocatorCore& core,const runtime::BnwUnitMovePending& pending) const {
    Impl::Guard guard(*this);guard.identity();
    for(const auto& [node,slot]:guard.p.slots) {
        if(slot.binding.pending.get()!=&pending) continue;
        Impl::binding_identity(slot.binding,guard);
        need(node==slot.binding.node&&slot.visibility_applied&&!slot.visible&&slot.proof&&
             slot.proof->slot_==&slot&&same_control(slot.proof->owner_,guard.p.self)&&
             same_control(slot.binding.core_control,core.factory_self_),
             "TRAIN action does not own actual hidden slot");
        core.require_initial_visibility_hidden_slot_identity_without_callbacks(slot.binding,*slot.proof);
        guard.identity();return slot.proof;
    }
    throw std::runtime_error("TRAIN action has no actual hidden Pending slot");
}
void BnwPendingUnitVisualOwner::require_action_hidden_slot_proof_current(
    const BnwPendingUnitHiddenSlotProof& proof) const {
    Impl::Guard guard(*this);guard.identity();
    for(const auto& [node,slot]:guard.p.slots) {
        if(slot.proof.get()!=&proof) continue;
        auto core=slot.binding.core_control.lock();
        need(core&&core.use_count()>1,"TRAIN action Core outside owner retired");
        Impl::binding_identity(slot.binding,guard);
        need(node==slot.binding.node&&slot.visibility_applied&&!slot.visible&&proof.slot_==&slot&&
             same_control(proof.owner_,guard.p.self),"TRAIN action hidden proof detached");
        core->require_initial_visibility_hidden_slot_identity_without_callbacks(slot.binding,proof);
        guard.identity();return;
    }
    throw std::runtime_error("TRAIN action hidden proof not issued by receiver");
}
}

namespace civ5::graphics {
void BnwPendingUnitVisualOwner::require_center_hidden_node(const runtime::BnwNativeUnitAllocatorCore& core,
    const void* node,const std::weak_ptr<const void>& control) const {
    Impl::Guard guard(*this);(void)Impl::center_node(core,node,control,guard);guard.identity();
}
std::uint64_t BnwPendingUnitVisualOwner::store_center_hidden_false(const runtime::BnwNativeUnitAllocatorCore& core,
    const void* node,const std::weak_ptr<const void>& control) const {
    Impl::Guard guard(*this);auto& slot=Impl::center_node(core,node,control,guard);
    need(slot.center_visibility_serial!=std::numeric_limits<std::uint64_t>::max(),"Center pending visibility store serial exhausted");
    ++slot.center_visibility_serial;slot.visible=false; // Actual same hidden draw slot consumes this value.
    guard.identity();return slot.center_visibility_serial;
}
void BnwPendingUnitVisualOwner::require_center_hidden_store(const runtime::BnwNativeUnitAllocatorCore& core,
    const void* node,const std::weak_ptr<const void>& control,std::uint64_t serial) const {
    Impl::Guard guard(*this);const auto& slot=Impl::center_node(core,node,control,guard);
    need(serial!=0&&slot.center_visibility_serial==serial&&!slot.visible,"Center pending visibility actual store changed");guard.identity();
}
}
