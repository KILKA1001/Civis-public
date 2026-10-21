// AFTER complete RootRead, child+10 completion and direct normal LoadReturn.
// Metadata reuse does not renew the retired cursor/factory Source operation.
void BnwWorldViewControlTree::issue_direct_root_scalar_parent_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::shared_ptr<BnwWorldViewControlTree>& tree) {
    require(tree&&tree.get()==this&&tree->core_==core_,"Direct root parent writer needs actual allocated InGame facade");
    const auto actual=core_;const auto source=actual->source;const auto gui=actual->context_gui_port;
    const auto first=[&]{actual->layout_origin();operation.require_origin_without_callbacks();actual->layout_origin();
        require(actual->context=="InGame"&&actual->source==source&&actual->context_gui_port==gui&&
            actual->context_construction_return&&actual->context_ordinary_state&&actual->context_layout_baseline&&
            &source->current_operation_loan()==&operation,"Direct root parent actual constructor/Source/GUI changed");};
    const auto full=[&]{first();operation.require_current();first();gui->require_current_owned(operation);first();};
    full();
    if(!actual->scalar_parent_attachment){
        require(!actual->context_parent_kind&&!actual->context_load_origin&&!actual->context_cursor_load_prefix,
            "Direct root parent command cannot replace an earned parent/load");
        const auto layout=source->authored_layout("InGame");full();
        require(layout.name=="Context"&&layout.attribute("Name")=="InGame",
            "Direct root parent actual selected creation document differs");
        const auto command=std::make_shared<const Core::ScalarParentAttachment>(tree,nullptr,0,0,operation.operation_serial());
        full();require(!actual->scalar_parent_attachment&&!actual->context_parent_kind&&actual->effect_revision<UINT64_MAX,
            "Direct root parent command replay/journal exhausted");
        // Existing genuine normalized parentless-root writer, before v10.
        // No native process pointer or child relationship is inferred here.
        actual->scalar_parent_attachment=command;actual->context_parent_kind=Core::ContextParentKind::native_null;
        ++actual->effect_revision;
    }
    require_scalar_parent_attachment_owned(actual,operation);full();
}

