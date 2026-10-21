// Included at namespace scope AFTER the actual renderer storage definitions.
namespace civ5::graphics::detail {
struct BnwLabelSystemFontAtlasMetadata::Impl final {
 const std::thread::id creator=std::this_thread::get_id();
 const std::weak_ptr<ui::UiScreenGlRenderer::WorldViewPaintLifetime> renderer_life;
 const std::weak_ptr<ui::UiScreenGlRenderer::SystemFontCacheStorage> storage;
 std::weak_ptr<const ui::UiScreenGlRenderer::SystemFontCacheEntry> entry;
 const ui::UiScreenGlRenderer* const renderer;
 const std::weak_ptr<const ui::detail::BnwLabelSystemFontSource> source;
 const std::weak_ptr<ui::BnwSystemFontCacheOwner> cpu_cache;
 const std::weak_ptr<const BnwRendererContextPin> context_pin;
 const GlContext* const context_identity;
 const GlContextNativeIdentity native;
 const std::weak_ptr<const BnwOwnedCameraRendererWindowLease> window;
 const std::uintptr_t window_occurrence;
 const std::int32_t height;
 const std::uint64_t occurrence;
 std::atomic<bool> alive{true};mutable std::atomic<bool> failed{false};
 Impl(const std::shared_ptr<ui::UiScreenGlRenderer::WorldViewPaintLifetime>& life,
  const std::shared_ptr<ui::UiScreenGlRenderer::SystemFontCacheStorage>& cache,
  const ui::UiScreenGlRenderer* actual,
  const std::shared_ptr<const ui::detail::BnwLabelSystemFontSource>& s,
  const std::shared_ptr<ui::BnwSystemFontCacheOwner>& cpu,
  const std::shared_ptr<const BnwRendererContextPin>& c,GlContextNativeIdentity n,
  std::weak_ptr<const BnwOwnedCameraRendererWindowLease> w,std::uintptr_t window_id,
  std::int32_t h,std::uint64_t sequence)
  :renderer_life(life),storage(cache),renderer(actual),source(s),cpu_cache(cpu),context_pin(c),
   context_identity(&c->context_reference_without_callbacks()),native(n),window(std::move(w)),window_occurrence(window_id),height(h),occurrence(sequence){}
 void independent_first()const{
  if(creator!=std::this_thread::get_id()||!alive.load()||failed.load())
   throw std::runtime_error("System font actual atlas metadata retired/foreign/failed");
  const auto life=renderer_life.lock();const auto cache=storage.lock();
  if(!life||!cache||life->creator!=creator||!life->alive.load()||life->failed.load()||life->renderer!=renderer||
   cache->creator!=creator||!cache->alive.load()||cache->failed.load()||cache->renderer!=renderer||
   cache->renderer_life.owner_before(life)||life.owner_before(cache->renderer_life))
   throw std::runtime_error("System font saved actual renderer/cache life changed");
 }
 struct Current final {
  std::shared_ptr<const ui::detail::BnwLabelSystemFontSource> source;
 };
 Current first()const{
  independent_first();
  Current result{source.lock()};const auto actual_entry=entry.lock();const auto cache=storage.lock();const auto cpu=cpu_cache.lock();
  const auto pin=context_pin.lock();
  if(!pin)throw std::runtime_error("System font genuine counted Context pin retired");
  pin->first_without_callbacks();const auto* const ctx=&pin->context_reference_without_callbacks();
  if(!result.source||!actual_entry||!cache||!cpu||!ctx||ctx!=context_identity||window.expired())
   throw std::runtime_error("System font actual cache source/CPU/GL/window retired");
  const auto same=[](const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);};
  const auto at=cache->entries.find(result.source.get());
  if(at==cache->entries.end()||at->second.get()!=actual_entry.get()||!same(at->second,actual_entry)||
   actual_entry->source.get()!=result.source.get()||!same(actual_entry->source,result.source)||
   actual_entry->cpu_cache.get()!=cpu.get()||!same(actual_entry->cpu_cache,cpu)||
   actual_entry->metadata->impl_.get()!=this||!actual_entry->atlas)
   throw std::runtime_error("System font actual backend positive allocation changed");
  const auto sources=actual_entry->atlas->sources_identity_no_current();
  const auto current_native=ctx->native_identity();
  if(sources.context!=ctx||sources.cache.get()!=cpu.get()||!same(sources.cache,cpu)||
   sources.window!=native.window||!sources.pair.pair_storage_available||!sources.pair.green_texture||!sources.pair.shadow_texture||
   sources.pair.green_texture==sources.pair.shadow_texture||
   !current_native.owner_thread||!native.owner_thread||current_native.window!=native.window||
   current_native.dc!=native.dc||current_native.context!=native.context)
   throw std::runtime_error("System font actual nonzero atlas pair/GL control changed");
  pin->first_without_callbacks();independent_first();return result;
 }
};
BnwLabelSystemFontAtlasMetadata::BnwLabelSystemFontAtlasMetadata(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwLabelSystemFontAtlasMetadata::~BnwLabelSystemFontAtlasMetadata()=default;
BnwLabelSystemFontAtlasMetadata::Guard::Guard(std::shared_ptr<Impl> value):impl_(std::move(value)){}
BnwLabelSystemFontAtlasMetadata::Guard BnwLabelSystemFontAtlasMetadata::capture_guard_without_callbacks()const{
 const auto saved=impl_;auto current=saved->first();current.source->require_current_without_callbacks();
 (void)saved->first();return Guard(saved);
}
void BnwLabelSystemFontAtlasMetadata::Guard::require_current_without_callbacks()const{
 const auto saved=impl_;
 try{auto current=saved->first();current.source->require_current_without_callbacks();(void)saved->first();}
 catch(...){saved->failed.store(true);throw;}
}
void BnwLabelSystemFontAtlasMetadata::Guard::require_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation)const{
 const auto saved=impl_;
 try{
  const auto pure=[&]{auto current=saved->first();current.source->require_current_without_callbacks();
   current.source->require_same_operation_without_callbacks(operation);(void)saved->first();};
  pure();
  {auto current=saved->first();current.source->require_current_owned();pure();}
  operation.require_current();pure();
  // Only source/metadata DATA survives observers; actual GPU Entry/facade
  // locks stay in callback-free cuts. Storage/life is independently tested.
  pure();
 }catch(...){saved->failed.store(true);throw;}
}
void BnwLabelSystemFontAtlasMetadata::Guard::require_actual_atlas_without_callbacks(
 const GlBnwSystemFontAtlas& actual)const{
 const auto saved=impl_;
 try{
  require_current_without_callbacks();
  {const auto entry=saved->entry.lock();
   if(!entry||entry->atlas.get()!=&actual)
    throw std::runtime_error("System font physical Atlas is not the canonical backend facade");}
  require_current_without_callbacks();
 }catch(...){saved->failed.store(true);throw;}
}
std::shared_ptr<GlBnwSystemFontAtlas>
BnwLabelSystemFontAtlasMetadata::Guard::retain_atlas_for_physical_cleanup_without_callbacks()const{
 const auto saved=impl_;
 try{
  require_current_without_callbacks();std::shared_ptr<GlBnwSystemFontAtlas> result;
  {const auto entry=saved->entry.lock();
   if(!entry||!entry->atlas)throw std::runtime_error("System font actual Atlas physical custody retired");
   result=entry->atlas;}
  require_current_without_callbacks();
  // Retaining Atlas alone cannot retain backend Entry. Its destructor retires
  // saved->alive FIRST, independently of this physical cleanup reference.
  return result;
 }catch(...){saved->failed.store(true);throw;}
}

