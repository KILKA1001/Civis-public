// Included inside civ5::ui after the complete Context storage, ctor/init
// consumers and require_normal_root_receiver_before_common_without_callbacks.
// This producer retains existing physical objects. It constructs no Context,
// ordinary Control, paint index, Frame or native readiness receipt.
//Constructed and strongly retained ONLY by the Root-owned Tree lexical
//wrapper. Entry keeps a weak original control; neither a consumer nor this
//producer can open, renew or mint this scope. Root exit stores alive=false
//FIRST, before clearing Core's weak control and busy, even if a local lock
//temporarily retains this allocation after lexical retirement.
struct detail::BnwNormalRootContextEntry::Scope final {
    const std::thread::id creator=std::this_thread::get_id();
    std::atomic<bool> alive{true};
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    explicit Scope(std::shared_ptr<BnwWorldViewControlTree::Core> actual):core(std::move(actual)){
        require(!core.expired(),"Context root lexical Scope requires a real typed Core owner");
    }
    Scope(const Scope&)=delete;
    Scope&operator=(const Scope&)=delete;
};

// Read-only entry shared by idle prepaint/history readers and the actual
// live Root scope. A busy flag alone never authorizes a reentrant caller.
bool BnwWorldViewControlTree::require_normal_root_or_idle_origin_without_callbacks(
    const std::shared_ptr<Core>& actual){
    require(actual&&actual->creator==std::this_thread::get_id()&&actual->facade_alive.load()&&!actual->failed.load(),
        "Root or idle reader has no current creator/owner");
    const auto scope=actual->normal_context_root_scope.lock();
    if(scope){
        require(scope->creator==std::this_thread::get_id()&&scope->alive.load(),
            "Root reader's actual lexical scope has retired");
        const auto physical=scope->core.lock();
        require(physical&&physical.get()==actual.get()&&!physical.owner_before(actual)&&!actual.owner_before(physical),
            "Root reader's lexical scope belongs to another actual Core");
        actual->layout_origin(true);
        return true;
    }
    actual->layout_origin(); // Any unrelated busy operation remains an error.
    return false;
}

