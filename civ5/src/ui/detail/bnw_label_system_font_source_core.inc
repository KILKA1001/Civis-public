// Inside civ5::ui AFTER complete Gui/Font/registry definitions. Private WIP;
// Root friend literal and whole-source peer are required before integration.
struct detail::BnwLabelSystemFontSource::Impl final {
 using Registry=detail::BnwLabelSystemFontRegistryFields;
 const std::thread::id creator=std::this_thread::get_id();
 const std::weak_ptr<detail::NotificationContextGuiPort> gui;
 const std::weak_ptr<detail::NotificationContextGuiPort::Impl> storage;
 const std::weak_ptr<Registry> registry;
 const std::weak_ptr<Registry::Entry> entry;
 const std::shared_ptr<Registry::Life> registry_life,entry_life;
 const Registry::Slot* const cell;
 const std::int32_t height;
 const std::uint64_t occurrence;
 const std::shared_ptr<const detail::BnwCompletedFontLoan::Impl> font;
 const BnwSystemFontCpuFrame frame;
 const std::uintptr_t hwnd;
 Impl(std::weak_ptr<detail::NotificationContextGuiPort> g,
      std::weak_ptr<detail::NotificationContextGuiPort::Impl> s,
      std::weak_ptr<Registry> r,std::weak_ptr<Registry::Entry> e,
      std::shared_ptr<Registry::Life> rl,std::shared_ptr<Registry::Life> el,
      const Registry::Slot* actual_cell,std::int32_t h,std::uint64_t serial,
      std::shared_ptr<const detail::BnwCompletedFontLoan::Impl> f,
      BnwSystemFontCpuFrame observed,std::uintptr_t actual_hwnd)
  :gui(std::move(g)),storage(std::move(s)),registry(std::move(r)),entry(std::move(e)),
   registry_life(std::move(rl)),entry_life(std::move(el)),cell(actual_cell),height(h),occurrence(serial),
   font(std::move(f)),frame(std::move(observed)),hwnd(actual_hwnd){}
 struct Current final {
  std::shared_ptr<detail::NotificationContextGuiPort::Impl> storage;
  std::shared_ptr<Registry> registry;
  std::shared_ptr<Registry::Entry> entry;
 };
 Current first()const{
  require(creator==std::this_thread::get_id()&&registry_life&&entry_life,
   "System font source foreign creator/missing independent life");
  registry_life->first();entry_life->first();
  // A lexical metadata view reuses the genuine Font guard. No Font allocation,
  // registry insertion, loader or externally returned Font loan is issued.
  require(bool(font),"System font source genuine Font metadata missing");
  detail::BnwCompletedFontLoan view(font);view.require_current_without_callbacks();
  const auto metrics=view.metrics_without_callbacks();
  require(metrics[0]==frame.source_label_ascent&&metrics[2]==height,
   "System font actual source Font scalar snapshot changed");
  Current current{storage.lock(),registry.lock(),entry.lock()};
  require(current.storage&&current.registry&&current.entry,"System font source canonical storage retired");
  {const auto facade=gui.lock();
   require(facade&&facade->impl_.get()==current.storage.get()&&
    !facade->impl_.owner_before(current.storage)&&!current.storage.owner_before(facade->impl_)&&
    current.storage->canonical_gui.lock().get()==facade.get()&&
    !current.storage->canonical_gui.owner_before(facade)&&!facade.owner_before(current.storage->canonical_gui),
    "System font source Gui control changed");}
  // No strong Gui facade survives this pure cut into an observer.
  const auto& s=current.storage;const auto& r=current.registry;const auto& e=current.entry;
  require(s->creator==creator&&s->facade_alive.load()&&!s->failed.load()&&
   s->system_font_registry.get()==r.get()&&!s->system_font_registry.owner_before(r)&&!r.owner_before(s->system_font_registry)&&
   r->life.get()==registry_life.get()&&!r->life.owner_before(registry_life)&&!registry_life.owner_before(r->life)&&
   e->life.get()==entry_life.get()&&!e->life.owner_before(entry_life)&&!entry_life.owner_before(e->life),
   "System font actual Gui manager/life changed");
  const auto at=r->map.find(height);
  require(at!=r->map.end()&&&at->second==cell&&e->height==height&&e->occurrence==occurrence&&
   e->source&&e->source->impl_.get()==this&&e->source->impl_->height==height,
   "System font actual signed-height cell/source changed");
  if(e->returned){
   require(at->second.completed.get()==e.get()&&!at->second.completed.owner_before(e)&&!e.owner_before(at->second.completed),
    "System font canonical positive allocation changed");
  }else{
   const bool actual_pending=std::any_of(r->pending.begin(),r->pending.end(),[&](const auto& item){
    return item.get()==e.get()&&!item.owner_before(e)&&!e.owner_before(item);
   });
   require(!at->second.completed&&r->busy&&actual_pending,
    "System font constructor pending allocation no longer actual");
  }
  require(s->hwnd==hwnd&&s->field_644==frame.client.width&&s->field_648==frame.client.height&&
   s->language_980a&&s->language_980a->completed&&!s->refreshing_language_980a&&
   s->language_980a->value==1&&s->language_980a->language==frame.locale.language&&
   frame.language_row.language==frame.locale.language&&frame.language_row.use_extended_font&&
   frame.source_line_height==height,
   "System font immutable selected Gui/client/language data changed");
  registry_life->first();entry_life->first();return current;
 }
};
detail::BnwLabelSystemFontSource::BnwLabelSystemFontSource(std::shared_ptr<const Impl> p):impl_(std::move(p)){}
detail::BnwLabelSystemFontSource::~BnwLabelSystemFontSource()=default;
void detail::BnwLabelSystemFontSource::require_current_without_callbacks()const{
 const auto saved=impl_;try{(void)saved->first();}catch(...){saved->entry_life->failed.store(true);throw;}
}
void detail::BnwLabelSystemFontSource::require_same_operation_without_callbacks(
 const runtime::BnwWorldViewSourceOperationLoan& operation)const{
 const auto saved=impl_;const auto current=saved->first();
 {const auto source=current.storage->source.lock();
  require(bool(source),"System font actual Source retired before operation identity");
  require(&source->current_operation_loan()==&operation,
   "System font atlas operation is not the actual current canonical Source loan");}
 (void)saved->first();
}
void detail::BnwLabelSystemFontSource::require_current_owned()const{
 const auto saved=impl_;
 try{
  const auto current=saved->first();
  const runtime::BnwWorldViewSourceOperationLoan* operation=nullptr;
  {const auto source=current.storage->source.lock();require(bool(source),"System font actual Source facade retired");
   operation=&source->current_operation_loan();}
  const auto pure=[&]{
   (void)saved->first();
   {const auto source=current.storage->source.lock();require(bool(source),"System font actual Source retired during observer");
    require(&source->current_operation_loan()==operation,"System font actual Source operation changed");}
   (void)saved->first();
  };
  // Source facade locks remain inside pure cuts; storage retention is not
  // admission and cannot mask original Source retirement during callbacks.
  pure();current.storage->first(*operation);pure();operation->require_current();pure();
  detail::BnwCompletedFontLoan view(saved->font);view.require_source_owned(*operation);pure();
  operation->require_current();pure();
 }catch(...){saved->entry_life->failed.store(true);throw;}
}
BnwSystemFontCpuFrame detail::BnwLabelSystemFontSource::captured_cpu_frame_owned()const{
 const auto saved=impl_;require_current_without_callbacks();require_current_owned();require_current_without_callbacks();
 auto result=saved->frame;require_current_without_callbacks();require_current_owned();require_current_without_callbacks();return result;
}
std::uintptr_t detail::BnwLabelSystemFontSource::hwnd_without_callbacks()const{
 const auto saved=impl_;(void)saved->first();const auto result=saved->hwnd;(void)saved->first();return result;
}

