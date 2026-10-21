#pragma once
#include "civ5/runtime/treasury.hpp"

namespace civ5::runtime {
enum class TreasuryDirty { misc_buttons, selection_buttons, game_data };
// Mandatory host integration. Deficit handling must run DoDeficit (BNW) or
// DoBankruptcy (VP), including unit/AI/random/Lua effects; it is not merely a
// notification. An unavailable implementation must throw, never skip it.
class TreasuryTurnServices {
  public:
    virtual ~TreasuryTurnServices() = default;
    virtual void treasury_dirty(std::int64_t player,TreasuryDirty kind) = 0;
    virtual void deficit(std::int64_t player,content::TechnologyProfile profile) = 0;
    virtual void unlock_trade_gold_achievement(std::int64_t player) = 0;
};
// Optional for generic detached calculations. The BNW live phase must supply
// its actual raw administration mirror. prepare is pure and may reject unknown
// source/int/capacity; synchronize performs only prevalidated no-throw stores.
class TreasuryNativeMirror {
 public:
    virtual ~TreasuryNativeMirror() = default;
    virtual void prepare(std::int64_t player) const = 0;
    virtual void synchronize(std::int64_t player) noexcept = 0;
};
struct TreasuryTurnContext {
    TreasuryContext &state;
    const MaintenanceWorld &world;
    MaintenanceContext &maintenance;
    const TreasuryRules &rules;
    const CityYieldRules &city_rules;
    const MaintenanceRules &maintenance_rules;
    const content::GlobalDefineCache &defines;
    const content::CustomModOptions *options;
    TreasuryTurnServices &services;
    std::int64_t active_player=-1;
    TreasuryServices income_services;
    TreasuryNativeMirror *native_mirror=nullptr;
};
struct GoldTurnResult {
    std::int64_t net_times100=0, projected_balance_times100=0, gross_gold=0;
    bool deficit_called=false, achievement_requested=false;
};
// Source-order mutations on the same treasury used by income/science, with
// ordered UI invalidations delivered to the host.
// These are not transactions: if a required host service throws, prior
// mutations remain. A caller must not retry without restoring its checkpoint.
void set_gold_times100(TreasuryTurnContext &context,std::int64_t player,std::int64_t value);
void change_gold_times100(TreasuryTurnContext &context,std::int64_t player,std::int64_t change);
void set_gold(TreasuryTurnContext &context,std::int64_t player,std::int64_t value);
void change_gold(TreasuryTurnContext &context,std::int64_t player,std::int64_t change);
GoldTurnResult do_gold(TreasuryTurnContext &context,std::int64_t player);
} // namespace civ5::runtime