struct detail::BnwNormalRootContextEntry::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    const std::shared_ptr<const BnwWorldViewControlTree> tree;
    const std::shared_ptr<BnwWorldViewControlTree::Core> core;
    const std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> source;
    const std::weak_ptr<const Scope> scope;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t serial,topology,effects,layout,sequence,constructor_serial;
    const std::optional<std::uint64_t> parser_serial;
    const std::shared_ptr<const BnwWorldViewControlTree::LayoutInitializationReturn> seal;
    const std::shared_ptr<const NotificationLayoutContextInitializationReturn> completion;
    const std::shared_ptr<const BnwWorldViewControlTree::LayoutContextInitializationOrigin> origin;
    const std::shared_ptr<const NotificationLayoutContextConstructionReturn> construction;
    const std::shared_ptr<NotificationLayoutContextOrdinaryState> context;
    const std::shared_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> occurrence;
    const std::shared_ptr<const NotificationContextNativeConstructorPrefix> prefix;
    const std::shared_ptr<const NotificationContextNativeConstructorReturn> returned;
    const std::shared_ptr<const NotificationLayoutOrdinaryReceiver> receiver;
    const std::shared_ptr<NotificationLayoutOrdinaryChildren78> children;
    // Identity only; never dereferenced before fresh Source and the genuine
    // completed receiver guard. Strong SAME Core retains its actual storage.
    const NotificationLayoutBaseline*const baseline;
    const BnwWorldViewControlTree::Core::LayoutFourCReturn four_c;
    const std::array<std::uint32_t,2> offset;
    const std::uint32_t alpha,flags;
    std::weak_ptr<const Impl> canonical_self;
    bool sealed=false; //Closed value publication, not Context readiness.

    Impl(std::shared_ptr<const BnwWorldViewControlTree> actual_tree,
        std::shared_ptr<BnwWorldViewControlTree::Core> actual_core,
        std::shared_ptr<runtime::BnwSelectedWorldViewContextOwner> actual_source,
        std::weak_ptr<const Scope> actual_scope,
        const runtime::BnwWorldViewSourceOperationLoan* actual_operation,
        std::uint64_t actual_serial,std::uint64_t actual_topology,std::uint64_t actual_effects,
        std::uint64_t actual_layout,std::uint64_t actual_sequence,std::optional<std::uint64_t> actual_parser_serial,
        std::uint64_t actual_constructor_serial,
        std::shared_ptr<const BnwWorldViewControlTree::LayoutInitializationReturn> actual_seal,
        std::shared_ptr<const NotificationLayoutContextInitializationReturn> actual_completion,
        std::shared_ptr<const BnwWorldViewControlTree::LayoutContextInitializationOrigin> actual_origin,
        std::shared_ptr<const NotificationLayoutContextConstructionReturn> actual_construction,
        std::shared_ptr<NotificationLayoutContextOrdinaryState> actual_context,
        std::shared_ptr<const BnwWorldViewControlTree::Core::LayoutReceiverOccurrence> actual_occurrence,
        std::shared_ptr<const NotificationContextNativeConstructorPrefix> actual_prefix,
        std::shared_ptr<const NotificationContextNativeConstructorReturn> actual_returned,
        std::shared_ptr<const NotificationLayoutOrdinaryReceiver> actual_receiver,
        std::shared_ptr<NotificationLayoutOrdinaryChildren78> actual_children,
        const NotificationLayoutBaseline* actual_baseline,
        BnwWorldViewControlTree::Core::LayoutFourCReturn actual_four_c,
        std::array<std::uint32_t,2> actual_offset,std::uint32_t actual_alpha,std::uint32_t actual_flags)
        :tree(std::move(actual_tree)),core(std::move(actual_core)),source(std::move(actual_source)),
         scope(std::move(actual_scope)),operation(actual_operation),serial(actual_serial),topology(actual_topology),effects(actual_effects),
         layout(actual_layout),sequence(actual_sequence),constructor_serial(actual_constructor_serial),
         parser_serial(actual_parser_serial),seal(std::move(actual_seal)),
         completion(std::move(actual_completion)),origin(std::move(actual_origin)),
         construction(std::move(actual_construction)),context(std::move(actual_context)),
         occurrence(std::move(actual_occurrence)),prefix(std::move(actual_prefix)),
         returned(std::move(actual_returned)),receiver(std::move(actual_receiver)),
         children(std::move(actual_children)),baseline(actual_baseline),four_c(actual_four_c),
         offset(actual_offset),alpha(actual_alpha),flags(actual_flags){}
    static bool same(const auto&a,const auto&b){
        return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);
    }
    void inputs()const{
        BnwWorldViewControlTree::require_normal_root_context_entry_inputs_without_callbacks(*this);
    }
    void first()const{
        const auto original=canonical_self.lock();
        require(creator==std::this_thread::get_id()&&sealed&&original&&original.get()==this,
            "Context entry is foreign or not its closed published allocation");
        inputs();
    }
    void full(const runtime::BnwWorldViewSourceOperationLoan& supplied)const{
        first();require(&supplied==operation,"Context entry FULL supplied another Source operation");
        supplied.require_operation(serial);first();
        source->require_sources_current();first();
        supplied.require_current();first();supplied.require_operation(serial);first();
    }
};

