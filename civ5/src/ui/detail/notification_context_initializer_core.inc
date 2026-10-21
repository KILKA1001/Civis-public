// Inside namespace civ5::ui, after Core and all existing private View bridges.
struct detail::NotificationLayoutContextInitializationReturn::Impl {
    struct Member {std::uint64_t id,parser_serial,initialized_sequence;};
    struct Lifetime {
        const std::thread::id creator=std::this_thread::get_id();
        std::atomic<bool> failed{false};
        bool destroyed=false,destruction_passed=false;
    };
    struct Domain {std::weak_ptr<BnwWorldViewControlTree::Core> core;std::uint64_t topology,before_effects,effects;};
    const std::weak_ptr<BnwWorldViewControlTree::Core> core;
    const std::uint64_t topology,effects,completion_sequence;
    const std::vector<Member> members;
    const std::array<float,2> native_zero;
    const std::shared_ptr<const BnwWorldViewControlTree::LayoutContextInitializationOrigin> origin;
    const std::shared_ptr<Lifetime> lifetime;
    const std::uint64_t source_serial;
    const std::vector<Domain> domains;
    const std::vector<BnwWorldViewControlTree::Core::StackPending> stack_results;
    Impl(const std::shared_ptr<BnwWorldViewControlTree::Core>& actual,std::vector<Member> values,
        std::array<float,2> zero,std::shared_ptr<const BnwWorldViewControlTree::LayoutContextInitializationOrigin> construction,
        std::shared_ptr<Lifetime> actual_lifetime,std::uint64_t serial,std::vector<Domain> actual_domains,
        std::vector<BnwWorldViewControlTree::Core::StackPending> stacks)
        :core(actual),topology(actual->topology_revision),effects(actual->effect_revision),completion_sequence(actual->layout_sequence),
         members(std::move(values)),native_zero(zero),origin(std::move(construction)),lifetime(std::move(actual_lifetime)),
         source_serial(serial),domains(std::move(actual_domains)),stack_results(std::move(stacks)){}
};

