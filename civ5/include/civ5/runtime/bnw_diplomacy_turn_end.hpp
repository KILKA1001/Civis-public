#pragma once
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/session_checkpoint_identity.hpp"
namespace civ5::runtime {
struct BnwDiplomacyTurnSourceFile {std::string_view path,sha256;};
inline constexpr std::array<BnwDiplomacyTurnSourceFile,2> kBnwDiplomacyTurnSources{{
 {"CvGameCoreDLL_Expansion2/CvDiplomacyRequests.cpp","43b2530e35c845669136f0d2f0684d06a3c08bd639bca8f8dab82685d1f42529"},
 {"CvGameCoreDLL_Expansion2/CvPlayer.cpp","9635fe516e590e133c3907ee25c13128fb666e3e7817c967e247affb700a952d"}
}};
class BnwDiplomacyTurnOwner;
class BnwDiplomacyReadReceipt {
 public:
  BnwDiplomacyReadReceipt(const BnwDiplomacyReadReceipt&)=delete;
  BnwDiplomacyReadReceipt& operator=(const BnwDiplomacyReadReceipt&)=delete;
  BnwDiplomacyReadReceipt(BnwDiplomacyReadReceipt&& o) noexcept :identity_(std::move(o.identity_)),guid_(o.guid_),world_(std::move(o.world_)),epoch_(std::move(o.epoch_)),used_(o.used_){o.used_=true;}
 private:
  friend BnwDiplomacyReadReceipt attest_bnw_diplomacy_read_context(const LiveSessionState&,const saves::SavedSession&,const SessionCheckpointIdentity&,const content::RulesDatabase&);
  friend BnwDiplomacyTurnOwner bind_bnw_diplomacy_turn_read(const LiveSessionState&,BnwDiplomacyReadReceipt&&,const content::RulesDatabase&);
  BnwDiplomacyReadReceipt(SessionCheckpointIdentity i,std::array<std::uint8_t,16> g,std::string w,std::string e):identity_(std::move(i)),guid_(g),world_(std::move(w)),epoch_(std::move(e)){}
  SessionCheckpointIdentity identity_;std::array<std::uint8_t,16> guid_{};std::string world_,epoch_;bool used_=false;
};
BnwDiplomacyReadReceipt attest_bnw_diplomacy_read_context(const LiveSessionState&,const saves::SavedSession&,const SessionCheckpointIdentity&,const content::RulesDatabase&);
class BnwDiplomacyTurnOwner {
 public:
  const auto& identity() const noexcept{return identity_;}const auto& map_guid() const noexcept{return guid_;}
  const auto& epoch() const noexcept{return epoch_;}std::uint64_t operations() const noexcept{return operations_;}
 private:
  friend BnwDiplomacyTurnOwner bind_bnw_diplomacy_turn_read(const LiveSessionState&,BnwDiplomacyReadReceipt&&,const content::RulesDatabase&);
  friend class BnwDiplomacyTurnOperations;
  BnwDiplomacyTurnOwner(SessionCheckpointIdentity i,std::array<std::uint8_t,16> g,std::string e):identity_(std::move(i)),guid_(g),epoch_(std::move(e)){}
  SessionCheckpointIdentity identity_;std::array<std::uint8_t,16> guid_{};std::string epoch_;std::uint64_t operations_=0;
};
BnwDiplomacyTurnOwner bind_bnw_diplomacy_turn_read(const LiveSessionState&,BnwDiplomacyReadReceipt&&,const content::RulesDatabase&);
// SAME retained Read/turn fields; no shadow request map or reset after a command.
std::string bnw_diplomacy_turn_epoch(const LiveSessionState&,const SessionCheckpointIdentity&,const content::RulesDatabase&);
struct BnwDiplomacyTurnDraft {LiveSessionState state;BnwDiplomacyTurnOwner owner;std::int64_t player=-1,before_next_ai=-1,after_next_ai=-1;bool object_present=false;};
struct BnwDiplomacyTurnExecution {std::unique_ptr<BnwDiplomacyTurnDraft> draft;std::string blocker;};
BnwDiplomacyTurnExecution execute_bnw_diplomacy_end_turn_draft(const LiveSessionState&,const BnwDiplomacyTurnOwner&,const content::RulesDatabase&,std::int64_t);
// Only CvDiplomacyRequests::BeginTurn's native scalar assignment. Does not
// perform player activation, send diplomacy or execute AI; useful on owned state.
BnwDiplomacyTurnExecution execute_bnw_diplomacy_begin_turn_scalar_draft(const LiveSessionState&,const BnwDiplomacyTurnOwner&,const content::RulesDatabase&,std::int64_t);
} // namespace civ5::runtime