void BnwWorldViewControlTree::require_normal_root_context_entry_inputs_without_callbacks(
    const detail::BnwNormalRootContextEntry::Impl& entry){
    using Entry=detail::BnwNormalRootContextEntry;
    require(entry.creator==std::this_thread::get_id()&&entry.tree&&entry.core&&entry.source&&
        entry.operation&&entry.serial,"Context entry lost its actual Tree/Core/Source custody");
    const auto lifetime=entry.scope.lock();
    require(lifetime&&lifetime->creator==std::this_thread::get_id()&&lifetime->alive.load(),
        "Context entry original lexical Scope retired, absent or foreign FIRST");
    const auto actual=lifetime->core.lock();
    require(actual&&Entry::Impl::same(actual,entry.core),
        "Context entry lexical Scope is not the SAME original weak Core");
    const auto current_scope=actual->normal_context_root_scope.lock();
    require(Entry::Impl::same(current_scope,lifetime),
        "Context entry original Scope allocation/control is no longer current");
    //Only the real alive original Scope+canonical weak membership permits
    //the following borrowed Core/Source/Context guards. busy or a repeated
    //Source serial alone can never resurrect this weak captured allocation.
    actual->layout_origin(true);
    require(Entry::Impl::same(entry.tree->core_,actual)&&Entry::Impl::same(actual->source,entry.source),
        "Context entry original Tree/Core/Source allocation/control replaced");
    // Source FIRST: compare the fresh owner-issued operation before any saved
    // operation or borrowed Context/native receiver dereference.
    const auto& fresh=entry.source->current_operation_loan();
    require(&fresh==entry.operation,"Context entry fresh Source replaced before saved operand");
    fresh.require_origin_without_callbacks();require(fresh.operation_serial()==entry.serial,
        "Context entry immutable current Source serial changed");
    require_normal_root_receiver_before_common_without_callbacks(actual,fresh,entry.serial);
    const auto same=[](const auto&a,const auto&b){return Entry::Impl::same(a,b);};
    const auto& now=*actual->context_layout_baseline;const auto physical=actual->context_ordinary_state->impl_;
    const auto& stamp=*actual->context_layout_four_c_return;
    require(actual->topology_revision==entry.topology&&actual->effect_revision==entry.effects&&
        actual->layout_sequence==entry.layout&& &now==entry.baseline&&now.sequence_==entry.sequence&&
        now.parsed_source_serial_==entry.parser_serial&&
        same(actual->layout_initialization_return,entry.seal)&&
        same(actual->layout_initialization_return->native_return_,entry.completion)&&
        same(actual->layout_context_initialization_origin,entry.origin)&&
        same(actual->context_construction_return,entry.construction)&&
        same(actual->context_ordinary_state,entry.context)&&same(actual->context_receiver_occurrence,entry.occurrence)&&
        same(physical->native_constructor_prefix,entry.prefix)&&same(physical->native_constructor_return,entry.returned)&&
        physical->serial==entry.constructor_serial&&same(now.base_.receiver_owned,entry.receiver)&&
        same(now.base_.children_78,entry.children)&&now.base_.alpha_word_44&&*now.base_.alpha_word_44==entry.alpha&&
        now.base_.flags_40==entry.flags&&entry.offset==std::array<std::uint32_t,2>{
            std::bit_cast<std::uint32_t>(now.base_.offset_28_2c[0]),std::bit_cast<std::uint32_t>(now.base_.offset_28_2c[1])}&&
        stamp.topology_revision==entry.four_c.topology_revision&&stamp.source_serial==entry.four_c.source_serial&&
        stamp.baseline_sequence==entry.four_c.baseline_sequence&&stamp.offset_store_sequence==entry.four_c.offset_store_sequence&&
        std::bit_cast<std::uint32_t>(stamp.actual_offset[0])==std::bit_cast<std::uint32_t>(entry.four_c.actual_offset[0])&&
        std::bit_cast<std::uint32_t>(stamp.actual_offset[1])==std::bit_cast<std::uint32_t>(entry.four_c.actual_offset[1]),
        "Context entry original ctor/init/root receiver or current physical tuple changed");
    actual->layout_origin(true);const auto& terminal=entry.source->current_operation_loan();
    require(&terminal==entry.operation,"Context entry terminal fresh Source changed before saved operation");
    terminal.require_origin_without_callbacks();require(terminal.operation_serial()==entry.serial,
        "Context entry terminal current Source serial changed");
}

