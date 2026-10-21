// Inside civ5::ui, after the complete canonical GUI manager.
struct detail::BnwWindowedRendererSharedDescriptorIdentity::Impl final {
    using Gui=NotificationContextGuiPort;
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<Gui::Impl> gui;
    const std::weak_ptr<Gui> facade;
    // Actual703FA1 tuple; the first argument is descriptor index, not a
    // fabricated resource type. Native pool index is never used as PORT ID.
    const std::array<std::uint32_t,8> initializer_arguments{0x1au,240u,0u,15u,15u,0u,0u,0u};
    // Allocate the actual resource. Unwritten bytes are deliberately not
    // initialized or exposed as authoritative frame constant values.
    const std::unique_ptr<std::byte[]> storage;
    Impl(const std::shared_ptr<Gui::Impl>& owner,const std::shared_ptr<Gui>& actual)
        :gui(owner),facade(actual),storage(new std::byte[240]){}
    void first()const{
        require(creator==std::this_thread::get_id()&&storage!=nullptr,
            "Shared descriptor physical resource retired/foreign");
        const auto manager=gui.lock();const auto actual=facade.lock();
        require(manager&&actual&&manager->creator==creator&&manager->facade_alive.load()&&!manager->failed.load(),
            "Shared descriptor canonical GUI retired/failed");
        const auto canonical=manager->canonical_gui.lock();
        require(canonical&&canonical.get()==actual.get()&&!canonical.owner_before(actual)&&!actual.owner_before(canonical)&&
            actual->impl_.get()==manager.get()&&!actual->impl_.owner_before(manager)&&!manager.owner_before(actual->impl_)&&
            manager->windowed_shared_descriptor.get()==this&&
            !manager->source.expired()&&!manager->host.expired()&&!manager->world.expired()&&
            !manager->scene.expired()&&!manager->window.expired(),
            "Shared descriptor is not SAME live canonical GUI resource");
        require(initializer_arguments==std::array<std::uint32_t,8>{0x1au,240u,0u,15u,15u,0u,0u,0u},
            "Shared descriptor native initialization layout changed");
        // No manager.first, Source operation, HWND/WGL, GL or allocator.
    }
};
detail::BnwWindowedRendererSharedDescriptorIdentity::BnwWindowedRendererSharedDescriptorIdentity(std::shared_ptr<Impl> value)
    :impl_(std::move(value)){}
void detail::BnwWindowedRendererSharedDescriptorIdentity::require_current_without_callbacks()const{impl_->first();}
void detail::BnwWindowedRendererSharedDescriptorIdentity::require_same_owner_without_callbacks(
    const BnwWindowedRendererSharedDescriptorIdentity& other)const{
    const auto saved=impl_;const auto candidate=other.impl_;saved->first();candidate->first();
    require(saved.get()==candidate.get()&&!saved.owner_before(candidate)&&!candidate.owner_before(saved),
        "Shared descriptor association changed canonical physical owner");
}
