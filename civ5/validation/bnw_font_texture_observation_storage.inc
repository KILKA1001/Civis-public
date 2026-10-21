// After complete UiScreenGlRenderer and request/resource headers, outside ui namespace.
// Metadata ONLY. No GlTexture owner, GL calls or UiRenderer out-of-line references.
namespace civ5::graphics::detail {
struct BnwFontTextureBorrower::Impl final {
    // Fixed query provenance, not inferred sampler/filter semantics.
    static constexpr std::uint32_t request_mode=0x20000,request_extra=0;
    enum class StoragePolicy {direct_raw_dds_linear};
    static constexpr StoragePolicy storage_policy=StoragePolicy::direct_raw_dds_linear;
    using Life=ui::UiScreenGlRenderer::WorldViewPaintLifetime;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<Life> renderer_life;
    const ui::UiScreenGlRenderer* const renderer;
    const std::shared_ptr<const assets::CivisResourceBankOwner> bank;
    const assets::CivisLoadedResource loaded;
    const std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
    const std::weak_ptr<ui::BnwClientSceneFrame> scene;
    const std::weak_ptr<const ui::SelectedBnwPreviewHost> host;
    const std::weak_ptr<const BnwOwnedCameraRendererWindowLease> window;
    const std::uintptr_t window_occurrence;
    const std::uint64_t generation,revision;
    const std::string key;
    std::weak_ptr<const void> cache_payload;
    const void* payload_identity=nullptr;
    std::atomic<bool> alive{true};
    mutable std::atomic<bool> failed{false};
    Impl(const std::shared_ptr<Life>& life,const ui::UiScreenGlRenderer* actual,
         std::shared_ptr<const assets::CivisResourceBankOwner> actual_bank,
         assets::CivisLoadedResource actual_loaded,
         std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> actual_source,
         std::weak_ptr<ui::BnwClientSceneFrame> actual_scene,
         std::weak_ptr<const ui::SelectedBnwPreviewHost> actual_host,
         std::weak_ptr<const BnwOwnedCameraRendererWindowLease> actual_window,
         std::uintptr_t actual_window_occurrence,
         std::uint64_t actual_generation,std::uint64_t actual_revision,std::string actual_key)
        :renderer_life(life),renderer(actual),bank(std::move(actual_bank)),loaded(std::move(actual_loaded)),
         source(std::move(actual_source)),scene(std::move(actual_scene)),host(std::move(actual_host)),
         window(std::move(actual_window)),window_occurrence(actual_window_occurrence),
         generation(actual_generation),revision(actual_revision),key(std::move(actual_key)){}
    void first()const {
        if(creator!=std::this_thread::get_id()||!alive.load()||failed.load())
            throw std::runtime_error("Font texture payload metadata retired/foreign/failed");
        const auto life=renderer_life.lock();
        if(!life||life->creator!=creator||!life->alive.load()||life->failed.load()||life->renderer!=renderer)
            throw std::runtime_error("Font texture actual renderer retired");
        if(!bank||!generation||!revision||!window_occurrence||source.expired()||scene.expired()||host.expired()||window.expired())
            throw std::runtime_error("Font texture source lineage retired");
        {
            // This temporary dies before any Source/Bank/GUI observer. It cannot
            // hide loss of the actual renderer map's outside GPU owner.
            const auto payload=cache_payload.lock();
            if(!payload||payload.get()!=payload_identity||payload.use_count()<=1)
                throw std::runtime_error("Font texture canonical cache payload retired");
        }
        bank->require_current();
    }
};
struct BnwFontTextureObservation::Impl final {
    using RequestLife=ui::detail::BnwFontTextureRequest::Life;
    using ResourceLife=runtime::detail::BnwFontResourceLoan::Life;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<RequestLife> request_life;
    const std::weak_ptr<ResourceLife> resource_life;
    const std::uint64_t occurrence,resource_epoch,serial;
    const ui::detail::BnwFontTextureRequest* const request_identity;
    const runtime::detail::BnwFontResourceLoan* const resource_identity;
    const std::shared_ptr<const BnwFontTextureBorrower> borrower;
    // No default/NULL-result factory. Only a future actual query/warning branch
    // may add a separately proved observed-NULL representation.
    Impl(std::weak_ptr<RequestLife> life,std::weak_ptr<ResourceLife> resource,
         std::uint64_t actual_occurrence,std::uint64_t epoch,std::uint64_t actual_serial,
         const ui::detail::BnwFontTextureRequest* actual_request,
         const runtime::detail::BnwFontResourceLoan* actual_resource,
         std::shared_ptr<const BnwFontTextureBorrower> actual_borrower)
        :request_life(std::move(life)),resource_life(std::move(resource)),occurrence(actual_occurrence),
         resource_epoch(epoch),serial(actual_serial),request_identity(actual_request),
         resource_identity(actual_resource),borrower(std::move(actual_borrower)){}
    void first()const {
        if(creator!=std::this_thread::get_id())throw std::runtime_error("Font texture observation foreign creator");
        const auto request=request_life.lock();const auto resource=resource_life.lock();
        if(!request||!resource)throw std::runtime_error("Font texture actual request/resource scope retired");
        request->require_current_owned(occurrence);resource->first(resource_epoch);
        if(!borrower)throw std::runtime_error("Font texture observed-NULL issuer is not implemented");
        borrower->impl_->first();
    }
};
}
