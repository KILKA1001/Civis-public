#pragma once
#include "civ5/runtime/live_session.hpp"
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
enum class BnwDealPrefixError {missing_parent,wrong_thread,stale_source,already_attempted,invalid_source,native_overflow};
class BnwDealPrefixRefusal final:public std::runtime_error {
 public:BnwDealPrefixRefusal(BnwDealPrefixError,const std::string&);
 BnwDealPrefixError code()const noexcept{return code_;}
 private:BnwDealPrefixError code_;
};
enum class BnwDealPrefixBoundary {
 not_attempted,expiry_scan,validity_scan,empty_returned,list_migration_returned,
 notifications_required,resource_happiness_required,resource_description_notifications_required,
 resource_total_required,team_effect_required,league_vote_effect_required,
 active_player_ui_required,failed_source
};
enum class BnwDealPrefixStoreKind {to_renewed_false,diplomacy_gold,resource_export};
struct BnwDealPrefixStore {
 BnwDealPrefixStoreKind kind;std::size_t deal=0,item=0;
 std::int32_t player=-1,resource=-1,before=0,after=0;
};
struct BnwDealPrefixObservation {
 BnwDealPrefixBoundary boundary=BnwDealPrefixBoundary::not_attempted;
 bool attempted=false,something_changed=false;
 std::size_t deal_index=0,item_index=0,migrated=0;
 std::int32_t last_from=-1,last_to=-1;
 std::vector<BnwDealPrefixStore> stores;
 std::string required_effect;
};
namespace detail {
// Detached source calculation/stores only. Supplied state/rules never issue a
// retained owner or a turn/notification/publication capability. Same engine is
// called by the actual selected owner, so scenario tests do not duplicate it.
void run_bnw_deals_detached_prefix(LiveSessionState&,const content::RulesDatabase&,const content::InfoTypeCatalog&,
 BnwDealPrefixObservation&);
}
class BnwGameDealsTurnPrefix final {
 public:
 static std::shared_ptr<BnwGameDealsTurnPrefix> from_selected_host(
   std::shared_ptr<const ui::SelectedBnwPreviewHost>);
 BnwGameDealsTurnPrefix(const BnwGameDealsTurnPrefix&)=delete;
 BnwGameDealsTurnPrefix& operator=(const BnwGameDealsTurnPrefix&)=delete;
 ~BnwGameDealsTurnPrefix();
 // No supplied player/turn/item/effect result. Once across shared aliases.
 void advance();
 void require_current()const;
 BnwDealPrefixObservation observe()const;
 std::shared_ptr<const LiveSessionState> read_detached_state()const;
 static constexpr bool live_publication_ready()noexcept{return false;}
 private:
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwGameDealsTurnPrefix(std::unique_ptr<Impl>);
};
}
