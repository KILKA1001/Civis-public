// After complete canonical Core and selected Root registry provider.
// Real own normalized constructors/attachments, not Lua readiness receipts.
void BnwWorldViewControlTree::construct_selected_nested_contexts_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>& existing){
    const auto root=core_;
    if(root->context_direct_load_return){
        require(existing[0]&&existing[0].get()==this,"Direct nested reuse actual Root slot absent");
        require_direct_selected_client_registry_owned(operation);
        require(borrow_direct_loaded_native_slots_owned(operation,existing[0])==existing,
            "Direct nested reuse provided slots differ from actual RootRead facades");
        return;
    }
    std::vector<const Core*> path;
    std::vector<std::shared_ptr<Core>> affected{root};
    const auto first=[&](const std::shared_ptr<Core>& actual){
        root->layout_origin();actual->layout_origin();operation.require_origin_without_callbacks();
        root->layout_origin();actual->layout_origin();
        require(&root->source->current_operation_loan()==&operation&&
            &actual->source->current_operation_loan()==&operation&&
            actual->source.get()==root->source.get()&&!actual->source.owner_before(root->source)&&
            !root->source.owner_before(actual->source)&&actual->context_construction_return&&
            actual->context_gui_port.get()==root->context_gui_port.get()&&
            !actual->context_gui_port.owner_before(root->context_gui_port)&&
            !root->context_gui_port.owner_before(actual->context_gui_port),
            "Nested Context actual creator/Source/GUI/constructor differs");
    };
    try{
        first(root);require(root->context=="InGame"&&existing[0].get()==this&&
            root->selected_client_registry&&root->context_layout_children,
            "Nested Context requires actual completed Root construction list");
        const auto styles=root->source->selected_client_styles_owned(operation);first(root);
        const auto visit=[&](auto&& self,BnwWorldViewControlTree& tree)->void{
            const auto actual=tree.core_;first(actual);
            affected.push_back(actual);
            require(path.size()<128&&std::find(path.begin(),path.end(),actual.get())==path.end(),
                "Nested Context actual parent cycle/depth exceeded");
            path.push_back(actual.get());struct Pop{std::vector<const Core*>&p;~Pop(){p.pop_back();}}pop{path};
            if(!actual->context_layout_children){
                require(!actual->context_load_origin&&!actual->layout_resource_bound&&actual->context_parent_kind,
                    "Nested Context construction must precede real load/resource prefix");
                const auto layout=actual->source->authored_layout(actual->context);first(actual);
                struct Declaration{std::string id,stem;std::uint64_t parent,order;};
                std::vector<Declaration> declarations;std::set<std::string> names;
                std::size_t ordinary=0;std::uint64_t order=0;
                const auto walk=[&](auto&& scan,const content::XmlNode& node,std::uint64_t parent)->void{
                    if(node.name=="Instance"||node.name=="InstanceData")return;
                    if(node.name=="LuaContext"){
                        const auto stem=node.attribute("FileName");auto id=node.attribute("ID");
                        if(id.empty()){const auto slash=stem.find_last_of("/\\");id=stem.substr(slash==std::string::npos?0:slash+1);}
                        require(!stem.empty()&&!id.empty()&&names.insert(id).second&&order<UINT64_MAX,
                            "Nested Context actual declaration missing/repeated/exhausted");
                        declarations.push_back({id,stem,parent,++order});return;
                    }
                    if(node.name!="Context"){
                        require(ordinary<actual->nodes.size()&&order<UINT64_MAX,
                            "Nested Context ordinary traversal exceeds actual retained objects");
                        auto& value=actual->nodes.at(ordinary++);
                        require(value.alive&&!value.dynamic&&value.authored.name==node.name&&value.authored.attributes==node.attributes&&
                            actual->topology_revision<UINT64_MAX&&actual->effect_revision<UINT64_MAX,
                            "Nested Context actual ordinary source occurrence differs");
                        value.attached_order=++order;parent=value.id;
                        ++actual->topology_revision;++actual->effect_revision;
                    }
                    for(const auto& child:node.children)scan(scan,child,parent);
                };
                walk(walk,layout,0);first(actual);
                require(ordinary==actual->nodes.size()&&actual->next_attached_order<=order&&actual->effect_revision<UINT64_MAX,
                    "Nested Context actual ordinary/source order differs");
                actual->next_attached_order=order;++actual->effect_revision;
                {
                    actual->busy=true;struct End{bool&v;~End(){v=false;}}end{actual->busy};
                    if(!actual->context_layout_baseline->parsed_source_serial_)
                        tree.capture_layout_node_canonical_owned(0,styles,operation.operation_serial());
                }
                first(actual);std::size_t next_control=0;
                const auto ordinary_before=[&](std::uint64_t boundary){
                    while(next_control<actual->nodes.size()&&actual->nodes[next_control].attached_order<boundary){
                        first(actual);const auto id=actual->nodes[next_control].id;
                        {
                            actual->busy=true;struct End{bool&v;~End(){v=false;}}end{actual->busy};
                            if(!Core::node(actual->nodes,id).layout_baseline)
                                tree.capture_layout_node_canonical_owned(id,styles,operation.operation_serial());
                        }
                        first(actual);const auto& node=Core::node(actual->nodes,id);
                        require(node.layout_baseline&&node.layout_receiver_occurrence&&node.layout_baseline->parsed_source_serial_,
                            "Nested Context actual ordinary base/parser did not return");++next_control;
                    }
                };
                std::vector<Core::ContextLayoutChild> completed;completed.reserve(declarations.size());
                for(const auto& declaration:declarations){
                    ordinary_before(declaration.order);first(actual);
                    // A real prepared VFS descriptor is required. Missing
                    // scoped source owner stops before allocating this child;
                    // all earlier actual objects remain in their parent map.
                    const auto descriptor=actual->source->prepared_source(declaration.id);first(actual);
                    require(descriptor&&descriptor->context()==declaration.id&&
                        actual->source->selected_context_declared_parent_owned(declaration.id)==actual->context&&
                        assets::AssetKey(descriptor->asset().requested_key().original_name()).normalized_name()==
                            assets::AssetKey(declaration.stem+".lua").normalized_name(),
                        "Nested Context requires genuine scoped parent/selected VFS source descriptor");
                    std::shared_ptr<BnwWorldViewControlTree> child;
                    for(const auto& previous:existing){
                        require(static_cast<bool>(previous),"Nested Context prior actual native Tree retired");
                        previous->core_->layout_origin();first(actual);
                        if(previous->core_->context==declaration.id){require(!child,"Nested prior Tree identity ambiguous");child=previous;}
                    }
                    require(!actual->constructed_nested_contexts.contains(declaration.id),
                        "Nested Context actual retained constructor replay");
                    if(!child)child=capture_owned(actual->source,declaration.id,operation,actual->context_gui_port);
                    first(actual);first(child->core_);
                    require(actual->constructed_nested_contexts.emplace(declaration.id,child).second,
                        "Nested Context real child retention failed");
                    const auto c=child->core_;
                    require(!c->context_load_origin,"Nested Context construction cannot replace actual prior load");
                    affected.push_back(c);
                    if(c->context_parent_kind){
                        require_scalar_parent_attachment_owned(c,operation);first(actual);first(c);
                        const auto previous=c->context_parent_core.lock();
                        require(previous&&previous.get()==actual.get()&&!previous.owner_before(actual)&&!actual.owner_before(previous)&&
                            c->context_parent_control_id==declaration.parent&&c->scalar_parent_attachment&&
                            c->scalar_parent_attachment->source_order==declaration.order,
                            "Nested Context reused constructor/parent/declaration/order differs");
                    }else{
                        actual->context_layout_children.reset();actual->native_context_child_vector_completion.reset();
                        c->context_parent_core=actual;c->context_parent_control_id=declaration.parent;
                        c->context_parent_kind=declaration.parent?Core::ContextParentKind::canonical_control:Core::ContextParentKind::canonical_context;
                    }
                    {
                        c->busy=true;struct End{bool&v;~End(){v=false;}}end{c->busy};
                        if(!c->context_layout_baseline->parsed_source_serial_)
                            child->capture_layout_node_canonical_owned(0,styles,operation.operation_serial());
                    }
                    first(actual);first(c);
                    require(actual->topology_revision<UINT64_MAX&&actual->effect_revision<UINT64_MAX,
                        "Nested Context real attachment history exhausted");
                    completed.push_back({c,declaration.parent,declaration.order});
                    ++actual->topology_revision;++actual->effect_revision;
                    self(self,*child);first(actual);first(c);
                }
                ordinary_before(UINT64_MAX);first(actual);
                require(next_control==actual->nodes.size()&&completed.size()==declarations.size()&&
                    actual->constructed_nested_contexts.size()==declarations.size(),
                    "Nested Context actual construction/attachment traversal incomplete");
                actual->context_layout_children.emplace(std::move(completed));first(actual);
                return;
            }
            // An engaged vector is validated against real owning parent
            // storage, never synthesized from XML. Traverse in its source order.
            for(const auto& attached:*actual->context_layout_children){
                first(actual);const auto c=attached.core.lock();require(static_cast<bool>(c),"Nested actual attachment retired");
                first(c);std::shared_ptr<BnwWorldViewControlTree> child;
                if(actual.get()==root.get()){
                    const auto found=root->selected_client_registry->children.find(c->context);
                    require(found!=root->selected_client_registry->children.end(),"Nested Root child not retained by actual registry");child=found->second;
                }else{
                    auto found=actual->constructed_nested_contexts.find(c->context);
                    if(found==actual->constructed_nested_contexts.end()){
                        // Main949 Action's completed vector is already backed
                        // by its genuine Notification constructor/link command.
                        actual->require_native_context_child_vector_history_owned(false);
                        require(actual->native_context_child_vector_completion&&c->scalar_parent_attachment,
                            "Nested prior full vector lacks genuine retained child constructor");
                        require_scalar_parent_attachment_owned(c,operation);first(actual);first(c);
                        child=c->scalar_parent_attachment->child_tree.lock();
                        require(child&&child->core_.get()==c.get()&&!child->core_.owner_before(c)&&!c.owner_before(child->core_),
                            "Nested prior factory child facade/control differs");
                        require(actual->constructed_nested_contexts.emplace(c->context,child).second,
                            "Nested prior factory child retention duplicated");
                    }else child=found->second;
                }
                const auto p=c->context_parent_core.lock();
                require(child&&child->core_.get()==c.get()&&!child->core_.owner_before(c)&&!c.owner_before(child->core_)&&
                    p&&p.get()==actual.get()&&!p.owner_before(actual)&&!actual.owner_before(p)&&
                    c->context_parent_control_id==attached.parent_control_id&&c->context_parent_kind==
                        (attached.parent_control_id?Core::ContextParentKind::canonical_control:Core::ContextParentKind::canonical_context),
                    "Nested actual attachment parent/allocation/control differs");
                self(self,*child);first(actual);first(c);
            }
            first(actual);
        };
        // This minimum slice closes the selected WorldView subtree only.
        // Other Root subtrees keep their explicit scoped-source/load frontier.
        require(existing[1]&&existing[1]->core_->context=="WorldView",
            "Selected minimum World subtree actual prior constructor absent");
        visit(visit,*existing[1]);first(root);
        // Construction alone never dispatches original Lua. The existing
        // Context+10 body requires its real cursor/preparation/resource/
        // subtype+5761 owners before invoking the protected Lua host.
    }catch(...){for(const auto& actual:affected)actual->failed.store(true);root->failed.store(true);throw;}
}