struct BnwWorldViewControlTree::ContextInitializationFrame {
    using Parent=detail::NotificationLayoutParentReceiver;
    using View=Parent::TreeView;
    using Key=Parent::NodeKey;
    using Return=detail::NotificationLayoutContextInitializationReturn;
    struct Domain {std::shared_ptr<Core> core;std::uint64_t topology,before_effects,effects;};
    const std::thread::id creator=std::this_thread::get_id();
    const std::shared_ptr<Core> selected;
    const runtime::BnwWorldViewSourceOperationLoan*const operation;
    const std::uint64_t serial;
    const std::shared_ptr<const LayoutContextInitializationOrigin> origin;
    std::vector<Domain> domains;
    std::vector<Key> children;
    std::vector<Core::StackPending> stack_results;
    std::map<std::uint64_t,Return::Impl::Member> completed_members;
    const std::shared_ptr<Return::Impl::Lifetime> completion=std::make_shared<Return::Impl::Lifetime>();
    bool entered=false;
    const int exceptions=std::uncaught_exceptions();
    ContextInitializationFrame(const std::shared_ptr<Core>& actual,
        const runtime::BnwWorldViewSourceOperationLoan& source)
        :selected(actual),operation(&source),serial(source.operation_serial()),origin(actual->layout_context_initialization_origin){
        selected->layout_origin();
        require(!selected->context_initialization_active,"Context5761 initializer reentered");
        require(origin&&origin->construction_,"Context5761 genuine construction/attachment/dimension/native-zero origin unissued");
        if(origin->direct_read_){
            require_context_direct_initialization_origin_owned(selected,origin);
            require(origin->initial_size_==selected->context_layout_baseline->base_.size_30_34&&
                origin->direct_serial_==serial,"Direct5761 actual GUI dimensions/Source changed before native body");
        }else{
        const auto owner=origin->tree_.lock();
        require(owner&&owner.get()==selected.get()&&!owner.owner_before(selected)&&!selected.owner_before(owner)&&
            origin->topology_==selected->topology_revision&&selected->context_layout_baseline&&
            selected->context_layout_baseline->parsed_source_serial_==origin->parser_serial_&&origin->parser_serial_!=0&&
            origin->initial_size_==selected->context_layout_baseline->base_.size_30_34,
            "Context5761 actual construction/parser/initial geometry differs");
        }
        for(float x:origin->native_zero_)require(std::isfinite(x),"Context5761 actual native-zero carrier unsupported");
        require(!selected->layout_initialization_return,"Context5761 completed initialization replay");
        require(&selected->source->current_operation_loan()==operation&&serial!=0,"Context5761 SAME Source loan unentered");
        // Reentry marker is a scope state, never a completed/ready flag. It
        // does not forbid legitimate child Lua operating after its own init.
        selected->context_initialization_active=true;entered=true;
    }
    void poison()noexcept {completion->failed.store(true);selected->failed.store(true);for(const auto& d:domains)d.core->failed.store(true);}
    void first()const {
        if(creator!=std::this_thread::get_id()){selected->failed.store(true);require(false,"Context5761 foreign creator before state borrow");}
        selected->layout_origin();
        for(const auto& d:domains)d.core->layout_origin();
    }
    void pure()const {
        first();require(entered&&selected->context_initialization_active,"Context5761 scope retired/replaced");
        require(&selected->source->current_operation_loan()==operation&&operation->operation_serial()==serial,
            "Context5761 actual Source operation replaced");first();
        require(selected->layout_context_initialization_origin.get()==origin.get()&&
            !selected->layout_context_initialization_origin.owner_before(origin)&&!origin.owner_before(selected->layout_context_initialization_origin),
            "Context5761 construction origin replaced");
        if(origin->direct_read_)require_context_direct_initialization_origin_owned(selected,origin);
        for(const auto& d:domains){
            require(d.core->topology_revision==d.topology&&d.core->effect_revision==d.effects,
                "Context5761 topology/effect changed outside actual owned stage");
            d.core->require_layout_resources_owned();first();
        }
    }
    void full() {
        pure();selected->source->require_sources_current();first();pure();
    }
    std::size_t domain_of(const std::shared_ptr<Core>& actual)const {
        for(std::size_t i=0;i<domains.size();++i)if(domains[i].core.get()==actual.get()&&
            !domains[i].core.owner_before(actual)&&!actual.owner_before(domains[i].core))return i;
        require(false,"Context5761 actual receiver domain absent");return 0;
    }
    void adopt_child_return(const LayoutChildSlot10Return& returned,Key key){
        // FIRST precedes use of receipt fields after the genuine loader call.
        first();require(returned.completion_&&returned.source_serial_==serial&&
            returned.child_id_==key.id,"Context5761 actual child+10 completion/source/receiver differs");
        const auto parent=returned.parent_.lock(),child=returned.child_.lock();
        require(parent&&child&&parent.get()==selected.get()&&!parent.owner_before(selected)&&!selected.owner_before(parent)&&
            child.get()==domains.at(key.domain).core.get()&&!child.owner_before(domains.at(key.domain).core)&&
            !domains.at(key.domain).core.owner_before(child),"Context5761 child receipt canonical control block changed");
        std::set<std::size_t> changed;
        for(const auto& d:returned.domains_){const auto core=d.core.lock();require(bool(core),"Context5761 returned child domain retired");
            auto found=std::find_if(domains.begin(),domains.end(),[&](const auto& x){return x.core.get()==core.get();});
            if(found==domains.end()){
                core->layout_origin();
                require(core->source.get()==selected->source.get()&&!core->source.owner_before(selected->source)&&
                    !selected->source.owner_before(core->source),"Context5761 returned new domain is foreign Source");
                // This preimage is captured by the genuine private Binding
                // BEFORE that domain's first prefix, carried by closed return.
                domains.push_back({core,d.topology,d.before_effects,d.before_effects});
            }
            const auto at=domain_of(core);require(changed.insert(at).second&&d.before_effects==domains[at].effects&&d.topology==domains[at].topology&&
                core->topology_revision==d.topology&&core->effect_revision==d.effects,
                "Context5761 child actual owned revision history changed");
            // Only a closed real child+10/load/Lua completion may report these
            // epochs; no arbitrary snapshot-after-callback success DTO exists.
            domains[at].effects=d.effects;
        }
        pure();
        const auto& core=domains.at(key.domain).core;
        const auto order=key.id==0?([&]{std::uint64_t result=0;
            for(const auto& c:*selected->context_layout_children){const auto actual=c.core.lock();
                if(actual&&actual.get()==core.get()){require(!result&&c.parent_control_id==0,"Context5761 child attachment repeated");result=c.attached_order;}}
            return result;}()):Core::node(core->nodes,key.id).attached_order;
        require(returned.child_order_==order&&order!=0,"Context5761 child+10 attachment order changed");
        if(key.domain==0){
            for(const auto& member:returned.members_){const auto& node=Core::node(selected->nodes,member.id);
                require(node.layout_baseline&&member.parser_serial!=0&&
                    node.layout_baseline->parsed_source_serial_==member.parser_serial&&member.initialized_sequence!=0&&
                    node.layout_baseline->sequence_>=member.initialized_sequence,
                    "Context5761 actual child pertype initialization occurrence changed");
                require(completed_members.emplace(member.id,Return::Impl::Member{member.id,member.parser_serial,member.initialized_sequence}).second,
                    "Context5761 child initialization member duplicated");
            }
        }else require(returned.members_.empty(),"Context5761 Context domain cannot submit ordinary local IDs to parent seal");
        adopt_stack_child_results_owned(*this,returned); //only genuine already-DTOR child receipt.
    }
    ~ContextInitializationFrame()noexcept {
        if(creator!=std::this_thread::get_id()){
            completion->failed.store(true);selected->failed.store(true);return;
        }
        if(entered){
            try{pure();require(std::uncaught_exceptions()==exceptions,"Context5761 completion unwinding");
                completion->destruction_passed=true;}catch(...){poison();}
            selected->context_initialization_active=false;entered=false;
        }
        completion->destroyed=true;
    }
};

