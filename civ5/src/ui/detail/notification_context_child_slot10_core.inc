// Include inside namespace civ5::ui AFTER complete ContextInitializationFrame
// and the genuine Parent/Scroll/resource Core bridges. Button's separate inc
// is included after this complete Frame; its declaration is a mandatory body.
struct detail::NotificationLayoutChildSlot10Completion::Impl {
    using Tree=BnwWorldViewControlTree;
    struct Lifetime {
        const std::thread::id creator=std::this_thread::get_id();
        std::atomic<bool> failed{false};
        bool destroyed=false,destruction_passed=false;
    };
    struct Domain {std::weak_ptr<Tree::Core> core;std::uint64_t topology,before_effects,effects;};
    struct Member {std::uint64_t id,parser_serial,initialized_sequence;};
    // Actual nondeferred Context+10 outputs. This is part of the existing
    // child completion, captured by its real producer; never a DirectReturn.
    struct ContextHistory {
        std::shared_ptr<NotificationLayoutContextOrdinaryState> state;
        std::shared_ptr<const NotificationLayoutContextConstructionReturn> constructor;
        std::shared_ptr<const NotificationContextSlot10LoadOrigin> load;
        std::shared_ptr<const NotificationLayoutContextInitializationReturn> initialized;
        std::shared_ptr<const NotificationContextLuaStatePreparation> preparation;
        std::shared_ptr<const NotificationContextCursorLoadPrefixReturn> prefix;
        std::shared_ptr<const NotificationContextInitialLuaGcReturn> gc;
        std::shared_ptr<const NotificationContextCursorRestoreReturn> cursor;
        std::shared_ptr<const NotificationContextLoaderCleanupReturn> cleanup;
        const runtime::LuaScriptHost* host;
    };
    const std::weak_ptr<Tree::Core> parent,child;
    const std::uint64_t child_id,child_order,source_serial;
    const std::vector<Domain> domains;
    const std::vector<Member> members;
    const std::vector<Tree::Core::StackPending> stack_results;
    const std::shared_ptr<Lifetime> lifetime;
    const std::optional<ContextHistory> context_history;
    Impl(const std::shared_ptr<Tree::Core>& p,const std::shared_ptr<Tree::Core>& c,
        std::uint64_t id,std::uint64_t order,std::uint64_t serial,
        std::vector<Domain> d,std::vector<Member> m,std::vector<Tree::Core::StackPending> stacks,std::shared_ptr<Lifetime> live,
        std::optional<ContextHistory> actual_context)
        :parent(p),child(c),child_id(id),child_order(order),source_serial(serial),
         domains(std::move(d)),members(std::move(m)),stack_results(std::move(stacks)),lifetime(std::move(live)),
         context_history(std::move(actual_context)){}
};