void BnwWorldViewControlTree::require_direct_context_load_history_owned(
    const std::shared_ptr<Core>& actual,const runtime::BnwWorldViewSourceOperationLoan& operation) {
    require(actual!=nullptr,"Direct registry actual Context missing");
    const bool normal=require_normal_root_or_idle_origin_without_callbacks(actual);
    operation.require_origin_without_callbacks();
    require(&actual->source->current_operation_loan()==&operation,
        "Direct registry current Source differs from actual Context");
    if(!actual->context_direct_load_return){
        // Original parent5761 calls the existing child+10 dispatcher. A child
        // does not enter DirectLoader/DirectExit and must not acquire that token.
        const auto returned=actual->context_child_slot10_return;
        require(returned&&returned->impl_&&returned->impl_->context_history&&
            !actual->context_direct_load_pending_return,
            "Selected registry requires real nondeferred child+10 completion");
        const auto& done=*returned->impl_;const auto& context=*done.context_history;
        const auto parent=done.parent.lock();
        require(done.child.lock()==actual&&done.child_id==0&&done.child_order!=0&&done.source_serial!=0&&
            done.lifetime&&done.lifetime->creator==std::this_thread::get_id()&&!done.lifetime->failed.load()&&
            done.lifetime->destroyed&&done.lifetime->destruction_passed&&parent&&parent!=actual&&
            parent->source==actual->source&&parent->context_gui_port==actual->context_gui_port&&
            actual->context_parent_core.lock()==parent&&!actual->context_parent_control_id&&
            actual->context_parent_kind==Core::ContextParentKind::canonical_context&&parent->context_layout_children,
            "Selected registry child+10 actual parent/caller DTOR/source/GUI changed");
        const auto physical=std::find_if(parent->context_layout_children->begin(),parent->context_layout_children->end(),
            [&](const auto& row){return row.core.lock()==actual;});
        require(physical!=parent->context_layout_children->end()&&physical->parent_control_id==0&&
            physical->attached_order==done.child_order,
            "Selected registry child+10 returned attachment order changed");
        require(context.state&&context.state==actual->context_ordinary_state&&context.state->impl_&&
            context.state->impl_->creator==done.lifetime->creator&&context.state->impl_->alive.load()&&
            !context.state->impl_->failed.load()&&context.state->impl_->core.lock()==actual&&
            context.state->impl_->source.lock()==actual->source&&context.constructor&&
            context.constructor==actual->context_construction_return&&context.load&&context.load->impl_&&
            context.load==actual->context_load_origin&&context.load->impl_->core.lock()==actual&&
            context.load->impl_->source_serial==done.source_serial&&!(context.load->impl_->flags&0x10000000u)&&
            context.load->impl_->manager==actual->context_gui_port&&context.initialized&&context.initialized->impl_&&
            actual->layout_initialization_return&&actual->layout_initialization_return->native_return_==context.initialized&&
            context.preparation&&context.preparation==actual->context_lua_preparation&&
            context.prefix&&context.prefix==actual->context_cursor_load_prefix&&context.prefix->impl_&&
            context.gc&&context.gc==actual->context_initial_lua_gc_return&&context.gc->impl_&&
            context.cursor&&context.cursor==actual->context_cursor_restore_return&&context.cursor->impl_&&
            context.cleanup&&context.cleanup==actual->context_loader_cleanup_return&&
            context.host==&operation.context_lua_host_owned(),
            "Selected registry child+10 typed constructor/5761/Lua/GC/cursor/cleanup/VM outputs changed");
        const auto& initialized=*context.initialized->impl_;const auto origin=initialized.origin;
        require(initialized.core.lock()==actual&&initialized.source_serial==done.source_serial&&
            initialized.lifetime&&initialized.lifetime->creator==done.lifetime->creator&&
            initialized.lifetime->destroyed&&initialized.lifetime->destruction_passed&&!initialized.lifetime->failed.load()&&
            !actual->context_initialization_active&&origin&&origin==actual->layout_context_initialization_origin&&
            !origin->direct_read_&&!origin->direct_caller_&&origin->tree_.lock()==actual&&
            origin->construction_==context.constructor&&origin->parser_serial_!=0&&actual->context_layout_baseline&&
            actual->context_layout_baseline->parsed_source_serial_==origin->parser_serial_&&
            context.load->impl_->parser_serial==origin->parser_serial_,
            "Selected registry child+10 actual legacy5761/parser/destruction lineage changed");
        const auto& gc=*context.gc->impl_;const auto& cursor=*context.cursor->impl_;const auto& prefix=*context.prefix->impl_;
        require(gc.creator==done.lifetime->creator&&gc.core.lock()==actual&&gc.serial==done.source_serial&&
            gc.preparation==context.preparation&&gc.cursor==context.prefix&&cursor.core.lock()==actual&&
            cursor.serial==done.source_serial&&cursor.prefix==context.prefix&&cursor.gc==context.gc&&
            cursor.manager==actual->context_gui_port&&cursor.parser_serial==origin->parser_serial_&&
            prefix.core.lock()==actual&&prefix.source_serial==done.source_serial&&
            prefix.manager==actual->context_gui_port&&prefix.parser_serial==origin->parser_serial_&&
            cursor.restored_cursor==prefix.previous_cursor,
            "Selected registry child+10 genuine GC/cursor restoration history changed");
        const auto parent_seal=parent->layout_initialization_return;
        require(parent_seal&&parent_seal->native_return_&&parent_seal->native_return_->impl_,
            "Selected registry child+10 has no completed actual parent5761");
        const auto& parent_done=*parent_seal->native_return_->impl_;
        require(parent_done.core.lock()==parent&&parent_done.source_serial==done.source_serial&&
            parent_done.lifetime&&parent_done.lifetime->creator==done.lifetime->creator&&
            parent_done.lifetime->destroyed&&parent_done.lifetime->destruction_passed&&!parent_done.lifetime->failed.load(),
            "Selected registry child+10 parent adoption/destruction lineage changed");
        auto root=parent;std::set<const Core*> ancestors;
        while(!root->context_direct_load_return){
            require(ancestors.size()<128&&ancestors.insert(root.get()).second&&
                root->context_parent_kind==Core::ContextParentKind::canonical_context&&!root->context_parent_control_id,
                "Selected registry child+10 has no genuine direct root ancestry");
            (void)require_normal_root_or_idle_origin_without_callbacks(root);
            const auto next=root->context_parent_core.lock();
            require(next&&next->source==actual->source&&next->context_gui_port==actual->context_gui_port,
                "Selected registry child+10 direct root ancestry retired/foreign");root=next;
        }
        require(root->context=="InGame","Selected registry child+10 ancestor is not actual InGame direct root");
        require_direct_context_load_history_owned(root,operation);
        const auto root_return=root->context_direct_load_return;
        const auto& root_done=*root_return->impl_;
        require(root_done.initialized&&root_done.initialized->impl_&&
            root_done.initialized->impl_->source_serial==done.source_serial,
            "Selected registry child+10 did not join the actual root5761 occurrence");
        for(const auto& domain:done.domains){
            const auto member=domain.core.lock();require(member&&member->source==actual->source,
                "Selected registry child+10 completed domain retired/foreign");
            (void)require_normal_root_or_idle_origin_without_callbacks(member);
            const auto adopted=std::find_if(parent_done.domains.begin(),parent_done.domains.end(),
                [&](const auto& row){return row.core.lock()==member;});
            const auto final=std::find_if(root_done.domains.begin(),root_done.domains.end(),
                [&](const auto& row){return row.core.lock()==member;});
            require(domain.before_effects<=domain.effects&&member->topology_revision==domain.topology&&
                !member->initial_lua_mutation_journal&&adopted!=parent_done.domains.end()&&
                adopted->topology==domain.topology&&adopted->effects>=domain.effects&&
                final!=root_done.domains.end()&&final->topology==domain.topology&&final->effects>=adopted->effects,
                "Selected registry child+10 lacks actual parent adoption/root final writer history");
        }
        for(const auto& entry:gc.domains){
            const auto member=entry.core.lock();const auto journal=entry.journal;
            const auto completed=std::find_if(done.domains.begin(),done.domains.end(),
                [&](const auto& row){return row.core.lock()==member;});
            require(member&&journal&&journal->creator==done.lifetime->creator&&journal->sealed&&
                journal->serial==done.source_serial&&completed!=done.domains.end()&&
                journal->topology==completed->topology&&journal->effects<=completed->effects,
                "Selected registry child+10 actual originalLua writer journal was not adopted");
        }
        // The fresh Host checker verifies the retained real coroutine/env and
        // original creation serial; it does not dereference the ended caller.
        context.host->require_context_lua_state_history_owned(operation,context.preparation,actual,context.prefix,done.source_serial);
        operation.require_same_source(*actual->source->prepared_source(actual->context));
        require(require_normal_root_or_idle_origin_without_callbacks(actual)==normal&&
            actual->context_child_slot10_return==returned&&root->context_direct_load_return==root_return&&
            parent->layout_initialization_return==parent_seal&&actual->context_initial_lua_gc_return==context.gc&&
            actual->context_cursor_restore_return==context.cursor&&actual->context_loader_cleanup_return==context.cleanup,
            "Selected registry child+10 actual history/origin replaced during fresh Source/VM validation");
        return;
    }
    const auto returned=actual->context_direct_load_return;
    require(returned&&returned->impl_,"Direct registry whole normal LoadReturn absent");
    const auto& done=*returned->impl_;const auto caller=done.caller;
    require(done.creator==std::this_thread::get_id()&&done.published&&done.core.lock()==actual&&
        caller&&!caller->alive.load()&&!caller->failed.load()&&caller->normal_returned&&caller->destruction_passed&&
        caller->core.lock()==actual&&caller->source==actual->source&&actual->context_ordinary_state&&
        actual->context_ordinary_state->impl_&&actual->context_ordinary_state->impl_->alive.load()&&
        !actual->context_ordinary_state->impl_->failed.load()&&caller->state==actual->context_ordinary_state&&
        caller->constructor==actual->context_ordinary_state->impl_->native_constructor_return&&
        actual->context_gui_port==caller->manager&&actual->context_lua_preparation==caller->prepared&&
        done.host==&operation.context_lua_host_owned()&&actual->layout_initialization_return&&
        actual->layout_initialization_return->native_return_==done.initialized&&done.initialized&&done.initialized->impl_&&
        done.initialized->impl_->lifetime&&done.initialized->impl_->lifetime->destroyed&&
        done.initialized->impl_->lifetime->destruction_passed&&!done.initialized->impl_->lifetime->failed.load()&&
        done.gc&&done.gc==actual->context_initial_lua_gc_return&&done.cursor&&done.cursor==actual->context_cursor_restore_return&&
        done.cleanup&&done.cleanup==actual->context_loader_cleanup_return&&done.cdata&&
        done.cdata==actual->context_ordinary_state->impl_->postread_root_cdata_return,
        "Direct registry historical typed constructor/VM/5761/GC/cursor/cleanup/destruction identity changed");
    // Physical graph topology remains confirmed. Layout effects/sequence from
    // the completed local Load prefix are historical after earned later writers.
    // Generic direct FULL and normal paint keep their separate strict gates.
    for(const auto& domain:done.domains){
        const auto member=domain.core.lock();require(member&&member->source==actual->source,
            "Direct registry historical initialized domain retired/foreign");
        (void)require_normal_root_or_idle_origin_without_callbacks(member);
        require(member->topology_revision==domain.topology&&!member->initial_lua_mutation_journal,
            "Direct registry current topology requires a genuine joined construction writer");
    }
    operation.require_same_source(*actual->source->prepared_source(actual->context));
    require(require_normal_root_or_idle_origin_without_callbacks(actual)==normal&&actual->context_direct_load_return==returned,
        "Direct registry actual origin/normal return replaced during history validation");
}

