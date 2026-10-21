// Included after canonical GuiPort Impl/methods, inside civ5::ui.
std::unique_ptr<detail::BnwCompletedFontLoan> detail::NotificationContextGuiPort::begin_font_lookup_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,std::optional<std::string_view> requested){
    const auto saved=impl_;saved->first(operation);
    require(bool(saved->font_registry),"Font96CC actual constructor stage missing");
    const auto registry=saved->font_registry;registry->first();
    require(!saved->busy,"Font96CC lookup reentered");saved->busy=true;
    struct End{std::shared_ptr<Impl> p;~End(){p->busy=false;}}end{saved};
    try{
        const auto pure=[&]{saved->first(operation,true);registry->first();
            require(saved->font_registry.get()==registry.get()&&
                !registry.owner_before(saved->font_registry)&&!saved->font_registry.owner_before(registry),
                "Font96CC canonical field owner replaced");};
        pure();operation.require_current();pure();
        const auto serial=operation.operation_serial();
        // Copy caller C-string before any subsequent Source callback.
        std::optional<std::string> pinned_request;
        if(requested){const auto nul=requested->find('\0');const auto count=std::min(nul,requested->size());
            std::string owned;owned.resize(count);pure();operation.require_operation(serial);pure();
            std::copy_n(requested->begin(),count,owned.begin());pure();operation.require_operation(serial);pure();
            pinned_request.emplace(std::move(owned));}
        pure();operation.require_operation(serial);pure();
        const auto key=detail::bnw_native_font_registry_key(pinned_request?
            std::optional<std::string_view>{*pinned_request}:std::nullopt);
        pure();operation.require_operation(serial);pure();
        // Original59EBD0 first inserts a NULL cell BEFORE allocation. Reserve
        // bookkeeping first so an earned map insertion cannot lose history.
        pure();registry->history.reserve(registry->history.size()+3);pure();operation.require_operation(serial);pure();
        registry->pending.reserve(registry->pending.size()+1);pure();operation.require_operation(serial);pure();
        // Private allocation precedes final cut. Equal-allocator node transfer
        // performs the actual NULL-cell insertion without allocator callbacks.
        auto slot=registry->map_96cc.find(key);bool inserted=false;
        if(slot==registry->map_96cc.end()){
            decltype(registry->map_96cc) staged;
            staged.try_emplace(key);pure();operation.require_operation(serial);pure();
            auto node=staged.extract(key);pure();operation.require_operation(serial);pure();
            const auto result=registry->map_96cc.insert(std::move(node));
            slot=result.position;inserted=result.inserted;pure();
        }
        if(inserted)registry->history.push_back({detail::NotificationContextFontRegistryFields::Store::null_slot_inserted,key,serial});
        pure();
        if(slot->second.completed){
            const auto facade=saved->canonical_gui.lock();require(facade&&facade->impl_.get()==saved.get(),"Font completed lookup lost actual Gui");
            auto loan=std::unique_ptr<detail::BnwCompletedFontLoan>(new detail::BnwCompletedFontLoan(
                std::make_shared<detail::BnwCompletedFontLoan::Impl>(facade,saved,registry,key,operation)));
            pure();loan->require_current_without_callbacks();pure();
            operation.require_operation(serial);pure();loan->require_current_without_callbacks();pure();return loan;
        }
        // Default initialization leaves scalar fields unassigned; the
        // subsequent owned ctor writes earn them, not value-initialization.
        auto font=std::unique_ptr<detail::NotificationContextFontRegistryFields::FontObject>(
            new detail::NotificationContextFontRegistryFields::FontObject);
        pure();operation.require_operation(serial);pure();
        const auto pending_index=registry->pending.size();
        const auto font_allocation=font.get();
        const std::weak_ptr<detail::NotificationContextFontRegistryFields::FontObject::Life> font_life=font->life;
        const auto font_epoch=font->life->epoch.load();
        const auto pending_font_cut=[&]{
            const auto first=font_life.lock();require(bool(first),"Font pending allocation fence expired");first->first(font_epoch);
            pure();operation.require_operation(serial);pure();first->first(font_epoch);
            require(pending_index<registry->pending.size()&&registry->pending[pending_index].key==key&&
                registry->pending[pending_index].source_serial==serial&&registry->pending[pending_index].font.get()==font_allocation,
                "Font pending actual owner changed across allocation/callback");
            const auto& owned=registry->pending[pending_index].font;
            require(owned->life.get()==first.get()&&!owned->life.owner_before(first)&&!first.owner_before(owned->life),
                "Font pending independent life/control substituted");
        };
        registry->pending.push_back({key,serial,std::move(font)});pending_font_cut();
        registry->pending[pending_index].font->constructor_history.reserve(10);pending_font_cut();
        registry->history.push_back({detail::NotificationContextFontRegistryFields::Store::font_allocated,key,serial});
        registry->pending.back().font->initialize_owned();
        registry->history.push_back({detail::NotificationContextFontRegistryFields::Store::font_constructor_returned,key,serial});
        pure();operation.require_operation(serial);pure();
        // Missing supplier is not a false native load. Retain the NULL slot
        // and real pending constructor prefix, poison this failed occurrence.
        // No successfulNULL, negative cache, rollback or fake positive Font.
        {
            // Real portable copy of the Original request FString, kept through
            // the loader and destroyed before registry map publication.
            pending_font_cut();const auto copied_request=pinned_request;pending_font_cut();
            require(copied_request==pinned_request,"Font actual copied request changed");
            #include "notification_font_scalar_load_prefix.inc"
        }
        pure();operation.require_operation(serial);pure();
        pending_font_cut();
        require(registry->pending.back().font&&registry->pending.back().font->completion&&
            !slot->second.completed,"Font publication lacks actual completed loader return");
        slot->second.completed=std::move(registry->pending.back().font);
        pure();operation.require_operation(serial);pure();
        const auto facade=saved->canonical_gui.lock();require(facade&&facade->impl_.get()==saved.get(),"Font publication lost actual Gui");
        auto loan=std::unique_ptr<detail::BnwCompletedFontLoan>(new detail::BnwCompletedFontLoan(
            std::make_shared<detail::BnwCompletedFontLoan::Impl>(facade,saved,registry,key,operation)));
        pure();loan->require_current_without_callbacks();pure();
            operation.require_operation(serial);pure();loan->require_current_without_callbacks();pure();return loan;
    }catch(...){registry->failed=true;saved->failed.store(true);throw;}
}
