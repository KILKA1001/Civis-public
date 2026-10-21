#include "civ5/runtime/bnw_selected_unitflag_context_owner.hpp"
#include "civ5/runtime/bnw_selected_worldview_context_owner.hpp"
#include "civ5/runtime/bnw_worldview_script_ui_adapter.hpp"
#include "civ5/runtime/script_ui_control_graph.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/ui/bnw_worldview_control_effects.hpp"
#include <atomic>
#include <exception>
#include <set>
#include <string>
#include <thread>
#include <utility>
namespace civ5::runtime {
namespace {
void require(bool value,const char* why){if(!value)throw ScriptError(why);}
template<class A,class B> bool same_owner(const A& a,const B& b){
    return !a.owner_before(b)&&!b.owner_before(a);
}
}
struct BnwSelectedUnitFlagContextOwner::State {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<BnwSelectedWorldViewContextOwner> owner;
    BnwSelectedWorldViewContextOwner* const owner_address;
    const std::shared_ptr<const ScriptUiChildSource> source;
    const std::shared_ptr<ui::BnwWorldViewControlTree> tree;
    const LuaScriptHost* const vm;
    std::shared_ptr<ScriptUiControlGraph> graph;
    std::weak_ptr<BnwSelectedUnitFlagContextOwner> self;
    mutable std::atomic<bool> alive{true},failed{false};
    mutable bool busy=false;
    struct Frame {const BnwWorldViewSourceOperationLoan* loan;std::uint64_t serial;};
    State(std::weak_ptr<BnwSelectedWorldViewContextOwner> actual,
          BnwSelectedWorldViewContextOwner* address,
          std::shared_ptr<const ScriptUiChildSource> descriptor,
          std::shared_ptr<ui::BnwWorldViewControlTree> existing,LuaScriptHost& host)
      :owner(std::move(actual)),owner_address(address),source(std::move(descriptor)),
       tree(std::move(existing)),vm(&host){}
    void fail()const noexcept {failed.store(true);tree->poison();}
    void pure()const {
        if(creator!=std::this_thread::get_id()){
            fail();throw ScriptError("UnitFlag context owner foreign thread");
        }
        require(alive.load()&&!failed.load(),"UnitFlag context owner retired or poisoned");
        // These checker locks end before any provider/VM/native callback.
        {const auto actual=self.lock();
         require(actual&&actual->state_.get()==this&&actual.use_count()>1,
                 "UnitFlag context facade/control or outside lifetime absent");}
        {const auto actual=owner.lock();
         require(actual&&actual.get()==owner_address&&same_owner(actual,owner),
                 "UnitFlag actual metadata owner/control absent");}
        require(source&&tree&&vm,"UnitFlag actual descriptor/tree/VM slice absent");
    }
    const BnwWorldViewSourceOperationLoan& active(const Frame& frame)const {
        pure();
        // Tree/its private Adapter retain this actual metadata owner. The short
        // weak checker adds no strong owner pin across source validation.
        // Fetch current active loan BEFORE dereferencing any old stack address.
        const auto& loan=owner_address->current_operation_loan();
        require(&loan==frame.loan,"UnitFlag native operation loan substituted");
        loan.require_operation(frame.serial);
        pure();
        const auto& after=owner_address->current_operation_loan();
        require(&after==frame.loan,"UnitFlag operation retired during validation");
        return after;
    }
    Frame capture()const {
        pure();
        const auto& loan=owner_address->current_operation_loan();
        const Frame frame{&loan,loan.operation_serial()};
        active(frame);return frame;
    }
    void validate(const Frame& frame)const {
        // Native loan owns SAME Bootstrap/VM, Host/world/generation and the real
        // GUI/window borrow. This raw VM address is compared only after that
        // genuine loan's currentness; it never grants VM lifetime by itself.
        active(frame).require_same_vm(*vm);active(frame).require_same_source(*source);
        owner_address->require_current("UnitFlagManager",source.get());active(frame);
        source->require_current();active(frame);
        tree->require_current();active(frame);
        const auto actual_source=tree->actual_source();active(frame);
        require(actual_source.get()==source.get()&&same_owner(actual_source,source),
                "UnitFlag tree/source descriptor or control substituted");
        const auto source_context=source->context();active(frame);
        const auto tree_context=tree->context();active(frame);
        require(source_context=="UnitFlagManager"&&tree_context=="UnitFlagManager",
                "UnitFlag original source/tree context substituted");
        if(graph){
            require(graph->context()=="UnitFlagManager","UnitFlag graph context substituted");
            graph->require_current();active(frame);
        }
        active(frame).require_same_vm(*vm);active(frame).require_same_source(*source);
        active(frame);pure();
    }
    void require_supplied(const Frame& frame,const BnwWorldViewSourceOperationLoan& supplied,
                          const LuaScriptHost& host)const {
        require(&supplied==frame.loan && &host==vm,"UnitFlag caller has another native loan/VM");
        active(frame).require_same_vm(host);active(frame);
    }
    template<class F> auto guarded(F&& operation)const {
        pure();
        if(busy){fail();throw ScriptError("UnitFlag context owner reentered");}
        busy=true;struct Exit{bool& busy;~Exit(){busy=false;}} exit{busy};
        try {
            const auto frame=capture();validate(frame);
            auto result=operation(frame);validate(frame);return result;
        }catch(...){fail();throw;}
    }
    void static_members(const Frame& frame)const {
        const auto controls=tree->static_controls();active(frame);
        const std::set<std::string,std::less<>> expected{
            "AirCraftFlags","CivilianFlags","MilitaryFlags","GarrisonFlags","CityContainer"};
        require(controls.size()==expected.size(),"UnitFlag original static container set differs");
        std::set<std::string,std::less<>> found;
        for(const auto& item:controls){
            const auto quote=tree->inspect_control(item.second);active(frame);
            require(expected.contains(item.first)&&found.insert(item.first).second&&
                    quote.alive&&!quote.dynamic&&quote.parent==0&&
                    quote.authored.name=="Container"&&quote.authored.attribute("ID")==item.first,
                    "UnitFlag original direct static container identity differs");
        }
        // NewUnitFlag/CityFlag/AirButton remain original Tree templates.
        // Only the original Lua's actual BuildInstanceForControl may create them.
        // ../SelectedUnitContainer belongs to the genuine InGame sibling owner;
        // it must not be manufactured in this local static graph.
    }
};
BnwSelectedUnitFlagContextOwner::BnwSelectedUnitFlagContextOwner(std::shared_ptr<State> state)
  :state_(std::move(state)){}
BnwSelectedUnitFlagContextOwner::~BnwSelectedUnitFlagContextOwner(){
    const auto state=state_;
    if(state->creator!=std::this_thread::get_id()){state->fail();std::terminate();}
    state->alive.store(false);
    if(state->graph){try{state->graph->retire();}catch(...){state->fail();std::terminate();}}
}
std::shared_ptr<BnwSelectedUnitFlagContextOwner> BnwSelectedUnitFlagContextOwner::build_owned(
    std::weak_ptr<BnwSelectedWorldViewContextOwner> owner,
    std::shared_ptr<const ScriptUiChildSource> source,
    std::shared_ptr<ui::BnwWorldViewControlTree> tree,LuaScriptHost& vm,
    std::vector<std::int64_t> mouse){
    require(source&&tree,"UnitFlag authentic descriptor/tree absent");
    BnwSelectedWorldViewContextOwner* address=nullptr;
    {const auto actual=owner.lock();require(static_cast<bool>(actual),"UnitFlag metadata owner absent");
     address=actual.get();}
    // This private factory is entered only by Bootstrap inside the real native
    // issuer. Existing Tree owns its canonical metadata owner; no new root,
    // descriptor, VM, XML graph or duplicate factory is minted here.
    auto state=std::make_shared<State>(std::move(owner),address,source,tree,vm);
    auto result=std::shared_ptr<BnwSelectedUnitFlagContextOwner>(
        new BnwSelectedUnitFlagContextOwner(state));
    state->self=result;
    state->guarded([&](const auto& frame){
        state->static_members(frame);state->validate(frame);
        auto graph=BnwWorldViewScriptUiAdapter::build_owned(tree,source,vm,std::move(mouse));
        state->active(frame);state->validate(frame);
        require(graph&&graph->context()=="UnitFlagManager","UnitFlag actual backend graph absent");
        state->graph=std::move(graph);state->validate(frame);return 0;
    });
    return result;
}
#include "bnw_unitflag_health_context_origin.inc"
void BnwSelectedUnitFlagContextOwner::require_current()const {
    const auto state=state_;state->guarded([](auto){return 0;});
}
BnwSelectedUnitFlagContextOwner::HostBindings
BnwSelectedUnitFlagContextOwner::bindings_for_host_owned(
    const BnwWorldViewSourceOperationLoan& loan,const LuaScriptHost& vm)const {
    const auto state=state_;
    return state->guarded([&](const auto& frame){
        state->require_supplied(frame,loan,vm);
        require(static_cast<bool>(state->graph),"UnitFlag private actual graph not built");
        return HostBindings{state->source,state->graph};
    });
}
void BnwSelectedUnitFlagContextOwner::require_host_operation_owned(
    const BnwWorldViewSourceOperationLoan& loan,const LuaScriptHost& vm)const {
    const auto state=state_;
    state->guarded([&](const auto& frame){state->require_supplied(frame,loan,vm);return 0;});
}
}
