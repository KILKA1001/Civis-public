namespace civ5::runtime {
std::pair<std::int32_t,std::int32_t> LuaScriptHost::unitflag_initializing_button_ids_owned(
 const BnwWorldViewSourceOperationLoan& loan,const std::weak_ptr<const void>& core,
 std::uint64_t node,std::uint64_t root)const {
 const auto p=impl_;
 if(p->ui_thread!=std::this_thread::get_id())throw ScriptError("UnitFlag initializing IDs foreign creator");
 try{
  p->worldview_require_callback_origin();
  const auto* const job=p->worldview_unitflag_constructor_job;
  if(!job||job->owner!=p.get()||!job->invoked||job->returned||p->worldview_operation!=&loan||
     p->worldview_host_owner!=this||!p->calling||!p->worldview_created_dispatching)
   throw ScriptError("UnitFlag IDs need actual live original constructor job");
  const std::pair<std::int32_t,std::int32_t> ids{job->player,job->full_id};
  require_unitflag_initializing_button_owned(loan,core,node,root,ids.first,ids.second);
  p->worldview_require_callback_origin();
  if(p->worldview_unitflag_constructor_job!=job||job->returned||job->player!=ids.first||job->full_id!=ids.second)
   throw ScriptError("UnitFlag original constructor IDs changed after actual binding proof");
  return ids;
 }catch(...){p->worldview_retire_created_registrations();p->record_failure();
  if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
