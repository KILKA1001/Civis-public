// PRIVATE, inside civ5::ui after complete Core, Source9 ctor and B4Guard2.
// Native7A9C30 is a DIRECT call from457481, not Context virtual1C.
// This real reached prefix ends before unavailable7493B0 path resource owner.
// No RootReadChildrenLoan, XML load, ControlsRef, factory, attach or completion.
struct BnwWorldViewControlTree::ContextRootReadFileFrame final {
    using State=detail::NotificationLayoutContextOrdinaryState;
    using Wide=detail::NotificationContextNativeWideStringValue;
    using Setup=detail::NotificationContextLuaLoadSetupReturn;
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true},failed{false};
    const std::weak_ptr<Core> core;
    const std::weak_ptr<State> state;
    const std::shared_ptr<const detail::NotificationContextRootReadB4Guard> name;
    const Setup::Guard setup;
    const std::shared_ptr<const detail::NotificationContextLuaStatePreparation> prepared;
    const std::shared_ptr<const detail::NotificationContextLoadNameLoan> actual_name;
    const std::shared_ptr<const Setup> actual_setup;
    const std::shared_ptr<const detail::NotificationContextNativeConstructorReturn> constructor;
    const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> receiver;
    const std::shared_ptr<const Core::LayoutReceiverOccurrence> occurrence;
    const std::shared_ptr<const Wide> actual_b4;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t source_serial,b4_revision;
    enum class Effect {copy_b4_to_510,append_xml};
    std::array<Effect,2> effects;
    std::size_t effect_count=0;
    std::shared_ptr<const Wide> expected_510;
    std::shared_ptr<ContextRootWatcherCustody> expected_watcher_b8;
    std::shared_ptr<ContextRootXml84> expected_xml84;
    enum class Cleanup {cursor34_8353f0,string18_retire,lua2c_8f8820,cursor10_8353f0,wide280_686f10,wide68_686f10};
    std::array<Cleanup,6> cleanup_effects;
    std::size_t cleanup_count=0;
    struct PathTemporary final {
        enum class Origin {selected_loose7493b0,watch_path7a0030};
        enum class Life {live,retired686f10};
        const std::thread::id creator=std::this_thread::get_id();
        const Origin origin;
        const std::shared_ptr<const assets::detail::CivisLoadedXmlPhysicalPathLoan> physical;
        const std::shared_ptr<const Wide> value;
        //4F13F0 initializes both caches to NULL. Selected loose4DC8C0 and
        //fixed2607A0030/48A0F0 copy buffer and clear both caches.407FB0 does
        //not assign either ref. This admitted qualified port has genuine NULL
        //branches in686F10; nonNULL refcount/destructor outcomes stay STOP.
        void*const reference214=nullptr;
        void*const reference20c=nullptr;
        Life life=Life::live;
        PathTemporary(Origin selected,std::shared_ptr<const assets::detail::CivisLoadedXmlPhysicalPathLoan> p,
            std::shared_ptr<const Wide> actual):origin(selected),physical(std::move(p)),value(std::move(actual)){}
        void require_fields_without_callbacks()const {
            require(creator==std::this_thread::get_id()&&physical&&value&&!reference214&&!reference20c&&
                !value->value_.empty()&&value->value_.size()<256&&
                std::find(value->value_.begin(),value->value_.end(),char16_t{})==value->value_.end(),
                "RootRead path temporary actual selected loose/fixed UTF16/caches changed");
            physical->require_current_without_callbacks();
        }
        void require_live_without_callbacks()const {
            require_fields_without_callbacks();require(life==Life::live,"RootRead live path temporary already retired");
        }
        void retire_686f10_without_callbacks(){
            require_live_without_callbacks();
            //Two actual686F10 NULL tests execute in original214→20C order.
            require(reference214==nullptr,"required real path214 shared-owner decrement/DTOR");
            require(reference20c==nullptr,"required real path20C shared-owner decrement/DTOR");
            life=Life::retired686f10;
        }
        void require_retired_without_callbacks()const {
            require_fields_without_callbacks();require(life==Life::retired686f10,"RootRead path temporary has no actual686F10 DTOR");
        }
    };
    std::shared_ptr<PathTemporary> temporary68,temporary280;
    bool entered=false;
    ContextRootReadFileFrame(std::shared_ptr<Core> c,std::shared_ptr<State> s,
        std::shared_ptr<const detail::NotificationContextRootReadB4Guard> n,Setup::Guard setup_guard,
        std::shared_ptr<const detail::NotificationContextLuaStatePreparation> p,
        std::shared_ptr<const detail::NotificationContextNativeConstructorReturn> ctor,
        std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver> r,
        std::shared_ptr<const Core::LayoutReceiverOccurrence> o,std::shared_ptr<const Wide> b4,
        std::shared_ptr<const Wide> before510,const runtime::BnwWorldViewSourceOperationLoan* op,
        std::uint64_t serial,std::uint64_t revision,
        std::shared_ptr<const detail::NotificationContextLoadNameLoan> named,std::shared_ptr<const Setup> setup_return)
        :core(std::move(c)),state(std::move(s)),name(std::move(n)),setup(std::move(setup_guard)),
         prepared(std::move(p)),actual_name(std::move(named)),actual_setup(std::move(setup_return)),constructor(std::move(ctor)),receiver(std::move(r)),occurrence(std::move(o)),
         actual_b4(std::move(b4)),operation(op),source_serial(serial),b4_revision(revision),expected_510(std::move(before510)){}
    void first()const {
        require(creator==std::this_thread::get_id()&&alive.load()&&!failed.load()&&entered,
            "RootRead file frame retired/foreign/failed/not entered FIRST");
        const auto actual=core.lock();const auto owned=state.lock();
        require(actual&&owned&&name&&prepared&&constructor&&constructor->impl_,
            "RootRead actual constructor/name/518 owner retired");
        // B4Guard validates actual Source pointer/scope without dereferencing old
        // operation. It is independent of cold layout_origin(false).
        name->require_data_current_without_callbacks(*operation);
        require(actual->creator==std::this_thread::get_id()&&actual->facade_alive.load()&&
            !actual->failed.load()&&actual->busy&&actual->context_root_read_file_frame.lock().get()==this,
            "RootRead SAME genuine entered Core frame changed");
        setup.require_current_without_callbacks(*operation,prepared);
        require(owned->impl_&&owned->impl_->creator==std::this_thread::get_id()&&
            owned->impl_->alive.load()&&!owned->impl_->failed.load()&&
            constructor->impl_->creator==std::this_thread::get_id()&&constructor->impl_->alive.load()&&
            !constructor->impl_->failed.load()&&constructor->impl_->port&&
            State::Impl::same(actual->context_ordinary_state,owned)&&
            State::Impl::same(owned->impl_->native_constructor_return,constructor)&&
            State::Impl::same(actual->context_ordinary_occurrence,occurrence)&&
            State::Impl::same(actual->context_lua_preparation,prepared)&&
            actual->context_layout_baseline&&
            State::Impl::same(actual->context_layout_baseline->base_.receiver_owned,receiver)&&
            State::Impl::same(owned->impl_->b4_value,actual_b4)&&owned->impl_->b4_revision==b4_revision&&
            State::Impl::same(owned->impl_->file_510_value,expected_510)&&
            State::Impl::same(owned->impl_->file_watcher_b8,expected_watcher_b8)&&
            State::Impl::same(owned->impl_->xml84_current,expected_xml84),
            "RootRead physical Context/constructor/B4/510/518 changed");
        require(cleanup_count<=cleanup_effects.size(),"RootRead cleanup count malformed");
        if(temporary280){if(cleanup_count>=5)temporary280->require_retired_without_callbacks();
            else temporary280->require_live_without_callbacks();}
        if(temporary68){if(cleanup_count>=6)temporary68->require_retired_without_callbacks();
            else temporary68->require_live_without_callbacks();}
        operation->require_origin_without_callbacks();
        require(source_serial&&source_serial==operation->operation_serial(),
            "RootRead current operation serial changed");
    }
    void full()const {first();operation->require_current();first();}
    void record(Effect value){require(effect_count<effects.size(),"RootRead file prefix replay");effects[effect_count++]=value;}
};

