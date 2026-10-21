#include "civ5/ui/civis_civilopedia_units_query_owner.hpp"
#include "civ5/runtime/lua_gameplay_services.hpp"
#include <fstream>
#include <iostream>
#include <thread>
using namespace civ5;using namespace civ5::ui;using namespace civ5::runtime;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F call){bool rejected=false;try{call();}catch(const ScriptError &){rejected=true;}check(rejected,"Expected guarded Units Lua rejection");}
std::unique_ptr<LuaScriptHost> vm(const std::filesystem::path &root,const SelectedBnwCivilopediaLayoutSources &sources,
    const CivilopediaLayoutOwnerKey &identity,const std::shared_ptr<CivisCivilopediaCallbackOwner> &owner,
    const ScriptGameplayProvider &gameplay,bool has_db=true,bool conflict=false){
    ScriptUiLayoutProvider layout;layout.context_id="CivilopediaScreen";
    layout.screen_size=[]{return std::pair<std::int64_t,std::int64_t>{1280,960};};
    layout.get_size=[](auto)->std::pair<std::int64_t,std::int64_t>{throw ScriptError("Unrequested fixture geometry");};
    layout.set_size=[](auto,auto,auto){throw ScriptError("Unrequested fixture geometry");};
    layout.set_offset=[](auto,auto,auto){throw ScriptError("Unrequested fixture geometry");};
    layout.calculate_internal_size=[](auto){throw ScriptError("Unrequested fixture geometry");};
    bind_civis_civilopedia_category_callbacks(layout,root,sources,identity,owner,77);
    ScriptEnumProvider enums;if(conflict)enums.tables={{"DB",{{"CreateQuery",1}}}};
    return std::make_unique<LuaScriptHost>(ScriptBudget{},ScriptSourceProvider{},ScriptModdingProvider{},has_db?gameplay:ScriptGameplayProvider{},
        ScriptSaveDataProvider{},ScriptPregameProvider{},ScriptMapProvider{},ScriptUiEventProvider{},ScriptContentProvider{},
        ScriptUserDataProvider{},std::move(enums),ScriptLocaleProvider{},std::move(layout));
}
ScriptCivisUnitsQueryProvider provider(const std::shared_ptr<CivisCivilopediaUnitsQueryOwner> &owner){
    const std::weak_ptr<CivisCivilopediaUnitsQueryOwner> weak=owner;
    return {[weak](auto sql){const auto held=weak.lock();if(!held)throw ScriptError("Fixture Units query owner expired");return held->prepare(sql);}};
}
std::string literal(std::string_view value){return "[==["+std::string(value)+"]==]";}
}
int main(int argc,char **argv){
    check(argc==2,"Expected installed game root");const std::filesystem::path root=argv[1];
    const auto sources=inspect_bnw_civilopedia_layout_sources(root);
    std::ifstream file(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua",std::ios::binary);
    const std::string source{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    const auto sql=CivisCivilopediaUnitsQueryOwner::declared_sql_from_source(source);
    CivilopediaLayoutOwnerKey identity;identity.frame_number=31;identity.owner_token="Civis:Units-Lua-fixture";
    identity.player_id=0;identity.game_turn=123;identity.payload_sha256=std::string(64,'a');identity.rules_sha256=std::string(64,'b');
    identity.styles_sha256=sources.styles_sha256;identity.xml_sha256=sources.xml_sha256;identity.civilopedia_sha256=sources.civilopedia_sha256;
    content::RulesDatabase rules;rules.apply_sql("CREATE TABLE Technologies(Type TEXT,Era TEXT);INSERT INTO Technologies VALUES('TECH_A','ERA_ANCIENT'),('TECH_B','ERA_MEDIEVAL');"
        "CREATE TABLE Units(ID INTEGER,Description TEXT,PortraitIndex INTEGER,IconAtlas TEXT,PreReqTech TEXT,FaithCost INTEGER,RequiresFaithPurchaseEnabled boolean,Cost INTEGER,ShowInPedia boolean);"
        "INSERT INTO Units VALUES(5000000001,'TXT_A',3,'ATLAS_A','TECH_A',0,0,30,1),(2,'TXT_B',4,'ATLAS_B','TECH_B',0,0,40,1);");
    ResearchContext setup;setup.game_turn=123;setup.speed=2;
    const std::vector<TeamTechnologies> teams;const auto types=rules.cache_info_types();
    const auto gameplay=lua_gameplay_services(rules,setup,teams,types);
    auto context=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
    auto owner=CivisCivilopediaUnitsQueryOwner::open(root,sources,rules,identity,context);
    auto host=vm(root,sources,identity,context,gameplay);
    rejects([&]{host->enable_civis_units_query("Wrong",provider(owner));});
    rejects([&]{host->enable_civis_units_query("CivilopediaScreen",{});});
    host->enable_civis_units_query("CivilopediaScreen",provider(owner));
    rejects([&]{host->enable_civis_units_query("CivilopediaScreen",provider(owner));});
    host->execute("OtherContext","assert(DB.CreateQuery==nil);old_db=DB;old_query=DB.Query","=unrequested-before");
    host->execute("CivilopediaScreen","declared_sql="+literal(sql)+R"lua(
        assert(type(DB.CreateQuery)=='function' and type(DB.Query)=='function')
        local existing=DB.Query;assert(DB.Query('SELECT 123 AS V')().V==123)
        local prepared=DB.CreateQuery(declared_sql)
        local iter=prepared('ERA_ANCIENT');prepared=nil;collectgarbage('collect')
        local row=iter();assert(row.ID==5000000001 and row.Description=='TXT_A' and row.PortraitIndex==3 and row.IconAtlas=='ATLAS_A')
        assert(iter()==nil and iter(nil,nil)==nil and DB.Query==existing)
        local q=DB.CreateQuery(declared_sql)
        local n=0;for unit in q('ERA_MEDIEVAL')do assert(unit.ID==2);n=n+1 end;assert(n==1)
        n=0;for unit in q('ERA_EMPTY')do n=n+1 end;assert(n==0)
        getmetatable(_G).__index.__held_units=q
        Controls.CategoryButton1:RegisterCallback(77,function()
            local count=0;for unit in q('ERA_ANCIENT')do assert(unit.ID==5000000001);count=count+1 end;assert(count==1)
        end)
    )lua","@installed-decoded-Units-SQL-fixture");
    auto observations=host->civis_units_query_observations("CivilopediaScreen");
    check(observations.prepared==2&&observations.invocations==3&&observations.rows==2&&observations.eof==4,
        "Exact successful fixture factory/bind/row/EOF observations differ");
    host->execute("OtherContext","assert(DB==old_db and DB.Query==old_query and DB.CreateQuery==nil)","=unrequested-after");
    host->execute("NewContext","assert(DB.CreateQuery==nil)","=unrequested-new");
    rejects([&]{host->enable_civis_units_query("CivilopediaScreen",provider(owner));});
    rejects([&]{host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);});
    host->complete_loading();const auto callback=host->dispatch_ui_control("CivilopediaScreen","CategoryButton1",77);
    check(callback.listeners==1&&callback.errors.empty(),"Ready retained Units callback must dispatch once");
    bool foreign=false;std::thread thread([&]{try{host->enable_civis_units_query("CivilopediaScreen",provider(owner));}catch(const ScriptError &){foreign=true;}});
    thread.join();check(foreign&&host->loading_complete(),"Foreign declaration must reject before VM mutation");
    foreign=false;std::thread read([&]{try{host->civis_units_query_observations("CivilopediaScreen");}catch(const ScriptError &){foreign=true;}});
    read.join();check(foreign&&host->loading_complete(),"Foreign diagnostics must reject before VM mutation");
    host->close_ui_context("CivilopediaScreen");
    host->execute("OtherContext","assert(not pcall(__held_units,'ERA_ANCIENT'));assert(2+2==4)","=closed-held-query-stack");rejects([&]{host->complete_loading();});
    for(const auto *bad:{"DB.CreateQuery()","DB.CreateQuery(nil)","DB.CreateQuery(123)","DB.CreateQuery(declared_sql,1)",
        "DB.CreateQuery(declared_sql..' ')","DB.CreateQuery(declared_sql..string.char(0))","DB.CreateQuery('DELETE FROM Units')",
        "q()","q(nil)","q(1)","q('ERA_ANCIENT',1)","q('ERA'..string.char(0)..'A')","it(1)","it(1,2,3)"}){
        auto current=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
        auto owned=CivisCivilopediaUnitsQueryOwner::open(root,sources,rules,identity,current);
        auto invalid=vm(root,sources,identity,current,gameplay);invalid->enable_civis_units_query("CivilopediaScreen",provider(owned));
        invalid->execute("CivilopediaScreen","declared_sql="+literal(sql)+";local q=DB.CreateQuery(declared_sql);local it=q('ERA_ANCIENT');assert(not pcall(function() "+bad+" end));assert(1+1==2)","=invalid-Units-argument-stack");
        rejects([&]{invalid->complete_loading();});
    }
    for(const auto *failure:{"generation","owner-expiry","context-stale","wrong-context"}){
        auto current=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
        auto owned=CivisCivilopediaUnitsQueryOwner::open(root,sources,rules,identity,current);
        auto invalid=vm(root,sources,identity,current,gameplay);invalid->enable_civis_units_query("CivilopediaScreen",provider(owned));
        invalid->execute("CivilopediaScreen","q=DB.CreateQuery("+literal(sql)+");it=q('ERA_ANCIENT');getmetatable(_G).__index.__held_iterator=it;getmetatable(_G).__index.__held_query=q","=retained-Units-handles");
        if(std::string_view(failure)=="generation")invalid->execute("CivilopediaScreen","q('ERA_MEDIEVAL');assert(not pcall(it));assert(1+1==2)","=old-iterator-generation");
        else if(std::string_view(failure)=="owner-expiry"){owned.reset();invalid->execute("CivilopediaScreen","assert(not pcall(it))","=expired-Units-owner");}
        else if(std::string_view(failure)=="context-stale"){auto changed=identity;++changed.frame_number;current->observe(changed);invalid->execute("CivilopediaScreen","assert(not pcall(it))","=stale-Units-owner");}
        else invalid->execute("OtherContext","assert(not pcall(__held_iterator));assert(2+2==4)","=foreign-Units-context");
        rejects([&]{invalid->complete_loading();});
    }
    for(const auto [has_db,conflict]:{std::pair{false,false},std::pair{true,true}}){
        auto current=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
        auto owned=CivisCivilopediaUnitsQueryOwner::open(root,sources,rules,identity,current);
        auto invalid=vm(root,sources,identity,current,gameplay,has_db,conflict);invalid->enable_civis_units_query("CivilopediaScreen",provider(owned));
        rejects([&]{invalid->execute("CivilopediaScreen","error('must not execute')","=Units-binding-failure");});
        invalid->execute("OtherContext","assert(1+1==2)","=Units-binding-failure-stack");rejects([&]{invalid->complete_loading();});
    }
    {
        auto current=std::make_shared<CivisCivilopediaCallbackOwner>(identity);auto invalid=vm(root,sources,identity,current,gameplay);
        invalid->enable_civis_units_query("CivilopediaScreen",{[](auto){return std::shared_ptr<CivisCivilopediaUnitsQuery>{};}});
        invalid->execute("CivilopediaScreen","assert(not pcall(DB.CreateQuery,"+literal(sql)+"));assert(1+1==2)","=invalid-provider-handle");
        rejects([&]{invalid->complete_loading();});
    }
    {
        auto current=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
        auto owned=CivisCivilopediaUnitsQueryOwner::open(root,sources,rules,identity,current);auto invalid=vm(root,sources,identity,current,gameplay);
        invalid->enable_civis_units_query("CivilopediaScreen",{[&](auto requested){
            try{invalid->execute("OtherContext","assert(1)","=reentrant-query-provider");}catch(const ScriptError &){}
            return owned->prepare(requested);
        }});
        invalid->execute("CivilopediaScreen","assert(not pcall(DB.CreateQuery,"+literal(sql)+"));assert(1+1==2)","=caught-reentrant-Units-provider");
        rejects([&]{invalid->complete_loading();});
    }
    {
        auto current=std::make_shared<CivisCivilopediaCallbackOwner>(identity);
        content::RulesDatabase step_error;
        step_error.apply_sql("CREATE TABLE Technologies(Type TEXT,Era TEXT);INSERT INTO Technologies VALUES('TECH_A','ERA_ANCIENT');"
            "CREATE VIEW Units AS SELECT abs(-9223372036854775808) AS ID,'TXT' AS Description,0 AS PortraitIndex,'ATLAS' AS IconAtlas,"
            "'TECH_A' AS PreReqTech,0 AS FaithCost,0 AS RequiresFaithPurchaseEnabled,1 AS Cost,1 AS ShowInPedia;");
        auto owned=CivisCivilopediaUnitsQueryOwner::open(root,sources,step_error,identity,current);
        auto invalid=vm(root,sources,identity,current,gameplay);invalid->enable_civis_units_query("CivilopediaScreen",provider(owned));
        invalid->execute("CivilopediaScreen","local q=DB.CreateQuery("+literal(sql)+");local it=q('ERA_ANCIENT');assert(not pcall(it));assert(1+1==2)","=caught-lazy-Units-step-error");
        const auto observed=invalid->civis_units_query_observations("CivilopediaScreen");
        check(observed.prepared==1&&observed.invocations==1&&observed.rows==0&&observed.eof==0,
            "Lazy Step error must not become successful row or EOF observation");
        rejects([&]{invalid->complete_loading();});
    }
    auto old=vm(root,sources,identity,context,gameplay);
    old->execute("CivilopediaScreen","assert(DB.CreateQuery==nil)","=old-Units-capability-frontier");old->complete_loading();
    std::cout<<"Civis Units Lua factory/GC/iterator/context/loading/error guards PASS\n";
}
