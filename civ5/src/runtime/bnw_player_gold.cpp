#include "civ5/runtime/bnw_live_player_gold_pending.hpp"
#include <algorithm>
#include <array>
#include <thread>
#include "civ5/runtime/bnw_player_gold.hpp"
#include <exception>
#include <limits>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {
namespace {
constexpr auto bnw=content::TechnologyProfile::bnw;
constexpr auto bnw_layout=saves::GameStateLayout::expansion2_version1;

std::int64_t unit_scalar(const saves::UnitCoreObservation &core,
                         std::string_view name) {
    const saves::UnitCoreField *found=nullptr;
    for (const auto &field:core.fields)
        if (field.name && std::string_view(field.name)==name) {
            if (found) throw TreasuryUnavailable("BNW deficit unit field duplicated");
            found=&field;
        }
    if (!found || found->values.size()!=1)
        throw TreasuryUnavailable("BNW deficit unit field unavailable");
    return found->values.front();
}
std::int64_t city_scalar(const saves::CityCoreObservation &core,
                         std::string_view name) {
    const saves::CityCoreField *found=nullptr;
    for (const auto &field:core.fields)
        if (field.name && std::string_view(field.name)==name) {
            if (found) throw TreasuryUnavailable("BNW deficit city field duplicated");
            found=&field;
        }
    if (!found || found->values.size()!=1)
        throw TreasuryUnavailable("BNW deficit city field unavailable");
    return found->values.front();
}

void prove_no_disband(const LiveSessionState &state,std::int64_t player,
                      const BnwPlayerGoldServices &services) {
    if (!services.deficit_unit_roster_current ||
        !services.deficit_city_roster_current ||
        state.world.game_state.layout!=bnw_layout ||
        !state.world.all_players ||
        state.world.all_players->layout!=bnw_layout ||
        state.cities.profile!=bnw)
        throw TreasuryUnavailable("BNW DoDeficit needs current unit and city rosters");
    const saves::SavedPlayerObservation *owner=nullptr;
    for (const auto &candidate:state.world.all_players->players)
        if (candidate.saved_player_id==player) {
            if (owner) throw TreasuryUnavailable("BNW DoDeficit player duplicated");
            owner=&candidate;
        }
    const auto live=state.cities.players.find(player);
    if (!owner || live==state.cities.players.end() ||
        owner->units.layout!=bnw_layout ||
        owner->cities.layout!=bnw_layout ||
        owner->cities.cities.size()!=live->second.cities.size() ||
        owner->cities.header.wire_city_count!=
            static_cast<std::int64_t>(owner->cities.cities.size()) ||
        owner->units.header.wire_unit_count!=
            static_cast<std::int64_t>(owner->units.units.size()) ||
        owner->cities.cities.size()>static_cast<std::size_t>(INT32_MAX))
        throw TreasuryUnavailable("BNW DoDeficit city/unit roster unavailable");
    std::set<std::int64_t> city_ids;
    for (const auto &city:owner->cities.cities) {
        if (city.core.layout!=bnw_layout)
            throw TreasuryUnavailable("BNW DoDeficit city profile mismatch");
        const auto id=city_scalar(city.core,"m_iID");
        const auto city_owner=city_scalar(city.core,"m_eOwner");
        if (id<0 || city_owner!=player || !city_ids.insert(id).second ||
            !live->second.cities.contains(id))
            throw TreasuryUnavailable("BNW DoDeficit city roster changed");
    }
    std::set<std::int64_t> ids;
    std::int64_t combat=0;
    for (const auto &unit:owner->units.units) {
        if (unit.core.layout!=bnw_layout)
            throw TreasuryUnavailable("BNW DoDeficit unit profile mismatch");
        const auto id=unit_scalar(unit.core,"m_iID");
        const auto unit_owner=unit_scalar(unit.core,"m_eOwner");
        const auto strength=unit_scalar(unit.core,"m_iBaseCombat");
        if (id<0 || unit_owner!=player || !ids.insert(id).second ||
            strength<0 || strength>INT32_MAX)
            throw TreasuryUnavailable("BNW DoDeficit unit identity/combat invalid");
        // CvUnit::IsCombatUnit is exactly m_iBaseCombat > 0.
        if (strength>0) {
            if (combat==INT32_MAX)
                throw TreasuryUnavailable("BNW DoDeficit combat count overflow");
            ++combat;
        }
    }
    if (combat>static_cast<std::int64_t>(owner->cities.cities.size()))
        throw TreasuryUnavailable("BNW DoDeficit disband branch needs live AI/Lua services");
    // Expansion2 CvPlayer.cpp:16848: when combat units <= cities, DoDeficit
    // returns without drawing RNG, selecting a unit or publishing an event.
}

class ActualRawTreasuryMirror final : public TreasuryNativeMirror {
 public:
    ActualRawTreasuryMirror(LiveSessionState &state,std::int64_t id):state_(state),id_(id),creator_(std::this_thread::get_id()) {
        if(id<0||id>=64||state.world.game_state.layout!=bnw_layout||!state.world.all_players||
           state.world.all_players->layout!=bnw_layout||state.world.all_players->players.size()!=64||
           !state.treasury.players.contains(id)||!state.maintenance.players.contains(id))
            throw TreasuryUnavailable("BNW gold mirror actual world/player unavailable");
        auto &p=state.world.all_players->players.at(static_cast<std::size_t>(id));
        auto &owned=state.treasury.players.at(id);auto &maintenance=state.maintenance.players.at(id);
        if(p.index!=static_cast<std::uint64_t>(id)||p.saved_player_id!=id||p.core.layout!=bnw_layout||
           p.administration.layout!=bnw_layout||p.administration.blocks.size()!=3||
           !p.administration.blocks[1].name||std::string_view(p.administration.blocks[1].name)!="treasury"||
           owned.source_core!=std::optional{std::pair{p.core.start_offset,p.core.end_offset}}||
           owned.source_administration!=std::optional{std::pair{p.administration.start_offset,p.administration.end_offset}})
            throw TreasuryUnavailable("BNW gold mirror physical raw/owned source differs");
        player_=&p;record_=&p.administration.blocks[1];
        const std::array<std::string_view,6> names={"m_iGold","m_iLifetimeGrossGoldIncome","m_iExpensePerTurnUnitMaintenance","m_iExpensePerTurnUnitSupply","m_GoldBalanceForTurnTimes100_wire_count","m_GoldChangeForTurnTimes100_wire_count"};
        for(std::size_t n=0;n<names.size();++n){fields_[n]=&field(names[n]);expected_[n]=fields_[n]->value;}
        arrays_[0]=&array("m_GoldBalanceForTurnTimes100");arrays_[1]=&array("m_GoldChangeForTurnTimes100");
        if(expected_[0]!=owned.balance_times100||expected_[1]!=owned.lifetime_gross_gold||
           expected_[2]!=maintenance.cached_unit_expense||expected_[3]!=maintenance.cached_supply_expense||
           arrays_[0]->values!=owned.balance_history_times100||arrays_[1]->values!=owned.income_history_times100)
            throw TreasuryUnavailable("BNW gold mirror requires current paired Treasury/Maintenance/raw fields");
        for(std::size_t n=0;n<2;++n){auto &raw=arrays_[n]->values;
            if(raw.size()>=static_cast<std::size_t>(INT32_MAX)||expected_[n+4]!=static_cast<std::int64_t>(raw.size()))
                throw TreasuryUnavailable("BNW gold mirror raw history count invalid");
            raw.reserve(raw.size()+1);
        }
        owned.balance_history_times100.reserve(owned.balance_history_times100.size()+1);
        owned.income_history_times100.reserve(owned.income_history_times100.size()+1);
        prepare(id);
    }
    void prepare(std::int64_t id) const override {
        if(std::this_thread::get_id()!=creator_||id!=id_||!state_.world.all_players||
           state_.world.all_players->players.size()!=64||&state_.world.all_players->players.at(static_cast<std::size_t>(id))!=player_||
           player_->administration.blocks.size()!=3||&player_->administration.blocks[1]!=record_)
            throw TreasuryUnavailable("BNW gold mirror physical source allocation changed");
        const std::array<std::string_view,6> names={"m_iGold","m_iLifetimeGrossGoldIncome","m_iExpensePerTurnUnitMaintenance","m_iExpensePerTurnUnitSupply","m_GoldBalanceForTurnTimes100_wire_count","m_GoldChangeForTurnTimes100_wire_count"};
        for(std::size_t n=0;n<fields_.size();++n)if(&field(names[n])!=fields_[n]||fields_[n]->value!=expected_[n])
            throw TreasuryUnavailable("BNW gold mirror raw target changed outside native producer");
        if(&array("m_GoldBalanceForTurnTimes100")!=arrays_[0]||&array("m_GoldChangeForTurnTimes100")!=arrays_[1])
            throw TreasuryUnavailable("BNW gold mirror raw history allocation changed");
        const auto &owned=state_.treasury.players.at(id);
        const auto &maintenance=state_.maintenance.players.at(id);
        for(const auto v:{owned.balance_times100,owned.lifetime_gross_gold,maintenance.cached_unit_expense,maintenance.cached_supply_expense})
            if(v<INT32_MIN||v>INT32_MAX)throw TreasuryUnavailable("BNW gold mirror changed value exceeds native i32");
        for(std::size_t n=0;n<2;++n){const auto& raw=arrays_[n]->values;
            const auto& live=n?owned.income_history_times100:owned.balance_history_times100;
            if(raw.size()!=static_cast<std::size_t>(expected_[n+4])||live.size()<raw.size()||live.size()>raw.size()+1||
               live.size()>raw.capacity()||live.size()>static_cast<std::size_t>(INT32_MAX)||!std::equal(raw.begin(),raw.end(),live.begin()))
                throw TreasuryUnavailable("BNW gold mirror native history prefix/capacity changed");
            for(const auto v:live)if(v<INT32_MIN||v>INT32_MAX)throw TreasuryUnavailable("BNW gold mirror native history value exceeds i32");
        }
    }
    void synchronize(std::int64_t id) noexcept override {
        const auto &owned=state_.treasury.players.at(id);const auto& maintenance=state_.maintenance.players.at(id);
        expected_[0]=owned.balance_times100;expected_[1]=owned.lifetime_gross_gold;
        expected_[2]=maintenance.cached_unit_expense;expected_[3]=maintenance.cached_supply_expense;
        for(std::size_t n=0;n<2;++n){auto& raw=arrays_[n]->values;const auto& live=n?owned.income_history_times100:owned.balance_history_times100;
            // prepare proved capacity/unchanged prefix. No allocation/provider
            // lies between these actual stores and the next callback.
            raw.resize(live.size());std::copy(live.begin(),live.end(),raw.begin());expected_[n+4]=static_cast<std::int64_t>(live.size());}
        for(std::size_t n=0;n<fields_.size();++n)fields_[n]->value=expected_[n];
    }
 private:
    saves::NamedSaveInteger& field(std::string_view name) const {
        saves::NamedSaveInteger* found=nullptr;for(auto& f:record_->fields)if(f.name&&std::string_view(f.name)==name){if(found)throw TreasuryUnavailable("BNW gold mirror duplicate raw field");found=&f;}
        if(!found)throw TreasuryUnavailable("BNW gold mirror raw field missing");return *found;
    }
    saves::PlotArrayObservation& array(std::string_view name) const {
        saves::PlotArrayObservation* found=nullptr;for(auto& a:record_->arrays)if(a.name&&std::string_view(a.name)==name){if(found)throw TreasuryUnavailable("BNW gold mirror duplicate raw history");found=&a;}
        if(!found||found->columns!=1)throw TreasuryUnavailable("BNW gold mirror raw history missing");return *found;
    }
    LiveSessionState& state_;std::int64_t id_;std::thread::id creator_;
    saves::SavedPlayerObservation* player_=nullptr;saves::PlayerAdministrationRecord* record_=nullptr;
    std::array<saves::NamedSaveInteger*,6> fields_{};std::array<std::int64_t,6> expected_{};
    std::array<saves::PlotArrayObservation*,2> arrays_{};
};
class GuardedServices final : public TreasuryTurnServices {
  public:
    GuardedServices(const LiveSessionState &state,
                    const BnwPlayerGoldServices &services)
        :state_(state),services_(services) { dirty_.reserve(3); }
    void treasury_dirty(std::int64_t,TreasuryDirty kind) override {
        dirty_.push_back(static_cast<std::uint8_t>(kind));
    }
    void deficit(std::int64_t player,content::TechnologyProfile) override {
        deficit_=true;
        prove_no_disband(state_,player,services_);
    }
    void unlock_trade_gold_achievement(std::int64_t) override {
        achievement_=true;
    }
    const std::vector<std::uint8_t> &dirty() const noexcept { return dirty_; }
    bool deficit_called() const noexcept { return deficit_; }
    bool achievement() const noexcept { return achievement_; }
  private:
    const LiveSessionState &state_;
    const BnwPlayerGoldServices &services_;
    std::vector<std::uint8_t> dirty_;
    bool deficit_=false,achievement_=false;
};
} // namespace

