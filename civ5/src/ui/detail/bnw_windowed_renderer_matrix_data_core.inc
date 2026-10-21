// Inside civ5::ui after the complete canonical GUI manager. No renderer body.
struct detail::BnwWindowedRendererMatrixDataLoan::Impl {
    using Gui=NotificationContextGuiPort;using State=Gui::Impl::WindowedMatrixState;
    const std::thread::id creator=std::this_thread::get_id();std::atomic<bool> alive{true};
    const std::weak_ptr<Gui::Impl> gui;
    const std::weak_ptr<Gui> facade;
    const std::shared_ptr<State> matrix;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t serial;
    bool sealed=false;
    Impl(std::shared_ptr<Gui::Impl> owner,std::shared_ptr<Gui> actual,std::shared_ptr<State> data,
        const runtime::BnwWorldViewSourceOperationLoan& op,std::uint64_t value)
        :gui(owner),facade(actual),matrix(std::move(data)),operation(&op),serial(value){}
    static std::array<std::uint32_t,2> profile(){
#if (defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__))
        // FXSAVE only observes the actual stack/tag/control state. No FNINIT,
        // FLDCW/LDMXCSR/FXRSTOR or exception/status clearing occurs.
        struct alignas(16) Snapshot {std::array<unsigned char,512> bytes;};Snapshot env;
        std::uint16_t cw=0;std::uint32_t mx=0;
        __asm__ volatile("fxsave %0\n\tfnstcw %1\n\tstmxcsr %2"
            :"=m"(env),"=m"(cw),"=m"(mx)::"memory");
        require(env.bytes[4]==0,"Windowed ortho nonempty inherited x87 stack requires separate numeric owner");
        require((cw&0x3fu)==0x3fu&&((cw>>8)&3u)!=1&&
            (mx&0x1f80u)==0x1f80u,"Windowed ortho unmasked/reserved numeric profile unsupported");
        return {cw,mx};
#else
        throw std::runtime_error("Windowed ortho needs genuine x87/SSE GNU-compatible x86 compiler/host");
#endif
    }
    static void same_controls(const std::array<std::uint32_t,2>& captured){
        const auto now=profile();require(now[0]==captured[0]&&
            (now[1]&~std::uint32_t{0x3f})==(captured[1]&~std::uint32_t{0x3f}),
            "Windowed ortho actual CW/MXCSR controls changed");
    }
    static std::array<std::uint32_t,16> compute(std::int32_t width,std::int32_t height,
        const std::array<std::uint32_t,2>& actual){
        same_controls(actual);require(width>0&&height>0,"Windowed ortho actual successful positive client extent required");
#if (defined(__i386__) || defined(__x86_64__)) && (defined(__GNUC__) || defined(__clang__))
        // Raw arguments are Original proven +0,W,H,+0,+0,+10, not guessed
        // caller dimensions. CVTSI2SS retains inherited actual MXCSR.
        std::array<std::uint32_t,6> args{0,0,0,0,0,0x41200000u};
        __asm__ volatile("cvtsi2ssl %[w],%%xmm0\n\tmovss %%xmm0,%[r]\n\t"
            "cvtsi2ssl %[h],%%xmm0\n\tmovss %%xmm0,%[b]"
            :[r]"=m"(args[1]),[b]"=m"(args[2]):[w]"m"(width),[h]"m"(height):"xmm0","memory");
        same_controls(actual);
        std::array<std::uint32_t,16> out;std::array<std::uint32_t,3> reciprocal;
        const std::uint64_t two=0x4000000000000000ull;
        // Exact local DLL x87 stack order. Register opcodes are explicit to
        // avoid assembler FDIVP/FDIVRP dialect reversal. Every reciprocal is
        // FSTP32-rounded before scale/translation. Net added stack depth zero,
        // maximum four. Control/status are inherited; sticky flags may accrue.
        __asm__ volatile(
            "flds 4(%[a])\n\tfsubs 0(%[a])\n\tfld1\n\tfld %%st(0)\n\t.byte 0xde,0xf2\n\t"
            "fxch %%st(1)\n\tfstps 0(%[r])\n\t"
            "flds 12(%[a])\n\tfsubs 8(%[a])\n\tfld %%st(1)\n\t.byte 0xde,0xf1\n\tfstps 4(%[r])\n\t"
            "flds 0(%[r])\n\tfldl %[two]\n\t.byte 0xdc,0xc9\n\tfxch %%st(1)\n\tfstps 0(%[o])\n\t"
            "fldz\n\tfsts 4(%[o])\n\tfsts 8(%[o])\n\tfsts 12(%[o])\n\tfsts 16(%[o])\n\t"
            "flds 4(%[r])\n\t.byte 0xde,0xca\n\tfxch %%st(1)\n\tfstps 20(%[o])\n\t"
            "fsts 24(%[o])\n\tfsts 28(%[o])\n\tfsts 32(%[o])\n\tfsts 36(%[o])\n\t"
            "flds 16(%[a])\n\tfsubs 20(%[a])\n\t.byte 0xde,0xfa\n\tfxch %%st(1)\n\tfstps 8(%[r])\n\t"
            "flds 8(%[r])\n\tfstps 40(%[o])\n\tfstps 44(%[o])\n\t"
            "flds 0(%[a])\n\tfadds 4(%[a])\n\tfmuls 0(%[r])\n\tfchs\n\tfstps 48(%[o])\n\t"
            "flds 8(%[a])\n\tfadds 12(%[a])\n\tfmuls 4(%[r])\n\tfchs\n\tfstps 52(%[o])\n\t"
            "flds 8(%[r])\n\tfmuls 16(%[a])\n\tfstps 56(%[o])\n\tfld1\n\tfstps 60(%[o])"
            :"=m"(out),"=m"(reciprocal):[a]"r"(args.data()),[r]"r"(reciprocal.data()),
             [o]"r"(out.data()),[two]"m"(two):"st","st(1)","st(2)","st(3)","memory");
        same_controls(actual);
        constexpr std::array<unsigned,9> zeros{1,2,3,4,6,7,8,9,11};
        for(auto index:zeros)require(out[index]==0,"Windowed ortho actual FLDZ word differs");
        require(out[14]==0x80000000u&&out[15]==0x3f800000u,
            "Windowed ortho genuine computed negative zero/depth unit differs");
        for(auto word:out)require((word&0x7fffffffu)<0x7f800000u,"Windowed ortho nonfinite result unsupported");
        return out;
#else
        (void)actual;throw std::runtime_error("Windowed ortho numeric producer unavailable on this host");
#endif
    }
    const runtime::BnwWorldViewSourceOperationLoan& first()const{
        require(creator==std::this_thread::get_id()&&alive.load()&&sealed,
            "Windowed matrix loan retired/foreign/unsealed before borrow");
        const auto manager=gui.lock();const auto actual=facade.lock();
        require(manager&&actual&&manager->facade_alive.load()&&!manager->failed.load(),
            "Windowed matrix physical manager/facade retired");
        const auto source=manager->source.lock();require(bool(source),"Windowed matrix Source owner retired");
        // Actual canonical current Source BEFORE any saved raw op dereference.
        const auto& current=source->current_operation_loan();
        require(&current==operation,"Windowed matrix lexical Source replaced before saved pointer");
        current.require_origin_without_callbacks();require(current.operation_serial()==serial,
            "Windowed matrix lexical Source serial changed");manager->first(current);
        const auto canonical=manager->canonical_gui.lock();
        require(canonical&&canonical.get()==actual.get()&&!canonical.owner_before(actual)&&!actual.owner_before(canonical)&&
            actual->impl_.get()==manager.get()&&!actual->impl_.owner_before(manager)&&!manager.owner_before(actual->impl_)&&
            !manager->windowed_matrix_busy&&matrix&&matrix->alive.load()&&matrix->completed&&
            matrix->creator==creator&&manager->windowed_matrix.get()==matrix.get()&&
            !manager->windowed_matrix.owner_before(matrix)&&!matrix.owner_before(manager->windowed_matrix)&&
            manager->revision==matrix->gui_revision&&manager->windowed_matrix_updates==matrix->update&&
            matrix->source_serial==serial&&matrix->dimensions==std::array<std::int32_t,2>{manager->field_644,manager->field_648}&&
            matrix->arithmetic_words==std::array<std::uint32_t,2>{std::bit_cast<std::uint32_t>(manager->arithmetic[0]),
                std::bit_cast<std::uint32_t>(manager->arithmetic[1])}&&matrix->gui_revision&&
            !manager->history.empty()&&manager->history.back().kind==Gui::Impl::Store::windowed_ortho&&
            manager->history.back().sequence==matrix->gui_revision&&manager->history.back().source_serial==serial,
            "Windowed matrix actual owned state/revision/paired dimensions/material changed");
        same_controls(matrix->numeric_before);manager->first(current);return current;
    }
};
detail::BnwWindowedRendererMatrixDataLoan::BnwWindowedRendererMatrixDataLoan(std::shared_ptr<Impl> value):impl_(std::move(value)){}
detail::BnwWindowedRendererMatrixDataLoan::~BnwWindowedRendererMatrixDataLoan(){impl_->alive.store(false);}
void detail::BnwWindowedRendererMatrixDataLoan::require_current_without_callbacks()const{const auto& current=impl_->first();(void)current;}
void detail::BnwWindowedRendererMatrixDataLoan::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto& current=impl_->first();require(&current==&operation,"Windowed matrix FULL another Source operand");
    const auto manager=impl_->facade.lock();require(bool(manager),"Windowed matrix facade retired before FULL");
    manager->require_current_owned(current);impl_->first();current.require_current();impl_->first();
}
#define BNW_WINDOWED_MATRIX_VALUE(method,type,member) \
type detail::BnwWindowedRendererMatrixDataLoan::method()const{impl_->first();const auto value=impl_->matrix->member;impl_->first();return value;}
using WindowedMatrixWords=std::array<std::uint32_t,16>;using WindowedMatrixPair=std::array<std::uint32_t,2>;
using WindowedMatrixExtent=std::array<std::int32_t,2>;
BNW_WINDOWED_MATRIX_VALUE(matrix_words_without_callbacks,WindowedMatrixWords,words)
BNW_WINDOWED_MATRIX_VALUE(arithmetic_words_without_callbacks,WindowedMatrixPair,arithmetic_words)
BNW_WINDOWED_MATRIX_VALUE(dimensions_without_callbacks,WindowedMatrixExtent,dimensions)
BNW_WINDOWED_MATRIX_VALUE(numeric_controls_without_callbacks,WindowedMatrixPair,numeric_before)
BNW_WINDOWED_MATRIX_VALUE(numeric_return_controls_without_callbacks,WindowedMatrixPair,numeric_after)
BNW_WINDOWED_MATRIX_VALUE(manager_revision_without_callbacks,std::uint64_t,gui_revision)
#undef BNW_WINDOWED_MATRIX_VALUE
detail::BnwWindowedRendererMatrixDataLoan::ShaderAccess::ShaderAccess(std::shared_ptr<Impl> state)
    :impl_(std::move(state)){impl_->first();}
