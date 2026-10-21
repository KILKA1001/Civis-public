// Same pending CvUnit/GetPlot suppliers, not published-roster visibility.
namespace civ5::runtime {
std::int32_t BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::read_dispatch_original_fog_without_callbacks()const{
 require_setup_graphical_enqueue_dispatch_without_callbacks();
 const auto borrower=ShouldDimCoreBorrowedOwner::from_weak(storage_->binding.core_control);
 const auto&core=borrower->current();const auto p=core.impl_;
 const auto&world=*p->state;const auto&node=*storage_->node;
 // Actual pending Unit.GetPlot. No NULL fallback in the original producer.
 const auto x=node.scalars.at("m_iX"),y=node.scalars.at("m_iY");const auto&grid=world.world.map_header.grid;
 need(x>=0&&y>=0&&static_cast<std::uint64_t>(x)<grid.width&&static_cast<std::uint64_t>(y)<grid.height,
  "Created pending Unit.GetPlot is NULL");
 need(world.world.map_plots&&world.world.map_plots->layout==bnw&&world.world.map_projection&&
  world.world.map_plots->plots.size()==grid.size()&&world.world.map_projection->cells.size()==grid.size(),
  "Created pending Unit/GetPlot actual allocation unavailable");
 const auto plot=grid.index(x,y);const auto&raw=world.world.map_plots->plots.at(plot);
 const auto&cell=world.world.map_projection->cells.at(plot);const auto xy=grid.coordinates(plot);
 need(plot==storage_->binding.plot&&raw.index==plot&&cell.index==plot&&raw.x==xy[0]&&raw.y==xy[1]&&
  cell.x==xy[0]&&cell.y==xy[1],"Created pending Unit/GetPlot canonical physical binding changed");
 // CvPlot::GetActiveFogOfWarMode source order: actual activeTeam then debug.
 const auto team=detail::bnw_game_active_team_read(world);
 need(team>=-1&&team<64,"Created actual Game active team outside native roster");
 const bool debug=p->host->session().bnw_game_debug_mode();
 p->check();require_setup_graphical_enqueue_dispatch_without_callbacks();
 std::int32_t native_mode=1; // exact SDK UNEXPLORED initializer, not absent data.
 if(debug||team!=-1){
  const bool revealed=debug||detail::bnw_plot_revealed_read(world.world,plot,team);
  if(revealed){const bool visible=debug||bnw_plot_is_visible(world,plot,team);native_mode=visible?0:2;}
 }
 p->check();require_setup_graphical_enqueue_dispatch_without_callbacks();
 // Actual Utility producer+58 mapping proven from PE, including default.
 return native_mode==0?2:native_mode==2?1:0;
}
bool BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan::read_dispatch_original_not_invisible_without_callbacks()const{
 require_setup_graphical_enqueue_dispatch_without_callbacks();
 const auto borrower=ShouldDimCoreBorrowedOwner::from_weak(storage_->binding.core_control);
 const auto&core=borrower->current();const auto p=core.impl_;const auto&world=*p->state;
 const auto&unit=*storage_->node;
 // Actual Game.GetActiveTeam THEN incoming SAME pending Unit.IsInvisible(team,true).
 const auto team=detail::bnw_game_active_team_read(world);
 need(team>=-1&&team<64,"Created actual Game active team outside native roster");
 const bool debug=p->host->session().bnw_game_debug_mode();
 p->check();require_setup_graphical_enqueue_dispatch_without_callbacks();
 bool invisible=false;
 if(!debug&&post_player_team(world,unit.scalars.at("m_eOwner"))!=team){
  const auto transport_owner=unit.scalars.at("m_transportUnit.owner"),transport_id=unit.scalars.at("m_transportUnit.id");
  bool cargo=false;
  if(transport_owner>=0&&transport_owner<64){
   // Resolve genuine registered pending objects before imported native GetAt.
   // Low-slot alias retains SDK GetAt semantics and authentic full-ID node.
   const auto registry=world.setup.extensions.find<Registry>();
   if(registry){const auto bank=registry->players.find(transport_owner);
    if(bank!=registry->players.end()){
     need(bank->second&&!bank->second->retired.load(),"Created transport native Bank retired");
     {const auto actual_world=bank->second->source.lock();
      need(actual_world&&actual_world.get()==p->state.get()&&
       !actual_world.owner_before(p->state)&&!p->state.owner_before(actual_world)&&bank->second->generation==p->generation,
       "Created transport native Bank SAME world/control/generation changed");}
     need(bank->second->slots>=0&&bank->second->last<bank->second->slots&&
      static_cast<std::size_t>(bank->second->slots)==bank->second->next.size(),
      "Created transport native pending manager allocation changed");
     const auto slot=transport_id&8191;
     const auto found=transport_id==-1||slot>bank->second->last?bank->second->pending_units.end():bank->second->pending_units.find(slot);
     if(found!=bank->second->pending_units.end()){
      need(bool(found->second),"Created transport pending native node absent");
      check_registered(*bank->second,*found->second,found->second,slot);
      const auto full=found->second->scalars.at("m_iID");
      cargo=(transport_id&~8191)==0||full==transport_id;
     }
    }
   }
   if(!cargo)cargo=native_imported_get_at(world,transport_owner,transport_id)!=nullptr;
  }
  if(cargo)invisible=true;
  else{
   const auto type=unit.scalars.at("m_eInvisibleType");
   if(type!=-1){
    need(world.world.map_plots&&world.world.map_plots->layout==bnw&&
     storage_->binding.plot<world.world.map_plots->plots.size(),
     "Created pending invisible Unit.GetPlot actual allocation unavailable");
    const auto plot=storage_->binding.plot;const auto&raw=world.world.map_plots->plots.at(plot);
    const auto&dim=world.world.map_plots->dimensions;const auto&counts=post_named(raw.arrays,"invisible_visibility");
    need(team>=0&&team<64&&dim.teams==80&&type>=0&&static_cast<std::uint64_t>(type)<dim.invisible_types&&
     counts.columns==1&&counts.values.size()==dim.teams*dim.invisible_types,
     "Created pending Unit actual invisible-visible source unavailable");
    const auto count=counts.values.at(static_cast<std::size_t>(team)*dim.invisible_types+type);
    need(count>=0&&count<=INT16_MAX,"Created pending Unit invisible-visible native short malformed");
    invisible=count<=0;
   }
  }
 }
 p->check();require_setup_graphical_enqueue_dispatch_without_callbacks();return !invisible;
}
}
