// Included inside civ5::runtime after Mountain1 methods and the real imported
// DamageArchive implementation. No public factory or caller numeric damage.
void BnwNativeAttritionPrefix::prepare_damage_owned(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&s,
 const BnwSelectedStockLuaSourceOwner&source,const void*control,std::uint64_t generation,
 std::uint64_t serial,const std::weak_ptr<ui::BnwEngineUiTurnOwner>&engine)const{
 const auto r=impl_;r->thread();
 require_current_without_callbacks(core,host,s,source,control,generation,r->serial,true);
 need(!r->damage&&r->reached.id&&!r->outer_returned,"Attrition damage requires one genuine native parent");
 std::uint64_t previous=r->serial;bool mountain=false;
 if(r->mountain){
  require_mountain_without_callbacks(core,host,s,source,control,generation,r->mountain->serial,true);
  need(r->mountain->reached.damage,"Attrition damage cannot enter false mountain branch");
  previous=r->mountain->serial;mountain=true;
 }else need(progress_without_callbacks().boundary==BnwAttritionPrefixBoundary::localized_damage_owner&&
           r->reached.draw&&r->rng_stored&&r->roll&&
           *r->roll<static_cast<std::uint64_t>(r->reached.chance),
           "Attrition damage requires genuine completed territory roll hit");
 need(previous<UINT64_MAX&&serial==previous+1,
      "Attrition damage requires exact NEXT actual Source operation after reached parent");
 // Reserve private once history BEFORE registration/providers/localization.
 r->damage=std::make_unique<AttritionDamageChild>();auto&q=*r->damage;
 q.previous_serial=previous;q.serial=serial;q.mountain=mountain;q.id=*r->reached.id;
 q.before=s.ui_source_revision;q.after=q.before;q.delta=mountain?50:r->reached.damage;q.engine=engine;
 const auto p=core.impl_;
 try{
  need(q.delta>0,"Attrition damage lacks actual reached positive native delta");
  const auto registered=core.bind_native_existing_damage_archive(q.id);
  need(registered&&registered->full_id==q.id&&registered->owner==r->actor,
       "Attrition damage actual archive producer absent");
  const auto archive=p->bank->imported_damage_fields.find(q.id);
  const auto plain=p->bank->imported_mission_runtime.find(q.id);
  need(archive!=p->bank->imported_damage_fields.end()&&archive->second&&
       plain!=p->bank->imported_mission_runtime.end()&&plain->second&&
       !registered->registered_lifetime.owner_before(archive->second)&&
       !archive->second.owner_before(registered->registered_lifetime)&&
       !registered->plain_runtime_lifetime.owner_before(plain->second)&&
       !plain->second.owner_before(registered->plain_runtime_lifetime),
       "Attrition damage requires SAME genuinely registered archive/plain controls");
  q.archive=archive->second;q.plain=plain->second;
  q.old_damage=q.archive->expected;q.old_loaded=q.archive->loaded;q.old_last=q.plain->last_full_health_turn;
  q.old_archive_changes=q.archive->changes;q.old_archive_dirty=q.archive->dirty;
  q.old_last_stores=q.plain->last_full_health_stores;
  const auto borrower=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(core.factory_self_);
  {BnwNativeUnitAllocatorCore::Impl::Guard invocation(p);
   p->check();(void)borrower->current();
   require_current_without_callbacks(core,host,s,source,control,generation,r->serial,true);
   if(mountain)require_mountain_without_callbacks(core,host,s,source,control,generation,previous,true);
   // This is the existing sealed Engine/DB/TextData producer. Source's closed
   // issuer join must obtain its weak receiver from actual same-Host LiveUiHost.
   // No string parameter, default language or public translated DTO exists.
   q.localization_entered=true;
   auto text=ui::BnwEngineUiTurnOwner::selected_localization_from_owned_weak(
       engine,"TXT_KEY_MISC_YOU_UNIT_WAS_DAMAGED_ATTRITION");
   q.localization_returned=true;q.localized_text=std::move(text);
   p->check();(void)borrower->current();
   require_current_without_callbacks(core,host,s,source,control,generation,r->serial,true);
   if(mountain)require_mountain_without_callbacks(core,host,s,source,control,generation,previous,true);
   q.max_hp=attrition_actual_max_hit_points(host);
   q.requested=narrow(std::int64_t(q.old_damage)+q.delta);
   // Native GetCurrHitPoints == GetMaxHitPoints is oldDamage==0. Reject
   // overflow before any scalar write, including the earlier plain write.
   const auto current_hp=narrow(std::int64_t(q.max_hp)-q.old_damage);
   q.last_condition=current_hp==q.max_hp&&q.requested<q.max_hp;
   q.new_last=q.last_condition?narrow(r->turn):q.old_last;
   q.new_damage=std::clamp(q.requested,0,q.max_hp);q.changed=q.new_damage!=q.old_damage;
   need(!q.last_condition||q.old_last_stores<UINT64_MAX,"Attrition plain store counter exhausted");
   need(!q.changed||q.old_archive_changes<UINT64_MAX,"Attrition archive change counter exhausted");
   need(!(q.last_condition||q.changed)||q.before<UINT64_MAX,"Attrition damage revision exhausted");
   q.after=q.before+static_cast<std::uint64_t>(q.last_condition||q.changed);
   const auto*u=native_imported_get_at(s,r->actor,q.id);
   need(u&&q.archive->unit==u&&q.plain->unit==u,"Attrition damage actual fullID/field unit differs");
   q.after_unit=busy_unit_stamp(*u);bool field_found=false;
   for(auto&f:q.after_unit.core_fields)if(f.field==q.archive->field){
    need(!field_found&&f.member=="m_iDamage"&&f.integers.size()==1&&f.integers[0]==q.old_damage,
         "Attrition expected after stamp lacks actual unique damage slot");
    field_found=true;f.integers[0]=q.new_damage;}
   need(field_found,"Attrition damage field absent in actual native row stamp");
   p->check();(void)borrower->current();
  }
  q.prepared=true;
  require_damage_without_callbacks(core,host,s,source,control,generation,serial,false);
 }catch(...){q.failed=true;throw;}
}

