// Inside actual Bootstrap::Impl. The callbacks reuse the existing roster
// readers; this seam supplies and retains their SAME selected Host owner.
template<class R>
std::function<R(std::int64_t)> retain_player_roster_read_owned(
 std::function<R(std::int64_t)> actual,
 const std::weak_ptr<const ui::SelectedBnwPreviewHost>& selected,
 const PregameRoster* roster){
 need(operation&&!diagnostic_world&&actual&&roster,
  "Stock Player roster reader requires actual retained source");
 const auto lease=*operation;
 return [this,actual=std::move(actual),selected,roster,lease](std::int64_t player)->R{
  try{
   const auto control=lease.checked();const auto serial=control->serial;
   vm_current_provider_bookend();
   // Weak in the stored closure; a real strong Host pin covers every raw
   // roster access. Stock source ownership is not cycled back from Lua.
   const auto keeper=selected.lock();
   need(bool(keeper),"Stock Player selected roster Host retired");
   const auto current=[&]{
    lease.require_operation(serial);
    const auto bound=control->selected_host.lock();
    need(bound&&bound.get()==keeper.get()&&!bound.owner_before(keeper)&&!keeper.owner_before(bound),
     "Stock Player selected roster Host replaced");
    need(control->world.get()==world.get()&&keeper->session().read_state_lease().get()==world.get()&&
     &keeper->pregame_roster()==roster,"Stock Player actual world/roster allocation changed");
    if(civis_sources)civis_sources->require_same_host(*keeper);
    vm_current_provider_bookend();
   };
   current();const R result=actual(player);current();return result;
  }catch(...){fail();throw;}
 };
}
ScriptGameplayProvider gameplay_read_services_with_roster_owned(){
 need(operation&&!diagnostic_world,"Stock Player roster wiring requires actual source");
 const auto lease=*operation;const auto control=lease.checked();const auto serial=control->serial;
 vm_current_provider_bookend();const auto keeper=control->selected_host.lock();
 need(bool(keeper),"Stock Player actual selected roster Host absent");
 need(control->world.get()==world.get()&&keeper->session().read_state_lease().get()==world.get(),
  "Stock Player roster wiring has different actual world");
 if(civis_sources)civis_sources->require_same_host(*keeper);
 const auto* roster=&keeper->pregame_roster();
 auto result=lua_gameplay_read_services(*database,world,types,roster);
 const std::weak_ptr<const ui::SelectedBnwPreviewHost> selected=keeper;
 result.player_leader=retain_player_roster_read_owned(std::move(result.player_leader),selected,roster);
 result.player_minor=retain_player_roster_read_owned(std::move(result.player_minor),selected,roster);
 result.player_minor_type=retain_player_roster_read_owned(std::move(result.player_minor_type),selected,roster);
 result.player_civilization=retain_player_roster_read_owned(std::move(result.player_civilization),selected,roster);
 need(&keeper->pregame_roster()==roster,"Stock Player roster changed during wiring");
 lease.require_operation(serial);vm_current_provider_bookend();return result;
}
