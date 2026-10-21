// Inside Host::Impl. Every allocating/callback-capable Lua operation remains
// in the actual lua_cpcall job; C++ nontrivial owners live in the outer frame.
static constexpr const char*const native_events_prefix_keys[]={"EventFactories","__index","Events","StateName","AppInitComplete","CameraViewChanged","CameraProjectionChanged","AnimationSamplingChanged","DontRecordCommandStreams","RecordCommandStreams","CityHandleCreated","UnitHandleCreated","UnitTypeChanged","WonderTypeChanged","WonderStateChanged","WonderTogglePlacement","ToggleDisplayUnits","GlobalUnitScale","LandmarkLibrarySwap","UnitLibrarySwap","BuildingLibrarySwap","SequenceGameInitComplete","StateMachineDumpStates","StateMachineRequestStates","StrategicViewStateChanged","AIProcessingStartedForPlayer","AIProcessingEndedForPlayer","AIProcessingStepChanged","GameViewTypeChanged","LuaEvent"};
#include "lua_native_context_events_publication.inc"
static void native_events_stack_table_physical(lua_State* L,int index,const void* pointer){
    if(!lua_istable(L,index)||lua_topointer(L,index)!=pointer||lua_getmetatable(L,index))
        return (void)luaL_error(L,"Events genuine pending stack table changed after FULL");
}
static void native_events_prefix_keys_physical(lua_State* L,const NativeContextEventsPrefixFrame& f){
    if(f.keys_ref==LUA_NOREF||f.keys_ref==LUA_REFNIL||!f.keys)
        return (void)luaL_error(L,"Events genuine retained keys absent");
    const int top=lua_gettop(L);lua_rawgeti(L,LUA_REGISTRYINDEX,f.keys_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f.keys||lua_objlen(L,-1)!=30||lua_getmetatable(L,-1))
        return (void)luaL_error(L,"Events retained key table/ref changed");
    const int keys=lua_gettop(L);
    for(int i=0;i<30;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_events_prefix_keys[i])||std::memcmp(value,native_events_prefix_keys[i],n))
            return (void)luaL_error(L,"Events retained interned key changed");
        lua_pop(L,1);
    }
    lua_settop(L,top);
}
static void native_events_pending_table_physical(lua_State* L,int reference,const void* pointer){
    if(reference==LUA_NOREF||reference==LUA_REFNIL||!pointer)
        return (void)luaL_error(L,"Events pending actual table/ref absent");
    lua_rawgeti(L,LUA_REGISTRYINDEX,reference);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=pointer||lua_getmetatable(L,-1))
        return (void)luaL_error(L,"Events pending actual table/ref changed");
    lua_pushnil(L);
    if(lua_next(L,-2))return (void)luaL_error(L,"Events pending generator/factory acquired unowned contents");
    lua_pop(L,1);
}
static void native_events_pending_generators_physical(lua_State* L,const NativeContextEventsPrefixFrame& f){
    if(f.generators_ref==LUA_NOREF||f.generators_ref==LUA_REFNIL||!f.generators)
        return (void)luaL_error(L,"Events actual generator table/ref absent");
    const int top=lua_gettop(L);lua_rawgeti(L,LUA_REGISTRYINDEX,f.generators_ref);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f.generators||lua_getmetatable(L,-1))
        return (void)luaL_error(L,"Events pending actual generators changed");
    const int generators=lua_gettop(L);int count=0;std::array<bool,232> ordered_seen{};std::size_t ordered_index=0;auto& p=owner(L);
    if(f.ordered_assigned_count>native_ordered_specs.size())return (void)luaL_error(L,"ordered Events real assignment count exceeds actual registrars");
    if(f.ordered_assigned_count)native_ordered_channel_physical(L);bool app_seen=false,camera_seen=false,projection_seen=false,animation_seen=false,dont_record_seen=false,record_seen=false,city_seen=false,unit_seen=false,unit_type_seen=false,wonder_type_seen=false,wonder_state_seen=false,wonder_placement_seen=false,display_units_seen=false,unit_scale_seen=false,landmark_swap_seen=false,unit_swap_seen=false,building_swap_seen=false,sequence_init_seen=false,state_dump_seen=false,state_request_seen=false,strategic_view_seen=false,ai_started_seen=false,ai_ended_seen=false,ai_step_seen=false,game_view_seen=false,lua_event_seen=false;lua_pushnil(L);
    while(lua_next(L,generators)){
        const void*const closure=lua_topointer(L,-1);const auto function=lua_tocfunction(L,-1);int key=0;
        if(f.app_constructor_assigned&&closure==f.app_constructor_assigned&&function==native_app_init_constructor&&!app_seen){
            app_seen=true;key=5;
        }else if(f.camera_constructor_assigned&&closure==f.camera_constructor_assigned&&function==native_camera_view_constructor&&!camera_seen){
            camera_seen=true;key=6;
        }else if(f.projection_constructor_assigned&&closure==f.projection_constructor_assigned&&function==native_camera_projection_constructor&&!projection_seen){
            projection_seen=true;key=7;
        }else if(f.animation_constructor_assigned&&closure==f.animation_constructor_assigned&&function==native_animation_sampling_constructor&&!animation_seen){
            animation_seen=true;key=8;
        }else if(f.dont_record_constructor_assigned&&closure==f.dont_record_constructor_assigned&&function==native_dont_record_commands_constructor&&!dont_record_seen){
            dont_record_seen=true;key=9;
        }else if(f.record_constructor_assigned&&closure==f.record_constructor_assigned&&function==native_record_commands_constructor&&!record_seen){
            record_seen=true;key=10;
        }else if(f.city_constructor_assigned&&closure==f.city_constructor_assigned&&function==native_city_handle_constructor&&!city_seen){
            city_seen=true;key=11;
        }else if(f.unit_constructor_assigned&&closure==f.unit_constructor_assigned&&function==native_unit_handle_constructor&&!unit_seen){
            unit_seen=true;key=12;
        }else if(f.unit_type_constructor_assigned&&closure==f.unit_type_constructor_assigned&&function==native_unit_type_constructor&&!unit_type_seen){
            unit_type_seen=true;key=13;
        }else if(f.wonder_type_constructor_assigned&&closure==f.wonder_type_constructor_assigned&&function==native_wonder_type_constructor&&!wonder_type_seen){
            wonder_type_seen=true;key=14;
        }else if(f.wonder_state_constructor_assigned&&closure==f.wonder_state_constructor_assigned&&function==native_wonder_state_constructor&&!wonder_state_seen){
            wonder_state_seen=true;key=15;
        }else if(f.wonder_placement_constructor_assigned&&closure==f.wonder_placement_constructor_assigned&&function==native_wonder_placement_constructor&&!wonder_placement_seen){
            wonder_placement_seen=true;key=16;
        }else if(f.display_units_constructor_assigned&&closure==f.display_units_constructor_assigned&&function==native_display_units_constructor&&!display_units_seen){
            display_units_seen=true;key=17;
        }else if(f.unit_scale_constructor_assigned&&closure==f.unit_scale_constructor_assigned&&function==native_unit_scale_constructor&&!unit_scale_seen){
            unit_scale_seen=true;key=18;
        }else if(f.landmark_swap_constructor_assigned&&closure==f.landmark_swap_constructor_assigned&&function==native_landmark_swap_constructor&&!landmark_swap_seen){
            landmark_swap_seen=true;key=19;
        }else if(f.unit_swap_constructor_assigned&&closure==f.unit_swap_constructor_assigned&&function==native_unit_swap_constructor&&!unit_swap_seen){
            unit_swap_seen=true;key=20;
        }else if(f.building_swap_constructor_assigned&&closure==f.building_swap_constructor_assigned&&function==native_building_swap_constructor&&!building_swap_seen){
            building_swap_seen=true;key=21;
        }else if(f.sequence_init_constructor_assigned&&closure==f.sequence_init_constructor_assigned&&function==native_sequence_init_constructor&&!sequence_init_seen){
            sequence_init_seen=true;key=22;
        }else if(f.state_dump_constructor_assigned&&closure==f.state_dump_constructor_assigned&&function==native_state_dump_constructor&&!state_dump_seen){
            state_dump_seen=true;key=23;
        }else if(f.state_request_constructor_assigned&&closure==f.state_request_constructor_assigned&&function==native_state_request_constructor&&!state_request_seen){
            state_request_seen=true;key=24;
        }else if(f.strategic_view_constructor_assigned&&closure==f.strategic_view_constructor_assigned&&function==native_strategic_view_constructor&&!strategic_view_seen){
            strategic_view_seen=true;key=25;
        }else if(f.ai_started_constructor_assigned&&closure==f.ai_started_constructor_assigned&&function==native_ai_started_constructor&&!ai_started_seen){
            ai_started_seen=true;key=26;
        }else if(f.ai_ended_constructor_assigned&&closure==f.ai_ended_constructor_assigned&&function==native_ai_ended_constructor&&!ai_ended_seen){
            ai_ended_seen=true;key=27;
        }else if(f.ai_step_constructor_assigned&&closure==f.ai_step_constructor_assigned&&function==native_ai_step_constructor&&!ai_step_seen){
            ai_step_seen=true;key=28;
        }else if(f.game_view_constructor_assigned&&closure==f.game_view_constructor_assigned&&function==native_game_view_constructor&&!game_view_seen){
            game_view_seen=true;key=29;
        }else if(f.lua_event_constructor_assigned&&closure==f.lua_event_constructor_assigned&&function==native_lua_event_constructor&&!lua_event_seen){
            lua_event_seen=true;key=30;
        }else{
            ordered_index=native_ordered_specs.size();
            for(std::size_t i=0;i<f.ordered_assigned_count;++i){
                const auto& channel=p.native_ordered_channels[i];
                if(native_ordered_binding_active(i,f.ordered_assigned_count)&&channel.assigned&&channel.assigned==channel.created&&closure==channel.assigned&&function==native_ordered_constructors[i]&&!ordered_seen[i]){
                    ordered_seen[i]=true;ordered_index=i;break;
                }
            }
            if(ordered_index==native_ordered_specs.size())return (void)luaL_error(L,"Events generator contents lack genuine ordered constructors");
        }
        if(key==30){
            if(!lua_getupvalue(L,-1,1)||!lua_istable(L,-1)||lua_topointer(L,-1)!=p.native_lua_event_group)
                return (void)luaL_error(L,"Events actual4CAF40 weak-group upvalue changed");
            lua_pop(L,1);if(lua_getupvalue(L,-1,2))return (void)luaL_error(L,"Events native LuaEvent one upvalue changed");
        }else if(lua_getupvalue(L,-1,1))return (void)luaL_error(L,"Events native constructor acquired unowned upvalues");
        ++count;
        if(key){lua_rawgeti(L,LUA_REGISTRYINDEX,f.keys_ref);lua_rawgeti(L,-1,key);lua_remove(L,-2);}
        else{
            native_ordered_name(L,ordered_index);std::size_t n=0;
            const char*const name=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
            if(!name||n!=std::strlen(native_ordered_specs[ordered_index].name)||std::memcmp(name,native_ordered_specs[ordered_index].name,n))
                return (void)luaL_error(L,"ordered Events original generator name changed");
        }
        const bool same=lua_rawequal(L,-1,-3);lua_pop(L,1);
        if(!same)return (void)luaL_error(L,"Events generator literal key differs from actual constructor");
        lua_pop(L,1);
    }
    if(count!=(f.app_constructor_assigned?1:0)+(f.camera_constructor_assigned?1:0)+(f.projection_constructor_assigned?1:0)+(f.animation_constructor_assigned?1:0)+(f.dont_record_constructor_assigned?1:0)+(f.record_constructor_assigned?1:0)+(f.city_constructor_assigned?1:0)+(f.unit_constructor_assigned?1:0)+(f.unit_type_constructor_assigned?1:0)+(f.wonder_type_constructor_assigned?1:0)+(f.wonder_state_constructor_assigned?1:0)+(f.wonder_placement_constructor_assigned?1:0)+(f.display_units_constructor_assigned?1:0)+(f.unit_scale_constructor_assigned?1:0)+(f.landmark_swap_constructor_assigned?1:0)+(f.unit_swap_constructor_assigned?1:0)+(f.building_swap_constructor_assigned?1:0)+(f.sequence_init_constructor_assigned?1:0)+(f.state_dump_constructor_assigned?1:0)+(f.state_request_constructor_assigned?1:0)+(f.strategic_view_constructor_assigned?1:0)+(f.ai_started_constructor_assigned?1:0)+(f.ai_ended_constructor_assigned?1:0)+(f.ai_step_constructor_assigned?1:0)+(f.game_view_constructor_assigned?1:0)+static_cast<int>(native_ordered_active_binding_count(f.ordered_assigned_count))+(f.lua_event_constructor_assigned?1:0)||
       (f.app_constructor_assigned&&(!app_seen||f.app_constructor_assigned!=f.app_constructor_created))||
       (f.camera_constructor_assigned&&(!camera_seen||f.camera_constructor_assigned!=f.camera_constructor_created||!f.app_constructor_assigned))||
       (f.projection_constructor_assigned&&(!projection_seen||f.projection_constructor_assigned!=f.projection_constructor_created||!f.camera_constructor_assigned))||
       (f.animation_constructor_assigned&&(!animation_seen||f.animation_constructor_assigned!=f.animation_constructor_created||!f.projection_constructor_assigned))||
       (f.dont_record_constructor_assigned&&(!dont_record_seen||f.dont_record_constructor_assigned!=f.dont_record_constructor_created||!f.animation_constructor_assigned))||
       (f.record_constructor_assigned&&(!record_seen||f.record_constructor_assigned!=f.record_constructor_created||!f.dont_record_constructor_assigned))||
       (f.city_constructor_assigned&&(!city_seen||f.city_constructor_assigned!=f.city_constructor_created||!f.record_constructor_assigned))||
       (f.unit_constructor_assigned&&(!unit_seen||f.unit_constructor_assigned!=f.unit_constructor_created||!f.city_constructor_assigned))||
       (f.unit_type_constructor_assigned&&(!unit_type_seen||f.unit_type_constructor_assigned!=f.unit_type_constructor_created||!f.unit_constructor_assigned))||
       (f.wonder_type_constructor_assigned&&(!wonder_type_seen||f.wonder_type_constructor_assigned!=f.wonder_type_constructor_created||!f.unit_type_constructor_assigned))||
       (f.wonder_state_constructor_assigned&&(!wonder_state_seen||f.wonder_state_constructor_assigned!=f.wonder_state_constructor_created||!f.wonder_type_constructor_assigned))||
       (f.wonder_placement_constructor_assigned&&(!wonder_placement_seen||f.wonder_placement_constructor_assigned!=f.wonder_placement_constructor_created||!f.wonder_state_constructor_assigned))||
       (f.display_units_constructor_assigned&&(!display_units_seen||f.display_units_constructor_assigned!=f.display_units_constructor_created||!f.wonder_placement_constructor_assigned))||
       (f.unit_scale_constructor_assigned&&(!unit_scale_seen||f.unit_scale_constructor_assigned!=f.unit_scale_constructor_created||!f.display_units_constructor_assigned))||
       (f.landmark_swap_constructor_assigned&&(!landmark_swap_seen||f.landmark_swap_constructor_assigned!=f.landmark_swap_constructor_created||!f.unit_scale_constructor_assigned))||
       (f.unit_swap_constructor_assigned&&(!unit_swap_seen||f.unit_swap_constructor_assigned!=f.unit_swap_constructor_created||!f.landmark_swap_constructor_assigned))||
       (f.building_swap_constructor_assigned&&(!building_swap_seen||f.building_swap_constructor_assigned!=f.building_swap_constructor_created||!f.unit_swap_constructor_assigned))||
       (f.sequence_init_constructor_assigned&&(!sequence_init_seen||f.sequence_init_constructor_assigned!=f.sequence_init_constructor_created||!f.building_swap_constructor_assigned))||
       (f.state_dump_constructor_assigned&&(!state_dump_seen||f.state_dump_constructor_assigned!=f.state_dump_constructor_created||!f.sequence_init_constructor_assigned))||
       (f.state_request_constructor_assigned&&(!state_request_seen||f.state_request_constructor_assigned!=f.state_request_constructor_created||!f.state_dump_constructor_assigned))||
       (f.strategic_view_constructor_assigned&&(!strategic_view_seen||f.strategic_view_constructor_assigned!=f.strategic_view_constructor_created||!f.state_request_constructor_assigned))||
       (f.ai_started_constructor_assigned&&(!ai_started_seen||f.ai_started_constructor_assigned!=f.ai_started_constructor_created||!f.strategic_view_constructor_assigned))||
       (f.ai_ended_constructor_assigned&&(!ai_ended_seen||f.ai_ended_constructor_assigned!=f.ai_ended_constructor_created||!f.ai_started_constructor_assigned))||
       (f.ai_step_constructor_assigned&&(!ai_step_seen||f.ai_step_constructor_assigned!=f.ai_step_constructor_created||!f.ai_ended_constructor_assigned))||
       (f.game_view_constructor_assigned&&(!game_view_seen||f.game_view_constructor_assigned!=f.game_view_constructor_created||!f.ai_step_constructor_assigned))||
       (f.lua_event_constructor_assigned&&(!lua_event_seen||f.lua_event_constructor_created!=f.lua_event_constructor_assigned||f.ordered_assigned_count!=232)))
        return (void)luaL_error(L,"Events genuine ordered constructor assignment changed");
    for(std::size_t i=0;i<f.ordered_assigned_count;++i){
        const auto& channel=p.native_ordered_channels[i];
        if(ordered_seen[i]!=native_ordered_binding_active(i,f.ordered_assigned_count)||!channel.assigned||channel.created!=channel.assigned||!f.game_view_constructor_assigned)
            return (void)luaL_error(L,"ordered Events actual constructor assignment changed");
    }
    lua_settop(L,top);
}
static void native_events_prefix_physical(lua_State* L,const NativeContextEventsPrefixFrame& f){
    // LAST FULL is followed only by raw copies of retained TValue keys. No
    // lua_pushstring/literal or GC-triggering key intern is used in this POST.
    const int top=lua_gettop(L);
    if(f.keys_ref!=LUA_NOREF)native_events_prefix_keys_physical(L,f);
    if(f.master){
        lua_rawgeti(L,LUA_REGISTRYINDEX,f.keys_ref);const int keys=lua_gettop(L);
        lua_rawgeti(L,keys,1);lua_rawget(L,LUA_GLOBALSINDEX);
        if(!lua_istable(L,-1)||lua_topointer(L,-1)!=f.master||lua_getmetatable(L,-1))
            return (void)luaL_error(L,"Events actual master EventFactories changed");
        if(f.master_ref!=LUA_NOREF){
            lua_rawgeti(L,LUA_REGISTRYINDEX,f.master_ref);
            if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"Events actual master ref rebound");
            lua_pop(L,1);
        }
        // Original573AC0 publishes maskFFFFFF only AFTER all four native
        // generator registrars and547460 closure. It is still nil here.
        lua_rawgeti(L,-1,0xFFFFFF);
        if(f.master_factory_assigned){
            if(f.master_factory_assigned!=f.factory||!f.factory_index_assigned||!lua_istable(L,-1)||lua_topointer(L,-1)!=f.factory)
                return (void)luaL_error(L,"Events actual selected mask factory changed after publication");
        }else if(!lua_isnil(L,-1))return (void)luaL_error(L,"Events selected mask factory was prematurely published");
    }
    lua_settop(L,top);
    if(f.factory_ref!=LUA_NOREF){
        if(f.factory_index_assigned)native_events_factory_index_physical(L,f);
        else native_events_pending_table_physical(L,f.factory_ref,f.factory);
    }
    if(f.generators_ref!=LUA_NOREF)native_events_pending_generators_physical(L,f);
    if(f.published_events_table)native_events_selected_publication_physical(L,f);
}
static void native_events_prefix_full(lua_State* L,Job& job){
    auto& p=owner(L);auto*const frame=p.events_prefix_frame;
    p.source_exception={};p.source_error=nullptr;
    try{
        if(!frame||frame->child!=job.worldview_child)throw ScriptError("Events actual load frame absent");
        frame->full();
    }catch(...){p.source_exception=std::current_exception();p.source_error="Events SAME load/source FULL refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    native_debug_config_full(L,job);
    p.source_exception={};p.source_error=nullptr;
    try{
        if(p.events_prefix_frame!=frame)throw ScriptError("Events actual frame replaced");
        frame->first();
    }catch(...){p.source_exception=std::current_exception();p.source_error="Events FIRST after callbacks refused";}
    if(p.source_error){p.record_failure();luaL_error(L,"%s",p.source_error);}
    if(job.context_state_environment!=frame->actual_environment||job.context_state_thread!=frame->actual_thread)
        return (void)luaL_error(L,"Events actual518 physical operand changed");
    const int top=lua_gettop(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,frame->thread);
    if(lua_tothread(L,-1)!=frame->actual_thread||lua_tothread(L,-1)==L||
       lua_status(frame->actual_thread)||(lua_gettop(frame->actual_thread)&&
        !native_selected_registrar_busy_thread_owned(L,frame->actual_thread,frame->thread,frame->environment,frame->actual_environment,frame->child)))
        return (void)luaL_error(L,"Events actual518 idle coroutine changed");
    lua_getfenv(L,-1);lua_rawgeti(L,LUA_REGISTRYINDEX,frame->environment);
    if(!lua_istable(L,-1)||lua_topointer(L,-1)!=frame->actual_environment||!lua_rawequal(L,-1,-2))
        return (void)luaL_error(L,"Events actual518 coroutine environment changed");
    if(frame->keys_ref!=LUA_NOREF){
        native_events_prefix_keys_physical(L,*frame);
        const int environment=lua_gettop(L);
        lua_rawgeti(L,LUA_REGISTRYINDEX,frame->keys_ref);lua_rawgeti(L,-1,4);lua_rawget(L,environment);
        std::size_t n=0;const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=job.context_state_name.size()||std::memcmp(value,job.context_state_name.data(),n))
            return (void)luaL_error(L,"Events genuine StateName changed");
    }
    lua_settop(L,top);
    native_profile_completed_physical_query(L,job);
    native_events_prefix_physical(L,*frame);
}
static void native_events_prepare_app_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||p.native_app_init_retired||p.native_app_init_keys_ref!=LUA_NOREF||
       p.native_app_init_weak_ref!=LUA_NOREF||p.native_app_init_generator_ref!=LUA_NOREF||
       !p.native_app_init_subscribers.empty()||!p.native_app_init_queue.empty())
        return (void)luaL_error(L,"AppInitComplete private channel requires its genuine first registrar");
    // Retain the authentic native literal keys before allocating the private
    // watcher. No C++ owner is created inside this longjmp boundary.
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_app_init_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_app_init_keys_table=keys_pointer;
    lua_pushvalue(L,keys);p.native_app_init_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_app_init_keys[i])||std::memcmp(value,native_app_init_keys[i],n))
            return (void)luaL_error(L,"AppInitComplete genuine prepared key changed");
        lua_pop(L,1);
    }
    // A native channel must not root abandoned retail event cycles. This
    // weak-value table is exclusively port lifetime bookkeeping.
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,app_mode);lua_rawgeti(L,keys,app_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,app_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,app_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"AppInitComplete genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_app_init_weak_table=weak_pointer;p.native_app_init_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_app_init_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_app_init_channel_physical(L);
    lua_pop(L,2); // private weak watcher and keys; actual generators stay top
}
static void native_events_register_app_init_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->generators||f->app_constructor_created||f->app_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 first generator is absent or replayed");
    native_events_prepare_app_channel(L,job);native_events_prefix_full(L,job);
    native_app_init_channel_physical(L);
    // Actual75E54B name literal; zero-upvalue native5744E0 constructor.
    // Created and assigned pointers are derived solely from actual Lua work.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,5);lua_remove(L,-2);
    lua_pushcclosure(L,native_app_init_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->app_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_app_init_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events actual AppInitComplete constructor closure changed after FULL");
    native_app_init_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    // Stack suffix is the retained generators, authentic key, actual closure.
    lua_rawset(L,-3);f->app_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_app_init_channel_physical(L);
    p.native_app_init_generator_ref=f->generators_ref;p.native_app_init_generator_table=f->generators;
    p.native_app_init_constructor_closure=constructor;
    p.native_app_init_epoch=f->epoch;p.native_app_init_writes=f->writes;
    p.native_app_init_uncertainty=f->uncertainty;
    // LAST FULL precedes pure actual-generation/retained-table checks only.
    native_app_init_current(L);lua_pop(L,1);
}

