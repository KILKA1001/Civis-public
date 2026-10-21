// Include AFTER notification_layout_parent_core.inc in the actual Core TU.
// Strong storage pins below are Core only. No checker owns a Tree facade.
BnwWorldViewControlTree::LayoutResourceBinding::LayoutResourceBinding(std::shared_ptr<Core> core,
    const std::shared_ptr<detail::NotificationLayoutResourceScope>& scope,
    const std::shared_ptr<const BnwWorldViewControlTree>& tree,
    const runtime::BnwWorldViewSourceOperationLoan& operation)
  :core_(std::move(core)),scope_(scope),tree_(tree),operation_(&operation),
   serial_(operation.operation_serial()),topology_(core_->topology_revision){}
void BnwWorldViewControlTree::LayoutResourceBinding::require_current_without_callbacks()const{
    core_->resource_first();
    require(core_->layout_resource_bound&&core_->layout_resource_operation==operation_&&
        core_->layout_resource_serial==serial_&&core_->topology_revision==topology_&&
        !tree_.expired()&&!scope_.expired()&&
        !core_->layout_resource_scope.owner_before(scope_)&&!scope_.owner_before(core_->layout_resource_scope)&&
        !core_->layout_resource_tree.owner_before(tree_)&&!tree_.owner_before(core_->layout_resource_tree),
        "Layout retained resource Tree/source binding retired or replaced");
    core_->require_layout_resources_owned();
}
BnwWorldViewControlTree::LayoutResourceBinding::~LayoutResourceBinding()noexcept{
    try{require_current_without_callbacks();
        core_->layout_resource_bound=false;core_->layout_resource_scope.reset();
        core_->layout_resource_tree.reset();core_->layout_resource_operation=nullptr;
    }catch(...){core_->failed.store(true);}
}
void BnwWorldViewControlTree::Core::resource_first()const{
    if(creator!=std::this_thread::get_id()){failed.store(true);require(false,"Layout raw resource foreign creator");}
    layout_origin(busy);
}
void BnwWorldViewControlTree::require_layout_resource_operation_without_callbacks(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto saved=core_;saved->resource_first();
    const auto& actual=saved->source->current_operation_loan();saved->resource_first();
    require(&actual==&operation&&actual.operation_serial()==operation.operation_serial(),
        "Layout raw resource Tree is not SAME entered Source operation");
    saved->resource_first();
}
std::unique_ptr<BnwWorldViewControlTree::LayoutResourceBinding>
BnwWorldViewControlTree::enter_layout_resource_scope_owned(
    const std::shared_ptr<detail::NotificationLayoutResourceScope>& scope,
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<const BnwWorldViewControlTree>& actual_tree){
    const auto saved=core_;saved->layout_origin();
    try{
        require(actual_tree&&actual_tree.get()==this&&actual_tree->core_.get()==saved.get()&&
            !actual_tree->core_.owner_before(saved)&&!saved.owner_before(actual_tree->core_),
            "Layout resource source slot is not actual Tree/Core");
        require(scope&&!saved->layout_resource_bound,"Layout resource Tree scope reentered");
        require_layout_resource_operation_without_callbacks(operation);
        scope->require_pure(operation,this);saved->layout_origin();
        auto binding=std::unique_ptr<LayoutResourceBinding>(new LayoutResourceBinding(saved,scope,actual_tree,operation));
        saved->layout_resource_scope=scope;saved->layout_resource_tree=actual_tree;
        saved->layout_resource_actual_tree=this;saved->layout_resource_operation=&operation;
        saved->layout_resource_serial=operation.operation_serial();saved->layout_resource_bound=true;
        binding->require_current_without_callbacks();return binding;
    }catch(...){saved->failed.store(true);throw;}
}
void BnwWorldViewControlTree::Core::require_layout_resources_owned()const{
    resource_first();
    // Parser prefixes may have invalidated a binding; these data-only rows
    // do not issue resource readiness. Every consumed earned row is checked.
    const auto check=[&](const auto& state,bool nine){
        if(!state.texture_binding_earned)return;
        if(state.texture_key.empty()){
            require(!state.texture_keeper,"Layout constructor NULL has a retained physical source");return;
        }
        require(layout_resource_bound&&layout_resource_operation&&!layout_resource_tree.expired(),
            "Layout consumed texture has no active real renderer resource scope");
        const auto scope=layout_resource_scope.lock();require(bool(scope),"Layout raw resource scope retired");
        scope->require_entry(*layout_resource_operation,layout_resource_actual_tree,state.texture_keeper);
        resource_first();
        require(state.texture_keeper&&state.texture_keeper->key_==state.texture_key,
            "Layout parsed key differs from retained selected texture");
        const auto fields=state.texture_keeper->native_114_118_;
        const std::array<std::uint32_t,2> dimensions=nine?
            std::array<std::uint32_t,2>{fields[1],fields[0]}:fields;
        // The consumer arrays deliberately have different field orders.
        if constexpr(requires{state.texture_extent_278;})
            require(state.texture_extent_278&&*state.texture_extent_278==dimensions,"NineSlice retained118/114 dimensions changed");
        else require(state.texture_extent_15c&&*state.texture_extent_15c==dimensions,"Button retained114/118 dimensions changed");
    };
    for(const auto& node:nodes)if(node.alive&&node.layout_baseline){
        const auto& state=*node.layout_baseline;
        if(state.slider_){check(state.slider_->track_,true);check(state.slider_->shuttle_,true);}
        if(state.button_)check(*state.button_,false);
        if(state.grid_)check(*state.grid_,true);
        if(state.image_){
            using Image=detail::NotificationLayoutImageState;const auto& image=*state.image_;
            if(image.texture_origin==Image::TextureOrigin::constructor_null){
                require(image.texture_key.empty()&&!image.texture_extent_c8&&!image.texture_keeper,
                    "Root Image constructor NULL observation substituted");
            }else if(image.texture_origin==Image::TextureOrigin::retained_raw){
                require(layout_resource_bound&&layout_resource_operation&&!layout_resource_tree.expired(),
                    "Root Image retained resource has no actual SAME scope");
                const auto scope=layout_resource_scope.lock();require(bool(scope),"Root Image retained resource scope retired");
                scope->require_entry(*layout_resource_operation,layout_resource_actual_tree,image.texture_keeper);resource_first();
                require(image.texture_keeper&&image.texture_keeper->key_==image.texture_key&&
                    image.texture_extent_c8&&*image.texture_extent_c8==image.texture_keeper->native_114_118_,
                    "Root Image retained raw key/114118 fields changed");
                const auto fields=image.texture_keeper->native_114_118_;
                require(image.resource_e8_ec==std::array<float,2>{static_cast<float>(fields[1]),static_cast<float>(fields[0])},
                    "Root Image actual118WIDTH/114HEIGHT parser operands changed");
            } // parsed_pending carries no positive resource grant and+10 binds it.
        }
    }
    resource_first();
}
void detail::NotificationLayoutParentReceiver::TreeView::initialize_slider_textures_owned(std::uint64_t id){
    const auto saved=binding_;saved->pure(true);
    try{
        auto& domain=saved->domains.front();const auto& actual=NotificationLayoutParentReceiver::node(*this,id);
        require(actual.state->kind_==NotificationLayoutBaseline::Kind::slider&&actual.state->slider_,
            "Layout resource initializer requires actual typed Slider");
        const auto state=actual.state->slider_;const auto scope=domain.core->layout_resource_scope.lock();
        require(scope&&domain.core->layout_resource_bound,"Slider actual resource scope unissued");
        const auto initialize=[&](auto& layer){
            saved->pure(true);const auto key=layer.texture_key;
            if(key.empty()){
                require(layer.texture_binding_earned&&!layer.texture_extent_278&&!layer.texture_keeper,
                    "Slider missing parsed Texture is not genuine constructor NULL");return;
            }
            auto owner=scope->load_owned(*saved->operation,domain.core->layout_resource_actual_tree,key);
            saved->first(true);saved->pure(true);
            scope->require_full(*saved->operation,domain.core->layout_resource_actual_tree);
            saved->first(true);saved->pure(true);
            require(layer.texture_key==key,"Slider Texture changed across source callback");
            preflight_store_owned({0,id});
            const auto fields=owner->native_114_118_;
            layer.texture_keeper=std::move(owner);layer.texture_extent_278=std::array<std::uint32_t,2>{fields[1],fields[0]};
            layer.texture_binding_earned=true;actual.state->record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);
            record_store_owned({0,id},Store::Kind::slider);saved->pure(true);
        };
        initialize(state->track_);initialize(state->shuttle_);saved->pure(true);
    }catch(...){saved->poison();throw;}
}
void detail::NotificationLayoutParentReceiver::TreeView::initialize_button_texture_owned(std::uint64_t id){
    const auto saved=binding_;saved->pure(true);
    try{
        auto& domain=saved->domains.front();const auto& actual=NotificationLayoutParentReceiver::node(*this,id);
        require(actual.state->kind_==NotificationLayoutBaseline::Kind::button&&actual.state->button_&&
            (actual.state->actual_type_=="UpButton"||actual.state->actual_type_=="DownButton"||actual.state->actual_type_=="Button"),
            "Layout resource initializer requires actual A3C9C0 Button");
        const auto state=actual.state->button_;const auto key=state->texture_key;
        if(key.empty()){
            require(state->texture_binding_earned&&!state->texture_extent_15c&&!state->texture_keeper,
                "Button missing parsed Texture is not genuine constructor NULL");return;
        }
        const auto scope=domain.core->layout_resource_scope.lock();
        require(scope&&domain.core->layout_resource_bound,"Button actual resource scope unissued");
        auto owner=scope->load_owned(*saved->operation,domain.core->layout_resource_actual_tree,key);
        saved->first(true);saved->pure(true);
        scope->require_full(*saved->operation,domain.core->layout_resource_actual_tree);
        saved->first(true);saved->pure(true);
        require(state->texture_key==key,"Button Texture changed across source callback");
        preflight_store_owned({0,id});
        if(actual.state->actual_type_=="Button")
            BnwNormalButtonSubobject118Backing::bind_source44_selected_keeper_owned(*state,owner);
        else {state->texture_keeper=std::move(owner);
            state->texture_extent_15c=state->texture_keeper->native_114_118_;state->texture_binding_earned=true;}
        actual.state->record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);
        record_store_owned({0,id},Store::Kind::slider);saved->pure(true);
    }catch(...){saved->poison();throw;}
}

