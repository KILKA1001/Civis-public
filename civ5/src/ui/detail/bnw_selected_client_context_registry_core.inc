// Same canonical Core/Source; selected client construction, not GUI slots.
void BnwWorldViewControlTree::construct_selected_client_context_registry_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>& existing){
    const auto root=core_;
    if(root->context_direct_load_return){adopt_direct_selected_client_registry_owned(operation,existing);return;}
    const auto pure=[&]{root->layout_origin();operation.require_origin_without_callbacks();root->layout_origin();
        require(root->context=="InGame"&&&root->source->current_operation_loan()==&operation&&
            existing[0]&&existing[0].get()==this&&existing[0]->core_.get()==root.get(),
            "Selected client registry actual root/Source/native slot differs");};
    try{
        pure();if(root->selected_client_registry&&root->context_layout_children&&
            root->selected_client_registry->children.size()==root->selected_client_registry->declarations.size()){
            require_selected_client_context_registry_owned(operation);return;
        }
        require(root->context_construction_return&&root->context_gui_port&&root->context_layout_baseline&&
            !root->context_load_origin&&!root->layout_resource_bound,
            "Selected client context construction must precede genuine root load");
        const auto gui=root->context_gui_port;gui->require_current_owned(operation);pure();
        const auto layout=root->source->authored_layout("InGame");pure();
        if(!root->selected_client_registry){
            auto registry=std::make_unique<Core::SelectedClientContextRegistry>();registry->serial=operation.operation_serial();
            std::uint64_t order=0;std::size_t ordinary=0;
            const auto walk=[&](auto&& self,const content::XmlNode& xml,std::uint64_t parent)->void{
                if(xml.name=="Instance"||xml.name=="InstanceData")return;
                if(xml.name=="LuaContext"){
                    auto id=xml.attribute("ID");const auto stem=xml.attribute("FileName");
                    if(id.empty()){const auto slash=stem.find_last_of("/\\");id=stem.substr(slash==std::string::npos?0:slash+1);}
                    require(!id.empty()&&!stem.empty()&&order<UINT64_MAX,
                        "Selected client child declaration lacks genuine identity");
                    require(registry->declarations.emplace(id,Core::SelectedClientContextRegistry::Declaration{
                        stem,parent,++order,xml}).second,"Selected client child declaration repeated");return;
                }
                if(xml.name!="Context"){
                    require(ordinary<root->nodes.size()&&order<UINT64_MAX,
                        "Selected client actual ordinary control/source traversal differs");
                    auto& node=root->nodes.at(ordinary++);
                    require(node.alive&&!node.dynamic&&node.authored.name==xml.name&&node.authored.attributes==xml.attributes,
                        "Selected client canonical ordinary receiver is not its exact source occurrence");
                    require(root->topology_revision<UINT64_MAX&&root->effect_revision<UINT64_MAX,
                        "Selected client actual ordinary attachment history exhausted");
                    // Genuine own attachment-order command over the existing
                    // canonical object, interleaved with Context declarations.
                    node.attached_order=++order;parent=node.id;
                    ++root->topology_revision;++root->effect_revision;
                }
                for(const auto& child:xml.children)self(self,child,parent);
            };
            walk(walk,layout,0);pure();
            require(ordinary==root->nodes.size(),"Selected client actual root ordinary object traversal incomplete");
            require(root->next_attached_order<=order&&root->effect_revision<UINT64_MAX,
                "Selected client actual interleaved attachment counter changed");
            root->next_attached_order=order;++root->effect_revision;
            registry->history.reserve(registry->declarations.size());
            // Commit the actual construction owner before child callbacks:
            // a failure retains earned prior objects, never inventing return.
            root->selected_client_registry=std::move(registry);
            // Actual Main946 root command already earned this parentless
            // construction. Reuse its exact constructor/facade/Source receipt.
            require_scalar_parent_attachment_owned(root,operation);
            require(root->context_parent_kind==Core::ContextParentKind::native_null,
                "Selected client actual root constructor parent differs");
        }
        pure();auto& registry=*root->selected_client_registry;
        require(root->context_parent_kind==Core::ContextParentKind::native_null&&
            registry.history.size()==registry.children.size(),"Selected client construction realm/history differs");
        const auto styles=root->source->selected_client_styles_owned(operation);pure();
        {
            root->busy=true;struct End{bool&v;~End(){v=false;}}end{root->busy};
            if(!root->context_layout_baseline->parsed_source_serial_)capture_layout_node_canonical_owned(0,styles,registry.serial);
        }
        pure();
        std::vector<std::pair<std::string,Core::SelectedClientContextRegistry::Declaration>> ordered;
        for(const auto& entry:registry.declarations)ordered.push_back(entry);
        std::sort(ordered.begin(),ordered.end(),[](const auto& x,const auto& y){return x.second.order<y.second.order;});
        std::size_t next_control=0;
        const auto construct_controls_before=[&](std::uint64_t boundary){
            // The canonical root node vector is the real source preorder.
            // Earn ordinary base constructors/parsers in the interleaved
            // declaration order, not only the ancestors of YieldIconManager.
            while(next_control<root->nodes.size()&&root->nodes[next_control].attached_order<boundary){
                pure();const auto control=root->nodes[next_control].id;
                {
                    root->busy=true;struct End{bool&v;~End(){v=false;}}end{root->busy};
                    if(!Core::node(root->nodes,control).layout_baseline)
                        capture_layout_node_canonical_owned(control,styles,registry.serial);
                }
                pure();const auto& node=Core::node(root->nodes,control);
                require(node.layout_baseline&&node.layout_receiver_occurrence&&node.layout_baseline->parsed_source_serial_,
                    "Selected client ordinary base construction/parser did not return");
                ++next_control;
            }
        };
        for(const auto& [id,declaration]:ordered){
            construct_controls_before(declaration.order);
            pure();if(registry.children.contains(id))continue;
            // Initialize only actual ancestor receivers needed by this source
            // attachment. Other control subtype/load effects remain explicit.
            {
                root->busy=true;struct End{bool&v;~End(){v=false;}}end{root->busy};
                std::vector<std::uint64_t> parents;auto parent=declaration.parent;
                while(parent){parents.push_back(parent);parent=Core::node(root->nodes,parent).parent;}
                std::reverse(parents.begin(),parents.end());
                for(auto control:parents)if(!Core::node(root->nodes,control).layout_baseline)
                    capture_layout_node_canonical_owned(control,styles,registry.serial);
            }
            pure();std::shared_ptr<BnwWorldViewControlTree> child;
            // Canonical previously allocated native Tree identities are reused.
            // No replacement WorldView/UnitFlag object/VM/controller is minted.
            for(std::size_t i=1;i<existing.size();++i){
                require(static_cast<bool>(existing[i]),"Selected client genuine prior native Tree retired");
                existing[i]->core_->layout_origin();
                if(existing[i]->core_->context==id){require(!child,"Selected client prior native Tree ambiguous");child=existing[i];}
            }
            if(!child)child=capture_owned(root->source,id,operation,gui);pure();
            const auto actual=child->core_;actual->layout_origin();
            require(actual->context_construction_return&&actual->context_gui_port.get()==gui.get()&&
                !actual->context_gui_port.owner_before(gui)&&!gui.owner_before(actual->context_gui_port)&&
                actual->source.get()==root->source.get()&&!actual->source.owner_before(root->source)&&!root->source.owner_before(actual->source),
                "Selected client actual Context constructor/GUI/Source differs");
            // Retain the actual returned object immediately. If a later
            // parser/attachment callback fails, its earned fields remain in
            // this poisoned owner rather than disappearing with a local.
            require(registry.children.emplace(id,child).second,"Selected client Context allocated twice");
            require(!actual->context_load_origin,"Selected client construction cannot replace an actual loaded Context");
            if(actual->context_parent_kind){
                require_scalar_parent_attachment_owned(actual,operation);pure();actual->layout_origin();
                const auto previous=actual->context_parent_core.lock();
                require(previous&&previous.get()==root.get()&&!previous.owner_before(root)&&!root.owner_before(previous)&&
                    actual->context_parent_control_id==declaration.parent&&actual->scalar_parent_attachment&&
                    actual->scalar_parent_attachment->source_order==declaration.order,
                    "Selected client existing actual parent/declaration/order differs");
            }else{
                root->context_layout_children.reset();root->native_context_child_vector_completion.reset();
                actual->context_parent_core=root;actual->context_parent_control_id=declaration.parent;
                actual->context_parent_kind=declaration.parent?Core::ContextParentKind::canonical_control:Core::ContextParentKind::canonical_context;
            }
            const auto child_layout=actual->source->authored_layout(id);pure();actual->layout_origin();
            bool nested=false;
            const auto inspect=[&](auto&& self,const content::XmlNode& node)->void{
                if(node.name=="Instance"||node.name=="InstanceData")return;
                if(node.name=="LuaContext"){nested=true;return;}
                for(const auto& c:node.children)self(self,c);
            };
            inspect(inspect,child_layout);
            if(!nested)actual->context_layout_children.emplace();
            {
                actual->busy=true;struct End{bool&v;~End(){v=false;}}end{actual->busy};
                if(!actual->context_layout_baseline->parsed_source_serial_)child->capture_layout_node_canonical_owned(0,styles,registry.serial);
            }
            pure();actual->layout_origin();
            require(root->topology_revision<UINT64_MAX&&root->effect_revision<UINT64_MAX&&
                registry.history.capacity()>registry.history.size(),"Selected client actual attachment history exhausted");
            // Retained object is now attached; vector capacity was reserved
            // before creation. This is an attachment, not Lua load/handler ready.
            registry.history.push_back({id,declaration.parent,declaration.order,registry.serial});
            ++root->topology_revision;++root->effect_revision;pure();
        }
        construct_controls_before(UINT64_MAX);
        require(next_control==root->nodes.size(),"Selected client ordinary construction list incomplete");
        pure();require_selected_client_context_registry_owned(operation);
        require(registry.children.size()==registry.declarations.size(),"Selected client actual construction list is incomplete");
        // The Context attachment list is earned only after ALL actual object
        // constructors/parsers and source-ordered attachments have returned.
        if(!root->context_layout_children){
            std::vector<Core::ContextLayoutChild> children;children.reserve(registry.history.size());
            for(const auto& entry:registry.history)children.push_back({registry.children.at(entry.id)->core_,entry.parent,entry.order});
            root->context_layout_children.emplace(std::move(children));
        }
        pure();
        // Own object namespace now has the actual full declaration list and
        // ordinary base objects. Subtype/+10 effects, nested Context lists and
        // scripts/VM/environments/handlers are not run or marked ready here.
    }catch(...){root->failed.store(true);throw;}
}
void BnwWorldViewControlTree::require_selected_client_context_registry_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto root=core_;
    if(root->context_direct_load_return){require_direct_selected_client_registry_owned(operation);return;}
    root->layout_origin();operation.require_origin_without_callbacks();root->layout_origin();
    require(root->context=="InGame"&&root->selected_client_registry&&root->context_construction_return&&
        root->context_parent_kind==Core::ContextParentKind::native_null&&root->context_gui_port&&
        &root->source->current_operation_loan()==&operation,"Selected client registry construction owner absent");
    const auto& registry=*root->selected_client_registry;
    require(registry.serial&&registry.history.size()==registry.children.size(),"Selected client actual attachment history differs");
    for(const auto& entry:registry.history){
        const auto found=registry.children.find(entry.id);const auto declared=registry.declarations.find(entry.id);
        require(found!=registry.children.end()&&declared!=registry.declarations.end()&&found->second&&
            entry.parent==declared->second.parent&&entry.order==declared->second.order&&entry.serial==registry.serial,
            "Selected client attached Context declaration/history differs");
        const auto& actual=found->second->core_;actual->layout_origin();
        const auto parent=actual->context_parent_core.lock();
        require(parent&&parent.get()==root.get()&&!parent.owner_before(root)&&!root.owner_before(parent)&&
            actual->context_parent_control_id==entry.parent&&actual->context_parent_kind==
                (entry.parent?Core::ContextParentKind::canonical_control:Core::ContextParentKind::canonical_context)&&
            actual->source.get()==root->source.get()&&!actual->source.owner_before(root->source)&&!root->source.owner_before(actual->source)&&
            actual->context_construction_return&&actual->context_layout_baseline&&actual->context_layout_baseline->parsed_source_serial_&&
            actual->context_gui_port.get()==root->context_gui_port.get()&&
            !actual->context_gui_port.owner_before(root->context_gui_port)&&
            !root->context_gui_port.owner_before(actual->context_gui_port),
            "Selected client same actual child/parent/constructor/parser/GUI retired");
    }
    if(root->context_layout_children){
        require(root->context_layout_children->size()==registry.history.size()&&
            registry.children.size()==registry.declarations.size(),
            "Selected client earned full attachment vector changed");
        for(std::size_t i=0;i<registry.history.size();++i){
            const auto& entry=registry.history[i];const auto& attached=(*root->context_layout_children)[i];
            const auto actual=registry.children.at(entry.id)->core_;const auto held=attached.core.lock();
            require(held&&held.get()==actual.get()&&!held.owner_before(actual)&&!actual.owner_before(held)&&
                attached.parent_control_id==entry.parent&&attached.attached_order==entry.order,
                "Selected client actual attachment vector identity/order differs");
        }
    }
    root->layout_origin();operation.require_origin_without_callbacks();root->layout_origin();
}
//Canonical registry-backed parser entry; scalar-five receipt is NOT its issuer.
void BnwWorldViewControlTree::prepare_selected_client_registry_baselines_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::map<std::string,content::XmlNode,std::less<>>& borrowed_styles){
    const auto saved=core_;
    if(saved->context_direct_load_return){require_direct_registry_baselines_owned(operation,borrowed_styles);return;}
    saved->layout_origin();operation.require_origin_without_callbacks();saved->layout_origin();
    const auto styles=borrowed_styles;
    const auto pure=[&]{saved->layout_origin();operation.require_origin_without_callbacks();saved->layout_origin();
        require(&saved->source->current_operation_loan()==&operation,"Registry canonical parser Source replaced");};
    try{
        pure();const auto actual_root=saved->source->selected_client_context_slot1_owned(operation);pure();
        require(actual_root&&actual_root.get()==this&&actual_root->core_==saved,
            "Registry parser requires actual published canonical client Root");
        //Repeat is admitted FIRST by the real published final initializer,
        //never an initialparser static-only rule or a rendererready boolean.
        if(saved->layout_initialization_return){
            const auto seal=saved->layout_initialization_return;const auto completed=seal->native_return_;
            require(completed&&completed->impl_&&completed->impl_->source_serial==operation.operation_serial(),
                "Registry parser reuse requires genuine same-current-Source final initializer cohort");
            require_context_initialization_return_owned(saved,completed);pure();
            for(const auto& domain:completed->impl_->domains){const auto c=domain.core.lock();
                require(bool(c),"Registry completed parser cohort domain retired");c->layout_origin();c->require_layout_resources_owned();
                require(c->source==saved->source&&!c->source.owner_before(saved->source)&&!saved->source.owner_before(c->source)&&
                    c->layout_styles&&Core::same_layout_styles(*c->layout_styles,styles),
                    "Registry completed parser cohort retained full styles/Source changed");
                const auto verify=[&](const detail::NotificationLayoutBaseline& b,const content::XmlNode& authored){
                    const auto expected=Core::layout_parser_layers(authored,styles);
                    require(b.parsed_source_serial_&&b.parser_layers_.size()==expected.size(),
                        "Registry completed parser metadata count changed");
                    for(std::size_t i=0;i<expected.size();++i)require(Core::same_layout_xml(b.parser_layers_[i],expected[i]),
                        "Registry completed actual used parser metadata changed");
                };
                require(c->context_layout_baseline&&c->context_receiver_occurrence,
                    "Registry completed Context canonical parser/allocation absent");
                verify(*c->context_layout_baseline,c->root_authored);
                //No new dynamic capability is issued: finalcohort/lifetime/epoch
                //is mandatory; this reads existing real owned parser metadata only.
                for(const auto& n:c->nodes)if(n.alive){require(n.layout_baseline&&n.layout_receiver_occurrence,
                        "Registry completed actual node parser/allocation absent");
                    verify(*n.layout_baseline,n.layout_parser_authored.value_or(n.authored));}
                pure();
            }
            require_context_initialization_return_owned(saved,completed);pure();
            require(saved->layout_initialization_return==seal&&seal->native_return_==completed,
                "Registry completed occurrence replaced across repeat metadata validation");return;
        }
        require_selected_client_context_registry_owned(operation);pure();
        //This exact existing cohort is Root+ALLgenuine directconstructors plus
        //the physicallyconstructed WorldView subtree, not discoveredXML objects.
        const auto trees=collect_selected_client_retained_context_trees_owned(operation,actual_root);pure();
        require(!trees.empty()&&trees.front()==actual_root,"Registry parser actual constructor cohort missing");
        for(const auto& tree:trees){pure();require(bool(tree),"Registry parser actual child facade absent");
            const auto c=tree->core_;c->layout_origin();c->require_layout_resources_owned();
            require(c->context_construction_return&&c->context_layout_baseline&&c->context_gui_port==saved->context_gui_port&&
                c->source==saved->source&&!c->source.owner_before(saved->source)&&!saved->source.owner_before(c->source)&&
                c->layout_resource_tree.lock()==tree&&c->layout_resource_actual_tree==tree.get()&&
                c->layout_resource_operation==&operation&&c->layout_resource_serial==operation.operation_serial(),
                "Registry parser actual constructor/Source/GUI/resource lender differs");
            const auto verify_layers=[&](const detail::NotificationLayoutBaseline& b,const content::XmlNode& authored){
                const auto expected=Core::layout_parser_layers(authored,styles);
                require(b.parsed_source_serial_&&b.parser_layers_.size()==expected.size(),"Registry retained canonical parser layer count differs");
                for(std::size_t i=0;i<expected.size();++i)require(Core::same_layout_xml(b.parser_layers_[i],expected[i]),
                    "Registry actual used style/authored parser layers changed");
            };
            if(c->layout_styles)require(Core::same_layout_styles(*c->layout_styles,styles),"Registry retained full style namespace differs");
            const auto serial=operation.operation_serial();c->layout_origin();
            if(!c->context_layout_baseline->parsed_source_serial_){
                require(!c->context_load_origin,"Registry root parser cannot run after native preload");
                c->busy=true;struct End{bool& value;~End(){value=false;}}end{c->busy};
                tree->capture_layout_node_canonical_owned(0,styles,serial);
            }
            c->layout_origin();verify_layers(*c->context_layout_baseline,c->root_authored);
            std::vector<std::uint64_t> actual_nodes;
            for(const auto& n:c->nodes)if(n.alive){require(!n.dynamic,"Registry initial parser cannot infer dynamic instance constructor");actual_nodes.push_back(n.id);}
            for(const auto id:actual_nodes){pure();c->layout_origin();const auto& n=Core::node(c->nodes,id);
                if(!n.layout_baseline||!n.layout_baseline->parsed_source_serial_){
                    require(!c->context_load_origin,"Registry ordinary parser cannot run after native preload");
                    c->busy=true;struct End{bool& value;~End(){value=false;}}end{c->busy};
                    tree->capture_layout_node_canonical_owned(id,styles,serial);
                }
                c->layout_origin();const auto& returned=Core::node(c->nodes,id);
                require(returned.layout_baseline&&returned.layout_receiver_occurrence&&returned.layout_baseline->parsed_source_serial_,
                    "Registry actual ordinary constructor/parser did not return");
                verify_layers(*returned.layout_baseline,returned.layout_parser_authored.value_or(returned.authored));
            }
            c->layout_origin();operation.require_current();pure();c->layout_origin();c->require_layout_resources_owned();
            require(c->nodes.size()==actual_nodes.size(),"Registry ordinary constructor set changed across parser POST");
            if(!c->layout_styles)c->layout_styles.emplace(styles);
            else require(Core::same_layout_styles(*c->layout_styles,styles),"Registry full style namespace changed across POST");
            pure();
        }
        require_selected_client_context_registry_owned(operation);pure();
        //Only genuine existing base constructor/parser closure. No missing
        //nested vector, subtype/+10, Lua environment or initialized seal issued.
    }catch(...){saved->failed.store(true);throw;}
}
std::shared_ptr<BnwWorldViewControlTree> BnwWorldViewControlTree::lookup_selected_client_context_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,std::string_view id)const{
    require_selected_client_context_registry_owned(operation);const auto root=core_;
    const auto& registry=*root->selected_client_registry;
    // No subset NULL. Every original declaration must have its actual child
    // construction/attachment, plus the ordinary physical receiver constructors.
    // Lua/script readiness is a distinct mandatory load phase, not this lookup.
    require(registry.children.size()==registry.declarations.size()&&root->context_layout_children&&
        root->context_construction_return,"Selected client complete namespace/all child construction producer missing");
    for(const auto& declaration:registry.declarations){
        const auto found=registry.children.find(declaration.first);
        require(found!=registry.children.end()&&found->second->core_->context_construction_return&&
            found->second->core_->context_layout_baseline&&found->second->core_->context_layout_baseline->parsed_source_serial_,
            "Selected client namespace contains an uncreated/unparsed actual Context");
    }
    for(const auto& node:root->nodes)if(node.alive&&!node.dynamic)
        require(node.layout_baseline&&node.layout_receiver_occurrence,
            "Selected client namespace physical ordinary receiver constructor missing");
    const auto found=registry.children.find(id);
    if(found==registry.children.end())return {}; // earned complete-scope miss only
    const auto result=found->second;require_selected_client_context_registry_owned(operation);return result;
}
