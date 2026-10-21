#pragma once
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/runtime/script_ui_control_graph.hpp"
#include <chrono>
#include <cfenv>
#include <exception>
#include <fstream>
#include <limits>
#include <map>
#include <thread>
// Generic test owner over real confined loose handles. This is deliberately
// not the missing selected InGame/package activation issuer.
void lua_child_context_tests() {
    using namespace civ5::runtime;
    using namespace civ5::assets;
    struct Lease {};
    struct Source final:ScriptUiChildSource {
        std::weak_ptr<Lease> lease;AssetHandle handle;Bytes pin;std::string id;
        Source(std::weak_ptr<Lease> l,AssetHandle h,std::string n):lease(l),handle(std::move(h)),pin(handle.read_bytes()),id(std::move(n)){}
        std::string_view context()const override{return id;}
        const AssetHandle& asset()const override{return handle;}
        void require_current()const override {
            if(lease.expired()||handle.read_bytes()!=pin)throw ScriptError("Test source lease/bytes changed");
        }
        void require_captured_bytes(std::span<const std::uint8_t> bytes)const override {
            require_current();if(bytes.size()!=pin.size()||!std::equal(bytes.begin(),bytes.end(),pin.begin()))
                throw ScriptError("Captured source differs from same pin");
        }
    };
    struct Owner final:ScriptUiContextSourceOwner {
        std::weak_ptr<Lease> lease;
        std::map<std::string,std::shared_ptr<Source>> sources;
        std::vector<std::string> requests;
        std::function<void()> on_resolve,on_check;
        void require_current(std::string_view context,const ScriptUiChildSource *source)const override {
            if(lease.expired())throw ScriptError("Root lease dropped");
            if(on_check)on_check();
            if(lease.expired())throw ScriptError("Root lease dropped during callback");
            if(!source) {if(context!="Root")throw ScriptError("Unowned root");return;}
            bool found=false;for(const auto &[key,value]:sources)if(value.get()==source&&value->id==context)found=true;
            if(!found)throw ScriptError("Foreign child descriptor");
        }
        std::shared_ptr<const ScriptUiChildSource> resolve_child(std::string_view parent,
            const ScriptUiChildSource *source,std::string_view request)override {
            require_current(parent,source);requests.emplace_back(request);
            if(on_resolve)on_resolve();
            const auto found=sources.find(std::string(request));
            if(found==sources.end())throw ScriptError("Request outside owned set");
            return found->second;
        }
    };
    const auto root=std::filesystem::temp_directory_path()/
        ("civis-child-context-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    const auto put=[&](const char *name,std::string_view text){std::ofstream out(root/name,std::ios::binary);out.write(text.data(),static_cast<std::streamsize>(text.size()));};
    put("Child.lua",R"lua(
        assert(parentMarker==nil and Controls==nil)
        childMarker=41
        include('ChildHelper')
        assert(includedChild==true)
        GameEvents.ChildShared.Add(function() return 10 end)
        assert(GameEvents.ParentVisible.TestAll()==true)
        assert(GameEvents.ChildShared.AccumulateINT()==11)
        grand=ContextPtr:LoadNewContext('grand')
        assert(type(grand)=='table' and type(grand.LoadNewContext)=='function')
        GameEvents.RetainedChild.Add(function() assert(childMarker==41); return true end)
    )lua");
    put("Grand.lua",R"lua(
        assert(parentMarker==nil and childMarker==nil)
        GameEvents.ChildShared.Add(function() return 100 end)
    )lua");
    put("Plain.lua","GameEvents.Plain.Add(function() return true end)");
    put("Error.lua","GameEvents.Partial.Add(function() return true end); error('child failed')");
    put("Syntax.lua","local =");
    put("Yield.lua","coroutine.yield()");
    put("Rebound.lua","ContextPtr={}");
    put("CallbackRebound.lua",R"lua(
        GameEvents.RebindChild.Add(function() ContextPtr={}; return true end)
        GameEvents.RebindChild.Add(function() print('must not run after child rebound'); return true end)
    )lua");
    put("Caught.lua","pcall(function() ContextPtr:LoadNewContext('missing') end)");
    put("Update.lua",R"lua(
        local retained=ContextPtr
        GameEvents.UpdateShared.Add(function() return 10 end)
        ContextPtr:SetUpdate(function(dt)
            assert(rawequal(retained,ContextPtr) and type(dt)=='number')
            assert(GameEvents.UpdateShared.AccumulateINT()==11)
            print('child update',dt)
        end)
    )lua");
    put("UpdateRebound.lua","ContextPtr:SetUpdate(function(dt) ContextPtr={} end)");
    auto index=std::make_shared<LooseResourceIndex>(root);
    for(const char *name:{"Child.lua","Grand.lua","Plain.lua","Error.lua","Syntax.lua","Caught.lua","Yield.lua","Rebound.lua","CallbackRebound.lua","Update.lua","UpdateRebound.lua"})index->add_file(name);
    auto lease=std::make_shared<Lease>();
    const auto owner_for=[&] {
        auto result=std::make_shared<Owner>();result->lease=lease;
        for(const auto &[key,file]:std::map<std::string,std::string>{{"child","Child.lua"},{"grand","Grand.lua"},
            {"plain","Plain.lua"},{"error","Error.lua"},{"syntax","Syntax.lua"},{"caught","Caught.lua"},
            {"yield","Yield.lua"},{"rebound","Rebound.lua"},{"callback-rebound","CallbackRebound.lua"},{"update","Update.lua"},{"update-rebound","UpdateRebound.lua"}}) {
            auto lookup=index->find(AssetKey(file));
            result->sources.emplace(key,std::make_shared<Source>(lease,std::get<AssetHandle>(std::move(lookup)),"Owned-"+key));
        }
        return result;
    };
    // A genuine generic test frame service, tied to the exact source owner,
    // VM thread and externally retained lease. It is not a selected frame issuer.
    struct Updates final:ScriptUiUpdateSourceOwner {
        struct Frame final:ScriptUiUpdateFrame {
            const Updates *owner;std::weak_ptr<Lease> lease;
            std::uint64_t generation,serial;double delta;
            Frame(const Updates *o,std::weak_ptr<Lease> l,std::uint64_t g,std::uint64_t s,double d):
                owner(o),lease(l),generation(g),serial(s),delta(d){}
            const ScriptUiUpdateSourceOwner& issuer()const override{return owner->foreign_issuer?*owner->foreign_issuer:*owner;}
            std::uint64_t sequence()const override{return serial+(owner->rebind_observation?1:0);}
            double delta_seconds()const override{return delta;}
            void require_current(const ScriptUiContextSourceOwner &source)const override {
                owner->require_current(source);
                if(owner->on_frame_check)owner->on_frame_check();
                if(lease.expired()||generation!=owner->generation)throw ScriptError("Retired/rebound frame lease");
            }
        };
        const Owner *source=nullptr;std::weak_ptr<Lease> lease;
        const std::thread::id thread=std::this_thread::get_id();
        std::uint64_t generation=0,serial=0;double delta=0;
        bool pending=false,rebind_observation=false;
        const ScriptUiUpdateSourceOwner *foreign_issuer=nullptr;
        std::size_t consumes=0;
        std::function<void()> on_check,on_consume,on_frame_check;
        void require_current(const ScriptUiContextSourceOwner &value)const override {
            if(&value!=source||lease.expired()||thread!=std::this_thread::get_id())throw ScriptError("Foreign/retired update service");
            source->require_current("Root",nullptr);
            if(on_check)on_check();
            if(lease.expired())throw ScriptError("Update lease dropped in callback");
        }
        std::unique_ptr<const ScriptUiUpdateFrame> consume_frame()override {
            ++consumes;if(on_consume)on_consume();
            if(!pending)return {};
            pending=false;return std::make_unique<Frame>(this,lease,generation,serial,delta);
        }
        void issue(std::uint64_t s,double d){if(pending)throw ScriptError("Test frame already pending");++generation;serial=s;delta=d;pending=true;}
    };
    auto owner=owner_for();
    ScriptSourceProvider includes{
        [](std::string_view context){return context=="Owned-child"?std::vector<std::string>{"ChildHelper.lua"}:std::vector<std::string>{};},
        [](std::string_view context,std::string_view path){if(context!="Owned-child"||path!="ChildHelper.lua")throw ScriptError("Foreign include");return std::string("includedChild=true");}};
    LuaScriptHost host({},includes);host.enable_ui_context_sources(owner);
    host.execute("Root",R"lua(
        parentMarker=9
        GameEvents.ParentVisible.Add(function() return true end)
        GameEvents.ChildShared.Add(function() return 1 end)
        retained=ContextPtr:LoadNewContext('child')
        assert(parentMarker==9 and childMarker==nil)
        assert(type(retained)=='table' and not rawequal(retained,ContextPtr))
        assert(GameEvents.ChildShared.AccumulateINT()==111)
    )lua","=owned-parent");
    check(owner->requests==std::vector<std::string>{"child","grand"},"Actual resolver request/nested load order must persist");
    const auto loaded_journal=host.execution_snapshot();
    std::vector<std::string> child_requests;bool child_include=false,child_executed=false;
    for(const auto &record:loaded_journal.records) {
        if(record.action==ScriptLuaAction::child_load) {
            child_requests.push_back(record.name);
            check(record.outcome==ScriptLuaActionOutcome::succeeded,"Child completion must follow actual execution/postflight");
        }
        if(record.action==ScriptLuaAction::include&&record.context=="Owned-child")
            child_include=record.name=="ChildHelper.lua"&&record.outcome==ScriptLuaActionOutcome::succeeded&&
                record.source_sha256.size()==64&&record.source_bytes==std::string_view("includedChild=true").size();
        if(record.action==ScriptLuaAction::context_execute&&record.context=="Owned-child")
            child_executed=record.source_sha256.size()==64&&record.source_bytes>0;
    }
    check(child_requests==owner->requests&&child_include&&child_executed,
          "Same VM journal must reflect nested requests, actual child bytes and executed include without selected activation claim");
    host.execute("Root","assert(type(retained.LoadNewContext)=='function'); assert(GameEvents.RetainedChild.TestAll())","=after-parent");
    check(host.accumulate_int("ChildShared").value==111,"Parent and children must register in existing shared groups");
    host.complete_loading();check(host.loading_complete(),"Generic successful source set may complete without selected activation claim");
    // Actual retained child receiver remains a child: an authenticated root
    // closure cannot use its LoadNewContext method as root authority.
    rejects([&]{host.execute("Root","pcall(function() retained:LoadNewContext('plain') end)","=foreign-receiver");});
    rejects([&]{host.complete_loading();});
    check(owner->requests.size()==2,"Foreign receiver must refuse before provider resolution");
    const auto failure=[&](std::string_view body,std::function<void(Owner&,LuaScriptHost&)> setup={}) {
        auto owned=owner_for();LuaScriptHost child;child.enable_ui_context_sources(owned);
        if(setup)setup(*owned,child);
        rejects([&]{child.execute("Root",body,"=child-negative");});
        rejects([&]{child.complete_loading();});
        check(!child.loading_complete(),"Caught child failure cannot become ready");
        const auto terminal_journal=child.execution_snapshot();
        check(terminal_journal.failed&&!terminal_journal.records.empty(),"Failed child attempt must retain actual source history");
        for(const auto &record:terminal_journal.records)
            check(record.outcome!=ScriptLuaActionOutcome::open,"Protected failure must retain stopped or incomplete attempts, never an unreported live action");
    };
    // The authentic generic child include must retain the writer/epoch
    // guard even when its provider catches a native completion reentry.
    {
        auto owned=owner_for();LuaScriptHost* running=nullptr;bool refused_reentry=false;
        ScriptSourceProvider interrupted_includes=includes;
        interrupted_includes.read=[&](std::string_view context,std::string_view path){
            if(context!="Owned-child"||path!="ChildHelper.lua")throw ScriptError("Foreign include");
            try{running->complete_loading();}catch(...){refused_reentry=true;}
            return std::string("print('helper must not execute'); includedChild=true");
        };
        LuaScriptHost child({},interrupted_includes);running=&child;child.enable_ui_context_sources(owned);
        rejects([&]{child.execute("Root","retained=ContextPtr:LoadNewContext('child')","=child-include-completion-reentry");});
        check(refused_reentry&&child.messages().empty()&&!child.loading_complete(),
              "Caught completion must stop before returned include execution and keep the actual child failed");
        const auto errors=child.source_errors();
        check(errors.size()==1&&errors[0].context=="Owned-child"&&errors[0].path=="ChildHelper.lua"&&
              errors[0].message=="Include provider compiler/source operation retired",
              "The actual provider stamp must report its real retirement before compilation");
        const auto history=child.execution_snapshot();bool failed_include=false;
        for(const auto& record:history.records){
            check(record.outcome!=ScriptLuaActionOutcome::open,"Retired child include cannot leave an open actual operation");
            if(record.action==ScriptLuaAction::include&&record.context=="Owned-child"&&record.name=="ChildHelper.lua")
                failed_include=record.outcome==ScriptLuaActionOutcome::error&&record.source_sha256.empty()&&record.source_bytes==0;
        }
        check(history.failed&&failed_include,"Retired provider must never attest a compiled/executed helper");
        rejects([&]{child.complete_loading();});
    }
    // Rebinding AFTER successful load is detected on callback delivery, not
    // only when another LoadNewContext happens to access that receiver.
    {
        auto owned=owner_for();LuaScriptHost child;child.enable_ui_context_sources(owned);
        child.execute("Root","retained=ContextPtr:LoadNewContext('callback-rebound')","=loaded-child-rebind");
        child.complete_loading();
        rejects([&]{child.test_all("RebindChild");});
        check(child.messages().empty()&&!child.loading_complete(),"Child rebound must stop the next actual callback and revoke readiness");
        rejects([&]{child.complete_loading();});
    }
    {
        auto owned=owner_for();LuaScriptHost child;child.enable_ui_context_sources(owned);
        child.execute("Root",R"lua(
            retained=ContextPtr:LoadNewContext('plain')
            GameEvents.RebindRoot.Add(function() ContextPtr={}; return true end)
            GameEvents.RebindRoot.Add(function() print('must not run after root rebound'); return true end)
        )lua","=root-callback-rebind");
        child.complete_loading();
        rejects([&]{child.test_all("RebindRoot");});
        check(child.messages().empty()&&!child.loading_complete(),"Root callback rebound must stop next callback and completion");
        rejects([&]{child.complete_loading();});
    }
    failure("ContextPtr:LoadNewContext('plain'); ContextPtr={}");
    failure("ContextPtr:LoadNewContext('callback-rebound'); pcall(function() GameEvents.RebindChild.TestAll() end)");
    // Inherited fallback cannot impersonate the retained own ContextPtr field.
    failure("local kept=ContextPtr; ContextPtr=nil; setmetatable(_G,{__index=function(_,key) if key=='ContextPtr' then return kept end end})");
    failure("pcall(function() ContextPtr:LoadNewContext('error') end)");
    failure("pcall(function() ContextPtr:LoadNewContext('syntax') end)");
    failure("pcall(function() ContextPtr:LoadNewContext('yield') end)");
    failure("pcall(function() ContextPtr:LoadNewContext('rebound') end)");
    failure("pcall(function() ContextPtr:LoadNewContext('caught') end)");
    failure("local dynamic=loadstring(\"return ContextPtr:LoadNewContext('plain')\"); pcall(dynamic)");
    failure("local f=ContextPtr.LoadNewContext; pcall(function() f({},'plain') end)");
    failure("ContextPtr:LoadNewContext('plain'); pcall(function() ContextPtr:LoadNewContext('plain') end)");
    failure("pcall(function() ContextPtr:LoadNewContext('plain') end)",[](Owner &owned,LuaScriptHost &child){
        owned.on_resolve=[&child]{try{child.execute("Root","assert(true)","=provider-reentry");}catch(...){}};
    });
    failure("pcall(function() ContextPtr:LoadNewContext('plain') end)",[](Owner &owned,LuaScriptHost &child){
        owned.on_resolve=[&child]{try{child.complete_loading();}catch(...){}};
    });
    failure("pcall(function() ContextPtr:LoadNewContext('plain') end)",[](Owner &owned,LuaScriptHost &){owned.on_resolve=[] {throw 7;};});
    failure("pcall(function() ContextPtr:LoadNewContext('plain') end)",[&](Owner &owned,LuaScriptHost &){
        owned.on_resolve=[&]{put("Plain.lua","error('substituted file')");};
    });
    put("Plain.lua","GameEvents.Plain.Add(function() return true end)");
    {
        auto owned=owner_for();LuaScriptHost child;child.enable_ui_context_sources(owned);
        auto graph=std::make_shared<ScriptUiControlGraph>("Root",
            civ5::content::read_xml_metadata("<Context><Container ID='Box'/></Context>"),[]{});
        child.enable_ui_control_graph(graph);
        child.execute("Root",R"lua(
            assert(type(ContextPtr.BuildInstance)=='function' and type(ContextPtr.DestroyChild)=='function')
            local before=ContextPtr
            local loaded=ContextPtr:LoadNewContext('plain')
            assert(rawequal(before,ContextPtr) and type(ContextPtr.BuildInstance)=='function')
            assert(type(loaded.LoadNewContext)=='function' and Controls.Box~=nil)
        )lua","=graph-receiver-preserved");
        child.complete_loading();
        check(child.loading_complete(),"Graph receiver must gain loader without replacing graph methods");
    }
    failure("pcall(function() ContextPtr:LoadNewContext('plain') end)",[&](Owner &owned,LuaScriptHost &){
        owned.on_resolve=[&]{lease.reset();};
    });
    lease=std::make_shared<Lease>();
    {
        auto owned=owner_for();ScriptUiLayoutProvider layout;layout.context_id="Root";
        layout.controls={{"Box","Container",false,true}};
        layout.screen_size=[] {return std::pair<std::int64_t,std::int64_t>{10,10};};
        layout.get_size=[](std::string_view){return std::pair<std::int64_t,std::int64_t>{1,1};};
        layout.set_size=[](std::string_view,std::int64_t,std::int64_t){};
        layout.set_offset=[](std::string_view,std::int64_t,std::int64_t){};
        layout.calculate_internal_size=[](std::string_view){};
        layout.callback_events={1};layout.callback_owner_current=[] {return true;};
        LuaScriptHost child({},{},{},{},{},{},{},{},{},{},{},{},layout);
        child.enable_ui_context_handlers({"Root",[] {return false;},[](bool){}});
        child.enable_ui_context_sources(owned);
        auto frame_lease=std::make_shared<Lease>();auto updates=std::make_shared<Updates>();
        updates->source=owned.get();updates->lease=frame_lease;child.enable_ui_updates(updates);
        child.execute("Root",R"lua(
            local before=ContextPtr
            assert(type(before.IsHidden)=='function' and not before:IsHidden())
            assert(type(before.SetInputHandler)=='function' and type(before.SetShowHideHandler)=='function')
            local loaded=before:LoadNewContext('plain')
            assert(rawequal(before,ContextPtr) and not before:IsHidden())
            assert(type(loaded.LoadNewContext)=='function')
            before:SetUpdate(function(dt) assert(rawequal(before,ContextPtr) and not before:IsHidden()); print('layout update',dt) end)
        )lua","=layout-receiver-preserved");
        child.complete_loading();
        check(child.loading_complete(),"Layout receiver methods must survive loader installation");
        updates->issue(1,0.25);const auto result=child.dispatch_ui_update();
        check(result.errors.empty()&&result.listeners==1&&child.messages().size()==1&&child.messages()[0].text=="layout update\t0.25",
              "SetUpdate must augment the genuine layout receiver and retain its current handlers");
    }
    {
        auto owned=owner_for();LuaScriptHost child;child.enable_ui_context_sources(owned);
        child.execute("Root","retained=ContextPtr:LoadNewContext('plain')","=retire");
        child.close_ui_context("Owned-plain");rejects([&]{child.test_all("Plain");});rejects([&]{child.complete_loading();});
    }
    {
        auto owned=owner_for();auto frame_lease=std::make_shared<Lease>();auto updates=std::make_shared<Updates>();
        updates->source=owned.get();updates->lease=frame_lease;
        LuaScriptHost child;child.enable_ui_context_sources(owned);child.enable_ui_updates(updates);
        child.execute("Root",R"lua(
            GameEvents.UpdateShared.Add(function() return 1 end)
            local kept=ContextPtr
            ContextPtr:SetUpdate(function(dt)
                assert(rawequal(kept,ContextPtr) and type(dt)=='number')
                assert(GameEvents.UpdateShared.AccumulateINT()==11)
                print('root update',dt)
            end)
            retained=ContextPtr:LoadNewContext('update')
        )lua","=update-shared-vm");
        check(child.registered_ui_updates()==2,"Real retained parent and child closures must survive top-level return");
        rejects([&]{child.dispatch_ui_update();});check(updates->consumes==0,"Pre-completion dispatch cannot consume a frame");
        child.complete_loading();updates->issue(1,0.25);
        auto result=child.dispatch_ui_update();
        check(result.errors.empty()&&result.listeners==2&&updates->consumes==1&&!updates->pending,
              "Exactly one owned frame must deliver numeric dt to both existing contexts");
        check(child.messages().size()==2&&child.messages()[0].context=="Root"&&child.messages()[1].context=="Owned-update"&&
              child.messages()[0].text=="root update\t0.25"&&child.messages()[1].text=="child update\t0.25",
              "Shared GameEvents and retained environments must deliver in generic context installation order");
        result=child.dispatch_ui_update();
        check(result.errors.empty()&&result.listeners==0&&updates->consumes==2&&child.messages().size()==2,
              "No pending frame must not replay a consumed callback batch");
        child.execute("Root","ContextPtr:SetUpdate(function(dt) print('replacement',dt); ContextPtr:SetUpdate(nil) end)","=replace-update");
        child.complete_loading();updates->issue(2,0);
        result=child.dispatch_ui_update();
        check(result.errors.empty()&&result.listeners==2&&child.registered_ui_updates()==1,
              "Replacement must retain the running closure through generic self-unregister");
        updates->issue(3,0.5);result=child.dispatch_ui_update();
        check(result.errors.empty()&&result.listeners==1&&child.messages().size()==5&&
              child.messages()[2].text=="replacement\t0"&&child.messages()[4].context=="Owned-update",
              "Unregister must take effect on the next frame and leave the child callback intact");
        child.close_ui_context("Owned-update");
        check(child.registered_ui_updates()==0&&!child.loading_complete(),"Close must release the actual child callback and retire readiness");
        rejects([&]{child.dispatch_ui_update();});
    }
    // Commands below are a real synchronous test-owned service. Lease release
    // and caught reentry happen inside the actual Lua callback, not a DTO probe.
    const auto update_failure=[&](std::string_view body,
        std::function<void(LuaScriptHost&,Updates&,std::shared_ptr<Lease>&)> action={},
        std::function<void(Updates&,LuaScriptHost&)> setup={}) {
        auto owned=owner_for();auto frame_lease=std::make_shared<Lease>();auto updates=std::make_shared<Updates>();
        updates->source=owned.get();updates->lease=frame_lease;LuaScriptHost *host_ptr=nullptr;
        ScriptUiEventProvider events;events.commands={"UpdateAction"};
        events.command=[&](std::string_view,std::span<const ScriptValue>){if(action)action(*host_ptr,*updates,frame_lease);};
        LuaScriptHost child({},{},{},{},{},{},{},events);host_ptr=&child;
        child.enable_ui_context_sources(owned);child.enable_ui_updates(updates);
        child.execute("Root",body,"=update-failure");child.complete_loading();updates->issue(1,0.25);
        if(setup)setup(*updates,child);
        const auto result=child.dispatch_ui_update();
        check(!result.errors.empty()&&!child.loading_complete()&&child.messages().empty(),
              "Update error/currentness failure must be sticky and stop the next retained callback");
        rejects([&]{child.complete_loading();});rejects([&]{child.dispatch_ui_update();});
    };
    const std::string later_child=R"lua(
        GameEvents.UpdateShared.Add(function() return 1 end)
        ContextPtr:LoadNewContext('update')
    )lua";
    update_failure("ContextPtr:SetUpdate(function(dt) error('update failed') end)"+later_child);
    update_failure("ContextPtr:SetUpdate(function(dt) coroutine.yield() end)"+later_child);
    update_failure("ContextPtr:SetUpdate(function(dt) ContextPtr={} end)"+later_child);
    update_failure("local f; f=function(dt) setfenv(f,{}) end; ContextPtr:SetUpdate(f)"+later_child);
    update_failure("ContextPtr:LoadNewContext('update-rebound')");
    update_failure("ContextPtr:SetUpdate(function(dt) Events.UpdateAction() end)"+later_child,
        [](LuaScriptHost&,Updates&,std::shared_ptr<Lease> &l){l.reset();});
    update_failure("ContextPtr:SetUpdate(function(dt) Events.UpdateAction() end)"+later_child,
        [](LuaScriptHost &h,Updates&,std::shared_ptr<Lease>&){try{h.dispatch_ui_update();}catch(...){}});
    update_failure("ContextPtr:SetUpdate(function(dt) Events.UpdateAction() end)"+later_child,
        [](LuaScriptHost &h,Updates&,std::shared_ptr<Lease>&){try{h.close_ui_context("Root");}catch(...){}});
    update_failure("ContextPtr:SetUpdate(function(dt) Events.UpdateAction() end)"+later_child,
        [](LuaScriptHost&,Updates &u,std::shared_ptr<Lease>&){u.rebind_observation=true;});
    update_failure("ContextPtr:SetUpdate(function(dt) Events.UpdateAction() end)"+later_child,
        [&](LuaScriptHost&,Updates&,std::shared_ptr<Lease>&){lease.reset();});
    lease=std::make_shared<Lease>();
    update_failure("ContextPtr:SetUpdate(function(dt) end)",{},[](Updates &u,LuaScriptHost &h){
        u.on_consume=[&h]{try{h.complete_loading();}catch(...){}};
    });
    update_failure("ContextPtr:SetUpdate(function(dt) end)",{},[](Updates &u,LuaScriptHost &h){
        u.on_frame_check=[&h]{try{h.execute("Root","assert(true)","=frame-check-reentry");}catch(...){}};
    });
    update_failure("ContextPtr:SetUpdate(function(dt) end)",{},[](Updates &u,LuaScriptHost &){u.on_consume=[]{throw 7;};});
    update_failure("ContextPtr:SetUpdate(function(dt) end)",{},[](Updates &u,LuaScriptHost &){u.delta=-1;});
    update_failure("ContextPtr:SetUpdate(function(dt) end)",{},[](Updates &u,LuaScriptHost &){u.delta=std::numeric_limits<double>::quiet_NaN();});
    update_failure("ContextPtr:SetUpdate(function(dt) end)",{},[](Updates &u,LuaScriptHost &){u.delta=std::numeric_limits<double>::infinity();});
    {
        auto owned=owner_for();auto frame_lease=std::make_shared<Lease>();auto updates=std::make_shared<Updates>();
        updates->source=owned.get();updates->lease=frame_lease;
        LuaScriptHost child;child.enable_ui_context_sources(owned);child.enable_ui_updates(updates);
        child.execute("Root","ContextPtr:SetUpdate(function(dt) end)","=update-freshness");child.complete_loading();
        updates->issue(8,0.1);check(child.dispatch_ui_update().errors.empty(),"First genuine frame must run");
        updates->issue(8,0.1);check(!child.dispatch_ui_update().errors.empty()&&!child.loading_complete(),
                                  "A second pending observation with an already consumed sequence must refuse");
    }
    {
        auto owned=owner_for();auto other=owner_for();auto frame_lease=std::make_shared<Lease>();auto updates=std::make_shared<Updates>();
        updates->source=other.get();updates->lease=frame_lease;
        LuaScriptHost child;child.enable_ui_context_sources(owned);
        rejects([&]{child.enable_ui_updates(updates);});check(!child.loading_complete(),"Foreign source channel cannot issue updates");
    }
    update_failure("ContextPtr:SetUpdate(function(dt) end)",{},[](Updates &u,LuaScriptHost &){
        // Another real generic service is the wrong issuer even if values match.
        static Updates other;u.foreign_issuer=&other;
    });
    const auto update_registration_failure=[&](std::string_view body,std::function<void(Updates&,LuaScriptHost&)> setup={}) {
        auto owned=owner_for();auto frame_lease=std::make_shared<Lease>();auto updates=std::make_shared<Updates>();
        updates->source=owned.get();updates->lease=frame_lease;
        LuaScriptHost child;child.enable_ui_context_sources(owned);child.enable_ui_updates(updates);
        if(setup)setup(*updates,child);
        rejects([&]{child.execute("Root",body,"=update-registration-refusal");});
        rejects([&]{child.complete_loading();});check(updates->consumes==0,"Caught invalid setter cannot grant a frame delivery");
    };
    update_registration_failure("pcall(function() ContextPtr:SetUpdate(loadstring('return true')) end)");
    update_registration_failure("local f=function(dt) end; setfenv(f,{}); pcall(function() ContextPtr:SetUpdate(f) end)");
    update_registration_failure("pcall(function() ContextPtr.SetUpdate({},function(dt) end) end)");
    update_registration_failure("retained=ContextPtr:LoadNewContext('plain'); pcall(function() retained:SetUpdate(function(dt) end) end)");
    update_registration_failure("pcall(function() ContextPtr:SetUpdate(function(dt) end) end)",[](Updates &u,LuaScriptHost &h){
        u.on_check=[&h]{try{h.complete_loading();}catch(...){}};
    });
    update_registration_failure("pcall(function() ContextPtr:SetUpdate(function(dt) end) end)",[](Updates &u,LuaScriptHost &){u.on_check=[]{throw 7;};});
    {
        auto owned=owner_for();auto frame_lease=std::make_shared<Lease>();auto updates=std::make_shared<Updates>();
        updates->source=owned.get();updates->lease=frame_lease;
        LuaScriptHost child;child.enable_ui_context_sources(owned);child.enable_ui_updates(updates);
        child.execute("Root",R"lua(
            ContextPtr:SetUpdate(function(dt)
                print('first closure')
                ContextPtr:SetUpdate(function(nextDt) print('next closure',nextDt) end)
            end)
        )lua","=update-self-replace");child.complete_loading();
        updates->issue(1,0.25);check(child.dispatch_ui_update().errors.empty(),"A running retained closure may replace its next callback");
        updates->issue(2,0.5);check(child.dispatch_ui_update().errors.empty()&&child.messages().size()==2&&
              child.messages()[0].text=="first closure"&&child.messages()[1].text=="next closure\t0.5",
              "The replacement must run only in the next owned frame");
    }
    {
        auto owned=owner_for();auto frame_lease=std::make_shared<Lease>();auto updates=std::make_shared<Updates>();
        updates->source=owned.get();updates->lease=frame_lease;
        auto graph=std::make_shared<ScriptUiControlGraph>("Root",
            civ5::content::read_xml_metadata("<Context><Container ID='Box'/></Context>"),[]{});
        ScriptUiEventProvider events;events.commands={"RetireUpdateGraph"};events.command=[&](std::string_view,std::span<const ScriptValue>){graph->retire();};
        LuaScriptHost child({},{},{},{},{},{},{},events);child.enable_ui_context_sources(owned);child.enable_ui_updates(updates);child.enable_ui_control_graph(graph);
        child.execute("Root",R"lua(
            assert(type(ContextPtr.BuildInstance)=='function' and Controls.Box~=nil)
            ContextPtr:SetUpdate(function(dt) Events.RetireUpdateGraph() end)
        )lua","=update-graph-receiver");child.complete_loading();updates->issue(1,0.25);
        check(!child.dispatch_ui_update().errors.empty()&&!child.loading_complete(),"Actual graph retirement inside update must revoke the receiver");
    }
    {
        auto owned=owner_for();LuaScriptHost child;child.enable_ui_context_sources(owned);
        child.execute("Root","ContextPtr:LoadNewContext('plain')","=lease");
        owned->on_check=[&]{lease.reset();};
        rejects([&]{child.test_all("Plain");});rejects([&]{child.complete_loading();});
        check(!child.loading_complete(),"Callback releasing only external lease must revoke retained child");
    }
    // Temp fixture cleanup only; this test does not touch installed assets.
    std::filesystem::remove_all(root);
}
// Generic host query tests; no selected map/gameplay/UI issuer is supplied.
void lua_map_init_query_tests() {
    using namespace civ5::runtime;
    using namespace civ5::assets;
    struct Lease {};
    struct Source final:ScriptUiChildSource {
        std::weak_ptr<Lease> lease;AssetHandle handle;Bytes pin;
        Source(std::weak_ptr<Lease> l,AssetHandle h):lease(l),handle(std::move(h)),pin(handle.read_bytes()){}
        std::string_view context()const override{return "Child";}
        const AssetHandle& asset()const override{return handle;}
        void require_current()const override {
            if(lease.expired()||handle.read_bytes()!=pin)throw ScriptError("Query child source retired/changed");
        }
        void require_captured_bytes(std::span<const std::uint8_t> value)const override {
            require_current();if(value.size()!=pin.size()||!std::equal(value.begin(),value.end(),pin.begin()))
                throw ScriptError("Query child bytes differ from exact source pin");
        }
    };
    struct Owner final:ScriptUiContextSourceOwner {
        std::weak_ptr<Lease> lease;std::shared_ptr<Source> child;
        mutable std::function<void()> callback;
        void require_current(std::string_view context,const ScriptUiChildSource *source)const override {
            if(lease.expired())throw ScriptError("Query owner expired");
            if(callback)callback();
            if(lease.expired())throw ScriptError("Query owner expired in callback");
            if(source?(source!=child.get()||context!="Child"):context!="Root")
                throw ScriptError("Query foreign source identity");
        }
        std::shared_ptr<const ScriptUiChildSource> resolve_child(std::string_view parent,
            const ScriptUiChildSource *source,std::string_view request)override {
            require_current(parent,source);if(request!="child"||!child)throw ScriptError("Unknown query child");
            return child;
        }
    };
    const auto expect_throw=[](auto&& action,const char *message) {
        bool rejected=false;try{action();}catch(...){rejected=true;}check(rejected,message);
    };
    const ScriptMapInitFields initial{80,52,true,false,90,-90};
    auto lease=std::make_shared<Lease>();
    const auto owner_for=[&] {auto p=std::make_shared<Owner>();p->lease=lease;return p;};
    const auto query=[&](LuaScriptHost &vm,const Owner &owner) {
        return vm.query_map_init_data("Root",owner,4,initial);
    };
    for(const auto source:{"", "GetMapInitData=0", "GetMapInitData=setmetatable({}, {__call=function() error('must not invoke nonfunction') end})",
        "function GetMapInitData(id) assert(id==4); return nil end",
        "function GetMapInitData(id) assert(id==4); return 23 end"}) {
        auto owner=owner_for();LuaScriptHost vm;vm.enable_ui_context_sources(owner);
        vm.execute("Root",source,"=query-fallback");
        const auto result=query(vm,*owner);
        check(!result.table_result&&result.fields==initial,"Native query fallback preserves caller initial fields");
    }
    {
        auto owner=owner_for();LuaScriptHost vm;vm.enable_ui_context_sources(owner);
        vm.execute("Root",R"lua(
            local loadedThread=coroutine.running()
            local sequence={'Width','Height','WrapX','WrapY','TopLatitude','BottomLatitude'}
            local calls=0
            GameEvents.MapQuery.Add(function() return calls end)
            function GetMapInitData(id)
                assert(id==4 and coroutine.running()==loadedThread); calls=calls+1
                local n=0
                return setmetatable({}, {__index=function(t,key)
                    n=n+1; assert(key==sequence[n] and coroutine.running()==loadedThread); print('field',key)
                    if key=='Width' then return '17' end
                    if key=='WrapX' then return 0 end
                    if key=='WrapY' then return false end
                    if key=='TopLatitude' then return 'not-a-number' end
                    if key=='BottomLatitude' then return true end
                end})
            end
        )lua","=query-field-order");
        auto result=query(vm,*owner);
        check(result.table_result&&result.fields==ScriptMapInitFields{17,52,true,false,0,0},
            "Sequential native field reads preserve nil, numeric strings, Lua truth and tonumber zero");
        check(vm.messages().size()==6&&vm.messages()[2].text=="field\tWrapX", "Real __index effects retained in SDK order");
        check(vm.accumulate_int("MapQuery").value==1,"Query uses SAME existing GameEvents registry");
        check(query(vm,*owner).table_result&&vm.accumulate_int("MapQuery").value==2,"Query is a new call, not snapshot replay");
        vm.execute("Root","assert(GetMapInitData(4).Width=='17')","=query-followup-same-thread");
        check(query(vm,*owner).table_result&&vm.accumulate_int("MapQuery").value==4,
            "Repeated owned-context execution and query retain the original thread identity");
    }
    {
        auto owner=owner_for();LuaScriptHost vm;vm.enable_ui_context_sources(owner);
        vm.execute("Root","function GetMapInitData(id) return {} end","=query-empty-table");
        const auto result=query(vm,*owner);
        check(result.table_result&&result.fields==initial,"Empty native table is successful with six preserved initials");
    }
    {
        auto owner=owner_for();LuaScriptHost vm;vm.enable_ui_context_sources(owner);
        vm.execute("Root",R"lua(
            local seen=0
            -- Keep the ACTUAL inherited base for keys outside this test lookup.
            -- print intentionally reads env.tostring and respects overrides.
            local inherited=getmetatable(getfenv()).__index
            assert(type(inherited)=='table' and type(inherited.tostring)=='function')
            setmetatable(getfenv(), {__index=function(t,key)
                if key=='GetMapInitData' then
                    seen=seen+1; print('lookup',seen)
                    return function(id) return {Width=id+seen} end
                end
                return inherited[key]
            end})
        )lua","=query-global-effects");
        const auto result=query(vm,*owner);
        check(result.table_result&&result.fields.width==5&&vm.messages().size()==1,
            "Native global lookup retains real __index effects in the context environment");
    }
    {
        auto owner=owner_for();LuaScriptHost vm;vm.enable_ui_context_sources(owner);
        vm.execute("Root","function GetMapInitData(id) return {WrapX=false,WrapY=0} end","=query-wrap-truth");
        const auto result=query(vm,*owner);
        check(result.table_result&&!result.fields.wrap_x&&result.fields.wrap_y,
            "Lua false overwrites true initial; numeric zero overwrites false initial as true");
    }
    {
        auto owner=owner_for();LuaScriptHost vm;vm.enable_ui_context_sources(owner);
        vm.execute("Root",R"lua(
            tostring=function(value) return 'override:'..value end
            function GetMapInitData(id) print('kept'); return {Width=id} end
        )lua","=query-print-override");
        const auto result=query(vm,*owner);
        check(result.table_result&&result.fields.width==4&&vm.messages().size()==1&&
              vm.messages()[0].text=="override:kept",
              "Actual per-context tostring override must still govern print");
    }
    // These deliberately lose tostring. They are errors in the SCRIPT, not
    // permission for the host to install a hidden default or suppress failure.
    for(const auto body:{
        "tostring=false; function GetMapInitData(id) print('bad'); return {} end",
        "setmetatable(getfenv(),{__index=function(t,key) if key=='GetMapInitData' then print('bad'); return function(id) return {} end end end})"}) {
        auto owner=owner_for();LuaScriptHost vm;vm.enable_ui_context_sources(owner);
        vm.execute("Root",body,"=query-print-unavailable");
        bool exact=false;
        try{(void)query(vm,*owner);}catch(const ScriptError& error){
            exact=std::string(error.what()).find("tostring")!=std::string::npos;
        }
        check(exact&&vm.messages().empty()&&!vm.loading_complete(),
              "Explicit tostring override/lost inheritance must fail before print publication");
        expect_throw([&]{vm.complete_loading();},"Print failure remains sticky, not a successful map query");
    }
    // Reused source_integer policy follows process FP mode, including ties.
    // This test does not independently prove the shipped Lua DLL conversion.
    struct RoundRestore {int previous=std::fegetround();~RoundRestore(){std::fesetround(previous);}} round_restore;
    for(const auto mode:{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO}) {
        check(std::fesetround(mode)==0,"Query test FP mode available");
        auto owner=owner_for();LuaScriptHost vm;vm.enable_ui_context_sources(owner);
        vm.execute("Root",R"lua(function GetMapInitData(id)
            return {Width=1.5,Height=-1.5,TopLatitude=2.5,BottomLatitude=-2.5}
        end)lua","=query-fistp");
        const auto result=query(vm,*owner);
        const ScriptMapInitFields expected=mode==FE_TONEAREST?ScriptMapInitFields{2,-2,true,false,2,-2}:
            mode==FE_DOWNWARD?ScriptMapInitFields{1,-2,true,false,2,-3}:
            mode==FE_UPWARD?ScriptMapInitFields{2,-1,true,false,3,-2}:ScriptMapInitFields{1,-1,true,false,2,-2};
        check(result.fields==expected,
              "Query conversion retains existing host FP-mode rounding for both signs and ties");
    }
    check(std::fesetround(FE_TONEAREST)==0,"Reset FP mode before boundary tests");
    {
        auto owner=owner_for();LuaScriptHost vm;vm.enable_ui_context_sources(owner);
        vm.execute("Root","function GetMapInitData(id) return {Width=2147483647,Height=-2147483648} end","=query-int32");
        const auto result=query(vm,*owner);
        check(result.fields.width==std::numeric_limits<std::int32_t>::max()&&result.fields.height==std::numeric_limits<std::int32_t>::min(),"Supported native int32 endpoints remain exact");
    }
    for(const auto source:{
        "function GetMapInitData(id) return {Width=2147483648} end",
        "function GetMapInitData(id) return {Width=-2147483649} end",
        "function GetMapInitData(id) return {Width=0/0} end",
        "function GetMapInitData(id) return {Width=math.huge} end",
        "function GetMapInitData(id) error('query failure') end",
        "function GetMapInitData(id) coroutine.yield() end",
        "function GetMapInitData(id) ContextPtr={}; return {} end",
        "function GetMapInitData(id) setfenv(0,{}); return {} end",
        "function GetMapInitData(id) return {} end; setfenv(0,{})",
        "function GetMapInitData(id) return setmetatable({}, {__index=function(t,k) if k=='WrapX' then error('third field') end; print(k); return 11 end}) end",
        "function GetMapInitData(id) return setmetatable({}, {__index=function(t,k) ContextPtr={}; return 11 end}) end"}) {
        auto owner=owner_for();LuaScriptHost vm;vm.enable_ui_context_sources(owner);
        vm.execute("Root",source,"=query-failure");vm.complete_loading();
        expect_throw([&]{(void)query(vm,*owner);},"Query error/yield/rebind/range refuses snapshot");
        if(std::string_view(source).find("third field")!=std::string_view::npos)
            check(vm.messages().size()==2&&vm.messages()[0].text=="Width"&&vm.messages()[1].text=="Height",
                "Third field error stops later reads; preceding effects retained without a snapshot/replay");
        check(!vm.loading_complete(),"Failed query clears ready and cannot publish a partial result");
        expect_throw([&]{vm.complete_loading();},"Query failure is sticky, not replayable");
    }
    {
        auto owner=owner_for();LuaScriptHost vm;vm.enable_ui_context_sources(owner);
        vm.execute("Root","setmetatable(getfenv(), {__index=function(t,k) if k=='GetMapInitData' then error('global lookup') end end})","=query-global-index");
        expect_throw([&]{(void)query(vm,*owner);},"Global __index lookup itself is protected");
    }
    {
        auto owner=owner_for();LuaScriptHost vm;vm.enable_ui_context_sources(owner);
        expect_throw([&]{(void)query(vm,*owner);},"Query never silently creates a missing retained context");
    }
    {
        auto owner=owner_for();auto foreign=owner_for();LuaScriptHost vm;vm.enable_ui_context_sources(owner);
        vm.execute("Root","function GetMapInitData(id) return {} end","=query-foreign");
        expect_throw([&]{(void)query(vm,*foreign);},"Same-looking foreign owner cannot authorize query");
    }
    {
        auto owner=owner_for();LuaScriptHost vm;vm.enable_ui_context_sources(owner);
        vm.execute("Root","function GetMapInitData(id) return {} end","=query-reentry");
        bool nested=false;owner->callback=[&]{if(nested)return;nested=true;
            expect_throw([&]{(void)query(vm,*owner);},"Nested query refuses host reentry");};
        expect_throw([&]{(void)query(vm,*owner);},"Provider cannot catch reentry and publish outer snapshot");
        owner->callback={};
    }
    {
        auto owner=owner_for();LuaScriptHost vm;vm.enable_ui_context_sources(owner);
        vm.execute("Root","function GetMapInitData(id) return {} end","=query-expired");
        owner->callback=[&]{lease.reset();};
        expect_throw([&]{(void)query(vm,*owner);},"Last external source lease drop during callback refuses query");
        owner->callback={};lease=std::make_shared<Lease>();
    }
    {
        auto owner=owner_for();LuaScriptHost vm;vm.enable_ui_context_sources(owner);
        vm.execute("Root","function GetMapInitData(id) return {} end","=query-thread");
        bool rejected=false;std::thread worker([&]{try{(void)query(vm,*owner);}catch(...){rejected=true;}});worker.join();
        check(rejected&&!vm.loading_complete(),"Query requires the original VM owner thread");
    }
    {
        auto owner=owner_for();LuaScriptHost vm;vm.enable_ui_context_sources(owner);
        vm.execute("Root",R"lua(function GetMapInitData(id)
            return setmetatable({}, {__index=function(t,key) print(key); return 12 end})
        end)lua","=query-field-retirement");
        owner->callback=[&]{if(vm.messages().size()>=2)lease.reset();};
        expect_throw([&]{(void)query(vm,*owner);},"Real second-field effect followed by lease retirement refuses partial snapshot");
        check(vm.messages().size()==2,"Lease failure stops later __index fields after real preceding effects");
        owner->callback={};lease=std::make_shared<Lease>();
    }
    {
        auto owner=owner_for();LuaScriptHost vm({0,10000});vm.enable_ui_context_sources(owner);
        vm.execute("Root","function GetMapInitData(id) while true do end end","=query-budget");
        expect_throw([&]{(void)query(vm,*owner);},"Retained query thread obeys host instruction budget");
        expect_throw([&]{vm.complete_loading();},"Budget exhaustion is sticky");
    }
    {
        auto owner=owner_for();LuaScriptHost vm({128*1024,0});vm.enable_ui_context_sources(owner);
        vm.execute("Root","function GetMapInitData(id) local x=string.rep('x',1048576); return {Width=#x} end","=query-oom");
        expect_throw([&]{(void)query(vm,*owner);},"Target-thread Lua allocation failure remains protected and publishes no snapshot");
        check(!vm.loading_complete(),"Target-thread OOM is sticky");
    }
    const auto root=std::filesystem::temp_directory_path()/
        ("civis-map-query-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    const auto file=root/"Child.lua";
    {std::ofstream out(file,std::ios::binary);out<<R"lua(
        local loadedThread=coroutine.running()
        GameEvents.ChildQuery.Add(function() return 7 end)
        function GetMapInitData(id)
            assert(coroutine.running()==loadedThread and GameEvents.ParentQuery.AccumulateINT()==3)
            return {Width=id+GameEvents.ChildQuery.AccumulateINT()}
        end
    )lua";}
    auto index=std::make_shared<LooseResourceIndex>(root);index->add_file("Child.lua");
    {
        auto owner=owner_for();owner->child=std::make_shared<Source>(lease,std::get<AssetHandle>(index->find(AssetKey("Child.lua"))));
        LuaScriptHost vm;vm.enable_ui_context_sources(owner);
        vm.execute("Root","GameEvents.ParentQuery.Add(function() return 3 end); child=ContextPtr:LoadNewContext('child')","=query-load-order");
        const auto result=vm.query_map_init_data("Child",*owner,4,initial);
        check(result.table_result&&result.fields.width==11&&vm.accumulate_int("ChildQuery").value==7,
            "Actual retained child bytes/environment share existing parent events and load order");
        vm.close_ui_context("Child");
        expect_throw([&]{(void)vm.query_map_init_data("Child",*owner,4,initial);},"Retired child query refuses");
    }
    {
        auto owner=owner_for();owner->child=std::make_shared<Source>(lease,std::get<AssetHandle>(index->find(AssetKey("Child.lua"))));
        LuaScriptHost vm;vm.enable_ui_context_sources(owner);
        vm.execute("Root","GameEvents.ParentQuery.Add(function() return 3 end); child=ContextPtr:LoadNewContext('child')","=query-child-pin");
        {std::ofstream out(file,std::ios::binary|std::ios::trunc);out<<"-- changed real child source";}
        expect_throw([&]{(void)vm.query_map_init_data("Child",*owner,4,initial);},"Retained child source byte mutation refuses before query");
    }
    std::filesystem::remove_all(root);
}

// Actual calls on one retained generic VM, not selected startup authority.
void lua_execution_ledger_tests() {
    using namespace civ5::runtime;
    LuaScriptHost vm;
    vm.execute("Ledger",R"lua(
        local keep=function() return 7 end
        local absent=function() return 9 end
        GameEvents.Ledger.Add(keep)
        GameEvents.Ledger.Remove(absent)
        GameEvents.Ledger.Remove(keep)
        GameEvents.Ledger.Add(keep)
        GameEvents.Ledger.Add(keep)
        GameEvents.Ledger.RemoveAll()
        assert(GameEvents.Ledger.Count()==0)
        GameEvents.Fault.Add(function() error('ledger callback') end)
        GameEvents.Fault.Add(function() return true end)
    )lua","=ledger-root");
    const auto result=vm.test_all("Fault");
    check(result.value==true&&result.errors.size()==1,"Journal must preserve callback-error and return semantics");
    const auto before=vm.execution_snapshot();
    check(before.host_allocation!=nullptr&&!before.records.empty()&&!vm.loading_complete(),"Diagnostic snapshot cannot attest startup");
    std::vector<ScriptLuaExecutionRecord> removals;
    bool cleared=false,dispatch_error=false;
    std::uint64_t previous=0;
    for(const auto &record:before.records) {
        check(record.begin>previous&&record.end>record.begin,"Journal begin order and actual completion order must be monotone");
        previous=record.begin;
        if(record.action==ScriptLuaAction::event_remove)removals.push_back(record);
        if(record.action==ScriptLuaAction::event_clear)
            cleared=record.count_before==2&&record.count_after==0&&record.removed==2;
        if(record.action==ScriptLuaAction::event_dispatch&&record.name=="Fault")
            dispatch_error=record.outcome==ScriptLuaActionOutcome::error&&record.count_after==2;
    }
    check(removals.size()==2&&removals[0].removed==0&&removals[0].count_after==1&&
          removals[1].removed==1&&removals[1].count_after==0&&cleared&&dispatch_error,
          "Absent removal, real removal, clear and swallowed callback error must remain distinct actual records");
    vm.complete_loading();
    check(vm.loading_complete(),"A protected event callback error is not an incomplete source load");
    std::exception_ptr foreign_completion;
    std::thread completion_thread([&]{try{vm.complete_loading();}catch(...){foreign_completion=std::current_exception();}});
    completion_thread.join();
    check(foreign_completion!=nullptr&&vm.loading_complete(),"Foreign completion must refuse before touching owner-thread loading state");
    rejects([&]{vm.execute("Ledger","retainedWrite=19; error('root failure')","=ledger-failed-root");});
    auto failed=vm.execution_snapshot();
    check(failed.host_allocation==before.host_allocation&&failed.failed&&failed.records.size()>before.records.size()&&
          failed.records.back().outcome==ScriptLuaActionOutcome::error,"Failure must retain SAME VM and completed prior actions");
    vm.execute("Ledger","assert(retainedWrite==19)","=ledger-retained-write");
    failed.records.clear();
    check(!vm.execution_snapshot().records.empty(),"Copied diagnostic records cannot erase retained history");
    std::exception_ptr foreign;
    std::thread thread([&]{try{(void)vm.execution_snapshot();}catch(...){foreign=std::current_exception();}});thread.join();
    check(foreign!=nullptr,"Journal readback must preserve the owner-thread boundary");

    bool inside=false;LuaScriptHost *same=nullptr;
    ScriptSourceProvider provider{[](std::string_view){return std::vector<std::string>{"Caught.lua"};},
        [&](std::string_view,std::string_view){
            inside=true;rejects([&]{same->execute("Ledger","assert(false)","=caught-reentry");});
            return std::string("includeWrite=(includeWrite or 0)+1");}};
    LuaScriptHost uncertain({},provider);same=&uncertain;
    uncertain.execute("Ledger","GameEvents.Caught.Add(function() include('Caught'); return true end)","=register-caught");
    check(uncertain.test_all("Caught").value==true&&inside,"Caught provider reentry must preserve actual original dispatch result");
    const auto journal=uncertain.execution_snapshot();bool marked=false;
    for(const auto &record:journal.records)if(record.action==ScriptLuaAction::event_dispatch&&record.name=="Caught")
        marked=record.outcome==ScriptLuaActionOutcome::uncertain;
    check(journal.uncertainty_epoch>0&&marked,"Swallowed VM reentry must not become an ordinary successful journal dispatch");
    rejects([&]{uncertain.complete_loading();});
    check(!uncertain.loading_complete(),"Caught VM reentry cannot be accepted as a complete generic source load");

    LuaScriptHost strict({}, {[](std::string_view){return std::vector<std::string>{"Strict.lua"};},
        [](std::string_view,std::string_view){return std::string("strictWrite=23; error('caught strict error')");},true});
    strict.execute("Ledger","assert(not pcall(function() include('Strict') end)); GameEvents.AfterStrict.Add(function() return true end); assert(strictWrite==23)","=strict-caught");
    const auto strict_journal=strict.execution_snapshot();bool unfinished=false,after_strict=false;
    for(const auto &record:strict_journal.records) {
        if(record.action==ScriptLuaAction::include)unfinished=record.outcome==ScriptLuaActionOutcome::incomplete;
        if(record.action==ScriptLuaAction::event_add)after_strict=record.name=="AfterStrict"&&
            record.parent==strict_journal.records.front().begin;
    }
    check(unfinished&&after_strict,"Caught direct include error must remain incomplete; later real registration belongs to its root");
    auto supplied_copy=strict_journal;supplied_copy.records.clear();
    rejects([&]{strict.complete_loading();});
    check(!strict.loading_complete()&&strict.execution_snapshot().host_allocation==strict_journal.host_allocation,
          "Caught strict include must revoke generic completion using SAME private history, despite a cleared diagnostic copy");
    strict.execute("Ledger","assert(strictWrite==23)","=retained-strict-stores");
    check(strict.test_all("AfterStrict").value==true,"Completion refusal must retain reached writes and actual listener registration");
    rejects([&]{strict.complete_loading();});

    LuaScriptHost previously_ready({}, {[](std::string_view){return std::vector<std::string>{"Strict.lua"};},
        [](std::string_view,std::string_view){return std::string("runtimeWrite=29; error('caught runtime include')");},true});
    previously_ready.execute("Ledger","initialLoad=11","=actual-initial-root");previously_ready.complete_loading();
    check(previously_ready.loading_complete(),"Successful actual root may complete generic loading");
    previously_ready.execute("Ledger","assert(not pcall(function() include('Strict') end));assert(initialLoad==11 and runtimeWrite==29)","=caught-after-completion");
    rejects([&]{previously_ready.complete_loading();});
    check(!previously_ready.loading_complete(),"Failed owner-thread completion must revoke previous generic readiness and retain actual writes");

    LuaScriptHost *active=nullptr;bool attempted_completion=false;
    LuaScriptHost interrupted({}, {[](std::string_view){return std::vector<std::string>{"Active.lua"};},
        [&](std::string_view,std::string_view){attempted_completion=true;rejects([&]{active->complete_loading();});return std::string("afterAttempt=37");}});
    active=&interrupted;
    // Completion reentry retires the real provider loan before compilation.
    // The returned text was never executed; only reached outer writes survive.
    interrupted.execute("Ledger","beforeAttempt=31;local matched=include('Active');assert(#matched==1 and matched[1]=='Active.lua' and afterAttempt==nil);afterOuterAttempt=41", "=caught-completion-reentry");
    check(attempted_completion&&!interrupted.loading_complete(),"Caught completion during actual provider must quarantine returned source and revoke readiness");
    interrupted.execute("Ledger","assert(beforeAttempt==31 and afterOuterAttempt==41 and afterAttempt==nil)","=completion-reentry-reached-outer-writes");
    const auto interrupted_history=interrupted.execution_snapshot();
    check(interrupted_history.failed&&interrupted_history.failure_epoch>0&&
          interrupted_history.uncertainty_epoch>0&&interrupted.source_errors().size()==1&&
          interrupted.source_errors()[0].context=="Ledger"&&interrupted.source_errors()[0].path=="Active.lua"&&
          interrupted.source_errors()[0].message=="Include provider compiler/source operation retired",
          "Caught generic completion must retain failure/uncertainty and the actual provider-loan refusal");
    bool refused_include=false,uncertain_root=false;
    for(const auto& record:interrupted_history.records){
        if(record.action==ScriptLuaAction::include&&record.name=="Active.lua")
            refused_include=record.outcome==ScriptLuaActionOutcome::error&&
                            record.source_sha256.empty()&&record.source_bytes==0;
        if(record.action==ScriptLuaAction::context_execute&&record.name=="=caught-completion-reentry")
            uncertain_root=record.outcome==ScriptLuaActionOutcome::uncertain;
    }
    check(refused_include&&uncertain_root,
          "Provider reentry must record refused uncompiled include and uncertain reached root, not successful returned-source execution");

    LuaScriptHost* ready_active=nullptr;bool ready_attempt=false;
    LuaScriptHost ready_interrupted({}, {[](std::string_view){return std::vector<std::string>{"Active.lua"};},
        [&](std::string_view,std::string_view){ready_attempt=true;rejects([&]{ready_active->complete_loading();});return std::string("afterReadyAttempt=43");}});
    ready_active=&ready_interrupted;
    ready_interrupted.execute("Ledger","beforeAttempt=19","=ready-before-completion-reentry");
    ready_interrupted.complete_loading();
    check(ready_interrupted.loading_complete(),"Previously ready generic fixture must genuinely complete first");
    ready_interrupted.execute("Ledger","local matched=include('Active');assert(#matched==1 and matched[1]=='Active.lua' and beforeAttempt==19 and afterReadyAttempt==nil);afterReadyOuterAttempt=47", "=caught-ready-completion-reentry");
    check(ready_attempt&&!ready_interrupted.loading_complete()&&ready_interrupted.execution_snapshot().failed&&
          ready_interrupted.source_errors().size()==1&&
          ready_interrupted.source_errors()[0].context=="Ledger"&&ready_interrupted.source_errors()[0].path=="Active.lua"&&
          ready_interrupted.source_errors()[0].message=="Include provider compiler/source operation retired",
          "Caught completion must preserve reached writes, quarantine uncompiled provider text and revoke previous readiness");
    ready_interrupted.execute("Ledger","assert(beforeAttempt==19 and afterReadyOuterAttempt==47 and afterReadyAttempt==nil)","=ready-completion-reentry-reached-outer-writes");
    rejects([&]{ready_interrupted.complete_loading();});
    rejects([&]{interrupted.complete_loading();});

    // Real Lua allocator pressure: stopped GC retains each protected call's
    // temporary C closure. No fault hook, mutable budget or VM getter is used.
    LuaScriptHost completion_budget({128*1024,0});
    completion_budget.execute("Ledger","retainedValue=41;collectgarbage('collect');collectgarbage('stop')","=completion-real-budget");
    completion_budget.complete_loading();
    const auto budget_history=completion_budget.execution_snapshot();
    bool completion_oom=false;
    for(unsigned attempt=0;attempt<16384;++attempt) {
        check(completion_budget.loading_complete(),"Preallocation failure scenario requires genuine prior completion");
        try {completion_budget.complete_loading();}
        catch(const ScriptError &error) {
            check(std::string(error.what()).find("memory")!=std::string::npos,"Completion pressure must reach actual Lua allocator failure, not an unrelated guard");
            completion_oom=true;break;
        }
    }
    const auto after_budget=completion_budget.execution_snapshot();
    check(completion_oom&&!completion_budget.loading_complete()&&
          after_budget.host_allocation==budget_history.host_allocation&&
          after_budget.records.size()==budget_history.records.size()&&!after_budget.failed,
          "Actual pre-perform Lua closure OOM must revoke old readiness without inventing an executed loading job or erasing source history");
}
void lua_script_host_tests() {
    lua_execution_ledger_tests();
    lua_child_context_tests();
    lua_map_init_query_tests();
    using namespace civ5::runtime;
    LuaScriptHost host;
    check(LuaScriptHost::version()=="Lua 5.1.5"&&host.memory_bytes()>0&&!host.loading_complete(),"Lua must be owned x64 VM with explicit loading state");
    host.execute("A","print('mod',5000000001,false)","=mod-print.lua");
    check(host.messages().size()==1&&host.messages()[0].context=="A"&&
          host.messages()[0].text=="mod\t5000000001\tfalse",
          "Lua output must be structured even before gameplay services are attached");
    check(!host.test_all("Empty").value&&host.test_all("Empty").errors.empty(),"No event listeners must return no result");
    host.execute("A",R"lua(
        local keys={-2147483648,2147483647,2147483648,-5000000001,5000000001,
                    -9007199254740992,9007199254740992,1.5,-1.5,math.huge,-math.huge,0}
        local values={}
        for i,key in ipairs(keys) do values[key]=i end
        -- Trigger array/hash growth, then both lookup and next() on wide keys.
        for i=1,1024 do values[i]=i+100 end
        for i,key in ipairs(keys) do assert(values[key]==i) end
        local count=0
        for key,value in pairs(values) do assert(values[key]==value); count=count+1 end
        assert(count==1024+#keys and values[0/0]==nil)
        assert(not pcall(function() values[0/0]=1 end))
        assert(values[5000000002]==nil and values[-5000000002]==nil)
        assert(string.format('%d %u %x',-5000000001,5000000001,5000000001)=='-5000000001 5000000001 12a05f201')
        for _,key in ipairs(keys) do values[key]=nil; assert(values[key]==nil) end
    )lua","=wide-table-keys.lua");
    check(true,"Wide, fractional and infinite Lua table keys must survive rehash, traversal and removal without int conversion overflow");
    host.execute("A",R"lua(
        assert(_VERSION == 'Lua 5.1')
        assert(getfenv(0) == _G)
        assert(io == nil and type(os.clock) == 'function' and os.execute == nil and package == nil and dofile == nil and loadfile == nil)
        assert(unpack({4,5}) == 4 and math.floor(3.9) == 3)
        marker = 19
        local nested = loadstring('return marker')
        assert(nested() == 19)
        hits = 0
        function yes(p,t,s,b,n,x)
            assert(p==5000000001 and t==-5000000001 and s=='a\0b' and b==false and n==nil and x==1.25)
            assert(coroutine.running() ~= nil and getfenv(0) == _G)
            hits=hits+1
            return 0
        end
        assert(GameEvents.Wide.Add(yes)==yes)
        GameEvents.Wide.Add(yes)
        assert(GameEvents.Wide.Count()==2)
        assert(GameEvents.Wide.Add()==nil and GameEvents.Wide.Count()==2)
    )lua","=wide.lua");
    const ScriptValue wide[]={std::int64_t(5000000001LL),std::int64_t(-5000000001LL),std::string("a\0b",3),false,std::monostate{},1.25};
    auto result=host.test_all("Wide",wide);
    check(result.value==true&&result.errors.empty(),"Binary64 wide integers, embedded NUL and Lua truthiness must cross native callback threads");
    host.execute("A","assert(hits==2); GameEvents.Wide.Remove(yes); assert(GameEvents.Wide.Count()==1)","=remove.lua");
    check(host.test_all("Wide",wide).value==true,"Adjacent duplicate removal must leave the next copy");
    host.execute("A","assert(hits==3); GameEvents.Wide.RemoveAll(); assert(GameEvents.Wide.Count()==0)","=clear.lua");
    check(!host.test_all("Wide",wide).value,"RemoveAll must leave no result");
    host.execute("A",R"lua(
        order=''
        GameEvents.Veto.Add(function() order=order..'a'; return false end)
        GameEvents.Veto.Add(function() order=order..'b'; return true end)
        GameEvents.Nil.Add(function() end)
        GameEvents.Fault.Add(function() error('mod callback failed') end)
        GameEvents.Fault.Add(function() order=order..'c'; return true end)
        GameEvents.Yield.Add(function() coroutine.yield() end)
    )lua","=veto.lua");
    check(host.test_all("Veto").value==false&&host.test_all("Nil").value==false,"TestAll combines false and nil without skipping callbacks");
    result=host.test_all("Fault");check(result.value==true&&result.errors.size()==1&&result.errors[0].find("mod callback failed")!=std::string::npos,"Native error aggregation skips failed result, records error and continues");
    host.execute("A","assert(order=='abc')","=order.lua");
    check(host.test_all("Yield").errors.size()==1,"Callbacks must be protected calls, not resumable coroutine yields");
    host.execute("B","assert(marker==nil); marker=27; GameEvents.Wide.Add(function() return true end)","=context.lua");
    host.execute("A","assert(marker==19 and GameEvents.Wide.Count()==0)","=isolated.lua");
    check(host.test_all("Wide").value==true,"Event dispatch must include other contexts without leaking globals or counts");
    host.execute("A",R"lua(
        lateHits=0
        local late=function() lateHits=lateHits+1; return true end
        GameEvents.Late.Add(function() GameEvents.Late.Add(late); return true end)
        local b=function() return true end
        selfRemove=function() GameEvents.RemoveDuring.Remove(selfRemove); return true end
        GameEvents.RemoveDuring.Add(selfRemove)
        GameEvents.RemoveDuring.Add(b)
    )lua","=mutation.lua");
    check(host.test_all("Late").value==true,"Add during dispatch must succeed");
    host.execute("A","assert(lateHits==0)","=late-first.lua");host.test_all("Late");
    host.execute("A","assert(lateHits==1)","=late-second.lua");
    result=host.test_all("RemoveDuring");check(result.value==true&&result.errors.size()==1,"Live shifted list and snapshotted count must expose removed final slot as callback error");
    const ScriptValue inexact[]={std::int64_t(9007199254740993LL)};rejects([&]{host.test_all("Wide",inexact);});
    const ScriptValue boundary[]={std::int64_t(9007199254740992LL)};
    host.execute("A","GameEvents.Boundary.Add(function(x) return x==9007199254740992 end)","=boundary.lua");
    check(host.test_all("Boundary",boundary).value==true,"Exact binary64 integer boundary must remain supported");
    const ScriptValue nan[]={std::numeric_limits<double>::quiet_NaN()};rejects([&]{host.test_all("Wide",nan);});
    host.complete_loading();check(host.loading_complete(),"Explicitly completed successful loading must become ready");
    host.execute("A","assert(true)","=later.lua");check(!host.loading_complete(),"New content must invalidate earlier loading attestation");
    host.complete_loading();LuaResearchSelectionServices bridge(host,[](std::int64_t){});
    check(!bridge.test_all("NoHandlers",0,1),"Research bridge must preserve absence of handlers");
    rejects([&]{bridge.test_all("Fault",0,1);});
    host.execute("A","GameEvents.PlayerCanEverResearch.Add(function(p,t) return t~=5000000001 end)","=restriction.lua");
    rejects([&]{bridge.test_all("PlayerCanEverResearch",5000000000LL,5000000001LL);});host.complete_loading();
    check(bridge.test_all("PlayerCanEverResearch",5000000000LL,5000000001LL)==false&&bridge.test_all("PlayerCanEverResearch",5000000000LL,7)==true,"Real Lua handlers must decide research on wide world IDs");
    LuaScriptHost broken;rejects([&]{broken.execute("A","beforeFailure=1; error('load failure')","=bad.lua");});
    broken.execute("A","assert(beforeFailure==1)","=retained.lua");rejects([&]{broken.complete_loading();});
    LuaScriptHost syntax;rejects([&]{syntax.execute("A","local =","=syntax.lua");});rejects([&]{syntax.complete_loading();});
    LuaScriptHost binary;rejects([&]{binary.execute("A",std::string("\x1bLua",4),"=binary");});
    LuaScriptHost bom;bom.execute("A","\xef\xbb\xbf" "assert(2+2==4)","=bom.lua");bom.complete_loading();check(bom.loading_complete(),"UTF-8 source BOM must be accepted");
    rejects([&]{LuaScriptHost tiny({1,0});});
    LuaScriptHost memory({128*1024,0});rejects([&]{memory.execute("A","x=string.rep('x',1048576)","=oom.lua");});
    memory.execute("A","assert(x==nil)","=after-oom.lua");check(memory.memory_bytes()<=128*1024,"OOM must remain protected and respect wide allocation accounting");
    LuaScriptHost bounded({0,10000});rejects([&]{bounded.execute("A","while true do end","=loop.lua");});
    bounded.execute("A","assert(true)","=after-loop.lua");
    rejects([&]{bounded.execute("A","while true do pcall(function() while true do end end) end","=caught-loop.lua");});
    LuaScriptHost callback_budget({0,10000});callback_budget.execute("A","GameEvents.Spin.Add(function() while true do end end)","=callback-loop.lua");
    rejects([&]{callback_budget.test_all("Spin");});
}
