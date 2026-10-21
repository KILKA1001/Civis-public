//Inside ui AFTER complete Child/Context returns; real owned copies, no XML geometry issuer.
void BnwWorldViewControlTree::adopt_stack_child_results_owned(ContextInitializationFrame& frame,const LayoutChildSlot10Return& returned){
    frame.pure();require(returned.completion_&&returned.completion_->impl_,"Stack adoption needs genuine Child+10 receipt");
    const auto& done=*returned.completion_->impl_;
    require(done.lifetime&&done.lifetime->destroyed&&done.lifetime->destruction_passed&&!done.lifetime->failed.load()&&
        done.source_serial==frame.serial,"Stack child result copied before genuineDTOR/currentSource");
    Core::merge_stack_results_owned(frame.stack_results,done.stack_results);frame.pure();
}
void BnwWorldViewControlTree::finalize_stack_results_owned(ContextInitializationFrame& frame){
    frame.full();
    for(auto& p:frame.stack_results){const auto c=p.core.lock();require(bool(c),"Final Stack result owner retired");
        (void)frame.domain_of(c);Core::require_stack_pending_owned(p);
        if(c==frame.selected){const auto member=frame.completed_members.find(p.id);
            require(member!=frame.completed_members.end()&&member->second.parser_serial==p.parser_serial&&
                member->second.initialized_sequence<=p.baseline_sequence,"Stack final initializer member absent");p.member_finished=true;}
        require(p.member_finished&&p.anchor_returned&&(p.size_returned||p.empty_initializer),
            "Stack final result lacks genuine member/anchor/size-or-earned-empty initializer");
        require(!p.empty_initializer||p.children.empty(),"Stack empty branch cannot grant nonempty result");
        for(const auto& child:p.children)require(child.derived_returned,"Stack final child cursor supplier absent");
        frame.pure();
    }
    frame.full();
}
void BnwWorldViewControlTree::publish_stack_results_owned(const std::shared_ptr<Core>& owner,
    const std::shared_ptr<const detail::NotificationLayoutContextInitializationReturn>& returned){
    require_context_initialization_return_owned(owner,returned);owner->layout_origin();
    require(owner->layout_initialization_return&&owner->layout_initialization_return->native_return_==returned,
        "Stack publication needs actual final published Context seal");
    for(const auto& source:returned->impl_->stack_results){Core::require_stack_pending_owned(source);
        const auto c=source.core.lock();auto p=source;p.cohort_owner=owner;p.cohort=returned;
        require(p.member_finished&&p.anchor_returned&&(p.size_returned||p.empty_initializer),"Stack publication incomplete actual body");
        c->nodes.at(p.id-1).stack_initializer_result=std::make_shared<const Core::StackPending>(std::move(p));
    }
    require_context_initialization_return_owned(owner,returned);
}
BnwWorldViewControlTree::StackLayoutOwned BnwWorldViewControlTree::stack_layout_owned(
    const Control& control,const runtime::BnwWorldViewSourceOperationLoan& operation)const{
    const auto saved=core_;saved->layout_origin();operation.require_origin_without_callbacks();saved->layout_origin();
    const auto id=saved->checked(control);
    const auto pure=[&]{saved->layout_origin();operation.require_origin_without_callbacks();saved->layout_origin();
        require(&saved->source->current_operation_loan()==&operation,"Stack getter SAME Source changed");saved->require_layout_resources_owned();};
    try{
        pure();const auto value=Core::node(saved->nodes,id).stack_initializer_result;
        require(bool(value),"Stack actual completed initializer result unissued");
        const auto owner=value->cohort_owner.lock();const auto cohort=value->cohort.lock();
        require(owner&&cohort&&cohort->impl_&&cohort->impl_->source_serial==operation.operation_serial()&&
            owner->source==saved->source&&!owner->source.owner_before(saved->source)&&!saved->source.owner_before(owner->source),
            "Stack final initializer cohort/source retired or foreign");
        require_context_initialization_return_owned(owner,cohort);pure();Core::require_stack_pending_owned(*value);
        require(value->core.lock()==saved&&value->id==id&&value->member_finished&&value->anchor_returned&&
            (value->size_returned||value->empty_initializer),"Stack canonical result member changed");
        StackLayoutOwned result{value->size,value->extent,value->offset,value->anchor,value->bounds,{},value->size_returned,value->empty_initializer};
        for(const auto& child:value->children){const auto c=child.core.lock();require(bool(c),"Stack getter actual child retired");
            c->layout_origin();c->require_layout_resources_owned();
            const auto tree=c->layout_resource_tree.lock();
            require(tree&&tree->core_==c&&tree.get()==c->layout_resource_actual_tree&&
                c->layout_resource_operation==&operation&&c->layout_resource_serial==operation.operation_serial(),
                "Stack getter requires real SAME child Tree resource lender");
            std::optional<Control> member;
            if(child.id)member.emplace(Control(c,child.id)); //genuine checked allocation, not metadata-sourced ID.
            require(child.derived_returned,"Stack child actual derived cursor missing");
            result.children.push_back({tree,child.id,std::move(member),child.derived_offset,child.native_offset,child.native_size});
            pure();
        }
        require_context_initialization_return_owned(owner,cohort);pure();Core::require_stack_pending_owned(*value);
        require(Core::node(saved->nodes,id).stack_initializer_result==value,"Stack result canonical carrier replaced");return result;
    }catch(...){saved->failed.store(true);throw;}
}
//Supported consumer entry: canonical retained selected root, not a rendererready boolean.
void BnwWorldViewControlTree::ensure_selected_root_context_initialized_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation){
    const auto saved=core_;saved->layout_origin();operation.require_origin_without_callbacks();saved->layout_origin();
    const auto pure=[&]{saved->layout_origin();operation.require_origin_without_callbacks();saved->layout_origin();
        require(&saved->source->current_operation_loan()==&operation,"Root initializer entry SAME Source changed");
        saved->require_layout_resources_owned();};
    try{
        pure();const auto root=saved->source->selected_client_context_slot1_owned(operation);pure();
        require(root&&root.get()==this&&root->core_==saved&&saved->context=="InGame"&&
            saved->context_parent_kind&&*saved->context_parent_kind==Core::ContextParentKind::native_null&&
            saved->context_construction_return&&saved->context_gui_port&&saved->context_layout_baseline,
            "Root initializer entry requires actual canonical selected root/constructor/parent");
        root->require_selected_client_context_registry_owned(operation);pure();
        if(saved->layout_context_initialization_origin&&saved->layout_context_initialization_origin->direct_read_){
            // Direct loading has already executed its initializer, Lua and
            // cleanup. Reuse only its real completed whole-load return; never
            // run the cold root parser or initialize the physical objects twice.
            const auto completed=require_context_direct_load_return_owned(operation);pure();
            require(completed&&saved->context_direct_load_return==completed,
                "Root direct whole-load completion changed across resource validation");
            require_context_direct_load_return_owned(operation);pure();return;
        }
        require(saved->context_layout_baseline->parsed_source_serial_.has_value(),
            "Root legacy initializer entry requires its genuine parser");
        if(!saved->layout_initialization_return){
            if(!saved->context_load_origin){prepare_context_slot10_load_origin_owned(operation);pure();}
            //Pre-initialization only: exact physicalCursor/LuaPrep/dimension afterstate.
            const auto load=require_context_slot10_load_origin_owned(saved,operation);pure();
            require(load&&saved->layout_context_initialization_origin,
                "Selected root actual deferred/incomplete preload cannot grant5761 initialization");
            initialize_context_5761_owned(operation);pure();
        }
        const auto seal=saved->layout_initialization_return;
        require(seal&&seal->native_return_&&seal->native_return_->impl_&&saved->context_load_origin&&
            saved->context_cursor_load_prefix&&saved->context_lua_preparation,
            "Root completed initializer requires actual publishedload/cursor/Lua/seal lineage");
        const auto completed=seal->native_return_;
        require(completed->impl_->source_serial==operation.operation_serial(),
            "Root initializer reuse across a new Source operation requires genuine reuse lender");
        //Do NOT compare the historical load-afterepoch to post5761 fields or restamp it.
        //The actual immutable finalcohort validates all final adopted owned stores.
        require_context_initialization_return_owned(saved,completed);pure();
        require(saved->layout_initialization_return==seal&&saved->layout_initialization_return->native_return_==completed,
            "Root initializer completed occurrence replaced during reuse validation");
    }catch(...){saved->failed.store(true);throw;}
}