std::shared_ptr<BnwWorldViewControlTree::Core>
BnwWorldViewControlTree::require_context_initialization_return_owned(
    const std::shared_ptr<Core>& actual,const std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn>& returned){
    require(bool(actual),"Context initializer completion canonical Core absent");actual->layout_origin();
    require(returned&&returned->impl_,"Context initializer typed completion absent");
    const auto& done=*returned->impl_;const auto core=done.core.lock();
    require(done.lifetime&&done.lifetime->creator==std::this_thread::get_id(),"Context initializer completion foreign creator");
    require(!done.lifetime->failed.load()&&done.lifetime->destroyed&&done.lifetime->destruction_passed&&
        !actual->context_initialization_active&&!actual->busy,"Context initializer scope destruction incomplete");
    std::vector<std::shared_ptr<Core>> current_domains;
    for(const auto& domain:done.domains){const auto current=domain.core.lock();
        require(bool(current),"Context initializer completed domain retired");current->layout_origin();current_domains.push_back(current);}
    for(std::size_t i=0;i<done.domains.size();++i){const auto& domain=done.domains[i];const auto& current=current_domains[i];
        require(current->topology_revision==domain.topology&&current->effect_revision==domain.effects,
            "Context initializer actual domain changed after completion DTOR");
        current->require_layout_resources_owned();
        for(const auto& live:current_domains)live->layout_origin();
    }
    require(core&&core.get()==actual.get()&&!core.owner_before(actual)&&!actual.owner_before(core)&&
        done.topology==actual->topology_revision&&done.effects==actual->effect_revision&&
        done.completion_sequence==actual->layout_sequence&&done.origin&&done.origin->construction_&&
        done.origin.get()==actual->layout_context_initialization_origin.get(),
        "Context initializer actual post-complete occurrence differs");
    if(done.origin->direct_read_)require_context_direct_initialization_origin_owned(actual,done.origin);
    actual->require_layout_resources_owned();actual->layout_origin();return actual;
}
BnwWorldViewControlTree::LayoutInitializationReturn::LayoutInitializationReturn(
    const std::shared_ptr<Core>& actual,const std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn>& returned)
    :tree_(require_context_initialization_return_owned(actual,returned)),topology_(returned->impl_->topology),
     members_([&]{std::vector<Member> result;for(const auto& member:returned->impl_->members)
         result.push_back({member.id,member.parser_serial,member.initialized_sequence});return result;}()),
     native_zero_(returned->impl_->native_zero),native_return_(returned){}

