// Same pure completed Root receiver checks used before paint and by Common.
// This is validation only: no Common entry, arithmetic, Prefix/Matrix or cursor.
void BnwWorldViewControlTree::require_normal_root_receiver_before_common_without_callbacks(
    const std::shared_ptr<Core>& actual,const runtime::BnwWorldViewSourceOperationLoan& operation,
    std::uint64_t serial){
    require(bool(actual),"Prepaint native Root receiver lacks actual owner");
    const bool expected_busy=require_normal_root_or_idle_origin_without_callbacks(actual);
    const auto& current=actual->source->current_operation_loan();
    require(&current==&operation,"Prepaint Root fresh Source differs before saved operation");
    current.require_origin_without_callbacks();require(current.operation_serial()==serial,
        "Prepaint Root immutable Source serial changed");
    const auto seal=actual->layout_initialization_return;
    require(seal&&seal->native_return_&&seal->native_return_->impl_,
        "Normal root requires genuine RootRead/attach then Context slot10/full initializer return");
    const auto done=seal->native_return_->impl_;
    require(bool(done->origin),"Normal root initializer origin absent");
    const bool direct=bool(done->origin->direct_read_);
    // A direct load keeps the Root parser genuinely absent. Its completed
    // Lua/show-hide writers have their own immutable post-load domains; the
    // earlier initializer is retained as history, never rewritten as current.
    const auto loaded=direct?require_context_direct_load_return_without_callbacks(actual,operation):
        std::shared_ptr<const detail::NotificationContextDirectLoadReturn>{};
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    require(done->lifetime&&done->lifetime->creator==std::this_thread::get_id()&&
        !done->lifetime->failed.load()&&done->lifetime->destroyed&&done->lifetime->destruction_passed&&
        !actual->context_initialization_active&&same(done->core.lock(),actual)&&same(seal->tree_.lock(),actual)&&
        done->topology==actual->topology_revision&&seal->topology_==actual->topology_revision&&
        (direct||(done->effects==actual->effect_revision&&done->completion_sequence==actual->layout_sequence))&&
        done->origin&&done->origin->construction_&&same(done->origin->tree_.lock(),actual)&&
        same(actual->layout_context_initialization_origin,done->origin)&&
        same(actual->context_construction_return,done->origin->construction_),
        "Normal root actual completed slot10 domain/lifetime/origin changed");
    const auto require_domains=[&](const auto& domains){
        bool root_found=false;
        for(const auto& domain:domains){
            const auto owner=domain.core.lock();require(bool(owner),"Normal root completed domain retired");
            owner->layout_origin(owner.get()==actual.get()?expected_busy:false);
            require(same(owner->source,actual->source)&&owner->topology_revision==domain.topology&&
                owner->effect_revision==domain.effects,"Normal root completed domain changed after DTOR");
            if(same(owner,actual)){require(!root_found,"Normal root completed domain duplicated");root_found=true;}
        }
        require(root_found,"Normal root completion omitted its actual root domain");
    };
    if(direct){
        require(loaded&&loaded->impl_&&same(loaded->impl_->initialized,seal->native_return_),
            "Normal root direct load completed another initializer");
        require_domains(loaded->impl_->domains);
    }else require_domains(done->domains);
    require(actual->context_layout_baseline&&actual->context_receiver_occurrence&&
        actual->context_ordinary_state&&actual->context_ordinary_state->impl_,
        "Normal root actual physical Context receiver unissued");
    const auto state=actual->context_ordinary_state;const auto c=state->impl_;
    require(c->creator==std::this_thread::get_id()&&c->alive.load()&&!c->failed.load()&&
        same(c->core.lock(),actual)&&same(c->source.lock(),actual->source)&&
        same(c->constructor,actual->context_construction_return)&&same(c->occurrence,actual->context_receiver_occurrence)&&
        c->occurrence->id==0&&c->occurrence->instance==0&&c->serial&&
        c->occurrence->constructor_serial==c->serial&&c->native_constructor_prefix&&c->native_constructor_return,
        "Normal root physical Context constructor allocation/control retired or replaced");
    const auto p=c->native_constructor_prefix->impl_;const auto r=c->native_constructor_return->impl_;
    require(p&&r&&p->creator==std::this_thread::get_id()&&r->creator==std::this_thread::get_id()&&
        p->alive.load()&&!p->failed.load()&&r->alive.load()&&!r->failed.load()&&p->port&&r->port&&
        same(p->core.lock(),actual)&&same(r->core.lock(),actual)&&same(p->projection.lock(),state)&&
        same(r->projection.lock(),state)&&same(r->prefix,c->native_constructor_prefix)&&
        p->serial==c->serial&&r->serial==c->serial,
        "Normal root A368D0 physical completed ctor chain missing/current allocation changed");
    p->port->require_reached();r->port->require_completed(); //OWN immutable ctor DATA, not cold operation authority.
    const auto& base=*actual->context_layout_baseline;
    require(base.kind_==detail::NotificationLayoutBaseline::Kind::context&&base.base_.alpha_word_44&&
        (direct?!base.parsed_source_serial_:(base.parsed_source_serial_&&*base.parsed_source_serial_))&&
        base.sequence_&&base.base_.receiver_owned&&
        base.base_.receiver_owned->impl_,"Normal root physical28/2C/44/receiver history absent");
    const auto receiver=base.base_.receiver_owned;const auto f=receiver->impl_;
    require(f->creator==std::this_thread::get_id()&&same(f->canonical.lock(),receiver)&&
        same(f->core.lock(),actual)&&same(f->occurrence.lock(),c->occurrence)&&
        same(f->context.lock(),state)&&f->id==0&&f->serial==c->serial,
        "Normal root earned Context subtype18 differs; no tag/kind-derived vtable");
    // A368D0+18 is pinned56F630. Current post-init28/2C need the genuine
    // own4C/offset stamp, never a constructor zero or generic double layout.
    require(actual->context_layout_four_c_return.has_value(),"Normal root genuine post-init own4C unavailable");
    const auto& stamp=*actual->context_layout_four_c_return;
    require(stamp.topology_revision==actual->topology_revision&&stamp.source_serial&&stamp.baseline_sequence&&
        stamp.baseline_sequence<=base.sequence_&&stamp.offset_store_sequence&&
        stamp.offset_store_sequence==actual->context_layout_offset_store_sequence&&
        std::bit_cast<std::uint32_t>(stamp.actual_offset[0])==std::bit_cast<std::uint32_t>(base.base_.offset_28_2c[0])&&
        std::bit_cast<std::uint32_t>(stamp.actual_offset[1])==std::bit_cast<std::uint32_t>(base.base_.offset_28_2c[1]),
        "Normal root actual common28/2C differs from completed offset producer");
    const auto children=base.base_.children_78;
    if(children)require(children->impl_&&children->impl_->creator==std::this_thread::get_id()&&
        same(children->impl_->parent.lock(),receiver),"Normal root actual78 owner differs");
    require(require_normal_root_or_idle_origin_without_callbacks(actual)==expected_busy,
        "Prepaint Root lexical exclusion changed during pure validation");
    require(&actual->source->current_operation_loan()==&operation,
        "Prepaint completed Root receiver Source changed during pure validation");
    current.require_origin_without_callbacks();
}

