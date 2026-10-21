// PRIVATE actual namespace/Game/player provider child; no whole-frame ACK.
namespace civ5::runtime {
bool BnwSpClientPregameStartup::frame_observer_participant(std::int32_t actor){
 current();need(registration_returned_&&local_parent_&&pregame_context_&&actor>=0&&actor<64,
  "SP frame Pregame/participant actual owner absent");
 try{
  return pregame_context_->with_frame_pregame([&](const BnwSpClientPregameStartup& actual){
   need(&actual==this,"SP frame Pregame canonical namespace substituted");current();
   const auto status=actual_namespace_.archive.integer_arrays.find("slot_status");
   need(status!=actual_namespace_.archive.integer_arrays.end()&&status->second.size()==64,
    "SP frame Pregame slotStatus native64 unavailable");
   const auto value=status->second[static_cast<std::size_t>(actor)];
   need(value>=0&&value<=4,"SP frame Pregame slotStatus invalid");current();
   if(value!=4)return false; // Only actual observer requires574100.
   //574100 obtains actual inner+124 info twice, then7F3240 first-map lookup.
   // This is the already allocated same registry/participant owner.
   local_parent_->require_before_registration();current();
   local_parent_->require_before_registration();current();
   const auto found=participants_.find(actor);current();
   if(found==participants_.end())return false;
   need(found->second&&found->second->id==actor,"SP frame actual participant identity changed");
   return true;
  });
 }catch(...){poisoned_.store(true);if(local_parent_)local_parent_->quarantine();throw;}
}
bool BnwSpClientPregameStartup::frame_is_host(){
 current();need(registration_returned_&&local_parent_,"SP frame host lacks genuine returned registration");
 try{
  local_parent_->require_before_registration();current();
  if(local_parent_->inner46)return true; // Original596680 +C8 short-circuit.
  local_parent_->require_before_registration();current();
  local_parent_->require_before_registration();current();
  const auto found=local_parent_->process->registry.find(local_parent_->info_parent_id);
  need(found!=local_parent_->process->registry.end()&&found->second==local_parent_.get(),
   "SP frame host info service registry unavailable");
  current();if(found->second->inner46)return true; //7F3510 actual parent+C8.
  const bool result=host_id_!=-1&&host_id_==local_id_;current();return result;
 }catch(...){poisoned_.store(true);if(local_parent_)local_parent_->quarantine();throw;}
}
}
