// Include inside namespace civ5::ui, AFTER the actual ControlTree::Core body.
// Root joins Baseline's captured node/context state and the closed context
// child-vector fields below. No caller can submit this Binding or numerical
// observations; all addresses come from the genuine retained Core objects.
struct detail::NotificationLayoutParentReceiver::TreeView::Binding {
    using Tree=BnwWorldViewControlTree;
    using Core=Tree::Core;
    struct Domain {
        std::shared_ptr<Core> core;
        std::uint64_t topology,effects,before_effects;
        std::size_t node_count;
        bool owns_busy;
    };
    std::vector<Domain> domains;
    std::vector<NodeView> captured;
    const runtime::BnwWorldViewSourceOperationLoan* operation=nullptr;
    std::uint64_t serial=0;
    bool graph_active=false,destruction_passed=false,finished=false,busy_released=false;
    const int exceptions=std::uncaught_exceptions();
    void poison()noexcept{for(auto& d:domains)d.core->failed.store(true);}
    void release_busy()noexcept{for(auto& d:domains)if(d.owns_busy)d.core->busy=false;busy_released=true;}
    void first(bool active)const {
        for(const auto& d:domains)d.core->layout_origin(!d.owns_busy||!busy_released);
    }
    std::uint64_t domain_of(const std::shared_ptr<Core>& core)const {
        for(std::size_t i=0;i<domains.size();++i)
            if(domains[i].core.get()==core.get()&&!domains[i].core.owner_before(core)&&!core.owner_before(domains[i].core))return i;
        require(false,"Layout context child/parent canonical Core is outside captured closure");return 0;
    }
    std::uint64_t add_domain(const std::shared_ptr<Core>& actual) {
        require(bool(actual),"Layout captured context weak owner expired");
        for(std::size_t i=0;i<domains.size();++i)if(domains[i].core.get()==actual.get()){
            require(!domains[i].core.owner_before(actual)&&!actual.owner_before(domains[i].core),"Layout context Core forged control");return i;
        }
        actual->layout_origin();
        const auto& selected=domains.front().core;
        require(actual->source.get()==selected->source.get()&&!actual->source.owner_before(selected->source)&&
            !selected->source.owner_before(actual->source),"Layout context chain is not SAME Source owner");
        // Allocate BEFORE earning busy so a throwing insertion cannot strand it.
        domains.push_back({actual,actual->topology_revision,actual->effect_revision,actual->effect_revision,actual->nodes.size(),true});
        actual->busy=true;return domains.size()-1;
    }
    void capture_domain_nodes(std::size_t i){
        const auto& core=domains.at(i).core;core->layout_origin(true);
        require(core->context_layout_baseline&&core->context_parent_kind,
            "Layout actual Context scalar constructor/parent history uncaptured");
        std::optional<NodeKey> parent;std::uint64_t order=0;
        if(*core->context_parent_kind!=Core::ContextParentKind::native_null){
            const auto target=core->context_parent_core.lock();require(bool(target),"Layout actual parent Context retired");
            const auto pd=domain_of(target);
            const auto pid=*core->context_parent_kind==Core::ContextParentKind::canonical_context?0:core->context_parent_control_id;
            require(*core->context_parent_kind!=Core::ContextParentKind::canonical_context||core->context_parent_control_id==0,
                "Layout canonical Context parent local ID substituted");
            parent=NodeKey{pd,pid};
            if(core->scalar_parent_attachment){
                // Scalar own-parent lookup consumes the actually executed
                // partial link command; it does not enumerate child Contexts.
                Tree::require_scalar_parent_attachment_owned(core,*operation,true,true);
                const auto& link=core->scalar_parent_attachment;
                require(link->parent_control_id==pid,"Layout scalar actual parent control differs");
                order=link->source_order;
                if(target->context_layout_children){
                    bool seen=false;
                    for(const auto& child:*target->context_layout_children){const auto actual=child.core.lock();
                        if(actual&&actual.get()==core.get()){
                            require(!actual.owner_before(core)&&!core.owner_before(actual)&&child.parent_control_id==pid&&!seen&&
                                child.attached_order==order,"Layout full/scalar actual attachment history differs");seen=true;}}
                    require(seen,"Layout full vector lacks its actually linked scalar child");
                }
            }else{
                // Preserve the prior genuine full-vector path without issuing
                // an empty or five-slot vector when the broader XML is open.
                require(target->context_layout_children.has_value(),"Layout actual parent attachment history uncaptured");
                bool seen=false;
                for(const auto& child:*target->context_layout_children){const auto actual=child.core.lock();
                    if(actual&&actual.get()==core.get()){
                        require(!actual.owner_before(core)&&!core.owner_before(actual)&&child.parent_control_id==pid&&!seen,
                            "Layout Context parent/attachment canonical history inconsistent");seen=true;order=child.attached_order;}}
                require(seen&&order!=0,"Layout actual parent attachment missing");
            }
        }
        captured.push_back({{i,0},order,parent,&*core->context_layout_baseline,std::nullopt});
        for(auto& n:core->nodes)if(n.alive){
            require(n.layout_baseline&&n.id&&n.id<=core->nodes.size()&&!n.external_parent_id,
                "Layout ordinary constructor/parser baseline uncaptured");
            captured.push_back({{i,n.id},n.attached_order,NodeKey{i,n.parent},&*n.layout_baseline,n.layout_base_dispatch});
        }
    }
    void capture_closed(const std::shared_ptr<Core>& selected){
        selected->layout_origin(true);
        domains.push_back({selected,selected->topology_revision,selected->effect_revision,selected->effect_revision,selected->nodes.size(),false});
        try{
            const auto& actual=selected->source->current_operation_loan();selected->layout_origin(true);
            operation=&actual;serial=actual.operation_serial();require(serial!=0,"Layout source operation unissued");
            // Initial scalar closure follows ONLY actual parent contexts.
            // Other child Contexts are admitted when a reached receiver calls
            // children(). Full94/Stack enumeration is still mandatory there.
            for(std::size_t i=0;i<domains.size();++i){const auto core=domains[i].core;core->layout_origin(true);
                require(core->context_layout_baseline&&core->context_parent_kind,
                    "Layout genuine Context scalar baseline/parent absent");
                if(*core->context_parent_kind!=Core::ContextParentKind::native_null)add_domain(core->context_parent_core.lock());
            }
            for(std::size_t i=0;i<domains.size();++i)capture_domain_nodes(i);
            for(const auto& start:captured){std::vector<NodeKey> path;auto key=start.key;
                for(;;){require(std::find(path.begin(),path.end(),key)==path.end(),"Layout actual parent topology cycle");path.push_back(key);
                    const auto n=std::find_if(captured.begin(),captured.end(),[&](const auto& x){return x.key==key;});
                    require(n!=captured.end(),"Layout actual parent receiver absent");if(!n->parent)break;key=*n->parent;
                }
            }
            pure(false);
        }catch(...){poison();release_busy();throw;}
    }
    void admit_children_owned(NodeKey parent){
        pure(true);const auto core=domains.at(parent.domain).core;
        // An engaged actual full vector is needed even for a reached empty
        // receiver. No metadata slice/hidden/DeferLoad omission earns it.
        if(!core->context_layout_children.has_value()){
            // Diagnose ONLY the first already-refused actual receiver, AFTER
            // existing Binding PURE/source/lifetime/busy predicates. No new
            // issuer, state mutation, vector construction or callback occurs.
            const auto why=std::string("Reached receiver actual complete Context child vector uncaptured")+
                " [context="+core->context+" receiver_id="+std::to_string(parent.id)+
                " domain="+std::to_string(parent.domain)+
                " constructor="+(core->context_construction_return?"1":"0")+
                " context_flags="+std::to_string(core->context_layout_baseline?core->context_layout_baseline->base_.flags_40:0U)+
                " native_child_vector_completion="+(core->native_context_child_vector_completion?"1":"0")+
                " actual_scalar_child_commands="+std::to_string(core->scalar_child_attachment_history.size())+
                " vector_engaged=0]";
            require(false,why.c_str()); // same refusal; caller catch quarantines.
        }
        core->require_native_context_child_vector_history_owned(true);
        const auto found=std::find_if(captured.begin(),captured.end(),[&](const auto& n){return n.key==parent;});
        require(found!=captured.end(),"Reached child enumeration parent absent");
        for(const auto& child:*core->context_layout_children)if(child.parent_control_id==parent.id){
            const auto actual=child.core.lock();require(bool(actual),"Reached actual child Context retired");
            const auto prior=domains.size();const auto at=add_domain(actual);
            require(actual->context_parent_kind&&actual->context_parent_core.lock().get()==core.get()&&
                actual->context_parent_control_id==parent.id&&child.attached_order!=0,
                "Reached actual child attachment/parent changed");
            if(domains.size()!=prior)capture_domain_nodes(at);
            const auto n=std::find_if(captured.begin(),captured.end(),[&](const auto& x){return x.key==NodeKey{at,0};});
            require(n!=captured.end()&&n->parent&&*n->parent==parent&&n->attached_order==child.attached_order,
                "Reached child canonical parent/order differs");
        }
        pure(true);
    }
    Binding(const std::shared_ptr<Core>& selected,const Tree::LayoutOperandLoan& loan,std::uint64_t id){
        selected->layout_origin(true);loan.require_for_core_without_callbacks(selected,id);capture_closed(selected);
    }
    // Distinct initializer mint, BEFORE any LayoutOperandLoan. Only the actual
    // private NativeIssuer Tree entry may borrow this entered Source operation.
    // It does not turn parsed serial/XML state into an initialized return.
    Binding(const std::shared_ptr<Core>& selected,const runtime::BnwWorldViewSourceOperationLoan& actual){
        selected->layout_origin(true);const auto& entered=selected->source->current_operation_loan();selected->layout_origin(true);
        require(&entered==&actual&&entered.operation_serial()==actual.operation_serial(),"Layout initializer is not actual entered SAME Source");
        capture_closed(selected);
    }
    void pure(bool active)const {
        first(active);
        require(!finished&&graph_active==active,"Layout receiver scope retired/substituted");
        const auto& current=domains.front().core->source->current_operation_loan();first(active);
        require(&current==operation&&current.operation_serial()==serial,"Layout actual SAME Source operation retired/replaced");
        for(const auto& d:domains){
            require(d.core->topology_revision==d.topology&&d.core->effect_revision==d.effects&&d.core->nodes.size()==d.node_count,
                "Layout canonical topology/effect epoch changed outside receiver");
            require(d.core->context_layout_baseline&&d.core->context_parent_kind,
                "Layout captured Context source state retired");
        }
        for(const auto& view:captured){const auto& core=domains.at(view.key.domain).core;
            if(view.key.id==0){require(&*core->context_layout_baseline==view.state,"Layout Context baseline replaced");}
            else {const auto& n=Core::node(core->nodes,view.key.id);
                require(n.layout_baseline&&&*n.layout_baseline==view.state&&n.attached_order==view.attached_order&&
                    view.parent&&*view.parent==NodeKey{view.key.domain,n.parent}&&!n.external_parent_id&&
                    n.layout_base_dispatch==view.base_dispatch&&
                    bool(n.layout_baseline->base_.flags_40&2U)==n.hidden,"Layout actual canonical node membership/hidden state changed");
            }
        }
        for(const auto& d:domains)d.core->require_layout_resources_owned();
        first(active);
    }
    void full() {
        pure(true);
        try {
            const auto& source=domains.front().core->source;
            source->require_sources_current();
            // FIRST after callback COPY/invoke/target destruction: no borrowed
            // baseline/Core node dereference until ALL facade lifetimes pass.
            first(true);pure(true);
        }catch(...){poison();throw;}
    }
    ~Binding()noexcept{release_busy();}
};
detail::NotificationLayoutParentReceiver::TreeView::TreeView(
    std::shared_ptr<Binding> binding,std::vector<NodeView> nodes):binding_(std::move(binding)),nodes_(nodes.begin(),nodes.end()){
    require(binding_&&!binding_->graph_active,"Layout receiver scope reentered");
    binding_->graph_active=true;
    try {require_full();}catch(...){binding_->graph_active=false;binding_->poison();throw;}
}
detail::NotificationLayoutParentReceiver::TreeView::~TreeView()noexcept{
    const auto saved=binding_;
    try {saved->pure(true);saved->destruction_passed=std::uncaught_exceptions()==saved->exceptions;if(!saved->destruction_passed)saved->poison();}
    catch(...){saved->poison();}
    saved->graph_active=false;saved->release_busy();
}
void detail::NotificationLayoutParentReceiver::TreeView::require_pure()const{
    const auto saved=binding_;try {saved->pure(true);}catch(...){saved->poison();throw;}
}
void detail::NotificationLayoutParentReceiver::TreeView::require_children_owned(NodeKey key)const{
    const auto saved=binding_;
    try{
        saved->pure(true);saved->admit_children_owned(key);
        for(const auto& actual:saved->captured)
            if(std::none_of(nodes_.begin(),nodes_.end(),[&](const auto& n){return n.key==actual.key;}))nodes_.push_back(actual);
        saved->pure(true);
    }catch(...){saved->poison();throw;}
}
std::array<float,2> detail::NotificationLayoutParentReceiver::TreeView::arithmetic_pair_owned()const{
    require_pure();const auto port=binding_->domains.front().core->context_gui_port;
    require(bool(port),"Layout reached View canonical mutable GUI arithmetic namespace absent");
    for(const auto& domain:binding_->domains){const auto actual=domain.core->context_gui_port;
        require(actual&&actual.get()==port.get()&&!actual.owner_before(port)&&!port.owner_before(actual),
            "Layout reached lazy domains name different mutable GUI arithmetic owners");}
    const auto result=port->arithmetic_pair_without_callbacks(*binding_->operation);require_pure();return result;
}
void detail::NotificationLayoutParentReceiver::TreeView::require_full()const{
    const auto saved=binding_;saved->full(); // no raw TreeView access after callback
}
void detail::NotificationLayoutParentReceiver::TreeView::preflight_store_owned(NodeKey key)const{
    const auto saved=binding_;saved->pure(true);const auto& domain=saved->domains.at(key.domain);
    require(domain.core->effect_revision<UINT64_MAX&&domain.core->layout_sequence<UINT64_MAX,"Layout receiver history exhausted before store");
    domain.core->initial_lua_preflight_store_owned();
}
void detail::NotificationLayoutParentReceiver::TreeView::record_store_owned(NodeKey key,Store::Kind kind){
    const auto saved=binding_;saved->pure(true);auto& domain=saved->domains.at(key.domain);
    require(domain.core->effect_revision<UINT64_MAX&&domain.core->layout_sequence<UINT64_MAX,"Layout receiver history exhausted");
    stores_.push_back({key,kind});++domain.core->effect_revision;++domain.core->layout_sequence;
    domain.core->initial_lua_record_store_owned(Binding::Core::InitialLuaWriter::receiver_store,key.id,static_cast<std::uint64_t>(kind));
    if(kind==Store::Kind::base_anchor){
        // Only an actual28/2c store invalidates its prior completed4C stamp.
        // Unrelated94 bounds stores still advance the ordinary Core epoch.
        if(key.id==0){domain.core->context_layout_offset_store_sequence=domain.core->layout_sequence;
            domain.core->context_layout_four_c_return.reset();}
        else {auto& actual=domain.core->nodes.at(key.id-1);
            actual.layout_offset_store_sequence=domain.core->layout_sequence;actual.layout_four_c_return.reset();}
    }
    domain.effects=domain.core->effect_revision;saved->pure(true);
}
void detail::NotificationLayoutParentReceiver::TreeView::record_four_c_return_owned(NodeKey key){
    const auto saved=binding_;saved->pure(true);const auto& actual=NotificationLayoutParentReceiver::node(*this,key);
    const auto& domain=saved->domains.at(key.domain);
    const auto offset_sequence=key.id==0?domain.core->context_layout_offset_store_sequence:
        Binding::Core::node(domain.core->nodes,key.id).layout_offset_store_sequence;
    require(offset_sequence!=0,"Completed4C requires its actual28/2c store history");
    four_c_returns_.push_back({key,actual.state->sequence_,actual.state->base_.offset_28_2c,offset_sequence});saved->pure(true);
}