std::uint32_t BnwWorldViewControlTree::require_normal_root_ordered_child_flags_without_callbacks(
    const std::shared_ptr<Core>& actual,
    const std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn>& seal,
    const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& root,
    const std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>& child){
    const auto same=[](const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    require(actual&&seal&&seal->impl_&&root&&child&&child->impl_,
        "Actual ordered78 child physical owner missing");
    const bool expected_busy=require_normal_root_or_idle_origin_without_callbacks(actual);
    const auto receiver=child->impl_;const auto owner=receiver->core.lock();
    const auto occurrence=receiver->occurrence.lock();
    require(owner&&occurrence&&receiver->creator==std::this_thread::get_id(),
        "Normal56F630 physical child allocation/occurrence retired");
    owner->layout_origin(owner.get()==actual.get()?expected_busy:false);
    require(same(owner->source,actual->source)&&same(receiver->canonical.lock(),child)&&
        receiver->serial&&receiver->serial==occurrence->constructor_serial&&occurrence->id==receiver->id,
        "Normal56F630 genuine child canonical receiver/Source differs");
    bool domain_found=false;
    const auto require_domain=[&](const auto& domains){
        for(const auto& d:domains){if(same(d.core.lock(),owner)){
            require(!domain_found&&owner->topology_revision==d.topology&&owner->effect_revision==d.effects,
                "Normal56F630 actual child completed domain duplicated or changed");domain_found=true;}}
    };
    require(bool(seal->impl_->origin),"Normal56F630 actual initializer origin absent");
    if(seal->impl_->origin->direct_read_){
        const auto& operation=actual->source->current_operation_loan();
        const auto loaded=require_context_direct_load_return_without_callbacks(actual,operation);
        require(loaded&&loaded->impl_&&same(loaded->impl_->initialized,seal),
            "Normal56F630 direct load completed another initializer");
        require_domain(loaded->impl_->domains);
    }else require_domain(seal->impl_->domains);
    require(domain_found,"Normal56F630 actual child domain absent from genuine completed load");
    const detail::NotificationLayoutBaseline* physical=nullptr;
    if(receiver->id==0){
        require(owner->context_layout_baseline&&owner->context_ordinary_state&&
            same(receiver->context.lock(),owner->context_ordinary_state)&&
            same(owner->context_receiver_occurrence,occurrence),
            "Normal56F630 child Context subtype constructor unissued");
        physical=&*owner->context_layout_baseline;
    }else{
        const auto& node=Core::node(owner->nodes,receiver->id);
        require(node.layout_baseline&&same(node.layout_receiver_occurrence,occurrence)&&
            node.layout_baseline->standalone_label_&&
            same(receiver->standalone_label.lock(),node.layout_baseline->standalone_label_),
            "Normal56F630 unsupported child subtype requires its real dispatcher");
        physical=&*node.layout_baseline;
        const auto fields=physical->standalone_label_->impl_;fields->first();
        const auto* word=std::get_if<std::uint32_t>(&fields->fields[0x40/4]);
        require(word&&*word==physical->base_.flags_40,
            "Normal56F630 child physical40 disagrees with paired common owner");
    }
    require(same(physical->base_.receiver_owned,child)&&same(physical->base_.parent_38.lock(),root),
        "Normal56F630 real ordered78/paired native38 association missing");
    const auto native_flags=physical->base_.flags_40;
    require(require_normal_root_or_idle_origin_without_callbacks(actual)==expected_busy,
        "Normal56F630 actual root lexical exclusion changed");
    return native_flags;
}

// Native first-control DATA capability. Strong forward custody + weak self;
// there is no Tree/Core/Gui backedge to this selection and no active loan alias.
struct detail::BnwNormalRootBootstrapSelection::Impl final {
    const std::thread::id creator=std::this_thread::get_id();
    const std::shared_ptr<const BnwWorldViewControlTree> tree;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t serial,topology,effects,layout,sequence;
    const std::shared_ptr<const NotificationLayoutContextInitializationReturn> completion;
    const std::shared_ptr<NotificationLayoutContextOrdinaryState> context;
    const std::shared_ptr<const NotificationLayoutOrdinaryReceiver> root,selected;
    const std::shared_ptr<NotificationLayoutOrdinaryChildren78> children;
    const std::vector<std::shared_ptr<const NotificationLayoutOrdinaryReceiver>> ordered;
    const std::vector<std::uint32_t> prefix_flags;
    const std::array<std::uint32_t,2> root_offset;
    const std::uint32_t root_alpha,root_flags;
    const std::uint64_t control;
    const std::size_t ordinal,count;
    std::weak_ptr<const Impl> canonical_self;
    bool sealed=false;
    Impl(std::shared_ptr<const BnwWorldViewControlTree> actual_tree,
        const runtime::BnwWorldViewSourceOperationLoan* actual_operation,std::uint64_t actual_serial,
        std::uint64_t actual_topology,std::uint64_t actual_effects,std::uint64_t actual_layout,std::uint64_t actual_sequence,
        std::shared_ptr<const NotificationLayoutContextInitializationReturn> actual_completion,
        std::shared_ptr<NotificationLayoutContextOrdinaryState> actual_context,
        std::shared_ptr<const NotificationLayoutOrdinaryReceiver> actual_root,
        std::shared_ptr<const NotificationLayoutOrdinaryReceiver> actual_selected,
        std::shared_ptr<NotificationLayoutOrdinaryChildren78> actual_children,
        std::vector<std::shared_ptr<const NotificationLayoutOrdinaryReceiver>> actual_ordered,
        std::vector<std::uint32_t> actual_flags,std::array<std::uint32_t,2> actual_root_offset,
        std::uint32_t actual_root_alpha,std::uint32_t actual_root_flags,
        std::uint64_t actual_control,std::size_t actual_ordinal,std::size_t actual_count)
        :tree(std::move(actual_tree)),operation(actual_operation),serial(actual_serial),
         topology(actual_topology),effects(actual_effects),layout(actual_layout),sequence(actual_sequence),
         completion(std::move(actual_completion)),context(std::move(actual_context)),root(std::move(actual_root)),
         selected(std::move(actual_selected)),children(std::move(actual_children)),ordered(std::move(actual_ordered)),
         prefix_flags(std::move(actual_flags)),root_offset(actual_root_offset),root_alpha(actual_root_alpha),
         root_flags(actual_root_flags),control(actual_control),ordinal(actual_ordinal),count(actual_count){}
    static bool same(const auto&a,const auto&b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);}
    void first()const{BnwWorldViewControlTree::require_normal_root_bootstrap_selection_current_without_callbacks(*this);}
    void full(const runtime::BnwWorldViewSourceOperationLoan& supplied)const{
        first();require(&supplied==operation,"Native prepaint selection another supplied Source");
        supplied.require_operation(serial);first();
        const auto owner=tree->core_;const auto source=owner->source;
        source->require_sources_current();first();supplied.require_current();first();
        supplied.require_operation(serial);first();
    }
};

