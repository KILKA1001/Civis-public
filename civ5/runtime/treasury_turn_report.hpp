#pragma once
#include "civ5/runtime/treasury_turn.hpp"
#include <iomanip>
#include <sstream>

namespace civ5::validation {
struct TreasuryTurnProbeWorld final : runtime::TreasuryTurnServices {
    std::vector<const char *> events;
    void treasury_dirty(std::int64_t,runtime::TreasuryDirty kind) override {
        switch(kind) {
        case runtime::TreasuryDirty::misc_buttons:events.push_back("misc_buttons_dirty");break;
        case runtime::TreasuryDirty::selection_buttons:events.push_back("selection_buttons_dirty");break;
        case runtime::TreasuryDirty::game_data:events.push_back("game_data_dirty");break;
        }
    }
    void deficit(std::int64_t,content::TechnologyProfile) override {
        events.push_back("deficit_world_required");
        throw runtime::TreasuryUnavailable("Bankruptcy requires live unit/AI/random/Lua services");
    }
    void unlock_trade_gold_achievement(std::int64_t) override {events.push_back("trade_gold_achievement_requested");}
};
// Each player runs on an independent owned copy of the imported state. This
// verifies DoGold integration, not the order of a complete multiplayer turn.
inline std::string treasury_turn_report(const runtime::TreasuryContext &source,const runtime::MaintenanceWorld &w,
    const runtime::MaintenanceContext &maintenance,const runtime::TreasuryRules &rules,const runtime::CityYieldRules &cr,
    const runtime::MaintenanceRules &mr,const content::GlobalDefineCache &d,const content::CustomModOptions *o,
    std::int64_t active_player) {
    std::ostringstream out;
    const auto list=[&](const auto &v){out<<'[';bool comma=false;for(auto n:v){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    out<<"{\"disposable_copies\":true,\"full_turn\":false,\"steam_called\":false,\"game_turn\":"<<w.setup.game_turn
       <<",\"game_type\":"<<w.setup.game_type<<",\"active_player\":"<<active_player
       <<",\"deficit_threshold\":"<<std::get<std::int64_t>(d.find("DEFICIT_UNIT_DISBANDING_THRESHOLD")->lookup.value)
       <<",\"enable_achievements\":"<<((!o||o->enabled("ENABLE_ACHIEVEMENTS"))?"true":"false")<<",\"players\":[";
    bool comma=false;
    for(const auto &[id,original]:source.players) {
        if(comma)out<<',';comma=true;
        auto state=source;auto cache=maintenance;TreasuryTurnProbeWorld services;
        runtime::TreasuryTurnContext context{state,w,cache,rules,cr,mr,d,o,services,active_player,{}};
        std::optional<runtime::GoldTurnResult> q;std::string error;
        try {q=runtime::do_gold(context,id);}
        catch(const runtime::TreasuryUnavailable &e){error=e.what();}
        catch(const runtime::CityYieldUnavailable &e){error=e.what();}
        catch(const runtime::MaintenanceUnavailable &e){error=e.what();}
        const auto &p=state.players.at(id);
        out<<"{\"id\":"<<id<<",\"status\":"<<std::quoted(q?"applied":"unresolved")<<",\"error\":"<<std::quoted(error)
           <<",\"minor\":"<<(w.cities.players.at(id).minor?"true":"false")<<",\"quote\":";
        if(q)out<<"{\"net_times100\":"<<q->net_times100<<",\"projected_balance_times100\":"<<q->projected_balance_times100
            <<",\"gross_gold\":"<<q->gross_gold<<",\"deficit_called\":"<<(q->deficit_called?"true":"false")
            <<",\"achievement_requested\":"<<(q->achievement_requested?"true":"false")<<'}';else out<<"null";
        out<<",\"balance_times100\":"<<p.balance_times100<<",\"lifetime_gross_gold\":"<<p.lifetime_gross_gold
           <<",\"balance_history_times100\":";list(p.balance_history_times100);
        out<<",\"income_history_times100\":";list(p.income_history_times100);
        out<<",\"unit_expense_after\":"<<cache.players.at(id).cached_unit_expense<<",\"supply_expense_after\":"<<cache.players.at(id).cached_supply_expense;
        out<<",\"events\":[";bool ec=false;for(auto event:services.events){if(ec)out<<',';ec=true;out<<std::quoted(event);}out<<"]}";
    }
    out<<"]}";return out.str();
}
} // namespace civ5::validation
