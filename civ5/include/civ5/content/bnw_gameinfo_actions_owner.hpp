#pragma once
#include "civ5/content/rules_database.hpp"
#include "civ5/content/detail/bnw_original_action_order.hpp"
#include <array>
#include <atomic>
#include <bit>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <variant>
#include <vector>
#include <algorithm>
#include <utility>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::content {
using BnwGameInfoActionField=std::pair<std::string_view,DatabaseScriptValue>;
using BnwGameInfoAction=std::array<BnwGameInfoActionField,28>;
// Genuine current typed hotkey caches and SetGlobalActionInfo result. Rows own
// all strings. SQL row/Localization callbacks never survive construction.
class BnwGameInfoActionsOwner final {
 public:
  BnwGameInfoActionsOwner(const BnwGameInfoActionsOwner&)=delete;
  BnwGameInfoActionsOwner&operator=(const BnwGameInfoActionsOwner&)=delete;
  void require_current()const;
  void require_same_rules(const RulesDatabase&)const;
  const std::vector<BnwGameInfoAction>& actions()const;
 private:
  friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
  enum class Subtype:std::int32_t {interface_mode=0,command=1,build=2,promotion=3,specialist=4,control=5,automate=6,mission=7};
  struct Hotkey {
   std::int32_t authored_id=-1,action_index=-1,hotkey_value=-1,priority=-1,
    alt_value=-1,alt_priority=-1,order_priority=0,mission=-1,command=-1,automate=-1;
   bool alt=false,shift=false,ctrl=false,alt_alt=false,shift_alt=false,ctrl_alt=false,
    confirm=false,visible=false;
   std::string type,help,disabled_help,hotkey,hotkey_string,description,
    description_key,hotkey_description_key,hotkey_alt_description_key;
   std::optional<std::string> text_key;
  };
  struct Collection {std::map<std::int32_t,Hotkey> slots;std::int32_t span=0;};
  struct Key {std::string_view key;std::int32_t value;};
  struct KeyText {std::string_view key,text;bool localized;};
  const std::thread::id creator_=std::this_thread::get_id();
  mutable std::atomic<bool> failed_{false};
  std::shared_ptr<const RulesDatabase> database_;
  std::function<bool()> current_;
  std::array<Collection,8> collections_;
  std::vector<BnwGameInfoAction> actions_;
  explicit BnwGameInfoActionsOwner(std::shared_ptr<const RulesDatabase>);
  static std::shared_ptr<const BnwGameInfoActionsOwner> create_owned(
   std::shared_ptr<const RulesDatabase>,const InfoTypeCatalog&,
   const std::function<std::string(std::string_view)>&,const std::function<void()>&);
  static std::int32_t get_int(std::int64_t value)noexcept;
  static std::int32_t lookup(const InfoTypeCatalog&,const std::optional<std::string>&);
  static std::int32_t key_value(std::string_view);
  static std::string key_string(std::string_view,bool,bool,bool,
   const std::function<std::string(std::string_view)>&);
  void prefetch(Subtype,std::string_view,const InfoTypeCatalog&,
   const std::function<std::string(std::string_view)>&,const std::function<void()>&);
  void set_global_action_info(const InfoTypeCatalog&,
   const std::function<std::string(std::string_view)>&,const std::function<void()>&);
  BnwGameInfoAction action_fields(Subtype,std::int32_t,const Hotkey&)const;
};
#include "civ5/content/detail/bnw_gameinfo_actions_core.inc"
}