std::vector<std::shared_ptr<BnwWorldViewControlTree>>
BnwWorldViewControlTree::borrow_direct_root_read_children_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const {
    const auto actual=core_;const bool normal=require_normal_root_or_idle_origin_without_callbacks(actual);
    operation.require_origin_without_callbacks();
    require(&actual->source->current_operation_loan()==&operation&&actual->context_layout_baseline&&
        actual->context_construction_return&&actual->context_ordinary_state,
        "Direct RootRead borrower actual constructor/source absent");
    const auto slot=actual->context_root_read_pending_completion;
    require(slot&&slot->impl_&&slot->impl_->creator==std::this_thread::get_id()&&slot->impl_->alive.load()&&
        slot->impl_->canonical.lock()==slot&&slot->impl_->core.lock()==actual&&slot->impl_->source==actual->source&&
        slot->impl_->state==actual->context_ordinary_state&&!slot->impl_->staged_return&&slot->impl_->completed_return,
        "Direct RootRead borrower requires actual completed canonical typed read");
    const auto done=slot->impl_->completed_return;const auto cursor=done->returned_cursor;const auto frame=done->returned_frame;
    require(cursor&&frame&&!cursor->alive.load()&&!cursor->failed.load()&&!frame->alive.load()&&!frame->failed.load()&&
        done->actual_pending.lock()==slot&&cursor->core.lock()==actual&&frame->core.lock()==actual&&
        cursor->frame.lock()==frame&&cursor->document==frame->expected_xml84&&cursor->ordinal==cursor->child_count&&
        cursor->completed_nodes.size()==cursor->child_count&&actual->context_layout_children&&
        actual->context_ordinary_state->impl_->xml84_current==cursor->document,
        "Direct RootRead completed document/nodes/lexical destruction identity changed");
    using Cleanup=detail::NotificationContextRootReadCompletionSlot::Impl::CompletedReturn::Cleanup;
    static constexpr std::array expected{Cleanup::cursor34_8353f0,Cleanup::string18_retire,Cleanup::lua2c_8f8820,
        Cleanup::cursor10_8353f0,Cleanup::wide280_686f10,Cleanup::wide68_686f10};
    require(done->cleanup_effects==expected,"Direct RootRead six actual cleanup order changed");
    std::vector<std::shared_ptr<BnwWorldViewControlTree>> result;std::size_t context_index=0;
    for(const auto& node:cursor->completed_nodes){
        require(node&&node->actual_cursor.lock()==cursor&&node->document==cursor->document,
            "Direct RootRead node belongs to another actual cursor/document");
        if(node->kind!=ContextRootNodeReturn::Kind::distinct_context)continue;
        const auto child=node->child.lock();const auto attachment=node->attached.lock();
        require(child&&attachment&&attachment->impl_&&attachment->impl_->alive.load()&&
            attachment->impl_->core.lock()==actual&&attachment->impl_->distinct_context_child.lock()==child&&
            attachment->impl_->completed.impl_==slot->impl_&&child->context_root_distinct_attach_return==attachment&&
            context_index<actual->context_layout_children->size(),
            "Direct RootRead genuine distinct child/parent78 attachment retired");
        const auto& physical=(*actual->context_layout_children)[context_index++];
        require(physical.core.lock()==child&&physical.parent_control_id==0&&
            physical.attached_order==attachment->impl_->append_index+1,
            "Direct RootRead current physical graph/order differs from actual completed append");
        std::shared_ptr<ContextRootChildAttempt> selected;
        for(const auto& attempt:actual->context_root_child_attempts){
            if(attempt&&attempt->document==cursor->document&&attempt->declaration==node->declaration&&attempt->child_core==child){
                require(!selected,"Direct RootRead duplicate actual child attempt");selected=attempt;
            }
        }
        require(selected&&selected->child&&selected->child->core_==child&&selected->factory&&selected->wrapper&&
            !selected->pending_wrapper&&selected->controls_v64_return&&!selected->pending_controls_v64_return&&
            selected->attach_progress&&selected->receiver==attachment->impl_->child.lock()&&
            selected->descriptor&&selected->descriptor->context()==child->context&&selected->selected_context==child->context&&
            selected->constructor==child->context_ordinary_state->impl_->native_constructor_return,
            "Direct RootRead actual retained facade/factory/v64/descriptor custody changed");
        const auto parent=child->context_parent_core.lock();
        require(parent==actual&&!child->context_parent_control_id&&child->context_parent_kind==Core::ContextParentKind::canonical_context&&
            child->source==actual->source&&child->context_gui_port==actual->context_gui_port,
            "Direct RootRead actual parent38/source/GUI differs");
        (void)require_normal_root_or_idle_origin_without_callbacks(child);
        operation.require_same_source(*selected->descriptor);result.push_back(selected->child);
    }
    require(context_index==actual->context_layout_children->size(),
        "Direct RootRead current graph contains an unearned Context attachment");
    operation.require_current();
    require(require_normal_root_or_idle_origin_without_callbacks(actual)==normal&&
        actual->context_root_read_pending_completion==slot&&slot->impl_->completed_return==done,
        "Direct RootRead canonical completed history replaced during fresh Source check");
    require(actual->context_layout_children&&actual->context_layout_children->size()==result.size(),
        "Direct RootRead current Context graph changed during fresh Source check");
    for(std::size_t i=0;i<result.size();++i){
        const auto& current=(*actual->context_layout_children)[i];
        const auto child=result[i]->core_;
        require(current.core.lock()==child&&current.parent_control_id==0&&child->context_parent_core.lock()==actual&&
            !child->context_parent_control_id&&child->context_parent_kind==Core::ContextParentKind::canonical_context,
            "Direct RootRead current physical parent/facade changed during Source POST");
    }
    return result;
}

