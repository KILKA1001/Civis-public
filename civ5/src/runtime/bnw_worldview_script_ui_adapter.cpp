#include "civ5/runtime/bnw_worldview_script_ui_adapter.hpp"
#include "civ5/runtime/script_ui_control_graph.hpp"
#include "civ5/runtime/bnw_worldview_native_context_issuer.hpp"
#include "civ5/runtime/bnw_unitflag_health_instance_loan.hpp"
#include <algorithm>
#include <atomic>
#include <map>
#include <set>
#include <utility>
#include <limits>
#include <optional>
#include <type_traits>
#include <exception>
namespace civ5::runtime {
namespace {
void require(bool value,const char* why){if(!value)throw ScriptError(why);}
}
struct BnwWorldViewScriptUiAdapter::State {
    using Control=ui::BnwWorldViewControlTree::Control;
    const std::thread::id creator=std::this_thread::get_id();
    const std::shared_ptr<ui::BnwWorldViewControlTree> tree;
    const std::shared_ptr<const ScriptUiChildSource> source;
    const LuaScriptHost* const vm;
    const std::shared_ptr<BnwSelectedWorldViewContextOwner> source_owner;
    const std::string context;
    const std::vector<std::int64_t> events;
    std::map<std::uint64_t,Control> controls;
    std::vector<ScriptUiGraphBackendNamed> statics;
    static constexpr std::uint64_t selected_alias=std::numeric_limits<std::uint64_t>::max();
    std::weak_ptr<ui::BnwWorldViewControlTree> selected_tree;
    std::optional<Control> selected_control;
    bool selected_bound=false;
    std::map<std::pair<std::uint64_t,std::int64_t>,std::shared_ptr<const ScriptUiOwnedControlCallback>> callbacks;
    struct UnitFlagButtonOperands {std::uint64_t instance=0;std::optional<std::int32_t> owner,full_id;};
    std::map<std::uint64_t,UnitFlagButtonOperands> unitflag_button_operands;
    mutable std::atomic<bool> failed{false};
    mutable bool busy=false;bool retired=false,dispatch_active=false;
    std::atomic<bool> facade_alive{true};
    struct Frame {
        const BnwWorldViewSourceOperationLoan* loan;
        std::uint64_t serial;
        bool operator==(const Frame&)const=default;
    };
    // Actual canonical adapter button state above; no shadow widget/renderer.
    const Control& unitflag_button(std::uint64_t id)const {
        const auto& handle=control(id);const auto node=tree->inspect_control(handle);const auto name=node.authored.attribute("ID");
        require(context=="UnitFlagManager"&&node.dynamic&&node.alive&&node.instance!=0&&
            node.authored.name=="Button"&&(name=="NormalButton"||name=="HealthBarButton"),
            "UnitFlag Void target is not an actual original dynamic flag button");
        return handle;
    }
    std::array<ScriptValue,2> unitflag_voids(std::uint64_t id,const Frame& frame)const {
        (void)unitflag_button(id);const auto node=tree->inspect_control(control(id));
        const auto found=unitflag_button_operands.find(id);
        require(found!=unitflag_button_operands.end()&&found->second.instance==node.instance&&
            found->second.owner.has_value()&&found->second.full_id.has_value(),
            "UnitFlag button requires genuine prior SetVoid1/SetVoid2 stores");
        vm->require_unitflag_button_unit_owned(loan(frame),*found->second.owner,*found->second.full_id);
        validate(frame);return {ScriptValue{std::int64_t{*found->second.owner}},ScriptValue{std::int64_t{*found->second.full_id}}};
    }
    State(std::shared_ptr<ui::BnwWorldViewControlTree> actual,
          std::shared_ptr<const ScriptUiChildSource> descriptor,const LuaScriptHost& host,
          std::shared_ptr<BnwSelectedWorldViewContextOwner> metadata_owner,
          std::string name,std::vector<std::int64_t> mouse):tree(std::move(actual)),source(std::move(descriptor)),
          vm(&host),source_owner(std::move(metadata_owner)),context(std::move(name)),events(std::move(mouse)){}
    void fail()const noexcept {failed.store(true);tree->poison();}
    void entry()const {
        if(creator!=std::this_thread::get_id()){fail();throw ScriptError("WorldView graph adapter foreign thread");}
        if(failed.load()||retired||busy){fail();throw ScriptError("WorldView graph adapter failed, retired or reentered");}
    }
    const BnwWorldViewSourceOperationLoan& loan(const Frame& frame)const {
        // Fetch active reference BEFORE dereferencing any prior stack address.
        // Owner forbids replacing/reentering a live loan and seals its scope.
        const auto& actual=source_owner->current_operation_loan();
        require(&actual==frame.loan,"WorldView graph active source loan substituted");
        actual.require_operation(frame.serial);return actual;
    }
    void validate(const Frame& frame)const {
        loan(frame).require_same_vm(*vm);loan(frame).require_same_source(*source);
        source->require_current();loan(frame).require_operation(frame.serial);
        tree->require_current();loan(frame).require_operation(frame.serial);
        const auto actual=tree->actual_source();
        require(actual.get()==source.get()&&!actual.owner_before(source)&&!source.owner_before(actual),
                "WorldView graph/tree source descriptor control substituted");
        require(source->context()==context&&tree->context()==context,"WorldView graph/source context substituted");
        loan(frame).require_same_vm(*vm);loan(frame).require_same_source(*source);
        if(selected_bound){
            {
            const auto target=selected_tree.lock();require(target&&selected_control,"WorldView selected sibling parent retired");
            target->require_current();loan(frame).require_operation(frame.serial);
            const auto metadata=target->metadata_source_owner();
            require(metadata.get()==source_owner.get()&&!metadata.owner_before(source_owner)&&!source_owner.owner_before(metadata)&&
                target->context()=="InGame","WorldView selected sibling source control substituted");
            const auto quote=target->inspect_control(*selected_control);
            require(quote.alive&&!quote.dynamic&&quote.authored.name=="Container"&&quote.authored.attribute("ID")=="SelectedUnitContainer",
                "WorldView selected sibling control substituted");
            loan(frame).require_same_vm(*vm);
            } // Drop every temporary selected-Tree/source pin before acceptance.
            require(!selected_tree.expired(),"WorldView selected sibling parent retired during validation");
        }
        // No callbackful validation follows this final weak parent fence.
        require(!failed.load()&&!retired,"WorldView graph poisoned during owner validation");
    }
    template<class F> auto guarded(F&& operation)const {
        entry();busy=true;struct Exit{bool& busy;~Exit(){busy=false;}} exit{busy};
        try {
            const auto& actual=source_owner->current_operation_loan();
            const Frame frame{&actual,actual.operation_serial()};validate(frame);
            auto result=operation(frame);validate(frame);return result;
        }catch(...){fail();throw;}
    }
    void render_fail()const noexcept {
        failed.store(true);
        // No mutable/borrowed backend access on a foreign creator.
        if(creator==std::this_thread::get_id())tree->poison();
    }
    void render_first(bool expected_busy)const {
        if(creator!=std::this_thread::get_id()){
            failed.store(true);throw ScriptError("Render adapter foreign creator");
        }
        require(facade_alive.load()&&!failed.load()&&!retired&&busy==expected_busy,
            "Render adapter facade/thread/failure/reentry changed before borrow");
    }
    void render_pure(const Frame& frame,bool expected_busy=true)const {
        render_first(expected_busy);
        const auto& actual=source_owner->current_operation_loan();render_first(expected_busy);
        require(&actual==frame.loan,"Render adapter active Source loan substituted");
        const auto serial=actual.operation_serial();render_first(expected_busy);
        require(serial==frame.serial,"Render adapter Source serial substituted");
        actual.require_render_operation_owned();render_first(expected_busy);
    }
    template<class F> decltype(auto) render_call(const Frame& frame,F&& operation,
                                               bool expected_busy=true)const {
        render_pure(frame,expected_busy);
        try{
            if constexpr(std::is_void_v<decltype(operation())>){
                operation();render_pure(frame,expected_busy);
            }else{
                decltype(auto) result=operation();render_pure(frame,expected_busy);return result;
            }
        }catch(...){
            const auto original=std::current_exception();
            try{render_first(expected_busy);}catch(...){render_fail();}
            render_fail();std::rethrow_exception(original);
        }
    }
    const BnwWorldViewSourceOperationLoan& render_loan(const Frame& frame)const {
        render_pure(frame);
        const auto& actual=source_owner->current_operation_loan();render_first(true);
        render_call(frame,[&]{actual.require_operation(frame.serial);});return actual;
    }
    void render_full(const Frame& frame)const {
        render_loan(frame).require_same_vm(*vm);render_pure(frame);
        render_loan(frame).require_same_source(*source);render_pure(frame);
        render_call(frame,[&]{source->require_current();});
        render_loan(frame).require_operation(frame.serial);render_pure(frame);
        render_call(frame,[&]{tree->require_current();});
        render_loan(frame).require_operation(frame.serial);render_pure(frame);
        const auto actual=render_call(frame,[&]{return tree->actual_source();});
        require(actual.get()==source.get()&&!actual.owner_before(source)&&!source.owner_before(actual),
                "WorldView graph/tree source descriptor control substituted");
        const auto source_context=render_call(frame,[&]{return source->context();});
        const auto tree_context=render_call(frame,[&]{return tree->context();});
        require(source_context==context&&tree_context==context,"WorldView graph/source context substituted");
        render_loan(frame).require_same_vm(*vm);render_pure(frame);
        render_loan(frame).require_same_source(*source);render_pure(frame);
        if(selected_bound){
            {
            const auto target=selected_tree.lock();require(target&&selected_control,"WorldView selected sibling parent retired");
            render_call(frame,[&]{target->require_current();});
            render_loan(frame).require_operation(frame.serial);render_pure(frame);
            const auto metadata=render_call(frame,[&]{return target->metadata_source_owner();});
            const auto target_context=render_call(frame,[&]{return target->context();});
            require(metadata.get()==source_owner.get()&&!metadata.owner_before(source_owner)&&!source_owner.owner_before(metadata)&&
                target_context=="InGame","WorldView selected sibling source control substituted");
            const auto quote=render_call(frame,[&]{return target->inspect_control(*selected_control);});
            require(quote.alive&&!quote.dynamic&&quote.authored.name=="Container"&&quote.authored.attribute("ID")=="SelectedUnitContainer",
                "WorldView selected sibling control substituted");
            render_loan(frame).require_same_vm(*vm);render_pure(frame);
            }
            require(!selected_tree.expired(),"WorldView selected sibling parent retired during validation");
        }
        render_pure(frame);
    }
    const Control& render_control(const Frame& frame,std::uint64_t id)const {
        render_pure(frame);
        const auto found=controls.find(id);require(found!=controls.end(),"WorldView graph handle not issued by this owner");
        const auto quote=render_call(frame,[&]{return tree->inspect_control(found->second);});
        require(quote.id==id&&quote.alive,"WorldView graph handle removed or substituted");
        render_full(frame);return found->second;
    }
    template<class F> decltype(auto) render_operation_call(const Frame& frame,F&& operation)const {
        render_full(frame);
        if constexpr(std::is_void_v<decltype(operation())>){
            render_call(frame,std::forward<F>(operation));render_full(frame);
        }else{
            decltype(auto) result=render_call(frame,std::forward<F>(operation));
            render_full(frame);return result;
        }
    }
    void render_callback_current(const ScriptUiOwnedControlCallback& callback,const Frame& frame)const {
        const auto& host=render_call(frame,[&]() -> const LuaScriptHost& {return callback.host();});
        const auto& descriptor=render_call(frame,[&]() -> const ScriptUiChildSource& {return callback.source();});
        require(&host==vm&&&descriptor==source.get(),"WorldView callback belongs to another VM/source descriptor");
        render_call(frame,[&]{callback.require_current();});render_full(frame);
        const auto& final_host=render_call(frame,[&]() -> const LuaScriptHost& {return callback.host();});
        const auto& final_descriptor=render_call(frame,[&]() -> const ScriptUiChildSource& {return callback.source();});
        require(&final_host==vm&&&final_descriptor==source.get(),"WorldView callback changed owner after validation");
    }
    template<class F> auto guarded_render(F&& operation)const {
        try{
            render_first(false);
            const auto& actual=source_owner->current_operation_loan();render_first(false);
            actual.require_render_operation_owned();render_first(false);
            busy=true;struct Exit{bool& busy;~Exit(){busy=false;}}exit{busy};
            const Frame frame{&actual,actual.operation_serial()};render_pure(frame);
            render_full(frame);
            auto result=operation(frame);render_pure(frame);render_full(frame);return result;
        }catch(...){
            const auto original=std::current_exception();
            // Exit has restored busy. Never consult facade/backend first.
            try{render_first(false);}catch(...){render_fail();}
            render_fail();std::rethrow_exception(original);
        }
    }
    bool health_operation()const {
        try{
            render_first(false);const auto& active=source_owner->current_operation_loan();render_first(false);
            const auto result=active.unitflag_health_operation_without_callbacks();render_first(false);return result;
        }catch(...){render_fail();throw;}
    }
    template<class F> auto guarded_health(std::uint64_t id,BnwUnitFlagHealthCommand command,F&& operation)const {
        try{
            render_first(false);const auto found=controls.find(id);
            require(found!=controls.end(),"UnitFlag health handle not originally issued by this adapter");
            const auto control=found->second;
            busy=true;struct Exit{bool&v;~Exit(){v=false;}}exit{busy};
            render_first(true);const auto& actual=source_owner->current_operation_loan();render_first(true);
            const Frame frame{&actual,actual.operation_serial()};render_first(true);
            const auto pure=[&]{
                render_first(true);const auto& active=source_owner->current_operation_loan();render_first(true);
                require(&active==frame.loan,"UnitFlag health adapter active Source replaced");
                require(active.operation_serial()==frame.serial,"UnitFlag health adapter serial replaced");render_first(true);
                active.require_unitflag_health_control_owned(control,command);render_first(true);
                tree->require_unitflag_health_backend_origin_without_callbacks(control,active,command);render_first(true);
            };
            const auto full=[&]{
                pure();actual.require_same_vm(*vm);render_first(true);pure();
                actual.require_same_source(*source);render_first(true);pure();
                actual.require_operation(frame.serial);render_first(true);pure();
            };
            full();auto result=operation(control,actual);render_first(true);pure();full();return result;
        }catch(...){render_fail();throw;}
    }
    const Control& control(std::uint64_t id)const {
        const auto found=controls.find(id);require(found!=controls.end(),"WorldView graph handle not issued by this owner");
        const auto quote=tree->inspect_control(found->second);
        require(quote.id==id&&quote.alive,"WorldView graph handle removed or substituted");return found->second;
    }
    void layout_first()const {
        require(facade_alive.load()&&creator==std::this_thread::get_id()&&!failed.load()&&!retired&&busy,
            "Layout adapter facade/thread/failure/reentry changed before borrow");
    }
    void layout_pure(const Frame& frame,const Control& actual_control)const {
        layout_first();
        const auto& actual=source_owner->current_operation_loan();layout_first();
        require(&actual==frame.loan&&actual.operation_serial()==frame.serial,
            "Layout adapter SAME active Source operation substituted");
        actual.require_render_operation_owned();layout_first();
        tree->require_layout_backend_origin_without_callbacks(actual_control,actual);layout_first();
        // Stored context is immutable; the closed Tree pure guard above
        // already proves actual NotificationPanel/control/operation identity.
        // Actual descriptor/source checks remain in layout_full, each with
        // an immediate saved-State FIRST before borrowed fields are reused.
        require(context=="NotificationPanel",
            "Layout adapter actual selected context substituted");
    }
    void layout_full(const Frame& frame,const Control& control)const {
        layout_pure(frame,control);
        frame.loan->require_same_vm(*vm);layout_pure(frame,control);
        frame.loan->require_same_source(*source);layout_pure(frame,control);
        source->require_current();layout_pure(frame,control);
        tree->require_current();layout_pure(frame,control);
        const auto actual=tree->actual_source();layout_pure(frame,control);
        require(actual.get()==source.get()&&!actual.owner_before(source)&&!source.owner_before(actual),
            "Layout adapter canonical descriptor changed");
    }
    template<class F> auto layout_guarded(std::uint64_t id,F&& operation)const {
        entry();require(facade_alive.load(),"Layout adapter facade retired");
        const auto found=controls.find(id);require(found!=controls.end(),"Layout canonical control was not issued");
        const auto actual_control=found->second;
        busy=true;struct Exit{bool& v;~Exit(){v=false;}}exit{busy};
        try {
            const auto& active=source_owner->current_operation_loan();layout_first();
            const Frame frame{&active,active.operation_serial()};layout_pure(frame,actual_control);
            layout_full(frame,actual_control);
            auto result=operation(frame,actual_control);layout_pure(frame,actual_control);
            layout_full(frame,actual_control);return result;
        }catch(...){fail();throw;}
    }
    void consume_first()const {
        if(creator!=std::this_thread::get_id()){failed.store(true);throw ScriptError("ConsumeMouseOver foreign creator");}
        require(facade_alive.load()&&!failed.load()&&!retired&&busy,"ConsumeMouseOver saved facade retired/failed/reentered");
    }
    void consume_pure(const Frame& frame,const Control& target)const {
        consume_first();const auto& actual=source_owner->current_operation_loan();consume_first();
        require(&actual==frame.loan&&actual.operation_serial()==frame.serial,"ConsumeMouseOver SAME source occurrence changed");
        actual.require_render_operation_owned();consume_first();
        tree->require_mouse_consume_backend_origin_without_callbacks(target,actual);consume_first();
        require(context=="UnitFlagManager","ConsumeMouseOver retained context differs");
    }
    void consume_full(const Frame& frame,const Control& target)const {
        consume_pure(frame,target);
        frame.loan->require_same_vm(*vm);consume_first();consume_pure(frame,target);
        frame.loan->require_same_source(*source);consume_first();consume_pure(frame,target);
        source->require_current();consume_first();consume_pure(frame,target);
        tree->require_current();consume_first();consume_pure(frame,target);
        const auto actual=tree->actual_source();consume_first();consume_pure(frame,target);
        require(actual.get()==source.get()&&!actual.owner_before(source)&&!source.owner_before(actual),"ConsumeMouseOver descriptor control changed");
    }
    void consume_owned(std::uint64_t id,bool value)const {
        if(creator!=std::this_thread::get_id()){failed.store(true);throw ScriptError("ConsumeMouseOver foreign creator before mutable borrow");}
        try {
            require(facade_alive.load()&&!failed.load()&&!retired&&!busy,"ConsumeMouseOver entry retired/failed/reentered");
            const auto found=controls.find(id);require(found!=controls.end(),"ConsumeMouseOver handle not issued by actual adapter");
            const auto target=found->second;busy=true;struct Exit{bool& v;~Exit(){v=false;}}exit{busy};
            const auto& actual=source_owner->current_operation_loan();consume_first();
            const Frame frame{&actual,actual.operation_serial()};consume_pure(frame,target);consume_full(frame,target);
            tree->set_consume_mouse_over_owned(target,value,actual);consume_first();consume_pure(frame,target);
            consume_full(frame,target);
        }catch(...){failed.store(true);throw;}
    }
    bool event(std::int64_t value)const {return std::find(events.begin(),events.end(),value)!=events.end();}
    void callback_current(const ScriptUiOwnedControlCallback& callback,const Frame& frame)const {
        require(&callback.host()==vm && &callback.source()==source.get(),"WorldView callback belongs to another VM/source descriptor");
        callback.require_current();validate(frame);
        require(&callback.host()==vm && &callback.source()==source.get(),"WorldView callback changed owner after validation");
    }
};
BnwWorldViewScriptUiAdapter::BnwWorldViewScriptUiAdapter(std::unique_ptr<State> state):state_(std::move(state)){}
BnwWorldViewScriptUiAdapter::~BnwWorldViewScriptUiAdapter(){state_->facade_alive.store(false);}
std::shared_ptr<ScriptUiControlGraph> BnwWorldViewScriptUiAdapter::build_owned(
    std::shared_ptr<ui::BnwWorldViewControlTree> tree,std::shared_ptr<const ScriptUiChildSource> source,
    LuaScriptHost& vm,
    std::vector<std::int64_t> events){
    require(tree&&source,"WorldView graph adapter actual owner slice absent");
    const auto source_owner=tree->metadata_source_owner();
    const auto& loan=source_owner->current_operation_loan();
    const State::Frame frame{&loan,loan.operation_serial()};
    const auto current_loan=[&]() -> const BnwWorldViewSourceOperationLoan& {
        const auto& actual=source_owner->current_operation_loan();
        require(&actual==frame.loan,"WorldView graph factory active loan substituted");
        actual.require_operation(frame.serial);return actual;
    };
    current_loan().require_same_vm(vm);current_loan().require_same_source(*source);
    source->require_current();current_loan().require_operation(frame.serial);
    const std::string context(source->context());
    require((context=="WorldView"||context=="ActionInfoPanel"||context=="UnitFlagManager"||context=="NotificationPanel")&&tree->context()==context,
            "WorldView graph adapter source/tree context differs");
    const auto actual_source=tree->actual_source();
    require(actual_source.get()==source.get()&&!actual_source.owner_before(source)&&!source.owner_before(actual_source),
            "WorldView graph adapter source is not the tree's actual descriptor");
    require(events.size()==2&&events[0]!=events[1],"WorldView adapter needs actual selected left/right Mouse events");
    auto state=std::make_unique<State>(tree,source,vm,source_owner,context,std::move(events));
    state->guarded([&](auto){
        for(const auto& item:tree->static_controls()){
            const auto quote=tree->inspect_control(item.second);
            require(!quote.dynamic&&quote.alive&&state->controls.emplace(quote.id,item.second).second,
                    "WorldView static canonical control identity duplicated");
            state->statics.push_back({item.first,quote.id});
        }
        if(context=="ActionInfoPanel"){
            const std::map<std::string,std::string,std::less<>> required{
                {"EndTurnButton","GridButton"},{"EndTurnText","Label"},
                {"EndTurnButtonProductionAlpha","AlphaAnim"},{"EndTurnButtonScienceAlpha","AlphaAnim"},
                {"EndTurnButtonFreeTechAlpha","AlphaAnim"},{"EndTurnButtonEndTurnAlpha","AlphaAnim"},
                {"EndTurnButtonMouseOverAlpha","AlphaAnim"}};
            require(state->statics.size()==required.size(),"ActionInfoPanel actual static control set differs");
            for(const auto& item:state->statics){
                const auto expected=required.find(item.name);
                require(expected!=required.end()&&tree->inspect_control(state->control(item.id)).authored.name==expected->second,
                        "ActionInfoPanel actual static control kind differs");
            }
        }
        return 0;
    });
    state->validate(frame);
    auto adapter=std::shared_ptr<BnwWorldViewScriptUiAdapter>(new BnwWorldViewScriptUiAdapter(std::move(state)));
    auto graph=std::shared_ptr<ScriptUiControlGraph>(new ScriptUiControlGraph(ScriptUiControlGraph::OwnedBackendTag{},context,adapter));
    graph->unitflag_health_adapter_=adapter;
    adapter->state_->validate(frame);return graph;
}
ui::BnwWorldViewControlTree::Control BnwWorldViewScriptUiAdapter::unitflag_health_control_owned(
 const std::shared_ptr<ScriptUiControlGraph>& graph,std::uint64_t id,std::uint64_t instance_root,
 const BnwSelectedWorldViewContextOwner& metadata){
 require(static_cast<bool>(graph),"UnitFlag original member Graph absent");
 const auto life=graph->layout_lifetime_;
 if(life->creator!=std::this_thread::get_id()){
  life->failed.store(true);throw ScriptError("UnitFlag original member Graph foreign creator");
 }
 require(life->alive.load()&&!life->failed.load(),"UnitFlag original member Graph retired before borrow");
 const auto adapter=graph->unitflag_health_adapter_.lock();
 require(adapter&&graph->backend_.get()==adapter.get()&&!graph->backend_.owner_before(adapter)&&!adapter.owner_before(graph->backend_),
     "UnitFlag original member Graph canonical Adapter substituted");
 const auto saved=adapter->state_;
 const auto first=[&]{
  if(saved->creator!=std::this_thread::get_id()){saved->failed.store(true);throw ScriptError("UnitFlag member State foreign creator");}
  require(saved->facade_alive.load()&&!saved->failed.load()&&!saved->retired&&life->alive.load()&&!life->failed.load(),
      "UnitFlag member Graph/Adapter retired before pure fields");
 };
 try{
  first();require(life->owner==graph.get()&&!graph->retired_&&!graph->poisoned_,"UnitFlag member Graph storage substituted");
  require(saved->source_owner.get()==&metadata,"UnitFlag member Adapter metadata replaced");
  const auto found=saved->controls.find(id);require(found!=saved->controls.end(),"UnitFlag original Control was not issued");
  const auto actual=found->second;
  saved->tree->require_unitflag_health_control_without_callbacks(actual,id,instance_root,metadata);first();
  return actual;
 }catch(...){life->failed.store(true);saved->render_fail();throw;}
}
void BnwWorldViewScriptUiAdapter::require_unitflag_health_current_owned(std::uint64_t id,
 BnwUnitFlagHealthCommand command)const {
 const auto saved=state_;
 saved->guarded_health(id,command,[](const State::Control&,const BnwWorldViewSourceOperationLoan&){return 0;});
}
template<class F> void ScriptUiControlGraph::unitflag_health_owned(Id id,BnwUnitFlagHealthCommand command,F&& operation)const {
 const auto life=layout_lifetime_;
 const auto first=[&]{
  if(life->creator!=std::this_thread::get_id()){
   life->failed.store(true);throw ScriptError("UnitFlag health Graph foreign creator before borrow");
  }
  require(life->alive.load()&&!life->failed.load(),"UnitFlag health Graph facade retired/failed before borrow");
 };
 first();const auto adapter=unitflag_health_adapter_.lock();
 const auto pure=[&](bool active){
  first();const auto& graph=*life->owner;
  require(graph.thread_==life->creator&&!graph.retired_&&!graph.poisoned_&&graph.busy_==active&&adapter&&
      graph.backend_.get()==adapter.get()&&!graph.backend_.owner_before(adapter)&&!adapter.owner_before(graph.backend_)&&
      !graph.unitflag_health_adapter_.owner_before(adapter)&&!adapter.owner_before(graph.unitflag_health_adapter_),
      "UnitFlag health canonical Graph/backend/reentry changed");
 };
 try{
  pure(false);life->owner->busy_=true;
  struct Exit{std::shared_ptr<LayoutLifetime> life;~Exit(){if(life->alive.load())life->owner->busy_=false;}}exit{life};
  pure(true);adapter->require_unitflag_health_current_owned(id,command);first();pure(true);
  operation(*adapter,id);first();pure(true);
  adapter->require_unitflag_health_current_owned(id,command);first();pure(true);
 }catch(...){
  life->failed.store(true);
  if(life->alive.load())life->owner->poisoned_=true;
  if(adapter){const auto saved=adapter->state_;saved->render_fail();}
  throw;
 }
}
void ScriptUiControlGraph::require_unitflag_health_current_owned(Id id,BnwUnitFlagHealthCommand command)const {
 unitflag_health_owned(id,command,[](BnwWorldViewScriptUiAdapter&,Id){});
}
void ScriptUiControlGraph::set_unitflag_health_hidden_owned(Id id,bool value){
 unitflag_health_owned(id,BnwUnitFlagHealthCommand::hide,[&](BnwWorldViewScriptUiAdapter& adapter,Id actual){adapter.set_hidden(actual,value);});
}
void ScriptUiControlGraph::set_unitflag_health_offset_x_owned(Id id,double value){
 unitflag_health_owned(id,BnwUnitFlagHealthCommand::offset_x,[&](BnwWorldViewScriptUiAdapter& adapter,Id actual){adapter.set_offset_x(actual,value);});
}
void ScriptUiControlGraph::set_unitflag_health_percent_owned(Id id,double value){
 unitflag_health_owned(id,BnwUnitFlagHealthCommand::percent,[&](BnwWorldViewScriptUiAdapter& adapter,Id actual){adapter.set_percent(actual,value);});
}
void ScriptUiControlGraph::set_unitflag_health_fg_color_owned(Id id,std::array<double,4> value){
 unitflag_health_owned(id,BnwUnitFlagHealthCommand::fg_color,[&](BnwWorldViewScriptUiAdapter& adapter,Id actual){adapter.set_fg_color(actual,value);});
}
bool BnwWorldViewScriptUiAdapter::context_hidden_owned(
        const std::shared_ptr<ScriptUiControlGraph>& graph,
        const BnwWorldViewSourceOperationLoan& operation,const LuaScriptHost& vm){
    // Caller is the actual protected ContextPtr scope. No full source callback
    // occurs here; this temporary backend pin dies before its caller's POST.
    require(graph&&graph->backend_,"Context visibility actual backend missing");
    const auto adapter=std::dynamic_pointer_cast<BnwWorldViewScriptUiAdapter>(graph->backend_);
    require(static_cast<bool>(adapter),"Context visibility backend is not actual WorldView adapter");
    auto& s=*adapter->state_;
    try {
        s.entry();
        require(s.vm==&vm,"Context visibility SAME actual VM substituted");
        const auto& actual=s.source_owner->current_operation_loan();
        require(&actual==&operation,"Context visibility active operation substituted");
        const auto serial=actual.operation_serial();
        actual.require_render_operation_owned();
        actual.require_same_vm(vm);actual.require_same_source(*s.source);
        require(!s.failed.load()&&!s.retired&&!s.busy,"Context visibility adapter retired during pure check");
        const bool result=s.tree->context_update_hidden_without_callbacks(actual);
        const auto& after=s.source_owner->current_operation_loan();
        require(&after==&operation&&after.operation_serial()==serial,"Context visibility operation changed during pure query");
        require(!s.failed.load()&&!s.retired&&!s.busy,"Context visibility adapter failed during pure query");
        return result;
    }catch(...){s.fail();throw;}
}
void BnwWorldViewScriptUiAdapter::bind_selected_container_owned(const std::shared_ptr<ScriptUiControlGraph>& graph,
        const std::shared_ptr<ui::BnwWorldViewControlTree>& target){
    require(graph&&target,"WorldView selected sibling actual owner missing");
    const auto adapter=std::dynamic_pointer_cast<BnwWorldViewScriptUiAdapter>(graph->backend_);
    require(adapter&&adapter->state_->context=="UnitFlagManager","WorldView selected sibling graph is not canonical UnitFlag adapter");
    auto& state=*adapter->state_;
    state.guarded([&](auto frame){
        require(!state.selected_bound&&target->context()=="InGame","WorldView selected sibling binding replaced or not InGame");
        const auto owner=target->metadata_source_owner();
        require(owner.get()==state.source_owner.get()&&!owner.owner_before(state.source_owner)&&!state.source_owner.owner_before(owner),
            "WorldView selected sibling source owner differs");
        state.loan(frame).require_same_vm(*state.vm);
        const auto selected=target->static_control("SelectedUnitContainer");const auto quote=target->inspect_control(selected);
        const auto controls=target->static_control("WorldViewControls");const auto control_quote=target->inspect_control(controls);
        const auto bulk=target->static_control("BulkUI");const auto bulk_quote=target->inspect_control(bulk);
        require(quote.parent==control_quote.id&&control_quote.parent==bulk_quote.id&&bulk_quote.parent==0&&
            quote.authored.name=="Container"&&control_quote.authored.name=="Container"&&bulk_quote.authored.name=="Container",
            "WorldView SelectedUnitContainer genuine InGame declaration chain differs");
        state.validate(frame);state.selected_control.emplace(selected);state.selected_tree=target;state.selected_bound=true;
        state.validate(frame);return 0;
    });
}
std::uint64_t BnwWorldViewScriptUiAdapter::lookup_control(std::string_view path)const {
    return state_->guarded([&](auto){require(state_->context=="UnitFlagManager"&&state_->selected_bound&&path=="../SelectedUnitContainer",
        "WorldView lookup path is outside bound selected sibling");return State::selected_alias;});
}
void BnwWorldViewScriptUiAdapter::require_current()const {state_->guarded([](auto){return 0;});}
void BnwWorldViewScriptUiAdapter::retire(){
    if(state_->creator!=std::this_thread::get_id()){state_->fail();throw ScriptError("WorldView graph retire foreign thread");}
    state_->retired=true;state_->fail();
}
void BnwWorldViewScriptUiAdapter::poison(){state_->fail();}
std::vector<ScriptUiGraphBackendNamed> BnwWorldViewScriptUiAdapter::static_controls()const {
    const auto saved=state_;
    return saved->guarded_render([&](const State::Frame& frame){return saved->statics;});
}
ScriptUiGraphBackendNode BnwWorldViewScriptUiAdapter::inspect(std::uint64_t id)const {
    const auto saved=state_;
    return saved->guarded_render([&](const State::Frame& frame){
        if(id==State::selected_alias){require(saved->selected_bound&&saved->selected_control,"WorldView foreign handle not issued");
            const auto target=saved->selected_tree.lock();require(static_cast<bool>(target),"WorldView foreign handle owner retired");
            const auto n=saved->render_operation_call(frame,[&]{return target->inspect_control(*saved->selected_control);});
            return ScriptUiGraphBackendNode{id,0,n.authored,n.hidden,n.alive,n.dynamic};}
        const auto n=saved->render_operation_call(frame,[&]{return saved->tree->inspect_control(saved->render_control(frame,id));});
        return ScriptUiGraphBackendNode{n.id,n.parent,n.authored,n.hidden,n.alive,n.dynamic};});
}
std::vector<ScriptUiGraphBackendNamed> BnwWorldViewScriptUiAdapter::build(std::string_view name,std::uint64_t parent){
    const auto saved=state_;
    return saved->guarded_render([&](const State::Frame& frame){
        require(parent,"WorldView instance requires actual parent control; unattached root issuer not supplied");
        auto instance=saved->render_operation_call(frame,[&]{return saved->tree->build_instance(name,saved->render_control(frame,parent));});
        auto next=saved->controls;std::vector<ScriptUiGraphBackendNamed> result;
        for(const auto& item:instance.controls){const auto quote=saved->render_operation_call(frame,[&]{return saved->tree->inspect_control(item.second);});
            require(next.emplace(quote.id,item.second).second,"WorldView dynamic canonical control reused");
            result.push_back({item.first,quote.id});}
        require(!result.empty()&&result.front().id==saved->render_operation_call(frame,[&]{return saved->tree->inspect_control(instance.root);}).id,
                "WorldView graph instance root is not first original named root");
        saved->render_full(frame);saved->controls.swap(next);return result;
    });
}
void BnwWorldViewScriptUiAdapter::set_hidden(std::uint64_t id,bool value){
    const auto saved=state_;
    if(saved->health_operation()){
        saved->guarded_health(id,BnwUnitFlagHealthCommand::hide,[&](const State::Control& control,const BnwWorldViewSourceOperationLoan& active){
            saved->tree->set_unitflag_health_hidden_owned(control,value,active);return 0;
        });return;
    }
    saved->guarded_render([&](const State::Frame& frame){saved->render_operation_call(frame,[&]{return saved->tree->set_hidden(saved->render_control(frame,id),value);});return 0;});
}
void BnwWorldViewScriptUiAdapter::change_parent(std::uint64_t id,std::uint64_t parent){
    const auto saved=state_;
    saved->guarded_render([&](const State::Frame& frame){if(parent==State::selected_alias){
            require(saved->selected_bound&&saved->selected_control,"WorldView cross-parent alias not issued");
            const auto target=saved->selected_tree.lock();require(static_cast<bool>(target),"WorldView cross-parent target retired");
            saved->render_operation_call(frame,[&]{return saved->tree->change_parent_selected_container_owned(saved->render_control(frame,id),target,*saved->selected_control);});
        }else saved->render_operation_call(frame,[&]{return saved->tree->change_parent(saved->render_control(frame,id),saved->render_control(frame,parent));});return 0;});
}
void BnwWorldViewScriptUiAdapter::destroy_child(std::uint64_t parent,std::uint64_t id){
    const auto saved=state_;
    saved->guarded_render([&](const State::Frame& frame){saved->render_operation_call(frame,[&]{return saved->tree->destroy_child(saved->render_control(frame,parent),saved->render_control(frame,id));});return 0;});
}
void BnwWorldViewScriptUiAdapter::set_alpha(std::uint64_t id,double value){
    const auto saved=state_;
    saved->guarded_render([&](const State::Frame& frame){saved->render_operation_call(frame,[&]{return saved->tree->set_alpha(saved->render_control(frame,id),value);});return 0;});
}
void BnwWorldViewScriptUiAdapter::set_consume_mouse_over(std::uint64_t id,bool value){
 const auto saved=state_;saved->consume_owned(id,value);
}
void BnwWorldViewScriptUiAdapter::set_disabled(std::uint64_t id,bool value){
    const auto saved=state_;
    saved->guarded_render([&](const State::Frame& frame){saved->render_operation_call(frame,[&]{return saved->tree->set_disabled(saved->render_control(frame,id),value);});return 0;});
}
void BnwWorldViewScriptUiAdapter::set_text(std::uint64_t id,std::string value){
    const auto saved=state_;
    saved->guarded_render([&](const State::Frame& frame){saved->render_operation_call(frame,[&]{return saved->tree->set_text(saved->render_control(frame,id),std::move(value));});return 0;});
}
void BnwWorldViewScriptUiAdapter::set_tooltip(std::uint64_t id,std::string value){
    const auto saved=state_;
    saved->guarded_render([&](const State::Frame& frame){saved->render_operation_call(frame,[&]{return saved->tree->set_tooltip(saved->render_control(frame,id),std::move(value));});return 0;});
}
std::uint64_t BnwWorldViewScriptUiAdapter::get_button(std::uint64_t id)const {
    const auto saved=state_;
    return saved->guarded_render([&](const State::Frame& frame){const auto actual=saved->render_operation_call(frame,[&]{return saved->tree->get_button(saved->render_control(frame,id));});
        const auto quote=saved->render_operation_call(frame,[&]{return saved->tree->inspect_control(actual);});(void)saved->render_control(frame,quote.id);return quote.id;});
}
void BnwWorldViewScriptUiAdapter::set_world_position(std::uint64_t id,std::array<double,3> value){
    const auto saved=state_;
    saved->guarded_render([&](const State::Frame& frame){saved->render_operation_call(frame,[&]{return saved->tree->set_world_position(saved->render_control(frame,id),value);});return 0;});
}
void BnwWorldViewScriptUiAdapter::set_offset(std::uint64_t id,std::array<double,2> value){
    const auto saved=state_;
    saved->guarded_render([&](const State::Frame& frame){saved->render_operation_call(frame,[&]{return saved->tree->set_offset(saved->render_control(frame,id),value);});return 0;});
}
void BnwWorldViewScriptUiAdapter::set_offset_x(std::uint64_t id,double value){
    const auto saved=state_;
    if(saved->health_operation()){
        saved->guarded_health(id,BnwUnitFlagHealthCommand::offset_x,[&](const State::Control& control,const BnwWorldViewSourceOperationLoan& active){
            saved->tree->set_unitflag_health_offset_x_owned(control,value,active);return 0;
        });return;
    }
    saved->guarded_render([&](const State::Frame& frame){saved->render_operation_call(frame,[&]{return saved->tree->set_offset_x(saved->render_control(frame,id),value);});return 0;});
}
void BnwWorldViewScriptUiAdapter::set_color(std::uint64_t id,std::array<double,4> value){
    const auto saved=state_;
    saved->guarded_render([&](const State::Frame& frame){saved->render_operation_call(frame,[&]{return saved->tree->set_color(saved->render_control(frame,id),value);});return 0;});
}
void BnwWorldViewScriptUiAdapter::set_fg_color(std::uint64_t id,std::array<double,4> value){
    const auto saved=state_;
    if(saved->health_operation()){
        saved->guarded_health(id,BnwUnitFlagHealthCommand::fg_color,[&](const State::Control& control,const BnwWorldViewSourceOperationLoan& active){
            saved->tree->set_unitflag_health_fg_color_owned(control,value,active);return 0;
        });return;
    }
    saved->guarded_render([&](const State::Frame& frame){saved->render_operation_call(frame,[&]{return saved->tree->set_fg_color(saved->render_control(frame,id),value);});return 0;});
}
void BnwWorldViewScriptUiAdapter::set_texture(std::uint64_t id,std::string value){
    const auto saved=state_;
    saved->guarded_render([&](const State::Frame& frame){saved->render_operation_call(frame,[&]{return saved->tree->set_texture(saved->render_control(frame,id),std::move(value));});return 0;});
}
void BnwWorldViewScriptUiAdapter::set_texture_offset(std::uint64_t id,std::array<double,2> value){
    const auto saved=state_;
    saved->guarded_render([&](const State::Frame& frame){saved->render_operation_call(frame,[&]{return saved->tree->set_texture_offset(saved->render_control(frame,id),value);});return 0;});
}
void BnwWorldViewScriptUiAdapter::set_percent(std::uint64_t id,double value){
    const auto saved=state_;
    if(saved->health_operation()){
        saved->guarded_health(id,BnwUnitFlagHealthCommand::percent,[&](const State::Control& control,const BnwWorldViewSourceOperationLoan& active){
            saved->tree->set_unitflag_health_percent_owned(control,value,active);return 0;
        });return;
    }
    saved->guarded_render([&](const State::Frame& frame){saved->render_operation_call(frame,[&]{return saved->tree->set_percent(saved->render_control(frame,id),value);});return 0;});
}
void BnwWorldViewScriptUiAdapter::animation_set_to_beginning(std::uint64_t id){
    const auto saved=state_;
    saved->guarded_render([&](const State::Frame& frame){saved->render_operation_call(frame,[&]{return saved->tree->animation_set_to_beginning(saved->render_control(frame,id));});return 0;});
}
void BnwWorldViewScriptUiAdapter::animation_play(std::uint64_t id){
    const auto saved=state_;
    saved->guarded_render([&](const State::Frame& frame){saved->render_operation_call(frame,[&]{return saved->tree->animation_play(saved->render_control(frame,id));});return 0;});
}
void BnwWorldViewScriptUiAdapter::calculate_size(std::uint64_t id){
    const auto saved=state_;
    saved->layout_guarded(id,[&](const auto& frame,const auto& control){
        BnwWorldViewNativeContextIssuer::dispatch_layout_control_owned(*saved->tree,control,*frame.loan,
            BnwWorldViewNativeContextIssuer::LayoutControlCommand::calculate_size,0.0);
        saved->layout_pure(frame,control);return 0;
    });
}
void BnwWorldViewScriptUiAdapter::reprocess_anchoring(std::uint64_t id){
    const auto saved=state_;
    saved->layout_guarded(id,[&](const auto& frame,const auto& control){
        BnwWorldViewNativeContextIssuer::dispatch_layout_control_owned(*saved->tree,control,*frame.loan,
            BnwWorldViewNativeContextIssuer::LayoutControlCommand::reprocess_anchoring,0.0);
        saved->layout_pure(frame,control);return 0;
    });
}
void BnwWorldViewScriptUiAdapter::set_size_y(std::uint64_t id,double value){
    const auto saved=state_;
    saved->layout_guarded(id,[&](const auto& frame,const auto& control){
        BnwWorldViewNativeContextIssuer::dispatch_layout_control_owned(*saved->tree,control,*frame.loan,
            BnwWorldViewNativeContextIssuer::LayoutControlCommand::set_size_y,value);
        saved->layout_pure(frame,control);return 0;
    });
}
void BnwWorldViewScriptUiAdapter::calculate_internal_size(std::uint64_t id){
    const auto saved=state_;
    saved->layout_guarded(id,[&](const auto& frame,const auto& control){
        BnwWorldViewNativeContextIssuer::dispatch_layout_control_owned(*saved->tree,control,*frame.loan,
            BnwWorldViewNativeContextIssuer::LayoutControlCommand::calculate_internal_size,0.0);
        saved->layout_pure(frame,control);return 0;
    });
}
std::array<double,2> BnwWorldViewScriptUiAdapter::get_offset_val(std::uint64_t id)const{
    const auto saved=state_;
    return saved->layout_guarded(id,[&](const auto& frame,const auto& control){
        const auto result=saved->tree->get_offset_val(control);saved->layout_pure(frame,control);return result;
    });
}
double BnwWorldViewScriptUiAdapter::get_size_y(std::uint64_t id)const{
    const auto saved=state_;
    return saved->layout_guarded(id,[&](const auto& frame,const auto& control){
        const auto result=saved->tree->get_size_y(control);saved->layout_pure(frame,control);return result;
    });
}
double BnwWorldViewScriptUiAdapter::get_ratio(std::uint64_t id)const{
    const auto saved=state_;
    return saved->layout_guarded(id,[&](const auto& frame,const auto& control){
        const auto result=saved->tree->get_ratio(control);saved->layout_pure(frame,control);return result;
    });
}
void BnwWorldViewScriptUiAdapter::register_callback(std::uint64_t id,std::int64_t event,
    std::shared_ptr<const ScriptUiOwnedControlCallback> callback){
    const auto saved=state_;
    try{
    saved->guarded_render([&](const State::Frame& frame){
        const auto node=saved->render_operation_call(frame,[&]{return saved->tree->inspect_control(saved->render_control(frame,id));});
        if(saved->context=="UnitFlagManager"){
            // Original Initialize is still inside its actual new/listener
            // call. Prove THAT current instance before callback storage; no
            // returned HealthMember or later readiness receipt is consulted.
            require(node.dynamic&&node.alive&&node.instance!=0&&node.authored.name=="Button"&&
                    (node.authored.attribute("ID")=="NormalButton"||node.authored.attribute("ID")=="HealthBarButton")&&
                    saved->event(event)&&callback,
                    "UnitFlag callback target/event differs from original dynamic button");
            const auto stored=saved->unitflag_button_operands.find(id);
            require(stored!=saved->unitflag_button_operands.end()&&stored->second.instance==node.instance&&
                stored->second.owner.has_value()&&stored->second.full_id.has_value(),
                "UnitFlag callback requires genuine prior native SetVoid1 and SetVoid2 stores");
            const std::weak_ptr<const void> physical=saved->tree->core_;
            saved->render_call(frame,[&]{saved->vm->require_unitflag_initializing_button_owned(*frame.loan,physical,
                id,node.instance,*stored->second.owner,*stored->second.full_id);});
            saved->render_full(frame);
        }else require(node.authored.name=="GridButton"&&node.authored.attribute("ID")=="EndTurnButton"&&
                saved->context=="ActionInfoPanel"&&saved->event(event)&&callback,
                "WorldView callback target/event is outside actual ActionInfoPanel slice");
        saved->render_callback_current(*callback,frame);
        auto next=saved->callbacks;next.insert_or_assign({id,event},std::move(callback));
        saved->render_full(frame);saved->callbacks.swap(next);
        // Destruction of a replaced descriptor is included before final guard.
        next.clear();saved->render_full(frame);return 0;
    });
    callback.reset();saved->render_first(false);
    saved->guarded_render([](const State::Frame&){return 0;});
    }catch(...){
        const auto original=std::current_exception();
        callback.reset();
        try{saved->render_first(false);}catch(...){saved->render_fail();}
        saved->render_fail();std::rethrow_exception(original);
    }
}
ScriptUiEventResult BnwWorldViewScriptUiAdapter::invoke_callback(std::uint64_t id,std::int64_t event,
    std::span<const ScriptValue> args){
    const auto saved=state_;
    // The actual LuaHost can enter this only through the graph's private route.
    // Source/input binding remains a later union, never a public event publisher.
    std::shared_ptr<const ScriptUiOwnedControlCallback> callback;
    bool blocked=false;
    try{
    const auto serial=saved->guarded_render([&](const State::Frame& frame){
        require(!saved->dispatch_active&&saved->event(event)&&args.empty(),"WorldView callback nested dispatch/event/args refused");
        const auto node=saved->render_operation_call(frame,[&]{return saved->tree->inspect_control(saved->render_control(frame,id));});
        if(saved->context=="UnitFlagManager"){
            // Do not read a cached HealthMember or native Void tuple as
            // authorization to dispatch this real dynamic widget callback.
            require(node.dynamic&&node.alive&&node.instance!=0&&node.authored.name=="Button"&&
                    (node.authored.attribute("ID")=="NormalButton"||node.authored.attribute("ID")=="HealthBarButton"),
                    "UnitFlag callback target differs from original dynamic button");
            saved->render_full(frame);
            throw ScriptError("UnitFlag callback invocation requires genuine current button callback admission before lookup/dispatch; native unit/Void data cannot issue it");
        }
        require(saved->context=="ActionInfoPanel"&&node.authored.attribute("ID")=="EndTurnButton",
                "WorldView callback target differs");
        const auto found=saved->callbacks.find({id,event});
        require(found!=saved->callbacks.end(),"WorldView actual source callback is not registered");
        callback=found->second;saved->render_callback_current(*callback,frame);
        blocked=node.disabled||saved->render_operation_call(frame,[&]{return saved->tree->inherited_hidden(saved->render_control(frame,id));});
        return frame;
    });
    if(blocked){callback.reset();saved->render_first(false);saved->guarded_render([&](const State::Frame& frame){
        require(frame==serial,"WorldView blocked callback teardown operation substituted");return 0;});
        return {}; // Exactly zero calls; no consumed-input receipt is issued.
    }
    // Expected Lua setters are allowed during the retained original callback;
    // nested callback dispatch is refused. VM Proto/environment guards belong
    // to the genuine callback descriptor and actual host binding.
    saved->render_pure(serial,false);saved->dispatch_active=true;
    struct Exit{bool& active;~Exit(){active=false;}} exit{saved->dispatch_active};
    try {
        auto result=saved->render_call(serial,[&]{return callback->invoke(args);},false);
        if(!result.errors.empty()){
            callback.reset();saved->render_pure(serial,false);saved->render_fail();return result;
        }
        saved->guarded_render([&](const State::Frame& frame){require(frame==serial,"WorldView callback operation substituted");
            saved->render_callback_current(*callback,frame);return 0;});
        callback.reset();saved->render_first(false);saved->guarded_render([&](const State::Frame& frame){require(frame==serial,"WorldView callback teardown operation substituted");return 0;});
        return result;
    }catch(...){
        const auto original=std::current_exception();
        callback.reset();
        try{saved->render_first(false);}catch(...){saved->render_fail();}
        saved->render_fail();std::rethrow_exception(original);
    }
    }catch(...){
        const auto original=std::current_exception();
        // Covers initial admission and blocked/descriptor-DTOR paths too.
        callback.reset();
        try{saved->render_first(false);}catch(...){saved->render_fail();}
        saved->render_fail();std::rethrow_exception(original);
    }
}
}

#include "bnw_worldview_initial_lua_graph_factory.inc"
#include "bnw_worldview_initial_lua_graph_verify.inc"
#include "bnw_worldview_unitflag_inflight_core_verify.inc"
#include "bnw_worldview_unitflag_inflight_void_setters.inc"


#include "bnw_context_showhide_adapter.inc"
