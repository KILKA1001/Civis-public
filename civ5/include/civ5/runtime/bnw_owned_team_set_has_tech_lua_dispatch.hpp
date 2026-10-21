#pragma once
#include "civ5/runtime/bnw_barbarian_team_has_tech.hpp"
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include <functional>
#include <memory>
#include <thread>

namespace civ5::runtime {
class LuaScriptHost;
class BnwSelectedStockLuaBootstrap;
// Only the actual SourceOwner can mint this service for one current operation.
// It owns no Host/world/VM; the authentic attempt retains their real identity.
// No public callback constructor or readiness/no-listener attestation exists.
class BnwOwnedTeamSetHasTechLuaDispatch final {
 private:
    friend class BnwSelectedStockLuaSourceOwner;
    friend class BnwSelectedStockLuaBootstrap;
    friend class LuaScriptHost;
    struct Arguments {
        std::int64_t team=-1,technology=-1,player=-1;
        bool value=false,first=false,announce=false;
    };
    using Receipt=std::shared_ptr<const BnwMajorTeamHasTechPending>;
    using Begin=std::function<Receipt(const Arguments&)>;
    using Stage=std::function<void(const Receipt&)>;
    const std::thread::id thread_=std::this_thread::get_id();
    const BnwStockLuaAttemptLease lease_;
    const std::uint64_t serial_;
    const Begin begin_;
    const Stage before_hook_,after_hook_,before_researched_,after_researched_;
    std::weak_ptr<const BnwOwnedTeamSetHasTechLuaDispatch> self_;
    explicit BnwOwnedTeamSetHasTechLuaDispatch(BnwStockLuaAttemptLease lease,
        Begin begin,Stage before,Stage after,Stage before_researched={},Stage after_researched={}):lease_(std::move(lease)),
        serial_(lease_.operation_serial()),begin_(std::move(begin)),
        before_hook_(std::move(before)),after_hook_(std::move(after)),
        before_researched_(std::move(before_researched)),after_researched_(std::move(after_researched)){}
    static std::shared_ptr<const BnwOwnedTeamSetHasTechLuaDispatch> issue(
        BnwStockLuaAttemptLease lease,Begin begin,Stage before,Stage after) {
        lease.require_active();
        if(!begin||!before||!after)throw ScriptError("Team:SetHasTech actual source service incomplete");
        auto result=std::shared_ptr<BnwOwnedTeamSetHasTechLuaDispatch>(
            new BnwOwnedTeamSetHasTechLuaDispatch(std::move(lease),std::move(begin),std::move(before),std::move(after)));
        result->self_=result;result->require_current();return result;
    }
    // Legacy four-stage issuance remains source-incomplete at the mandatory
    // researched hook. Only the genuine SourceOwner may issue this suffix.
    static std::shared_ptr<const BnwOwnedTeamSetHasTechLuaDispatch> issue(
        BnwStockLuaAttemptLease lease,Begin begin,Stage before,Stage after,
        Stage before_researched,Stage after_researched) {
        lease.require_active();
        if(!begin||!before||!after||!before_researched||!after_researched)
            throw ScriptError("Team:SetHasTech actual researched source service incomplete");
        auto result=std::shared_ptr<BnwOwnedTeamSetHasTechLuaDispatch>(
            new BnwOwnedTeamSetHasTechLuaDispatch(std::move(lease),std::move(begin),std::move(before),std::move(after),
                std::move(before_researched),std::move(after_researched)));
        result->self_=result;result->require_current();return result;
    }
    void before_researched(const Receipt& result)const {
        require_current();
        if(!before_researched_||!after_researched_)
            throw BnwMajorTeamHasTechUnavailable("TeamTechResearched actual current SourceOwner stages unavailable");
        if(!result||result->phase()!=BnwMajorTeamHasTechPhase::team_tech_researched_pending||result->team_tech_researched_completed())
            throw ScriptError("TeamTechResearched mandatory map frontier missing or consumed");
        before_researched_(result);require_current();
        if(result->phase()!=BnwMajorTeamHasTechPhase::uncertain||result->team_tech_researched_completed())
            throw ScriptError("TeamTechResearched actual attempt not reserved before Lua");
    }
    void after_researched(const Receipt& result)const {
        require_current();after_researched_(result);require_current();
        if(!result->team_tech_researched_completed()||!result->after_research_effect())
            throw ScriptError("TeamTechResearched genuine dispatch/suffix effect missing");
        const auto phase=result->phase();
        const bool researched_players=phase==BnwMajorTeamHasTechPhase::first_research_reward_pending||
            phase==BnwMajorTeamHasTechPhase::technology_announcement_pending||phase==BnwMajorTeamHasTechPhase::technology_share_pending||
            phase==BnwMajorTeamHasTechPhase::resource_announcement_pending||phase==BnwMajorTeamHasTechPhase::owed_culture_building_pending||
            phase==BnwMajorTeamHasTechPhase::research_choice_notification_pending||phase==BnwMajorTeamHasTechPhase::espionage_research_pending||phase==BnwMajorTeamHasTechPhase::major_happiness_pending||
            phase==BnwMajorTeamHasTechPhase::happiness_founder_pending||phase==BnwMajorTeamHasTechPhase::happiness_foreign_founder_pending||
            phase==BnwMajorTeamHasTechPhase::happiness_suffix_pending||phase==BnwMajorTeamHasTechPhase::technology_ui_pending;
        if(researched_players&&result->player_research_effects().empty())
            throw ScriptError("TeamTechResearched actual living-member research/rank effects absent");
        if((phase==BnwMajorTeamHasTechPhase::resource_announcement_pending||phase==BnwMajorTeamHasTechPhase::owed_culture_building_pending||
            phase==BnwMajorTeamHasTechPhase::research_choice_notification_pending||phase==BnwMajorTeamHasTechPhase::espionage_research_pending||phase==BnwMajorTeamHasTechPhase::major_happiness_pending)&&
           !result->announcement_share_effect())
            throw ScriptError("TeamTechResearched actual announcement/share publication missing");
        const bool happiness_phase=phase==BnwMajorTeamHasTechPhase::happiness_founder_pending||
            phase==BnwMajorTeamHasTechPhase::happiness_foreign_founder_pending||phase==BnwMajorTeamHasTechPhase::happiness_suffix_pending;
        if(happiness_phase){
            const auto happiness=result->happiness_prefix_effect();
            if(!happiness||happiness->stores.size()<6||
               (phase==BnwMajorTeamHasTechPhase::happiness_suffix_pending&&!happiness->religion_complete))
                throw ScriptError("TeamTechResearched actual paired happiness/founder effects missing");
        }
        if(phase==BnwMajorTeamHasTechPhase::technology_ui_pending&&result->happiness_suffix_effects().empty())
            throw ScriptError("TeamTechResearched actual living-major happiness and GameData effects absent");
        if(phase==BnwMajorTeamHasTechPhase::major_happiness_pending||happiness_phase||phase==BnwMajorTeamHasTechPhase::technology_ui_pending){
            const auto espionage=result->espionage_prefix_effect();
            if(!espionage||!espionage->complete||espionage->targets_completed!=22)
                throw ScriptError("TeamTechResearched real full espionage target prefix missing");
        }
        if(!researched_players&&phase!=BnwMajorTeamHasTechPhase::artifact_notifications_pending&&phase!=BnwMajorTeamHasTechPhase::era_change_pending&&
           phase!=BnwMajorTeamHasTechPhase::map_reveal_pending&&phase!=BnwMajorTeamHasTechPhase::player_research_pending)
            throw ScriptError("TeamTechResearched actual suffix mandatory frontier missing");
    }
    void require_current()const {
        if(thread_!=std::this_thread::get_id())throw ScriptError("Team:SetHasTech foreign source thread");
        lease_.require_operation(serial_);
    }
    Receipt begin(const Arguments& args)const {
        require_current();
        if(!args.value)throw BnwMajorTeamHasTechUnavailable("Team:SetHasTech false/repeat suffix not implemented");
        const auto result=begin_(args);require_current();
        if(!result||result->phase()!=BnwMajorTeamHasTechPhase::team_set_has_tech_pending)
            throw ScriptError("Team:SetHasTech issuer did not perform paired native prefix");
        const auto& command=result->command();
        // NO_PLAYER may resolve to the genuine current SDK leader in the issuer.
        if(command.team!=args.team||command.technology!=args.technology||
           (args.player!=-1&&command.player!=args.player)||
           command.first!=args.first||command.announce!=args.announce)
            throw ScriptError("Team:SetHasTech issuer substituted actual Lua arguments");
        require_current();return result;
    }
    void before_hook(const Receipt& result)const {
        require_current();before_hook_(result);require_current();
        if(result->phase()!=BnwMajorTeamHasTechPhase::uncertain)
            throw ScriptError("TeamSetHasTech actual attempt was not reserved before Lua");
    }
    void after_hook(const Receipt& result)const {
        require_current();after_hook_(result);require_current();
        const auto phase=result->phase();
        if(!result->policy_free_unit_effect()||
           (phase!=BnwMajorTeamHasTechPhase::first_trait_unit_pending&&
            phase!=BnwMajorTeamHasTechPhase::remaining_process_players_pending&&
            phase!=BnwMajorTeamHasTechPhase::resource_plot_yields_pending&&
            phase!=BnwMajorTeamHasTechPhase::city_routes_pending&&
            phase!=BnwMajorTeamHasTechPhase::team_tech_researched_pending))
            throw ScriptError("TeamSetHasTech actual policy/free-unit stores did not reach their mandatory frontier");
    }
    const char* mandatory_stop(const Receipt& result)const {
        require_current();
        switch(result->phase()){
        case BnwMajorTeamHasTechPhase::first_trait_unit_pending:return "Team:SetHasTech reached trait unit creation pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::remaining_process_players_pending:return "Team:SetHasTech reached next ever-alive member process pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::resource_plot_yields_pending:return "Team:SetHasTech reached resource plot yield/layout pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::city_routes_pending:return "Team:SetHasTech reached city route refresh pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::team_tech_researched_pending:return "Team:SetHasTech reached SAME-VM TeamTechResearched pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::artifact_notifications_pending:return "Team:SetHasTech reached actual artifact notifications pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::era_change_pending:return "Team:SetHasTech reached actual SetCurrentEra pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::map_reveal_pending:return "Team:SetHasTech reached map reveal/deferred fog pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::player_research_pending:return "Team:SetHasTech reached player research/rank invalidation pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::first_research_reward_pending:return "Team:SetHasTech reached real first-research unit/free-tech reward pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::technology_announcement_pending:return "Team:SetHasTech reached actual tech announcement pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::technology_share_pending:return "Team:SetHasTech reached actual updateTechShare pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::resource_announcement_pending:return "Team:SetHasTech reached real resource announcement/force-reveal/findCity pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::owed_culture_building_pending:return "Team:SetHasTech reached real owed culture building grant pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::research_choice_notification_pending:return "Team:SetHasTech reached actual chooseTech notification pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::espionage_research_pending:return "Team:SetHasTech reached actual espionage CanResearch/research-cost list rebuild pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::major_happiness_pending:return "Team:SetHasTech reached actual living-major DoUpdateHappiness pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::happiness_founder_pending:return "Team:SetHasTech reached actual SAME-VM founder accumulator pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::happiness_foreign_founder_pending:return "Team:SetHasTech reached second actual SAME-VM foreign founder accumulator pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::happiness_suffix_pending:return "Team:SetHasTech reached actual natural wonder/minor/league/connection happiness suffix pending; caller not resumed";
        case BnwMajorTeamHasTechPhase::technology_ui_pending:return "Team:SetHasTech reached genuine reward option/notification/popup and GameplayTechAcquired pending; caller not resumed";
        default:throw ScriptError("Team:SetHasTech actual pending frontier changed before mandatory stop");
        }
    }
};
}
