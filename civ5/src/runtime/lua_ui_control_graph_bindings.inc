// Included inside LuaScriptHost::Impl. Persistent scratch and strong graph
// parents live in Impl across Lua longjmp; control userdata contain POD only.
struct GraphControl {std::size_t slot;ScriptUiControlGraph::Id id;};
struct GraphSlot {std::shared_ptr<ScriptUiControlGraph> graph;bool bound=false,touched=false;int environment=LUA_NOREF;int screen_manager=LUA_NOREF,screen_method=LUA_NOREF;};
struct GraphInstance {std::size_t slot;int table;ScriptUiControlGraph::Id root;};
std::vector<GraphSlot> control_graphs;
std::vector<GraphInstance> graph_instances;
std::vector<ScriptUiControlGraph::Named> graph_scratch;
std::set<std::string> executed_contexts;
static constexpr std::size_t graph_no_owner=std::numeric_limits<std::size_t>::max();
struct GraphCompiledSource {const Proto *root;int retained;std::string context;};
std::vector<GraphCompiledSource> graph_compiled_sources;
std::size_t graph_execution_scope=graph_no_owner;
static bool graph_contains_proto(const Proto *root,const Proto *candidate) {
    if(root==candidate)return true;
    for(int i=0;i<root->sizep;++i)if(graph_contains_proto(root->p[i],candidate))return true;
    return false;
}
std::size_t graph_compiled_owner(lua_State *L,int function)const {
    if(!lua_isfunction(L,function)||lua_iscfunction(L,function))return graph_no_owner;
    // The bundled Lua51 lapi.c::lua_topointer returns clvalue for functions.
    // Proto identity cannot be changed by setfenv, chunk names or Lua DTOs.
    const auto *closure=static_cast<const Closure*>(lua_topointer(L,function));
    for(std::size_t i=0;i<graph_compiled_sources.size();++i)
        if(graph_contains_proto(graph_compiled_sources[i].root,closure->l.p))return i;
    return graph_no_owner;
}
std::size_t graph_retain_compiled(lua_State *L,std::string_view context) {
    if(graph_compiled_sources.size()>=4096)return luaL_error(L,"Authored compiled source budget exhausted");
    const auto *closure=static_cast<const Closure*>(lua_topointer(L,-1));
    lua_pushvalue(L,-1);const int retained=luaL_ref(L,LUA_REGISTRYINDEX);
    // Registry roots retain the complete Proto tree, preventing GC pointer reuse.
    graph_compiled_sources.push_back({closure->l.p,retained,std::string(context)});
    return graph_compiled_sources.size()-1;
}
static void graph_require_epoch(lua_State *L,std::uint64_t epoch) {
    auto &self=owner(L);
    if(self.failure_epoch!=epoch)throw ScriptError("Authored graph validator caught a host invocation failure");
}