std::unique_ptr<TreasuryNativeMirror> BnwLivePlayerGoldPending::make_actual_mirror(LiveSessionState& state,std::int64_t actor){
 return std::make_unique<ActualRawTreasuryMirror>(state,actor);
}

LiveActionResult run_bnw_player_gold_phase(
    LiveSessionState &state,std::int64_t player,
    const BnwPlayerGoldRules &rules,const BnwPlayerGoldServices &services) {
    if (!services.lua_ready || !services.founder_benefits_read_only ||
        !services.lua_ready())
        return {LiveActionOutcome::rejected,
                "BNW gold requires a ready read-only Lua income service"};
    if (state.player_gold_publication)
        return {LiveActionOutcome::rejected,
                "BNW gold has unpublished active-player UI effects"};
    if (rules.active_player==player && !services.dirty_after_commit)
        return {LiveActionOutcome::rejected,
                "BNW gold active player requires a post-commit UI service"};
    if (state.setup.profile!=bnw || state.player_research.profile!=bnw ||
        state.cities.profile!=bnw || state.maintenance.profile!=bnw ||
        state.treasury.profile!=bnw || rules.treasury.profile!=bnw ||
        rules.cities.profile!=bnw || rules.maintenance.profile!=bnw ||
        rules.defines.profile()!=content::GlobalDefineProfile::bnw ||
        !state.setup.players.contains(player) ||
        !state.player_research.players.contains(player) ||
        !state.cities.players.contains(player) ||
        !state.maintenance.players.contains(player) ||
        !state.treasury.players.contains(player))
        return {LiveActionOutcome::rejected,
                "BNW gold requires matching owned player and rules state"};

    std::optional<ActualRawTreasuryMirror> mirror;
    try { mirror.emplace(state,player); }
    catch(const std::exception& error) {return {LiveActionOutcome::rejected,std::string("BNW gold raw mirror unavailable: ")+error.what()};}
    GuardedServices guarded(state,services);
    MaintenanceWorld world{state.setup,state.player_research,state.cities,
                           state.technologies};
    TreasuryServices income;
    income.founder_benefits_religion=[&](std::int64_t id) {
        if (!services.lua_ready())
            throw TreasuryUnavailable("BNW gold Lua host became unavailable");
        return services.founder_benefits_read_only(id,state);
    };
    TreasuryTurnContext context{state.treasury,world,state.maintenance,
        rules.treasury,rules.cities,rules.maintenance,rules.defines,nullptr,
        guarded,rules.active_player,std::move(income),&*mirror};
    // Reserve a concrete in-flight diagnostic before the first native write.
    // Rejected detached actions must be discarded; retained diagnostic copies
    // may not export/retry partially executed gold as a completed phase.
    state.player_gold_publication=BnwPlayerGoldPublication{player,{},0,true};
    state.player_gold_publication->dirty_kinds.reserve(3);
    try {
        (void)do_gold(context,player);
    } catch (const std::exception &error) {
        state.player_gold_publication->dirty_kinds.assign(guarded.dirty().begin(),guarded.dirty().end());
        return {LiveActionOutcome::rejected,
                std::string("BNW gold blocked after reserved native phase: ")+error.what()};
    } catch (...) {
        state.player_gold_publication->dirty_kinds.assign(guarded.dirty().begin(),guarded.dirty().end());
        throw;
    }
    if (!guarded.dirty().empty()) {
        state.player_gold_publication->dirty_kinds.assign(guarded.dirty().begin(),guarded.dirty().end());
        state.player_gold_publication->uncertain=false;
    } else state.player_gold_publication.reset();
    // CvTreasury::DoGold invokes only gDLL->UnlockAchievement here. This
    // external Steam side effect is excluded from this milestone, as is
    // CvPlayerAchievements::StartTurn. No gameplay state depends on it.
    (void)guarded.achievement();
    return {LiveActionOutcome::committed,{}};
}

