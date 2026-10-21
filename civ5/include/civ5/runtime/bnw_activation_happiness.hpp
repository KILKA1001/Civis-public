#pragma once
#include "civ5/runtime/bnw_happiness_ui_pump.hpp"
#include "civ5/runtime/bnw_activation_connections.hpp"
#include "civ5/runtime/bnw_total_jons_culture.hpp"
#include "civ5/runtime/bnw_grand_culture_priority.hpp"
#include "civ5/runtime/bnw_grand_first_roll.hpp"
#include "civ5/runtime/bnw_grand_united_nations_category.hpp"
#include "civ5/runtime/bnw_grand_second_roll.hpp"
#include "civ5/runtime/bnw_grand_spaceship_category.hpp"
#include "civ5/runtime/bnw_grand_third_roll.hpp"
#include "civ5/runtime/bnw_grand_conquest_category.hpp"
#include "civ5/runtime/bnw_grand_fourth_roll.hpp"
#include "civ5/runtime/bnw_grand_pressure.hpp"
#include "civ5/runtime/bnw_grand_selection.hpp"
namespace civ5::runtime {
class BnwGameActivationHappinessPending;
class BnwGrandFirstRollJournal;
enum class BnwActivationHappinessStage {native_prefix,awaiting_selected_lua,native_suffix_dirty_pending,ui_consumed,failed};
class BnwActivationHappinessOwner {
 public:
    BnwActivationHappinessOwner(BnwActivationHappinessOwner &&)=default;
    BnwActivationHappinessOwner &operator=(BnwActivationHappinessOwner &&)=default;
    BnwActivationHappinessOwner(const BnwActivationHappinessOwner &)=delete;
    BnwActivationHappinessStage stage() const noexcept{return stage_;}
    const LiveSessionState &state() const;
    const std::string &frontier() const noexcept{return frontier_;}
    std::uint64_t native_prefix_executions()const noexcept{return 1;}
    std::uint64_t religion_dispatches()const noexcept{return religion_dispatches_;}
    bool native_update_complete()const noexcept{return stage_==BnwActivationHappinessStage::native_suffix_dirty_pending||stage_==BnwActivationHappinessStage::ui_consumed;}
    bool ui_consumed()const noexcept{return stage_==BnwActivationHappinessStage::ui_consumed;}
    const BnwTotalJonsCulturePending *total_culture()const noexcept{return total_culture_.get();}
    const BnwGrandCulturePriorityPending *grand_culture_priority()const noexcept{return grand_culture_priority_.get();}
    const BnwGrandFirstRollJournal *grand_first_roll()const noexcept{return grand_first_roll_?&*grand_first_roll_:nullptr;}
    const BnwGrandUnitedNationsPriorityJournal *grand_united_nations_category()const noexcept{return grand_united_nations_category_?&*grand_united_nations_category_:nullptr;}
    const BnwGrandSecondRollJournal *grand_second_roll()const noexcept{return grand_second_roll_?&*grand_second_roll_:nullptr;}
    const BnwGrandSpaceshipPriorityJournal *grand_spaceship_category()const noexcept{return grand_spaceship_category_?&*grand_spaceship_category_:nullptr;}
    const BnwGrandThirdRollJournal *grand_third_roll()const noexcept{return grand_third_roll_?&*grand_third_roll_:nullptr;}
    const BnwGrandConquestPriorityJournal *grand_conquest_category()const noexcept{return grand_conquest_category_?&*grand_conquest_category_:nullptr;}
    const BnwGrandFourthRollJournal *grand_fourth_roll()const noexcept{return grand_fourth_roll_?&*grand_fourth_roll_:nullptr;}
    const BnwGrandPressureJournal *grand_pressure()const noexcept{return grand_pressure_?&*grand_pressure_:nullptr;}
    const BnwGrandSelectionPending *grand_selection()const noexcept{return grand_selection_?&*grand_selection_:nullptr;}
 private:
    friend struct BnwActivationHappinessAccess;
    friend struct BnwGameActivationPendingAccess;
    friend BnwGrandSelectionPending stage_bnw_grand_selection(
        const BnwGameActivationHappinessPending &,BnwTeamTechLuaEpoch &,
        const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,
        const content::RulesDatabase &,std::shared_ptr<const BnwGrandMinimumAgeOwner>);
    friend BnwGrandPressureJournal stage_bnw_grand_pressure(
        const BnwGameActivationHappinessPending &,BnwTeamTechLuaEpoch &,
        const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,const content::RulesDatabase &);
    friend BnwGrandConquestPriorityJournal stage_bnw_grand_conquest_category(
        const BnwGameActivationHappinessPending &,const BnwTeamTechLuaEpoch &,
        const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,const content::RulesDatabase &);
    friend BnwGrandFourthRollJournal stage_bnw_grand_fourth_roll(
        const BnwGameActivationHappinessPending &,BnwTeamTechLuaEpoch &,
        const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,const content::RulesDatabase &);
    friend BnwGrandSpaceshipPriorityJournal stage_bnw_grand_spaceship_category(
        const BnwGameActivationHappinessPending &,const BnwTeamTechLuaEpoch &,
        const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,const content::RulesDatabase &);
    friend BnwGrandThirdRollJournal stage_bnw_grand_third_roll(
        const BnwGameActivationHappinessPending &,BnwTeamTechLuaEpoch &,
        const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,const content::RulesDatabase &);
    friend BnwGrandSecondRollJournal stage_bnw_grand_second_roll(
        const BnwGameActivationHappinessPending &,BnwTeamTechLuaEpoch &,
        const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,
        const content::RulesDatabase &);
    friend BnwGrandFirstRollJournal stage_bnw_grand_first_roll(
        const BnwGameActivationHappinessPending &,BnwTeamTechLuaEpoch &,
        const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,
        const content::RulesDatabase &);
    friend BnwGrandUnitedNationsPriorityJournal stage_bnw_grand_united_nations_category(
        const BnwGameActivationHappinessPending &,const BnwTeamTechLuaEpoch &,
        const BnwGrandWorldAverageRules &,const BnwHappinessSuffixRules &,
        const content::RulesDatabase &);
    BnwActivationHappinessOwner(std::unique_ptr<BnwHappinessPrefixDraft> prefix,BnwHappinessPrefixRules pr,BnwHappinessSuffixRules sr,BnwReligionHappinessRules rr,BnwHappinessGameDataOwner client)
        :prefix_(std::move(prefix)),prefix_rules_(std::move(pr)),suffix_rules_(std::move(sr)),religion_rules_(std::move(rr)),client_(std::move(client)){}
    // Prefix stays alive for source_world pointer in the sole detached epoch.
    std::unique_ptr<BnwHappinessPrefixDraft> prefix_;
    BnwHappinessPrefixRules prefix_rules_;BnwHappinessSuffixRules suffix_rules_;BnwReligionHappinessRules religion_rules_;
    BnwHappinessGameDataOwner client_;std::unique_ptr<BnwHappinessSuffixExecution> suffix_;
    // The next source query temporarily owns the SAME epoch. state() must
    // keep exposing that current world rather than the earlier prefix copy.
    std::unique_ptr<BnwTotalJonsCulturePending> total_culture_;
    std::unique_ptr<BnwGrandCulturePriorityPending> grand_culture_priority_;
    std::optional<BnwGrandFirstRollJournal> grand_first_roll_;
    std::optional<BnwGrandUnitedNationsPriorityJournal> grand_united_nations_category_;
    std::optional<BnwGrandSecondRollJournal> grand_second_roll_;
    std::optional<BnwGrandSpaceshipPriorityJournal> grand_spaceship_category_;
    std::optional<BnwGrandThirdRollJournal> grand_third_roll_;
    std::optional<BnwGrandConquestPriorityJournal> grand_conquest_category_;
    std::optional<BnwGrandFourthRollJournal> grand_fourth_roll_;
    std::optional<BnwGrandPressureJournal> grand_pressure_;
    std::optional<BnwGrandSelectionPending> grand_selection_;
    BnwActivationHappinessStage stage_=BnwActivationHappinessStage::native_prefix;
    std::uint64_t religion_dispatches_=0;std::string frontier_;
};
struct BnwActivationHappinessStart {std::unique_ptr<BnwActivationHappinessOwner> owner;std::string blocker;};
// Bind at the actual source happiness step after the exact native activation
// prefix/CityConnections and gold. This is not an independent active-human
// DoTurn request. Caller is the enclosing private activation executor.
BnwActivationHappinessStart stage_bnw_activation_happiness(
    const LiveSessionState &,const BnwActivationConnectionsOwner &,
    const content::RulesDatabase &,const SessionCheckpointIdentity &,std::int64_t player,
    std::span<const BnwGameActivationStep> actual_completed_prefix);
// Missing actual stock epoch retains the sealed native prefix for later bind,
// without executing it again. Once an actual verified religion RHS is entered,
// failure closes this attempt; no re-dispatch or native update retry is allowed.
LiveActionResult continue_bnw_activation_happiness(BnwActivationHappinessOwner &,
    const SavedContentProfile &,BnwGameplayLuaEpochOwner &,const content::RulesDatabase &);
// Explicit separate standalone client pump; does not publish or advance turn.
LiveActionResult consume_bnw_activation_happiness_ui(BnwActivationHappinessOwner &,
    const content::RulesDatabase &);
}
