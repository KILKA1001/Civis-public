// Inside civ5::ui AFTER complete Name/Input/Context/constructor types.
// No changes to the existing COLD NameLoan/Input first or its public surface.
struct detail::NotificationContextRootReadB4Guard::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};
    const std::shared_ptr<NotificationContextLoadNameLoan::Impl> name;
    const std::shared_ptr<NotificationContextNativeLoadNameInputLoan::Impl> input;
    const std::shared_ptr<NotificationLayoutContextOrdinaryState::Impl> storage;
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
    const std::weak_ptr<NotificationLayoutContextOrdinaryState> context;
    const std::shared_ptr<const NotificationContextNativeConstructorReturn> constructor;
    const std::shared_ptr<const NotificationContextNativeConstructorPrefix> prefix;
    const std::shared_ptr<const NotificationContextNativeWideStringValue> value;
    const std::shared_ptr<const NotificationContextLuaStatePreparation> preparation;
    const std::shared_ptr<const NotificationContextNativeWideStringValue>*const field;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t revision,serial,source_scope;
    Impl(std::shared_ptr<NotificationContextLoadNameLoan::Impl> n,
        std::shared_ptr<NotificationContextNativeLoadNameInputLoan::Impl> i,
        std::shared_ptr<NotificationLayoutContextOrdinaryState::Impl> s,
        std::weak_ptr<BnwWorldViewControlTree::Core> c,
        std::weak_ptr<runtime::BnwSelectedWorldViewContextOwner> o,
        std::weak_ptr<NotificationLayoutContextOrdinaryState> state,
        std::shared_ptr<const NotificationContextNativeConstructorReturn> ctor,
        std::shared_ptr<const NotificationContextNativeConstructorPrefix> before,
        std::shared_ptr<const NotificationContextNativeWideStringValue> v,
        std::shared_ptr<const NotificationContextLuaStatePreparation> p,
        const std::shared_ptr<const NotificationContextNativeWideStringValue>*f,
        const runtime::BnwWorldViewSourceOperationLoan*op,
        std::uint64_t r,std::uint64_t current_serial,std::uint64_t current_source_scope)
        :name(std::move(n)),input(std::move(i)),storage(std::move(s)),core(std::move(c)),source(std::move(o)),
         context(std::move(state)),constructor(std::move(ctor)),prefix(std::move(before)),value(std::move(v)),
         preparation(std::move(p)),field(f),operation(op),revision(r),serial(current_serial),source_scope(current_source_scope){}
    void first_data(const runtime::BnwWorldViewSourceOperationLoan& op)const{
        require(creator==std::this_thread::get_id()&&alive.load()&&!failed.load(),
            "RootReadB4 own Guard retired/foreign/failed FIRST");
        require(name&&name->creator==std::this_thread::get_id()&&name->alive.load()&&!name->failed.load(),
            "RootReadB4 NameLoan retired/failed FIRST");
        require(input&&input->creator==std::this_thread::get_id()&&input->alive.load()&&!input->failed.load(),
            "RootReadB4 input lexical life retired/failed FIRST");
        require(storage&&storage->creator==std::this_thread::get_id()&&storage->alive.load()&&!storage->failed.load(),
            "RootReadB4 physical Context retired/failed FIRST");
        //Only compare the offered address; do not call a raw op method here.
        require(operation==&op&&input->operation==&op&&input->serial==serial&&serial&&source_scope,
            "RootReadB4 SAME captured Source operand/serial metadata changed");
        const auto actual=core.lock();const auto owner=source.lock();const auto state=context.lock();
        require(actual&&owner&&state,"RootReadB4 actual Core/Source/Context facade retired");
        require(actual->creator==std::this_thread::get_id()&&actual->facade_alive.load()&&!actual->failed.load(),
            "RootReadB4 actual Core creator/facade/failed FIRST");
        require(NotificationLayoutContextOrdinaryState::Impl::same(actual->source,owner)&&
            NotificationLayoutContextOrdinaryState::Impl::same(actual->context_ordinary_state,state)&&
            NotificationLayoutContextOrdinaryState::Impl::same(state->impl_,storage),
            "RootReadB4 SAME retained Context/Core/Source controls changed");
        //Existing pure Source fence compares its actual active pointer/scope.
        //It never dereferences op, unlike current_operation_loan(). ZERO op calls.
        owner->require_operation_origin_without_callbacks(op,source_scope);
        const auto active=actual->context_load_name_frame.lock();const auto canonical=input->canonical_input.lock();
        require(NotificationLayoutContextOrdinaryState::Impl::same(name->input,input)&&canonical&&
            NotificationLayoutContextOrdinaryState::Impl::same(canonical,input)&&active&&
            NotificationLayoutContextOrdinaryState::Impl::same(active,input)&&
            NotificationLayoutContextOrdinaryState::Impl::same(input->core.lock(),actual)&&
            NotificationLayoutContextOrdinaryState::Impl::same(input->source.lock(),owner)&&
            NotificationLayoutContextOrdinaryState::Impl::same(input->context.lock(),state)&&
            field==&storage->b4_value&&field==name->field&&revision==name->revision&&
            input->named_store_revision==revision&&storage->b4_revision==revision&&
            NotificationLayoutContextOrdinaryState::Impl::same(storage->b4_value,value)&&
            NotificationLayoutContextOrdinaryState::Impl::same(name->value,value)&&
            NotificationLayoutContextOrdinaryState::Impl::same(name->storage,storage)&&
            preparation&&NotificationLayoutContextOrdinaryState::Impl::same(actual->context_lua_preparation,preparation),
            "RootReadB4 current FIELD/VALUE/revision/518/input lexical control changed");
        require(constructor&&constructor->impl_&&prefix&&prefix->impl_,"RootReadB4 actual constructor absent");
        const auto done=constructor->impl_;const auto before=prefix->impl_;
        require(done->creator==std::this_thread::get_id()&&done->alive.load()&&!done->failed.load()&&
            before->creator==std::this_thread::get_id()&&before->alive.load()&&!before->failed.load(),
            "RootReadB4 actual constructor/prefix retired/failed FIRST");
        require(NotificationLayoutContextOrdinaryState::Impl::same(storage->native_constructor_return,constructor)&&
            NotificationLayoutContextOrdinaryState::Impl::same(input->constructor,constructor)&&
            NotificationLayoutContextOrdinaryState::Impl::same(done->core.lock(),actual)&&
            NotificationLayoutContextOrdinaryState::Impl::same(done->source.lock(),owner)&&
            NotificationLayoutContextOrdinaryState::Impl::same(done->projection.lock(),state)&&
            NotificationLayoutContextOrdinaryState::Impl::same(done->prefix,prefix)&&
            NotificationLayoutContextOrdinaryState::Impl::same(storage->native_constructor_prefix,prefix)&&
            NotificationLayoutContextOrdinaryState::Impl::same(before->core.lock(),actual)&&
            NotificationLayoutContextOrdinaryState::Impl::same(before->source.lock(),owner)&&
            NotificationLayoutContextOrdinaryState::Impl::same(before->projection.lock(),state)&&
            done->serial&&done->serial==before->serial&&done->port&&before->port,
            "RootReadB4 constructor canonical controls/physical journals changed");
        before->port->require_reached();done->port->require_completed();
        //No Core.layout_origin(false), no busy bool, no RootRead/Source renewal.
    }
};
detail::NotificationContextRootReadB4Guard::NotificationContextRootReadB4Guard(std::shared_ptr<Impl> p):impl_(std::move(p)){}
void detail::NotificationContextRootReadB4Guard::retire_owned()const noexcept{impl_->alive.store(false);}
detail::NotificationContextRootReadB4Guard::~NotificationContextRootReadB4Guard(){retire_owned();}
void detail::NotificationContextRootReadB4Guard::require_data_current_without_callbacks(
    const runtime::BnwWorldViewSourceOperationLoan& op)const{impl_->first_data(op);}