void detail::NotificationLayoutParentReceiver::TreeView::initialize_image_texture_owned(NodeKey key){
    using Image=NotificationLayoutImageState;
    const auto saved=binding_;saved->pure(true);
    try{
        auto& domain=saved->domains.at(key.domain);const auto& actual=NotificationLayoutParentReceiver::node(*this,key);
        require(actual.state->kind_==NotificationLayoutBaseline::Kind::image&&actual.state->actual_type_=="Image"&&actual.state->image_,
            "Root Image resource binding requires actual typed canonical receiver");
        const auto state=actual.state->image_;const auto texture_key=state->texture_key;
        if(state->texture_origin==Image::TextureOrigin::constructor_null){
            require(texture_key.empty()&&!state->texture_extent_c8&&!state->texture_keeper,
                "Root Image constructor NULL resource observation substituted");return;
        }
        require(!texture_key.empty(),"Root Image parsed Texture cannot become native NULL");
        const auto scope=domain.core->layout_resource_scope.lock();
        require(scope&&domain.core->layout_resource_bound,"Root Image actual SAME resource scope unissued");
        auto owner=scope->load_owned(*saved->operation,domain.core->layout_resource_actual_tree,texture_key);
        saved->first(true);saved->pure(true);
        scope->require_full(*saved->operation,domain.core->layout_resource_actual_tree);
        saved->first(true);saved->pure(true);
        require(actual.state->image_.get()==state.get()&&!actual.state->image_.owner_before(state)&&
            !state.owner_before(actual.state->image_)&&state->texture_key==texture_key,
            "Root Image actual state/key changed across raw source callback");
        const auto fields=owner->native_114_118_;
        preflight_store_owned(key);
        state->texture_keeper=std::move(owner);state->texture_extent_c8=fields;
        state->resource_e8_ec={static_cast<float>(fields[1]),static_cast<float>(fields[0])};
        state->texture_origin=Image::TextureOrigin::retained_raw;
        actual.state->record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);
        record_store_owned(key,Store::Kind::image);saved->pure(true);
    }catch(...){saved->poison();throw;}
}
