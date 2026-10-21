#include "civ5/graphics/bnw_selected_pending_unit_visual_loan.hpp"
#include "civ5/graphics/bnw_pending_unit_visual_owner.hpp"
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include "civ5/ui/bnw_owned_camera_gui_screen.hpp"
#include <exception>
#include <limits>
#include <optional>
#include <stdexcept>
#include <thread>

namespace civ5::graphics {
namespace {
void require(bool value,const char* why) { if(!value)throw std::runtime_error(why); }
template<class A,class B> bool same_owner(const A& a,const B& b) {
    return !a.owner_before(b)&&!b.owner_before(a);
}
}
struct BnwSelectedPendingUnitVisualLoan::State {
    const std::thread::id creator=std::this_thread::get_id();
    std::shared_ptr<ui::BnwClientSceneFrame> scene;
    std::shared_ptr<const ui::BnwOwnedCameraGuiScreen> gui;
    const runtime::BnwStockLuaAttemptLease attempt;
    std::optional<runtime::BnwStockLuaAttemptLease::ScopeIdentity> scope;
    std::shared_ptr<const runtime::LiveSessionState> world;
    std::shared_ptr<const ui::BnwClientFrame> frame;
    std::shared_ptr<BnwPendingUnitVisualOwner> receiver;
    std::shared_ptr<const BnwPendingUnitHiddenSlotProof> hidden;
    std::optional<runtime::BnwNativeUnitAllocatorCore::InitialVisibilityOpeningBinding> opening;
    const BnwSelectedPendingUnitVisualLoan* loan=nullptr;
    std::uint64_t scope_serial=0,attempt_serial=0,opening_revision=0;
    bool counted=false,active=false;

