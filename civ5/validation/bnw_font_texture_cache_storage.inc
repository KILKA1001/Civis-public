// Inside civ5::ui before the actual renderer constructor. GPU storage is
// independent of the raw renderer member so allocator callbacks cannot destroy
// a map while its insertion is still executing. Retention does not renew life.
struct UiScreenGlRenderer::FontTextureCacheEntry final {
    const std::shared_ptr<graphics::detail::BnwFontTextureBorrower::Impl> metadata;
    std::unique_ptr<graphics::GlTexture> texture;
    FontTextureCacheEntry(std::shared_ptr<graphics::detail::BnwFontTextureBorrower::Impl> state,
                          std::unique_ptr<graphics::GlTexture> actual)
        :metadata(std::move(state)),texture(std::move(actual)){}
    ~FontTextureCacheEntry(){
        metadata->alive.store(false);
        if(metadata->creator!=std::this_thread::get_id()){
            metadata->failed.store(true);std::terminate();
        }
        texture.reset();
    }
};
struct UiScreenGlRenderer::FontTextureCacheStorage final {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<WorldViewPaintLifetime> renderer_life;
    const UiScreenGlRenderer* const renderer;
    std::atomic<bool> alive{true},failed{false},active{false};
    std::uint64_t revision=0;
    std::unordered_map<std::string,std::shared_ptr<const FontTextureCacheEntry>> entries;
    FontTextureCacheStorage(const std::shared_ptr<WorldViewPaintLifetime>& life,const UiScreenGlRenderer* actual)
        :renderer_life(life),renderer(actual){}
    void cleanup_if_quiescent_owned()noexcept{
        if(!alive.load()&&!active.load()){
            if(creator!=std::this_thread::get_id())std::terminate();
            entries.clear();
        }
    }
    void retire_owned()noexcept{
        alive.store(false);
        if(creator!=std::this_thread::get_id()){failed.store(true);std::terminate();}
        cleanup_if_quiescent_owned();
    }
    ~FontTextureCacheStorage(){retire_owned();}
};
