// Original53C990. Same typed frame and original interface lifetime order.
// Neither missing-gate function below accepts a stub predicate/ready DTO.
// Root/Policy own the actual6AA190/component/loadstate/150 receiver; Parent
// owns the actual App+6F0 current UI context/type getter. Replace ONLY their
// strict provider bodies when those genuine publishers exist.
namespace civ5::runtime {
void BnwLocalClientScalarOwner::require_actual_frame_postcontext_prefix_owned(const BnwSpClientUpdateLoan& loan){
 auto& p=*loan.impl_;loan.require_current_without_callbacks();require_frame_game_origin_owned(p.scope);
 try{
  need(p.entered&&frame_update_entered_&&!p.postcontext_prefix_entered&&!p.postcontext_prefix_returned,
   "SP actual outer post-context prefix replay/reentry");p.postcontext_prefix_entered=true;
  require_frame_native_proxy00_owned(loan);
  //6AA190 unconditionally dereferences genuine client00;88 is the actual
  // current first-map count, NOT expected_count0C or a saved DTO.
  const auto count=imported_pregame_->actual_frame_proxy_map_count_owned(p.scope,loan);
  loan.require_current_without_callbacks();
  if(count-1<=1){
   const bool inner45=imported_pregame_->actual_frame_proxy_inner45_owned(p.scope,loan);
   loan.require_current_without_callbacks();
   if(!inner45&&legacy_component08_)
    throw std::runtime_error("SP6AA190 real legacy08 virtual34/inner70 effects required");
  }
  // Both actual component Update calls follow6AA190, in original04 then08
  // order. Genuine nullable constructor slots are read, no fake callback.
  if(legacy_component04_)throw std::runtime_error("SP original component04 Update8 required");
  loan.require_current_without_callbacks();
  if(legacy_component08_)throw std::runtime_error("SP original component08 UpdateC required");
  loan.require_current_without_callbacks();
  // Selected first-release Steam-disabled profile explicitly omits only
  // Steam76F660 achievements. It grants no Steam pointer/callback/receipt.
  need(native_client_profile_==NativeClientProfile::selected_civis_launcher,
   "SP optional Steam exclusion requires selected launcher profile");
  require_frame_native_proxy00_owned(loan);
  need(frame_loadstate158_constructed_,"SP actual loadstate158 constructor absent");
  if(frame_loadstate158_!=0)throw std::runtime_error("SP native7AF500 states1..8 mandatory Game/client effects required");
  //7AF500 state0: real App UI context/type9 BEFORE608A30 BEFORE vector128.
  // Parent solely owns this actual predicate; absent UI owner is a STOP.
  if(frame_has_actual_in_game_context_owned(loan)){
   loan.require_current_without_callbacks();
   if(frame_loadstate_turn_boundary_owned(loan)){
    loan.require_current_without_callbacks();
    need(frame_pending_vectors_constructed_,"SP loadstate real128 vector constructor absent");
    if(!frame_pending128_.empty())
     throw std::runtime_error("SP loadstate128 original637B80/495E70/isHost/517690 required");
   }
  }
  loan.require_current_without_callbacks();require_frame_native_proxy00_owned(loan);
  const bool pending150=imported_pregame_->actual_frame_proxy_pending_event150_owned(p.scope,loan);
  loan.require_current_without_callbacks();
  if(pending150)throw std::runtime_error("SP actual777020 event150 producer required before timer");
  require_frame_game_origin_owned(p.scope);loan.require_current_without_callbacks();p.postcontext_prefix_returned=true;
 }catch(...){p.failed.store(true);poisoned_.store(true);source_.invalidate();throw;}
}
bool BnwLocalClientScalarOwner::frame_has_actual_in_game_context_owned(const BnwSpClientUpdateLoan& loan){
 loan.require_current_without_callbacks();const auto& p=*loan.impl_;require_frame_game_origin_owned(p.scope);
 throw std::runtime_error("SP frame mandatory STOP: actual current App6F0 UI context/type9 owner not yet supplied");
}
void BnwLocalClientScalarOwner::execute_frame_game_timer_owned(const BnwSpClientUpdateLoan& loan){
 auto& p=*loan.impl_;loan.require_current_without_callbacks();require_frame_game_origin_owned(p.scope);
 try{
  if(p.game_timer_entered||p.game_timer_returned){
   p.failed.store(true);source_.invalidate();throw std::runtime_error("SP actual53C990 timer child repeated/reentered");
  }
  need(p.entered&&!p.consumed&&frame_update_entered_&&p.client_get==this,
   "SP53C990 lacks SAME canonical entered frame caller");
  p.game_timer_entered=true; // Once attempt, before all native/provider reads.
  if(!frame_has_actual_in_game_context_owned(loan)){
   loan.require_current_without_callbacks();p.game_timer_returned=true;return;
  }
  loan.require_current_without_callbacks();
  const auto active=read_frame_active_player_owned(p.lease);loan.require_current_without_callbacks();
  // Genuine CvDllGame::GetPlayer is unconditional new CvDllPlayer(GetPlayer).
  // NO_PLAYER is unsupported here, not invented as a NULL optional factory.
  // Wrapper acquisition verifies actual raw native64 manager and full ID.
  with_frame_player_owned(p.lease,active,[&](InterfaceLoan& active_player){
   {
    Player2Reference active2(active_player);active_player.current();
    (void)active2.has_turn_timer_expired(); //53CA2A result deliberately ignored.
    active2.return_owned();active_player.current();
   }
   loan.require_current_without_callbacks();
   const bool host=read_frame_is_host_owned(p.lease);loan.require_current_without_callbacks();
   if(host){
    // Native localSP host is true. Every actor MUST acquire/query/destroy;
    // timerOFF cannot bypass this loop or IsHuman/Pregame/client reads.
    for(std::int32_t actor=0;actor<64;++actor){
     loan.require_current_without_callbacks();active_player.current();
     with_frame_player_owned(p.lease,actor,[&](InterfaceLoan& player){
      Player2Reference queried(player);player.current();
      //53CAAE calls original Player1+18 while Player2 query ref is retained.
      bool eligible=player.is_human();loan.require_current_without_callbacks();player.current();
      if(!eligible){
       eligible=read_frame_observer_participant_owned(p.lease,actor);
       loan.require_current_without_callbacks();player.current();
      }
      if(eligible){
       const auto& actual_client=players_[static_cast<std::size_t>(actor)];
       need(actual_client.id==actor,"SP53C990 actual native PlayerClient slot identity differs");
       const bool field0f=actual_client.field_0f; // Actual43F780 on reset-owned slot.
       loan.require_current_without_callbacks();player.current();
       if(!field0f){
        const bool loading=frame_has_pending_load_vector_owned(loan,actor); //Policy real740630.
        loan.require_current_without_callbacks();player.current();
        if(!loading){
         const bool boundary=frame_engine_turn_boundary_owned(loan); //Policy actual3F4.
         loan.require_current_without_callbacks();player.current();
         if(boundary){
          const bool expired=queried.has_turn_timer_expired();
          loan.require_current_without_callbacks();player.current();
          if(expired){
           send_turn_complete_owned(p.lease,actor); // Original552D60; no fake ACK.
           loan.require_current_without_callbacks();player.current();
          }
         }
        }
       }
      }
      queried.return_owned();player.current(); // queriedDestroy BEFORE Player1Destroy.
     });
     loan.require_current_without_callbacks();active_player.current();
    }
   }
   // activePlayer1 survives host/all64 and is released by its original owner
   // only now at53CB98. Its first queried Player2 was already destroyed.
   loan.require_current_without_callbacks();active_player.current();
  });
  loan.require_current_without_callbacks();require_frame_game_origin_owned(p.scope);
  p.game_timer_returned=true;
 }catch(...){p.failed.store(true);poisoned_.store(true);source_.invalidate();throw;}
}
}