struct BnwWorldViewControlTree::ContextChildSlot10Frame {
    using Parent=detail::NotificationLayoutParentReceiver;
    using View=Parent::TreeView;
    using Key=Parent::NodeKey;
    using Baseline=detail::NotificationLayoutBaseline;
    using Completion=detail::NotificationLayoutChildSlot10Completion;
    struct Domain {std::shared_ptr<Core> core;std::uint64_t topology,before_effects,effects;};
    const std::thread::id creator=std::this_thread::get_id();
    ContextInitializationFrame& parent;
    const Key target;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t serial,order;
    std::vector<Domain> domains;
    std::map<std::uint64_t,Completion::Impl::Member> members;
    std::vector<Key> active_path,finished;
    std::vector<Core::StackPending> stack_results;
    std::vector<Key> empty_stacks;
    struct ReceiverDestructionReturn final {
        std::uint64_t domain,id,source_serial,topology,effects,actual_return_sequence;
    };
    std::optional<ReceiverDestructionReturn> receiver_destruction_return;
    std::uint64_t receiver_return_sequence=0;
    const std::shared_ptr<Completion::Impl::Lifetime> lifetime=std::make_shared<Completion::Impl::Lifetime>();
    const int exceptions=std::uncaught_exceptions();
    ContextChildSlot10Frame(ContextInitializationFrame& outer,Key key)
        :parent(outer),target(key),operation(outer.operation),serial(outer.serial),order([&]{
            outer.pure();require(key.domain<outer.domains.size(),"Child+10 actual domain absent");
            const auto& c=outer.domains.at(key.domain).core;
            if(key.id)return Core::node(c->nodes,key.id).attached_order;
            require(outer.selected->context_layout_children.has_value(),"Child+10 actual attachment list absent");
            std::uint64_t result=0;
            for(const auto& x:*outer.selected->context_layout_children){const auto a=x.core.lock();
                if(a&&a.get()==c.get()&&!a.owner_before(c)&&!c.owner_before(a)){
                    require(!result&&x.parent_control_id==0,"Child+10 actual Context attachment repeated");result=x.attached_order;}}
            return result;}()) {
        parent.first();require(order!=0,"Child+10 actual attachment order unearned");
        for(const auto& d:outer.domains)domains.push_back({d.core,d.topology,d.effects,d.effects});
        pure();
    }
    void poison()noexcept {
        lifetime->failed.store(true);parent.completion->failed.store(true);
        parent.selected->failed.store(true);for(const auto& d:domains)d.core->failed.store(true);
    }
    void first()const {
        if(creator!=std::this_thread::get_id()){
            lifetime->failed.store(true);parent.selected->failed.store(true);
            require(false,"Child+10 foreign creator before mutable/borrowed state");
        }
        parent.first();for(const auto& d:domains)d.core->layout_origin();
    }
    // Only a private actual Binding capture, before its first owned store,
    // supplies a new domain preimage. No after-callback snapshot can do so.
    void admit_binding_domain_owned(const View::Binding::Domain& d){
        const auto& actual=d.core;actual->layout_origin();
        require(actual->source.get()==parent.selected->source.get()&&
            !actual->source.owner_before(parent.selected->source)&&!parent.selected->source.owner_before(actual->source),
            "Child+10 new domain is not SAME canonical Source owner");
        for(const auto& x:domains)if(x.core.get()==actual.get()){
            require(!x.core.owner_before(actual)&&!actual.owner_before(x.core)&&x.topology==d.topology&&x.effects==d.before_effects,
                "Child+10 genuine Binding preimage differs from tracked domain");return;}
        require(actual->topology_revision==d.topology&&actual->effect_revision==d.effects,
            "Child+10 closed new domain changed before admission");
        domains.push_back({actual,d.topology,d.before_effects,d.effects});
    }
    std::size_t domain_of(const std::shared_ptr<Core>& core)const {
        for(std::size_t i=0;i<domains.size();++i)if(domains[i].core.get()==core.get()&&
            !domains[i].core.owner_before(core)&&!core.owner_before(domains[i].core))return i;
        require(false,"Child+10 new domain needs genuine pre-prefix typed admission");return 0;
    }
    void pure()const {
        first();require(!lifetime->failed.load()&&!lifetime->destroyed,"Child+10 occurrence retired/failed");
        require(&parent.selected->source->current_operation_loan()==operation&&operation->operation_serial()==serial,
            "Child+10 actual SAME Source operation substituted");first();
        for(const auto& d:domains){require(d.core->topology_revision==d.topology&&d.core->effect_revision==d.effects,
            "Child+10 current epoch drift outside actual owned stage");d.core->require_layout_resources_owned();first();}
    }
    void full(){pure();parent.selected->source->require_sources_current();first();pure();}
    Baseline& baseline_owned(Key key)const {
        pure();const auto& c=domains.at(key.domain).core;
        if(!key.id){require(c->context_layout_baseline.has_value(),"Child+10 genuine Context baseline absent");return *c->context_layout_baseline;}
        const auto& n=Core::node(c->nodes,key.id);require(n.layout_baseline.has_value(),"Child+10 genuine subtype baseline absent");
        return *c->nodes.at(key.id-1).layout_baseline;
    }
    // Defined closed mutation boundary for actual internal typed receiver calls.
    // It never accepts a supplied success value or submits numeric observations.
    template<class F>void with_receiver_owned(std::uint64_t domain,std::uint64_t id,const F& actual){
        using Target=std::decay_t<F>;
        pure();const auto core=domains.at(domain).core;std::shared_ptr<View::Binding> binding;
        std::vector<Parent::FourCReturn> returned;
        std::vector<Core::StackPending> actual_stacks;
        try{
            core->busy=true;
            {struct Exit{bool& value;~Exit(){value=false;}}exit{core->busy};
             binding=std::make_shared<View::Binding>(core,*operation);
             // Existing entry domains must match before any numerical prefix.
             for(const auto& d:binding->domains){const auto at=domain_of(d.core);
                 require(d.topology==domains[at].topology&&d.effects==domains[at].effects,
                     "Child+10 actual View entry source/epoch changed");}
             {
                View view(binding,binding->captured);view.require_pure();
                std::optional<Target> copied;
                try{
                    copied.emplace(actual);binding->first(true);view.require_pure();
                    (*copied)(view,Key{0,id});binding->first(true);view.require_pure();
                    copied.reset();binding->first(true);view.require_pure();
                    view.require_full();binding->first(true);view.require_pure();
                    returned=view.four_c_returns_;view.require_pure();
                    actual_stacks=Core::capture_stack_view_owned(view);view.require_pure();
                }catch(...){binding->poison();const auto error=std::current_exception();copied.reset();
                    binding->first(true);std::rethrow_exception(error);}
             } // Actual View DTOR before current field reuse/publication.
             binding->pure(false);require(binding->destruction_passed&&!binding->graph_active,
                 "Child+10 receiver target destruction refused");
            }
            first();
            for(const auto& d:binding->domains){
                admit_binding_domain_owned(d);const auto at=domain_of(d.core);
                require(domains[at].topology==d.topology&&d.core->effect_revision==d.effects,
                    "Child+10 genuine View post-DTOR history changed");domains[at].effects=d.effects;}
            pure();
            Core::merge_stack_results_owned(stack_results,std::move(actual_stacks));
            for(const auto& stamp:returned){const auto& d=binding->domains.at(stamp.key.domain);
                const Core::LayoutFourCReturn value{stamp.actual_offset,stamp.baseline_sequence,serial,d.topology,stamp.offset_store_sequence};
                if(!stamp.key.id)d.core->context_layout_four_c_return=value;
                else {Core::node(d.core->nodes,stamp.key.id);d.core->nodes.at(stamp.key.id-1).layout_four_c_return=value;}}
            pure();
            //Earned DATA only after real View DTOR/binding destruction_passed,
            //adoption and all currentness checks. Binding storage is NOT kept.
            require(binding->destruction_passed&&!binding->graph_active&&receiver_return_sequence<UINT64_MAX,
                "Child+10 real receiver destruction return missing/overflow");
            const auto& actual_domain=domains.at(domain);
            receiver_destruction_return=ReceiverDestructionReturn{domain,id,serial,actual_domain.topology,
                actual_domain.effects,++receiver_return_sequence};pure();
        }catch(...){if(binding)binding->poison();poison();throw;}
        // Binding dies before the next View; no stale busy release overlaps it.
    }
    std::vector<Key> children_owned(Key key){
        std::vector<Key> result;
        with_receiver_owned(key.domain,key.id,[&](View& view,Key local){
            for(const auto child:Parent::children(view,local)){
                const auto& c=view.binding_->domains.at(child.domain).core;
                auto at=std::find_if(domains.begin(),domains.end(),[&](const auto& d){return d.core.get()==c.get();});
                if(at==domains.end()){const auto& d=view.binding_->domains.at(child.domain);
                    require(d.before_effects==d.effects,"Child+10 enumeration domain already mutated before admission");
                    domains.push_back({c,d.topology,d.before_effects,d.effects});at=std::prev(domains.end());}
                else require(!at->core.owner_before(c)&&!c.owner_before(at->core),"Child+10 child Core control block differs");
                result.push_back({static_cast<std::uint64_t>(at-domains.begin()),child.id});}
        });pure();return result;
    }
    void record_member_owned(Key key){
        pure();auto& b=baseline_owned(key);require(b.parsed_source_serial_&&*b.parsed_source_serial_!=0,
            "Child+10 actual completed parser history absent");
        require(std::find(finished.begin(),finished.end(),key)==finished.end(),"Child+10 pertype completion replayed");
        finished.push_back(key);
        for(auto& result:stack_results){const auto actual=result.core.lock();
            if(actual==domains.at(key.domain).core&&result.id==key.id){result.member_finished=true;
                result.empty_initializer=std::find(empty_stacks.begin(),empty_stacks.end(),key)!=empty_stacks.end();}}

        if(key.domain==0&&key.id)require(members.emplace(key.id,Completion::Impl::Member{
            key.id,*b.parsed_source_serial_,b.sequence_}).second,"Child+10 ordinary member completion repeated");
    }
    void adopt_context_initializer_owned(const std::shared_ptr<Core>& actual,
        const std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn>& result){
        first();require(result&&result->impl_,"Child Context5761 genuine return absent");
        const auto& done=*result->impl_;const auto owner=done.core.lock();
        require(owner&&owner.get()==actual.get()&&!owner.owner_before(actual)&&!actual.owner_before(owner)&&
            done.source_serial==serial&&done.lifetime&&done.lifetime->creator==creator&&
            !done.lifetime->failed.load()&&done.lifetime->destroyed&&done.lifetime->destruction_passed,
            "Child Context5761 actual newly completed occurrence differs");
        for(const auto& d:done.domains){const auto c=d.core.lock();require(bool(c),"Child Context5761 returned domain retired");
            c->layout_origin();auto found=std::find_if(domains.begin(),domains.end(),[&](const auto& x){return x.core.get()==c.get();});
            if(found==domains.end()){
                require(c->source.get()==parent.selected->source.get()&&
                    !c->source.owner_before(parent.selected->source)&&!parent.selected->source.owner_before(c->source),
                    "Child Context5761 returned foreign Source domain");
                domains.push_back({c,d.topology,d.before_effects,d.effects});
            }else require(!found->core.owner_before(c)&&!c.owner_before(found->core)&&found->effects==d.before_effects,
                "Child Context5761 sealed domain preimage differs");
            const auto at=domain_of(c);require(d.topology==domains[at].topology&&
                c->topology_revision==d.topology&&c->effect_revision==d.effects,
                "Child Context5761 actual post-complete domain differs");domains[at].effects=d.effects;}
        pure();require_context_initialization_return_owned(actual,result);first();pure();
    }
    void adopt_context_initial_lua_mutations_owned(const std::shared_ptr<Core>&,
        const std::shared_ptr<const detail::NotificationContextInitialLuaGcReturn>&);
    ~ContextChildSlot10Frame()noexcept {
        if(creator!=std::this_thread::get_id()){lifetime->failed.store(true);parent.selected->failed.store(true);return;}
        try{pure();require(active_path.empty()&&std::uncaught_exceptions()==exceptions,
            "Child+10 incomplete body/unwinding at destruction");lifetime->destruction_passed=true;}
        catch(...){poison();}
        lifetime->destroyed=true;
    }
};

