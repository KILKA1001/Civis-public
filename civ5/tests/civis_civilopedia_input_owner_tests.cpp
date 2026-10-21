#include "civ5/ui/civis_civilopedia_input_owner.hpp"
#include <fstream>
#include <iostream>
#include <thread>
using namespace civ5::runtime;using namespace civ5::ui;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F call){bool rejected=false;try{call();}catch(const ScriptError &){rejected=true;}check(rejected,"Expected guarded input rejection");}
std::string read(const std::filesystem::path &root){
    std::ifstream file(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua");
    return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}
std::string block(const std::string &source,std::string_view first,std::string_view last){
    const auto begin=source.find(first),end=source.find(last,begin);
    check(begin!=source.npos&&end!=source.npos,"Selected installed input source changed");return source.substr(begin,end-begin);
}
ScriptUiLayoutProvider layout(const std::filesystem::path &root,const SelectedBnwCivilopediaLayoutSources &sources,
    const CivilopediaLayoutOwnerKey &identity,const std::shared_ptr<CivisCivilopediaCallbackOwner> &owner){
    ScriptUiLayoutProvider result;result.context_id="CivilopediaScreen";
    result.screen_size=[]{return std::pair<std::int64_t,std::int64_t>{1280,960};};
    // Diagnostic viewport only. Unrequested geometry cannot silently succeed.
    result.get_size=[](auto)->std::pair<std::int64_t,std::int64_t>{throw ScriptError("Unrequested diagnostic geometry");};
    result.set_size=[](auto,auto,auto){throw ScriptError("Unrequested diagnostic geometry");};
    result.set_offset=[](auto,auto,auto){throw ScriptError("Unrequested diagnostic geometry");};
    result.calculate_internal_size=[](auto){throw ScriptError("Unrequested diagnostic geometry");};
    bind_civis_civilopedia_category_callbacks(result,root,sources,identity,owner,77);
    bind_civis_civilopedia_top_level_button_callbacks(result,root,sources,identity,owner,77);return result;
}
std::unique_ptr<LuaScriptHost> vm(ScriptUiLayoutProvider provider,const std::shared_ptr<CivisCivilopediaInputOwner> &input,
    ScriptUiEventProvider events={}){
    auto result=std::make_unique<LuaScriptHost>(ScriptBudget{},ScriptSourceProvider{},ScriptModdingProvider{},ScriptGameplayProvider{},
        ScriptSaveDataProvider{},ScriptPregameProvider{},ScriptMapProvider{},std::move(events),ScriptContentProvider{},
        ScriptUserDataProvider{},ScriptEnumProvider{},ScriptLocaleProvider{},std::move(provider));
    result->enable_ui_text_entry(input->text_provider());result->enable_ui_context_handlers(input->context_provider());return result;
}
}
int main(int argc,char **argv){
    check(argc==2,"Expected installed game root");const std::filesystem::path root=argv[1];
    const auto sources=inspect_bnw_civilopedia_layout_sources(root);const auto source=read(root);
    const auto enter=block(source,"function OnSearchTextEnter( dummyString, dummyControl )","function OnSearchNotFoundOK()");
    const auto handlers=block(source,"function InputHandler( uiMsg, wParam, lParam )","----------------------------------------------------------------\n-- 'Active'");
    const auto search=block(source,"function OnSearchButtonClicked()","Controls.SearchButton:RegisterCallback(");
    CivilopediaLayoutOwnerKey identity;identity.frame_number=31;identity.owner_token="Civis:captured-input-diagnostic";
    identity.player_id=0;identity.game_turn=123;identity.payload_sha256=std::string(64,'a');identity.rules_sha256=std::string(64,'b');
    identity.styles_sha256=sources.styles_sha256;identity.civilopedia_sha256=sources.civilopedia_sha256;identity.xml_sha256=sources.xml_sha256;
    for(const auto *text:{"Search","Цивилопедия"}){
        auto owner=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
        auto input=CivisCivilopediaInputOwner::open(root,sources,identity,owner,false);
        check(input->max_length()==50&&input->text().empty(),"Actual active XML must supply empty EditBox and MaxLength50");
        std::vector<ScriptValue> observed;ScriptUiEventProvider events;events.commands={"Record"};
        events.command=[&](auto,auto args){observed.assign(args.begin(),args.end());};
        auto host=vm(layout(root,sources,identity,owner),input,std::move(events));
        // The wrapper is a diagnostic observer, not an implementation of engine
        // article search. Enter's function/registration and GetText line are
        // selected installed code; actual payload is captured owner input.
        host->execute("CivilopediaScreen","function OnSearchButtonClicked()\nlocal searchString = Controls.SearchEditBox:GetText();\nEvents.Record(searchString)\nend\n"+enter,"@installed-BNW-Enter-and-GetText");
        rejects([&]{host->replace_ui_text("CivilopediaScreen","SearchEditBox",text);});
        rejects([&]{host->submit_ui_text("CivilopediaScreen","SearchEditBox");});
        host->complete_loading();host->replace_ui_text("CivilopediaScreen","SearchEditBox",text);
        const auto result=host->submit_ui_text("CivilopediaScreen","SearchEditBox");
        check(result.listeners==1&&result.errors.empty()&&observed==std::vector<ScriptValue>{std::string(text)},"Actual Enter closure must call GetText on captured EN/RU input");
        host->execute("CivilopediaScreen",R"lua(
            Controls.SearchEditBox:RegisterCallback(function(text,control)
                assert(control==Controls.SearchEditBox and text==control:GetText())
                assert(coroutine.running()~=nil and getfenv(0)==_G)
                Events.Record(text,control.id);control:ClearCallback()
            end)
        )lua","=typed-Enter-arguments-and-unregister");host->complete_loading();
        const auto cleared=host->submit_ui_text("CivilopediaScreen","SearchEditBox");
        check(cleared.listeners==1&&cleared.errors.empty()&&observed==std::vector<ScriptValue>{std::string(text),std::string("SearchEditBox")}&&
            host->registered_ui_control_callbacks("CivilopediaScreen")==0,"Retained receiver/text, replacement and self-clear must be real Lua state");
        rejects([&]{host->submit_ui_text("CivilopediaScreen","SearchEditBox");});
        host->replace_ui_text("CivilopediaScreen","SearchEditBox",std::string(50,'a'));
        rejects([&]{host->replace_ui_text("CivilopediaScreen","SearchEditBox",std::string(51,'a'));});
        rejects([&]{host->replace_ui_text("CivilopediaScreen","SearchEditBox",std::string("a\0b",3));});
        rejects([&]{host->replace_ui_text("CivilopediaScreen","SearchEditBox",std::string("\xc0\xaf",2));});
        check(input->text()==std::string(50,'a'),"Rejected input must preserve prior captured state");
        std::string russian;for(int i=0;i<50;++i)russian+="я";
        host->replace_ui_text("CivilopediaScreen","SearchEditBox",russian);
        rejects([&]{host->replace_ui_text("CivilopediaScreen","SearchEditBox",russian+"я");});
        bool wrong_thread=false;std::thread thread([&]{try{host->replace_ui_text("CivilopediaScreen","SearchEditBox","x");}catch(const ScriptError &){wrong_thread=true;}});thread.join();
        check(wrong_thread&&input->text()==russian,"Only owner thread may replace captured text");
        auto changed=identity;++changed.frame_number;owner->observe(changed);
        rejects([&]{host->replace_ui_text("CivilopediaScreen","SearchEditBox","x");});rejects([&]{input->text();});
        input->close();host->close_ui_context("CivilopediaScreen");
    }
    {
        auto owner=std::make_shared<CivisCivilopediaCallbackOwner>(identity);auto input=CivisCivilopediaInputOwner::open(root,sources,identity,owner,false);
        auto host=vm(layout(root,sources,identity,owner),input);
        host->execute("CivilopediaScreen",handlers,"@installed-BNW-context-handlers");host->complete_loading();
        check(host->registered_ui_context_handlers("CivilopediaScreen")==2,"Actual source must retain both context handlers");
        const auto up=host->dispatch_ui_input("CivilopediaScreen",{ScriptUiKeyEvent::key_up,ScriptUiKey::enter});
        check(up.listeners==1&&up.errors.empty()&&!up.handled,"Actual InputHandler ignores KeyUp without invoking missing controls");
        const auto down=host->dispatch_ui_input("CivilopediaScreen",{ScriptUiKeyEvent::key_down,ScriptUiKey::escape});
        check(down.listeners==1&&!down.errors.empty()&&down.errors.front().find("SearchFoundNothing")!=std::string::npos&&
              host->registered_ui_context_handlers("CivilopediaScreen")==0&&!host->loading_complete(),
              "Actual keydown must report unsupported popup visibility and retire failed VM");
        rejects([&]{host->dispatch_ui_input("CivilopediaScreen",{});});
    }
    {
        auto owner=std::make_shared<CivisCivilopediaCallbackOwner>(identity);auto input=CivisCivilopediaInputOwner::open(root,sources,identity,owner,false);
        auto host=vm(layout(root,sources,identity,owner),input);
        host->execute("CivilopediaScreen",search+enter,"@installed-BNW-actual-search-body");host->complete_loading();
        host->replace_ui_text("CivilopediaScreen","SearchEditBox","Цивилопедия");const auto result=host->submit_ui_text("CivilopediaScreen","SearchEditBox");
        check(result.listeners==1&&!result.errors.empty()&&result.errors.front().find("SetUICursor")!=std::string::npos&&
              host->registered_ui_control_callbacks("CivilopediaScreen")==0,"Full original search must report real next service rather than invent article data");
    }
    {
        auto owner=std::make_shared<CivisCivilopediaCallbackOwner>(identity);auto input=CivisCivilopediaInputOwner::open(root,sources,identity,owner,false);
        auto host=vm(layout(root,sources,identity,owner),input);
        host->execute("CivilopediaScreen",R"lua(
            ContextPtr:SetInputHandler(function(event,key,param)
                assert(param==0);return event==KeyEvents.KeyDown and key==Keys.VK_ESCAPE
            end)
            ContextPtr:SetShowHideHandler(function(hidden) assert(hidden==ContextPtr:IsHidden()) end)
        )lua","=Civis-typed-context-contract");host->complete_loading();
        check(host->dispatch_ui_input("CivilopediaScreen",{}).handled==false&&
              host->dispatch_ui_input("CivilopediaScreen",{ScriptUiKeyEvent::key_down,ScriptUiKey::escape}).handled,
              "Typed context input must preserve boolean handled result");
        check(host->dispatch_ui_visibility("CivilopediaScreen",true).errors.empty()&&input->hidden(),"Visibility must capture state before real Lua handler");
        rejects([&]{host->dispatch_ui_input("CivilopediaScreen",{});});
        check(host->dispatch_ui_visibility("CivilopediaScreen",false).errors.empty()&&!input->hidden(),"Show must reopen captured visibility on current owner");
        host->close_ui_context("CivilopediaScreen");
        check(host->registered_ui_context_handlers("CivilopediaScreen")==0,"Close unregisters retained context handlers");
        rejects([&]{host->dispatch_ui_visibility("CivilopediaScreen",true);});
        rejects([&]{host->complete_loading();});
    }
    {
        auto owner=std::make_shared<CivisCivilopediaCallbackOwner>(identity);auto input=CivisCivilopediaInputOwner::open(root,sources,identity,owner,false);
        const auto provider=input->text_provider();input.reset();rejects([&]{provider.get_text();});
    }
    {
        auto owner=std::make_shared<CivisCivilopediaCallbackOwner>(identity);auto input=CivisCivilopediaInputOwner::open(root,sources,identity,owner,false);
        std::unique_ptr<LuaScriptHost> host;bool guarded=false;ScriptUiEventProvider events;events.commands={"Reenter","Close"};
        events.command=[&](std::string_view name,auto){
            if(name=="Reenter")try{host->replace_ui_text("CivilopediaScreen","SearchEditBox","nested");}catch(const ScriptError &){guarded=true;}
            else host->close_ui_context("CivilopediaScreen");
        };
        host=vm(layout(root,sources,identity,owner),input,std::move(events));
        host->execute("CivilopediaScreen","Controls.SearchEditBox:RegisterCallback(function() Events.Reenter() end)","=input-reentry");host->complete_loading();
        check(host->submit_ui_text("CivilopediaScreen","SearchEditBox").errors.empty()&&guarded&&input->text().empty(),
              "Reentrant captured-text mutation must fail before state change");
        host->execute("CivilopediaScreen","Controls.SearchEditBox:RegisterCallback(function() Events.Close() end)","=input-close-during-dispatch");host->complete_loading();
        const auto closed=host->submit_ui_text("CivilopediaScreen","SearchEditBox");
        check(closed.listeners==1&&!closed.errors.empty()&&!host->loading_complete()&&host->registered_ui_control_callbacks("CivilopediaScreen")==0,
              "Closing from Enter must defer unref until protected callback returns and retire VM");
    }
    std::cout<<"Source-backed Enter/GetText EN/RU, typed context handlers, visibility lifecycle, original unsupported services and ownership guards passed. Civis semantic keys/scalar limits; no native ABI parity claim.\n";
}
