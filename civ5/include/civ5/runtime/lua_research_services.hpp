#pragma once
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/runtime/research_selection.hpp"
namespace civ5::runtime {
class LuaResearchSelectionServices final:public ResearchSelectionServices {
    LuaScriptHost &host_;
    std::function<void(std::int64_t)> dirty_;
  public:
    LuaResearchSelectionServices(LuaScriptHost &host,std::function<void(std::int64_t)> dirty):host_(host),dirty_(std::move(dirty)) {
        if(!dirty_)throw std::invalid_argument("Research Lua bridge requires UI service");
    }
    std::optional<bool> test_all(std::string_view event,std::int64_t player,std::int64_t tech) override {
        if(!host_.loading_complete())throw ResearchSelectionUnavailable("Research Lua content loading is incomplete");
        const ScriptValue args[]={player,tech};const auto result=host_.test_all(event,args);
        if(!result.errors.empty())throw ResearchSelectionUnavailable("Research Lua handler failed: "+result.errors.front());
        if(!host_.loading_complete())throw ResearchSelectionUnavailable("Research Lua content became incomplete during the callback");
        return result.value;
    }
    void research_queue_dirty(std::int64_t team) override {dirty_(team);}
};
} // namespace civ5::runtime
