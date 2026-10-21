#include "civ5/runtime/civis_lua_table_cardinality.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
namespace {
void check(bool value,const char *reason) {
    if(!value)throw std::runtime_error(reason);
}
int count_binding(lua_State *state) {
    bool rejected=false;
    std::int64_t count=0;
    try {
        if(lua_gettop(state)!=1)throw std::invalid_argument("Expected one table");
        count=civ5::runtime::count_civis_raw_table_keys(state,1);
    }catch(const std::exception &) {
        rejected=true;
    }
    // All C++ exception objects are destroyed before the Lua longjmp.
    if(rejected)return luaL_error(state,"Civis raw table count requires exactly one table");
    lua_pushinteger(state,static_cast<lua_Integer>(count));
    return 1;
}
void execute(lua_State *state,const std::string &source,const char *name) {
    if(luaL_loadbuffer(state,source.data(),source.size(),name)||lua_pcall(state,0,0,0)) {
        const auto *message=lua_tostring(state,-1);
        throw std::runtime_error(message?message:"Lua diagnostic failed without a message");
    }
    check(lua_gettop(state)==0,"Diagnostic execution leaked the Lua stack");
}
template<class F>void rejects(F call) {
    bool rejected=false;
    try {call();}catch(const std::invalid_argument &) {rejected=true;}
    check(rejected,"Expected raw cardinality argument rejection");
}
}
int main(int argc,char **argv) {
    check(argc==2,"Expected installed game root");
    std::ifstream file(std::filesystem::path(argv[1])/
        "Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.lua");
    check(bool(file),"Installed Civilopedia source unavailable");
    const std::string source{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    const auto begin=source.find("function DependencySort(articles)");
    const auto fallback=source.find("\n\tfor i,v in ipairs(GameConceptsList) do",begin);
    const auto next=source.find("CivilopediaCategory[CategoryTech].PopulateList",fallback);
    const auto end=source.rfind("\nend",next);
    check(begin!=source.npos&&fallback!=source.npos&&next!=source.npos&&end!=source.npos&&end>fallback,
          "Installed dependency sort/fallback source boundaries changed");
    check(source.find("table.count(dependencies[a]) == 0",begin)<fallback,
          "Installed raw dependency cardinality call changed");
    std::unique_ptr<lua_State,decltype(&lua_close)> state(luaL_newstate(),lua_close);
    check(bool(state),"Lua allocation failed");
    for(const auto open:{luaopen_base,luaopen_table}) {
        lua_pushcfunction(state.get(),open);
        lua_call(state.get(),0,0);
    }
    lua_pushcfunction(state.get(),count_binding);
    lua_setglobal(state.get(),"explicit_civis_count");
    execute(state.get(),R"lua(
        assert(table.count==nil)
        requested={}; setmetatable(requested,{__index=_G}); requested._G=requested
        requested.table={}
        for key,value in pairs(table) do requested.table[key]=value end
        requested.table.count=explicit_civis_count
        unrequested={};setmetatable(unrequested,{__index=_G})
        assert(unrequested.table.count==nil and table.count==nil)
        local harness=function()
            local object={}
            local raw=setmetatable({[1]=false,[1000]=true,named=0,[false]=0,[object]=true},{
                __index=function() error('unexpected lookup') end,
                __pairs=function() error('unexpected pairs metamethod') end,
                __len=function() error('unexpected length metamethod') end})
            assert(table.count(raw)==5 and table.count({})==0)
            raw[object]=nil; assert(table.count(raw)==4)
            assert(not pcall(table.count,nil) and not pcall(table.count,1))
            assert(not pcall(table.count,'value') and not pcall(table.count,false))
            assert(not pcall(table.count,function()end) and not pcall(table.count))
            assert(not pcall(table.count,{},{}))
        end
        setfenv(harness,requested); harness()
    )lua","=explicit-Civis-raw-cardinality-harness");
    execute(state.get(),"local harness=function()\n"+source.substr(begin,fallback-begin)+R"lua(
        local a={Type='A',InsertAfter='C'}
        local b={Type='B',InsertBefore='A'}
        local c={Type='C'}
        local sparseEdges=DependencySort({a,b,c})
        assert(#sparseEdges==3 and sparseEdges[1]==b and sparseEdges[2]==c and sparseEdges[3]==a)
        assert(#DependencySort({})==0)
        local x={Type='X',InsertAfter='Y'}
        local y={Type='Y',InsertAfter='X'}
        local old={x,y,headingOpen=true}
        assert(DependencySort(old)==nil)
        GameConceptsList={old}
    )lua"+source.substr(fallback,end-fallback)+R"lua(
        assert(GameConceptsList[1]==old and GameConceptsList[1].headingOpen==true)
        GameConceptsList={{a,b,c}}
    )lua"+source.substr(fallback,end-fallback)+R"lua(
        assert(GameConceptsList[1][1]==b and GameConceptsList[1][3]==a)
        assert(GameConceptsList[1].headingOpen==false)
    end
    setfenv(harness,requested);harness()
    assert(table.count==nil and unrequested.table.count==nil)
    )lua","@installed-Civilopedia-verbatim-DependencySort-and-fallback");
    lua_newtable(state.get());
    check(civ5::runtime::count_civis_raw_table_keys(state.get(),-1)==0&&lua_gettop(state.get())==1,
          "Negative table index must preserve the stack");
    rejects([&]{civ5::runtime::count_civis_raw_table_keys(state.get(),0);});
    rejects([&]{civ5::runtime::count_civis_raw_table_keys(state.get(),2);});
    rejects([&]{civ5::runtime::count_civis_raw_table_keys(state.get(),-2);});
    rejects([&]{civ5::runtime::count_civis_raw_table_keys(nullptr,1);});
    lua_pushnil(state.get());
    rejects([&]{civ5::runtime::count_civis_raw_table_keys(state.get(),-1);});
    check(lua_gettop(state.get())==2,"Rejected raw count changed the stack");
    check(lua_checkstack(state.get(),LUAI_MAXCSTACK-3)!=0,"Cannot prepare traversal-capacity refusal fixture");
    lua_settop(state.get(),LUAI_MAXCSTACK-1);
    bool capacity_rejected=false;
    try {civ5::runtime::count_civis_raw_table_keys(state.get(),1);}
    catch(const std::runtime_error &) {capacity_rejected=true;}
    check(capacity_rejected&&lua_gettop(state.get())==LUAI_MAXCSTACK-1&&lua_istable(state.get(),1),
          "Traversal capacity refusal must preserve stack top and table");
    lua_settop(state.get(),2);
    std::cout<<"Explicit Civis raw cardinality passed: verbatim installed DependencySort/fallback, sparse object edges, raw keys, nil/arity rejection and context isolation. Native parity and host lifecycle integration remain unverified.\n";
}
