namespace civ5::runtime {
void BnwWorldViewScriptUiAdapter::require_initializing_button_core_owned(
 const std::shared_ptr<ScriptUiControlGraph>& graph,const std::weak_ptr<const void>& expected,
 const BnwWorldViewSourceOperationLoan& operation,const LuaScriptHost& host,
 std::uint64_t id,std::uint64_t root){
 require(graph&&graph->backend_&&!expected.expired(),"UnitFlag Initialize actual native backend/Core absent");
 const auto adapter=std::dynamic_pointer_cast<BnwWorldViewScriptUiAdapter>(graph->backend_);
 require(bool(adapter),"UnitFlag Initialize backend is not actual canonical adapter");
 const auto saved=adapter->state_;
 try{
  // This private query is nested inside THAT adapter's actual registration
  // operation. It does not issue a second reentry/guarded_render scope.
  saved->render_first(true);const State::Frame frame{&operation,operation.operation_serial()};
  saved->render_pure(frame);
  require(saved->vm==&host&&saved->context=="UnitFlagManager",
   "UnitFlag Initialize actual adapter VM/context substituted");
  const std::weak_ptr<const void> physical=saved->tree->core_;
  require(!physical.expired()&&!physical.owner_before(expected)&&!expected.owner_before(physical),
   "UnitFlag Initialize graph physical Core differs from actual registering button owner");
  const auto target=saved->render_operation_call(frame,[&]{return saved->tree->inspect_control(saved->render_control(frame,id));});
  require(target.dynamic&&target.alive&&target.instance==root&&target.authored.name=="Button"&&
   (target.authored.attribute("ID")=="NormalButton"||target.authored.attribute("ID")=="HealthBarButton"),
   "UnitFlag Initialize target is not SAME actual canonical built button");
  const auto anchor=saved->render_operation_call(frame,[&]{return saved->tree->inspect_control(saved->render_control(frame,root));});
  require(anchor.dynamic&&anchor.alive&&anchor.instance==root&&anchor.authored.attribute("ID")=="Anchor",
   "UnitFlag Initialize actual canonical instance Anchor retired/substituted");
  saved->render_operation_call(frame,[&]{saved->tree->require_unitflag_button_slot10_return_owned(operation,id,root);return 0;});
  saved->render_full(frame);require(!physical.expired()&&!expected.expired(),"UnitFlag Initialize Core retired during validation");
 }catch(...){saved->render_fail();throw;}
}
}