static std::size_t graph_slot(lua_State *L) {
    const auto slot=static_cast<std::size_t>(lua_tointeger(L,lua_upvalueindex(1)));
    if(slot>=owner(L).control_graphs.size())luaL_error(L,"Authored graph slot retired");return slot;
}
static void graph_require_context(lua_State *L,std::size_t slot) {
    lua_Debug caller{};
    const int level=owner(L).worldview_control_entry==L?2:1;
    if(!lua_getstack(L,level,&caller)||!lua_getinfo(L,"f",&caller))luaL_error(L,"Control call has no selected Lua caller");
    auto &self=owner(L);const auto caller_owner=self.graph_compiled_owner(L,-1);lua_pop(L,1);
    if(caller_owner==graph_no_owner||self.graph_execution_scope==graph_no_owner)
        luaL_error(L,"Control call has no authenticated compiled source/listener scope");
    const auto &context=self.control_graphs[slot].graph->context();
    if(self.graph_compiled_sources[caller_owner].context!=context||
       self.graph_compiled_sources[self.graph_execution_scope].context!=context)
        luaL_error(L,"Control call comes from a foreign compiled context owner");
}
static void graph_push_control(lua_State *L,std::size_t slot,ScriptUiControlGraph::Id id) {
    auto *value=static_cast<GraphControl*>(lua_newuserdata(L,sizeof(GraphControl)));*value={slot,id};
    lua_newtable(L);lua_newtable(L);
    constexpr const char *methods[]={"SetHide","ChangeParent","DestroyChild","IsHidden","SetAlpha","SetDisabled","SetText","SetToolTipString","GetButton","RegisterCallback","SetWorldPosition","SetWorldPositionVal","SetOffset","SetOffsetVal","SetOffsetX","SetColor","SetTexture","SetTextureOffset","SetPercent","SetToBeginning","Play","SetFGColor","CalculateSize","ReprocessAnchoring","GetOffsetVal","GetSizeY","SetSizeY","CalculateInternalSize","GetRatio","SetConsumeMouseOver","SetVoid1","SetVoid2"};
    const int count=owner(L).control_graphs[slot].graph->backend_?32:4;
    for(int operation=0;operation<count;++operation) {
        // Keep layout22..28 and mouse29 unchanged. The appended public
        // Void methods30/31 use the existing physical control-body22/23.
        const int native_operation=operation>=30?operation-8:operation;
        lua_pushinteger(L,static_cast<lua_Integer>(slot));lua_pushinteger(L,native_operation);
        lua_pushcclosure(L,owner(L).control_graphs[slot].graph->backend_?(operation>=30?worldview_control_call:(operation==29?worldview_mouse_consume_control_call:(operation>=22?worldview_layout_control_call:worldview_control_call))):graph_control_call,2);lua_setfield(L,-2,methods[operation]);
    }
    lua_setfield(L,-2,"__index");
    lua_pushlightuserdata(L,&owner(L).control_graphs);lua_setfield(L,-2,"__civis_control_tag");
    // The per-control method table is private to the metatable. The common
    // registry tag is used only to authenticate the userdata representation.
    lua_pushliteral(L,"AuthoredControl");lua_setfield(L,-2,"__metatable");
    lua_setmetatable(L,-2);
}
static GraphControl *graph_checked(lua_State *L,int index,std::size_t slot) {
    // Metatables are per-control so compare an opaque registry token instead
    // of accepting a script-authored table/ID. Full userdata are never exposed
    // by another public API with this private marker.
    if(lua_type(L,index)!=LUA_TUSERDATA||lua_objlen(L,index)!=sizeof(GraphControl)||!lua_getmetatable(L,index))
        {luaL_error(L,"Expected authored control userdata");return nullptr;}
    lua_getfield(L,-1,"__civis_control_tag");lua_pushlightuserdata(L,&owner(L).control_graphs);
    const bool tagged=lua_rawequal(L,-1,-2);lua_pop(L,3);
    if(!tagged){luaL_error(L,"Foreign control userdata refused");return nullptr;}
    auto *value=static_cast<GraphControl*>(lua_touserdata(L,index));
    if(value->slot!=slot){luaL_error(L,"Foreign context control refused");return nullptr;}return value;
}
static int graph_control_call(lua_State *L) {
    auto &self=owner(L);const bool ready=self.ready,failed=self.failed;const auto epoch=self.failure_epoch;self.ready=false;self.failed=true;
    const auto slot=graph_slot(L);auto &entry=self.control_graphs[slot];entry.touched=true;
    graph_require_context(L,slot);
    const auto operation=static_cast<int>(lua_tointeger(L,lua_upvalueindex(2)));
    if(lua_gettop(L)!=(operation==3?1:2))return luaL_error(L,"Authored control argument count differs");
    const auto id=graph_checked(L,1,slot)->id;
    ScriptUiControlGraph::Id other=0;
    if(operation==0)luaL_checktype(L,2,LUA_TBOOLEAN);
    else if(operation==1||operation==2)other=graph_checked(L,2,slot)->id;
    bool hidden=false; // POD survives Lua error paths; no copied Node spans a Lua API.
    entry.touched=true;
    self.source_exception={};self.source_error=nullptr;
    try {
        if(operation==0)entry.graph->set_hidden(id,lua_toboolean(L,2)!=0);
        else if(operation==1)entry.graph->change_parent(id,other);
        else if(operation==2)entry.graph->destroy_child(id,other);
        // inspect returns an owning Node temporary, destroyed at this full-expression
        // boundary before any Lua result/error API. Keep the existing final fences.
        else if(operation==3)hidden=entry.graph->inspect(id).hidden;
        else throw ScriptError("Unsupported authored control operation");
        entry.graph->require_current();graph_require_epoch(L,epoch);self.ready=ready;self.failed=failed;
    }catch(const std::exception &error){self.source_exception=std::current_exception();self.source_error=error.what();}
    catch(...){self.source_exception=std::current_exception();self.source_error="Authored control operation failed";}
    if(self.source_error)return luaL_error(L,"%s",self.source_error);
    if(operation==3){lua_pushboolean(L,hidden);return 1;}return 0;
}
static int graph_context_call(lua_State *L) {
    auto &self=owner(L);const bool ready=self.ready,failed=self.failed;const auto epoch=self.failure_epoch;self.ready=false;if(self.worldview_control_entry==L){worldview_require_lua_origin(L);self.failed.set_loader_sentinel();}else self.failed=true;
    const auto slot=graph_slot(L);auto &entry=self.control_graphs[slot];entry.touched=true;
    graph_require_context(L,slot);
    const auto operation=static_cast<int>(lua_tointeger(L,lua_upvalueindex(2)));
    if(!lua_rawequal(L,1,lua_upvalueindex(3)))return luaL_error(L,"ContextPtr receiver differs from graph owner");
    if(operation==7)return worldview_context_hidden_owned(L,slot,ready,failed);
    if(operation==6){
        if(!entry.graph->backend_||self.worldview_control_entry!=L||lua_gettop(L)!=1)
            return luaL_error(L,"IsHotLoad requires actual protected native ContextPtr receiver");
        self.source_exception={};self.source_error=nullptr;
        try {
            self.worldview_require_callback_origin();
            const auto* const native=self.worldview_native_host;
            const auto* const loader=self.end_turn_child_loader;
            if(!native||!loader||loader->entry!=self.state||!loader->current()||
               loader->child!=native->child||native->child>=self.child_contexts.size())
                throw ScriptError("IsHotLoad requires SAME current initial native child-load occurrence");
            const auto child_slot=native->child;
            const auto& child=self.child_contexts[child_slot];
            // The native producer admits a new source only after rejecting every
            // earlier named child/executed occurrence. Its SAME healthy loader
            // is the cold-load authority; an imported save or false fields are not.
            if(!self.worldview_native_child_controls_current(child_slot)||
               child.name!=entry.graph->context()||!child.loading||child.loaded||child.retired||
               child.execution_thread==LUA_NOREF||child.receiver==LUA_NOREF||
               child.execution_source==graph_no_owner||child.execution_source!=self.graph_execution_scope||
               !self.executed_contexts.contains(child.name))
                throw ScriptError("IsHotLoad canonical initial context/source execution changed");
            for(std::size_t i=0;i<self.child_contexts.size();++i)
                if(i!=child_slot&&self.child_contexts[i].name==child.name)
                    throw ScriptError("IsHotLoad hot/repeated context has no qualified reload owner");
            lua_rawgeti(L,LUA_REGISTRYINDEX,child.execution_thread);
            const bool same_thread=lua_isthread(L,-1)&&lua_tothread(L,-1)==L;lua_pop(L,1);
            lua_rawgeti(L,LUA_REGISTRYINDEX,child.receiver);
            const bool same_receiver=lua_istable(L,-1)&&lua_rawequal(L,1,-1)&&
                lua_rawequal(L,lua_upvalueindex(3),-1);lua_pop(L,1);
            if(!same_thread||!same_receiver)
                throw ScriptError("IsHotLoad retained coroutine/ContextPtr receiver changed");
            self.child_require_current(child_slot);self.worldview_require_callback_origin();
            child.source->require_captured_bytes(child.bytes);self.worldview_require_callback_origin();
            entry.graph->require_current();self.worldview_require_callback_origin();
            if(self.worldview_native_host!=native||self.end_turn_child_loader!=loader||!loader->current()||
               loader->child!=child_slot||native->child!=child_slot||
               !self.worldview_native_child_controls_current(child_slot))
                throw ScriptError("IsHotLoad initial-load authority retired during validation");
            // Recheck actual Lua bindings after every owner/backend callback.
            self.child_require_bindings(L);
            lua_rawgeti(L,LUA_REGISTRYINDEX,child.execution_thread);
            const bool final_thread=lua_isthread(L,-1)&&lua_tothread(L,-1)==L;lua_pop(L,1);
            lua_rawgeti(L,LUA_REGISTRYINDEX,child.receiver);
            const bool final_receiver=lua_istable(L,-1)&&lua_rawequal(L,1,-1)&&
                lua_rawequal(L,lua_upvalueindex(3),-1);lua_pop(L,1);
            if(!final_thread||!final_receiver)
                throw ScriptError("IsHotLoad coroutine/ContextPtr rebound during validation");
        }catch(const std::exception& error){self.source_exception=std::current_exception();self.source_error=error.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="IsHotLoad actual initial-load lifecycle refused";}
        if(self.source_error){self.record_failure();return luaL_error(L,"%s",self.source_error);}
        self.ready=ready;if(failed)self.failed.set_loader_sentinel();else self.failed=false;
        lua_pushboolean(L,0);return 1;
    }
    if(operation==8){
        worldview_register_showhide_owned(L,slot,2);worldview_require_lua_origin(L);self.ready=ready;
        if(failed)self.failed.set_loader_sentinel();else self.failed=false;return 0;
    }
    if(operation==4||operation==5){
        if(!entry.graph->backend_||self.worldview_control_entry!=L||lua_gettop(L)!=2)
            return luaL_error(L,"WorldView context registration requires actual protected native receiver");
        worldview_register_owned_context_callback(L,slot,operation==5,2);
        worldview_require_lua_origin(L);self.ready=ready;
        if(failed)self.failed.set_loader_sentinel();else self.failed=false;
        return 0;
    }
    if(operation==3){
        if(!entry.graph->backend_||lua_gettop(L)!=2)return luaL_error(L,"WorldView lookup receiver/argument count differs");
        luaL_checktype(L,2,LUA_TSTRING);std::size_t size=0;const auto* path=lua_tolstring(L,2,&size);
        ScriptUiControlGraph::Id result=0;self.source_exception={};self.source_error=nullptr;
        try{worldview_require_lua_origin(L);result=entry.graph->lookup_control(std::string_view(path,size));
            entry.graph->require_current();self.worldview_require_callback_origin();}
        catch(const std::exception& error){self.source_exception=std::current_exception();self.source_error=error.what();}
        catch(...){self.source_exception=std::current_exception();self.source_error="WorldView authentic sibling lookup refused";}
        if(self.source_error){self.record_failure();return luaL_error(L,"%s",self.source_error);}
        self.ready=ready;if(failed){worldview_require_lua_origin(L);self.failed.set_loader_sentinel();}else self.failed=false;
        graph_push_control(L,slot,result);return 1;
    }
    const auto expected=operation==0?3:operation==1?4:2;
    if(lua_gettop(L)!=expected)return luaL_error(L,"Authored instance argument count differs");
    ScriptUiControlGraph::Id parent=0,root=0;
    const char *name=nullptr;std::size_t name_size=0;
    if(operation<2) {
        luaL_checktype(L,2,LUA_TSTRING);name=lua_tolstring(L,2,&name_size);luaL_checktype(L,3,LUA_TTABLE);
        lua_pushnil(L);if(lua_next(L,3)){lua_pop(L,2);return luaL_error(L,"Instance output table must be empty");}
        if(operation==1)parent=graph_checked(L,4,slot)->id;
    } else {
        luaL_checktype(L,2,LUA_TTABLE);
        for(const auto &instance:self.graph_instances)if(instance.slot==slot) {
            lua_rawgeti(L,LUA_REGISTRYINDEX,instance.table);const bool match=lua_rawequal(L,2,-1);lua_pop(L,1);
            if(match){root=instance.root;break;}
        }
        if(!root)return luaL_error(L,"DestroyChild instance table is foreign or retired");
    }
    entry.touched=true;
    self.source_exception={};self.source_error=nullptr;
    try {
        if(operation<2)self.graph_scratch=entry.graph->build(std::string_view(name,name_size),parent);
        else entry.graph->destroy_child(0,root);
        entry.graph->require_current();graph_require_epoch(L,epoch);
        if(operation<2&&self.graph_instances.size()>=4096)throw ScriptError("Authored instance table budget exhausted");
    }catch(const std::exception &error){self.source_exception=std::current_exception();self.source_error=error.what();}
    catch(...){self.source_exception=std::current_exception();self.source_error="Authored instance operation failed";}
    if(self.source_error)return luaL_error(L,"%s",self.source_error);
    if(operation<2) {
        // Any Lua OOM below poisons the touched graph in Impl::call. Partial
        // tables/nodes can never be accepted or replayed as a healthy graph.
        lua_pushvalue(L,3);const int ref=luaL_ref(L,LUA_REGISTRYINDEX);
        try {self.graph_instances.push_back({slot,ref,self.graph_scratch.front().id});}
        catch(...){self.source_exception=std::current_exception();return luaL_error(L,"Instance retention failed");}
        for(const auto &node:self.graph_scratch) {
            lua_pushlstring(L,node.name.data(),node.name.size());graph_push_control(L,slot,node.id);lua_rawset(L,3);
        }
        self.graph_scratch.clear();
    }
    self.ready=ready;if(failed&&self.worldview_control_entry==L){worldview_require_lua_origin(L);self.failed.set_loader_sentinel();}else self.failed=failed;return 0;
}
static void install_control_graph(lua_State *L,int environment,std::size_t slot) {
    auto &self=owner(L);auto &entry=self.control_graphs[slot];
    entry.touched=true;
    lua_pushvalue(L,environment);entry.environment=luaL_ref(L,LUA_REGISTRYINDEX);
    self.graph_scratch=entry.graph->static_controls();
    lua_newtable(L);
    for(const auto &node:self.graph_scratch) {
        lua_pushlstring(L,node.name.data(),node.name.size());graph_push_control(L,slot,node.id);lua_rawset(L,-3);
    }
    lua_setfield(L,environment,"Controls");self.graph_scratch.clear();
    lua_newtable(L);
    constexpr const char *methods[]={"BuildInstance","BuildInstanceForControl","DestroyChild","LookUpControl","SetInputHandler","SetShutdown","IsHotLoad","IsHidden","SetShowHideHandler"};
    const int method_count=entry.graph->backend_?9:3;
    for(int operation=0;operation<method_count;++operation) {
        lua_pushinteger(L,static_cast<lua_Integer>(slot));lua_pushinteger(L,operation);lua_pushvalue(L,-3);
        lua_pushcclosure(L,owner(L).control_graphs[slot].graph->backend_?worldview_context_call:graph_context_call,3);lua_setfield(L,-2,methods[operation]);
    }
    lua_setfield(L,environment,"ContextPtr");entry.bound=true;
    if(entry.graph->backend_)worldview_install_screen_manager(L,environment,slot);
}
