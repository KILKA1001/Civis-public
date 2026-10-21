// Inside civ5::ui before the renderer constructor. Independent backend
// storage survives allocation callbacks; alive/renderer life remains authority.
struct UiScreenGlRenderer::SystemFontCacheEntry final {
 const std::shared_ptr<const detail::BnwLabelSystemFontSource> source;
 const std::shared_ptr<BnwSystemFontCacheOwner> cpu_cache;
 const std::shared_ptr<const graphics::detail::BnwLabelSystemFontAtlasMetadata> metadata;
 std::shared_ptr<graphics::GlBnwSystemFontAtlas> atlas;
 SystemFontCacheEntry(std::shared_ptr<const detail::BnwLabelSystemFontSource> s,
  std::shared_ptr<BnwSystemFontCacheOwner> c,
  std::shared_ptr<const graphics::detail::BnwLabelSystemFontAtlasMetadata> m,
  std::shared_ptr<graphics::GlBnwSystemFontAtlas> a)
  :source(std::move(s)),cpu_cache(std::move(c)),metadata(std::move(m)),atlas(std::move(a)){}
 ~SystemFontCacheEntry();
};
struct UiScreenGlRenderer::SystemFontCacheStorage final {
 const std::thread::id creator=std::this_thread::get_id();
 const std::weak_ptr<WorldViewPaintLifetime> renderer_life;
 const UiScreenGlRenderer* const renderer;
 std::atomic<bool> alive{true},failed{false},active{false};
 std::uint64_t sequence=0;
 // Backend ownership index only. Native signed-height map lives once in Gui.
 std::map<const detail::BnwLabelSystemFontSource*,std::shared_ptr<const SystemFontCacheEntry>> entries;
 SystemFontCacheStorage(const std::shared_ptr<WorldViewPaintLifetime>& life,const UiScreenGlRenderer* actual)
  :renderer_life(life),renderer(actual){}
 void cleanup_if_quiescent_owned()noexcept{
  if(!alive.load()&&!active.load()){
   if(creator!=std::this_thread::get_id())std::terminate();entries.clear();
   graphics::BnwOwnedCameraRendererWindowLease::drain_retired_creator_storage();
  }
 }
 void retire_owned()noexcept{
  alive.store(false);if(creator!=std::this_thread::get_id()){failed.store(true);std::terminate();}
  cleanup_if_quiescent_owned();
 }
 ~SystemFontCacheStorage(){retire_owned();}
};