std::vector<std::shared_ptr<BnwWorldViewControlTree>>
BnwWorldViewControlTree::collect_direct_root_read_tree_facades_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const std::shared_ptr<BnwWorldViewControlTree>& root)const {
    require(root&&root.get()==this&&root->core_==core_&&core_->context=="InGame",
        "Direct retained facade traversal needs actual InGame root");
    require_direct_context_load_history_owned(core_,operation);
    const auto collect=[&]{
        std::vector<std::shared_ptr<BnwWorldViewControlTree>> result;std::set<const Core*> unique;
        const auto visit=[&](auto&& self,const std::shared_ptr<BnwWorldViewControlTree>& tree,std::size_t depth)->void{
            require(tree&&depth<128&&result.size()<4096&&unique.insert(tree->core_.get()).second,
                "Direct retained facade duplicate/cycle/depth budget exceeded");
            const auto actual=tree->core_;(void)require_normal_root_or_idle_origin_without_callbacks(actual);
            require(actual->source==core_->source&&actual->context_gui_port==core_->context_gui_port&&
                actual->context_ordinary_state&&actual->context_construction_return,
                "Direct retained physical facade/constructor/Source/GUI differs");
            result.push_back(tree);
            if(actual->context_root_read_pending_completion){
                const auto children=tree->borrow_direct_root_read_children_owned(operation);
                for(const auto& child:children)self(self,child,depth+1);
            }else if(actual->context_child_slot10_return&&actual->context_child_slot10_return->impl_&&
                actual->context_child_slot10_return->impl_->context_history){
                require_direct_context_load_history_owned(actual,operation);
                // This child used the actual +10 path, not child RootRead. Its
                // retained parent-factory v64 return earned the physical zero78.
                const auto parent=actual->context_parent_core.lock();
                require(parent!=nullptr,"Selected child+10 physical parent retired");
                const auto at=std::find_if(parent->context_root_child_attempts.begin(),parent->context_root_child_attempts.end(),
                    [&](const auto& attempt){return attempt&&attempt->child==tree&&attempt->child_core==actual;});
                require(at!=parent->context_root_child_attempts.end()&&(*at)->controls_v64_return&&
                    !(*at)->pending_controls_v64_return&&(*at)->controls_v64_return->impl_,
                    "Selected child+10 has no actual parent-factory v64 return");
                const auto& v64=*(*at)->controls_v64_return->impl_;const auto ended=v64.ended_frame.lock();
                const auto& base=actual->context_layout_baseline->base_;
                require(v64.origin==detail::NotificationLayoutNativeChildControlsReturn::Origin::distinct_context&&
                    v64.creator==std::this_thread::get_id()&&v64.alive.load()&&v64.attempt.lock()==*at&&
                    v64.canonical.lock()==(*at)->controls_v64_return&&v64.factory==(*at)->factory&&
                    v64.selected==(*at)->descriptor&&v64.context==actual->context_ordinary_state&&
                    ended&&!ended->alive.load()&&!ended->failed.load()&&ended->context==v64.context&&
                    v64.receiver==base.receiver_owned&&v64.children78==base.children_78&&v64.traversed_count==0&&
                    base.children_78&&base.children_78->impl_&&base.children_78->impl_->parent.lock()==base.receiver_owned&&
                    base.children_78->impl_->ordered.empty()&&actual->context_root_child_attempts.empty()&&
                    (!actual->context_layout_children||actual->context_layout_children->empty()),
                    "Selected child+10 actual v64 zero78/DTOR/physical graph changed");
            }else{
                // A deferred constructor's physical zero78 is not a completed
                // XML traversal or a load-ready grant. Never discover its XML.
                const auto& base=actual->context_layout_baseline->base_;
                require((base.flags_40&0x10000000u)&&base.children_78&&base.children_78->impl_&&
                    base.children_78->impl_->parent.lock()==base.receiver_owned&&base.children_78->impl_->ordered.empty()&&
                    actual->context_root_child_attempts.empty()&&!actual->context_layout_children,
                    "Direct retained child has neither actual completed Read nor genuine deferred empty78");
            }
        };
        visit(visit,root,0);return result;
    };
    const auto before=collect();operation.require_current();require_direct_context_load_history_owned(core_,operation);
    const auto after=collect();require(after==before,"Direct retained facades/order changed across current Source");return after;
}

