#pragma once
#include "civ5/runtime/bnw_net_message_debug_config_owner.hpp"
#include <cstdint>
#include <memory>
#include <string_view>
namespace civ5::ui {class BnwNativeResourceBootSettingsOwner;class SelectedBnwPreviewHost;}
namespace civ5::runtime {
class BnwRetainedTurnCursor;
enum class BnwNetDebugOwnDelivery { actual_disabled_return, actual_file_write_and_flush };
struct BnwNetDebugOwnManagerFacts {
 bool own_manager_constructed=false,enabled=false,original_manager_state_proven=false;
 bool original_path_and_format_parity_proven=false;
 std::filesystem::path actual_own_log_path;
 std::uint64_t registrations=0,write_calls=0,written_bytes=0,flush_calls=0;
 BnwNetDebugSelectedConfigKind config_lane=BnwNetDebugSelectedConfigKind::process_language_without_config;
 BnwNetDebugConfigOperand operand=BnwNetDebugConfigOperand::unavailable_or_key_missing;
};
// Real standalone manager, owned registry and OS file sink. The original PE
// manager/list/path/format is not represented by this diagnostic facts object.
class BnwNetMessageDebugManagerOwner final {
 public:
 ~BnwNetMessageDebugManagerOwner();
 void require_current()const;
 // Real fixed diagnostic producer, never a GameTurn or world-ready receipt.
 BnwNetDebugOwnDelivery write_bootstrap_diagnostic()const;
 BnwNetDebugOwnManagerFacts source_facts()const;
 BnwNetMessageDebugManagerOwner(const BnwNetMessageDebugManagerOwner&)=delete;
 BnwNetMessageDebugManagerOwner&operator=(const BnwNetMessageDebugManagerOwner&)=delete;
 private:
 friend class ::civ5::ui::BnwNativeResourceBootSettingsOwner;
 friend class BnwRetainedTurnCursor;
 static std::shared_ptr<const BnwNetMessageDebugManagerOwner> capture_from_boot(
   const std::shared_ptr<const ui::BnwNativeResourceBootSettingsOwner>&);
 void require_same_boot(const ui::BnwNativeResourceBootSettingsOwner&)const;
 void require_same_host(const ui::SelectedBnwPreviewHost&)const;
 std::shared_ptr<const BnwNetMessageDebugManagerOwner> retain_factory_owner_without_callbacks()const;
 void require_source_bookends()const;
 BnwNetDebugOwnDelivery submit_owned_message(std::string_view)const;
 // Future original ordered clock issuer alone may submit after its own source
 // reservation. This does not receive arbitrary callbacks or receipt booleans.
 BnwNetDebugOwnDelivery submit_game_turn_message(std::int32_t turn)const;
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwNetMessageDebugManagerOwner(std::shared_ptr<Impl>);
};
} // namespace civ5::runtime
