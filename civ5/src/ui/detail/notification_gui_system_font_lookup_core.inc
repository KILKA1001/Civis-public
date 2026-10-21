// Native50E500: the canonical Gui+10 manager map is keyed ONLY by the
// signed unadjusted line-height returned by40FF60 (Font+40C10).
std::shared_ptr<const detail::BnwLabelSystemFontBorrower>
detail::NotificationContextGuiPort::begin_system_font_lookup_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const detail::BnwCompletedFontLoan& font){
 using Registry=detail::BnwLabelSystemFontRegistryFields;
 const auto saved=impl_;
 // Capture genuine immutable metadata before the first callback. The caller
 // owns its lexical loan; no raw Font/Gui facade is reread after this cut.
 const auto font_metadata=font.impl_;
 detail::BnwCompletedFontLoan view(font_metadata);
 const auto pure=[&]{
  saved->first(operation);view.require_current_without_callbacks();
  {const auto facade=saved->canonical_gui.lock();
   require(facade&&facade->impl_.get()==saved.get()&&!facade->impl_.owner_before(saved)&&!saved.owner_before(facade->impl_),
    "System font lookup canonical Gui facade retired/replaced");
   require(!font_metadata->gui.owner_before(facade)&&!facade.owner_before(font_metadata->gui),
    "System font input Font belongs to another canonical Gui");}
 };
 const auto full=[&]{pure();operation.require_current();pure();view.require_source_owned(operation);pure();};
 pure();full();
 require(saved->language_980a&&saved->language_980a->completed&&!saved->refreshing_language_980a&&
  saved->language_980a->value==1,"System font lookup requires actual completed Gui980A positive branch");
 const auto metrics=view.metrics_without_callbacks();pure();
 const std::int32_t height=metrics[2];
 if(!saved->system_font_registry){
  require(!saved->system_font_creating,"System font manager creation reentered");saved->system_font_creating=true;
  {struct End{std::shared_ptr<Impl> p;~End(){p->system_font_creating=false;}}end{saved};
   auto manager=std::make_shared<Registry>();pure();full();manager->life->first();
   require(!saved->system_font_registry,"System font manager substituted during allocation");
   saved->system_font_registry=std::move(manager);
  }
  pure();full();
 }
 const auto registry=saved->system_font_registry;
 const auto first=[&]{pure();registry->life->first();
  require(saved->system_font_registry.get()==registry.get()&&!saved->system_font_registry.owner_before(registry)&&
   !registry.owner_before(saved->system_font_registry),"System font actual Gui+10 manager changed");};
 first();require(!registry->busy,"System font signed-height lookup reentered");registry->busy=true;
 std::shared_ptr<const detail::BnwLabelSystemFontBorrower> result;
 try{
  {struct End{std::shared_ptr<Registry> r;~End(){r->busy=false;}}end{registry};
   const auto check=[&]{first();full();first();};check();
   auto at=registry->map.find(height);
   if(at==registry->map.end()){
    decltype(registry->map) staged;staged.try_emplace(height);check();
    auto node=staged.extract(height);check();
    const auto inserted=registry->map.insert(std::move(node));first();
    require(inserted.inserted,"System font actual NULL cell insertion replaced");at=inserted.position;
   }
   const auto* const cell=&at->second;
   auto entry=at->second.completed;
   if(!entry){
    require(registry->sequence<UINT64_MAX,"System font allocation sequence exhausted");
    registry->pending.reserve(registry->pending.size()+1);check();
    entry=std::make_shared<Registry::Entry>(height,registry->sequence+1);check();entry->life->first();
    require(!cell->completed,"System font NULL cell replaced before constructor");
    registry->pending.push_back(entry);++registry->sequence;
    const auto pending=[&]{first();entry->life->first();
     require(!cell->completed&&!entry->returned&&registry->busy&&
      std::any_of(registry->pending.begin(),registry->pending.end(),[&](const auto& e){
       return e.get()==entry.get()&&!e.owner_before(entry)&&!entry.owner_before(e);}),
      "System font pending actual constructor allocation changed");};
    // Language/face/scales are read from the activated locale DB ONLY on
    // genuine MISS. No XML parsing, policy/title issuer or HIT SQL fallback.
    {auto data=operation.system_font_language_data_owned();pending();check();
     require(data.locale.language==saved->language_980a->language,
      "System font MISS selected locale differs from actual Gui980A");
     BnwSystemFontCpuFrame frame;
     frame.locale=std::move(data.locale);pending();check();
     frame.language_row.language=frame.locale.language;pending();check();
     frame.language_row.font_name=std::move(data.font_name);
     frame.language_row.width_scale=data.width_scale;frame.language_row.height_scale=data.height_scale;
     frame.language_row.use_extended_font=saved->language_980a->value==1;
     frame.client.width=saved->field_644;frame.client.height=saved->field_648;
     frame.client.revision=saved->viewport_revision;
     frame.client.surface_owner="Gui.Window."+std::to_string(saved->occurrence);pending();check();
     frame.source_label_ascent=metrics[0];frame.source_line_height=height;
     // Only immutable quote DATA from the genuine completed Font. Font FIRST
     // is refreshed before each post-allocation read of its actual allocation.
     view.require_current_without_callbacks();pending();
     require(font_metadata->allocation->resource_quote.has_value(),"System font genuine Font quote missing");
     {const auto quote_owner=font_metadata->registry.lock();pending();check();
      require(quote_owner&&saved->font_registry.get()==quote_owner.get()&&
       !saved->font_registry.owner_before(quote_owner)&&!quote_owner.owner_before(saved->font_registry),
       "System font immutable quote physical registry changed");
      frame.font_key=font_metadata->allocation->resource_quote->request;pending();check();
      frame.font_sha256=font_metadata->allocation->resource_quote->loaded.payload_identity.sha256;pending();check();
     }
     pending();check();
     auto state=std::make_shared<const detail::BnwLabelSystemFontSource::Impl>(saved->canonical_gui,saved,
      registry,entry,registry->life,entry->life,cell,height,entry->occurrence,font_metadata,std::move(frame),saved->hwnd);
     pending();check();
     auto source=std::shared_ptr<const detail::BnwLabelSystemFontSource>(new detail::BnwLabelSystemFontSource(std::move(state)));
     pending();check();entry->source=std::move(source);
    }
    pending();check();entry->source->require_current_owned();pending();
    entry->cpu=BnwSystemFontCpuOwner::create_from_actual_gui_source(entry->source);pending();check();
    entry->cache=BnwSystemFontCacheOwner::create(entry->cpu);pending();check();
    {const auto serial=operation.operation_serial();pending();
     auto state=std::make_shared<const detail::BnwLabelSystemFontRequest::Impl>(entry,entry->life,
      entry->source,entry->cache,height,entry->occurrence,serial);pending();check();
     detail::BnwLabelSystemFontRequest request(std::move(state));
     const auto request_guard=request.capture_guard_without_callbacks();request_guard.require_source_owned(operation);pending();
     auto pair=operation.system_font_atlas_query_owned(request);pending();check();
     require(bool(pair),"System font actual renderer returned no atlas metadata");
     pair->require_pending_source_owned(operation,request_guard);pending();check();entry->atlas=pair;
    }
    pending();check();
    // Request and copied language DTO cleanup precedes actual publication.
    auto pair=entry->atlas.lock();require(bool(pair),"System font pending real atlas pair retired");
    pair->require_current_without_callbacks();pending();check();
    const auto pending_at=std::find_if(registry->pending.begin(),registry->pending.end(),[&](const auto& e){return e.get()==entry.get();});
    require(pending_at!=registry->pending.end()&&!cell->completed,"System font publication pending owner changed");
    at->second.completed=entry;entry->returned=true;registry->pending.erase(pending_at);
    first();entry->source->require_current_owned();first();pair->require_cached_source_owned(operation,*entry->source);check();
   }
   // Native HIT retains the exact existing cache allocation. Source/CPU/GPU
   // validation has no language SQL supplier and does not change the key.
   first();require(entry->returned&&entry->source&&entry->cpu&&entry->cache,"System font positive cell lacks real CPU allocation");
   auto pair=entry->atlas.lock();require(bool(pair),"System font positive cell lost renderer-owned pair");
   entry->source->require_current_owned();check();pair->require_cached_source_owned(operation,*entry->source);check();
   auto state=std::make_shared<const detail::BnwLabelSystemFontBorrower::Impl>(registry->life,entry->life,registry,entry,
    entry->source,entry->cpu,entry->cache,pair,cell,height,entry->occurrence);check();
   result=std::shared_ptr<const detail::BnwLabelSystemFontBorrower>(new detail::BnwLabelSystemFontBorrower(std::move(state)));
   check();result->require_source_owned(operation);check();
  }
  first();full();first();require(!registry->busy,"System font manager cleanup did not unwind");
  result->require_source_owned(operation);first();full();first();return result;
 }catch(...){registry->life->failed.store(true);saved->failed.store(true);throw;}
}
