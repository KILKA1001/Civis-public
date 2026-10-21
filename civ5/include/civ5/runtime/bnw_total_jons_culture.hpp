#pragma once
#include "civ5/runtime/bnw_grand_world_averages.hpp"
#include "civ5/runtime/bnw_grand_guess_empty.hpp"
namespace civ5::runtime {
struct BnwMinorCultureTerm {std::int64_t minor=-1;std::int32_t trait=-1,influence=0,era=-1,flat=0,per_building=0,total=0;bool ally=false,friends=false,at_war=false;};
struct BnwTotalJonsCultureQuote {
    bool native_early_return=false;
    std::int32_t cities=0,excess=0,traits=0,free=0,minors=0,before_religion=0;
    std::int32_t religion_other=0,religion=0,bonus=0,golden_age=0;
    std::optional<std::int32_t> total;
    std::size_t trade_slots=0,minor_slots=0;
    std::vector<BnwMinorCultureTerm> minor_terms;
    std::uint64_t founder_calls=0;
};
inline constexpr std::array kBnwTotalJonsCultureSources{
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvMinorCivAI.cpp","83995c0c02dd4f3d7805201660c12c00a3ebfaed85e68c0b01744a1803d91055"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvTradeClasses.cpp","6d5d287c04e8ca1b0afb86d9d907ed12dbfb65d29c4594dad213770e0cd0c5c8"},
 BnwGrandWorldAverageSourceFile{"CvGameCoreDLL_Expansion2/CvBeliefClasses.cpp","508ab69d79d79bbc752a6e76110e3f025c6e57dd32dbfbc4880df392e652307c"}
};
enum class BnwTotalJonsCultureStatus { awaiting_founder,complete,discarded };
struct BnwTotalJonsCultureAdvance {bool applied=false,dispatched=false,nonrepeatable=false;std::string blocker;};
class BnwTotalJonsCulturePending {
 public:
    BnwTotalJonsCulturePending(BnwTotalJonsCulturePending&&) noexcept=default;
    BnwTotalJonsCulturePending(const BnwTotalJonsCulturePending&)=delete;
    auto status()const noexcept{return status_;}
    const auto &quote()const noexcept{return quote_;}
    const auto *epoch()const noexcept{return epoch_.get();}
    const auto &before_revision()const noexcept{return before_;}
    const auto &after_revision()const noexcept{return after_;}
    auto player()const noexcept{return player_;}
    const auto &identity()const noexcept{return identity_;}
    const auto &profile()const noexcept{return profile_;}
    // Available only after this actual source producer completed. No publish.
    std::unique_ptr<BnwTeamTechLuaEpoch> take_completed_epoch();
 private:
    friend std::unique_ptr<BnwTotalJonsCulturePending> stage_bnw_total_jons_culture(std::unique_ptr<BnwTeamTechLuaEpoch>&,const SavedContentProfile&,const SessionCheckpointIdentity&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&,std::int64_t);
    friend BnwTotalJonsCultureAdvance continue_bnw_total_jons_culture(BnwTotalJonsCulturePending&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
    friend const BnwTotalJonsCultureQuote &checked_bnw_total_jons_culture(const BnwTotalJonsCulturePending&,const BnwTeamTechLuaEpoch&,const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,const content::RulesDatabase&);
    BnwTotalJonsCulturePending()=default;
    std::unique_ptr<BnwTeamTechLuaEpoch> epoch_;
    SavedContentProfile profile_;SessionCheckpointIdentity identity_;std::int64_t player_=-1;
    BnwTotalJonsCultureQuote quote_;std::string before_,after_;
    const LiveSessionState *world_address_=nullptr;const LuaScriptHost *host_address_=nullptr;
    BnwTotalJonsCultureStatus status_=BnwTotalJonsCultureStatus::awaiting_founder;
};
// The same enclosing epoch is moved only after the native pre-callback prefix
// and seal are accepted. A pre-dispatch rejection retains it without replay;
// any failure after callbacks start discards it and forbids automatic retry.
// Missing stock contexts/listeners are never inferred from an empty registry.
std::unique_ptr<BnwTotalJonsCulturePending> stage_bnw_total_jons_culture(
 std::unique_ptr<BnwTeamTechLuaEpoch>&,const SavedContentProfile&,
 const SessionCheckpointIdentity&,const BnwGrandWorldAverageRules&,
 const BnwHappinessSuffixRules&,const content::RulesDatabase&,std::int64_t);
BnwTotalJonsCultureAdvance continue_bnw_total_jons_culture(
 BnwTotalJonsCulturePending&,const BnwGrandWorldAverageRules&,
 const BnwHappinessSuffixRules&,const content::RulesDatabase&);
// Check at the point the source caller consumes this completed value. Also
// works after take_completed_epoch returned to its SAME enclosing owner;
// altered current world/VM, identities or post-callback inputs are refused.
const BnwTotalJonsCultureQuote &checked_bnw_total_jons_culture(
 const BnwTotalJonsCulturePending&,const BnwTeamTechLuaEpoch&,
 const BnwGrandWorldAverageRules&,const BnwHappinessSuffixRules&,
 const content::RulesDatabase&);
}
