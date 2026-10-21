// Inside civ5::ui AFTER complete Name/Input/Context constructor and common
// ResizeRegistration types. Real86AC20 registration is independent of Lua61.
struct detail::NotificationContextResizeRegistrationReturn::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    const std::weak_ptr<const void> core,manager,context;
    const std::shared_ptr<const void> name_storage;
    const std::shared_ptr<const NotificationContextLuaStatePreparation> prepared;
    const std::shared_ptr<NotificationLayoutResizeRegistration> entry;
    const std::uint64_t serial,source_scope,gui_sequence;
    Impl(std::weak_ptr<const void> c,std::weak_ptr<const void> g,std::weak_ptr<const void> state,
      std::shared_ptr<const void> name,std::shared_ptr<const NotificationContextLuaStatePreparation> vm,
      std::shared_ptr<NotificationLayoutResizeRegistration> registered,
      std::uint64_t s,std::uint64_t scope,std::uint64_t sequence)
      :core(std::move(c)),manager(std::move(g)),context(std::move(state)),name_storage(std::move(name)),
       prepared(std::move(vm)),entry(std::move(registered)),serial(s),source_scope(scope),gui_sequence(sequence){}
};
detail::NotificationContextResizeRegistrationReturn::NotificationContextResizeRegistrationReturn(std::shared_ptr<const Impl> p)
    :impl_(std::move(p)){}
detail::NotificationContextResizeRegistrationReturn::~NotificationContextResizeRegistrationReturn()=default;

void detail::NotificationLayoutResizeRegistration::dispatch_context_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op)const {
    require(role==Role::context_load_668750&&creator==std::this_thread::get_id()&&!failed.load(),
        "Context resize callback foreign/failed/role FIRST");
    const auto actual=core.lock();const auto gui=manager.lock();const auto state=context_receiver.lock();
    const auto ctor=context_constructor.lock();
    require(actual&&gui&&state&&ctor,"Context resize actual receiver/constructor retired");
    actual->layout_origin();
    const auto source=actual->source;require(source!=nullptr,"Context resize actual Source retired FIRST");
    const auto scope=source->capture_active_operation_scope_serial_without_callbacks(&op);
    source->require_operation_origin_without_callbacks(op,scope);op.require_origin_without_callbacks();
    require(actual->context_ordinary_state.get()==state.get()&&
        !actual->context_ordinary_state.owner_before(state)&&!state.owner_before(actual->context_ordinary_state)&&
        actual->context_gui_port.get()==gui.get()&&!actual->context_gui_port.owner_before(gui)&&
        !gui.owner_before(actual->context_gui_port),"Context resize physical receiver/Gui changed");
    gui->impl_->first(op);
    require(std::any_of(gui->impl_->resize_registrations.begin(),gui->impl_->resize_registrations.end(),
        [&](const auto& entry){return entry.get()==this;}),"Context resize callback outside actual dispatcher");
    // Registration really completed; later event0 execution needs the genuine
    // Context v98/init/Parent lender. Existing parser98 authority is distinct.
    require(false,"required Context668750 event0/v98 lifecycle supplier before size/anchoring effects");
}