    State(const ui::BnwClientSceneFrame& supplied,
          const std::shared_ptr<const ui::BnwOwnedCameraGuiScreen>& supplied_gui,
          const runtime::BnwStockLuaAttemptLease& supplied_attempt)
        :gui(supplied_gui),attempt(supplied_attempt) {
        require(supplied.gui_thread_==creator,"TRAIN Scene loan foreign thread");
        if(supplied.pending_visual_action_) {
            supplied.pending_visual_action_failed_=true;
            throw std::runtime_error("TRAIN Scene loan reentry");
        }
        require(!supplied.pending_visual_action_failed_,"TRAIN Scene loan quarantined");
        scene=supplied.gui_self_.lock();
        require(scene&&scene.get()==&supplied&&same_owner(scene,supplied.gui_self_),
                "TRAIN Scene loan requires canonical selected Scene");
        require(scene->selected_host_&&gui&&gui->thread_==creator&&
                same_owner(gui,gui->self_)&&same_owner(gui,scene->gui_screen_),
                "TRAIN Scene loan GUI is not actual compositor association");
        {
            const auto canonical=gui->self_.lock();
            require(canonical&&canonical.get()==gui.get()&&same_owner(canonical,gui),
                    "TRAIN Scene loan rejects aliased GUI facade");
        }
        require(gui.use_count()>static_cast<long>(gui->pins_+1),
                "TRAIN Scene loan GUI outside owner absent");
        require(static_cast<std::size_t>(scene.use_count())>scene->gui_publication_pins_.load()+1,
                "TRAIN Scene loan Scene outside owner absent");
        receiver=scene->pending_unit_visuals_;
        frame=scene->frame_;
        require(receiver&&frame,"TRAIN Scene loan requires actual renderer receiver/frame");
        require(scene->pending_visual_action_serial_!=std::numeric_limits<std::uint64_t>::max(),
                "TRAIN Scene loan serial exhausted");
        scope_serial=++scene->pending_visual_action_serial_;
        // Both pins are real GUI-backed storage. Existing GUI checks subtract
        // these counts before each mutable Window/provider callback.
        gui->require_origin_lifetime_without_frame_callbacks();
        scope.emplace(attempt.pin_active_scope_without_callbacks());
        require(scope->creator==creator&&scope->control&&
                !scope->control->selected_host.expired()&&
                same_owner(scope->control->selected_host,scene->selected_host_),
                "TRAIN loan actual Attempt selected Host differs");
        ++gui->pins_;++scene->gui_publication_pins_;counted=true;
    }
    void identity() const {
        require(creator==std::this_thread::get_id(),"TRAIN Scene loan creator changed");
        require(active&&loan&&scene->pending_visual_action_==loan&&
                scene->pending_visual_action_serial_==scope_serial&&
                !scene->pending_visual_action_failed_,"TRAIN Scene loan stack retired");
        require(scene->gui_thread_==creator&&same_owner(scene,scene->gui_self_)&&
                static_cast<std::size_t>(scene.use_count())>scene->gui_publication_pins_.load(),
                "TRAIN Scene loan canonical Scene facade retired");
        require(gui->thread_==creator&&same_owner(gui,gui->self_)&&
                gui.use_count()>static_cast<long>(gui->pins_),
                "TRAIN Scene loan canonical GUI facade retired");
        {
            const auto associated=scene->gui_screen_.lock();
            require(associated&&associated.get()==gui.get()&&same_owner(associated,gui),
                    "TRAIN Scene loan current GUI association replaced");
        }
        require(scene->pending_unit_visuals_.get()==receiver.get()&&
                same_owner(scene->pending_unit_visuals_,receiver)&&
                scene->frame_.get()==frame.get()&&same_owner(scene->frame_,frame),
                "TRAIN Scene loan receiver/frame registration replaced");
        require(scene->scene_state_==&scene->session_.state()&&
                scene->scene_state_generation_==scene->session_.state_generation(),
                "TRAIN Scene loan world allocation replaced");
        require(scope.has_value(),"TRAIN loan actual operation scope missing");
        attempt.require_scope_identity_without_callbacks(*scope);
        gui->require_origin_lifetime_without_frame_callbacks();
        if(world) {
            const auto actual=scene->session_.read_state_lease();
            require(actual.get()==world.get()&&same_owner(actual,world)&&scope->world==world.get(),
                    "TRAIN Scene loan Attempt world differs from selected Scene");
        }
    }
    void opening_identity() const {
        require(opening.has_value(),"TRAIN genuine pre-assignment Core opening absent");
        const auto core=opening->core_control.lock();
        require(core&&core.get()==opening->core&&same_owner(core,opening->core_control)&&
                core.use_count()>1,"TRAIN opening Core outside owner retired");
        core->require_initial_visibility_opening_identity_without_callbacks(*opening);
        require(opening->host.get()==scene->selected_host_.get()&&
                same_owner(opening->host,scene->selected_host_)&&
                opening->state.get()==world.get()&&same_owner(opening->state,world)&&
                opening->generation==scene->scene_state_generation_&&
                opening->before_revision==opening_revision,
                "TRAIN Core opening differs from actual Scene world/Host/revision");
        if(hidden)core->require_initial_visibility_hidden_slot_descends_from_opening_without_callbacks(*opening,*hidden);
    }
    void bind_attempt() {
        identity();attempt_serial=attempt.operation_serial();identity();
        require(scope->serial==attempt_serial,"TRAIN actual Attempt serial differs from pure scope");
        world=attempt.world_lease();identity();
        opening_revision=world->ui_source_revision;
    }
    void before_stores() const {
        identity();require(!hidden,"TRAIN pre-store loan already earned hidden proof");
        require(world&&world->ui_source_revision==opening_revision,
                "TRAIN pre-store publication already changed");
        gui->require_same_selected_host(*scene->selected_host_);identity();
        require(scene->current(),"TRAIN pre-store Scene is stale");identity();
        scene->require_publication_finality(frame);identity();
        attempt.require_operation(attempt_serial);identity();
        receiver->require_current();identity();
        require(world->ui_source_revision==opening_revision,
                "TRAIN source callback changed pre-store revision");
    }
    void finality() const {
        identity();
        if(opening)opening_identity();
        if(hidden) receiver->require_action_hidden_slot_proof_current(*hidden);
        else before_stores();
        identity();
    }
    void close() noexcept {
        if(active){scene->pending_visual_action_=nullptr;active=false;loan=nullptr;}
    }
    ~State() {
        close(); // Clear borrowed stack pointer before any canonical pin releases.
        if(counted){--gui->pins_;--scene->gui_publication_pins_;}
    }
};
void BnwSelectedPendingUnitVisualLoan::require_pre_store_current()const {state_.before_stores();}
void BnwSelectedPendingUnitVisualLoan::bind_pre_assignment_core(
    const runtime::BnwNativeUnitAllocatorCore& core)const {
    state_.before_stores();require(!state_.opening,"TRAIN pre-assignment opening already bound");
    auto actual=core.capture_initial_visibility_opening_before_assignment();
    state_.identity();state_.opening.emplace(std::move(actual));
    state_.opening_identity();
    {
        const auto canonical=state_.opening->core_control.lock();
        require(canonical&&canonical.get()==state_.opening->core&&canonical.use_count()>1,
                "TRAIN pre-assignment Core outside owner retired");
        canonical->require_initial_visibility_opening_current_without_callbacks(*state_.opening);
    }
    state_.before_stores();state_.opening_identity();
}
const std::shared_ptr<BnwPendingUnitVisualOwner>&
BnwSelectedPendingUnitVisualLoan::canonical_receiver()const {
    state_.before_stores();return state_.receiver;
}
void BnwSelectedPendingUnitVisualLoan::require_hidden_slot_proof(
    const runtime::BnwNativeUnitAllocatorCore& core,const runtime::BnwUnitMovePending& pending)const {
    state_.identity();state_.opening_identity();
    require(state_.opening->core==&core,"TRAIN hidden proof supplied Core differs from opening");
    require(!state_.hidden,"TRAIN hidden proof already adopted");
    auto proof=state_.receiver->require_action_hidden_slot_proof(core,pending);
    state_.identity();state_.receiver->require_action_hidden_slot_proof_current(*proof);
    state_.identity();state_.hidden=std::move(proof);state_.opening_identity();
}
} // namespace civ5::graphics

namespace civ5::ui {
void BnwClientSceneFrame::with_actual_pending_unit_visual_loan(
    const std::shared_ptr<const BnwOwnedCameraGuiScreen>& gui,
    const runtime::BnwStockLuaAttemptLease& attempt,
    const std::function<void(const graphics::BnwSelectedPendingUnitVisualLoan&)>& incoming)const {
    using Loan=graphics::BnwSelectedPendingUnitVisualLoan;
    Loan::State state(*this,gui,attempt);Loan loan(state);
    state.loan=&loan;state.active=true;pending_visual_action_=&loan;
    std::optional<std::function<void(const Loan&)>> target;
    try {
        state.identity();
        // All borrowed application target storage is copied before first real
        // GUI/Scene/Attempt/provider invocation, under canonical counted pins.
        target.emplace(incoming);state.identity();
        if(!*target)throw std::invalid_argument("TRAIN Scene loan target absent");
        state.bind_attempt();state.before_stores();
        (*target)(loan);state.finality();
        target.reset();state.finality();state.close();
    }catch(...) {
        pending_visual_action_failed_=true;
        target.reset(); // Actual copied target dies inside genuine active scope.
        state.close();throw;
    }
}
} // namespace civ5::ui
