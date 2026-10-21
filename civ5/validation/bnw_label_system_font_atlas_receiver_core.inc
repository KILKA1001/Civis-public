// Inside civ5::ui AFTER the complete actual WorldViewGridProvider. Atlas
// construction uses only this renderer's genuine Source/Gui/window stack.
std::shared_ptr<const graphics::detail::BnwLabelSystemFontAtlasMetadata>
UiScreenGlRenderer::query_system_font_atlas_owned(
 const BnwWorldViewRendererProjectionLoan& projection,
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const BnwOwnedCameraGuiScreen::Loan& gui,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,
 const detail::BnwLabelSystemFontRequest::Guard& request){
 using Metadata=graphics::detail::BnwLabelSystemFontAtlasMetadata;
 const auto life=worldview_lifetime_;
 const auto renderer_first=[&]{
  if(life->creator!=std::this_thread::get_id()||!life->alive.load()||life->failed.load()||
   life->renderer!=this||!life->active.load()||life->active_scope)
   throw std::runtime_error("System font atlas requires actual pre-paint Source renderer scope");
 };
 renderer_first();request.first_without_callbacks();
 const auto storage=system_font_cache_;const auto* const actual_context=&context_;
 const auto first=[&]{
  renderer_first();request.first_without_callbacks();
  if(!storage||storage->creator!=std::this_thread::get_id()||!storage->alive.load()||storage->failed.load()||
   storage->renderer!=this||storage->renderer_life.owner_before(life)||life.owner_before(storage->renderer_life))
   throw std::runtime_error("System font actual saved renderer cache storage retired/replaced");
 };
 first();if(storage->active.exchange(true)){life->failed.store(true);throw std::runtime_error("System font atlas receiver reentered");}
 std::shared_ptr<const Metadata> result;
 try{
  {struct End{std::shared_ptr<SystemFontCacheStorage> s;~End(){s->active.store(false);s->cleanup_if_quiescent_owned();}}end{storage};
   const auto source=request.source_without_callbacks();first();
   const auto cpu_cache=request.cache_without_callbacks();first();
   const auto height=request.height_without_callbacks();first();
   const auto serial=request.serial_without_callbacks();first();
   const auto pure=[&]{
    first();projection.require_current_without_callbacks();first();
    if(projection.renderer_!=this||projection.lifetime_.get()!=life.get()||
     projection.lifetime_.owner_before(life)||life.owner_before(projection.lifetime_))
     throw std::runtime_error("System font actual renderer projection control changed");
    source->require_current_without_callbacks();first();
   };
   const auto full=[&]{
    pure();operation.require_operation(serial);first();pure();
    request.require_source_owned(operation);first();pure();
    gui.require_current();first();pure();window.require_current();first();pure();
   };
   full();
   // Acquire the exact original GlContext shared control. The short Window
   // facade lock ends before every callback; Context storage does not admit
   // a retired renderer, Source, window transaction or Gui allocation.
   std::shared_ptr<const graphics::detail::BnwRendererContextPin> context_pin;
   std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window_origin;
   {auto window_owner=window.retain_owner();first();pure();
    if(!window_owner)throw std::runtime_error("System font actual Window facade missing");
    // Account this exact temporary Window facade BEFORE the allocating
    // Context-pin getter can invoke new_handler. The existing RendererPin
    // owns no extra facade slot and cannot renew an outside Window owner.
    const graphics::BnwOwnedCameraRendererWindowLease::RendererPin window_pin(window_owner);
    window_pin.require_current_without_native_callbacks();first();pure();
    window_origin=window_owner;
    context_pin=window_owner->retain_context_pin_without_native_callbacks();first();pure();
    window_pin.require_current_without_native_callbacks();first();pure();
   }
   full();
   if(!context_pin)throw std::runtime_error("System font genuine counted Context pin missing");
   context_pin->first_without_callbacks();const auto* const context=&context_pin->context_reference_without_callbacks();
   if(context!=actual_context)throw std::runtime_error("System font GL owner is not this renderer's actual Context");
   const auto native=context->native_identity();first();pure();
   const auto hwnd=window.window();first();pure();const auto dc=window.dc();first();pure();
   const auto wgl=window.wgl_context();first();pure();const auto window_occurrence=window.window_occurrence();first();pure();
   if(!native.owner_thread||!native.context||native.window!=hwnd||native.dc!=dc||native.context!=wgl||
    source->hwnd_without_callbacks()!=hwnd)throw std::runtime_error("System font actual HWND/DC/WGL/source identity changed");
   full();context->check("Before actual Gui system font atlas pair");first();full();
   const auto same=[](const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);};
   if(const auto old=storage->entries.find(source.get());old!=storage->entries.end()){
    const auto entry=old->second;first();
    if(entry->source.get()!=source.get()||!same(entry->source,source)||entry->cpu_cache.get()!=cpu_cache.get()||
     !same(entry->cpu_cache,cpu_cache)||!same(entry->metadata->impl_->window,window_origin)||
     entry->metadata->impl_->window_occurrence!=window_occurrence||entry->metadata->impl_->context_identity!=context)
     throw std::runtime_error("System font actual backend cache allocation substituted");
    result=entry->metadata;result->require_pending_source_owned(operation,request);first();full();
   }else{
    const auto cpu=cpu_cache->source_cpu_owner();first();full();
    if(!cpu)throw std::runtime_error("System font genuine CPU source missing before pair");
    const auto observed=cpu->current();first();full();
    if(observed.source_line_height!=height||observed.client.width<=0||observed.client.height<=0||
     !observed.cpu_raster_available)throw std::runtime_error("System font actual GDI constructor geometry missing");
    auto atlas=graphics::GlBnwSystemFontAtlas::create_from_owned_renderer_context(context_pin,cpu_cache);first();full();
    const auto pair=atlas->current();first();full();
    if(!pair.pair_storage_available||!pair.green_texture||!pair.shadow_texture||pair.green_texture==pair.shadow_texture||
     pair.width!=observed.atlas_width||pair.height!=observed.atlas_height)
     throw std::runtime_error("System font real green/shadow atlas pair did not match GDI geometry");
    if(storage->sequence==UINT64_MAX)throw std::runtime_error("System font backend allocation sequence exhausted");
    const auto occurrence=storage->sequence+1;
    auto state=std::make_shared<Metadata::Impl>(life,storage,this,source,cpu_cache,context_pin,native,
     window_origin,window_occurrence,height,occurrence);first();full();
    auto metadata=std::shared_ptr<const Metadata>(new Metadata(std::move(state)));first();full();
    auto entry=std::make_shared<const SystemFontCacheEntry>(source,cpu_cache,metadata,std::move(atlas));first();full();
    metadata->impl_->entry=entry;
    // Stage allocation outside the actual live map. Transfer the node without
    // allocator callbacks after the last genuine source check.
    decltype(storage->entries) staged;staged.emplace(source.get(),entry);first();full();
    auto node=staged.extract(source.get());first();full();
    const auto inserted=storage->entries.insert(std::move(node));first();
    if(!inserted.inserted)throw std::runtime_error("System font actual backend insertion replaced");
    storage->sequence=occurrence;result=metadata;
    result->require_pending_source_owned(operation,request);first();full();
   }
   first();full();
  }
  // GDI snapshots, Context temporary, callback locals and the active exclusion
  // have unwound. Last POST reads independent life before any successful return.
  first();if(storage->active.load())throw std::runtime_error("System font backend receiver cleanup did not unwind");
  result->require_pending_source_owned(operation,request);first();operation.require_current();first();
  result->require_current_without_callbacks();first();return result;
 }catch(...){storage->failed.store(true);life->failed.store(true);throw;}
}
std::shared_ptr<const graphics::detail::BnwLabelSystemFontAtlasMetadata>
UiScreenGlRenderer::WorldViewGridProvider::query_system_font_atlas_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const BnwOwnedCameraGuiScreen::Loan& gui,
 const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,
 const detail::BnwLabelSystemFontRequest::Guard& request)const{
 const auto life=life_;request.first_without_callbacks();require_for_source(operation);
 const auto first=[&]{
  if(life->creator!=std::this_thread::get_id()||!life->alive.load()||life->failed.load())
   throw std::runtime_error("System font Grid actual renderer life retired");request.first_without_callbacks();
 };
 first();auto* const renderer=const_cast<UiScreenGlRenderer*>(projection_.renderer_);
 if(!renderer||life->renderer!=renderer)throw std::runtime_error("System font Grid actual renderer replaced");
 auto result=renderer->query_system_font_atlas_owned(projection_,operation,gui,window,request);first();
 require_for_source(operation);first();result->require_pending_source_owned(operation,request);first();
 require_for_source(operation);first();return result;
}
