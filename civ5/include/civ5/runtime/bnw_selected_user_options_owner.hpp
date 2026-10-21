#pragma once
#include "civ5/ui/detail/notification_context_root_small_ui_policy_loan.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <memory>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <cstdint>
#include <vector>
namespace civ5::ui {class BnwNativeResourceBootSettingsOwner;class BnwNativePackageDiscoveryOwner;}
namespace civ5::runtime {
class BnwSelectedWorldViewContextOwner;
class BnwSelectedLanguageInputOwner;
struct BnwUserOptionsPregameSlotObservation {
 std::string section,key;std::int32_t value=0;bool default_inserted=false;
};
enum class BnwUserOptionsInputKind { existing_file, absent_file };
// Measured source facts, never an issuer or permission for a gameplay callback.
struct BnwUserOptionsSourceReceipt {
 BnwUserOptionsInputKind kind=BnwUserOptionsInputKind::absent_file;
 std::filesystem::path logical_path,selected_path;
 std::optional<FileIdentity> file;
 bool default_inserted=false;
 // Original profile is read-only. This owned slot needs Civis profile persistence.
 bool pending_civis_profile_persistence=false;
};
// Closed early factory, actual UserSettings.ini/GameSettings native value.
// Diagnostic source/bytes are not a late gameplay callback permission.
class BnwSelectedUserOptionsOwner final {
 public:
 ~BnwSelectedUserOptionsOwner();
 void require_current()const;
 BnwUserOptionsSourceReceipt source_receipt()const;
 // Copied registry facts; neither setters nor Game/loader authority.
 std::vector<BnwUserOptionsPregameSlotObservation> pregame_input_observation()const;
 BnwSelectedUserOptionsOwner(const BnwSelectedUserOptionsOwner&)=delete;
 BnwSelectedUserOptionsOwner&operator=(const BnwSelectedUserOptionsOwner&)=delete;
 private:
 friend class ::civ5::ui::BnwNativeResourceBootSettingsOwner;
 friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
 friend class BnwSelectedStockLuaSourceOwner;
 friend class BnwWorldViewSourceOperationLoan;
 friend class ::civ5::ui::detail::NotificationContextRootSmallUiPolicyLoan;
 friend class BnwSelectedStockLuaBootstrap;
 friend class BnwMajorTeamHasTechPending;
 friend class BnwFreshGame;
 void require_same_game_input(const BnwSelectedLanguageInputOwner&)const;
 std::int32_t last_civilization()const;
 static std::shared_ptr<const BnwSelectedUserOptionsOwner> capture_from_boot(
  const std::shared_ptr<const ui::BnwNativeResourceBootSettingsOwner>&);
 bool no_reward_popups()const;
 bool minimap_game_setting_owned(std::string_view)const;
 std::unique_ptr<const ui::detail::NotificationContextRootSmallUiPolicyLoan> context_small_ui_policy_owned(
  const BnwWorldViewSourceOperationLoan&,const std::weak_ptr<BnwSelectedWorldViewContextOwner>&,std::uint64_t)const;
 void require_same_boot(const ui::BnwNativeResourceBootSettingsOwner&)const;
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwSelectedUserOptionsOwner(std::shared_ptr<Impl>);
};
}