std::unique_ptr<const detail::NotificationContextRootReadB4Guard>
BnwWorldViewControlTree::capture_context_root_read_b4_guard_owned(
    const runtime::BnwWorldViewSourceOperationLoan& op,
    const std::shared_ptr<const detail::NotificationContextLoadNameLoan>& loan,
    const std::shared_ptr<const detail::NotificationContextLuaStatePreparation>& actual_prepared){
    using Guard=detail::NotificationContextRootReadB4Guard;
    const auto actual=core_;
    require(actual&&actual->creator==std::this_thread::get_id()&&actual->facade_alive.load()&&
        !actual->failed.load()&&!actual->busy,"RootReadB4 capture needs COLD actual Core FIRST");
    require(loan&&loan->impl_,"RootReadB4 capture genuine NameLoan absent");
    const auto actual_loan=loan;const auto name=actual_loan->impl_;
    require(name->creator==std::this_thread::get_id()&&name->alive.load()&&!name->failed.load()&&name->input,
        "RootReadB4 capture NameLoan retired/failed FIRST");const auto input=name->input;
    require(input->creator==std::this_thread::get_id()&&input->alive.load()&&!input->failed.load()&&
        input->operation==&op,"RootReadB4 capture input lexical borrower retired/substituted FIRST");
    const auto storage=name->storage;
    require(storage&&storage->creator==std::this_thread::get_id()&&storage->alive.load()&&!storage->failed.load(),
        "RootReadB4 capture physical Context retired/failed FIRST");
    const auto owner=input->source.lock();const auto context=input->context.lock();
    require(owner&&context&&detail::NotificationLayoutContextOrdinaryState::Impl::same(actual->source,owner)&&
        detail::NotificationLayoutContextOrdinaryState::Impl::same(actual->context_ordinary_state,context)&&
        detail::NotificationLayoutContextOrdinaryState::Impl::same(input->core.lock(),actual),
        "RootReadB4 capture foreign actual Core/Source/Context");
    //Pointer FIRST before the old cold checker can call op.operation_serial().
    const auto source_scope=owner->capture_active_operation_scope_serial_without_callbacks(&op);
    const auto prepared=actual_prepared;
    const auto current=[&]{
        require(actual->creator==std::this_thread::get_id()&&actual->facade_alive.load()&&!actual->failed.load()&&!actual->busy,
            "RootReadB4 capture cold Core changed FIRST");
        require(input->creator==std::this_thread::get_id()&&input->alive.load()&&!input->failed.load()&&input->operation==&op,
            "RootReadB4 capture input lifetime changed FIRST");
        owner->require_operation_origin_without_callbacks(op,source_scope);
        actual_loan->require_current_without_callbacks(op,prepared);
    };
    const auto full=[&]{current();op.require_current();current();};
    try{
        full();const auto value=name->value;const auto field=name->field;const auto revision=name->revision;
        const auto serial=input->serial;const auto constructor=input->constructor;const auto prefix=constructor->impl_->prefix;
        //ALL operands are retained VALUES before allocations/RootRead entry.
        auto data=std::make_shared<Guard::Impl>(name,input,storage,actual,owner,context,constructor,prefix,value,
            prepared,field,&op,revision,serial,source_scope);full();
        auto result=std::unique_ptr<const Guard>(new Guard(data));full();
        result->require_data_current_without_callbacks(op);return result;
    }catch(...){name->failed.store(true);throw;}
}
