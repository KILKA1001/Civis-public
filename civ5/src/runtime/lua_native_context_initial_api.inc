namespace civ5::runtime {
void LuaScriptHost::invoke_context_initial_lua_owned(const BnwWorldViewSourceOperationLoan& loan,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>& prepared,
    const std::weak_ptr<const void>& core,
    const std::shared_ptr<const ui::detail::NotificationContextCursorLoadPrefixReturn>& cursor)const {
    const auto p=impl_;
    try{
        require_context_lua_state_owned(loan,prepared,core,cursor);
        Impl::WorldViewNativeHostScope native(*p,*this,loan);p->worldview_require_callback_origin();
        const auto& saved=*prepared->impl_;native.child=saved.child;native.child_source=saved.source;
        auto& child=p->child_contexts[saved.child];
        if(child.loaded||child.loading||child.retired||p->executed_contexts.contains(child.name))
            throw ScriptError("Context initialLua original source occurrence replay");
        std::shared_ptr<ScriptUiControlGraph> graph;
        for(const auto& entry:p->control_graphs)if(entry.graph->context()==child.name){
            if(graph)throw ScriptError("Context initialLua registered graph ambiguous");graph=entry.graph;}
        if(!graph)throw ScriptError("Context initialLua needs genuine SAME perContext registered native effects backend");
        BnwWorldViewScriptUiAdapter::require_initial_lua_core_owned(graph,core,saved.source,loan,*this);
        p->worldview_require_callback_origin();p->child_require_current(saved.child);p->worldview_require_callback_origin();
        // Selected Civis loader normalization creates NEW owning VFS/path
        // temporaries BEFORE the real compiled-source invocation. None is an
        // alias of the saved asset or an Original stack/control-block receipt.
        if(child.loader_temporaries)throw ScriptError("Selected Civis Context loader temporary creation replay");
        const auto operands=loan.context_loader_resource_operand_owned(saved.source);p->worldview_require_callback_origin();
        const auto serial=loan.operation_serial();
        if(operands.serial_!=serial||!operands.generation_||!operands.bundle_||
           operands.source_.get()!=saved.source.get()||operands.context_!=child.name)
            throw ScriptError("Selected Civis Context loader canonical operand mismatch");
        child.loader_temporaries=std::make_unique<Impl::ContextLoaderTemporaries>(core,prepared,saved.source,
            operands.bundle_,serial,operands.generation_,p->failure_epoch,p->failed.writes_,p->execution_uncertainty,
            child.name,operands.lua_path_,operands.xml_path_,operands.stem_);
        auto& temp=*child.loader_temporaries;
        const auto current=[&]{p->worldview_require_callback_origin();loan.require_operation(serial);
            p->worldview_require_callback_origin();operands.bundle_->require_current();p->worldview_require_callback_origin();};
        const auto resolve=[&](Impl::ContextLoaderPathTemporary::Role role,const std::string& path,const std::string& expected){
            current();assets::AssetKey key(path);
            if(!key.safe_relative_path())throw ScriptError("Selected Civis loader path outside canonical VFS");
            auto mounted=operands.bundle_->resources().resolve(key);current();
            auto* actual=std::get_if<assets::AssetHandle>(&mounted.asset);
            if(!actual)throw ScriptError("Selected Civis loader VFS path missing/ambiguous");
            auto handle=std::make_unique<assets::AssetHandle>(std::move(*actual));
            auto bytes=std::make_unique<assets::Bytes>(handle->read_bytes(Impl::child_source_limit));current();
            const auto identity=identify_bytes(*bytes);const auto bank=operands.bundle_->loaded(path);current();
            if(!bank.bytes||identity.sha256!=expected||identity.bytes!=bank.payload_identity.bytes||
               identity.sha256!=bank.payload_identity.sha256||
               identify_bytes(*bank.bytes).sha256!=identity.sha256||
               handle->candidate().normalized_name!=bank.candidate.normalized_name||
               handle->candidate().payload_size!=bank.candidate.payload_size||
               handle->candidate().provenance.kind!=bank.candidate.provenance.kind||
               handle->candidate().provenance.source_id!=bank.candidate.provenance.source_id||
               handle->candidate().provenance.source_relative_path!=bank.candidate.provenance.source_relative_path||
               handle->candidate().provenance.archive_relative_path!=bank.candidate.provenance.archive_relative_path)
                throw ScriptError("Selected Civis loader fresh resource differs from canonical Bank winner");
            return std::make_unique<Impl::ContextLoaderPathTemporary>(role,path,identity,std::move(handle),std::move(bytes));
        };
        temp.paths[0]=resolve(Impl::ContextLoaderPathTemporary::Role::script_compile_path,operands.lua_path_,operands.lua_sha_);
        temp.paths[1]=resolve(Impl::ContextLoaderPathTemporary::Role::context_layout_source_path,operands.xml_path_,operands.xml_sha_);
        // The directory resolver owns its own fresh script-resource backing;
        // its path is an actual derived canonical directory, not a NULL pair.
        temp.paths[2]=resolve(Impl::ContextLoaderPathTemporary::Role::script_source_directory_path,operands.lua_path_,operands.lua_sha_);
        temp.filename=std::make_unique<std::string>(temp.paths[0]->handle->candidate().original_name);
        current();if(temp.filename->empty()||temp.filename->find('\0')!=temp.filename->npos||
            temp.paths[2]->source_directory.empty()||
            assets::normalize_asset_name(temp.paths[2]->source_directory)!=
              assets::normalize_asset_name(assets::path_utf8(std::filesystem::path(operands.stem_).parent_path())))
            throw ScriptError("Selected Civis loader actual filename invalid");
        child.chunk="@"+*temp.filename;child.loading=true;
        child.bytes=*temp.paths[0]->buffer;current();child.source->require_captured_bytes(child.bytes);current();
        Impl::Job job{p.get(),Impl::Operation::native_worldview_load,child.name,{},{},{},{},{}};
        job.worldview_child=saved.child;
        {
            Impl::EndTurnChildLoaderScope loader(*p,p->state);loader.child=saved.child;
            p->call(job);p->worldview_require_callback_origin();
            if(!child.loaded||child.loading||!p->executed_contexts.contains(child.name)||!temp.refs_returned)
                throw ScriptError("Selected Civis loader genuine compiled Lua/temporary refs did not return");
            loader.returned=true;
        }
        current(); // all temporary owners remain alive through GC and cursor
        p->worldview_require_callback_origin();loan.require_same_source(*saved.source);p->worldview_require_callback_origin();
        BnwWorldViewScriptUiAdapter::require_initial_lua_core_owned(graph,core,saved.source,loan,*this);
        p->worldview_require_callback_origin();
        Impl::Job register_gc{p.get(),Impl::Operation::native_context_register_gc,child.name,{},{},{},{},{}};
        register_gc.worldview_child=saved.child;p->call(register_gc);p->worldview_require_callback_origin();
        Impl::Job query{p.get(),Impl::Operation::native_context_initial_state_query,child.name,{},{},{},{},{}};
        query.worldview_child=saved.child;p->call(query);p->worldview_require_callback_origin();
        if(query.context_state_thread!=saved.actual_thread||query.context_state_environment!=saved.actual_environment)
            throw ScriptError("Context initialLua reused another coroutine/env");
        native.returned=true;
    }catch(...){p->worldview_retire_callback_registrations();p->worldview_retire_context_callback_registrations();
        p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
void LuaScriptHost::require_context_initial_lua_state_owned(const BnwWorldViewSourceOperationLoan& loan,
    const std::shared_ptr<const ui::detail::NotificationContextLuaStatePreparation>& prepared,
    const std::weak_ptr<const void>& core,
    const std::shared_ptr<const ui::detail::NotificationContextCursorLoadPrefixReturn>& cursor)const {
    const auto p=impl_;
    try{
        require_context_lua_state_owned(loan,prepared,core,cursor);
        Impl::WorldViewNativeHostScope native(*p,*this,loan);p->worldview_require_callback_origin();
        const auto& saved=*prepared->impl_;native.child=saved.child;native.child_source=saved.source;
        Impl::Job query{p.get(),Impl::Operation::native_context_initial_state_query,p->child_contexts[saved.child].name,{},{},{},{},{}};
        query.worldview_child=saved.child;p->call(query);p->worldview_require_callback_origin();
        if(query.context_state_thread!=saved.actual_thread||query.context_state_environment!=saved.actual_environment)
            throw ScriptError("Context initialLua completed manager coroutine/env rebound");
        native.returned=true;
    }catch(...){p->record_failure();if(p->end_turn_lifetime)p->end_turn_lifetime->failed=true;throw;}
}
}