void BnwWorldViewControlTree::initialize_context_5761_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation){
    using Parent=detail::NotificationLayoutParentReceiver;using View=Parent::TreeView;
    using Return=detail::NotificationLayoutContextInitializationReturn;
    const auto saved=core_;saved->layout_origin();
    try{
        std::shared_ptr<const Return::Impl> completed;
        {
        ContextInitializationFrame frame(saved,operation);
        // Capture the authentic complete topology for this supported branch.
        // This short View is gone BEFORE any child+10, including child Lua.
        std::shared_ptr<View::Binding> binding;
        std::vector<Parent::FourCReturn> four_c;
        {
            saved->busy=true;struct Exit{bool& value;~Exit(){value=false;}}exit{saved->busy};
            binding=std::make_shared<View::Binding>(saved,operation);
            {
                View view(binding,binding->captured);view.require_pure();
                const auto& receiver=Parent::node(view,Parent::NodeKey{0,0});
                require(receiver.state->kind_==detail::NotificationLayoutBaseline::Kind::context,
                    "Context5761 target is not actual Context baseline");
                frame.children=Parent::children(view,Parent::NodeKey{0,0});
                for(const auto& domain:binding->domains)frame.domains.push_back({domain.core,domain.topology,domain.before_effects,domain.effects});
                view.preflight_store_owned({0,0});receiver.state->record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
                receiver.state->base_.flags_40|=1U; // Native OR1 preserves earned800 and all other actual flags.
                view.record_store_owned({0,0},Parent::Store::Kind::initializer_flags);
                Parent::slot4c(view,Parent::NodeKey{0,0}); // ONLY native own4C, never Reprocess.
                view.require_pure();auto& state=*Parent::node(view,Parent::NodeKey{0,0}).state;
                const auto& b=state.base_;
                const std::array<float,2> extent{
                    static_cast<float>(std::fabs(static_cast<long double>(b.margin_70_74[0]))+b.size_30_34[0]),
                    static_cast<float>(std::fabs(static_cast<long double>(b.margin_70_74[1]))+b.size_30_34[1])};
                const auto add=[](float a,float b){volatile float value=a+b;return value;};
                const std::array<float,4> bounds{b.offset_28_2c[0],b.offset_28_2c[1],
                    add(b.offset_28_2c[0],b.size_30_34[0]),add(b.offset_28_2c[1],b.size_30_34[1])};
                for(float x:extent)require(std::isfinite(x),"Context5761 native extent unsupported");
                for(float x:bounds)require(std::isfinite(x),"Context5761 native bounds unsupported");
                view.preflight_store_owned({0,0});state.record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
                state.base_.extent_20_24=extent;state.base_.bounds_10_1c=bounds;
                view.record_store_owned({0,0},Parent::Store::Kind::bounds_union);
                view.require_full();view.require_pure();four_c=view.four_c_returns_;view.require_pure();
            }
            binding->pure(false);require(binding->destruction_passed,"Context5761 prefix View destructor refused");
        }
        // No eager domain busy locks survive into original child execution.
        // Accept only this closed View's actual stores, then current guards.
        for(const auto& domain:binding->domains)frame.domains[frame.domain_of(domain.core)].effects=domain.effects;
        require(binding->destruction_passed&&!binding->graph_active,"Context5761 prefix destruction missing");
        binding->finished=true;frame.pure();
        for(const auto& returned:four_c){const auto& domain=frame.domains.at(returned.key.domain);
            Core::LayoutFourCReturn stamp{returned.actual_offset,returned.baseline_sequence,frame.serial,domain.topology,returned.offset_store_sequence};
            if(returned.key.id==0)domain.core->context_layout_four_c_return=stamp;
            else {Core::node(domain.core->nodes,returned.key.id);domain.core->nodes.at(returned.key.id-1).layout_four_c_return=stamp;}
        }
        // Retire this Binding before constructing a later one: its destructor
        // releases owned busy fields and must never overlap the next View.
        binding.reset();frame.first();
        frame.full();
        for(const auto key:frame.children){
            frame.pure();
            {
                auto returned=initialize_context_child_slot10_owned(frame,key.domain,key.id);
                frame.first();frame.adopt_child_return(returned,key);
            } // Actual returned target destruction FIRST, before child fields.
            frame.first();frame.full();
            // Native reads current child bounds AFTER its actual+10 returns.
            // Deferred Context remains a real child: it is not omitted/NULL.
            const auto& child_core=frame.domains.at(key.domain).core;
            const auto& child=key.id==0?*child_core->context_layout_baseline:
                *Core::node(child_core->nodes,key.id).layout_baseline;
            const auto child_bounds=child.base_.bounds_10_1c;
            for(float x:child_bounds)require(std::isfinite(x),"Context5761 child native bounds unsupported");
            {
                saved->busy=true;struct Exit{bool& value;~Exit(){value=false;}}exit{saved->busy};
                binding=std::make_shared<View::Binding>(saved,operation);
                {
                    View view(binding,binding->captured);view.require_pure();
                    const auto actual_children=Parent::children(view,Parent::NodeKey{0,0});
                    require(actual_children==frame.children,"Context5761 actual direct child-vector changed");
                    const auto& actual_child=*Parent::node(view,key).state;
                    require(actual_child.base_.bounds_10_1c==child_bounds,"Context5761 child changed before native union");
                    auto& state=*Parent::node(view,Parent::NodeKey{0,0}).state;auto bounds=state.base_.bounds_10_1c;
                    // COMISS strict-greater select preserves parent on ties.
                    bounds[0]=bounds[0]>child_bounds[0]?child_bounds[0]:bounds[0];
                    bounds[1]=bounds[1]>child_bounds[1]?child_bounds[1]:bounds[1];
                    bounds[2]=child_bounds[2]>bounds[2]?child_bounds[2]:bounds[2];
                    bounds[3]=child_bounds[3]>bounds[3]?child_bounds[3]:bounds[3];
                    view.preflight_store_owned({0,0});state.record_owned(detail::NotificationLayoutBaseline::Mutation::receiver_prefix);
                    state.base_.bounds_10_1c=bounds;view.record_store_owned({0,0},Parent::Store::Kind::bounds_union);
                    view.require_full();view.require_pure();
                }
                binding->pure(false);require(binding->destruction_passed,"Context5761 union scope destructor refused");
            }
            for(const auto& domain:binding->domains)frame.domains[frame.domain_of(domain.core)].effects=domain.effects;
            binding->finished=true;binding.reset();frame.first();frame.full();
        }
        frame.pure();const auto& root=*saved->context_layout_baseline;
        if(frame.origin->direct_read_)require(!root.parsed_source_serial_,"Direct5761 cannot publish a legacy root parser");
        else require(root.parsed_source_serial_&&*root.parsed_source_serial_!=0,"Context5761 actual root parser history absent");
        std::vector<Return::Impl::Member> members;members.push_back({0,frame.origin->direct_read_?0:*root.parsed_source_serial_,root.sequence_});
        std::size_t count=0;
        for(const auto& node:saved->nodes)if(node.alive){++count;const auto found=frame.completed_members.find(node.id);
            require(found!=frame.completed_members.end(),"Context5761 real pertype child+10 completion absent");members.push_back(found->second);}
        require(count==frame.completed_members.size(),"Context5761 completed child member set differs");
        frame.full();
        // Actual body, all child execution, union and View DTOR are complete.
        // The new receipt constructor is private, not a boolean/public grant.
        std::vector<Return::Impl::Domain> completed_domains;
        for(const auto& domain:frame.domains)completed_domains.push_back({domain.core,domain.topology,domain.before_effects,domain.effects});
        finalize_stack_results_owned(frame); //exact final adoptedcohort, stale earlier stack fields refuse.
        completed=std::make_shared<const Return::Impl>(saved,std::move(members),frame.origin->native_zero_,frame.origin,
            frame.completion,frame.serial,std::move(completed_domains),std::move(frame.stack_results));
        frame.pure();
        } // Genuine producer scope DTOR, including all current guards, FIRST.
        saved->layout_origin();
        require(completed&&completed->lifetime->creator==std::this_thread::get_id()&&
            !completed->lifetime->failed.load()&&completed->lifetime->destroyed&&completed->lifetime->destruction_passed,
            "Context5761 producer destruction refused");
        saved->source->require_sources_current();
        for(const auto& domain:completed->domains){const auto current=domain.core.lock();
            require(bool(current),"Context5761 actual domain retired during completion callback");current->layout_origin();}
        saved->layout_origin();
        require(&saved->source->current_operation_loan()==&operation,"Context5761 Source changed after producer DTOR");
        operation.require_operation(completed->source_serial);saved->layout_origin();
        const auto body=std::shared_ptr<const Return>(new Return(completed));
        const auto seal=std::shared_ptr<const LayoutInitializationReturn>(new LayoutInitializationReturn(saved,body));
        saved->layout_origin();require_context_initialization_return_owned(saved,body);saved->layout_origin();
        require(!saved->layout_initialization_return,"Context5761 initialization receipt replay");
        saved->layout_initialization_return=seal;
        publish_stack_results_owned(saved,body); //ONLY actual ContextFrameDTOR+publishedfinalseal.
    }catch(...){saved->failed.store(true);throw;}
}