// Defined mandatory suffix frontiers. Existing GUI mouse-cache payload is
// not eligibility, hover callback or a returned diagnostic LuaObject owner.
// Actual preceding receiver stores remain partial on these refusals.
void detail::NotificationLayoutParentReceiver::TreeView::scroll_fraction_diagnostic_owned(
    std::uint64_t id,float supplied){
    try{binding_->first(true);}catch(...){binding_->poison();throw;}const auto saved=binding_;
    try{
        saved->pure(true);saved->full();saved->first(true);saved->pure(true);
        const auto& actual=NotificationLayoutParentReceiver::node(*this,id);
        require(actual.state->kind_==NotificationLayoutBaseline::Kind::scroll_panel&&actual.state->scroll_&&
            std::isfinite(supplied)&&actual.state->scroll_->fraction_a4==supplied&&
            (supplied<0.0F||supplied>1.0F),"Scroll diagnostic lacks SAME actual out-of-range fraction");
        // B8/C0 optional callback presence must come from the actual retained
        // LuaObject; an absent current supplier is not a genuine nil branch.
        throw std::runtime_error(supplied>1.0F?
            "Reached Scroll C0 diagnostic: actual8F89F0 callback lookup/4F70C0 invoke/8F8820 release owner required":
            "Reached Scroll B8 diagnostic: actual8F89F0 callback lookup/4F70C0 invoke/8F8820 release owner required");
    }catch(...){saved->poison();throw;}
}
void detail::NotificationLayoutParentReceiver::TreeView::disable_hovered_receiver_owned(std::uint64_t id){
    try{binding_->first(true);}catch(...){binding_->poison();throw;}const auto saved=binding_;
    try{
        saved->pure(true);saved->full();saved->first(true);saved->pure(true);
        const auto& actual=NotificationLayoutParentReceiver::node(*this,id);
        require(((actual.state->kind_==NotificationLayoutBaseline::Kind::slider&&actual.state->slider_)||
            (actual.state->kind_==NotificationLayoutBaseline::Kind::button&&actual.state->button_))&&
            (actual.state->base_.flags_40&0x20000U)&&!(actual.state->base_.flags_40&0x10000U),
            "Disable hover suffix lacks SAME typed disabled receiver after actual hover-bit clear");
        // Caller already earned its changed-disabled/clear10000 stores.
        // Do not claim the missing GUI removal and receiver callback returned.
        throw std::runtime_error("Reached disabled-hover suffix: actual GUI680EE0 then SAME receiver4B82A0(0) callback owner required");
    }catch(...){saved->poison();throw;}
}
void detail::NotificationLayoutParentReceiver::TreeView::post_mouse_rehit_owned(std::uint64_t id){
    try{binding_->first(true);}catch(...){binding_->poison();throw;}const auto saved=binding_;
    try{
        saved->pure(true);saved->full();saved->first(true);saved->pure(true);
        const auto& actual=NotificationLayoutParentReceiver::node(*this,id);
        require((actual.state->kind_==NotificationLayoutBaseline::Kind::slider&&actual.state->slider_)||
            (actual.state->kind_==NotificationLayoutBaseline::Kind::button&&actual.state->button_),
            "Mouse rehit suffix lacks SAME actual typed changed-disabled receiver");
        // Cached650/64C values alone cannot decide GUI13B4 or authenticate
        // the HWND+A4 branch. Never post or return using guessed eligibility.
        throw std::runtime_error("Reached mouse rehit suffix: genuine GUI13B4 event14b eligibility owner required before PostMessageA(HWND+A4,WM_MOUSEMOVE,cached650,cached64C)");
    }catch(...){saved->poison();throw;}
}

