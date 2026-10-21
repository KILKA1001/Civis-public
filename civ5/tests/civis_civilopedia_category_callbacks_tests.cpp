#include "civ5/ui/civis_civilopedia_category_callbacks.hpp"
#include <fstream>
#include <iostream>
#include <thread>
using namespace civ5::runtime;
using namespace civ5::ui;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F call){bool rejected=false;try{call();}catch(const ScriptError &){rejected=true;}check(rejected,"Expected explicit ScriptError rejection");}
ScriptUiLayoutProvider fixture(int &scroll_calls){
    ScriptUiLayoutProvider result;result.context_id="CivilopediaScreen";
    result.controls={{"ScrollPanel","ScrollPanel",true},{"LeftScrollPanel","ScrollPanel",true}};
    result.screen_size=[]{return std::pair<std::int64_t,std::int64_t>{1280,960};};
    result.get_size=[](auto){return std::pair<std::int64_t,std::int64_t>{1000,834};};
    result.set_size=[](auto,auto,auto){};result.set_offset=[](auto,auto,auto){};
    result.calculate_internal_size=[&](auto){++scroll_calls;};return result;
}
std::unique_ptr<LuaScriptHost> vm(ScriptUiLayoutProvider layout,ScriptUiEventProvider events={},ScriptBudget budget={}){
    ScriptEnumProvider enums;enums.tables={{"Mouse",{{"eLClick",77}}}};
    return std::make_unique<LuaScriptHost>(budget,ScriptSourceProvider{},ScriptModdingProvider{},
        ScriptGameplayProvider{},ScriptSaveDataProvider{},ScriptPregameProvider{},ScriptMapProvider{},
        std::move(events),ScriptContentProvider{},ScriptUserDataProvider{},std::move(enums),ScriptLocaleProvider{},std::move(layout));
}
std::string selected_category_block(const std::filesystem::path &root){
    std::ifstream file(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua");
    const std::string source{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    const auto declarations=source.find("local CategoryHomePage = 1;");
    const auto declarations_end=source.find("-- the instance managers",declarations);
    const auto registration=source.find("function SetSelectedCategory( thisCategory )");
    const auto registration_end=source.find("-- setup the special case stuff for each category",registration);
    check(declarations!=source.npos&&declarations_end!=source.npos&&registration!=source.npos&&registration_end!=source.npos,
          "Installed category registration source block changed");
    // Exact installed declarations, SetSelectedCategory body and loop. This
    // selected diagnostic chunk intentionally stops before unrelated services.
    return source.substr(declarations,declarations_end-declarations)+source.substr(registration,registration_end-registration);
}
}
int main(int argc,char **argv){
    check(argc==2,"Expected installed game root");const std::filesystem::path root=argv[1];
    const auto sources=inspect_bnw_civilopedia_layout_sources(root);check(sources.verified,"Selected installed source is unverified");
    CivilopediaLayoutOwnerKey identity;identity.frame_number=31;identity.owner_token="Civis:category-diagnostic";
    identity.player_id=0;identity.game_turn=123;identity.payload_sha256=std::string(64,'a');identity.rules_sha256=std::string(64,'b');
    identity.styles_sha256=sources.styles_sha256;identity.civilopedia_sha256=sources.civilopedia_sha256;identity.xml_sha256=sources.xml_sha256;
    const auto source=selected_category_block(root);
    // Every captured Lua loop variable must resolve to its own actual tag. The
    // home category completes its real scroll calls; remaining category clicks
    // reach the next honest missing SelectedCategoryTab service, then retire VM.
    std::size_t category_count=0;
    for(int category=1;;++category){
        int scroll_calls=0;auto layout=fixture(scroll_calls);
        auto owner=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
        const auto ids=bind_civis_civilopedia_category_callbacks(layout,root,sources,identity,owner,77);
        category_count=ids.size();if(category>static_cast<int>(ids.size()))break;
        auto host=vm(std::move(layout));host->execute("CivilopediaScreen",source,"@installed-BNW-category-registration");
        check(host->registered_ui_control_callbacks("CivilopediaScreen")==ids.size(),"All actual XML category controls must own Lua callbacks");
        rejects([&]{host->dispatch_ui_control("CivilopediaScreen",ids.front(),77);});
        host->complete_loading();const auto result=host->dispatch_ui_control("CivilopediaScreen",ids[category-1],77);
        check(!host->messages().empty()&&host->messages().back().text=="SetSelectedCategory("+std::to_string(category)+")",
              "Actual installed callback closure must retain its own category loop tag");
        if(category==1)check(result.listeners==1&&result.errors.empty()&&scroll_calls==2,"Actual installed home click must run both scroll services");
        else{
            check(result.listeners==1&&!result.errors.empty()&&result.errors.front().find("SelectedCategoryTab")!=std::string::npos&&
                  !host->loading_complete()&&host->registered_ui_control_callbacks("CivilopediaScreen")==0,
                  "Unsupported category service must fail honestly and retire VM callbacks");
            rejects([&]{host->complete_loading();});rejects([&]{host->dispatch_ui_control("CivilopediaScreen",ids.front(),77);});
        }
    }
    int scroll_calls=0;auto owner=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
    auto make_layout=[&]{auto layout=fixture(scroll_calls);bind_civis_civilopedia_category_callbacks(layout,root,sources,identity,owner,77);return layout;};
    {
        std::vector<ScriptValue> observed;ScriptUiEventProvider events;events.commands={"Record"};
        events.command=[&](auto,std::span<const ScriptValue> args){observed.assign(args.begin(),args.end());};
        auto host=vm(make_layout(),std::move(events));
        host->execute("CivilopediaScreen",R"lua(
            Controls.CategoryButton1:RegisterCallback(Mouse.eLClick,function() error('replaced callback executed') end)
            Controls.CategoryButton1:RegisterCallback(Mouse.eLClick,function(...)
                assert(coroutine.running()~=nil and getfenv(0)==_G)
                Events.Record(...);Controls.CategoryButton1:ClearCallback(Mouse.eLClick)
            end)
        )lua","=replace-clear-real-closures");host->complete_loading();
        const ScriptValue args[]={std::int64_t(5000000001LL),std::string("a\0b",3),false,1.25,std::monostate{}};
        const auto result=host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77,args);
        check(result.listeners==1&&result.errors.empty()&&observed==std::vector<ScriptValue>(std::begin(args),std::end(args))&&
              host->registered_ui_control_callbacks("CivilopediaScreen")==0,"Replacement, arguments and self-unregistration must preserve real closure state");
        rejects([&]{host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);});
    }
    {
        auto host=vm(make_layout());host->execute("CivilopediaScreen",source,"=source");host->complete_loading();
        bool wrong_thread=false;std::thread thread([&]{try{host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);}catch(const ScriptError &){wrong_thread=true;}});thread.join();
        check(wrong_thread,"Callback must reject non-owner thread");
        rejects([&]{host->dispatch_ui_control("OtherContext","CategoryButton1",77);});
        host->close_ui_context("CivilopediaScreen");
        check(host->registered_ui_control_callbacks("CivilopediaScreen")==0,"Close must unregister actual retained Lua closures");
        rejects([&]{host->complete_loading();});
        rejects([&]{host->execute("CivilopediaScreen","assert(true)","=closed");});
        rejects([&]{host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);});
    }
    {
        std::unique_ptr<LuaScriptHost> host;bool reentrant_rejected=false;ScriptUiEventProvider events;events.commands={"Reenter"};
        events.command=[&](auto,auto){try{host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);}catch(const ScriptError &){reentrant_rejected=true;}};
        host=vm(make_layout(),std::move(events));host->execute("CivilopediaScreen","Controls.CategoryButton1:RegisterCallback(Mouse.eLClick,function() Events.Reenter() end)","=reentry");host->complete_loading();
        const auto result=host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);
        check(result.listeners==1&&result.errors.empty()&&reentrant_rejected,"Reentrant invocation must fail without corrupting executing Lua stack");
    }
    {
        std::unique_ptr<LuaScriptHost> host;ScriptUiEventProvider events;events.commands={"CloseContext"};
        events.command=[&](auto,auto){host->close_ui_context("CivilopediaScreen");};
        host=vm(make_layout(),std::move(events));
        host->execute("CivilopediaScreen","Controls.CategoryButton1:RegisterCallback(77,function() Events.CloseContext() end)","=close-inside-callback");host->complete_loading();
        const auto result=host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);
        check(result.listeners==1&&!result.errors.empty()&&!host->loading_complete()&&
              host->registered_ui_control_callbacks("CivilopediaScreen")==0,
              "Close during invocation must retire references after protected callback returns without acknowledging a valid frame");
    }
    {
        auto host=vm(make_layout());host->execute("CivilopediaScreen",source,"=source");host->complete_loading();
        auto changed=identity;++changed.frame_number;owner->observe(changed);owner->observe(identity);
        rejects([&]{host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);});
        check(scroll_calls==0,"Stale owner must reject before any external callback effect");
    }
    owner=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
    {
        auto host=vm(make_layout());host->execute("CivilopediaScreen",source,"=source");host->complete_loading();owner.reset();
        rejects([&]{host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);});
    }
    owner=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
    for(const auto *bad_source:{"Controls.CategoryButton1:RegisterCallback(78,function() end)",
        "Controls.CategoryButton1:RegisterCallback(77,42)",
        "Controls.CategoryButton1.RegisterCallback({id='CategoryButton1'},77,function() end)"}){
        auto host=vm(make_layout());rejects([&]{host->execute("CivilopediaScreen",bad_source,"=invalid-registration");});
        rejects([&]{host->complete_loading();});
    }
    {
        auto host=vm(make_layout(),{},ScriptBudget{0,10000});
        host->execute("CivilopediaScreen","Controls.CategoryButton1:RegisterCallback(77,function() while true do end end)","=bounded-control-callback");host->complete_loading();
        const auto result=host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);
        check(result.listeners==1&&!result.errors.empty()&&!host->loading_complete()&&host->registered_ui_control_callbacks("CivilopediaScreen")==0,
              "Callback instruction budget must fail protected and retire VM");
    }
    {
        auto layout=fixture(scroll_calls);auto host=vm(std::move(layout));
        host->execute("CivilopediaScreen","assert(Controls.ScrollPanel.RegisterCallback==nil and Controls.ScrollPanel.ClearCallback==nil)","=no-capability");
        check(host->registered_ui_control_callbacks("CivilopediaScreen")==0,"No callback methods must exist without explicit capability");
    }
    {
        auto host=vm(make_layout());host->execute("CivilopediaScreen",source,"=source");
        rejects([&]{host->execute("CivilopediaScreen","error('later parent failure')","=failed-parent");});
        rejects([&]{host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);});
    }
    std::cout<<"Installed XML + actual category loop: "<<category_count<<" registered closures; replacement, args, self-clear, close, stale/expired/thread/reentrant/error/loading guards passed. Diagnostic source subset only; full parent and native parity unverified.\n";
}
