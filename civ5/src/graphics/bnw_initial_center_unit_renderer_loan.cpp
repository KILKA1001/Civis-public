#include "civ5/graphics/bnw_initial_center_unit_renderer_loan.hpp"
#include "civ5/graphics/bnw_pending_unit_visual_owner.hpp"
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include "civ5/ui/bnw_owned_camera_gui_screen.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include "../../validation/gl_bnw_warrior_owned_scene.hpp"
#include <exception>
#include <limits>
#include <optional>
#include <vector>

namespace civ5::graphics {
namespace {
void center_need(bool v,const char* why){if(!v)throw std::runtime_error(why);}
template<class A,class B>bool center_control(const A&a,const B&b){return !a.owner_before(b)&&!b.owner_before(a);}
}
struct BnwInitialCenterUnitVisibilityProof::State {
    using Core=runtime::BnwNativeUnitAllocatorCore;
    using Plan=Core::InitialCenterUnitVisibilityPlan;
    const std::thread::id creator=std::this_thread::get_id();
    std::weak_ptr<const BnwInitialCenterUnitVisibilityProof> self;
    std::weak_ptr<const Core> core;
    std::weak_ptr<Plan::Storage> plan;
    std::weak_ptr<BnwInitialCenterUnitRendererStorage> renderer;
    std::weak_ptr<BnwPendingUnitVisualOwner> pending_receiver;
    Plan::Binding binding;
    std::vector<Plan::Entry> entries;
    std::vector<Plan::Setter> setters;
    std::vector<std::uint64_t> stores;
    struct MissingReceiver {
        std::weak_ptr<BnwInitialCenterUnitRendererStorage> source;
        std::int32_t owner=-1,full_id=-1;
        const void* node=nullptr;
        std::weak_ptr<const void> node_control;
        void current(const Plan::Entry& entry,const BnwInitialCenterUnitRendererStorage& actual)const {
            center_need(entry.pending&&entry.resolved&&owner==entry.owner&&full_id==entry.full_id&&
                node==entry.native_node&&center_control(node_control,entry.native_control)&&
                !node_control.expired()&&center_control(source,actual.self_),
                "Center missing receiver receipt is not SAME canonical pending Plan node/control");
            actual.require_missing_center_receiver(owner,full_id);
        }
    };
    std::vector<std::optional<MissingReceiver>> missing_receivers;
    bool fanout=false;
    std::size_t borrowed_core_pins=0;
    static bool entry_same(const Plan::Entry&a,const Plan::Entry&b){
        return a.owner==b.owner&&a.asked_id==b.asked_id&&a.full_id==b.full_id&&a.unit_type==b.unit_type&&a.imported==b.imported&&
            a.native_node==b.native_node&&center_control(a.native_control,b.native_control)&&
            a.pending==b.pending&&a.resolved==b.resolved&&a.visible==b.visible&&
            a.check_visibility==b.check_visibility&&a.duration==b.duration;
    }
    void matches(const Plan& actual)const {
        actual.require_current_without_callbacks();
        const auto b=actual.capture_binding();
        center_need(center_control(plan,actual.storage_)&&b.core==binding.core&&
            center_control(b.core_control,binding.core_control)&&b.host.get()==binding.host.get()&&
            center_control(b.host,binding.host)&&b.session==binding.session&&b.state.get()==binding.state.get()&&
            center_control(b.state,binding.state)&&b.pending.get()==binding.pending.get()&&
            center_control(b.pending,binding.pending)&&b.source==binding.source&&b.registry==binding.registry&&
            center_control(b.registry_control,binding.registry_control)&&b.bank==binding.bank&&
            center_control(b.bank_control,binding.bank_control)&&b.generation==binding.generation&&b.world_revision==binding.world_revision&&
            b.active_player==binding.active_player&&b.active_team==binding.active_team&&b.x==binding.x&&
            b.y==binding.y&&b.plot==binding.plot&&b.attempt_serial==binding.attempt_serial,
            "Center visibility proof Plan canonical source/control differs");
        const auto& rows=actual.ordered_entries();const auto& journal=actual.setter_journal();
        center_need(rows.size()==entries.size()&&journal.size()==setters.size()&&
                    actual.requires_visibility_fanout()==fanout,"Center visibility proof complete roster differs");
        for(std::size_t n=0;n<rows.size();++n)center_need(entry_same(rows[n],entries[n]),"Center visibility proof actual roster entry differs");
        for(std::size_t n=0;n<journal.size();++n)center_need(journal[n].before==setters[n].before&&journal[n].after==setters[n].after&&
            journal[n].unit_info_dirty==setters[n].unit_info_dirty,"Center visibility proof setter journal differs");
        actual.require_current_without_callbacks();
    }
    void current(const Core& supplied)const {
        center_need(creator==std::this_thread::get_id(),"Center visibility proof foreign creator");
        const auto retained=supplied.retain_factory_owner();
        center_need(retained.get()==binding.core&&retained.get()==&supplied&&center_control(core,retained)&&retained.use_count()>static_cast<long>(borrowed_core_pins+1),
                    "Center visibility proof Core canonical/outside owner retired");
        const auto storage=plan.lock();center_need(bool(storage),"Center visibility proof canonical Plan retired");
        const Plan actual(storage);matches(actual);actual.require_visibility_dispatch_without_callbacks();
        const auto source=renderer.lock();center_need(bool(source),"Center visibility proof initialized renderer retired");
        // Count this real checker lock before source checks so it cannot mask
        // retirement of the Presenter-held initialized renderer facade.
        struct RendererPin {
            BnwInitialCenterUnitRendererStorage& source;
            explicit RendererPin(BnwInitialCenterUnitRendererStorage& s):source(s){++source.borrowed_pins_;}
            ~RendererPin()noexcept{--source.borrowed_pins_;}
        } pin(*source);
        source->identity();
        center_need(source->state_==binding.state.get()&&source->generation_==binding.generation&&
            center_control(source->host_,binding.host),"Center visibility proof renderer world differs");
        center_need(stores.size()==entries.size()&&missing_receivers.size()==entries.size(),"Center visibility proof incomplete actual outcomes");
        for(std::size_t n=0;n<entries.size();++n){
            const auto& e=entries[n];
            if(!fanout||!e.resolved){center_need(stores[n]==0&&!missing_receivers[n],"Center visibility proof invented skipped effect");continue;}
            if(e.pending){
                center_need(stores[n]==0&&missing_receivers[n].has_value(),
                    "Center missing renderer outcome fabricated a visibility store");
                missing_receivers[n]->current(e,*source);
                const auto receiver=pending_receiver.lock();center_need(bool(receiver),"Center visibility proof pending origin retired");
                receiver->require_center_hidden_node(supplied,e.native_node,e.native_control);
                missing_receivers[n]->current(e,*source);
            }else{
                center_need(stores[n]!=0&&!missing_receivers[n],"Center registered imported receiver lacks actual store");
                center_need(source->imported_occurrence_->owner==e.owner&&source->imported_occurrence_->id==e.full_id&&
                    source->imported_occurrence_->type==e.unit_type&&source->imported_->center_visibility_serial_==stores[n]&&
                    source->imported_->center_visible_==e.visible,"Center visibility proof imported actual store changed");
            }
        }
        source->identity();actual.require_current_without_callbacks();
    }
};
struct BnwInitialCenterUnitRendererLoan::State {
    using Core=runtime::BnwNativeUnitAllocatorCore;
    using Plan=Core::InitialCenterUnitVisibilityPlan;
    const std::thread::id creator=std::this_thread::get_id();
    std::shared_ptr<ui::BnwClientSceneFrame> scene;
    std::shared_ptr<const ui::BnwOwnedCameraGuiScreen> gui;
    std::shared_ptr<const ui::BnwClientFrame> frame;
    std::shared_ptr<BnwInitialCenterUnitRendererStorage> renderer;
    std::shared_ptr<BnwPendingUnitVisualOwner> receiver;
    std::shared_ptr<const Core> core;
    const runtime::BnwUnitMovePending* pending=nullptr;
    const runtime::BnwStockLuaAttemptLease attempt;
    std::optional<runtime::BnwStockLuaAttemptLease::ScopeIdentity> scope;
    const BnwInitialCenterUnitRendererLoan* loan=nullptr;
    std::uint64_t serial=0;
    bool counted=false,active=false,failed=false,preflighted=false,applied=false;
    std::shared_ptr<BnwInitialCenterUnitVisibilityProof::State> snapshot;
    State(const ui::BnwClientSceneFrame& supplied,const Core& actual_core,
          const runtime::BnwUnitMovePending& actual_pending,const runtime::BnwStockLuaAttemptLease& actual_attempt)
        :pending(&actual_pending),attempt(actual_attempt){
        center_need(supplied.gui_thread_==creator,"Center renderer loan Scene foreign creator");
        if(supplied.center_unit_renderer_action_){supplied.center_unit_renderer_failed_=true;throw std::runtime_error("Center renderer loan reentry");}
        center_need(!supplied.center_unit_renderer_failed_,"Center renderer loan quarantined");
        scene=supplied.gui_self_.lock();
        center_need(scene&&scene.get()==&supplied&&center_control(scene,supplied.gui_self_)&&
            scene->gui_publication_pins_.load()+1<static_cast<std::size_t>(scene.use_count()),"Center renderer loan Scene outside facade retired");
        renderer=scene->center_unit_renderer_.lock();receiver=scene->pending_unit_visuals_;gui=scene->gui_screen_.lock();frame=scene->frame_;
        center_need(renderer&&receiver&&gui&&frame&&gui->thread_==creator&&center_control(gui,gui->self_)&&
            gui.use_count()>static_cast<long>(gui->pins_+1),"Center renderer loan actual initialized owner/GUI absent");
        core=actual_core.retain_factory_owner();
        center_need(core.get()==&actual_core&&core.use_count()>1,"Center renderer loan Core outside facade absent");
        center_need(scene->center_unit_renderer_serial_!=std::numeric_limits<std::uint64_t>::max(),"Center renderer loan serial exhausted");
        serial=++scene->center_unit_renderer_serial_;
        scope.emplace(attempt.pin_active_scope_without_callbacks());
        center_need(scope->creator==creator&&scope->control&&center_control(scope->control->selected_host,scene->selected_host_),
            "Center renderer loan actual Attempt Host differs");
        ++gui->pins_;++scene->gui_publication_pins_;++renderer->borrowed_pins_;counted=true;
    }
    void pure()const {
        center_need(creator==std::this_thread::get_id()&&active&&!failed&&loan&&
            scene->center_unit_renderer_action_==loan&&scene->center_unit_renderer_serial_==serial&&
            !scene->center_unit_renderer_failed_,"Center renderer loan active stack retired");
        center_need(center_control(scene,scene->gui_self_)&&
            static_cast<std::size_t>(scene.use_count())>scene->gui_publication_pins_.load()&&
            gui->thread_==creator&&center_control(gui,gui->self_)&&gui.use_count()>static_cast<long>(gui->pins_)&&
            core.use_count()>1&&center_control(core,core->factory_self_),"Center renderer loan canonical outside facade retired");
        center_need(scope.has_value(),"Center renderer loan actual Attempt scope absent");
        attempt.require_scope_identity_without_callbacks(*scope);
        {
            const auto associated=scene->gui_screen_.lock();
            center_need(associated&&associated.get()==gui.get()&&center_control(associated,gui),
                "Center renderer loan current GUI association replaced");
        } // The checker lock ends before ANY provider/target call.
        center_need(scene->frame_.get()==frame.get()&&center_control(scene->frame_,frame),
            "Center renderer loan original actual Frame association replaced");
        gui->require_origin_lifetime_without_frame_callbacks();
        renderer->identity();
        const auto registered=scene->center_unit_renderer_.lock();
        center_need(registered&&registered.get()==renderer.get()&&center_control(registered,renderer)&&
            scene->pending_unit_visuals_.get()==receiver.get()&&center_control(scene->pending_unit_visuals_,receiver)&&
            scope->world==renderer->state_,"Center renderer loan registration/world changed");
    }
    void plan(const Plan& actual)const {
        pure();actual.require_current_without_callbacks();
        const auto b=actual.capture_binding();
        center_need(b.core==core.get()&&center_control(b.core_control,core)&&b.pending.get()==pending&&
            b.host.get()==scene->selected_host_.get()&&center_control(b.host,scene->selected_host_)&&
            b.session==&scene->session_&&b.state.get()==scope->world&&b.state.get()==renderer->state_&&
            b.generation==scene->scene_state_generation_&&b.attempt_serial==scope->serial,
            "Center renderer loan genuine Plan/Core/Pending/Attempt source differs");
        if(snapshot)snapshot->matches(actual);
        pure();actual.require_current_without_callbacks();
    }
    void pre_current()const {
        // ONE original Scene/GUI spans genuine Init mutations. Validate its
        // canonical origin/initialized renderer and fresh owned sources, not
        // the deliberately stale pre-Init Frame/UI revision as readiness.
        pure();scene->verify_owned();pure();
        gui->require_origin_lifetime_without_frame_callbacks();pure();
        attempt.require_scope_identity_without_callbacks(*scope);pure();
    }
    void final_current()const {
        pure();scene->verify_owned();pure(); // No stale pre-store frame check after actual Core effects.
        if(snapshot){
            const auto retained=snapshot->plan.lock();center_need(bool(retained),"Center renderer final Plan retired");
            const Plan actual(retained);snapshot->matches(actual);
            if(applied)snapshot->current(*core);
        }
        pure();
    }
    void close()noexcept {
        if(active){scene->center_unit_renderer_action_=nullptr;active=false;loan=nullptr;}
    }
    ~State(){close();if(snapshot)--snapshot->borrowed_core_pins;if(counted){--renderer->borrowed_pins_;--gui->pins_;--scene->gui_publication_pins_;}}
};
BnwInitialCenterUnitVisibilityProof::BnwInitialCenterUnitVisibilityProof(std::shared_ptr<const State> state):state_(std::move(state)){}
std::shared_ptr<const BnwInitialCenterUnitVisibilityProof> BnwInitialCenterUnitVisibilityProof::retain_factory_owner()const {
    center_need(state_&&state_->creator==std::this_thread::get_id(),"Center visibility proof creator absent");
    const auto self=state_->self.lock();center_need(self&&self.get()==this&&center_control(self,state_->self),"Center visibility proof canonical factory differs");return self;
}
void BnwInitialCenterUnitVisibilityProof::require_for_plan(const runtime::BnwNativeUnitAllocatorCore::InitialCenterUnitVisibilityPlan& plan)const {
    const auto self=retain_factory_owner();state_->matches(plan);
    const auto binding=plan.capture_binding();center_need(binding.core,"Center visibility proof Core retired");state_->current(*binding.core);
}
void BnwInitialCenterUnitVisibilityProof::require_current_for_core(const runtime::BnwNativeUnitAllocatorCore& core)const {
    const auto self=retain_factory_owner();state_->current(core);
}
void BnwInitialCenterUnitRendererLoan::require_current_for_center(const runtime::BnwNativeUnitAllocatorCore& core,
    const runtime::BnwUnitMovePending& pending)const {
    state_.pure();center_need(state_.core.get()==&core&&state_.pending==&pending,"Center renderer loan substituted Core/Pending");
    if(state_.preflighted)state_.final_current();else state_.pre_current();
}
void BnwInitialCenterUnitRendererLoan::preflight_plot_unit_visibility(const runtime::BnwNativeUnitAllocatorCore::InitialCenterUnitVisibilityPlan& plan)const {
    state_.pure();center_need(!state_.preflighted&&!state_.applied,"Center visibility preflight replay");state_.pre_current();state_.plan(plan);
    auto snapshot=std::make_shared<BnwInitialCenterUnitVisibilityProof::State>();
    snapshot->core=state_.core;snapshot->plan=plan.storage_;snapshot->renderer=state_.renderer;
    snapshot->pending_receiver=state_.receiver;snapshot->binding=plan.capture_binding();
    snapshot->entries=plan.ordered_entries();snapshot->setters=plan.setter_journal();
    snapshot->fanout=plan.requires_visibility_fanout();snapshot->stores.resize(snapshot->entries.size(),0);
    snapshot->missing_receivers.resize(snapshot->entries.size());
    state_.plan(plan);
    for(std::size_t n=0;n<snapshot->entries.size();++n){
        const auto& e=snapshot->entries[n];
        if(!snapshot->fanout||!e.resolved)continue;
        center_need(!e.check_visibility&&e.duration==0.01f,"Center visibility actual native arguments unsupported");
        if(e.pending){
            // Actual Core36 phase is BEFORE GameplayUnitCreated. Authenticate
            // the pending native origin, then perform a genuine full-key lookup
            // in the initialized drawn actor registry. A MISS is a no-effect
            // outcome for either requested visibility value, NOT false-store.
            state_.receiver->require_center_hidden_node(*state_.core,e.native_node,e.native_control);
            state_.renderer->require_missing_center_receiver(e.owner,e.full_id);
            snapshot->missing_receivers[n].emplace(BnwInitialCenterUnitVisibilityProof::State::MissingReceiver{
                state_.renderer,e.owner,e.full_id,e.native_node,e.native_control});
            snapshot->missing_receivers[n]->current(e,*state_.renderer);
        }else{
            const auto& actual=*state_.renderer->imported_occurrence_;
            center_need(e.imported&&actual.owner==e.owner&&actual.id==e.full_id&&actual.type==e.unit_type&&
                actual.x==snapshot->binding.x&&actual.y==snapshot->binding.y,
                "Center visibility complete roster contains an unregistered imported renderer");
        }
        state_.plan(plan);
    }
    state_.pre_current();state_.plan(plan);snapshot->matches(plan);
    snapshot->borrowed_core_pins=1;state_.snapshot=std::move(snapshot);state_.preflighted=true;state_.pure();
}
std::shared_ptr<const BnwInitialCenterUnitVisibilityProof> BnwInitialCenterUnitRendererLoan::apply_plot_unit_visibility(
    const runtime::BnwNativeUnitAllocatorCore::InitialCenterUnitVisibilityPlan& plan)const {
    state_.plan(plan);center_need(state_.preflighted&&!state_.applied&&state_.snapshot,"Center visibility apply lacks exact preflight/replayed");
    state_.final_current();state_.plan(plan);
    plan.require_visibility_dispatch_without_callbacks();state_.plan(plan);
    for(std::size_t n=0;n<state_.snapshot->entries.size();++n){
        const auto& e=state_.snapshot->entries[n];
        if(!state_.snapshot->fanout||!e.resolved)continue;
        state_.plan(plan);
        if(e.pending){
            center_need(state_.snapshot->missing_receivers[n].has_value(),"Center pending receiver lacks actual preflight MISS");
            const auto& missing=*state_.snapshot->missing_receivers[n];
            missing.current(e,*state_.renderer);
            state_.receiver->require_center_hidden_node(*state_.core,e.native_node,e.native_control);
            missing.current(e,*state_.renderer); // No payload/model/visibility setter on native NULL route.
        }else{
            state_.renderer->imported_->store_center_unit_visibility(*state_.renderer,e.visible);
            state_.snapshot->stores[n]=state_.renderer->imported_->center_visibility_serial_;
        }
        state_.plan(plan);
    }
    state_.applied=true;state_.final_current();state_.snapshot->current(*state_.core);state_.plan(plan);
    auto proof=std::shared_ptr<const BnwInitialCenterUnitVisibilityProof>(new BnwInitialCenterUnitVisibilityProof(state_.snapshot));
    state_.snapshot->self=proof;state_.plan(plan);proof->require_for_plan(plan);return proof;
}
void BnwInitialCenterUnitRendererLoan::require_plot_unit_visibility_current(
    const runtime::BnwNativeUnitAllocatorCore::InitialCenterUnitVisibilityPlan& plan,const BnwInitialCenterUnitVisibilityProof& proof)const {
    state_.plan(plan);center_need(state_.applied&&proof.state_.get()==state_.snapshot.get(),"Center visibility loan proof belongs to another actual operation");
    proof.require_for_plan(plan);state_.final_current();state_.plan(plan);
}
} // namespace civ5::graphics

namespace civ5::ui {
void BnwClientSceneFrame::with_actual_initial_center_unit_renderer(
    const runtime::BnwNativeUnitAllocatorCore& core,const runtime::BnwUnitMovePending& pending,
    const runtime::BnwStockLuaAttemptLease& attempt,
    const std::function<void(const graphics::BnwInitialCenterUnitRendererLoan&)>& incoming)const {
    using Loan=graphics::BnwInitialCenterUnitRendererLoan;
    Loan::State state(*this,core,pending,attempt);Loan loan(state);
    state.loan=&loan;state.active=true;state.scene->center_unit_renderer_action_=&loan;
    std::optional<std::function<void(const Loan&)>> target;
    try{
        state.pure();target.emplace(incoming);state.pure();
        if(!*target)throw std::runtime_error("Center renderer loan actual target absent");
        state.pre_current();(*target)(loan);state.final_current();
        target.reset();state.final_current();state.close();
    }catch(...){
        state.failed=true;state.scene->center_unit_renderer_failed_=true;
        target.reset();state.close();throw;
    }
}
} // namespace civ5::ui