void BnwWorldViewControlTree::run_layout_parent_receiver_owned(
    const Control& control,const LayoutOperandLoan& loan,LayoutParentCommand command){
    using Parent=detail::NotificationLayoutParentReceiver;
    using View=Parent::TreeView;
    const auto saved=core_;const auto actual=control;
    std::shared_ptr<View::Binding> binding;
    try {
        saved->layout_origin();saved->layout_current_owned();saved->layout_origin();
        const auto id=saved->checked(actual);
        const auto& target=Core::node(saved->nodes,id);
        require(target.layout_baseline&&target.layout_baseline->stack_&&
            target.layout_baseline->stack_->field_84==1&&target.layout_baseline->stack_->field_8c==0,
            "Actual selected parent receiver requires earned Top/noWrap Stack state");
        if(command==LayoutParentCommand::reprocess_stack)
            require(target.layout&&target.layout->calculate_returned&&target.layout->effect_revision==saved->effect_revision&&
                target.layout->topology_revision==saved->topology_revision,
                "Actual Stack anchoring requires current returned size");
        saved->busy=true;struct Exit{bool& value;~Exit(){value=false;}}exit{saved->busy};
        binding=std::make_shared<View::Binding>(saved,loan,id);
        std::vector<Parent::Store> stores;
        std::vector<Parent::DerivedChildPosition> positions;
        std::vector<Parent::FourCReturn> four_c_returns;
        {
            View view(binding,binding->captured);
            if(command==LayoutParentCommand::calculate_stack)Parent::stack_calculate(view,{0,id});
            else {require(command==LayoutParentCommand::reprocess_stack,"Actual parent receiver command unissued");Parent::reprocess_anchoring(view,{0,id});}
            view.require_full();view.require_pure();
            stores=view.stores_;positions=view.child_positions_;four_c_returns=view.four_c_returns_;
            view.require_pure();
        } // The actual View destructor must complete before any returned flag.
        binding->pure(false);
        require(binding->destruction_passed,"Layout target/scope destruction did not complete");
        // These observations are derived from actual stored canonical state.
        // Partial prefixes remain in Baseline history on failure; they never
        // issue calculate_returned/anchoring_returned through this path.
        for(const auto& store:stores){
            if(store.kind==Parent::Store::Kind::scroll_size||store.kind==Parent::Store::Kind::scroll_internal||
                store.kind==Parent::Store::Kind::slider||store.kind==Parent::Store::Kind::image||store.kind==Parent::Store::Kind::grid)continue; // typed resource/UV stores issue no LayoutState return
            if(store.key.id==0)continue; // Context fields are actual state, not a Lua ControlQuote.
            const auto& domain=binding->domains.at(store.key.domain);
            Core::node(domain.core->nodes,store.key.id); // checked genuine member before mutable access
            auto& node=domain.core->nodes.at(store.key.id-1);
            const auto& state=*node.layout_baseline;
            auto result=node.layout.value_or(LayoutState{});
            result.native_offset=state.base_.offset_28_2c;
            result.calculated_size=state.base_.size_30_34;
            result.native_extent=state.base_.extent_20_24;
            result.native_bounds=state.base_.bounds_10_1c;
            result.source_serial=binding->serial;result.topology_revision=domain.topology;
            result.effect_revision=domain.effects;result.command_sequence=domain.core->layout_sequence;
            if(store.kind==Parent::Store::Kind::stack_size){
                result.calculate_returned=true;result.child_offsets.clear();
                for(const auto& position:positions)if(position.stack==store.key){
                    require(position.child.domain==store.key.domain,"Layout ordinary Stack child came from another context domain");
                    auto prior=std::find_if(result.child_offsets.begin(),result.child_offsets.end(),[&](const auto& item){return item.first==position.child.id;});
                    if(prior==result.child_offsets.end())result.child_offsets.emplace_back(position.child.id,position.position);
                    else prior->second=position.position;
                }
            }
            if(store.kind==Parent::Store::Kind::stack_anchor){
                require(state.stack_.has_value(),"Layout Stack prepass storage replaced");
                result.anchor_origin={state.stack_->field_90,state.stack_->field_94};
                result.anchor_prepass_stored=true;result.anchoring_returned=true;
            }
            node.layout=std::move(result);
        }
        for(const auto& returned:four_c_returns){const auto& domain=binding->domains.at(returned.key.domain);
            Core::LayoutFourCReturn stamp{returned.actual_offset,returned.baseline_sequence,binding->serial,domain.topology,returned.offset_store_sequence};
            if(returned.key.id==0)domain.core->context_layout_four_c_return=stamp;
            else {Core::node(domain.core->nodes,returned.key.id);domain.core->nodes.at(returned.key.id-1).layout_four_c_return=stamp;}
        }
        binding->pure(false);binding->finished=true;
    }catch(...){if(binding)binding->poison();saved->failed.store(true);throw;}
}

