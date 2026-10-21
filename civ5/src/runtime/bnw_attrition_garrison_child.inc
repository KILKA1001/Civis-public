// Included inside runtime AFTER genuine damage child implementation.
void BnwNativeAttritionPrefix::prepare_garrison_owned(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&s,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t serial)const{
 const auto r=impl_;r->thread();need(bool(r->damage),"Attrition garrison actual damage parent absent");
 require_damage_without_callbacks(core,host,s,source,control,generation,r->damage->serial,true);
 need(r->damage->changed&&!r->garrison&&r->damage->serial<UINT64_MAX&&serial==r->damage->serial+1,
      "Attrition garrison requires SAME physically changed HP and exact NEXT operation");
 r->garrison=std::make_unique<AttritionGarrisonChild>();auto&q=*r->garrison;
 q.serial=serial;q.previous=r->damage->serial;q.revision=s.ui_source_revision;
 const auto p=core.impl_;const auto borrower=BnwNativeUnitAllocatorCore::ShouldDimCoreBorrowedOwner::from_weak(core.factory_self_);
 try{
  const auto read=[&]{need(q.entered<3&&q.returned==q.entered,"Attrition garrison native read once order changed");
   const auto index=q.entered++;BnwNativeUnitAllocatorCore::Impl::Guard invocation(p);
   p->check();(void)borrower->current();
   require_damage_without_callbacks(core,host,s,source,control,generation,q.previous,true);
   auto actual=BnwNativeImportedGarrisonRead::read_same_live(host,s,r->actor,r->damage->id);
   q.reads[index]=std::move(actual);++q.returned;
   need(q.reads[index]->facts_without_callbacks().observed_damage==r->damage->new_damage,
        "Attrition garrison solver did not observe genuine AFTER live damage");
   p->check();(void)borrower->current();
   require_damage_without_callbacks(core,host,s,source,control,generation,q.previous,true);
   q.reads[index]->require_without_callbacks(host,s);return q.reads[index];
  };
  const auto first=read();
  if(!first->facts_without_callbacks().garrisoned)q.boundary=BnwAttritionGarrisonBoundary::info_bar_dirty_owner;
  else{const auto second=read(); // Actual GetGarrisonedCity() != NULL query, including repeated IsGarrisoned.
   if(!second->city_without_callbacks())q.boundary=BnwAttritionGarrisonBoundary::info_bar_dirty_owner;
   else{const auto third=read(); // Original second GetGarrisonedCity()->updateStrengthValue() receiver.
    need(third->city_without_callbacks()&&third->city_without_callbacks()==second->city_without_callbacks(),
         "Attrition garrison required city-strength receiver changed/null");
    q.boundary=BnwAttritionGarrisonBoundary::city_strength_owner;
   }
  }
  q.prepared=true;require_garrison_without_callbacks(core,host,s,source,control,generation,serial,false);
 }catch(...){q.failed=true;throw;}
}
void BnwNativeAttritionPrefix::require_garrison_without_callbacks(const BnwNativeUnitAllocatorCore&core,
 const ui::SelectedBnwPreviewHost&host,const LiveSessionState&s,const BnwSelectedStockLuaSourceOwner&source,
 const void*control,std::uint64_t generation,std::uint64_t serial,bool executed)const{
 const auto r=impl_;r->thread();need(r->garrison&&r->damage,"Attrition garrison actual partial history absent");
 const auto&q=*r->garrison;
 require_damage_without_callbacks(core,host,s,source,control,generation,q.previous,true);
 need(!q.failed&&q.prepared&&q.executed==executed&&q.serial==serial&&q.previous==r->damage->serial&&
      q.previous<UINT64_MAX&&q.serial==q.previous+1&&q.revision==s.ui_source_revision&&
      q.entered==q.returned&&q.returned>=1&&q.returned<=3&&r->damage->changed,
      "Attrition garrison SAME executed HP/serial/revision/native read history differs");
 for(std::uint32_t n=0;n<q.returned;++n){need(bool(q.reads[n]),"Attrition garrison actual returned read lost");
  q.reads[n]->require_without_callbacks(host,s);
  need(q.reads[n]->facts_without_callbacks().observed_damage==r->damage->new_damage,
       "Attrition garrison changed AFTER damage-child operand join");}
 const bool first=q.reads[0]->facts_without_callbacks().garrisoned;
 need(first?(q.returned>=2&&(!q.reads[1]->city_without_callbacks()?
      (q.returned==2&&q.boundary==BnwAttritionGarrisonBoundary::info_bar_dirty_owner):
      (q.returned==3&&q.reads[2]->city_without_callbacks()==q.reads[1]->city_without_callbacks()&&
       q.boundary==BnwAttritionGarrisonBoundary::city_strength_owner))):
      (q.returned==1&&q.boundary==BnwAttritionGarrisonBoundary::info_bar_dirty_owner),
      "Attrition garrison physical original NULL/equality/strength branch differs");
}
void BnwNativeAttritionPrefix::execute_garrison_without_callbacks(const LiveSessionState&s)const{
 const auto r=impl_;r->thread();need(r->garrison&&!r->garrison->failed&&!r->garrison->executed,
      "Attrition garrison actual once native condition reentry");
 const auto core=r->core.lock();const auto host=r->host.lock();const auto source=r->source.lock();const auto scope=r->control.lock();
 need(core&&host&&source&&scope,"Attrition garrison actual issued owners expired");
 require_garrison_without_callbacks(*core,*host,s,*source,scope.get(),r->generation,r->garrison->serial,false);
 r->garrison->executed=true; // Native reads returned. No city/dirty consumer invoked.
}
BnwAttritionGarrisonProgress BnwNativeAttritionPrefix::garrison_progress_without_callbacks()const{
 const auto r=impl_;r->thread();BnwAttritionGarrisonProgress o;if(!r->garrison)return o;
 const auto&q=*r->garrison;o.issued=true;o.prepared=q.prepared;o.executed=q.executed;o.failed=q.failed;
 o.native_calls_entered=q.entered;o.native_calls_returned=q.returned;o.source_serial=q.serial;
 o.previous_source_serial=q.previous;o.revision=q.revision;o.unit=r->damage->id;
 if(q.returned)o.first=q.reads[0]->facts_without_callbacks();if(q.executed)o.boundary=q.boundary;return o;
}
void BnwNativeAttritionPrefix::fail_garrison_without_callbacks()const noexcept{
 const auto r=impl_;if(r->creator!=std::this_thread::get_id())std::terminate();if(r->garrison)r->garrison->failed=true;
}
