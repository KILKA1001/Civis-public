#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <atomic>
#include <memory>
#include <thread>
namespace civ5::ui { class BnwNativeLogManagerSourceOwner; }
namespace civ5::content { class BnwGlobalsCacheSourceOwner; class RulesDatabase; }
namespace civ5::runtime {
class BnwGrandSelectionPending;
struct BnwTeamTechLuaEpoch;
class BnwGameActivationHappinessPending;
namespace grand_logging_detail {
// Computation/effect-order fixtures only: this interface issues no authority.
enum class Kind { city, grand };
enum class Flag { logging, ai, split };
enum class TextKind { unknown, native_null, bytes };
struct Text { TextKind kind=TextKind::unknown; std::string value; };
struct Receiver { std::function<void(const std::string&)> msg; };
struct Source {
 std::function<bool()> current;
 std::function<std::optional<std::uint8_t>(Kind,Flag)> flag;
 std::function<void(Kind)> construct_strings, destroy_strings;
 std::function<std::optional<Text>(Kind)> civilization;
 std::function<std::shared_ptr<const Receiver>(Kind,const std::string&)> get_log;
 std::function<std::optional<std::int32_t>()> strategy_count;
 std::function<std::optional<std::int32_t>(Kind,std::size_t)> elapsed;
 std::function<std::optional<Text>(Kind,std::int32_t)> strategy_type;
};
enum class Step { logging, ai, strings, civilization, split, get_log, count,
 elapsed, info, msg, cleanup, reason0, complete };
enum class Outcome { dependency_pending, conditional_loggers_returned, quarantined };
struct Progress {
 Kind kind=Kind::grand; Step step=Step::logging;
 std::size_t confirmed_city_messages=0,confirmed_grand_messages=0;
 std::size_t get_log_attempts=0,msg_attempts=0,string_effect_attempts=0;
 bool city_returned=false,reason0_returned=false,grand_returned=false;
 bool external_effect_in_flight=false,quarantined=false;
 std::string frontier;
};
class Engine;
class Journal final {
 public:
 Journal(bool city_pending,std::int32_t active,
   std::array<std::int32_t,4> priorities,std::array<std::int32_t,4> pressure_changes,
   std::optional<bool> interrupt_wonders={});
 Journal(const Journal&)=delete;Journal&operator=(const Journal&)=delete;
 Journal(Journal&&)=delete;Journal&operator=(Journal&&)=delete;
 Progress progress()const;
 private:
 friend class Engine;
 const std::thread::id thread_=std::this_thread::get_id();
 std::atomic<bool> poisoned_{false};bool busy_=false;
 Progress progress_;std::int32_t active_;std::array<std::int32_t,4> priorities_,pressure_;
 std::optional<bool> interrupt_;std::shared_ptr<const Source> source_;
 std::shared_ptr<const Receiver> receiver_;std::string civilization_,filename_,message_;
 std::int32_t count_=0,elapsed_=0;std::size_t row_=0;
};
// SAME ordered engine, deliberately outside actual activation authority.
Outcome resume_diagnostic(std::shared_ptr<Journal>,std::shared_ptr<const Source>);
}
struct BnwGameActivationPendingAccess;
class BnwGrandLoggingContinuation;
// No current producer. Actual source-startup/GC and LogManager owners must
// jointly initialize this private capability after their mandatory prefixes.
class BnwGrandLoggingSourceOwner final {
 friend class civ5::content::BnwGlobalsCacheSourceOwner;
 friend class civ5::ui::BnwNativeLogManagerSourceOwner;
 friend class BnwGrandLoggingContinuation;
 BnwGrandLoggingSourceOwner()=default;
 BnwGrandLoggingSourceOwner(const BnwGrandLoggingSourceOwner&)=delete;
 std::shared_ptr<const civ5::content::BnwGlobalsCacheSourceOwner> globals_;
 std::shared_ptr<const content::RulesDatabase> database_;
 std::shared_ptr<const civ5::ui::BnwNativeLogManagerSourceOwner> logs_;
 std::shared_ptr<const BnwGameActivationHappinessPending> activation_;
 // Epoch is externally owned at the current call site, not a Pending child.
 std::shared_ptr<const BnwTeamTechLuaEpoch> epoch_owner_;
 std::shared_ptr<const grand_logging_detail::Source> program_;
 std::function<void()> require_current_;
 const BnwTeamTechLuaEpoch*epoch_=nullptr;
 const BnwGrandSelectionPending*selection_=nullptr;
};
// Root retains this once alongside the ACTUAL selection; not a copied Pending.
class BnwGrandLoggingContinuation final {
 friend struct BnwGameActivationPendingAccess;
 BnwGrandLoggingContinuation(const BnwGrandSelectionPending&,
    const BnwTeamTechLuaEpoch&,const content::RulesDatabase&,
    std::shared_ptr<const BnwGrandLoggingSourceOwner>);
 public:
 BnwGrandLoggingContinuation(const BnwGrandLoggingContinuation&)=delete;
 BnwGrandLoggingContinuation&operator=(const BnwGrandLoggingContinuation&)=delete;
 static grand_logging_detail::Outcome resume(std::shared_ptr<BnwGrandLoggingContinuation>);
 grand_logging_detail::Progress progress()const{return journal_->progress();}
 private:
 void require_current()const;
 const BnwGrandSelectionPending*selection_;const BnwTeamTechLuaEpoch*epoch_;
 const content::RulesDatabase*database_;
 std::shared_ptr<const BnwGrandLoggingSourceOwner> source_;
 std::shared_ptr<grand_logging_detail::Journal> journal_;
};
}