void BnwWorldViewControlTree::initialize_layout_parent_receiver_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation){
    using Parent=detail::NotificationLayoutParentReceiver;using View=Parent::TreeView;
    const auto saved=core_;std::shared_ptr<View::Binding> binding;
    try {
        saved->layout_origin();saved->layout_current_owned();saved->layout_origin();
        saved->busy=true;struct Exit{bool& value;~Exit(){value=false;}}exit{saved->busy};
        binding=std::make_shared<View::Binding>(saved,operation);
        std::vector<Parent::FourCReturn> returns;
        {
            View view(binding,binding->captured);
            Parent::reprocess_anchoring(view,Parent::NodeKey{0,0});
            view.require_full();view.require_pure();returns=view.four_c_returns_;view.require_pure();
        }
        binding->pure(false);require(binding->destruction_passed,"Actual initializer scope destruction incomplete");
        for(const auto& returned:returns){const auto& domain=binding->domains.at(returned.key.domain);
            Core::LayoutFourCReturn stamp{returned.actual_offset,returned.baseline_sequence,binding->serial,domain.topology,returned.offset_store_sequence};
            if(returned.key.id==0)domain.core->context_layout_four_c_return=stamp;
            else {Core::node(domain.core->nodes,returned.key.id);domain.core->nodes.at(returned.key.id-1).layout_four_c_return=stamp;}
        }
        // This is only actual4C occurrence history. It never issues a
        // LayoutOperandLoan or an all-pertype/resource initialized flag.
        binding->pure(false);binding->finished=true;
    }catch(...){if(binding)binding->poison();saved->failed.store(true);throw;}
}