detail::BnwWindowedRendererMatrixDataLoan::ShaderAccess
detail::BnwWindowedRendererMatrixDataLoan::capture_shader_access_without_callbacks()const{
    impl_->first();return ShaderAccess(impl_);
}
void detail::BnwWindowedRendererMatrixDataLoan::ShaderAccess::require_current_without_callbacks()const{
    impl_->first();
}
void detail::BnwWindowedRendererMatrixDataLoan::ShaderAccess::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    // FIRST reacquires the real canonical Source before the saved operand.
    const auto& current=impl_->first();require(&current==&operation,
        "Windowed matrix Shader DATA FULL another Source operand");
    const auto manager=impl_->facade.lock();require(bool(manager),
        "Windowed matrix Shader DATA facade retired before FULL");
    manager->require_current_owned(current);impl_->first();
    current.require_current();impl_->first();
}

detail::BnwWindowedRendererSharedDescriptorIdentity
detail::BnwWindowedRendererMatrixDataLoan::ShaderAccess::shared_descriptor_identity_without_callbacks()const{
    const auto saved=impl_;saved->first();
    const auto manager=saved->gui.lock();require(bool(manager),"Shared descriptor capture GUI retired");
    const auto control=manager->windowed_shared_descriptor;
    require(bool(control),"Shared descriptor actual canonical constructor absent");
    const auto result=BnwWindowedRendererSharedDescriptorIdentity(control);
    result.require_current_without_callbacks();saved->first();
    require(manager->windowed_shared_descriptor.get()==control.get()&&
        !manager->windowed_shared_descriptor.owner_before(control)&&!control.owner_before(manager->windowed_shared_descriptor),
        "Shared descriptor capture canonical resource changed");
    result.require_current_without_callbacks();return result;
}