std::shared_ptr<BnwWorldViewControlTree>
BnwWorldViewControlTree::retain_context_slot10_tree_owned(ContextChildSlot10Frame& frame,
    const std::shared_ptr<const LayoutContextSlot10LoadOrigin>& origin){
    frame.pure();require(origin&&origin->impl_,"Context457270 authentic typed load origin absent");
    const auto actual=origin->impl_->core.lock();require(bool(actual),"Context457270 loaded Core retired");
    frame.first();const auto domain=frame.domain_of(actual);
    const auto checked=require_context_slot10_load_origin_owned(actual,*frame.operation);frame.first();frame.pure();
    require(checked.get()==origin.get()&&!checked.owner_before(origin)&&!origin.owner_before(checked),
        "Context457270 actual prefix receipt replaced");
    const auto retained=actual->layout_resource_tree.lock();
    require(retained&&retained.get()==actual->layout_resource_actual_tree&&
        retained->core_.get()==actual.get()&&!retained->core_.owner_before(actual)&&!actual.owner_before(retained->core_)&&
        actual->layout_resource_operation==frame.operation&&actual->layout_resource_serial==frame.serial&&
        frame.domains.at(domain).core.get()==actual.get(),"Context457270 canonical resource Tree absent/substituted");
    // Only private capture_owned constructs these Trees, as mutable objects.
    // ResourceScope's const view refers to that SAME object/control block.
    // No wrapper/second Tree/second Core is constructed by this conversion.
    auto tree=std::const_pointer_cast<BnwWorldViewControlTree>(retained);
    frame.full();require(tree.get()==actual->layout_resource_actual_tree&&
        tree->core_.get()==actual.get()&&!tree.owner_before(retained)&&!retained.owner_before(tree),
        "Context457270 canonical mutable factory Tree changed");
    return tree;
}

