#include "civ5/runtime/script_ui_control_graph.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
using namespace civ5::runtime;
namespace {
void check(bool v,const char *what){if(!v)throw std::runtime_error(what);}
template<class F> void refusal(F f,const char *what){bool refused=false;try{f();}catch(...){refused=true;}check(refused,what);}
std::string text(const std::filesystem::path &p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Missing installed source");std::ostringstream s;s<<f.rdbuf();return s.str();}
ScriptUiControlGraph::Id named(const std::vector<ScriptUiControlGraph::Named> &nodes,std::string_view name){for(const auto &n:nodes)if(n.name==name)return n.id;throw std::runtime_error("Missing authored ID");}
}
int main(int argc,char **argv) {
    if(argc!=2)throw std::runtime_error("Expected canonical installed asset root");
    const auto root=std::filesystem::path(argv[1]);
    const auto xml=civ5::content::read_xml(root/"Assets/UI/InGame/YieldIconManager.xml");
    const auto manager=text(root/"Assets/UI/InstanceManager.lua");
    // This test's validator is deliberately a test lease, not a native owner.
    bool current=true;
    auto graph=std::make_shared<ScriptUiControlGraph>("YieldTest",xml,[&]{if(!current)throw std::runtime_error("Test lease retired");});
    const auto controls=graph->static_controls();const auto scrap=named(controls,"Scrap"),store=named(controls,"YieldStore");
    check(graph->inspect(scrap).hidden&&!graph->inspect(store).hidden,"Authored static hidden state");
    const auto first=graph->build("AnchorInstance",scrap),second=graph->build("AnchorInstance",scrap);
    const auto anchor=named(first,"Anchor"),stack=named(first,"Stack");
    check(anchor!=named(second,"Anchor")&&stack!=named(second,"Stack"),"Clones must have independent actual handles");
    check(graph->inspect(stack).parent==anchor&&graph->inspect(anchor).parent==scrap,"Authored clone subtree and requested parent");
    check(graph->inspect(anchor).authored.attribute("DepthAdjust")=="-25"&&graph->inspect(stack).authored.attribute("Padding")=="-40","Preserve authored anchor/stack attributes without geometry claims");
    graph->change_parent(anchor,store);graph->set_hidden(anchor,true);
    check(graph->inspect(anchor).parent==store&&graph->inspect(anchor).hidden,"Real graph mutations");
    refusal([&]{graph->change_parent(anchor,stack);},"Parent cycles must be rejected");
    graph->destroy_child(store,anchor);
    refusal([&]{graph->inspect(stack);},"Destroyed descendants cannot remain usable");
    check(graph->inspect(named(second,"Anchor")).alive,"Sibling instance survives other destruction");
    bool foreign_refused=false;std::thread worker([&]{try{graph->inspect(scrap);}catch(...){foreign_refused=true;}});worker.join();
    check(foreign_refused,"Foreign constructor thread refused");
    current=false;refusal([&]{graph->inspect(store);},"Lease retirement must revoke retained handles");current=true;

    // Guard failure AFTER mutation staging conservatively poisons the graph;
    // it cannot be resumed by making the external test lease true again.
    int checks=0;bool fail_after=false;
    auto uncertain=std::make_shared<ScriptUiControlGraph>("Uncertain",xml,[&]{if(fail_after&&++checks==2)throw std::runtime_error("After staged mutation");});
    const auto us=named(uncertain->static_controls(),"Scrap");fail_after=true;
    refusal([&]{uncertain->build("ImageInstance",us);},"Post-mutation owner change rejected");fail_after=false;
    refusal([&]{uncertain->static_controls();},"Uncertain mutation may never replay");

    LuaScriptHost host;
    auto lua_graph=std::make_shared<ScriptUiControlGraph>("LuaPool",xml,[]{});
    host.enable_ui_control_graph(lua_graph);
    host.execute("LuaPool",manager,"=actual shipped InstanceManager");
    host.execute("LuaPool",R"lua(
        local pool=InstanceManager:new("AnchorInstance","Anchor",Controls.Scrap)
        local first=pool:GetInstance()
        first.Anchor:ChangeParent(Controls.YieldStore)
        pool:ReleaseInstance(first)
        local again=pool:GetInstance()
        assert(rawequal(first,again))
        assert(rawequal(first.Anchor,again.Anchor))
        assert(pool.m_iCount==1 and pool.m_iAllocatedInstances==1 and pool.m_iAvailableInstances==0)
        pool:ResetInstances()
        local replay=pool:GetInstance()
        assert(rawequal(first,replay))
        replay.Anchor:ChangeParent(Controls.Scrap)
        local retained=replay.Stack
        pool:DestroyInstances()
        assert(not pcall(function() retained:SetHide(false) end))
    )lua","=pool reuse and destroyed descendant");
    refusal([&]{host.complete_loading();},"Caught stale-control failure still masks VM readiness");
    refusal([&]{lua_graph->require_current();},"Caught mutation/control failure poisons touched graph");

    // Generic authored local hidden state, exercised through actual Lua bindings.
    // The stock InGame toggle uses this exact colon/no-value-argument shape.
    LuaScriptHost visibility;
    auto vg=std::make_shared<ScriptUiControlGraph>("Visibility",xml,[]{});
    visibility.enable_ui_control_graph(vg);
    visibility.execute("Visibility",R"lua(
        assert(type(Controls.Scrap:IsHidden())=='boolean' and Controls.Scrap:IsHidden())
        assert(not Controls.YieldStore:IsHidden())
        Controls.Scrap:SetHide(not Controls.Scrap:IsHidden())
        assert(not Controls.Scrap:IsHidden())
        Controls.Scrap:SetHide(true);assert(Controls.Scrap:IsHidden())
        local first={};local sibling={}
        ContextPtr:BuildInstanceForControl('AnchorInstance',first,Controls.Scrap)
        ContextPtr:BuildInstanceForControl('AnchorInstance',sibling,Controls.Scrap)
        first.Anchor:SetHide(true)
        first.Anchor:ChangeParent(Controls.YieldStore)
        assert(first.Anchor:IsHidden() and type(first.Stack:IsHidden())=='boolean')
        LuaEvents.HiddenRead.Add(function(c) assert(c:IsHidden()) end)
        LuaEvents.HiddenRead(first.Anchor)
        retained_first=first;retained_sibling=sibling
    )lua","=hidden read, toggle, reparent and same-owner listener");
    visibility.complete_loading();vg->require_current();
    check(visibility.source_errors().empty(),"Actual hidden getter succeeds before negative lifetime case");
    visibility.execute("Visibility",R"lua(
        Controls.YieldStore:DestroyChild(retained_first.Anchor)
        assert(type(retained_sibling.Anchor:IsHidden())=='boolean')
        assert(not pcall(function() retained_first.Stack:IsHidden() end))
    )lua","=destroyed getter descendant");
    refusal([&]{visibility.complete_loading();},"Caught destroyed getter retains failure");
    refusal([&]{vg->require_current();},"Destroyed getter poisons touched graph");
    for(const auto source:{
        "Controls.Scrap:IsHidden(true)",
        "Controls.Scrap.IsHidden({})",
        "Controls.Scrap.IsHidden(newproxy(true))",
        "loadstring('return Controls.Scrap:IsHidden()')()"}) {
        LuaScriptHost invalid;
        invalid.enable_ui_control_graph(std::make_shared<ScriptUiControlGraph>("InvalidGetter",xml,[]{}));
        refusal([&]{invalid.execute("InvalidGetter",source,"=invalid getter authority/arity");},"Getter rejects invalid arguments/receiver/compiled caller");
        refusal([&]{invalid.complete_loading();},"Invalid getter cannot restore readiness");
    }
    LuaScriptHost foreign_getter;
    foreign_getter.enable_ui_control_graph(std::make_shared<ScriptUiControlGraph>("ReadA",xml,[]{}));
    foreign_getter.enable_ui_control_graph(std::make_shared<ScriptUiControlGraph>("ReadB",xml,[]{}));
    foreign_getter.execute("ReadB","LuaEvents.ForeignRead.Add(function(control,env) setfenv(1,env);return control:IsHidden() end)","=foreign getter listener");
    refusal([&]{foreign_getter.execute("ReadA","LuaEvents.ForeignRead(Controls.Scrap,_G)","=authentic foreign getter handoff");},"Getter requires same compiled owner despite genuine receiver and fenv");
    refusal([&]{foreign_getter.complete_loading();},"Foreign getter failure is sticky");
    for(bool retire:{false,true}) {
        LuaScriptHost callback_getter;int visits=0;bool armed=false;
        std::shared_ptr<ScriptUiControlGraph> cg;
        cg=std::make_shared<ScriptUiControlGraph>("CallbackGetter",xml,[&]{
            if(armed&&++visits==2) {
                if(retire)cg->retire();
                else refusal([&]{callback_getter.execute("Other","return","=caught getter callback reentry");},"Getter callback cannot reenter host");
            }
        });
        callback_getter.enable_ui_control_graph(cg);
        callback_getter.execute("CallbackGetter","saved=Controls.Scrap","=bind getter before callback arming");
        armed=true;
        refusal([&]{callback_getter.execute("CallbackGetter","assert(type(saved:IsHidden())=='boolean')","=getter owner callback");},"Getter refuses callback retirement or caught reentry");
        check(visits>=2,"Getter callback reaches API validation after pre-call validation");
        refusal([&]{callback_getter.complete_loading();},"Getter callback failure retains sticky host failure");
    }
    LuaScriptHost closed_getter;
    auto clg=std::make_shared<ScriptUiControlGraph>("ClosedGetter",xml,[]{});
    closed_getter.enable_ui_control_graph(clg);
    closed_getter.execute("ClosedGetter","saved=Controls.Scrap;assert(saved:IsHidden())","=retain actual getter");
    closed_getter.close_ui_context("ClosedGetter");
    refusal([&]{closed_getter.execute("ClosedGetter","saved:IsHidden()","=closed getter");},"Closed context cannot reuse retained getter");

    // Genuine include resolves/compiles installed InstanceManager in this VM.
    // A Lua loadstring replacement cannot substitute arbitrary source or names.
    int included_reads=0;
    ScriptSourceProvider included_sources{
        [](std::string_view){return std::vector<std::string>{"InstanceManager.lua","Nested.lua"};},
        [&](std::string_view,std::string_view path){
            ++included_reads;
            if(path=="InstanceManager.lua")return manager;
            if(path=="Nested.lua")return std::string("include('InstanceManager'); nested_loaded=true");
            throw std::runtime_error("Unselected source");
        }};
    LuaScriptHost included({},included_sources);
    auto included_graph=std::make_shared<ScriptUiControlGraph>("Included",xml,[]{});
    included.enable_ui_control_graph(included_graph);
    included.execute("Included",R"lua(
        loadstring=function() error('Lua-visible compiler must not run') end
        compile=function() error('Global compiler spoof must not run') end
        assert(include('Nested')[1]=='Nested.lua' and nested_loaded)
        local factory=InstanceManager
        local pool=factory:new('AnchorInstance','Anchor',Controls.Scrap)
        local first=pool:GetInstance();pool:ReleaseInstance(first)
        assert(rawequal(first,pool:GetInstance()))
        assert(include('InstanceManager')[1]=='InstanceManager.lua')
        -- Previously retained functions remain owned after a repeat include/GC.
        collectgarbage('collect');pool:ResetInstances()
        local again=pool:GetInstance();again.Anchor:ChangeParent(Controls.YieldStore)
        LuaEvents.Included.Add(function(c) again.Anchor:ChangeParent(c) end)
        LuaEvents.Included(Controls.Scrap)
        pool:DestroyInstances()
    )lua","=real nested and repeated include");
    included.complete_loading();included_graph->require_current();
    check(included_reads==3&&included.source_errors().empty(),"Nested/repeated include uses selected provider exactly once per match");

    for(bool foreign_owner:{false,true}) {
        int reads=0;LuaScriptHost rejected({}, {included_sources.files,
            [&](std::string_view,std::string_view){++reads;return manager;}});
        auto ga=std::make_shared<ScriptUiControlGraph>("IncludeA",xml,[]{});
        auto gb=std::make_shared<ScriptUiControlGraph>("IncludeB",xml,[]{});
        rejected.enable_ui_control_graph(ga);rejected.enable_ui_control_graph(gb);
        rejected.execute("IncludeA","saved_include=include","=authentic captured include");
        if(foreign_owner) {
            rejected.execute("IncludeB","LuaEvents.BorrowInclude.Add(function(fn,env) setfenv(1,env);fn('InstanceManager') end)","=foreign include borrower");
            rejected.execute("IncludeA","LuaEvents.BorrowInclude(saved_include,_G)","=A include handoff");
        }else {
            rejected.execute("IncludeA","loadstring(\"include('InstanceManager')\",'@InstanceManager.lua')()","=unknown caller spoofs include name");
        }
        check(reads==0&&!rejected.source_errors().empty(),"Unknown/foreign include caller is rejected before source read");
        refusal([&]{rejected.complete_loading();},"Caught include authorization failure cannot restore readiness");
        refusal([&]{ga->require_current();},"Rejected include poisons its touched owner graph");
    }
    LuaScriptHost substituted({},included_sources);
    auto sg=std::make_shared<ScriptUiControlGraph>("Substitution",xml,[]{});substituted.enable_ui_control_graph(sg);
    substituted.execute("Substitution","IncludeFileList[1]='Unlisted.lua';include('Unlisted')","=Lua list is not source authority");
    check(!substituted.source_errors().empty(),"Lua-visible list mutation cannot authorize an unselected source");
    refusal([&]{substituted.complete_loading();},"Unselected include retains failure");
    int provider_calls=0;LuaScriptHost *reentry_host=nullptr;
    LuaScriptHost provider_reentry({}, {included_sources.files,[&](std::string_view,std::string_view){
        ++provider_calls;refusal([&]{reentry_host->execute("Other","return","=caught include provider reentry");},"Provider reentry rejected");return manager;}});
    reentry_host=&provider_reentry;
    auto pg=std::make_shared<ScriptUiControlGraph>("Provider",xml,[]{});provider_reentry.enable_ui_control_graph(pg);
    provider_reentry.execute("Provider","include('InstanceManager')","=provider catches host reentry");
    check(provider_calls==1&&!provider_reentry.source_errors().empty(),"Caught provider reentry fails include compilation");
    refusal([&]{provider_reentry.complete_loading();},"Provider reentry cannot restore readiness");
    refusal([&]{pg->require_current();},"Provider reentry poisons its owner");
    LuaScriptHost throwing({}, {included_sources.files,[](std::string_view,std::string_view)->std::string{throw std::runtime_error("provider failure");}});
    auto tg=std::make_shared<ScriptUiControlGraph>("Throwing",xml,[]{});throwing.enable_ui_control_graph(tg);
    throwing.execute("Throwing","include('InstanceManager')","=provider exception");
    check(throwing.source_errors().size()==1,"Provider exceptions produce an include diagnostic");
    refusal([&]{throwing.complete_loading();},"Provider exception retains failure");

    LuaScriptHost foreign;
    auto a=std::make_shared<ScriptUiControlGraph>("A",xml,[]{}),b=std::make_shared<ScriptUiControlGraph>("B",xml,[]{});
    foreign.enable_ui_control_graph(a);foreign.enable_ui_control_graph(b);
    foreign.execute("B",R"lua(
        LuaEvents.ForeignControl.Add(function(parent)
            local output={}
            ContextPtr:BuildInstanceForControl("ImageInstance",output,parent)
        end)
    )lua","=context B consumer");
    refusal([&]{foreign.execute("A","LuaEvents.ForeignControl(Controls.Scrap)","=context A foreign parent");},"Cross-context parent userdata rejected in same VM");

    LuaScriptHost direct;
    direct.enable_ui_control_graph(std::make_shared<ScriptUiControlGraph>("DirectA",xml,[]{}));
    direct.enable_ui_control_graph(std::make_shared<ScriptUiControlGraph>("DirectB",xml,[]{}));
    direct.execute("DirectB","LuaEvents.DirectForeign.Add(function(control) control:SetHide(false) end)","=foreign method consumer");
    refusal([&]{direct.execute("DirectA","LuaEvents.DirectForeign(Controls.Scrap)","=foreign direct method");},"Receiver's own closure cannot authorize a foreign Lua caller environment");

    bool callback_reentry=false;LuaScriptHost guarded;std::shared_ptr<ScriptUiControlGraph> guarded_graph;
    guarded_graph=std::make_shared<ScriptUiControlGraph>("Guarded",xml,[&]{
        if(callback_reentry)refusal([&]{guarded.execute("Other","return","=native validator reentry");},"Native guard callback cannot reenter VM");
    });
    guarded.enable_ui_control_graph(guarded_graph);callback_reentry=true;
    refusal([&]{guarded.execute("Guarded","Controls.Scrap:SetHide(true)","=guard reentry check");},"Repeated validator reentry refuses source");
    // Existing execute marks the VM failed even for a caught reentry attempt;
    // this conservative host behavior must not be relaxed by graph validation.
    refusal([&]{guarded.complete_loading();},"Guard reentry cannot leave a ready VM");

    // Regression 1: pre-call guard succeeds; only the API's first validator
    // catches reentry. Cover each saved failed/ready restoration path separately.
    for(bool context_api:{false,true}) {
        LuaScriptHost inner;int validations=0;bool armed=false;
        auto g=std::make_shared<ScriptUiControlGraph>("Inner",xml,[&]{
            if(armed&&++validations==2)
                refusal([&]{inner.execute("Other","return","=caught API validator reentry");},"Actual reentry refused");
        });
        inner.enable_ui_control_graph(g);inner.execute("Inner","return","=bind before arming");
        armed=true;
        refusal([&]{inner.execute("Inner",context_api?
            "ContextPtr:BuildInstance('ImageInstance',{})":"Controls.Scrap:SetHide(true)",
            "=API-only caught reentry");},"Sticky callback failure refuses outer source");
        check(validations>=2,"Regression reaches API validation after successful pre-call guard");
        refusal([&]{inner.complete_loading();},"Caught API reentry cannot restore loading readiness");
        refusal([&]{g->require_current();},"Caught API reentry poisons the graph permanently");
    }
    // Regression 2: genuine A userdata/environment travel through this VM's
    // actual LuaEvents. B mutates its own fenv, retaining its compiled B owner.
    for(bool context_api:{false,true}) {
        LuaScriptHost forged;auto ga=std::make_shared<ScriptUiControlGraph>("EnvA",xml,[]{});
        auto gb=std::make_shared<ScriptUiControlGraph>("EnvB",xml,[]{});
        forged.enable_ui_control_graph(ga);forged.enable_ui_control_graph(gb);
        forged.execute("EnvB",context_api?R"lua(
            LuaEvents.ForeignEnv.Add(function(context,env)
                setfenv(1,env);context:BuildInstance("ImageInstance",{})
            end)
        )lua":R"lua(
            LuaEvents.ForeignEnv.Add(function(control,env)
                setfenv(1,env);control:SetHide(false)
            end)
        )lua","=B listener with mutable environment");
        refusal([&]{forged.execute("EnvA",context_api?
            "LuaEvents.ForeignEnv(ContextPtr,_G)":"LuaEvents.ForeignEnv(Controls.Scrap,_G)",
            "=A authentic receiver handoff");},"setfenv cannot forge compiled listener ownership");
        refusal([&]{forged.complete_loading();},"Foreign compiled listener rejects loading readiness");
        refusal([&]{ga->require_current();},"Borrowed A graph remains poisoned");
    }
    // A compiled same-owner listener remains usable. Dynamically compiled code
    // has no Host-authenticated source provenance and explicitly refuses graph use.
    LuaScriptHost owned;auto same=std::make_shared<ScriptUiControlGraph>("Same",xml,[]{});
    owned.enable_ui_control_graph(same);
    owned.execute("Same","LuaEvents.Same.Add(function(c) c:SetHide(false) end);LuaEvents.Same(Controls.Scrap)","=same compiled listener");
    owned.complete_loading();same->require_current();
    LuaScriptHost dynamic;auto unknown=std::make_shared<ScriptUiControlGraph>("Unknown",xml,[]{});
    dynamic.enable_ui_control_graph(unknown);
    refusal([&]{dynamic.execute("Unknown","loadstring('Controls.Scrap:SetHide(false)')()","=unknown dynamically compiled caller");},"loadstring fenv confers no graph authority");
    refusal([&]{dynamic.complete_loading();},"Unknown compiled source rejects readiness");

    LuaScriptHost receiver;
    auto r=std::make_shared<ScriptUiControlGraph>("Receiver",xml,[]{});receiver.enable_ui_control_graph(r);
    receiver.execute("Receiver",R"lua(
        local output={}
        assert(not pcall(function() ContextPtr.BuildInstance({},"AnchorInstance",output) end))
    )lua","=forged ContextPtr receiver");
    refusal([&]{receiver.complete_loading();},"Caught receiver error does not declare a ready VM");

    LuaScriptHost close;
    auto closed=std::make_shared<ScriptUiControlGraph>("Closed",xml,[]{});close.enable_ui_control_graph(closed);
    close.execute("Closed","saved=Controls.Scrap","=retained control");close.close_ui_context("Closed");
    refusal([&]{close.execute("Closed","saved:SetHide(false)","=retired context");},"Closed graph context cannot reopen");
    refusal([&]{closed->static_controls();},"Externally retained parent cannot restore closed graph");
    std::cout<<"PASS: actual authored subtrees, shipped Lua pooling, foreign context/thread and uncertain lifetime refusals\n";
}