void BnwLabelSystemFontAtlasMetadata::require_current_without_callbacks()const{
 const auto saved=impl_;try{auto current=saved->first();current.source->require_current_without_callbacks();(void)saved->first();}
 catch(...){saved->failed.store(true);throw;}
}
void BnwLabelSystemFontAtlasMetadata::require_pending_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const ui::detail::BnwLabelSystemFontRequest::Guard& request)const{
 const auto saved=impl_;
 try{
  request.first_without_callbacks();require_current_without_callbacks();
  auto source=request.source_without_callbacks();auto cache=request.cache_without_callbacks();
  saved->independent_first();const auto current=saved->first();
  const auto same=[](const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);};
  if(current.source.get()!=source.get()||!same(current.source,source)||!same(saved->cpu_cache,cache)||
   saved->height!=request.height_without_callbacks())throw std::runtime_error("System font atlas request source/key replaced");
  request.require_source_owned(operation);saved->independent_first();request.first_without_callbacks();
  require_current_without_callbacks();operation.require_current();saved->independent_first();
  request.first_without_callbacks();require_current_without_callbacks();
 }catch(...){saved->failed.store(true);throw;}
}
void BnwLabelSystemFontAtlasMetadata::require_cached_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const ui::detail::BnwLabelSystemFontSource& source)const{
 const auto saved=impl_;const auto source_metadata=source.impl_;
 try{
  require_current_without_callbacks();auto current=saved->first();
  if(current.source->impl_.get()!=source_metadata.get()||current.source->impl_.owner_before(source_metadata)||
   source_metadata.owner_before(current.source->impl_))throw std::runtime_error("System font cached Gui source changed");
  current.source->require_current_owned();saved->independent_first();require_current_without_callbacks();
  operation.require_current();saved->independent_first();require_current_without_callbacks();
 }catch(...){saved->failed.store(true);throw;}
}
}
namespace civ5::ui {
UiScreenGlRenderer::SystemFontCacheEntry::~SystemFontCacheEntry(){
 metadata->impl_->alive.store(false);
 if(metadata->impl_->creator!=std::this_thread::get_id()){metadata->impl_->failed.store(true);std::terminate();}
 atlas.reset();
}
}
