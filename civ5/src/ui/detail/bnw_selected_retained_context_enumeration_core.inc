// Readonly actual physical resource cohort. Never constructs or issues a
// lender, full recursive Context namespace, child-vector or script readiness.
// The immutable Source4 API is reused with this explicit minimum scope.
std::vector<std::shared_ptr<BnwWorldViewControlTree>>
BnwWorldViewControlTree::collect_selected_client_retained_context_trees_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<BnwWorldViewControlTree>& actual_root)const{
    const auto root=core_;
    if(root->context_direct_load_return){
        require_direct_selected_client_registry_owned(operation);
        return collect_direct_root_read_tree_facades_owned(operation,actual_root);
    }
    std::vector<std::shared_ptr<BnwWorldViewControlTree>> result;
    std::vector<const Core*> path;std::set<const Core*> unique;
    const auto first=[&](const std::shared_ptr<Core>& actual){
        root->layout_origin();actual->layout_origin();operation.require_origin_without_callbacks();
        root->layout_origin();actual->layout_origin();
        require(&actual->source->current_operation_loan()==&operation&&
            actual->source.get()==root->source.get()&&!actual->source.owner_before(root->source)&&
            !root->source.owner_before(actual->source)&&actual->context_construction_return&&
            actual->context_layout_baseline&&actual->context_layout_baseline->parsed_source_serial_&&
            actual->context_gui_port&&root->context_gui_port&&
            actual->context_gui_port.get()==root->context_gui_port.get()&&
            !actual->context_gui_port.owner_before(root->context_gui_port)&&
            !root->context_gui_port.owner_before(actual->context_gui_port),
            "Retained Context enumeration canonical constructor/parser/GUI/Source differs");
    };
    try{
        first(root);operation.require_render_operation_owned();first(root);
        require(actual_root&&actual_root.get()==this&&actual_root->core_.get()==root.get()&&
            !actual_root->core_.owner_before(root)&&!root.owner_before(actual_root->core_)&&
            root->context=="InGame"&&root->selected_client_registry,
            "Retained Context enumeration requires actual published client Root");
        require_selected_client_context_registry_owned(operation);first(root);
        const auto visit=[&](auto&& self,const std::shared_ptr<BnwWorldViewControlTree>& tree)->void{
            require(static_cast<bool>(tree),"Retained Context enumeration actual facade missing");
            const auto actual=tree->core_;first(actual);
            require(path.size()<128&&result.size()<4096&&unique.insert(actual.get()).second&&
                std::find(path.begin(),path.end(),actual.get())==path.end(),
                "Retained Context enumeration duplicate/cycle/actual budget exceeded");
            require(actual->context_layout_children.has_value(),
                "Retained Context enumeration needs actual completed child construction list");
            path.push_back(actual.get());struct Pop{std::vector<const Core*>&p;~Pop(){p.pop_back();}}pop{path};
            result.push_back(tree);std::uint64_t previous_order=0;
            const auto& children=*actual->context_layout_children;
            if(actual.get()==root.get())require(children.size()==root->selected_client_registry->children.size()&&
                children.size()==root->selected_client_registry->declarations.size(),
                "Retained Root Context enumeration incomplete actual declaration/attachment list");
            else require(children.size()==actual->constructed_nested_contexts.size(),
                "Retained nested Context enumeration incomplete actual owning attachment list");
            for(const auto& attachment:children){
                first(actual);const auto c=attachment.core.lock();
                require(static_cast<bool>(c),"Retained Context attachment actual child retired");first(c);
                require(attachment.attached_order>previous_order,
                    "Retained Context actual XML attachment order changed");previous_order=attachment.attached_order;
                std::shared_ptr<BnwWorldViewControlTree> child;
                if(actual.get()==root.get()){
                    const auto owned=root->selected_client_registry->children.find(c->context);
                    require(owned!=root->selected_client_registry->children.end(),
                        "Retained Root child lacks actual constructor ownership");child=owned->second;
                }else{
                    const auto owned=actual->constructed_nested_contexts.find(c->context);
                    require(owned!=actual->constructed_nested_contexts.end(),
                        "Retained nested child lacks actual constructor ownership");child=owned->second;
                }
                const auto parent=c->context_parent_core.lock();
                require(child&&child->core_.get()==c.get()&&!child->core_.owner_before(c)&&!c.owner_before(child->core_)&&
                    parent&&parent.get()==actual.get()&&!parent.owner_before(actual)&&!actual.owner_before(parent)&&
                    c->context_parent_control_id==attachment.parent_control_id&&c->context_parent_kind==
                        (attachment.parent_control_id?Core::ContextParentKind::canonical_control:Core::ContextParentKind::canonical_context),
                    "Retained Context actual child allocation/parent/control changed");
                if(attachment.parent_control_id){
                    const auto& node=Core::node(actual->nodes,attachment.parent_control_id);
                    require(node.alive&&!node.dynamic&&node.layout_baseline&&node.layout_receiver_occurrence,
                        "Retained Context actual ordinary parent constructor retired");
                }
                if(actual.get()!=root.get()||c->context=="WorldView"){
                    self(self,child);first(actual);first(c);
                }else{
                    // Minimal resource cohort: every physically constructed
                    // Root child plus the complete physically owned WorldView
                    // subtree. Discovered XML beneath other Root children is
                    // NOT an object, complete-vector or script-ready grant.
                    require(c->constructed_nested_contexts.empty(),
                        "Resource cohort other Root subtree has new actual objects requiring its genuine lender join");
                    require(result.size()<4096&&unique.insert(c.get()).second,
                        "Resource cohort actual direct child duplicate/budget exceeded");
                    result.push_back(child);first(actual);first(c);
                }
            }
            first(actual);
        };
        visit(visit,actual_root);first(root);
        const auto before=result;
        // Full SOURCE POST is followed by another actual attachment validation.
        // This result owns real facades temporarily, never a scope/ready grant.
        operation.require_current();first(root);root->source->require_sources_current();first(root);
        result.clear();unique.clear();require(path.empty(),"Retained Context enumeration traversal scope leaked");
        visit(visit,actual_root);first(root);
        require(result.size()==before.size(),"Retained Context actual construction list changed across SOURCE POST");
        for(std::size_t i=0;i<result.size();++i)
            require(result[i].get()==before[i].get()&&!result[i].owner_before(before[i])&&!before[i].owner_before(result[i]),
                "Retained Context actual allocation/order changed across SOURCE POST");
        require_selected_client_context_registry_owned(operation);first(root);
        return result;
    }catch(...){root->failed.store(true);throw;}
}