void BnwWorldViewControlTree::read_context_root_file_prefix_until_required_owner_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    std::shared_ptr<const detail::NotificationContextLoadNameLoan> actual_name,
    std::shared_ptr<const detail::NotificationContextLuaStatePreparation> prepared,
    std::shared_ptr<const detail::NotificationContextLuaLoadSetupReturn> actual_setup,
    std::shared_ptr<const detail::NotificationContextRootSmallUiPolicyLoan> actual_policy){
    using Frame=ContextRootReadFileFrame;using State=Frame::State;using Wide=Frame::Wide;
    const auto actual=core_;actual->layout_origin();
    require(&actual->source->current_operation_loan()==&operation,
        "RootRead canonical Source replaced before raw operation");
    operation.require_origin_without_callbacks();
    require(actual_name&&prepared&&actual_setup&&actual_policy&&!actual->context_root_read_file_frame.lock(),
        "RootRead requires genuine Name/518/full preRead setup; no prefix conversion");
    // Capture both lexical guards BEFORE every allocation and before busy entry.
    auto b4_unique=capture_context_root_read_b4_guard_owned(operation,actual_name,prepared);
    const auto setup=actual_setup->capture_guard_without_callbacks(operation,prepared);
    require(b4_unique!=nullptr,"RootRead captured B4 guard absent");
    b4_unique->require_data_current_without_callbacks(operation);
    setup.require_current_without_callbacks(operation,prepared);
    const auto state=actual->context_ordinary_state;
    require(state&&state->impl_&&state->impl_->native_constructor_return&&actual->context_layout_baseline,
        "RootRead needs actual full40F9C0 Context receiver");
    const auto ctor=state->impl_->native_constructor_return;
    const auto b4=state->impl_->b4_value;const auto before510=state->impl_->file_510_value;
    const auto receiver=actual->context_layout_baseline->base_.receiver_owned;
    const auto occurrence=actual->context_ordinary_occurrence;
    const auto revision=state->impl_->b4_revision;const auto serial=operation.operation_serial();
    require(ctor->impl_&&ctor->impl_->alive.load()&&ctor->impl_->port&&b4&&before510&&receiver&&occurrence,
        "RootRead actual physical constructor/510 custody absent");
    ctor->impl_->port->require_completed(); // immutable constructor snapshot, not current510
    const auto cold=[&]{
        b4_unique->require_data_current_without_callbacks(operation);
        actual->layout_origin();setup.require_current_without_callbacks(operation,prepared);
        require(State::Impl::same(actual->context_ordinary_state,state)&&
            State::Impl::same(state->impl_->native_constructor_return,ctor)&&
            State::Impl::same(state->impl_->b4_value,b4)&&state->impl_->b4_revision==revision&&
            State::Impl::same(state->impl_->file_510_value,before510)&&
            State::Impl::same(actual->context_lua_preparation,prepared)&&
            actual->context_layout_baseline&&State::Impl::same(actual->context_layout_baseline->base_.receiver_owned,receiver)&&
            State::Impl::same(actual->context_ordinary_occurrence,occurrence)&&
            actual->context_root_read_file_frame.expired(),"RootRead cold captured actual operands changed");
    };
    std::shared_ptr<Frame> frame;
    try{
        cold();operation.require_current();cold();
        // unique->shared can allocate. Keep b4_unique owning through allocation;
        // copy the guard by shared custody only after pure physical validation.
        auto name=std::shared_ptr<const detail::NotificationContextRootReadB4Guard>(std::move(b4_unique));
        name->require_data_current_without_callbacks(operation);
        actual->layout_origin();setup.require_current_without_callbacks(operation,prepared);
        operation.require_current();name->require_data_current_without_callbacks(operation);
        actual->layout_origin();setup.require_current_without_callbacks(operation,prepared);
        require(State::Impl::same(state->impl_->file_510_value,before510)&&
            State::Impl::same(actual->context_ordinary_state,state)&&actual->context_root_read_file_frame.expired(),
            "RootRead name guard allocation changed actual fields");
        frame=std::make_shared<Frame>(actual,state,name,setup,prepared,ctor,receiver,occurrence,b4,before510,
            &operation,serial,revision,actual_name,actual_setup);
        name->require_data_current_without_callbacks(operation);actual->layout_origin();
        setup.require_current_without_callbacks(operation,prepared);operation.require_current();
        name->require_data_current_without_callbacks(operation);actual->layout_origin();
        setup.require_current_without_callbacks(operation,prepared);
        require(State::Impl::same(actual->context_ordinary_state,state)&&
            State::Impl::same(state->impl_->native_constructor_return,ctor)&&
            State::Impl::same(state->impl_->file_510_value,before510)&&actual->context_root_read_file_frame.expired(),
            "RootRead frame allocation changed actual physical preimage");
        actual->busy=true;actual->context_root_read_file_frame=frame;frame->entered=true;
        {
        struct Exit final {std::shared_ptr<Frame> frame;std::shared_ptr<Core> actual;
            ~Exit(){frame->alive.store(false);
                if(actual->context_root_read_file_frame.lock()==frame){actual->context_root_read_file_frame.reset();actual->busy=false;}}
        } exit{frame,actual};
        frame->full();
        // 7A9C56→81FEC0: actual immutable B4 VALUE is pinned before copying.
        const auto retained_b4=b4;auto copy=retained_b4->value_;frame->full();
        auto copied=std::shared_ptr<const Wide>(new Wide(std::move(copy)));frame->full();
        state->impl_->file_510_value=copied;frame->expected_510=copied;
        frame->record(Frame::Effect::copy_b4_to_510);frame->full();
        // 7A9C6C→81F050 exact literal suffix. Do not change constructor snapshots.
        const auto retained_510=copied;auto with_xml=retained_510->value_;frame->full();
        with_xml.append(u".xml");frame->full();
        auto appended=std::shared_ptr<const Wide>(new Wide(std::move(with_xml)));frame->full();
        state->impl_->file_510_value=appended;frame->expected_510=appended;
        frame->record(Frame::Effect::append_xml);frame->full();
        require(frame->effect_count==2&&frame->effects[0]==Frame::Effect::copy_b4_to_510&&
            frame->effects[1]==Frame::Effect::append_xml,"RootRead actual ordered510 stores changed");
        resolve_and_watch_context_root_file_selected_owned(operation,frame,*actual_policy);
        require_context_root_read_staged_before_frame_exit_owned(frame);
        } // actual lexical RootRead frame Exit retires alive/weak frame/busy
        publish_context_root_read_after_scope_owned(operation);
    }catch(...){
        if(frame){frame->alive.store(false);frame->failed.store(true);}
        actual->failed.store(true); // no successful RootRead cleanup/return or pool reuse
        if(frame&&actual->context_root_read_file_frame.lock()==frame){
            actual->context_root_read_file_frame.reset();actual->busy=false;
        }
        throw;
    }
}