#define BNW_WINDOWED_MATRIX_SHADER_VALUE(method,type,member) \
type detail::BnwWindowedRendererMatrixDataLoan::ShaderAccess::method()const{impl_->first();const auto value=impl_->matrix->member;impl_->first();return value;}
BNW_WINDOWED_MATRIX_SHADER_VALUE(matrix_words_without_callbacks,WindowedMatrixWords,words)
BNW_WINDOWED_MATRIX_SHADER_VALUE(arithmetic_words_without_callbacks,WindowedMatrixPair,arithmetic_words)
BNW_WINDOWED_MATRIX_SHADER_VALUE(dimensions_without_callbacks,WindowedMatrixExtent,dimensions)
BNW_WINDOWED_MATRIX_SHADER_VALUE(numeric_controls_without_callbacks,WindowedMatrixPair,numeric_before)
BNW_WINDOWED_MATRIX_SHADER_VALUE(numeric_return_controls_without_callbacks,WindowedMatrixPair,numeric_after)
BNW_WINDOWED_MATRIX_SHADER_VALUE(manager_revision_without_callbacks,std::uint64_t,gui_revision)
#undef BNW_WINDOWED_MATRIX_SHADER_VALUE
std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>
detail::NotificationContextGuiPort::capture_windowed_renderer_matrix_data_owned(const runtime::BnwWorldViewSourceOperationLoan& operation){
    using Loan=BnwWindowedRendererMatrixDataLoan;const auto state=impl_;
    try{
        std::optional<std::uint64_t> captured_serial;
        const auto canonical=[&]() -> const runtime::BnwWorldViewSourceOperationLoan& {
            require(state->creator==std::this_thread::get_id()&&state->facade_alive.load()&&!state->failed.load(),
                "Windowed matrix producer actual manager retired/foreign before Source borrow");
            const auto source=state->source.lock();require(bool(source),"Windowed matrix producer Source owner retired");
            const auto& fresh=source->current_operation_loan();
            require(&fresh==&operation,"Windowed matrix producer needs actual current Source before saved operand");
            fresh.require_origin_without_callbacks();
            if(captured_serial)require(fresh.operation_serial()==*captured_serial,
                "Windowed matrix producer lexical Source serial replaced");
            state->first(fresh);return fresh;
        };
        const auto serial=canonical().operation_serial();captured_serial=serial;
        const auto guarded_full=[&]{const auto& fresh=canonical();require_current_owned(fresh);canonical();};
        guarded_full();
        const auto actual=state->canonical_gui.lock();require(actual&&actual.get()==this&&
            !actual->impl_.owner_before(state)&&!state.owner_before(actual->impl_),"Windowed matrix actual canonical GUI facade absent");
        require(!state->windowed_matrix_busy&&state->windowed_matrix_updates<UINT64_MAX&&state->revision<UINT64_MAX&&
            state->history.size()<state->history.max_size(),
            "Windowed matrix reentered/store history exhausted");
        state->windowed_matrix_busy=true;struct End{bool& busy;~End(){busy=false;}}end{state->windowed_matrix_busy};
        const auto revision=state->revision;const auto update=state->windowed_matrix_updates;
        const auto dimensions=std::array<std::int32_t,2>{state->field_644,state->field_648};
        const auto arithmetic=std::array<std::uint32_t,2>{std::bit_cast<std::uint32_t>(state->arithmetic[0]),
            std::bit_cast<std::uint32_t>(state->arithmetic[1])};
        const auto pure=[&]{canonical();require(state->windowed_matrix_busy&&state->revision==revision&&
            state->windowed_matrix_updates==update&&dimensions==std::array<std::int32_t,2>{state->field_644,state->field_648}&&
            arithmetic==std::array<std::uint32_t,2>{std::bit_cast<std::uint32_t>(state->arithmetic[0]),std::bit_cast<std::uint32_t>(state->arithmetic[1])},
            "Windowed matrix allocating/numeric supplier POST changed manager operands");};
        pure();auto data=std::make_shared<Impl::WindowedMatrixState>();pure();guarded_full();pure();
        state->history.reserve(state->history.size()+1);pure();guarded_full();pure();
        data->numeric_before=Loan::Impl::profile();pure();
        data->words=Loan::Impl::compute(dimensions[0],dimensions[1],data->numeric_before);pure();
        data->numeric_after=Loan::Impl::profile();pure();guarded_full();pure();Loan::Impl::same_controls(data->numeric_before);
        data->dimensions=dimensions;data->arithmetic_words=arithmetic;data->source_serial=serial;
        data->gui_revision=revision+1;data->update=update+1;
        pure();auto result=std::shared_ptr<Loan>(new Loan(std::make_shared<Loan::Impl>(state,actual,data,canonical(),serial)));
        pure();guarded_full();pure();Loan::Impl::same_controls(data->numeric_before);
        // Reserved journal push and the following stores perform no allocation
        // or callback. The real mutable manager advances; old loan is retired.
        canonical();state->record(canonical(),Impl::Store::windowed_ortho);
        if(state->windowed_matrix)state->windowed_matrix->alive.store(false);
        state->windowed_matrix=data;state->windowed_matrix_updates=data->update;data->completed=true;
        state->windowed_matrix_busy=false;
        guarded_full();canonical();
        require(state->revision==data->gui_revision&&state->windowed_matrix==data,
            "Windowed matrix completed store FULL POST changed");
        canonical();result->impl_->sealed=true;result->require_source_owned(canonical());canonical();
        result->require_current_without_callbacks();canonical();return result;
    }catch(...){state->failed.store(true);if(state->windowed_matrix)state->windowed_matrix->alive.store(false);throw;}
}
std::shared_ptr<const detail::BnwWindowedRendererMatrixDataLoan>
BnwWorldViewControlTree::windowed_renderer_matrix_data_owned(std::shared_ptr<const BnwWorldViewControlTree> actual_tree,
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    require(actual_tree&&actual_tree.get()==this,"Windowed matrix Tree lender requires SAME actual Tree");
    const auto owner=actual_tree->core_;owner->layout_origin(true);
    const auto& current=owner->source->current_operation_loan();owner->layout_origin(true);
    require(&current==&operation,"Windowed matrix Tree requires fresh actual Source before saved operand");
    current.require_origin_without_callbacks();const auto gui=owner->context_gui_port;
    require(bool(gui),"Windowed matrix real canonical GUI manager absent");
    auto result=gui->capture_windowed_renderer_matrix_data_owned(current);owner->layout_origin(true);
    require(owner->context_gui_port.get()==gui.get()&&!owner->context_gui_port.owner_before(gui)&&
        !gui.owner_before(owner->context_gui_port),"Windowed matrix Tree canonical GUI changed");
    result->require_source_owned(current);owner->layout_origin(true);result->require_current_without_callbacks();return result;
}
