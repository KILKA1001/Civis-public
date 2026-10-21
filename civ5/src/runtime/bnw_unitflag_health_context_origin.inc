// SAME dedicated UnitFlag context. These methods never call Source.current,
// Graph.current, Tree.current or the recursive active-operation accessor.
BnwSelectedUnitFlagContextOwner::HealthOrigin
BnwSelectedUnitFlagContextOwner::retain_health_origin_without_callbacks(const LuaScriptHost&vm)const{
 const auto saved=state_;
 require(saved&&saved->creator==std::this_thread::get_id(),"UnitFlag health context foreign creator before mutable read");
 saved->pure();require(saved->vm==&vm&&saved->graph,"UnitFlag health origin SAME actual VM/graph absent");
 HealthOrigin result{saved,saved->owner,saved->source,saved->tree,saved->graph,saved->vm};
 require_health_origin_without_callbacks(result,vm);return result;
}
void BnwSelectedUnitFlagContextOwner::require_health_origin_without_callbacks(const HealthOrigin&origin,const LuaScriptHost&vm){
 const auto&saved=origin.state;
 require(saved&&saved->creator==std::this_thread::get_id(),"UnitFlag saved health context foreign creator before mutable read");
 saved->pure();
 require(origin.vm==&vm&&saved->vm==&vm&&saved->graph&&
  same_owner(origin.owner,saved->owner)&&same_owner(origin.source,saved->source)&&
  same_owner(origin.tree,saved->tree)&&same_owner(origin.graph,saved->graph)&&
  origin.owner.lock().get()==saved->owner_address&&origin.source.lock().get()==saved->source.get()&&
  origin.tree.lock().get()==saved->tree.get()&&origin.graph.lock().get()==saved->graph.get(),
  "UnitFlag health origin actual context/metadata/descriptor/tree/graph controls changed");
 saved->pure();
}
