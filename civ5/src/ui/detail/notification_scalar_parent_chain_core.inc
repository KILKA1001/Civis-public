// Genuine existing NativeIssuer5 commands. Root/World history stays PARTIAL.
void BnwWorldViewControlTree::Core::require_native_context_child_vector_history_owned(bool expected_busy)const{
    layout_origin(expected_busy);
    const auto completed=native_context_child_vector_completion;
    if(!completed)return; // Other genuine constructor families retain their own issuers.
    require(context_layout_children&&completed->constructor&&completed->constructor==context_construction_return&&
        completed->constructor_source_serial&&completed->selected_document.name=="Context"&&
        completed->selected_document.attributes==root_authored.attributes&&
        (context=="NotificationPanel"||context=="ActionInfoPanel")&&
        completed->commands.size()==context_layout_children->size()&&
        completed->commands.size()==scalar_child_attachment_history.size(),
        "Native closed Context vector constructor/declaration history replaced");
    for(std::size_t i=0;i<completed->commands.size();++i){
        const auto& command=completed->commands[i];const auto& entry=context_layout_children->at(i);
        require(command&&command==scalar_child_attachment_history[i]&&command->parent_ctor==context_construction_return&&
            command->parent_name==context&&command->parent_control_id==entry.parent_control_id&&
            command->source_order==entry.attached_order,
            "Native closed Context vector actual ordered attachment replaced");
        const auto child=command->child_tree.lock(),parent=command->parent_tree.lock();
        const auto entry_core=entry.core.lock();
        require(child&&parent&&entry_core&&parent->core_.get()==this&&
            entry_core.get()==child->core_.get()&&!entry_core.owner_before(child->core_)&&
            !child->core_.owner_before(entry_core),
            "Native closed Context vector actual child/parent facade retired or replaced");
    }
    layout_origin(expected_busy);
}
void BnwWorldViewControlTree::require_scalar_parent_attachment_owned(
    const std::shared_ptr<Core>& child,const runtime::BnwWorldViewSourceOperationLoan& operation,
    bool child_expected_busy,bool parent_expected_busy){
    require(static_cast<bool>(child),"Scalar parent canonical child absent");child->layout_origin(child_expected_busy);
    operation.require_origin_without_callbacks();child->layout_origin(child_expected_busy);
    const auto link=child->scalar_parent_attachment;
    require(link&&link->constructor_serial&&link->child_ctor==child->context_construction_return&&
        child->context_construction_return&&child->context_layout_baseline&&
        link->child_name==child->context&&child->context_parent_kind&&
        &child->source->current_operation_loan()==&operation,
        "Scalar parent actual constructor/link history absent or replaced");
    const auto facade=link->child_tree.lock();
    require(facade&&facade->core_.get()==child.get()&&!facade->core_.owner_before(child)&&!child.owner_before(facade->core_),
        "Scalar parent actual child facade retired");
    if(*child->context_parent_kind==Core::ContextParentKind::native_null){
        require(child->context=="InGame"&&link->parent_name.empty()&&!link->parent_ctor&&
            link->parent_tree.expired()&&child->context_parent_core.expired()&&
            !child->context_parent_control_id&&!link->parent_control_id&&!link->source_order,
            "Scalar root is not its actual owned parentless NativeIssuer constructor");
    }else{
        const auto parent_tree=link->parent_tree.lock();const auto parent=child->context_parent_core.lock();
        require(parent_tree&&parent&&parent_tree->core_.get()==parent.get()&&
            !parent_tree->core_.owner_before(parent)&&!parent.owner_before(parent_tree->core_),
            "Scalar parent actual parent facade/Core retired");parent->layout_origin(parent_expected_busy);
        require(link->parent_ctor==parent->context_construction_return&&parent->context_construction_return&&
            link->parent_name==parent->context&&link->source_order&&
            link->parent_control_id==child->context_parent_control_id&&
            *child->context_parent_kind==(link->parent_control_id?Core::ContextParentKind::canonical_control:Core::ContextParentKind::canonical_context)&&
            parent->source.get()==child->source.get()&&!parent->source.owner_before(child->source)&&
            !child->source.owner_before(parent->source)&&parent->context_gui_port&&child->context_gui_port&&
            parent->context_gui_port.get()==child->context_gui_port.get()&&
            !parent->context_gui_port.owner_before(child->context_gui_port)&&!child->context_gui_port.owner_before(parent->context_gui_port),
            "Scalar parent actual constructor/source/GUI/control lineage differs");
        std::size_t count=0;
        for(const auto& entry:parent->scalar_child_attachment_history)
            if(entry.get()==link.get()&&!entry.owner_before(link)&&!link.owner_before(entry))++count;
        require(count==1,"Scalar parent actual attachment command absent/repeated");
        if(link->parent_control_id){const auto& node=Core::node(parent->nodes,link->parent_control_id);
            require(node.alive&&!node.dynamic,"Scalar parent actual authored control retired");}
        parent->layout_origin(parent_expected_busy);
    }
    operation.require_origin_without_callbacks();child->layout_origin(child_expected_busy);
}
void BnwWorldViewControlTree::attach_existing_native_scalar_parent_chain_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>& trees){
    operation.require_origin_without_callbacks();operation.require_render_operation_owned();
    constexpr const char* names[]={"InGame","WorldView","UnitFlagManager","ActionInfoPanel","NotificationPanel"};
    const auto serial=operation.operation_serial();
    const auto first=[&]{operation.require_origin_without_callbacks();
        for(std::size_t i=0;i<trees.size();++i){require(static_cast<bool>(trees[i]),"Scalar native actual slot absent");
            const auto actual=trees[i]->core_;actual->layout_origin();
            require(actual->context==names[i]&&actual->context_construction_return&&actual->context_layout_baseline&&
                &actual->source->current_operation_loan()==&operation&&actual->source.get()==trees[0]->core_->source.get()&&
                !actual->source.owner_before(trees[0]->core_->source)&&!trees[0]->core_->source.owner_before(actual->source),
                "Scalar parent chain actual NativeIssuer/source/constructor differs");
            require(actual->context_gui_port&&trees[0]->core_->context_gui_port&&
                actual->context_gui_port.get()==trees[0]->core_->context_gui_port.get()&&
                !actual->context_gui_port.owner_before(trees[0]->core_->context_gui_port)&&
                !trees[0]->core_->context_gui_port.owner_before(actual->context_gui_port),
                "Scalar parent chain has another genuine GUI/window creation realm");
        }};
    try{
        first();trees[0]->core_->context_gui_port->require_current_owned(operation);first();
        const auto root=trees[0]->core_;
        if(!root->scalar_parent_attachment){
            require(!root->context_parent_kind&&!root->context_load_origin,
                "Scalar own Root parent command cannot replace a prior parent/load");
            const auto layout=root->source->authored_layout("InGame");first();
            require(layout.name=="Context"&&layout.attribute("Name")=="InGame",
                "Scalar own Root creator differs from its actual selected document");
            auto command=std::make_shared<const Core::ScalarParentAttachment>(trees[0],nullptr,0,0,serial);
            require(root->effect_revision<UINT64_MAX,"Scalar Root parent store history exhausted");
            root->scalar_parent_attachment=std::move(command);
            // Actual normalized parentless ROOT constructor. This is not a
            // claim about a retail App pointer or viewport dimensions.
            root->context_parent_kind=Core::ContextParentKind::native_null;
            ++root->effect_revision;
        }
        require_scalar_parent_attachment_owned(root,operation);first();
        constexpr std::size_t parents[]={0,0,0,1,3};
        constexpr const char* stems[]={"","Assets/UI/InGame/WorldView/WorldView","Assets/UI/InGame/UnitFlagManager",
            "Assets/UI/InGame/WorldView/ActionInfoPanel","Assets/UI/InGame/WorldView/NotificationPanel"};
        for(std::size_t i=1;i<trees.size();++i){
            const auto child=trees[i]->core_,parent=trees[parents[i]]->core_;
            if(child->scalar_parent_attachment){require_scalar_parent_attachment_owned(child,operation);first();continue;}
            require(!child->context_parent_kind&&!child->context_load_origin,
                "Scalar actual child parent command cannot replace prior parent/load");
            const auto layout=parent->source->authored_layout(parent->context);first();
            std::uint64_t order=0,parent_control=0,child_order=0;std::size_t ordinary=0,matches=0;
            const auto walk=[&](auto&& self,const content::XmlNode& node,std::uint64_t control)->void{
                if(node.name=="Instance"||node.name=="InstanceData")return;
                if(node.name=="LuaContext"){
                    require(order<UINT64_MAX,"Scalar original declaration order exhausted");++order;
                    if(node.attribute("ID")==child->context){
                        require(node.attribute("FileName")==stems[i]&&node.children.empty(),
                            "Scalar actual parent declaration has another source/embedded merge");
                        parent_control=control;child_order=order;++matches;
                    }return;
                }
                if(node.name!="Context"){
                    require(ordinary<parent->nodes.size()&&order<UINT64_MAX,"Scalar actual ordinary/source traversal differs");
                    const auto& value=parent->nodes.at(ordinary++);
                    require(value.alive&&!value.dynamic&&value.authored.name==node.name&&value.authored.attributes==node.attributes,
                        "Scalar actual parent receiver is not its original XML occurrence");
                    ++order;control=value.id;
                }
                for(const auto& item:node.children)self(self,item,control);
            };
            walk(walk,layout,0);first();
            require(matches==1&&ordinary==parent->nodes.size()&&child_order,
                "Scalar actual parent declaration missing/ambiguous");
            parent->scalar_child_attachment_history.reserve(parent->scalar_child_attachment_history.size()+1);first();
            auto command=std::make_shared<const Core::ScalarParentAttachment>(trees[i],trees[parents[i]],parent_control,child_order,serial);
            require(child->effect_revision<UINT64_MAX&&parent->effect_revision<UINT64_MAX&&
                child->topology_revision<UINT64_MAX&&parent->topology_revision<UINT64_MAX,
                "Scalar actual attachment store history exhausted");
            // A subsequent genuine child attachment invalidates the old full
            // vector BEFORE the new child becomes observable; no stale completion.
            parent->context_layout_children.reset();
            parent->native_context_child_vector_completion.reset();
            parent->scalar_child_attachment_history.push_back(command);
            child->scalar_parent_attachment=command;child->context_parent_core=parent;
            child->context_parent_control_id=parent_control;
            child->context_parent_kind=parent_control?Core::ContextParentKind::canonical_control:Core::ContextParentKind::canonical_context;
            ++child->effect_revision;++child->topology_revision;++parent->effect_revision;++parent->topology_revision;
            require_scalar_parent_attachment_owned(child,operation);first();
        }
        operation.require_operation(serial);first();
        for(const auto& tree:trees)require_scalar_parent_attachment_owned(tree->core_,operation);
        first();
        // Finalize only the two genuinely closed existing native factories.
        // Root/World have additional unconstructed declarations and stay OPEN.
        // This is not enumeration of the five slots as a complete namespace.
        for(const std::size_t index:{std::size_t{3},std::size_t{4}}){
            const auto actual=trees[index]->core_;first();
            if(actual->native_context_child_vector_completion){
                actual->require_native_context_child_vector_history_owned(false);first();continue;
            }
            require(!actual->context_layout_children&&!actual->context_load_origin,
                "Native closed Context factory completion cannot replace a prior vector/load");
            const auto layout=actual->source->authored_layout(actual->context);first();
            require(layout.name=="Context"&&layout.attributes==actual->root_authored.attributes,
                "Native closed Context factory selected document differs");
            std::vector<Core::ContextLayoutChild> completed;
            std::vector<std::shared_ptr<const Core::ScalarParentAttachment>> commands;
            std::set<std::string> declarations;
            std::size_t ordinary=0;std::uint64_t order=0;
            const auto walk=[&](auto&& self,const content::XmlNode& node,std::uint64_t control)->void{
                if(node.name=="Instance"||node.name=="InstanceData")return;
                if(node.name=="LuaContext"){
                    require(order<UINT64_MAX,"Native closed Context declaration order exhausted");++order;
                    const auto id=node.attribute("ID");
                    require(!id.empty()&&declarations.insert(id).second&&node.children.empty(),
                        "Native closed Context declaration missing ID/duplicate/embedded override");
                    std::shared_ptr<Core> child;
                    for(const auto& tree:trees)if(tree->core_->context==id){
                        require(!child,"Native closed Context actual child ambiguous");child=tree->core_;
                    }
                    require(child&&child.get()!=actual.get(),
                        "Native closed Context required child factory has not returned");
                    require_scalar_parent_attachment_owned(child,operation);first();
                    const auto command=child->scalar_parent_attachment;
                    const auto parent=child->context_parent_core.lock();
                    require(parent&&parent.get()==actual.get()&&!parent.owner_before(actual)&&!actual.owner_before(parent)&&
                        command->parent_name==actual->context&&command->child_name==id&&
                        command->parent_control_id==control&&command->source_order==order&&
                        command->parent_ctor==actual->context_construction_return&&
                        command->child_ctor==child->context_construction_return,
                        "Native closed Context child source-order/constructor/attachment differs");
                    completed.push_back({child,control,order});commands.push_back(command);return;
                }
                if(node.name!="Context"){
                    require(ordinary<actual->nodes.size()&&order<UINT64_MAX,
                        "Native closed Context actual ordinary factory traversal incomplete");
                    const auto& built=actual->nodes[ordinary++];++order;
                    require(built.alive&&!built.dynamic&&built.parent==control&&built.authored.name==node.name&&
                        built.authored.attributes==node.attributes,
                        "Native closed Context ordinary receiver/source occurrence differs");
                    control=built.id;
                }
                for(const auto& child:node.children)self(self,child,control);
            };
            walk(walk,layout,0);first();
            require(ordinary==actual->nodes.size()&&commands.size()==actual->scalar_child_attachment_history.size(),
                "Native closed Context factory traversal omitted an actual child/ordinary receiver");
            for(std::size_t n=0;n<commands.size();++n)
                require(commands[n]==actual->scalar_child_attachment_history[n],
                    "Native closed Context factory actual attachment order differs");
            auto completion=std::make_shared<const Core::NativeContextChildVectorCompletion>(
                actual->context_construction_return,layout,std::move(commands),serial);first();
            actual->source->require_sources_current();first();operation.require_operation(serial);first();
            require(actual->effect_revision<UINT64_MAX&&!actual->context_layout_children&&
                !actual->native_context_child_vector_completion,
                "Native closed Context vector publication replay/history exhausted");
            actual->context_layout_children.emplace(std::move(completed));
            actual->native_context_child_vector_completion=std::move(completion);++actual->effect_revision;
            actual->require_native_context_child_vector_history_owned(false);first();
        }
        // No initialized Context, script/API readiness or fullframe return.
    }catch(...){for(const auto& tree:trees)if(tree)tree->core_->failed.store(true);throw;}
}
void BnwWorldViewControlTree::initialize_scalar_parent_baselines_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::map<std::string,content::XmlNode,std::less<>>& styles){
    const auto selected=core_;selected->layout_origin();
    std::vector<std::shared_ptr<BnwWorldViewControlTree>> chain;
    std::set<const Core*> seen;auto current=selected;
    try{
        for(;;){
            current->layout_origin();require_scalar_parent_attachment_owned(current,operation);
            require(chain.size()<128&&seen.insert(current.get()).second,"Scalar actual parent constructor cycle/depth exceeded");
            const auto facade=current->scalar_parent_attachment->child_tree.lock();
            require(facade&&facade->core_.get()==current.get(),"Scalar ancestor actual parser facade retired");chain.push_back(facade);
            if(*current->context_parent_kind==Core::ContextParentKind::native_null)break;
            const auto parent=current->context_parent_core.lock();require(static_cast<bool>(parent),"Scalar actual ancestor retired");current=parent;
        }
        std::reverse(chain.begin(),chain.end());
        for(const auto& tree:chain){const auto actual=tree->core_;actual->layout_origin();
            require_scalar_parent_attachment_owned(actual,operation);
            actual->context_gui_port->require_current_owned(operation);actual->layout_origin();
            if(actual->layout_styles){require(Core::same_layout_styles(*actual->layout_styles,styles),"Scalar actual retained styles changed");continue;}
            require(actual->context_construction_return&&actual->context_layout_baseline&&!actual->context_load_origin,
                "Scalar ancestor parser must precede its authentic load");
            actual->busy=true;struct End{bool&v;~End(){v=false;}}end{actual->busy};
            if(!actual->context_layout_baseline->parsed_source_serial_)
                tree->capture_layout_node_canonical_owned(0,styles,operation.operation_serial());
            for(const auto& node:actual->nodes)if(node.alive){
                if(!node.layout_baseline)tree->capture_layout_node_canonical_owned(node.id,styles,operation.operation_serial());
                require(static_cast<bool>(Core::node(actual->nodes,node.id).layout_receiver_occurrence),
                    "Scalar actual ancestor node has no original constructor/parser occurrence");
            }
            actual->source->require_sources_current();actual->layout_origin(true);
            const auto serial=operation.operation_serial();actual->layout_origin(true);
            operation.require_operation(serial);actual->layout_origin(true);
            actual->layout_styles.emplace(styles);require_scalar_parent_attachment_owned(actual,operation,true,false);
        }
        for(const auto& tree:chain){tree->core_->layout_origin();require_scalar_parent_attachment_owned(tree->core_,operation);}
        selected->layout_origin();
        // Real canonical parser/runtime dimensions handle F,F from current
        // GUI/window. No viewport values or completed5761/+10 are invented.
    }catch(...){for(const auto& tree:chain)tree->core_->failed.store(true);selected->failed.store(true);throw;}
}
