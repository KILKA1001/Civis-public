#pragma once
#include "civ5/runtime/bnw_grand_pressure.hpp"
#include <stdexcept>
namespace civ5::content { class BnwGlobalsCacheSourceOwner; }
namespace civ5::runtime {
// Diagnostic source arithmetic only; none of these operands issue authority.
struct BnwGrandSelectionDecision {
 bool evaluated=false,minimum_age_read=false,set_active_called=false,active_written=false,specialization_called=false;
 std::int32_t best_strategy=-1,best_priority=-1,active_after=-1,age_after=0;
 std::uint32_t slots_visited=0;
};
inline BnwGrandSelectionDecision evaluate_bnw_grand_selection(
 const std::array<std::int32_t,4>&priorities,std::int32_t active,std::int32_t incremented_age,
 std::optional<std::int32_t> minimum_age={}) {
 if(active<-1||active>=4||incremented_age<0)throw std::runtime_error("BNW Grand selection native active/age invalid");
 BnwGrandSelectionDecision q;q.active_after=active;q.age_after=incremented_age;
 if(active!=-1){q.minimum_age_read=true;if(!minimum_age)throw std::runtime_error("BNW Grand selection actual cached minimum age unavailable");if(incremented_age<*minimum_age)return q;}
 q.evaluated=true;
 for(std::size_t i=0;i<priorities.size();++i){++q.slots_visited;if(priorities[i]>q.best_priority){q.best_priority=priorities[i];q.best_strategy=static_cast<std::int32_t>(i);}}
 if(q.best_strategy!=active){q.set_active_called=true;q.specialization_called=true;if(q.best_strategy!=-1){q.active_written=true;q.active_after=q.best_strategy;q.age_after=0;}}
 return q;
}
class BnwGrandSelectionPending;
struct BnwGrandSelectionProgress;
// No current producer exists. Only future genuine GC cached-store/lifecycle
// code may initialize this sealed child. No raw scalar constructor/factory.
class BnwGrandMinimumAgeOwner final {
 friend class civ5::content::BnwGlobalsCacheSourceOwner;
 friend class BnwGrandSelectionPending;
 friend const BnwGrandSelectionProgress& checked_bnw_grand_selection(const BnwGrandSelectionPending&,const BnwTeamTechLuaEpoch&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
 friend BnwGrandSelectionPending stage_bnw_grand_selection(const BnwGameActivationHappinessPending&,BnwTeamTechLuaEpoch&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&,std::shared_ptr<const BnwGrandMinimumAgeOwner>);
 BnwGrandMinimumAgeOwner()=default;
 BnwGrandMinimumAgeOwner(const BnwGrandMinimumAgeOwner&)=delete;
 BnwGrandMinimumAgeOwner&operator=(const BnwGrandMinimumAgeOwner&)=delete;
 std::int32_t require_current(const content::RulesDatabase&)const;
 std::shared_ptr<const civ5::content::BnwGlobalsCacheSourceOwner> source_;
 std::shared_ptr<const content::RulesDatabase> database_;
 std::function<bool()> actual_source_current_;
 std::optional<std::int32_t> cached_value_;
};
enum class BnwGrandSelectionFrontier {city_specialization_log,grand_strategy_log};
struct BnwGrandSelectionProgress {
 std::int32_t active_before=-1,incremented_age=0;
 std::array<std::int32_t,4> priorities{},pressure_log_changes{};
 BnwGrandSelectionDecision decision;
 std::optional<std::int32_t> minimum_age;
 std::optional<bool> minor;
 bool dirty_written=false;
 std::optional<bool> dirty_before,dirty_after,interrupt_wonders;
 BnwGrandSelectionFrontier frontier=BnwGrandSelectionFrontier::grand_strategy_log;
};
class BnwGrandSelectionPending {
 public:
 BnwGrandSelectionPending(BnwGrandSelectionPending&&)noexcept=default;
 BnwGrandSelectionPending&operator=(BnwGrandSelectionPending&&)noexcept=default;
 BnwGrandSelectionPending(const BnwGrandSelectionPending&)=delete;
 BnwGrandSelectionPending&operator=(const BnwGrandSelectionPending&)=delete;
 auto player()const noexcept{return player_;}
 const auto&identity()const noexcept{return identity_;}
 const auto&profile()const noexcept{return profile_;}
 const auto&progress()const noexcept{return progress_;}
 const auto&frontier()const noexcept{return frontier_;}
 const auto&before_grand_revision()const noexcept{return before_grand_;}
 const auto&after_grand_revision()const noexcept{return after_grand_;}
 private:
 friend BnwGrandSelectionPending stage_bnw_grand_selection(const BnwGameActivationHappinessPending&,BnwTeamTechLuaEpoch&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&,std::shared_ptr<const BnwGrandMinimumAgeOwner>);
 friend const BnwGrandSelectionProgress& checked_bnw_grand_selection(const BnwGrandSelectionPending&,const BnwTeamTechLuaEpoch&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
 BnwGrandSelectionPending()=default;
 SessionCheckpointIdentity identity_;SavedContentProfile profile_;std::int64_t player_=-1;
 BnwGrandSelectionProgress progress_;std::string before_grand_,after_grand_,before_specialization_,after_specialization_,source_grand_,roster_,frontier_;
 std::array<SimulationRandomState,2> raw_{},prefix_{},live_{};
 const LiveSessionState*world_=nullptr,*source_=nullptr;const LuaScriptHost*host_=nullptr;
 std::shared_ptr<const BnwGrandMinimumAgeOwner> minimum_age_;
};
BnwGrandSelectionPending stage_bnw_grand_selection(
 const BnwGameActivationHappinessPending&,BnwTeamTechLuaEpoch&,
 const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&,
 std::shared_ptr<const BnwGrandMinimumAgeOwner> minimum_age={});
// Narrow retained prefix check only. Neither mandatory logger has completed.
const BnwGrandSelectionProgress& checked_bnw_grand_selection(
 const BnwGrandSelectionPending&,const BnwTeamTechLuaEpoch&,
 const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
}