std::shared_ptr<const BnwWorldViewControlTree::LayoutContextSlot10LoadOrigin>
BnwWorldViewControlTree::prepare_context_slot10_load_owned(ContextChildSlot10Frame& frame,
    std::uint64_t domain,std::uint64_t id){
    frame.full();require(id==0,"Context457270 load receiver must be actual Context root");
    const auto actual=frame.domains.at(domain).core;
    require(actual->context_layout_baseline&&actual->context_layout_baseline->kind_==
        detail::NotificationLayoutBaseline::Kind::context&&!actual->context_load_origin,
        "Context457270 actual fresh parsed load occurrence absent/replayed");
    const auto sequence=actual->context_layout_baseline->sequence_;
    const auto before=frame.domains.at(domain).effects;
    const auto retained=actual->layout_resource_tree.lock();
    require(retained&&retained.get()==actual->layout_resource_actual_tree&&retained->core_.get()==actual.get()&&
        !retained->core_.owner_before(actual)&&!actual.owner_before(retained->core_)&&
        actual->layout_resource_operation==frame.operation&&actual->layout_resource_serial==frame.serial,
        "Context457270 actual canonical factory/resource Tree unavailable before prefix");
    const auto tree=std::const_pointer_cast<BnwWorldViewControlTree>(retained);
    try{
        // Root-owned body does actual deferred decision; nondeferred30,34,40.
        // Its Source/GUI callbacks are bracketed by saved Frame FIRST here.
        tree->prepare_context_slot10_load_origin_owned(*frame.operation);frame.first();
        const auto result=require_context_slot10_load_origin_owned(actual,*frame.operation);frame.first();
        require(result&&result->impl_,"Context457270 real prefix returned no opaque origin");
        const auto& load=*result->impl_;const auto owner=load.core.lock();
        require(owner&&owner.get()==actual.get()&&!owner.owner_before(actual)&&!actual.owner_before(owner)&&
            load.source_serial==frame.serial&&load.topology==frame.domains.at(domain).topology&&
            load.before_effects==before&&load.before_sequence==sequence&&
            load.after_effects==actual->effect_revision&&load.after_sequence==actual->context_layout_baseline->sequence_,
            "Context457270 genuine prefix preimage/owned history differs");
        // Advance only the already tracked actual Core through this sealed
        // real occurrence. No current-after snapshot creates its provenance.
        frame.domains.at(domain).effects=load.after_effects;frame.pure();frame.full();
        return result;
    }catch(...){frame.poison();throw;}
}

BnwWorldViewControlTree::LayoutContextSlot10LoadBranch
BnwWorldViewControlTree::require_context_slot10_load_origin_owned(ContextChildSlot10Frame& frame,
    const std::shared_ptr<const LayoutContextSlot10LoadOrigin>& origin){
    frame.pure();require(origin&&origin->impl_,"Context457270 branch receipt absent");
    const auto actual=origin->impl_->core.lock();require(bool(actual),"Context457270 branch receiver retired");
    frame.first();frame.domain_of(actual);
    const auto checked=require_context_slot10_load_origin_owned(actual,*frame.operation);frame.first();frame.pure();
    require(checked.get()==origin.get()&&!checked.owner_before(origin)&&!origin.owner_before(checked),
        "Context457270 genuine branch origin replaced");
    return (origin->impl_->flags&0x10000000U)?LayoutContextSlot10LoadBranch::deferred:
        LayoutContextSlot10LoadBranch::nondeferred;
}