static void native_events_prepare_camera_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->app_constructor_assigned||p.native_camera_view_retired||
       p.native_camera_view_keys_ref!=LUA_NOREF||p.native_camera_view_weak_ref!=LUA_NOREF||
       p.native_camera_view_generator_ref!=LUA_NOREF||!p.native_camera_view_subscribers.empty()||
       !p.native_camera_view_queue.empty())
        return (void)luaL_error(L,"CameraViewChanged channel requires genuine ordered second registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_camera_view_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_camera_view_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_camera_view_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_camera_view_keys[i])||std::memcmp(value,native_camera_view_keys[i],n))
            return (void)luaL_error(L,"CameraViewChanged genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,camera_view_mode);lua_rawgeti(L,keys,camera_view_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,camera_view_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,camera_view_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"CameraViewChanged genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_camera_view_weak_table=weak_pointer;p.native_camera_view_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_camera_view_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_camera_view_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_camera_view_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->app_constructor_assigned||!f->generators||
       f->camera_constructor_created||f->camera_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 CameraViewChanged order/replay refused");
    native_app_init_current(L);native_events_prepare_camera_channel(L,job);
    native_events_prefix_full(L,job);native_camera_view_channel_physical(L);
    // Real75E565 name, 75E572 ->493CB0 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,6);lua_remove(L,-2);
    lua_pushcclosure(L,native_camera_view_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->camera_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_camera_view_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine493CB0 CameraViewChanged closure changed after FULL");
    native_camera_view_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->camera_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_camera_view_channel_physical(L);
    p.native_camera_view_generator_ref=f->generators_ref;p.native_camera_view_generator_table=f->generators;
    p.native_camera_view_constructor_closure=constructor;
    p.native_camera_view_epoch=f->epoch;p.native_camera_view_writes=f->writes;
    p.native_camera_view_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);lua_pop(L,1);
}

static void native_events_prepare_projection_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->camera_constructor_assigned||p.native_camera_projection_retired||
       p.native_camera_projection_keys_ref!=LUA_NOREF||p.native_camera_projection_weak_ref!=LUA_NOREF||
       p.native_camera_projection_generator_ref!=LUA_NOREF||!p.native_camera_projection_subscribers.empty()||
       !p.native_camera_projection_queue.empty())
        return (void)luaL_error(L,"CameraProjectionChanged channel requires genuine ordered third registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_camera_projection_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_camera_projection_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_camera_projection_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_camera_projection_keys[i])||std::memcmp(value,native_camera_projection_keys[i],n))
            return (void)luaL_error(L,"CameraProjectionChanged genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,camera_projection_mode);lua_rawgeti(L,keys,camera_projection_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,camera_projection_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,camera_projection_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"CameraProjectionChanged genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_camera_projection_weak_table=weak_pointer;p.native_camera_projection_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_camera_projection_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_camera_projection_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_camera_projection_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->camera_constructor_assigned||!f->generators||
       f->projection_constructor_created||f->projection_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 CameraProjectionChanged order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_events_prepare_projection_channel(L,job);
    native_events_prefix_full(L,job);native_camera_projection_channel_physical(L);
    // Real75E585 name, 75E595 ->42F3F0 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,7);lua_remove(L,-2);
    lua_pushcclosure(L,native_camera_projection_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->projection_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_camera_projection_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine42F3F0 CameraProjectionChanged closure changed after FULL");
    native_camera_projection_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->projection_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_camera_projection_channel_physical(L);
    p.native_camera_projection_generator_ref=f->generators_ref;p.native_camera_projection_generator_table=f->generators;
    p.native_camera_projection_constructor_closure=constructor;
    p.native_camera_projection_epoch=f->epoch;p.native_camera_projection_writes=f->writes;
    p.native_camera_projection_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);lua_pop(L,1);
}

static void native_events_prepare_animation_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->projection_constructor_assigned||p.native_animation_sampling_retired||
       p.native_animation_sampling_keys_ref!=LUA_NOREF||p.native_animation_sampling_weak_ref!=LUA_NOREF||
       p.native_animation_sampling_generator_ref!=LUA_NOREF||!p.native_animation_sampling_subscribers.empty()||
       !p.native_animation_sampling_queue.empty())
        return (void)luaL_error(L,"AnimationSamplingChanged channel requires genuine ordered fourth registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_animation_sampling_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_animation_sampling_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_animation_sampling_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_animation_sampling_keys[i])||std::memcmp(value,native_animation_sampling_keys[i],n))
            return (void)luaL_error(L,"AnimationSamplingChanged genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,animation_sampling_mode);lua_rawgeti(L,keys,animation_sampling_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,animation_sampling_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,animation_sampling_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"AnimationSamplingChanged genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_animation_sampling_weak_table=weak_pointer;p.native_animation_sampling_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_animation_sampling_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_animation_sampling_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_animation_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->projection_constructor_assigned||!f->generators||
       f->animation_constructor_created||f->animation_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 AnimationSamplingChanged order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_events_prepare_animation_channel(L,job);
    native_events_prefix_full(L,job);native_animation_sampling_channel_physical(L);
    // Real75E5A8 name, 75E5B5 ->4D00F0 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,8);lua_remove(L,-2);
    lua_pushcclosure(L,native_animation_sampling_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->animation_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_animation_sampling_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine4D00F0 AnimationSamplingChanged closure changed after FULL");
    native_animation_sampling_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->animation_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_animation_sampling_channel_physical(L);
    p.native_animation_sampling_generator_ref=f->generators_ref;p.native_animation_sampling_generator_table=f->generators;
    p.native_animation_sampling_constructor_closure=constructor;
    p.native_animation_sampling_epoch=f->epoch;p.native_animation_sampling_writes=f->writes;
    p.native_animation_sampling_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);lua_pop(L,1);
}

static void native_events_prepare_dont_record_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->animation_constructor_assigned||p.native_dont_record_commands_retired||
       p.native_dont_record_commands_keys_ref!=LUA_NOREF||p.native_dont_record_commands_weak_ref!=LUA_NOREF||
       p.native_dont_record_commands_generator_ref!=LUA_NOREF||!p.native_dont_record_commands_subscribers.empty()||
       !p.native_dont_record_commands_queue.empty())
        return (void)luaL_error(L,"DontRecordCommandStreams channel requires genuine ordered fifth registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_dont_record_commands_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_dont_record_commands_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_dont_record_commands_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_dont_record_commands_keys[i])||std::memcmp(value,native_dont_record_commands_keys[i],n))
            return (void)luaL_error(L,"DontRecordCommandStreams genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,dont_record_commands_mode);lua_rawgeti(L,keys,dont_record_commands_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,dont_record_commands_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,dont_record_commands_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"DontRecordCommandStreams genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_dont_record_commands_weak_table=weak_pointer;p.native_dont_record_commands_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_dont_record_commands_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_dont_record_commands_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_dont_record_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->animation_constructor_assigned||!f->generators||
       f->dont_record_constructor_created||f->dont_record_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 DontRecordCommandStreams order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_events_prepare_dont_record_channel(L,job);
    native_events_prefix_full(L,job);native_dont_record_commands_channel_physical(L);
    // Real75E5C8 name, 75E5D5 ->5DA740 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,9);lua_remove(L,-2);
    lua_pushcclosure(L,native_dont_record_commands_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->dont_record_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_dont_record_commands_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine5DA740 DontRecordCommandStreams closure changed after FULL");
    native_dont_record_commands_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->dont_record_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_dont_record_commands_channel_physical(L);
    p.native_dont_record_commands_generator_ref=f->generators_ref;p.native_dont_record_commands_generator_table=f->generators;
    p.native_dont_record_commands_constructor_closure=constructor;
    p.native_dont_record_commands_epoch=f->epoch;p.native_dont_record_commands_writes=f->writes;
    p.native_dont_record_commands_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);lua_pop(L,1);
}

static void native_events_prepare_record_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->dont_record_constructor_assigned||p.native_record_commands_retired||
       p.native_record_commands_keys_ref!=LUA_NOREF||p.native_record_commands_weak_ref!=LUA_NOREF||
       p.native_record_commands_generator_ref!=LUA_NOREF||!p.native_record_commands_subscribers.empty()||
       !p.native_record_commands_queue.empty())
        return (void)luaL_error(L,"RecordCommandStreams channel requires genuine ordered sixth registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_record_commands_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_record_commands_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_record_commands_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_record_commands_keys[i])||std::memcmp(value,native_record_commands_keys[i],n))
            return (void)luaL_error(L,"RecordCommandStreams genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,record_commands_mode);lua_rawgeti(L,keys,record_commands_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,record_commands_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,record_commands_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"RecordCommandStreams genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_record_commands_weak_table=weak_pointer;p.native_record_commands_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_record_commands_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_record_commands_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_record_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->dont_record_constructor_assigned||!f->generators||
       f->record_constructor_created||f->record_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 RecordCommandStreams order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_events_prepare_record_channel(L,job);
    native_events_prefix_full(L,job);native_record_commands_channel_physical(L);
    // Real75E5EB name, 75E5F8 ->54B3F0 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,10);lua_remove(L,-2);
    lua_pushcclosure(L,native_record_commands_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->record_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_record_commands_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine54B3F0 RecordCommandStreams closure changed after FULL");
    native_record_commands_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->record_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_record_commands_channel_physical(L);
    p.native_record_commands_generator_ref=f->generators_ref;p.native_record_commands_generator_table=f->generators;
    p.native_record_commands_constructor_closure=constructor;
    p.native_record_commands_epoch=f->epoch;p.native_record_commands_writes=f->writes;
    p.native_record_commands_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);lua_pop(L,1);
}

static void native_events_prepare_city_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->record_constructor_assigned||p.native_city_handle_retired||
       p.native_city_handle_keys_ref!=LUA_NOREF||p.native_city_handle_weak_ref!=LUA_NOREF||
       p.native_city_handle_generator_ref!=LUA_NOREF||!p.native_city_handle_subscribers.empty()||
       !p.native_city_handle_queue.empty())
        return (void)luaL_error(L,"CityHandleCreated channel requires genuine ordered seventh registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_city_handle_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_city_handle_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_city_handle_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_city_handle_keys[i])||std::memcmp(value,native_city_handle_keys[i],n))
            return (void)luaL_error(L,"CityHandleCreated genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,city_handle_mode);lua_rawgeti(L,keys,city_handle_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,city_handle_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,city_handle_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"CityHandleCreated genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_city_handle_weak_table=weak_pointer;p.native_city_handle_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_city_handle_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_city_handle_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_city_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->record_constructor_assigned||!f->generators||
       f->city_constructor_created||f->city_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 CityHandleCreated order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_events_prepare_city_channel(L,job);
    native_events_prefix_full(L,job);native_city_handle_channel_physical(L);
    // Real75E60B name, 75E618 ->5EE540 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,11);lua_remove(L,-2);
    lua_pushcclosure(L,native_city_handle_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->city_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_city_handle_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine5EE540 CityHandleCreated closure changed after FULL");
    native_city_handle_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->city_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_city_handle_channel_physical(L);
    p.native_city_handle_generator_ref=f->generators_ref;p.native_city_handle_generator_table=f->generators;
    p.native_city_handle_constructor_closure=constructor;
    p.native_city_handle_epoch=f->epoch;p.native_city_handle_writes=f->writes;
    p.native_city_handle_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);lua_pop(L,1);
}

static void native_events_prepare_unit_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->city_constructor_assigned||p.native_unit_handle_retired||
       p.native_unit_handle_keys_ref!=LUA_NOREF||p.native_unit_handle_weak_ref!=LUA_NOREF||
       p.native_unit_handle_generator_ref!=LUA_NOREF||!p.native_unit_handle_subscribers.empty()||
       !p.native_unit_handle_queue.empty())
        return (void)luaL_error(L,"UnitHandleCreated channel requires genuine ordered eighth registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_unit_handle_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_unit_handle_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_unit_handle_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_unit_handle_keys[i])||std::memcmp(value,native_unit_handle_keys[i],n))
            return (void)luaL_error(L,"UnitHandleCreated genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,unit_handle_mode);lua_rawgeti(L,keys,unit_handle_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,unit_handle_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,unit_handle_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"UnitHandleCreated genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_unit_handle_weak_table=weak_pointer;p.native_unit_handle_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_unit_handle_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_unit_handle_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_unit_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->city_constructor_assigned||!f->generators||
       f->unit_constructor_created||f->unit_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 UnitHandleCreated order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_events_prepare_unit_channel(L,job);
    native_events_prefix_full(L,job);native_unit_handle_channel_physical(L);
    // Real75E62E name, 75E63B ->6FC690 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,12);lua_remove(L,-2);
    lua_pushcclosure(L,native_unit_handle_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->unit_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_unit_handle_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine6FC690 UnitHandleCreated closure changed after FULL");
    native_unit_handle_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->unit_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_unit_handle_channel_physical(L);
    p.native_unit_handle_generator_ref=f->generators_ref;p.native_unit_handle_generator_table=f->generators;
    p.native_unit_handle_constructor_closure=constructor;
    p.native_unit_handle_epoch=f->epoch;p.native_unit_handle_writes=f->writes;
    p.native_unit_handle_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);lua_pop(L,1);
}

static void native_events_prepare_unit_type_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->unit_constructor_assigned||p.native_unit_type_retired||
       p.native_unit_type_keys_ref!=LUA_NOREF||p.native_unit_type_weak_ref!=LUA_NOREF||
       p.native_unit_type_generator_ref!=LUA_NOREF||!p.native_unit_type_subscribers.empty()||
       !p.native_unit_type_queue.empty())
        return (void)luaL_error(L,"UnitTypeChanged channel requires genuine ordered ninth registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_unit_type_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_unit_type_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_unit_type_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_unit_type_keys[i])||std::memcmp(value,native_unit_type_keys[i],n))
            return (void)luaL_error(L,"UnitTypeChanged genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,unit_type_mode);lua_rawgeti(L,keys,unit_type_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,unit_type_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,unit_type_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"UnitTypeChanged genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_unit_type_weak_table=weak_pointer;p.native_unit_type_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_unit_type_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_unit_type_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_unit_type_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->unit_constructor_assigned||!f->generators||
       f->unit_type_constructor_created||f->unit_type_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 UnitTypeChanged order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_events_prepare_unit_type_channel(L,job);
    native_events_prefix_full(L,job);native_unit_type_channel_physical(L);
    // Real75E64E name, 75E65B ->578400 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,13);lua_remove(L,-2);
    lua_pushcclosure(L,native_unit_type_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->unit_type_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_unit_type_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine578400 UnitTypeChanged closure changed after FULL");
    native_unit_type_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->unit_type_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_unit_type_channel_physical(L);
    p.native_unit_type_generator_ref=f->generators_ref;p.native_unit_type_generator_table=f->generators;
    p.native_unit_type_constructor_closure=constructor;
    p.native_unit_type_epoch=f->epoch;p.native_unit_type_writes=f->writes;
    p.native_unit_type_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);lua_pop(L,1);
}

static void native_events_prepare_wonder_type_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->unit_type_constructor_assigned||p.native_wonder_type_retired||
       p.native_wonder_type_keys_ref!=LUA_NOREF||p.native_wonder_type_weak_ref!=LUA_NOREF||
       p.native_wonder_type_generator_ref!=LUA_NOREF||!p.native_wonder_type_subscribers.empty()||
       !p.native_wonder_type_queue.empty())
        return (void)luaL_error(L,"WonderTypeChanged channel requires genuine ordered tenth registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_wonder_type_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_wonder_type_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_wonder_type_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_wonder_type_keys[i])||std::memcmp(value,native_wonder_type_keys[i],n))
            return (void)luaL_error(L,"WonderTypeChanged genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,wonder_type_mode);lua_rawgeti(L,keys,wonder_type_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,wonder_type_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,wonder_type_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"WonderTypeChanged genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_wonder_type_weak_table=weak_pointer;p.native_wonder_type_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_wonder_type_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_wonder_type_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_wonder_type_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->unit_type_constructor_assigned||!f->generators||
       f->wonder_type_constructor_created||f->wonder_type_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 WonderTypeChanged order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_events_prepare_wonder_type_channel(L,job);
    native_events_prefix_full(L,job);native_wonder_type_channel_physical(L);
    // Real75E66E name, 75E67E ->5A2390 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,14);lua_remove(L,-2);
    lua_pushcclosure(L,native_wonder_type_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->wonder_type_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_wonder_type_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine5A2390 WonderTypeChanged closure changed after FULL");
    native_wonder_type_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->wonder_type_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_wonder_type_channel_physical(L);
    p.native_wonder_type_generator_ref=f->generators_ref;p.native_wonder_type_generator_table=f->generators;
    p.native_wonder_type_constructor_closure=constructor;
    p.native_wonder_type_epoch=f->epoch;p.native_wonder_type_writes=f->writes;
    p.native_wonder_type_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);lua_pop(L,1);
}

static void native_events_prepare_wonder_state_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->wonder_type_constructor_assigned||p.native_wonder_state_retired||
       p.native_wonder_state_keys_ref!=LUA_NOREF||p.native_wonder_state_weak_ref!=LUA_NOREF||
       p.native_wonder_state_generator_ref!=LUA_NOREF||!p.native_wonder_state_subscribers.empty()||
       !p.native_wonder_state_queue.empty())
        return (void)luaL_error(L,"WonderStateChanged channel requires genuine ordered eleventh registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_wonder_state_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_wonder_state_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_wonder_state_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_wonder_state_keys[i])||std::memcmp(value,native_wonder_state_keys[i],n))
            return (void)luaL_error(L,"WonderStateChanged genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,wonder_state_mode);lua_rawgeti(L,keys,wonder_state_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,wonder_state_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,wonder_state_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"WonderStateChanged genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_wonder_state_weak_table=weak_pointer;p.native_wonder_state_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_wonder_state_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_wonder_state_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_wonder_state_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->wonder_type_constructor_assigned||!f->generators||
       f->wonder_state_constructor_created||f->wonder_state_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 WonderStateChanged order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_events_prepare_wonder_state_channel(L,job);
    native_events_prefix_full(L,job);native_wonder_state_channel_physical(L);
    // Real75E691 name, 75E69E ->7187E0 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,15);lua_remove(L,-2);
    lua_pushcclosure(L,native_wonder_state_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->wonder_state_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_wonder_state_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine7187E0 WonderStateChanged closure changed after FULL");
    native_wonder_state_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->wonder_state_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_wonder_state_channel_physical(L);
    p.native_wonder_state_generator_ref=f->generators_ref;p.native_wonder_state_generator_table=f->generators;
    p.native_wonder_state_constructor_closure=constructor;
    p.native_wonder_state_epoch=f->epoch;p.native_wonder_state_writes=f->writes;
    p.native_wonder_state_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);lua_pop(L,1);
}

static void native_events_prepare_wonder_placement_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->wonder_state_constructor_assigned||p.native_wonder_placement_retired||
       p.native_wonder_placement_keys_ref!=LUA_NOREF||p.native_wonder_placement_weak_ref!=LUA_NOREF||
       p.native_wonder_placement_generator_ref!=LUA_NOREF||!p.native_wonder_placement_subscribers.empty()||
       !p.native_wonder_placement_queue.empty())
        return (void)luaL_error(L,"WonderTogglePlacement channel requires genuine ordered twelfth registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_wonder_placement_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_wonder_placement_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_wonder_placement_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_wonder_placement_keys[i])||std::memcmp(value,native_wonder_placement_keys[i],n))
            return (void)luaL_error(L,"WonderTogglePlacement genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,wonder_placement_mode);lua_rawgeti(L,keys,wonder_placement_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,wonder_placement_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,wonder_placement_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"WonderTogglePlacement genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_wonder_placement_weak_table=weak_pointer;p.native_wonder_placement_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_wonder_placement_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_wonder_placement_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_wonder_placement_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->wonder_state_constructor_assigned||!f->generators||
       f->wonder_placement_constructor_created||f->wonder_placement_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 WonderTogglePlacement order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_events_prepare_wonder_placement_channel(L,job);
    native_events_prefix_full(L,job);native_wonder_placement_channel_physical(L);
    // Real75E6B1 name, 75E6BE ->7173A0 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,16);lua_remove(L,-2);
    lua_pushcclosure(L,native_wonder_placement_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->wonder_placement_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_wonder_placement_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine7173A0 WonderTogglePlacement closure changed after FULL");
    native_wonder_placement_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->wonder_placement_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_wonder_placement_channel_physical(L);
    p.native_wonder_placement_generator_ref=f->generators_ref;p.native_wonder_placement_generator_table=f->generators;
    p.native_wonder_placement_constructor_closure=constructor;
    p.native_wonder_placement_epoch=f->epoch;p.native_wonder_placement_writes=f->writes;
    p.native_wonder_placement_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);lua_pop(L,1);
}

static void native_events_prepare_display_units_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->wonder_placement_constructor_assigned||p.native_display_units_retired||
       p.native_display_units_keys_ref!=LUA_NOREF||p.native_display_units_weak_ref!=LUA_NOREF||
       p.native_display_units_generator_ref!=LUA_NOREF||!p.native_display_units_subscribers.empty()||
       !p.native_display_units_queue.empty())
        return (void)luaL_error(L,"ToggleDisplayUnits channel requires genuine ordered thirteenth registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_display_units_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_display_units_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_display_units_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_display_units_keys[i])||std::memcmp(value,native_display_units_keys[i],n))
            return (void)luaL_error(L,"ToggleDisplayUnits genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,display_units_mode);lua_rawgeti(L,keys,display_units_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,display_units_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,display_units_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"ToggleDisplayUnits genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_display_units_weak_table=weak_pointer;p.native_display_units_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_display_units_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_display_units_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_display_units_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->wonder_placement_constructor_assigned||!f->generators||
       f->display_units_constructor_created||f->display_units_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 ToggleDisplayUnits order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_events_prepare_display_units_channel(L,job);
    native_events_prefix_full(L,job);native_display_units_channel_physical(L);
    // Real75E6D4 name, 75E6E1 ->721560 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,17);lua_remove(L,-2);
    lua_pushcclosure(L,native_display_units_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->display_units_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_display_units_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine721560 ToggleDisplayUnits closure changed after FULL");
    native_display_units_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->display_units_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_display_units_channel_physical(L);
    p.native_display_units_generator_ref=f->generators_ref;p.native_display_units_generator_table=f->generators;
    p.native_display_units_constructor_closure=constructor;
    p.native_display_units_epoch=f->epoch;p.native_display_units_writes=f->writes;
    p.native_display_units_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);lua_pop(L,1);
}

static void native_events_prepare_unit_scale_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->display_units_constructor_assigned||p.native_unit_scale_retired||
       p.native_unit_scale_keys_ref!=LUA_NOREF||p.native_unit_scale_weak_ref!=LUA_NOREF||
       p.native_unit_scale_generator_ref!=LUA_NOREF||!p.native_unit_scale_subscribers.empty()||
       !p.native_unit_scale_queue.empty())
        return (void)luaL_error(L,"GlobalUnitScale channel requires genuine ordered fourteenth registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_unit_scale_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_unit_scale_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_unit_scale_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_unit_scale_keys[i])||std::memcmp(value,native_unit_scale_keys[i],n))
            return (void)luaL_error(L,"GlobalUnitScale genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,unit_scale_mode);lua_rawgeti(L,keys,unit_scale_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,unit_scale_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,unit_scale_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"GlobalUnitScale genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_unit_scale_weak_table=weak_pointer;p.native_unit_scale_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_unit_scale_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_unit_scale_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_unit_scale_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->display_units_constructor_assigned||!f->generators||
       f->unit_scale_constructor_created||f->unit_scale_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 GlobalUnitScale order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_events_prepare_unit_scale_channel(L,job);
    native_events_prefix_full(L,job);native_unit_scale_channel_physical(L);
    // Real75E6F4 name, 75E701 ->686920 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,18);lua_remove(L,-2);
    lua_pushcclosure(L,native_unit_scale_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->unit_scale_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_unit_scale_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine686920 GlobalUnitScale closure changed after FULL");
    native_unit_scale_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->unit_scale_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_unit_scale_channel_physical(L);
    p.native_unit_scale_generator_ref=f->generators_ref;p.native_unit_scale_generator_table=f->generators;
    p.native_unit_scale_constructor_closure=constructor;
    p.native_unit_scale_epoch=f->epoch;p.native_unit_scale_writes=f->writes;
    p.native_unit_scale_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);lua_pop(L,1);
}

static void native_events_prepare_landmark_swap_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->unit_scale_constructor_assigned||p.native_landmark_swap_retired||
       p.native_landmark_swap_keys_ref!=LUA_NOREF||p.native_landmark_swap_weak_ref!=LUA_NOREF||
       p.native_landmark_swap_generator_ref!=LUA_NOREF||!p.native_landmark_swap_subscribers.empty()||
       !p.native_landmark_swap_queue.empty())
        return (void)luaL_error(L,"LandmarkLibrarySwap channel requires genuine ordered fifteenth registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_landmark_swap_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_landmark_swap_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_landmark_swap_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_landmark_swap_keys[i])||std::memcmp(value,native_landmark_swap_keys[i],n))
            return (void)luaL_error(L,"LandmarkLibrarySwap genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,landmark_swap_mode);lua_rawgeti(L,keys,landmark_swap_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,landmark_swap_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,landmark_swap_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"LandmarkLibrarySwap genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_landmark_swap_weak_table=weak_pointer;p.native_landmark_swap_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_landmark_swap_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_landmark_swap_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_landmark_swap_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->unit_scale_constructor_assigned||!f->generators||
       f->landmark_swap_constructor_created||f->landmark_swap_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 LandmarkLibrarySwap order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_events_prepare_landmark_swap_channel(L,job);
    native_events_prefix_full(L,job);native_landmark_swap_channel_physical(L);
    // Real75E717 name, 75E724 ->6D6BF0 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,19);lua_remove(L,-2);
    lua_pushcclosure(L,native_landmark_swap_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->landmark_swap_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_landmark_swap_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine6D6BF0 LandmarkLibrarySwap closure changed after FULL");
    native_landmark_swap_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->landmark_swap_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_landmark_swap_channel_physical(L);
    p.native_landmark_swap_generator_ref=f->generators_ref;p.native_landmark_swap_generator_table=f->generators;
    p.native_landmark_swap_constructor_closure=constructor;
    p.native_landmark_swap_epoch=f->epoch;p.native_landmark_swap_writes=f->writes;
    p.native_landmark_swap_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);lua_pop(L,1);
}

static void native_events_prepare_unit_swap_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->landmark_swap_constructor_assigned||p.native_unit_swap_retired||
       p.native_unit_swap_keys_ref!=LUA_NOREF||p.native_unit_swap_weak_ref!=LUA_NOREF||
       p.native_unit_swap_generator_ref!=LUA_NOREF||!p.native_unit_swap_subscribers.empty()||
       !p.native_unit_swap_queue.empty())
        return (void)luaL_error(L,"UnitLibrarySwap channel requires genuine ordered sixteenth registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_unit_swap_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_unit_swap_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_unit_swap_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_unit_swap_keys[i])||std::memcmp(value,native_unit_swap_keys[i],n))
            return (void)luaL_error(L,"UnitLibrarySwap genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,unit_swap_mode);lua_rawgeti(L,keys,unit_swap_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,unit_swap_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,unit_swap_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"UnitLibrarySwap genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_unit_swap_weak_table=weak_pointer;p.native_unit_swap_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_unit_swap_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_unit_swap_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_unit_swap_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->landmark_swap_constructor_assigned||!f->generators||
       f->unit_swap_constructor_created||f->unit_swap_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 UnitLibrarySwap order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_events_prepare_unit_swap_channel(L,job);
    native_events_prefix_full(L,job);native_unit_swap_channel_physical(L);
    // Real75E737 name, 75E744 ->693E90 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,20);lua_remove(L,-2);
    lua_pushcclosure(L,native_unit_swap_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->unit_swap_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_unit_swap_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine693E90 UnitLibrarySwap closure changed after FULL");
    native_unit_swap_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->unit_swap_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_unit_swap_channel_physical(L);
    p.native_unit_swap_generator_ref=f->generators_ref;p.native_unit_swap_generator_table=f->generators;
    p.native_unit_swap_constructor_closure=constructor;
    p.native_unit_swap_epoch=f->epoch;p.native_unit_swap_writes=f->writes;
    p.native_unit_swap_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);lua_pop(L,1);
}

static void native_events_prepare_building_swap_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->unit_swap_constructor_assigned||p.native_building_swap_retired||
       p.native_building_swap_keys_ref!=LUA_NOREF||p.native_building_swap_weak_ref!=LUA_NOREF||
       p.native_building_swap_generator_ref!=LUA_NOREF||!p.native_building_swap_subscribers.empty()||
       !p.native_building_swap_queue.empty())
        return (void)luaL_error(L,"BuildingLibrarySwap channel requires genuine ordered seventeenth registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_building_swap_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_building_swap_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_building_swap_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_building_swap_keys[i])||std::memcmp(value,native_building_swap_keys[i],n))
            return (void)luaL_error(L,"BuildingLibrarySwap genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,building_swap_mode);lua_rawgeti(L,keys,building_swap_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,building_swap_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,building_swap_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"BuildingLibrarySwap genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_building_swap_weak_table=weak_pointer;p.native_building_swap_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_building_swap_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_building_swap_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_building_swap_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->unit_swap_constructor_assigned||!f->generators||
       f->building_swap_constructor_created||f->building_swap_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 BuildingLibrarySwap order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_events_prepare_building_swap_channel(L,job);
    native_events_prefix_full(L,job);native_building_swap_channel_physical(L);
    // Real75E757 name, 75E767 ->50E3F0 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,21);lua_remove(L,-2);
    lua_pushcclosure(L,native_building_swap_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->building_swap_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_building_swap_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine50E3F0 BuildingLibrarySwap closure changed after FULL");
    native_building_swap_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->building_swap_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_building_swap_channel_physical(L);
    p.native_building_swap_generator_ref=f->generators_ref;p.native_building_swap_generator_table=f->generators;
    p.native_building_swap_constructor_closure=constructor;
    p.native_building_swap_epoch=f->epoch;p.native_building_swap_writes=f->writes;
    p.native_building_swap_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_building_swap_current(L);lua_pop(L,1);
}

static void native_events_prepare_sequence_init_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->building_swap_constructor_assigned||p.native_sequence_init_retired||
       p.native_sequence_init_keys_ref!=LUA_NOREF||p.native_sequence_init_weak_ref!=LUA_NOREF||
       p.native_sequence_init_generator_ref!=LUA_NOREF||!p.native_sequence_init_subscribers.empty()||
       !p.native_sequence_init_queue.empty())
        return (void)luaL_error(L,"SequenceGameInitComplete channel requires genuine ordered eighteenth registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_sequence_init_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_sequence_init_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_sequence_init_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_sequence_init_keys[i])||std::memcmp(value,native_sequence_init_keys[i],n))
            return (void)luaL_error(L,"SequenceGameInitComplete genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,sequence_init_mode);lua_rawgeti(L,keys,sequence_init_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,sequence_init_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,sequence_init_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"SequenceGameInitComplete genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_sequence_init_weak_table=weak_pointer;p.native_sequence_init_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_sequence_init_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_sequence_init_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_sequence_init_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->building_swap_constructor_assigned||!f->generators||
       f->sequence_init_constructor_created||f->sequence_init_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 SequenceGameInitComplete order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_building_swap_current(L);native_events_prepare_sequence_init_channel(L,job);
    native_events_prefix_full(L,job);native_sequence_init_channel_physical(L);
    // Real75E77A name, 75E787 ->6A0930 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,22);lua_remove(L,-2);
    lua_pushcclosure(L,native_sequence_init_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->sequence_init_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_sequence_init_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine6A0930 SequenceGameInitComplete closure changed after FULL");
    native_sequence_init_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->sequence_init_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_sequence_init_channel_physical(L);
    p.native_sequence_init_generator_ref=f->generators_ref;p.native_sequence_init_generator_table=f->generators;
    p.native_sequence_init_constructor_closure=constructor;
    p.native_sequence_init_epoch=f->epoch;p.native_sequence_init_writes=f->writes;
    p.native_sequence_init_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_building_swap_current(L);native_sequence_init_current(L);lua_pop(L,1);
}

static void native_events_prepare_state_dump_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->sequence_init_constructor_assigned||p.native_state_dump_retired||
       p.native_state_dump_keys_ref!=LUA_NOREF||p.native_state_dump_weak_ref!=LUA_NOREF||
       p.native_state_dump_generator_ref!=LUA_NOREF||!p.native_state_dump_subscribers.empty()||
       !p.native_state_dump_queue.empty())
        return (void)luaL_error(L,"StateMachineDumpStates channel requires genuine ordered nineteenth registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_state_dump_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_state_dump_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_state_dump_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_state_dump_keys[i])||std::memcmp(value,native_state_dump_keys[i],n))
            return (void)luaL_error(L,"StateMachineDumpStates genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,state_dump_mode);lua_rawgeti(L,keys,state_dump_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,state_dump_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,state_dump_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"StateMachineDumpStates genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_state_dump_weak_table=weak_pointer;p.native_state_dump_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_state_dump_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_state_dump_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_state_dump_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->sequence_init_constructor_assigned||!f->generators||
       f->state_dump_constructor_created||f->state_dump_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 StateMachineDumpStates order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_building_swap_current(L);native_sequence_init_current(L);native_events_prepare_state_dump_channel(L,job);
    native_events_prefix_full(L,job);native_state_dump_channel_physical(L);
    // Real75E79A name, 75E7A7 ->6A8590 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,23);lua_remove(L,-2);
    lua_pushcclosure(L,native_state_dump_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->state_dump_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_state_dump_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine6A8590 StateMachineDumpStates closure changed after FULL");
    native_state_dump_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->state_dump_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_state_dump_channel_physical(L);
    p.native_state_dump_generator_ref=f->generators_ref;p.native_state_dump_generator_table=f->generators;
    p.native_state_dump_constructor_closure=constructor;
    p.native_state_dump_epoch=f->epoch;p.native_state_dump_writes=f->writes;
    p.native_state_dump_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_building_swap_current(L);native_sequence_init_current(L);native_state_dump_current(L);lua_pop(L,1);
}

static void native_events_prepare_state_request_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->state_dump_constructor_assigned||p.native_state_request_retired||
       p.native_state_request_keys_ref!=LUA_NOREF||p.native_state_request_weak_ref!=LUA_NOREF||
       p.native_state_request_generator_ref!=LUA_NOREF||!p.native_state_request_subscribers.empty()||
       !p.native_state_request_queue.empty())
        return (void)luaL_error(L,"StateMachineRequestStates channel requires genuine ordered twentieth registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_state_request_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_state_request_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_state_request_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_state_request_keys[i])||std::memcmp(value,native_state_request_keys[i],n))
            return (void)luaL_error(L,"StateMachineRequestStates genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,state_request_mode);lua_rawgeti(L,keys,state_request_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,state_request_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,state_request_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"StateMachineRequestStates genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_state_request_weak_table=weak_pointer;p.native_state_request_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_state_request_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_state_request_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_state_request_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->state_dump_constructor_assigned||!f->generators||
       f->state_request_constructor_created||f->state_request_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 StateMachineRequestStates order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_building_swap_current(L);native_sequence_init_current(L);native_state_dump_current(L);native_events_prepare_state_request_channel(L,job);
    native_events_prefix_full(L,job);native_state_request_channel_physical(L);
    // Real75E7BD name, 75E7CA ->545DD0 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,24);lua_remove(L,-2);
    lua_pushcclosure(L,native_state_request_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->state_request_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_state_request_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine545DD0 StateMachineRequestStates closure changed after FULL");
    native_state_request_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->state_request_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_state_request_channel_physical(L);
    p.native_state_request_generator_ref=f->generators_ref;p.native_state_request_generator_table=f->generators;
    p.native_state_request_constructor_closure=constructor;
    p.native_state_request_epoch=f->epoch;p.native_state_request_writes=f->writes;
    p.native_state_request_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_building_swap_current(L);native_sequence_init_current(L);native_state_dump_current(L);native_state_request_current(L);lua_pop(L,1);
}

static void native_events_prepare_strategic_view_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->state_request_constructor_assigned||p.native_strategic_view_retired||
       p.native_strategic_view_keys_ref!=LUA_NOREF||p.native_strategic_view_weak_ref!=LUA_NOREF||
       p.native_strategic_view_generator_ref!=LUA_NOREF||!p.native_strategic_view_subscribers.empty()||
       !p.native_strategic_view_queue.empty())
        return (void)luaL_error(L,"StrategicViewStateChanged channel requires genuine ordered twenty-first registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_strategic_view_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_strategic_view_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_strategic_view_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_strategic_view_keys[i])||std::memcmp(value,native_strategic_view_keys[i],n))
            return (void)luaL_error(L,"StrategicViewStateChanged genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,strategic_view_mode);lua_rawgeti(L,keys,strategic_view_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,strategic_view_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,strategic_view_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"StrategicViewStateChanged genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_strategic_view_weak_table=weak_pointer;p.native_strategic_view_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_strategic_view_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_strategic_view_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_strategic_view_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->state_request_constructor_assigned||!f->generators||
       f->strategic_view_constructor_created||f->strategic_view_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 StrategicViewStateChanged order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_building_swap_current(L);native_sequence_init_current(L);native_state_dump_current(L);native_state_request_current(L);native_events_prepare_strategic_view_channel(L,job);
    native_events_prefix_full(L,job);native_strategic_view_channel_physical(L);
    // Real75E7DD name, 75E7EA ->6D8240 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,25);lua_remove(L,-2);
    lua_pushcclosure(L,native_strategic_view_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->strategic_view_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_strategic_view_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine6D8240 StrategicViewStateChanged closure changed after FULL");
    native_strategic_view_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->strategic_view_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_strategic_view_channel_physical(L);
    p.native_strategic_view_generator_ref=f->generators_ref;p.native_strategic_view_generator_table=f->generators;
    p.native_strategic_view_constructor_closure=constructor;
    p.native_strategic_view_epoch=f->epoch;p.native_strategic_view_writes=f->writes;
    p.native_strategic_view_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_building_swap_current(L);native_sequence_init_current(L);native_state_dump_current(L);native_state_request_current(L);native_strategic_view_current(L);lua_pop(L,1);
}

static void native_events_prepare_ai_started_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->strategic_view_constructor_assigned||p.native_ai_started_retired||
       p.native_ai_started_keys_ref!=LUA_NOREF||p.native_ai_started_weak_ref!=LUA_NOREF||
       p.native_ai_started_generator_ref!=LUA_NOREF||!p.native_ai_started_subscribers.empty()||
       !p.native_ai_started_queue.empty())
        return (void)luaL_error(L,"AIProcessingStartedForPlayer channel requires genuine ordered twenty-second registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_ai_started_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_ai_started_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_ai_started_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_ai_started_keys[i])||std::memcmp(value,native_ai_started_keys[i],n))
            return (void)luaL_error(L,"AIProcessingStartedForPlayer genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,ai_started_mode);lua_rawgeti(L,keys,ai_started_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,ai_started_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,ai_started_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"AIProcessingStartedForPlayer genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_ai_started_weak_table=weak_pointer;p.native_ai_started_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_ai_started_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_ai_started_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_ai_started_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->strategic_view_constructor_assigned||!f->generators||
       f->ai_started_constructor_created||f->ai_started_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 AIProcessingStartedForPlayer order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_building_swap_current(L);native_sequence_init_current(L);native_state_dump_current(L);native_state_request_current(L);native_strategic_view_current(L);native_events_prepare_ai_started_channel(L,job);
    native_events_prefix_full(L,job);native_ai_started_channel_physical(L);
    // Real75E800 name, 75E80D ->5D9C60 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,26);lua_remove(L,-2);
    lua_pushcclosure(L,native_ai_started_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->ai_started_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_ai_started_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine5D9C60 AIProcessingStartedForPlayer closure changed after FULL");
    native_ai_started_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->ai_started_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_ai_started_channel_physical(L);
    p.native_ai_started_generator_ref=f->generators_ref;p.native_ai_started_generator_table=f->generators;
    p.native_ai_started_constructor_closure=constructor;
    p.native_ai_started_epoch=f->epoch;p.native_ai_started_writes=f->writes;
    p.native_ai_started_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_building_swap_current(L);native_sequence_init_current(L);native_state_dump_current(L);native_state_request_current(L);native_strategic_view_current(L);native_ai_started_current(L);lua_pop(L,1);
}

static void native_events_prepare_ai_ended_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->ai_started_constructor_assigned||p.native_ai_ended_retired||
       p.native_ai_ended_keys_ref!=LUA_NOREF||p.native_ai_ended_weak_ref!=LUA_NOREF||
       p.native_ai_ended_generator_ref!=LUA_NOREF||!p.native_ai_ended_subscribers.empty()||
       !p.native_ai_ended_queue.empty())
        return (void)luaL_error(L,"AIProcessingEndedForPlayer channel requires genuine ordered twenty-third registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_ai_ended_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_ai_ended_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_ai_ended_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_ai_ended_keys[i])||std::memcmp(value,native_ai_ended_keys[i],n))
            return (void)luaL_error(L,"AIProcessingEndedForPlayer genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,ai_ended_mode);lua_rawgeti(L,keys,ai_ended_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,ai_ended_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,ai_ended_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"AIProcessingEndedForPlayer genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_ai_ended_weak_table=weak_pointer;p.native_ai_ended_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_ai_ended_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_ai_ended_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_ai_ended_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->ai_started_constructor_assigned||!f->generators||
       f->ai_ended_constructor_created||f->ai_ended_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 AIProcessingEndedForPlayer order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_building_swap_current(L);native_sequence_init_current(L);native_state_dump_current(L);native_state_request_current(L);native_strategic_view_current(L);native_ai_started_current(L);native_events_prepare_ai_ended_channel(L,job);
    native_events_prefix_full(L,job);native_ai_ended_channel_physical(L);
    // Real75E820 name, 75E82D ->548570 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,27);lua_remove(L,-2);
    lua_pushcclosure(L,native_ai_ended_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->ai_ended_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_ai_ended_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine548570 AIProcessingEndedForPlayer closure changed after FULL");
    native_ai_ended_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->ai_ended_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_ai_ended_channel_physical(L);
    p.native_ai_ended_generator_ref=f->generators_ref;p.native_ai_ended_generator_table=f->generators;
    p.native_ai_ended_constructor_closure=constructor;
    p.native_ai_ended_epoch=f->epoch;p.native_ai_ended_writes=f->writes;
    p.native_ai_ended_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_building_swap_current(L);native_sequence_init_current(L);native_state_dump_current(L);native_state_request_current(L);native_strategic_view_current(L);native_ai_started_current(L);native_ai_ended_current(L);lua_pop(L,1);
}

static void native_events_prepare_ai_step_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->ai_ended_constructor_assigned||p.native_ai_step_retired||
       p.native_ai_step_keys_ref!=LUA_NOREF||p.native_ai_step_weak_ref!=LUA_NOREF||
       p.native_ai_step_generator_ref!=LUA_NOREF||!p.native_ai_step_subscribers.empty()||
       !p.native_ai_step_queue.empty())
        return (void)luaL_error(L,"AIProcessingStepChanged channel requires genuine ordered twenty-fourth registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_ai_step_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_ai_step_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_ai_step_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_ai_step_keys[i])||std::memcmp(value,native_ai_step_keys[i],n))
            return (void)luaL_error(L,"AIProcessingStepChanged genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,ai_step_mode);lua_rawgeti(L,keys,ai_step_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,ai_step_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,ai_step_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"AIProcessingStepChanged genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_ai_step_weak_table=weak_pointer;p.native_ai_step_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_ai_step_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_ai_step_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_ai_step_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->ai_ended_constructor_assigned||!f->generators||
       f->ai_step_constructor_created||f->ai_step_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 AIProcessingStepChanged order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_building_swap_current(L);native_sequence_init_current(L);native_state_dump_current(L);native_state_request_current(L);native_strategic_view_current(L);native_ai_started_current(L);native_ai_ended_current(L);native_events_prepare_ai_step_channel(L,job);
    native_events_prefix_full(L,job);native_ai_step_channel_physical(L);
    // Real75E840 name, 75E850 ->5C4EE0 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,28);lua_remove(L,-2);
    lua_pushcclosure(L,native_ai_step_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->ai_step_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_ai_step_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine5C4EE0 AIProcessingStepChanged closure changed after FULL");
    native_ai_step_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->ai_step_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_ai_step_channel_physical(L);
    p.native_ai_step_generator_ref=f->generators_ref;p.native_ai_step_generator_table=f->generators;
    p.native_ai_step_constructor_closure=constructor;
    p.native_ai_step_epoch=f->epoch;p.native_ai_step_writes=f->writes;
    p.native_ai_step_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_building_swap_current(L);native_sequence_init_current(L);native_state_dump_current(L);native_state_request_current(L);native_strategic_view_current(L);native_ai_started_current(L);native_ai_ended_current(L);native_ai_step_current(L);lua_pop(L,1);
}

static void native_events_prepare_game_view_channel(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->ai_step_constructor_assigned||p.native_game_view_retired||
       p.native_game_view_keys_ref!=LUA_NOREF||p.native_game_view_weak_ref!=LUA_NOREF||
       p.native_game_view_generator_ref!=LUA_NOREF||!p.native_game_view_subscribers.empty()||
       !p.native_game_view_queue.empty())
        return (void)luaL_error(L,"GameViewTypeChanged channel requires genuine ordered twenty-fifth registrar");
    lua_createtable(L,18,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_pushstring(L,native_game_view_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    p.native_game_view_keys_table=keys_pointer;lua_pushvalue(L,keys);
    p.native_game_view_keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<18;++i){
        lua_rawgeti(L,keys,i+1);std::size_t n=0;
        const char* value=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&n):nullptr;
        if(!value||n!=std::strlen(native_game_view_keys[i])||std::memcmp(value,native_game_view_keys[i],n))
            return (void)luaL_error(L,"GameViewTypeChanged genuine prepared key changed");
        lua_pop(L,1);
    }
    lua_createtable(L,0,0);const int weak=lua_gettop(L);const void*const weak_pointer=lua_topointer(L,weak);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,weak,weak_pointer);
    lua_createtable(L,0,1);const int meta=lua_gettop(L);const void*const meta_pointer=lua_topointer(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,game_view_mode);lua_rawgeti(L,keys,game_view_weak);lua_rawset(L,meta);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,meta,meta_pointer);
    lua_rawgeti(L,keys,game_view_mode);lua_rawget(L,meta);lua_rawgeti(L,keys,game_view_weak);
    if(!lua_rawequal(L,-1,-2))return (void)luaL_error(L,"GameViewTypeChanged genuine watcher mode changed");
    lua_pop(L,2);lua_setmetatable(L,weak);
    p.native_game_view_weak_table=weak_pointer;p.native_game_view_weak_meta=meta_pointer;
    lua_pushvalue(L,weak);p.native_game_view_weak_ref=luaL_ref(L,LUA_REGISTRYINDEX);
    native_events_prefix_full(L,job);native_game_view_channel_physical(L);lua_pop(L,2);
}
static void native_events_register_game_view_generator(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;native_events_prefix_full(L,job);
    if(!f||!f->ai_step_constructor_assigned||!f->generators||
       f->game_view_constructor_created||f->game_view_constructor_assigned)
        return (void)luaL_error(L,"Events actual75E540 GameViewTypeChanged order/replay refused");
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_building_swap_current(L);native_sequence_init_current(L);native_state_dump_current(L);native_state_request_current(L);native_strategic_view_current(L);native_ai_started_current(L);native_ai_ended_current(L);native_ai_step_current(L);native_events_prepare_game_view_channel(L,job);
    native_events_prefix_full(L,job);native_game_view_channel_physical(L);
    // Real75E863 name, 75E870 ->6FBEE0 zero-upvalue constructor.
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->keys_ref);lua_rawgeti(L,-1,29);lua_remove(L,-2);
    lua_pushcclosure(L,native_game_view_constructor,0);const void*const constructor=lua_topointer(L,-1);
    f->game_view_constructor_created=constructor;native_events_prefix_full(L,job);
    if(!lua_iscfunction(L,-1)||lua_topointer(L,-1)!=constructor||
       lua_tocfunction(L,-1)!=native_game_view_constructor||lua_getupvalue(L,-1,1))
        return (void)luaL_error(L,"Events genuine6FBEE0 GameViewTypeChanged closure changed after FULL");
    native_game_view_channel_physical(L);
    lua_rawgeti(L,LUA_REGISTRYINDEX,f->generators_ref);lua_insert(L,-3);
    lua_rawset(L,-3);f->game_view_constructor_assigned=constructor;
    native_events_prefix_full(L,job);native_game_view_channel_physical(L);
    p.native_game_view_generator_ref=f->generators_ref;p.native_game_view_generator_table=f->generators;
    p.native_game_view_constructor_closure=constructor;
    p.native_game_view_epoch=f->epoch;p.native_game_view_writes=f->writes;
    p.native_game_view_uncertainty=f->uncertainty;
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_building_swap_current(L);native_sequence_init_current(L);native_state_dump_current(L);native_state_request_current(L);native_strategic_view_current(L);native_ai_started_current(L);native_ai_ended_current(L);native_ai_step_current(L);native_game_view_current(L);lua_pop(L,1);
}

#include "lua_native_context_events_ordered_registration.inc"
#include "lua_native_context_events_lua_event_registration.inc"
static void perform_native_context_events_factory_prefix(lua_State* L,Job& job){
    auto& p=owner(L);auto*const f=p.events_prefix_frame;
    if(job.operation!=Operation::native_context_events_factory_prefix||!f)
        return (void)luaL_error(L,"Events actual protected registrar operation absent");
    if(!lua_checkstack(L,64))return (void)luaL_error(L,"Events actual registrar stack reserve failed");
    native_events_prefix_full(L,job);
    // Retain thirty real TString keys before further allocations/callbacks.
    // This port keeper is private; it does not publish Events or a setup loan.
    lua_createtable(L,30,0);const int keys=lua_gettop(L);const void*const keys_pointer=lua_topointer(L,keys);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,keys,keys_pointer);
    for(int i=0;i<30;++i){
        lua_pushstring(L,native_events_prefix_keys[i]);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
        lua_rawseti(L,keys,i+1);native_events_prefix_full(L,job);
        native_events_stack_table_physical(L,keys,keys_pointer);
    }
    f->keys=keys_pointer;lua_pushvalue(L,keys);
    f->keys_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_events_prefix_full(L,job);
    // Real573AC0 first obtains GLOBALS.EventFactories; a previously published
    // table needs its own genuine owner. A capability namespace is not one.
    lua_rawgeti(L,keys,1);lua_gettable(L,LUA_GLOBALSINDEX);native_events_prefix_full(L,job);
    if(!lua_isnil(L,-1))return (void)luaL_error(L,"Events existing master EventFactories has no genuine retained native owner");
    lua_pop(L,1);lua_createtable(L,0,0);
    const int master=lua_gettop(L);const void*const master_pointer=lua_topointer(L,master);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,master,master_pointer);
    f->master=master_pointer;
    lua_rawgeti(L,keys,1);lua_pushvalue(L,master);lua_settable(L,LUA_GLOBALSINDEX);
    native_events_prefix_full(L,job);
    lua_pushvalue(L,master);f->master_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_events_prefix_full(L,job);
    // Mask is the actual4573AA immediate, not an inferred UI capability set.
    lua_rawgeti(L,master,0xFFFFFF);native_events_prefix_full(L,job);
    if(!lua_isnil(L,-1))return (void)luaL_error(L,"Events maskFFFFFF already acquired an unowned factory");
    lua_pop(L,1);lua_createtable(L,0,0);
    const int factory=lua_gettop(L);const void*const factory_pointer=lua_topointer(L,factory);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,factory,factory_pointer);
    f->factory=factory_pointer;
    lua_pushvalue(L,factory);f->factory_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_events_prefix_full(L,job);
    lua_rawgeti(L,keys,2);lua_createtable(L,0,0);
    const int generators=lua_gettop(L);const void*const generators_pointer=lua_topointer(L,generators);
    native_events_prefix_full(L,job);native_events_stack_table_physical(L,generators,generators_pointer);
    f->generators=generators_pointer;
    lua_pushvalue(L,generators);f->generators_ref=luaL_ref(L,LUA_REGISTRYINDEX);native_events_prefix_full(L,job);
    // Actual75E540 first installs the real AppInitComplete constructor.
    // The lazy event itself is not constructed/emitted during registration.
    native_events_register_app_init_generator(L,job);native_events_prefix_full(L,job);
    native_app_init_current(L);
    // Actual next75E572 registers real493CB0 CameraViewChanged, including
    // its distinct list/channel and authentic4x4 Lua matrix marshalling.
    native_events_register_camera_view_generator(L,job);native_events_prefix_full(L,job);
    native_app_init_current(L);native_camera_view_current(L);
    // Actual third75E595 ->42F3F0 CameraProjectionChanged is now bound to
    // its distinct real list/channel and proven raw4x4 matrix marshaller.
    native_events_register_camera_projection_generator(L,job);native_events_prefix_full(L,job);
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);
    // Actual next three75E540 constructors are bound in their original order.
    native_events_register_animation_generator(L,job);native_events_prefix_full(L,job);
    native_events_register_dont_record_generator(L,job);native_events_prefix_full(L,job);
    native_events_register_record_generator(L,job);native_events_prefix_full(L,job);
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);
    // Actual original75E540 city and unit handle factories, with distinct field order.
    native_events_register_city_generator(L,job);native_events_prefix_full(L,job);
    native_events_register_unit_generator(L,job);native_events_prefix_full(L,job);
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);
    // Actual ninth/tenth string factories; undefined native tail is a real Call STOP.
    native_events_register_unit_type_generator(L,job);native_events_prefix_full(L,job);
    native_events_register_wonder_type_generator(L,job);native_events_prefix_full(L,job);
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);
    // Actual next eight75E540 world/init factories in their original order.
    native_events_register_wonder_state_generator(L,job);native_events_prefix_full(L,job);
    native_events_register_wonder_placement_generator(L,job);native_events_prefix_full(L,job);
    native_events_register_display_units_generator(L,job);native_events_prefix_full(L,job);
    native_events_register_unit_scale_generator(L,job);native_events_prefix_full(L,job);
    native_events_register_landmark_swap_generator(L,job);native_events_prefix_full(L,job);
    native_events_register_unit_swap_generator(L,job);native_events_prefix_full(L,job);
    native_events_register_building_swap_generator(L,job);native_events_prefix_full(L,job);
    native_events_register_sequence_init_generator(L,job);native_events_prefix_full(L,job);
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_building_swap_current(L);native_sequence_init_current(L);
    // The actual final seven75E540 factories complete this original registrar.
    native_events_register_state_dump_generator(L,job);native_events_prefix_full(L,job);
    native_events_register_state_request_generator(L,job);native_events_prefix_full(L,job);
    native_events_register_strategic_view_generator(L,job);native_events_prefix_full(L,job);
    native_events_register_ai_started_generator(L,job);native_events_prefix_full(L,job);
    native_events_register_ai_ended_generator(L,job);native_events_prefix_full(L,job);
    native_events_register_ai_step_generator(L,job);native_events_prefix_full(L,job);
    native_events_register_game_view_generator(L,job);native_events_prefix_full(L,job);
    native_app_init_current(L);native_camera_view_current(L);native_camera_projection_current(L);native_animation_sampling_current(L);native_dont_record_commands_current(L);native_record_commands_current(L);native_city_handle_current(L);native_unit_handle_current(L);native_unit_type_current(L);native_wonder_type_current(L);native_wonder_state_current(L);native_wonder_placement_current(L);native_display_units_current(L);native_unit_scale_current(L);native_landmark_swap_current(L);native_unit_swap_current(L);native_building_swap_current(L);native_sequence_init_current(L);native_state_dump_current(L);native_state_request_current(L);native_strategic_view_current(L);native_ai_started_current(L);native_ai_ended_current(L);native_ai_step_current(L);native_game_view_current(L);
    // Real79AA90 then46B010: all232 genuine constructors are bound in the
    // retail order. Constructors remain lazy; nothing is emitted here.
    native_ordered_prepare_registration(L,job);native_events_prefix_full(L,job);
    native_ordered_register_group(L,job,0x79AA90u,0,186);native_events_prefix_full(L,job);
    native_ordered_register_group(L,job,0x46B010u,186,232);native_events_prefix_full(L,job);
    // Actual42AA50 is a real synchronous weak-list LuaEvent constructor.
    native_events_register_lua_event_generator(L,job);native_events_prefix_full(L,job);
    native_events_publish_factory_and_selected(L,job);native_events_prefix_full(L,job);
    // All four registrars/547460/4637C0 returned after actual publication.
    // This is Events data only; LuaEvents/API/Controls are mandatory next.

}
