// Included inside civ5::ui AFTER the complete actual WorldViewGridProvider.
std::shared_ptr<const UiScreenGlRenderer::FontTextureCacheEntry>
UiScreenGlRenderer::retain_font_texture_payload_owned(
    const BnwWorldViewRendererProjectionLoan& projection,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const BnwOwnedCameraGuiScreen::Loan& gui,
    const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,
    const runtime::detail::BnwFontResourceLoan& resource,
    const detail::BnwFontTextureRequest& request){
    using Metadata=graphics::detail::BnwFontTextureBorrower::Impl;
    const auto life=worldview_lifetime_;
    const auto renderer_first=[&]{
        if(life->creator!=std::this_thread::get_id()||!life->alive.load()||life->failed.load()||
           life->renderer!=this||!life->active.load()||life->active_scope)
            throw std::runtime_error("Font payload requires actual pre-paint renderer/Source scope");
    };
    renderer_first();
    const auto cache=font_texture_cache_;
    if(!cache||cache->creator!=std::this_thread::get_id()||!cache->alive.load()||cache->failed.load()||
       cache->renderer!=this||cache->renderer_life.owner_before(life)||life.owner_before(cache->renderer_life))
        throw std::runtime_error("Font actual cache storage retired/substituted");
    if(cache->active.exchange(true)){
        life->failed.store(true);throw std::runtime_error("Font texture receiver reentered");
    }
    struct End {std::shared_ptr<FontTextureCacheStorage> storage;~End(){
        storage->active.store(false);storage->cleanup_if_quiescent_owned();}} end{cache};
    try{
        request.require_pending_query_owned(operation,resource);renderer_first();
        const auto occurrence=request.texture_occurrence_owned(operation,resource);renderer_first();
        const auto request_life=request.texture_lifetime_owned(operation,resource);renderer_first();
        const auto resource_life=std::weak_ptr(resource.life_);
        const auto resource_epoch=resource.life_->epoch.load();
        const auto serial=operation.operation_serial();renderer_first();
        const auto first=[&]{
            renderer_first();
            if(!cache->alive.load()||cache->failed.load()||cache->creator!=std::this_thread::get_id())
                throw std::runtime_error("Font saved cache owner retired during query");
            const auto rq=request_life.lock();const auto rl=resource_life.lock();
            if(!rq||!rl)throw std::runtime_error("Font payload request/resource fence retired");
            rq->require_current_owned(occurrence);rl->first(resource_epoch);
        };
        const auto pure=[&]{
            first();projection.require_current_without_callbacks();first();
            if(projection.renderer_!=this||projection.lifetime_.get()!=life.get()||
               projection.lifetime_.owner_before(life)||life.owner_before(projection.lifetime_))
                throw std::runtime_error("Font payload actual renderer control substituted");
        };
        const auto full=[&]{
            pure();operation.require_operation(serial);first();pure();
            request.require_pending_query_owned(operation,resource);first();pure();
            resource.require_current_owned(operation);first();pure();
            gui.require_current();first();pure();window.require_current();first();pure();
        };
        full();
        // The genuine pending-row issuer owns base/index derivation and BC.
        // Both roles use the SAME exact staged key, fixed query20000/extra0,
        // cache owner and DDS upload; this backend never derives another key.
        (void)request.optional_index_owned(operation,resource);first();full();
        const auto source_key=std::string(request.source_owned(operation,resource));first();full();
        const assets::AssetKey key(source_key);first();full();
        auto bank=resource.bank_owner_owned(operation);first();full();
        const auto& capability=resource.bank_capability_owned(operation);first();full();
        if(!bank||&bank->resources(capability)!=&resources_)
            throw std::runtime_error("Font texture table is not SAME canonical accepted Bank");
        first();
        const auto source=resource.source_;
        const auto generation=resource.generation_;
        const auto scene_origin=projection.scene_;
        std::weak_ptr<const SelectedBnwPreviewHost> host_origin;
        {
            const auto scene=scene_origin.lock();
            if(!scene||!scene->selected_host_)throw std::runtime_error("Font actual Scene/Host retired");
            const auto host=scene->selected_host_->retain_civis_resource_host_owned();
            first();
            if(host.get()!=scene->selected_host_.get()||host.owner_before(scene->selected_host_)||
               scene->selected_host_.owner_before(host))
                throw std::runtime_error("Font actual Host canonical control replaced");
            host->require_same_civis_resources(*bank);first();host_origin=host;
        }
        first();full();
        const auto native=context_.native_identity();first();
        const auto hwnd=window.window();first();pure();
        const auto dc=window.dc();first();pure();
        const auto wgl=window.wgl_context();first();pure();
        const std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window_origin=window.retain_owner();
        first();pure();
        const auto window_occurrence=window.window_occurrence();first();pure();
        if(!native.owner_thread||!native.context||native.window!=hwnd||native.dc!=dc||native.context!=wgl)
            throw std::runtime_error("Font texture actual GL/window identity changed");
        full();context_.check("Before owned Font DDS payload");first();full();
        // Pure staged capability query. Missing bytes are supplier STOP, not
        // native9AF940 NULL and not a whole-Font false return.
        auto loaded=capability.loaded(key);first();full();
        if(!loaded.bytes)throw std::runtime_error("Font texture exact winner is not genuinely staged");
        const auto same=[](const auto& a,const auto& b){return !a.owner_before(b)&&!b.owner_before(a);};
        const auto entry_current=[&](const std::shared_ptr<const FontTextureCacheEntry>& entry){
            first();entry->metadata->first();first();
            const auto& state=*entry->metadata;
            if(!entry->texture||!entry->texture->belongs_to(context_)||state.key!=source_key||
               state.generation!=generation||state.bank.get()!=bank.get()||!same(state.bank,bank)||
               !same(state.source,source)||!same(state.scene,scene_origin)||!same(state.host,host_origin)||
               !same(state.window,window_origin)||state.window_occurrence!=window_occurrence||
               state.loaded.bytes.get()!=loaded.bytes.get()||!same(state.loaded.bytes,loaded.bytes))
                throw std::runtime_error("Font cache payload source/GL/epoch replaced");
            const auto at=cache->entries.find(key.normalized_name());
            if(at==cache->entries.end()||at->second.get()!=entry.get()||!same(at->second,entry))
                throw std::runtime_error("Font cache outside canonical payload owner replaced");
            first();
        };
        if(const auto old=cache->entries.find(key.normalized_name());old!=cache->entries.end()){
            const auto result=old->second;entry_current(result);full();entry_current(result);return result;
        }
        // Own raw DDS storage/upload prefix only. Explicit legacy D3D9 source
        // profile; linear UNORM storage is NOT a claim about20000 sampler bits.
        auto dds=assets::DdsData::parse(*loaded.bytes,assets::NumericDdsProfile::legacy_d3d9,
            assets::DdsHeaderProfile::civ5_d3dx_size24);first();full();
        if(dds.face_count()!=1)throw std::runtime_error("Font texture non-2D source is unsupported");
        auto texture=std::make_unique<graphics::GlTexture>(context_,dds,graphics::DdsSamplingColor::linear);
        first();full();
        if(!texture->id()||!texture->belongs_to(context_))
            throw std::runtime_error("Font DDS upload did not return an actual owned texture");
        if(cache->revision==UINT64_MAX)throw std::runtime_error("Font texture cache revision exhausted");
        const auto revision=cache->revision+1;
        auto metadata=std::make_shared<Metadata>(life,this,bank,loaded,source,
            scene_origin,host_origin,window_origin,window_occurrence,generation,revision,source_key);first();full();
        auto result=std::make_shared<const FontTextureCacheEntry>(metadata,std::move(texture));first();full();
        metadata->cache_payload=result;metadata->payload_identity=result.get();
        full(); // Last observer before genuine cache publication.
        const auto added=cache->entries.emplace(key.normalized_name(),result);first();
        if(!added.second)throw std::runtime_error("Font cache publication replaced during same-thread capture");
        cache->revision=revision;entry_current(result);full();entry_current(result);return result;
    }catch(...){cache->failed.store(true);life->failed.store(true);throw;}
}
std::unique_ptr<graphics::detail::BnwFontTextureObservation>
UiScreenGlRenderer::WorldViewGridProvider::query_font_texture_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const BnwOwnedCameraGuiScreen::Loan& gui,
    const graphics::BnwOwnedCameraRendererWindowLease::Loan& window,
    const runtime::detail::BnwFontResourceLoan& resource,
    const detail::BnwFontTextureRequest& request)const{
    const auto life=life_;require_for_source(operation);
    if(life->creator!=std::this_thread::get_id()||!life->alive.load()||life->failed.load())
        throw std::runtime_error("Font Grid receiver actual renderer retired");
    auto* const renderer=const_cast<UiScreenGlRenderer*>(projection_.renderer_);
    if(!renderer||life->renderer!=renderer)throw std::runtime_error("Font Grid actual renderer replaced");
    const auto payload=renderer->retain_font_texture_payload_owned(projection_,operation,gui,window,resource,request);
    if(life->creator!=std::this_thread::get_id()||!life->alive.load()||life->failed.load())
        throw std::runtime_error("Font Grid receiver retired after actual DDS upload");
    require_for_source(operation);payload->metadata->first();
    const auto first=[&]{
        if(life->creator!=std::this_thread::get_id()||!life->alive.load()||life->failed.load())
            throw std::runtime_error("Font actual renderer retired before query publication");
        payload->metadata->first();
    };
    const auto full=[&]{
        first();require_for_source(operation);first();
        request.require_pending_query_owned(operation,resource);first();
    };
    full();
    const auto occurrence=request.texture_occurrence_owned(operation,resource);first();
    const auto request_life=request.texture_lifetime_owned(operation,resource);first();
    const auto resource_life=std::weak_ptr(resource.life_);const auto epoch=resource.life_->epoch.load();
    const auto serial=operation.operation_serial();first();full();
    auto borrower=std::shared_ptr<const graphics::detail::BnwFontTextureBorrower>(
        new graphics::detail::BnwFontTextureBorrower(payload->metadata));first();full();
    auto state=std::make_unique<graphics::detail::BnwFontTextureObservation::Impl>(request_life,resource_life,
        occurrence,epoch,serial,&request,&resource,std::move(borrower));first();full();
    auto result=std::unique_ptr<graphics::detail::BnwFontTextureObservation>(
        new graphics::detail::BnwFontTextureObservation(std::move(state)));first();full();
    result->require_current_owned(operation,resource,request);first();full();
    // Genuine own-Civis GL payload only, fixed native request20000/extra0.
    // Linear raw DDS storage is explicit backend policy, NOT sampling parity.
    // There is no observed-NULL factory or warning normalization in this domain.
    return result;
}