void BnwWorldViewControlTree::initialize_control_5761_child_owned(
    ContextChildSlot10Frame& frame,std::uint64_t domain,std::uint64_t id){
    using Parent=detail::NotificationLayoutParentReceiver;using Baseline=detail::NotificationLayoutBaseline;
    const ContextChildSlot10Frame::Key key{domain,id};
    frame.with_receiver_owned(domain,id,[&](Parent::TreeView& view,Parent::NodeKey local){
        auto& state=*Parent::node(view,local).state;
        view.preflight_store_owned(local);state.record_owned(Baseline::Mutation::receiver_prefix);
        state.base_.flags_40|=1U;view.record_store_owned(local,Parent::Store::Kind::initializer_flags);
        Parent::slot4c(view,local);view.require_pure();auto& b=Parent::node(view,local).state->base_;
        const auto add=[](float a,float b){volatile float x=a+b;require(std::isfinite(x),"Child5761 FP32 bounds overflow");return x;};
        const std::array<float,2> extent{static_cast<float>(std::fabs(static_cast<long double>(b.margin_70_74[0]))+b.size_30_34[0]),
            static_cast<float>(std::fabs(static_cast<long double>(b.margin_70_74[1]))+b.size_30_34[1])};
        const std::array<float,4> bounds{b.offset_28_2c[0],b.offset_28_2c[1],add(b.offset_28_2c[0],b.size_30_34[0]),add(b.offset_28_2c[1],b.size_30_34[1])};
        for(float x:extent)require(std::isfinite(x),"Child5761 native extent unsupported");
        view.preflight_store_owned(local);Parent::node(view,local).state->record_owned(Baseline::Mutation::receiver_prefix);
        b.extent_20_24=extent;b.bounds_10_1c=bounds;view.record_store_owned(local,Parent::Store::Kind::bounds_union);
    });
    const auto children=frame.children_owned(key);
    for(const auto child:children){
        dispatch_context_child_slot10_body_owned(frame,child.domain,child.id);frame.first();frame.full();
        const auto bounds=frame.baseline_owned(child).base_.bounds_10_1c;
        frame.with_receiver_owned(domain,id,[&](Parent::TreeView& view,Parent::NodeKey local){
            auto& state=*Parent::node(view,local).state;auto current=state.base_.bounds_10_1c;
            for(float x:bounds)require(std::isfinite(x),"Child5761 actual returned bounds unsupported");
            current[0]=current[0]>bounds[0]?bounds[0]:current[0];current[1]=current[1]>bounds[1]?bounds[1]:current[1];
            current[2]=bounds[2]>current[2]?bounds[2]:current[2];current[3]=bounds[3]>current[3]?bounds[3]:current[3];
            view.preflight_store_owned(local);state.record_owned(Baseline::Mutation::receiver_prefix);
            state.base_.bounds_10_1c=current;view.record_store_owned(local,Parent::Store::Kind::bounds_union);
        });
    }
}


void BnwWorldViewControlTree::bind_scroll_slot10_callback_owned(
    ContextChildSlot10Frame& frame,std::uint64_t domain,std::uint64_t scroll_id,
    std::uint64_t handle_id,std::uint32_t index){
    using Parent=detail::NotificationLayoutParentReceiver;
    using Baseline=detail::NotificationLayoutBaseline;
    frame.full();require(index<3&&handle_id!=0,"Scroll7ADD60 callback index/receiver invalid");
    const auto actual_core=frame.domains.at(domain).core;
    const auto& scroll=frame.baseline_owned({domain,scroll_id});
    require(scroll.kind_==Baseline::Kind::scroll_panel&&scroll.scroll_&&scroll.scroll_->handles_c8_cc_d0&&
        (*scroll.scroll_->handles_c8_cc_d0)[index]==handle_id&&
        std::find(frame.finished.begin(),frame.finished.end(),Parent::NodeKey{domain,handle_id})!=frame.finished.end(),
        "Scroll7ADD60 genuine completed resolver handle absent");
    frame.with_receiver_owned(domain,handle_id,[&](Parent::TreeView& view,Parent::NodeKey key){
        view.require_pure();const auto& b=*Parent::node(view,key).state;
        require((index==0&&b.actual_type_=="UpButton"&&b.kind_==Baseline::Kind::button)||
            (index==1&&b.actual_type_=="DownButton"&&b.kind_==Baseline::Kind::button)||
            (index==2&&b.kind_==Baseline::Kind::slider),"Scroll7ADD60 callback receiver subtype differs");
        view.preflight_store_owned(key);
        auto& node=actual_core->nodes.at(handle_id-1);
        node.layout_scroll_callback=Core::LayoutScrollCallback{actual_core,scroll_id,
            static_cast<Core::LayoutScrollCallback::Kind>(index)};
        node.layout_baseline->record_owned(Baseline::Mutation::receiver_prefix);
        view.record_store_owned(key,Parent::Store::Kind::initializer_flags);
    });
    frame.first();frame.full();
    // Native callback targets6463A0/6E9AF0/478630 are fixed by index. This
    // private binding is not an invocation/GUI event/physical-input grant.
}

