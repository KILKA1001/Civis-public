#include "civ5/graphics/bnw_train_presented_frame_proof.hpp"
#include "civ5/ui/bnw_train_renderer_refresh_receipt.hpp"
#include "civ5/ui/bnw_owned_camera_gui_screen.hpp"
#include "civ5/ui/bnw_client_scene_frame.hpp"
#include <limits>
#include <optional>
#include <stdexcept>
#include <thread>
namespace civ5::graphics {
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
template<class A,class B>bool same_control(const A&a,const B&b){return !a.owner_before(b)&&!b.owner_before(a);}
}
struct BnwTrainPresentedFrameProof::State {
    const std::thread::id creator=std::this_thread::get_id();
    std::shared_ptr<const ui::BnwTrainRendererRefreshReceipt> receipt;
    std::shared_ptr<const ui::BnwOwnedCameraGuiScreen> gui;
    ui::BnwTrainRendererRefreshReceipt::RendererOrigin origin;
    const BnwTrainPresentedFrameProof* proof=nullptr;
    std::uint64_t serial=0;
    bool counted=false,active=false,failed=false;
    State(const std::shared_ptr<const ui::BnwTrainRendererRefreshReceipt>& supplied,
          const std::shared_ptr<const ui::BnwOwnedCameraGuiScreen>& incoming) {
        need(supplied&&incoming,"TRAIN presentation receipt/GUI absent");
        need(supplied->creator_==creator&&incoming->thread_==creator,"TRAIN presentation foreign creator");
        receipt=supplied->retain_factory_owner();
        need(receipt.get()==supplied.get()&&same_control(receipt,supplied),"TRAIN presentation receipt alias");
        gui=incoming;
        {
            const auto canonical=gui->self_.lock();
            need(canonical&&canonical.get()==gui.get()&&same_control(canonical,gui),"TRAIN presentation GUI alias");
        }
        if(gui->train_presented_scope_){gui->train_presented_failed_=true;throw std::runtime_error("TRAIN presented scope reentry");}
        need(!gui->train_presented_failed_,"TRAIN presented scope failed");
        need(gui->train_presented_serial_!=std::numeric_limits<std::uint64_t>::max(),"TRAIN presentation scope serial exhausted");
        // Pure immutable storage snapshot precedes any application target copy.
        origin=receipt->origin_;
        need(origin.scene&&origin.host&&origin.state&&origin.opening_gui&&origin.core&&origin.pending,
             "TRAIN presentation actual origin absent");
        need(origin.scene->gui_thread_==creator,"TRAIN presentation Scene foreign creator");
        serial=++gui->train_presented_serial_;
        ++gui->pins_;++origin.scene->gui_publication_pins_;counted=true;
    }
    void pure()const {
        need(creator==std::this_thread::get_id(),"TRAIN presented scope creator changed");
        need(active&&!failed&&proof&&gui->train_presented_scope_==proof&&
             gui->train_presented_serial_==serial&&!gui->train_presented_failed_,"TRAIN presented scope retired");
        need(receipt->creator_==creator&&!receipt->invalidated_.load()&&!receipt->factory_self_.expired()&&
             same_control(receipt,receipt->factory_self_)&&receipt.use_count()>1,
             "TRAIN presentation receipt factory/outside owner retired");
        need(gui->thread_==creator&&!gui->self_.expired()&&same_control(gui,gui->self_)&&
             gui.use_count()>static_cast<long>(gui->pins_),"TRAIN presentation GUI outside owner retired");
        const auto& actual=receipt->origin_;
        need(actual.scene.get()==origin.scene.get()&&same_control(actual.scene,origin.scene)&&
             actual.host.get()==origin.host.get()&&same_control(actual.host,origin.host)&&
             actual.opening_gui.get()==origin.opening_gui.get()&&same_control(actual.opening_gui,origin.opening_gui)&&
             actual.core.get()==origin.core.get()&&same_control(actual.core,origin.core)&&
             actual.pending.get()==origin.pending.get()&&same_control(actual.pending,origin.pending)&&
             actual.state.get()==origin.state.get()&&same_control(actual.state,origin.state)&&
             actual.generation==origin.generation&&actual.operation_serial==origin.operation_serial&&
             actual.earned_revision==origin.earned_revision,"TRAIN presentation immutable receipt origin replaced");
    }
    void current(bool consuming=false)const {
        pure();
        if(consuming)receipt->require_presented_scope_without_callbacks();
        else need(!receipt->presented_,"TRAIN presented proof already consumed");
        gui->require_train_presented_origin(*receipt);pure();
        if(consuming)receipt->require_presented_scope_without_callbacks();
    }
    void close()noexcept {
        if(active){gui->train_presented_scope_=nullptr;active=false;proof=nullptr;}
    }
    ~State(){close();if(counted){--gui->pins_;--origin.scene->gui_publication_pins_;}}
};
void BnwTrainPresentedFrameProof::require_current_for_train(const ui::BnwTrainRendererRefreshReceipt& supplied)const {
    state_.pure();need(&supplied==state_.receipt.get(),"TRAIN presented proof wrong receipt");state_.current();
}
std::shared_ptr<const ui::BnwOwnedCameraGuiScreen>
BnwTrainPresentedFrameProof::canonical_gui_for_train(const ui::BnwTrainRendererRefreshReceipt& receipt)const {
    require_current_for_train(receipt);return state_.gui;
}
void BnwTrainPresentedFrameProof::with_actual_presented_frame(
    const std::shared_ptr<const ui::BnwTrainRendererRefreshReceipt>& receipt,
    const std::shared_ptr<const ui::BnwOwnedCameraGuiScreen>& gui,
    const std::function<void(const BnwTrainPresentedFrameProof&)>& incoming) {
    // Authenticate the actual factory before protecting constructor failures.
    // This weak control adds no outside receipt pin during application callbacks.
    need(receipt&&receipt->creator_==std::this_thread::get_id(),"TRAIN presentation receipt creator absent");
    const auto actual_address=receipt.get();
    const auto factory=receipt->factory_self_;
    {
        const auto canonical=factory.lock();
        need(canonical&&canonical.get()==actual_address&&same_control(canonical,receipt),
             "TRAIN presentation receipt substituted factory");
    }
    State state=[&]()->State {
        try{return State(receipt,gui);}
        catch(...){
            const auto canonical=factory.lock();
            if(canonical&&canonical.get()==actual_address)
                canonical->quarantine_presented_scope_without_callbacks();
            throw;
        }
    }();
    BnwTrainPresentedFrameProof proof(state);
    state.proof=&proof;state.active=true;state.gui->train_presented_scope_=&proof;
    std::optional<std::function<void(const BnwTrainPresentedFrameProof&)>> target;
    try {
        state.pure();target.emplace(incoming);state.pure();
        need(bool(*target),"TRAIN after-present target absent");
        {
            const auto borrowed=state.receipt->borrow_renderer_origin();
            state.pure();
            need(borrowed.scene.get()==state.origin.scene.get()&&same_control(borrowed.scene,state.origin.scene)&&
                 borrowed.state.get()==state.origin.state.get()&&same_control(borrowed.state,state.origin.state)&&
                 borrowed.core.get()==state.origin.core.get()&&same_control(borrowed.core,state.origin.core)&&
                 borrowed.pending.get()==state.origin.pending.get()&&same_control(borrowed.pending,state.origin.pending)&&
                 borrowed.generation==state.origin.generation&&borrowed.operation_serial==state.origin.operation_serial&&
                 borrowed.earned_revision==state.origin.earned_revision,"TRAIN actual borrowed origin changed");
        }
        state.current();
        (*target)(proof);state.current(true);
        target.reset();state.current(true);
        // Actual copied target is gone; Source performs its last pure check
        // before clearing the barrier. No provider may run after this release.
        state.receipt->finish_presented_scope_after_target();
        state.pure();state.close();
    }catch(...){
        state.failed=true;state.gui->train_presented_failed_=true;
        target.reset(); // Real target destruction precedes canonical loan teardown.
        state.receipt->quarantine_presented_scope_without_callbacks();
        state.close();throw;
    }
}
}
