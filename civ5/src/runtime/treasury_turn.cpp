#include "civ5/runtime/treasury_turn.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
bool vp(const TreasuryTurnContext &c) { return c.state.profile==content::TechnologyProfile::vp542; }
void validate(const TreasuryTurnContext &c) {
    const auto profile=c.state.profile;
    if(profile!=c.world.setup.profile||profile!=c.world.diplomacy.profile||profile!=c.world.cities.profile||
       profile!=c.maintenance.profile||profile!=c.rules.profile||profile!=c.city_rules.profile||profile!=c.maintenance_rules.profile||
       c.defines.profile()!=(vp(c)?content::GlobalDefineProfile::vp542:content::GlobalDefineProfile::bnw)||
       (vp(c)?(!c.options||!c.options->all_succeeded()):c.options!=nullptr))
        throw std::runtime_error("Treasury turn profile/rules mismatch");
    if(c.world.setup.game_turn<0)throw std::invalid_argument("Treasury turn cannot have negative game turn");
}
void synchronize_native(TreasuryTurnContext &c,std::int64_t id) {
    if(c.native_mirror) {c.native_mirror->prepare(id);c.native_mirror->synchronize(id);}
}
bool needs_sample(const std::vector<std::int64_t> &history,std::int64_t turn) {
    return history.size()<static_cast<std::uint64_t>(turn);
}
void track_income(TreasuryTurnContext &c,std::int64_t id,std::int64_t amount) {
    auto &h=c.state.players.at(id).income_history_times100;
    // The original .back() on an empty turn-zero history is undefined. Own
    // storage starts its first sample here; no dummy turns or fixed capacity.
    if(h.empty()||needs_sample(h,c.world.setup.game_turn))h.push_back(amount);
    else h.back()=wide::add(h.back(),amount);
}
std::int64_t define(const TreasuryTurnContext &c,const char *name) {
    const auto *v=c.defines.find(name);
    if(!v||!v->lookup.error.empty()||!std::holds_alternative<std::int64_t>(v->lookup.value))
        throw TreasuryUnavailable(std::string("Unresolved treasury turn define: ")+name);
    return std::get<std::int64_t>(v->lookup.value);
}
}
void set_gold_times100(TreasuryTurnContext &c,std::int64_t id,std::int64_t value) {
    validate(c);auto &p=c.state.players.at(id);
    if(p.balance_times100==value)return;
    // BNW only asserts on a negative assignment; VP actually clamps it.
    p.balance_times100=vp(c)?std::max<std::int64_t>(value,0):value;
    synchronize_native(c,id);
    if(id==c.active_player)for(auto kind:{TreasuryDirty::misc_buttons,TreasuryDirty::selection_buttons,TreasuryDirty::game_data})
        c.services.treasury_dirty(id,kind);
}
void change_gold_times100(TreasuryTurnContext &c,std::int64_t id,std::int64_t change) {
    validate(c);auto value=wide::add(c.state.players.at(id).balance_times100,change);
    if(vp(c))value=std::max<std::int64_t>(value,0);
    set_gold_times100(c,id,value);
    if(vp(c)&&change>0)track_income(c,id,change);
}
void set_gold(TreasuryTurnContext &c,std::int64_t id,std::int64_t value) {
    set_gold_times100(c,id,wide::multiply_divide(value,100,1));
}
void change_gold(TreasuryTurnContext &c,std::int64_t id,std::int64_t change) {
    change_gold_times100(c,id,wide::multiply_divide(change,100,1));
}
GoldTurnResult do_gold(TreasuryTurnContext &c,std::int64_t id) {
    validate(c);GoldTurnResult q;auto &p=c.state.players.at(id);
    // BNW's player.calculateGoldRateTimes100 adds an anarchy gate outside
    // Treasury.CalculateBaseNetGoldTimes100. VP has the gate in the latter.
    if(vp(c)||!p.anarchy)q.net_times100=net_gold_times100(c.state,c.world,c.maintenance,c.rules,c.city_rules,
        c.maintenance_rules,c.defines,c.options,id,c.income_services).net_times100;
    // Maintenance queries above store native cached unit/supply costs. Pair
    // those with raw treasury before any later deficit or Lua income provider.
    synchronize_native(c,id);
    q.projected_balance_times100=wide::add(p.balance_times100,q.net_times100);
    if(q.projected_balance_times100<0||(vp(c)&&c.world.cities.players.at(id).minor)) {
        set_gold(c,id,0);
        if(q.projected_balance_times100<=wide::multiply_divide(define(c,"DEFICIT_UNIT_DISBANDING_THRESHOLD"),100,1)) {
            q.deficit_called=true;c.services.deficit(id,c.state.profile);
        }
    } else change_gold_times100(c,id,q.net_times100);
    if(vp(c)&&q.net_times100<0)track_income(c,id,q.net_times100);
    // Re-query after the host's deficit effects, rather than reusing the
    // pre-deficit quote. Scripts, scraps and gifts may change income/state.
    q.gross_gold=gross_gold_times100(c.state,c.world,c.maintenance,c.rules,c.city_rules,c.defines,c.options,id,c.income_services).total_times100/100;
    if(q.gross_gold>0)p.lifetime_gross_gold=wide::add(p.lifetime_gross_gold,q.gross_gold);
    synchronize_native(c,id);
    if(needs_sample(p.balance_history_times100,c.world.setup.game_turn))p.balance_history_times100.push_back(p.balance_times100);
    synchronize_native(c,id);
    if(!vp(c)&&needs_sample(p.income_history_times100,c.world.setup.game_turn))p.income_history_times100.push_back(q.net_times100);
    synchronize_native(c,id);
    // isGameMultiPlayer = network || hotseat || PBEM. VP's achievements human
    // reason deliberately ignores the AI-control options.
    if((!vp(c)||c.options->enabled("ENABLE_ACHIEVEMENTS"))&&id!=-1&&c.world.setup.is_human(c.world.setup.players.at(id))&&
       c.world.setup.game_type!=1&&c.world.setup.game_type!=2&&c.world.setup.game_type!=3) {
        std::int64_t with_trade=0,without_trade=0;
        for(const auto &[cid,city]:c.world.cities.players.at(id).cities)
            with_trade=wide::add(with_trade,city_yield_times100(c.world.cities,c.city_rules,c.defines,id,cid,c.world.cities.gold).total_times100);
        for(const auto &[cid,city]:c.world.cities.players.at(id).cities)
            without_trade=wide::add(without_trade,city_yield_times100(c.world.cities,c.city_rules,c.defines,id,cid,c.world.cities.gold,true).total_times100);
        if(wide::subtract(with_trade,without_trade)/100>=200) {
            q.achievement_requested=true;c.services.unlock_trade_gold_achievement(id);
        }
    }
    return q;
}
} // namespace civ5::runtime