void BnwWorldViewControlTree::dispatch_context_child_slot10_body_owned(
    ContextChildSlot10Frame& frame,std::uint64_t domain,std::uint64_t id){
    using Parent=detail::NotificationLayoutParentReceiver;using Baseline=detail::NotificationLayoutBaseline;
    using Scroll=detail::NotificationLayoutScrollReceiver;using Key=Parent::NodeKey;
    const Key key{domain,id};frame.full();
    require(std::find(frame.active_path.begin(),frame.active_path.end(),key)==frame.active_path.end()&&
        std::find(frame.finished.begin(),frame.finished.end(),key)==frame.finished.end(),"Child+10 cycle/repeated actual receiver");
    frame.active_path.push_back(key);
    struct PathExit{std::vector<Key>& values;~PathExit(){values.pop_back();}}path{frame.active_path};
    const auto kind=frame.baseline_owned(key).kind_;
    if(kind==Baseline::Kind::context){
        require(id==0,"Child+10 Context must be actual canonical Context root");
        // The genuine required prepare adapter must consume its sealed load
        // prefix before returning: compare the tracked preimage and advance
        // only that real origin's owned epoch. No after-snapshot repair.
        const auto origin=prepare_context_slot10_load_owned(frame,domain,id);frame.first();frame.pure();
        const auto branch=require_context_slot10_load_origin_owned(frame,origin);frame.first();frame.pure();
        if(branch==LayoutContextSlot10LoadBranch::nondeferred){
            const auto tree=retain_context_slot10_tree_owned(frame,origin);frame.first();frame.pure();
            const auto actual=frame.domains.at(domain).core;
            require(tree&&tree->core_.get()==actual.get()&&!tree->core_.owner_before(actual)&&!actual.owner_before(tree->core_)&&
                !actual->layout_initialization_return,"Child Context457270 actual new loader/tree occurrence absent");
            tree->initialize_context_5761_owned(*frame.operation);frame.first();
            const auto seal=actual->layout_initialization_return;
            require(seal&&seal->native_return_,"Child Context457270 returned without genuine5761 seal");
            frame.adopt_context_initializer_owned(actual,seal->native_return_);frame.full();
            execute_context_slot10_initial_lua_owned(frame,origin,seal->native_return_);frame.first();frame.full();
        }else require(branch==LayoutContextSlot10LoadBranch::deferred,"Child Context457270 unknown loader branch");
        // Deferred is a returned genuine load branch, never initialized/NULL.
        // Its real existing bounds are still unioned by the parent5761 body.
    }else if(kind==Baseline::Kind::stack){
        const auto& initial=frame.baseline_owned(key);require(initial.stack_.has_value(),"Stack4224D0 actual state absent");
        constexpr std::array<std::array<float,2>,4> directions{{{0,1},{0,-1},{-1,0},{1,0}}};
        const auto growth=initial.stack_->field_84,wrap=initial.stack_->field_88;
        require(growth<directions.size()&&wrap<directions.size(),"Stack4224D0 native direction index unsupported");
        const auto first=directions[growth],second=directions[wrap];
        initialize_control_5761_child_owned(frame,domain,id);frame.first();frame.full();
        const auto actual_children=frame.children_owned(key);
        if(actual_children.empty())frame.empty_stacks.push_back(key); //genuine closed enumeration, never XMLempty.
        if(!actual_children.empty())frame.with_receiver_owned(domain,id,[&](Parent::TreeView& view,Key local){
            Parent::stack_calculate(view,local);view.require_pure();Parent::reprocess_anchoring(view,local);view.require_pure();});
        frame.with_receiver_owned(domain,id,[&](Parent::TreeView& view,Key local){
            auto& state=*Parent::node(view,local).state;auto& stack=*state.stack_;const auto& b=state.base_;
            const auto mul=[](float a,float b){volatile float x=a*b;require(std::isfinite(x),"Stack4224D0 MULSS unsupported");return x;};
            const auto add=[](float a,float b){volatile float x=a+b;require(std::isfinite(x),"Stack4224D0 ADDSS unsupported");return x;};
            // Saved native direction operands precede5761; FP32 instruction order.
            const float along=add(mul(first[1],b.size_30_34[1]),mul(b.size_30_34[0],first[0]));
            const float across=add(mul(b.size_30_34[1],second[1]),mul(b.size_30_34[0],second[0]));
            auto origin=b.offset_28_2c;
            if(along<0){origin[0]=add(origin[0],mul(first[0],along));origin[1]=add(origin[1],mul(first[1],along));}
            if(stack.field_8c>0&&across<0){origin[0]=add(mul(second[0],across),origin[0]);origin[1]=add(mul(second[1],across),origin[1]);}
            view.preflight_store_owned(local);state.record_owned(Baseline::Mutation::receiver_prefix);
            stack.field_90=origin[0];stack.field_94=origin[1];view.record_store_owned(local,Parent::Store::Kind::stack_anchor);
        });
    }else if(kind==Baseline::Kind::scroll_panel){
        const auto& state=frame.baseline_owned(key);require(state.scroll_&&state.scroll_->handles_c8_cc_d0,
            "Scroll7ADD60 genuine resolver bindings absent");const auto handles=*state.scroll_->handles_c8_cc_d0;
        for(std::size_t n=0;n<handles.size();++n)if(handles[n]){
            dispatch_context_child_slot10_body_owned(frame,domain,handles[n]);frame.first();frame.full();
            // Root actual input/event receiver must retain these native callback
            // bindings from this real store occurrence; no boolean readiness.
            bind_scroll_slot10_callback_owned(frame,domain,id,handles[n],static_cast<std::uint32_t>(n));
            frame.first();frame.full();
        }
        initialize_control_5761_child_owned(frame,domain,id);frame.first();frame.full();
        frame.with_receiver_owned(domain,id,[&](Parent::TreeView& view,Key local){
            Scroll::internal_size(view,local.id);view.require_pure();Parent::reprocess_anchoring(view,local);view.require_pure();});
    }else if(kind==Baseline::Kind::slider){
        std::optional<Scroll::InitializationReturn> returned;
        frame.with_receiver_owned(domain,id,[&](Parent::TreeView& view,Key local){returned.emplace(Scroll::initialize_slider_owned(view,local.id));});
        frame.first();frame.pure();require(returned.has_value(),"Slider6D3180 genuine body return absent");
        Scroll::publish_slider_initialization_owned(*returned);frame.first();frame.pure();
        returned.reset();frame.first();frame.full();
    }else if(kind==Baseline::Kind::grid){
        initialize_grid_slot10_owned(frame,domain,id);frame.first();frame.full();
    }else if(kind==Baseline::Kind::button){
        initialize_button_slot10_owned(frame,domain,id);frame.first();frame.full();
    }else if(kind==Baseline::Kind::label){
        initialize_standalone_label_slot10_owned(frame,domain,id);frame.first();frame.full();
    }else{
        // No Base4C/94-to-slot10 inference. The real subtype body is a required
        // definition, conditional on actual pertype constructor/vtable authority.
        initialize_other_control_slot10_owned(frame,domain,id);frame.first();frame.full();
    }
    frame.record_member_owned(key);frame.full();
}

