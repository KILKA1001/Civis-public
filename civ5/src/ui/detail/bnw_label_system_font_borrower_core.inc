// Inside civ5::ui after Source/Request and the canonical Gui definitions.
struct detail::BnwLabelSystemFontBorrower::Impl final {
 using Registry=detail::BnwLabelSystemFontRegistryFields;
 const std::shared_ptr<Registry::Life> registry_life,entry_life;
 const std::weak_ptr<Registry> registry;
 const std::weak_ptr<Registry::Entry> entry;
 const std::weak_ptr<const detail::BnwLabelSystemFontSource> source;
 const std::weak_ptr<BnwSystemFontCpuOwner> cpu;
 const std::weak_ptr<BnwSystemFontCacheOwner> cache;
 const std::weak_ptr<const graphics::detail::BnwLabelSystemFontAtlasMetadata> atlas;
 const Registry::Slot* const cell;
 const std::int32_t height;
 const std::uint64_t occurrence;
 Impl(std::shared_ptr<Registry::Life> rl,std::shared_ptr<Registry::Life> el,
      std::weak_ptr<Registry> r,std::weak_ptr<Registry::Entry> e,
      std::weak_ptr<const detail::BnwLabelSystemFontSource> s,
      std::weak_ptr<BnwSystemFontCpuOwner> c,std::weak_ptr<BnwSystemFontCacheOwner> ring,
      std::weak_ptr<const graphics::detail::BnwLabelSystemFontAtlasMetadata> pair,
      const Registry::Slot* actual_cell,std::int32_t h,std::uint64_t sequence)
  :registry_life(std::move(rl)),entry_life(std::move(el)),registry(std::move(r)),entry(std::move(e)),
   source(std::move(s)),cpu(std::move(c)),cache(std::move(ring)),atlas(std::move(pair)),
   cell(actual_cell),height(h),occurrence(sequence){}
 struct Current final {
  std::shared_ptr<const detail::BnwLabelSystemFontSource> source;
  std::shared_ptr<const graphics::detail::BnwLabelSystemFontAtlasMetadata> atlas;
 };
 Current first()const{
  registry_life->first();entry_life->first();
  Current result{source.lock(),atlas.lock()};
  require(result.source&&result.atlas,"Label170 weak source/atlas metadata retired");
  result.source->require_current_without_callbacks();result.atlas->require_current_without_callbacks();
  const auto r=registry.lock();const auto e=entry.lock();const auto c=cpu.lock();const auto ring=cache.lock();
  require(r&&e&&c&&ring,"Label170 actual canonical CPU allocation retired");
  const auto same=[](const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);};
  const auto at=r->map.find(height);
  require(at!=r->map.end()&&&at->second==cell&&at->second.completed.get()==e.get()&&same(at->second.completed,e)&&
   e->returned&&e->height==height&&e->occurrence==occurrence&&
   r->life.get()==registry_life.get()&&same(r->life,registry_life)&&
   e->life.get()==entry_life.get()&&same(e->life,entry_life)&&
   e->source.get()==result.source.get()&&same(e->source,result.source)&&
   e->cpu.get()==c.get()&&same(e->cpu,c)&&e->cache.get()==ring.get()&&same(e->cache,ring)&&
   same(e->atlas,result.atlas)&&ring->source_cpu_owner().get()==c.get()&&same(ring->source_cpu_owner(),c),
   "Label170 typed borrower is outside the actual positive signed-height allocation");
  registry_life->first();entry_life->first();return result;
 }
 template<class First>void full(const runtime::BnwWorldViewSourceOperationLoan& operation,const First& fence)const{
  fence();auto current=first();operation.require_current();fence();
  current.source->require_current_owned();fence();
  current.atlas->require_cached_source_owned(operation,*current.source);fence();
  operation.require_current();fence();
 }
 struct TextOperation final {
  const std::shared_ptr<const Impl> saved;
  const std::shared_ptr<Registry::Entry> physical;
  explicit TextOperation(std::shared_ptr<const Impl> input):saved(std::move(input)),physical(saved->entry.lock()){
   (void)saved->first();require(physical&&!physical->text_operation_active,"System font actual text producer reentered");
   physical->text_operation_active=true;
  }
  ~TextOperation(){physical->text_operation_active=false;}
 };
};
detail::BnwLabelSystemFontBorrower::BnwLabelSystemFontBorrower(std::shared_ptr<const Impl> p):impl_(std::move(p)){}
detail::BnwLabelSystemFontBorrower::~BnwLabelSystemFontBorrower()=default;
std::shared_ptr<const graphics::detail::BnwLabelSystemFontAtlasMetadata>
detail::BnwLabelSystemFontBorrower::atlas_metadata_without_callbacks()const{
 // Metadata custody only. Genuine Paint must independently prove SAME170
 // BEFORE calling this getter; neither metadata nor this copy issues Paint.
 const auto saved=impl_;
 try{
  std::shared_ptr<const graphics::detail::BnwLabelSystemFontAtlasMetadata> result;
  {const auto current=saved->first();result=current.atlas;}
  // All temporary canonical owner locks are gone before the final FIRST.
  {const auto current=saved->first();
   require(current.atlas.get()==result.get()&&!current.atlas.owner_before(result)&&
    !result.owner_before(current.atlas),"Label170 actual Atlas metadata changed during DATA capture");}
  (void)saved->first();return result;
 }catch(...){saved->entry_life->failed.store(true);throw;}
}

void detail::BnwLabelSystemFontBorrower::require_current_without_callbacks()const{
 const auto saved=impl_;try{(void)saved->first();}catch(...){saved->entry_life->failed.store(true);throw;}
}
void detail::BnwLabelSystemFontBorrower::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
 const auto saved=impl_;
 try{
  auto current=saved->first();operation.require_current();(void)saved->first();
  current.source->require_current_owned();(void)saved->first();
  current.atlas->require_cached_source_owned(operation,*current.source);(void)saved->first();
  operation.require_current();(void)saved->first();
 }catch(...){saved->entry_life->failed.store(true);throw;}
}