void BnwWorldViewControlTree::require_normal_root_bootstrap_selection_current_without_callbacks(
    const detail::BnwNormalRootBootstrapSelection::Impl& selected){
    using Selection=detail::BnwNormalRootBootstrapSelection;
    require(selected.creator==std::this_thread::get_id()&&selected.sealed&&selected.tree&&
        selected.canonical_self.lock().get()==&selected&&selected.operation&&selected.serial&&selected.control&&
        selected.ordinal<selected.count&&selected.ordered.size()==selected.count&&
        selected.prefix_flags.size()==selected.ordinal+1,
        "Native bootstrap selection foreign/unsealed or missing original control");
    const auto actual=selected.tree->core_;
    const bool expected_busy=require_normal_root_or_idle_origin_without_callbacks(actual);
    const auto& fresh=actual->source->current_operation_loan();
    require(&fresh==selected.operation,"Native selection current Source differs before saved operation");
    fresh.require_origin_without_callbacks();require(fresh.operation_serial()==selected.serial,
        "Native selection immutable lexical Source serial changed");
    // These checks allow authentic active Root here. Absence was required
    // ONLY while minting the original prepaint DATA capability.
    require_normal_root_receiver_before_common_without_callbacks(actual,fresh,selected.serial);
    const auto same=[](const auto&a,const auto&b){return Selection::Impl::same(a,b);};
    const auto& base=*actual->context_layout_baseline;
    require(actual->topology_revision==selected.topology&&actual->effect_revision==selected.effects&&
        actual->layout_sequence==selected.layout&&base.sequence_==selected.sequence&&
        same(actual->layout_initialization_return->native_return_,selected.completion)&&
        same(actual->context_ordinary_state,selected.context)&&same(base.base_.receiver_owned,selected.root)&&
        same(base.base_.children_78,selected.children)&&selected.children&&selected.children->impl_&&
        selected.children->impl_->ordered.size()==selected.count&&base.base_.alpha_word_44&&
        *base.base_.alpha_word_44==selected.root_alpha&&base.base_.flags_40==selected.root_flags&&
        selected.root_offset==std::array<std::uint32_t,2>{std::bit_cast<std::uint32_t>(base.base_.offset_28_2c[0]),
            std::bit_cast<std::uint32_t>(base.base_.offset_28_2c[1])},
        "Native selection original completed Root/ordered78/physical common state changed");
    for(std::size_t i=0;i<selected.count;++i)
        require(same(selected.ordered[i],selected.children->impl_->ordered[i]),
            "Native selection actual ordered78 receiver control was reordered or replaced");
    for(std::size_t i=0;i<=selected.ordinal;++i){
        const auto flags=require_normal_root_ordered_child_flags_without_callbacks(
            actual,selected.completion,selected.root,selected.ordered[i]);
        require(flags==selected.prefix_flags[i],"Native selection actual physical40 changed after capture");
        require(i==selected.ordinal?!(flags&0x2002u):bool(flags&0x2002u),
            "Native selection genuine first-visible mask2002 branch changed");
    }
    const auto receiver=selected.selected->impl_;
    require(same(selected.ordered[selected.ordinal],selected.selected)&&receiver&&
        receiver->id==selected.control&&same(receiver->core.lock(),actual)&&receiver->standalone_label.lock(),
        "Native first visible receiver requires its actual non-Label or foreign-domain dispatcher");
    require(require_normal_root_or_idle_origin_without_callbacks(actual)==expected_busy,
        "Native selection actual root lexical exclusion changed");
    fresh.require_origin_without_callbacks();
    require(&actual->source->current_operation_loan()==selected.operation&&fresh.operation_serial()==selected.serial,
        "Native selection terminal pure Source/control cut changed");
}

