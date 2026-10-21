// Header implementation; no new compilation unit/build-system dependency.
inline std::int32_t BnwGameInfoActionsOwner::get_int(std::int64_t value)noexcept{
 return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value));
}
inline BnwGameInfoActionsOwner::BnwGameInfoActionsOwner(std::shared_ptr<const RulesDatabase> db):database_(std::move(db)){
 if(!database_)throw std::invalid_argument("Actions actual selected RulesDatabase absent");
 current_=database_->immutable_preview_fingerprint_currentness();require_current();
}
inline void BnwGameInfoActionsOwner::require_current()const{
 if(creator_!=std::this_thread::get_id()){failed_.store(true);throw std::runtime_error("Actions foreign constructor thread");}
 if(failed_.load()||!current_||!current_()){
  failed_.store(true);throw std::runtime_error("Actions SAME immutable RulesDatabase retired/poisoned");
 }
}
inline void BnwGameInfoActionsOwner::require_same_rules(const RulesDatabase& actual)const{
 require_current();if(database_.get()!=&actual)throw std::runtime_error("Actions another RulesDatabase wrapper");require_current();
}
inline const std::vector<BnwGameInfoAction>& BnwGameInfoActionsOwner::actions()const{require_current();return actions_;}
inline std::int32_t BnwGameInfoActionsOwner::lookup(const InfoTypeCatalog& types,const std::optional<std::string>& name){
 const auto*at=name?types.find(*name):nullptr;return at?get_int(at->id):-1;
}
inline std::int32_t BnwGameInfoActionsOwner::key_value(std::string_view input){
#include "civ5/content/detail/bnw_gameinfo_actions_key_map.inc"
 for(const auto&key:integer_keys)if(key.key==input)return key.value;
 return -1;
}
inline std::string BnwGameInfoActionsOwner::key_string(std::string_view input,bool shift,bool alt,bool ctrl,
 const std::function<std::string(std::string_view)>& localized){
#include "civ5/content/detail/bnw_gameinfo_actions_key_map.inc"
 std::string result;if(input.empty())return result;
 if(shift)result+=localized("TXT_KEY_SHIFT");
 if(alt)result+=localized("TXT_KEY_ALT");
 if(ctrl)result+=localized("TXT_KEY_CTRL");
 for(const auto&key:text_keys)if(key.key==input){result+=key.localized?localized(key.text):std::string(key.text);break;}
 return result;
}
inline void BnwGameInfoActionsOwner::prefetch(Subtype subtype,std::string_view table,const InfoTypeCatalog& types,
 const std::function<std::string(std::string_view)>& localized,const std::function<void()>& source_current){
 require_current();source_current();auto&collection=collections_.at(static_cast<std::size_t>(subtype));
 collection.slots.clear();collection.span=0;
 const auto report=database_->prefetch_info(std::string(table),[&](std::uint64_t slot,std::int64_t id,const RulesRowView& row){
  require_current();source_current();
  if(slot>=static_cast<std::uint64_t>(INT32_MAX))throw std::runtime_error("Actions typed slot exceeds signed source index");
  Hotkey info;info.authored_id=get_int(id);
  const auto text=[&](std::string_view name){return row.text(name).value_or("");};
  info.type=text("Type");info.text_key=row.text("Text");
  if(info.text_key&&info.text_key->empty())info.text_key.reset();
  info.help=text("Help");info.disabled_help=text("DisabledHelp");
  if(info.text_key)(void)localized(*info.text_key); // CvBaseInfo real text cache effect.
  const auto description=row.text("Description");
  if(description){info.description_key=*description;info.description=localized(*description);
   if(!info.text_key&&!description->empty())info.text_key=*description;
  }
  info.hotkey=text("HotKey");info.hotkey_value=key_value(info.hotkey);
  info.alt_value=key_value(text("HotKeyAlt"));
  info.priority=get_int(row.integer("HotKeyPriority"));info.alt_priority=get_int(row.integer("HotKeyPriorityAlt"));
  info.order_priority=get_int(row.integer("OrderPriority"));
  info.alt=row.boolean("AltDown");info.shift=row.boolean("ShiftDown");info.ctrl=row.boolean("CtrlDown");
  info.alt_alt=row.boolean("AltDownAlt");info.shift_alt=row.boolean("ShiftDownAlt");info.ctrl_alt=row.boolean("CtrlDownAlt");
  info.hotkey_description_key=info.text_key.value_or("");
  info.hotkey_string=key_string(info.hotkey,info.shift,info.alt,info.ctrl,localized);
  if(subtype==Subtype::command||subtype==Subtype::automate){
   info.automate=lookup(types,row.text("Automate"));info.confirm=row.boolean("ConfirmCommand");info.visible=row.boolean("Visible");
   if(subtype==Subtype::automate)info.command=lookup(types,row.text("Command"));
  }else if(subtype==Subtype::mission||subtype==Subtype::interface_mode)info.visible=row.boolean("Visible");
  if(subtype==Subtype::interface_mode)info.mission=lookup(types,row.text("Mission"));
  // Cache only fields reached by the SDK ActionInfo getters/link writes.
  // This does not assert complete Build/Promotion/Specialist gameplay caches.
  collection.slots.emplace(static_cast<std::int32_t>(slot),std::move(info));
  collection.span=static_cast<std::int32_t>(slot+1);require_current();source_current();return true;
 });
 source_current();require_current();
 if(!report.all_succeeded())throw std::runtime_error("Actions typed PrefetchCollection SQL/cache failure: "+std::string(table)+" "+report.error);
 if(report.slots>static_cast<std::uint64_t>(INT32_MAX))throw std::runtime_error("Actions typed collection span overflow");
 collection.span=static_cast<std::int32_t>(report.slots);
}
inline void BnwGameInfoActionsOwner::set_global_action_info(const InfoTypeCatalog& types,
 const std::function<std::string(std::string_view)>& localized,const std::function<void()>& source_current){
 require_current();source_current();actions_.clear(); // Retire old actions before new publication, SDK order.
 std::int32_t mission_count=0;bool count_returned=false;
 const auto count=database_->query_rows({"SELECT COUNT(*) AS Count FROM Missions",{}},[&](const RulesRowView&row){
  const auto value=row.integer("Count");if(value<0||value>INT32_MAX)throw std::runtime_error("Actions CvTypes mission Count overflow");
  mission_count=static_cast<std::int32_t>(value);count_returned=true;
 },1);
 if(!count.all_succeeded()||!count_returned)throw std::runtime_error("Actions actual CvTypes mission Count unavailable");
 struct Ordered {Subtype subtype;std::int32_t index,priority;};std::vector<Ordered> order;
 const auto collect=[&](Subtype subtype,std::int32_t range){
  const auto&collection=collections_.at(static_cast<std::size_t>(subtype));
  for(auto at=collection.slots.begin();at!=collection.slots.end()&&at->first<range;++at)
   order.push_back({subtype,at->first,at->second.order_priority});
 };
 // SDK fixed enum ranges:10 Commands,24 InterfaceModes,44 Controls.
 collect(Subtype::command,10);collect(Subtype::interface_mode,24);
 collect(Subtype::build,collections_[2].span);collect(Subtype::promotion,collections_[3].span);
 collect(Subtype::specialist,collections_[4].span);collect(Subtype::control,44);
 collect(Subtype::automate,collections_[6].span);collect(Subtype::mission,mission_count);
 action_order_detail::OriginalActionOrder<Ordered>::run(order);
 actions_.reserve(order.size());
 for(const auto&entry:order){
  require_current();source_current();
  if(actions_.size()>static_cast<std::size_t>(INT32_MAX))throw std::runtime_error("Actions output index overflow");
  auto&info=collections_.at(static_cast<std::size_t>(entry.subtype)).slots.at(entry.index);
  const auto action_index=static_cast<std::int32_t>(actions_.size());
  // Link writes precede the new ActionInfo push_back, in the SDK order.
  if(entry.subtype==Subtype::build)info.mission=lookup(types,std::string("MISSION_BUILD"));
  else if(entry.subtype==Subtype::promotion)info.command=lookup(types,std::string("COMMAND_PROMOTION"));
  else if(entry.subtype==Subtype::specialist)info.mission=lookup(types,std::string("MISSION_JOIN"));
  info.action_index=action_index;
  if(entry.subtype==Subtype::promotion||entry.subtype==Subtype::specialist){
   const bool promotion=entry.subtype==Subtype::promotion;
   const auto linked=promotion?info.command:info.mission;
   const auto linked_type=promotion?Subtype::command:Subtype::mission;
   const auto&linked_collection=collections_.at(static_cast<std::size_t>(linked_type));
   const auto linked_info=linked_collection.slots.find(linked);
   if(linked_info==linked_collection.slots.end())throw std::runtime_error("Actions required mission/command description link is absent");
   info.hotkey_string=key_string(info.hotkey,info.shift,info.alt,info.ctrl,localized);
   info.hotkey_description_key=info.description; // SDK GetDescription(), not Description key substitution.
   info.hotkey_alt_description_key=linked_info->second.description;
  }
  source_current();require_current();actions_.push_back(action_fields(entry.subtype,entry.index,info));
 }
 require_current();source_current();
}
inline BnwGameInfoAction BnwGameInfoActionsOwner::action_fields(Subtype subtype,std::int32_t index,const Hotkey& h)const{
 const bool command=subtype==Subtype::command,automate=subtype==Subtype::automate,
  build=subtype==Subtype::build,specialist=subtype==Subtype::specialist,promotion=subtype==Subtype::promotion,
  mission=subtype==Subtype::mission,is_interface=subtype==Subtype::interface_mode,control=subtype==Subtype::control;
 const auto i=[](std::int32_t value)->DatabaseScriptValue{return std::int64_t(value);};
 const auto t=[](const std::optional<std::string>&value)->DatabaseScriptValue{return value?DatabaseScriptValue(*value):DatabaseScriptValue(std::monostate{});};
 return {{{"MissionData",i(build||specialist?index:-1)},
  {"CommandData",i(promotion?index:(command||automate?h.automate:-1))},
  {"AutomateType",i(command||automate?h.automate:-1)},
  {"InterfaceModeType",i(is_interface?index:-1)},
  {"MissionType",i(build||specialist?h.mission:(mission?index:-1))},
  {"CommandType",i(command?index:(promotion||automate?h.command:-1))},
  {"ControlType",i(control?index:-1)},{"OriginalIndex",i(index)},
  {"ConfirmCommand",command||automate?h.confirm:false},
  {"Visible",control?false:(command||automate||mission||is_interface?h.visible:true)},
  {"SubType",i(static_cast<std::int32_t>(subtype))},{"Type",h.type},{"TextKey",t(h.text_key)},
  {"ActionInfoIndex",i(h.action_index)},{"HotKeyVal",i(h.hotkey_value)},
  {"HotKeyPriority",i(h.priority)},{"HotKeyValAlt",i(h.alt_value)},
  {"HotKeyPriorityAlt",i(h.alt_priority)},{"OrderPriority",i(h.order_priority)},
  {"AltDown",h.alt},{"ShiftDown",h.shift},{"CtrlDown",h.ctrl},
  {"AltDownAlt",h.alt_alt},{"ShiftDownAlt",h.shift_alt},{"CtrlDownAlt",h.ctrl_alt},
  {"HotKey",h.hotkey_string},{"Help",h.help},{"DisabledHelp",h.disabled_help}}};
}
inline std::shared_ptr<const BnwGameInfoActionsOwner> BnwGameInfoActionsOwner::create_owned(
 std::shared_ptr<const RulesDatabase> db,const InfoTypeCatalog& types,
 const std::function<std::string(std::string_view)>& localized,const std::function<void()>& source_current){
 if(!localized||!source_current||!types.report().all_succeeded())throw std::runtime_error("Actions actual selected typed/Localization source absent");
 source_current();auto result=std::shared_ptr<BnwGameInfoActionsOwner>(new BnwGameInfoActionsOwner(std::move(db)));
 try{
  // Relevant SDK PrefetchGameData collection order, retaining real slot holes.
  result->prefetch(Subtype::interface_mode,"InterfaceModes",types,localized,source_current);
  result->prefetch(Subtype::specialist,"Specialists",types,localized,source_current);
  result->prefetch(Subtype::automate,"Automates",types,localized,source_current);
  result->prefetch(Subtype::build,"Builds",types,localized,source_current);
  result->prefetch(Subtype::command,"Commands",types,localized,source_current);
  result->prefetch(Subtype::control,"Controls",types,localized,source_current);
  result->prefetch(Subtype::mission,"Missions",types,localized,source_current);
  result->prefetch(Subtype::promotion,"UnitPromotions",types,localized,source_current);
  result->set_global_action_info(types,localized,source_current);source_current();result->require_current();return result;
 }catch(...){result->failed_.store(true);throw;}
}
