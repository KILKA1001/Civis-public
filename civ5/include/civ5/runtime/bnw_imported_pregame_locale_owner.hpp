#pragma once
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include <memory>
#include <string>
#include <cstdint>
namespace civ5::saves {struct SavedSession;}
namespace civ5::assets {class CivisResourceBankOwner;}
namespace civ5::content {class RulesDatabase;}
namespace civ5::runtime {
class BnwSpClientPregameStartup;class BnwSpClientUpdateLoan;
// Actual early Civis Bank + selected language + activated localization. This
// source does not borrow a later WorldView/UI frame or native Locale DLL.
class BnwImportedPregameLocaleOwner final {
 friend class BnwSpClientPregameStartup;
 struct Impl;
 std::shared_ptr<Impl> impl_;
 explicit BnwImportedPregameLocaleOwner(std::shared_ptr<Impl>);
 void require_import(const saves::SavedSession&,const content::RulesDatabase&,
  const SessionCheckpointIdentity&)const;
 std::shared_ptr<const BnwImportedPregameLocaleOwner> retain_canonical()const;
 std::string default_nickname(bool,std::int32_t)const;
 void publish_registration_player_event(const BnwSpClientPregameStartup&,std::int32_t)const;
 void publish_actual_client155_event(const BnwSpClientPregameStartup&,const BnwSpClientUpdateLoan&,std::uint32_t)const;
 void publish_actual_control_client_event(const BnwSpClientPregameStartup&,const BnwSpClientUpdateLoan&,std::uint32_t)const;
 void require_first_release_client_debug_configuration(const BnwSpClientPregameStartup&)const;
public:
 static std::shared_ptr<const BnwImportedPregameLocaleOwner> capture(
  std::shared_ptr<const assets::CivisResourceBankOwner>,const content::RulesDatabase&,
  const assets::ResourceMountTable& same_activated_rules_resources);
 ~BnwImportedPregameLocaleOwner();
 void require_current()const;
 BnwImportedPregameLocaleOwner(const BnwImportedPregameLocaleOwner&)=delete;
 BnwImportedPregameLocaleOwner&operator=(const BnwImportedPregameLocaleOwner&)=delete;
};
}