std::array<std::shared_ptr<BnwWorldViewControlTree>,5>
BnwWorldViewControlTree::borrow_direct_loaded_native_slots_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,const std::shared_ptr<BnwWorldViewControlTree>& root)const {
    constexpr std::array<std::string_view,5> names{"InGame","WorldView","UnitFlagManager","ActionInfoPanel","NotificationPanel"};
    const auto trees=collect_direct_root_read_tree_facades_owned(operation,root);
    std::array<std::shared_ptr<BnwWorldViewControlTree>,5> result{};
    for(const auto& tree:trees)for(std::size_t i=0;i<names.size();++i)if(tree->core_->context==names[i]){
        require(!result[i],"Direct actual native slot name is ambiguous");result[i]=tree;
    }
    for(const auto& tree:result)require(tree!=nullptr,"Direct required native slot was not physically constructed by RootRead");
    require(result[0]==root,"Direct required slot0 is another InGame facade");
    // Strict first activation: factory/wrapper/attachment are not whole load.
    if(!core_->selected_client_registry)(void)root->require_context_direct_load_return_owned(operation);
    else require_direct_selected_client_registry_owned(operation);
    for(std::size_t i=1;i<result.size();++i)require_direct_context_load_history_owned(result[i]->core_,operation);
    operation.require_current();const auto after=collect_direct_root_read_tree_facades_owned(operation,root);
    require(after==trees,"Direct selected native slots changed during strict LoadReady validation");return result;
}