void BnwNativeAttritionPrefix::require_damage_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&s,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t serial,bool executed)const{
 const auto r=impl_;r->thread();const auto p=core.impl_;const auto self=r->self.lock();
 const auto h=r->host.lock();const auto w=r->world.lock();const auto issuer=r->source.lock();const auto scope=r->control.lock();
 need(r->damage&&r->reached.id&&!r->failed&&!r->damage->failed&&self&&self.get()==this&&p&&h&&h.get()==&host&&
      w&&w.get()==&s&&issuer&&issuer.get()==&source&&scope&&scope.get()==control&&
      generation==r->generation&&generation==p->generation&&p->state.get()==&s&&p->host.get()==&host&&
      !r->core.owner_before(core.factory_self_)&&!core.factory_self_.owner_before(r->core)&&
      p->bank.get()==r->bank.lock().get()&&p->player_attrition_prefixes.contains(r->turn)&&
      !p->player_attrition_prefixes.at(r->turn).owner_before(r->self)&&
      !r->self.owner_before(p->player_attrition_prefixes.at(r->turn)),
      "Attrition damage SAME Source/Core/Host/world/control lifetime changed");
 const auto&q=*r->damage;
 need(!q.mountain||(r->mountain&&r->mountain->executed&&!r->mountain->failed&&r->mountain->reached.damage),
      "Attrition damage genuine mountain predecessor lost");
 need(q.prepared&&q.localization_entered&&q.localization_returned&&q.serial==serial&&serial&&
      q.previous_serial<UINT64_MAX&&serial==q.previous_serial+1&&q.executed==executed&&
      q.attempted==executed&&q.before==r->after&&q.id==*r->reached.id&&
      q.max_hp==attrition_actual_max_hit_points(host)&&
      q.previous_serial==(q.mountain?r->mountain->serial:r->serial),
      "Attrition damage ordered native child/locale/phase/source serial differs");
 core.require_live_gold_roster_source_without_callbacks(host,s,r->actor);
 const auto b=r->bank.lock();need(b&&!b->retired.load()&&b->source.lock().get()==&s&&b->generation==generation,
      "Attrition damage canonical Bank retired");
 (void)actual_attrition_first_read(s,*b,r->actor,&r->reached);
 if(q.mountain)(void)actual_attrition_mountain_read(host,s,*b,r->actor,q.id,
       r->mountain->reached.info,&r->mountain->reached);
 need(s.world.all_players&&s.world.all_players->players.size()==64&&s.world.all_players->players.data()==r->players&&
      s.ui_source_revision==(executed?q.after:q.before)&&s.setup.game_turn==r->turn&&
      s.random.map.algorithm()==RandomAlgorithm::bnw&&s.random.gameplay.algorithm()==RandomAlgorithm::bnw&&
      s.random.map.state()==r->map_rng&&s.random.gameplay.state()==r->after_rng,
      "Attrition damage native64/world/clock/RNG changed");
 // ONE exact after row is accepted only for this privately executed child.
 // Old parent's physical record is retained literally; no in-place baseline
 // rewrite or caller numeric revision masks its old pre-damage witnesses.
 for(std::size_t n=0;n<64;++n){const auto&player=s.world.all_players->players[n];const auto&old=r->managers[n];
  need(&player==old.player&&player.saved_player_id==old.player_id&&player.units.layout==old.units_layout&&
       player.units.header.layout==old.header_layout&&player.units.header.fields.data()==old.header&&
       player.units.units.data()==old.units&&player.units.header.source_slots==old.source_slots&&
       player.units.header.fields.size()==old.header_values.size()&&player.units.units.size()==old.rows.size(),
       "Attrition damage actual native64 storage changed");
  for(std::size_t k=0;k<old.header_values.size();++k)
   need(player.units.header.fields[k].value==old.header_values[k],"Attrition damage FLTA header changed");
  for(std::size_t k=0;k<old.rows.size();++k){const auto&unit=player.units.units[k];
   require_busy_unit_stamp(unit,executed&&&unit==q.archive->unit?q.after_unit:old.rows[k]);}
 }
 const auto archive=b->imported_damage_fields.find(q.id);const auto plain=b->imported_mission_runtime.find(q.id);
 need(archive!=b->imported_damage_fields.end()&&archive->second.get()==q.archive.get()&&
      !archive->second.owner_before(q.archive)&&!q.archive.owner_before(archive->second)&&
      plain!=b->imported_mission_runtime.end()&&plain->second.get()==q.plain.get()&&
      !plain->second.owner_before(q.plain)&&!q.plain.owner_before(plain->second)&&
      q.archive->creator==r->creator&&q.plain->creator==r->creator&&
      q.archive->owner.lock().get()==b.get()&&q.plain->owner.lock().get()==b.get()&&
      q.archive->generation==generation&&q.plain->generation==generation&&
      q.archive->actor==r->actor&&q.plain->actor==r->actor&&q.archive->id==q.id&&q.plain->id==q.id&&q.archive->loaded==q.old_loaded&&
      q.archive->unit==q.plain->unit&&native_imported_get_at(s,r->actor,q.id)==q.archive->unit&&
      q.archive->field&&q.archive->field->encoding==saves::UnitCoreEncoding::i32&&
      q.archive->field->values.size()==1&&q.archive->field->bytes.empty()&&
      &q.archive->field->values.front()==q.archive->value&&
      *q.archive->value==(executed?q.new_damage:q.old_damage)&&q.archive->expected==*q.archive->value&&
      q.archive->changes==q.old_archive_changes+static_cast<std::uint64_t>(executed&&q.changed)&&
      q.archive->dirty==(q.old_archive_dirty||(executed&&q.changed))&&
      q.plain->last_full_health_turn==(executed?q.new_last:q.old_last)&&
      q.plain->last_full_health_stores==q.old_last_stores+static_cast<std::uint64_t>(executed&&q.last_condition)&&
      q.last_written==(executed&&q.last_condition)&&q.assignment_invoked==executed,
      "Attrition damage native scalar/plain physical alias or store history changed");
}

