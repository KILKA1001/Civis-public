#include "civ5/ui/civis_civilopedia_category_callbacks.hpp"
#include <fstream>
#include <iostream>
#include <thread>
using namespace civ5::runtime;
using namespace civ5::ui;
namespace {
void check(bool value,const char *reason){if(!value)throw std::runtime_error(reason);}
template<class F>void rejects(F call){
    bool rejected=false;try{call();}catch(const ScriptError &){rejected=true;}
    check(rejected,"Expected guarded raw count rejection");
}
std::unique_ptr<LuaScriptHost> vm(const std::filesystem::path &root,
    const SelectedBnwCivilopediaLayoutSources &sources,const CivilopediaLayoutOwnerKey &identity,
    const std::shared_ptr<CivisCivilopediaCallbackOwner> &owner){
    ScriptUiLayoutProvider layout;layout.context_id="CivilopediaScreen";
    layout.screen_size=[]{return std::pair<std::int64_t,std::int64_t>{1280,960};};
    layout.get_size=[](auto)->std::pair<std::int64_t,std::int64_t>{throw ScriptError("Unrequested diagnostic geometry");};
    layout.set_size=[](auto,auto,auto){throw ScriptError("Unrequested diagnostic geometry");};
    layout.set_offset=[](auto,auto,auto){throw ScriptError("Unrequested diagnostic geometry");};
    layout.calculate_internal_size=[](auto){throw ScriptError("Unrequested diagnostic geometry");};
    bind_civis_civilopedia_category_callbacks(layout,root,sources,identity,owner,77);
    return std::make_unique<LuaScriptHost>(ScriptBudget{},ScriptSourceProvider{},ScriptModdingProvider{},ScriptGameplayProvider{},
        ScriptSaveDataProvider{},ScriptPregameProvider{},ScriptMapProvider{},ScriptUiEventProvider{},ScriptContentProvider{},
        ScriptUserDataProvider{},ScriptEnumProvider{},ScriptLocaleProvider{},std::move(layout));
}
}
int main(int argc,char **argv){
    check(argc==2,"Expected installed game root");const std::filesystem::path root=argv[1];
    const auto sources=inspect_bnw_civilopedia_layout_sources(root);
    check(sources.verified,"Installed Civilopedia sources unavailable");
    CivilopediaLayoutOwnerKey identity;identity.frame_number=31;identity.owner_token="Civis:raw-count-diagnostic";
    identity.player_id=0;identity.game_turn=123;identity.payload_sha256=std::string(64,'a');identity.rules_sha256=std::string(64,'b');
    identity.styles_sha256=sources.styles_sha256;identity.civilopedia_sha256=sources.civilopedia_sha256;identity.xml_sha256=sources.xml_sha256;
    std::ifstream file(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua");
    const std::string source{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    const auto begin=source.find("function DependencySort(articles)");
    const auto fallback=source.find("\n\tfor i,v in ipairs(GameConceptsList) do",begin);
    const auto next=source.find("CivilopediaCategory[CategoryTech].PopulateList",fallback);
    const auto end=source.rfind("\nend",next);
    check(begin!=source.npos&&fallback!=source.npos&&next!=source.npos&&end!=source.npos&&end>fallback,
          "Installed dependency sort source boundaries changed");
    const auto owner=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
    auto host=vm(root,sources,identity,owner);
    rejects([&]{host->enable_civis_raw_table_count("WrongContext");});
    host->enable_civis_raw_table_count("CivilopediaScreen");
    rejects([&]{host->enable_civis_raw_table_count("CivilopediaScreen");});
    host->execute("OtherContext",R"lua(
        assert(table.count==nil)
        before_table=table;before_sort=table.sort;before_insert=table.insert
        getmetatable(_G).__index.__CIVIS_original_table=table
    )lua","=raw-count-unrequested-before");
    host->execute("CivilopediaScreen",R"lua(
        assert(table~=__CIVIS_original_table and table.sort==__CIVIS_original_table.sort)
        assert(table.insert==__CIVIS_original_table.insert and __CIVIS_original_table.count==nil)
        local object={}
        local raw=setmetatable({[1]=false,[1000]=true,named=0,[false]=0,[object]=true},{
            __index=function()error('unexpected lookup')end,
            __pairs=function()error('unexpected pairs')end,
            __len=function()error('unexpected length')end})
        assert(table.count(raw)==5);raw[object]=nil;assert(table.count(raw)==4)
        assert(table.count({})==0)
        Controls.CategoryButton1:RegisterCallback(77,function()assert(table.count(raw)==4)end)
    )lua"+source.substr(begin,fallback-begin)+R"lua(
        local a={Type='A',InsertAfter='C'};local b={Type='B',InsertBefore='A'};local c={Type='C'}
        local sorted=DependencySort({a,b,c})
        assert(sorted[1]==b and sorted[2]==c and sorted[3]==a)
        local old={{Type='X',InsertAfter='Y'},{Type='Y',InsertAfter='X'},headingOpen=true}
        GameConceptsList={old}
    )lua"+source.substr(fallback,end-fallback)+R"lua(
        assert(GameConceptsList[1]==old and old.headingOpen==true)
    )lua","@installed-Civilopedia-raw-count-DependencySort");
    rejects([&]{host->enable_civis_raw_table_count("CivilopediaScreen");});
    host->execute("OtherContext","assert(table==before_table and table.sort==before_sort and table.insert==before_insert and table.count==nil)",
                  "=raw-count-unrequested-after");
    host->execute("NewContext","assert(table.count==nil)","=raw-count-new-unrequested");
    rejects([&]{host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);});
    host->complete_loading();
    const auto callback=host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);
    check(callback.listeners==1&&callback.errors.empty(),"Ready owner callback must execute retained raw count closure");
    bool foreign=false;
    std::thread thread([&]{try{host->enable_civis_raw_table_count("CivilopediaScreen");}catch(const ScriptError &){foreign=true;}});
    thread.join();check(foreign&&host->loading_complete(),"Foreign declaration must reject before changing ready VM");
    foreign=false;
    std::thread invocation([&]{try{host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);}catch(const ScriptError &){foreign=true;}});
    invocation.join();check(foreign&&host->loading_complete(),"Foreign callback invocation must reject before touching the VM");
    host->close_ui_context("CivilopediaScreen");
    rejects([&]{host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);});
    rejects([&]{host->enable_civis_raw_table_count("CivilopediaScreen");});
    rejects([&]{host->execute("CivilopediaScreen","table.count({})","=closed-count");});
    for(const auto *bad:{"nil","1","'text'","false","function()end","","{},{}"}){
        const auto current=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
        auto invalid=vm(root,sources,identity,current);invalid->enable_civis_raw_table_count("CivilopediaScreen");
        const auto args=std::string(bad).empty()?std::string{}:","+std::string(bad);
        invalid->execute("CivilopediaScreen","assert(not pcall(table.count"+args+"))\nassert(table.count({[7]=false})==1)",
                         "=caught-invalid-count-stack");
        check(!invalid->loading_complete(),"Caught count failure must not publish readiness");
        rejects([&]{invalid->complete_loading();});
    }
    {
        auto stale_owner=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
        auto stale=vm(root,sources,identity,stale_owner);stale->enable_civis_raw_table_count("CivilopediaScreen");
        stale->execute("CivilopediaScreen","assert(table.count({})==0)","=current-count");
        auto changed=identity;++changed.frame_number;stale_owner->observe(changed);
        stale->execute("CivilopediaScreen","assert(not pcall(table.count,{}))","=stale-count");
        rejects([&]{stale->complete_loading();});
    }
    {
        auto expired_owner=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
        auto expired=vm(root,sources,identity,expired_owner);expired->enable_civis_raw_table_count("CivilopediaScreen");
        expired_owner.reset();rejects([&]{expired->execute("CivilopediaScreen","table.count({})","=expired-bind");});
        rejects([&]{expired->complete_loading();});
    }
    {
        auto current=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
        auto unrequested=vm(root,sources,identity,current);
        unrequested->execute("CivilopediaScreen","assert(table.count==nil)","=old-count-boundary");
        rejects([&]{unrequested->enable_civis_raw_table_count("CivilopediaScreen");});
    }
    for(const auto *corrupt:{"13","{count=function()return 999 end}"}){
        auto current=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
        auto invalid=vm(root,sources,identity,current);invalid->enable_civis_raw_table_count("CivilopediaScreen");
        invalid->execute("OtherContext","getmetatable(_G).__index.table="+std::string(corrupt),"=corrupt-inherited-table");
        rejects([&]{invalid->execute("CivilopediaScreen","error('must not execute')","=binding-failure");});
        // The protected host call must restore its stack so unrelated contexts
        // can still be inspected; the failed VM must never become ready.
        invalid->execute("OtherContext","assert(1+1==2)","=after-binding-failure-stack");
        rejects([&]{invalid->complete_loading();});
    }
    std::cout<<"Explicit host raw count capability passed: installed sort/fallback, context table copy/identity, raw keys, duplicate/late/thread/stale/closed/expired guards, arity failures and protected binding stack restoration. Native parity remains unverified.\n";
}