detail::NotificationLayoutChildSlot10Completion::NotificationLayoutChildSlot10Completion(
    std::shared_ptr<const Impl> actual):impl_(std::move(actual)){
    require(impl_&&impl_->lifetime&&impl_->lifetime->creator==std::this_thread::get_id(),"Child+10 completion creator absent");
    require(!impl_->lifetime->failed.load()&&impl_->lifetime->destroyed&&impl_->lifetime->destruction_passed,
        "Child+10 completion before genuine body/scope DTOR");
}

BnwWorldViewControlTree::LayoutChildSlot10Return::LayoutChildSlot10Return(
    const std::shared_ptr<Core>& parent,const std::shared_ptr<Core>& child,
    std::uint64_t id,const runtime::BnwWorldViewSourceOperationLoan& source,
    const std::shared_ptr<const detail::NotificationLayoutChildSlot10Completion>& result)
    :parent_(parent),child_(child),child_id_(id),child_order_([&]{
        require(bool(parent),"Child+10 return actual parent absent");parent->layout_origin();
        require(result&&result->impl_,"Child+10 actual opaque body completion absent");return result->impl_->child_order;}()),
     source_serial_(source.operation_serial()),members_([&]{std::vector<Member> values;
        for(const auto& x:result->impl_->members)values.push_back({x.id,x.parser_serial,x.initialized_sequence});return values;}()),
     domains_([&]{std::vector<Domain> values;for(const auto& d:result->impl_->domains)
         values.push_back({d.core,d.topology,d.before_effects,d.effects});return values;}()),completion_(result){
    parent->layout_origin();require(bool(child),"Child+10 return actual child absent");child->layout_origin();
    const auto& done=*result->impl_;const auto p=done.parent.lock(),c=done.child.lock();
    require(p&&c&&p.get()==parent.get()&&!p.owner_before(parent)&&!parent.owner_before(p)&&
        c.get()==child.get()&&!c.owner_before(child)&&!child.owner_before(c)&&done.child_id==id&&done.child_order!=0&&
        done.source_serial==source_serial_&&source_serial_!=0&&done.lifetime&&done.lifetime->creator==std::this_thread::get_id()&&
        !done.lifetime->failed.load()&&done.lifetime->destroyed&&done.lifetime->destruction_passed,
        "Child+10 return actual post-DTOR parent/receiver/source differs");
    require(&parent->source->current_operation_loan()==&source,"Child+10 return Source operation replaced");
    for(const auto& d:done.domains){const auto live=d.core.lock();require(bool(live),"Child+10 completion domain expired");
        live->layout_origin();require(live->topology_revision==d.topology&&live->effect_revision==d.effects,
            "Child+10 completion domain changed after scope DTOR");live->require_layout_resources_owned();
        parent->layout_origin();child->layout_origin();}
    source.require_operation(source_serial_);parent->layout_origin();child->layout_origin();
}