detail::BnwNormalRootContextEntry::BnwNormalRootContextEntry(
    std::shared_ptr<const Impl> actual,const IssuerKey&):impl_(std::move(actual)){}
void detail::BnwNormalRootContextEntry::require_current_without_callbacks()const{
    const auto actual=impl_;require(bool(actual),"Context entry closed control absent");actual->first();
}
void detail::BnwNormalRootContextEntry::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto actual=impl_;require(bool(actual),"Context entry FULL closed control absent");
    try{actual->first();actual->full(operation);actual->first();}
    catch(...){actual->core->failed.store(true);throw;}
}
void detail::BnwNormalRootContextEntry::require_same_tree_without_callbacks(const BnwWorldViewControlTree& tree)const{
    const auto actual=impl_;require(bool(actual),"Context entry SAME Tree closed control absent");actual->first();
    require(actual->tree.get()==&tree&&Impl::same(tree.core_,actual->core),
        "Context entry is not this SAME actual Tree/Context domain");actual->first();
}
void detail::BnwNormalRootContextEntry::require_same_entry_without_callbacks(const BnwNormalRootContextEntry& supplied)const{
    const auto actual=impl_,other=supplied.impl_;
    require(actual&&other,"Context entry SAME closed controls absent");actual->first();other->first();
    require(Impl::same(actual,other),"Context entry consumer retained another original allocation/control");
    actual->first();other->first();
}

