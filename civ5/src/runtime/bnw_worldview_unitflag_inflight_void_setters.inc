namespace civ5::runtime {
void BnwWorldViewScriptUiAdapter::set_void1(std::uint64_t id,std::int32_t value){
 const auto saved=state_;
 saved->guarded_render([&](const State::Frame& frame){
  const auto node=saved->render_operation_call(frame,[&]{return saved->tree->inspect_control(saved->render_control(frame,id));});
  require(value>=0&&value<64&&saved->context=="UnitFlagManager"&&node.dynamic&&node.alive&&node.instance&&
   node.authored.name=="Button"&&(node.authored.attribute("ID")=="NormalButton"||node.authored.attribute("ID")=="HealthBarButton"),
   "UnitFlag actual Void1 target/native actor invalid");
  const std::weak_ptr<const void> physical=saved->tree->core_;
  const auto ids=saved->render_call(frame,[&]{return saved->vm->unitflag_initializing_button_ids_owned(*frame.loan,physical,id,node.instance);});
  require(ids.first==value,"UnitFlag original SetVoid1 differs from authentic constructor actor");
  auto next=saved->unitflag_button_operands;auto& stored=next[id];
  require(!stored.instance||stored.instance==node.instance,"UnitFlag Void1 actual dynamic instance replaced");
  stored.instance=node.instance;stored.owner=value;
  saved->render_full(frame);saved->unitflag_button_operands.swap(next);
  next.clear();saved->render_full(frame);return 0;
 });
}
void BnwWorldViewScriptUiAdapter::set_void2(std::uint64_t id,std::int32_t value){
 const auto saved=state_;
 saved->guarded_render([&](const State::Frame& frame){
  const auto node=saved->render_operation_call(frame,[&]{return saved->tree->inspect_control(saved->render_control(frame,id));});
  require(value>=0&&saved->context=="UnitFlagManager"&&node.dynamic&&node.alive&&node.instance&&
   node.authored.name=="Button"&&(node.authored.attribute("ID")=="NormalButton"||node.authored.attribute("ID")=="HealthBarButton"),
   "UnitFlag actual Void2 target/native full-ID invalid");
  const std::weak_ptr<const void> physical=saved->tree->core_;
  const auto ids=saved->render_call(frame,[&]{return saved->vm->unitflag_initializing_button_ids_owned(*frame.loan,physical,id,node.instance);});
  require(ids.second==value,"UnitFlag original SetVoid2 differs from authentic complete constructor unit ID");
  auto next=saved->unitflag_button_operands;auto& stored=next[id];
  require(!stored.instance||stored.instance==node.instance,"UnitFlag Void2 actual dynamic instance replaced");
  stored.instance=node.instance;stored.full_id=value;
  saved->render_full(frame);saved->unitflag_button_operands.swap(next);
  next.clear();saved->render_full(frame);return 0;
 });
}
}