void BnwWorldViewControlTree::adopt_direct_selected_client_registry_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::array<std::shared_ptr<BnwWorldViewControlTree>,5>& existing) {
    require(existing[0]&&existing[0].get()==this,"Direct registry canonical root slot absent");
    if(core_->selected_client_registry){require_direct_selected_client_registry_owned(operation);return;}
    const auto slots=borrow_direct_loaded_native_slots_owned(operation,existing[0]);
    require(slots==existing,"Direct registry provided slots differ from actual RootRead facades");
    const auto children=borrow_direct_root_read_children_owned(operation);
    auto registry=std::make_unique<Core::SelectedClientContextRegistry>();
    registry->serial=core_->context_direct_load_return->impl_->serial;
    registry->history.reserve(children.size());
    for(std::size_t i=0;i<children.size();++i){
        const auto child=children[i];const auto& physical=(*core_->context_layout_children)[i];
        const auto at=std::find_if(core_->context_root_child_attempts.begin(),core_->context_root_child_attempts.end(),
            [&](const auto& attempt){return attempt&&attempt->child==child&&attempt->child_core==child->core_;});
        require(at!=core_->context_root_child_attempts.end(),"Direct registry actual retained child attempt absent");
        const auto& declaration=*(*at)->declaration;const auto name=child->core_->context;
        require(registry->declarations.emplace(name,Core::SelectedClientContextRegistry::Declaration{
            declaration.attribute("FileName"),physical.parent_control_id,physical.attached_order,declaration}).second&&
            registry->children.emplace(name,child).second,"Direct registry duplicate actual child declaration");
        registry->history.push_back({name,physical.parent_control_id,physical.attached_order,registry->serial});
    }
    require_direct_context_load_history_owned(core_,operation);operation.require_current();
    require(!core_->selected_client_registry&&borrow_direct_root_read_children_owned(operation)==children,
        "Direct registry actual children changed before metadata index publication");
    // Metadata indices only. No topology/effects/attachments/parser mutation.
    core_->selected_client_registry=std::move(registry);require_direct_selected_client_registry_owned(operation);
}