detail::BnwNormalRootContextEntry BnwWorldViewControlTree::capture_normal_root_context_entry_owned(
    std::shared_ptr<const BnwWorldViewControlTree> same_tree,
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    using Entry=detail::BnwNormalRootContextEntry;
    const auto tree=std::move(same_tree);const auto actual=core_;
    const auto scope=actual->normal_context_root_scope.lock();
    require(scope&&scope->creator==std::this_thread::get_id()&&scope->alive.load(),
        "Context entry producer has no actual already-opened alive lexical Scope FIRST");
    require(Entry::Impl::same(scope->core.lock(),actual)&&
        Entry::Impl::same(actual->normal_context_root_scope.lock(),scope),
        "Context entry producer Scope is not the actual canonical weak Core control");
    require(tree&&tree.get()==this&&Entry::Impl::same(tree->core_,actual),
        "Context entry producer requires SAME actual Tree custody");
    actual->layout_origin(true);const auto source=actual->source;
    const auto& fresh=source->current_operation_loan();
    require(&fresh==&operation,"Context entry producer fresh Source differs before supplied operand");
    fresh.require_origin_without_callbacks();const auto serial=fresh.operation_serial();
    const auto pure=[&]{
        require(scope->creator==std::this_thread::get_id()&&scope->alive.load(),
            "Context entry producer original lexical Scope retired FIRST");
        require(Entry::Impl::same(scope->core.lock(),actual)&&
            Entry::Impl::same(actual->normal_context_root_scope.lock(),scope),
            "Context entry producer original Scope allocation/control replaced");
        actual->layout_origin(true);require(Entry::Impl::same(tree->core_,actual)&&Entry::Impl::same(actual->source,source),
            "Context entry producer actual Tree/Core/Source replaced");
        const auto& current=source->current_operation_loan();
        require(&current==&operation,"Context entry producer current Source replaced before old operand");
        current.require_origin_without_callbacks();require(current.operation_serial()==serial,
            "Context entry producer current lexical Source serial changed");
        require_normal_root_receiver_before_common_without_callbacks(actual,current,serial);
    };
    const auto full=[&]{pure();source->require_sources_current();pure();operation.require_current();pure();
        operation.require_operation(serial);pure();};
    try{
        pure();full();pure();
        //The genuine helper above earned every following borrowed read. Copies
        //are OWN real controls/scalars, never copied XML or a root-index DTO.
        const auto topology=actual->topology_revision,effects=actual->effect_revision,layout=actual->layout_sequence;
        const auto seal=actual->layout_initialization_return;const auto completion=seal->native_return_;
        const auto origin=actual->layout_context_initialization_origin;
        const auto construction=actual->context_construction_return;
        const auto context=actual->context_ordinary_state;const auto occurrence=actual->context_receiver_occurrence;
        const auto prefix=context->impl_->native_constructor_prefix;
        const auto returned=context->impl_->native_constructor_return;
        const auto constructor_serial=context->impl_->serial;
        const auto*const baseline=&*actual->context_layout_baseline;
        const auto sequence=baseline->sequence_;
        const auto parser_serial=baseline->parsed_source_serial_;
        const auto receiver=baseline->base_.receiver_owned;
        const auto children=baseline->base_.children_78;
        const auto four_c=*actual->context_layout_four_c_return;
        const std::array<std::uint32_t,2> offset{
            std::bit_cast<std::uint32_t>(baseline->base_.offset_28_2c[0]),
            std::bit_cast<std::uint32_t>(baseline->base_.offset_28_2c[1])};
        const auto alpha=*baseline->base_.alpha_word_44,flags=baseline->base_.flags_40;
        const auto pending=[&]{
            pure();const auto& now=*actual->context_layout_baseline;const auto& stamp=*actual->context_layout_four_c_return;
            require(actual->topology_revision==topology&&actual->effect_revision==effects&&actual->layout_sequence==layout&&
                &now==baseline&&now.sequence_==sequence&&now.parsed_source_serial_==parser_serial&&
                Entry::Impl::same(actual->layout_initialization_return,seal)&&Entry::Impl::same(seal->native_return_,completion)&&
                Entry::Impl::same(actual->layout_context_initialization_origin,origin)&&
                Entry::Impl::same(actual->context_construction_return,construction)&&
                Entry::Impl::same(actual->context_ordinary_state,context)&&Entry::Impl::same(actual->context_receiver_occurrence,occurrence)&&
                context->impl_->serial==constructor_serial&&Entry::Impl::same(context->impl_->native_constructor_prefix,prefix)&&
                Entry::Impl::same(context->impl_->native_constructor_return,returned)&&
                Entry::Impl::same(now.base_.receiver_owned,receiver)&&Entry::Impl::same(now.base_.children_78,children)&&
                now.base_.alpha_word_44&&*now.base_.alpha_word_44==alpha&&now.base_.flags_40==flags&&
                offset==std::array<std::uint32_t,2>{std::bit_cast<std::uint32_t>(now.base_.offset_28_2c[0]),
                    std::bit_cast<std::uint32_t>(now.base_.offset_28_2c[1])}&&
                stamp.topology_revision==four_c.topology_revision&&stamp.source_serial==four_c.source_serial&&
                stamp.baseline_sequence==four_c.baseline_sequence&&stamp.offset_store_sequence==four_c.offset_store_sequence&&
                std::bit_cast<std::uint32_t>(stamp.actual_offset[0])==std::bit_cast<std::uint32_t>(four_c.actual_offset[0])&&
                std::bit_cast<std::uint32_t>(stamp.actual_offset[1])==std::bit_cast<std::uint32_t>(four_c.actual_offset[1]),
                "Context entry allocator/FULL bookend changed original physical operands");
            pure();
        };
        pending();full();pending();
        auto owned=std::make_shared<Entry::Impl>(tree,actual,source,std::weak_ptr<const Entry::Scope>(scope),&operation,serial,topology,effects,layout,sequence,
            parser_serial,constructor_serial,seal,completion,origin,construction,context,occurrence,prefix,returned,
            receiver,children,baseline,four_c,offset,alpha,flags);
        pending();full();pending();owned->inputs();
        require(!owned->sealed&&owned->canonical_self.expired(),"Context entry allocation was already published");
        owned->canonical_self=owned;owned->sealed=true;owned->first();owned->full(operation);owned->first();
        const Entry::IssuerKey key;return Entry(owned,key);
    }catch(...){actual->failed.store(true);throw;}
}