detail::BnwNormalRootBootstrapSelection::BnwNormalRootBootstrapSelection(
    std::shared_ptr<const Impl> actual,const IssuerKey&):impl_(std::move(actual)){}
void detail::BnwNormalRootBootstrapSelection::require_current_without_callbacks()const{
    const auto actual=impl_;actual->first();
}
void detail::BnwNormalRootBootstrapSelection::require_source_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto actual=impl_;actual->full(operation);actual->first();
}
std::uint64_t detail::BnwNormalRootBootstrapSelection::actual_control_without_callbacks()const{
    const auto actual=impl_;actual->first();const auto id=actual->control;actual->first();return id;
}
void detail::BnwNormalRootBootstrapSelection::require_actual_publication_without_callbacks(
    const BnwWorldViewControlRenderPublication& publication)const{
    const auto actual=impl_;actual->first();
    BnwWorldViewControlTree::require_normal_root_bootstrap_selection_publication_without_callbacks(*actual,publication);
    actual->first();
}
void BnwWorldViewControlTree::require_normal_root_bootstrap_selection_publication_without_callbacks(
    const detail::BnwNormalRootBootstrapSelection::Impl& selection,
    const BnwWorldViewControlRenderPublication& publication){
    selection.first();const auto original=publication.original_owner_without_callbacks();
    require(original.get()==&publication&&detail::BnwNormalRootBootstrapSelection::Impl::same(original->tree_,selection.tree)&&
        original->original_owner_operation_==selection.operation&&original->original_owner_source_serial_&&
        *original->original_owner_source_serial_==selection.serial&&
        original->actual_checked_control_id_for_paint_without_callbacks()==selection.control,
        "Native prepaint selection is not SAME original Publication/Tree/control/Source");
    selection.first();
}