BnwTurnHandler make_bnw_player_gold_handler(
    BnwPlayerGoldRules rules,BnwPlayerGoldServices services) {
    const auto publisher=services;
    return {BnwTurnPhase::player_gold,
            [rules,services=std::move(services)](LiveSessionState &state,
                                                 std::int64_t player) {
                return run_bnw_player_gold_phase(state,player,rules,services);
            },
            [publisher](LiveSession &session) {
                if(!session.state().player_gold_publication)
                    return LiveActionResult{LiveActionOutcome::committed,{}};
                return publish_bnw_player_gold(session,publisher);
            }};
}

LiveActionResult apply_bnw_player_gold(
    LiveSession &session,std::int64_t player,
    const BnwPlayerGoldRules &rules,const BnwPlayerGoldServices &services) {
    auto result=apply_live_action(session,[&](LiveSessionState &state) {
        return run_bnw_player_gold_phase(state,player,rules,services);
    });
    if (result.outcome==LiveActionOutcome::rejected ||
        !session.state().player_gold_publication) return result;
    return publish_bnw_player_gold(session,services);
}

LiveActionResult publish_bnw_player_gold(
    LiveSession &session,const BnwPlayerGoldServices &services) {
    const auto &pending=session.state().player_gold_publication;
    if (!pending)
        return {LiveActionOutcome::rejected,"BNW gold has no UI publication"};
    if (!services.dirty_after_commit || pending->uncertain ||
        pending->next_step>pending->dirty_kinds.size())
        return {LiveActionOutcome::rejected,
                "BNW gold UI publication is unavailable or uncertain"};
    while (session.state().player_gold_publication &&
           session.state().player_gold_publication->next_step<
           session.state().player_gold_publication->dirty_kinds.size()) {
        const auto step=*session.state().player_gold_publication;
        const auto kind=step.dirty_kinds[step.next_step];
        if (kind>static_cast<std::uint8_t>(TreasuryDirty::game_data))
            return {LiveActionOutcome::rejected,"BNW gold UI dirty kind is invalid"};
        // Treat the external call as in flight before it runs. A reentrant
        // publisher must not observe this same step as available again.
        session.mutable_state().player_gold_publication->uncertain=true;
        try {
            services.dirty_after_commit(step.player,static_cast<TreasuryDirty>(kind));
        } catch (const std::exception &error) {
            session.mutable_state().player_gold_publication->uncertain=true;
            return {LiveActionOutcome::committed,
                    std::string("BNW gold committed; UI publication uncertain: ")+error.what()};
        } catch (...) {
            session.mutable_state().player_gold_publication->uncertain=true;
            return {LiveActionOutcome::committed,
                    "BNW gold committed; UI publication uncertain"};
        }
        ++session.mutable_state().player_gold_publication->next_step;
        session.mutable_state().player_gold_publication->uncertain=false;
    }
    session.mutable_state().player_gold_publication.reset();
    return {LiveActionOutcome::committed,{}};
}

LiveActionResult run_bnw_player_turn_with_gold(
    LiveSession &session,std::int64_t player,
    std::span<const BnwTurnHandler> other_handlers,
    const BnwPlayerGoldRules &rules,const BnwPlayerGoldServices &services) {
    std::vector<BnwTurnHandler> handlers(other_handlers.begin(),other_handlers.end());
    handlers.push_back(make_bnw_player_gold_handler(rules,services));
    return run_bnw_player_turn(session,player,handlers);
}

} // namespace civ5::runtime