std::shared_ptr<const detail::NotificationContextResizeRegistrationReturn>
detail::NotificationContextLoadNameLoan::register_context_resize_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op,
    const std::shared_ptr<const NotificationContextLuaStatePreparation>& prepared_input)const {
    using Return=NotificationContextResizeRegistrationReturn;using Registration=NotificationLayoutResizeRegistration;
    const auto prepared=prepared_input;const auto saved=impl_;
    require(saved!=nullptr,"Context86AC20 real Name lender absent");
    require_current_without_callbacks(op,prepared);
    const auto actual=saved->input->core.lock();const auto state=saved->input->context.lock();
    const auto constructor=saved->input->constructor;const auto manager=actual?actual->context_gui_port:nullptr;
    require(actual&&state&&constructor&&manager,"Context86AC20 actual physical owners absent");
    const auto gui=manager->impl_;const auto storage=state->impl_;
    require(gui&&storage&&saved->resize_return.expired()&&saved->resize_entry.expired()&&!saved->resize_registering,
        "Context86AC20 same named load registration replay/reentered");
    const auto core_sequence=actual->context_layout_baseline->sequence_,effects=actual->effect_revision;
    const auto flags=actual->context_layout_baseline->base_.flags_40;
    const auto size=actual->context_layout_baseline->base_.size_30_34;
    std::uint64_t revision=gui->revision;std::size_t history=gui->history.size(),entries=gui->resize_registrations.size();
    const auto pure=[&](bool entered){
        require_current_without_callbacks(op,prepared);gui->first(op);
        require(saved->resize_registering==entered&&state->impl_.get()==storage.get()&&
            !state->impl_.owner_before(storage)&&!storage.owner_before(state->impl_)&&
            actual->context_ordinary_state.get()==state.get()&&!actual->context_ordinary_state.owner_before(state)&&
            !state.owner_before(actual->context_ordinary_state)&&actual->context_gui_port.get()==manager.get()&&
            !actual->context_gui_port.owner_before(manager)&&!manager.owner_before(actual->context_gui_port)&&
            manager->impl_.get()==gui.get()&&!manager->impl_.owner_before(gui)&&!gui.owner_before(manager->impl_)&&
            actual->context_layout_baseline->sequence_==core_sequence&&actual->effect_revision==effects&&
            actual->context_layout_baseline->base_.flags_40==flags&&
            actual->context_layout_baseline->base_.size_30_34==size&&
            gui->revision==revision&&gui->history.size()==history&&gui->resize_registrations.size()==entries,
            "Context86AC20 physical Context/Gui/control/journal changed across observer/allocation");
        require((flags&0x300000U)==0x300000U&&constructor->impl_&&constructor->impl_->alive.load()&&
            !constructor->impl_->failed.load()&&storage->native_constructor_return.get()==constructor.get()&&
            !storage->native_constructor_return.owner_before(constructor)&&!constructor.owner_before(storage->native_constructor_return),
            "Context86AC20 genuine Full dimensions/actual completed ctor absent");
        constructor->impl_->port->require_completed();
        const auto dimensions=manager->dimensions_without_callbacks(op);
        for(std::size_t axis=0;axis<2;++axis)
            require(std::bit_cast<std::uint32_t>(size[axis])==
                std::bit_cast<std::uint32_t>(static_cast<float>(dimensions[axis])),
                "Context86AC20 actual4572F1 dimensions changed");
    };
    pure(false);require(revision<UINT64_MAX&&history<gui->history.max_size()&&entries<gui->resize_registrations.max_size(),
        "Context86AC20 dispatcher/history exhausted");
    saved->resize_registering=true;
    struct Exit {std::shared_ptr<Impl> saved;~Exit(){saved->resize_registering=false;}}exit{saved};
    const auto full=[&]{pure(true);require_current_owned(op,prepared);pure(true);};
    try {
        full();gui->resize_registrations.reserve(entries+1);full();gui->history.reserve(history+1);full();
        // Snapshot every constructor operand by VALUE before the allocation.
        const auto ctor_serial=constructor->impl_->serial,serial=saved->input->serial,source_scope=saved->input->source_scope;
        auto entry=std::shared_ptr<Registration>(new Registration(actual,manager,state,constructor,
            ctor_serial,serial,flags));full();
        const auto data=std::make_shared<const Return::Impl>(actual,manager,state,saved,prepared,entry,
            serial,source_scope,revision+1);full();
        auto returned=std::shared_ptr<const Return>(new Return(data));full();
        require(gui->resize_registrations.capacity()>entries&&gui->history.capacity()>history,
            "Context86AC20 reserved dispatcher/history capacity changed");
        // Nonalloc real ordered registry publication, matching8A7040 insertion.
        gui->resize_registrations.push_back(entry);
        gui->history.push_back({revision+1,serial,NotificationContextGuiPort::Impl::Store::resize_registration});
        ++gui->revision;++revision;++history;++entries;
        saved->resize_entry=entry;saved->resize_return=returned;full();
        saved->resize_registering=false;pure(false);
        require_context_resize_owned(op,prepared,returned);return returned;
    }catch(...){saved->failed.store(true);actual->failed.store(true);gui->failed.store(true);throw;}
}

void detail::NotificationContextLoadNameLoan::require_context_resize_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op,
    const std::shared_ptr<const NotificationContextLuaStatePreparation>& prepared_input,
    const std::shared_ptr<const NotificationContextResizeRegistrationReturn>& returned_input)const {
    const auto prepared=prepared_input;const auto returned=returned_input;const auto saved=impl_;
    require(saved&&returned&&returned->impl_,"Context86AC20 actual completed registry return absent");
    const auto data=returned->impl_;
    const auto pure=[&]{
        require_current_without_callbacks(op,prepared);
        const auto actual=saved->input->core.lock();const auto state=saved->input->context.lock();
        const auto manager=actual?actual->context_gui_port:nullptr;
        require(actual&&state&&manager&&data->creator==std::this_thread::get_id()&&
            !saved->resize_registering&&saved->resize_return.lock().get()==returned.get()&&
            saved->resize_entry.lock().get()==data->entry.get()&&
            !data->core.expired()&&!data->manager.expired()&&!data->context.expired()&&
            !data->core.owner_before(actual)&&!actual.owner_before(data->core)&&
            !data->manager.owner_before(manager)&&!manager.owner_before(data->manager)&&
            !data->context.owner_before(state)&&!state.owner_before(data->context)&&
            data->name_storage.get()==saved.get()&&!data->name_storage.owner_before(saved)&&!saved.owner_before(data->name_storage)&&
            data->prepared.get()==prepared.get()&&!data->prepared.owner_before(prepared)&&!prepared.owner_before(data->prepared)&&
            data->serial==saved->input->serial&&data->source_scope==saved->input->source_scope&&data->entry&&
            data->entry->role==NotificationLayoutResizeRegistration::Role::context_load_668750&&
            !data->entry->failed.load()&&data->entry->core.lock().get()==actual.get()&&
            data->entry->context_receiver.lock().get()==state.get()&&data->entry->manager.lock().get()==manager.get(),
            "Context86AC20 SAME private completion/control/input/518 changed");
        const auto gui=manager->impl_;gui->first(op);
        require(std::any_of(gui->resize_registrations.begin(),gui->resize_registrations.end(),
            [&](const auto& entry){return entry.get()==data->entry.get()&&!entry.owner_before(data->entry)&&!data->entry.owner_before(entry);}),
            "Context86AC20 completed receiver absent from actual ordered dispatcher");
        require(std::any_of(gui->history.begin(),gui->history.end(),[&](const auto& event){
            return event.sequence==data->gui_sequence&&event.source_serial==data->serial&&
                event.kind==NotificationContextGuiPort::Impl::Store::resize_registration;}),
            "Context86AC20 actual registry store history absent");
    };
    try{pure();require_current_owned(op,prepared);pure();}
    catch(...){saved->failed.store(true);if(const auto c=saved->input->core.lock())c->failed.store(true);throw;}
}