struct detail::BnwLabelSystemFontRequest::Impl final {
 using Registry=detail::BnwLabelSystemFontRegistryFields;
 const std::weak_ptr<Registry::Entry> entry;
 const std::shared_ptr<Registry::Life> life;
 const std::shared_ptr<const detail::BnwLabelSystemFontSource> source;
 const std::weak_ptr<BnwSystemFontCacheOwner> cache;
 const std::int32_t height;
 const std::uint64_t occurrence,serial;
 mutable std::atomic<bool> alive{true};
 Impl(std::weak_ptr<Registry::Entry> e,std::shared_ptr<Registry::Life> l,
      std::shared_ptr<const detail::BnwLabelSystemFontSource> s,std::weak_ptr<BnwSystemFontCacheOwner> c,
      std::int32_t h,std::uint64_t sequence,std::uint64_t actual_serial)
  :entry(std::move(e)),life(std::move(l)),source(std::move(s)),cache(std::move(c)),height(h),occurrence(sequence),serial(actual_serial){}
 void first()const{
  require(alive.load()&&life&&source,"System font request lexical lifetime retired");life->first();
  source->require_current_without_callbacks();
  const auto e=entry.lock();const auto c=cache.lock();
  require(e&&c&&!e->returned&&e->height==height&&e->occurrence==occurrence&&
   e->life.get()==life.get()&&!e->life.owner_before(life)&&!life.owner_before(e->life)&&
   e->source.get()==source.get()&&!e->source.owner_before(source)&&!source.owner_before(e->source)&&
   e->cache.get()==c.get()&&!e->cache.owner_before(c)&&!c.owner_before(e->cache)&&e->cpu,
   "System font request actual pending CPU/cache allocation changed");
  life->first();
 }
};
detail::BnwLabelSystemFontRequest::BnwLabelSystemFontRequest(std::shared_ptr<const Impl> p):impl_(std::move(p)){}
detail::BnwLabelSystemFontRequest::~BnwLabelSystemFontRequest(){impl_->alive.store(false);}
void detail::BnwLabelSystemFontRequest::require_pending_without_callbacks()const{const auto saved=impl_;saved->first();}
void detail::BnwLabelSystemFontRequest::require_pending_source_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
 const auto saved=impl_;saved->first();operation.require_operation(saved->serial);saved->first();
 saved->source->require_current_owned();saved->first();operation.require_operation(saved->serial);saved->first();
}
std::shared_ptr<const detail::BnwLabelSystemFontSource> detail::BnwLabelSystemFontRequest::source_without_callbacks()const{
 const auto saved=impl_;saved->first();auto result=saved->source;saved->first();return result;
}
std::shared_ptr<BnwSystemFontCacheOwner> detail::BnwLabelSystemFontRequest::cache_without_callbacks()const{
 const auto saved=impl_;saved->first();auto result=saved->cache.lock();saved->first();return result;
}
std::int32_t detail::BnwLabelSystemFontRequest::height_without_callbacks()const{
 const auto saved=impl_;saved->first();return saved->height;
}