void BnwNativeAttritionPrefix::execute_damage_without_callbacks(LiveSessionState&s)const{
 const auto r=impl_;r->thread();need(r->damage&&!r->damage->failed&&r->damage->prepared&&
      !r->damage->attempted&&!r->damage->executed,"Attrition native damage child replay");
 const auto core=r->core.lock();const auto host=r->host.lock();const auto source=r->source.lock();const auto scope=r->control.lock();
 need(core&&host&&source&&scope,"Attrition damage actual issued entry expired");
 require_damage_without_callbacks(*core,*host,s,*source,scope.get(),r->generation,r->damage->serial,false);
 auto&q=*r->damage;
 // All allocation, provider calls, pointer/shape/arithmetic/currentness and
 // overflow validation is above. The native ordered tail cannot throw.
 q.attempted=true;
 if(q.last_condition){q.plain->last_full_health_turn=q.new_last;
  q.plain->last_full_health_stores=q.old_last_stores+1;q.last_written=true;}
 q.assignment_invoked=true;
 if(q.changed){*q.archive->value=q.new_damage;q.archive->expected=q.new_damage;
  q.archive->changes=q.old_archive_changes+1;q.archive->dirty=true;}
 s.ui_source_revision=q.after;q.executed=true;
 // No IsGarrisoned/InfoBar/selection/Gameplay/popup/death call is performed.
 // Root must adopt this exact private partial history before callbackful POST.
}
BnwAttritionDamageProgress BnwNativeAttritionPrefix::damage_progress_without_callbacks()const{
 const auto r=impl_;r->thread();BnwAttritionDamageProgress o;if(!r->damage)return o;
 const auto&q=*r->damage;o.issued=true;o.localization_entered=q.localization_entered;
 o.localization_returned=q.localization_returned;o.prepared=q.prepared;o.attempted=q.attempted;
 o.executed=q.executed;o.failed=q.failed;o.last_full_health_written=q.last_written;
 o.damage_assignment_invoked=q.assignment_invoked;o.damage_changed=q.executed&&q.changed;
 o.unit=q.id;o.previous_source_serial=q.previous_serial;o.source_serial=q.serial;
 o.before_revision=q.before;o.after_revision=q.executed?q.after:q.before;
 if(q.prepared){o.before_damage=q.old_damage;o.requested_damage=q.requested;o.max_hit_points=q.max_hp;
  o.before_last_full_health=q.old_last;o.after_damage=q.executed?q.new_damage:q.old_damage;
  o.after_last_full_health=q.executed?q.new_last:q.old_last;}
 if(q.executed)o.boundary=q.changed?BnwAttritionDamageBoundary::garrison_owner:BnwAttritionDamageBoundary::death_query_owner;
 return o;
}
void BnwNativeAttritionPrefix::fail_damage_without_callbacks()const noexcept{
 const auto r=impl_;if(r->creator!=std::this_thread::get_id())std::terminate();
 if(r->damage)r->damage->failed=true;
}