detail::BnwNormalRootBootstrapSelection BnwWorldViewControlTree::capture_normal_root_bootstrap_selection_owned(
    std::shared_ptr<const BnwWorldViewControlTree> same_tree,
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    using Selection=detail::BnwNormalRootBootstrapSelection;
    const auto tree=std::move(same_tree);const auto actual=core_;
    require(tree&&tree.get()==this&&tree->core_==actual,"Native prepaint selection requires SAME actual Tree custody");
    actual->layout_origin();const auto& fresh=actual->source->current_operation_loan();
    require(&fresh==&operation,"Native prepaint capture fresh Source differs before supplied operand");
    fresh.require_origin_without_callbacks();const auto serial=fresh.operation_serial();
    const auto first=[&]{
        actual->layout_origin();require(tree->core_==actual&&!actual->busy,
            "Native first-control capture must precede original paint");
        const auto& current=actual->source->current_operation_loan();
        require(&current==&operation,"Native first-control capture Source was replaced");
        current.require_origin_without_callbacks();require(current.operation_serial()==serial,
            "Native first-control capture Source serial changed");
        require_normal_root_receiver_before_common_without_callbacks(actual,current,serial);
        const auto gui=actual->context_gui_port;require(bool(gui),"Native prepaint selection canonical Gui absent");
        const auto manager=gui->impl_;manager->first(current);const auto preceding=manager->normal_gui_batch_prefix.lock();
        require(!manager->normal_gui_batch_beginning&&!manager->normal_root_history_capturing&&
            !manager->windowed_matrix_busy&&manager->normal_root_history.expired()&&
            (!preceding||!preceding->alive.load()),
            "Native prepaint selection cannot be minted after active Root/Prefix epoch begins");
    };
    const auto full=[&]{first();actual->source->require_sources_current();first();operation.require_current();first();};
    first();full();first();
    const auto completion=actual->layout_initialization_return->native_return_;
    const auto context=actual->context_ordinary_state;
    const auto& original_base=*actual->context_layout_baseline;
    const auto root=original_base.base_.receiver_owned;const auto children=original_base.base_.children_78;
    const auto topology=actual->topology_revision,effects=actual->effect_revision,layout=actual->layout_sequence,
        sequence=original_base.sequence_;
    const std::array<std::uint32_t,2> offset{std::bit_cast<std::uint32_t>(original_base.base_.offset_28_2c[0]),
        std::bit_cast<std::uint32_t>(original_base.base_.offset_28_2c[1])};
    const auto alpha=*original_base.base_.alpha_word_44,flags=original_base.base_.flags_40;
    const auto count=children?children->impl_->ordered.size():0;
    require(children&&count,"Native prepaint bootstrap has no actual ordered78 child");
    try{
        std::vector<std::shared_ptr<const detail::NotificationLayoutOrdinaryReceiver>> ordered;
        std::vector<std::uint32_t> prefix_flags;
        const auto pending=[&]{first();const auto& now=*actual->context_layout_baseline;
            require(Selection::Impl::same(actual->layout_initialization_return->native_return_,completion)&&
                Selection::Impl::same(actual->context_ordinary_state,context)&&actual->topology_revision==topology&&
                actual->effect_revision==effects&&actual->layout_sequence==layout&&now.sequence_==sequence&&
                Selection::Impl::same(now.base_.receiver_owned,root)&&Selection::Impl::same(now.base_.children_78,children)&&
                children->impl_->ordered.size()==count&&now.base_.alpha_word_44&&*now.base_.alpha_word_44==alpha&&
                now.base_.flags_40==flags&&offset==std::array<std::uint32_t,2>{
                    std::bit_cast<std::uint32_t>(now.base_.offset_28_2c[0]),std::bit_cast<std::uint32_t>(now.base_.offset_28_2c[1])},
                "Native prepaint lookup allocation/FULL POST changed actual Root inputs");
        };
        pending();full();pending();ordered.reserve(count);pending();full();pending();
        prefix_flags.reserve(count);pending();full();pending();
        // Capacity already earned; no borrowed vector/ref crosses allocation.
        for(std::size_t i=0;i<count;++i)ordered.push_back(children->impl_->ordered[i]);
        const auto order_current=[&]{pending();require(ordered.size()==count,"Native prepaint owned order incomplete");
            for(std::size_t i=0;i<count;++i)require(Selection::Impl::same(ordered[i],children->impl_->ordered[i]),
                "Native prepaint actual ordered78 member/control changed");};
        order_current();
        std::optional<std::size_t> ordinal;
        for(std::size_t i=0;i<count;++i){
            order_current();const auto physical=require_normal_root_ordered_child_flags_without_callbacks(actual,completion,root,ordered[i]);
            prefix_flags.push_back(physical); //Reserved OWN scalar copy, no effect on native cursor.
            if(!(physical&0x2002u)){ordinal=i;break;}
        }
        require(ordinal.has_value(),"Native prepaint Root has no visible receiver for bootstrap");
        const auto selected=ordered[*ordinal];const auto receiver=selected->impl_;
        require(receiver&&receiver->id&&Selection::Impl::same(receiver->core.lock(),actual)&&
            receiver->standalone_label.lock(),
            "Native first visible receiver requires genuine Context/non-Label/foreign dispatcher");
        const auto control=receiver->id;
        const auto selected_current=[&]{order_current();require(prefix_flags.size()==*ordinal+1&&
            selected==ordered[*ordinal]&&selected->impl_==receiver&&receiver->id==control,
            "Native selected first receiver identity changed during lookup");
            for(std::size_t i=0;i<=*ordinal;++i)require(prefix_flags[i]==
                require_normal_root_ordered_child_flags_without_callbacks(actual,completion,root,ordered[i]),
                "Native prepaint prefix physical40 changed during FULL/allocation");};
        selected_current();full();selected_current();
        // Every allocator argument is an OWN value. Moved vectors are checked
        // through SAME new owned control, never dereferenced after move.
        auto owned=std::make_shared<Selection::Impl>(tree,&operation,serial,topology,effects,layout,sequence,
            completion,context,root,selected,children,std::move(ordered),std::move(prefix_flags),offset,alpha,flags,
            control,*ordinal,count);
        pending();full();pending();
        require(owned&&owned->ordered.size()==count&&owned->prefix_flags.size()==*ordinal+1&&
            owned->selected==selected&&owned->ordinal==*ordinal&&owned->control==control&&!owned->sealed,
            "Native first-control allocation POST changed actual captured arguments");
        for(std::size_t i=0;i<count;++i)require(Selection::Impl::same(owned->ordered[i],children->impl_->ordered[i]),
            "Native first-control allocation POST changed actual ordered78");
        for(std::size_t i=0;i<=*ordinal;++i)require(owned->prefix_flags[i]==
            require_normal_root_ordered_child_flags_without_callbacks(actual,completion,root,owned->ordered[i]),
            "Native first-control allocation POST changed physical40");
        owned->canonical_self=owned;owned->sealed=true;owned->first();owned->full(operation);owned->first();
        const Selection::IssuerKey key;return Selection(owned,key);
    }catch(...){actual->failed.store(true);throw;}
}