void BnwWorldViewControlTree::require_direct_selected_client_registry_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation)const {
    require_direct_context_load_history_owned(core_,operation);
    require(core_->context=="InGame"&&core_->selected_client_registry,
        "Direct registry actual canonical index absent");
    const auto children=borrow_direct_root_read_children_owned(operation);const auto& registry=*core_->selected_client_registry;
    require(registry.serial==core_->context_direct_load_return->impl_->serial&&registry.history.size()==children.size()&&
        registry.children.size()==children.size()&&registry.declarations.size()==children.size(),
        "Direct registry typed completed load/RootRead identity differs");
    for(std::size_t i=0;i<children.size();++i){
        const auto& child=children[i];const auto& physical=(*core_->context_layout_children)[i];
        const auto& row=registry.history[i];const auto member=registry.children.find(child->core_->context);
        const auto declared=registry.declarations.find(child->core_->context);
        require(member!=registry.children.end()&&member->second==child&&declared!=registry.declarations.end()&&
            row.id==child->core_->context&&row.parent==physical.parent_control_id&&row.order==physical.attached_order&&
            row.serial==registry.serial&&declared->second.parent==row.parent&&declared->second.order==row.order,
            "Direct registry actual facade/append order/current declaration index changed");
        const auto actual_attempt=std::find_if(core_->context_root_child_attempts.begin(),core_->context_root_child_attempts.end(),
            [&](const auto& attempt){return attempt&&attempt->child==child&&attempt->child_core==child->core_;});
        require(actual_attempt!=core_->context_root_child_attempts.end()&&(*actual_attempt)->declaration&&
            declared->second.stem==(*actual_attempt)->declaration->attribute("FileName")&&
            Core::same_layout_xml(declared->second.layer,*(*actual_attempt)->declaration),
            "Direct registry retained declaration layer/original FileName index changed");
    }
    operation.require_current();require_direct_context_load_history_owned(core_,operation);
}

void BnwWorldViewControlTree::require_direct_registry_baselines_owned(
    const runtime::BnwWorldViewSourceOperationLoan& operation,
    const std::map<std::string,content::XmlNode,std::less<>>& styles)const {
    const auto supplied_styles=styles;
    require_direct_selected_client_registry_owned(operation);
    const auto selected_styles=core_->source->selected_client_styles_owned(operation);
    require(Core::same_layout_styles(supplied_styles,selected_styles),"Direct registry supplied styles are not actual selected Source styles");
    // Native constructor/parser/5761/GC/normal-return history is authoritative.
    // Never call the legacy parser or invent parsed_source_serial_ for direct.
    require_direct_context_load_history_owned(core_,operation);require_direct_selected_client_registry_owned(operation);
}
