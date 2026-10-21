#pragma once
#include "civ5/runtime/file_identity.hpp"
#include <filesystem>
#include <memory>
#include <optional>
namespace civ5::ui {class BnwNativeResourceBootSettingsOwner;}
namespace civ5::runtime {
class BnwSelectedLanguageInputOwner;
class BnwRetainedTurnCursor;
enum class BnwNetDebugConfigOperand {unavailable_or_key_missing, explicit_zero, explicit_nonzero};
enum class BnwNetDebugSelectedConfigKind {same_original_profile, civis_selected_override, process_language_without_config};
struct BnwNetDebugConfigFileFacts {
 std::filesystem::path logical_path,selected_path;
 std::optional<FileIdentity> file;
 BnwNetDebugConfigOperand operand=BnwNetDebugConfigOperand::unavailable_or_key_missing;
};
struct BnwNetDebugConfigFacts {
 BnwNetDebugConfigFileFacts original_profile;
 BnwNetDebugSelectedConfigKind selected_kind=BnwNetDebugSelectedConfigKind::process_language_without_config;
 std::optional<BnwNetDebugConfigFileFacts> selected_civis_config;
 // Config is an input to the real manager setter, never a disabled receipt.
 bool manager_bootstrap_and_registered_log_list_required=true;
};
// Boot-only original versus selected Civis configuration supplier. Neither
// diagnostics nor explicit override proves original manager state or delivery.
class BnwNetMessageDebugConfigOwner final {
 public:
 ~BnwNetMessageDebugConfigOwner();
 void require_current()const;
 BnwNetDebugConfigFacts source_facts()const;
 BnwNetMessageDebugConfigOwner(const BnwNetMessageDebugConfigOwner&)=delete;
 BnwNetMessageDebugConfigOwner&operator=(const BnwNetMessageDebugConfigOwner&)=delete;
 private:
 friend class ::civ5::ui::BnwNativeResourceBootSettingsOwner;
 friend class BnwRetainedTurnCursor;
 friend class BnwNetMessageDebugManagerOwner;
 static std::shared_ptr<const BnwNetMessageDebugConfigOwner> capture_from_boot(
   const std::shared_ptr<const ui::BnwNativeResourceBootSettingsOwner>&);
 void require_same_boot(const ui::BnwNativeResourceBootSettingsOwner&)const;
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwNetMessageDebugConfigOwner(std::shared_ptr<Impl>);
};
} // namespace civ5::runtime