BnwWorldViewControlTree::LayoutChildSlot10Return
BnwWorldViewControlTree::initialize_context_child_slot10_owned(
    ContextInitializationFrame& parent,std::uint64_t domain,std::uint64_t id){
    using Completion=detail::NotificationLayoutChildSlot10Completion;
    parent.pure();std::shared_ptr<const Completion::Impl> completed;
    try{
        {
            ContextChildSlot10Frame frame(parent,{domain,id});
            dispatch_context_child_slot10_body_owned(frame,domain,id);frame.first();frame.full();
            std::vector<Completion::Impl::Domain> domains;std::vector<Completion::Impl::Member> members;
            for(const auto& d:frame.domains)domains.push_back({d.core,d.topology,d.before_effects,d.effects});
            // Local ordinary members belong only to the actual parent Context.
            // A nested Context's ordinary IDs never enter its parent's seal.
            if(domain==0)for(const auto& [key,m]:frame.members)members.push_back(m);
            std::optional<Completion::Impl::ContextHistory> context_history;
            if(id==0){
                const auto child=parent.domains.at(domain).core;
                const auto load=child->context_load_origin;
                require(child->context_layout_baseline&&child->context_layout_baseline->kind_==
                    detail::NotificationLayoutBaseline::Kind::context&&load&&load->impl_&&
                    load->impl_->core.lock()==child&&load->impl_->source_serial==frame.serial,
                    "Context+10 completion requires its actual returned load branch");
                if(!(load->impl_->flags&0x10000000u)){
                    const auto& host=parent.operation->context_lua_host_owned();frame.first();frame.full();
                    require(child->context_ordinary_state&&child->context_construction_return&&
                        child->layout_initialization_return&&child->layout_initialization_return->native_return_&&
                        child->context_lua_preparation&&child->context_cursor_load_prefix&&
                        child->context_initial_lua_gc_return&&child->context_cursor_restore_return&&
                        child->context_loader_cleanup_return&&!child->context_direct_load_return&&
                        !child->context_direct_load_pending_return,
                        "Context+10 completion lacks genuine5761/Lua/GC/cursor/cleanup outputs");
                    context_history.emplace(Completion::Impl::ContextHistory{child->context_ordinary_state,
                        child->context_construction_return,load,child->layout_initialization_return->native_return_,
                        child->context_lua_preparation,child->context_cursor_load_prefix,child->context_initial_lua_gc_return,
                        child->context_cursor_restore_return,child->context_loader_cleanup_return,&host});
                    frame.pure();
                } // Deferred+10 returns its real branch, without a load-ready history.
            }
            completed=std::make_shared<const Completion::Impl>(parent.selected,parent.domains.at(domain).core,
                id,frame.order,frame.serial,std::move(domains),std::move(members),std::move(frame.stack_results),frame.lifetime,
                std::move(context_history));
            frame.pure();
        } // Full actual child+10 body, copied targets and Frame DTOR completed.
        parent.first();require(completed&&completed->lifetime->creator==std::this_thread::get_id()&&
            !completed->lifetime->failed.load()&&completed->lifetime->destroyed&&completed->lifetime->destruction_passed,
            "Child+10 genuine scope destruction refused");
        parent.selected->source->require_sources_current();parent.first();
        const auto opaque=std::shared_ptr<const Completion>(new Completion(completed));
        // Only this post-DTOR publisher earns a subtype initialization occurrence.
        // BuildInstance/parser capture alone never writes this canonical member.
        if(domain==0)for(const auto& member:completed->members){
            auto& node=parent.selected->nodes.at(member.id-1);
            require(node.layout_baseline&&node.layout_baseline->parsed_source_serial_&&
                *node.layout_baseline->parsed_source_serial_==member.parser_serial&&
                node.layout_baseline->sequence_>=member.initialized_sequence,
                "Button subtype post-DTOR canonical parser/sequence changed");
            if(node.layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::button){
                require(!node.button_slot10_return,"Button genuine subtype occurrence replayed");
                node.button_slot10_return=opaque;
            }
        }
        auto returned=LayoutChildSlot10Return(parent.selected,parent.domains.at(domain).core,id,*parent.operation,opaque);
        if(id==0){
            const auto child=parent.domains.at(domain).core;
            parent.first();require(!child->context_child_slot10_return,
                "Context+10 actual post-DTOR completion publication replayed");
            child->context_child_slot10_return=opaque;
            parent.first();
        }
        return returned;
    }catch(...){parent.poison();throw;}
}

// A data clone's canonical Button state does not imply that its native +10
// initializer returned. Source4/5 positive registration must pass this query.
void BnwWorldViewControlTree::require_unitflag_button_slot10_return_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,std::uint64_t id,std::uint64_t root)const{
    const auto saved=core_;saved->layout_origin();operation.require_origin_without_callbacks();saved->layout_origin();
    require(saved->context=="UnitFlagManager"&&&saved->source->current_operation_loan()==&operation,
        "UnitFlag subtype query actual Core/context/Source differs");
    const auto& node=Core::node(saved->nodes,id);const auto& anchor=Core::node(saved->nodes,root);
    require(node.alive&&node.dynamic&&node.instance==root&&anchor.alive&&anchor.dynamic&&anchor.instance==root&&
        node.layout_receiver_occurrence&&node.layout_baseline&&node.layout_baseline->button_&&
        node.layout_baseline->kind_==detail::NotificationLayoutBaseline::Kind::button&&
        node.layout_baseline->actual_type_=="Button"&&node.button_slot10_return&&node.button_slot10_return->impl_,
        "UnitFlag dynamic Button requires genuine constructor/parser/+10 body return; template data capture is insufficient");
    const auto& done=*node.button_slot10_return->impl_;const auto owner=done.parent.lock();
    require(owner&&owner.get()==saved.get()&&!owner.owner_before(saved)&&!saved.owner_before(owner)&&
        done.source_serial==operation.operation_serial()&&done.lifetime&&done.lifetime->creator==saved->creator&&
        !done.lifetime->failed.load()&&done.lifetime->destroyed&&done.lifetime->destruction_passed,
        "UnitFlag Button subtype return is not SAME post-DTOR native occurrence");
    const auto member=std::find_if(done.members.begin(),done.members.end(),[&](const auto& x){return x.id==id;});
    require(member!=done.members.end()&&node.layout_baseline->parsed_source_serial_&&
        *node.layout_baseline->parsed_source_serial_==member->parser_serial&&member->initialized_sequence!=0&&
        node.layout_baseline->sequence_>=member->initialized_sequence&&
        node.layout_receiver_occurrence->id==id&&node.layout_receiver_occurrence->instance==root&&
        node.layout_receiver_occurrence->constructor_serial==member->parser_serial,
        "UnitFlag Button constructor/parser/initialized member was replaced");
    saved->layout_origin();operation.require_origin_without_callbacks();saved->layout_origin();
}
